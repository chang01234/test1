/**
 * \file        connector_batt_mgmt_service.c
 * \date        2024-03-19
 * \author      Kire Janev (kire.janev@dometic.com)
 * \brief       Battery management service that is the owner of the MBAT and MBATHIST classes.
 *
 * \li          2024-03-19 (KJ) Initial implementation
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

#include <stdint.h>
#include <string.h>

#include "connector_batt_mgmt_service.h"

#include "batt_mgmt_ddm_class_desc.h"
#include "connector.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "ddm_wrapper.h"
#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"
#include "product_database.h"

#ifndef CONNECTOR_BAT_MGMT_SERVICE_ENABLE_LOG
#define CONNECTOR_BAT_MGMT_SERVICE_ENABLE_LOG 0
#endif
#define RVC_MBAT_PARAM_MAPPING_DEPTH    40
#define RVC_MBAT_INST_MAPPING_DEPTH     60
#define DC_INST_MBAT_INST_MAPPING_DEPTH 10
#define PROD_MBAT_INST_MAPPING_DEPTH    10
#define MAX_NO_OF_LINKED_RVC_CLASSES    20
#define PROD_MODEL_MAPPING_DEPTH        10
#define MBAT_ERR_MAPPING_DEPTH          30

#define MODEL_SHUNT                    "GP-Shunt"
#define MODEL_BATTERY                  "GP-LFP"
#define PROPRIETARY_RAW_RVC_FRAME_SIZE 8

#define RVC_PROPRIETARY                       (uint32_t)0xEF00
#define DC_DISCONNECT_COMMAND                 (uint32_t)130767
#define BATTERY_SUMMARY_DGN                   (uint32_t)130545
#define DGN_DC_SOURCE_CONFIGURATION_STATUS_2  (uint32_t)130549
#define DGN_DC_SOURCE_CONFIGURATION_COMMAND_2 (uint32_t)130548
#define DGN_DC_SOURCE_CONFIGURATION_STATUS_1  (uint32_t)130551
#define DGN_DC_SOURCE_CONFIGURATION_COMMAND_1 (uint32_t)130550
#define GENERIC_CONFIG_STATUS_DGN             (uint32_t)130776
#define MAX_NO_OF_BATT_IN_BANK                4

#define BATT_MGMT_MBATHTR_REQUEST_EVENT 1

#define GP_RESPONSE_CMD         0x0D
#define GP_REQUEST_CMD          0x0C
#define GP_BATT_CMD             0x01
#define GP_BATT_CFG_CMD_1       0x02
#define GP_SHUNT_STS_1          0x04
#define GP_SHUNT_CMD_1          0x05
#define GP_SHUNT_STS_2          0x06
#define GP_SHUNT_CMD_2          0x07
#define GP_SHUNT_CMD_3          0x08
#define GP_PROP_MSG_PWD         0x4750
#define GP_PROD_TYPE_INVALID    0x00
#define GP_PROD_TYPE_BATT_SHUNT 0x01
#define GP_MSG_ID_0             0x00
#define GP_DEFAULT_VALUE        0xFF

#define RVC_GLOBAL_DEST_ADDR 0xFF
#define INVALID_INSTANCE     0xFF

#define GP_BATT_BLINK_MODE_BLINK_ACTIVE 1

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mbat_param_mapping_table, RVC_MBAT_PARAM_MAPPING_DEPTH);           /* RVCDCRC<X> to MBAT DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mbat_ddm_inst_mapping_table, RVC_MBAT_INST_MAPPING_DEPTH);         /* RVCDCRC<X> to MBAT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(dc_inst_mbat_ddm_inst_mapping_table, DC_INST_MBAT_INST_MAPPING_DEPTH); /* DC instance to MBAT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(mbat_prod_inst_mapping_table, RVC_MBAT_INST_MAPPING_DEPTH);            /* MBAT DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mbathist_param_mapping_table, RVC_MBAT_PARAM_MAPPING_DEPTH);           /* RVCDCRC<X> to MBATHIST DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mbathist_ddm_inst_mapping_table, RVC_MBAT_INST_MAPPING_DEPTH);         /* RVCDCRC<X> to MBATHIST DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(dc_inst_mbathist_ddm_inst_mapping_table, DC_INST_MBAT_INST_MAPPING_DEPTH); /* DC instance to MBATHIST DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(mbathist_prod_inst_mapping_table, RVC_MBAT_INST_MAPPING_DEPTH);            /* MBATHIST DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mshunt_param_mapping_table, RVC_MBAT_PARAM_MAPPING_DEPTH);           /* RVCDCRC<X> to MSHUNT DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mshunt_ddm_inst_mapping_table, RVC_MBAT_INST_MAPPING_DEPTH);         /* RVCDCRC<X> to MSHUNT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(dc_inst_mshunt_ddm_inst_mapping_table, DC_INST_MBAT_INST_MAPPING_DEPTH); /* DC instance to MSHUNT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(mshunt_prod_inst_mapping_table, RVC_MBAT_INST_MAPPING_DEPTH);            /* MSHUNT DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_param_mapping_table, RVC_MBAT_PARAM_MAPPING_DEPTH);           /* RVCDCRC<X> to MDCPROFILE DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_ddm_inst_mapping_table, RVC_MBAT_PARAM_MAPPING_DEPTH);        /* RVCDCRC<X> to MDCPROFILE DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(dc_inst_mdcprofile_ddm_inst_mapping_table, DC_INST_MBAT_INST_MAPPING_DEPTH); /* DC instance to MDCPROFILE DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(mdcprofile_prod_inst_mapping_table, RVC_MBAT_INST_MAPPING_DEPTH);            /* MDCPROFILE DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(prod_model_table, PROD_MODEL_MAPPING_DEPTH); /* PROD model cache */

DECLARE_SORTED_LIST_EXTRAM_PTR(mbat_err_table, MBAT_ERR_MAPPING_DEPTH); /* mabat error mapping */

typedef struct _mapped_params
{
    uint32_t sub_ddm2_param;
    uint32_t owned_ddm2_param;
} mapped_params_t;

typedef struct _service_map_t
{
    int32_t par;
    uint8_t command_number;
    uint32_t *raw;
    int32_t *(*conversion_to_ddm)(uint32_t);
} service_map_t;

typedef enum _gp_device_model_t
{
    NO_MODEL = 0,
    GP_SHUNT = 1,
    GP_BATT = 2,
} gp_device_model_t;
typedef struct _batt_map
{
    uint8_t batt_inst;
    uint8_t dc_inst;
    uint8_t sa;
    bool is_master;
    LIST_ENTRY(_batt_map) list_node;
} batt_map_t;

typedef struct _sub_rvc_param
{
    uint32_t ddm2_param;
    ddmw_item_t *ddm2_item;
} sub_rvc_param_list_t;

/* Go Power Battery Command interface */
typedef struct _gp_batt_cmd
{
    uint16_t pwd;
    uint8_t cmd;
    uint8_t batt_inst;
    uint8_t series_string;
    uint8_t master_slave : 2;
    uint8_t htr_mode : 2;
    uint8_t unused : 4;
    uint8_t not_used[2];
} PACKED gp_batt_cmd_t;

typedef struct _gp_batt_cfg_cmd_1
{
    uint16_t pwd;
    uint8_t cmd;
    uint8_t blink_mode;
    uint8_t not_used[4];
} PACKED gp_batt_cfg_cmd_1_t;

/* Go Power Request Command interface */
typedef struct _gp_req_cmd
{
    uint16_t pwd;
    uint8_t cmd;
    uint8_t prod_type;
    uint8_t msg_id;
    uint8_t not_used[3];
} PACKED gp_req_cmd_t;

/* Go Power DC_DISCONNECT_COMMAND interface */
typedef struct _gp_dc_disconnect_cmd
{
    uint8_t dc_instance;
    uint8_t command : 2;
    uint8_t not_used_bits : 6;
    uint8_t notused_bytes[6];
} PACKED gp_dc_disconnect_cmd_t;

/* Go Power DC_SOURCE_CONFIGURATION_COMMAND_1 interface */
typedef struct _gp_dc_src_cfg_cmd_1
{
    uint8_t dc_instance;
    uint8_t peukert_exponent;
    uint8_t temperature_coeficient;
    uint8_t charge_efficiency_factor;
    uint8_t time_remaining_averaging_period;
    uint16_t full_capacity;
    uint8_t tail_current;
} PACKED gp_dc_src_cfg_cmd_1_t;

/* Go Power DC_SOURCE_CONFIGURATION_COMMAND_2 interface */
typedef struct _gp_dc_src_cfg_cmd_2
{
    uint8_t dc_instance;
    uint8_t clear_history : 2;
    uint8_t set_100 : 2;
    uint8_t not_used_bits : 4;
    uint16_t charged_voltage;
    uint8_t shunt_voltage;
    uint16_t shunt_current;
    uint8_t not_used_byte;
} PACKED gp_dc_src_cfg_cmd_2_t;

/* Go Power GP_SHUNT_COMMAND_1 interface */
typedef struct _gp_shunt_stscmd_1
{
    uint16_t pwd;
    uint8_t cmd;
    uint16_t discharge_voltage;
    uint8_t discharge_floor;
    uint16_t high_voltage_limit;
} PACKED gp_shunt_stscmd_1_t;

/* Go Power GP_SHUNT_COMMAND_2 interface */
typedef struct _gp_shunt_stscmd_2
{
    uint16_t pwd;
    uint8_t cmd;
    uint8_t low_temp_lim;
    uint8_t high_temp_lim;
    uint16_t high_cur_lim;
    uint8_t batt_type : 4;
    uint8_t system_voltage : 4;
} PACKED gp_shunt_stscmd_2_t;

/* Go Power GP_SHUNT_COMMAND_3 interface */
typedef struct _gp_shunt_cmd_3
{
    uint16_t pwd;
    uint8_t cmd;
    uint8_t calibrate_zero_current : 2;
    uint8_t not_used_bits : 6;
    uint8_t not_used[4];
} PACKED gp_shunt_cmd_3_t;

/* Go Power Proprietary interface */
typedef struct _gp_req_resp
{
    uint16_t pwd;
    uint8_t cmd;
    uint8_t prod_type;
    uint8_t msg_id;
    uint8_t resp_data[3];
} PACKED gp_req_resp_t;

/* Go Power Message ID 0 Response interface */
typedef struct _gp_msg_id_0_resp
{
    uint8_t htr_mode;
    uint8_t htr_status;
    uint8_t not_used;
} PACKED gp_msg_id_0_resp_t;

/* Shunt mshunt instance, SA and dc source mapping */
typedef struct _shunt_mapping
{
    bool avl;
    uint8_t instance;
    uint8_t dc_instance;
    uint8_t prod_instance;
    uint8_t sa;
} PACKED shunt_mapping_t;

typedef struct _batt_err_mapping
{
    uint32_t rvc_ddm2_param;
    DDM2_global_error_codes_e err_code;
} PACKED batt_err_mapping_t;

typedef LIST_HEAD(batt_map_list_head, _batt_map) batt_map_list_t;

static void inventory_callback(uint32_t parameter);
static void connector_batt_mgmt_service_process_task(const DDMP2_FRAME *frame);
static void manage_mbat_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance, gp_device_model_t cached_model);
static void manage_mshunt_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance);
static void manage_mdcprofile_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance, int32_t mbat_ddm_instance);
static void manage_mbathist_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance, int32_t mbat_ddm_instance);
static void subscribe_to_prop_classes(uint32_t parameter);
static bool is_rvc_class_mdcprofile_class_related(uint32_t class_instance);
static bool is_rvc_class_mbat_class_related(uint32_t class_instance);
static bool is_rvc_class_mbathist_class_related(uint32_t class_instance);
static bool is_rvc_class_mshunt_class_related(uint32_t class_instance);
static void manage_subscriptions_for_prop_class(list_t *dgn_list, uint32_t parameter);
static void process_shunt(uint8_t *rvc_frame, uint8_t cmd, int32_t dc_instance);
static void remove_mbathist_instance(uint8_t mbathist_instance, uint8_t prod_instance);
static void remove_mdcprofile_instance(uint8_t mdcprofile_instance, uint8_t prod_instance);
static void remove_mshunt_instance(uint8_t mshunt_instance, uint8_t prod_instance);
static void remove_mbat_instance(uint8_t mbat_instance, uint8_t prod_instance);

static int32_t convert_mdcprofile0dischgvolt(uint32_t raw_value);
static int32_t convert_mshunt0dischgfloor(uint32_t raw_value);
static int32_t convert_mshunt0hvoltlim(uint32_t raw_value);
static int32_t convert_mshunt0lttemplim(uint32_t raw_value);
static int32_t convert_mshunt0htemplim(uint32_t raw_value);
static int32_t convert_mshunt0hcurrlim(uint32_t raw_value);
static int32_t convert_mshunt0sysvolt(uint32_t raw_value);
static void process_batt_mapping(uint8_t batt_inst, uint8_t dc_inst, uint8_t sa);
static void remove_batt_mapping(uint8_t dc_inst);
static uint8_t get_src_addresses_for_dc_inst(uint8_t dc_inst, int32_t *sa_list, uint8_t *batt_inst_list);
static void start_subscriptions_to_rvc_params(uint32_t class_instance);
static void stop_subscriptions_to_rvc_params(uint32_t class_instance);
static bool is_batt_inst_master(uint8_t dc_inst, uint8_t prod_inst, uint8_t sa);
static int32_t get_sa_for_master_batt_inst(uint8_t dc_inst);
static uint8_t get_mbat_inst_by_sa(uint8_t sa);
static uint8_t get_dc_inst_by_sa(uint8_t sa);
static void process_prop_data(RVCPROP0DATA_T *prop_data);
static void request_mbathtr_status(uint8_t mbat_ddm_inst);
static void process_sub_param_immediately(ddmw_item_t *item, const void *data, int size);
static bool is_class_mbat_mbathist_mapped(uint32_t class_instance);
static void process_rvc_class_linkage(uint32_t class_base, uint8_t instance, uint8_t dc_inst, uint8_t addr);
static void manage_subscriptions_for_rvc_on_req_class(list_t *dgn_list, uint32_t parameter);
static void process_gen_cfg_params(uint8_t instance);
static void process_batt_summ_params(uint8_t instance);
static void batt_indicate_request_handler(bool indicate, int ddm_instance);
static void process_pub_mbat_status(uint32_t parameter);
static void process_sub_mbat_status(uint8_t mbat_ddm_inst);

static EXT_RAM_ATTR uint8_t mbat_linked_idx;
static EXT_RAM_ATTR uint8_t mbathist_linked_idx;
static EXT_RAM_ATTR uint8_t mshunt_linked_idx;
static EXT_RAM_ATTR uint8_t mdcprofile_linked_idx;

static EXT_RAM_ATTR ddmw_t ddm_container;

static EXT_RAM_ATTR list_t l_rvcdcsrc;
static EXT_RAM_ATTR list_t l_rvcdcsrctwo;
static EXT_RAM_ATTR list_t l_rvcdcsrcthr;
static EXT_RAM_ATTR list_t l_rvcdcsrcfour;
static EXT_RAM_ATTR list_t l_rvcdcsrcfive;
static EXT_RAM_ATTR list_t l_rvcdcsrcsix;
static EXT_RAM_ATTR list_t l_rvcdcsrcsev;
static EXT_RAM_ATTR list_t l_rvcdcsrceig;
static EXT_RAM_ATTR list_t l_rvcdcsrcnine;
static EXT_RAM_ATTR list_t l_rvcdcsrcten;
static EXT_RAM_ATTR list_t l_rvcdcsrcele;
static EXT_RAM_ATTR list_t l_rvcdcsrctwe;
static EXT_RAM_ATTR list_t l_rvcdcsrcthir;
static EXT_RAM_ATTR list_t l_rvcdcsrccfgtwo;
static EXT_RAM_ATTR list_t l_rvcdcsrccfg;
static EXT_RAM_ATTR list_t l_rvcdcdisconn;
static EXT_RAM_ATTR list_t l_mbat;
static EXT_RAM_ATTR list_t l_mbathist;
static EXT_RAM_ATTR list_t l_mshunt;
static EXT_RAM_ATTR list_t l_mdcprofile;
static EXT_RAM_ATTR list_t l_prod;
static EXT_RAM_ATTR list_t l_batt_sum;
static EXT_RAM_ATTR list_t l_gen_cfg;

static EXT_RAM_ATTR batt_map_list_t l_batt_map;

static EXT_RAM_ATTR ddmw_item_t l_rvc_mgmt_ack;
static EXT_RAM_ATTR ddmw_item_t l_rvc_prop_addr;
static EXT_RAM_ATTR ddmw_item_t l_rvc_prop_data;
static EXT_RAM_ATTR ddmw_item_t l_rvc_prop_sync;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_two_inst;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_two_clrhist;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_two_sync;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_inst;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_fact;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_exp;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_coeff;
static EXT_RAM_ATTR ddmw_item_t l_dc_src_cfg_sync;
static EXT_RAM_ATTR ddmw_item_t l_rvc_mgmt_addr;

static EXT_RAM_ATTR shunt_mapping_t shunt;
static EXT_RAM_ATTR bool prop_cmd_3_was_sent;
static EXT_RAM_ATTR int32_t ack_parameter;
static EXT_RAM_ATTR TimerHandle_t gp_req_timer;
static void gp_req_timer_handler(TimerHandle_t xTimer);

static const mapped_params_t rvc_mbat_params[] =
    {
        {.sub_ddm2_param = RVCDCSRCTHR0SOH, .owned_ddm2_param = MBAT0SOH},
        {.sub_ddm2_param = RVCDCSRCTHR0CAP, .owned_ddm2_param = MBAT0CAPREM},
        {.sub_ddm2_param = RVCDCSRCTHR0RELCAP, .owned_ddm2_param = MBAT0CAPREL},
        {.sub_ddm2_param = RVCDCSRCTHR0RIPPLE, .owned_ddm2_param = MBAT0ACRIPPLE},

        {.sub_ddm2_param = RVCDCSRCELE0DISCHGST, .owned_ddm2_param = MBAT0DISCHGST},
        {.sub_ddm2_param = RVCDCSRCELE0CHGST, .owned_ddm2_param = MBAT0CHGST},
        {.sub_ddm2_param = RVCDCSRCELE0CHGDET, .owned_ddm2_param = MBAT0CHGDET},
        {.sub_ddm2_param = RVCDCSRCELE0RESST, .owned_ddm2_param = MBAT0RESST},
        {.sub_ddm2_param = RVCDCSRCELE0CAPACITY, .owned_ddm2_param = MBAT0BANK},

        {.sub_ddm2_param = RVCDCSRCTWO0SOC, .owned_ddm2_param = MBAT0SOC},
        {.sub_ddm2_param = RVCDCSRCTWO0STEMP, .owned_ddm2_param = MBAT0TEMP},
        {.sub_ddm2_param = RVCDCSRCTWO0TIMEREM, .owned_ddm2_param = MBAT0TIME},
        {.sub_ddm2_param = RVCDCSRCTWO0TIMEREMTYPE, .owned_ddm2_param = MBAT0TIMESTS},

        {.sub_ddm2_param = RVCDCSRC0VOLT, .owned_ddm2_param = MBAT0VOLT},
        {.sub_ddm2_param = RVCDCSRC0CURR, .owned_ddm2_param = MBAT0CURR},

        {.sub_ddm2_param = RVCDCSRCFOUR0CHGST, .owned_ddm2_param = MBAT0DCHGST},
        {.sub_ddm2_param = RVCDCSRCFOUR0TYPE, .owned_ddm2_param = MBAT0TYPE},
        {.sub_ddm2_param = RVCDCSRCFOUR0CURR, .owned_ddm2_param = MBAT0DCURR},
        {.sub_ddm2_param = RVCDCSRCFOUR0VOLT, .owned_ddm2_param = MBAT0DVOLT},

        {.sub_ddm2_param = RVCDCSRCFIVE0VOLT, .owned_ddm2_param = MBAT0HPVOLT},

        {.sub_ddm2_param = RVCDCSRCSIX0HVLIM, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0HVDIS, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0LVLIM, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0LVDIS, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0LSOCLIM, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0LSOCDIS, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0LDCTEMPLIM, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0LDCTEMPDIS, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0HDCCURRLIM, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0HDCCURRDIS, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0HDCTEMPLIM, .owned_ddm2_param = MBAT0STATUS},
        {.sub_ddm2_param = RVCDCSRCSIX0HDCTEMPDIS, .owned_ddm2_param = MBAT0STATUS},
};

