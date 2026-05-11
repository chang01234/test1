/**
 * \file        connector_event_notification_defaults.h
 * \date        2024-06-27
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Default compile-time settings for event notification connector.
 *
 * \li          2024-06-27  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef CONNECTOR_EVENT_NOTIFICATION_DEFAULTS_H_
#define CONNECTOR_EVENT_NOTIFICATION_DEFAULTS_H_

#include "configuration.h"

/**
 * @brief       Defines the verbosity level for event notification logging.
 *
 * 0 - Extra logging is turned off, only warnings and errors are logged
 * 1 - Additional information is logged
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_VERBOSE_LOG
#define CONNECTOR_EVENT_NOTIFICATION_VERBOSE_LOG 0
#endif

/**
 * @brief       Sets the maximum number of events that can be stored in event
 *              log.
 *
 * Maximum number of event records to store in memory. The records are stored in
 * FIFO like structure. When FIFO is full, the oldest record is deleted to make
 * room for new records.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS
#define CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS 10
#endif

/**
 * @brief       Define how many different event types are supported
 *
 * Maximum number of event type definitions. The event type definitions define
 * how many different record types the connector can handle.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_TYPES
#define CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_TYPES 20
#endif

/**
 * @brief       Determines the maximum number of DDMs per event for event
 *              notifications.
 *
 * Maximum number of DDM parameter values that a record can store.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_MAX_DATA_IN_EVENT_RECORD
#define CONNECTOR_EVENT_NOTIFICATION_MAX_DATA_IN_EVENT_RECORD 4
#endif

/**
 * @brief       Establishes the maximum number of DDMs for event notification
 *              connector to subscribe to.
 *
 * Calculated as CONNECTOR_EVENT_NOTIFACATION_MAX_EVENT_RECORDS *
 * CONNECTOR_EVENT_NOTIFICATION_MAX_DATA_IN_EVENT_RECORD. This can be reduced in
 * order to lower memory consumption.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_MAX_DDM_TO_SUBSCRIBE
#define CONNECTOR_EVENT_NOTIFICATION_MAX_DDM_TO_SUBSCRIBE \
    (CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS * CONNECTOR_EVENT_NOTIFICATION_MAX_DATA_IN_EVENT_RECORD)
#endif

/**
 * @brief       Sets the maximum number of events that can be stored in event
 *              notification class log.
 *
 * Maximum number of event records to store in memory. The records are stored in
 * FIFO like structure. When FIFO is full, the oldest record is deleted to make
 * room for new records.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_CLOUD_MAX_EVENT_RECORDS
#define CONNECTOR_EVENT_NOTIFICATION_CLOUD_MAX_EVENT_RECORDS 20
#endif

/**
 * @brief       Sets how many milli seconds to wait for an event to be acknowledged.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_CLOUD_WAIT_ACK_MS
#define CONNECTOR_EVENT_NOTIFICATION_CLOUD_WAIT_ACK_MS 40000
#endif

/**
 * @brief       Sets how many milli seconds to wait for a new event to be sent.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_CLOUD_WAIT_SEND_MS
#define CONNECTOR_EVENT_NOTIFICATION_CLOUD_WAIT_SEND_MS 2000
#endif

/**
 * @brief       Defines the length of product name string.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_PRODUCT_NAME_LENGTH
#define CONNECTOR_EVENT_NOTIFICATION_PRODUCT_NAME_LENGTH 32
#endif

/**
 * @brief       Defines the length of version string.
 */
#ifndef CONNECTOR_EVENT_NOTIFICATION_VERSION_LENGTH
#define CONNECTOR_EVENT_NOTIFICATION_VERSION_LENGTH 16
#endif

#endif /* CONNECTOR_EVENT_NOTIFICATION_DEFAULTS_H_ */
