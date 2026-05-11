/*****************************************************************************
 * \file       connector_rcs.c
 * \brief      Connector for refrigerator control.
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
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "ble_peripheral.h"
#include "connector_system.h"
#include "ddm2_parameter_list.h"
#include "ddm_wrapper.h"
#include "fsm.h"
#include "network_discovery.h"
#include "utils.h"

/**********************************************************
 * Private defines
 *********************************************************/
// #define RCS_EXTENDED_LOG_ENABLED

#ifdef RCS_EXTENDED_LOG_ENABLED
#define RCS_EXTENDED_LOG(level, format, ...) LOG(level, format, __VA_ARGS__)
#else
#define RCS_EXTENDED_LOG(level, format, ...) ((void)0)
#endif  // RCS_EXTENDED_LOG_ENABLED

#define IS_TEMP_OUTSIDE_OUTER_LIMITS(current_temp) (((current_temp) > (target_temp[temp_alarm_data.tctrl] + temp_alarm_data.high_outer_limit)) || \
                                                    ((current_temp) < (target_temp[temp_alarm_data.tctrl] + temp_alarm_data.low_outer_limit)))

#define IS_TEMP_WITHIN_INNER_LIMITS(current_temp) (((current_temp) < (target_temp[temp_alarm_data.tctrl] + temp_alarm_data.high_inner_limit)) && \
                                                   ((current_temp) > (target_temp[temp_alarm_data.tctrl] + temp_alarm_data.low_inner_limit)))

// if value is 0, fire it right away in lowest possible period
// clang-format off
#define CONVERT_TIMER_PERIOD_SEC_TO_MS(time_sec) ((time_sec) != (int32_t)0 ? (int32_t)((time_sec) * 1000) : (int32_t)pdTICKS_TO_MS(1))
// clang-format on

#define HISTORY_TIMER_PERIOD_MS (1000)

#define BT_FILTER_TIME_MS      400u
#define BT_ENTER_DOOR_STATE_MS 2000u

#define REFRIDGERATOR_COUNT_MAX 1u
#define REFRIDGE_CLASS_NAME     "rcs"
#define REFRIDGE_HISTO_PARAM    "hist"
#define REFRIDGE_HISTO_SIZE     8

/*
     Limit example:        Setting         Actual limit
     high outer limit        1000             8000
     high inner limit         500             7500
     target temp                2 = 7000 = 7 C
     low inner limit         -500             6500
     high inner limit       -1000             6000

*/
/**********************************************************
 * Private types
 *********************************************************/
typedef enum _sm_events_t
{
    // Bluetooth
    SM_EVENTS_BT_AVL = 2u,
    // Minibar
    SM_EVENTS_MB_AVL,
    SM_EVENTS_MB_DOORST,
    SM_EVENTS_MB_TSTAT,
    SM_EVENTS_MB_TCTRL,
    SM_EVENTS_MB_LGTON,
    // MQTT
    SM_EVENTS_MQTT_AVL,
    SM_EVENTS_MQTT_STATUS,
    // Refrigerator Control
    SM_EVENTS_RCS_THIOULIM,
    SM_EVENTS_RCS_THIINLIM,
    SM_EVENTS_RCS_TLOINLIM,
    SM_EVENTS_RCS_TLOOULIM,
    SM_EVENTS_RCS_TALT,
    SM_EVENTS_RCS_TALT2,
    SM_EVENTS_RCS_DALT,
    SM_EVENTS_RCS_ROOM,
    SM_EVENTS_RCS_THH,
    SM_EVENTS_RCS_TDH,
    SM_EVENTS_RCS_TWH,
    SM_EVENTS_RCS_CDFAIL,
    SM_EVENTS_RCS_PWRFAIL,
    // Generic
    SM_EVENTS_BT_FILTER_TIMEOUT,
    SM_EVENTS_BT_ENTER_DOOR_STATE_TIMEOUT,
    SM_EVENTS_DOOR_ALARM_TIMEOUT,
    SM_EVENTS_TEMP_ALARM_TIMEOUT,
    SM_EVENTS_HISTORY_POLL_TIMEOUT,
    SM_EVENTS_TEMP_ALARM_RESET_SM,
} sm_events_t;

typedef struct
{
    uint32_t cnt;
    uint32_t acc;
    uint16_t avg[REFRIDGE_HISTO_SIZE];
} refrigerator_history_t;

typedef struct _rcs_t
{
    struct
    {
        ddmw_item_t avl;               // RCS Available
        ddmw_item_t talrm;             // Temp alarm
        ddmw_item_t talt;              // Temp alarm time
        ddmw_item_t talt2;             // Temp alarm time 2
        ddmw_item_t dalrm;             // Door alarm
        ddmw_item_t dalt;              // Door alarm time
        ddmw_item_t cloud_failure;     // Cloud failure indication
        ddmw_item_t power_failure;     // Power failure indication
        ddmw_item_t high_outer_limit;  // High outer limit, as offset from set temp
        ddmw_item_t high_inner_limit;  // High inner limit, as offset from set temp
        ddmw_item_t low_outer_limit;   // Low outer limit, as offset from set temp
        ddmw_item_t low_inner_limit;   // Low inner limit, as offset from set temp
        ddmw_item_t thh;               // Temp Hour history
        ddmw_item_t tdh;               // Temp Day history
        ddmw_item_t twh;               // Temp Week history
        ddmw_item_t room;              // Room number string
        ddmw_item_t mb_avl;            // Minibar available
        ddmw_item_t mb_doorsta;        // Door status
        ddmw_item_t mb_doorind;        // Door indication
        ddmw_item_t mb_lightsta;       // Light status
        ddmw_item_t mb_tempsta;        // Temperature
        ddmw_item_t mb_tempctrl;       // Temperature control
        ddmw_item_t bt_avl;            // BT available
        ddmw_item_t bt_pair;           // BT pairing
        ddmw_item_t mqtt_avl;          // MQTT available
        ddmw_item_t mqtt_status;       // MQTT status
    } ddm;

    // History
    struct
    {
        struct tm time;
        refrigerator_history_t hour;
        refrigerator_history_t day;
        refrigerator_history_t week;
    } history;
} rcs_t;