static const mapped_params_t rvc_mbathist_params[] = {
    {.sub_ddm2_param = RVCDCSRCTHI0HIGHVOLT, .owned_ddm2_param = MBATHIST0HIGHVOLT},
    {.sub_ddm2_param = RVCDCSRCTHI0LOWVOLT, .owned_ddm2_param = MBATHIST0LOWVOLT},
    {.sub_ddm2_param = RVCDCSRCTEN0INPUT, .owned_ddm2_param = MBATHIST0DAYS7I},
    {.sub_ddm2_param = RVCDCSRCTEN0OUTPUT, .owned_ddm2_param = MBATHIST0DAYS7O},
    {.sub_ddm2_param = RVCDCSRCNINE0INPUT, .owned_ddm2_param = MBATHIST0YESTERDAY2I},
    {.sub_ddm2_param = RVCDCSRCNINE0OUTPUT, .owned_ddm2_param = MBATHIST0YESTERDAY2O},
    {.sub_ddm2_param = RVCDCSRCEIG0INPUT, .owned_ddm2_param = MBATHIST0YESTERDAYI},
    {.sub_ddm2_param = RVCDCSRCEIG0OUTPUT, .owned_ddm2_param = MBATHIST0YESTERDAYO},
    {.sub_ddm2_param = RVCDCSRCSEV0INPUT, .owned_ddm2_param = MBATHIST0TODAYI},
    {.sub_ddm2_param = RVCDCSRCSEV0OUTPUT, .owned_ddm2_param = MBATHIST0TODAYO},
    {.sub_ddm2_param = RVCDCSRCTWE0AVERAGE, .owned_ddm2_param = MBATHIST0AVEDISCHG},
    {.sub_ddm2_param = RVCDCSRCTWE0DEEP, .owned_ddm2_param = MBATHIST0DEEPDISCHG},
    {.sub_ddm2_param = RVCDCSRCTWE0CYCLES, .owned_ddm2_param = MBATHIST0CYCLES},
};

static const sub_rvc_param_list_t l_sub_rvc_param[] = {
    {.ddm2_param = RVCMGNT0ACK, .ddm2_item = &l_rvc_mgmt_ack},
    {.ddm2_param = RVCPROP0DATA, .ddm2_item = &l_rvc_prop_data},
    {.ddm2_param = RVCPROP0ADDR, .ddm2_item = &l_rvc_prop_addr},
    {.ddm2_param = RVCPROP0SYNC, .ddm2_item = &l_rvc_prop_sync},
    {.ddm2_param = RVCDCSRCCFGTWO0INST, .ddm2_item = &l_dc_src_cfg_two_inst},
    {.ddm2_param = RVCDCSRCCFGTWO0CLRHIST, .ddm2_item = &l_dc_src_cfg_two_clrhist},
    {.ddm2_param = RVCDCSRCCFGTWO0SYNC, .ddm2_item = &l_dc_src_cfg_two_sync},
    {.ddm2_param = RVCDCSRCCFG0INST, .ddm2_item = &l_dc_src_cfg_inst},
    {.ddm2_param = RVCDCSRCCFG0FACT, .ddm2_item = &l_dc_src_cfg_fact},
    {.ddm2_param = RVCDCSRCCFG0EXP, .ddm2_item = &l_dc_src_cfg_exp},
    {.ddm2_param = RVCDCSRCCFG0COEFF, .ddm2_item = &l_dc_src_cfg_coeff},
    {.ddm2_param = RVCDCSRCCFG0SYNC, .ddm2_item = &l_dc_src_cfg_sync},
    {.ddm2_param = RVCMGNT0ADDR, .ddm2_item = &l_rvc_mgmt_addr},
};

static const mapped_params_t rvc_mdcprofile_params[] = {
    {.sub_ddm2_param = RVCDCSRCCFGTWO0CHGVOLT, .owned_ddm2_param = MDCPROFILE0CHGVOLT},
    {.sub_ddm2_param = RVCDCSRCCFG0FACT, .owned_ddm2_param = MDCPROFILE0CHGEFF},
    {.sub_ddm2_param = RVCDCSRCCFG0EXP, .owned_ddm2_param = MDCPROFILE0PEUKCOEFF},
    {.sub_ddm2_param = RVCDCSRCCFG0COEFF, .owned_ddm2_param = MDCPROFILE0TEMPCOEFF},
    {.sub_ddm2_param = RVCDCSRCCFG0TAILCURR, .owned_ddm2_param = MDCPROFILE0TAILCURR},
};

static const mapped_params_t rvc_mshunt_params[] = {
    {.sub_ddm2_param = RVCDCSRCCFGTWO0SHTVOLT, .owned_ddm2_param = MSHUNT0RVOLT},
    {.sub_ddm2_param = RVCDCSRCCFGTWO0SHTCURR, .owned_ddm2_param = MSHUNT0RCURR},
    {.sub_ddm2_param = RVCDCDISCONN0STS, .owned_ddm2_param = MSHUNT0EXTREL},
    /* MSHUNT0STATUS   Not supported will be added later */
};

static const batt_err_mapping_t batt_err_mapping_table[] =
    {
        {RVCDCSRCSIX0HVLIM, MPS_DC_HIGH_VOLT_LIMIT_REACHED},
        {RVCDCSRCSIX0HVDIS, MPS_DC_HIGH_VOLT_LIMIT_CHARGE_DISCONNECTED},
        {RVCDCSRCSIX0LVLIM, MPS_DC_LOW_VOLT_LIMIT_REACHED},
        {RVCDCSRCSIX0LVDIS, MPS_DC_LOW_VOLT_LIMIT_LOAD_DISCONNECTED},
        {RVCDCSRCSIX0LSOCLIM, MPS_DC_LOW_STATE_CHARGE_LIMIT_REACHED},
        {RVCDCSRCSIX0LSOCDIS, MPS_DC_LOW_STATE_CHARGE_LIMIT_LOAD_DISCONNECTED},
        {RVCDCSRCSIX0LDCTEMPLIM, MPS_DC_LOW_TEMP_LIMIT_REACHED},
        {RVCDCSRCSIX0LDCTEMPDIS, MPS_DC_LOW_TEMP_LIMIT_DISCONNECTED},
        {RVCDCSRCSIX0HDCCURRLIM, MPS_DC_HIGH_CURR_LIMIT_REACHED},
        {RVCDCSRCSIX0HDCCURRDIS, MPS_DC_HIGH_CURR_LIMIT_DISCONNECTED},
        {RVCDCSRCSIX0HDCTEMPLIM, MPS_DC_HIGH_TEMP_LIMIT_REACHED},
        {RVCDCSRCSIX0HDCTEMPDIS, MPS_DC_HIGH_TEMP_LIMIT_DISCONNECTED},
};

static EXT_RAM_ATTR gp_shunt_stscmd_1_t gp_shunt_stscmd_1_stored;
static EXT_RAM_ATTR gp_shunt_stscmd_2_t gp_shunt_stscmd_2_stored;

static int32_t convert_mdcprofile0dischgvolt(uint32_t raw_value)
{
    /* Precision = 0.05 V, Value range = 0 to 3212.5 V, 0xFFFF – Do not change */
    int32_t voltage_mV = (int32_t)raw_value * 50;
    return voltage_mV;
}

static int32_t convert_mshunt0dischgfloor(uint32_t raw_value)
{
    int32_t value = 0;
    if (raw_value <= 200)
    {
        value = (raw_value * Ddm2_unit_factor_list[DDM2_UNIT_PERCENT]) / 2;
    }
    else
    {
        LOG(W, "mshunt0dischgfloor out of range value %d", raw_value);
    }
    return value;
}

static int32_t convert_mshunt0hvoltlim(uint32_t raw_value)
{
    /* Precision = 0.05 V, Value range = 0 to 3212.5 V, 0xFFFF – Do not change */
    int32_t voltage_mV = (int32_t)raw_value * 50;
    return voltage_mV;
}

static int32_t convert_mshunt0lttemplim(uint32_t raw_value)
{
    /* Precision = 1 C, Value range = -40 to 210 C */
    return ((int32_t)raw_value - 40) * Ddm2_unit_factor_list[DDM2_UNIT_DEGC];
}
static int32_t convert_mshunt0htemplim(uint32_t raw_value)
{
    /* Precision = 1 C, Value range = -40 to 210 C */
    return ((int32_t)(raw_value - 40) * Ddm2_unit_factor_list[DDM2_UNIT_DEGC]);
}
static int32_t convert_mshunt0hcurrlim(uint32_t raw_value)
{
    /* Precision = 0.05 A, Value range = -1600 to 1612.5 A */
    return ((int32_t)raw_value * 50) - (1600 * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]);
}

static int32_t convert_mshunt0sysvolt(uint32_t raw_value)
{
    int32_t value = 0;
    switch (raw_value)
    {
    case 1:
        value = 12 * Ddm2_unit_factor_list[DDM2_UNIT_VOLT];
        break;
    case 2:
        value = 24 * Ddm2_unit_factor_list[DDM2_UNIT_VOLT];
        break;
    case 3:
        value = 36 * Ddm2_unit_factor_list[DDM2_UNIT_VOLT];
        break;
    case 4:
        value = 48 * Ddm2_unit_factor_list[DDM2_UNIT_VOLT];
        break;
    default:
        break;
    }
    return value;
}

static void process_shunt(uint8_t *rvc_frame, uint8_t cmd, int32_t dc_instance)
{
    SORTED_LIST_VALUE_TYPE mshunt_instance;
    ddm_class_desc_t *mshunt_class;
    SORTED_LIST_VALUE_TYPE mdcprofile_instance;
    ddm_class_desc_t *mdcprofile_class;

    if (shunt.avl)
    {
        if ((sorted_list_unique_get(&mshunt_instance, &dc_inst_mshunt_ddm_inst_mapping_table, dc_instance, 0) == SORTED_LIST_OK) &&
            (sorted_list_unique_get(&mdcprofile_instance, &dc_inst_mdcprofile_ddm_inst_mapping_table, dc_instance, 0) == SORTED_LIST_OK))
        {
            mshunt_class = ddm_class_desc_find_by_ddm_instance(&l_mshunt, (uint8_t)mshunt_instance);
            mdcprofile_class = ddm_class_desc_find_by_ddm_instance(&l_mdcprofile, (uint8_t)mdcprofile_instance);
            if ((mshunt_class != NULL) && (mdcprofile_class != NULL))
            {
                if (cmd == GP_SHUNT_STS_1)
                {
                    gp_shunt_stscmd_1_t *gp_shunt_stscmd_1 = (gp_shunt_stscmd_1_t *)rvc_frame;
                    if (gp_shunt_stscmd_1->discharge_voltage != gp_shunt_stscmd_1_stored.discharge_voltage)
                    {
                        int32_t value = convert_mdcprofile0dischgvolt(gp_shunt_stscmd_1->discharge_voltage);
                        gp_shunt_stscmd_1_stored.discharge_voltage = gp_shunt_stscmd_1->discharge_voltage;
                        uint32_t param_to_update = MDCPROFILE0DISCHGVOLT | DDM2_PARAMETER_INSTANCE(mdcprofile_instance);
                        ddmw_item_t *item = ddmw_find_item(&ddm_container, param_to_update);
                        if (item != NULL)
                        {
                            if (ddmw_get_i32(item) != value)
                            {
                                ddm_class_desc_update(mdcprofile_class, param_to_update, value);
                            }
                        }
                    }
                    if (gp_shunt_stscmd_1->discharge_floor != gp_shunt_stscmd_1_stored.discharge_floor)
                    {
                        int32_t value = convert_mshunt0dischgfloor(gp_shunt_stscmd_1->discharge_floor);
                        gp_shunt_stscmd_1_stored.discharge_floor = gp_shunt_stscmd_1->discharge_floor;
                        uint32_t param_to_update = MSHUNT0DISCHGFLOOR | DDM2_PARAMETER_INSTANCE(mshunt_instance);
                        ddmw_item_t *item = ddmw_find_item(&ddm_container, param_to_update);
                        if (item != NULL)
                        {
                            if (ddmw_get_i32(item) != value)
                            {
                                ddm_class_desc_update(mshunt_class, param_to_update, value);
                            }
                        }
                    }
                    if (gp_shunt_stscmd_1->high_voltage_limit != gp_shunt_stscmd_1_stored.high_voltage_limit)
                    {
                        int32_t value = convert_mshunt0hvoltlim(gp_shunt_stscmd_1->high_voltage_limit);
                        gp_shunt_stscmd_1_stored.high_voltage_limit = gp_shunt_stscmd_1->high_voltage_limit;
                        uint32_t param_to_update = MSHUNT0HVOLTLIM | DDM2_PARAMETER_INSTANCE(mshunt_instance);
                        ddmw_item_t *item = ddmw_find_item(&ddm_container, param_to_update);
                        if (item != NULL)
                        {
                            if (ddmw_get_i32(item) != value)
                            {
                                ddm_class_desc_update(mshunt_class, param_to_update, value);
                            }
                        }
                    }
                }
                else if (cmd == GP_SHUNT_STS_2)
                {
                    gp_shunt_stscmd_2_t *gp_shunt_stscmd_2 = (gp_shunt_stscmd_2_t *)rvc_frame;
                    if (gp_shunt_stscmd_2->low_temp_lim != gp_shunt_stscmd_2_stored.low_temp_lim)
                    {
                        int32_t value = convert_mshunt0lttemplim(gp_shunt_stscmd_2->low_temp_lim);
                        gp_shunt_stscmd_2_stored.low_temp_lim = gp_shunt_stscmd_2->low_temp_lim;
                        uint32_t param_to_update = MSHUNT0LTTEMPLIM | DDM2_PARAMETER_INSTANCE(mshunt_instance);
                        ddmw_item_t *item = ddmw_find_item(&ddm_container, param_to_update);
                        if (item != NULL)
                        {
                            if (ddmw_get_i32(item) != value)
                            {
                                ddm_class_desc_update(mshunt_class, param_to_update, value);
                            }
                        }
                    }
                    if (gp_shunt_stscmd_2->high_temp_lim != gp_shunt_stscmd_2_stored.high_temp_lim)
                    {
                        int32_t value = convert_mshunt0htemplim(gp_shunt_stscmd_2->high_temp_lim);
                        gp_shunt_stscmd_2_stored.high_temp_lim = gp_shunt_stscmd_2->high_temp_lim;
                        uint32_t param_to_update = MSHUNT0HTEMPLIM | DDM2_PARAMETER_INSTANCE(mshunt_instance);
                        ddmw_item_t *item = ddmw_find_item(&ddm_container, param_to_update);
                        if (item != NULL)
                        {
                            if (ddmw_get_i32(item) != value)
                            {
                                ddm_class_desc_update(mshunt_class, param_to_update, value);
                            }
                        }
                    }
                    if (gp_shunt_stscmd_2->high_cur_lim != gp_shunt_stscmd_2_stored.high_cur_lim)
                    {
                        int32_t value = convert_mshunt0hcurrlim(gp_shunt_stscmd_2->high_cur_lim);
                        gp_shunt_stscmd_2_stored.high_cur_lim = gp_shunt_stscmd_2->high_cur_lim;
                        uint32_t param_to_update = MSHUNT0HCURRLIM | DDM2_PARAMETER_INSTANCE(mshunt_instance);
                        ddmw_item_t *item = ddmw_find_item(&ddm_container, param_to_update);
                        if (item != NULL)
                        {
                            if (ddmw_get_i32(item) != value)
                            {
                                ddm_class_desc_update(mshunt_class, param_to_update, value);
                            }
                        }
                    }
                    if (gp_shunt_stscmd_2->system_voltage != gp_shunt_stscmd_2_stored.system_voltage)
                    {
                        int32_t value = convert_mshunt0sysvolt(gp_shunt_stscmd_2->system_voltage);
                        gp_shunt_stscmd_2_stored.system_voltage = gp_shunt_stscmd_2->system_voltage;
                        uint32_t param_to_update = MSHUNT0SYSVOLT | DDM2_PARAMETER_INSTANCE(mshunt_instance);
                        ddmw_item_t *item = ddmw_find_item(&ddm_container, param_to_update);
                        if (item != NULL)
                        {
                            if (ddmw_get_i32(item) != value)
                            {
                                ddm_class_desc_update(mshunt_class, param_to_update, value);
                            }
                        }
                    }
                }
                else
                {
                    LOG(W, "Proprietary command %d not supported", cmd);
                }
            }
            else
            {
                LOG(E, "MSHUNT OR MDCPROFILE class not found");
            }
        }
        else
        {
            LOG(E, "MSHUNT OR MDCPROFILE instance not found");
        }
    }
}

static void gp_req_timer_handler(TimerHandle_t xTimer)
{
    uint8_t inst = (uint8_t)(uintptr_t)pvTimerGetTimerID(xTimer);
    ddmw_send_generic_event_data(&ddm_container, BATT_MGMT_MBATHTR_REQUEST_EVENT, &inst, sizeof(inst));
}

void send_raw_rvc_frame(uint8_t *data, uint32_t dgn)
{
    RVCMGNT0RAWTX_T txmsg;
    txmsg.is_ext_type = true;
    txmsg.addr = (uint8_t)ddmw_get_i32(&l_rvc_mgmt_addr);
    txmsg.dgn = dgn;
    memcpy(txmsg.data, data, sizeof(txmsg.data));
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, RVCMGNT0RAW, &txmsg, sizeof(txmsg), connector_batt_mgmt_service.connector_id, portMAX_DELAY);
}

/**
 * @brief Common handler for processing RVC class to MBAT/PROD linkage
 *
 * @param class_base RVC class base (e.g., RVCBATSUM0 or RVCGENCFG0)
 * @param instance RVC class instance
 * @param dc_inst DC instance value
 * @param addr Source address value
 */
static void process_rvc_class_linkage(uint32_t class_base, uint8_t instance, uint8_t dc_inst, uint8_t addr)
{
    /* Find MBAT instance for this DC instance */
    // TODO mbat is representing a battery instance, not dc instance
    SORTED_LIST_VALUE_TYPE mbat_ddm_instance;
    if (sorted_list_unique_get(&mbat_ddm_instance, &dc_inst_mbat_ddm_inst_mapping_table, dc_inst, 0) != SORTED_LIST_OK)
    {
        LOG(W, "No MBAT instance found for DC instance %d", dc_inst);
        return;
    }
    LOG(D, "Found MBAT instance %d for addr:0x%x-%d", mbat_ddm_instance, addr, addr);
    if (get_mbat_inst_by_sa(addr) == INVALID_INSTANCE)
    {
        LOG(W, "No MBAT instance found for addr 0x%x - %d", addr, addr);
    }

    int prod_ddm_inst = ProdDBSearchCache(&addr, FIELD_PROP_SA);
    if (prod_ddm_inst < 1)
    {
        return;
    }
    /* Get all PROD instances mapped to this MBAT instance */
    SORTED_LIST_VALUE_TYPE prod_instance;
    if (sorted_list_unique_get(&prod_instance, &mbat_prod_inst_mapping_table, mbat_ddm_instance, 0) != SORTED_LIST_OK)
    {
        LOG(W, "No PROD instance found for MBAT instance %d", mbat_ddm_instance);
        return;
    }
    if (prod_ddm_inst != (int)prod_instance)
    {
        LOG(W, "No matching MBAT->prod mapping for PROD->addr: 0x%x-%d", addr, addr);
        return;
    }

    /* Create RVC class instance parameter */
    uint32_t rvc_class_instance = class_base | DDM2_PARAMETER_INSTANCE(instance);

    /* Find the PROD class descriptor */
    ddm_class_desc_t *prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, (uint8_t)prod_instance);
    if (prod_inst == NULL)
    {
        LOG(E, "Failed to find PROD class descriptor for instance %d", prod_instance);
        return;
    }

    /* Add RVC class to MBAT->RVC classes mapping */
    if (sorted_list_unique_add(&rvc_mbat_ddm_inst_mapping_table, rvc_class_instance, mbat_ddm_instance) != SORTED_LIST_ENTRY_INSERTED)
    {
        LOG(W, "RVC class 0x%08X instance %d already mapped to MBAT instance %d", class_base, instance, mbat_ddm_instance);
        return;
    }

    LOG(D, "Added RVC class 0x%08X instance %d to MBAT->RVC mapping for MBAT instance %d", class_base, instance, mbat_ddm_instance);

    /* Update MBATXLINKED parameter */
    ddm_class_desc_t *mbat_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)mbat_ddm_instance);
    if (mbat_ddm_inst != NULL)
    {
        SORTED_LIST_KEY_TYPE mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

        if (sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
        {
            ddmw_item_t *p_mbatlinked_item = ddmw_find_item(&ddm_container, MBAT0LINKED | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance));
            if (p_mbatlinked_item)
            {
                ddmw_set_data(p_mbatlinked_item, mbat_linked, no_of_mbat_linked * sizeof(mbat_linked[0]));
                // ddmw_set_data(&mbat_ddm_inst->params_items[mbat_linked_idx].ddm2_item, mbat_linked, no_of_mbat_linked * sizeof(mbat_linked[0]));
                LOG(D, "Updated MBAT%dLINKED with %d linked classes", mbat_ddm_instance, no_of_mbat_linked);
            }
            else
            {
                LOG(W, "Could not find  MBAT%uLINKED item", (uint8_t)mbat_ddm_instance);
            }
        }
        else
        {
            LOG(E, "Failed to get linked RVC classes for MBAT instance %d", mbat_ddm_instance);
        }
    }
    else
    {
        LOG(E, "Failed to find MBAT class descriptor for instance %d", mbat_ddm_instance);
    }
}

