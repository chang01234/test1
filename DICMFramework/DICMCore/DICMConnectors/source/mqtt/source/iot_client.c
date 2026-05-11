/*
 * iot_client.c
 *
 *  Created on: 11 Sep 2019
 *      Author: Hardik.Shah
 */
#include "configuration.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iot_client.h"
#include "mqtt_db.h"

#include "connector_mqtt.h"
#include "ddm2_parameter_list.h"
#include "hal_cpu.h"
#include "hal_mem.h"

#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/timers.h"

#ifndef CONNECTOR_MQTT_IOT_CLIENT_TASK_STACK_SIZE
#define CONNECTOR_MQTT_IOT_CLIENT_TASK_STACK_SIZE (6144)
#endif

#define MQTT_NOTIFICATION_TOPIC      "/notifications"
#define MQTT_NOTIFICATION_QUEUE_SIZE 10

typedef struct mqtt_notification_t
{
    char *payload;
    size_t payload_size;
} mqtt_notification_t;

typedef struct _iot_cloud_config_t
{
    const char *fw_build_id;
    const int thing;
    const int ntwth;
} iot_cloud_config;

static iot_cloud_config iot_cloud_configs[] = {
#ifndef MIC_TEST_ENVIRONMENT
    {.fw_build_id = "SHE", .thing = 112, .ntwth = 113},
    {.fw_build_id = "ROT", .thing = 114, .ntwth = 115},
#else
    // MTC - Marine project
    {.fw_build_id = "MTC", .thing = 38, .ntwth = 39},
    // SHARC - Heater
    {.fw_build_id = "SHC", .thing = 98, .ntwth = 155},
    // LMC - LMC/ERIBA Caravans
    {.fw_build_id = "LMC", .thing = 49, .ntwth = 13},
    // AMTH - After Market Thermostat
    {.fw_build_id = "AMTH", .thing = 46, .ntwth = 47},
#if 0
        { .fw_build_id = "AMB", .thing = 123, .ntwth = 155},  // AMB - After Market Bridge
#endif
    // APO - Apollo Minibar
    {.fw_build_id = "APO", .thing = 44, .ntwth = 45},
    // SHE - Shape Climate Control AC
    {.fw_build_id = "SHE", .thing = 58, .ntwth = 61},
    // SDB -  Smart Delivery Box
    {.fw_build_id = "SDB", .thing = 40, .ntwth = 41},
    // ROT -  Rotary controller
    {.fw_build_id = "ROT", .thing = 62, .ntwth = 63},
#endif
    {.fw_build_id = NULL, .thing = 0, .ntwth = 0}};

// LEDs hooks (target specific)
#ifdef CONFIG_DICM_TARGET_MARINE_GW_1
#define IOT_LED_R(x) \
    {                \
    }
#define IOT_LED_G(x) \
    {                \
    }
#else
#define IOT_LED_R(x)        \
    {                       \
        LED_R((x) ? 1 : 0); \
    }  // Todo: invert param logic (0=off instead of on)
#define IOT_LED_G(x)        \
    {                       \
        LED_G((x) ? 1 : 0); \
    }
#endif

#define MQTT_CHECK_UPDATED_PARAMETERS_SHORT_TIME (5000 / 10)          // ms/tick = 5 seconds
#define MQTT_CHECK_UPDATED_PARAMETERS_LONG_TIME  (60000 * 60 * 2)     // ms = 120 minutes
#define MQTT_STARTUP_TIME                        ((60000 * 10) / 10)  // 10 minutes, this must be choosen to be longer than what 49 MQTT GET takes

#define MQTT_MAX_TOPIC_LENGTH (CONFIG_AWS_IOT_SHADOW_MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME + THING_NAME_MAX_SIZE + NETWORK_THING_NAME_MAX_SIZE)

#define NBR_OF_GET_TOPICS (3)

#define MIN_DELAY_MS 1000    // 1 second initial delay
#define MAX_DELAY_MS 300000  // 5 minutes maximum delay

#define MQTT_CONNECT_TIMEOUT_MS 10000  // 10 seconds

typedef struct _mqtt_subscription_t
{
    int sub;
    const char *topic;
    pApplicationHandler_t cbfunc;
} mqtt_subscription_t;

typedef struct _mqtt_topic_t
{
    char network_thing_name[NETWORK_THING_NAME_MAX_SIZE];
    char *ptopic[2];
} mqtt_topic_t;

const char UPDATE_DELTA[] = "/update/delta";
const char GET_ACCEPTED[] = "/get/accepted";
const char GET_REJECTED[] = "/get/rejected";
const char AWS_THINGS[] = "$aws/things/";
const char UPDATE[] = "/update";
const char GET[] = "/get";
const char SHADOW[] = "/shadow";
const char NAMED_SHADOW[] = "/shadow/name";
const char EMPTY_OBJECT[] = "{}";
const char LAST_WILL_TESTAMENT[] = "{\"state\":{\"reported\":{\"acxn\":{\"connection_status\":0}}}}";

const char CLAIMING[] = "claiming";
const char CREATE_KEYS_AND_CERTIFICATE_PUB[] = "$aws/certificates/create/json";
const char CREATE_KEYS_AND_CERTIFICATE_SUB_ACCEPTED[] = "$aws/certificates/create/json/accepted";
const char CREATE_KEYS_AND_CERTIFICATE_SUB_REJECTED[] = "$aws/certificates/create/json/rejected";
#define AWS_PROVISIONING_TEMPLATE_MAX_SIZE 100
#define AWS_PROVISIONING_TEMPLATE          "$aws/provisioning-templates/"
#define AWS_PROVISIONING_PAYLOAD_ACCEPTED  "/provision/json/accepted"
#define AWS_PROVISIONING_PAYLOAD_REJECTED  "/provision/json/rejected"
#define AWS_PROVISIONING_PAYLOAD           "/provision/json"

EXT_RAM_ATTR uint32_t thing_first_pub_ok[MQTT_MAX_SUBSCRIPTIONS_PER_SESSION];
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");

#define INCOMING_QUEUE_DEPTH (20)

static uint32_t force_update = 1;
static int nbr_of_subscriptions_cnt = 0;  // Numbers of subscriptions started, including root
static bool is_mqtt_connected = false;

static EXT_RAM_ATTR char get_topics[NBR_OF_GET_TOPICS][MQTT_MAX_TOPIC_LENGTH];

static QueueHandle_t mqtt_notification_queue = NULL;
static EXT_RAM_ATTR StaticQueue_t mqtt_notification_queue_struct;
static EXT_RAM_ATTR uint8_t mqtt_notification_queue_storage[MQTT_NOTIFICATION_QUEUE_SIZE * sizeof(mqtt_notification_t)];