typedef struct
{
    // DDM wrapper
    ddmw_t ddm;

    rcs_t item;
} connector_rcs_t;

typedef struct _rcs_sm_t
{
    fsm_t sm;

    TimerHandle_t timer;
} rcs_sm_t;

typedef struct _rcs_temp_alarm_data_t
{
    uint32_t alarm_timer;
    int32_t high_outer_limit;
    int32_t high_inner_limit;
    int32_t low_outer_limit;
    int32_t low_inner_limit;
    int32_t temp_alarm_time_2;
    int32_t temp_alarm_time_in_s;
    int32_t temp_alarm_time_2_in_s;
    int32_t tctrl;
} rcs_temp_alarm_data_t;

/**********************************************************
 * Private functions
 *********************************************************/
static int connector_rcs_init(void);
static void connector_rcs_register(void);
static int connector_rcs_update_internal_variable(uint32_t ddm_par, ddmw_item_t *item, int32_t *int_var);
static void connector_rcs_room_change_actions(void);
static void connector_rcs_task(const DDMP2_FRAME *frame);
static void connector_rcs_history_1s(uint32_t event_id);
static void connector_rcs_history_update_ddm(uint32_t event_id);
static void connector_rcs_history_add(refrigerator_history_t *history, int32_t temperature);
static void connector_rcs_history_shift(refrigerator_history_t *history);
static void connector_rcs_history_load(int instance);
static void connector_rcs_history_save(int instance);
static void connector_rcs_save_updated_parameters(uint32_t event_id);
static uint32_t connector_rcs_get_event_id(const DDMP2_FRAME *frame);
static void connector_rcs_temp_alarm_get_event_id(const fsm_event_t *const input_event, fsm_event_t *const output_event);

