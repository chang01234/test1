/*
 * connector_rvc.c
 *
 *  Created on: 16 Feb 2023
 *      Author: Andreas Lundeen
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/queue.h>

#include "configuration.h"

// IDF includes
#include "driver/gpio.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

// Library includes
#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

// Framework includes
#include "broker.h"
#include "common.h"
#include "connector.h"
#include "connector_rvc.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "dgnnode.h"
#include "hal_can.h"
#include "hal_cpu.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#include "product_conf_manager.h"
#include "product_database.h"
#include "rvc_int.h"
#include "rvc_to_ddm.h"
#include "uid_generator.h"
#include "utils.h"
#ifdef CONNECTOR_HMI
#include "hmi_data_get_version.h"
#endif

// DGN modules includes
#include "aircond.h"
#include "battery.h"
#include "date_time.h"
#include "dcsource.h"
#include "dimmer.h"
#include "heater_sharc.h"
#include "heatpump.h"
#include "inverter_charger.h"
#include "refrig.h"
#include "roof_fan.h"
#include "solar_charge_controller.h"
#include "thermostat.h"

/** Defines */
// #define TEST_RESPONSE

// #define HMI_EXTENDED_LOG
// #define HMI_IGNORE_CRITICAL_ERROR

#define TIMER_COUNT_2_S 2000

#ifndef DEVICE_CMAKE
#define DEVICE_CMAKE ""
#endif

#ifndef DEVICE_CMODEL
#define DEVICE_CMODEL ""
#endif

#define INVALID_RVC_INSTANCE 0

#define MAX_NUM_FAULTS                       8
#define MAX_NUM_DEVICES                      10
#define FAULT_TIMEOUT_MS                     2000  // 2 seconds
#define CONNECTOR_RVC_CHECK_DMRV_TIMER_EVENT 0
#define CONNECTOR_RVC_HEARTBEAT_TIMER_EVENT  1
#define CONNECTOR_RVC_REQUEST_TIMER_EVENT    2
#define ISO_REQUEST_QUEUE_LENGTH             16
#define ISO_REQUEST_QUEUE_ITEM_SIZE          sizeof(iso_request_t)
#define PROP_MESSAGE_QUEUE_LENGTH            16
#define PROP_MESSAGE_QUEUE_ITEM_SIZE         sizeof(prop_message_t)

#define RVC_MAIN_THERMOSTAT_DSA  88
#define RVC_AIRCONDITIONER_1_DSA 103
#define RVC_AIRCONDITIONER_2_DSA 104
#define RVC_AIRCONDITIONER_3_DSA 105
#define RVC_AIRCONDITIONER_4_DSA 106
#define RVC_REFRIGERATOR_DSA     107
#define RVC_DC_DIMMER_DSA        131

static const int multi_instance_DSA[] = {
    RVC_MAIN_THERMOSTAT_DSA,
    RVC_AIRCONDITIONER_1_DSA,
    RVC_AIRCONDITIONER_2_DSA,
    RVC_AIRCONDITIONER_3_DSA,
    RVC_AIRCONDITIONER_4_DSA,
    RVC_REFRIGERATOR_DSA,
    RVC_DC_DIMMER_DSA,
};

typedef void (*parameter_changed_t)(uint8_t table_index, int32_t i32Value);
typedef void (*set_func_t)(uint32_t table_index, DDMP2_FRAME *pFrame);
typedef void (*receive_func_t)(uint32_t table_index, int32_t value);
typedef bool (*receive_function_t)(uint8_t *p_data, uint8_t sa, size_t size);
typedef bool (*transmit_function_t)(uint8_t instance, uint8_t *p_data);
typedef void (*class_handle_function_t)(uint32_t dgn, DDMP2_FRAME *pFrame);

typedef enum
{
    CAN_STATUS_UNKNOWN = 0,
    CAN_STATUS_ACTIVE,
    CAN_STATUS_INACTIVE,
} CAN_STATUS_t;

typedef struct
{
    uint32_t ddm_class;
    uint32_t dgn_tx;
    transmit_function_t transmit_function;
    uint32_t dgn_rx;
    receive_function_t receive_function;
    class_handle_function_t class_handle_function;
} rvc_transceiver_ref_t;

typedef struct
{
    uint32_t ddm_class;
    uint32_t dgn;
    class_handle_function_t class_set_function;
} rvc_class_set_ref_t;

typedef struct fault_info
{
    uint32_t spn;
    uint8_t instance_id;
    bool red_lamp;
    bool yellow_lamp;
    uint64_t last_seen_time;
    bool is_active;
    uint8_t fmi;
    uint8_t occ_cnt;
    uint8_t dsa_ext;
} fault_info_t;

typedef struct device_entry
{
    uint8_t op_status;
    uint8_t sa;
    uint8_t dsa;
    uint64_t last_dmrv_time;
    size_t num_faults;
    fault_info_t faults[MAX_NUM_FAULTS];
} device_entry_t;

typedef enum
{
    DEVICE_STATE_UNKNOWN = 0,
    DEVICE_STATE_WAIT_DMRV,
    DEVICE_STATE_WAIT_PID,
    DEVICE_STATE_RESOLVED
} device_state_t;

typedef struct device_node_data
{
    device_state_t state;
    RVCDGN_zDGN_65259 pid_data;
    uint8_t rvc_heartbeat;
    bool is_resolved;  // Both DMRV and PID received
    device_entry_t device;
} device_node_data_t;

typedef struct
{
    uint8_t da;       // Destination Address
    uint32_t dgn;     // DGN number
    uint8_t retries;  // Number of retries left
} iso_request_t;

typedef struct
{
    uint8_t da;  // Destination Address
    uint8_t data[PROPDGN_DGN_61184_SIZE];
    size_t size;      // Data size
    uint8_t retries;  // Number of retries left
} prop_message_t;

static SemaphoreHandle_t fault_mutex;
static SemaphoreHandle_t rvc_heartbeat_mutex;

static QueueHandle_t iso_request_queue = NULL;
static StaticQueue_t iso_request_q;
static EXT_RAM_ATTR uint8_t iso_request_queue_storage[ISO_REQUEST_QUEUE_LENGTH * ISO_REQUEST_QUEUE_ITEM_SIZE];

static QueueHandle_t prop_message_queue = NULL;
static StaticQueue_t prop_message_q;
static EXT_RAM_ATTR uint8_t prop_message_queue_storage[PROP_MESSAGE_QUEUE_LENGTH * PROP_MESSAGE_QUEUE_ITEM_SIZE];

static EXT_RAM_ATTR uint8_t device_SA[MAX_NUM_DEVICES];
static bool check_and_add_sa_to_tracking(uint8_t sa);
static void remove_sa_from_tracking(uint8_t sa);
static void update_sa_in_tracking(uint8_t old_sa, uint8_t new_sa);

#ifdef CONFIG_POWER_MANAGEMENT
static void IRAM_ATTR rx_isr_handler(void *args);
#endif

static int initialize_connector(void);
static int initialize_can_driver(void);
static void initialize_can_gpio(void);
static void can_error_cb(int32_t error_code);
static void RVC_Library_Initialize(void);
static void rvc_process_task(void *Parameter);
static void rvc_txrx_task(void *Parameter);
static void Main_ProcessTimers(void);
static bool rvc_rx_cb(uint32_t dgn, uint8_t sa, uint8_t *p_data, size_t size);
static bool rvc_tx_cb(uint32_t dgn, uint8_t instance, uint8_t *p_data);
#ifdef CONNECTOR_RVC_INCLUDE_RAW_RVC_MP
static void rvc_mp_rx_cb(uint32_t canid, uint8_t size, uint8_t *p_data);
#endif
#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
static void rvc_std_rx_cb(uint32_t canid, uint8_t size, uint8_t *p_data);
#endif
static void feedback_handler(TimerHandle_t xTimer);
#ifdef TEST_RESPONSE
static void test_feedback_handler(TimerHandle_t xTimer);
#endif

static void enqueue_iso_request(uint8_t da, uint32_t dgn, uint8_t num_retries);
static void process_iso_request_queue(void);
static void enqueue_prop_msg(uint8_t da, uint8_t *p_data, size_t size, uint8_t num_retries);
static void process_prop_msg_queue(void);

static bool request_rvc_on_timer(DgnNode_t *dgn_node, void *arg);
static bool check_heartbeat_timer(DgnNode_t *dgn_node, void *arg);
static bool check_dmrv_timer_fcn(DgnNode_t *dgn_node, void *arg);

typedef struct update_sa
{
    uint8_t new_sa;
    RVCDGN_zDGN_65259 *pid;
} update_sa_t;

static bool update_sa_node_if_same_pid(DgnNode_t *dgn_node, void *arg);

// Add DGN specific receive/transmit functions here
/*************************************************************************************************************/
/*                            DGN SPECIFIC RECEIVE/TRANSMIT/HANDLE FUNCTION DECLARATION                      */
/*************************************************************************************************************/
static void handleRVCMGNT0(uint32_t dgn, DDMP2_FRAME *p_frame);
static bool transmit98048Dgn(uint8_t instance, uint8_t *p_data);
static bool receive59392Dgn(uint8_t *p_data, uint8_t sa, size_t size);
#ifdef RVC_CONFIG_IMPL_DOWNLOAD
static bool receive97536Dgn(uint8_t *p_data, uint8_t sa, size_t size);
#endif
static void handleRVCPIM0(uint32_t dgn, DDMP2_FRAME *p_frame);
static void handleRVCDM0(uint32_t dgn, DDMP2_FRAME *p_frame);
static bool transmit59904Dgn(uint8_t instance, uint8_t *p_data);
static bool transmit65259Dgn(uint8_t instance, uint8_t *p_data);
static bool receive65259Dgn(uint8_t *p_data, uint8_t sa, size_t size);
static bool transmit130762Dgn(uint8_t instance, uint8_t *p_data);
static bool receive130762Dgn(uint8_t *p_data, uint8_t sa, size_t size);
static bool receive130776Dgn(uint8_t *p_data, uint8_t sa, size_t size);
static void handleRVCGENCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);

#ifdef RVC_CONFIG_IMPL_PROPRIATARY
static bool receive61184Dgn(uint8_t *p_data, uint8_t sa, size_t size);
static bool transmit61184Dgn(uint8_t instance, uint8_t *p_data);
static void handleRVCPROP0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

static void status_changed_cb(msgcan_lib_status_type_t status_type, uint32_t value);
static void prod_rvc_reset_handler(int32_t cmd, int instance);
static void make_prod_prop_classes_unavailable(int ddm_instance);
void remove_generic_rvc_nodes(uint8_t sa);

/*************************************************************************************************************/
/*                            LOCAL VARIABLE DECLARATION FOR DGN MANAGEMENT                                  */
/*************************************************************************************************************/

#ifdef RVC_CONFIG_IMPL_PROPRIATARY
static EXT_RAM_ATTR struct
{
    PROPDGN_zDGN_61184 in;
    PROPDGN_zDGN_61184 out;
} l_61184_dgn;
#endif

static EXT_RAM_ATTR struct
{
    RVCDGN_zDGN_65259 in;
    RVCDGN_zDGN_65259 out;
} l_65259_dgn;

static EXT_RAM_ATTR struct
{
    RVCDGN_zDGN_130762 in;
    RVCDGN_zDGN_130762 out;
} l_130762_dgn;

// General reset
static EXT_RAM_ATTR RVCDGN_zDGN_98048 l_98048_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_59904 l_59904_dgn;

static EXT_RAM_ATTR list_t l_prod;
static EXT_RAM_ATTR list_t l_130776_dgn;

#ifdef TEST_RESPONSE
static TimerHandle_t test_timeout_timer;
#endif
CONNECTOR connector_rvc = {
    .name = "Generic RV/C connector",
    .initialize = initialize_connector,
};

/**
 * @brief Table containing references to which DGN Classes and its corresponding transmit/receive DGN definitions
 */
static const rvc_transceiver_ref_t l_rvc_transceiver_dgn_table[] =
    {
        {RVCMGNT0, GENERAL_RESET_DGN, transmit98048Dgn, ACKNOWLEDGMENT_DGN, receive59392Dgn, handleRVCMGNT0},
#ifdef RVC_CONFIG_IMPL_DOWNLOAD
        {DDMP2_INVALID_PARAMETER, 0, NULL, DOWNLOAD_DGN, receive97536Dgn, NULL},
#endif
        {RVCPIM0, PRODUCT_ID_MSG_DGN, transmit65259Dgn, PRODUCT_ID_MSG_DGN, receive65259Dgn, handleRVCPIM0},
        {PROD0, 0, NULL, 0, NULL, NULL},
        {DDMP2_INVALID_PARAMETER, INFORMATION_REQUEST_DGN, transmit59904Dgn, 0, NULL, NULL},
        {RVCDM0, DMRV_MSG_DGN, transmit130762Dgn, DMRV_MSG_DGN, receive130762Dgn, handleRVCDM0},
        {RVCGENCFG0, 0, NULL, GENERIC_CONFIGURATION_STATUS_DGN, receive130776Dgn, handleRVCGENCFG0},
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
        {RVCTHASTAT0, THERMOSTAT_AMBIENT_STATUS_DGN, transmit130972Dgn, 0, NULL, handleRVCTHASTAT0},
        {RVCTHTWO0, THERMOSTAT_STATUS_2_DGN, transmit130810Dgn, THERMOSTAT_COMMAND_2_DGN, receive130808Dgn, handleRVC2TH0},
        {RVCTH0, THERMOSTAT_STATUS_1_DGN, transmit131042Dgn, THERMOSTAT_COMMAND_1_DGN, receive130809Dgn, handleRVCTH0},
        {RVCTHSCHEDTWO0, THERMOSTAT_SCHEDULE_STATUS_2, transmit130806Dgn, THERMOSTAT_SCHEDULE_COMMAND_2, receive130804Dgn, handleRVC2THSCHED0},
        {RVCTHSCHED0, THERMOSTAT_SCHEDULE_STATUS_1, transmit130807Dgn, THERMOSTAT_SCHEDULE_COMMAND_1, receive130805Dgn, handleRVCTHSCHED0},
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
        {RVCTHASTAT0, 0, NULL, THERMOSTAT_AMBIENT_STATUS_DGN, receive130972Dgn, handleRVCTHASTAT0},
        {RVCTHTWO0, THERMOSTAT_COMMAND_2_DGN, transmit130808Dgn, THERMOSTAT_STATUS_2_DGN, receive130810Dgn, handleRVC2TH0},
        {RVCTH0, THERMOSTAT_COMMAND_1_DGN, transmit130809Dgn, THERMOSTAT_STATUS_1_DGN, receive131042Dgn, handleRVCTH0},
        {RVCTHSCHEDTWO0, THERMOSTAT_SCHEDULE_COMMAND_2, transmit130804Dgn, THERMOSTAT_SCHEDULE_STATUS_2, receive130806Dgn, handleRVC2THSCHED0},
        {RVCTHSCHED0, THERMOSTAT_SCHEDULE_COMMAND_1, transmit130805Dgn, THERMOSTAT_SCHEDULE_STATUS_1, receive130807Dgn, handleRVCTHSCHED0},
#endif
#ifdef RVC_CONFIG_BUS_TIME_USE
        {RVCTIME0, SET_DATE_TIME_COMMAND_DGN, transmit131070Dgn, DATE_TIME_STATUS, receive131071Dgn, handleRVCTIME0},
#endif
#ifdef RVC_CONFIG_BUS_TIME_KEEP
        {RVCTIME0, DATE_TIME_STATUS, transmit131071Dgn, SET_DATE_TIME_COMMAND_DGN, receive131070Dgn, handleRVCTIME0},
#endif
#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1) || defined(RVC_CONFIG_IMPL_DC_DIMMER_2)
        {RVCDIMTHR0, DC_DIMMER_STATUS_3_DGN, transmit130778Dgn, 0, NULL, handleRVC3DIM0},
        {RVCDIMTWO0, 0, 0, DC_DIMMER_COMMAND_2_DGN, receive130779Dgn, handleRVC2DIM0},
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_1) || defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
        {RVCDIMTHR0, 0, 0, DC_DIMMER_STATUS_3_DGN, receive130778Dgn, handleRVC3DIM0},
        {RVCDIMTWO0, DC_DIMMER_COMMAND_2_DGN, transmit130779Dgn, 0, NULL, handleRVC2DIM0},