static void process_batt_summ_params(uint8_t instance)
{
    int32_t batt_inst = 0, dc_inst = 0, addr = 0;

    ddmw_item_t *batt_item = ddmw_find_item(&ddm_container, RVCBATSUM0INST | DDM2_PARAMETER_INSTANCE(instance));
    ddmw_item_t *dc_item = ddmw_find_item(&ddm_container, RVCBATSUM0DC | DDM2_PARAMETER_INSTANCE(instance));
    ddmw_item_t *addr_item = ddmw_find_item(&ddm_container, RVCBATSUM0ADDR | DDM2_PARAMETER_INSTANCE(instance));

    if ((batt_item == NULL) || (dc_item == NULL) || (addr_item == NULL))
    {
        LOG(E, "Failed to find DDM items for RVCBATSUM%d parameters", instance);
        return;
    }

    batt_inst = ddmw_get_i32(batt_item);
    dc_inst = ddmw_get_i32(dc_item);
    addr = ddmw_get_i32(addr_item);

    LOG(D, "All RVCBATSUM%d parameters received: batt_inst=%d, dc_inst=%d, addr=0x%02X", instance, batt_inst, dc_inst, addr);

    if ((batt_inst == 0) || (dc_inst == 0))
    {
        LOG(W, "Data out of sync... (batt_inst:%d) (dc_inst:%d)", batt_inst, dc_inst);
        return;
    }
    /* Process the battery mapping (RVCBATSUM-specific) */
    process_batt_mapping(batt_inst, dc_inst, addr);

    /* Process common RVC class linkage */
    process_rvc_class_linkage(RVCBATSUM0, instance, dc_inst, addr);
}

static void process_gen_cfg_params(uint8_t instance)
{
    int32_t fw_rev = 0;
    int32_t addr = 0;

    ddmw_item_t *fw_rev_item = ddmw_find_item(&ddm_container, RVCGENCFG0FWREV | DDM2_PARAMETER_INSTANCE(instance));
    ddmw_item_t *addr_item = ddmw_find_item(&ddm_container, RVCGENCFG0ADDR | DDM2_PARAMETER_INSTANCE(instance));

    if ((fw_rev_item == NULL) || (addr_item == NULL))
    {
        LOG(E, "Failed to find DDM items for RVCGENCFG%d parameters", instance);
        return;
    }

    fw_rev = ddmw_get_i32(fw_rev_item);
    addr = ddmw_get_i32(addr_item);

    if (addr == 0)
    {
        // Ignore
        return;
    }
    LOG(D, "All RVCGENCFG%d parameters received: fw_rev=%u, addr=0x%02X", instance, fw_rev, addr);

    /* Update firmware version (RVCGENCFG-specific) */
    /* TODO Add check for specific model */
    int prod_inst = ProdDBSearchCache(&addr, FIELD_PROP_SA);
    if (prod_inst > 0)
    {
        // This is only valid for GoPower batteries
        int32_t type = ProdDBGetProductType(prod_inst);
        if (type == PRODDB_PRODUCTTYPE_BATTERY)
        {
            char manufacturer[32];
            manufacturer[0] = '\0';
            size_t sizemanuf = sizeof(manufacturer);
            ProdDBReadCache(FIELD_MANUF, prod_inst, &manufacturer, &sizemanuf);
            if ((strstr(manufacturer, "GOPOWER") != NULL) || (strstr(manufacturer, "Go Power!") != NULL))
            {
                // Only write when we don't have valid already
                char fw_rev_str[PROD_DB_MAX_FIELD_SIZE] = {0};
                size_t fw_rev_str_size = sizeof(fw_rev_str);
                ProdDBReadCache(FIELD_FWVER, prod_inst, fw_rev_str, &fw_rev_str_size);
                if (!ProdDBIsValidSemverVersion(fw_rev_str))
                {
                    snprintf(fw_rev_str, sizeof(fw_rev_str), "%u", fw_rev);

                    if (ProdDBUpdateCache(fw_rev_str, strlen(fw_rev_str) + 1, FIELD_FWVER, prod_inst))
                    {
                        LOG(D, "Updated FWVER to %s (SA: 0x%02X, DDM instance: %d)", fw_rev_str, addr, prod_inst);
                    }
                    else
                    {
                        LOG(D, "Failed to update FWVER (SA: 0x%02X)", addr);
                    }
                }
            }
        }
    }
}

