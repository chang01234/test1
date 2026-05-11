/*****************************************************************************
 * \file       connector_usm.c
 * \brief      Connector Update Service Manager
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

/*      TODO:
        - upd_active in service needs to be cleared after timeout or update ready.
        - Handle abort of a block
        - Block size, how to handle different needs of block size
*/
#include "connector_usm.h"

#include "broker.h"
#include "update_service_types.h"
#include <stdint.h>
#include <string.h>
#if defined(CONNECTOR_WIFI)
#include "connector_wifi.h"
#include "wifi_network.h"
#else
#warning CONNECTOR_WIFI is required for this module
#endif
#include "crc32.h"
#include "freertos/task.h"
#include "fsm.h"
#include "hal_cpu.h"
#include "hal_mem.h"
#if defined(CONNECTOR_LINDEV)
#include "connector_lindev.h"
#endif

#ifdef CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif /* CONFIG_PM_ENABLE */

/*****************************************************************************
 * Private defines
 ****************************************************************************/

#define USM_VERSION_1 (1)
#define USM_VERSION_2 (2)

#if defined(CONNECTOR_LINDEV)
#define LINDEV_DISABLE() connector_lindev_disable()
#define LINDEV_ENABLE()  connector_lindev_enable()
#else
#define LINDEV_DISABLE()
#define LINDEV_ENABLE()
#endif

#define USM_EXTENDED_LOGS 0

#ifndef CONNECTOR_USM_STATE_EXTENDED_LOGS
#define CONNECTOR_USM_STATE_EXTENDED_LOGS 0
#endif

#if USM_EXTENDED_LOGS
#define USM_LOG(level, format, ...) LOG(level, format, __VA_ARGS__)
#else
#define USM_LOG(level, format, ...) ((void)0)
#endif  // USM_EXTENDED_LOGS

#if CONNECTOR_USM_STATE_EXTENDED_LOGS
#define USM_STATE_LOG(level, format, ...) LOG(level, format, __VA_ARGS__)
#else
#define USM_STATE_LOG(level, format, ...) ((void)0)
#endif  // CONNECTOR_USM_STATE_EXTENDED_LOGS

// #define RESTART_DISABLED

#define MAX_FILENAME_SZ (DDMP2_MAX_VALUE_SIZE)

#ifndef CONNECTOR_USM_MAX_SERVICES
#define CONNECTOR_USM_MAX_SERVICES (2)
#endif
#ifndef CONNECTOR_USM_TASK_STACK_SIZE
#define CONNECTOR_USM_TASK_STACK_SIZE (3584)
#endif
#ifndef CONNECTOR_USM_PUBLISH_TIMEOUT_MS
#define CONNECTOR_USM_PUBLISH_TIMEOUT_MS (1000)
#endif
#ifndef CONNECTOR_USM_SUPERVISION_TIMEOUT_MS
#define CONNECTOR_USM_SUPERVISION_TIMEOUT_MS (5000)
#endif
#ifndef CONNECTOR_USM_SUPERVISION_TIMEOUT_MAX_RETRIES
#define CONNECTOR_USM_SUPERVISION_TIMEOUT_MAX_RETRIES (12)
#endif
#ifndef CONNECTOR_USM_RRQ_TIMEOUT_MS
#define CONNECTOR_USM_RRQ_TIMEOUT_MS (240000)
#endif
#ifndef CONNECTOR_USM_UPDATE_AREA_SIZE
#define CONNECTOR_USM_UPDATE_AREA_SIZE (10 * 1024)
#endif
#ifndef CONNECTOR_USM_SUSPEND_AP_STA_TIMEOUT_MS
#define CONNECTOR_USM_SUSPEND_AP_STA_TIMEOUT_MS (5000)
#endif
#ifndef CONNECTOR_USM_RESUME_AP_STA_TIMEOUT_MS
#define CONNECTOR_USM_RESUME_AP_STA_TIMEOUT_MS (5000)
#endif

// Local FSM event definitions
#define USM_PUB_DATA_TIMEOUT_EVENT     (100u)
#define USM_SUP_TIMEOUT_EVENT          (101u)
#define USM_RRQ_TIMEOUT_EVENT          (102u)
#define RESET_TIMEOUT_EVENT            (103u)
#define USM_RESUME_TIMEOUT_EVENT       (104u)  // Resume timer
#define USM_WIFI_STA_DISABLED_EVENT    (105u)
#define USM_WIFI_STA_ENABLED_EVENT     (106u)
#define USM_WIFI_AP_DISABLED_EVENT     (107u)
#define USM_WIFI_AP_ENABLED_EVENT      (108u)
#define USM_WIFI_AP_CONNECTED_EVENT    (109u)
#define USM_WIFI_AP_DISCONNECTED_EVENT (110u)

// Defining local parameter space
#define USM0_PUB_DATA_TIMER_PARAM       (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf0))
#define USM0_SUP_TIMER_PARAM            (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf1))
#define USM0_RRQ_TIMER_PARAM            (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf2))
#define RESET_TIMER_PARAM               (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf3))
#define USM0_RESUME_TIMER_PARAM         (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf4))
#define USM0_WIFI_STA_DISABLED_PARAM    (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf5))
#define USM0_WIFI_STA_ENABLED_PARAM     (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf6))
#define USM0_WIFI_AP_DISABLED_PARAM     (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf7))
#define USM0_WIFI_AP_ENABLED_PARAM      (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf8))
#define USM0_WIFI_AP_CONNECTED_PARAM    (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xf9))
#define USM0_WIFI_AP_DISCONNECTED_PARAM (DDM2_PARAMETER_CLASS(USM0AVL) | DDM2_PARAMETER_PROPERTY_FIELD(0xfa))

/*****************************************************************************
 * Private types
 ****************************************************************************/

typedef struct
{
    uint8_t available;
    uint8_t upd_active;  //! \~ Update active
    uint8_t failure_mode;
    uint8_t failure_value;
    uint16_t block_size;
    uint16_t frame_size;
    uint32_t tot_bytes_sent;
    uint16_t read_pos;  //! \~ Read position in stream buffer
    uint16_t block_byte_cnt;
    char fwids[MAX_FILENAME_SZ];
    int8_t instance;
} us_service_t;

typedef struct
{
    int entries;
    us_service_t us[CONNECTOR_USM_MAX_SERVICES];
} usm_service_table_t;

typedef struct
{
    int32_t version;
    USM0MODE_ENUM mode;
    USM0STATE_ENUM state;

    uint32_t upd_size;  //! \~ Total bytes of update image
    uint32_t tot_bytes_rxed;
    const char *pfile;
    usm_service_table_t table;
    uint16_t storage_area_size;  //! \~ How big area of bytes we have in firmware RAM that App can send before stopping
    uint8_t *storage;
    uint8_t descriptor_ignored;
    uint32_t sup_equal_counter;
    int32_t transfer_block_sz;  //! \~ The current transfer block size defined by manager
    uint32_t write_pos;
    int instance;
    crc32_t crc;
    uint8_t active_us;
    bool ota_network_started;
} usm_container_t;

/*****************************************************************************
 * Private functions
 ****************************************************************************/
static void usm_state_idle(fsm_t *const p_sm, const fsm_event_t *p_event);
static void usm_state_prepare_dd(fsm_t *const p_sm, const fsm_event_t *p_event);
static void usm_state_transfer(fsm_t *const p_sm, const fsm_event_t *p_event);
static void usm_state_transfer_ack(fsm_t *const p_sm, const fsm_event_t *p_event);
static void usm_state_extended(fsm_t *const p_sm, const fsm_event_t *p_event);
static void usm_state_network_start(fsm_t *const p_sm, const fsm_event_t *p_event);
static void usm_state_network_stop(fsm_t *const p_sm, const fsm_event_t *p_event);