#endif
#ifdef RVC_CONFIG_INTERF_SHARC_HTR
        {RVCHTR0, HEATER_SHARC_OPER_COMMAND_DGN, transmit130558PropDgn, HEATER_SHARC_OPER_FEEDBACK_DGN, receive130559PropDgn, handleRVCHTR0},
        {RVCHTRVER0, 0, NULL, HEATER_SHARC_VERSION_DGN, receive130552PropDgn, handleRVCHTRVER0},
        {RVCHTRFAULT0, 0, NULL, HEATER_SHARC_FAULTS_DGN, receive130553PropDgn, handleRVCHTRFAULT0},
        {RVCHTRSCHED0, HEATER_SHARC_SCHED_COMMAND_DGN, transmit130554PropDgn, HEATER_SHARC_SCHED_FEEDBACK_DGN, receive130555PropDgn, handleRVCHTRSCHED0},
        {RVCHTRST0, 0, NULL, HEATER_SHARC_STATUS_DGN, receive130557PropDgn, handleRVCHTRST0},
        {RVCHMI0, HEATER_SHARC_HMI_STATUS_DGN, transmit130556PropDgn, 0, NULL, handleRVCHMI0},
#endif
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
        {RVCHTR0, HEATER_SHARC_OPER_COMMAND_DGN, transmit130558PropDgn, HEATER_SHARC_OPER_FEEDBACK_DGN, receive130559PropDgn, handleRVCHTR0},
        {RVCHTRVER0, 0, NULL, HEATER_SHARC_VERSION_DGN, receive130552PropDgn, handleRVCHTRVER0},
        {RVCHTRFAULT0, 0, NULL, HEATER_SHARC_FAULTS_DGN, receive130553PropDgn, handleRVCHTRFAULT0},
        {RVCHTRSCHED0, HEATER_SHARC_SCHED_COMMAND_DGN, transmit130554PropDgn, HEATER_SHARC_SCHED_FEEDBACK_DGN, receive130555PropDgn, handleRVCHTRSCHED0},
        {RVCHTRST0, 0, NULL, HEATER_SHARC_STATUS_DGN, receive130557PropDgn, handleRVCHTRST0},
        {RVCHMI0, HEATER_SHARC_HMI_STATUS_DGN, transmit130556PropDgn, 0, NULL, handleRVCHMI0},
#endif
#ifdef RVC_CONFIG_IMPL_AIR_CONDITIONER
        {RVCAC0, AIR_CONDITIONER_STATUS_DGN, transmit131041Dgn, AIR_CONDITIONER_COMMAND_DGN, receive131040Dgn, handleRVCAC0},
        {RVCACTWO0, AIR_CONDITIONING_STATUS_2, transmit130505Dgn, 0, NULL, handleRVC2AC0},
#endif
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
        {RVCAC0, AIR_CONDITIONER_COMMAND_DGN, transmit131040Dgn, AIR_CONDITIONER_STATUS_DGN, receive131041Dgn, handleRVCAC0},