static int verify_cloud_settings(void);
static void mqtt_state_change(iot_client_t *iot_client, uint32_t status, int set);
static int str_replace(char *in, const char *search, const char *replace);
static void handle_set(iot_client_t *iot_client, uint32_t ddm_parameter, int32_t i32Value);
static void handle_set_string(iot_client_t *iot_client, uint32_t ddm_parameter, const char *str);
static void handle_set_binary(iot_client_t *iot_client, uint32_t ddm_parameter, const char *str);
static void update_txkb(iot_client_t *iot_client, int32_t value);
static void add_root_thing(iot_client_t *iot_client);
static int add_new_network_thing(iot_client_t *iot_client, const char *ntw_thing_name, int next_sub);
static int parse_json(const char *json, int thing);
static int parse_json_update_delta(iot_client_t *iot_client, const char *json);
static void aws_create_key_and_certificate_accepted(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params, void *ctx);
static void aws_create_key_and_certificate_rejected(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params, void *ctx);
static void aws_get_rejected_cb(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
static void aws_provisioning_template_accepted(AWS_IoT_Client *pClient, char *pTopicName, uint16_t topicNameLen, IoT_Publish_Message_Params *pParams, void *ctx);
static void aws_provisioning_template_rejected(AWS_IoT_Client *pClient, char *pTopicName, uint16_t topicNameLen, IoT_Publish_Message_Params *pParams, void *ctx);
static void aws_update_delta_cb(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
static void aws_get_accepted_cb(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data);
static IoT_Error_t aws_connect(iot_client_t *iot_client, AWS_IoT_Client *pClient);
static IoT_Error_t aws_disconnect(iot_client_t *iot_client);
static int get_next_updated_subscription(int start_subscription, uint8_t subscriptions_updated[]);
static int iot_client_get_cloud_structure(iot_client_t *iot_client, AWS_IoT_Client *pclient);
static void wait_for_internet(void);
static void iot_client_task(void *iot_client_ctx);

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
const uint16_t port = 8883;

#define UPDATE_INDEX 1
static const mqtt_subscription_t mqtt_sub_pub[] = {
    {.sub = 1, .topic = UPDATE_DELTA, .cbfunc = aws_update_delta_cb},
    {.sub = 0, .topic = UPDATE, .cbfunc = NULL}};
static const int size_of_mqtt_sub_pub = sizeof(mqtt_sub_pub) / sizeof(mqtt_sub_pub[0]);

static mqtt_topic_t *mqtt_topics;

static const mqtt_subscription_t mqtt_sub_get[] = {
    {.sub = 1, .topic = GET_ACCEPTED, .cbfunc = aws_get_accepted_cb},
    {.sub = 1, .topic = GET_REJECTED, .cbfunc = aws_get_rejected_cb},
    {.sub = 0, .topic = GET, .cbfunc = NULL}};
static const int size_of_mqtt_sub_get = sizeof(mqtt_sub_get) / sizeof(mqtt_sub_get[0]);

/*
 * verify_cloud_settings:
 *
 * A runtime verification of the Cloud settings, to check that they are valid
 * for the configured FIRMWARE_BUILD_ID.
 * This does not mean that the settings matches what actually already is
 * stored in the Cloud. That is currently not checked.
 *
 * NOTE: This function must be updated when a new FIRMWARE_BUILD_ID is
 * created in the HUB.
 */
static int verify_cloud_settings(void)
{
#if defined(THING_TYPE_ID) && defined(NTW_THING_TYPE_ID)
    iot_cloud_config *cfg = &iot_cloud_configs[0];

    do
    {
        if ((!strcmp(FIRMWARE_BUILD_ID, cfg->fw_build_id) &&
             (THING_TYPE_ID == cfg->thing) && (cfg0ntwth == cfg->ntwth)) ||
            // This Firmware will probably take the shadow of another
            // firmwares certificates. Always return 1.
            !strcmp(FIRMWARE_BUILD_ID, "DISP"))
        {
            return 1;
        }

        cfg++;
    } while (cfg->fw_build_id != NULL);

    // Ok, this firmware is not defined for checking yet so we can not
    // verify if it is correct. If ending up here we log and inform
    // that no check was possible. Always return 1.
    LOG(W, "No check of Cloud configuration was possible.");
    return 1;
#else
    return 1;
#endif  // defined (THING_TYPE_ID) && defined (NTW_THING_TYPE_ID)
    return 0;
}

static void mqtt_state_change(iot_client_t *iot_client, uint32_t status, int set)
{
    DDMP2_FRAME frame;
    uint32_t previous_status = iot_client->mqtt_status;
    CONNECTOR *connector = iot_client->connector;

    if (set)
    {
        iot_client->mqtt_status |= status;
    }
    else
    {
        iot_client->mqtt_status &= ~status;
    }

    if (previous_status != iot_client->mqtt_status)
    {
        uint32_t instance = DDM2_PARAMETER_INSTANCE(iot_client->instance);
        ddmp2_create_publish(&frame, MQTT0STAT | instance,
                             &iot_client->mqtt_status, sizeof(iot_client->mqtt_status),
                             connector->connector_id);
        TRUE_CHECK(connector_forward_frame_to_broker(&frame));
        if (iot_client->all_shadows_fetched)
        {
            mqtt_db_update_value(&frame);
        }
    }
}

// Works only for replacing two strings of the same length
static int str_replace(char *in, const char *search, const char *replace)
{
    char *where;
    int result = 0;

    if (strlen(search) == strlen(replace))
    {
        if ((where = strstr(in, search)) != NULL)
        {
            strncpy(where, replace, strlen(replace));
            result = 1;
        }
    }

    return result;
}

static void handle_set(iot_client_t *iot_client, uint32_t ddm_parameter, int32_t i32Value)
{
    CONNECTOR *connector = iot_client->connector;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, ddm_parameter,
                                   &i32Value, sizeof(i32Value),
                                   connector->connector_id,
                                   (TickType_t)portMAX_DELAY);
}

static void handle_set_string(iot_client_t *iot_client, uint32_t ddm_parameter, const char *str)
{
    CONNECTOR *connector = iot_client->connector;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, ddm_parameter,
                                   str, strlen(str),
                                   connector->connector_id,
                                   (TickType_t)portMAX_DELAY);
}

static void handle_set_binary(iot_client_t *iot_client, uint32_t ddm_parameter, const char *str)
{
    CONNECTOR *connector = iot_client->connector;
    char data[DDMP2_MAX_VALUE_SIZE];
    size_t str_len = strlen(str);
    uint8_t size = (uint8_t)(str_len / 2);

    // Check if string length is even (required for hex pairs)
    if ((str_len % 2) != 0)
    {
        LOG(E, "Invalid hex string length %zu - must be even", str_len);
        return;
    }

    // Ensure we don't exceed the buffer size
    if (size > DDMP2_MAX_VALUE_SIZE)
    {
        LOG(E, "Binary data size %d exceeds maximum %d", size, DDMP2_MAX_VALUE_SIZE);
        return;
    }

    // Convert hex string to binary
    for (int i = 0; i < size; i++)
    {
        char hex_byte[3] = {str[i * 2], str[i * 2 + 1], '\0'};
        data[i] = (uint8_t)strtoul(hex_byte, NULL, 16);
    }

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, ddm_parameter,
                                   data, size,
                                   connector->connector_id,
                                   (TickType_t)portMAX_DELAY);
}

static void update_txkb(iot_client_t *iot_client, int32_t value)
{
    DDMP2_FRAME frame;
    CONNECTOR *connector = iot_client->connector;

    if (iot_client->txkb_sum != value)
    {
        uint32_t instance = DDM2_PARAMETER_INSTANCE(iot_client->instance);

        // Store last published_value
        iot_client->txkb_sum = value;
        ddmp2_create_publish(&frame, MQTT0TXKB | instance,
                             &iot_client->txkb_sum, sizeof(iot_client->txkb_sum),
                             connector->connector_id);
        if (iot_client->all_shadows_fetched)
        {
            mqtt_db_update_value(&frame);
        }
    }
}

