/*
 * @file ble_state_machine.c
 *
 *  Created on: 8 mars 2024
 *      Author: Andlun
 */

#include "ble_state_machine.h"
#include "configuration.h"
#include "connector.h"
#include "ddm2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "fsm.h"
#include "peripheral_role.h"
#include <stdint.h>

#if defined(BLE_STATE_MACHINE_LOG_ENABLE)
#define BLE_SM_LOG_STATE(event)        LOG(D, "%s", STR(#event))
#define BLE_SM_LOG(level, format, ...) LOG(level, format, __VA_ARGS__)
#else
#define BLE_SM_LOG_STATE(event)        ((void)0)
#define BLE_SM_LOG(level, format, ...) ((void)0)
#endif

#define BLE_SEND_AGG_TIMEOUT_MS        1000
#define BLE_KEEP_ALIVE_TIMEOUT_MS      8000
#define BLE_INIT_SEND_DELAY_TIMEOUT_MS 1500

// Internal timer types
typedef enum
{
    BLE_SM_TIMER_AGGREGATION_TIMEOUT0,
    BLE_SM_TIMER_AGGREGATION_TIMEOUT1,
    BLE_SM_TIMER_AGGREGATION_TIMEOUT2,
    BLE_SM_TIMER_AGGREGATION_TIMEOUT3,
    BLE_SM_TIMER_AGGREGATION_TIMEOUT4,
    BLE_SM_TIMER_AGGREGATION_TIMEOUT5,
    BLE_SM_TIMER_KEEPALIVE_TIMEOUT0,
    BLE_SM_TIMER_KEEPALIVE_TIMEOUT1,
    BLE_SM_TIMER_KEEPALIVE_TIMEOUT2,
    BLE_SM_TIMER_KEEPALIVE_TIMEOUT3,
    BLE_SM_TIMER_KEEPALIVE_TIMEOUT4,
    BLE_SM_TIMER_KEEPALIVE_TIMEOUT5,
    BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT0,
    BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT1,
    BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT2,
    BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT3,
    BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT4,
    BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT5,
    BLE_SM_TIMER_DEFAULT_TIMEOUT,
} ble_sm_timer_type_t;

// State machine active object
typedef struct
{
    fsm_t l_fsm;
    int connector_id;
    bool is_ble_enable;
    TimerHandle_t timer; /**< Internal timer handle */
} ble_sm_t;

// Timer callback
static void ble_state_machine_timer_callback(TimerHandle_t xTimer);

// Local state functions
static void init_state(ble_sm_t *const p_fsm, fsm_event_t const *const p_event);
static void idle_state(ble_sm_t *const p_fsm, fsm_event_t const *const p_event);
static void disabled_state(ble_sm_t *const p_fsm, fsm_event_t const *const p_event);
static EXT_RAM_ATTR ble_sm_t l_ble_sm;

/*! \brief Initialize the BLE state machine
    \param connector_id The ID of the BLE connector service
*/
void init_ble_state_machine(int connector_id)
{
    l_ble_sm.connector_id = connector_id;
    // Create the timer, 2 s and pass ourself as the ID to be used in callback function
    l_ble_sm.timer = xTimerCreate(NULL, pdMS_TO_TICKS(2000), pdFALSE, (void *)BLE_SM_TIMER_DEFAULT_TIMEOUT, ble_state_machine_timer_callback);

    fsm_initialize(&l_ble_sm.l_fsm, FSM_STATE_HANDLER(init_state));

    l_ble_sm.is_ble_enable = true;
}