static void reset_supervision(void);
static int handle_supervision_timeout(void);
static uint32_t get_event_type(const DDMP2_FRAME *const p_frame);

static int initialize_usm(void);
static void usm_ddm_process_task(void *parameter);
static void usm_data_send(void);
static void handle_set(DDMP2_FRAME *pframe);
static void handle_subscribe(DDMP2_FRAME *pframe);
static void publish_int32(uint32_t parameter, int32_t value);
static void request_restart(void);
static void start_subscription_of_service(uint8_t instance);
static void end_subscription_of_service(uint8_t instance);
static void send_block_transfer_result(int32_t result, uint8_t instance);
static int handle_data_set(DDMP2_FRAME *pframe);
static int handle_data_ack(DDMP2_FRAME *pframe);
static void handle_service_status(DDMP2_FRAME *pframe);
static int handle_block_transfer_done(DDMP2_FRAME *pframe);
static void handle_read_request(DDMP2_FRAME *pframe);
static void handle_download_done(DDMP2_FRAME *pframe);
static void complete_rrq(void);
static void publish_timer_timeout(TimerHandle_t xTimer);
static void rrq_timer_timeout(TimerHandle_t xTimer);
static void supervision_timer_timeout(TimerHandle_t xTimer);
static void usm_close_file_transfer(int32_t status);
static void prepare_for_new_block(void);
static void store_service_transfer_config(const us_rrq_read_request_t *rrq, uint8_t instance);
static void reset_service_parameters(void);
static void reset_service_update_active(void);
static void resend_service_block(uint8_t instance);
static void save_service_fwlist(const DDMP2_FRAME *p_frame);
static void send_manager_fwlist(void);
static void suspend_system_activities(void);
static void resume_system_activities(void);

#if defined(CONNECTOR_WIFI)
static int join_string(char *str, const char *str_to_join);
static void resume_timer_timeout(TimerHandle_t xTimer);
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void usm_wifi_sta_handler(WIFI_NETWORK__STA_EVENT_ENUM event, union wifi_network__event_payload payload);
#endif
static void usm_wifi_ap_handler(WIFI_NETWORK__AP_EVENT_ENUM event);
static void mqtt_connect(int32_t connect);
#endif

#if defined(CONNECTOR_BLE)
extern void disable_ble(void);
extern void enable_ble(void);
#endif

/*****************************************************************************
 * Private variables
 ****************************************************************************/
static EXT_RAM_ATTR usm_container_t usm;
static EXT_RAM_ATTR char usm_current_fname[MAX_FILENAME_SZ];
static TimerHandle_t timeout_timer;  //!< \~ Generic timer
static TimerHandle_t sup_timer;      //!< \~ Supervision timer
static TimerHandle_t rrq_timer;      //!< \~ Supervision timer
#if defined(CONNECTOR_WIFI)
static TimerHandle_t resume_timer;  //!< \~ Resume timer
#endif
// static TaskHandle_t data_processing_handle;
static const uint32_t sub_list[] = {US0LIST, US0DATA, US0STAT, US0BTR, US0RRQ, US0PROG};
static fsm_t usm_fsm;

/*****************************************************************************
 * Public variables
 ****************************************************************************/
CONNECTOR connector_usm =
    {
        .name = "USM connector",
        .initialize = initialize_usm,
};

/*****************************************************************************
 * Implementation Private functions
 ****************************************************************************/

#ifdef CONFIG_PM_ENABLE
static esp_pm_lock_handle_t usm_pm_lock = (esp_pm_lock_handle_t)0;
#endif /* CONFIG_PM_ENABLE */
static void reset_supervision(void)
{
    // Clear counter for equal checks
    usm.sup_equal_counter = 0;
}

static int handle_supervision_timeout(void)
{
    int sup_ret_value = 0;
    static uint32_t prev_bytes_counter = 0;
    LOG(I, "test leo -> prev_bytes_counter = %"PRIu32", usm.tot_bytes_rxed = %"PRIu32"", prev_bytes_counter, usm.tot_bytes_rxed);
    if (prev_bytes_counter != usm.tot_bytes_rxed)
    {
        // Here we assume that we have restarted download or are downloading.
        // Remember last value to keep track of progress.
        prev_bytes_counter = usm.tot_bytes_rxed;

        // Clear counter for equal checks
        usm.sup_equal_counter = 0;
    }
    else
    {
        // Equal, count number of times.
        if (++usm.sup_equal_counter == CONNECTOR_USM_SUPERVISION_TIMEOUT_MAX_RETRIES)
        {
            usm.sup_equal_counter = 0;
            usm_close_file_transfer(1);
            sup_ret_value = 1;
        }

        USM_LOG(W, "Timeout, equal counter=%d", usm.sup_equal_counter);
    }

    // Restart timer
    TRUE_CHECK(xTimerReset(sup_timer, portMAX_DELAY));

    // supervision_timeout(&sup_ret_value);
    return sup_ret_value;
}

static void publish_int32(uint32_t parameter, int32_t value)
{
    int status = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                parameter | DDM2_PARAMETER_INSTANCE(usm.instance),
                                                &value, sizeof(value),
                                                connector_usm.connector_id,
                                                portMAX_DELAY);

    if (!status)
    {
        LOG(E, "Publish failed: parameter=0x%08X value=%d", parameter, value);
    }
}

static void request_restart(void)
{
    int32_t restart = (int32_t)GW0CUPD_RESTART;

    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                              GW0CUPD,
                                              &restart, sizeof(restart),
                                              connector_usm.connector_id,
                                              portMAX_DELAY));
}

static void usm_state_idle(fsm_t *p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");

#ifdef CONFIG_PM_ENABLE
        esp_pm_lock_release(usm_pm_lock);
#endif /* CONFIG_PM_ENABLE */
        break;
    }
    case FSM_EXIT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "EXIT_EVENT");

#ifdef CONFIG_PM_ENABLE
        esp_pm_lock_acquire(usm_pm_lock);
#endif /* CONFIG_PM_ENABLE */
        break;
    }
    case USMDD_SET_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USMDD_SET_EVENT");
        handle_download_done((DDMP2_FRAME *const)p_event->p_data);
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_prepare_dd));
        break;
    }
    case USMVER_SET_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USMVER_SET_EVENT");
        handle_set((DDMP2_FRAME *const)p_event->p_data);
        break;
    }
    case USMMODE_SET_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USMMODE_SET_EVENT");
        handle_set((DDMP2_FRAME *const)p_event->p_data);
        break;
    }
    case USMSTATE_SET_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USMSTATE_SET_EVENT");
        handle_set((DDMP2_FRAME *const)p_event->p_data);
        if (usm.state > USM0STATE_FILE_DOWNLOAD)
        {
            LOG(I, "Skip file download and handle extended state");
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_extended));
        }
        break;
    }
    case USSTAT_PUB_EVENT:
    {
        // Ignore in this state
        USM_STATE_LOG(W, "Ignored event: %d", p_event->id);
        break;
    }
    case RESET_TIMEOUT_EVENT:
    {
        request_restart();
        break;
    }
    case USM_WIFI_AP_DISCONNECTED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_AP_DISCONNECTED_EVENT");
        // Client disconnected. We also end up here in case of Manual mode.
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        break;
    }

    default:
    {
        USM_STATE_LOG(W, "unhandled event: %d", p_event->id);
        ESP_LOG_BUFFER_HEXDUMP("Event frame", (DDMP2_FRAME *const)p_event->p_data, ((DDMP2_FRAME *const)p_event->p_data)->frame_size, ESP_LOG_DEBUG);
        break;
    }
    }
}