static void connector_batt_mgmt_service_process_task(const DDMP2_FRAME *frame)
{
    /* Process the frame received from broker and update the corresponding parameter */
    ddmw_process(&ddm_container, frame);
    /* If the frame is PUBLISH, proceed with updating the owned parameters from the subscribed ones */
    if (frame->frame.control == DDMP2_CONTROL_PUBLISH)
    {
        uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter);

        switch (DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
        {
        case PROD0MDL:
        {
            uint8_t prod_inst = DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter);
            char model_buf[32] = {0};
            ddmw_item_t *model_item = ddmw_find_item(&ddm_container, frame->frame.publish.parameter);
            if (model_item != NULL)
            {
                ddmw_get_data(model_item, model_buf, sizeof(model_buf));
            }
            LOG(D, "Received model: %s", model_buf);
            gp_device_model_t detected = NO_MODEL;
            SORTED_LIST_VALUE_TYPE cached = NO_MODEL;
            if (sorted_list_unique_get(&cached, &prod_model_table, prod_inst, 0) == SORTED_LIST_OK)
            {
                detected = (gp_device_model_t)cached;
                if (detected == GP_BATT)
                {
                    ProdDBProdClassNodeAddIndicateHandler(prod_inst, batt_indicate_request_handler);
                }
            }
        }
        break;
        case RVCGENCFG0SYNC:
        {
            ddmw_item_t *sync_item = ddmw_find_item(&ddm_container, RVCGENCFG0SYNC | DDM2_PARAMETER_INSTANCE(instance));
            if (sync_item != NULL)
            {
                if (ddmw_get_i32(sync_item) == 1)
                {
                    process_gen_cfg_params(instance);
                }
            }
        }
        break;

        case RVCMGNT0ACK:
        {
            RVCMGNT0ACK_T ack_msg;
            ddmw_get_data(&l_rvc_mgmt_ack, &ack_msg, sizeof(RVCMGNT0ACK_T));
            if (ack_msg.dgn == DGN_DC_SOURCE_CONFIGURATION_COMMAND_2)
            {
                SORTED_LIST_VALUE_TYPE mbat_ddm_inst = INVALID_DDM_INSTANCE;
                if ((sorted_list_unique_get(&mbat_ddm_inst, &dc_inst_mbat_ddm_inst_mapping_table, ack_msg.inst, 0) == SORTED_LIST_OK) && (ack_parameter == MBATHIST0CLRHIST))
                {
                    /* Clean the parameter to acknowledge */
                    ack_parameter = 0;
                    ddmw_item_t *mbathist_clrhist_item = ddmw_find_item(&ddm_container, MBATHIST0CLRHIST | DDM2_PARAMETER_INSTANCE(mbat_ddm_inst));
                    if (mbathist_clrhist_item != NULL)
                    {
                        if (ack_msg.ack_code == 0)
                        {
                            ddmw_set_i32(mbathist_clrhist_item, One);
                        }
                        else
                        {
                            ddmw_set_i32(mbathist_clrhist_item, Zero);
                            LOG(E, "Failed to set DC_SOURCE_CFG_CMD_2 for DC instance %d, ack code %d", ack_msg.inst, ack_msg.ack_code);
                        }
                    }
                    else
                    {
                        LOG(E, "Failed to find MBATHIST0CLRHIST item for MBAT instance %d", mbat_ddm_inst);
                    }
                }
                SORTED_LIST_VALUE_TYPE mshunt_inst = INVALID_DDM_INSTANCE;
                if ((sorted_list_unique_get(&mshunt_inst, &dc_inst_mshunt_ddm_inst_mapping_table, ack_msg.inst, 0) == SORTED_LIST_OK) && (ack_parameter == MSHUNT0SETCAP))
                {
                    /* Clean the parameter to acknowledge */
                    ack_parameter = 0;
                    ddmw_item_t *mshunt_setcap_item = ddmw_find_item(&ddm_container, MSHUNT0SETCAP | DDM2_PARAMETER_INSTANCE(mshunt_inst));
                    if (mshunt_setcap_item != NULL)
                    {
                        if (ack_msg.ack_code == 0)
                        {
                            ddmw_set_i32(mshunt_setcap_item, One);
                        }
                        else
                        {
                            ddmw_set_i32(mshunt_setcap_item, Zero);
                            LOG(E, "Failed to set DC_SOURCE_CFG_CMD_2 for DC instance %d, ack code %d", ack_msg.inst, ack_msg.ack_code);
                        }
                    }
                }
                else
                {
                    LOG(W, "No MSHUNT instance found for DC instance %d", ack_msg.inst);
                }
            }
            if ((ack_msg.dgn == RVC_PROPRIETARY) && (prop_cmd_3_was_sent == true))
            {
                prop_cmd_3_was_sent = false;
                SORTED_LIST_VALUE_TYPE mshunt_inst = INVALID_DDM_INSTANCE;
                if (sorted_list_unique_get(&mshunt_inst, &dc_inst_mbat_ddm_inst_mapping_table, ack_msg.inst, 0) == SORTED_LIST_OK)
                {

                    ddmw_item_t *mshunt_calib_zero = ddmw_find_item(&ddm_container, MSHUNT0CALIBZERO | DDM2_PARAMETER_INSTANCE(mshunt_inst));
                    if (mshunt_calib_zero != NULL)
                    {
                        if (ack_msg.ack_code == 0)
                        {
                            ddmw_set_i32(mshunt_calib_zero, One);
                        }
                        else
                        {
                            ddmw_set_i32(mshunt_calib_zero, Zero);
                            LOG(E, "Failed to set GP Shunt Config CMD 3 for DC instance %d, ack code %d", ack_msg.inst, ack_msg.ack_code);
                        }
                    }
                    else
                    {
                        LOG(E, "Failed to find MSHUNT0CALIBZERO item for MSHUNT instance %d", mshunt_inst);
                    }
                }
            }
        }
        break;

        case RVCBATSUM0SYNC:
        {
            ddmw_item_t *sync_item = ddmw_find_item(&ddm_container, RVCBATSUM0SYNC | DDM2_PARAMETER_INSTANCE(instance));
            if (sync_item != NULL)
            {
                if (ddmw_get_i32(sync_item) == 1)
                {
                    process_batt_summ_params(instance);
                }
            }
        }
        break;

        /* Special handling for PRODXPROP parameter, the registration, management and linkage for the MBAT, MBATHIST, MSHUNT, MDCPROFILE
         classes goes through this parameter */
        case PROD0PROP:
        {
            PROD0PROP_T *prop_data = (PROD0PROP_T *)(frame->frame.publish.value.raw);
            prodxprop_type_t type = {0};
            type.data = prop_data->type;
            if ((type.type.cls == PRODXPROP_TYPE_CLASS_POWER) && (type.type.intf == PRODXPROP_TYPE_INTERFACE_RVC))
            {
                // Check product type from DB
                gp_device_model_t cached_model = NO_MODEL;
                uint8_t prod_inst = DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter);
                SORTED_LIST_VALUE_TYPE cached_value;
                if (sorted_list_unique_get(&cached_value, &prod_model_table, prod_inst, 0) == SORTED_LIST_OK)
                {
                    cached_model = (gp_device_model_t)cached_value;
                }
                else
                {
                    int32_t proddbtype = ProdDBGetProductType(prod_inst);
                    if (proddbtype == PRODDB_PRODUCTTYPE_SHUNT)
                    {
                        LOG(D, "Shunt type detected for instance: %d", prod_inst);
                        cached_model = GP_SHUNT;
                    }
                    else if (proddbtype == PRODDB_PRODUCTTYPE_BATTERY)
                    {
                        LOG(D, "Battery type detected for instance: %d", prod_inst);
                        cached_model = GP_BATT;
                    }
                    cached_value = cached_model;
                    sorted_list_unique_add(&prod_model_table, prod_inst, cached_value);
                }
                if (cached_model != NO_MODEL)
                {
                    // Only subscribe in case of correct type
                    size_t no_of_prop_classes = (ddmp2_value_size(frame) - sizeof(PROD0PROP_T)) / sizeof(uint32_t);
                    for (size_t i = 0; i < no_of_prop_classes; i++)
                    {
                        subscribe_to_prop_classes(prop_data->classes[i]);
                    }

                    SORTED_LIST_VALUE_TYPE mbat_ddm_instance;

                    if (sorted_list_unique_get(&mbat_ddm_instance, &dc_inst_mbat_ddm_inst_mapping_table, prop_data->inst, 0) != SORTED_LIST_OK)
                    {
                        LOG(D, "No MBAT instance found for DC instance %d", prop_data->inst);
                        mbat_ddm_instance = INVALID_DDM_INSTANCE;
                    }
                    else
                    {
                        LOG(D, "Found MBAT instance %d for DC instance %d", mbat_ddm_instance, prop_data->inst);
                    }

                    /* Manage the MBAT, MBATHIST, MSHUNT, MDCPROFILE classses (registering, linkage, deregistering etc.)*/
                    for (size_t i = 0; i < no_of_prop_classes; i++)
                    {
                        if (cached_model == GP_SHUNT)
                        {
                            if (is_rvc_class_mshunt_class_related(prop_data->classes[i]))
                            {
                                manage_mshunt_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                            }
                            if (is_rvc_class_mbat_class_related(prop_data->classes[i]))
                            {
                                manage_mbat_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), cached_model);
                            }
                            if (is_rvc_class_mbathist_class_related(prop_data->classes[i]))
                            {
                                manage_mbathist_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), mbat_ddm_instance);
                            }
                            if (is_rvc_class_mdcprofile_class_related(prop_data->classes[i]))
                            {
                                manage_mdcprofile_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), mbat_ddm_instance);
                            }
                        }
                        else
                        {
                            if (is_rvc_class_mbat_class_related(prop_data->classes[i]))
                            {
                                manage_mbat_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), cached_model);
                            }
                            if (is_rvc_class_mbathist_class_related(prop_data->classes[i]))
                            {
                                manage_mbathist_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), mbat_ddm_instance);
                            }
                        }
                    }
                }
            }
        }
        break;

        /* For all the other RVCDCSRC<X> parameters, proceed with standard updating of the corresponding MBAT parameters */
        case RVCDCSRCTHR0SOH:
        case RVCDCSRCTHR0CAP:
        case RVCDCSRCTHR0RELCAP:
        case RVCDCSRCTHR0RIPPLE:
        case RVCDCSRCELE0DISCHGST:
        case RVCDCSRCELE0CHGST:
        case RVCDCSRCELE0CHGDET:
        case RVCDCSRCELE0RESST:
        case RVCDCSRCELE0CAPACITY:
        case RVCDCSRCTWO0SOC:
        case RVCDCSRCTWO0STEMP:
        case RVCDCSRCTWO0TIMEREM:
        case RVCDCSRCTWO0TIMEREMTYPE:
        case RVCDCSRC0VOLT:
        case RVCDCSRC0CURR:
        case RVCDCSRCFOUR0CHGST:
        case RVCDCSRCFOUR0CURR:
        case RVCDCSRCFOUR0VOLT:
        case RVCDCSRCFIVE0VOLT:
        {
            uint32_t mbat_ddm_inst;
            ddm_class_desc_t *mbat_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MBAT DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mbat_ddm_inst, &rvc_mbat_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MBAT list*/
                mbat_class = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)mbat_ddm_inst);
                if (mbat_class != NULL)
                {
                    uint32_t mbat_param;
                    /* Depending on the RVCDCSRC parameter, get the mapped MBAT parameter that needs to be updated */
                    if (sorted_list_unique_get(&mbat_param, &rvc_mbat_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCDCSRCTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrctwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCDCSRCTHR0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcthr, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCDCSRCELE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcele, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCDCSRC0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrc, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCDCSRCFOUR0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcfour, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCDCSRCFIVE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcfive, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        default:
                            break;
                        }
                        if (rvc_class != NULL)
                        {
                            for (uint32_t i = 0; i < rvc_class->ddm_class_desc_size; i++)
                            {
                                if (rvc_class->params_items[i].ddm2_param == frame->frame.publish.parameter)
                                {
                                    /* Find the mapping entry for this parameter to get the factors */
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_mbat_params); map_idx++)
                                    {
                                        if (rvc_mbat_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            // Add special case for current
                                            if (rvc_mbat_params[map_idx].owned_ddm2_param == MBAT0CURR)
                                            {
                                                // According to definition of mbat0curr
                                                value = -value;
                                            }
                                            ddm_class_desc_update(mbat_class, mbat_param | DDM2_PARAMETER_INSTANCE(mbat_ddm_inst), value);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
        case RVCDCSRCFOUR0TYPE:
        {
            uint32_t mbat_ddm_inst;
            ddm_class_desc_t *rvc_class = NULL;
            ddm_class_desc_t *mbat_class = NULL;

            /* Find the specific MBAT DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mbat_ddm_inst, &rvc_mbat_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcfour, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                if (rvc_class != NULL)
                {
                    int32_t value = 0;

                    for (uint32_t i = 0; i < rvc_class->ddm_class_desc_size; i++)
                    {
                        if (rvc_class->params_items[i].ddm2_param == frame->frame.publish.parameter)
                        {
                            /* Find the specific class descriptor from the MBAT list*/
                            mbat_class = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)mbat_ddm_inst);
                            if (mbat_class != NULL)
                            {
                                /* Find the mapping entry for this parameter to get the factors */
                                for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_mbat_params); map_idx++)
                                {
                                    if (rvc_mbat_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                    {
                                        value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                        ddm_class_desc_update(mbat_class, MBAT0TYPE | DDM2_PARAMETER_INSTANCE(mbat_ddm_inst), value);
                                        break;
                                    }
                                }
                            }

                            /* Get the DC instance for this MBAT */
                            SORTED_LIST_KEY_TYPE dc_inst;
                            int no_of_dc_inst = 1;
                            if (sorted_list_get_keys(&dc_inst, &no_of_dc_inst, &dc_inst_mbat_ddm_inst_mapping_table, mbat_ddm_inst, 0) == SORTED_LIST_OK)
                            {
                                /* Get PROD instance */
                                SORTED_LIST_VALUE_TYPE prod_instance;
                                if (sorted_list_unique_get(&prod_instance, &mbat_prod_inst_mapping_table, mbat_ddm_inst, 0) == SORTED_LIST_OK)
                                {
                                    /* Check if type is "Custom" (value 8) */
                                    if (value == RVC4DCSRC0TYPE_CUSTOM)
                                    {
                                        LOG(D, "MBAT%d is Custom type - checking for MDCPROFILE", mbat_ddm_inst);

                                        /* Check if MDCPROFILE already exists */
                                        uint32_t mdcprofile_inst;
                                        if (sorted_list_unique_get(&mdcprofile_inst, &dc_inst_mdcprofile_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_FAIL)
                                        {
                                            /* Get all RVC classes linked to MBAT to find MDCPROFILE-related ones */
                                            SORTED_LIST_KEY_TYPE rvc_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                                            int no_of_rvc_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                                            if (sorted_list_get_keys(rvc_linked, &no_of_rvc_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_inst, 0) == SORTED_LIST_OK)
                                            {
                                                bool has_mdcprofile_classes = false;
                                                uint32_t first_mdcprofile_class = 0;

                                                /* Check if any MDCPROFILE-related RVC classes exist */
                                                for (int j = 0; j < no_of_rvc_linked; j++)
                                                {
                                                    if (is_rvc_class_mdcprofile_class_related(rvc_linked[j]))
                                                    {
                                                        has_mdcprofile_classes = true;
                                                        first_mdcprofile_class = rvc_linked[j];
                                                        break;
                                                    }
                                                }

                                                if (has_mdcprofile_classes)
                                                {
                                                    LOG(D, "Creating MDCPROFILE for MBAT%d (Custom type) with existing RVC classes", mbat_ddm_inst);
                                                    manage_mdcprofile_classes(first_mdcprofile_class, dc_inst, prod_instance, mbat_ddm_inst);

                                                    /* Process all other MDCPROFILE-related classes */
                                                    for (int j = 0; j < no_of_rvc_linked; j++)
                                                    {
                                                        if ((rvc_linked[j] != first_mdcprofile_class) && (is_rvc_class_mdcprofile_class_related(rvc_linked[j])))
                                                        {
                                                            manage_mdcprofile_classes(rvc_linked[j], dc_inst, prod_instance, mbat_ddm_inst);
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    LOG(D, "No MDCPROFILE-related RVC classes available yet for MBAT%d", mbat_ddm_inst);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            LOG(D, "MDCPROFILE%d already exists for MBAT%d", mdcprofile_inst, mbat_ddm_inst);
                                        }
                                    }
                                    else
                                    {
                                        LOG(D, "MBAT%d is NOT Custom type (%d) - MDCPROFILE will not be created", mbat_ddm_inst, value);
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        break;
        case RVCDCSRCSEV0INPUT:
        case RVCDCSRCSEV0OUTPUT:
        case RVCDCSRCEIG0INPUT:
        case RVCDCSRCEIG0OUTPUT:
        case RVCDCSRCNINE0INPUT:
        case RVCDCSRCNINE0OUTPUT:
        case RVCDCSRCTEN0INPUT:
        case RVCDCSRCTEN0OUTPUT:
        case RVCDCSRCTWE0AVERAGE:
        case RVCDCSRCTWE0DEEP:
        case RVCDCSRCTWE0CYCLES:
        case RVCDCSRCTHI0HIGHVOLT:
        case RVCDCSRCTHI0LOWVOLT:
        {
            uint32_t mbathist_ddm_inst;
            ddm_class_desc_t *mbathist_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MBATHIST DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mbathist_ddm_inst, &rvc_mbathist_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MBATHIST list*/
                mbathist_class = ddm_class_desc_find_by_ddm_instance(&l_mbathist, (uint8_t)mbathist_ddm_inst);
                if (mbathist_class != NULL)
                {
                    uint32_t mbathist_param;
                    /* Depending on the RVCDCSRC parameter, get the mapped MBATHIST parameter that needs to be updated */
                    if (sorted_list_unique_get(&mbathist_param, &rvc_mbathist_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCDCSRCTWE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrctwe, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCTHI0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcthir, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCCFGTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrccfgtwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCSEV0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcsev, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCEIG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrceig, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCNINE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcnine, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCTEN0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcten, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        default:
                            break;
                        }
                        if (rvc_class != NULL)
                        {
                            for (uint32_t i = 0; i < rvc_class->ddm_class_desc_size; i++)
                            {
                                if (rvc_class->params_items[i].ddm2_param == frame->frame.publish.parameter)
                                {
                                    /* Find the mapping entry for this parameter to get the factors */
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_mbathist_params); map_idx++)
                                    {
                                        if (rvc_mbathist_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            ddm_class_desc_update(mbathist_class, mbathist_param | DDM2_PARAMETER_INSTANCE(mbathist_ddm_inst), value);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
        case RVCDCSRCCFGTWO0BATTYPE:
        case RVCDCSRCCFGTWO0CHGVOLT:
        case RVCDCSRCCFG0FACT:
        case RVCDCSRCCFG0EXP:
        case RVCDCSRCCFG0COEFF:
        case RVCDCSRCCFG0TAILCURR:
        {
            uint32_t mdcprofile_inst;
            ddm_class_desc_t *mdcprofile_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MDCPROFILE DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mdcprofile_inst, &rvc_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MDCPROFILE list*/
                mdcprofile_class = ddm_class_desc_find_by_ddm_instance(&l_mdcprofile, (uint8_t)mdcprofile_inst);
                if (mdcprofile_class != NULL)
                {
                    uint32_t mdcprofile_param;
                    /* Depending on the RVCDCSRC parameter, get the mapped MDCPROFILE parameter that needs to be updated */
                    if (sorted_list_unique_get(&mdcprofile_param, &rvc_mdcprofile_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCDCSRCCFG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrccfg, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCCFGTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrccfgtwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        default:
                            break;
                        }
                        if (rvc_class != NULL)
                        {
                            for (uint32_t i = 0; i < rvc_class->ddm_class_desc_size; i++)
                            {
                                if (rvc_class->params_items[i].ddm2_param == frame->frame.publish.parameter)
                                {
                                    /* Find the mapping entry for this parameter to get the factors */
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_mdcprofile_params); map_idx++)
                                    {
                                        if (rvc_mdcprofile_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            ddm_class_desc_update(mdcprofile_class, mdcprofile_param | DDM2_PARAMETER_INSTANCE(mdcprofile_inst), value);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
        case RVCDCSRCCFGTWO0SHTVOLT:
        case RVCDCSRCCFGTWO0SHTCURR:
        case RVCDCDISCONN0STS:
        {
            uint32_t mshunt_inst;
            ddm_class_desc_t *mshunt_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;
            /* Find the specific MSHUNT DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mshunt_inst, &rvc_mshunt_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MSHUNT list*/
                mshunt_class = ddm_class_desc_find_by_ddm_instance(&l_mshunt, (uint8_t)mshunt_inst);
                if (mshunt_class != NULL)
                {
                    uint32_t mshunt_param;
                    /* Depending on the RVCDCSRC parameter, get the mapped MSHUNT parameter that needs to be updated */
                    if (sorted_list_unique_get(&mshunt_param, &rvc_mshunt_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCDCSRCCFGTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrccfgtwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCDISCONN0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcdisconn, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        default:
                            break;
                        }
                        if (rvc_class != NULL)
                        {
                            for (uint32_t i = 0; i < rvc_class->ddm_class_desc_size; i++)
                            {
                                if (rvc_class->params_items[i].ddm2_param == frame->frame.publish.parameter)
                                {
                                    /* Find the mapping entry for this parameter to get the factors */
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_mshunt_params); map_idx++)
                                    {
                                        if (rvc_mshunt_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            ddm_class_desc_update(mshunt_class, mshunt_param | DDM2_PARAMETER_INSTANCE(mshunt_inst), value);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
        case RVCPROP0SYNC:
        {
            if (ddmw_get_i32(&l_rvc_prop_sync) == 1)
            {
                RVCPROP0DATA_T prop_data;
                ddmw_get_data(&l_rvc_prop_data, &prop_data, sizeof(RVCPROP0DATA_T));
                process_prop_data(&prop_data);
            }
        }
        break;
        case RVCDCSRCSIX0HVLIM:
        case RVCDCSRCSIX0HVDIS:
        case RVCDCSRCSIX0LVLIM:
        case RVCDCSRCSIX0LVDIS:
        case RVCDCSRCSIX0LSOCLIM:
        case RVCDCSRCSIX0LSOCDIS:
        case RVCDCSRCSIX0LDCTEMPLIM:
        case RVCDCSRCSIX0LDCTEMPDIS:
        case RVCDCSRCSIX0HDCCURRLIM:
        case RVCDCSRCSIX0HDCCURRDIS:
        case RVCDCSRCSIX0HDCTEMPLIM:
        case RVCDCSRCSIX0HDCTEMPDIS:
        {
            process_pub_mbat_status(frame->frame.publish.parameter);
        }
        break;
        default:
            break;
        }
    }
    else if (frame->frame.control == DDMP2_CONTROL_SET)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(frame->frame.set.parameter))
        {
        case MBAT0HMODE:
        {
            int32_t SAs[MAX_NO_OF_BATT_IN_BANK];
            uint8_t batt_inst_list[MAX_NO_OF_BATT_IN_BANK];
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            TRUE_CHECK(sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mbat_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK);
            uint8_t no_of_sas = get_src_addresses_for_dc_inst(dc_inst_mapped, SAs, batt_inst_list);
            gp_batt_cmd_t gp_batt_cmd;
            memset(&gp_batt_cmd, GP_DEFAULT_VALUE, sizeof(gp_batt_cmd));
            ddmw_item_t *mbathmode_item = ddmw_find_item(&ddm_container, frame->frame.set.parameter);
            if (mbathmode_item != NULL)
            {
                int32_t htr_val = ddmw_get_i32(mbathmode_item);
                /* Send Battery command to all battery instances to set the heater state */
                for (uint8_t i = 0; i < no_of_sas; i++)
                {
                    gp_batt_cmd.pwd = GP_PROP_MSG_PWD;
                    gp_batt_cmd.cmd = GP_BATT_CMD;
                    gp_batt_cmd.batt_inst = batt_inst_list[i];
                    switch (htr_val)
                    {
                    case MBAT0HMODE_AUTOMATIC:
                        /* fallthrough */
                    case MBAT0HMODE_ON:
                        /* fallthrough */
                    case MBAT0HMODE_OFF:
                        gp_batt_cmd.htr_mode = htr_val;
                        process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&SAs[i], sizeof(SAs[0]));
                        process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_batt_cmd, sizeof(gp_batt_cmd_t));
                        process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
                        break;
                    default:
                        LOG(W, "Not correct mode: %d", htr_val);
                        break;
                    }
                }
                uint8_t mbat_inst = DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter);
                vTimerSetTimerID(gp_req_timer, (void *)(uintptr_t)mbat_inst);
                xTimerStart(gp_req_timer, portMAX_DELAY);
            }
        }
        break;
        case MSHUNT0SYSVOLT:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            TRUE_CHECK(sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mshunt_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK);
            gp_shunt_stscmd_2_t gp_shunt_cmd;
            memset(&gp_shunt_cmd, GP_DEFAULT_VALUE, sizeof(gp_shunt_cmd));
            gp_shunt_cmd.pwd = GP_PROP_MSG_PWD;
            gp_shunt_cmd.cmd = GP_SHUNT_CMD_2;
            uint8_t volts = (uint8_t)(frame->frame.set.value.int32 / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
            switch (volts)
            {
            case 12:
            {
                gp_shunt_cmd.system_voltage = 1;
            }
            break;
            case 24:
            {
                gp_shunt_cmd.system_voltage = 2;
            }
            break;
            case 36:
            {
                gp_shunt_cmd.system_voltage = 3;
            }
            break;
            case 48:
            {
                gp_shunt_cmd.system_voltage = 4;
            }
            break;
            default:
            {
                gp_shunt_cmd.system_voltage = 0xf; /* Leave as is */
            }
            break;
            }
            process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&shunt.sa, sizeof(shunt.sa));
            process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_shunt_cmd, sizeof(gp_shunt_stscmd_2_t));
            process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
        }
        break;
        case MSHUNT0CALIBZERO:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            TRUE_CHECK(sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mshunt_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK);
            gp_shunt_cmd_3_t gp_shunt_cmd;
            memset(&gp_shunt_cmd, GP_DEFAULT_VALUE, sizeof(gp_shunt_cmd));

            gp_shunt_cmd.pwd = GP_PROP_MSG_PWD;
            gp_shunt_cmd.cmd = GP_SHUNT_CMD_3;
            gp_shunt_cmd.calibrate_zero_current = (uint8_t)frame->frame.set.value.int32;
            process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&shunt.sa, sizeof(shunt.sa));
            process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_shunt_cmd, sizeof(gp_shunt_cmd_3_t));
            process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
        }
        break;
        case MDCPROFILE0DISCHGVOLT:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            TRUE_CHECK(sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK);
            gp_shunt_stscmd_1_t gp_shunt_cmd;
            memset(&gp_shunt_cmd, GP_DEFAULT_VALUE, sizeof(gp_shunt_cmd));
            gp_shunt_cmd.pwd = GP_PROP_MSG_PWD;
            gp_shunt_cmd.cmd = GP_SHUNT_CMD_1;
            /* RVC value = DDM value / DDM factor for votage - 1000 / precision - 0.05 */
            gp_shunt_cmd.discharge_voltage = (uint16_t)(frame->frame.set.value.int32 / 50);
            process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&shunt.sa, sizeof(shunt.sa));
            process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_shunt_cmd, sizeof(gp_shunt_stscmd_1_t));
            process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
        }
        break;
        case MSHUNT0DISCHGFLOOR:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            TRUE_CHECK(sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK);
            gp_shunt_stscmd_1_t gp_shunt_cmd;
            memset(&gp_shunt_cmd, GP_DEFAULT_VALUE, sizeof(gp_shunt_cmd));
            gp_shunt_cmd.pwd = GP_PROP_MSG_PWD;
            gp_shunt_cmd.cmd = GP_SHUNT_CMD_1;
            /* RVC value = DDM value / DDM factor for percent - 1000 / precision - 0.5 */
            gp_shunt_cmd.discharge_floor = (uint8_t)(frame->frame.set.value.int32 / 500);
            process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&shunt.sa, sizeof(shunt.sa));
            process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_shunt_cmd, sizeof(gp_shunt_stscmd_1_t));
            process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
        }
        break;
        case MBATHIST0CLRHIST:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mbathist_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                gp_dc_src_cfg_cmd_2_t dc_src_cfg_cmd_2;
                memset(&dc_src_cfg_cmd_2, GP_DEFAULT_VALUE, sizeof(dc_src_cfg_cmd_2));
                dc_src_cfg_cmd_2.dc_instance = dc_inst_mapped;
                dc_src_cfg_cmd_2.clear_history = (uint8_t)One;
                send_raw_rvc_frame((uint8_t *)&dc_src_cfg_cmd_2, DGN_DC_SOURCE_CONFIGURATION_COMMAND_2);
                ack_parameter = MBATHIST0CLRHIST;
            }
        }
        break;
        case MDCPROFILE0CHGEFF:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                gp_dc_src_cfg_cmd_1_t dc_src_cfg_cmd_1;
                memset(&dc_src_cfg_cmd_1, GP_DEFAULT_VALUE, sizeof(dc_src_cfg_cmd_1));
                dc_src_cfg_cmd_1.dc_instance = dc_inst_mapped;
                /* Precision 0.5% value range 0 .. 100% therefore multiplied by 2 */
                dc_src_cfg_cmd_1.charge_efficiency_factor = (uint8_t)((frame->frame.set.value.int32 * 2) / Ddm2_unit_factor_list[DDM2_UNIT_PERCENT]);
                send_raw_rvc_frame((uint8_t *)&dc_src_cfg_cmd_1, DGN_DC_SOURCE_CONFIGURATION_COMMAND_1);
            }
            else
            {
                LOG(E, "MDCPROFILE DC instance not found");
            }
        }
        break;
        case MDCPROFILE0PEUKCOEFF:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                gp_dc_src_cfg_cmd_1_t dc_src_cfg_cmd_1;
                memset(&dc_src_cfg_cmd_1, GP_DEFAULT_VALUE, sizeof(dc_src_cfg_cmd_1));
                dc_src_cfg_cmd_1.dc_instance = dc_inst_mapped;
                /* Precision = 0.01, Value range = 0 - 2.53. Therefore divided by 100 */
                dc_src_cfg_cmd_1.peukert_exponent = (uint8_t)(frame->frame.set.value.int32 / (Ddm2_unit_factor_list[DDM2_UNIT_DECIMAL] / 100));
                send_raw_rvc_frame((uint8_t *)&dc_src_cfg_cmd_1, DGN_DC_SOURCE_CONFIGURATION_COMMAND_1);
            }
            else
            {
                LOG(E, "MDCPROFILE DC instance not found");
            }
        }
        break;
        case MDCPROFILE0TEMPCOEFF:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                gp_dc_src_cfg_cmd_1_t dc_src_cfg_cmd_1;
                memset(&dc_src_cfg_cmd_1, GP_DEFAULT_VALUE, sizeof(dc_src_cfg_cmd_1));
                dc_src_cfg_cmd_1.dc_instance = dc_inst_mapped;
                /* Precision = 0.1 %CAP/C, Value range = 0 to 20 %CAP/C. Therefore divided by 10 */
                dc_src_cfg_cmd_1.temperature_coeficient = (uint8_t)(frame->frame.set.value.int32 / (Ddm2_unit_factor_list[DDM2_UNIT_DECIMAL] / 10));
                send_raw_rvc_frame((uint8_t *)&dc_src_cfg_cmd_1, DGN_DC_SOURCE_CONFIGURATION_COMMAND_1);
            }
            else
            {
                LOG(E, "MDCPROFILE DC instance not found");
            }
        }
        break;
        case MDCPROFILE0CHGVOLT:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                gp_dc_src_cfg_cmd_2_t dc_src_cfg_cmd_2;
                memset(&dc_src_cfg_cmd_2, GP_DEFAULT_VALUE, sizeof(dc_src_cfg_cmd_2));
                dc_src_cfg_cmd_2.dc_instance = dc_inst_mapped;
                /* RVC value = DDM value / DDM factor for votage - 1000 / precision 0.05 */
                dc_src_cfg_cmd_2.charged_voltage = (uint16_t)(frame->frame.set.value.int32 / 50);
                send_raw_rvc_frame((uint8_t *)&dc_src_cfg_cmd_2, DGN_DC_SOURCE_CONFIGURATION_COMMAND_2);
            }
            else
            {
                LOG(E, "MDCPROFILE DC instance not found");
            }
        }
        break;
        case MSHUNT0SETCAP:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mshunt_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                gp_dc_src_cfg_cmd_2_t dc_src_cfg_cmd_2;
                memset(&dc_src_cfg_cmd_2, GP_DEFAULT_VALUE, sizeof(dc_src_cfg_cmd_2));
                dc_src_cfg_cmd_2.dc_instance = dc_inst_mapped;
                dc_src_cfg_cmd_2.set_100 = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&dc_src_cfg_cmd_2, DGN_DC_SOURCE_CONFIGURATION_COMMAND_2);
                ack_parameter = MSHUNT0SETCAP;
            }
            else
            {
                LOG(E, "MSHUNT DC instance not found");
            }
        }
        break;
        case MSHUNT0EXTREL:
        {
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mshunt_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                gp_dc_disconnect_cmd_t dc_disconnect_cmd;
                memset(&dc_disconnect_cmd, GP_DEFAULT_VALUE, sizeof(dc_disconnect_cmd));
                dc_disconnect_cmd.dc_instance = dc_inst_mapped;
                dc_disconnect_cmd.command = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&dc_disconnect_cmd, DC_DISCONNECT_COMMAND);
            }
            else
            {
                LOG(E, "MSHUNT DC instance not found");
            }
        }
        break;
        default:
            break;
        }
    }
    else if (frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(frame->frame.subscribe.parameter))
        {
        case MBAT0HTR:
            /* fallthrough */
        case MBAT0HMODE:
        {
            request_mbathtr_status(DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.subscribe.parameter));
        }
        break;
        case MSHUNT0SYSVOLT:
        {
            gp_shunt_stscmd_2_t gp_shunt_cmd;
            memset(&gp_shunt_cmd, GP_DEFAULT_VALUE, sizeof(gp_shunt_cmd));
            gp_shunt_cmd.pwd = GP_PROP_MSG_PWD;
            gp_shunt_cmd.cmd = GP_SHUNT_CMD_2;

            process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&shunt.sa, sizeof(shunt.sa));
            process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_shunt_cmd, sizeof(gp_shunt_stscmd_2_t));
            process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
        }
        break;
        case MDCPROFILE0DISCHGVOLT:
        {
            gp_shunt_stscmd_1_t gp_shunt_cmd;
            memset(&gp_shunt_cmd, GP_DEFAULT_VALUE, sizeof(gp_shunt_cmd));
            gp_shunt_cmd.pwd = GP_PROP_MSG_PWD;
            gp_shunt_cmd.cmd = GP_SHUNT_CMD_1;

            process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&shunt.sa, sizeof(shunt.sa));
            process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_shunt_cmd, sizeof(gp_shunt_stscmd_1_t));
            process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
        }
        break;
        case MBAT0STATUS:
        {
            process_sub_mbat_status(DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.subscribe.parameter));
        }
        break;
        default:
            break;
        }
    }
    else if (frame->frame.control == DDMP2_CONTROL_GENERIC)
    {
        switch (frame->frame.generic.id)
        {
        case BATT_MGMT_MBATHTR_REQUEST_EVENT:
        {
            uint8_t mbat_instance = frame->frame.generic.data[0];
            request_mbathtr_status(mbat_instance);
        }
        break;
        default:
            break;
        }
    }
    /* Process all parameters and publish those that have been updated */
    ddmw_process_publish(&ddm_container);
}