static void add_root_thing(iot_client_t *iot_client)
{
    // Loop through all subscription/publish topics
    for (int j = 0; j < size_of_mqtt_sub_pub; j++)
    {
        char *topic;
        size_t size = strlen(AWS_THINGS) + strlen((char *)iot_client->thing_name) + strlen(SHADOW) + strlen(mqtt_sub_pub[j].topic) + 1;

        TRUE_CHECK(topic = hal_mem_malloc_prefer(size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
        topic[0] = '\0';

        strcat(topic, AWS_THINGS);
        strcat(topic, (char *)iot_client->thing_name);
        strcat(topic, SHADOW);
        strcat(topic, mqtt_sub_pub[j].topic);

        mqtt_topics[0].ptopic[j] = topic;
        LOG(D, "Topic=%s", mqtt_topics[nbr_of_subscriptions_cnt].ptopic[j]);
    }

    // Update counter
    nbr_of_subscriptions_cnt++;
}
static int add_new_network_thing(iot_client_t *iot_client, const char *ntw_thing_name, int next_sub)
{
    if (nbr_of_subscriptions_cnt < MAX_NBR_OF_SUBSCRIPTIONS)
    {
        LOG(D, "Adding topics to name=%s for sub=%d", ntw_thing_name, nbr_of_subscriptions_cnt);
        // Loop through all subscription/publish topics
        for (int j = 0; j < size_of_mqtt_sub_pub; j++)
        {
            char *topic;
            size_t size = strlen(AWS_THINGS) + strlen((char *)iot_client->thing_name) + strlen(NAMED_SHADOW) + strlen(mqtt_sub_pub[j].topic) + strlen(ntw_thing_name) + 1;

            TRUE_CHECK(topic = hal_mem_malloc_prefer(size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
            topic[0] = '\0';

            strcat(topic, AWS_THINGS);
            strcat(topic, (char *)iot_client->thing_name);
            strcat(topic, NAMED_SHADOW);
            strcat(topic, ntw_thing_name);
            strcat(topic, mqtt_sub_pub[j].topic);

            mqtt_topics[next_sub].ptopic[j] = topic;
            LOG(D, "Topic=%s", mqtt_topics[nbr_of_subscriptions_cnt].ptopic[j]);
        }

        // Update counter
        nbr_of_subscriptions_cnt++;

        return 1;
    }

    return 0;
}

static int parse_json(const char *json, int thing)
{
    cJSON *root;
    cJSON *reportedObj = NULL;
    cJSON *stateObj = NULL;
    cJSON *tmp;

    root = cJSON_Parse(json);
    if (root == NULL)  // Something went wrong in parsing
    {
        LOG(E, "Failed to parse JSON response: [%s]", (char *)json);
        return 0;
    }

    // Search for string object
    if ((stateObj = cJSON_GetObjectItemCaseSensitive(root, "state")) != NULL)
    {
        // Search for string object, start with reported
        if (((reportedObj = cJSON_GetObjectItemCaseSensitive(stateObj, "reported")) != NULL) && (reportedObj->child != NULL))
        {
            tmp = reportedObj->child;
            for (int i = 0; (i == 0) || (tmp != NULL); i++, tmp = tmp->next)
            {
                if (!strcmp(tmp->string, "acxn"))
                {
                    // Ignore connection status
                    continue;
                }

                if (mqtt_db_add_entry_from_get(tmp->string, thing) == 0)
                {
                    LOG(W, "Could not add name with str=%s", tmp->string);
                }
                else
                {
                    LOG(D, "Added cloudname[%d]=%s", i, tmp->string);
                }
            }

            // Parsing is done, move to database
            mqtt_move_entries_to_db();
        }
    }

    cJSON_Delete(root);
    return 1;
}

static int parse_json_update_delta(iot_client_t *iot_client, const char *json)
{
    cJSON *root;
    cJSON *stateObj = NULL;
    cJSON *tmp;
    uint32_t ddm_par;
    int res = 0;
    int index;
    bool found_param = false;

    root = cJSON_Parse(json);
    if (root == NULL)  // Something went wrong in parsing
    {
        LOG(E, "Failed to parse JSON response: [%s]", (char *)json);
        return 0;
    }

    // Search for string object
    if ((stateObj = cJSON_GetObjectItemCaseSensitive(root, "state")) != NULL)
    {
        // Search for string object
        tmp = stateObj->child;
        for (int i = 0; (i == 0) || (tmp != NULL); i++, tmp = tmp->next)
        {
            int cloud_set = -1;
            if (cJSON_IsString(tmp) || cJSON_IsNumber(tmp))
            {
                cloud_set = 1;
            }
            else if (cJSON_IsNull(tmp))
            {
                cloud_set = 0;
            }
            found_param = false;
            if (mqtt_db_get_root_cloud_ddm_parameter(tmp->string, &ddm_par, cloud_set, &index))
            {
                found_param = true;
            }
            else if (mqtt_db_get_cloud_ddm_parameter(tmp->string, &ddm_par, cloud_set, &index))
            {
                found_param = true;
            }

            if (found_param)
            {
                if (cJSON_IsString(tmp))
                {
                    int index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(ddm_par));
                    const DDM2_PARAMETER_LIST_DATA *param_data = &Ddm2_parameter_list_data[index];

                    if (param_data->in_type == DDM2_TYPE_STRING)
                    {
                        handle_set_string(iot_client, ddm_par, tmp->valuestring);
                    }
                    else
                    {
                        handle_set_binary(iot_client, ddm_par, tmp->valuestring);
                    }

                    LOG(D, "Cloud SET for 0x%x=%s", ddm_par, tmp->valuestring);
                    res = 1;
                }
                else if (cJSON_IsNumber(tmp))
                {
                    // Set integer value into system
                    handle_set(iot_client, ddm_par, tmp->valueint);
                    LOG(D, "Cloud SET for 0x%x=%d", ddm_par, tmp->valueint);
                    res = 1;
                }
                else if (cJSON_IsNull(tmp))
                {
                    // Ignore
                    LOG(D, "desired=null");
                }
            }
        }
    }

    cJSON_Delete(root);
    return res;
}

// POC: Thing and certificate registration
static void aws_create_key_and_certificate_accepted(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params, void *ctx)
{
    cJSON *root;
    cJSON *jsonObj = NULL;
    iot_client_t *iot_client = (iot_client_t *)ctx;

    // Parse json response
    //{
    //    "certificateId": "string",
    //    "certificatePem": "string",
    //    "privateKey": "string",
    //    "certificateOwnershipToken": "string"
    //}
    root = cJSON_Parse((char *)params->payload);
    if (root == NULL)  // Something went wrong in parsing
    {
        LOG(E, "%s: Failed to parse JSON response: [%s]", topic, (char *)params->payload);
        return;
    }

    // Accepted received
    // Search for string object
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "certificateId")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(D, "certificateId: %s", jsonObj->valuestring);
            iot_client->cert_id = malloc(strlen(jsonObj->valuestring) + 1);
            strcpy(iot_client->cert_id, jsonObj->valuestring);
        }
    }
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "certificatePem")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(D, "certificatePem: %s", jsonObj->valuestring);
            snprintf(iot_client->device_cert, DDMP2_JUMBO_FRAME_MAX_SIZE, "%s", jsonObj->valuestring);
        }
    }
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "privateKey")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(D, "privateKey: %s", jsonObj->valuestring);
            snprintf(iot_client->device_prv_key, DDMP2_JUMBO_FRAME_MAX_SIZE, "%s", jsonObj->valuestring);
        }
    }
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "certificateOwnershipToken")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(D, "certificateOwnershipToken: %s", jsonObj->valuestring);
            iot_client->cert_ownership_token = malloc(strlen(jsonObj->valuestring) + 1);
            strcpy(iot_client->cert_ownership_token, jsonObj->valuestring);
        }
    }
    iot_client->cert_done = true;

    cJSON_Delete(root);
}

static void aws_create_key_and_certificate_rejected(AWS_IoT_Client *client, char *topic, uint16_t topic_len, IoT_Publish_Message_Params *params, void *ctx)
{
    cJSON *root;
    cJSON *jsonObj = NULL;

    // Parse json response
    //{
    //    "certificateId": "string",
    //    "certificatePem": "string",
    //    "privateKey": "string",
    //    "certificateOwnershipToken": "string"
    //}
    root = cJSON_Parse((char *)params->payload);
    if (root == NULL)  // Something went wrong in parsing
    {
        LOG(E, "%s: Failed to parse JSON response: [%s]", topic, (char *)params->payload);
        return;
    }

    // Rejected response
    // Parse json error response
    //{
    //    "statusCode": int,
    //    "errorCode": "string",
    //    "errorMessage": "string"
    //}
    LOG(E, "Failed to register thing");
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "statusCode")) != NULL)
    {
        if (cJSON_IsNumber(jsonObj))
        {
            LOG(E, "statusCode: %d", jsonObj->valueint);
        }
    }
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "errorCode")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(E, "errorCode: %s", jsonObj->valuestring);
        }
    }
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "errorMessage")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(E, "errorMessage: %s", jsonObj->valuestring);
        }
    }

    cJSON_Delete(root);
}

static void aws_provisioning_template_accepted(AWS_IoT_Client *pClient, char *pTopicName, uint16_t topicNameLen, IoT_Publish_Message_Params *pParams, void *ctx)
{
    cJSON *root;
    cJSON *jsonObj = NULL;
    iot_client_t *iot_client = (iot_client_t *)ctx;
    CONNECTOR *connector = iot_client->connector;

    root = cJSON_Parse((char *)pParams->payload);

    if (root == NULL)  // Something went wrong in parsing
    {
        LOG(E, "%s: Failed to parse JSON response: [%s]", pTopicName, (char *)pParams->payload);
        return;
    }

    // Accepted response
    // Parse json response
    //{
    //    "deviceConfiguration": {
    //        "string": "string",
    //        ...
    //    },
    //    "thingName": "string"
    //}
    // Search for string object
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "thingName")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(C, "%s: thingName: %s", pTopicName, jsonObj->valuestring);
            snprintf(iot_client->thing_name, ONBOARDING_STRING_SIZE, "%s", jsonObj->valuestring);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, GW0THING, iot_client->thing_name,
                                           strlen(iot_client->thing_name),
                                           connector->connector_id, (TickType_t)portMAX_DELAY);
            iot_client->claim_done = true;
        }
    }
    else
    {
        LOG(E, "Could not find \"thingName\" in response!");
    }

    cJSON_Delete(root);
}

static void aws_provisioning_template_rejected(AWS_IoT_Client *pClient, char *pTopicName, uint16_t topicNameLen, IoT_Publish_Message_Params *pParams, void *ctx)
{
    cJSON *root;
    cJSON *jsonObj = NULL;

    root = cJSON_Parse((char *)pParams->payload);

    if (root == NULL)  // Something went wrong in parsing
    {
        LOG(E, "%s: Failed to parse JSON response: [%s]", pTopicName, (char *)pParams->payload);
        return;
    }

    LOG(C, "payload: (%s)", (char *)cJSON_Print(root));

    // Rejected response
    // Parse json error response
    //{
    //    "statusCode": int,
    //    "errorCode": "string",
    //    "errorMessage": "string"
    //}
    LOG(E, "Failed to register thing");
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "statusCode")) != NULL)
    {
        if (cJSON_IsNumber(jsonObj))
        {
            LOG(E, "statusCode: %d", jsonObj->valueint);
        }
    }
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "errorCode")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(E, "errorCode: %s", jsonObj->valuestring);
        }
    }
    if ((jsonObj = cJSON_GetObjectItemCaseSensitive(root, "errorMessage")) != NULL)
    {
        if (cJSON_IsString(jsonObj))
        {
            LOG(E, "errorMessage: %s", jsonObj->valuestring);
        }
    }

    cJSON_Delete(root);
}