static void usm_state_prepare_dd(fsm_t *p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT: start rrq_timer");
        TRUE_CHECK(xTimerReset(rrq_timer, portMAX_DELAY));
        gateway_ota_increase_count();
        gateway_ota_publish_status(CFG0OSTAT_ONGOING);
        break;
    }
    case FSM_EXIT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "EXIT_EVENT");
        TRUE_CHECK(xTimerStop(sup_timer, portMAX_DELAY));
        break;
    }
    case USRRQ_PUB_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USRRQ_PUB_EVENT");
        TRUE_CHECK(xTimerStop(rrq_timer, portMAX_DELAY));
        // All PUB of data are done before changing state, see in usm_state_network_start()
        handle_read_request((DDMP2_FRAME *const)p_event->p_data);
        complete_rrq();

        if (!usm.ota_network_started)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_start));
        }
        else
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_transfer));
        }

        break;
    }
    case USM_RRQ_TIMEOUT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_RRQ_TIMEOUT_EVENT");
        usm_close_file_transfer(1);
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_idle));
        break;
    }
    case USMMODE_SET_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USMMODE_SET_EVENT");
        handle_set((DDMP2_FRAME *const)p_event->p_data);
        break;
    }
    default:
    {
        USM_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void usm_state_transfer(fsm_t *p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");
        // Start supervision timer
        reset_supervision();
        TRUE_CHECK(xTimerReset(sup_timer, portMAX_DELAY));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "EXIT_EVENT");
        // Stop timer
        TRUE_CHECK(xTimerStop(sup_timer, portMAX_DELAY));
        break;
    }
    case USM_WIFI_AP_CONNECTED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_AP_CONNECTED_EVENT");
        // Reset supervision timeout
        reset_supervision();
        break;
    }
    case USM_WIFI_AP_DISCONNECTED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_AP_DISCONNECTED_EVENT");
        // Client disconnected.
        usm_close_file_transfer(1);
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        break;
    }
    case USMDATA_SET_EVENT:
    {
        // USM_STATE_LOG(D, "%s", "USMDATA_SET_EVENT");
        if (handle_data_set((DDMP2_FRAME *const)p_event->p_data))
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        }
        break;
    }
    case USDATA_PUB_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USDATA_PUB_EVENT");
        int ack_value = handle_data_ack((DDMP2_FRAME *const)p_event->p_data);
        if (ack_value == 0)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_transfer_ack));
        }
        else
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        }
        break;
    }
    case USM_SUP_TIMEOUT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_SUP_TIMEOUT_EVENT");
        if (handle_supervision_timeout() != 0)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        }
        break;
    }
    case USSTAT_PUB_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USSTAT_PUB_EVENT");
        handle_service_status((DDMP2_FRAME *const)p_event->p_data);
        break;
    }
    case USRRQ_PUB_EVENT:
    {
        LOG(W, "USRRQ_PUB_EVENT");
        LOG(W, "Wrong state to start download");

        int32_t error_status = USM0STAT_ERROR;
        publish_int32(USM0STAT, error_status);
        break;
    }
    default:
    {
        USM_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void usm_state_transfer_ack(fsm_t *p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");
        // Start supervision timer
        TRUE_CHECK(xTimerReset(sup_timer, portMAX_DELAY));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "EXIT_EVENT");
        // Stop timer
        TRUE_CHECK(xTimerStop(sup_timer, portMAX_DELAY));
        break;
    }
    case USM_WIFI_AP_DISCONNECTED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_AP_DISCONNECTED_EVENT");
        // Client disconnected.
        usm_close_file_transfer(1);
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        break;
    }

    case USMDATA_SET_EVENT:
    {
        LOG(E, "USMDATA_SET_EVENT should not arrive here!!");
        break;
    }
    case USBTR_PUB_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USBTR_PUB_EVENT");
        int transfer_state = handle_block_transfer_done((DDMP2_FRAME *const)p_event->p_data);

        if (transfer_state == 0)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_transfer));
        }
        else if (transfer_state == 1)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        }
        else if (transfer_state == 2)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_extended));
        }
        break;
    }
    case USM_SUP_TIMEOUT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_SUP_TIMEOUT_EVENT");
        if (handle_supervision_timeout() != 0)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        }
        break;
    }
    case USSTAT_PUB_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USSTAT_PUB_EVENT");
        handle_service_status((DDMP2_FRAME *const)p_event->p_data);
        break;
    }
    case USRRQ_PUB_EVENT:
    {
        LOG(W, "USRRQ_PUB_EVENT");
        LOG(W, "Wrong state to start download");

        int32_t error_status = USM0STAT_ERROR;
        publish_int32(USM0STAT, error_status);

        break;
    }
    default:
    {
        USM_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void usm_state_extended(fsm_t *p_sm, const fsm_event_t *p_event)
{
    const DDMP2_FRAME *frame = (DDMP2_FRAME *const)p_event->p_data;

    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "EXIT_EVENT");
        break;
    }
    case USMSTATE_SET_EVENT:
    {
        usm.state = frame->frame.set.value.int32;
        USM_STATE_LOG(D, "USMSTATE_SET_EVENT: %d", usm.state);
        publish_int32(USM0STATE, usm.state);
        break;
    }
    case USPROG_PUB_EVENT:
    {
        int32_t progress = frame->frame.publish.value.int32;
        USM_STATE_LOG(D, "USPROG_PUB_EVENT: %d", progress);
        publish_int32(USM0PROG, progress);
        break;
    }
    case USDATA_PUB_EVENT:
    {
        int32_t ostat = CFG0OSTAT_FAILED;
        int32_t usmstat = USM0STAT_ERROR;
        us_data_read_ack_t *ack = (us_data_read_ack_t *)&frame->frame.publish.value.raw[0];

        USM_STATE_LOG(D, "USDATA_PUB_EVENT: result=%d mode=%d", ack->result, usm.mode);

        if ((ack->result == US_ACK_RESULT_OK) ||
            (ack->result == US_ACK_RESULT_OK_RESTART))
        {
            ostat = CFG0OSTAT_OK;
            usmstat = USM0STAT_OK;
        }

        usm.state = USM0STATE_IDLE;
        publish_int32(USM0STATE, usm.state);

        publish_int32(USM0STAT, usmstat);

        gateway_ota_publish_status(ostat);

        if (ack->result == US_ACK_RESULT_OK_RESTART)
        {
            if (usm.mode == USM0MODE_AUTOMATIC)
            {
                request_restart();
            }
        }

        if (usm.mode == USM0MODE_MANUAL)
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_idle));
        }
        else
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        }
        break;
    }
    case USM_WIFI_AP_DISCONNECTED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_AP_DISCONNECTED_EVENT");
        // Client disconnected.
        usm_close_file_transfer(1);
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_network_stop));
        break;
    }
    default:
    {
        USM_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

#if defined(CONNECTOR_WIFI)
static void usm_state_network_start(fsm_t *const p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");
        // Start timer to let published data be sent
        TRUE_CHECK(xTimerReset(timeout_timer, portMAX_DELAY));
        break;
    }
    case FSM_EXIT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "EXIT_EVENT");
        // Stop timer
        TRUE_CHECK(xTimerStop(timeout_timer, portMAX_DELAY));
        break;
    }
    case USM_PUB_DATA_TIMEOUT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_PUB_DATA_TIMEOUT_EVENT");
        suspend_system_activities();

    #ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
        wifi_network__disable();
    #else
        connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_WIFI_STA_DISABLED_PARAM,
                                        NULL, 0, connector_usm.connector_id, portMAX_DELAY);
    #endif
        break;
    }

    case USM_WIFI_STA_DISABLED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_STA_DISABLED_EVENT");
        wifi_network__ap_enable();
        break;
    }
    case USM_WIFI_AP_ENABLED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_AP_ENABLED_EVENT");
        if (usm.mode == USM0MODE_MANUAL)
        {
            LOG(I, "Manual mode - network started");
            usm.ota_network_started = true;
        }

        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_transfer));
        break;
    }
    default:
    {
        USM_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void usm_state_network_stop(fsm_t *const p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");
        // Yield to allow publish to complete before stopping network
        TRUE_CHECK(xTimerReset(timeout_timer, portMAX_DELAY));
        break;
    }
    case USM_PUB_DATA_TIMEOUT_EVENT:
    {
        wifi_network__ap_disable();
        usm.ota_network_started = false;
        break;
    }
    case FSM_EXIT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "EXIT_EVENT");
        resume_system_activities();
        TRUE_CHECK(xTimerStop(resume_timer, portMAX_DELAY));
        break;
    }
    case USM_WIFI_AP_DISABLED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_AP_DISABLED_EVENT");
    #ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
        wifi_network__enable();
    #endif
        TRUE_CHECK(xTimerReset(resume_timer, portMAX_DELAY));
        break;
    }
    case USM_RESUME_TIMEOUT_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_RESUME_TIMEOUT_EVENT");
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_idle));
        break;
    }
    case USM_WIFI_STA_ENABLED_EVENT:
    {
        USM_STATE_LOG(D, "%s", "USM_WIFI_STA_ENABLED_EVENT");
        // fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_idle));
        // usm.ota_network_started = false;
        break;
    }
    default:
    {
        USM_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void usm_wifi_sta_handler(WIFI_NETWORK__STA_EVENT_ENUM event, union wifi_network__event_payload payload)
{
    // This function will handle Wi-Fi station events and forward them to USM over broker generic frames
    switch (event)
    {
    case WIFI_NETWORK__STA_EVENT_ENABLED:
        connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_WIFI_STA_ENABLED_PARAM,
                                          NULL, 0, connector_usm.connector_id, portMAX_DELAY);
        break;
    case WIFI_NETWORK__STA_EVENT_DISABLED:
        connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_WIFI_STA_DISABLED_PARAM,
                                          NULL, 0, connector_usm.connector_id, portMAX_DELAY);
        break;
    default:
        break;
    }
}
#endif