static int connector_batt_mgmt_init(void)
{
#if CONNECTOR_BAT_MGMT_SERVICE_ENABLE_LOG
    esp_log_level_set(FILENAME, ESP_LOG_DEBUG);
    esp_log_level_set("connector_rvc.c", ESP_LOG_DEBUG);
#endif
    ddmw_init(&ddm_container, &connector_batt_mgmt_service);
    ddmw_get_inventory(&ddm_container, inventory_callback);
    memset(&gp_shunt_stscmd_1_stored, 0, sizeof(gp_shunt_stscmd_1_stored));
    memset(&gp_shunt_stscmd_2_stored, 0, sizeof(gp_shunt_stscmd_2_stored));
    memset(&shunt, 0, sizeof(shunt));
    shunt.avl = false;
    prop_cmd_3_was_sent = false;
    ack_parameter = 0;
    gp_req_timer = NULL;

    LIST_INIT(&l_rvcdcsrc);
    LIST_INIT(&l_rvcdcsrctwo);
    LIST_INIT(&l_rvcdcsrcthr);
    LIST_INIT(&l_rvcdcsrcfour);
    LIST_INIT(&l_rvcdcsrcfive);
    LIST_INIT(&l_rvcdcsrcsix);
    LIST_INIT(&l_rvcdcsrcsev);
    LIST_INIT(&l_rvcdcsrceig);
    LIST_INIT(&l_rvcdcsrcnine);
    LIST_INIT(&l_rvcdcsrcten);
    LIST_INIT(&l_rvcdcsrcele);
    LIST_INIT(&l_rvcdcsrctwe);
    LIST_INIT(&l_rvcdcsrcthir);
    LIST_INIT(&l_rvcdcsrccfgtwo);
    LIST_INIT(&l_rvcdcsrccfg);
    LIST_INIT(&l_rvcdcdisconn);
    LIST_INIT(&l_mbat);
    LIST_INIT(&l_mbathist);
    LIST_INIT(&l_mshunt);
    LIST_INIT(&l_mdcprofile);
    LIST_INIT(&l_prod);
    LIST_INIT(&l_batt_map);
    LIST_INIT(&l_batt_sum);
    LIST_INIT(&l_gen_cfg);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mbat_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mbat_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(dc_inst_mbat_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(mbat_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mbathist_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mbathist_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(dc_inst_mbathist_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(mbathist_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mshunt_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mshunt_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(dc_inst_mshunt_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(mshunt_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(dc_inst_mdcprofile_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(mdcprofile_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(prod_model_table);

    INIT_SORTED_LIST_EXTRAM_PTR(mbat_err_table);

    for (unsigned int i = 0; i < ELEMENTS(rvc_mbat_params); i++)
    {
        TRUE_CHECK(sorted_list_unique_add(&rvc_mbat_param_mapping_table, rvc_mbat_params[i].sub_ddm2_param, rvc_mbat_params[i].owned_ddm2_param) == SORTED_LIST_ENTRY_INSERTED);
    }
    for (unsigned int i = 0; i < ELEMENTS(rvc_mbathist_params); i++)
    {
        TRUE_CHECK(sorted_list_unique_add(&rvc_mbathist_param_mapping_table, rvc_mbathist_params[i].sub_ddm2_param, rvc_mbathist_params[i].owned_ddm2_param) == SORTED_LIST_ENTRY_INSERTED);
    }
    for (unsigned int i = 0; i < ELEMENTS(rvc_mshunt_params); i++)
    {
        sorted_list_unique_add(&rvc_mshunt_param_mapping_table, rvc_mshunt_params[i].sub_ddm2_param, rvc_mshunt_params[i].owned_ddm2_param);
    }
    for (unsigned int i = 0; i < ELEMENTS(rvc_mdcprofile_params); i++)
    {
        sorted_list_unique_add(&rvc_mdcprofile_param_mapping_table, rvc_mdcprofile_params[i].sub_ddm2_param, rvc_mdcprofile_params[i].owned_ddm2_param);
    }

    mbat_linked_idx = 0;
    mshunt_linked_idx = 0;
    mdcprofile_linked_idx = 0;
    mbathist_linked_idx = 0;

    TRUE_CHECK(gp_req_timer = xTimerCreate(NULL, pdMS_TO_TICKS(1500), pdFALSE, (void *)0, gp_req_timer_handler));

    return 1;
}

/**
 * @brief This is called for any change in inventory
 *
 * @param parameter
 * @note When there is a proper device detection over RV-C, this inventory handler needs to be refactored, to include instances.
 */
static void inventory_callback(uint32_t parameter)
{
    /* If new PROD<x> class instance becomes available, create specific PROD<x> class descriptor for it,
    add all the items in the DDMW container, subscribe to the parameters and add the class descriptor in the list */
    if (DDMP2_INVENTORY_AVL(parameter))
    {
        switch (DDM2_PARAMETER_CLASS(parameter))
        {
        case PROD0:
        {
            if (DDM2_PARAMETER_INSTANCE_FIELD(parameter) == 0)
            {
                /* Ignore PROD0 */
            }
            else
            {
                ddm_class_desc_t *ddm_class = NULL;

                ddm_class = ddm_class_desc_find_by_ddm_instance(&l_prod, DDM2_PARAMETER_INSTANCE_FIELD(parameter));
                if (ddm_class == NULL)
                {
                    ddm_class = ddm_class_desc_prod_create();
                    if (ddm_class != NULL)
                    {
                        ddm_class_desc_prod_init(DDM2_PARAMETER_INSTANCE_FIELD(parameter), ddm_class);
                        ddmw_item_t *item = &ddm_class->params_items[PROD_CLASS_DESC_PROP_PARAM_INDEX].ddm2_item;
                        uint32_t param = ddm_class->params_items[PROD_CLASS_DESC_PROP_PARAM_INDEX].ddm2_param;
                        ddmw_add(&ddm_container, item, param, DDM2_PARAMETER_INSTANCE_FIELD(parameter));
                        ddmw_set_type(item, DDMW_ACTION_SET);
                        ddmw_subscribe(item);
                        item = &ddm_class->params_items[PROD_CLASS_DESC_CLIST_PARAM_INDEX].ddm2_item;
                        param = ddm_class->params_items[PROD_CLASS_DESC_CLIST_PARAM_INDEX].ddm2_param;
                        ddmw_add(&ddm_container, item, param, DDM2_PARAMETER_INSTANCE_FIELD(parameter));
                        ddmw_set_type(item, DDMW_ACTION_SET);
                        ddmw_subscribe(item);
                        item = &ddm_class->params_items[PROD_CLASS_DESC_MDL_PARAM_INDEX].ddm2_item;
                        param = ddm_class->params_items[PROD_CLASS_DESC_MDL_PARAM_INDEX].ddm2_param;
                        ddmw_add(&ddm_container, item, param, DDM2_PARAMETER_INSTANCE_FIELD(parameter));
                        ddmw_set_type(item, DDMW_ACTION_SET);
                        ddmw_subscribe(item);
                        ddm_class_desc_insert(ddm_class, &l_prod);
                    }
                }
            }
        }
        break;
        case RVCBATSUM0:
        case RVCGENCFG0:
        {
            manage_subscriptions_for_rvc_on_req_class((DDM2_PARAMETER_CLASS(parameter) == RVCBATSUM0) ? &l_batt_sum : &l_gen_cfg, parameter);
        }
        break;
        case RVCMGNT0:
        case RVCPROP0:
        case RVCDCSRCCFGTWO0:
            start_subscriptions_to_rvc_params(DDM2_PARAMETER_CLASS(parameter));
            break;
        default:
            break;
        }
    }
    else
    {
        switch (DDM2_PARAMETER_CLASS(parameter))
        {
        case PROD0:
        {
            if (DDM2_PARAMETER_INSTANCE_FIELD(parameter) == 0)
            {
                /* Ignore PROD0 */
            }
            else
            {
                uint8_t prod_instance = DDM2_PARAMETER_INSTANCE_FIELD(parameter);
                ddm_class_desc_t *ddm_class = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);

                if (ddm_class != NULL)
                {
                    LOG(D, "PROD%d is becoming unavailable - removing all linked classes", prod_instance);

                    /* Remove all MBAT instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE mbat_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mbat = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(mbat_instances, &no_of_mbat, &mbat_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        LOG(D, "Found %d MBAT instances linked to PROD%d", no_of_mbat, prod_instance);
                        for (int i = 0; i < no_of_mbat; i++)
                        {
                            remove_mbat_instance((uint8_t)mbat_instances[i], prod_instance);
                        }
                    }

                    /* Remove all MBATHIST instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE mbathist_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mbathist = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(mbathist_instances, &no_of_mbathist, &mbathist_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mbathist; i++)
                        {
                            remove_mbathist_instance((uint8_t)mbathist_instances[i], prod_instance);
                        }
                    }

                    /* Remove all MDCPROFILE instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE mdcprofile_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mdcprofile = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(mdcprofile_instances, &no_of_mdcprofile, &mdcprofile_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mdcprofile; i++)
                        {
                            remove_mdcprofile_instance((uint8_t)mdcprofile_instances[i], prod_instance);
                        }
                    }

                    /* Remove all MSHUNT instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE mshunt_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mshunt = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(mshunt_instances, &no_of_mshunt, &mshunt_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mshunt; i++)
                        {
                            remove_mshunt_instance((uint8_t)mshunt_instances[i], prod_instance);
                        }
                    }

                    //! \~ Clean indicate request handler for GoPower batteries before remove it.
                    SORTED_LIST_VALUE_TYPE cached_value;
                    if (sorted_list_unique_get(&cached_value, &prod_model_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        if ((gp_device_model_t)cached_value == GP_BATT)
                        {
                            ProdDBProdClassNodeAddIndicateHandler(prod_instance, NULL);
                        }
                    }

                    /* Remove cached model */
                    sorted_list_unique_remove(&prod_model_table, prod_instance);

                    /* Remove PROD DDMW items */
                    for (size_t i = 0; i < ddm_class->ddm_class_desc_size; i++)
                    {
                        ddmw_remove(&ddm_container, &ddm_class->params_items[i].ddm2_item);
                    }

                    /* Remove PROD descriptor */
                    ddm_class_desc_delete(ddm_class);

                    LOG(D, "PROD%d and all its linked classes successfully removed", prod_instance);
                }
            }
        }
        break;
        case RVCMGNT0:
        case RVCPROP0:
        case RVCDCSRCCFGTWO0:
            stop_subscriptions_to_rvc_params(DDM2_PARAMETER_CLASS(parameter));
            break;
        case RVCGENCFG0:
        case RVCBATSUM0:
        {
            /* The removal of RVCBATSUM and RVCGENCFG0 class instances must go in this way until we have PROD/MBAT instances
               for every battery on the RV-C bus, since we are mapping them based on the DC instance (master) instead of
               battery instances */

            uint8_t class_instance = DDM2_PARAMETER_INSTANCE_FIELD(parameter);
            ddm_class_desc_t *rvc_class = NULL;

            if (DDM2_PARAMETER_CLASS(parameter) == RVCGENCFG0)
            {
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_gen_cfg, class_instance);
            }
            else if (DDM2_PARAMETER_CLASS(parameter) == RVCBATSUM0)
            {
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_batt_sum, class_instance);
            }

            if (rvc_class != NULL)
            {
                /* Find the specific MBAT class instance that is linked to that RVCDCSRC instance*/
                SORTED_LIST_VALUE_TYPE mbat_ddm_instance;
                if (sorted_list_unique_get(&mbat_ddm_instance, &rvc_mbat_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(parameter), 0) != SORTED_LIST_OK)
                {
                    LOG(W, "No MBAT instance found for RVC class 0x%08X", DDM2_PARAMETER_CLASS_INSTANCE(parameter));
                }
                else
                {
                    /* Find the specific MBAT class descriptor node from the list */
                    ddm_class_desc_t *mbat_ddm_inst = NULL;
                    mbat_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)mbat_ddm_instance);
                    if (mbat_ddm_inst == NULL)
                    {
                        LOG(E, "MBAT class descriptor for instance %d not found", mbat_ddm_instance);
                    }
                    else
                    {
                        /* Remove the items for the specific RVC instance from the DDMW container */
                        for (unsigned int i = 0; i < rvc_class->ddm_class_desc_size; i++)
                        {
                            ddmw_remove(&ddm_container, &rvc_class->params_items[i].ddm2_item);
                        }

                        /* Remove the specific RVC class descriptor node */
                        ddm_class_desc_delete(rvc_class);

                        SORTED_LIST_KEY_TYPE mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                        /* Get all the RVC class instances that are linked to the specific MBAT instance */
                        if (sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) != SORTED_LIST_OK)
                        {
                            LOG(E, "Failed to get linked RVC classes for MBAT instance %d", mbat_ddm_instance);
                        }
                        else
                        {
                            /* If there is more than one RVC class instance linked to the MBAT class, proceed with updating the sorted list without the unavailable one*/
                            if (no_of_mbat_linked > 1)
                            {
                                for (int i = 0; i < no_of_mbat_linked; i++)
                                {
                                    if (mbat_linked[i] != DDM2_PARAMETER_CLASS_INSTANCE(parameter))
                                    {
                                        sorted_list_single_add(&rvc_mbat_ddm_inst_mapping_table, mbat_linked[i], mbat_ddm_instance);
                                    }
                                }
                                memset(mbat_linked, 0, sizeof(mbat_linked));
                                no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                                /* Get all the RVC class instances that are linked to the specific MBAT instance (without the unavailable one)*/
                                if (sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                                {
                                    /* Update the MBAT linked list DDM parameter*/
                                    ddmw_set_data(&mbat_ddm_inst->params_items[mbat_linked_idx].ddm2_item, mbat_linked, no_of_mbat_linked * sizeof(mbat_linked[0]));
                                }
                                else
                                {
                                    LOG(E, "Failed to get updated linked RVC classes for MBAT instance %d", mbat_ddm_instance);
                                }
                            }
                            else
                            {
                                /* If the unavailable RVC instance is the only one that is linked to the MBAT instance
                                remove the items for the MBAT instance from the DDMW container and make that MBAT instance unavailable*/
                                for (unsigned int i = 0; i < mbat_ddm_inst->ddm_class_desc_size; i++)
                                {
                                    ddmw_remove(&ddm_container, &mbat_ddm_inst->params_items[i].ddm2_item);
                                }

                                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MBAT0 | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance), &Zero, sizeof(Zero), connector_batt_mgmt_service.connector_id, portMAX_DELAY);

                                /* Remove the specific MBAT class descriptor node from the specific MBAT list */
                                ddm_class_desc_delete(mbat_ddm_inst);

                                SORTED_LIST_KEY_TYPE dc_inst_mapped;
                                int no_of_dc_inst_mapped = 1;

                                /* Get the specific DC instance that is linked to the MBAT instance and remove it from the sorted list*/
                                if (sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) != SORTED_LIST_OK)
                                {
                                    LOG(E, "Failed to get DC instance for MBAT instance %d", mbat_ddm_instance);
                                }
                                else
                                {
                                    /* Get the specific product instance linked to the MBAT instance from the sorted list */
                                    SORTED_LIST_VALUE_TYPE prod_inst_mapped;
                                    if (sorted_list_unique_get(&prod_inst_mapped, &mbat_prod_inst_mapping_table, mbat_ddm_instance, 0) != SORTED_LIST_OK)
                                    {
                                        LOG(E, "Failed to get PROD instance for MBAT instance %d", mbat_ddm_instance);
                                    }
                                    else
                                    {
                                        /* Remove the MBAT instance from PROD<X>CLIST */
                                        ddm_class_desc_t *prod_inst = NULL;
                                        prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_inst_mapped);
                                        if (prod_inst != NULL)
                                        {
                                            UPDLINKEDCLASS_T *remove_mbat = hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                                            if (remove_mbat != NULL)
                                            {
                                                remove_mbat->update[0] = 0;
                                                remove_mbat->updclass = MBAT0 | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance);
                                                ddmw_set_data(&prod_inst->params_items[PROD_CLASS_DESC_CLIST_PARAM_INDEX].ddm2_item, remove_mbat, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t));
                                                hal_mem_free(remove_mbat);
                                            }
                                        }
                                        /* Remove MBAT-PROD DDM instance mapping from mapping table */
                                        if (sorted_list_unique_remove(&mbat_prod_inst_mapping_table, mbat_ddm_instance) != SORTED_LIST_OK)
                                        {
                                            LOG(W, "Failed to remove MBAT-PROD mapping for instance %d", mbat_ddm_instance);
                                        }

                                        /* Remove the battery mappings related to the specific DC (MBAT) instance */
                                        remove_batt_mapping((uint8_t)dc_inst_mapped);

                                        LOG(D, "MBAT instance %d is no longer available. Removing it from the prod class %d.", mbat_ddm_instance, prod_inst_mapped);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
        default:
            break;
        }
    }
}

static void manage_mbat_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance, gp_device_model_t cached_model)
{
    uint32_t value;
    /* If there is no MBAT DDM instance related to a specific DC instance */
    if (sorted_list_unique_get(&value, &dc_inst_mbat_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *mbat_ddm_inst;

        /* Create a new MBAT class descriptor*/
        mbat_ddm_inst = ddm_class_desc_create(MBAT0);
        if (mbat_ddm_inst != NULL)
        {
            /* Register new MBAT instance in the system */
            int32_t mbat_ddm_instance = ddmw_register(&ddm_container, MBAT0);

            if (mbat_ddm_instance != -1)
            {
                LOG(D, "Created MBAT instance %d for DC instance %d", mbat_ddm_instance, dc_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mbat_ddm_inst_mapping_table, class_instance, mbat_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&dc_inst_mbat_ddm_inst_mapping_table, dc_inst, mbat_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Initialize the MBAT class descriptor */
                ddm_class_desc_init(mbat_ddm_instance, MBAT0, mbat_ddm_inst);

                /* Add all the items in the DDMW container and update specific items */
                for (uint32_t i = 0; i < mbat_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &mbat_ddm_inst->params_items[i].ddm2_item, mbat_ddm_inst->params_items[i].ddm2_param, mbat_ddm_instance);

                    if (DDM2_PARAMETER_BASE_INSTANCE(mbat_ddm_inst->params_items[i].ddm2_param) == MBAT0DCINST)
                    {
                        ddmw_set_i32(&mbat_ddm_inst->params_items[i].ddm2_item, dc_inst);
                    }
                    else if (DDM2_PARAMETER_BASE_INSTANCE(mbat_ddm_inst->params_items[i].ddm2_param) == MBAT0LINKED)
                    {
                        mbat_linked_idx = i;
                        SORTED_LIST_KEY_TYPE mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                        /* Find all the RVCDCSRC instances that are linked to the specific MBAT DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK);

                        ddmw_set_data(&mbat_ddm_inst->params_items[i].ddm2_item, mbat_linked, no_of_mbat_linked * sizeof(mbat_linked[0]));
                    }
                }
                /* Insert the MBAT class descriptor in the specific MBAT list */
                ddm_class_desc_insert(mbat_ddm_inst, &l_mbat);
                if (cached_model != GP_SHUNT)
                {
                    /* Request battery summary DGN in order to map the master/slave batteries */
                    RVCMGNT0REQ_T req = {0};
                    req.addr = RVC_GLOBAL_DEST_ADDR;
                    req.dgn = BATTERY_SUMMARY_DGN;
                    connector_send_frame_to_broker(DDMP2_CONTROL_SET, RVCMGNT0REQ, &req, sizeof(req), connector_batt_mgmt_service.connector_id, (TickType_t)portMAX_DELAY);

                    /* Request Generic configuration status DGN in order to update the FWVER number for the master battery */
                    req.dgn = GENERIC_CONFIG_STATUS_DGN;
                    connector_send_frame_to_broker(DDMP2_CONTROL_SET, RVCMGNT0REQ, &req, sizeof(req), connector_batt_mgmt_service.connector_id, (TickType_t)portMAX_DELAY);
                }
                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MBAT instance %d to PROD%uCLIST", mbat_ddm_instance, prod_instance);
                        uint32_t mbat_instance = MBAT0 | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &mbat_instance, sizeof(mbat_instance), connector_batt_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                if (cached_model == GP_SHUNT)
                {
                    /* Add the MBAT class instance to the list of linked instances to MSHUNT */
                    TRUE_CHECK(sorted_list_unique_add(&rvc_mshunt_ddm_inst_mapping_table, (MBAT0 | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance)), mbat_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                    /* Get all previously linked RVC instances */
                    SORTED_LIST_KEY_TYPE mshunt_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mshunt_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(mshunt_linked, &no_of_mshunt_linked, &rvc_mshunt_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        /* Prepare a buffer to hold all previous RVC instances and the mbathist class instance */
                        uint32_t linked_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int idx = 0;
                        /* Copy existing instances */
                        for (int i = 0; i < no_of_mshunt_linked; i++)
                        {
                            linked_instances[idx++] = mshunt_linked[i];
                        }
                        ddm_class_desc_t *mshunt_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mshunt, mbat_ddm_instance);
                        if (mshunt_ddm_inst != NULL)
                        {
                            ddmw_set_data(&mshunt_ddm_inst->params_items[mshunt_linked_idx].ddm2_item, linked_instances, idx * sizeof(linked_instances[0]));
                        }
                    }
                }

                /* Link MBAT DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&mbat_prod_inst_mapping_table, mbat_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Check if there are existing MBATHIST and MDCPROFILE classes for this DC instance */
                SORTED_LIST_VALUE_TYPE mbathist_ddm_instance;
                if (sorted_list_unique_get(&mbathist_ddm_instance, &dc_inst_mbathist_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_OK)
                {
                    /* MBATHIST exists, check if it's already linked */
                    bool mbathist_already_linked = false;
                    SORTED_LIST_KEY_TYPE mbat_linked_check[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mbat_linked_check = MAX_NO_OF_LINKED_RVC_CLASSES;

                    if (sorted_list_get_keys(mbat_linked_check, &no_of_mbat_linked_check, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mbat_linked_check; i++)
                        {
                            if (mbat_linked_check[i] == (MBATHIST0 | DDM2_PARAMETER_INSTANCE(mbathist_ddm_instance)))
                            {
                                mbathist_already_linked = true;
                                break;
                            }
                        }
                    }

                    if (!mbathist_already_linked)
                    {
                        LOG(D, "Linking existing MBATHIST instance %d to MBAT instance %d", mbathist_ddm_instance, mbat_ddm_instance);
                        TRUE_CHECK(sorted_list_unique_add(&rvc_mbat_ddm_inst_mapping_table, (MBATHIST0 | DDM2_PARAMETER_INSTANCE(mbathist_ddm_instance)), mbat_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                        /* Update MBAT linked list parameter */
                        SORTED_LIST_KEY_TYPE updated_mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int updated_no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        if (sorted_list_get_keys(updated_mbat_linked, &updated_no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                        {
                            ddmw_set_data(&mbat_ddm_inst->params_items[mbat_linked_idx].ddm2_item, updated_mbat_linked, updated_no_of_mbat_linked * sizeof(updated_mbat_linked[0]));
                        }
                    }
                }

                SORTED_LIST_VALUE_TYPE mdcprofile_ddm_instance;
                if (sorted_list_unique_get(&mdcprofile_ddm_instance, &dc_inst_mdcprofile_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_OK)
                {
                    /* MDCPROFILE exists, check if it's already linked */
                    bool mdcprofile_already_linked = false;
                    SORTED_LIST_KEY_TYPE mbat_linked_check[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mbat_linked_check = MAX_NO_OF_LINKED_RVC_CLASSES;

                    if (sorted_list_get_keys(mbat_linked_check, &no_of_mbat_linked_check, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mbat_linked_check; i++)
                        {
                            if (mbat_linked_check[i] == (MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_instance)))
                            {
                                mdcprofile_already_linked = true;
                                break;
                            }
                        }
                    }

                    if (!mdcprofile_already_linked)
                    {
                        LOG(D, "Linking existing MDCPROFILE instance %d to MBAT instance %d", mdcprofile_ddm_instance, mbat_ddm_instance);
                        TRUE_CHECK(sorted_list_unique_add(&rvc_mbat_ddm_inst_mapping_table, (MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_instance)), mbat_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                        /* Update MBAT linked list parameter */
                        SORTED_LIST_KEY_TYPE updated_mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int updated_no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        if (sorted_list_get_keys(updated_mbat_linked, &updated_no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                        {
                            ddmw_set_data(&mbat_ddm_inst->params_items[mbat_linked_idx].ddm2_item, updated_mbat_linked, updated_no_of_mbat_linked * sizeof(updated_mbat_linked[0]));
                        }
                    }
                }
            }
            else
            {
                ddm_class_desc_delete(mbat_ddm_inst);
            }
        }
        else
        {
            LOG(E, "MBAT0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MBAT DDM instance related to a specific DC instance
            check whether the RVCDCSRC<X> is linked to the MBAT DDM instance and if not, link it
            (update the corresponding MBAT parameter) */

        ddm_class_desc_t *mbat_ddm_inst;
        SORTED_LIST_KEY_TYPE mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

        for (int i = 0; i < no_of_mbat_linked; i++)
        {
            if (mbat_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            mbat_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)value);
            if (mbat_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MBAT instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_mbat_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);

                no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                TRUE_CHECK(sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
                ddmw_set_data(&mbat_ddm_inst->params_items[mbat_linked_idx].ddm2_item, mbat_linked, no_of_mbat_linked * sizeof(mbat_linked[0]));
            }
        }
    }
}

static void manage_mshunt_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance)
{
    uint32_t value;

    /* If there is no MSHUNT DDM instance related to a specific DC instance */
    if (sorted_list_unique_get(&value, &dc_inst_mshunt_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *mshunt_ddm_inst;

        /* Create a new MSHUNT class descriptor*/
        mshunt_ddm_inst = ddm_class_desc_create(MSHUNT0);
        if (mshunt_ddm_inst != NULL)
        {
            /* Register new MSHUNT instance in the system */
            int32_t mshunt_ddm_instance = ddmw_register(&ddm_container, MSHUNT0);

            if (mshunt_ddm_instance != -1)
            {
                PROD0PROP_T prodprop;
                size_t prop_size = sizeof(PROD0PROP_T);
                ProdDBReadCache(FIELD_PROP, prod_instance, &prodprop, &prop_size);
                shunt.avl = true;
                shunt.dc_instance = dc_inst;
                shunt.instance = mshunt_ddm_instance;
                shunt.prod_instance = prod_instance;
                shunt.sa = prodprop.addr;
                LOG(D, "Created MSHUNT instance %d for DC instance %d", mshunt_ddm_instance, dc_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mshunt_ddm_inst_mapping_table, class_instance, mshunt_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&dc_inst_mshunt_ddm_inst_mapping_table, dc_inst, mshunt_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                /* Initialize the MSHUNT class descriptor */
                ddm_class_desc_init(mshunt_ddm_instance, MSHUNT0, mshunt_ddm_inst);

                /* Add all the items except AVL in the DDMW container and update specific items */
                for (uint32_t i = 1; i < mshunt_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &mshunt_ddm_inst->params_items[i].ddm2_item, mshunt_ddm_inst->params_items[i].ddm2_param, mshunt_ddm_instance);

                    if (DDM2_PARAMETER_BASE_INSTANCE(mshunt_ddm_inst->params_items[i].ddm2_param) == MSHUNT0LINKED)
                    {
                        mshunt_linked_idx = i;
                        SORTED_LIST_KEY_TYPE mshunt_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_mshunt_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                        /* Find all the RVCDCSRC instances that are linked to the specific MSHUNT DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(mshunt_linked, &no_of_mshunt_linked, &rvc_mshunt_ddm_inst_mapping_table, mshunt_ddm_instance, 0) == SORTED_LIST_OK);
                        ddmw_set_data(&mshunt_ddm_inst->params_items[i].ddm2_item, mshunt_linked, no_of_mshunt_linked * sizeof(mshunt_linked[0]));
                    }
                }
                /* Insert the MSHUNT class descriptor in the specific MSHUNT list */
                ddm_class_desc_insert(mshunt_ddm_inst, &l_mshunt);

                RVCMGNT0REQ_T req = {0};
                req.addr = RVC_GLOBAL_DEST_ADDR;
                req.dgn = DGN_DC_SOURCE_CONFIGURATION_STATUS_1;
                connector_send_frame_to_broker(DDMP2_CONTROL_SET, RVCMGNT0REQ, &req, sizeof(req), connector_batt_mgmt_service.connector_id, (TickType_t)portMAX_DELAY);
                req.addr = RVC_GLOBAL_DEST_ADDR;
                req.dgn = DGN_DC_SOURCE_CONFIGURATION_STATUS_2;
                connector_send_frame_to_broker(DDMP2_CONTROL_SET, RVCMGNT0REQ, &req, sizeof(req), connector_batt_mgmt_service.connector_id, (TickType_t)portMAX_DELAY);

                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MSHUNT instance %d to PROD%uCLIST", mshunt_ddm_instance, prod_instance);
                        uint32_t mshunt_instance = MSHUNT0 | DDM2_PARAMETER_INSTANCE(mshunt_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &mshunt_instance, sizeof(mshunt_instance), connector_batt_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                /* Link MSHUNT DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&mshunt_prod_inst_mapping_table, mshunt_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Check if there is an existing MBAT instance for this DC instance */
                SORTED_LIST_VALUE_TYPE mbat_ddm_instance;
                if (sorted_list_unique_get(&mbat_ddm_instance, &dc_inst_mbat_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_OK)
                {
                    /* Check if MBAT instance is already in the mshunt linked classes */
                    SORTED_LIST_KEY_TYPE mshunt_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mshunt_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                    bool mbat_already_linked = false;

                    if (sorted_list_get_keys(mshunt_linked, &no_of_mshunt_linked, &rvc_mshunt_ddm_inst_mapping_table, mshunt_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mshunt_linked; i++)
                        {
                            if (mshunt_linked[i] == (MBAT0 | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance)))
                            {
                                mbat_already_linked = true;
                                break;
                            }
                        }
                    }

                    if (!mbat_already_linked)
                    {
                        LOG(D, "Linking existing MBAT instance %d to MSHUNT instance %d", mbat_ddm_instance, mshunt_ddm_instance);

                        /* Add the MBAT class instance to the list of linked instances to MSHUNT */
                        TRUE_CHECK(sorted_list_unique_add(&rvc_mshunt_ddm_inst_mapping_table, (MBAT0 | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance)), mshunt_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                        /* Update MSHUNT linked list parameter */
                        memset(mshunt_linked, 0, sizeof(mshunt_linked));
                        no_of_mshunt_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        if (sorted_list_get_keys(mshunt_linked, &no_of_mshunt_linked, &rvc_mshunt_ddm_inst_mapping_table, mshunt_ddm_instance, 0) == SORTED_LIST_OK)
                        {
                            ddmw_set_data(&mshunt_ddm_inst->params_items[mshunt_linked_idx].ddm2_item, mshunt_linked, no_of_mshunt_linked * sizeof(mshunt_linked[0]));
                        }
                    }
                }
                else
                {
                    ddm_class_desc_delete(mshunt_ddm_inst);
                }
                int32_t type = MSHUNT0TYPE_BATTERY;
                ddmw_item_t *mshunt_item = ddmw_find_item(&ddm_container, MSHUNT0TYPE | DDM2_PARAMETER_INSTANCE(mshunt_ddm_instance));
                ddmw_set_data(mshunt_item, &type, sizeof(MSHUNT0TYPE_BATTERY));
            }
        }
        else
        {
            LOG(E, "MSHUNT0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MSHUNT DDM instance related to a specific DC instance
            check whether the RVCDCSRC<X> is linked to the MSHUNT DDM instance and if not, link it */
        ddm_class_desc_t *mshunt_ddm_inst;
        SORTED_LIST_KEY_TYPE mshunt_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mshunt_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(mshunt_linked, &no_of_mshunt_linked, &rvc_mshunt_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

        for (int i = 0; i < no_of_mshunt_linked; i++)
        {
            if (mshunt_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            mshunt_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mshunt, (uint8_t)value);
            if (mshunt_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MSHUNT instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_mshunt_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);
                no_of_mshunt_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                TRUE_CHECK(sorted_list_get_keys(mshunt_linked, &no_of_mshunt_linked, &rvc_mshunt_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

                ddmw_set_data(&mshunt_ddm_inst->params_items[mshunt_linked_idx].ddm2_item, mshunt_linked, no_of_mshunt_linked * sizeof(mshunt_linked[0]));
            }
        }
    }
}

static void manage_mbathist_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance, int32_t mbat_ddm_instance)
{
    uint32_t value;

    /* If there is no MBATHIST DDM instance related to a specific DC instance */
    if (sorted_list_unique_get(&value, &dc_inst_mbathist_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *mbathist_ddm_inst;

        /* Create a new MBATHIST class descriptor*/
        mbathist_ddm_inst = ddm_class_desc_create(MBATHIST0);
        if (mbathist_ddm_inst != NULL)
        {
            /* Register new MBATHIST instance in the system */
            int32_t mbathist_ddm_instance = ddmw_register(&ddm_container, MBATHIST0);

            if (mbathist_ddm_instance != -1)
            {
                LOG(D, "Created MBATHIST instance %d for DC instance %d", mbathist_ddm_instance, dc_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mbathist_ddm_inst_mapping_table, class_instance, mbathist_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&dc_inst_mbathist_ddm_inst_mapping_table, dc_inst, mbathist_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                /* Initialize the MBATHIST class descriptor */
                ddm_class_desc_init(mbathist_ddm_instance, MBATHIST0, mbathist_ddm_inst);

                /* Add all the items except AVL in the DDMW container and update specific items */
                for (uint32_t i = 1; i < mbathist_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &mbathist_ddm_inst->params_items[i].ddm2_item, mbathist_ddm_inst->params_items[i].ddm2_param, mbathist_ddm_instance);

                    if (DDM2_PARAMETER_BASE_INSTANCE(mbathist_ddm_inst->params_items[i].ddm2_param) == MBATHIST0DCINST)
                    {
                        ddmw_set_i32(&mbathist_ddm_inst->params_items[i].ddm2_item, dc_inst);
                    }
                    if (DDM2_PARAMETER_BASE_INSTANCE(mbathist_ddm_inst->params_items[i].ddm2_param) == MBATHIST0INST)
                    {
                        /* Find the data from MBAT<mbat_ddm_instance>INST */
                        ddmw_item_t *mbat0inst_item = ddmw_find_item(&ddm_container, MBAT0INST | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance));
                        if (mbat0inst_item && (mbat0inst_item->size == sizeof(int32_t)))
                        {
                            /* Copy data */
                            ddmw_set_i32(&mbathist_ddm_inst->params_items[i].ddm2_item, ddmw_get_i32(mbat0inst_item));
                        }
                    }
                    if (DDM2_PARAMETER_BASE_INSTANCE(mbathist_ddm_inst->params_items[i].ddm2_param) == MBATHIST0LINKED)
                    {
                        mbathist_linked_idx = i;
                        SORTED_LIST_KEY_TYPE mbathist_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_mbathist_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                        /* Find all the RVCDCSRC instances that are linked to the specific MBATHIST DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(mbathist_linked, &no_of_mbathist_linked, &rvc_mbathist_ddm_inst_mapping_table, mbathist_ddm_instance, 0) == SORTED_LIST_OK);
                        ddmw_set_data(&mbathist_ddm_inst->params_items[i].ddm2_item, mbathist_linked, no_of_mbathist_linked * sizeof(mbathist_linked[0]));
                    }
                }
                /* Insert the MBATHIST class descriptor in the specific MBATHIST list */
                ddm_class_desc_insert(mbathist_ddm_inst, &l_mbathist);

                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MBATHIST instance %d to PROD%uCLIST", mbathist_ddm_instance, prod_instance);
                        uint32_t mbathist_instance = MBATHIST0 | DDM2_PARAMETER_INSTANCE(mbathist_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &mbathist_instance, sizeof(mbathist_instance), connector_batt_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                /* Add the MBATHIST class instance to the list of linked instances to MBAT */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mbat_ddm_inst_mapping_table, (MBATHIST0 | DDM2_PARAMETER_INSTANCE(mbathist_ddm_instance)), mbat_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Get all previously linked RVC instances */
                SORTED_LIST_KEY_TYPE mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                int no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                if (sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                {
                    /* Prepare a buffer to hold all previous RVC instances and the mbathist class instance */
                    uint32_t linked_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int idx = 0;
                    /* Copy existing instances */
                    for (int i = 0; i < no_of_mbat_linked; i++)
                    {
                        linked_instances[idx++] = mbat_linked[i];
                    }
                    ddm_class_desc_t *mbat_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbat, mbat_ddm_instance);
                    if (mbat_ddm_inst != NULL)
                    {
                        ddmw_set_data(&mbat_ddm_inst->params_items[mbat_linked_idx].ddm2_item, linked_instances, idx * sizeof(linked_instances[0]));
                    }
                }

                /* Link MBATHIST DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&mbathist_prod_inst_mapping_table, mbathist_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);
            }
            else
            {
                ddm_class_desc_delete(mbathist_ddm_inst);
            }
        }
        else
        {
            LOG(E, "MBATHIST0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MBATHIST DDM instance related to a specific DC instance
            check whether the RVCDCSRC<X> is linked to the MBATHIST DDM instance and if not, link it */
        ddm_class_desc_t *mbathist_ddm_inst;
        SORTED_LIST_KEY_TYPE mbathist_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mbathist_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(mbathist_linked, &no_of_mbathist_linked, &rvc_mbathist_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

        for (int i = 0; i < no_of_mbathist_linked; i++)
        {
            if (mbathist_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            mbathist_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbathist, (uint8_t)value);
            if (mbathist_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MBATHIST instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_mbathist_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);
                no_of_mbathist_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                TRUE_CHECK(sorted_list_get_keys(mbathist_linked, &no_of_mbathist_linked, &rvc_mbathist_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
                ddmw_set_data(&mbathist_ddm_inst->params_items[mbathist_linked_idx].ddm2_item, mbathist_linked, no_of_mbathist_linked * sizeof(mbathist_linked[0]));
            }
        }
    }
}

static void manage_mdcprofile_classes(uint32_t class_instance, uint8_t dc_inst, uint8_t prod_instance, int32_t mbat_ddm_instance)
{
    uint32_t value;

    /* If there is no DCPROFILE DDM instance related to a specific DC instance */
    if (sorted_list_unique_get(&value, &dc_inst_mdcprofile_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *mdcprofile_ddm_inst;
        /* Create a new MDCPROFILE class descriptor*/
        mdcprofile_ddm_inst = ddm_class_desc_create(MDCPROFILE0);
        if (mdcprofile_ddm_inst != NULL)
        {
            /* Register new MDCPROFILE instance in the system */
            int32_t mdcprofile_ddm_instance = ddmw_register(&ddm_container, MDCPROFILE0);

            if (mdcprofile_ddm_instance != -1)
            {
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mdcprofile_ddm_inst_mapping_table, class_instance, mdcprofile_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&dc_inst_mdcprofile_ddm_inst_mapping_table, dc_inst, mdcprofile_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                /* Initialize the MDCPROFILE class descriptor */
                ddm_class_desc_init(mdcprofile_ddm_instance, MDCPROFILE0, mdcprofile_ddm_inst);

                /* Add all the items except AVL in the DDMW container and update specific items */
                for (uint32_t i = 1; i < mdcprofile_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &mdcprofile_ddm_inst->params_items[i].ddm2_item, mdcprofile_ddm_inst->params_items[i].ddm2_param, mdcprofile_ddm_instance);
                    if (DDM2_PARAMETER_BASE_INSTANCE(mdcprofile_ddm_inst->params_items[i].ddm2_param) == MDCPROFILE0LINKED)
                    {
                        mdcprofile_linked_idx = i;
                        SORTED_LIST_KEY_TYPE mdcprofile_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_mdcprofile_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        /* Find all the RVCDCSRC instances that are linked to the specific MDCPROFILE DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(mdcprofile_linked, &no_of_mdcprofile_linked, &rvc_mdcprofile_ddm_inst_mapping_table, mdcprofile_ddm_instance, 0) == SORTED_LIST_OK);
                        ddmw_set_data(&mdcprofile_ddm_inst->params_items[i].ddm2_item, mdcprofile_linked, no_of_mdcprofile_linked * sizeof(mdcprofile_linked[0]));
                    }
                }
                /* Insert the MDCPROFILE class descriptor in the specific MDCPROFILE list */
                ddm_class_desc_insert(mdcprofile_ddm_inst, &l_mdcprofile);

                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MDCPROFILE instance %d to PROD%uCLIST", mdcprofile_ddm_instance, prod_instance);
                        ddmw_set_u32(prod_clist, MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_instance));
                    }
                }

                /* Add the MDCPROFILE class instance to the list of linked instances to MBAT */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mbat_ddm_inst_mapping_table, (MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_instance)), mbat_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Get all previously linked RVC instances */
                SORTED_LIST_KEY_TYPE mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                int no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                if (sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
                {
                    /* Prepare a buffer to hold all previous RVC instances and the mdcprofile class instance */
                    uint32_t linked_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int idx = 0;
                    /* Copy existing instances */
                    for (int i = 0; i < no_of_mbat_linked; i++)
                    {
                        linked_instances[idx++] = mbat_linked[i];
                    }
                    ddm_class_desc_t *mbat_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbat, mbat_ddm_instance);
                    if (mbat_ddm_inst != NULL)
                    {
                        ddmw_set_data(&mbat_ddm_inst->params_items[mbat_linked_idx].ddm2_item, linked_instances, idx * sizeof(linked_instances[0]));
                    }
                }

                /* Link MDCPROFILE DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&mdcprofile_prod_inst_mapping_table, mdcprofile_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);
            }
            else
            {
                ddm_class_desc_delete(mdcprofile_ddm_inst);
            }
        }
        else
        {
            LOG(E, "MDCPROFILE0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MDCPROFILE DDM instance related to a specific DC instance
            check whether the RVCDCSRC<X> is linked to the MDCPROFILE DDM instance and if not, link it */
        ddm_class_desc_t *mdcprofile_ddm_inst;
        SORTED_LIST_KEY_TYPE mdcprofile_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mdcprofile_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(mdcprofile_linked, &no_of_mdcprofile_linked, &rvc_mdcprofile_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

        for (int i = 0; i < no_of_mdcprofile_linked; i++)
        {
            if (mdcprofile_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            mdcprofile_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mdcprofile, (uint8_t)value);
            if (mdcprofile_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MDCPROFILE instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_mdcprofile_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);

                no_of_mdcprofile_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                TRUE_CHECK(sorted_list_get_keys(mdcprofile_linked, &no_of_mdcprofile_linked, &rvc_mdcprofile_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
                ddmw_set_data(&mdcprofile_ddm_inst->params_items[mdcprofile_linked_idx].ddm2_item, mdcprofile_linked, no_of_mdcprofile_linked * sizeof(mdcprofile_linked[0]));
            }
        }
    }
}

static void subscribe_to_prop_classes(uint32_t parameter)
{
    switch (DDM2_PARAMETER_CLASS(parameter))
    {
    case RVCDCSRC0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrc, parameter);
    }
    break;
    case RVCDCSRCTWO0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrctwo, parameter);
    }
    break;

    case RVCDCSRCTHR0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcthr, parameter);
    }
    break;

    case RVCDCSRCFOUR0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcfour, parameter);
    }
    break;

    case RVCDCSRCFIVE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcfive, parameter);
    }
    break;

    case RVCDCSRCSIX0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcsix, parameter);
    }
    break;

    case RVCDCSRCSEV0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcsev, parameter);
    }
    break;

    case RVCDCSRCEIG0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrceig, parameter);
    }
    break;

    case RVCDCSRCNINE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcnine, parameter);
    }
    break;

    case RVCDCSRCTEN0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcten, parameter);
    }
    break;

    case RVCDCSRCELE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcele, parameter);
    }
    break;

    case RVCDCSRCTWE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrctwe, parameter);
    }
    break;

    case RVCDCSRCTHI0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrcthir, parameter);
    }
    break;

    case RVCDCSRCCFGTWO0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrccfgtwo, parameter);
    }
    break;
    case RVCDCSRCCFG0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcsrccfg, parameter);
    }
    break;
    case RVCDCDISCONN0:
    {
        manage_subscriptions_for_prop_class(&l_rvcdcdisconn, parameter);
    }
    break;
    default:
        break;
    }
}

/**
 * @brief Remove MBATHIST instance and cleanup all its mappings
 * @param mbathist_instance MBATHIST DDM instance to remove
 * @param prod_instance PROD instance that owns this MBATHIST
 */
static void remove_mbathist_instance(uint8_t mbathist_instance, uint8_t prod_instance)
{
    LOG(D, "Removing MBATHIST%d from PROD%d", mbathist_instance, prod_instance);

    ddm_class_desc_t *mbathist_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbathist, mbathist_instance);
    if (mbathist_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < mbathist_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &mbathist_ddm_inst->params_items[i].ddm2_item);
        }

        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MBATHIST0 | DDM2_PARAMETER_INSTANCE(mbathist_instance),
                                       &Zero, sizeof(Zero),
                                       connector_batt_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MBATHIST mappings */
        SORTED_LIST_KEY_TYPE mbathist_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mbathist_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(mbathist_linked, &no_of_mbathist_linked, &rvc_mbathist_ddm_inst_mapping_table, mbathist_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_mbathist_linked; i++)
            {
                sorted_list_unique_remove(&rvc_mbathist_ddm_inst_mapping_table, mbathist_linked[i]);
                LOG(D, "Removed RVC class 0x%08X from MBATHIST%d mapping", mbathist_linked[i], mbathist_instance);
            }
        }
        /* Remove all RVC class instances linked to MBATHIST from their descriptors and DDMW container */
        for (int i = 0; i < no_of_mbathist_linked; i++)
        {
            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(mbathist_linked[i]);

            switch (DDM2_PARAMETER_CLASS(mbathist_linked[i]))
            {
            case RVCDCSRCSEV0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcsev, rvc_instance);
                break;
            case RVCDCSRCEIG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrceig, rvc_instance);
                break;
            case RVCDCSRCNINE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcnine, rvc_instance);
                break;
            case RVCDCSRCTEN0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcten, rvc_instance);
                break;
            case RVCDCSRCTWE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrctwe, rvc_instance);
                break;
            case RVCDCSRCTHI0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcthir, rvc_instance);
                break;
            default:
                break;
            }

            if (rvc_class != NULL)
            {
                /* Remove all DDMW items for this RVC class */
                for (unsigned int j = 0; j < rvc_class->ddm_class_desc_size; j++)
                {
                    ddmw_remove(&ddm_container, &rvc_class->params_items[j].ddm2_item);
                }

                /* Delete the RVC class descriptor */
                ddm_class_desc_delete(rvc_class);
                LOG(D, "Removed RVC class 0x%08X instance %d and its DDMW items", DDM2_PARAMETER_CLASS(mbathist_linked[i]), rvc_instance);
            }
        }

        /* Remove DC->MBATHIST mapping */
        SORTED_LIST_KEY_TYPE dc_inst;
        int no_of_dc = 1;
        if (sorted_list_get_keys(&dc_inst, &no_of_dc, &dc_inst_mbathist_ddm_inst_mapping_table, mbathist_instance, 0) == SORTED_LIST_OK)
        {
            sorted_list_unique_remove(&dc_inst_mbathist_ddm_inst_mapping_table, dc_inst);
            LOG(D, "Removed DC instance %d -> MBATHIST%d mapping", dc_inst, mbathist_instance);
        }

        /* Remove MBATHIST->PROD mapping */
        sorted_list_unique_remove(&mbathist_prod_inst_mapping_table, mbathist_instance);

        /* Delete descriptor */
        ddm_class_desc_delete(mbathist_ddm_inst);
        LOG(D, "MBATHIST%d successfully removed", mbathist_instance);
    }
}