static void connector_rcs_bt_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_bt_sm_state_wait_avl(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_bt_sm_state_register_rcs(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_bt_sm_state_wait_filter_period(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_bt_sm_state_get_door_status(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_bt_sm_state_enabled(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_bt_sm_state_disabled(fsm_t *fsm, const fsm_event_t *p_event);

static void connector_rcs_door_alarm_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_door_alarm_sm_state_closed(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_door_alarm_sm_state_open(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_door_alarm_sm_state_alarm_active(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_door_alarm_sm_state_transition(fsm_t *fsm, const fsm_event_t *p_event);

static void connector_rcs_temp_alarm_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_temp_alarm_sm_state_update_variables(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_temp_alarm_sm_state_wait_avl(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_temp_alarm_sm_state_arm_alert(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_temp_alarm_sm_state_outside_interval(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_temp_alarm_sm_state_outside_interval_2(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_temp_alarm_sm_state_raise_alert(fsm_t *fsm, const fsm_event_t *p_event);

static void connector_rcs_mqtt_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_mqtt_sm_state_wait_avl(fsm_t *fsm, const fsm_event_t *p_event);
static void connector_rcs_mqtt_sm_state_status(fsm_t *fsm, const fsm_event_t *p_event);

static void generic_event_timer_callback(TimerHandle_t xTimer);

/**********************************************************
 * Private variables
 *********************************************************/
static const int32_t target_temp[4] = {0, 12000, 7000, 4000};
static EXT_RAM_ATTR connector_rcs_t rcs = {0};
static EXT_RAM_ATTR rcs_temp_alarm_data_t temp_alarm_data;
static EXT_RAM_ATTR TimerHandle_t history_poll_timer;

static EXT_RAM_ATTR rcs_sm_t bt_pairing_sm_handle;
static EXT_RAM_ATTR rcs_sm_t door_alarm_sm_handle;
static EXT_RAM_ATTR rcs_sm_t temp_alarm_sm_handle;
static EXT_RAM_ATTR fsm_t mqtt_sm_handle;

/**********************************************************
 * Public variables
 *********************************************************/
CONNECTOR connector_rcs =
    {
        .name = "Refrigerator control connector",
        .initialize = connector_rcs_init,
        .process_event = connector_rcs_task,
};

/**********************************************************
 * Implementation
 *********************************************************/

//! \brief Handle timers timeouts
static void generic_event_timer_callback(TimerHandle_t xTimer)
{
    const sm_events_t event = (const sm_events_t)pvTimerGetTimerID(xTimer);
    ddmw_send_generic_event_data(&rcs.ddm, event, NULL, 0);
}

/**********************************************************
 * Function:    connector_rcs_init
 * Description: Initialize the connector
 *********************************************************/
static int connector_rcs_init(void)
{
    // DDM
    ddmw_init(&rcs.ddm, &connector_rcs);

    // Load history data
    connector_rcs_history_load(0);

    // Clean
    memset(&bt_pairing_sm_handle, 0, sizeof(bt_pairing_sm_handle));
    memset(&door_alarm_sm_handle, 0, sizeof(door_alarm_sm_handle));
    memset(&temp_alarm_sm_handle, 0, sizeof(temp_alarm_sm_handle));
    memset(&mqtt_sm_handle, 0, sizeof(mqtt_sm_handle));

    // Create timers
    TRUE_CHECK_RETURN0(bt_pairing_sm_handle.timer = xTimerCreate(NULL, UINT32_MAX, pdFALSE, NULL, generic_event_timer_callback));
    TRUE_CHECK_RETURN0(door_alarm_sm_handle.timer = xTimerCreate(NULL, UINT32_MAX, pdFALSE, NULL, generic_event_timer_callback));
    TRUE_CHECK_RETURN0(temp_alarm_sm_handle.timer = xTimerCreate(NULL, UINT32_MAX, pdFALSE, NULL, generic_event_timer_callback));
    TRUE_CHECK_RETURN0(history_poll_timer = xTimerCreate(NULL, pdMS_TO_TICKS(HISTORY_TIMER_PERIOD_MS), pdTRUE, (void *)SM_EVENTS_HISTORY_POLL_TIMEOUT, generic_event_timer_callback));

    // Initialize state machines
    fsm_initialize(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_init));
    fsm_initialize(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_init));
    fsm_initialize(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_init));
    fsm_initialize(&mqtt_sm_handle, FSM_STATE_HANDLER(connector_rcs_mqtt_sm_state_init));

    // Start history timer
    xTimerStart(history_poll_timer, portMAX_DELAY);

    return 1;
}

/**********************************************************
 * Function:    connector_rcs_update_internal_variable
 * Description: Update internal variables
 *********************************************************/
static int connector_rcs_update_internal_variable(uint32_t ddm_par, ddmw_item_t *item, int32_t *int_var)
{
    int index;
    int factor;
    int32_t temp_var;

    // Get index in DDM parameter list
    if ((index = ddm2_parameter_list_lookup(ddm_par)) == -1)
    {
        return 0;
    }

    // Get factor and adapt
    factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].out_unit];
    temp_var = ddmw_get_i32(item);
    temp_var /= factor;

    *int_var = temp_var;
    return 1;
}

/**********************************************************
 * Function:    connector_rcs_register
 * Description: Register DDM parameters
 *********************************************************/
static void connector_rcs_register(void)
{
    // Register new instance to DDM
    uint32_t instance = ddmw_register(&rcs.ddm, RCS0AVL);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.avl, RCS0AVL, instance);

    // Alarms
    ddmw_add(&rcs.ddm, &rcs.item.ddm.talrm, RCS0TALRM, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.dalrm, RCS0DALRM, instance);

    // Limits
    ddmw_add(&rcs.ddm, &rcs.item.ddm.high_outer_limit, RCS0THIOULIM, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.high_inner_limit, RCS0THIINLIM, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.low_inner_limit, RCS0TLOINLIM, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.low_outer_limit, RCS0TLOOULIM, instance);

    // Times
    ddmw_add(&rcs.ddm, &rcs.item.ddm.talt, RCS0TALT, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.talt2, RCS0TALT2, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.dalt, RCS0DALT, instance);

    // History
    ddmw_add(&rcs.ddm, &rcs.item.ddm.thh, RCS0THH, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.tdh, RCS0TDH, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.twh, RCS0TWH, instance);

    // Room string
    ddmw_add(&rcs.ddm, &rcs.item.ddm.room, RCS0ROOM, instance);

    // Failures
    ddmw_add(&rcs.ddm, &rcs.item.ddm.cloud_failure, RCS0CDFAIL, instance);
    ddmw_add(&rcs.ddm, &rcs.item.ddm.power_failure, RCS0PWRFAIL, instance);

    // Load stored values or used default ones
    ddmw_load_str(&rcs.item.ddm.room, "RCS", instance, "ROOM", "");

    ddmw_load_i32(&rcs.item.ddm.talt, "RCS", instance, "TALT", 3600000);   // One hour for setting temperature
    ddmw_load_i32(&rcs.item.ddm.talt2, "RCS", instance, "TALT2", 300000);  // 5 minutes for alarm
    ddmw_load_i32(&rcs.item.ddm.dalt, "RCS", instance, "DALT", 300000);    // 5 minutes for door alarm
    ddmw_load_i32(&rcs.item.ddm.high_outer_limit, "RCS", instance, "THIOULIM", 2000);
    ddmw_load_i32(&rcs.item.ddm.high_inner_limit, "RCS", instance, "THIINLIM", 1500);
    ddmw_load_i32(&rcs.item.ddm.low_inner_limit, "RCS", instance, "TLOINLIM", -500);
    ddmw_load_i32(&rcs.item.ddm.low_outer_limit, "RCS", instance, "TLOOULIM", -1000);

    ddmw_set_i32(&rcs.item.ddm.dalrm, 0);
    ddmw_set_i32(&rcs.item.ddm.talrm, 0);
    // At startup we have a power failure/reset situation
    ddmw_set_i32(&rcs.item.ddm.power_failure, 1);

    connector_rcs_room_change_actions();
}

/**********************************************************
 * Function:    connector_rcs_room_change_actions
 * Description: Room change actions
 *********************************************************/
static void connector_rcs_room_change_actions(void)
{
    char str[32] = {0};

    // Get room string
    if (ddmw_get_data(&rcs.item.ddm.room, str, ELEMENTS(str) - 1))
    {
        // OK, room string length is not zero

        // Check if onboarded, i.e. we have a thing name.
        if ((char)gw0thing[0] != '\0')
        {
            // Change BLE advertising length
            ble_peripheral_dicm_update_advertising_name(str);

            // Set txt data for service
            network_discovery__mdns_update("room", str);
        }
    }
}

/**********************************************************
 * BT pairing state machine implementation
 *********************************************************/
static void connector_rcs_bt_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: Init pair state");

        ddmw_add(&rcs.ddm, &rcs.item.ddm.bt_pair, BT0PAIR, 0);
        ddmw_set_type(&rcs.item.ddm.bt_pair, DDMW_ACTION_SET);
        ddmw_add(&rcs.ddm, &rcs.item.ddm.mb_avl, MB0AVL, 0);
        ddmw_subscribe(&rcs.item.ddm.mb_avl);
        ddmw_add(&rcs.ddm, &rcs.item.ddm.bt_avl, BT0AVL, 0);
        ddmw_subscribe(&rcs.item.ddm.bt_avl);

        fsm_state_change(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_wait_avl));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_bt_sm_state_wait_avl(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_AVL:
    {
        if ((ddmw_get_i32(&rcs.item.ddm.mb_avl) == 1) && (ddmw_is_subscribed(&rcs.item.ddm.mb_doorsta) == false))
        {
            if (ddmw_get_i32(&rcs.item.ddm.bt_avl) == 1)
            {
                RCS_EXTENDED_LOG(I, "%s", "Registering rcs");
                fsm_state_change(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_register_rcs));
            }
        }
        break;
    }
    case SM_EVENTS_BT_AVL:
    {
        if ((ddmw_get_i32(&rcs.item.ddm.bt_avl) == 1) && (ddmw_is_subscribed(&rcs.item.ddm.mb_doorsta) == false))
        {
            if (ddmw_get_i32(&rcs.item.ddm.mb_avl) == 1)
            {
                RCS_EXTENDED_LOG(I, "%s", "Registering rcs");
                fsm_state_change(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_register_rcs));
            }
        }
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_bt_sm_state_register_rcs(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: Registering rcs!");

        connector_rcs_register();
        ddmw_set_i32(&rcs.item.ddm.avl, 1);

        ddmw_add(&rcs.ddm, &rcs.item.ddm.mb_doorsta, MB0DOORST, 0);
        ddmw_subscribe(&rcs.item.ddm.mb_doorsta);

        fsm_state_change(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_wait_filter_period));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_bt_sm_state_wait_filter_period(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: Wait filter state");

        vTimerSetTimerID(bt_pairing_sm_handle.timer, (void *)SM_EVENTS_BT_FILTER_TIMEOUT);
        TRUE_CHECK_RETURN(xTimerChangePeriod(bt_pairing_sm_handle.timer, pdMS_TO_TICKS(BT_FILTER_TIME_MS), portMAX_DELAY));

        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_BT_FILTER_TIMEOUT:
    {
        fsm_state_change(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_get_door_status));
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_bt_sm_state_get_door_status(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");

        vTimerSetTimerID(bt_pairing_sm_handle.timer, (void *)SM_EVENTS_BT_ENTER_DOOR_STATE_TIMEOUT);
        TRUE_CHECK_RETURN(xTimerChangePeriod(bt_pairing_sm_handle.timer, pdMS_TO_TICKS(BT_ENTER_DOOR_STATE_MS), portMAX_DELAY));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_BT_ENTER_DOOR_STATE_TIMEOUT:
    {
        uint32_t door_status = ddmw_get_i32(&rcs.item.ddm.mb_doorsta);

        RCS_EXTENDED_LOG(I, "Get door status![%d]", door_status);
        if (door_status == 1)
        {
            RCS_EXTENDED_LOG(I, "State PAIR_STATE_ENABLED enter: %i", door_status);

            ddmw_set_i32(&rcs.item.ddm.bt_pair, 1);
            fsm_state_change(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_enabled));
        }
        else
        {
            RCS_EXTENDED_LOG(I, "State PAIR_STATE_DISABLED enter: %i", door_status);
            fsm_state_change(&bt_pairing_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_bt_sm_state_disabled));
        }
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_bt_sm_state_enabled(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_bt_sm_state_disabled(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    default:
        break;
    }
}

/**********************************************************
 * Door alarm state machine implementation
 *********************************************************/
static void connector_rcs_door_alarm_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_AVL:
    {
        if (ddmw_get_i32(&rcs.item.ddm.mb_avl) == 1)
        {
            ddmw_add(&rcs.ddm, &rcs.item.ddm.mb_lightsta, MB0LGTON, 0);
            ddmw_set_type(&rcs.item.ddm.mb_lightsta, DDMW_ACTION_SET);  // Will do a SET instead of PUBLISH
            ddmw_subscribe(&rcs.item.ddm.mb_lightsta);

            if ((ddmw_get_i32(&rcs.item.ddm.bt_avl) == 1))
            {
                fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_transition));
            }
        }
        break;
    }
    case SM_EVENTS_BT_AVL:
    {
        if ((ddmw_get_i32(&rcs.item.ddm.bt_avl) == 1))
        {
            if (ddmw_get_i32(&rcs.item.ddm.mb_avl) == 1)
            {
                fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_transition));
            }
        }
    }
    default:
        break;
    }
}

static void connector_rcs_door_alarm_sm_state_transition(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");

        uint32_t door_status = ddmw_get_i32(&rcs.item.ddm.mb_doorsta);
        if (door_status == 1)
        {
            fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_open));
        }
        else
        {
            fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_closed));
        }

        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_door_alarm_sm_state_closed(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "FSM_ENTRY_EVENT: Door state closed lights=%d", ddmw_get_i32(&rcs.item.ddm.mb_lightsta));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_DOORST:
    {
        uint32_t door_state = ddmw_get_i32(&rcs.item.ddm.mb_doorsta);
        if (door_state == 1)
        {
            fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_open));
        }
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_door_alarm_sm_state_open(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "FSM_ENTRY_EVENT: Door state open lights=%d", ddmw_get_i32(&rcs.item.ddm.mb_lightsta));

        int32_t door_alarm_time_s = 0;
        TRUE_CHECK(connector_rcs_update_internal_variable(RCS0DALT, &rcs.item.ddm.dalt, &door_alarm_time_s));
        uint32_t door_alarm_time_ms = (uint32_t)CONVERT_TIMER_PERIOD_SEC_TO_MS(door_alarm_time_s);

        vTimerSetTimerID(door_alarm_sm_handle.timer, (void *)SM_EVENTS_DOOR_ALARM_TIMEOUT);
        TRUE_CHECK_RETURN(xTimerChangePeriod(door_alarm_sm_handle.timer, pdMS_TO_TICKS(door_alarm_time_ms), portMAX_DELAY));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_RCS_DALT:
    {
        // Reset timer, since DALT has been changed
        fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_open));
        break;
    }
    case SM_EVENTS_MB_DOORST:
    {
        uint32_t door_state = ddmw_get_i32(&rcs.item.ddm.mb_doorsta);
        if (door_state == 0)
        {
            if (ddmw_get_i32(&rcs.item.ddm.mb_lightsta) == 1)
            {
                ddmw_set_i32(&rcs.item.ddm.mb_lightsta, 0);
            }

            TRUE_CHECK_RETURN(xTimerStop(door_alarm_sm_handle.timer, portMAX_DELAY));
            fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_closed));
        }
        break;
    }
    case SM_EVENTS_DOOR_ALARM_TIMEOUT:
    {
        ddmw_set_i32(&rcs.item.ddm.dalrm, 1);
        fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_alarm_active));
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_door_alarm_sm_state_alarm_active(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: Door state alarm active");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_DOORST:
    {
        uint32_t door_state = ddmw_get_i32(&rcs.item.ddm.mb_doorsta);
        if (door_state == 0)
        {
            ddmw_set_i32(&rcs.item.ddm.dalrm, 0);
            fsm_state_change(&door_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_door_alarm_sm_state_closed));
        }
    }
    default:
        break;
    }
}