static void usm_wifi_ap_handler(WIFI_NETWORK__AP_EVENT_ENUM event)
{
    // This function will handle Wi-Fi access point events and forward them to USM over broker generic frames
    switch (event)
    {
    case WIFI_NETWORK__AP_EVENT_STARTED:
        connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_WIFI_AP_ENABLED_PARAM,
                                          NULL, 0, connector_usm.connector_id, portMAX_DELAY);
        break;
    case WIFI_NETWORK__AP_EVENT_STOPPED:
        connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_WIFI_AP_DISABLED_PARAM,
                                          NULL, 0, connector_usm.connector_id, portMAX_DELAY);
        break;
    case WIFI_NETWORK__AP_EVENT_CONNECTED:
        connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_WIFI_AP_CONNECTED_PARAM,
                                          NULL, 0, connector_usm.connector_id, portMAX_DELAY);
        break;
    case WIFI_NETWORK__AP_EVENT_DISCONNECTED:
        connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_WIFI_AP_DISCONNECTED_PARAM,
                                          NULL, 0, connector_usm.connector_id, portMAX_DELAY);
        break;
    default:
        break;
    }
}

static void mqtt_connect(int32_t connect)
{
    LOG(I, "MQTT connect: %d", connect);
    TRUE_CHECK(connector_send_frame_to_broker(
        DDMP2_CONTROL_SET, MQTT0CONNECT,
        &connect, sizeof(connect),
        0, portMAX_DELAY));
}

static void usm_bt_connection(int32_t bton)
{
    LOG(I, "BT connection: %d", bton);
    TRUE_CHECK(connector_send_frame_to_broker(
        DDMP2_CONTROL_SET, BT0ON,
        &bton, sizeof(bton),
        connector_usm.connector_id,
        portMAX_DELAY));
}

static int join_string(char *str, const char *str_to_join)
{
    int i;

    // Search for double null to determine end of all strings
    for (i = 0;; i++)
    {
        if ((str[i] == '\0') && (str[i + 1] == '\0'))
        {
            break;
        }
    }

    // Keep strings separated with null termination.
    // Move one step ahead of i to keep null as separator.
    strcpy(&str[i + 1], str_to_join);

    return i + strlen(str_to_join) + sizeof(char);
}

static void resume_timer_timeout(TimerHandle_t xTimer)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_RESUME_TIMER_PARAM, NULL, 0, connector_usm.connector_id, portMAX_DELAY);
}
#else
static void usm_state_network_start(fsm_t *const p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");
        suspend_system_activities();
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_transfer));
        break;
    }
    default:
    {
        // Ignore other events
        break;
    }
    }
}

static void usm_state_network_stop(fsm_t *const p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        USM_STATE_LOG(D, "%s", "ENTRY_EVENT");
        fsm_state_change(p_sm, FSM_STATE_HANDLER(usm_state_idle));
        resume_system_activities();
        break;
    }
    default:
    {
        // Ignore other events
        break;
    }
    }
}
#endif  // defined(CONNECTOR_WIFI)

static int initialize_usm(void)
{
    uint32_t device_class = USM0;

    memset(&usm, 0, sizeof(usm));

#ifdef CONFIG_PM_ENABLE
    esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "usm", &usm_pm_lock);
#endif /* CONFIG_PM_ENABLE */
    fsm_initialize(&usm_fsm, (fsm_state_fcn_t)usm_state_idle);

    usm.storage = hal_mem_malloc_prefer(CONNECTOR_USM_UPDATE_AREA_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);

    if (!usm.storage)
    {
        LOG(E, "Failed to allocate storage");
        return 0;
    }

    usm.version = USM_VERSION_1;
    usm.mode = USM0MODE_AUTOMATIC;
    usm.state = USM0STATE_IDLE;
    usm.ota_network_started = false;

    // Register class
    usm.instance = broker_register_instance(&device_class, connector_usm.connector_id);
    TRUE_CHECK(usm.instance != -1);

    // Set how many frames we can handle. TODO: Add support for malloc failure and then a new malloc with less size until ok.
    usm.storage_area_size = CONNECTOR_USM_UPDATE_AREA_SIZE;

    // Start the Update Service Manager
    TRUE_CHECK(xTaskCreate(usm_ddm_process_task, "usm_ddm", CONNECTOR_USM_TASK_STACK_SIZE, NULL, xTASK_PRIORITY_NORMAL, NULL));
    TRUE_CHECK(timeout_timer = xTimerCreate(NULL, pdMS_TO_TICKS(CONNECTOR_USM_PUBLISH_TIMEOUT_MS), pdFALSE, NULL, publish_timer_timeout));
    TRUE_CHECK(rrq_timer = xTimerCreate(NULL, pdMS_TO_TICKS(CONNECTOR_USM_RRQ_TIMEOUT_MS), pdFALSE, NULL, rrq_timer_timeout));
    TRUE_CHECK(sup_timer = xTimerCreate(NULL, pdMS_TO_TICKS(CONNECTOR_USM_SUPERVISION_TIMEOUT_MS), pdFALSE, NULL, supervision_timer_timeout));
#if defined(CONNECTOR_WIFI)
    resume_timer = xTimerCreate(NULL, pdMS_TO_TICKS(CONNECTOR_USM_RESUME_AP_STA_TIMEOUT_MS), pdFALSE, (void *)0, resume_timer_timeout);
    TRUE_CHECK(resume_timer != NULL);
#endif

    // Subscribe to inventory
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, GW0INV, NULL, 0, connector_usm.connector_id, (TickType_t)portMAX_DELAY));
#if defined(CONNECTOR_WIFI)
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    TRUE_CHECK(wifi_network__register_handler(usm_wifi_sta_handler) == WIFI_NETWORK__OK);
#endif
    TRUE_CHECK(wifi_network__ap_register_handler(usm_wifi_ap_handler) == WIFI_NETWORK__OK);
#endif
    LOG(I, "USM Connector initialized");
    return 1;
}

