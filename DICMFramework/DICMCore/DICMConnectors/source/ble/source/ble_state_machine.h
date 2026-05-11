/*
 * @file ble_state_machine.h
 *
 *  Created on: 8 mars 2024
 *      Author: Andlun
 */

#ifndef BLE_STATE_MACHINE_H_
#define BLE_STATE_MACHINE_H_

#include <stdint.h>

#include "fsm.h"

typedef enum ble_sm_event_ids_t
{
    BLE_SM_STACK_SYNC_DONE_EVENT = FSM_USER_EVENT,
    BLE_SM_ADV_START_EVENT,
    BLE_SM_ADV_RESTART_EVENT,
    BLE_SM_ADV_STOP_EVENT,
    BLE_SM_SCAN_START_EVENT,
    BLE_SM_SCAN_COMPLETE_EVENT,
    BLE_SM_CONNECT_TO_PERIPHERAL_EVENT,
    BLE_SM_CONNECT_TO_PERIPHERAL_SERVICE_DISCOVERY_EVENT,
    BLE_SM_CONNECT_TO_PERIPHERAL_DONE_EVENT,
    BLE_SM_TIMEOUT_EVENT,
    BLE_SM_BUFFER_DATA_EVENT,
    BLE_SM_AGGREGATION_TIMEOUT_EVENT,
    BLE_SM_KEEPALIVE_TIMEOUT_EVENT,
    BLE_SM_BLE_WRITE_COMPLETE_EVENT,
    BLE_SM_NETWORK_EVENT,
    BLE_SM_INCOMING_MPS_DATA_EVENT,
    BLE_SM_INCOMING_CREDITS_EVENT,
    BLE_SM_ENABLE_CONNECTIONS_EVENT,
    BLE_SM_DISABLE_CONNECTIONS_EVENT,
    BLE_SM_QUEUE_DELAYED_EVENT,
} ble_sm_event_ids_t;

typedef enum ble_sm_internal_generic_event_t
{
    BLE_STATE_MACHINE_EVENT_GENERIC_PARAMETER = 0  // BLE state machine generic ID
} ble_sm_internal_generic_event_t;

typedef struct ble_sm_generic_event_with_arg_t
{
    uint32_t id;
    union  // argument
    {
        void *data;
        int32_t argument;
        int32_t whitelist_index;
        int32_t timer_id;
    };

} ble_sm_generic_event_with_arg_t;

void init_ble_state_machine(int connector_id);
int ble_state_machine_dispatch_event(void *p_event);
void ble_state_machine_generate_event(const ble_sm_event_ids_t event_id, const int32_t argument);

#endif /* BLE_STATE_MACHINE_H_ */