/**********************************************************
 * Temperature alarm state machine implementation
 *********************************************************/
static void connector_rcs_temp_alarm_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_AVL:
    {
        if (ddmw_get_i32(&rcs.item.ddm.mb_avl) == 1)
        {
            ddmw_add(&rcs.ddm, &rcs.item.ddm.mb_tempsta, MB0TSTAT, 0);
            ddmw_subscribe(&rcs.item.ddm.mb_tempsta);
            ddmw_add(&rcs.ddm, &rcs.item.ddm.mb_tempctrl, MB0TCTRL, 0);
            ddmw_subscribe(&rcs.item.ddm.mb_tempctrl);

            fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_update_variables));
        }
        break;
    }
    default:
        break;
    }
}

/*!	\brief Wait available state
 *
 *	State for updating variables. Transition state.
 */
static void connector_rcs_temp_alarm_sm_state_update_variables(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT ->TEMP_UPDATE_VARIABLES");

        // Read limits and update variables
        temp_alarm_data.high_outer_limit = ddmw_get_i32(&rcs.item.ddm.high_outer_limit);
        temp_alarm_data.high_inner_limit = ddmw_get_i32(&rcs.item.ddm.high_inner_limit);
        temp_alarm_data.low_outer_limit = ddmw_get_i32(&rcs.item.ddm.low_outer_limit);
        temp_alarm_data.low_inner_limit = ddmw_get_i32(&rcs.item.ddm.low_inner_limit);
        // temp_alarm_data.temp_alarm_time = ddmw_get_i32(&rcs.item.ddm.talt);
        temp_alarm_data.temp_alarm_time_2 = ddmw_get_i32(&rcs.item.ddm.talt2);

        TRUE_CHECK(connector_rcs_update_internal_variable(RCS0TALT, &rcs.item.ddm.talt, &temp_alarm_data.temp_alarm_time_in_s));
        TRUE_CHECK(connector_rcs_update_internal_variable(RCS0TALT2, &rcs.item.ddm.talt2, &temp_alarm_data.temp_alarm_time_2_in_s));

        // Clear alarm if set
        if (ddmw_get_i32(&rcs.item.ddm.talrm))
        {
            ddmw_set_i32(&rcs.item.ddm.talrm, 0);
        }

        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_wait_avl));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    default:
        break;
    }
}