static void usm_data_send(void)
{
    uint32_t length_to_send;
    const void *data_ptr;
    int last;
    uint8_t i = usm.active_us;
    last = 0;

    if (usm.table.us[i].available && usm.table.us[i].upd_active)
    {
        // Check if we have any more data to send
        while (true)
        {
            // More to send?
            if (usm.write_pos > usm.table.us[i].read_pos)
            {
                // Send in packets of frame_size
                length_to_send = MIN((usm.write_pos - usm.table.us[i].read_pos), usm.table.us[i].frame_size);

                // Check if length fits into service block size
                if ((length_to_send + usm.table.us[i].block_byte_cnt) > usm.table.us[i].block_size)
                {
                    // Adapt to what fits
                    length_to_send = usm.table.us[i].block_size - usm.table.us[i].block_byte_cnt;
                }

                // Find out which data to send
                data_ptr = (const void *)(usm.storage + usm.table.us[i].read_pos);

                // Should we wait for another frame to be able to formward full sized frames?
                if (length_to_send < usm.table.us[i].frame_size)
                {
                    // Check if this is the last frame in transfer
                    if (((usm.table.us[i].tot_bytes_sent + length_to_send) == usm.upd_size) || ((usm.table.us[i].block_byte_cnt + length_to_send) >= usm.table.us[i].block_size))
                    {
                        // last frame in block or full transfer can be shorter than frame_size
                    }
                    else
                    {
                        // Wait for next frame
                        break;
                    }
                }
                // Increase read position
                usm.table.us[i].read_pos += length_to_send;

                // Increase byte counter this block
                usm.table.us[i].block_byte_cnt += length_to_send;

                // Increase number of total bytes sent
                usm.table.us[i].tot_bytes_sent += length_to_send;

                // Check if this is the last frame in transfer
                if ((usm.table.us[i].tot_bytes_sent == usm.upd_size) || (usm.table.us[i].block_byte_cnt >= usm.table.us[i].block_size))
                {
                    USM_LOG(W, "tot=0x%x, of=0x%x, wpos=%d, rpos=%d", usm.table.us[i].tot_bytes_sent, usm.upd_size, usm.write_pos, usm.table.us[i].read_pos);
                    last = 1;
                }

                // Block CRC calculation
                usm.crc = crc32_update(usm.crc, data_ptr, length_to_send);
                if (last)
                {
                    usm.crc = crc32_finalize(usm.crc);
                    USM_LOG(W, "srv[%d], CRC 0x%x for len=0x%x, last=%d", usm.table.us[i].instance, usm.crc, length_to_send, last);
                }
                // Send
                TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                                          US0DATA | DDM2_PARAMETER_INSTANCE(usm.table.us[i].instance),
                                                          data_ptr,
                                                          length_to_send,
                                                          connector_usm.connector_id,
                                                          portMAX_DELAY));
                USM_LOG(W, "srv[%d], CRC 0x%x for len=0x%x, last=%d", usm.table.us[i].instance, usm.crc, length_to_send, last);
                if (last)
                {
                    break;
                }
            }
            else
            {
                break;
            }
            // Yield to not starv other tasks in case of high load
            vTaskDelay((TickType_t)0);
        }
    }
}

static uint32_t get_event_type(const DDMP2_FRAME *const p_frame)
{
    uint32_t retvalue = FSM_UNDEFINED_EVENT;
    switch (p_frame->frame.control)
    {
    case DDMP2_CONTROL_GENERIC:
    {
        switch (p_frame->frame.generic.id)
        {
        case USM0_PUB_DATA_TIMER_PARAM:
        {
            retvalue = USM_PUB_DATA_TIMEOUT_EVENT;
            break;
        }
        case USM0_SUP_TIMER_PARAM:
        {
            retvalue = USM_SUP_TIMEOUT_EVENT;
            break;
        }
        case USM0_RRQ_TIMER_PARAM:
        {
            retvalue = USM_RRQ_TIMEOUT_EVENT;
            break;
        }
        case USM0_RESUME_TIMER_PARAM:
        {
            retvalue = USM_RESUME_TIMEOUT_EVENT;
            break;
        }
        case USM0_WIFI_STA_DISABLED_PARAM:
        {
            retvalue = USM_WIFI_STA_DISABLED_EVENT;
            break;
        }
        case USM0_WIFI_STA_ENABLED_PARAM:
        {
            retvalue = USM_WIFI_STA_ENABLED_EVENT;
            break;
        }
        case USM0_WIFI_AP_DISABLED_PARAM:
        {
            retvalue = USM_WIFI_AP_DISABLED_EVENT;
            break;
        }
        case USM0_WIFI_AP_ENABLED_PARAM:
        {
            retvalue = USM_WIFI_AP_ENABLED_EVENT;
            break;
        }
        case USM0_WIFI_AP_CONNECTED_PARAM:
        {
            retvalue = USM_WIFI_AP_CONNECTED_EVENT;
            break;
        }
        case USM0_WIFI_AP_DISCONNECTED_PARAM:
        {
            retvalue = USM_WIFI_AP_DISCONNECTED_EVENT;
            break;
        }
        case RESET_TIMER_PARAM:
        {
            retvalue = RESET_TIMEOUT_EVENT;
            break;
        }
        default:
        {
            LOG(W, "Unhandled GENERIC id! 0x%x", p_frame->frame.generic.id);
            break;
        }
        }
        break;
    }
    case DDMP2_CONTROL_PUBLISH:
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.publish.parameter))
        {
        case US0LIST:
        {
            // Handle US fw lists
            save_service_fwlist(p_frame);
            break;
        }
        case US0DATA:
        {
            retvalue = USDATA_PUB_EVENT;
            break;
        }
        case US0STAT:
        {
            retvalue = USSTAT_PUB_EVENT;
            break;
        }
        case US0BTR:
        {
            retvalue = USBTR_PUB_EVENT;
            break;
        }
        case US0RRQ:
        {
            retvalue = USRRQ_PUB_EVENT;
            break;
        }
        case US0PROG:
        {
            retvalue = USPROG_PUB_EVENT;
            break;
        }
        case GW0INV:
        {
            size_t sz = ddmp2_value_size(p_frame);
            uint32_t entry;
            uint8_t available;

            USM_LOG(D, "frame size=%d", sz);

            for (size_t i = 0; i < sz; i += 4)
            {
                entry = *((uint32_t *)&p_frame->frame.publish.value.raw[i]);
                available = DDM2_PARAMETER_PROPERTY_FIELD(entry);

                USM_LOG(D, "class=0x%x", entry);

                if (available && (DDM2_PARAMETER_CLASS(entry) == US0))
                {
                    // Start suscription of Update service class
                    start_subscription_of_service(DDM2_PARAMETER_INSTANCE_FIELD(entry));
                }
                else if (!available && (DDM2_PARAMETER_CLASS(entry) == US0AVL))
                {
                    // End suscription of Update service class
                    end_subscription_of_service(DDM2_PARAMETER_INSTANCE_FIELD(entry));
                }
            }
            break;
        }
        default:
        {
            LOG(W, "Unhandled PUB parameter! 0x%x", DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.publish.parameter));
            break;
        }
        }
        break;
    }
    case DDMP2_CONTROL_SET:
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
        {
        case USM0DATA:
        {
            retvalue = USMDATA_SET_EVENT;
            break;
        }
        case USM0DD:
        {
            retvalue = USMDD_SET_EVENT;
            break;
        }
        case USM0STATE:
        {
            retvalue = USMSTATE_SET_EVENT;
            break;
        }
        case USM0MODE:
        {
            retvalue = USMMODE_SET_EVENT;
            break;
        }
        case USM0VER:
        {
            retvalue = USMVER_SET_EVENT;
            break;
        }
        default:
        {
            LOG(W, "Unhandled SET parameter! 0x%x", DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter));
            break;
        }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    return retvalue;
}