/*! \brief Dispatch an event to the BLE state machine
    \param p_event Pointer to the event to dispatch
    \return 1 on success, or 0 if the event is not handled
*/
int ble_state_machine_dispatch_event(void *p_event)
{
    // This must be a generic event
    DDMP2_FRAME *p_frame = (DDMP2_FRAME *)p_event;

    TRUE_CHECK_RETURN0(p_frame->frame.control == DDMP2_CONTROL_GENERIC);
    TRUE_CHECK_RETURN0(p_frame->frame.generic.id == BLE_STATE_MACHINE_EVENT_GENERIC_PARAMETER);

    ble_sm_generic_event_with_arg_t *p_ble_sm_event = (ble_sm_generic_event_with_arg_t *)p_frame->frame.generic.data;
    fsm_event_t event =
        {
            .id = p_ble_sm_event->id,
            .p_data = NULL,
        };

    switch (event.id)
    {
    case BLE_SM_ADV_RESTART_EVENT:
    case BLE_SM_CONNECT_TO_PERIPHERAL_DONE_EVENT:  // These events contains the whitelist index
    case BLE_SM_CONNECT_TO_PERIPHERAL_EVENT:
    case BLE_SM_CONNECT_TO_PERIPHERAL_SERVICE_DISCOVERY_EVENT:
    case BLE_SM_BUFFER_DATA_EVENT:
    case BLE_SM_AGGREGATION_TIMEOUT_EVENT:
    case BLE_SM_KEEPALIVE_TIMEOUT_EVENT:
    case BLE_SM_BLE_WRITE_COMPLETE_EVENT:
    case BLE_SM_INCOMING_CREDITS_EVENT:
    case BLE_SM_NETWORK_EVENT:            // This event contains the NET+MST data (ble_sm_network_event_data_t)
    case BLE_SM_INCOMING_MPS_DATA_EVENT:  // This event contains 8 bytes of N-BUS data
    case BLE_SM_QUEUE_DELAYED_EVENT:
        event.p_data = p_ble_sm_event;
        break;
    }
    // Dispatch to state machine
    fsm_state_dispatch(&l_ble_sm.l_fsm, &event);

    return 1;
}

/*! \brief Generate an event for the BLE state machine; send a generic frame to the BLE connector ring buffer.
    \param event_id The ID of the event to generate
    \param argument The argument to pass with the event
*/
void ble_state_machine_generate_event(const ble_sm_event_ids_t event_id, const int32_t argument)
{
    const ble_sm_generic_event_with_arg_t Event =
        {
            .id = event_id,
            .data = (void *)argument,
        };

    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, BLE_STATE_MACHINE_EVENT_GENERIC_PARAMETER, &Event, 8, l_ble_sm.connector_id, portMAX_DELAY);
}

/*! \brief Timer callback for the BLE state machine; generate various timeout events based on the timer type
    \param xTimer The timer that expired
*/
static void ble_state_machine_timer_callback(TimerHandle_t xTimer)
{
    ble_sm_timer_type_t timer_type = (ble_sm_timer_type_t)pvTimerGetTimerID(xTimer);
    ble_sm_event_ids_t event_id = BLE_SM_TIMEOUT_EVENT;
    int32_t data = 0;

    switch (timer_type)
    {
    case BLE_SM_TIMER_AGGREGATION_TIMEOUT0:
    case BLE_SM_TIMER_AGGREGATION_TIMEOUT1:
    case BLE_SM_TIMER_AGGREGATION_TIMEOUT2:
    case BLE_SM_TIMER_AGGREGATION_TIMEOUT3:
    case BLE_SM_TIMER_AGGREGATION_TIMEOUT4:
    case BLE_SM_TIMER_AGGREGATION_TIMEOUT5:
        event_id = BLE_SM_AGGREGATION_TIMEOUT_EVENT;
        data = (int32_t)timer_type;
        break;

    case BLE_SM_TIMER_KEEPALIVE_TIMEOUT0:
    case BLE_SM_TIMER_KEEPALIVE_TIMEOUT1:
    case BLE_SM_TIMER_KEEPALIVE_TIMEOUT2:
    case BLE_SM_TIMER_KEEPALIVE_TIMEOUT3:
    case BLE_SM_TIMER_KEEPALIVE_TIMEOUT4:
    case BLE_SM_TIMER_KEEPALIVE_TIMEOUT5:
        event_id = BLE_SM_KEEPALIVE_TIMEOUT_EVENT;
        data = (int32_t)timer_type;
        break;

    case BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT0:
    case BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT1:
    case BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT2:
    case BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT3:
    case BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT4:
    case BLE_SM_TIMER_QUEUE_DELAY_TIMEOUT5:
        event_id = BLE_SM_QUEUE_DELAYED_EVENT;
        data = (int32_t)timer_type;
        break;

    case BLE_SM_TIMER_DEFAULT_TIMEOUT:
    default:
        break;
    }

    ble_state_machine_generate_event(event_id, data);
}

