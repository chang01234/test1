/*****************************************************************************
 * \file       connector_mqtt.c
 * \brief      MQTT connector
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/

#include "configuration.h"

#include "broker.h"
#include "connector_mqtt.h"
#include "ddm2_parameter_list.h"
#include "freertos/FreeRTOS.h"
#include "sorted_list.h"

#include "iot_client.h"
#include "mqtt_db.h"

#include <stdbool.h>
#include <string.h>

#ifndef CONNECTOR_MQTT_TASK_STACK_SIZE
#define CONNECTOR_MQTT_TASK_STACK_SIZE (3072)
#endif

DECLARE_SORTED_LIST_EXTRAM_PTR(sub_table, SUBSCRIPTION_DEPTH);  //!< \~ Subscription table storage (unique key)

static EXT_RAM_ATTR iot_client_t iot_client_inst;

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
#ifndef MIC_TEST_ENVIRONMENT
const char HostAddress[] = "a21anmqwekbvg-ats.iot.eu-west-1.amazonaws.com";
#else
const char HostAddress[] = "a20p950kjrqyr5-ats.iot.eu-west-1.amazonaws.com";
#endif
COMPILE_TIME_ASSERT(sizeof(gw0connurl) >= sizeof(HostAddress));

static void start_subscription(iot_client_t *iot_client, uint32_t ddm_parameter);
static void inventory_handler_cb(void *argument, uint32_t class_and_instance, bool available);
static void handle_subscribe(iot_client_t *iot_client, DDMP2_FRAME *pframe);
static void handle_set(iot_client_t *iot_client, DDMP2_FRAME *pframe);
static void connector_mqtt_task(void *iot_client_ctx);
static int connector_mqtt_initialize(void);

static void start_subscription(iot_client_t *iot_client, uint32_t ddm_parameter)
{
    int index;
    uint32_t parameter = DDM2_PARAMETER_CLASS(ddm_parameter);
    uint32_t base_instance = DDM2_PARAMETER_BASE_INSTANCE(parameter);
    CONNECTOR *connector = iot_client->connector;

    while ((index = ddm2_parameter_list_lookup(base_instance)) != -1)
    {
        if (Ddm2_parameter_list_data[index].cloud && (Ddm2_parameter_list_data[index].out_type != DDM2_TYPE_NONE))
        {
            LOG(D, "Subscribe to 0x%x", parameter | DDM2_PARAMETER_INSTANCE_PART(ddm_parameter));
            TRUE_CHECK(
                connector_send_frame_to_broker(
                    DDMP2_CONTROL_SUBSCRIBE,
                    parameter | DDM2_PARAMETER_INSTANCE_PART(ddm_parameter),
                    NULL, 0, connector->connector_id, portMAX_DELAY));
        }

        parameter++;
        base_instance = DDM2_PARAMETER_BASE_INSTANCE(parameter);
    }
}

static void inventory_handler_cb(void *argument, uint32_t class_and_instance, bool available)
{
    iot_client_t *iot_client = (iot_client_t *)argument;

    if (available)
    {
        start_subscription(iot_client, class_and_instance);
    }
    else
    {
        sorted_list_unique_remove(iot_client->subscr_table, class_and_instance);
        mqtt_db_instance_deleted(class_and_instance);
    }
}

static void handle_subscribe(iot_client_t *iot_client, DDMP2_FRAME *pframe)
{
    uint32_t instance = DDM2_PARAMETER_INSTANCE(iot_client->instance);
    const uint32_t parameter = pframe->frame.subscribe.parameter;
    CONNECTOR *connector = iot_client->connector;

    if (parameter == (MQTT0STAT | instance))
    {
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MQTT0STAT | instance,
                                       &iot_client->mqtt_status, sizeof(iot_client->mqtt_status),
                                       connector->connector_id, portMAX_DELAY);
    }
    else if (parameter == (MQTT0TXKB | instance))
    {
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MQTT0TXKB | instance,
                                       &iot_client->txkb_sum, sizeof(iot_client->txkb_sum),
                                       connector->connector_id, portMAX_DELAY);
    }
    else if (parameter == (MQTT0PROVTMPLT | instance))
    {
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MQTT0PROVTMPLT | instance,
                                       iot_client->provtmplt, strlen((const char *)iot_client->provtmplt),
                                       connector->connector_id, portMAX_DELAY);
    }
    else if (parameter == (MQTT0CONNECT | instance))
    {
        if (xSemaphoreTake(iot_client->connect_mutex, portMAX_DELAY) == pdTRUE)
        {
            int32_t connect = iot_client->connect;
            xSemaphoreGive(iot_client->connect_mutex);

            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MQTT0CONNECT | instance,
                                           &connect, sizeof(connect),
                                           connector->connector_id, portMAX_DELAY);
        }
    }
}

static void handle_set(iot_client_t *iot_client, DDMP2_FRAME *pframe)
{
    const uint32_t instance = DDM2_PARAMETER_INSTANCE(iot_client->instance);
    const uint32_t parameter = pframe->frame.set.parameter;
    CONNECTOR *connector = iot_client->connector;

    if (parameter == (MQTT0UPDATE | instance))
    {
        iot_client_cloud_update(pframe->frame.set.value.int32);
    }
    else if (parameter == (MQTT0PROVTMPLT | instance))
    {
        int value_size = (int)MIN(ddmp2_value_size(pframe), sizeof(iot_client->provtmplt) - 1);

        memcpy(iot_client->provtmplt, pframe->frame.set.value.raw, value_size);
        iot_client->provtmplt[value_size] = '\0';

        LOG(D, "Got provisioning template: '%s'", (char *)iot_client->provtmplt);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, parameter,
                                       iot_client->provtmplt, value_size,
                                       connector->connector_id,
                                       portMAX_DELAY);
    }
    else if (parameter == (MQTT0CONNECT | instance))
    {
        bool connect = pframe->frame.set.value.int32 != 0;

        LOG(D, "Got MQTT0CONNECT set: %d", connect);

        if (xSemaphoreTake(iot_client->connect_mutex, portMAX_DELAY) == pdTRUE)
        {
            iot_client->connect = connect;
            xSemaphoreGive(iot_client->connect_mutex);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, parameter,
                                           &pframe->frame.set.value.int32,
                                           sizeof(pframe->frame.set.value.int32),
                                           connector->connector_id, portMAX_DELAY);
        }
    }
}

static void connector_mqtt_task(void *iot_client_ctx)
{
    iot_client_t *iot_client = (iot_client_t *)iot_client_ctx;
    CONNECTOR *connector = iot_client->connector;
    DDMP2_FRAME *pframe;
    size_t frame_size;

    LOG(I, "Loading cloud parameters...");
    for (int i = 0; i < DDM2_PARAMETER_COUNT; i++)
    {
        if (Ddm2_parameter_list_data[i].cloud)
        {
            uint32_t parameter = Ddm2_parameter_list_data[i].parameter;
            uint32_t class_and_instance = DDMP2_INVENTORY_CLASS_INSTANCE(parameter);

            TRUE_CHECK(inventory_handler_add_any(
                           &iot_client->inv_handler,
                           class_and_instance) == 0);
        }
    }

    for (;;)
    {
        TRUE_CHECK(pframe = xRingbufferReceive(connector->to_connector, &frame_size, portMAX_DELAY));

        if ((pframe == NULL) || (frame_size == 0))
        {
            continue;
        }

        if (!inventory_handler_update(&iot_client->inv_handler, pframe))
        {
            switch (pframe->frame.control)
            {
            case DDMP2_CONTROL_PUBLISH:
                if (iot_client->all_shadows_fetched)
                {
                    mqtt_db_update_value(pframe);
                }
                break;
            case DDMP2_CONTROL_SUBSCRIBE:
                handle_subscribe(iot_client, pframe);
                break;
            case DDMP2_CONTROL_SET:
                handle_set(iot_client, pframe);
                break;
            default:
                LOG(E, "IOT Client received UNHANDLED frame %02x from broker!", pframe->frame.control);
            }
        }
        vRingbufferReturnItem(connector->to_connector, pframe);
    }
}

static int connector_mqtt_initialize(void)
{
    BaseType_t error = 0;
    uint32_t mqtt0 = MQTT0;
    iot_client_t *iot_client = &iot_client_inst;
    CONNECTOR *connector = &connector_mqtt;

    memset(iot_client, 0, sizeof(*iot_client));

    // Explicit allocation in external memory
    INIT_SORTED_LIST_EXTRAM_PTR(sub_table);

    // For backwards compatibility
    if (strlen((const char *)gw0connurl) == 0)
    {
        // Select hostname
        strcpy((char *)gw0connurl, HostAddress);
    }

    // Set pointer values after allocation done
    iot_client->device_cert = (char *)gw0awscc;
    iot_client->device_prv_key = (char *)gw0awsccpk;
    iot_client->root_ca = (char *)gw0awsca;
    iot_client->thing_name = (char *)gw0thing;

    iot_client->subscr_table = &sub_table;
    iot_client->connector = connector;

    iot_client->connect = true;
    iot_client->connect_mutex = xSemaphoreCreateMutex();
    TRUE_CHECK_RETURN1(iot_client->connect_mutex != NULL);

    TRUE_CHECK_RETURN0((iot_client->instance = broker_register_instance(&mqtt0, connector->connector_id)) != -1);

    // Initialize MQTT database
    mqtt_db_init();

    LOG(I, "MQTT connector initialized with instance %d", iot_client->instance);

    TRUE_CHECK(inventory_handler_init(
                   &iot_client->inv_handler,
                   iot_client->subscr_table,
                   inventory_handler_cb,
                   iot_client) == 0);

    error = xTaskCreate(
        connector_mqtt_task,
        "connector_mqtt_task",
        CONNECTOR_MQTT_TASK_STACK_SIZE,
        iot_client,
        xTASK_PRIORITY_NORMAL,
        NULL);
    TRUE_CHECK(error == pdPASS);

    iot_client_initialize(iot_client);

    return 1;
}

CONNECTOR connector_mqtt = {
    .name = "MQTT connector",
    .initialize = connector_mqtt_initialize,
};

int connector_mqtt_notification_publish(const char *notification, size_t notification_len)
{
    // Publish the notification to the MQTT broker
    int retval = iot_client_notification_publish(notification, notification_len);

    return retval;
}

const char *connector_mqtt_get_thing(void)
{
    const char *thing_name = iot_client_get_thing();
    return thing_name;
}

const char *iot_client_get_thing(void)
{
    // Return the thing_name from the global iot_client_inst
    return iot_client_inst.thing_name;
}