/**
 * @brief Remove MDCPROFILE instance and cleanup all its mappings
 * @param mdcprofile_instance MDCPROFILE DDM instance to remove
 * @param prod_instance PROD instance that owns this MDCPROFILE
 */
static void remove_mdcprofile_instance(uint8_t mdcprofile_instance, uint8_t prod_instance)
{
    LOG(D, "Removing MDCPROFILE%d from PROD%d", mdcprofile_instance, prod_instance);

    ddm_class_desc_t *mdcprofile_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mdcprofile, mdcprofile_instance);
    if (mdcprofile_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < mdcprofile_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &mdcprofile_ddm_inst->params_items[i].ddm2_item);
        }

        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(mdcprofile_instance),
                                       &Zero, sizeof(Zero),
                                       connector_batt_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MDCPROFILE mappings */
        SORTED_LIST_KEY_TYPE mdcprofile_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mdcprofile_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(mdcprofile_linked, &no_of_mdcprofile_linked, &rvc_mdcprofile_ddm_inst_mapping_table, mdcprofile_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_mdcprofile_linked; i++)
            {
                sorted_list_unique_remove(&rvc_mdcprofile_ddm_inst_mapping_table, mdcprofile_linked[i]);
                LOG(D, "Removed RVC class 0x%08X from MDCPROFILE%d mapping", mdcprofile_linked[i], mdcprofile_instance);
            }
        }

        /* Remove all RVC class instances linked to MDCPROFILE from their descriptors and DDMW container */
        for (int i = 0; i < no_of_mdcprofile_linked; i++)
        {
            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(mdcprofile_linked[i]);

            switch (DDM2_PARAMETER_CLASS(mdcprofile_linked[i]))
            {
            case RVCDCSRCCFG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrccfg, rvc_instance);
                break;

            case RVCDCSRCCFGTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrccfgtwo, rvc_instance);
                break;

            default:
                break;
            }

            if (rvc_class != NULL)
            {
                /* Remove all DDMW items for this RVC class */
                for (unsigned int j = 0; j < rvc_class->ddm_class_desc_size; j++)
                {
                    ddmw_remove(&ddm_container, &rvc_class->params_items[j].ddm2_item);
                }

                /* Delete the RVC class descriptor */
                ddm_class_desc_delete(rvc_class);
                LOG(D, "Removed RVC class 0x%08X instance %d and its DDMW items", DDM2_PARAMETER_CLASS(mdcprofile_linked[i]), rvc_instance);
            }
        }

        /* Remove DC->MDCPROFILE mapping */
        SORTED_LIST_KEY_TYPE dc_inst;
        int no_of_dc = 1;
        if (sorted_list_get_keys(&dc_inst, &no_of_dc, &dc_inst_mdcprofile_ddm_inst_mapping_table, mdcprofile_instance, 0) == SORTED_LIST_OK)
        {
            sorted_list_unique_remove(&dc_inst_mdcprofile_ddm_inst_mapping_table, dc_inst);
            LOG(D, "Removed DC instance %d -> MDCPROFILE%d mapping", dc_inst, mdcprofile_instance);
        }

        /* Remove MDCPROFILE->PROD mapping */
        sorted_list_unique_remove(&mdcprofile_prod_inst_mapping_table, mdcprofile_instance);

        /* Delete descriptor */
        ddm_class_desc_delete(mdcprofile_ddm_inst);
        LOG(D, "MDCPROFILE%d successfully removed", mdcprofile_instance);
    }
}

