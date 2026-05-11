/*
 * iot_client.h
 *
 *  Created on: 11 Sep 2019
 *      Author: Hardik.Shah
 */
#ifndef MAIN_IOT_CLIENT_H_
#define MAIN_IOT_CLIENT_H_

#include "connector.h"
#include "sorted_list.h"
#include "inventory_handler.h"

#ifndef AWS_IOT_MODEM_WRAPPER
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_version.h"
#else
#include "modem/aws_wrapper/include/aws_iot_config.h"
#include "modem/aws_wrapper/include/aws_iot_log.h"
#include "modem/aws_wrapper/include/aws_iot_mqtt_client_interface.h"
#include "modem/aws_wrapper/include/aws_iot_version.h"
#endif

#include <stddef.h>
#include <stdint.h>

#define AWS_MQTT_PROV_TEMPLATE_MAX_SIZE    40

typedef struct _iot_client_t
{
    char *device_cert;
    char *device_prv_key;
    char *root_ca;
    char *cert_id;
    char *cert_ownership_token;
    bool cert_done;

    char *prov_template_accepted;
    char *prov_template_rejected;
    char *prov_template_pub;

    char provtmplt[AWS_MQTT_PROV_TEMPLATE_MAX_SIZE];
    int32_t txkb_sum;
    bool all_shadows_fetched;
    uint32_t mqtt_status;

    char *thing_name;
    bool claim_done;

    char *payload;

    SORTED_LIST *subscr_table;
    CONNECTOR *connector;
    int instance;
    inventory_handler_t inv_handler;

    int32_t nbr_of_get_accepted;

    AWS_IoT_Client *pClient;

    bool connect;
    SemaphoreHandle_t connect_mutex;
} iot_client_t;

int iot_client_initialize(iot_client_t *iot_client);
void iot_client_cloud_update(int32_t value);

/**
 * @brief Publish MQTT notifications to the notification topic.
 *
 * This function allocates memory for the notification payload,
 * copies the notification string, and sends it to the MQTT notification queue.
 * It ensures that the payload is null-terminated and handles memory allocation errors.
 *
 * @param notification Pointer to the notification string.
 * @param notification_len Length of the notification string.
 * @return CONNECTOR_MQTT_NOTIFICATION_ERR_NONE on success, negative error code on failure.
 * @retval CONNECTOR_MQTT_NOTIFICATION_ERR_QUEUE_FULL if the notification queue is full.
 * @retval CONNECTOR_MQTT_NOTIFICATION_ERR_MEM if there was a memory allocation error.
 * @retval CONNECTOR_MQTT_NOTIFICATION_ERR_NO_QUEUE if the notification queue is not initialized.
 */
int iot_client_notification_publish(const char *notification, size_t notification_len);

/**
 * @brief Get the thing name from the global Iot instance.
 *
 * This function retrieves the thing name from the global IoT structure.
 *
 * @return Pointer to the thing name string. This pointer is valid as long as the IoT client is initialized.
 *         If the thing name is not set, it will return NULL.
 */
const char *iot_client_get_thing(void);

#endif /* MAIN_IOT_CLIENT_H_ */