/*!	\brief Wait available state
 *
 *	This state is a transition state which only is used when starting up.
 *	The temperature is either ok or outside inner limits.
 */
static void connector_rcs_temp_alarm_sm_state_wait_avl(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: ->TEMP_STATE_WAIT_VAL");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_TSTAT:
    {
        int32_t current_temp = ddmw_get_i32(&rcs.item.ddm.mb_tempsta);
        RCS_EXTENDED_LOG(I, "Temp: %i", current_temp);

        temp_alarm_data.tctrl = ddmw_get_i32(&rcs.item.ddm.mb_tempctrl);
        if (temp_alarm_data.tctrl > (int32_t)(sizeof(target_temp) / sizeof(uint32_t)))
        {
            break;
        }
        RCS_EXTENDED_LOG(I, "Ctrl: %i", target_temp[temp_alarm_data.tctrl]);

        if (IS_TEMP_WITHIN_INNER_LIMITS(current_temp))
        {
            fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_arm_alert));
        }
        else  // outside temperature inner limit
        {
            fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_outside_interval));
        }

        break;
    }
    case SM_EVENTS_TEMP_ALARM_RESET_SM:
    {
        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_update_variables));
        break;
    }
    default:
        break;
    }
}

/*!	\brief Arm alert state
 *
 *	In this state the current temp status is ok and we check for
 *	temperature status that goes outside outer limits.
 *
 *	TODO: Need data! Need to check for tctrl change!!
 */
static void connector_rcs_temp_alarm_sm_state_arm_alert(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: ->TEMP_STATE_ARM_ALERT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_TSTAT:
    {
        int32_t current_temp = ddmw_get_i32(&rcs.item.ddm.mb_tempsta);
        if (IS_TEMP_OUTSIDE_OUTER_LIMITS(current_temp))
        {
            if (temp_alarm_data.temp_alarm_time_2 == 0)
            {
                ddmw_set_i32(&rcs.item.ddm.talrm, 1);
                fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_raise_alert));
            }
            else
            {
                fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_outside_interval_2));
            }
        }
        break;
    }
    case SM_EVENTS_TEMP_ALARM_RESET_SM:
    {
        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_update_variables));
        break;
    }
    default:
        break;
    }
}

/*!	\brief Outside inner interval state
 *
 *	In this state we check that the temperature will reach
 *	the set temperature within time at startup.
 *
 *	TODO: Need data! Need to check for tctrl change!!
 */