/**
 * @brief Remove MSHUNT instance and cleanup all its mappings
 * @param mshunt_instance MSHUNT DDM instance to remove
 * @param prod_instance PROD instance that owns this MSHUNT
 */
static void remove_mshunt_instance(uint8_t mshunt_instance, uint8_t prod_instance)
{
    LOG(D, "Removing MSHUNT%d from PROD%d", mshunt_instance, prod_instance);

    ddm_class_desc_t *mshunt_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mshunt, mshunt_instance);
    if (mshunt_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < mshunt_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &mshunt_ddm_inst->params_items[i].ddm2_item);
        }

        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MSHUNT0 | DDM2_PARAMETER_INSTANCE(mshunt_instance),
                                       &Zero, sizeof(Zero),
                                       connector_batt_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MSHUNT mappings */
        SORTED_LIST_KEY_TYPE mshunt_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mshunt_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(mshunt_linked, &no_of_mshunt_linked, &rvc_mshunt_ddm_inst_mapping_table, mshunt_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_mshunt_linked; i++)
            {
                sorted_list_unique_remove(&rvc_mshunt_ddm_inst_mapping_table, mshunt_linked[i]);
                LOG(D, "Removed RVC class 0x%08X from MSHUNT%d mapping", mshunt_linked[i], mshunt_instance);
            }
        }

        /* Remove all RVC class instances linked to MSHUNT from their descriptors and DDMW container */
        for (int i = 0; i < no_of_mshunt_linked; i++)
        {
            /* Skip MBAT class instances as they are handled separately */
            if (DDM2_PARAMETER_CLASS(mshunt_linked[i]) == MBAT0)
            {
                continue;
            }

            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(mshunt_linked[i]);

            switch (DDM2_PARAMETER_CLASS(mshunt_linked[i]))
            {
            case RVCDCSRCCFGTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrccfgtwo, rvc_instance);
                break;
            case RVCDCDISCONN0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcdisconn, rvc_instance);
                break;
            default:
                break;
            }

            if (rvc_class != NULL)
            {
                /* Remove all DDMW items for this RVC class */
                for (unsigned int j = 0; j < rvc_class->ddm_class_desc_size; j++)
                {
                    ddmw_remove(&ddm_container, &rvc_class->params_items[j].ddm2_item);
                }

                /* Delete the RVC class descriptor */
                ddm_class_desc_delete(rvc_class);
                LOG(D, "Removed RVC class 0x%08X instance %d and its DDMW items", DDM2_PARAMETER_CLASS(mshunt_linked[i]), rvc_instance);
            }
        }

        /* Remove DC->MSHUNT mapping */
        SORTED_LIST_KEY_TYPE dc_inst;
        int no_of_dc = 1;
        if (sorted_list_get_keys(&dc_inst, &no_of_dc, &dc_inst_mshunt_ddm_inst_mapping_table, mshunt_instance, 0) == SORTED_LIST_OK)
        {
            sorted_list_unique_remove(&dc_inst_mshunt_ddm_inst_mapping_table, dc_inst);
            LOG(D, "Removed DC instance %d -> MSHUNT%d mapping", dc_inst, mshunt_instance);
        }

        /* Remove MSHUNT->PROD mapping */
        sorted_list_unique_remove(&mshunt_prod_inst_mapping_table, mshunt_instance);

        /* Clear global shunt structure if this matches */
        if (shunt.instance == mshunt_instance)
        {
            shunt.avl = false;
            shunt.instance = 0;
            shunt.dc_instance = 0;
            shunt.prod_instance = 0;
            shunt.sa = 0;
        }

        /* Delete descriptor */
        ddm_class_desc_delete(mshunt_ddm_inst);
        LOG(D, "MSHUNT%d successfully removed", mshunt_instance);
    }
}

/**
 * @brief Remove MBAT instance and cleanup all its mappings
 * @param mbat_instance MBAT DDM instance to remove
 * @param prod_instance PROD instance that owns this MBAT
 */
static void remove_mbat_instance(uint8_t mbat_instance, uint8_t prod_instance)
{
    LOG(D, "Removing MBAT%d from PROD%d", mbat_instance, prod_instance);

    ddm_class_desc_t *mbat_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbat, mbat_instance);
    if (mbat_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < mbat_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &mbat_ddm_inst->params_items[i].ddm2_item);
        }
        LOG(D, "MBAT instance %d: All DDMW items removed", mbat_instance);
        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MBAT0 | DDM2_PARAMETER_INSTANCE(mbat_instance),
                                       &Zero, sizeof(Zero),
                                       connector_batt_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MBAT mappings (includes MBATHIST and MDCPROFILE class instances) */
        SORTED_LIST_KEY_TYPE mbat_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mbat_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(mbat_linked, &no_of_mbat_linked, &rvc_mbat_ddm_inst_mapping_table, mbat_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_mbat_linked; i++)
            {
                sorted_list_unique_remove(&rvc_mbat_ddm_inst_mapping_table, mbat_linked[i]);
                LOG(D, "Removed class 0x%08X from MBAT%d mapping", mbat_linked[i], mbat_instance);
            }
        }

        /* Remove all RVC class descriptors and their DDMW items that were linked to this MBAT */
        for (int i = 0; i < no_of_mbat_linked; i++)
        {
            uint32_t linked_class = mbat_linked[i];

            /* Skip MBATHIST and MDCPROFILE as they are handled separately */
            if ((DDM2_PARAMETER_CLASS(linked_class) == MBATHIST0) ||
                (DDM2_PARAMETER_CLASS(linked_class) == MDCPROFILE0))
            {
                continue;
            }

            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(linked_class);

            switch (DDM2_PARAMETER_CLASS(linked_class))
            {
            case RVCDCSRC0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrc, rvc_instance);
                break;
            case RVCDCSRCTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrctwo, rvc_instance);
                break;
            case RVCDCSRCTHR0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcthr, rvc_instance);
                break;
            case RVCDCSRCFOUR0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcfour, rvc_instance);
                break;
            case RVCDCSRCFIVE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcfive, rvc_instance);
                break;
            case RVCDCSRCSIX0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcsix, rvc_instance);
                break;
            case RVCDCSRCELE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcele, rvc_instance);
                break;
            default:
                break;
            }

            if (rvc_class != NULL)
            {
                /* Remove all DDMW items for this RVC class */
                for (unsigned int j = 0; j < rvc_class->ddm_class_desc_size; j++)
                {
                    ddmw_remove(&ddm_container, &rvc_class->params_items[j].ddm2_item);
                }

                /* Delete the RVC class descriptor */
                ddm_class_desc_delete(rvc_class);
                LOG(D, "Removed RVC class 0x%08X instance %d and its DDMW items", DDM2_PARAMETER_CLASS(linked_class), rvc_instance);
            }
        }

        /* Remove DC->MBAT mapping */
        SORTED_LIST_KEY_TYPE dc_inst;
        int no_of_dc = 1;
        if (sorted_list_get_keys(&dc_inst, &no_of_dc, &dc_inst_mbat_ddm_inst_mapping_table, mbat_instance, 0) == SORTED_LIST_OK)
        {
            sorted_list_unique_remove(&dc_inst_mbat_ddm_inst_mapping_table, dc_inst);
            LOG(D, "Removed DC instance %d -> MBAT%d mapping", dc_inst, mbat_instance);

            /* Remove battery mappings for this DC instance */
            remove_batt_mapping((uint8_t)dc_inst);
        }

        /* Remove MBAT->PROD mapping */
        sorted_list_unique_remove(&mbat_prod_inst_mapping_table, mbat_instance);

        /* Delete descriptor */
        ddm_class_desc_delete(mbat_ddm_inst);
        LOG(D, "MBAT%d successfully removed", mbat_instance);
    }
}

static bool is_rvc_class_mbathist_class_related(uint32_t class_instance)
{
    bool is_rvc_mbathist_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCDCSRCSEV0:
    case RVCDCSRCEIG0:
    case RVCDCSRCNINE0:
    case RVCDCSRCTEN0:
    case RVCDCSRCTWE0:
    case RVCDCSRCTHI0:
        is_rvc_mbathist_related = true;
        break;

    default:
        is_rvc_mbathist_related = false;
        break;
    }
    return is_rvc_mbathist_related;
}

static bool is_rvc_class_mshunt_class_related(uint32_t class_instance)
{
    bool is_rvc_mshunt_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCDCSRCCFGTWO0:
    case RVCDCDISCONN0:
        is_rvc_mshunt_related = true;
        break;

    default:
        is_rvc_mshunt_related = false;
        break;
    }
    return is_rvc_mshunt_related;
}

static bool is_rvc_class_mdcprofile_class_related(uint32_t class_instance)
{
    bool is_rvc_mdcprofile_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCDCSRCCFGTWO0:
    case RVCDCSRCCFG0:
        is_rvc_mdcprofile_related = true;
        break;

    default:
        is_rvc_mdcprofile_related = false;
        break;
    }
    return is_rvc_mdcprofile_related;
}

static bool is_rvc_class_mbat_class_related(uint32_t class_instance)
{
    bool is_rvc_mbat_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCDCSRCTHR0:
    case RVCDCSRCELE0:
    case RVCDCSRCTWO0:
    case RVCDCSRC0:
    case RVCDCSRCFOUR0:
    case RVCDCSRCFIVE0:
    case RVCDCSRCSIX0:
        is_rvc_mbat_related = true;
        break;

    default:
        is_rvc_mbat_related = false;
        break;
    }
    return is_rvc_mbat_related;
}

static void manage_subscriptions_for_prop_class(list_t *dgn_list, uint32_t parameter)
{
    ddm_class_desc_t *ddm_class = NULL;
    uint8_t class_instance = DDM2_PARAMETER_INSTANCE_FIELD(parameter);

    ddm_class = ddm_class_desc_find_by_ddm_instance(dgn_list, class_instance);
    if (ddm_class == NULL)
    {
        ddm_class = ddm_class_desc_create(DDM2_PARAMETER_CLASS(parameter));
        if (ddm_class != NULL)
        {
            ddm_class_desc_init(class_instance, DDM2_PARAMETER_CLASS(parameter), ddm_class);
            for (unsigned int i = 0; i < ddm_class->ddm_class_desc_size; i++)
            {
                ddmw_add(&ddm_container, &ddm_class->params_items[i].ddm2_item, ddm_class->params_items[i].ddm2_param, class_instance);
                ddmw_set_type(&ddm_class->params_items[i].ddm2_item, DDMW_ACTION_SET);
                ddmw_subscribe(&ddm_class->params_items[i].ddm2_item);
            }
            ddm_class_desc_insert(ddm_class, dgn_list);
        }
    }
}

static void manage_subscriptions_for_rvc_on_req_class(list_t *dgn_list, uint32_t parameter)
{
    ddm_class_desc_t *ddm_class = NULL;
    uint8_t class_instance = DDM2_PARAMETER_INSTANCE_FIELD(parameter);

    ddm_class = ddm_class_desc_find_by_ddm_instance(dgn_list, class_instance);
    if (ddm_class == NULL)
    {
        ddm_class = ddm_class_desc_create(DDM2_PARAMETER_CLASS(parameter));
        if (ddm_class != NULL)
        {
            ddm_class_desc_init(class_instance, DDM2_PARAMETER_CLASS(parameter), ddm_class);
            for (unsigned int i = 0; i < ddm_class->ddm_class_desc_size; i++)
            {
                if (i == 1)
                {
                    /* Subscribe to the SYNC parameter at the end */
                    continue;
                }
                ddmw_add(&ddm_container, &ddm_class->params_items[i].ddm2_item, ddm_class->params_items[i].ddm2_param, class_instance);
                ddmw_set_type(&ddm_class->params_items[i].ddm2_item, DDMW_ACTION_SET);
                ddmw_subscribe(&ddm_class->params_items[i].ddm2_item);
            }

            if (ddm_class->ddm_class_desc_size > 2)
            {
                ddmw_add(&ddm_container, &ddm_class->params_items[1].ddm2_item, ddm_class->params_items[1].ddm2_param, class_instance);
                ddmw_set_type(&ddm_class->params_items[1].ddm2_item, DDMW_ACTION_SET);
                ddmw_subscribe(&ddm_class->params_items[1].ddm2_item);
            }

            ddm_class_desc_insert(ddm_class, dgn_list);
        }
    }
}

static void start_subscriptions_to_rvc_params(uint32_t class_instance)
{
    for (size_t i = 0; i < ELEMENTS(l_sub_rvc_param); i++)
    {
        if (DDM2_PARAMETER_CLASS(l_sub_rvc_param[i].ddm2_param) == class_instance)
        {
            ddmw_add(&ddm_container, l_sub_rvc_param[i].ddm2_item, l_sub_rvc_param[i].ddm2_param, DDM2_PARAMETER_INSTANCE_FIELD(l_sub_rvc_param[i].ddm2_param));
            ddmw_set_type(l_sub_rvc_param[i].ddm2_item, DDMW_ACTION_SET);
            ddmw_subscribe(l_sub_rvc_param[i].ddm2_item);
        }
    }
}

static void stop_subscriptions_to_rvc_params(uint32_t class_instance)
{
    for (size_t i = 0; i < ELEMENTS(l_sub_rvc_param); i++)
    {
        if (DDM2_PARAMETER_CLASS(l_sub_rvc_param[i].ddm2_param) == class_instance)
        {
            ddmw_remove(&ddm_container, l_sub_rvc_param[i].ddm2_item);
        }
    }
}