static void aws_update_delta_cb(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData)
{
    iot_client_t *iot_client = (iot_client_t *)pData;

    LOG(D, "AWS update delta callback");
    LOG(D, "%.*s\t%.*s", topicNameLen, topicName, (int)params->payloadLen, (char *)params->payload);

    parse_json_update_delta(iot_client, (char *)params->payload);
}

static void aws_get_accepted_cb(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData)
{
    int result = 0;
    char expected_network_name[NETWORK_THING_NAME_MAX_SIZE] = {
        '\0',
    };
    iot_client_t *iot_client = (iot_client_t *)pData;

    LOG(I, "AWS Get accepted callback topiclen=%d", topicNameLen);
    LOG(I, "%.*s\t%.*s", topicNameLen, topicName, (int)params->payloadLen, (char *)params->payload);

    snprintf(expected_network_name, sizeof(expected_network_name), "__%d", iot_client->nbr_of_get_accepted);

    if (strstr(topicName, expected_network_name) != NULL)
    {
        if (parse_json((char *)params->payload, iot_client->nbr_of_get_accepted) != 1)
        {
            LOG(E, "Failed to parse JSON file");
        }
        else
        {
            result = 1;
        }
    }

    // We do not need these subscriptions anymore, unsubscribe
    ZERO_CHECK(aws_iot_mqtt_unsubscribe(pClient, topicName, topicNameLen));
    str_replace(topicName, "accepted", "rejected");
    ZERO_CHECK(aws_iot_mqtt_unsubscribe(pClient, topicName, topicNameLen));

    // Do this last
    if (result)
    {
        iot_client->nbr_of_get_accepted++;
    }
}

static void aws_get_rejected_cb(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData)
{
    iot_client_t *iot_client = (iot_client_t *)pData;

    // This is not an error so INFO printout
    LOG(I, "AWS Get rejected callback");
    LOG(I, "%.*s\t%.*s", topicNameLen, topicName, (int)params->payloadLen, (char *)params->payload);

    // We do not need these subscriptions anymore, unsubscribe
    ZERO_CHECK(aws_iot_mqtt_unsubscribe(pClient, topicName, topicNameLen));
    str_replace(topicName, "rejected", "accepted");
    ZERO_CHECK(aws_iot_mqtt_unsubscribe(pClient, topicName, topicNameLen));

    // Check for error 404 = No shadow exist
    if (strstr((char *)params->payload, "404") != NULL)
    {
#if 0
        // Test code to fix error with storing more than needed
        nbr_of_created_network_things--;
        save_to_flash_int32("nbrofthings", nbr_of_created_network_things);
        LOG(W, "Error no shadow: Update flash entry to=%d", nbr_of_created_network_things);
#endif
        iot_client->all_shadows_fetched = true;
    }
}

static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
    if (is_mqtt_connected)
    {
        is_mqtt_connected = false;
        LOG(E, "MQTT Disconnect");
    }

    if (NULL == pClient)
    {
        return;
    }

    if (aws_iot_is_autoreconnect_enabled(pClient))
    {
        LOG(I, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    }
    else
    {
        IoT_Error_t rc = aws_iot_mqtt_attempt_reconnect(pClient);
        LOG(I, "Auto Reconnect not enabled. Starting manual reconnect...");
        if (NETWORK_RECONNECTED == rc)
        {
            LOG(I, "Manual Reconnect Successful");
        }
        else
        {
            LOG(E, "Manual Reconnect Failed - %d", rc);
        }
    }
}