static void handle_set(DDMP2_FRAME *pframe)
{
    switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.set.parameter))
    {
    case USM0VER:
        if (pframe->frame.set.value.int32 == USM_VERSION_1 ||
            pframe->frame.set.value.int32 == USM_VERSION_2)
        {
            usm.version = pframe->frame.set.value.int32;
        }
        else
        {
            usm.version = USM_VERSION_1;
        }

        publish_int32(USM0VER, usm.version);
        break;
    case USM0MODE:
        usm.mode = (USM0MODE_ENUM)pframe->frame.set.value.int32;
        publish_int32(USM0MODE, usm.mode);
        break;
    case USM0STATE:
        usm.state = (USM0STATE_ENUM)pframe->frame.set.value.int32;
        publish_int32(USM0STATE, usm.state);
        break;
    default:
        LOG(W, "USM received UNHANDLED set %08x from broker!", pframe->frame.set.parameter);
        break;
    }
}

/* The USM handles subscriptions in another way than a normal connector.
 * Some of its parameters will not return anything when subscribing to
 * as they are used as communication protocol.
 */
static void handle_subscribe(DDMP2_FRAME *pframe)
{
    switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter))
    {
    case USM0TBS:
        usm.transfer_block_sz = CONNECTOR_USM_UPDATE_AREA_SIZE;
        publish_int32(USM0TBS, usm.transfer_block_sz);
        break;
    case USM0LIST:
        // Reply
        send_manager_fwlist();
        break;
    case USM0VER:
        publish_int32(USM0VER, usm.version);
        break;
    case USM0MODE:
        publish_int32(USM0MODE, usm.mode);
        break;
    case USM0STATE:
        publish_int32(USM0STATE, usm.state);
        break;
    case USM0RRQ:
        // fallthrough
    case USM0DATA:
        // fallthrough
    case USM0DD:
        // fallthrough
    case USM0PROG:
        // fallthrough
    case USM0STAT:
        // Ignore
        break;
    default:
        LOG(W, "USM received UNHANDLED subscribe %08x from broker!", pframe->frame.subscribe.parameter);
        break;
    }
}

static void usm_ddm_process_task(void *parameter)
{
    DDMP2_FRAME *pframe;
    DDMP2_FRAME l_frame;
    fsm_event_t m_event;
    size_t frame_size;

    for (;;)
    {
        pframe = (DDMP2_FRAME *)xRingbufferReceive(connector_usm.to_connector, &frame_size, portMAX_DELAY);
        TRUE_CHECK(pframe);
        if (frame_size != 0)
        {
            memcpy(&l_frame, pframe, frame_size);
        }
        vRingbufferReturnItem(connector_usm.to_connector, pframe);

        m_event.p_data = (void *)&l_frame;
        m_event.id = get_event_type(&l_frame);
        if (m_event.id != FSM_UNDEFINED_EVENT)
        {
            fsm_state_dispatch(&usm_fsm, &m_event);
        }
        else
        {
            pframe = &l_frame;
            switch (pframe->frame.control)
            {
            case DDMP2_CONTROL_SUBSCRIBE:
            {
                handle_subscribe(pframe);
                break;
            }
            case DDMP2_CONTROL_PUBLISH:
                // fallthrough
                break;
            default:
            {
                LOG(E, "USM received UNHANDLED frame %02x from broker!", pframe->frame.control);
                break;
            }
            }
        }
        // Yield to not starv other tasks in case of high load
        vTaskDelay((TickType_t)0);
    }
}

static void start_subscription_of_service(uint8_t instance)
{
    int i = 0;

    for (i = 0; i < CONNECTOR_USM_MAX_SERVICES; i++)
    {
        if (!usm.table.us[i].available)
        {
            USM_LOG(I, "Starting subscription from instance %d", instance);

            usm.table.us[i].available = 1;
            usm.table.us[i].instance = (int8_t)instance;

            for (int j = 0; j < (int)ELEMENTS(sub_list); j++)
            {
                TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                                          sub_list[j] | DDM2_PARAMETER_INSTANCE(instance),
                                                          NULL,
                                                          0,
                                                          connector_usm.connector_id,
                                                          portMAX_DELAY));
            }
            break;
        }
    }
    if (i == CONNECTOR_USM_MAX_SERVICES)
    {
        // Failed to allocate us slot
        LOG(E, "No free slots. Failed to start subscribing to instance %d", instance);
    }
}

static void end_subscription_of_service(uint8_t instance)
{
    for (int i = 0; i < CONNECTOR_USM_MAX_SERVICES; i++)
    {
        if (usm.table.us[i].instance == (int8_t)instance)
        {
            USM_LOG(I, "Ending update service instance=%d", instance);

            usm.table.us[i].available = 0;
            usm.table.us[i].instance = -1;
            usm.table.us[i].fwids[0] = '\0';
            break;
        }
    }
}

static int handle_data_set(DDMP2_FRAME *pframe)
{
    size_t length = ddmp2_value_size(pframe);

    TRUE_CHECK(length <= DDMP2_MAX_VALUE_SIZE);
    USM_LOG(D, "Data set, len=%d", length);

    if ((usm.write_pos + length) > usm.storage_area_size)
    {
        LOG(E, "Aborting. Received too many bytes. Will write outside allocated buffer!! %d(%d)/%d",
            usm.write_pos, usm.write_pos + length, usm.storage_area_size);
        ESP_LOG_BUFFER_HEXDUMP("frame", pframe->frame.set.value.raw, length, ESP_LOG_INFO);
        usm_close_file_transfer(1);
        return 1;
    }

    // Copy frame to memory buffer
    memcpy(usm.storage + usm.write_pos, pframe->frame.set.value.raw, length);

    // Increase write pos in storage buffer
    usm.write_pos += length;
    usm.tot_bytes_rxed += length;

    // Send data in packets of frame_size
    usm_data_send();

    return 0;
}

static void send_block_transfer_result(int32_t result, uint8_t instance)
{
    // Set Block transfer result
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                              US0BTR | DDM2_PARAMETER_INSTANCE(instance),
                                              &result, sizeof(result),
                                              connector_usm.connector_id,
                                              portMAX_DELAY));
}

static int handle_data_ack(DDMP2_FRAME *pframe)
{
    int return_value = 0;
    us_data_read_ack_t *ack = (us_data_read_ack_t *)&pframe->frame.publish.value.raw[0];
    int received_instance;

    received_instance = DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter);
    uint8_t i = usm.active_us;
    if (usm.table.us[i].available && usm.table.us[i].upd_active)
    {
        if (usm.table.us[i].instance == (int8_t)received_instance)  // && (ack->seqno == usm.expected_seqno))
        {

            if (ack->result == US_ACK_RESULT_OK)
            {
                USM_LOG(W, "Ack ok from %d, sz=%d", usm.table.us[i].instance, ddmp2_value_size(pframe));

                if (usm.crc == ack->crc)
                {
                    send_block_transfer_result((int32_t)US_ACK_RESULT_OK, usm.table.us[i].instance);
                }
                else
                {
                    LOG(E, "srv[%d], Wrong block crc, rec=0x%x, exp=0x%x", usm.table.us[i].instance, ack->crc, usm.crc);
                    ESP_LOG_BUFFER_HEX("Wrong crc ack", pframe, pframe->frame_size);
                    send_block_transfer_result((int32_t)US_ACK_RESULT_NOTOK, usm.table.us[i].instance);
                }
            }
            else
            {
                // Error TODO:
                LOG(W, "Bad Ack %d sz:%d", ack->result, ddmp2_value_size(pframe));
                ESP_LOG_BUFFER_HEX("Bad ack", pframe, pframe->frame_size);
                usm.table.us[i].failure_mode = 1;
                usm.table.us[i].failure_value = ack->result;
                usm_close_file_transfer(ack->result);
                return_value = 2;
            }
        }
        else
        {
            LOG(E, "Received ack in wrong state");
        }
    }
    return return_value;
}