static void connector_rcs_temp_alarm_sm_state_outside_interval(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: ->TEMP_STATE_OUTSIDE_INTERVAL");

        uint32_t alarm_timer_ms = CONVERT_TIMER_PERIOD_SEC_TO_MS(temp_alarm_data.temp_alarm_time_in_s);

        vTimerSetTimerID(temp_alarm_sm_handle.timer, (void *)SM_EVENTS_TEMP_ALARM_TIMEOUT);
        TRUE_CHECK_RETURN(xTimerChangePeriod(temp_alarm_sm_handle.timer, pdMS_TO_TICKS(alarm_timer_ms), portMAX_DELAY));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        TRUE_CHECK(xTimerStop(temp_alarm_sm_handle.timer, portMAX_DELAY));
        break;
    }
    case SM_EVENTS_MB_TSTAT:
    {
        int32_t current_temp = ddmw_get_i32(&rcs.item.ddm.mb_tempsta);
        if (IS_TEMP_WITHIN_INNER_LIMITS(current_temp))
        {
            fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_arm_alert));
        }
        break;
    }
    case SM_EVENTS_TEMP_ALARM_TIMEOUT:
    {
        ddmw_set_i32(&rcs.item.ddm.talrm, 1);
        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_raise_alert));
        break;
    }
    case SM_EVENTS_TEMP_ALARM_RESET_SM:
    {
        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_update_variables));
        break;
    }
    default:
        break;
    }
}

/*!	\brief Outside inner interval 2 state
 *
 *	In this state we stay when the temperature is outside the inner limits
 *	and activate an alarm when timer expires.
 *
 *	TODO: Need data! Need to check for tctrl change!!
 */
static void connector_rcs_temp_alarm_sm_state_outside_interval_2(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: ->TEMP_STATE_OUTSIDE_INTERVAL_2");

        uint32_t alarm_timer_2_ms = CONVERT_TIMER_PERIOD_SEC_TO_MS(temp_alarm_data.temp_alarm_time_2_in_s);

        vTimerSetTimerID(temp_alarm_sm_handle.timer, (void *)SM_EVENTS_TEMP_ALARM_TIMEOUT);
        TRUE_CHECK_RETURN(xTimerChangePeriod(temp_alarm_sm_handle.timer, pdMS_TO_TICKS(alarm_timer_2_ms), portMAX_DELAY));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        TRUE_CHECK(xTimerStop(temp_alarm_sm_handle.timer, portMAX_DELAY));
        break;
    }
    case SM_EVENTS_MB_TSTAT:
    {
        int32_t current_temp = ddmw_get_i32(&rcs.item.ddm.mb_tempsta);
        if (IS_TEMP_WITHIN_INNER_LIMITS(current_temp))
        {
            fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_arm_alert));
        }
        break;
    }
    case SM_EVENTS_TEMP_ALARM_TIMEOUT:
    {
        ddmw_set_i32(&rcs.item.ddm.talrm, 1);
        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_raise_alert));
        break;
    }
    case SM_EVENTS_TEMP_ALARM_RESET_SM:
    {
        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_update_variables));
        break;
    }
    default:
        break;
    }
}

/*!	\brief Raise alert state
 *
 *	In this state the temperature alarm is active.
 *	Stay here until the temperature is within the inner limits.
 *
 *	TODO: Need tctrl! Need to check for tctrl change!!
 */
static void connector_rcs_temp_alarm_sm_state_raise_alert(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: ->TEMP_STATE_RAISE_ALERT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MB_TSTAT:
    {
        int32_t current_temp = ddmw_get_i32(&rcs.item.ddm.mb_tempsta);
        if (IS_TEMP_WITHIN_INNER_LIMITS(current_temp))
        {
            ddmw_set_i32(&rcs.item.ddm.talrm, 0);
            fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_arm_alert));
        }
        break;
    }
    case SM_EVENTS_TEMP_ALARM_RESET_SM:
    {
        fsm_state_change(&temp_alarm_sm_handle.sm, FSM_STATE_HANDLER(connector_rcs_temp_alarm_sm_state_update_variables));
        break;
    }
    default:
        break;
    }
}

/**********************************************************
 * MQTT state machine implementation
 *********************************************************/
static void connector_rcs_mqtt_sm_state_init(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");

        ddmw_add(&rcs.ddm, &rcs.item.ddm.mqtt_avl, MQTT0AVL, 0);
        ddmw_subscribe(&rcs.item.ddm.mqtt_avl);

        fsm_state_change(&mqtt_sm_handle, FSM_STATE_HANDLER(connector_rcs_mqtt_sm_state_wait_avl));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    default:
        break;
    }
}

static void connector_rcs_mqtt_sm_state_wait_avl(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MQTT_AVL:
    {
        if (ddmw_get_i32(&rcs.item.ddm.mqtt_avl) == 1)
        {
            ddmw_add(&rcs.ddm, &rcs.item.ddm.mqtt_status, MQTT0STAT, 0);
            ddmw_subscribe(&rcs.item.ddm.mqtt_status);

            fsm_state_change(&mqtt_sm_handle, FSM_STATE_HANDLER(connector_rcs_mqtt_sm_state_status));
        }
    }
    default:
        break;
    }
}

static void connector_rcs_mqtt_sm_state_status(fsm_t *fsm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_ENTRY_EVENT: MQTT status updated");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        RCS_EXTENDED_LOG(I, "%s", "FSM_EXIT_EVENT");
        break;
    }
    case SM_EVENTS_MQTT_STATUS:
    {
        int32_t status = ddmw_get_i32(&rcs.item.ddm.mqtt_status);
        RCS_EXTENDED_LOG(I, "status: 0x%x", status);

        if ((status & MQTT0STAT_PUBLISH_FAIL) || (status & MQTT0STAT_RECONNECTING))
        {
            // This can indicate that we have lost data when sending to Cloud.
            // Set Cloud failure indication.
            ddmw_set_i32(&rcs.item.ddm.cloud_failure, 1);
        }
    }
    default:
        break;
    }
}

