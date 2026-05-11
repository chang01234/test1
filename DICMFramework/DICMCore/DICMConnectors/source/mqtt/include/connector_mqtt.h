/*
 * connector_mqtt.h
 *
 *  Created on: 11 Sep 2019
 *      Author: Hardik.Shah
 */

#ifndef CONNECTOR_MQTT_H_
#define CONNECTOR_MQTT_H_

#include "configuration.h"

#include "connector.h"
#include <stddef.h>

extern CONNECTOR connector_mqtt;

#define CONNECTOR_MQTT_NOTIFICATION_ERR_NONE       0   //!< No error
#define CONNECTOR_MQTT_NOTIFICATION_ERR_QUEUE_FULL -1  //!< Notification queue is full
#define CONNECTOR_MQTT_NOTIFICATION_ERR_MEM        -2  //!< Memory allocation error
#define CONNECTOR_MQTT_NOTIFICATION_ERR_NO_QUEUE   -3  //!< Notification queue not initialized

/**
 * @brief Publish MQTT notifications to the notification topic.
 *
 * @param notification Pointer to the notification string.
 * @param notification_len Length of the notification string.
 *
 * @return CONNECTOR_MQTT_NOTIFICATION_ERR_NONE on success, negative error code on failure.
 * @retval CONNECTOR_MQTT_NOTIFICATION_ERR_QUEUE_FULL if the notification queue is full.
 * @retval CONNECTOR_MQTT_NOTIFICATION_ERR_MEM if there was a memory allocation error.
 * @retval CONNECTOR_MQTT_NOTIFICATION_ERR_NO_QUEUE if the notification queue is not initialized.
 *
 * @pre Parameter @p notification must be a valid pointer to a null-terminated string.
 * @pre Parameter @p notification_len must be the length of the string pointed to by notification.
 */
int connector_mqtt_notification_publish(const char *notification, size_t notification_len);

/**
 * @brief Get the thing name from the MQTT connector.
 *
 * @return Pointer to the thing name string. This pointer is valid as long as the MQTT connector is initialized.
 *         If the thing name is not set, it will return NULL.
 */
const char *connector_mqtt_get_thing(void);

#endif /* CONNECTOR_MQTT_H_ */