static void process_batt_mapping(uint8_t batt_inst, uint8_t dc_inst, uint8_t sa)
{
    batt_map_t *batt_map = NULL;
    uint8_t batt_count = 0;
    batt_map_t *bm = NULL;
    LIST_FOREACH(bm, &l_batt_map, list_node)
    {
        if (bm->dc_inst == dc_inst)
        {
            if ((bm->batt_inst == batt_inst) && (bm->sa == sa))
            {
                LOG(W, "Battery instance %d with SA %d is already mapped to DC instance %d", batt_inst, sa, dc_inst);
                return;
            }
            batt_count++;
        }
    }
    if (batt_count >= MAX_NO_OF_BATT_IN_BANK)
    {
        LOG(E, "Maximum number of batteries per DC instance (%d) exceeded for dc_inst %d", MAX_NO_OF_BATT_IN_BANK, dc_inst);
        return;
    }
    batt_map = hal_mem_malloc_prefer(sizeof(batt_map_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (batt_map != NULL)
    {
        SORTED_LIST_VALUE_TYPE mbat_ddm_instance;
        SORTED_LIST_VALUE_TYPE prod_instance;
        if (sorted_list_unique_get(&mbat_ddm_instance, &dc_inst_mbat_ddm_inst_mapping_table, (const SORTED_LIST_KEY_TYPE)dc_inst, 0) == SORTED_LIST_OK)
        {
            if (sorted_list_unique_get(&prod_instance, &mbat_prod_inst_mapping_table, mbat_ddm_instance, 0) == SORTED_LIST_OK)
            {
                batt_map->is_master = is_batt_inst_master(dc_inst, (uint8_t)prod_instance, sa);
                batt_map->batt_inst = batt_inst;
                batt_map->dc_inst = dc_inst;
                batt_map->sa = sa;
                LIST_INSERT_HEAD(&l_batt_map, batt_map, list_node);
                LOG(D, "Battery instance %d with SA %d mapped to DC instance %d. is_master: %d", batt_map->batt_inst, batt_map->sa, batt_map->dc_inst, batt_map->is_master);
                ddm_class_desc_t *mbat_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)mbat_ddm_instance);
                if (batt_map->is_master && (mbat_ddm_inst != NULL))
                {
                    /* updated MBAT<X>INST with batt_inst */
                    ddmw_item_t *mbat0inst_item = ddmw_find_item(&ddm_container, MBAT0INST | DDM2_PARAMETER_INSTANCE(mbat_ddm_instance));
                    if (mbat0inst_item)
                    {
                        ddmw_set_i32(mbat0inst_item, batt_inst);
                    }
                    SORTED_LIST_VALUE_TYPE mbathist_ddm_instance;
                    /* Check any corresponding MBATHIST0 class for this dc inst */
                    if (sorted_list_unique_get(&mbathist_ddm_instance, &dc_inst_mbathist_ddm_inst_mapping_table, (const SORTED_LIST_KEY_TYPE)dc_inst, 0) == SORTED_LIST_OK)
                    {
                        /* updated MBATHIST<X>INST with batt_inst */
                        ddmw_item_t *mbathist0inst_item = ddmw_find_item(&ddm_container, MBATHIST0INST | DDM2_PARAMETER_INSTANCE(mbathist_ddm_instance));
                        if (mbathist0inst_item)
                        {
                            ddmw_set_i32(mbathist0inst_item, batt_inst);
                        }
                    }
                }
            }
            else
            {
                LOG(E, "No PROD instance found for MBAT instance %d", mbat_ddm_instance);
                hal_mem_free(batt_map);
            }
        }
        else
        {
            LOG(E, "No MBAT instance found for DC instance %d", dc_inst);
            hal_mem_free(batt_map);
        }
    }
    else
    {
        LOG(E, "Memory allocation failed for batt_map");
    }
}

static void remove_batt_mapping(uint8_t dc_inst)
{
    batt_map_t *batt_map = NULL;
    batt_map_t *batt_map_tmp = NULL;
    LIST_FOREACH_SAFE(batt_map, &l_batt_map, list_node, batt_map_tmp)
    {
        if (batt_map->dc_inst == dc_inst)
        {
            LIST_REMOVE(batt_map, list_node);
            hal_mem_free(batt_map);
        }
    }
}

static uint8_t get_src_addresses_for_dc_inst(uint8_t dc_inst, int32_t *sa_list, uint8_t *batt_inst_list)
{
    TRUE_CHECK(sa_list != NULL);
    uint8_t sa_count = 0;
    batt_map_t *batt_map = NULL;
    LIST_FOREACH(batt_map, &l_batt_map, list_node)
    {
        if (batt_map->dc_inst == dc_inst)
        {
            if (sa_count < MAX_NO_OF_BATT_IN_BANK)
            {
                sa_list[sa_count] = batt_map->sa;
                if (batt_inst_list != NULL)
                {
                    batt_inst_list[sa_count] = batt_map->batt_inst;
                }
                sa_count++;
            }
            else
            {
                LOG(E, "Maximum number of SAs per battery bank exceeded");
                break;
            }
        }
    }
    return sa_count;
}

static bool is_batt_inst_master(uint8_t dc_inst, uint8_t prod_inst, uint8_t sa)
{
    bool is_master = false;
    int prod_instance = ProdDBSearchCache(&sa, FIELD_PROP_SA);
    if (prod_instance == prod_inst)
    {
        is_master = true;
    }
    return is_master;
}

static int32_t get_sa_for_master_batt_inst(uint8_t dc_inst)
{
    int32_t sa = 0;
    batt_map_t *batt_map = NULL;
    LIST_FOREACH(batt_map, &l_batt_map, list_node)
    {
        if ((batt_map->dc_inst == dc_inst) && (batt_map->is_master))
        {
            sa = batt_map->sa;
            break;
        }
    }
    return sa;
}

static uint8_t get_mbat_inst_by_sa(uint8_t sa)
{
    uint8_t ret_val = INVALID_INSTANCE;
    uint8_t dc_inst = INVALID_INSTANCE;
    batt_map_t *batt_map = NULL;
    LIST_FOREACH(batt_map, &l_batt_map, list_node)
    {
        if (batt_map->sa == sa)
        {
            dc_inst = batt_map->dc_inst;
            break;
        }
    }
    if (dc_inst != INVALID_INSTANCE)
    {
        SORTED_LIST_VALUE_TYPE mbat_ddm_instance;
        if (sorted_list_unique_get(&mbat_ddm_instance, &dc_inst_mbat_ddm_inst_mapping_table, dc_inst, 0) == SORTED_LIST_OK)
        {
            /* MBAT instance found for the given DC instance */
            ret_val = (uint8_t)mbat_ddm_instance;
        }
    }
    return ret_val;
}

static MAYBE_UNUSED uint8_t get_dc_inst_by_sa(uint8_t sa)
{
    uint8_t ret_val = INVALID_INSTANCE;
    batt_map_t *batt_map = NULL;
    LIST_FOREACH(batt_map, &l_batt_map, list_node)
    {
        if (batt_map->sa == sa)
        {
            ret_val = batt_map->dc_inst;
            break;
        }
    }
    return ret_val;
}

static void request_mbathtr_status(uint8_t mbat_ddm_inst)
{
    // Only valid for batteries and not shunts
    SORTED_LIST_VALUE_TYPE prod_instance;
    if (sorted_list_unique_get(&prod_instance, &mbat_prod_inst_mapping_table, mbat_ddm_inst, 0) != SORTED_LIST_OK)
    {
        LOG(W, "No PROD instance found for MBAT instance %d", mbat_ddm_inst);
    }
    else
    {
        // Is this product a battery product?
        if (ProdDBGetProductType(prod_instance) == PRODDB_PRODUCTTYPE_BATTERY)
        {
            int32_t addr;
            SORTED_LIST_KEY_TYPE dc_inst_mapped;
            int no_of_dc_inst_mapped = 1;
            TRUE_CHECK(sorted_list_get_keys(&dc_inst_mapped, &no_of_dc_inst_mapped, &dc_inst_mbat_ddm_inst_mapping_table, (SORTED_LIST_VALUE_TYPE)mbat_ddm_inst, 0) == SORTED_LIST_OK);
            addr = get_sa_for_master_batt_inst(dc_inst_mapped);
            if (addr != 0)
            {
                /* Request the status of the heater from the master battery only */
                gp_req_cmd_t gp_cmd;
                memset(&gp_cmd, GP_DEFAULT_VALUE, sizeof(gp_req_cmd_t));
                gp_cmd.pwd = GP_PROP_MSG_PWD;
                gp_cmd.cmd = GP_REQUEST_CMD;
                gp_cmd.prod_type = GP_PROD_TYPE_BATT_SHUNT;
                gp_cmd.msg_id = GP_MSG_ID_0;
                process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&addr, sizeof(addr));
                process_sub_param_immediately(&l_rvc_prop_data, (const void *)&gp_cmd, sizeof(gp_cmd));
                process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
            }
            else
            {
                LOG(W, "No master battery found for DC instance %d", dc_inst_mapped);
            }
        }
    }
}

static void process_prop_data(RVCPROP0DATA_T *prop_data)
{
    TRUE_CHECK(prop_data != NULL);
    /* Check for GoPower proprietary message */
    gp_req_resp_t *gp_resp = (gp_req_resp_t *)prop_data->data;
    if (gp_resp->pwd == GP_PROP_MSG_PWD) /* GoPower proprietary message password */
    {
        LOG(D, "Received data: cmd_type=0x%02X, prod_type=0x%02X, msg_id=0x%02X", gp_resp->cmd, gp_resp->prod_type, gp_resp->msg_id);
        switch (gp_resp->cmd)
        {
        case GP_RESPONSE_CMD:
        {
            if ((gp_resp->msg_id == GP_MSG_ID_0) && (gp_resp->prod_type == GP_PROD_TYPE_BATT_SHUNT))
            {
                gp_msg_id_0_resp_t *msg_id_0_resp = (gp_msg_id_0_resp_t *)gp_resp->resp_data;
                uint8_t addr = (uint8_t)ddmw_get_i32(&l_rvc_prop_addr);
                uint8_t mbat_inst = get_mbat_inst_by_sa(addr);
                if (mbat_inst != INVALID_INSTANCE)
                {
                    /* Parse heater mode */
                    ddmw_item_t *l_mbathmode_status = NULL;
                    l_mbathmode_status = ddmw_find_item(&ddm_container, MBAT0HMODE | DDM2_PARAMETER_INSTANCE(mbat_inst));
                    if (l_mbathmode_status != NULL)
                    {
                        int32_t htr_mode = (int32_t)msg_id_0_resp->htr_mode;
                        switch (htr_mode)
                        {
                        case MBAT0HMODE_AUTOMATIC:
                            /* fallthrough */
                        case MBAT0HMODE_ON:
                            /* fallthrough */
                        case MBAT0HMODE_OFF:
                            LOG(D, "Setting MBAT0MODE for MBAT instance %d to %d", mbat_inst, htr_mode);
                            ddmw_set_i32(l_mbathmode_status, htr_mode);
                            break;
                        default:
                            LOG(W, "Unexpected heater mode received: %d", htr_mode);
                            break;
                        }
                    }
                    else
                    {
                        LOG(W, "MBAT0HMODE item not found for MBAT instance %d", mbat_inst);
                    }
                    /* Parse heater status */
                    ddmw_item_t *l_mbathtr_status = NULL;
                    l_mbathtr_status = ddmw_find_item(&ddm_container, MBAT0HTR | DDM2_PARAMETER_INSTANCE(mbat_inst));
                    if (l_mbathtr_status != NULL)
                    {
                        int32_t htr_status = (int32_t)msg_id_0_resp->htr_status;
                        switch (htr_status)
                        {
                        case 0:
                            htr_status = MBAT0HTR_ON;
                            break;
                        case 1:
                            htr_status = MBAT0HTR_OFF;
                            break;
                        default:
                            break;
                        }
                        if (htr_status > MBAT0HTR_OFF_SOC_TOO_LOW)
                        {
                            LOG(W, "Unexpected heater status received: %d", htr_status);
                        }
                        else
                        {
                            LOG(D, "Setting MBAT0HTR for MBAT instance %d to %d", mbat_inst, htr_status);
                            ddmw_set_i32(l_mbathtr_status, htr_status);
                        }
                    }
                    else
                    {
                        LOG(W, "MBAT0HTR item not found for MBAT instance %d", mbat_inst);
                    }
                }
                else
                {
                    LOG(W, "No MBAT instance found for SA %d", addr);
                }
            }
        }
        break;
        case GP_SHUNT_STS_1:
        case GP_SHUNT_STS_2:
        {
            /* Shunt proprietary messages - map SA to DC instance and process */
            uint8_t addr = (uint8_t)ddmw_get_i32(&l_rvc_prop_addr);
            LOG(D, "Processing shunt status 0x%02X from SA %d (DC instance %d)", gp_resp->cmd, addr, shunt.dc_instance);
            process_shunt(prop_data->data, gp_resp->cmd, shunt.dc_instance);
        }
        break;
        case GP_SHUNT_CMD_2:
        case GP_SHUNT_CMD_3:
        {
            LOG(D, "GOT COMMAND");
        }
        break;
        default:
            break;
        }
    }
}

static void process_sub_param_immediately(ddmw_item_t *item, const void *data, int size)
{
    TRUE_CHECK(item != NULL);
    TRUE_CHECK(data != NULL);
    TRUE_CHECK(size > 0);

    /* Process the sub-parameter immediately, but set the flag back to false after setting the data,
       since the current implementation in the DDMW library does not support setting data with immediate processing
       for parameters that the connector is subscribed to. */
    ddmw_set_requires_immediate_processing(item, true);
    ddmw_set_data(item, data, size);
    ddmw_set_requires_immediate_processing(item, false);
}

static MAYBE_UNUSED bool is_class_mbat_mbathist_mapped(uint32_t class_instance)
{
    uint32_t base_class = DDM2_PARAMETER_BASE_INSTANCE(class_instance);
    bool ret = false;
    /* Check if this class appears in rvc_mbat_params table */
    for (size_t i = 0; i < ELEMENTS(rvc_mbat_params); i++)
    {
        if (DDM2_PARAMETER_CLASS(rvc_mbat_params[i].sub_ddm2_param) == base_class)
        {
            ret = true;
            break;
        }
    }

    if (!ret)
    {
        /* Check if this class appears in rvc_mbathist_params table */
        for (size_t i = 0; i < ELEMENTS(rvc_mbathist_params); i++)
        {
            if (DDM2_PARAMETER_CLASS(rvc_mbathist_params[i].sub_ddm2_param) == base_class)
            {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

/**
 * @brief Used to handle indicate request for GoPower batteries
 * @param indicate True if there is an indicate request
 * @param ddm_instance Instance of battery
 * @return None
 */
static void batt_indicate_request_handler(bool indicate, int ddm_instance)
{
    if ((indicate == false) || (ddm_instance <= INVALID_DDM_INSTANCE))
    {
        return;
    }

    int prod_inst = ddm_instance;
    PROD0PROP_T prodprop;
    size_t prop_size = sizeof(PROD0PROP_T);

    ProdDBReadCache(FIELD_PROP, prod_inst, &prodprop, &prop_size);

    gp_batt_cfg_cmd_1_t gp_batt_cfg_cmd_1;
    memset(&gp_batt_cfg_cmd_1, GP_DEFAULT_VALUE, sizeof(gp_batt_cfg_cmd_1));
    gp_batt_cfg_cmd_1.pwd = GP_PROP_MSG_PWD;
    gp_batt_cfg_cmd_1.cmd = GP_BATT_CFG_CMD_1;
    gp_batt_cfg_cmd_1.blink_mode = GP_BATT_BLINK_MODE_BLINK_ACTIVE;
    send_raw_rvc_frame((uint8_t *)&gp_batt_cfg_cmd_1, (RVC_PROPRIETARY | ((uint8_t)prodprop.addr)));
}

/**
 * @brief: Process the mbat status
 * @param parameter DDM2 parameter of RVC data
 * @return None
 */
static void process_pub_mbat_status(uint32_t parameter)
{
    uint32_t mbat_ddm_inst;
    ddm_class_desc_t *mbat_class = NULL;
    ddm_class_desc_t *rvc_class = NULL;
    bool is_set = false;

    /* Find the specific MBAT DDM instance that is linked to the incoming parameter's RVC class instance*/
    if (sorted_list_unique_get(&mbat_ddm_inst, &rvc_mbat_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(parameter), 0) != SORTED_LIST_OK)
    {
        return;
    }
    /* Find the specific class descriptor from the MBAT list*/
    mbat_class = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)mbat_ddm_inst);
    if (mbat_class == NULL)
    {
        return;
    }

    uint32_t mbat_param;
    /* Depending on the RVCDCSRC parameter, get the mapped MBAT parameter that needs to be updated */
    if (sorted_list_unique_get(&mbat_param, &rvc_mbat_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(parameter), 0) != SORTED_LIST_OK)
    {
        return;
    }

    rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrcsix, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(parameter));
    if (rvc_class == NULL)
    {
        return;
    }

    for (uint32_t i = 0; i < rvc_class->ddm_class_desc_size; i++)
    {
        if (rvc_class->params_items[i].ddm2_param == parameter)
        {
            /* Find the mapping entry for this parameter to get the factors */
            for (size_t map_idx = 0; map_idx < ELEMENTS(batt_err_mapping_table); map_idx++)
            {
                if (batt_err_mapping_table[map_idx].rvc_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(parameter))
                {
                    int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);

                    int no_of_err_code_list = MBAT_ERR_MAPPING_DEPTH;
                    SORTED_LIST_KEY_TYPE *get_err_code_list = hal_mem_malloc_prefer((sizeof(SORTED_LIST_KEY_TYPE) * MBAT_ERR_MAPPING_DEPTH), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                    uint16_t *set_err_code_list = hal_mem_malloc_prefer((sizeof(uint16_t) * MBAT_ERR_MAPPING_DEPTH), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);

                    if (value)
                    {
                        if (sorted_list_unique_get(&mbat_ddm_inst, &mbat_err_table, batt_err_mapping_table[map_idx].err_code, false) != SORTED_LIST_OK)
                        {
                            TRUE_CHECK(sorted_list_unique_add(&mbat_err_table, batt_err_mapping_table[map_idx].err_code, mbat_ddm_inst) == SORTED_LIST_ENTRY_INSERTED);
                            if (sorted_list_get_keys(get_err_code_list, &no_of_err_code_list, &mbat_err_table, mbat_ddm_inst, 0) == SORTED_LIST_OK)
                            {
                                if (no_of_err_code_list)
                                {
                                    for (int j = 0; j < no_of_err_code_list; j++)
                                    {
                                        set_err_code_list[j] = (uint16_t)(get_err_code_list[j] & 0xFFFF);
                                    }
                                    is_set = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        if (sorted_list_unique_get(&mbat_ddm_inst, &mbat_err_table, batt_err_mapping_table[map_idx].err_code, true) == SORTED_LIST_OK)
                        {

                            if (sorted_list_get_keys(get_err_code_list, &no_of_err_code_list, &mbat_err_table, mbat_ddm_inst, 0) == SORTED_LIST_OK)
                            {
                                for (int j = 0; j < no_of_err_code_list; j++)
                                {
                                    set_err_code_list[j] = (uint16_t)(get_err_code_list[j] & 0xFFFF);
                                }
                            }
                            is_set = true;
                        }
                    }
                    if (is_set)
                    {
                        for (uint32_t i = 0; i < mbat_class->ddm_class_desc_size; i++)
                        {
                            if (DDM2_PARAMETER_BASE_INSTANCE(mbat_class->params_items[i].ddm2_param) == mbat_param)
                            {
                                ddmw_set_data(&mbat_class->params_items[i].ddm2_item, set_err_code_list, no_of_err_code_list * sizeof(set_err_code_list[0]));
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

/**
 * @brief: Process the subscribe mbat status
 * @param mbat_ddm_inst Instance of mabt status
 * @return None
 */
static void process_sub_mbat_status(uint8_t mbat_ddm_inst)
{
    ddm_class_desc_t *mbat_class = NULL;

    /* Find the specific class descriptor from the MBAT list*/
    mbat_class = ddm_class_desc_find_by_ddm_instance(&l_mbat, (uint8_t)mbat_ddm_inst);
    if (mbat_class == NULL)
    {
        return;
    }

    int no_of_err_code_list = MBAT_ERR_MAPPING_DEPTH;
    SORTED_LIST_KEY_TYPE *get_err_code_list = hal_mem_malloc_prefer((sizeof(SORTED_LIST_KEY_TYPE) * MBAT_ERR_MAPPING_DEPTH), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    uint16_t *set_err_code_list = hal_mem_malloc_prefer((sizeof(uint16_t) * MBAT_ERR_MAPPING_DEPTH), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);

    if (sorted_list_get_keys(get_err_code_list, &no_of_err_code_list, &mbat_err_table, mbat_ddm_inst, 0) == SORTED_LIST_OK)
    {
        if (no_of_err_code_list)
        {
            for (int j = 0; j < no_of_err_code_list; j++)
            {
                set_err_code_list[j] = (uint16_t)(get_err_code_list[j] & 0xFFFF);
            }
        }
    }

    for (uint32_t i = 0; i < mbat_class->ddm_class_desc_size; i++)
    {
        if (DDM2_PARAMETER_BASE_INSTANCE(mbat_class->params_items[i].ddm2_param) == MBAT0STATUS)
        {
            ddmw_set_requires_immediate_processing(&mbat_class->params_items[i].ddm2_item, true);
            ddmw_set_data(&mbat_class->params_items[i].ddm2_item, set_err_code_list, no_of_err_code_list * sizeof(set_err_code_list[0]));
            ddmw_set_requires_immediate_processing(&mbat_class->params_items[i].ddm2_item, false);
        }
    }
}

CONNECTOR connector_batt_mgmt_service =
    {
        .name = "Battery management service connector",
        .initialize = connector_batt_mgmt_init,
        .process_event = connector_batt_mgmt_service_process_task,
};

COMPILE_TIME_ASSERT(sizeof(gp_shunt_stscmd_2_t) == 8);
COMPILE_TIME_ASSERT(sizeof(gp_shunt_cmd_3_t) == 8);
COMPILE_TIME_ASSERT(sizeof(gp_dc_src_cfg_cmd_1_t) == 8);
COMPILE_TIME_ASSERT(sizeof(gp_dc_src_cfg_cmd_2_t) == 8);
COMPILE_TIME_ASSERT(sizeof(gp_dc_disconnect_cmd_t) == 8);
COMPILE_TIME_ASSERT(sizeof(gp_batt_cfg_cmd_1_t) == 8);