//!	\brief Update history accumulators
static void connector_rcs_history_1s(uint32_t event_id)
{
    if (event_id == SM_EVENTS_RCS_THH ||
        event_id == SM_EVENTS_RCS_TDH ||
        event_id == SM_EVENTS_RCS_TWH ||
        event_id == SM_EVENTS_HISTORY_POLL_TIMEOUT)
    {
        switch (event_id)
        {
        case SM_EVENTS_RCS_THH:
        case SM_EVENTS_RCS_TDH:
        case SM_EVENTS_RCS_TWH:
        {
            memset(&rcs.item.history, 0, sizeof(rcs.item.history));
            connector_system_get_local_time(&rcs.item.history.time);
            break;
        }
        default:
            break;
        }

        // Get availability & voltage
        int32_t available = ddmw_get_i32(&rcs.item.ddm.avl);
        int32_t temp = ddmw_get_i32(&rcs.item.ddm.mb_tempsta);

        // Available?
        if (available)
        {
            // Add to history
            connector_rcs_history_add(&rcs.item.history.hour, temp);
            connector_rcs_history_add(&rcs.item.history.day, temp);
            connector_rcs_history_add(&rcs.item.history.week, temp);
        }

        // Get time
        struct tm time;
        if (connector_system_get_local_time(&time))
        {
            // New day?
            if (time.tm_mday != rcs.item.history.time.tm_mday)
            {
                connector_rcs_history_shift(&rcs.item.history.week);
            }

            // New hour?
            if (time.tm_hour != rcs.item.history.time.tm_hour)
            {
                connector_rcs_history_shift(&rcs.item.history.day);
            }

            // New minute?
            if (time.tm_min != rcs.item.history.time.tm_min)
            {
                // New 10 minutes?
                if ((time.tm_min % 10) == 0)
                {
                    connector_rcs_history_shift(&rcs.item.history.hour);
                    connector_rcs_history_save(0);
                }

                // Update DDM
                connector_rcs_history_update_ddm(event_id);
            }

            // Save time
            rcs.item.history.time = time;
        }
    }
}

//! \brief Add history
static void connector_rcs_history_add(refrigerator_history_t *history, int32_t temperature)
{
    history->acc += temperature;
    history->cnt++;
    history->avg[0] = (uint16_t)(history->acc / history->cnt);
}

//! \brief Shift history
static void connector_rcs_history_shift(refrigerator_history_t *history)
{
    for (int j = REFRIDGE_HISTO_SIZE - 1; j > 0; j--)
    {
        history->acc = 0;
        history->cnt = 0;
        history->avg[j] = history->avg[j - 1];
    }
}

//! \brief Set ddm history
static void connector_rcs_history_update_ddm(uint32_t event_id)
{
    // DDM data set by external client?
    switch (event_id)
    {
    case SM_EVENTS_RCS_THH:
        memset(&rcs.item.history.hour, 0, sizeof(rcs.item.history.hour));
        break;
    case SM_EVENTS_RCS_TDH:
        memset(&rcs.item.history.day, 0, sizeof(rcs.item.history.day));
        break;
    case SM_EVENTS_RCS_TWH:
        memset(&rcs.item.history.week, 0, sizeof(rcs.item.history.week));
        break;
    default:
        break;
    }

    // Build DDM data
    uint16_t thh[REFRIDGE_HISTO_SIZE];
    uint16_t tdh[REFRIDGE_HISTO_SIZE];
    uint16_t twh[REFRIDGE_HISTO_SIZE];
    for (int i = 0; i < REFRIDGE_HISTO_SIZE; i++)
    {
        thh[i] = rcs.item.history.hour.avg[i];
        tdh[i] = rcs.item.history.day.avg[i];
        twh[i] = rcs.item.history.week.avg[i];
    }

    // Set DDM data
    ddmw_set_data(&rcs.item.ddm.thh, &thh, MIN(REFRIDGE_HISTO_SIZE, 6) * sizeof(uint16_t));
    ddmw_set_data(&rcs.item.ddm.tdh, &tdh, MIN(REFRIDGE_HISTO_SIZE, 8) * sizeof(uint16_t));
    ddmw_set_data(&rcs.item.ddm.twh, &twh, MIN(REFRIDGE_HISTO_SIZE, 7) * sizeof(uint16_t));
}

//! \brief Load history
static void connector_rcs_history_load(int instance)
{
    ZERO_CHECK(instance);

    // Build key
    char key[32];
    sprintf(key, "%s%d%s", REFRIDGE_CLASS_NAME, instance, REFRIDGE_HISTO_PARAM);

    // Load history data from blob
    size_t size = sizeof(rcs.item.history);
    config_get_blob(key, &rcs.item.history, &size);

    // Update DDM
    connector_rcs_history_update_ddm(0);
}

//! \brief Save history
static void connector_rcs_history_save(int instance)
{
    ZERO_CHECK(instance);

    // Build key
    char key[32];
    sprintf(key, "%s%d%s", REFRIDGE_CLASS_NAME, instance, REFRIDGE_HISTO_PARAM);

    // Save history data to blob
    config_set_blob(key, &rcs.item.history, sizeof(rcs.item.history));
}

//! \brief Save rcs parameter's updates in flash
static void connector_rcs_save_updated_parameters(uint32_t event_id)
{
    switch (event_id)
    {
    case SM_EVENTS_RCS_TALT:
    {
        ddmw_save_i32(&rcs.item.ddm.talt, "RCS", 0, "TALT");
        break;
    }
    case SM_EVENTS_RCS_TALT2:
    {
        ddmw_save_i32(&rcs.item.ddm.talt2, "RCS", 0, "TALT2");
        break;
    }
    case SM_EVENTS_RCS_THIOULIM:
    {
        ddmw_save_i32(&rcs.item.ddm.high_outer_limit, "RCS", 0, "THIOULIM");
        break;
    }
    case SM_EVENTS_RCS_THIINLIM:
    {
        ddmw_save_i32(&rcs.item.ddm.high_inner_limit, "RCS", 0, "THIINLIM");
        break;
    }
    case SM_EVENTS_RCS_TLOINLIM:
    {
        ddmw_save_i32(&rcs.item.ddm.low_inner_limit, "RCS", 0, "TLOINLIM");
        break;
    }
    case SM_EVENTS_RCS_TLOOULIM:
    {
        ddmw_save_i32(&rcs.item.ddm.low_outer_limit, "RCS", 0, "TLOOULIM");
        break;
    }
    case SM_EVENTS_RCS_DALT:
    {
        ddmw_save_i32(&rcs.item.ddm.dalt, "RCS", 0, "DALT");
        break;
    }
    case SM_EVENTS_RCS_ROOM:
    {
        char str[DDMW_ITEM_DATA_CAPACITY];

        ddmw_get_data(&rcs.item.ddm.room, str, sizeof(str));
        ddmw_save_str("RCS", 0, "ROOM", str);

        connector_rcs_room_change_actions();
        break;
    }
    default:
        break;
    }
}