static void handle_service_status(DDMP2_FRAME *pframe)
{
    uint8_t available_services = 0;
    int32_t error_status;

    for (int i = 0; i < CONNECTOR_USM_MAX_SERVICES; i++)
    {
        if (usm.table.us[i].available)
        {
            available_services++;

            USM_LOG(W, "Available srv=%d", available_services);

            // TODO: Add enum for status
            if (1 == pframe->frame.publish.value.int32)
            {
                usm.descriptor_ignored++;
            }
        }
    }

    if (available_services == usm.descriptor_ignored)
    {
        // Clear
        usm.descriptor_ignored = 0;

        // Send status to App
        error_status = USM0STAT_ERROR;
        publish_int32(USM0STAT, error_status);

        USM_LOG(W, "Status published = %d", error_status);
    }
}

static int handle_block_transfer_done(DDMP2_FRAME *pframe)
{
    int return_value = 0;
    bool block_done = false;
    static int restart_received = 0;
    us_service_t *us = &usm.table.us[usm.active_us];

    if (!us->available || !us->upd_active)
    {
        USM_LOG(W, "Service not available or not active srv=%d", usm.active_us);
        return return_value;
    }

    if (DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) == usm.active_us)
    {
        us_ack_result_t us_ack = (us_ack_result_t)pframe->frame.publish.value.int32;

        switch (us_ack)
        {
        case US_ACK_RESULT_OK_RESTART:
            LOG(C, "-> US_ACK_RESULT_OK_RESTART");
            restart_received = 1;
            gateway_ota_publish_status(CFG0OSTAT_OK);
            // Fallthrough
        case US_ACK_RESULT_OK:
            if ((us->read_pos >= usm.transfer_block_sz) ||
                (us->tot_bytes_sent == usm.upd_size))
            {
                block_done = true;

                if (us->tot_bytes_sent == usm.upd_size)
                {
                    if (us_ack == US_ACK_RESULT_OK_RESTART)
                    {
                        return_value = 1;
                    }
                    else
                    {
                        return_value = 2;
                    }
                }
            }
            else
            {
                us->block_byte_cnt = 0;
                LOG(W, "Block restart");
            }
            break;
        case US_ACK_RESULT_RESEND:
            resend_service_block((uint8_t)(us->instance));
            break;
        case US_ACK_RESULT_NOTOK:
            // Fallthrough
        case US_ACK_RESULT_ABORT:
            LOG(W, "Bad BTR US_ACK_RESULT_ABORT");
            usm_close_file_transfer(1);
            return_value = 1;
            break;
        default:
            break;
        };
    }

    USM_LOG(I, "block_done=%d", block_done);

    if (block_done)
    {
        int32_t progress = 0;

        // Ack
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  USM0DATA | DDM2_PARAMETER_INSTANCE(usm.instance),
                                                  &Zero,
                                                  1,  // Only one byte
                                                  connector_usm.connector_id,
                                                  portMAX_DELAY));

        // Progress
        if (usm.upd_size)
        {
            // Calculate to percent
            progress = (int32_t)((usm.tot_bytes_rxed * 100) / usm.upd_size);
            LOG(I, "%u/%u = %d%%", usm.tot_bytes_rxed, usm.upd_size, progress);
        }

        publish_int32(USM0PROG, progress);

        // Prepare to receive new block
        prepare_for_new_block();

        if (return_value == 1)
        {
            if (usm.mode == USM0MODE_AUTOMATIC)
            {
                usm.state = USM0STATE_IDLE;
                publish_int32(USM0STATE, usm.state);
            }

            gateway_ota_publish_status(CFG0OSTAT_OK);
        }

        // Check if we are in IDLE. IDLE means that all services have sent the last ack.
        // If in IDLE and restart is requested from one service then we restart.
        if (restart_received)
        {
            LOG(I, "Restarting...");
#ifndef RESTART_DISABLED
            request_restart();
#else
            restart_received = 0;  // Needed if restart is not used (when testing)
#endif
        }
    }

    return return_value;
}

static void handle_read_request(DDMP2_FRAME *const pframe)
{
    us_rrq_read_request_t *rrq;
    size_t file_name_size;
    //  char tmpfile[MAX_FILENAME_SZ];

#if USM_EXTENDED_LOGS
    ESP_LOG_BUFFER_HEXDUMP("RRQ", pframe->frame.set.value.raw, ddmp2_value_size(pframe), ESP_LOG_DEBUG);
#endif

    rrq = (us_rrq_read_request_t *)pframe->frame.set.value.raw;

    // Overwrite the frames parameter received from US
    store_service_transfer_config(rrq, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));

    reset_service_update_active();

    // Store file
    USM_LOG(I, "Received Frame sz=%d", pframe->frame_size);
    memset(usm_current_fname, 0, sizeof(usm_current_fname));
    file_name_size = pframe->frame_size - 5 - sizeof(us_rrq_read_request_t) + 1;
    memcpy(usm_current_fname, rrq->transfer_info, MIN(sizeof(usm_current_fname) - 1, file_name_size));

    USM_LOG(I, "Stored fwid=%s, sz=%d", usm_current_fname, file_name_size);  // TODO: Change name from fname to FWID

    // Set file name, all updates services must now have this file name to be accepted.
    usm.pfile = usm_current_fname;

    // Total bytes received cleared
    usm.tot_bytes_rxed = 0;

    // Write position in stream cleared
    usm.write_pos = 0;

    USM_LOG(I, "Read size=0x%x", usm.upd_size);
    for (int i = 0; i < CONNECTOR_USM_MAX_SERVICES; i++)
    {
        LOG(I, "[%d] srv.avl=%d, inst=%d, %d, %d, %d", i, usm.table.us[i].available,
            (uint8_t)(usm.table.us[i].instance), rrq->version, rrq->frame_size, rrq->block_size);
        // Search for registered services, correct instance and matching block size.
        if (usm.table.us[i].available && ((uint8_t)(usm.table.us[i].instance) == DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter)))
        {
            usm.state = USM0STATE_FILE_DOWNLOAD;
            publish_int32(USM0STATE, usm.state);

            usm.version = rrq->version;
            publish_int32(USM0VER, usm.version);

            LOG(I, "Download request from service=%d", usm.table.us[i].instance);
            usm.table.us[i].upd_active = 1;
            usm.active_us = i;
            break;
        }
        else
        {
            LOG(W, "Not available or wrong instance %d", usm.table.us[i].instance);
            // ESP_LOG_BUFFER_HEXDUMP("SRV", &usm.table.us[i], sizeof(us_service_t), ESP_LOG_INFO);
        }
    }
}

static void handle_download_done(DDMP2_FRAME *const pframe)
{
    us_dd_download_done_t *dd;

    dd = (us_dd_download_done_t *)pframe->frame.publish.value.raw;
    usm.upd_size = dd->size;
    pframe->frame_size -= DDMP2_CONTROL_SIZE + DDMP2_PARAMETER_SIZE;
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                              USM0DD | DDM2_PARAMETER_INSTANCE(usm.instance),
                                              pframe->frame.set.value.raw,
                                              pframe->frame_size,
                                              connector_usm.connector_id,
                                              portMAX_DELAY));
}

static void rrq_timer_timeout(TimerHandle_t xTimer)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_RRQ_TIMER_PARAM,
                                      NULL, 0, connector_usm.connector_id,
                                      portMAX_DELAY);
}

static void publish_timer_timeout(TimerHandle_t xTimer)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_PUB_DATA_TIMER_PARAM,
                                      NULL, 0, connector_usm.connector_id,
                                      portMAX_DELAY);
}

/**
 * @brief Suspend system activities such as LIN, MQTT and BT to free up resources for USM download.
 */
