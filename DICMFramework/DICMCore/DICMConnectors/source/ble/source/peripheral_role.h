/*! \file peripheral_role.h
    \brief Bluetooth Low Energy peripheral role
*/
#ifndef PERIPHERAL_ROLE_H
#define PERIPHERAL_ROLE_H

#include "ddm2.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "nimble/ble.h"
#include "nvs.h"
#include "sorted_list.h"
#include <stdbool.h>
#include <stdint.h>

#define INVALID_CONNECTION_HANDLE 0xffff

//! \~ Maximum number of BLE peripheral connections
#ifndef BLE_PERIPHERAL_MAX_CONNECTIONS
#define BLE_PERIPHERAL_MAX_CONNECTIONS 2
#endif

/* Define if not defined advertising interval when a device was paired previously to DICM */
#ifndef BT0ADVINTVL_DEFAULT
#define BT0ADVINTVL_DEFAULT 2000
#endif

/* Define if not defined advertising interval when no device were paired previously to DICM */
/* Initially advertisements are enabled if it is not defined in configuration */
#ifndef BT0ADVINTVL_INITIAL
#define BT0ADVINTVL_INITIAL BT0ADVINTVL_DEFAULT
#endif

#ifndef CONNECTOR_BLE_PERIPHERAL_NOTIFICATION_QUEUE_LENGTH
#define CONNECTOR_BLE_PERIPHERAL_NOTIFICATION_QUEUE_LENGTH 256
#endif

#define PERIPHERAL_ADVERTISE_DURATION_MS      (BT0ADVINTVL_DEFAULT * 5)                                                                     // \~ Advertisement duration in ms
#define PERIPHERAL_NOTIFY_OVERFLOW_RETRY_MS   20                                                                                            //!< \~ GATT notification overflow retry interval in ms
#define PERIPHERAL_NOTIFY_OVERFLOW_QUEUE_SIZE (CONNECTOR_BLE_PERIPHERAL_NOTIFICATION_QUEUE_LENGTH * sizeof(buffered_notification_frame_t))  //!< \~ Size of GATT notification overflow queue in bytes
#define PERIPHERAL_AGGREGATION_TIMEOUT_MS     250                                                                                           //!< \~ Peripheral aggregation timeout in ms
#define PERIPHERAL_AGGREGATION_BUFFER_SIZE    256                                                                                           //!< \~ Maximum size of the aggregation buffer, and also maximum size of a single frame
#define BUFFERED_NOTIFICATION_FRAME_DATA_SIZE 255                                                                                           //!< \~ Maximum data size in buffered notification frame
#define PERIPHERAL_PRINT_INTERVAL_MS          500                                                                                           //!< \~ Interval for printing notification queue status in ms

typedef enum send_notification_result_enum_t
{
    SEND_NOTIFICATION_RESULT_SUCCESS = 0,
    SEND_NOTIFICATION_RESULT_MBUF_ALLOCATION_FAILED = 1,
    SEND_NOTIFICATION_RESULT_NOTIFICATION_FAILED = 2,
    SEND_NOTIFICATION_RESULT_INVALID_PARAMETER = 3,
    SEND_NOTIFICATION_RESULT_QUEUE_FULL = 4,
} send_notification_result_enum_t;

typedef struct buffered_notification_frame_t
{
    uint8_t length;                                       //!< \~ Length of frame inside aggregated data
    uint8_t data[BUFFERED_NOTIFICATION_FRAME_DATA_SIZE];  //!< \~ Frame data
} PACKED buffered_notification_frame_t;

typedef struct peripheral_connection_state_t
{
    uint16_t connection_handle;                                                         //!< \~ Connection handle for this peripheral connection
} peripheral_connection_state_t;

typedef struct aggregated_ddmp2_frame_t
{
    uint8_t length;  //!< \~ Length of frame inside aggregated data
    uint8_t data[];  //!< \~ Frame data
} PACKED aggregated_ddmp2_frame_t;

typedef uint8_t *(*get_data_buffer_function_t)(size_t *const);
typedef void (*ble_peripheral_connect_cb_t)(bool connected, struct ble_gap_conn_desc *p_desc, int connection_slot);
typedef bool (*ble_peripheral_security_check_cb_t)(void);

extern int32_t bt0advintvl;
extern int32_t bt0clients;
extern nvs_handle_t nvs_ble;  // connector_ble.c

int connector_ble_peripheral_connector_id(const int peripheral_connection_slot);
int connector_ble_peripheral_connection_slot(const int connector_slot);
int send_gatt(const DDMP2_FRAME *const pframe, const int connection_slot);
void gatt_server_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_server_initialize(void);
void peripheral_advertise_stop(void);
void peripheral_advertise(const int start);
int peripheral_send_gatt_publish(const DDMP2_FRAME *pframe);
int peripheral_send_gatt_broadcast(const void *data, const uint16_t data_size);
bool ble_peripheral_get_bonded_flag(void);
void ble_peripheral_handle_clients(const DDMP2_FRAME *const pframe);
void ble_peripheral_disconnect_all(void);
int ble_peripheral_handle_frame(const DDMP2_FRAME *const pframe, const int connection_slot);
int ble_peripheral_connection_slot_lookup(const uint16_t connection_handle, const int remove);
void ble_peripheral_initialize(void);
void btc_class_unregister(void);
void ble_nimble_stack_start_init(void);
void ble_nimble_stack_stop_deinit(void);

extern peripheral_connection_state_t peripheral_connection_state[BLE_PERIPHERAL_MAX_CONNECTIONS];

#endif  // PERIPHERAL_ROLE_H