static IoT_Error_t aws_connect(iot_client_t *iot_client, AWS_IoT_Client *pClient)
{
    int needed;
    bool isClaimingCerts = false;
    IoT_Error_t rc = FAILURE;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;
    IoT_MQTT_Will_Options connectMqttWillOptions = iotMqttWillOptionsDefault;
    static EXT_RAM_ATTR char lwt_topic[MQTT_MAX_TOPIC_LENGTH];
    CONNECTOR *connector = iot_client->connector;
    static int sub_started = 0;
    char mac_address[19];

    snprintf(mac_address, sizeof(mac_address), "%02x:%02x:%02x:%02x:%02x:%02x",
             device_information.id[0], device_information.id[1], device_information.id[2],
             device_information.id[3], device_information.id[4], device_information.id[5]);

    LOG(C, "MAC address: %s", mac_address);

    // connectParams.keepAliveIntervalInSec = 10;
    // connectParams.isCleanSession = true;
    // connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    /* POC */
    if (strcmp(CLAIMING, (const char *)iot_client->thing_name) == 0)
    {
        // Claim cert procedure
        connectParams.pClientID = (const char *)mac_address;
        connectParams.clientIDLen = (uint16_t)strlen((char *)mac_address);
        connectParams.isWillMsgPresent = false;
        // clear claiming.
        char reset_thing[5];
        memset(reset_thing, 0, sizeof(reset_thing));
        connector_send_frame_to_broker(DDMP2_CONTROL_SET, GW0THING, reset_thing, sizeof(reset_thing),
                                       connector->connector_id, (TickType_t)portMAX_DELAY);
        isClaimingCerts = true;

        TRUE_CHECK(iot_client->prov_template_accepted = hal_mem_malloc_prefer(AWS_PROVISIONING_TEMPLATE_MAX_SIZE,
                                                                              HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
        TRUE_CHECK(iot_client->prov_template_rejected = hal_mem_malloc_prefer(AWS_PROVISIONING_TEMPLATE_MAX_SIZE,
                                                                              HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
        TRUE_CHECK(iot_client->prov_template_pub = hal_mem_malloc_prefer(AWS_PROVISIONING_TEMPLATE_MAX_SIZE,
                                                                         HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
    }
    else
    {
        connectParams.pClientID = (char *)iot_client->thing_name;
        connectParams.clientIDLen = (uint16_t)strlen((char *)iot_client->thing_name);
        connectParams.isWillMsgPresent = true;
        add_root_thing(iot_client);
    }
    connectMqttWillOptions.pMessage = (char *)LAST_WILL_TESTAMENT;
    connectMqttWillOptions.msgLen = strlen(LAST_WILL_TESTAMENT);
    connectMqttWillOptions.pTopicName = lwt_topic;
    needed = snprintf(lwt_topic, sizeof(lwt_topic),
                      "$aws/things/%s/shadow/update",
                      iot_client->thing_name);
    if ((needed < 0) || (needed >= (int)sizeof(lwt_topic)))
    {
        LOG(E, "LWT topic too long.");
        return FAILURE;
    }
    connectMqttWillOptions.topicNameLen = strlen(lwt_topic);
    connectParams.will = connectMqttWillOptions;

    LOG(D, "LWT topic %s", connectMqttWillOptions.pTopicName);
    LOG(D, "LWT payload %s", connectMqttWillOptions.pMessage);

    LOG(I, "Connecting [%s] to AWS...", connectParams.pClientID);
    do
    {
        static int failures = 0;

        rc = aws_iot_mqtt_connect(pClient, &connectParams);
        if (SUCCESS != rc)
        {
            // Notify connection error every 10th failure via the
            // MQTT0STAT parameter.
            if ((failures % 10) == 0)
            {
                mqtt_state_change(iot_client, MQTT0STAT_CONNECT_FAIL, 1);
            }

            // Calculate exponential backoff delay with a base of 2
            // Cap at MAX_DELAY_MS to avoid extremely long delays
            uint32_t delay_ms = MIN_DELAY_MS;
            if (failures > 1)
            {
                delay_ms = MIN_DELAY_MS * (1 << (failures - 1));             // 2^(failures-1) * MIN_DELAY
                if ((delay_ms > MAX_DELAY_MS) || (delay_ms < MIN_DELAY_MS))  // Check for overflow
                {
                    delay_ms = MAX_DELAY_MS;
                }
            }

            LOG(E, "Error(%d) cnt(%d) connecting to %s:%d, retry in %d ms",
                rc, failures, (char *)gw0connurl, port, delay_ms);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));

            failures++;
        }
        else
        {
            LOG(I, "Connected to %s:%d", (char *)gw0connurl, port);
            failures = 0;
        }
    } while (SUCCESS != rc);

    if (isClaimingCerts)
    {
        IoT_Publish_Message_Params paramsQOS1;
        cJSON *root = cJSON_CreateObject();
        cJSON *parameters = cJSON_CreateObject();

        // Subscribe to CREATE_KEYS_AND_CERTIFICATE_SUB_ACCEPTED
        rc = aws_iot_mqtt_subscribe(pClient, CREATE_KEYS_AND_CERTIFICATE_SUB_ACCEPTED,
                                    strlen(CREATE_KEYS_AND_CERTIFICATE_SUB_ACCEPTED),
                                    QOS1, aws_create_key_and_certificate_accepted, iot_client);
        if (rc != SUCCESS)
        {
            // error
            LOG(E, "Error subscribing to %s: %d ", CREATE_KEYS_AND_CERTIFICATE_SUB_ACCEPTED, rc);
        }
        // Subscribe to CREATE_KEYS_AND_CERTIFICATE_SUB_REJECTED
        rc = aws_iot_mqtt_subscribe(pClient, CREATE_KEYS_AND_CERTIFICATE_SUB_REJECTED,
                                    strlen(CREATE_KEYS_AND_CERTIFICATE_SUB_REJECTED),
                                    QOS1, aws_create_key_and_certificate_rejected, iot_client);
        if (rc != SUCCESS)
        {
            // error
            LOG(E, "Error subscribing to %s: %d ", CREATE_KEYS_AND_CERTIFICATE_SUB_REJECTED, rc);
        }
        // Publish and empty request
        paramsQOS1.payload = (void *)iot_client->payload;
        paramsQOS1.isRetained = 0;
        paramsQOS1.qos = QOS1;
        strcpy(iot_client->payload, EMPTY_OBJECT);
        paramsQOS1.payloadLen = strlen(EMPTY_OBJECT);

        rc = aws_iot_mqtt_publish(pClient, CREATE_KEYS_AND_CERTIFICATE_PUB, strlen(CREATE_KEYS_AND_CERTIFICATE_PUB), &paramsQOS1);
        if (rc != SUCCESS)
        {
            LOG(E, "Error publishing to %s: %d ", CREATE_KEYS_AND_CERTIFICATE_PUB, rc);
        }
        // Need to wait for response
        while (!iot_client->cert_done)
        {
            rc = aws_iot_mqtt_yield(pClient, 100);
            if (rc != SUCCESS)
            {
                LOG(E, "Error yielding: %d ", rc);
            }
        }

        snprintf(iot_client->prov_template_accepted, AWS_PROVISIONING_TEMPLATE_MAX_SIZE, "%s%s%s",
                 AWS_PROVISIONING_TEMPLATE, iot_client->provtmplt, AWS_PROVISIONING_PAYLOAD_ACCEPTED);
        LOG(D, "Subscribe to: %s", iot_client->prov_template_accepted);
        rc = aws_iot_mqtt_subscribe(pClient, iot_client->prov_template_accepted,
                                    strlen(iot_client->prov_template_accepted),
                                    QOS1, aws_provisioning_template_accepted, iot_client);
        if (rc != SUCCESS)
        {
            // error
            LOG(E, "Error subscribing to %s: %d ", iot_client->prov_template_accepted, rc);
        }

        snprintf(iot_client->prov_template_rejected, AWS_PROVISIONING_TEMPLATE_MAX_SIZE, "%s%s%s",
                 AWS_PROVISIONING_TEMPLATE, iot_client->provtmplt, AWS_PROVISIONING_PAYLOAD_REJECTED);
        LOG(D, "Subscribe to: %s", iot_client->prov_template_rejected);
        rc = aws_iot_mqtt_subscribe(pClient, iot_client->prov_template_rejected,
                                    strlen(iot_client->prov_template_rejected),
                                    QOS1, aws_provisioning_template_rejected, iot_client);
        if (rc != SUCCESS)
        {
            // error
            LOG(E, "Error subscribing to %s: %d ", iot_client->prov_template_rejected, rc);
        }

        // RegisterThing request
        //{
        //  "certificateOwnershipToken": "string",
        //  "parameters": {
        //     "serialNumber": "string",
        //     "macAddress": "string",
        //     "fwVersion": "string",
        //     "firmwareId": "string",
        //     "skuId": "string",
        //     "domain": "string"
        //     "AWS::IoT::Certificate::Id": "string"
        //   }
        //}

        cJSON_AddStringToObject(root, "certificateOwnershipToken", iot_client->cert_ownership_token);
        cJSON_AddItemToObject(root, "parameters", parameters);
        cJSON_AddStringToObject(parameters, "serialNumber", (const char *)device_information.id_string);
        cJSON_AddStringToObject(parameters, "macAddress", (const char *)mac_address);
        cJSON_AddStringToObject(parameters, "fwVersion", (const char *)device_information.firmware_version);
        cJSON_AddStringToObject(parameters, "firmwareId", FIRMWARE_BUILD_ID);
        cJSON_AddStringToObject(parameters, "skuId", (const char *)gw0sku);
        cJSON_AddStringToObject(parameters, "domain", "dometic.com");
        cJSON_AddStringToObject(parameters, "AWS::IoT::Certificate::Id", iot_client->cert_id);

        snprintf(iot_client->payload, CONFIG_AWS_IOT_MQTT_TX_BUF_LEN, "%s", cJSON_PrintUnformatted(root));
        paramsQOS1.payloadLen = strlen(iot_client->payload);
        snprintf(iot_client->prov_template_pub, AWS_PROVISIONING_TEMPLATE_MAX_SIZE, "%s%s%s",
                 AWS_PROVISIONING_TEMPLATE, iot_client->provtmplt, AWS_PROVISIONING_PAYLOAD);
        LOG(D, "Publish to: %s", iot_client->prov_template_pub);
        rc = aws_iot_mqtt_publish(pClient, iot_client->prov_template_pub, strlen(iot_client->prov_template_pub), &paramsQOS1);
        if (rc != SUCCESS)
        {
            LOG(E, "Error publishing to %s: %d ", iot_client->prov_template_pub, rc);
        }

        LOG(I, "Waiting for provisioning template response...");

        // Need to wait for response
        while (!iot_client->claim_done)
        {
            rc = aws_iot_mqtt_yield(pClient, 100);
            if (rc != SUCCESS)
            {
                LOG(E, "Error yielding: %d ", rc);
            }
        }

        // We have received all data
        config_set_str("gw0awscc", iot_client->device_cert);
        config_set_str("gw0awsccpk", iot_client->device_prv_key);
        config_set_str("gw0thing", iot_client->thing_name);

        aws_iot_mqtt_disconnect(pClient);

        cJSON_Delete(root);

        hal_mem_free(iot_client->prov_template_accepted);
        hal_mem_free(iot_client->prov_template_rejected);
        hal_mem_free(iot_client->prov_template_pub);

        // Disconnect and reconnect with new certs
        return NETWORK_MANUALLY_DISCONNECTED;
    }

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(pClient, true);
    if (SUCCESS != rc)
    {
        LOG(E, "Unable to set Auto Reconnect to true - %d", rc);
        // abort();
    }

    if (!sub_started)
    {
        LOG(I, "Subscribe to: %s", mqtt_topics[0].ptopic[0]);
        rc = aws_iot_mqtt_subscribe(pClient, mqtt_topics[0].ptopic[0],
                                    strlen(mqtt_topics[0].ptopic[0]), QOS1,
                                    mqtt_sub_pub[0].cbfunc, iot_client);
        LOG(D, "rc = %d, sub topic=%s", rc, mqtt_topics[0].ptopic[0]);

        sub_started = 1;
    }
    else
    {
        LOG(D, "Resubscribe");

        // Subscribe to everything again
        rc = aws_iot_mqtt_resubscribe(pClient);
    }

    if (rc == SUCCESS)
    {
        IOT_LED_R(0);
        IOT_LED_G(0);

        LOG(I, "Subscribing...ok");
    }
    else
    {
        LOG(E, "Error subscribing : %d ", rc);
    }

    return rc;
}

static IoT_Error_t aws_disconnect(iot_client_t *iot_client)
{
    IoT_Error_t rc = SUCCESS;

    // Check if client is valid
    if (!iot_client->pClient)
    {
        return rc;
    }

    if (aws_iot_mqtt_is_client_connected(iot_client->pClient))
    {
        rc = aws_iot_mqtt_disconnect(iot_client->pClient);
        if (rc == SUCCESS)
        {
            LOG(I, "MQTT client disconnected successfully");
            is_mqtt_connected = false;
            mqtt_state_change(iot_client, MQTT0STAT_NOT_CONNECTED, 1);
        }
        else
        {
            LOG(E, "Failed to disconnect MQTT client: %d", rc);
        }
    }

    return rc;
}

static int get_next_updated_subscription(int start_subscription, uint8_t subscriptions_updated[])
{
    for (int i = start_subscription; i < MAX_NBR_OF_SUBSCRIPTIONS; i++)
    {
        if (subscriptions_updated[i])
        {
            return i;
        }
    }

    return MAX_NBR_OF_SUBSCRIPTIONS;  // Indicates no more
}

#if 0
const char * ntwthingsname[] = {
        "",
        "__climate",
        "__mechanical",
        "__power",
        "__tank",
        "__dcload",
        "__dimmer",
        "__scene",
        "__dontexist"};
#endif

/*
 * TODO: Potential problem found.
 * If the code below is called 300 times we should not subscribe again
 * as the subscriptions are present, but we should do a new publish of
 * GET shadow, as it might be gone (?) or have not reached AWS (?).
 */
static int iot_client_get_cloud_structure(iot_client_t *iot_client, AWS_IoT_Client *pclient)
{
    IoT_Publish_Message_Params paramsQOS1;
    IoT_Error_t rc = FAILURE;
    char network_name[NETWORK_THING_NAME_MAX_SIZE];
    network_name[0] = '\0';
    static int thing_cnt = 0;
    static int nbr_of_tries = 0;

    if ((thing_cnt == iot_client->nbr_of_get_accepted) || (nbr_of_tries > 300))
    {
        // Clear
        nbr_of_tries = 0;

        // Use get accepted as this is incremented after each successful GET
        snprintf(network_name, sizeof(network_name), "/__%d", iot_client->nbr_of_get_accepted);

        // Loop through all subscription/publish topics
        for (int j = 0; j < size_of_mqtt_sub_get; j++)
        {
            char *topic;

            get_topics[j][0] = '\0';
            topic = &get_topics[j][0];

            strcpy(topic, AWS_THINGS);
            strcat(topic, (char *)iot_client->thing_name);
            strcat(topic, NAMED_SHADOW);
            strcat(topic, network_name);
            strcat(topic, mqtt_sub_get[j].topic);

            if (mqtt_sub_get[j].sub)
            {
                rc = aws_iot_mqtt_subscribe(pclient, topic, strlen(topic), QOS1, mqtt_sub_get[j].cbfunc, iot_client);
                LOG(I, "rc=%d, Subscribe to %s with len=%d", rc, topic, strlen(topic));
            }
            else
            {
                paramsQOS1.payload = (void *)iot_client->payload;
                paramsQOS1.isRetained = 0;
                paramsQOS1.qos = QOS1;
                strcpy(iot_client->payload, EMPTY_OBJECT);
                paramsQOS1.payloadLen = strlen(EMPTY_OBJECT);

                rc = aws_iot_mqtt_publish(pclient, topic, strlen(topic), &paramsQOS1);
                LOG(I, "rc=%d, Get from %s with len=%d, payload=%s", rc, topic, strlen(topic), iot_client->payload);
            }

            if (rc != SUCCESS)
            {
                break;  // Leave loop
            }
        }

#if 1
        if (rc == SUCCESS)
        {
            // At SUCCESS we increase with one
            ++thing_cnt;
        }
#endif
    }
    else
    {
        // A get can take some time, increase counter and wait
        ++nbr_of_tries;
    }

    return (int)rc;
}

static void wait_for_internet(void)
{
    for (;;)
    {
        int32_t dummy = 0;
        // Wait for 10s
        // Test internet connection
        vTaskDelay(pdMS_TO_TICKS(5000));
        connector_send_frame_to_broker(DDMP2_CONTROL_SET, GW0ICT, &dummy, sizeof(dummy), connector_mqtt.connector_id, (TickType_t)portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
        EventBits_t uxBits = xEventGroupWaitBits(network_events, INTERNET_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(4000));
        if (((EventBits_t)INTERNET_BIT & uxBits) != 0)
        {
            // Internet check ok.
            break;
        }
        else
        {
            // Just wait again
        }
    }
}

/**
 * @brief Publish MQTT notifications from the queue.
 *
 * This function processes the MQTT notification queue and publishes
 * notifications to the notification MQTT topic. It retrieves notifications from the queue,
 * publishes them using the AWS IoT MQTT client, and frees the allocated memory for the payload.
 */
static void publish_notification(iot_client_t *iot_client)
{
    mqtt_notification_t notification;
    while (xQueueReceive(mqtt_notification_queue, &notification, 0) == pdTRUE)
    {
        IoT_Publish_Message_Params params = {
            .qos = QOS1,
            .payload = (void *)notification.payload,
            .payloadLen = notification.payload_size,
            .isRetained = 0};
        IoT_Error_t pub_rc = aws_iot_mqtt_publish(iot_client->pClient, MQTT_NOTIFICATION_TOPIC, strlen(MQTT_NOTIFICATION_TOPIC), &params);
        LOG(I, "Published to %s, rc=%d", MQTT_NOTIFICATION_TOPIC, pub_rc);
        hal_mem_free(notification.payload);
    }
}

static void iot_client_task(void *iot_client_ctx)
{
    iot_client_t *iot_client = (iot_client_t *)iot_client_ctx;
    IoT_Error_t rc = FAILURE;
    static EXT_RAM_ATTR AWS_IoT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Publish_Message_Params paramsQOS1;
    int done = 0;
    bool connect = true;

    // Wait for system startup
    xEventGroupWaitBits(system_events, (SYSTEM_START_BIT), pdFALSE, pdFALSE, portMAX_DELAY);
    LOG(D, "System Ok!");

    LOG(I, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
    mqttInitParams.enableAutoReconnect = false;  // We enable this later below
    mqttInitParams.pHostURL = (char *)gw0connurl;
    mqttInitParams.port = port;

    mqttInitParams.pDeviceCertLocation = iot_client->device_cert;
    mqttInitParams.pDevicePrivateKeyLocation = iot_client->device_prv_key;

    mqttInitParams.tlsHandshakeTimeout_ms = 10000;  // default 5000
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;

    iot_client->pClient = &client;

    if (!verify_cloud_settings())
    {
        LOG(E, "Wrong configuration! No Cloud connection possible.");

        for (;;)
        {
            static uint32_t cnt = 0;
            IOT_LED_R(cnt & 0x1);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

#if defined(CONNECTOR_MQTT_USES_MODEM) && !defined(IOT_MODEM_OLD)
    // Init AWS client
    ZERO_CHECK(aws_iot_mqtt_init(&client, &mqttInitParams));
#endif

    // Check that we have a valid thing and certificates
    do
    {
        // Check that we have all certificates and a thing. If not, wait.
        if ((iot_client->device_cert[0] != '\0') && (iot_client->device_prv_key[0] != '\0') &&
            ((char)iot_client->thing_name[0] != '\0') && ((char)gw0connurl[0] != '\0') && (cfg0ntwth != 0))
        {
            if (!strcmp((char *)iot_client->thing_name, CLAIMING) && (char)iot_client->provtmplt[0] == '\0')
            {
                continue;
            }

            if (iot_client->root_ca[0] == '\0')
            {
                // Use predefined root certificate
                mqttInitParams.pRootCALocation = (char *)aws_root_ca_pem_start;
            }
            else
            {
                // Use root certificate loaded from the outside
                mqttInitParams.pRootCALocation = iot_client->root_ca;
            }

#if !defined(CONNECTOR_MQTT_USES_MODEM) || defined(IOT_MODEM_OLD)
            // Init AWS client
            ZERO_CHECK(aws_iot_mqtt_init(&client, &mqttInitParams));
#endif
            done = 1;
        }
        else
        {
            LOG(D, "Waiting for -> cert=%d key=%d thing=%d connurl=%d ntwth=%d...",
                iot_client->device_cert[0] == '\0', iot_client->device_prv_key[0] == '\0',
                (char)iot_client->thing_name[0] == '\0', (char)gw0connurl[0] == '\0',
                cfg0ntwth == 0);
            // Wait for 10s
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    } while (!done);

    paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *)iot_client->payload;
    paramsQOS1.isRetained = 0;

    for (;;)
    {
        static TickType_t prev_short_tick = 0;
        static TickType_t prev_long_tick = 0;
        TickType_t cur_tick = 0;
        static uint32_t cnt = 0;
        static bool first_time = true;
        int first_publish;
        static int root_thing_publish_ok = 0;
        // EventBits_t bits_set;
        int inventory_installed = 0;
        int cloud_set_done;
        int root_set_done;
        int block;
        int32_t update_time;
        ClientState client_state;
        uint8_t sub_updated[MAX_NBR_OF_SUBSCRIPTIONS] = {0};
        int publish_err_cnt;
        int force_next_update;
        uint32_t mqtt_publish_fail;

        // Check if we should be connected
        if (xSemaphoreTake(iot_client->connect_mutex, portMAX_DELAY) == pdTRUE)
        {
            connect = iot_client->connect;
            xSemaphoreGive(iot_client->connect_mutex);
        }

        if (!connect)
        {
            LOG(D, "MQTT is disconnected. Waiting...");
            vTaskDelay(pdMS_TO_TICKS(MQTT_CONNECT_TIMEOUT_MS));
            continue;
        }

        // Wait for IP
        // It's rather meaningless to proceed without an IP address
        // Note: This code is written with the knowledge that Wi-Fi or Modem is used.
        //       If both are used then another logic needs to be implemented.
        LOG(D, "Wait for IP");
        xEventGroupWaitBits(network_events, (WIFI_IP_BIT | MODEM_IP_BIT), pdFALSE, pdFALSE, portMAX_DELAY);
        LOG(D, "Got IP");
        LOG(D, "Wait for INTERNET");
        wait_for_internet();
        LOG(D, "Got INTERNET");

        IOT_LED_G(1);
        IOT_LED_R(1);
        // LED_B(0);

        rc = aws_iot_mqtt_yield(&client, 100);
        LOG(D, "MQTT Yield: rc=%d", rc);

        if (connect && !first_time && (rc == NETWORK_MANUALLY_DISCONNECTED))
        {
            // Attempt to reconnect when AWS IoT was manually disconnected
            rc = aws_iot_mqtt_attempt_reconnect(&client);
            LOG(D, "MQTT Manual Reconnect: rc=%d", rc);
        }
        else if (connect && !((NETWORK_ATTEMPTING_RECONNECT == rc) || (NETWORK_RECONNECTED == rc) || (SUCCESS == rc)))
        {
            // DEBUG
            client_state = aws_iot_mqtt_get_client_state(&client);
            LOG(D, "MQTT Client State: %d", client_state);

            // end debug

            if (!aws_iot_mqtt_is_client_connected(&client))
            {
                // When state is equal to not connected then we can connect.
                rc = aws_connect(iot_client, &client);

                if ((rc == SUCCESS) && first_time)
                {
                    first_time = false;  // for doing the things below only once

                    // Check if we have any network thing
                    iot_client_get_cloud_structure(iot_client, &client);

                    // Indicate connected
                    mqtt_state_change(iot_client, MQTT0STAT_CONNECTED, 1);
                }
            }
        }

        if (mqtt_db_get_ddm_parameter_last_value(GW0CUPDT, &update_time))
        {
            update_time = MQTT_CHECK_UPDATED_PARAMETERS_LONG_TIME;
        }

        // update_time is in ms and we have 10 ms/tick
        update_time /= 10;  // 10 ms tick

#if 0
        if (!get_shadow_called)
        {
            rc = aws_get_shadow(&client);
            get_shadow_called = 1;
        }
#endif
        while ((NETWORK_ATTEMPTING_RECONNECT == rc) || (NETWORK_RECONNECTED == rc) || (SUCCESS == rc))
        {
            cloud_set_done = 0;
            root_set_done = 0;

            if (xSemaphoreTake(iot_client->connect_mutex, portMAX_DELAY) == pdTRUE)
            {
                bool connect_val = iot_client->connect;
                xSemaphoreGive(iot_client->connect_mutex);

                if (connect && !connect_val)
                {
                    connect = false;
                    rc = aws_disconnect(iot_client);
                    if (rc == SUCCESS)
                    {
                        client_state = aws_iot_mqtt_get_client_state(&client);
                        LOG(I, "Disconnected successfully. Client state: %d", client_state);
                    }
                    else
                    {
                        LOG(E, "Error during disconnect: %d", rc);
                    }

                    break;
                }
            }
            else
            {
                LOG(W, "Failed to get connect mutex!");
            }

            // Max time the yield function will wait for read messages
            rc = aws_iot_mqtt_yield(&client, 100);

#if 0
            if (cnt%64==0)
            {
                LOG(D, "IOT while, rc=%d, force=%d", rc, force_update);
            }
#endif
            cnt++;

            if (NETWORK_ATTEMPTING_RECONNECT == rc)
            {
                IOT_LED_R(cnt & 1);

                mqtt_state_change(iot_client, MQTT0STAT_RECONNECTING, 1);
                mqtt_state_change(iot_client, MQTT0STAT_CONNECTED, 0);

                force_update = 1;
                is_mqtt_connected = false;

                // If the client is attempting to reconnect we will skip the rest of the loop.
                continue;
            }

            // Indicate OK on LEDs
            IOT_LED_R(0);
            IOT_LED_G(0);

            mqtt_state_change(iot_client, MQTT0STAT_RECONNECTING, 0);
            mqtt_state_change(iot_client, MQTT0STAT_CONNECTED, 1);

            is_mqtt_connected = true;

            publish_notification(iot_client);

            if (!iot_client->all_shadows_fetched)
            {
                iot_client_get_cloud_structure(iot_client, &client);
            }
            else
            {
                if (!inventory_installed)
                {
                    // MQTT database is built. Subscribe to inventory
                    TRUE_CHECK(inventory_handler_start(
                                   &iot_client->inv_handler,
                                   iot_client->connector) == 0);
                    inventory_installed = 1;
                }
            }

            // LOG(I, "Stack remaining for task '%s' is %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));

            // TRUE_CHECK(xQueueReceive(connector_mqtt.to_connector, (void *)&ddmp_msg, portMAX_DELAY));
            // vTaskDelay(1000 / portTICK_RATE_MS);

            cur_tick = xTaskGetTickCount();
            // LOG(D, "Ticks=%d,%d", cur_tick - prev_long_tick, cur_tick - prev_short_tick);

            mqtt_publish_fail = 0;

            // Short time checks
            if (((cur_tick - prev_short_tick) > MQTT_CHECK_UPDATED_PARAMETERS_SHORT_TIME) || force_update)
            {
                prev_short_tick = cur_tick;

                // Clear array
                memset(sub_updated, 0, MAX_NBR_OF_SUBSCRIPTIONS);

                if (force_update || (thing_first_pub_ok[0] == 0))
                {
                    // Always update root block(s) on force
                    root_set_done = 1;
                }
                else
                {
                    // Check if Cloud has generated any set or if any updated root parameters
                    root_set_done = mqtt_db_has_root_cloud_set_or_prio(&block);
                }

                if (iot_client->all_shadows_fetched)
                {
                    // Check if Cloud has generated any set or if any updated ntw thing parameters
                    cloud_set_done = mqtt_db_has_cloud_set_or_prio(sub_updated);
                }

                if (cloud_set_done)
                {
                    LOG(D, "Cloud SET done!");
                }

                // If we have published to root thing once then check for publish error
                if (root_thing_publish_ok)
                {
                    // At every short time check, evaluate any mqtt publish error.
                    // If stored value in the array for each thing is 0 then we have a publish error.
                    // int i = 0 means root thing.
                    // mqtt_get_nbr_of_created_blocks returns created network things, therefore <=.
                    for (int i = 0; i <= mqtt_get_nbr_of_created_blocks(); i++)
                    {
                        if (thing_first_pub_ok[i] == 0)
                        {
                            mqtt_publish_fail = 1;
                            break;  // Leave loop
                        }
                    }

                    // Change state according to what is found
                    mqtt_state_change(iot_client, MQTT0STAT_PUBLISH_FAIL, mqtt_publish_fail ? 1 : 0);
                }
            }

            // Long time checks
            // We also wait for subscription started.
            // LOG(I, "time=%d r=%d && s=%d, c=%d, m=%d", cur_tick - prev_long_tick, root_set_done, all_shadows_fetched, cloud_set_done, mqtt_publish_fail);
            // LOG(I, "f=%d && s=%d", force_update, all_shadows_fetched);
            if (iot_client->all_shadows_fetched && (((cur_tick - prev_long_tick) > (TickType_t)update_time) || root_set_done || cloud_set_done || mqtt_publish_fail || force_update))
            {
                int next_sub = 0;  // Next subscription, including root
                int res = 0;
                // const char *pname;
                int topic_name_min_len;

                // LOG(D, "Ticks=%d,%d", cur_tick - prev_long_tick, cur_tick - prev_short_tick);

                LOG(D, "csd=%d || mpf=%d && rsd=%d", cloud_set_done, mqtt_publish_fail, root_set_done);
                if ((cloud_set_done || mqtt_publish_fail) && !root_set_done)
                {
                    next_sub = get_next_updated_subscription(next_sub, sub_updated);
                    LOG(D, "Prepare JSON. Start at subscription=%d", next_sub);
                    if (next_sub >= MAX_NBR_OF_SUBSCRIPTIONS)
                    {
                        // Should not happen and is impossible to handle
                        goto unrecoverable_error;
                    }
                }

                if ((cur_tick - prev_long_tick) > (TickType_t)update_time)
                {
                    prev_long_tick = cur_tick;
                }

                do
                {
                    topic_name_min_len = strlen(AWS_THINGS) + strlen((char *)iot_client->thing_name) + strlen(SHADOW) + strlen(mqtt_sub_pub[UPDATE_INDEX].topic) + 1;
                    first_publish = 0;
                    if (next_sub)
                    {
                        // Now we are expected to act on a network thing. A non zero value means network thing.
                        // It is fully possible that we might end up here when all network things has not been fetched using "GET".
                        // If that is the case we can not continue this loop.
                        // Leave do loop with break.
                        if (!iot_client->all_shadows_fetched)
                        {
                            break;
                        }

                        if (mqtt_topics[next_sub].ptopic[UPDATE_INDEX] == NULL)
                        {
                            TRUE_CHECK(add_new_network_thing(iot_client, (const char *)mqtt_get_networkname(next_sub - 1), next_sub));

                            // Start subscription for update/delta
                            ZERO_CHECK(aws_iot_mqtt_subscribe(&client, mqtt_topics[next_sub].ptopic[0],
                                                              strlen(mqtt_topics[next_sub].ptopic[0]), QOS1,
                                                              mqtt_sub_pub[0].cbfunc, iot_client));
                            LOG(I, "Subscription for %s, started", mqtt_topics[next_sub].ptopic[0]);
                            first_publish = 1;
                        }
                    }
                    iot_client->payload[0] = '\0';
                    // first_publish = (thing_first_pub_ok[next_sub] == 0 ? 1 : 0);
                    LOG(D, "force: %d || %d", force_update, thing_first_pub_ok[next_sub]);

                    if (force_update || (thing_first_pub_ok[next_sub] == 0))
                    {
                        force_next_update = 1;
                    }
                    else
                    {
                        force_next_update = 0;
                    }
                    // TODO: Maybe force_next_update and first_publish could be one parameter
                    // first_publish is only used to add the thing type to the json file.
                    // Maybe we can live with up to 17 bytes more at every force?
                    // Investigate if this is a way forward to simplify the solution.
                    res = mqtt_db_create_json(iot_client->payload, CONFIG_AWS_IOT_MQTT_TX_BUF_LEN, root_set_done, next_sub, topic_name_min_len,
                                              force_next_update, first_publish);
                    LOG(D, "Created json %d", res);
                    publish_err_cnt = 5;

                    while ((res != 0) && (publish_err_cnt > 0))
                    {
                        LOG(I, "String to Cloud = %s", iot_client->payload);
                        paramsQOS1.payloadLen = strlen(iot_client->payload);

                        IOT_LED_G(1);

                        rc = aws_iot_mqtt_publish(&client, mqtt_topics[next_sub].ptopic[UPDATE_INDEX],
                                                  strlen(mqtt_topics[next_sub].ptopic[UPDATE_INDEX]), &paramsQOS1);
                        LOG(D, "Published to topic=%s, rc =%d", mqtt_topics[next_sub].ptopic[UPDATE_INDEX], rc);
                        if (rc == SUCCESS)
                        {
                            static int txcnt = 0;
                            IOT_LED_G(0);
                            publish_err_cnt = 0;
                            txcnt++;
                            thing_first_pub_ok[next_sub] = 1;

                            update_txkb(iot_client, mqtt_db_transmitted_kbytes());

                            // Indicate that we have published to root thing successfully.
                            // It is very important that we always can access the root thing.
                            // Otherwise it can be impossible to reach the GW from the Cloud.
                            if (!root_thing_publish_ok && (next_sub == 0))
                            {
                                root_thing_publish_ok = 1;
                                // This is actually a status indication that we have published ok once to root thing.
                                // Publish failure is another bit in the status event.
                                mqtt_state_change(iot_client, MQTT0STAT_PUBLISH_OK, 1);
                            }
                        }
                        else
                        {
                            int cnt = 0;
                            IoT_Error_t tmp;

                            publish_err_cnt--;
                            IOT_LED_R(1);
                            do
                            {
                                tmp = aws_iot_mqtt_yield(&client, 100);
                                LOG(I, "Yield resp=%d", tmp);
                            } while (++cnt < 5);

                            // Quit if not timeout
                            if (rc != MQTT_REQUEST_TIMEOUT_ERROR)
                            {
                                publish_err_cnt = 0;
                            }
                        }

                        if ((rc == MQTT_REQUEST_TIMEOUT_ERROR) && (publish_err_cnt == 0))
                        {
                            LOG(W, "QOS1 publish ack not received.");
                            rc = SUCCESS;
                            thing_first_pub_ok[next_sub] = 0;  // 0 = Will force update of complete (network) thing data at next publish
                            break;                             // Leave do loop
                        }
                    }

                    // Move to next subscription
                    next_sub++;

                    // Correct thing if this is not a long time update and
                    // if publish to a network thing not is in failure.
                    if (cloud_set_done && (thing_first_pub_ok[next_sub] != 0))
                    {
                        next_sub = get_next_updated_subscription(next_sub, sub_updated);
                    }

                    // Clear root set
                    root_set_done = 0;

                    if ((next_sub >= MAX_NBR_OF_SUBSCRIPTIONS) || (next_sub > mqtt_get_nbr_of_created_blocks()))
                    {
                        // Quit
                        res = 0;
                    }
                    else
                    {
                        // Force to 1 to continue to support "holes" (unsupported things) in the Cloud structure
                        res = 1;
                    }

                } while (res != 0);

            unrecoverable_error:

                // Everything updated once, clear force
                force_update = 0;
            }
        }

        LOG(W, "MQTT rc=%d", rc);

        // Some error occured.
        // If we failed to publish we need to be sure the Cloud is updated to our status.
        // Force update of all parameters next time we are connected.
        force_update = 1;

        if (rc == MQTT_MAX_SUBSCRIPTIONS_REACHED_ERROR)
        {
            // We should not end up here, but if we do we are in trouble.
            LOG(W, "Restarting rc=%d", rc);
            vTaskDelay(pdMS_TO_TICKS(1000));
            hal_cpu_reset(HALCPU_RESET_FLAG_NONE);
        }

        // If we drop out of while loop some error has occured.
        // We need to yield to get AWS process to ping and to be alive.
        rc = aws_iot_mqtt_yield(&client, 100);

        IOT_LED_R(1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int iot_client_initialize(iot_client_t *iot_client)
{
    BaseType_t error = 0;

    LOG(C, "Initialize AWS IoT client: id_string=%s", device_information.id_string);

    TRUE_CHECK_RETURN1((iot_client->payload = hal_mem_malloc_prefer(CONFIG_AWS_IOT_MQTT_TX_BUF_LEN, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
    TRUE_CHECK_RETURN1((mqtt_topics = hal_mem_malloc_prefer(MAX_NBR_OF_SUBSCRIPTIONS * sizeof(*mqtt_topics), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
    memset(mqtt_topics, 0, MAX_NBR_OF_SUBSCRIPTIONS * sizeof(*mqtt_topics));
    memset(thing_first_pub_ok, 0, sizeof(thing_first_pub_ok));

    // Create MQTT notification queue with static storage in external RAM
    mqtt_notification_queue = xQueueCreateStatic(
        MQTT_NOTIFICATION_QUEUE_SIZE,
        sizeof(mqtt_notification_t),
        mqtt_notification_queue_storage,
        &mqtt_notification_queue_struct);
    TRUE_CHECK_RETURN1(mqtt_notification_queue != NULL);

    error = xTaskCreate(
        iot_client_task,
        "iot_client_task",
        CONNECTOR_MQTT_IOT_CLIENT_TASK_STACK_SIZE,
        iot_client,
        xTASK_PRIORITY_BELOW_NORMAL,
        NULL);
    TRUE_CHECK(error == pdPASS);

    return 0;
}

void iot_client_cloud_update(int32_t value)
{
    if (value == 1)
    {
        force_update = 1;
    }
}

int iot_client_notification_publish(const char *notification, size_t notification_len)
{
    int retval = CONNECTOR_MQTT_NOTIFICATION_ERR_NONE;
    mqtt_notification_t mqtt_notification = {
        .payload = (char *)notification,
        .payload_size = notification_len};

    // Increase robustness by ensuring Queue is initialized (when EVM connector is started before MQTT connector)
    if (mqtt_notification_queue != NULL)
    {
        // Allocate memory for the notification
        mqtt_notification.payload = hal_mem_malloc_prefer(notification_len + 1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (mqtt_notification.payload != NULL)
        {
            mqtt_notification.payload_size = notification_len;
            strncpy(mqtt_notification.payload, notification, notification_len);
            mqtt_notification.payload[notification_len] = '\0';  // Ensure null-termination

            if (xQueueSend(mqtt_notification_queue, &mqtt_notification, 0) != pdPASS)
            {
                LOG(W, "Failed to send MQTT notification to queue");
                hal_mem_free(mqtt_notification.payload);
                retval = CONNECTOR_MQTT_NOTIFICATION_ERR_QUEUE_FULL;
            }
        }
        else
        {
            LOG(W, "Failed to allocate memory for MQTT notification payload");
            retval = CONNECTOR_MQTT_NOTIFICATION_ERR_MEM;
        }
    }
    else
    {
        LOG(W, "MQTT notification queue not initialized");
        retval = CONNECTOR_MQTT_NOTIFICATION_ERR_NO_QUEUE;
    }
    return retval;
}