/*! \brief Get state machine event id
 *
 * Determine state machines event id, depending on DDM2 updates or timers timeouts
 *
 */
static uint32_t connector_rcs_get_event_id(const DDMP2_FRAME *frame)
{
    uint32_t event_id = FSM_UNDEFINED_EVENT;

    if (ddmw_is_generic_event_updated(&rcs.ddm))
    {
        uint32_t generic_event_id = ddmw_get_generic_event_id(&rcs.ddm);
        switch (generic_event_id)
        {
        case SM_EVENTS_BT_FILTER_TIMEOUT:
        case SM_EVENTS_BT_ENTER_DOOR_STATE_TIMEOUT:
        case SM_EVENTS_DOOR_ALARM_TIMEOUT:
        case SM_EVENTS_TEMP_ALARM_TIMEOUT:
        case SM_EVENTS_HISTORY_POLL_TIMEOUT:
        {
            event_id = generic_event_id;
            break;
        }
        default:
            LOG(E, "Invalid generic event id[%d] provided", generic_event_id);
            break;
        }
    }
    else
    {
        // Bluetooth
        if (ddmw_is_updated(&rcs.item.ddm.bt_avl))
        {
            event_id = SM_EVENTS_BT_AVL;
        }
        // Minibar
        else if (ddmw_is_updated(&rcs.item.ddm.mb_avl))
        {
            event_id = SM_EVENTS_MB_AVL;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.mb_doorsta))
        {
            event_id = SM_EVENTS_MB_DOORST;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.mb_lightsta))
        {
            event_id = SM_EVENTS_MB_LGTON;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.mb_tempsta))
        {
            event_id = SM_EVENTS_MB_TSTAT;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.mb_tempctrl))
        {
            event_id = SM_EVENTS_MB_TCTRL;
        }
        // Refrigirator control
        else if (ddmw_is_updated(&rcs.item.ddm.high_outer_limit))
        {
            event_id = SM_EVENTS_RCS_THIOULIM;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.high_inner_limit))
        {
            event_id = SM_EVENTS_RCS_THIINLIM;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.low_inner_limit))
        {
            event_id = SM_EVENTS_RCS_TLOINLIM;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.low_outer_limit))
        {
            event_id = SM_EVENTS_RCS_TLOOULIM;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.talt))
        {
            event_id = SM_EVENTS_RCS_TALT;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.talt2))
        {
            event_id = SM_EVENTS_RCS_TALT2;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.dalt))
        {
            event_id = SM_EVENTS_RCS_DALT;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.room))
        {
            event_id = SM_EVENTS_RCS_ROOM;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.thh))
        {
            event_id = SM_EVENTS_RCS_THH;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.tdh))
        {
            event_id = SM_EVENTS_RCS_TDH;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.twh))
        {
            event_id = SM_EVENTS_RCS_TWH;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.cloud_failure))
        {
            event_id = SM_EVENTS_RCS_CDFAIL;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.power_failure))
        {
            event_id = SM_EVENTS_RCS_PWRFAIL;
        }
        // MQTT
        else if (ddmw_is_updated(&rcs.item.ddm.mqtt_avl))
        {
            event_id = SM_EVENTS_MQTT_AVL;
        }
        else if (ddmw_is_updated(&rcs.item.ddm.mqtt_status))
        {
            event_id = SM_EVENTS_MQTT_STATUS;
        }
        else
        {
            // Do nothing, either subscription frame is received or
            // no parameter of intereset(sm events perspective) has been updated
        }
    }

    RCS_EXTENDED_LOG(D, "Event id[%d]", event_id);

    return event_id;
}

/*! \brief Determine event id for temperature alarm state macine
 *
 * Indicate reseting of temperature alarm state machine, if any of
 * the parameters stated in the function definition is updated.
 *
 */
static void connector_rcs_temp_alarm_get_event_id(const fsm_event_t *const input_event, fsm_event_t *const output_event)
{
    switch (input_event->id)
    {
    case SM_EVENTS_RCS_TALT:
    case SM_EVENTS_RCS_TALT2:
    case SM_EVENTS_RCS_THIOULIM:
    case SM_EVENTS_RCS_THIINLIM:
    case SM_EVENTS_RCS_TLOINLIM:
    case SM_EVENTS_RCS_TLOOULIM:
    case SM_EVENTS_MB_TCTRL:
    {
        output_event->id = SM_EVENTS_TEMP_ALARM_RESET_SM;
        break;
    }
    default:
        output_event->id = input_event->id;
        break;
    }
}

//! \brief Run connector task
static void connector_rcs_task(const DDMP2_FRAME *frame)
{
    fsm_event_t event = {FSM_UNDEFINED_EVENT, NULL};
    fsm_event_t temp_alarm_event = {FSM_UNDEFINED_EVENT, NULL};

    ddmw_process(&rcs.ddm, frame);

    event.id = connector_rcs_get_event_id(frame);
    connector_rcs_temp_alarm_get_event_id(&event, &temp_alarm_event);

    fsm_state_dispatch(&bt_pairing_sm_handle.sm, &event);
    fsm_state_dispatch(&door_alarm_sm_handle.sm, &event);
    fsm_state_dispatch(&temp_alarm_sm_handle.sm, &temp_alarm_event);
    fsm_state_dispatch(&mqtt_sm_handle, &event);

    // Is it neccessary to poll it each second, or each min is enough?
    connector_rcs_history_1s(event.id);

    connector_rcs_save_updated_parameters(event.id);

    ddmw_process_publish(&rcs.ddm);
}