#endif
#ifdef RVC_CONFIG_IMPL_ROOF_FAN
        {RVCRFAN0, ROOF_FAN_STATUS_1_DGN, transmit130727Dgn, ROOF_FAN_COMMAND_1_DGN, receive130726Dgn, handleRVCRFAN0},
        {RVCRFANTWO0, ROOF_FAN_STATUS_2_DGN, transmit130531Dgn, ROOF_FAN_COMMAND_2_DGN, receive130530Dgn, handleRVCRFANTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_ROOF_FAN
        {RVCRFAN0, ROOF_FAN_COMMAND_1_DGN, transmit130726Dgn, ROOF_FAN_STATUS_1_DGN, receive130727Dgn, handleRVCRFAN0},
        {RVCRFANTWO0, ROOF_FAN_COMMAND_2_DGN, transmit130530Dgn, ROOF_FAN_STATUS_2_DGN, receive130531Dgn, handleRVCRFANTWO0},
#endif
#ifdef RVC_CONFIG_IMPL_PROPRIATARY
        {RVCPROP0, PROP_MSG_DGN, transmit61184Dgn, PROP_MSG_DGN, receive61184Dgn, handleRVCPROP0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
        {RVCDCSRC0, 0, NULL, DC_SOURCE_STATUS_1_DGN, receive131069Dgn, handleRVCDCSRC0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
        {RVCDCSRCTWO0, 0, NULL, DC_SOURCE_STATUS_2_DGN, receive131068Dgn, handleRVCDCSRCTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
        {RVCDCSRCTHR0, 0, NULL, DC_SOURCE_STATUS_3_DGN, receive131067Dgn, handleRVCDCSRCTHR0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
        {RVCDCSRCFOUR0, 0, NULL, DC_SOURCE_STATUS_4_DGN, receive130761Dgn, handleRVCDCSRCFOUR0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
        {RVCDCSRCFIVE0, 0, NULL, DC_SOURCE_STATUS_5_DGN, receive130760Dgn, handleRVCDCSRCFIVE0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
        {RVCDCSRCSIX0, 0, NULL, DC_SOURCE_STATUS_6_DGN, receive130759Dgn, handleRVCDCSRCSIX0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
        {RVCDCSRCSEV0, 0, NULL, DC_SOURCE_STATUS_7_DGN, receive130732Dgn, handleRVCDCSRCSEV0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
        {RVCDCSRCEIG0, 0, NULL, DC_SOURCE_STATUS_8_DGN, receive130731Dgn, handleRVCDCSRCEIG0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
        {RVCDCSRCNINE0, 0, NULL, DC_SOURCE_STATUS_9_DGN, receive130730Dgn, handleRVCDCSRCNINE0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
        {RVCDCSRCTEN0, 0, NULL, DC_SOURCE_STATUS_10_DGN, receive130729Dgn, handleRVCDCSRCTEN0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
        {RVCDCSRCELE0, 0, NULL, DC_SOURCE_STATUS_11_DGN, receive130725Dgn, handleRVCDCSRCELE0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
        {RVCDCSRCTWE0, 0, NULL, DC_SOURCE_STATUS_12_DGN, receive130552Dgn, handleRVCDCSRCTWE0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
        {RVCDCSRCTHI0, 0, NULL, DC_SOURCE_STATUS_13_DGN, receive130535Dgn, handleRVCDCSRCTHI0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
        {RVCDCSRCCFG0, DC_SOURCE_CONFIGURATION_COMMAND_1_DGN, transmit130550Dgn, DC_SOURCE_CONFIGURATION_STATUS_1_DGN, receive130551Dgn, handleRVCDCSRCCFG0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
        {RVCDCSRCCFGTWO0, DC_SOURCE_CONFIGURATION_COMMAND_2_DGN, transmit130548Dgn, DC_SOURCE_CONFIGURATION_STATUS_2_DGN, receive130549Dgn, handleRVCDCSRCCFGTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
        {RVCDCSRCCONN0, 0, NULL, DC_SOURCE_CONNECTION_STATUS_DGN, receive130512Dgn, handleRVCDCSRCCONN0},
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
        {RVCDCSRCCMD0, DC_SOURCE_COMMAND_DGN, transmit130724Dgn, 0, NULL, handleRVCDCSRCCMD0},
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
        {RVCDCSRCCFGTHR0, DC_SOURCE_CONFIGURATION_COMMAND_3_DGN, transmit130526Dgn, 0, NULL, handleRVCDCSRCCFGTHR0},
#endif
#ifdef RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS
        {RVCDCDISCONN0, DC_DISCONNECT_COMMAND_DGN, transmit130767Dgn, DC_DISCONNECT_STATUS_DGN, receive130768Dgn, handleRVCDCDISCONN0},
#endif
#ifdef RVC_CONFIG_INTERF_BATTERY_SUMMARY
        {RVCBATSUM0, 0, NULL, BATTERY_SUMMARY_DGN, receive130545Dgn, handleRVCBATSUM0},
#endif
#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
        {RVCHPUMP0, HEAT_PUMP_COMMAND_DGN, transmit130970Dgn, HEAT_PUMP_STATUS_DGN, receive130971Dgn, handleRVCHPUMP0},
#endif
#ifdef RVC_CONFIG_IMPL_REFRIGERATOR
        {RVCREFRIG0, REFRIGERATOR_STATUS_DGN, transmit130515Dgn, REFRIGERATOR_COMMAND_DGN, receive130514Dgn, handleRVCREFRIG0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS
        {RVCSOLAR0, SOLAR_CONTROLLER_COMMAND_DGN, transmit130737Dgn, SOLAR_CONTROLLER_STATUS_DGN, receive130739Dgn, handleRVCSOLAR0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_2
        {RVCSOLARTWO0, 0, NULL, SOLAR_CONTROLLER_STATUS_2_DGN, receive130693Dgn, handleRVCSOLARTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_3
        {RVCSOLARTHR0, 0, NULL, SOLAR_CONTROLLER_STATUS_3_DGN, receive130692Dgn, handleRVCSOLARTHR0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_4
        {RVCSOLARFOUR0, 0, NULL, SOLAR_CONTROLLER_STATUS_4_DGN, receive130691Dgn, handleRVCSOLARFOUR0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_5
        {RVCSOLARFIVE0, 0, NULL, SOLAR_CONTROLLER_STATUS_5_DGN, receive130690Dgn, handleRVCSOLARFIVE0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_6
        {RVCSOLARSIX0, 0, NULL, SOLAR_CONTROLLER_STATUS_6_DGN, receive130689Dgn, handleRVCSOLARSIX0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_BATTERY_STATUS
        {RVCSOLARBAT0, 0, NULL, SOLAR_CONTROLLER_BATTERY_STATUS_DGN, receive130688Dgn, handleRVCSOLARBAT0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS
        {RVCSOLARARR0, 0, NULL, SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS_DGN, receive130559Dgn, handleRVCSOLARARR0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS
        {RVCSOLARCFG0, SOLAR_CONTROLLER_CONFIGURATION_COMMAND_DGN, transmit130736Dgn, SOLAR_CONTROLLER_CONFIGURATION_STATUS_DGN, receive130738Dgn, handleRVCSOLARCFG0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_2
        {RVCSOLARCFGTWO0, SOLAR_CONTROLLER_CONFIGURATION_COMMAND_2_DGN, transmit130557Dgn, SOLAR_CONTROLLER_CONFIGURATION_STATUS_2_DGN, receive130558Dgn, handleRVCSOLARCFGTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_3
        {RVCSOLARCFGTHR0, SOLAR_CONTROLLER_CONFIGURATION_COMMAND_3_DGN, transmit130555Dgn, SOLAR_CONTROLLER_CONFIGURATION_STATUS_3_DGN, receive130556Dgn, handleRVCSOLARCFGTHR0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_4
        {RVCSOLARCFGFOUR0, SOLAR_CONTROLLER_CONFIGURATION_COMMAND_4_DGN, transmit130553Dgn, SOLAR_CONTROLLER_CONFIGURATION_STATUS_4_DGN, receive130554Dgn, handleRVCSOLARCFGFOUR0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_5
        {RVCSOLARCFGFIVE0, SOLAR_CONTROLLER_CONFIGURATION_COMMAND_5_DGN, transmit130510Dgn, SOLAR_CONTROLLER_CONFIGURATION_STATUS_5_DGN, receive130511Dgn, handleRVCSOLARCFGFIVE0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_STATUS
        {RVCSOLAREQ0, 0, NULL, SOLAR_EQUALIZATION_STATUS_DGN, receive130735Dgn, handleRVCSOLAREQ0},
#endif
#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_CONFIGURATION_STATUS
        {RVCSOLAREQCFG0, SOLAR_EQUALIZATION_CONFIGURATION_COMMAND_DGN, transmit130733Dgn, SOLAR_EQUALIZATION_CONFIGURATION_STATUS_DGN, receive130734Dgn, handleRVCSOLAREQCFG0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_1
        {RVCINVERTAC0, 0, NULL, INVERTER_AC_STATUS_1_DGN, receive131031Dgn, handleRVCINVERTAC0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_2
        {RVCINVERTACTWO0, 0, NULL, INVERTER_AC_STATUS_2_DGN, receive131030Dgn, handleRVCINVERTACTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_3
        {RVCINVERTACTHREE0, 0, NULL, INVERTER_AC_STATUS_3_DGN, receive131029Dgn, handleRVCINVERTACTHREE0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_4
        {RVCINVERTACFOUR0, 0, NULL, INVERTER_AC_STATUS_4_DGN, receive130959Dgn, handleRVCINVERTACFOUR0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_DC_STATUS
        {RVCINVERTDC0, 0, NULL, INVERTER_DC_STATUS_DGN, receive130792Dgn, handleRVCINVERTDC0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_STATUS
        {RVCINVERT0, INVERTER_COMMAND_DGN, transmit131027Dgn, INVERTER_STATUS_DGN, receive131028Dgn, handleRVCINVERT0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_1
        {RVCINVERTCFG0, INVERTER_CONFIGURATION_COMMAND_1_DGN, transmit131024Dgn, INVERTER_CONFIGURATION_STATUS_1_DGN, receive131026Dgn, handleRVCINVERTCFG0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_2
        {RVCINVERTCFGTWO0, INVERTER_CONFIGURATION_COMMAND_2_DGN, transmit131023Dgn, INVERTER_CONFIGURATION_STATUS_2_DGN, receive131025Dgn, handleRVCINVERTCFGTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_3
        {RVCINVERTCFGTHREE0, INVERTER_CONFIGURATION_COMMAND_3_DGN, transmit130765Dgn, INVERTER_CONFIGURATION_STATUS_3_DGN, receive130766Dgn, handleRVCINVERTCFGTHREE0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_4
        {RVCINVERTCFGFOUR0, INVERTER_CONFIGURATION_COMMAND_4_DGN, transmit130714Dgn, INVERTER_CONFIGURATION_STATUS_4_DGN, receive130715Dgn, handleRVCINVERTCFGFOUR0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS
        {RVCINVERTTEMP0, 0, NULL, INVERTER_TEMPERATURE_STATUS_DGN, receive130749Dgn, handleRVCINVERTTEMP0},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS_2
        {RVCINVERTTEMPTWO0, 0, NULL, INVERTER_TEMPERATURE_STATUS_2_DGN, receive130507Dgn, handleRVCINVERTTEMPTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_1
        {RVCCHRGAC0, 0, NULL, CHARGER_AC_STATUS_1_DGN, receive131018Dgn, handleRVCCHRGAC0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_3
        {RVCCHRGACTHREE0, 0, NULL, CHARGER_AC_STATUS_3_DGN, receive131016Dgn, handleRVCCHRGACTHREE0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS
        {RVCCHRG0, CHARGER_COMMAND_DGN, transmit131013Dgn, CHARGER_STATUS_DGN, receive131015Dgn, handleRVCCHRG0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS_2
        {RVCCHRGTWO0, 0, NULL, CHARGER_STATUS_2_DGN, receive130723Dgn, handleRVCCHRGTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS
        {RVCCHRGCFG0, CHARGER_CONFIGURATION_COMMAND_DGN, transmit131012Dgn, CHARGER_CONFIGURATION_STATUS_DGN, receive131014Dgn, handleRVCCHRGCFG0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_2
        {RVCCHRGCFGTWO0, CHARGER_CONFIGURATION_COMMAND_2_DGN, transmit130965Dgn, CHARGER_CONFIGURATION_STATUS_2_DGN, receive130966Dgn, handleRVCCHRGCFGTWO0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_3
        {RVCCHRGCFGTHREE0, CHARGER_CONFIGURATION_COMMAND_3_DGN, transmit130763Dgn, CHARGER_CONFIGURATION_STATUS_3_DGN, receive130764Dgn, handleRVCCHRGCFGTHREE0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_4
        {RVCCHRGCFGFOUR0, CHARGER_CONFIGURATION_COMMAND_4_DGN, transmit130750Dgn, CHARGER_CONFIGURATION_STATUS_4_DGN, receive130751Dgn, handleRVCCHRGCFGFOUR0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_EQUALIZATION_CONFIGURATION_STATUS
        {RVCCHRGEQCFG0, CHARGER_EQUALIZATION_CONFIGURATION_COMMAND_DGN, transmit130967Dgn, CHARGER_EQUALIZATION_CONFIGURATION_STATUS_DGN, receive130968Dgn, handleRVCCHRGEQCFG0},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_ACFAULT_CONFIG_STATUS_1
        {RVCCHRGACFAULTCFG0, CHARGER_ACFAULT_CONFIG_COMMAND_1_DGN, transmit130951Dgn, CHARGER_ACFAULT_CONFIG_STATUS_1_DGN, receive130953Dgn, handleRVCCHRGACFAULTCFG0},
#endif
};

typedef struct
{
    PIMAGE_eTABLE rvc_var_id;
    uint16_t cross_ref_id;
    char *string;  // RVC parameter to reset
} rvc_string_t;

static volatile uint16_t main_u16TicksInit = 0;
static volatile uint16_t main_u16TicksSys = 0;
static volatile uint16_t main_u16Elapsed_ms = 0;
static volatile uint16_t main_u16Ticks10ms = 0;
static volatile uint16_t main_u16Ticks1000ms = 0;

static int __attribute__((unused)) sleep_indication = 0;
static uint8_t dimm_instance = 0;
#ifdef CONFIG_POWER_MANAGEMENT
static const TickType_t inactive_to_unknown_timeout = pdMS_TO_TICKS(10);  //!< Time with no packets before ging to unknown state
static const TickType_t activity_detect_timeout = pdMS_TO_TICKS(2000);    //!< Time to listen for bus activity in unknown state
// Time to sleep in unknown state before listening for bus activity, incremental backoff
static const TickType_t retry_backoff_ticks[] = {
    pdMS_TO_TICKS(30000),
    pdMS_TO_TICKS(60000),
    pdMS_TO_TICKS(120000),
    pdMS_TO_TICKS(240000),
    pdMS_TO_TICKS(480000),
};
#endif

static uint8_t cur_operation = 0;

static TimerHandle_t timeout_timer;  //!< \~ Reset timer
static TimerHandle_t check_dmrv_timer;
static TimerHandle_t rvc_heartbeat_timer;
static TimerHandle_t rvc_request_timer;

static TaskHandle_t can_txrx_task_handle = NULL;  //!< TX RX task handle for task notifications from ISR

static void feedback_handler(TimerHandle_t xTimer)
{
    (void)xTimer;
    xEventGroupSetBits(callback_accept_handle, CB_TARGET_TEMP_NOT_BLOCKED);
    LOG(I, "Callback accepted");
}

static void check_dmrv_timer_handler(TimerHandle_t check_dmrv_timer)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, CONNECTOR_RVC_CHECK_DMRV_TIMER_EVENT, NULL, 0, connector_rvc.connector_id, portMAX_DELAY);
}

static void rvc_heartbeat_timer_handler(TimerHandle_t rvc_heartbeat_timer)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, CONNECTOR_RVC_HEARTBEAT_TIMER_EVENT, NULL, 0, connector_rvc.connector_id, portMAX_DELAY);
}

static void rvc_request_timer_handler(TimerHandle_t rvc_request_timer)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, CONNECTOR_RVC_REQUEST_TIMER_EVENT, NULL, 0, connector_rvc.connector_id, portMAX_DELAY);
}
#ifdef TEST_RESPONSE
static uint32_t send_index;
static uint8_t send_instance = 0;
static void test_feedback_handler(TimerHandle_t xTimer)
{
    (void)xTimer;
    // Simulating receiving DGN data

    LOG(I, "test_feedback_handler: %d, sent dgn: %d", send_index, Rvc_table[send_index].rvc_dgn);
    switch (Rvc_table[send_index].rvc_dgn)
    {
    case THERMOSTAT_STATUS_1_DGN:
        // Sending Thermostat 1 status. Nothing to handle
        // Just send back what was sent for now
        switch (DDM2_PARAMETER_INSTANCE_FIELD(Rvc_table[send_index].ddm_parameter))
        {
        case 0:
            PIMAGE_SetValue(VAR_DGN_130809_OPERATING_MODE, PIMAGE_GetValue(VAR_DGN_131042_OPERATING_MODE_Z1));            // Forced on
            PIMAGE_SetValue(VAR_DGN_130809_FAN_MODE, PIMAGE_GetValue(VAR_DGN_131042_FAN_MODE_Z1));                        // Auto
            PIMAGE_SetValue(VAR_DGN_130809_FAN_SPEED, PIMAGE_GetValue(VAR_DGN_131042_FAN_SPEED_Z1));                      // 50 %
            PIMAGE_SetValue(VAR_DGN_130809_SHEDULE_MODE, PIMAGE_GetValue(VAR_DGN_131042_SCHEDULE_MODE_Z1));               // Disabled
            PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT, PIMAGE_GetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z1));  // Disabled
            PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL, PIMAGE_GetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z1));  // Disabled
            rvc_parameter_update_cb(VAR_DGN_130809_SYNC, 1);
            break;
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
        case 1:
            PIMAGE_SetValue(VAR_DGN_130809_OPERATING_MODE_Z2, PIMAGE_GetValue(VAR_DGN_131042_OPERATING_MODE_Z2));            // Forced on
            PIMAGE_SetValue(VAR_DGN_130809_FAN_MODE_Z2, PIMAGE_GetValue(VAR_DGN_131042_FAN_MODE_Z2));                        // Auto
            PIMAGE_SetValue(VAR_DGN_130809_FAN_SPEED_Z2, PIMAGE_GetValue(VAR_DGN_131042_FAN_SPEED_Z2));                      // 50 %
            PIMAGE_SetValue(VAR_DGN_130809_SHEDULE_MODE_Z2, PIMAGE_GetValue(VAR_DGN_131042_SCHEDULE_MODE_Z2));               // Disabled
            PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z2, PIMAGE_GetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z2));  // Disabled
            PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z2, PIMAGE_GetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z2));  // Disabled
            rvc_parameter_update_cb(VAR_DGN_130809_SYNC_Z2, 1);
            break;
#endif
        }
        break;
    case THERMOSTAT_COMMAND_1_DGN:
        // Sending Thermostat 1 command. Simulate a response of Thermostat 1 status
        // which instance ?
        switch (DDM2_PARAMETER_INSTANCE_FIELD(Rvc_table[send_index].ddm_parameter))
        {
        case 0:
            PIMAGE_SetValue(VAR_DGN_131042_OPERATING_MODE_Z1, PIMAGE_GetValue(VAR_DGN_130809_OPERATING_MODE));            // Forced on
            PIMAGE_SetValue(VAR_DGN_131042_FAN_MODE_Z1, PIMAGE_GetValue(VAR_DGN_130809_FAN_MODE));                        // Auto
            PIMAGE_SetValue(VAR_DGN_131042_FAN_SPEED_Z1, PIMAGE_GetValue(VAR_DGN_130809_FAN_SPEED));                      // 50 %
            PIMAGE_SetValue(VAR_DGN_131042_SCHEDULE_MODE_Z1, PIMAGE_GetValue(VAR_DGN_130809_SHEDULE_MODE));               // Disabled
            PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z1, PIMAGE_GetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT));  // Disabled
            PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z1, PIMAGE_GetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL));  // Disabled
            PIMAGE_SetValue(VAR_DGN_131042_SYNC_Z1, 1);
            rvc_parameter_update_cb(VAR_DGN_131042_SYNC_Z1, 1);
            break;
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
        case 1:
            PIMAGE_SetValue(VAR_DGN_131042_OPERATING_MODE_Z2, PIMAGE_GetValue(VAR_DGN_130809_OPERATING_MODE_Z2));            // Forced on
            PIMAGE_SetValue(VAR_DGN_131042_FAN_MODE_Z2, PIMAGE_GetValue(VAR_DGN_130809_FAN_MODE_Z2));                        // Auto
            PIMAGE_SetValue(VAR_DGN_131042_FAN_SPEED_Z2, PIMAGE_GetValue(VAR_DGN_130809_FAN_SPEED_Z2));                      // 50 %
            PIMAGE_SetValue(VAR_DGN_131042_SCHEDULE_MODE_Z2, PIMAGE_GetValue(VAR_DGN_130809_SHEDULE_MODE_Z2));               // Disabled
            PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z2, PIMAGE_GetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z2));  // Disabled
            PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z2, PIMAGE_GetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z2));  // Disabled
            PIMAGE_SetValue(VAR_DGN_131042_SYNC_Z2, 1);
            rvc_parameter_update_cb(VAR_DGN_131042_SYNC_Z2, 1);
            break;
#endif
        }
        break;
    case THERMOSTAT_SCHEDULE_COMMAND_1:
    {
        uint32_t class_instance = 0;
        int32_t value;
        // Thermostat Schedule command
        // Simulate a Thermostat Schedule status
        // Find class instance connected to zone instance. Normally one-to-one map 1-1 2-2 etc
        for (int i = 0; i < 10; i++)
        {
            // if (sched_instances[i].inst == send_instance+1)
            {
                // LOG(I, "Found class instance: %d for zone instance: %d", i, sched_instances[i].inst);
                class_instance = i;
                break;
            }
        }
        switch (sched_instances[class_instance].inst)
        {
        case 1:
            // Zone 1
            value = PIMAGE_GetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z1);
            PIMAGE_SetValue(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z1, value);
            rvc_parameter_update_cb(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z1, value);
            switch (value)
            {
            case 0:
                // Sleep
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_HOUR_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_0_START_HOUR_Z1, value);
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_MINUTE_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_0_START_MINUTE_Z1, value);
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z1, value);
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z1, value);
                PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z1, 1);
                rvc_parameter_update_cb(VAR_DGN_130807_SYNC_Z1, 1);
                PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z1, 0);
                break;
            case 1:
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_HOUR_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_1_START_HOUR_Z1, value);
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_MINUTE_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_1_START_MINUTE_Z1, value);
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z1, value);
                value = PIMAGE_GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z1);
                PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z1, value);
                rvc_parameter_update_cb(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z1, value);
                PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z1, 1);
                rvc_parameter_update_cb(VAR_DGN_130807_SYNC_Z1, 1);
                PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z1, 0);
                break;
            }

            break;
        case 2:
            // Zone 2
            break;
        case 3:
            // Zone 3
            break;
        case 4:
            // Zone 4
            break;
        default:
            LOG(E, "unexpected zone %d", sched_instances[class_instance].inst);
            break;
        }
        break;
    }
    case THERMOSTAT_SCHEDULE_COMMAND_2:
        // Thermostat Schedule 2 command
        // Simulate a Thermostat Schedule 2 status
        PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE, PIMAGE_GetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z1));
        rvc_parameter_update_cb(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE, PIMAGE_GetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE));
        break;

    default:
        break;
    }
}
#endif

static void install_parameters(void)
{
    uint32_t ddm_class;
    int __attribute__((unused)) instance;
    LIST_INIT(&l_prod);
    LIST_INIT(&l_130776_dgn);

    // Manager class
    ddm_class = RVCMGNT0;
    // Register devices
    broker_register_instance(&ddm_class, connector_rvc.connector_id);

    ddm_class = RVCPIM0;
    broker_register_instance(&ddm_class, connector_rvc.connector_id);

    ddm_class = RVCDM0;
    broker_register_instance(&ddm_class, connector_rvc.connector_id);

#if defined(RVC_CONFIG_IMPL_AIR_CONDITIONER) || defined(RVC_CONFIG_INTERF_AIR_CONDITIONER)
    aircond_init(connector_rvc.connector_id);
#endif
#ifdef RVC_CONFIG_INTERF_BATTERY_SUMMARY
    battery_init(connector_rvc.connector_id, &l_prod);
#endif
#if defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3) ||                              \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6) ||                              \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9) ||                              \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12) ||                           \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13) || defined(RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1) || defined(RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2) || \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS) || defined(RVC_CONFIG_IMPL_DC_SOURCE_COMMAND) || defined(RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3) ||           \
    defined(RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS)
    dcsource_init(connector_rvc.connector_id, &l_prod);
#endif
#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1) || defined(RVC_CONFIG_INTERF_DC_DIMMER_1)
    dimmer_init(connector_rvc.connector_id);
#endif
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    heater_sharc_init(connector_rvc.connector_id);
#endif
#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
    heatpump_init(connector_rvc.connector_id);
#endif
#ifdef RVC_CONFIG_IMPL_REFRIGERATOR
    refrig_init(connector_rvc.connector_id);
#endif
#if defined(RVC_CONFIG_IMPL_ROOF_FAN) || defined(RVC_CONFIG_INTERF_ROOF_FAN)
    roof_fan_init(connector_rvc.connector_id);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    thermostat_init(connector_rvc.connector_id);
#endif
#if defined(RVC_CONFIG_BUS_TIME_USE) || defined(RVC_CONFIG_BUS_TIME_KEEP)
    date_time_init(connector_rvc.connector_id);
#endif

#if defined(RVC_CONFIG_SOLAR_CONTROLLER)
    solar_charge_controller_init(connector_rvc.connector_id, &l_prod);
#endif
#if defined(RVC_CONFIG_INVERTER_CHARGER)
    inverter_charger_init(connector_rvc.connector_id, &l_prod);
#endif

#ifdef RVC_CONFIG_IMPL_PROPRIATARY
    ddm_class = RVCPROP0;
    instance = broker_register_instance(&ddm_class, connector_rvc.connector_id);
    ASSERT(instance > -1);

    memset(&l_61184_dgn, 0xFF, sizeof(l_61184_dgn));
    l_61184_dgn.in.size = 0;
    l_61184_dgn.out.size = 0;
#endif

    memset(&l_65259_dgn, 0x0, sizeof(l_65259_dgn));
    snprintf(l_65259_dgn.out.cMake, RVCDGN_DGN_65259_FIELD_SIZE, DEVICE_CMAKE);
    snprintf(l_65259_dgn.out.cModel, RVCDGN_DGN_65259_FIELD_SIZE, DEVICE_CMODEL);
    memset(&l_130762_dgn, 0x0, sizeof(l_130762_dgn));
    l_130762_dgn.out.u8DSA = CONNECTOR_RVC_DSA;
    l_130762_dgn.out.u8DSAExtension = 0xFF;
    memset(&l_98048_dgn, 0, sizeof(l_98048_dgn));
}

static void enqueue_iso_request(uint8_t da, uint32_t dgn, uint8_t num_retries)
{
    iso_request_t req = {.da = da, .dgn = dgn, .retries = num_retries};
    if (iso_request_queue)
    {
        if (xQueueSend(iso_request_queue, &req, 0) != pdPASS)
        {
            LOG(W, "ISO request queue full, dropping request DA=%02X DGN=%u", da, dgn);
        }
    }
}

static void process_iso_request_queue(void)
{
    iso_request_t req;
    int processed = 0;
    while ((iso_request_queue) && (xQueueReceive(iso_request_queue, &req, 0) == pdPASS))
    {
        bool sent = NMEA2K_TxISORequest(BSP_CAN_RVC, req.da, req.dgn);
        if (!sent)
        {
            if (req.retries > 0)
            {
                req.retries--;
                // Requeue for retry
                if (xQueueSendToFront(iso_request_queue, &req, 0) != pdPASS)
                {
                    LOG(W, "ISO request queue full, dropping request DA=%02X DGN=%u", req.da, req.dgn);
                }
                else
                {
                    LOG(D, "ISO request DA=%02X DGN=%u requeued, %d retries remaining", req.da, req.dgn, req.retries);
                }
            }
            else
            {
                // No more retries left
                LOG(W, "ISO request DA=%02X DGN=%u dropped", req.da, req.dgn);
            }

            // Break out of the loop if sending failed, to try again later
            break;
        }
        else
        {
            processed++;
        }
    }
    if (processed > 0)
    {
        LOG(D, "Processed %d ISO requests", processed);
    }
}

/**
 * @brief Enqueue proprietary message for transmission
 *
 * @param da Destination address
 * @param p_data Pointer to data buffer
 * @param size Data size
 */
static void enqueue_prop_msg(uint8_t da, uint8_t *p_data, size_t size, uint8_t num_retries)
{
    if (!prop_message_queue || !p_data || size == 0 || size > PROPDGN_DGN_61184_SIZE)
    {
        LOG(W, "Invalid proprietary message parameters");
        return;
    }

    prop_message_t msg = {
        .da = da,
        .size = size,
        .retries = num_retries};
    memcpy(msg.data, p_data, size);

    if (xQueueSend(prop_message_queue, &msg, 0) != pdPASS)
    {
        LOG(W, "Proprietary message queue full, dropping message DA=%02X", da);
    }
    else
    {
        LOG(D, "Enqueued proprietary message for DA=%02X, queue depth=%d",
            da, uxQueueMessagesWaiting(prop_message_queue));
    }
}

/**
 * @brief Process proprietary message queue with TX_READY flow control and retry logic
 * Sends first message immediately to bootstrap, then respects TX_READY for subsequent messages
 * Includes retry mechanism similar to ISO requests
 */
static void process_prop_msg_queue(void)
{
    if (!prop_message_queue)
    {
        return;
    }

    static uint32_t messages_sent = 0;
    prop_message_t msg;
    int processed = 0;

    // Check if we have messages in queue
    if (uxQueueMessagesWaiting(prop_message_queue) == 0)
    {
        return;  // Queue empty
    }

    // Determine if we can send based on TX_READY or first message bootstrap
    bool can_send = false;

    if (messages_sent == 0)
    {
        // First message - send immediately to bootstrap TX_READY mechanism
        can_send = true;
        LOG(D, "Sending first proprietary message to bootstrap TX_READY");
    }
    else
    {
        // Subsequent messages - wait for TX_READY notification
        if (ulTaskNotifyTake(pdTRUE, 0) > 0)
        {
            can_send = true;
            LOG(D, "TX_READY received, can send next message");
        }
    }

    // Process messages if we can send
    if (can_send)
    {
        while ((prop_message_queue) && (xQueueReceive(prop_message_queue, &msg, 0) == pdPASS))
        {
            // Prepare message for transmission
            l_61184_dgn.out.addr = msg.da;
            l_61184_dgn.out.size = msg.size;
            memcpy(l_61184_dgn.out.u8Data, msg.data, msg.size);

            // Attempt to send the proprietary message
            bool sent = NMEA2K_SetTxRequest(BSP_CAN_RVC, PROP_MSG_DGN, 0);

            if (!sent)
            {
                // Transmission failed, handle retry
                if (msg.retries > 0)
                {
                    msg.retries--;
                    // Requeue for retry
                    if (xQueueSendToFront(prop_message_queue, &msg, 0) != pdPASS)
                    {
                        LOG(W, "Proprietary message queue full, dropping message DA=%02X", msg.da);
                    }
                    else
                    {
                        LOG(D, "Proprietary message DA=%02X requeued, %d retries left", msg.da, msg.retries);
                    }
                }
                else
                {
                    // No more retries left
                    LOG(W, "Proprietary message DA=%02X dropped", msg.da);
                }

                // Break out of the loop if sending failed, to try again later
                break;
            }
            else
            {
                // Successfully sent
                messages_sent++;
                processed++;

                LOG(D, "Sent proprietary message #%d to DA=%02X (queue has %d msgs)",
                    messages_sent, msg.da, uxQueueMessagesWaiting(prop_message_queue));

                // Only send one message per TX_READY (except for first bootstrap message)
                if (messages_sent > 1)
                {
                    break;
                }
            }
        }
    }

    if (processed > 0)
    {
        LOG(D, "Processed %d proprietary messages", processed);
    }
}

//! \~ Initialize CAN
static int initialize_can_driver(void)
{
    int ret = 0;
    LOG(I, "Starting CAN...");

    // Install CAN driver
    if (HALCAN_Start(BSP_CAN_RVC))
    {
        LOG(D, "CAN started");
        ret = 1;
    }
    else
    {
        LOG(E, "Failed to install CAN driver");
    }
    return ret;
}

static void initialize_can_gpio(void)
{
    // Pin is setup by HAL according to board configuration

    /* Pin is connected to CAN EN/STB pin depending on CAN tranceiver used. Enable CAN / disable sleep. */
    // gpio_set_level(CAN_SLEEP_PIN, !CAN_SLEEP_LEVEL);
    CAN_SLEEP(!CAN_SLEEP_LEVEL);
}

/* This function is called when CAN TX is failing.
 * When no communication is available towards the controller we here
 * assume that no error codes should be saved and we use the first error code
 * parameter for HMI display. When the error disapear we also assume we can
 * clear the critical error fault.
 */
static void can_error_cb(int32_t error_code)
{
    // TODO Use generic class error parameter
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    uint8_t __attribute__((unused)) warn;
    PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_1, error_code);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, HTR0ERRCD1, &error_code, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);

    // Normalize
    error_code = !!error_code;

    LOG(W, "Critical error=%d", (int)error_code);

    // Read warning state
    warn = PIMAGE_u8GetValue(VAR_DGN_130553_WARNING_FAULT_ACTIVE);

    error_code <<= 1;
    error_code |= warn;

    // Publish read warning and critical status
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, HTR0ERRST, &error_code, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
#endif
}

static void RVC_Library_Initialize(void)
{
    // Initialize PI
    PIMAGE_Initialize();

    // Initialize CAN
    HALCAN_Initialize(can_error_cb);

    // Update SWId on CAN
    const char *version = gateway_get_firmware_version();
    MSGCAN_UpdateSWId(0, (void *)version, strlen(version));
#ifdef CONNECTOR_HMI
    const char *hmi_version = hmi_data_get_version();
    MSGCAN_UpdateSWId(1, (void *)hmi_version, strlen(hmi_version));
#else
    MSGCAN_UpdateSWId(1, " ", 1);  // Not used, use one space
#endif
    MSGCAN_UpdateSWId(2, " ", 1);  // Not used, use one space
    MSGCAN_UpdateSWId(3, " ", 1);  // Not used, use one space

    // Initialize CAN mail boxes
    // TODO: Define parameters below
    MSGCAN_Initialize(RVC_INITIAL_SOURCE_ADDRESS, 12345678, 0x01, status_changed_cb);
    MSGCAN_SetRxCallback(rvc_rx_cb);
    MSGCAN_SetTxCallback(rvc_tx_cb);
#ifdef CONNECTOR_RVC_INCLUDE_RAW_RVC_MP
    NMEA2K_SetMPRawDataCallback(rvc_mp_rx_cb);
#endif
#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
    NMEA2K_SetStandardDGNCallback(rvc_std_rx_cb);
#endif
}

#ifdef CONFIG_POWER_MANAGEMENT
static void rx_isr_handler(void *args)
{
    (void)args;
    BaseType_t higher_priority_task_woken = pdFALSE;

    gpio_intr_disable(DEVICE_TWAI_RX);

    // Wakeup can_txrx_task.
    if (can_txrx_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(can_txrx_task_handle, &higher_priority_task_woken);
    }

    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}
#endif

/**
 * @brief Check if SA exists in tracking array, add if not found
 *
 * @param sa Source address to check and add
 * @return true if SA was already in array, false if it was newly added
 */
static bool MAYBE_UNUSED check_and_add_sa_to_tracking(uint8_t sa)
{
    if (sa == NMEA2K_GLOBAL_ADDRESS)
    {
        return true;  // Don't track global address
    }

    // Check if SA already exists
    for (size_t i = 0; i < MAX_NUM_DEVICES; i++)
    {
        if (device_SA[i] == sa)
        {
            return true;  // Already exists
        }
    }

    // If not found, add to first available slot
    for (size_t i = 0; i < MAX_NUM_DEVICES; i++)
    {
        if (device_SA[i] == 0)
        {
            device_SA[i] = sa;
            LOG(D, "Added new SA: %02X to slot %d", sa, i);
            return false;  // Newly added
        }
    }

    LOG(W, "Device tracking array is full, cannot add SA: %02X", sa);
    return true;  // Array full, treat as "found" to avoid repeated requests
}

static void remove_sa_from_tracking(uint8_t sa)
{
    for (size_t i = 0; i < MAX_NUM_DEVICES; i++)
    {
        if (device_SA[i] == sa)
        {
            device_SA[i] = 0;
            LOG(D, "Removed SA: %02X from slot %d", sa, i);
            return;
        }
    }
}

static void update_sa_in_tracking(uint8_t old_sa, uint8_t new_sa)
{
    for (size_t i = 0; i < MAX_NUM_DEVICES; i++)
    {
        if (device_SA[i] == old_sa)
        {
            device_SA[i] = new_sa;
            LOG(D, "Updated SA from %02X to %02X in slot %d", old_sa, new_sa, i);
            return;
        }
    }
}

static int initialize_connector(void)
{
    int result = 0;

    DgnNodeInit();
    RVC_Library_Initialize();

    int err = 0;
    err = ProdDBInit();
    if (err != 0)
    {
        LOG(E, "ProdDBInit returned error %d", err);
    }
    err = product_conf_manager_init();
    if (err != 0)
    {
        LOG(E, "product_conf_manager_init returned error %d", err);
    }
    if (initialize_can_driver())
    {
        initialize_can_gpio();
#ifdef CONFIG_POWER_MANAGEMENT
        hal_can_pause((HAL_CAN_DEVICE)BSP_CAN_RVC, true);
#endif
        callback_accept_handle = xEventGroupCreate();

        TRUE_CHECK(timeout_timer = xTimerCreate(NULL, pdMS_TO_TICKS(3000), pdFALSE, (void *)0, feedback_handler));
#ifdef TEST_RESPONSE
        TRUE_CHECK(test_timeout_timer = xTimerCreate("Test timeout", pdMS_TO_TICKS(3000), pdFALSE, (void *)0, test_feedback_handler));
#endif
        iso_request_queue = xQueueCreateStatic(ISO_REQUEST_QUEUE_LENGTH, ISO_REQUEST_QUEUE_ITEM_SIZE, iso_request_queue_storage, &iso_request_q);
        if (iso_request_queue == NULL)
        {
            LOG(E, "ISO request queue cannot be created");
        }

        prop_message_queue = xQueueCreateStatic(PROP_MESSAGE_QUEUE_LENGTH, PROP_MESSAGE_QUEUE_ITEM_SIZE, prop_message_queue_storage, &prop_message_q);
        if (prop_message_queue == NULL)
        {
            LOG(E, "Proprietary message queue cannot be created");
        }

        memset(device_SA, 0, sizeof(device_SA));

        TRUE_CHECK(xTaskCreate(rvc_process_task, "rvc_ddmp", 4096, NULL, xTASK_PRIORITY_NORMAL, NULL));

        TRUE_CHECK(xTaskCreate(rvc_txrx_task, "rvc_txrx", 4096, NULL, xTASK_PRIORITY_ABOVE_NORMAL, &can_txrx_task_handle));

        // Set default value so that callbacks are accepted = 1
        xEventGroupSetBits(callback_accept_handle, CB_TARGET_TEMP_NOT_BLOCKED);
#ifdef CONFIG_POWER_MANAGEMENT
        // Register RX ISR
        /* DEVICE_TWAI_RX is setup by HAL, here it is reconfigured and added interrupt type and interrupt handler */
        TRUE_CHECK(gpio_isr_handler_add(DEVICE_TWAI_RX, rx_isr_handler, NULL) == ESP_OK);
        TRUE_CHECK(gpio_set_intr_type(DEVICE_TWAI_RX, GPIO_INTR_NEGEDGE) == ESP_OK);
        TRUE_CHECK(gpio_intr_enable(DEVICE_TWAI_RX) == ESP_OK);
#endif
        fault_mutex = xSemaphoreCreateMutex();
        if (fault_mutex != NULL)
        {
            TRUE_CHECK(check_dmrv_timer = xTimerCreate(NULL, pdMS_TO_TICKS(1000), pdTRUE, (void *)0, check_dmrv_timer_handler));
            xTimerStart(check_dmrv_timer, portMAX_DELAY);
        }
        else
        {
            LOG(E, "Fault mutex cannot be created");
        }
        rvc_heartbeat_mutex = xSemaphoreCreateMutex();
        if (rvc_heartbeat_mutex != NULL)
        {
            TRUE_CHECK(rvc_heartbeat_timer = xTimerCreate(NULL, pdMS_TO_TICKS(1000), pdTRUE, (void *)0, rvc_heartbeat_timer_handler));
            xTimerStart(rvc_heartbeat_timer, portMAX_DELAY);
        }
        else
        {
            LOG(E, "RVC heartbeat mutex cannot be created");
        }

        TRUE_CHECK(rvc_request_timer = xTimerCreate(NULL, pdMS_TO_TICKS(5000), pdTRUE, (void *)0, rvc_request_timer_handler));
        xTimerStart(rvc_request_timer, portMAX_DELAY);
        result = 1;
    }

    return result;
}

static bool rvc_rx_cb(uint32_t dgn, uint8_t sa, uint8_t *p_data, size_t size)
{
    // Publish received data
    bool is_resolved = false;
    uint8_t temp_array[DDMP2_MAX_VALUE_SIZE];
    RVCMGNT0RAWRX_T *l_dgn = (RVCMGNT0RAWRX_T *)&temp_array[0];
    l_dgn->is_ext_type = true;  // Extended
    l_dgn->addr = sa;
    l_dgn->dgn = dgn;
    if (size > (DDMP2_MAX_VALUE_SIZE - sizeof(RVCMGNT0RAWRX_T)))
    {
        LOG(E, "Received size too large: %d", size);
        return false;
    }
    memcpy(l_dgn->data, p_data, size);

    TRUE_CHECK(xSemaphoreTake(rvc_heartbeat_mutex, portMAX_DELAY));

    DgnNode_t *dgn_node = DgnNodeFindBySourceAddress(&l_prod, sa);
    if (dgn_node == NULL)
    {
        // New device source address detected
        // Create a new
        device_node_data_t node_data = {0};
        node_data.device.dsa = 0;
        node_data.device.sa = sa;
        node_data.state = DEVICE_STATE_WAIT_DMRV;
        dgn_node = DgnNodeCreate(INVALID_RVC_INSTANCE, 0, sa, &node_data, sizeof(device_node_data_t));
        if (dgn_node)
        {
            DgnNodeInsert(dgn_node, &l_prod);
            LOG(D, "Requesting DMRV (trig by dgn:%d) for SA: 0x%02X", dgn, sa);
            enqueue_iso_request(sa, DMRV_MSG_DGN, 3);
        }
        else
        {
            LOG(E, "Failed to create dgn_node for sa: 0x%02x", sa);
        }
    }
    else
    {
        switch (((device_node_data_t *)dgn_node->dgn_data)->state)
        {
        case DEVICE_STATE_WAIT_DMRV:
            // If DMRV -> request PID
            if (dgn == DMRV_MSG_DGN)
            {
                enqueue_iso_request(sa, PRODUCT_ID_MSG_DGN, 3);
                LOG(D, "Requesting PID for SA: 0x%02X", sa);
                ((device_node_data_t *)dgn_node->dgn_data)->rvc_heartbeat = 0;
                is_resolved = true;
            }
            break;
        case DEVICE_STATE_WAIT_PID:
            // PID -> let PID rx handler create PROD class
            if (dgn == PRODUCT_ID_MSG_DGN)
            {
                ((device_node_data_t *)dgn_node->dgn_data)->rvc_heartbeat = 0;
            }
            break;
        case DEVICE_STATE_RESOLVED:
            // Reset heartbeat timer for the existing device on any message
            ((device_node_data_t *)dgn_node->dgn_data)->rvc_heartbeat = 0;
            is_resolved = true;
            break;
        default:
            break;
        }
    }
    xSemaphoreGive(rvc_heartbeat_mutex);
    // Determine if RVCMGNT0RAWRX should be published
    bool should_publish_rawrx = false;

    if (dgn == DMRV_MSG_DGN)
    {
        // Always publish DMRV
        should_publish_rawrx = true;
    }
    else if (dgn == PRODUCT_ID_MSG_DGN)
    {
        // Only publish PID if device is whitelisted
        RVCDGN_zDGN_65259 zDGN;
        RVCDGN_DGN_65259_Extract(&zDGN, p_data, size);
        if (product_conf_manager_find_model(zDGN.cMake, zDGN.cModel))
        {
            LOG(I, "Device PID whitelisted: Make='%s', Model='%s', SN='%s'", zDGN.cMake, zDGN.cModel, zDGN.cSerialNumber);
            should_publish_rawrx = true;
        }
    }

    if ((should_publish_rawrx) || (is_resolved))
    {
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCMGNT0RAWRX, l_dgn, sizeof(RVCMGNT0RAWRX_T) + size, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }

    // Loop through common receive functions
    for (int i = 0; i < (int)ELEMENTS(l_rvc_transceiver_dgn_table); i++)
    {
        if (l_rvc_transceiver_dgn_table[i].dgn_rx == dgn)
        {
            if ((l_rvc_transceiver_dgn_table[i].receive_function) && ((is_resolved) || (dgn == PRODUCT_ID_MSG_DGN)))
            {
                return l_rvc_transceiver_dgn_table[i].receive_function(p_data, sa, size);
            }
            break;
        }
    }
    return false;
}

static bool rvc_tx_cb(uint32_t dgn, uint8_t instance, uint8_t *p_data)
{
    for (int i = 0; i < (int)ELEMENTS(l_rvc_transceiver_dgn_table); i++)
    {
        if ((l_rvc_transceiver_dgn_table[i].dgn_tx == dgn) && l_rvc_transceiver_dgn_table[i].transmit_function)
        {
            return l_rvc_transceiver_dgn_table[i].transmit_function(instance, p_data);
        }
    }
    return false;
}

#ifdef CONNECTOR_RVC_INCLUDE_RAW_RVC_MP
static void rvc_mp_rx_cb(uint32_t canid, uint8_t size, uint8_t *p_data)
{
    // This will only forward the MP rx frames as is
    // Publish received data
    uint8_t temp_array[DDMP2_MAX_VALUE_SIZE];
    RVCMGNT0RAWRX_T *l_dgn = (RVCMGNT0RAWRX_T *)&temp_array[0];
    l_dgn->is_ext_type = true;  // Extended
    l_dgn->addr = canid & 0xFF;
    l_dgn->dgn = (canid >> 8) & 0x3FFFF;
    if (size > (DDMP2_MAX_VALUE_SIZE - sizeof(RVCMGNT0RAWRX_T)))
    {
        LOG(E, "Received size too large: %d", size);
        return;
    }
    memcpy(l_dgn->data, p_data, size);
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCMGNT0RAWRX, l_dgn, sizeof(RVCMGNT0RAWRX_T) + size, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
}
#endif

#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
static void rvc_std_rx_cb(uint32_t canid, uint8_t size, uint8_t *p_data)
{
    uint8_t temp_array[DDMP2_MAX_VALUE_SIZE];
    RVCMGNT0RAWRX_T *l_dgn = (RVCMGNT0RAWRX_T *)&temp_array[0];
    l_dgn->is_ext_type = false;  // Standard
    l_dgn->addr = 0xFF;
    l_dgn->dgn = canid;
    if (size > (DDMP2_MAX_VALUE_SIZE - sizeof(RVCMGNT0RAWRX_T)))
    {
        LOG(E, "Received size too large: %d", size);
        return;
    }
    memcpy(l_dgn->data, p_data, size);
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCMGNT0RAWRX, l_dgn, sizeof(RVCMGNT0RAWRX_T) + size, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
}
#endif

#ifdef CONFIG_POWER_MANAGEMENT
static bool can_transfers_pending(void)
{
    bool pending = false;

    if (((HALCAN_TxSize(HAL_CAN_DEVICE_TWAI) - HALCAN_TxFree(HAL_CAN_DEVICE_TWAI)) > 0) ||
        ((HALCAN_RxSize(HAL_CAN_DEVICE_TWAI) - HALCAN_RxFree(HAL_CAN_DEVICE_TWAI)) > 0) ||
        NMEA2K_OngoingMPSession())
    {
        pending = true;
    }

    return pending;
}
#endif

#ifdef TWAI_EXTENDED_LOG
void twai_logs(void)
{
    twai_status_info_t status_info;
    TRUE_CHECK(twai_get_status_info(&status_info) == ESP_OK);
    LOG(I, "LOG TWAI status %d: txerr = %d, rxerr = %d, txfail = %d, rxfail = %d, rxover = %d, arblos = %d, buserr = %d, txq = %d, rxq = %d",
        status_info.state,
        status_info.tx_error_counter,
        status_info.rx_error_counter,
        status_info.tx_failed_count,
        status_info.rx_missed_count,
        status_info.rx_overrun_count,
        status_info.arb_lost_count,
        status_info.bus_error_count,
        status_info.msgs_to_tx,
        status_info.msgs_to_rx);
    get_print_twai_bus_error();
}
#endif /* CAN_EXTENDED_LOG */

static bool request_rvc_on_timer(DgnNode_t *dgn_node, void *arg)
{
    bool ret_val = false;
#if defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7) && defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8) && defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9) &&    \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10) && defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12) && defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13) && \
    defined(RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_3)
    int ddm_instance = dgn_node->ddm_instance;
    PROD0PROP_T prodprop;
    size_t prop_size = sizeof(PROD0PROP_T);
    ProdDBReadCache(FIELD_PROP, ddm_instance, &prodprop, &prop_size);
    prodxprop_type_t prop_type_raw = {0};
    prop_type_raw.data = prodprop.type;

    if ((prop_type_raw.type.cls == PRODXPROP_TYPE_CLASS_POWER) && (prop_type_raw.type.intf == PRODXPROP_TYPE_INTERFACE_RVC))
    {
        // Check detailed type
        int32_t outtype = ProdDBGetProductType(ddm_instance);
        if ((outtype == PRODDB_PRODUCTTYPE_SHUNT) || (outtype == PRODDB_PRODUCTTYPE_BATTERY))
        {
            enqueue_iso_request(prodprop.addr, DC_SOURCE_STATUS_7_DGN, 1);   // DC_SOURCE_STATUS_7
            enqueue_iso_request(prodprop.addr, DC_SOURCE_STATUS_8_DGN, 1);   // DC_SOURCE_STATUS_8
            enqueue_iso_request(prodprop.addr, DC_SOURCE_STATUS_9_DGN, 1);   // DC_SOURCE_STATUS_9
            enqueue_iso_request(prodprop.addr, DC_SOURCE_STATUS_10_DGN, 1);  // DC_SOURCE_STATUS_10
            enqueue_iso_request(prodprop.addr, DC_SOURCE_STATUS_12_DGN, 1);  // DC_SOURCE_STATUS_12
            enqueue_iso_request(prodprop.addr, DC_SOURCE_STATUS_13_DGN, 1);  // DC_SOURCE_STATUS_13
        }

        if ((outtype == PRODDB_PRODUCTTYPE_INVERTER) || (outtype == PRODDB_PRODUCTTYPE_INVERTERCHARGER))
        {
            enqueue_iso_request(prodprop.addr, INVERTER_CONFIGURATION_STATUS_3_DGN, 1);  // INVERTER_CONFIGURATION_STATUS_3
        }
    }
#endif
    return ret_val;
}

static bool check_heartbeat_timer(DgnNode_t *dgn_node, void *arg)
{
    if ((dgn_node->source_address != 0) && dgn_node->dgn_data)
    {
        device_node_data_t *p_device_node = (device_node_data_t *)dgn_node->dgn_data;
        p_device_node->rvc_heartbeat++;
        if (p_device_node->rvc_heartbeat > 5)
        {
            LOG(W, "Clearing device slot for SA %02X DSA %02X", dgn_node->source_address, p_device_node->device.dsa);
            LOG(W, "Clearing DGN node for SA %02X", dgn_node->source_address);
            if ((dgn_node->ddm_instance != INVALID_DDM_INSTANCE) && (dgn_node->ddm_instance > 0))
            {
                make_prod_prop_classes_unavailable(dgn_node->ddm_instance);
                ProdDBProdClassNodeDelete(dgn_node->ddm_instance);
            }
            remove_sa_from_tracking(dgn_node->source_address);
            DgnNodeDelete(dgn_node);
        }
    }
    return false;
}

static bool check_dmrv_timer_fcn(DgnNode_t *dgn_node, void *arg)
{
    uint32_t now = hal_cpu_get_millis();
    TRUE_CHECK(xSemaphoreTake(fault_mutex, portMAX_DELAY));
    if (!dgn_node->dgn_data || ((device_node_data_t *)dgn_node->dgn_data)->device.dsa == 0)
    {
        // Skip
    }
    else
    {
        device_entry_t *dev = &(((device_node_data_t *)dgn_node->dgn_data)->device);
        for (size_t j = 0; j < dev->num_faults; j++)
        {
            fault_info_t *f = &dev->faults[j];
            if ((f->is_active) && (now - f->last_seen_time > FAULT_TIMEOUT_MS))
            {
                LOG(D, "Clearing fault SPN %d for SA %02X DSA %02X", f->spn, dgn_node->source_address, dev->dsa);
                f->is_active = false;
                f->red_lamp = false;
                f->yellow_lamp = false;
                f->fmi = 0;
                f->occ_cnt = 0;
                f->dsa_ext = 0;
            }
        }
    }
    xSemaphoreGive(fault_mutex);
    return false;
}

static void rvc_process_task(void *Parameter)
{
    DDMP2_FRAME *ddmp_msg;
    size_t msg_size;

    install_parameters();

    // Publish own version?
    // setOwnVersion();

    // Get Heater FW Version IDs
    // TODO Move to heater implementation

    for (;;)
    {
        // Wait for new DDMP events
        ddmp_msg = (DDMP2_FRAME *)connector_wait_for_frame(&connector_rvc, &msg_size);

#ifdef RVC_EXTENDED_LOG
        ESP_LOG_BUFFER_HEXDUMP("DDMP->RVC  ", (uint8_t *)ddmp_msg, ddmp_msg->frame_size + DDMP2_METADATA_SIZE, ESP_LOG_DEBUG);

        LOG(D, "RV-C action 0x%x", ddmp_msg->frame.control);
#endif

        if (!ProdDBFrameHandler(ddmp_msg))
        {
            if ((ddmp_msg->frame.control == DDMP2_CONTROL_SET) || (ddmp_msg->frame.control == DDMP2_CONTROL_SUBSCRIBE))
            {
                for (uint32_t i = 0; i < ELEMENTS(l_rvc_transceiver_dgn_table); i++)
                {
                    if (DDM2_PARAMETER_CLASS(ddmp_msg->frame.set.parameter) == l_rvc_transceiver_dgn_table[i].ddm_class)
                    {
                        // Class to handle
                        if (l_rvc_transceiver_dgn_table[i].class_handle_function)
                        {
                            l_rvc_transceiver_dgn_table[i].class_handle_function(l_rvc_transceiver_dgn_table[i].dgn_tx, ddmp_msg);
                        }
                        break;
                    }
                }
            }
            else if (ddmp_msg->frame.control == DDMP2_CONTROL_PUBLISH)
            {
                /* No PUBLISH frames expected to be received, since this connector does not subscribe to any parameters */
            }
            else if (ddmp_msg->frame.control == DDMP2_CONTROL_GENERIC)
            {
                switch (ddmp_msg->frame.generic.id)
                {
                case CONNECTOR_RVC_CHECK_DMRV_TIMER_EVENT:
                    TRUE_CHECK(xSemaphoreTake(rvc_heartbeat_mutex, portMAX_DELAY));
                    DgnNodeLoopAndExecute(&l_prod, check_dmrv_timer_fcn, NULL);
                    xSemaphoreGive(rvc_heartbeat_mutex);
                    break;
                case CONNECTOR_RVC_HEARTBEAT_TIMER_EVENT:
                    TRUE_CHECK(xSemaphoreTake(rvc_heartbeat_mutex, portMAX_DELAY));
                    DgnNodeLoopAndExecute(&l_prod, check_heartbeat_timer, NULL);
                    xSemaphoreGive(rvc_heartbeat_mutex);
                    break;
                case CONNECTOR_RVC_REQUEST_TIMER_EVENT:
                    TRUE_CHECK(xSemaphoreTake(rvc_heartbeat_mutex, portMAX_DELAY));
                    DgnNodeLoopAndExecute(&l_prod, request_rvc_on_timer, NULL);
                    xSemaphoreGive(rvc_heartbeat_mutex);
                    break;
                default:
                    LOG(W, "Invalid generic connector RV-C event");
                    break;
                }
            }
        }
        vRingbufferReturnItem(connector_rvc.to_connector, ddmp_msg);
    }
}

/**
 * @brief Make all classes in PRODXPROP parameter unavailable before removing product node
 *
 * @param ddm_instance DDM instance of the product node
 */
static void make_prod_prop_classes_unavailable(int ddm_instance)
{
    if (ddm_instance == INVALID_DDM_INSTANCE)
    {
        return;
    }

    // Read number of linked proprietary classes
    int no_linked_prop = 0;
    size_t no_linked_prop_size = sizeof(no_linked_prop);
    ProdDBReadCache(FIELD_NO_OF_LINKED_PROP_CLASSES, ddm_instance, &no_linked_prop, &no_linked_prop_size);

    if ((no_linked_prop_size == 0) || (no_linked_prop == 0))
    {
        LOG(D, "No linked classes to remove for instance %d", ddm_instance);
        return;
    }

    // Allocate memory for PRODXPROP structure
    size_t prop_size = sizeof(PROD0PROP_T) + no_linked_prop * sizeof(uint32_t);
    PROD0PROP_T *p_prodprop = (PROD0PROP_T *)hal_mem_malloc_prefer(prop_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);

    if (p_prodprop == NULL)
    {
        LOG(E, "Cannot allocate memory for PROD0PROP_T");
        return;
    }

    // Read PRODXPROP field
    ProdDBReadCache(FIELD_PROP, ddm_instance, p_prodprop, &prop_size);

    if (prop_size == 0)
    {
        LOG(W, "No property data found for instance %d", ddm_instance);
        hal_mem_free(p_prodprop);
        return;
    }

    LOG(D, "Making %d linked classes unavailable for DDM instance %d", no_linked_prop, ddm_instance);
    prodxprop_type_t prop_type_raw = {0};
    prop_type_raw.data = p_prodprop->type;

    if ((prop_type_raw.type.cls == PRODXPROP_TYPE_CLASS_POWER) && (prop_type_raw.type.intf == PRODXPROP_TYPE_INTERFACE_RVC))
    {
        remove_dcsource_nodes(p_prodprop->addr);
        remove_battery_nodes(p_prodprop->addr);
        remove_generic_rvc_nodes(p_prodprop->addr);
        remove_solar_charge_controller_nodes(p_prodprop->addr);
        remove_inverter_charger_nodes(p_prodprop->addr);
    }
    // Iterate through linked classes and make them unavailable
    UPDLINKEDCLASS_T *remove_class = hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    for (int i = 0; i < no_linked_prop; i++)
    {
        if (remove_class != NULL)
        {
            remove_class->update[0] = 0;
            remove_class->updclass = p_prodprop->classes[i];
            ProdDBUpdateCache(remove_class, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), FIELD_PROP_CLASS, ddm_instance);
        }
    }
    hal_mem_free(remove_class);

    ProdDBUpdateCache(p_prodprop, 0, FIELD_PROP, ddm_instance);
    hal_mem_free(p_prodprop);

    LOG(D, "All linked classes made unavailable for instance %d", ddm_instance);
}

static void rvc_txrx_task(void *Parameter)
{
#ifdef CONFIG_POWER_MANAGEMENT
    uint8_t backoff_idx = 0;
    uint16_t notify_counter = 0;
    CAN_STATUS_t can_status = CAN_STATUS_UNKNOWN;

    for (;;)
    {
        switch (can_status)
        {
        case CAN_STATUS_UNKNOWN:
            // Waiting for bus activity
#ifdef CAN_EXTENDED_LOG
            LOG(D, "Looking for traffic.");
#endif
            if (ulTaskNotifyTake(pdTRUE, activity_detect_timeout) > 0)
            {
                // Activity detected
#ifdef CAN_EXTENDED_LOG
                LOG(D, "Unknown -> Active");
#endif
                can_status = CAN_STATUS_ACTIVE;
                backoff_idx = 0;

                // Disable interrupts and enable driver
                hal_can_resume((HAL_CAN_DEVICE)BSP_CAN_RVC, true);
            }
            else
            {
                // No activity, disable transceiver and wait
#ifdef CAN_EXTENDED_LOG
                LOG(D, "No activity. Sleeping.");
#endif
                CAN_SLEEP(CAN_SLEEP_LEVEL);
                vTaskDelay(retry_backoff_ticks[backoff_idx]);

                if (backoff_idx < (ELEMENTS(retry_backoff_ticks) - 1))
                {
                    backoff_idx++;
                }
                CAN_SLEEP(!CAN_SLEEP_LEVEL);
            }
            break;

        case CAN_STATUS_ACTIVE:
            // Process timers
            Main_ProcessTimers();
            // Inputs
            NMEA2K_ProcessRx();

            // Task 10 ms
            if (main_u16Ticks10ms >= 10)
            {
                // LOG(P, "main_u16Ticks10ms = %d", main_u16Ticks10ms);
                NMEA2K_UpdateTimers(main_u16Ticks10ms);
                MSGCAN_Process(main_u16Ticks10ms);
                main_u16Ticks10ms = 0;
            }

            // Task 1000 ms
            if (main_u16Ticks1000ms >= 1000)
            {
                main_u16Ticks1000ms -= 1000;
            }

            //  Outputs
            NMEA2K_ProcessTx();
            // Switch state when no transfers are pending

#ifdef TWAI_EXTENDED_LOG
            twai_logs();
#endif
            if (can_transfers_pending())
            {
                // Can't sleep communication ongoing
            }
            else
            {
                // Enable interrupts and pause driver
#ifdef CAN_EXTENDED_LOG
                LOG(D, "Active -> Inactive");
#endif
                can_status = CAN_STATUS_INACTIVE;
                TRUE_CHECK(gpio_intr_enable(DEVICE_TWAI_RX) == ESP_OK);
                hal_can_pause((HAL_CAN_DEVICE)BSP_CAN_RVC, false);
            }
            break;

        case CAN_STATUS_INACTIVE:
            // Let task sleep until next RX interrupt
            if (ulTaskNotifyTake(pdTRUE, inactive_to_unknown_timeout) > 0)
            {
                // Traffic detected, unpause driver
#ifdef CAN_EXTENDED_LOG
                LOG(D, "Inactive -> Active");
#endif
                main_u16Elapsed_ms = notify_counter = 0;
                can_status = CAN_STATUS_ACTIVE;
                hal_can_resume((HAL_CAN_DEVICE)BSP_CAN_RVC, false);
            }
            else
            {
                Main_ProcessTimers();
                // Task 10 ms
                if (main_u16Ticks10ms >= 10)
                {
                    NMEA2K_UpdateTimers(main_u16Ticks10ms);
                    MSGCAN_Process(main_u16Ticks10ms);
                    main_u16Ticks10ms = 0;
                }
                /* Go to RVC unknown state after 1 second of inactivity */
                if (notify_counter >= TIMER_COUNT_2_S)
                {
                    // No traffic detected after a timeout
#ifdef CAN_EXTENDED_LOG
                    LOG(D, "Inactive -> Unknown");
#endif
                    notify_counter = 0;
                    main_u16Elapsed_ms = 0;
                    can_status = CAN_STATUS_UNKNOWN;
                    // Enable interrupts and disable driver
                    TRUE_CHECK(gpio_intr_enable(DEVICE_TWAI_RX) == ESP_OK);
                    hal_can_stop((HAL_CAN_DEVICE)BSP_CAN_RVC);
                }
                notify_counter = main_u16Elapsed_ms;
            }
            break;

        default:
            can_status = CAN_STATUS_UNKNOWN;
            break;
        }
    }
#else
    for (;;)
    {
        if (!sleep_indication)
        {
            // Process timers
            Main_ProcessTimers();

            // Inputs
            NMEA2K_ProcessRx();
            process_iso_request_queue();
            process_prop_msg_queue();
            // // Task 1 ms
            // if (main_u16Ticks1ms >= 1)
            // {
            // 	NMEA2K_UpdateTimers(main_u16Ticks1ms);
            // 	main_u16Ticks1ms = 0;
            // }

            // Task 10 ms
            if (main_u16Ticks10ms >= 10)
            {
                NMEA2K_UpdateTimers(main_u16Ticks10ms);
                MSGCAN_Process(main_u16Ticks10ms);
                main_u16Ticks10ms = 0;
            }

            // Task 1000 ms
            if (main_u16Ticks1000ms >= 1000)
            {
                main_u16Ticks1000ms -= 1000;
            }

            //  Outputs
            NMEA2K_ProcessTx();

            vTaskDelay(1);
        }
        else
        {
            /* CAN enable/sleep pin. Enable sleep/standby mode. */
            CAN_SLEEP(CAN_SLEEP_LEVEL);
        }
    }
#endif
}

//-----------------------------------------------------------------------------
// Function:    Main_ProcessTimers
//-----------------------------------------------------------------------------
// Description: Update main timers.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Main_ProcessTimers(void)
{
    // Extract elapsed time since the last call
    uint16_t u16Elapsed;
    uint16_t u16Now = hal_cpu_get_millis();

    // LOG(I, "u16Now=%d", u16Now);

    if (main_u16TicksInit == 0)
    {
        u16Elapsed = 0;
        main_u16TicksInit = 1;
    }
    else
    {
        /* Handle wrap around of timer */
        if (main_u16TicksSys <= u16Now)
        {
            u16Elapsed = u16Now - main_u16TicksSys;
        }
        else
        {
            u16Elapsed = 0xFFFF - main_u16TicksSys + u16Now + 1;
        }
    }

    main_u16TicksSys = u16Now;

    // Update timers
    main_u16Elapsed_ms += u16Elapsed;
    main_u16Ticks10ms += u16Elapsed;
    main_u16Ticks1000ms += u16Elapsed;
}

void request_info_request_dgn(uint32_t dgn, uint8_t da)
{
    if (NMEA2K_SetTxRequest(0, INFORMATION_REQUEST_DGN, 0))
    {
        l_59904_dgn.u32DGN = dgn;
        l_59904_dgn.da = da;
    }
}

uint8_t get_dimm_instance_app(void)
{
    return dimm_instance;
}

void set_dimm_instance_app(uint8_t instance)
{
    dimm_instance = instance;
}
uint8_t get_cur_oper(void)
{
    return cur_operation;
}

void set_cur_oper(uint8_t cur_op)
{
    cur_operation = cur_op;
}

/********************************************************************************************************/
/*                       DGN Receive/Transmit/Handler functions                                         */
/********************************************************************************************************/
/**
 * @brief RVCMGNT0 class set handler function
 *
 * Handles the SET/SUBSRIBE on entire class RVCMGNT0
 *
 * @param dgn Transmit dgn, not used
 * @param p_frame Received DDM2P frame
 */
static void handleRVCMGNT0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    (void)dgn;
    // Handle RVC Manager class. Only instance 0
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (p_frame->frame.subscribe.parameter)
        {
        case RVCMGNT0ADDR:
        {
            MsgCan_Address_Claimed_Update();
            break;
        }
        default:
            break;
        }
    }
    else
    {
        switch (p_frame->frame.set.parameter)
        {
        case RVCMGNT0RAW:
        {
            RVCMGNT0RAWTX_T *txmsg;

            txmsg = (RVCMGNT0RAWTX_T *)p_frame->frame.set.value.raw;
            // Bypass library calls
            HALCAN_zMSG msg;
            if (txmsg->is_ext_type)
            {
                // Extended
                msg.eIdType = HALCAN_IDTYPE_EXTENDED;
                msg.u8Length = sizeof(txmsg->data);
                memcpy(msg.au8Data, txmsg->data, sizeof(txmsg->data));
                // 29 bits
                msg.u32Id = (txmsg->dgn << 8) | txmsg->addr;
                msg.u32Id |= (6 << 26);  // Priority 6 default
                HALCAN_Write(BSP_CAN_RVC, &msg);
            }
#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
            else
            {
                msg.eIdType = HALCAN_IDTYPE_STANDARD;
                msg.u8Length = ddmp2_value_size(p_frame) - sizeof(RVCMGNT0RAWRX_T);
                memcpy(msg.au8Data, txmsg->data, msg.u8Length);
                // 11 bits
                msg.u32Id = txmsg->dgn;
                HALCAN_Write(BSP_CAN_RVC, &msg);
            }
#endif
            break;
        }
        case RVCMGNT0REQ:
        {
            int value;
            RVCMGNT0REQ_T *p_req = (RVCMGNT0REQ_T *)p_frame->frame.set.value.raw;
            enqueue_iso_request(p_req->addr, p_req->dgn, 1);
            value = 1;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.set.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
            break;
        }
        case RVCMGNT0RESETCONF:
        {
            // Parse bitfield
            RVCMGNT0RESETCONF_T *p_reset_conf = (RVCMGNT0RESETCONF_T *)p_frame->frame.set.value.raw;
            l_98048_dgn.u2Reboot = p_reset_conf->reboot;
            l_98048_dgn.u2ClearFaults = p_reset_conf->clear_faults;
            l_98048_dgn.u2ResetToDefault = p_reset_conf->reset_to_default_settings;
            l_98048_dgn.u2ResetStatistics = p_reset_conf->reset_statistics;
            l_98048_dgn.u2TestMode = p_reset_conf->test_mode;
            l_98048_dgn.u2ResetToOEMSettings = p_reset_conf->reset_to_oem_specific_settings;
            l_98048_dgn.u2RebootEnterBootloader = p_reset_conf->enter_bootloader;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.set.parameter, p_reset_conf, sizeof(RVCMGNT0RESETCONF_T), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
            break;
        }
        case RVCMGNT0RESET:
        {
            // Send General reset to given destination address
            int32_t addr = p_frame->frame.set.value.int32;
            if ((addr > 0) && (addr < 256))
            {
                // Set the command frame
                l_98048_dgn.da = (uint8_t)addr;
                NMEA2K_SetTxRequest(BSP_CAN_RVC, GENERAL_RESET_DGN, 0);
            }
            break;
        }
        case RVCMGNT0DOWNLOAD:
        {
            // For speed reasons we send directly
            // Bypass library calls
            HALCAN_zMSG msg;
            RVCMGNT0DOWNLOAD_T *p_dl = (RVCMGNT0DOWNLOAD_T *)p_frame->frame.set.value.raw;
            msg.eIdType = HALCAN_IDTYPE_EXTENDED;
            msg.u8Length = 8;
            memcpy(msg.au8Data, p_dl->data, msg.u8Length);
            // 29 bits
            msg.u32Id = ((DOWNLOAD_DGN | p_dl->addr) << 8) | NMEA2K_GetSourceAddr(BSP_CAN_RVC);
            msg.u32Id |= (7 << 26);  // Priority 7
            HALCAN_Write(BSP_CAN_RVC, &msg);
            break;
        }
        default:
            break;
        }
    }
}

/**
 * @brief Prepare a General reset frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
static bool transmit98048Dgn(uint8_t instance, uint8_t *p_data)
{
    RVCDGN_DGN_98048_Stuff(p_data, &l_98048_dgn);
    if ((l_98048_dgn.da > 0) && (l_98048_dgn.da < 255))
    {
        return true;
    }
    else
    {
        return false;
    }
}

#ifdef RVC_CONFIG_IMPL_DOWNLOAD

/**
 * @brief Download DGN received (97536)
 *
 * @param p_data DGN data pointer
 * @param sa source address
 * @param size size of data
 * @return true if message has been handled
 */
static bool receive97536Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCMGNT0DOWNLOAD_T download_msg;
    if (size != RVCDGN_DGN_97536_SIZE)
    {
        LOG(E, "Unexpected size received %d", size);
        return false;
    }
    download_msg.addr = sa;
    memcpy(download_msg.data, p_data, size);
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCMGNT0DOWNLOAD, &download_msg, sizeof(download_msg), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    return true;
}
#endif

/**
 * @brief ACK DGN received (59392)
 *
 * @param p_data DGN data pointer
 * @param sa source address
 * @param size size of data
 * @return true if message has been handled
 */
static bool receive59392Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCMGNT0ACK_T ack_ddm2;
    RVCDGN_zDGN_59392 l_59392dgn;
    RVCDGN_DGN_59392_Extract(&l_59392dgn, p_data);
    ack_ddm2.ack_code = l_59392dgn.u8Code;
    ack_ddm2.inst = l_59392dgn.u8Instance;
    ack_ddm2.inst_bank = (uint8_t)l_59392dgn.u4instanceBank;
    ack_ddm2.source_addr = l_59392dgn.u8SourceAddress;
    ack_ddm2.dgn = l_59392dgn.u24Dgn;
    ack_ddm2.to_from_addr = sa;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCMGNT0ACK, &ack_ddm2, sizeof(ack_ddm2), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    return true;
}

/**
 * @brief RVCDM0 class set handler function
 *
 * Handles the SET/SUBSRIBE on entire class RVCDM0
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
static void handleRVCDM0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (p_frame->frame.subscribe.parameter)
        {
        case RVCDM0OPST:
        {
            value = (l_130762_dgn.in.u2OperatingStatus2 << 2) | l_130762_dgn.in.u2OperatingStatus1;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0YLAMPST:
        {
            value = l_130762_dgn.in.u2YellowLampStatus;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0RLAMPST:
        {
            value = l_130762_dgn.in.u2RedLampStatus;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0DSA:
        {
            value = l_130762_dgn.in.u8DSA;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0SPN:
        {
            uint32_t spn = 0;
            bool is_multi_instance = false;
            for (size_t i = 0; i < ELEMENTS(multi_instance_DSA); i++)
            {
                if (l_130762_dgn.in.u8DSA == multi_instance_DSA[i])
                {
                    is_multi_instance = true;
                    break;
                }
            }
            // If SPN_MSB == 0, it means it is generic error or universal error for multi instance devices and it should be handled with single-instance parsing
            if ((l_130762_dgn.in.u8SPN_MSB == 0) || (!is_multi_instance))
            {
                spn = ((uint32_t)l_130762_dgn.in.u8SPN_MSB << 11) | ((uint32_t)l_130762_dgn.in.u8SPN_ISB << 3) | (l_130762_dgn.in.u3SPN_LSB & 0x07);
            }
            else if (l_130762_dgn.in.u8SPN_MSB == 1)
            {
                spn = ((uint16_t)l_130762_dgn.in.u8SPN_MSB << 3) | (l_130762_dgn.in.u3SPN_LSB & 0x07);
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &spn, sizeof(spn), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0FMI:
        {
            value = l_130762_dgn.in.u5FMI;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0OCNT:
        {
            value = l_130762_dgn.in.u7OccurCount;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0DSAEXT:
        {
            value = l_130762_dgn.in.u8DSAExtension;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCDM0BANKSEL:
        {
            value = l_130762_dgn.in.u4BankSelect;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        default:
            break;
        }
    }
    else
    {
        switch (p_frame->frame.set.parameter)
        {
        case RVCDM0OPST:
        {
            l_130762_dgn.out.u2OperatingStatus1 = p_frame->frame.set.value.uint32 & 0x0003;
            l_130762_dgn.out.u2OperatingStatus2 = (p_frame->frame.set.value.uint32 >> 2) & 0x0003;
        }
        break;
        case RVCDM0YLAMPST:
        {
            l_130762_dgn.out.u2YellowLampStatus = p_frame->frame.set.value.uint32;
        }
        break;
        case RVCDM0RLAMPST:
        {
            l_130762_dgn.out.u2RedLampStatus = p_frame->frame.set.value.uint32;
        }
        break;
        case RVCDM0DSA:
        {
            l_130762_dgn.out.u8DSA = p_frame->frame.set.value.uint32;
        }
        break;
        case RVCDM0SPN:
        {
            if (p_frame->frame.set.value.uint32 <= 255)
            {
                // Generic SPN
                l_130762_dgn.out.u8SPN_MSB = 0;
                l_130762_dgn.out.u8SPN_ISB = (p_frame->frame.set.value.uint32 >> 3) & 0xFF;
                l_130762_dgn.out.u3SPN_LSB = p_frame->frame.set.value.uint32 & 0x07;
            }
            else
            {
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_IMPL_AIR_CONDITIONER)
                // Send as multiple instance (whether it is a generic or universal error)
                l_130762_dgn.out.u8SPN_MSB = (p_frame->frame.set.value.uint32 >> 11) & 0xFF;
                l_130762_dgn.out.u8SPN_ISB = (p_frame->frame.set.value.uint32 >> 3) & 0xFF;
                l_130762_dgn.out.u3SPN_LSB = p_frame->frame.set.value.uint32 & 0x07;
#else
                // Send as single instance
                l_130762_dgn.out.u8SPN_MSB = (p_frame->frame.set.value.uint32 >> 3) & 0xFF;
                l_130762_dgn.out.u8SPN_ISB = 0;
                l_130762_dgn.out.u3SPN_LSB = p_frame->frame.set.value.uint32 & 0x07;
#endif
            }
        }
        break;
        case RVCDM0FMI:
        {
            l_130762_dgn.out.u5FMI = p_frame->frame.set.value.uint32;
        }
        break;
        case RVCDM0OCNT:
        {
            l_130762_dgn.out.u7OccurCount = p_frame->frame.set.value.uint32;
        }
        break;
        case RVCDM0DSAEXT:
        {
            l_130762_dgn.out.u8DSAExtension = p_frame->frame.set.value.uint32;
        }
        break;
        case RVCDM0BANKSEL:
        {
            l_130762_dgn.out.u4BankSelect = p_frame->frame.set.value.uint32;
        }
        break;
        case RVCDM0SYNC:
        {
            // Set the command frame
            NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, 0);
        }
        default:
            break;
        }
    }
}

/**
 * @brief Prepare a DMRV frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
static bool transmit130762Dgn(uint8_t instance, uint8_t *p_data)
{
    // LOG(D, "Transmit DMRV:130762 function");
    // Stuff message data
    RVCDGN_DGN_130762_Stuff(p_data, &l_130762_dgn.out);
    return true;
}

/**
 * @brief DMRV DGN received (130762)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
static bool receive130762Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    LOG(D, "DMRV:130762 received");
    int32_t value;
    bool updated_data = false;
    RVCDGN_zDGN_130762 zDGN;
    RVCDGN_DGN_130762_Extract(&zDGN, p_data);
    device_entry_t *dev = NULL;
    // uint8_t op_status = (l_130762_dgn.in.u2OperatingStatus2 << 2) | l_130762_dgn.in.u2OperatingStatus1;
    uint8_t op_status = (zDGN.u2OperatingStatus2 << 2) | zDGN.u2OperatingStatus1;
    TRUE_CHECK(xSemaphoreTake(fault_mutex, portMAX_DELAY));
    TRUE_CHECK(xSemaphoreTake(rvc_heartbeat_mutex, portMAX_DELAY));

    DgnNode_t *dgn_node = DgnNodeFindBySourceAddress(&l_prod, sa);
    if (dgn_node == NULL)
    {
        xSemaphoreGive(rvc_heartbeat_mutex);
        xSemaphoreGive(fault_mutex);
        return false;
    }
    switch (((device_node_data_t *)dgn_node->dgn_data)->state)
    {
    case DEVICE_STATE_UNKNOWN:
        break;
    case DEVICE_STATE_WAIT_DMRV:
        // Store DSA
        ((device_node_data_t *)dgn_node->dgn_data)->state = DEVICE_STATE_WAIT_PID;
        ((device_node_data_t *)dgn_node->dgn_data)->device.dsa = zDGN.u8DSA;
        dev = &(((device_node_data_t *)dgn_node->dgn_data)->device);
        break;
    default:
        dev = &(((device_node_data_t *)dgn_node->dgn_data)->device);
        break;
    }
    if (dev == NULL)
    {
        xSemaphoreGive(rvc_heartbeat_mutex);
        xSemaphoreGive(fault_mutex);
        return false;
    }
    dev->op_status = op_status;
    dev->last_dmrv_time = 0;
    bool found_fault = false;
    uint32_t spn = 0;
    uint8_t instance_num = 0;
    bool is_multi_instance = false;
    for (size_t i = 0; i < ELEMENTS(multi_instance_DSA); i++)
    {
        if (zDGN.u8DSA == multi_instance_DSA[i])
        {
            is_multi_instance = true;
            break;
        }
    }
    bool no_spn = true;
    // If SPN_MSB == 0, it means it is generic error or universal error for multi instance devices and it should be handled with single-instance parsing
    if ((zDGN.u8SPN_MSB == 0) || (!is_multi_instance))
    {
        spn = ((uint32_t)zDGN.u8SPN_MSB << 11) | ((uint32_t)zDGN.u8SPN_ISB << 3) | (zDGN.u3SPN_LSB & 0x07);
        if (spn != 0x7FFFF)
        {
            // There is data
            no_spn = false;
        }
        instance_num = 0;
    }
    else if (zDGN.u8SPN_MSB == 1)
    {
        spn = ((uint16_t)zDGN.u8SPN_MSB << 3) | (zDGN.u3SPN_LSB & 0x07);
        if (spn != 0x7FF)
        {
            no_spn = false;
        }
        instance_num = zDGN.u8SPN_ISB;
    }
    if (!no_spn)
    {
        for (size_t i = 0; i < dev->num_faults; i++)
        {
            if (dev->faults[i].spn == spn)
            {
                dev->faults[i].last_seen_time = hal_cpu_get_millis();
                dev->faults[i].red_lamp = zDGN.u2RedLampStatus;
                dev->faults[i].yellow_lamp = zDGN.u2YellowLampStatus;
                dev->faults[i].occ_cnt = zDGN.u7OccurCount;
                dev->faults[i].fmi = zDGN.u5FMI;
                dev->faults[i].is_active = true;
                found_fault = true;
                break;
            }
        }
        if (!found_fault && (dev->num_faults < MAX_NUM_FAULTS))
        {
            fault_info_t *f = &dev->faults[dev->num_faults++];
            f->spn = spn;
            f->instance_id = instance_num;
            f->red_lamp = zDGN.u2RedLampStatus;
            f->yellow_lamp = zDGN.u2YellowLampStatus;
            f->occ_cnt = zDGN.u7OccurCount;
            f->fmi = zDGN.u5FMI;
            f->last_seen_time = hal_cpu_get_millis();
            f->is_active = true;
        }
    }
    xSemaphoreGive(rvc_heartbeat_mutex);
    xSemaphoreGive(fault_mutex);

    if (zDGN.u2OperatingStatus1 != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u2OperatingStatus1 = zDGN.u2OperatingStatus1;
        value = (zDGN.u2OperatingStatus2 << 2) | zDGN.u2OperatingStatus1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0OPST, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2OperatingStatus2 != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u2OperatingStatus2 = zDGN.u2OperatingStatus2;
        value = (zDGN.u2OperatingStatus2 << 2) | zDGN.u2OperatingStatus1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0OPST, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2YellowLampStatus != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u2YellowLampStatus = zDGN.u2YellowLampStatus;
        value = zDGN.u2YellowLampStatus;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0YLAMPST, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2RedLampStatus != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u2RedLampStatus = zDGN.u2RedLampStatus;
        value = zDGN.u2RedLampStatus;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0RLAMPST, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DSA != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u8DSA = zDGN.u8DSA;
        value = zDGN.u8DSA;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0DSA, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u5FMI != NMEA2K_UINT5_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u5FMI = zDGN.u5FMI;
        value = zDGN.u5FMI;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0FMI, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u7OccurCount != NMEA2K_UINT7_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u7OccurCount = zDGN.u7OccurCount;
        value = zDGN.u7OccurCount;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0OCNT, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DSAExtension != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u8DSAExtension = zDGN.u8DSAExtension;
        value = zDGN.u8DSAExtension;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0DSAEXT, &value, 1, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4BankSelect != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        l_130762_dgn.in.u4BankSelect = zDGN.u4BankSelect;
        value = zDGN.u4BankSelect;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0BANKSEL, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8SPN_MSB != NMEA2K_UINT8_NO_DATA) || (zDGN.u8SPN_ISB != NMEA2K_UINT8_NO_DATA) || (zDGN.u3SPN_LSB != NMEA2K_UINT3_NO_DATA))
    {
        updated_data = true;
        l_130762_dgn.in.u8SPN_MSB = zDGN.u8SPN_MSB;
        l_130762_dgn.in.u8SPN_ISB = zDGN.u8SPN_ISB;
        l_130762_dgn.in.u3SPN_LSB = zDGN.u3SPN_LSB;
        value = spn;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0SPN, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    if (updated_data == true)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDM0SYNC, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCPIM0 class set handler function
 *
 * Handles the SET/SUBSRIBE on entire class RVCPIM0
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
static void handleRVCPIM0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    // Handle RVC Manager class. Only instance 0
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (p_frame->frame.subscribe.parameter)
        {
        case RVCPIM0MAKE:
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, l_65259_dgn.in.cMake, strlen(l_65259_dgn.in.cMake), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
            break;
        }
        case RVCPIM0MDL:
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, l_65259_dgn.in.cModel, strlen(l_65259_dgn.in.cModel), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
            break;
        }
        case RVCPIM0SNR:
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, l_65259_dgn.in.cSerialNumber, strlen(l_65259_dgn.in.cSerialNumber), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
            break;
        }
        case RVCPIM0UNIT:
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, l_65259_dgn.in.cUnitNumber, strlen(l_65259_dgn.in.cUnitNumber), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
            break;
        }
        default:
            break;
        }
    }
    else
    {
        switch (p_frame->frame.set.parameter)
        {
        case RVCPIM0MAKE:
            ddmp2_extract_string_from_frame(p_frame, l_65259_dgn.out.cMake, RVCDGN_DGN_65259_FIELD_SIZE);
            break;
        case RVCPIM0MDL:
            ddmp2_extract_string_from_frame(p_frame, l_65259_dgn.out.cModel, RVCDGN_DGN_65259_FIELD_SIZE);
            break;
        case RVCPIM0SNR:
            ddmp2_extract_string_from_frame(p_frame, l_65259_dgn.out.cSerialNumber, RVCDGN_DGN_65259_FIELD_SIZE);
            break;
        case RVCPIM0UNIT:
            ddmp2_extract_string_from_frame(p_frame, l_65259_dgn.out.cUnitNumber, RVCDGN_DGN_65259_FIELD_SIZE);
            break;
        case RVCPIM0SYNC:
            // Set the command frame
            NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, 0);
            break;
        default:
            break;
        }
    }
}
/**
 * @brief Prepare a PIM frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
static bool transmit65259Dgn(uint8_t instance, uint8_t *p_data)
{
    LOG(D, "Transmit 65259 function");
    // Stuff message data
    if (RVCDGN_DGN_65259_Stuff(p_data, &l_65259_dgn.out) < 8)
    {
        LOG(W, "Wrong in RVCDGN_DGN_65259_Stuff");
        return false;
    }
    else
    {
        return true;
    }
}

/**
 * @brief Prepare a Info Request frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
static bool transmit59904Dgn(uint8_t instance, uint8_t *p_data)
{
    LOG(D, "Transmit 59904 function");
    // Stuff message data
    RVCDGN_DGN_59904_Stuff(p_data, &l_59904_dgn);
    // clear dest address, default to global
    l_59904_dgn.da = 0xFF;
    return true;
}

static bool update_sa_node_if_same_pid(DgnNode_t *dgn_node, void *arg)
{
    RVCDGN_zDGN_65259 *p_pid = ((update_sa_t *)arg)->pid;
    if (dgn_node->dgn_data)
    {
        RVCDGN_zDGN_65259 *pzDGN = (RVCDGN_zDGN_65259 *)&(((device_node_data_t *)dgn_node->dgn_data)->pid_data);
        if (strncmp(pzDGN->cSerialNumber, p_pid->cSerialNumber, RVCDGN_DGN_65259_FIELD_SIZE) == 0 &&
            strncmp(pzDGN->cModel, p_pid->cModel, RVCDGN_DGN_65259_FIELD_SIZE) == 0 &&
            strncmp(pzDGN->cMake, p_pid->cMake, RVCDGN_DGN_65259_FIELD_SIZE) == 0)
        {
            // Found node with same PID
            LOG(I, "SA (%x -> %x) updated for device with SN: %s, MDL: %s, MANUF: %s", dgn_node->source_address, ((update_sa_t *)arg)->new_sa, p_pid->cSerialNumber, p_pid->cModel, p_pid->cMake);
            update_sa_in_tracking(dgn_node->source_address, ((update_sa_t *)arg)->new_sa);
            dgn_node->source_address = ((update_sa_t *)arg)->new_sa;
            ProdDBUpdateCache((const void *)&dgn_node->source_address, sizeof(uint8_t), FIELD_PROP_SA, dgn_node->ddm_instance);
            ProdDBUpdateCache((const void *)&dgn_node->source_address, sizeof(uint8_t), FIELD_PROP, dgn_node->ddm_instance);
        }
    }

    return false;
}

static int32_t getProdDBTypeFromDSA(uint8_t dsa)
{
    int32_t output_val = 0;
    switch (dsa)
    {
    case 0x42:
        // fallthrough
    case 0x43:
        // Inverter
        output_val = (int32_t)PRODDB_PRODUCTTYPE_INVERTER;
        break;
    case 0x45:
        // Shunt - Battery State of Charge Monitor
        output_val = (int32_t)PRODDB_PRODUCTTYPE_SHUNT;
        break;
    case 0x46:
        // Battery
        output_val = (int32_t)PRODDB_PRODUCTTYPE_BATTERY;
        break;
    case 0x4A:
        // fallthrough
    case 0x4B:
        // Converter
        output_val = (int32_t)PRODDB_PRODUCTTYPE_CONVERTER;
        break;
    case 0x4C:
        // Charge Controller
        output_val = (int32_t)PRODDB_PRODUCTTYPE_CHARGER;
        break;
    case 0x8D:
        // Solar Charge Controller
        output_val = (int32_t)PRODDB_PRODUCTTYPE_SOLARCHARGER;
        break;
    default:
        LOG(W, "Not supported dsa: 0x%02x", dsa);
        break;
    }
    return output_val;
}
/**
 * @brief PIM DGN received (65259)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
static bool receive65259Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    LOG(D, "PIM 65259 received");
    // Special handling. We have already extracted data into structure
    RVCDGN_zDGN_65259 zDGN;
    RVCDGN_DGN_65259_Extract(&zDGN, p_data, size);
    int32_t MAYBE_UNUSED value;
    bool ret_val = true;
    bool updated_data = false;
    int32_t ddm_instance = -1;
    DgnNode_t *dgn_node = NULL;

    TRUE_CHECK(xSemaphoreTake(rvc_heartbeat_mutex, portMAX_DELAY));
    dgn_node = DgnNodeFindBySourceAddress(&l_prod, sa);

    if (product_conf_manager_find_model(zDGN.cMake, zDGN.cModel))
    {
        if (dgn_node)
        {
            switch (((device_node_data_t *)dgn_node->dgn_data)->state)
            {
            case DEVICE_STATE_WAIT_PID:
            {
                prod_database_t *prod_data = hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (prod_data != NULL)
                {
                    ((device_node_data_t *)dgn_node->dgn_data)->state = DEVICE_STATE_RESOLVED;
                    memset(prod_data, 0, sizeof(prod_database_t));
                    strncpy(prod_data->mdl, zDGN.cModel, strlen(zDGN.cModel));
                    strncpy(prod_data->sn, zDGN.cSerialNumber, strlen(zDGN.cSerialNumber));

                    ddm_instance = ProdDBProdClassNodeCreate(prod_data, sizeof(prod_database_t), connector_rvc.connector_id);
                    if (ddm_instance > 0)  // Valid instance
                    {
                        device_node_data_t node_data = *(device_node_data_t *)dgn_node->dgn_data;
                        node_data.pid_data = zDGN;
                        node_data.device.sa = sa;
                        dgn_node->ddm_instance = ddm_instance;
                        DgnNodeUpdate(dgn_node, &node_data);
                        prodxprop_type_t ptype = {0};
                        ptype.type.intf = PRODXPROP_TYPE_INTERFACE_RVC;
                        // Set initial type according to dsa
                        uint8_t dsa = ((device_node_data_t *)dgn_node->dgn_data)->device.dsa;
                        // TODO Split out in generic function
                        LOG(D, "Check DSA 0x%02x", dsa);
                        switch (dsa)
                        {
                        // Power products
                        case 0x42:
                            // fallthrough
                        case 0x43:
                            // Inverter
                        case 0x45:
                            // Shunt - Battery State of Charge Monitor
                        case 0x46:
                            // Battery
                        case 0x4A:
                            // fallthrough
                        case 0x4B:
                            // Converter
                        case 0x4C:
                            // Charge Controller
                        case 0x8D:
                            // Solar Charge Controller
                            ptype.type.cls = PRODXPROP_TYPE_CLASS_POWER;
                            ProdDBUpdateCache((const void *)&ptype, sizeof(ptype), FIELD_PROP_TYPE, ddm_instance);
                            break;
                        default:
                        {
                            if ((dsa >= 0x58) && (dsa <= 0x5D))
                            {
                                ptype.type.cls = PRODXPROP_TYPE_CLASS_CLIMATE;
                            }
                            else if ((dsa >= 0x5E) && (dsa <= 0x66))
                            {
                                ptype.type.cls = PRODXPROP_TYPE_CLASS_CLIMATE;
                            }
                            else if (((dsa >= 0x67) && (dsa <= 0x6A)))
                            {
                                ptype.type.cls = PRODXPROP_TYPE_CLASS_CLIMATE;
                            }
                            ProdDBUpdateCache((const void *)&ptype, sizeof(ptype), FIELD_PROP_TYPE, ddm_instance);
                            break;
                        }
                        }
                        // Store DSA in ProdDB
                        ProdDBMetaData_t metadata;
                        size_t metadata_size = sizeof(ProdDBMetaData_t);
                        metadata.data_u8[0] = dsa;  // First index is DSA
                        ProdDBUpdateCache(&metadata, metadata_size, FIELD_METADATA, ddm_instance);

                        ProdDBUpdateCache(zDGN.cMake, strlen(zDGN.cMake), FIELD_MANUF, ddm_instance);
                        int32_t proddbtype = getProdDBTypeFromDSA(dsa);
                        // Set type from DSA. This can be overridden by setting in product conf.json if set.
                        proddbtype = ProdDBSetProductType(ddm_instance, proddbtype);
                        ProdDBUpdateCache((const void *)&sa, sizeof(uint8_t), FIELD_PROP_SA, ddm_instance);
                        ProdDBUpdateCache((const void *)&sa, sizeof(uint8_t), FIELD_PROP, ddm_instance);
                        ProdDBProdClassNodeAddResetHandler(ddm_instance, prod_rvc_reset_handler);
                        char version[64] = {0};
                        if (product_conf_manager_extract_fw_version(zDGN.cMake, prod_data->mdl, prod_data->mdl, version, sizeof(version)) == ESP_OK)
                        {
                            // Update db
                            if (ProdDBUpdateCache(version, strlen(version), FIELD_FWVER, ddm_instance))
                            {
                                LOG(I, "Updated FWVER to: %s for DDM instance %d", version, ddm_instance);
                            }
                        }
                        version[0] = '\0';
                        if (product_conf_manager_extract_hw_version(zDGN.cMake, prod_data->mdl, prod_data->mdl, version, sizeof(version)) == ESP_OK)
                        {
                            // Update db
                            if (ProdDBUpdateCache(version, strlen(version), FIELD_HWVER, ddm_instance))
                            {
                                LOG(I, "Updated HWVER to: %s for DDM instance %d", version, ddm_instance);
                            }
                        }
                        // update FWID
                        fwid_query_result_t fwid_query_result = {0};
                        fwid_query_result.count = 1;
                        char fwid0[PRODUCT_CONF_MANAGER_MAX_FWID_LEN];
                        char *tmp_array[1] = {fwid0};
                        fwid_query_result.fwids = tmp_array;

                        bool bFoundModel = product_conf_manager_get_model_fwids(zDGN.cMake, zDGN.cModel, &fwid_query_result);
                        if (bFoundModel && (fwid_query_result.count == 1))
                        {
                            ProdDBUpdateCache(fwid0, strlen(fwid0), FIELD_FWID, ddm_instance);
                        }
                        else
                        {
                            LOG(W, "No fw id mapped to %s", zDGN.cModel);
                        }
                    }
                    else if (ddm_instance == PROD_DB_ERR_PRODUCT_ALREADY_EXISTS)
                    {
                        // Same product but no dgnnode found for this sa.
                        // update source address
                        LOG(W, "PROD_DB_ERR_PRODUCT_ALREADY_EXISTS but not with sa : %x", sa);
                        // If the device is resolved and the PID data is the same, but SA is different, update the SA
                        const update_sa_t update_sa = {
                            .new_sa = sa,
                            .pid = &zDGN,
                        };

                        DgnNodeLoopAndExecute(&l_prod, update_sa_node_if_same_pid, (void *)&update_sa);
                        // Check that update was successful
                        dgn_node = DgnNodeFindBySourceAddress(&l_prod, sa);
                        if (dgn_node == NULL)
                        {
                            LOG(E, "DgnNode still not found (SA: %x)", sa);
                            ret_val = false;
                        }
                    }
                    else
                    {
                        LOG(E, "ProdDBProdClassNodeCreate returned error %d", ddm_instance);
                    }
                    hal_mem_free(prod_data);
                }
                else
                {
                    LOG(E, "Cannot allocate memory for prod_database_t structure");
                    ret_val = false;
                }
                break;
            }
            default:
                break;
            }
        }
        if (memcmp(zDGN.cMake, l_65259_dgn.in.cMake, RVCDGN_DGN_65259_FIELD_SIZE) != 0)
        {
            memcpy(l_65259_dgn.in.cMake, zDGN.cMake, RVCDGN_DGN_65259_FIELD_SIZE);
            updated_data = true;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPIM0MAKE, l_65259_dgn.in.cMake, strlen(l_65259_dgn.in.cMake), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        if (memcmp(zDGN.cModel, l_65259_dgn.in.cModel, RVCDGN_DGN_65259_FIELD_SIZE) != 0)
        {
            memcpy(l_65259_dgn.in.cModel, zDGN.cModel, RVCDGN_DGN_65259_FIELD_SIZE);
            updated_data = true;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPIM0MDL, l_65259_dgn.in.cModel, strlen(l_65259_dgn.in.cModel), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        if (memcmp(zDGN.cSerialNumber, l_65259_dgn.in.cSerialNumber, RVCDGN_DGN_65259_FIELD_SIZE) != 0)
        {
            memcpy(l_65259_dgn.in.cSerialNumber, zDGN.cSerialNumber, RVCDGN_DGN_65259_FIELD_SIZE);
            updated_data = true;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPIM0SNR, l_65259_dgn.in.cSerialNumber, strlen(l_65259_dgn.in.cSerialNumber), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        if (memcmp(zDGN.cUnitNumber, l_65259_dgn.in.cUnitNumber, RVCDGN_DGN_65259_FIELD_SIZE) != 0)
        {
            memcpy(l_65259_dgn.in.cUnitNumber, zDGN.cUnitNumber, RVCDGN_DGN_65259_FIELD_SIZE);
            updated_data = true;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPIM0UNIT, l_65259_dgn.in.cUnitNumber, strlen(l_65259_dgn.in.cUnitNumber), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            if (dgn_node)
            {
                device_node_data_t node_data = *((device_node_data_t *)dgn_node->dgn_data);
                node_data.pid_data = zDGN;
                node_data.device.sa = sa;

                DgnNodeUpdate(dgn_node, &node_data);
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPIM0SYNC, &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    else
    {
        // This sa is not supported device
        if (dgn_node)
        {
            // remove node
            LOG(W, "Clearing device slot for SA %02X DSA %02X because it is not whitelisted", sa, ((device_node_data_t *)dgn_node->dgn_data)->device.dsa);
            DgnNodeDelete(dgn_node);
        }
    }

    xSemaphoreGive(rvc_heartbeat_mutex);

    return ret_val;
}

/**
 * @brief Generic Configuration Status DGN received (130776)
 *
 * @param p_data DGN data pointer
 * @param sa Source address
 * @param size Data size
 * @return true if message has been handled
 */
bool receive130776Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    int32_t value;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    RVCDGN_zDGN_130776 zDGN;
    RVCDGN_DGN_130776_Extract(&zDGN, p_data);

    DgnNode_t *prod_node = NULL;
    prod_node = DgnNodeFindBySourceAddress(&l_prod, sa);

    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindBySourceAddress(&l_130776_dgn, sa);
    if (dgn_node == NULL)
    {
        uint32_t device_class;
        device_class = RVCGENCFG0;
        ddm_instance = broker_register_instance(&device_class, connector_rvc.connector_id);
        if (ddm_instance == -1)
        {
            LOG(E, "Registration failed for class %08x", device_class);
            return false;
        }
        dgn_node = DgnNodeCreate(INVALID_RVC_INSTANCE, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130776));
        if (dgn_node == NULL)
        {
            LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, device_class);
            return false;
        }
        DgnNodeInsert(dgn_node, &l_130776_dgn);
        if (prod_node && (prod_node->ddm_instance != -1))
        {
            uint32_t add_class_inst = device_class;
            ProdDBUpdateCache((const void *)&add_class_inst, sizeof(uint32_t), FIELD_PROP_CLASS, prod_node->ddm_instance);
            // Publish PROP
            ProdDBUpdateCache((const void *)&add_class_inst, 0, FIELD_PROP, prod_node->ddm_instance);
        }
    }
    else
    {
        ddm_instance = dgn_node->ddm_instance;
    }
    // Compare with last data
    // Manufacturer Code (combine LSB and MSB)
    uint16_t manufacturer_code = ((uint16_t)zDGN.u3ManufacturerCodeMSB << 8) | zDGN.u8ManufacturerCodeLSB;
    if (manufacturer_code < (NMEA2K_UINT16_NO_DATA & 0x07FF))
    {
        value = manufacturer_code;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0MC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        updated_data = true;
    }

    // Function Instance
    if (zDGN.u5FunctionInstance < NMEA2K_UINT5_NO_DATA)
    {
        value = zDGN.u5FunctionInstance;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0FUNCINST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        updated_data = true;
    }

    // Function
    if (zDGN.u8Function != NMEA2K_UINT8_NO_DATA)
    {
        value = zDGN.u8Function;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0FUNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        updated_data = true;
    }

    // Firmware Revision
    if (zDGN.u8FirmwareRevision != NMEA2K_UINT8_NO_DATA)
    {
        value = zDGN.u8FirmwareRevision;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0FWREV | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        updated_data = true;
    }

    // Configuration Type (combine LSB, normal, and MSB)
    uint32_t config_type = ((uint32_t)zDGN.u8ConfigTypeMSB << 16) | ((uint32_t)zDGN.u8ConfigType << 8) | zDGN.u8ConfigTypeLSB;
    if (config_type < (NMEA2K_UINT32_NO_DATA & 0xFFFFFF))
    {
        value = config_type;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0CONFTYPE | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        updated_data = true;
    }

    // Configuration Revision
    if (zDGN.u8ConfigRevision != NMEA2K_UINT8_NO_DATA)
    {
        value = zDGN.u8ConfigRevision;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0CONFREV | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        updated_data = true;
    }

    if (sa != 0xFF)
    {
        value = (int32_t)sa;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0ADDR | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        updated_data = true;
    }

    if (true == updated_data)
    {
        value = 1;
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCGENCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCGENCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130776 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130776_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130776_Extract(&zDGN, dgn_node->dgn_data);

        if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
            {
            case RVCGENCFG0MC:
            {
                uint16_t manufacturer_code = ((uint16_t)zDGN.u3ManufacturerCodeMSB << 8) | zDGN.u8ManufacturerCodeLSB;
                if (manufacturer_code < (NMEA2K_UINT16_NO_DATA & 0x07FF))
                {
                    value = manufacturer_code;
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            }

            case RVCGENCFG0FUNCINST:
                if (zDGN.u5FunctionInstance < NMEA2K_UINT5_NO_DATA)
                {
                    value = zDGN.u5FunctionInstance;
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
                }
                break;

            case RVCGENCFG0FUNC:
                if (zDGN.u8Function != NMEA2K_UINT8_NO_DATA)
                {
                    value = zDGN.u8Function;
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
                }
                break;

            case RVCGENCFG0FWREV:
                if (zDGN.u8FirmwareRevision != NMEA2K_UINT8_NO_DATA)
                {
                    value = zDGN.u8FirmwareRevision;
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
                }
                break;

            case RVCGENCFG0CONFTYPE:
            {
                uint32_t config_type = ((uint32_t)zDGN.u8ConfigTypeMSB << 16) | ((uint32_t)zDGN.u8ConfigType << 8) | zDGN.u8ConfigTypeLSB;
                if (config_type < (NMEA2K_UINT32_NO_DATA & 0xFFFFFF))
                {
                    value = config_type;
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            }

            case RVCGENCFG0CONFREV:
                if (zDGN.u8ConfigRevision != NMEA2K_UINT8_NO_DATA)
                {
                    value = zDGN.u8ConfigRevision;
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
                }
                break;

            case RVCGENCFG0ADDR:
                if (dgn_node->source_address != NMEA2K_GLOBAL_ADDRESS)
                {
                    value = (int32_t)dgn_node->source_address;
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            case RVCGENCFG0SYNC:
            {
                // Always publish sync as 1, since we have DgnNode for the instance, which means that the DGN has been processed at least once
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &One, sizeof(One), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
            default:
                break;
            }
        }
    }
}

#ifdef RVC_CONFIG_IMPL_PROPRIATARY
/**
 * @brief Proprietary DGN (61184) received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive61184Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    int32_t value;
    // Extract message content
    PROPDGN_zDGN_61184 zDGN;
    PROPDGN_DGN_61184_Extract(&zDGN, p_data, size);

    value = (int32_t)sa;
    l_61184_dgn.in.addr = sa;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPROP0ADDR, &value, sizeof(int32_t), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);

    memcpy(l_61184_dgn.in.u8Data, zDGN.u8Data, size);
    l_61184_dgn.in.size = size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPROP0DATA, zDGN.u8Data, size, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    value = 1;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPROP0SYNC, &value, sizeof(int32_t), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);

    return true;
}

/**
 * @brief Prepare Proprietary DGN 61184 (0xEF00) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit61184Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    PROPDGN_DGN_61184_Stuff(p_data, &l_61184_dgn.out);

    return true;
}

/**
 * @brief RVCPROP0 class handler function
 *
 * @param index Transmit DGN
 * @param p_frame Received DDM2P frame
 */
void handleRVCPROP0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
    uint8_t raw_data[PROPDGN_DGN_61184_SIZE];
    if (instance >= 1)
    {
        LOG(E, "Wrong instance received: %d", instance);
        return;
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCPROP0ADDR:
        {
            value = l_61184_dgn.in.addr;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        case RVCPROP0DATA:
        {
            uint8_t prop_data[PROPDGN_DGN_61184_SIZE - 1];
            memcpy(prop_data, l_61184_dgn.in.u8Data, l_61184_dgn.in.size);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prop_data, l_61184_dgn.in.size, connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
        }
        break;
        default:
            break;
        }
    }
    else
    {
        if (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter) == RVCPROP0ADDR || DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter) == RVCPROP0SYNC)
        {
            value = p_frame->frame.set.value.int32;
        }
        else if (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter) == RVCPROP0DATA)  // handle prop. data to avoid overflow in value
        {
            ASSERT(ddmp2_value_size(p_frame) <= DDMP2_MAX_VALUE_SIZE);
            memcpy(raw_data, p_frame->frame.set.value.raw, ddmp2_value_size(p_frame));
        }
        if (convert_ddm_system_value_to_rvc_value(p_frame->frame.set.parameter, &value, true))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCPROP0ADDR:
                l_61184_dgn.out.addr = (uint8_t)value;
                break;
            case RVCPROP0DATA:
            {
                size_t data_size = ddmp2_value_size(p_frame);
                if (data_size < PROPDGN_DGN_61184_SIZE)
                {
                    l_61184_dgn.out.size = data_size;
                    memcpy(l_61184_dgn.out.u8Data, p_frame->frame.set.value.raw, data_size);
                }
            }
            break;
            case RVCPROP0SYNC:
            {
                enqueue_prop_msg(l_61184_dgn.out.addr, l_61184_dgn.out.u8Data, l_61184_dgn.out.size, 1);
            }
            break;
            default:
                break;
            }
        }
    }
}
#endif