static void suspend_system_activities(void)
{
    LOG(W, "Suspending system activities");
#ifdef CONNECTOR_BLE
    disable_ble();
#endif
#if defined(CONNECTOR_WIFI)
    mqtt_connect(Zero);
    usm_bt_connection(Zero);
#endif
    LINDEV_DISABLE();
}

/**
 * @brief Resume system activities such as LIN, MQTT and BT after USM download is done.
 */
static void resume_system_activities(void)
{
    LOG(W, "Resuming system activities");
#ifdef CONNECTOR_BLE
        enable_ble();
#endif
#if defined(CONNECTOR_WIFI)
    usm_bt_connection(One);
    mqtt_connect(One);
#endif
    LINDEV_ENABLE();
}

static void complete_rrq(void)
{
#if defined(CONNECTOR_WIFI)
    char ssid[WIFI_NETWORK__SSID_LENGTH];
    char pwd[WIFI_NETWORK__PASSWORD_LENGTH];
#endif
    DDMP2_FRAME frame;
    int len = 0;
    int size_to_send;
    usm_rrq_read_request_t *manager_rrq = (usm_rrq_read_request_t *)frame.frame.publish.value.raw;

    if (usm.pfile && (strlen(usm.pfile) > 1))
    {
#if defined(CONNECTOR_WIFI)
        // Get AP SSID and password to use
        connector_wifi_get_ap_info(ssid, pwd);

        // Update AP settings
        wifi_network__ap_update_settings(ssid, pwd);
#endif

        // Setup the manager. Use received us block size here
        manager_rrq->block_size = usm.table.us[usm.active_us].block_size;

        // Transfer block size to use
        usm.transfer_block_sz = usm.table.us[usm.active_us].block_size;
        publish_int32(USM0TBS, usm.transfer_block_sz);

        // Clear complete value frame
        memset(manager_rrq->transfer_info, 0, DDMP2_MAX_VALUE_SIZE - sizeof(manager_rrq->block_size));

        strcpy(manager_rrq->transfer_info, usm.pfile);
#if defined(CONNECTOR_WIFI)
        // Join SSID and password after filename, separated with null characters
        join_string(manager_rrq->transfer_info, ssid);
        // Join last string and store complete length of all strings and null characters
        len = join_string(manager_rrq->transfer_info, pwd);
#endif

        // Calculate frame size to send
        size_to_send = sizeof(usm_rrq_read_request_t) - sizeof(char) + len;

        USM_LOG(I, "RRQ frame size=%d", size_to_send);

        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  USM0RRQ | DDM2_PARAMETER_INSTANCE(usm.instance),
                                                  manager_rrq,
                                                  size_to_send,
                                                  connector_usm.connector_id,
                                                  portMAX_DELAY));
        // Clear service
        reset_service_parameters();

        prepare_for_new_block();
    }
}

static void supervision_timer_timeout(TimerHandle_t xTimer)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, USM0_SUP_TIMER_PARAM,
                                      NULL, 0, connector_usm.connector_id,
                                      portMAX_DELAY);
}

static void usm_close_file_transfer(int32_t status)
{
    int32_t error_status;

    LOG(I, "Closing file transfer (%u)", status);

    // Stop supervision timer
    TRUE_CHECK(xTimerStop(sup_timer, portMAX_DELAY));

    usm.version = USM_VERSION_1;
    usm.state = USM0STATE_IDLE;
    usm.mode = USM0MODE_AUTOMATIC;

    // Clear CRC calculation
    usm.crc = crc32_init();

    error_status = status;
    publish_int32(USM0STAT, error_status);

    if (USM0STAT_OK == status)
    {
        gateway_ota_publish_status(CFG0OSTAT_OK);
    }
    else
    {
        gateway_ota_publish_status(CFG0OSTAT_FAILED);
    }
}

static void prepare_for_new_block(void)
{
    for (int j = 0; j < CONNECTOR_USM_MAX_SERVICES; j++)
    {
        usm.table.us[j].read_pos = 0;
        usm.table.us[j].block_byte_cnt = 0;
    }

    // Done with block, clear write pointer
    usm.write_pos = 0;

    // Clear CRC calculation
    usm.crc = crc32_init();
}

static void store_service_transfer_config(const us_rrq_read_request_t *rrq, uint8_t instance)
{
    for (int j = 0; j < CONNECTOR_USM_MAX_SERVICES; j++)
    {
        if ((uint8_t)(usm.table.us[j].instance) == instance)
        {
            usm.table.us[j].block_size = MIN(rrq->block_size, CONNECTOR_USM_UPDATE_AREA_SIZE);
            usm.table.us[j].frame_size = MIN(rrq->frame_size, DDMP2_MAX_VALUE_SIZE);
            break;
        }
    }
}

static void reset_service_parameters(void)
{
    for (int j = 0; j < CONNECTOR_USM_MAX_SERVICES; j++)
    {
        usm.table.us[j].tot_bytes_sent = 0;
        usm.table.us[j].failure_mode = 0;
        usm.table.us[j].read_pos = 0;
        usm.table.us[j].block_byte_cnt = 0;
    }
}

static void reset_service_update_active(void)
{
    usm.active_us = 0xFF;
    for (int j = 0; j < CONNECTOR_USM_MAX_SERVICES; j++)
    {
        usm.table.us[j].upd_active = 0;
    }
}

static void resend_service_block(uint8_t instance)
{
    int resend_block_size;
    uint8_t j = usm.active_us;

    if ((uint8_t)(usm.table.us[j].instance) == instance)
    {
        // Calculate block size
        resend_block_size = usm.table.us[j].tot_bytes_sent % usm.table.us[j].block_size;

        // Check result, 0 means a full block
        if (0 == resend_block_size)
        {
            resend_block_size = usm.table.us[j].block_size;
        }

        // Reduce total byte counter as resend will be done
        usm.table.us[j].tot_bytes_sent -= resend_block_size;

        // Point to the begining of block to resend
        usm.table.us[j].read_pos -= resend_block_size;

        // Clear CRC calculation
        usm.crc = crc32_init();

        // Restart data transfer
        usm_data_send();
    }
}

static void save_service_fwlist(const DDMP2_FRAME *p_frame)
{
    int8_t instance = (int8_t)DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.publish.parameter);
    for (int i = 0; i < CONNECTOR_USM_MAX_SERVICES; i++)
    {
        if (usm.table.us[i].available && (usm.table.us[i].instance == instance))
        {
            ddmp2_extract_string_from_frame(p_frame, usm.table.us[i].fwids, MAX_FILENAME_SZ);
            LOG(I, "Storing new fwlist (%s) from %d", usm.table.us[i].fwids, instance);
        }
    }

    // Update our subscribers
    send_manager_fwlist();
}

static void send_manager_fwlist(void)
{
    // Send concatenated string of all services' fw ids
    char *list_of_ids = NULL;
    size_t size_of_string = 0;

    for (int i = 0; i < CONNECTOR_USM_MAX_SERVICES; i++)
    {
        if (usm.table.us[i].available && (usm.table.us[i].instance != -1))
        {
            size_of_string += strlen(usm.table.us[i].fwids);
            size_of_string++;  // Add separator
        }
    }
    if (size_of_string)
    {
        list_of_ids = hal_mem_malloc_prefer(size_of_string + 1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);  // Add for string termination
        list_of_ids[0] = '\0';
        for (int i = 0; i < CONNECTOR_USM_MAX_SERVICES; i++)
        {
            if (usm.table.us[i].available && (usm.table.us[i].instance != -1))
            {
                strcat(list_of_ids, usm.table.us[i].fwids);
                strcat(list_of_ids, ";");
            }
        }
        size_of_string--;  // Remove last separator
    }
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, USM0LIST, list_of_ids, size_of_string, connector_usm.connector_id, portMAX_DELAY);
    if (list_of_ids)
    {
        free(list_of_ids);
    }
}