/*! \brief Initialize the BLE state machine
    \param p_fsm Pointer to the BLE state machine instance
    \param p_event Pointer to the event that triggered the state initialization
*/
static void init_state(ble_sm_t *const p_fsm, fsm_event_t const *const p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
        BLE_SM_LOG(D, "%s", "FSM_ENTRY_EVENT");
        // Initial handling
        break;

    case FSM_EXIT_EVENT:
        BLE_SM_LOG(D, "%s", "FSM_EXIT_EVENT");
        break;

    case BLE_SM_STACK_SYNC_DONE_EVENT:
        BLE_SM_LOG_STATE(BLE_SM_STACK_SYNC_DONE_EVENT);
        fsm_state_change((fsm_t *)p_fsm, FSM_STATE_HANDLER(idle_state));
        break;

    default:
        break;
    }
}

/*! \brief Handle the idle state of the BLE state machine
    \param p_fsm Pointer to the BLE state machine instance
    \param p_event Pointer to the event that triggered the state handling
*/
static void idle_state(ble_sm_t *const p_fsm, fsm_event_t const *const p_event)
{
    const ble_sm_generic_event_with_arg_t *Generic_event = p_event->p_data;

    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
        BLE_SM_LOG(D, "%s", "FSM_ENTRY_EVENT");
        if (p_fsm->is_ble_enable == true)
        {
            // Start advertising
            LOG(D, "peripheral_advertise(1)");
            peripheral_advertise(1);
            // Start re-connect timer
            xTimerStart(p_fsm->timer, portMAX_DELAY);
        }
        break;

    case FSM_EXIT_EVENT:
        BLE_SM_LOG(D, "%s", "FSM_EXIT_EVENT");
        // Stop timer
        xTimerStop(p_fsm->timer, portMAX_DELAY);
        if (p_fsm->is_ble_enable == true)
        {
            // Stop advertising
            peripheral_advertise_stop();
        }
        break;

    case BLE_SM_ADV_RESTART_EVENT:
        BLE_SM_LOG_STATE(BLE_SM_ADV_RESTART_EVENT);
        if (p_fsm->is_ble_enable == true)
        {
            // Restart advertising
            if (Generic_event->argument)
            {
                peripheral_advertise_stop();
            }
            peripheral_advertise(Generic_event->argument);
        }
        break;

    case BLE_SM_DISABLE_CONNECTIONS_EVENT:
        BLE_SM_LOG_STATE(BLE_SM_DISABLE_CONNECTIONS_EVENT);
        // Calls FSM_EXIT_EVENT prior to changing state so advertising is stopped
        fsm_state_change((fsm_t *)p_fsm, FSM_STATE_HANDLER(disabled_state));
        break;

    default:
        break;
    }
}

/*! \brief Handle the disabled state of the BLE state machine
    \param p_fsm Pointer to the BLE state machine instance
    \param p_event Pointer to the event that triggered the state handling
*/
static void disabled_state(ble_sm_t *const p_fsm, fsm_event_t const *const p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
        BLE_SM_LOG_STATE(FSM_ENTRY_EVENT);
        ble_peripheral_disconnect_all();
        break;

    case FSM_EXIT_EVENT:
        BLE_SM_LOG_STATE(FSM_EXIT_EVENT);
        break;

    case BLE_SM_ENABLE_CONNECTIONS_EVENT:
        BLE_SM_LOG_STATE(BLE_SM_ENABLE_CONNECTIONS_EVENT);
        fsm_state_change((fsm_t *)p_fsm, FSM_STATE_HANDLER(idle_state));
        break;

    default:
        LOG(W, "Ignored event id: %d", p_event->id);
        break;
    }
}

/**
 * @brief Disable ble by deinitializing the nimble stack
 */
void disable_ble(void)
{
    l_ble_sm.is_ble_enable = false;
    fsm_state_change((fsm_t *)(&l_ble_sm), FSM_STATE_HANDLER(idle_state));
    ble_nimble_stack_stop_deinit();
}

/**
 * @brief Enable ble by initializing the nimble stack
 */
void enable_ble(void)
{
    ble_nimble_stack_start_init();
    fsm_state_change((fsm_t *)(&l_ble_sm), FSM_STATE_HANDLER(idle_state));
    l_ble_sm.is_ble_enable = true;
}