/**
 * @brief Will be called whenever the library detects any of supported status updates.
 *
 * @param status_type[in] Type of status update
 * @param value[in] Value for given update
 */
static void status_changed_cb(msgcan_lib_status_type_t status_type, uint32_t value)
{
    if (MSGCAN_LIB_STATUS_ADDRESS_CLAIMED_UPDATE == status_type)
    {
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCMGNT0ADDR, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
    else if (MSGCAN_LIB_STATUS_PROP_DGN_TX_READY == status_type)
    {
        xTaskNotifyGive(can_txrx_task_handle);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCPROP0SYNC, &value, sizeof(value), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
}

static void prod_rvc_reset_handler(int32_t cmd, int instance)
{
    memset(&l_98048_dgn, 0, sizeof(l_98048_dgn));  // Clear previous commands
    switch (cmd)
    {
    case PROD0RESET_RESTART:
    {
        l_98048_dgn.u2Reboot = One;
    }
    break;
    case PROD0RESET_RESET_TO_FACTORY_SETTINGS:
    {
        l_98048_dgn.u2ResetToOEMSettings = One;
    }
    break;
    case PROD0RESET_RESET_TO_DEFAULT_SETTINGS:
    {
        l_98048_dgn.u2ResetToDefault = One;
    }
    break;
    case PROD0RESET_CLEAR_FAULTS:
    {
        l_98048_dgn.u2ClearFaults = One;
    }
    break;
    default:
        break;
    }

    if (cmd != PROD0RESET_IDLE)
    {
        // Send General reset to given destination address
        uint8_t addr = 0xFF;  // Global address by default
        PROD0PROP_T prodprop;
        size_t prop_size = sizeof(PROD0PROP_T);

        ProdDBReadCache(FIELD_PROP, instance, &prodprop, &prop_size);
        if (prop_size != 0)
        {
            addr = prodprop.addr;
        }
        if (addr != NMEA2K_GLOBAL_ADDRESS)
        {
            // Set the command frame
            l_98048_dgn.da = addr;
            NMEA2K_SetTxRequest(BSP_CAN_RVC, GENERAL_RESET_DGN, 0);
        }

        int32_t reset_cmd = cmd;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, PROD0RESET | DDM2_PARAMETER_INSTANCE(instance), &reset_cmd, sizeof(reset_cmd), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);
    }
}

/**
 * @brief Remove all generic RV-C nodes associated with a specific source address
 *
 * @param sa Source address of the device
 */
void remove_generic_rvc_nodes(uint8_t sa)
{
    DgnNode_t *dgn_node = NULL;

    LOG(D, "Removing generic RV-C nodes for SA=%02X", sa);

    // Remove RVCGENCFG0 node
    dgn_node = DgnNodeFindBySourceAddress(&l_130776_dgn, sa);
    if (dgn_node != NULL)
    {
        LOG(D, "Removing RVCGENCFG0 node: SA=%02X, DDM_inst=%d, RVC_inst=%d",
            dgn_node->source_address, dgn_node->ddm_instance, dgn_node->rvc_instance);

        // Publish unavailable class
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCGENCFG0 | DDM2_PARAMETER_INSTANCE(dgn_node->ddm_instance),
                                       &Zero, sizeof(Zero), connector_rvc.connector_id, (TickType_t)portMAX_DELAY);

        // Remove the node
        DgnNodeDelete(dgn_node);
    }

    LOG(D, "Completed removing generic RV-C nodes for SA=%02X", sa);
}
