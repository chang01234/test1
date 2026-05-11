/**
 * \file        connector_ic_mgmt_service.c
 * \date        2025-12-10
 * \author      Kire Janev (kire.janev@dometic.com)
 * \brief       Inverter charger management service that is the owner of the MINVERT/MCHRG and their related classes.
 *
 * \li          2025-12-10 (KJ) Initial implementation
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

#include "connector_ic_mgmt_service.h"

#include "connector.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "ddm_wrapper.h"
#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"
#include "ic_mgmt_ddm_class_desc.h"
#include "product_database.h"
#include <string.h>

#define RVC_MINVERT_PARAM_MAPPING_DEPTH     30
#define RVC_MINVERT_INST_MAPPING_DEPTH      60
#define INV_INST_MINVERT_INST_MAPPING_DEPTH 10
#define PROD_MINVERT_INST_MAPPING_DEPTH     10
#define MAX_NO_OF_LINKED_RVC_CLASSES        20
#define PROD_MODEL_MAPPING_DEPTH            10

#define RVC_PROPRIETARY        (uint32_t)0xEF00
#define DGN_INVERTER_CMD       (uint32_t)131027
#define DGN_INVERTER_CFG_CMD_1 (uint32_t)131024
#define DGN_INVERTER_CFG_CMD_2 (uint32_t)131023
#define DGN_INVERTER_CFG_CMD_3 (uint32_t)130765
#define DGN_INVERTER_CFG_CMD_4 (uint32_t)130714
#define DGN_CHARGER_CMD        (uint32_t)131013
#define DGN_CHARGER_CFG_CMD    (uint32_t)131012
#define DGN_CHARGER_CFG_CMD_2  (uint32_t)130965
#define DGN_CHARGER_CFG_CMD_3  (uint32_t)130763
#define DGN_CHARGER_CFG_CMD_4  (uint32_t)130750
#define DGN_CHARGER_EQ_CFG_CMD (uint32_t)130967

#define PROPRIETARY_RAW_RVC_FRAME_SIZE     8
#define PROP_CHRG_CFG_CMD_OP_CONFIG        0xE7
#define PROP_CHRG_CFG_CMD_OP_REQUEST       0xE8
#define PROP_CHRG_CFG_RESP_OP_REPORT       0xE6
#define PROP_INTERNAL_INV_TEMPS_OP_REQUEST 0xE3
#define PROP_INTERNAL_INV_TEMPS_OP_REPORT  0xE2

#define RVC_PROPRIETARY (uint32_t)0xEF00

#define GP_DEFAULT_VALUE                        0xFF
#define PROP_INTERNAL_INV_TEMPS_OFFSET          40
#define PROP_CHRG_CFG_OVPR_VOLTAGE_SCALE_FACTOR 10
#define RVC_VOLT_GAIN_FACTOR                    20
#define RVC_SEC_GAIN_FACTOR                     2

#define RVC_GLOBAL_DEST_ADDR 0xFF
#define INVALID_INSTANCE     0xFF

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_minvert_param_mapping_table, RVC_MINVERT_PARAM_MAPPING_DEPTH);             /* RVC<X> to MINVERT DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_minvert_ddm_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);           /* RVC<X> to MINVERT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(inv_inst_minvert_ddm_inst_mapping_table, INV_INST_MINVERT_INST_MAPPING_DEPTH); /* Inverter instance to MINVERT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(minvert_prod_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);              /* MINVERT DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_macout_param_mapping_table, RVC_MINVERT_PARAM_MAPPING_DEPTH);             /* RVC<X> to MACOUT DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_macout_ddm_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);           /* RVC<X> to MACOUT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(inv_inst_macout_ddm_inst_mapping_table, INV_INST_MINVERT_INST_MAPPING_DEPTH); /* Inverter instance to MACOUT DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(macout_prod_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);              /* MACOUT DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mchrg_param_mapping_table, RVC_MINVERT_PARAM_MAPPING_DEPTH);              /* RVC<X> to MCHRG DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mchrg_ddm_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);            /* RVC<X> to MCHRG DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(chrg_inst_mchrg_ddm_inst_mapping_table, INV_INST_MINVERT_INST_MAPPING_DEPTH); /* Charger instance to MCHRG DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(mchrg_prod_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);               /* MCHRG DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_macin_param_mapping_table, RVC_MINVERT_PARAM_MAPPING_DEPTH);              /* RVC<X> to MACIN DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_macin_ddm_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);            /* RVC<X> to MACIN DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(chrg_inst_macin_ddm_inst_mapping_table, INV_INST_MINVERT_INST_MAPPING_DEPTH); /* Charger instance to MACIN DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(macin_prod_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);               /* MACIN DDM instance to PROD DDM instance mapping */

DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_param_mapping_table, RVC_MINVERT_PARAM_MAPPING_DEPTH);              /* RVC<X> to MDCPROFILE DDM parameter mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_ddm_inst_mapping_table, RVC_MINVERT_PARAM_MAPPING_DEPTH);           /* RVC<X> to MDCPROFILE DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(chrg_inst_mdcprofile_ddm_inst_mapping_table, INV_INST_MINVERT_INST_MAPPING_DEPTH); /* Charger instance to MDCPROFILE DDM instance mapping */
DECLARE_SORTED_LIST_EXTRAM_PTR(mdcprofile_prod_inst_mapping_table, RVC_MINVERT_INST_MAPPING_DEPTH);               /* MDCPROFILE DDM instance to PROD DDM instance mapping */

typedef struct _mapped_params
{
    uint32_t sub_ddm2_param;
    uint32_t owned_ddm2_param;
} mapped_params_t;

typedef struct _sub_rvc_param
{
    uint32_t ddm2_param;
    ddmw_item_t *ddm2_item;
} sub_rvc_param_list_t;

/* INVERTER_CMD interface */
typedef struct _inverter_cmd
{
    uint8_t inv_instance;
    uint8_t inverter_enable : 2;
    uint8_t load_sense_enable : 2;
    uint8_t passthrough_enable : 2;
    uint8_t generator_support_enable : 2;
    uint8_t notused_bytes[5];
    uint8_t inverter_enable_on_startup : 2;
    uint8_t load_sense_enable_on_startup : 2;
    uint8_t passthrough_enable_on_startup : 2;
    uint8_t generator_support_enable_on_startup : 2;
} PACKED inverter_cmd_t;

/* INVERTER_CFG_CMD_1 interface */
typedef struct _inverter_cfg_cmd_1
{
    uint8_t inv_instance;
    uint16_t load_sense_pwr_threshold;
    uint16_t load_sense_interval;
    uint16_t shutdown_voltage_min;
    uint8_t notused_byte;
} PACKED inverter_cfg_cmd_1_t;

/* INVERTER_CFG_CMD_2 interface */
typedef struct _inverter_cfg_cmd_2
{
    uint8_t inv_instance;
    uint16_t shutdown_voltage_max;
    uint16_t warn_voltage_min;
    uint16_t warn_voltage_max;
    uint8_t notused_byte;
} PACKED inverter_cfg_cmd_2_t;

/* INVERTER_CFG_CMD_3 interface */
typedef struct _inverter_cfg_cmd_3
{
    uint8_t inv_instance;
    uint16_t shutdown_delay;
    uint8_t stack_mode;
    uint16_t shutdown_recovery_level;
    uint16_t generator_support_engage_current;
} PACKED inverter_cfg_cmd_3_t;

/* INVERTER_CFG_CMD_4 interface */
typedef struct _inverter_cfg_cmd_4
{
    uint8_t inv_instance;
    uint16_t output_ac_voltage;
    uint8_t output_frequency;
    uint16_t power_limit;
    uint16_t power_time_limit;
} PACKED inverter_cfg_cmd_4_t;

/* CHARGER_CMD interface */
typedef struct _charger_cmd
{
    uint8_t chrg_instance;
    uint8_t status;
    uint8_t default_state_power_up : 2;
    uint8_t auto_recharge_enable : 2;
    uint8_t force_charge : 4;
    uint16_t control_voltage;
    uint16_t control_current;
    uint8_t notused_byte;
} PACKED charger_cmd_t;

/* CHARGER_CFG_CMD interface */
typedef struct _charger_cfg_cmd
{
    uint8_t chrg_instance;
    uint8_t chrg_algorithm;
    uint8_t chrg_mode;
    uint8_t batt_sensor_present : 2;
    uint8_t chrg_installation_line : 2;
    uint16_t bank_size;
    uint8_t batt_type;
    uint8_t max_chrg_current;
} PACKED charger_cfg_cmd_t;

/* CHARGER_CFG_CMD_2 interface */
typedef struct _charger_cfg_cmd_2
{
    uint8_t chrg_instance;
    uint8_t max_chrg_curr_percent;
    uint8_t charge_rate_limit;
    uint8_t shore_breaker_size;
    uint8_t default_batt_temp;
    uint16_t recharge_voltage;
    uint8_t notused_byte;
} PACKED charger_cfg_cmd_2_t;

/* CHARGER_CFG_CMD_3 interface */
typedef struct _charger_cfg_cmd_3
{
    uint8_t chrg_instance;
    uint16_t bulk_voltage;
    uint16_t absorption_voltage;
    uint16_t float_voltage;
    uint8_t temp_comp_constant;
} PACKED charger_cfg_cmd_3_t;

/* CHARGER_CFG_CMD_4 interface */
typedef struct _charger_cfg_cmd_4
{
    uint8_t chrg_instance;
    uint16_t bulk_time;
    uint16_t absorption_time;
    uint16_t float_time;
    uint8_t notused_byte;
} PACKED charger_cfg_cmd_4_t;

/* CHARGER_EQ_CFG_CMD interface */
typedef struct _charger_eq_cfg_cmd
{
    uint8_t chrg_instance;
    uint16_t eq_voltage;
    uint16_t eq_time;
    uint8_t notused_bytes[3];
} PACKED charger_eq_cfg_cmd_t;

/* PROP_CHRG_CFG_REQ_CMD_RESP interface - OVP, OVPR, UVPR are DC volts * 10 */
typedef struct _prop_chrg_cfg_req_cmd_resp
{
    uint8_t operation;
    uint8_t notused_bytes[3];
    uint8_t ovp;
    uint8_t ovpr_limit_voltage;
    uint8_t uvpr_limit_voltage;
    uint8_t notused_byte;
} PACKED prop_chrg_cfg_req_cmd_resp_t;

/* PROP_REQ_INTERNAL_INV_TEMPS interface */
typedef struct _prop_req_internal_inv_temps
{
    uint8_t operation;
    uint8_t notused_bytes[7];
} PACKED prop_req_internal_inv_temps_t;

/* PROP_INV_INTERNAL_TEMP_REPORT interface */
typedef struct _prop_inv_internal_temp_report
{
    uint8_t operation;
    uint8_t transformer_temp;
    uint8_t fet_temp;
    uint8_t notused_bytes[5];
} PACKED prop_inv_internal_temp_report_t;

/* PROP_INV_CHRG_OPER_CHECK interface*/
typedef struct _prop_inv_chrg_oper_check
{
    uint8_t operation;
    uint8_t notused_bytes[7];
} PACKED prop_inv_chrg_oper_check_t;

/* Address to MDCPROFILE instance mapping */
typedef struct _mdcprofile_inst_addr_mapping
{
    uint8_t addr;
    uint8_t mdcprofile_instance;
} PACKED mdcprofile_inst_addr_mapping_t;

/* Address to MINVERT instance mapping */
typedef struct _minvert_inst_addr_mapping
{
    uint8_t addr;
    uint8_t minvert_instance;
} PACKED minvert_inst_addr_mapping_t;

static void inventory_callback(uint32_t parameter);
static void connector_ic_mgmt_service_process_task(const DDMP2_FRAME *frame);
static void manage_minvert_classes(uint32_t class_instance, uint8_t inv_inst, uint8_t prod_instance);
static void manage_macout_classes(uint32_t class_instance, uint8_t inv_inst, uint8_t prod_instance, uint8_t minvert_ddm_instance);
static void manage_mchrg_classes(uint32_t class_instance, uint8_t chrg_inst, uint8_t prod_instance, uint8_t minvert_ddm_instance);
static void manage_macin_classes(uint32_t class_instance, uint8_t chrg_inst, uint8_t prod_instance, uint8_t mchrg_ddm_instance);
static void manage_mdcprofile_classes(uint32_t class_instance, uint8_t chrg_inst, uint8_t prod_instance, uint8_t mchrg_ddm_instance);
static void subscribe_to_prop_classes(uint32_t parameter);
static bool is_rvc_class_minvert_class_related(uint32_t class_instance);
static bool is_rvc_class_macout_class_related(uint32_t class_instance);
static bool is_rvc_class_mchrg_class_related(uint32_t class_instance);
static bool is_rvc_class_macin_class_related(uint32_t class_instance);
static bool is_rvc_class_mdcprofile_class_related(uint32_t class_instance);
static void manage_subscriptions_for_prop_class(list_t *dgn_list, uint32_t parameter);
static void remove_macout_instance(uint8_t macout_instance, uint8_t prod_instance);
static void remove_minvert_instance(uint8_t minvert_instance, uint8_t prod_instance);
static void remove_mchrg_instance(uint8_t mchrg_instance, uint8_t prod_instance);
static void remove_macin_instance(uint8_t macin_instance, uint8_t prod_instance);
static void remove_mdcprofile_instance(uint8_t mdcprofile_instance, uint8_t prod_instance);
static void start_subscriptions_to_rvc_params(uint32_t class_instance);
static void stop_subscriptions_to_rvc_params(uint32_t class_instance);
static void send_raw_rvc_frame(uint8_t *data, uint32_t dgn);
static void process_sub_param_immediately(ddmw_item_t *item, const void *data, int size);
static void process_prop_data(RVCPROP0DATA_T *prop_data, uint8_t prop_addr);
static void add_minvert_mapping(uint8_t minvert_instance, uint8_t prod_instance);
static void add_mdcprofile_mapping(uint8_t mdcprofile_instance, uint8_t prod_instance);
static void remove_minvert_mapping(uint8_t minvert_instance);
static void remove_mdcprofile_mapping(uint8_t mdcprofile_instance);
static uint8_t get_minvert_instance_from_addr(uint8_t addr);
static uint8_t get_mdcprofile_instance_from_addr(uint8_t addr);
static void check_and_update_addr_minvert_inst_mapping(uint8_t prod_instance);
static void check_and_update_addr_mdcprofile_inst_mapping(uint8_t prod_instance);

static EXT_RAM_ATTR uint8_t minvert_linked_idx;
static EXT_RAM_ATTR uint8_t macout_linked_idx;
static EXT_RAM_ATTR uint8_t mchrg_linked_idx;
static EXT_RAM_ATTR uint8_t macin_linked_idx;
static EXT_RAM_ATTR uint8_t mdcprofile_linked_idx;

static EXT_RAM_ATTR ddmw_t ddm_container;

static EXT_RAM_ATTR list_t l_rvcinvert;
static EXT_RAM_ATTR list_t l_rvcinvertcfg;
static EXT_RAM_ATTR list_t l_rvcinvertcfgtwo;
static EXT_RAM_ATTR list_t l_rvcinvertcfgthree;
static EXT_RAM_ATTR list_t l_rvcinvertcfgfour;
static EXT_RAM_ATTR list_t l_rvcinvertdc;
static EXT_RAM_ATTR list_t l_rvcinverttemp;
static EXT_RAM_ATTR list_t l_rvcinverttemptwo;
static EXT_RAM_ATTR list_t l_rvcinvertac;
static EXT_RAM_ATTR list_t l_rvcinvertactwo;
static EXT_RAM_ATTR list_t l_rvcinvertacthree;
static EXT_RAM_ATTR list_t l_rvcchrg;
static EXT_RAM_ATTR list_t l_rvcchrgtwo;
static EXT_RAM_ATTR list_t l_rvcchrgcfg;
static EXT_RAM_ATTR list_t l_rvcchrgac;
static EXT_RAM_ATTR list_t l_rvcchrgacthree;
static EXT_RAM_ATTR list_t l_rvcchrgcfgtwo;
static EXT_RAM_ATTR list_t l_rvcchrgacfaultcfg;
static EXT_RAM_ATTR list_t l_rvcdcsrc;
static EXT_RAM_ATTR list_t l_rvcdcsrctwo;
static EXT_RAM_ATTR list_t l_rvcchrgcfgthree;
static EXT_RAM_ATTR list_t l_rvcchrgcfgfour;
static EXT_RAM_ATTR list_t l_rvcchrgeqcfg;
static EXT_RAM_ATTR list_t l_minvert;
static EXT_RAM_ATTR list_t l_macout;
static EXT_RAM_ATTR list_t l_mchrg;
static EXT_RAM_ATTR list_t l_macin;
static EXT_RAM_ATTR list_t l_mdcprofile;
static EXT_RAM_ATTR list_t l_prod;

static EXT_RAM_ATTR ddmw_item_t l_rvc_prop_addr;
static EXT_RAM_ATTR ddmw_item_t l_rvc_prop_data;
static EXT_RAM_ATTR ddmw_item_t l_rvc_prop_sync;
static EXT_RAM_ATTR ddmw_item_t l_rvc_mgmt_addr;
static EXT_RAM_ATTR ddmw_item_t l_rvc_mgmt_ack;

static EXT_RAM_ATTR mdcprofile_inst_addr_mapping_t mdcprofile_inst_addr_map[10];
static EXT_RAM_ATTR minvert_inst_addr_mapping_t minvert_inst_addr_map[10];

static const mapped_params_t rvc_minvert_params[] = {
    {.sub_ddm2_param = RVCINVERT0ENABLE, .owned_ddm2_param = MINVERT0ENABLE},
    {.sub_ddm2_param = RVCINVERT0LOADSENSE, .owned_ddm2_param = MINVERT0LOADSENSE},

    {.sub_ddm2_param = RVCINVERTCFG0SHUTDOWNVOLTMIN, .owned_ddm2_param = MINVERT0SHDOWNVOLTMIN},
    {.sub_ddm2_param = RVCINVERTCFG0LOADSENSEINT, .owned_ddm2_param = MINVERT0LSENSEINT},
    {.sub_ddm2_param = RVCINVERTCFG0LOADSENSEPOWTH, .owned_ddm2_param = MINVERT0LSENSEPOW},

    {.sub_ddm2_param = RVCINVERTCFGTWO0SHUTDOWNVOLTMAX, .owned_ddm2_param = MINVERT0SHDOWNVOLTMAX},
    {.sub_ddm2_param = RVCINVERTCFGTWO0WARNVOLTMAX, .owned_ddm2_param = MINVERT0WARNVOLTMAX},
    {.sub_ddm2_param = RVCINVERTCFGTWO0WARNVOLTMIN, .owned_ddm2_param = MINVERT0WARNVOLTMIN},

    {.sub_ddm2_param = RVCINVERTCFGTHREE0STACKMODE, .owned_ddm2_param = MINVERT0STACK},
    {.sub_ddm2_param = RVCINVERTCFGTHREE0SHUTDOWNRECLVL, .owned_ddm2_param = MINVERT0RECVOLT},

    {.sub_ddm2_param = RVCINVERTCFGFOUR0VOLT, .owned_ddm2_param = MINVERT0OUTVOLT},
    {.sub_ddm2_param = RVCINVERTCFGFOUR0FREQ, .owned_ddm2_param = MINVERT0OUTFREQ},
    {.sub_ddm2_param = RVCINVERTCFGFOUR0POWLIMIT, .owned_ddm2_param = MINVERT0POWLIM},

    {.sub_ddm2_param = RVCINVERTDC0VOLT, .owned_ddm2_param = MINVERT0VOLT},
    {.sub_ddm2_param = RVCINVERTDC0CURR, .owned_ddm2_param = MINVERT0CURR},

    {.sub_ddm2_param = RVCINVERTTEMP0FETONE, .owned_ddm2_param = MINVERT0FET1TEMP},
    {.sub_ddm2_param = RVCINVERTTEMPTWO0PB, .owned_ddm2_param = MINVERT0PCBTEMP},

    {.sub_ddm2_param = RVCINVERTACTWO0CAP, .owned_ddm2_param = MINVERT0MAXCURR},
};

static const mapped_params_t rvc_macout_params[] = {
    {.sub_ddm2_param = RVCINVERTAC0VOLT, .owned_ddm2_param = MACOUT0RMSVOLT},
    {.sub_ddm2_param = RVCINVERTAC0CURR, .owned_ddm2_param = MACOUT0RMSCURR},
    {.sub_ddm2_param = RVCINVERTAC0FREQ, .owned_ddm2_param = MACOUT0FREQ},
    {.sub_ddm2_param = RVCINVERTACTWO0PCURR, .owned_ddm2_param = MACOUT0PEAKCURR},
    {.sub_ddm2_param = RVCINVERTACTWO0PVOLT, .owned_ddm2_param = MACOUT0PEAKVOLT},
    {.sub_ddm2_param = RVCINVERTACTHREE0WAVEFORM, .owned_ddm2_param = MACOUT0WAVE},
    {.sub_ddm2_param = RVCINVERTACTHREE0REALPOW, .owned_ddm2_param = MACOUT0POWER},
};

static const mapped_params_t rvc_mchrg_params[] = {
    {.sub_ddm2_param = RVCCHRG0DEFST, .owned_ddm2_param = MCHRG0POWUP},
    {.sub_ddm2_param = RVCCHRG0FORCE, .owned_ddm2_param = MCHRG0FORCE},
    {.sub_ddm2_param = RVCCHRG0VOLT, .owned_ddm2_param = MCHRG0VOLT},
    {.sub_ddm2_param = RVCCHRG0CURR, .owned_ddm2_param = MCHRG0CURR},

    {.sub_ddm2_param = RVCCHRGCFG0MODE, .owned_ddm2_param = MCHRG0MODE},

    {.sub_ddm2_param = RVCCHRGTWO0VOLT, .owned_ddm2_param = MCHRG0CVOLT},
    {.sub_ddm2_param = RVCCHRGTWO0CURR, .owned_ddm2_param = MCHRG0CCURR},
    {.sub_ddm2_param = RVCCHRGTWO0TEMP, .owned_ddm2_param = MCHRG0TEMP},
};

static const mapped_params_t rvc_macin_params[] = {
    {.sub_ddm2_param = RVCCHRGCFGTWO0BREAKER, .owned_ddm2_param = MACIN0MAXCURR},

    {.sub_ddm2_param = RVCCHRGAC0VOLT, .owned_ddm2_param = MACIN0RMSVOLT},
    {.sub_ddm2_param = RVCCHRGAC0CURR, .owned_ddm2_param = MACIN0RMSCURR},
    {.sub_ddm2_param = RVCCHRGAC0FREQ, .owned_ddm2_param = MACIN0FREQ},

    {.sub_ddm2_param = RVCCHRGACTHREE0REALPOW, .owned_ddm2_param = MACIN0POWER},

    {.sub_ddm2_param = RVCCHRGACFAULTCFG0LVOLT, .owned_ddm2_param = MACIN0LVOLT},
};

static const mapped_params_t rvc_mdcprofile_params[] = {
    {.sub_ddm2_param = RVCDCSRC0VOLT, .owned_ddm2_param = MDCPROFILE0DCVOLT},

    {.sub_ddm2_param = RVCDCSRCTWO0STEMP, .owned_ddm2_param = MDCPROFILE0TEMP},

    {.sub_ddm2_param = RVCCHRGCFG0ALGO, .owned_ddm2_param = MDCPROFILE0ALGO},
    {.sub_ddm2_param = RVCCHRGCFG0BANK, .owned_ddm2_param = MDCPROFILE0BANK},
    {.sub_ddm2_param = RVCCHRGCFG0TYPE, .owned_ddm2_param = MDCPROFILE0TYPE},
    {.sub_ddm2_param = RVCCHRGCFG0MAXCURR, .owned_ddm2_param = MDCPROFILE0MAXCURR},

    {.sub_ddm2_param = RVCCHRGCFGTHREE0AVOLT, .owned_ddm2_param = MDCPROFILE0BVOLT},
    {.sub_ddm2_param = RVCCHRGCFGTHREE0FVOLT, .owned_ddm2_param = MDCPROFILE0FVOLT},

    {.sub_ddm2_param = RVCCHRGCFGFOUR0ATIME, .owned_ddm2_param = MDCPROFILE0ABSDUR},

    {.sub_ddm2_param = RVCCHRGEQCFG0VOLT, .owned_ddm2_param = MDCPROFILE0EQVOLT},
};

static const sub_rvc_param_list_t l_sub_rvc_param[] = {
    {.ddm2_param = RVCPROP0DATA, .ddm2_item = &l_rvc_prop_data},
    {.ddm2_param = RVCPROP0ADDR, .ddm2_item = &l_rvc_prop_addr},
    {.ddm2_param = RVCPROP0SYNC, .ddm2_item = &l_rvc_prop_sync},
    {.ddm2_param = RVCMGNT0ADDR, .ddm2_item = &l_rvc_mgmt_addr},
    {.ddm2_param = RVCMGNT0ACK, .ddm2_item = &l_rvc_mgmt_ack},
};

static void send_raw_rvc_frame(uint8_t *data, uint32_t dgn)
{
    RVCMGNT0RAWTX_T txmsg;
    txmsg.is_ext_type = true;
    txmsg.addr = (uint8_t)ddmw_get_i32(&l_rvc_mgmt_addr);
    txmsg.dgn = dgn;
    memcpy(txmsg.data, data, sizeof(txmsg.data));
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, RVCMGNT0RAW, &txmsg, sizeof(txmsg), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
}

static void connector_ic_mgmt_service_process_task(const DDMP2_FRAME *frame)
{
    /* Process the frame received from broker and update the corresponding parameter */
    ddmw_process(&ddm_container, frame);
    /* If the frame is PUBLISH, proceed with updating the owned parameters from the subscribed ones */
    if (frame->frame.control == DDMP2_CONTROL_PUBLISH)
    {
        // uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter);

        switch (DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
        {

        /* Special handling for PRODXPROP parameter, the registration, management and linkage for the MINVERT, MCHRG, MACIN, MACOUT, MDCPROFILE
         classes goes through this parameter */
        case PROD0PROP:
        {
            PROD0PROP_T *prop_data = (PROD0PROP_T *)(frame->frame.publish.value.raw);
            prodxprop_type_t type = {0};
            type.data = prop_data->type;
            if ((type.type.cls == PRODXPROP_TYPE_CLASS_POWER) && (type.type.intf == PRODXPROP_TYPE_INTERFACE_RVC))
            {
                // Check if this is a inverter/charger type
                int prod_inst = DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter);
                int32_t type = ProdDBGetProductType(prod_inst);
                if ((type == PRODDB_PRODUCTTYPE_INVERTERCHARGER) ||
                    (type == PRODDB_PRODUCTTYPE_INVERTER) ||
                    (type == PRODDB_PRODUCTTYPE_CHARGER))
                {
                    size_t no_of_prop_classes = (ddmp2_value_size(frame) - sizeof(PROD0PROP_T)) / sizeof(uint32_t);
                    bool is_prod_inv_chg = false;

                    for (size_t i = 0; i < no_of_prop_classes; i++)
                    {
                        if (is_rvc_class_minvert_class_related(prop_data->classes[i]) ||
                            is_rvc_class_macout_class_related(prop_data->classes[i]) ||
                            is_rvc_class_mchrg_class_related(prop_data->classes[i]) ||
                            is_rvc_class_macin_class_related(prop_data->classes[i]) ||
                            is_rvc_class_mdcprofile_class_related(prop_data->classes[i]))
                        {
                            is_prod_inv_chg = true;
                            break;
                        }
                    }
                    if (is_prod_inv_chg)
                    {
                        for (size_t i = 0; i < no_of_prop_classes; i++)
                        {
                            subscribe_to_prop_classes(prop_data->classes[i]);
                        }

                        SORTED_LIST_VALUE_TYPE minvert_ddm_instance, mchrg_ddm_instance;

                        if (sorted_list_unique_get(&minvert_ddm_instance, &inv_inst_minvert_ddm_inst_mapping_table, prop_data->inst, 0) != SORTED_LIST_OK)
                        {
                            LOG(D, "No MINVERT instance found for INV instance %d", prop_data->inst);
                            minvert_ddm_instance = INVALID_DDM_INSTANCE;
                        }
                        else
                        {
                            LOG(D, "Found MINVERT instance %d for INV instance %d", minvert_ddm_instance, prop_data->inst);
                        }

                        if (sorted_list_unique_get(&mchrg_ddm_instance, &chrg_inst_mchrg_ddm_inst_mapping_table, prop_data->inst, 0) != SORTED_LIST_OK)
                        {
                            LOG(D, "No MCHRG instance found for CHRG instance %d", prop_data->inst);
                            mchrg_ddm_instance = INVALID_DDM_INSTANCE;
                        }
                        else
                        {
                            LOG(D, "Found MCHRG instance %d for CHRG instance %d", mchrg_ddm_instance, prop_data->inst);
                        }

                        check_and_update_addr_minvert_inst_mapping(DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        check_and_update_addr_mdcprofile_inst_mapping(DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));

                        /* Manage the MINVERT, MCHRG, MACIN, MACOUT, MDCPROFILE classses (registering, linkage, deregistering etc.)*/
                        for (size_t i = 0; i < no_of_prop_classes; i++)
                        {
                            if (is_rvc_class_minvert_class_related(prop_data->classes[i]))
                            {
                                manage_minvert_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                            }
                            if (is_rvc_class_macout_class_related(prop_data->classes[i]))
                            {
                                manage_macout_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), minvert_ddm_instance);
                            }
                            if (is_rvc_class_mchrg_class_related(prop_data->classes[i]))
                            {
                                manage_mchrg_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), minvert_ddm_instance);
                            }
                            if (is_rvc_class_macin_class_related(prop_data->classes[i]))
                            {
                                manage_macin_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), mchrg_ddm_instance);
                            }
                            if (is_rvc_class_mdcprofile_class_related(prop_data->classes[i]))
                            {
                                manage_mdcprofile_classes(prop_data->classes[i], prop_data->inst, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter), mchrg_ddm_instance);
                            }
                        }
                    }
                }
            }
        }
        break;

        case RVCINVERT0ENABLE:
        case RVCINVERT0ENABLESTART:
        case RVCINVERTCFG0SHUTDOWNVOLTMIN:
        case RVCINVERTCFG0LOADSENSEINT:
        case RVCINVERTCFG0LOADSENSEPOWTH:
        case RVCINVERTCFGTWO0SHUTDOWNVOLTMAX:
        case RVCINVERTCFGTWO0WARNVOLTMAX:
        case RVCINVERTCFGTWO0WARNVOLTMIN:
        case RVCINVERTCFGTHREE0STACKMODE:
        case RVCINVERTCFGTHREE0SHUTDOWNRECLVL:
        case RVCINVERTCFGFOUR0VOLT:
        case RVCINVERTCFGFOUR0FREQ:
        case RVCINVERTCFGFOUR0POWLIMIT:
        case RVCINVERTDC0VOLT:
        case RVCINVERTDC0CURR:
        case RVCINVERTTEMP0FETONE:
        case RVCINVERTTEMPTWO0PB:
        case RVCINVERTACTWO0CAP:
        {
            uint32_t minv_ddm_inst;
            ddm_class_desc_t *minv_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MINVERT DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&minv_ddm_inst, &rvc_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MINVERT list*/
                minv_class = ddm_class_desc_find_by_ddm_instance(&l_minvert, (uint8_t)minv_ddm_inst);
                if (minv_class != NULL)
                {
                    uint32_t minv_param;
                    /* Depending on the RVCDCSRC parameter, get the mapped MINVERT parameter that needs to be updated */
                    if (sorted_list_unique_get(&minv_param, &rvc_minvert_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCINVERT0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvert, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTCFG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfg, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTCFGTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfgtwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTCFGTHREE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfgthree, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTCFGFOUR0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfgfour, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTDC0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertdc, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTTEMP0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinverttemp, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTTEMPTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinverttemptwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;

                        case RVCINVERTACTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertactwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
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
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_minvert_params); map_idx++)
                                    {
                                        if (rvc_minvert_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            ddm_class_desc_update(minv_class, minv_param | DDM2_PARAMETER_INSTANCE(minv_ddm_inst), value);
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

        case RVCINVERT0STATUS:
        {
            /* Special handling for RVCINVERT0STATUS parameter to update the MINVERT parameters */
            uint32_t minv_ddm_inst;
            ddm_class_desc_t *minv_class = NULL;
            /* Find the specific MINVERT DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&minv_ddm_inst, &rvc_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MINVERT list*/
                minv_class = ddm_class_desc_find_by_ddm_instance(&l_minvert, (uint8_t)minv_ddm_inst);
                if (minv_class != NULL)
                {
                    int32_t enable = 0, passthrough = 0, loadsense = 0;

                    int32_t status_value = frame->frame.publish.value.int32;

                    if (status_value == 0)
                    {
                        enable = 0;
                        passthrough = 0;
                        loadsense = 0;
                    }
                    else if (status_value == 1 || status_value == 3 || status_value == 5 || status_value == 6)
                    {
                        enable = 1;
                        passthrough = 0;
                        loadsense = 0;
                    }
                    else if (status_value == 2)
                    {
                        enable = 1;
                        passthrough = 1;
                        loadsense = 0;
                    }
                    else if (status_value == 4)
                    {
                        enable = 1;
                        passthrough = 0;
                        loadsense = 1;
                    }

                    ddm_class_desc_update(minv_class, MINVERT0ENABLE | DDM2_PARAMETER_INSTANCE(minv_ddm_inst), enable);
                    ddm_class_desc_update(minv_class, MINVERT0PASSTHROUGH | DDM2_PARAMETER_INSTANCE(minv_ddm_inst), passthrough);
                    ddm_class_desc_update(minv_class, MINVERT0LOADSENSE | DDM2_PARAMETER_INSTANCE(minv_ddm_inst), loadsense);
                }
            }
        }
        break;

        case RVCINVERTAC0VOLT:
        case RVCINVERTAC0CURR:
        case RVCINVERTAC0FREQ:
        case RVCINVERTACTWO0PCURR:
        case RVCINVERTACTWO0PVOLT:
        case RVCINVERTACTHREE0WAVEFORM:
        case RVCINVERTACTHREE0REALPOW:
        {
            uint32_t macout_ddm_inst;
            ddm_class_desc_t *macout_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MACOUT DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&macout_ddm_inst, &rvc_macout_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MACOUT list*/
                macout_class = ddm_class_desc_find_by_ddm_instance(&l_macout, (uint8_t)macout_ddm_inst);
                if (macout_class != NULL)
                {
                    uint32_t macout_param;
                    /* Depending on the RVCDCSRC parameter, get the mapped MACOUT parameter that needs to be updated */
                    if (sorted_list_unique_get(&macout_param, &rvc_macout_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCINVERTAC0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertac, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCINVERTACTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertactwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCINVERTACTHREE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertacthree, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
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
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_macout_params); map_idx++)
                                    {
                                        if (rvc_macout_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            ddm_class_desc_update(macout_class, macout_param | DDM2_PARAMETER_INSTANCE(macout_ddm_inst), value);
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

        case RVCCHRG0OPERST:
        {
            /* Special handling for RVCCHRG0OPERST parameter to update the MCHRG parameters */
            uint32_t mchrg_ddm_inst;
            ddm_class_desc_t *mchrg_class = NULL;
            /* Find the specific MCHRG DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mchrg_ddm_inst, &rvc_mchrg_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MCHRG list*/
                mchrg_class = ddm_class_desc_find_by_ddm_instance(&l_mchrg, (uint8_t)mchrg_ddm_inst);
                if (mchrg_class != NULL)
                {
                    int32_t enable;
                    int32_t status_value = frame->frame.publish.value.int32;
                    if (status_value == 0)
                    {
                        enable = 0;
                    }
                    else
                    {
                        enable = 1;
                    }

                    ddm_class_desc_update(mchrg_class, MCHRG0ENA | DDM2_PARAMETER_INSTANCE(mchrg_ddm_inst), enable);
                    ddm_class_desc_update(mchrg_class, MCHRG0OPER | DDM2_PARAMETER_INSTANCE(mchrg_ddm_inst), status_value);
                }
            }
        }
        break;
        case RVCCHRG0DEFST:
        case RVCCHRG0FORCE:
        case RVCCHRG0VOLT:
        case RVCCHRG0CURR:
        case RVCCHRGTWO0VOLT:
        case RVCCHRGTWO0CURR:
        case RVCCHRGTWO0TEMP:
        case RVCCHRGCFG0MODE:
        {
            uint32_t mchrg_ddm_inst;
            ddm_class_desc_t *mchrg_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MCHRG DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mchrg_ddm_inst, &rvc_mchrg_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MCHRG list*/
                mchrg_class = ddm_class_desc_find_by_ddm_instance(&l_mchrg, (uint8_t)mchrg_ddm_inst);
                if (mchrg_class != NULL)
                {
                    uint32_t mchrg_param;
                    /* Depending on the RVC parameter, get the mapped MCHRG parameter that needs to be updated */
                    if (sorted_list_unique_get(&mchrg_param, &rvc_mchrg_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCCHRG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrg, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgtwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGCFG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfg, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
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
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_mchrg_params); map_idx++)
                                    {
                                        if (rvc_mchrg_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            ddm_class_desc_update(mchrg_class, mchrg_param | DDM2_PARAMETER_INSTANCE(mchrg_ddm_inst), value);
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

        case RVCCHRGCFGTWO0BREAKER:
        case RVCCHRGAC0VOLT:
        case RVCCHRGAC0CURR:
        case RVCCHRGAC0FREQ:
        case RVCCHRGACTHREE0REALPOW:
        case RVCCHRGACFAULTCFG0LVOLT:
        {
            uint32_t macin_ddm_inst;
            ddm_class_desc_t *macin_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MACIN DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&macin_ddm_inst, &rvc_macin_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MACIN list*/
                macin_class = ddm_class_desc_find_by_ddm_instance(&l_macin, (uint8_t)macin_ddm_inst);
                if (macin_class != NULL)
                {
                    uint32_t macin_param;
                    /* Depending on the RVC parameter, get the mapped MACIN parameter that needs to be updated */
                    if (sorted_list_unique_get(&macin_param, &rvc_macin_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCCHRGCFGTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfgtwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGAC0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgac, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGACTHREE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgacthree, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGACFAULTCFG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgacfaultcfg, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
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
                                    for (size_t map_idx = 0; map_idx < ELEMENTS(rvc_macin_params); map_idx++)
                                    {
                                        if (rvc_macin_params[map_idx].sub_ddm2_param == DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter))
                                        {
                                            int32_t value = ddmw_get_i32(&rvc_class->params_items[i].ddm2_item);
                                            ddm_class_desc_update(macin_class, macin_param | DDM2_PARAMETER_INSTANCE(macin_ddm_inst), value);
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

        case RVCDCSRC0VOLT:
        case RVCDCSRCTWO0STEMP:
        case RVCCHRGCFG0ALGO:
        case RVCCHRGCFG0BANK:
        case RVCCHRGCFG0TYPE:
        case RVCCHRGCFG0MAXCURR:
        case RVCCHRGCFGTHREE0AVOLT:
        case RVCCHRGCFGTHREE0FVOLT:
        case RVCCHRGCFGFOUR0ATIME:
        case RVCCHRGEQCFG0VOLT:
        {
            uint32_t mdcprofile_ddm_inst;
            ddm_class_desc_t *mdcprofile_class = NULL;
            ddm_class_desc_t *rvc_class = NULL;

            /* Find the specific MDCPROFILE DDM instance that is linked to the incoming parameter's RVC class instance*/
            if (sorted_list_unique_get(&mdcprofile_ddm_inst, &rvc_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_CLASS_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
            {
                /* Find the specific class descriptor from the MDCPROFILE list*/
                mdcprofile_class = ddm_class_desc_find_by_ddm_instance(&l_mdcprofile, (uint8_t)mdcprofile_ddm_inst);
                if (mdcprofile_class != NULL)
                {
                    uint32_t mdcprofile_param;
                    /* Depending on the RVC parameter, get the mapped MDCPROFILE parameter that needs to be updated */
                    if (sorted_list_unique_get(&mdcprofile_param, &rvc_mdcprofile_param_mapping_table, DDM2_PARAMETER_BASE_INSTANCE(frame->frame.publish.parameter), 0) == SORTED_LIST_OK)
                    {
                        switch (DDM2_PARAMETER_CLASS(frame->frame.publish.parameter))
                        {
                        case RVCDCSRC0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrc, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCDCSRCTWO0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcdcsrctwo, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGCFG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfg, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGCFGTHREE0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfgthree, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGCFGFOUR0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfgfour, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
                        }
                        break;
                        case RVCCHRGEQCFG0:
                        {
                            rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgeqcfg, (uint8_t)DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.publish.parameter));
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
                                            ddm_class_desc_update(mdcprofile_class, mdcprofile_param | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_inst), value);
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
                uint8_t prop_addr = (uint8_t)ddmw_get_i32(&l_rvc_prop_addr);
                ddmw_get_data(&l_rvc_prop_data, &prop_data, sizeof(RVCPROP0DATA_T));
                process_prop_data(&prop_data, prop_addr);
            }
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
        case MINVERT0ENABLE:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cmd_t inv_cmd;
                memset(&inv_cmd, GP_DEFAULT_VALUE, sizeof(inverter_cmd_t));
                inv_cmd.inv_instance = inv_inst_mapped;
                inv_cmd.inverter_enable = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&inv_cmd, DGN_INVERTER_CMD);
            }
        }
        break;

        case MINVERT0ENASTARTUP:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cmd_t inv_cmd;
                memset(&inv_cmd, GP_DEFAULT_VALUE, sizeof(inverter_cmd_t));
                inv_cmd.inv_instance = inv_inst_mapped;
                inv_cmd.inverter_enable_on_startup = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&inv_cmd, DGN_INVERTER_CMD);
            }
        }
        break;

        case MINVERT0LOADSENSE:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cmd_t inv_cmd;
                memset(&inv_cmd, GP_DEFAULT_VALUE, sizeof(inverter_cmd_t));
                inv_cmd.inv_instance = inv_inst_mapped;
                inv_cmd.load_sense_enable = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&inv_cmd, DGN_INVERTER_CMD);
            }
        }
        break;

        case MINVERT0SHDOWNVOLTMIN:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_1_t inv_cfg_cmd_1;
                memset(&inv_cfg_cmd_1, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_1_t));
                inv_cfg_cmd_1.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_1.shutdown_voltage_min = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_1, DGN_INVERTER_CFG_CMD_1);
            }
        }
        break;

        case MINVERT0LSENSEINT:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_1_t inv_cfg_cmd_1;
                memset(&inv_cfg_cmd_1, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_1_t));
                inv_cfg_cmd_1.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_1.load_sense_interval = (uint16_t)((frame->frame.set.value.int32 * RVC_SEC_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_SECOND]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_1, DGN_INVERTER_CFG_CMD_1);
            }
        }
        break;

        case MINVERT0LSENSEPOW:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_1_t inv_cfg_cmd_1;
                memset(&inv_cfg_cmd_1, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_1_t));
                inv_cfg_cmd_1.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_1.load_sense_pwr_threshold = (uint16_t)(frame->frame.set.value.int32 / Ddm2_unit_factor_list[DDM2_UNIT_WATT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_1, DGN_INVERTER_CFG_CMD_1);
            }
        }
        break;

        case MINVERT0SHDOWNVOLTMAX:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_2_t inv_cfg_cmd_2;
                memset(&inv_cfg_cmd_2, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_2_t));
                inv_cfg_cmd_2.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_2.shutdown_voltage_max = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_2, DGN_INVERTER_CFG_CMD_2);
            }
        }
        break;

        case MINVERT0WARNVOLTMAX:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_2_t inv_cfg_cmd_2;
                memset(&inv_cfg_cmd_2, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_2_t));
                inv_cfg_cmd_2.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_2.warn_voltage_max = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_2, DGN_INVERTER_CFG_CMD_2);
            }
        }
        break;

        case MINVERT0WARNVOLTMIN:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_2_t inv_cfg_cmd_2;
                memset(&inv_cfg_cmd_2, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_2_t));
                inv_cfg_cmd_2.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_2.warn_voltage_min = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_2, DGN_INVERTER_CFG_CMD_2);
            }
        }
        break;

        case MINVERT0STACK:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_3_t inv_cfg_cmd_3;
                memset(&inv_cfg_cmd_3, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_3_t));
                inv_cfg_cmd_3.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_3.stack_mode = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_3, DGN_INVERTER_CFG_CMD_3);
            }
        }
        break;

        case MINVERT0RECVOLT:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_3_t inv_cfg_cmd_3;
                memset(&inv_cfg_cmd_3, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_3_t));
                inv_cfg_cmd_3.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_3.shutdown_recovery_level = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_3, DGN_INVERTER_CFG_CMD_3);
            }
        }
        break;

        case MINVERT0OUTFREQ:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_4_t inv_cfg_cmd_4;
                memset(&inv_cfg_cmd_4, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_4_t));
                inv_cfg_cmd_4.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_4.output_frequency = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_4, DGN_INVERTER_CFG_CMD_4);
            }
        }
        break;

        case MINVERT0OUTVOLT:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_4_t inv_cfg_cmd_4;
                memset(&inv_cfg_cmd_4, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_4_t));
                inv_cfg_cmd_4.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_4.output_ac_voltage = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_4, DGN_INVERTER_CFG_CMD_4);
            }
        }
        break;

        case MINVERT0POWLIM:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                inverter_cfg_cmd_4_t inv_cfg_cmd_4;
                memset(&inv_cfg_cmd_4, GP_DEFAULT_VALUE, sizeof(inverter_cfg_cmd_4_t));
                inv_cfg_cmd_4.inv_instance = inv_inst_mapped;
                inv_cfg_cmd_4.power_limit = (uint16_t)(frame->frame.set.value.int32 / Ddm2_unit_factor_list[DDM2_UNIT_WATT]);
                send_raw_rvc_frame((uint8_t *)&inv_cfg_cmd_4, DGN_INVERTER_CFG_CMD_4);
            }
        }
        break;

        case MCHRG0ENA:
        {
            bool is_mchrg0ena_settable = false;
            SORTED_LIST_KEY_TYPE prod_inst_mapped;
            if (sorted_list_unique_get(&prod_inst_mapped, &mchrg_prod_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                char prod_manuf[PROD_DB_MAX_FIELD_SIZE] = {0};
                size_t prod_manuf_size = sizeof(prod_manuf);
                ProdDBReadCache(FIELD_MANUF, prod_inst_mapped, &prod_manuf, &prod_manuf_size);

                if ((prod_manuf_size != 0) && strstr(prod_manuf, "SILVERLEAF") != NULL)
                {
                    // Instances are the same for MCHRG and MINVERT for Inverter charger products
                    ddmw_item_t *minvert_enable_item = ddmw_find_item(&ddm_container, MINVERT0ENABLE | DDM2_PARAMETER_INSTANCE(DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter)));
                    if (minvert_enable_item != NULL)
                    {
                        int32_t minvert_enable = ddmw_get_i32(minvert_enable_item);
                        if (minvert_enable == 1)
                        {
                            is_mchrg0ena_settable = true;
                        }
                    }
                }
                else
                {
                    is_mchrg0ena_settable = true;
                }
            }

            if (is_mchrg0ena_settable)
            {
                SORTED_LIST_KEY_TYPE chrg_inst_mapped;
                int no_of_chrg_inst_mapped = 1;
                if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mchrg_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
                {
                    charger_cmd_t chrg_cmd;
                    memset(&chrg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cmd_t));
                    chrg_cmd.chrg_instance = chrg_inst_mapped;
                    chrg_cmd.status = (uint8_t)frame->frame.set.value.int32;
                    send_raw_rvc_frame((uint8_t *)&chrg_cmd, DGN_CHARGER_CMD);
                }
            }
        }
        break;

        case MCHRG0POWUP:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mchrg_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cmd_t chrg_cmd;
                memset(&chrg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cmd_t));
                chrg_cmd.chrg_instance = chrg_inst_mapped;
                chrg_cmd.default_state_power_up = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&chrg_cmd, DGN_CHARGER_CMD);
            }
        }
        break;

        case MCHRG0FORCE:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mchrg_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cmd_t chrg_cmd;
                memset(&chrg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cmd_t));
                chrg_cmd.chrg_instance = chrg_inst_mapped;
                chrg_cmd.force_charge = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&chrg_cmd, DGN_CHARGER_CMD);
            }
        }
        break;

        case MCHRG0MODE:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mchrg_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_t chrg_cfg_cmd;
                memset(&chrg_cfg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_t));
                chrg_cfg_cmd.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd.chrg_mode = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd, DGN_CHARGER_CFG_CMD);
            }
        }
        break;

        case MDCPROFILE0ALGO:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_t chrg_cfg_cmd;
                memset(&chrg_cfg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_t));
                chrg_cfg_cmd.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd.chrg_algorithm = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd, DGN_CHARGER_CFG_CMD);
            }
        }
        break;

        case MDCPROFILE0BANK:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_t chrg_cfg_cmd;
                memset(&chrg_cfg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_t));
                chrg_cfg_cmd.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd.bank_size = (uint16_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd, DGN_CHARGER_CFG_CMD);
            }
        }
        break;

        case MDCPROFILE0TYPE:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_t chrg_cfg_cmd;
                memset(&chrg_cfg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_t));
                chrg_cfg_cmd.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd.batt_type = (uint8_t)frame->frame.set.value.int32;
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd, DGN_CHARGER_CFG_CMD);
            }
        }
        break;

        case MDCPROFILE0MAXCURR:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_t chrg_cfg_cmd;
                memset(&chrg_cfg_cmd, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_t));
                chrg_cfg_cmd.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd.max_chrg_current = (uint8_t)(frame->frame.set.value.int32 / Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]);
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd, DGN_CHARGER_CFG_CMD);
            }
        }
        break;

        case MDCPROFILE0BVOLT:
        {
            bool is_mdcprofile0bvolt_settable = false;
            SORTED_LIST_KEY_TYPE prod_inst_mapped;
            if (sorted_list_unique_get(&prod_inst_mapped, &mdcprofile_prod_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                char prod_manuf[PROD_DB_MAX_FIELD_SIZE] = {0};
                size_t prod_manuf_size = sizeof(prod_manuf);
                ProdDBReadCache(FIELD_MANUF, prod_inst_mapped, &prod_manuf, &prod_manuf_size);

                if ((prod_manuf_size != 0) && strstr(prod_manuf, "SILVERLEAF") != NULL)
                {
                    // Instances are the same for MCHRG and MINVERT for Inverter charger products
                    ddmw_item_t *mdcprofile_type = ddmw_find_item(&ddm_container, MDCPROFILE0TYPE | DDM2_PARAMETER_INSTANCE(DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter)));
                    if (mdcprofile_type != NULL)
                    {
                        int32_t batt_type = ddmw_get_i32(mdcprofile_type);
                        if (batt_type == RVC4DCSRC0TYPE_CUSTOM)
                        {
                            is_mdcprofile0bvolt_settable = true;
                        }
                    }
                }
                else
                {
                    is_mdcprofile0bvolt_settable = true;
                }
            }
            if (is_mdcprofile0bvolt_settable)
            {
                SORTED_LIST_KEY_TYPE chrg_inst_mapped;
                int no_of_chrg_inst_mapped = 1;
                if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
                {
                    charger_cfg_cmd_3_t chrg_cfg_cmd_3;
                    memset(&chrg_cfg_cmd_3, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_3_t));
                    chrg_cfg_cmd_3.chrg_instance = chrg_inst_mapped;
                    chrg_cfg_cmd_3.absorption_voltage = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                    send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd_3, DGN_CHARGER_CFG_CMD_3);
                }
            }
        }
        break;

        case MDCPROFILE0FVOLT:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_3_t chrg_cfg_cmd_3;
                memset(&chrg_cfg_cmd_3, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_3_t));
                chrg_cfg_cmd_3.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd_3.float_voltage = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd_3, DGN_CHARGER_CFG_CMD_3);
            }
        }
        break;

        case MDCPROFILE0ABSDUR:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_4_t chrg_cfg_cmd_4;
                memset(&chrg_cfg_cmd_4, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_4_t));
                chrg_cfg_cmd_4.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd_4.absorption_time = (uint16_t)(frame->frame.set.value.int32 / Ddm2_unit_factor_list[DDM2_UNIT_MINUTE]);
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd_4, DGN_CHARGER_CFG_CMD_4);
            }
        }
        break;

        case MDCPROFILE0EQVOLT:
        {
            bool is_mdcprofile0eqvolt_settable = false;
            SORTED_LIST_KEY_TYPE prod_inst_mapped;
            if (sorted_list_unique_get(&prod_inst_mapped, &mdcprofile_prod_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                char prod_manuf[PROD_DB_MAX_FIELD_SIZE] = {0};
                size_t prod_manuf_size = sizeof(prod_manuf);
                ProdDBReadCache(FIELD_MANUF, prod_inst_mapped, &prod_manuf, &prod_manuf_size);

                if ((prod_manuf_size != 0) && strstr(prod_manuf, "SILVERLEAF") != NULL)
                {
                    // Instances are the same for MCHRG and MINVERT for Inverter charger products
                    ddmw_item_t *mdcprofile_type = ddmw_find_item(&ddm_container, MDCPROFILE0TYPE | DDM2_PARAMETER_INSTANCE(DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter)));
                    if (mdcprofile_type != NULL)
                    {
                        int32_t batt_type = ddmw_get_i32(mdcprofile_type);
                        if (batt_type == RVC4DCSRC0TYPE_CUSTOM)
                        {
                            is_mdcprofile0eqvolt_settable = true;
                        }
                    }
                }
                else
                {
                    is_mdcprofile0eqvolt_settable = true;
                }
            }
            if (is_mdcprofile0eqvolt_settable)
            {
                SORTED_LIST_KEY_TYPE chrg_inst_mapped;
                int no_of_chrg_inst_mapped = 1;
                if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
                {
                    charger_eq_cfg_cmd_t chrg_eq_cfg_cmd;
                    memset(&chrg_eq_cfg_cmd, GP_DEFAULT_VALUE, sizeof(charger_eq_cfg_cmd_t));
                    chrg_eq_cfg_cmd.chrg_instance = chrg_inst_mapped;
                    chrg_eq_cfg_cmd.eq_voltage = (uint16_t)((frame->frame.set.value.int32 * RVC_VOLT_GAIN_FACTOR) / Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
                    send_raw_rvc_frame((uint8_t *)&chrg_eq_cfg_cmd, DGN_CHARGER_EQ_CFG_CMD);
                }
            }
        }
        break;

        case MDCPROFILE0HVDISCONN:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped, prod_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                if (sorted_list_unique_get(&prod_inst_mapped, &mdcprofile_prod_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
                {
                    PROD0PROP_T prodprop;
                    size_t prodprop_size = sizeof(PROD0PROP_T);
                    uint8_t dest_addr = 0xFF;
                    ProdDBReadCache(FIELD_PROP, prod_inst_mapped, &prodprop, &prodprop_size);
                    if (prodprop_size != 0)
                    {
                        dest_addr = prodprop.addr;
                    }
                    prop_chrg_cfg_req_cmd_resp_t prop_chrg_cfg_req_cmd_resp;
                    memset(&prop_chrg_cfg_req_cmd_resp, GP_DEFAULT_VALUE, sizeof(prop_chrg_cfg_req_cmd_resp_t));
                    prop_chrg_cfg_req_cmd_resp.operation = PROP_CHRG_CFG_CMD_OP_CONFIG;
                    prop_chrg_cfg_req_cmd_resp.ovpr_limit_voltage = (uint8_t)(((frame->frame.set.value.int32) * PROP_CHRG_CFG_OVPR_VOLTAGE_SCALE_FACTOR) / (Ddm2_unit_factor_list[DDM2_UNIT_VOLT]));
                    process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&dest_addr, sizeof(dest_addr));
                    process_sub_param_immediately(&l_rvc_prop_data, (const void *)&prop_chrg_cfg_req_cmd_resp, sizeof(prop_chrg_cfg_req_cmd_resp_t));
                    process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
                }
            }
        }
        break;

        case MDCPROFILE0LVDISCONN:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped, prod_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                if (sorted_list_unique_get(&prod_inst_mapped, &mdcprofile_prod_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
                {
                    PROD0PROP_T prodprop;
                    size_t prodprop_size = sizeof(PROD0PROP_T);
                    uint8_t dest_addr = 0xFF;
                    ProdDBReadCache(FIELD_PROP, prod_inst_mapped, &prodprop, &prodprop_size);
                    if (prodprop_size != 0)
                    {
                        dest_addr = prodprop.addr;
                    }
                    prop_chrg_cfg_req_cmd_resp_t prop_chrg_cfg_req_cmd_resp;
                    memset(&prop_chrg_cfg_req_cmd_resp, GP_DEFAULT_VALUE, sizeof(prop_chrg_cfg_req_cmd_resp_t));
                    prop_chrg_cfg_req_cmd_resp.operation = PROP_CHRG_CFG_CMD_OP_CONFIG;
                    prop_chrg_cfg_req_cmd_resp.uvpr_limit_voltage = (uint8_t)(((frame->frame.set.value.int32) * PROP_CHRG_CFG_OVPR_VOLTAGE_SCALE_FACTOR) / (Ddm2_unit_factor_list[DDM2_UNIT_VOLT]));
                    process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&dest_addr, sizeof(dest_addr));
                    process_sub_param_immediately(&l_rvc_prop_data, (const void *)&prop_chrg_cfg_req_cmd_resp, sizeof(prop_chrg_cfg_req_cmd_resp_t));
                    process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
                }
            }
        }
        break;

        case MACIN0MAXCURR:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_macin_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                charger_cfg_cmd_2_t chrg_cfg_cmd_2;
                memset(&chrg_cfg_cmd_2, GP_DEFAULT_VALUE, sizeof(charger_cfg_cmd_2_t));
                chrg_cfg_cmd_2.chrg_instance = chrg_inst_mapped;
                chrg_cfg_cmd_2.shore_breaker_size = (uint8_t)(frame->frame.set.value.int32 / Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]);
                send_raw_rvc_frame((uint8_t *)&chrg_cfg_cmd_2, DGN_CHARGER_CFG_CMD_2);
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
        case MDCPROFILE0HVDISCONN:
        case MDCPROFILE0LVDISCONN:
        {
            SORTED_LIST_KEY_TYPE chrg_inst_mapped, prod_inst_mapped;
            int no_of_chrg_inst_mapped = 1;
            if (sorted_list_get_keys(&chrg_inst_mapped, &no_of_chrg_inst_mapped, &chrg_inst_mdcprofile_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                if (sorted_list_unique_get(&prod_inst_mapped, &mdcprofile_prod_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
                {
                    PROD0PROP_T prodprop;
                    size_t prodprop_size = sizeof(PROD0PROP_T);
                    uint8_t dest_addr = 0xFF;
                    ProdDBReadCache(FIELD_PROP, prod_inst_mapped, &prodprop, &prodprop_size);
                    if (prodprop_size != 0)
                    {
                        dest_addr = prodprop.addr;
                    }
                    prop_chrg_cfg_req_cmd_resp_t prop_chrg_cfg_req_cmd_resp;
                    memset(&prop_chrg_cfg_req_cmd_resp, GP_DEFAULT_VALUE, sizeof(prop_chrg_cfg_req_cmd_resp_t));
                    prop_chrg_cfg_req_cmd_resp.operation = PROP_CHRG_CFG_CMD_OP_REQUEST;
                    process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&dest_addr, sizeof(dest_addr));
                    process_sub_param_immediately(&l_rvc_prop_data, (const void *)&prop_chrg_cfg_req_cmd_resp, sizeof(prop_chrg_cfg_req_cmd_resp_t));
                    process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
                }
            }
        }
        break;

        case MINVERT0TRANSFTEMP:
        case MINVERT0FET1TEMP:
        {
            SORTED_LIST_KEY_TYPE inv_inst_mapped, prod_inst_mapped;
            int no_of_inv_inst_mapped = 1;
            if (sorted_list_get_keys(&inv_inst_mapped, &no_of_inv_inst_mapped, &inv_inst_minvert_ddm_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
            {
                if (sorted_list_unique_get(&prod_inst_mapped, &minvert_prod_inst_mapping_table, DDM2_PARAMETER_INSTANCE_FIELD(frame->frame.set.parameter), 0) == SORTED_LIST_OK)
                {
                    PROD0PROP_T prodprop;
                    size_t prodprop_size = sizeof(PROD0PROP_T);
                    uint8_t dest_addr = 0xFF;
                    ProdDBReadCache(FIELD_PROP, prod_inst_mapped, &prodprop, &prodprop_size);
                    if (prodprop_size != 0)
                    {
                        dest_addr = prodprop.addr;
                    }
                    prop_req_internal_inv_temps_t prop_req_internal_inv_temps;
                    memset(&prop_req_internal_inv_temps, GP_DEFAULT_VALUE, sizeof(prop_req_internal_inv_temps_t));
                    prop_req_internal_inv_temps.operation = PROP_INTERNAL_INV_TEMPS_OP_REQUEST;
                    process_sub_param_immediately(&l_rvc_prop_addr, (const void *)&dest_addr, sizeof(dest_addr));
                    process_sub_param_immediately(&l_rvc_prop_data, (const void *)&prop_req_internal_inv_temps, sizeof(prop_req_internal_inv_temps_t));
                    process_sub_param_immediately(&l_rvc_prop_sync, (const void *)&One, sizeof(One));
                }
            }
        }
        break;

        default:
            break;
        }
    }

    /* Process all parameters and publish those that have been updated */
    ddmw_process_publish(&ddm_container);
}

static int connector_ic_mgmt_init(void)
{
    ddmw_init(&ddm_container, &connector_ic_mgmt_service);
    ddmw_get_inventory(&ddm_container, inventory_callback);

    LIST_INIT(&l_rvcinvert);
    LIST_INIT(&l_rvcinvertcfg);
    LIST_INIT(&l_rvcinvertcfgtwo);
    LIST_INIT(&l_rvcinvertcfgthree);
    LIST_INIT(&l_rvcinvertcfgfour);
    LIST_INIT(&l_rvcinvertdc);
    LIST_INIT(&l_rvcinverttemp);
    LIST_INIT(&l_rvcinverttemptwo);
    LIST_INIT(&l_rvcinvertac);
    LIST_INIT(&l_rvcinvertactwo);
    LIST_INIT(&l_rvcinvertacthree);

    LIST_INIT(&l_rvcchrg);
    LIST_INIT(&l_rvcchrgtwo);
    LIST_INIT(&l_rvcchrgcfg);
    LIST_INIT(&l_rvcchrgac);
    LIST_INIT(&l_rvcchrgacthree);
    LIST_INIT(&l_rvcchrgcfgtwo);
    LIST_INIT(&l_rvcchrgacfaultcfg);
    LIST_INIT(&l_rvcdcsrc);
    LIST_INIT(&l_rvcdcsrctwo);
    LIST_INIT(&l_rvcchrgcfgthree);
    LIST_INIT(&l_rvcchrgcfgfour);
    LIST_INIT(&l_rvcchrgeqcfg);

    LIST_INIT(&l_minvert);
    LIST_INIT(&l_macout);
    LIST_INIT(&l_mchrg);
    LIST_INIT(&l_macin);
    LIST_INIT(&l_mdcprofile);
    LIST_INIT(&l_prod);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_minvert_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_minvert_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(inv_inst_minvert_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(minvert_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_macout_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_macout_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(inv_inst_macout_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(macout_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mchrg_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mchrg_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(chrg_inst_mchrg_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(mchrg_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_macin_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_macin_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(chrg_inst_macin_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(macin_prod_inst_mapping_table);

    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_param_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(rvc_mdcprofile_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(chrg_inst_mdcprofile_ddm_inst_mapping_table);
    INIT_SORTED_LIST_EXTRAM_PTR(mdcprofile_prod_inst_mapping_table);

    for (unsigned int i = 0; i < ELEMENTS(rvc_minvert_params); i++)
    {
        TRUE_CHECK(sorted_list_unique_add(&rvc_minvert_param_mapping_table, rvc_minvert_params[i].sub_ddm2_param, rvc_minvert_params[i].owned_ddm2_param) == SORTED_LIST_ENTRY_INSERTED);
    }
    for (unsigned int i = 0; i < ELEMENTS(rvc_macout_params); i++)
    {
        TRUE_CHECK(sorted_list_unique_add(&rvc_macout_param_mapping_table, rvc_macout_params[i].sub_ddm2_param, rvc_macout_params[i].owned_ddm2_param) == SORTED_LIST_ENTRY_INSERTED);
    }
    for (unsigned int i = 0; i < ELEMENTS(rvc_mchrg_params); i++)
    {
        TRUE_CHECK(sorted_list_unique_add(&rvc_mchrg_param_mapping_table, rvc_mchrg_params[i].sub_ddm2_param, rvc_mchrg_params[i].owned_ddm2_param) == SORTED_LIST_ENTRY_INSERTED);
    }
    for (unsigned int i = 0; i < ELEMENTS(rvc_macin_params); i++)
    {
        TRUE_CHECK(sorted_list_unique_add(&rvc_macin_param_mapping_table, rvc_macin_params[i].sub_ddm2_param, rvc_macin_params[i].owned_ddm2_param) == SORTED_LIST_ENTRY_INSERTED);
    }
    for (unsigned int i = 0; i < ELEMENTS(rvc_mdcprofile_params); i++)
    {
        TRUE_CHECK(sorted_list_unique_add(&rvc_mdcprofile_param_mapping_table, rvc_mdcprofile_params[i].sub_ddm2_param, rvc_mdcprofile_params[i].owned_ddm2_param) == SORTED_LIST_ENTRY_INSERTED);
    }

    minvert_linked_idx = 0;
    macout_linked_idx = 0;
    mchrg_linked_idx = 0;
    macin_linked_idx = 0;
    mdcprofile_linked_idx = 0;

    memset(mdcprofile_inst_addr_map, 0, sizeof(mdcprofile_inst_addr_map));
    memset(minvert_inst_addr_map, 0, sizeof(minvert_inst_addr_map));

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
        case RVCMGNT0:
        case RVCPROP0:
            start_subscriptions_to_rvc_params(DDM2_PARAMETER_CLASS_INSTANCE(parameter));
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

                    /* Remove all MINVERT instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE minvert_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_minvert = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(minvert_instances, &no_of_minvert, &minvert_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        LOG(D, "Found %d MINVERT instances linked to PROD%d", no_of_minvert, prod_instance);
                        for (int i = 0; i < no_of_minvert; i++)
                        {
                            remove_minvert_instance((uint8_t)minvert_instances[i], prod_instance);
                        }
                    }

                    /* Remove all MACOUT instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE macout_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_macout = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(macout_instances, &no_of_macout, &macout_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_macout; i++)
                        {
                            remove_macout_instance((uint8_t)macout_instances[i], prod_instance);
                        }
                    }

                    /* Remove all MCHRG instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE mchrg_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mchrg = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(mchrg_instances, &no_of_mchrg, &mchrg_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mchrg; i++)
                        {
                            remove_mchrg_instance((uint8_t)mchrg_instances[i], prod_instance);
                        }
                    }

                    /* Remove all MACIN instances linked to this PROD */
                    SORTED_LIST_KEY_TYPE macin_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_macin = MAX_NO_OF_LINKED_RVC_CLASSES;
                    if (sorted_list_get_keys(macin_instances, &no_of_macin, &macin_prod_inst_mapping_table, prod_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_macin; i++)
                        {
                            remove_macin_instance((uint8_t)macin_instances[i], prod_instance);
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
            stop_subscriptions_to_rvc_params(DDM2_PARAMETER_CLASS_INSTANCE(parameter));
            break;
        default:
            break;
        }
    }
}

static void manage_minvert_classes(uint32_t class_instance, uint8_t inv_inst, uint8_t prod_instance)
{
    uint32_t value;
    /* If there is no MINVERT DDM instance related to a specific DC instance */
    if (sorted_list_unique_get(&value, &inv_inst_minvert_ddm_inst_mapping_table, inv_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *minvert_ddm_inst;

        /* Create a new MINVERT class descriptor*/
        minvert_ddm_inst = ddm_class_desc_create(MINVERT0);
        if (minvert_ddm_inst != NULL)
        {
            /* Register new MINVERT instance in the system */
            int32_t minvert_ddm_instance = ddmw_register(&ddm_container, MINVERT0);
            if (minvert_ddm_instance != -1)
            {
                LOG(D, "Created MINVERT instance %d for INV instance %d", minvert_ddm_instance, inv_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_minvert_ddm_inst_mapping_table, class_instance, minvert_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&inv_inst_minvert_ddm_inst_mapping_table, inv_inst, minvert_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Initialize the MINVERT class descriptor */
                ddm_class_desc_init(minvert_ddm_instance, MINVERT0, minvert_ddm_inst);
                /* Add all the items except AVL in the DDMW container and update specific items */
                for (uint32_t i = 1; i < minvert_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &minvert_ddm_inst->params_items[i].ddm2_item, minvert_ddm_inst->params_items[i].ddm2_param, minvert_ddm_instance);
                    if (DDM2_PARAMETER_BASE_INSTANCE(minvert_ddm_inst->params_items[i].ddm2_param) == MINVERT0INST)
                    {
                        ddmw_set_i32(&minvert_ddm_inst->params_items[i].ddm2_item, inv_inst);
                    }
                    else if (DDM2_PARAMETER_BASE_INSTANCE(minvert_ddm_inst->params_items[i].ddm2_param) == MINVERT0LINKED)
                    {
                        minvert_linked_idx = i;
                        SORTED_LIST_KEY_TYPE minvert_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                        /* Find all the RVC instances that are linked to the specific MINVERT DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(minvert_linked, &no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, minvert_ddm_instance, 0) == SORTED_LIST_OK);

                        ddmw_set_data(&minvert_ddm_inst->params_items[i].ddm2_item, minvert_linked, no_of_minvert_linked * sizeof(minvert_linked[0]));
                    }
                }
                /* Insert the MINVERT class descriptor in the specific MINVERT list */
                ddm_class_desc_insert(minvert_ddm_inst, &l_minvert);

                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MINVERT instance %d to PROD%uCLIST", minvert_ddm_instance, prod_instance);
                        uint32_t minvert_instance = MINVERT0 | DDM2_PARAMETER_INSTANCE(minvert_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &minvert_instance, sizeof(minvert_instance), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                /* Link MINVERT DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&minvert_prod_inst_mapping_table, minvert_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Add MINVERT mapping */
                add_minvert_mapping(minvert_ddm_instance, prod_instance);

                /* Check if there are existing MCHRG and MACOUT classes for this inverter instance */
                SORTED_LIST_VALUE_TYPE macout_ddm_instance;
                if (sorted_list_unique_get(&macout_ddm_instance, &inv_inst_macout_ddm_inst_mapping_table, inv_inst, 0) == SORTED_LIST_OK)
                {
                    /* MACOUT exists, check if it's already linked */
                    bool macout_already_linked = false;
                    SORTED_LIST_KEY_TYPE minvert_linked_check[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_minvert_linked_check = MAX_NO_OF_LINKED_RVC_CLASSES;

                    if (sorted_list_get_keys(minvert_linked_check, &no_of_minvert_linked_check, &rvc_minvert_ddm_inst_mapping_table, minvert_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_minvert_linked_check; i++)
                        {
                            if (minvert_linked_check[i] == (MACOUT0 | DDM2_PARAMETER_INSTANCE(macout_ddm_instance)))
                            {
                                macout_already_linked = true;
                                break;
                            }
                        }
                    }

                    if (!macout_already_linked)
                    {
                        LOG(D, "Linking existing MACOUT instance %d to MINVERT instance %d", macout_ddm_instance, minvert_ddm_instance);
                        TRUE_CHECK(sorted_list_unique_add(&rvc_minvert_ddm_inst_mapping_table, (MACOUT0 | DDM2_PARAMETER_INSTANCE(macout_ddm_instance)), minvert_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                        /* Update MINVERT linked list parameter */
                        SORTED_LIST_KEY_TYPE updated_minvert_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int updated_no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        if (sorted_list_get_keys(updated_minvert_linked, &updated_no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, minvert_ddm_instance, 0) == SORTED_LIST_OK)
                        {
                            ddmw_set_data(&minvert_ddm_inst->params_items[minvert_linked_idx].ddm2_item, updated_minvert_linked, updated_no_of_minvert_linked * sizeof(updated_minvert_linked[0]));
                        }
                    }
                }

                SORTED_LIST_VALUE_TYPE mchrg_ddm_instance;
                // inverter and charger instance must be the same in case of inverter/charger combo units
                if (sorted_list_unique_get(&mchrg_ddm_instance, &chrg_inst_mchrg_ddm_inst_mapping_table, inv_inst, 0) == SORTED_LIST_OK)
                {
                    /* MCHRG exists, check if it's already linked */
                    bool mchrg_already_linked = false;
                    SORTED_LIST_KEY_TYPE minvert_linked_check[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_minvert_linked_check = MAX_NO_OF_LINKED_RVC_CLASSES;

                    if (sorted_list_get_keys(minvert_linked_check, &no_of_minvert_linked_check, &rvc_minvert_ddm_inst_mapping_table, minvert_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_minvert_linked_check; i++)
                        {
                            if (minvert_linked_check[i] == (MCHRG0 | DDM2_PARAMETER_INSTANCE(mchrg_ddm_instance)))
                            {
                                mchrg_already_linked = true;
                                break;
                            }
                        }
                    }

                    if (!mchrg_already_linked)
                    {
                        LOG(D, "Linking existing MCHRG instance %d to MINVERT instance %d", mchrg_ddm_instance, minvert_ddm_instance);
                        TRUE_CHECK(sorted_list_unique_add(&rvc_minvert_ddm_inst_mapping_table, (MCHRG0 | DDM2_PARAMETER_INSTANCE(mchrg_ddm_instance)), minvert_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                        /* Update MINVERT linked list parameter */
                        SORTED_LIST_KEY_TYPE updated_minvert_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int updated_no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        if (sorted_list_get_keys(updated_minvert_linked, &updated_no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, minvert_ddm_instance, 0) == SORTED_LIST_OK)
                        {
                            ddmw_set_data(&minvert_ddm_inst->params_items[minvert_linked_idx].ddm2_item, updated_minvert_linked, updated_no_of_minvert_linked * sizeof(updated_minvert_linked[0]));
                        }
                    }
                }
            }
            else
            {
                ddm_class_desc_delete(minvert_ddm_inst);
            }
        }
        else
        {
            LOG(E, "MINVERT0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MINVERT DDM instance related to a specific INV instance
            check whether the RVC<X> is linked to the MINVERT DDM instance and if not, link it
            (update the corresponding MINVERT parameter) */

        ddm_class_desc_t *minvert_ddm_inst;
        SORTED_LIST_KEY_TYPE minvert_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(minvert_linked, &no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

        for (int i = 0; i < no_of_minvert_linked; i++)
        {
            if (minvert_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            minvert_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_minvert, (uint8_t)value);
            if (minvert_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MINVERT instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_minvert_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);
                no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                TRUE_CHECK(sorted_list_get_keys(minvert_linked, &no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
                ddmw_set_data(&minvert_ddm_inst->params_items[minvert_linked_idx].ddm2_item, minvert_linked, no_of_minvert_linked * sizeof(minvert_linked[0]));
            }
        }
    }
}

static void manage_macout_classes(uint32_t class_instance, uint8_t inv_inst, uint8_t prod_instance, uint8_t minvert_ddm_instance)
{
    uint32_t value;
    int32_t macout_ddm_instance = INVALID_DDM_INSTANCE;
    /* If there is no MACOUT DDM instance related to a specific DC instance */
    if (sorted_list_unique_get(&value, &inv_inst_macout_ddm_inst_mapping_table, inv_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *macout_ddm_inst;

        /* Create a new MACOUT class descriptor*/
        macout_ddm_inst = ddm_class_desc_create(MACOUT0);
        if (macout_ddm_inst != NULL)
        {
            /* Register new MACOUT instance in the system */
            macout_ddm_instance = ddmw_register(&ddm_container, MACOUT0);

            if (macout_ddm_instance != -1)
            {
                LOG(D, "Created MACOUT instance %d for INV instance %d", macout_ddm_instance, inv_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_macout_ddm_inst_mapping_table, class_instance, macout_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&inv_inst_macout_ddm_inst_mapping_table, inv_inst, macout_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                /* Initialize the MACOUT class descriptor */
                ddm_class_desc_init(macout_ddm_instance, MACOUT0, macout_ddm_inst);

                /* Add all the items except AVL in the DDMW container and update specific items */
                for (uint32_t i = 1; i < macout_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &macout_ddm_inst->params_items[i].ddm2_item, macout_ddm_inst->params_items[i].ddm2_param, macout_ddm_instance);

                    if (DDM2_PARAMETER_BASE_INSTANCE(macout_ddm_inst->params_items[i].ddm2_param) == MACOUT0LINKED)
                    {
                        macout_linked_idx = i;
                        SORTED_LIST_KEY_TYPE macout_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_macout_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                        /* Find all the RVC instances that are linked to the specific MACOUT DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(macout_linked, &no_of_macout_linked, &rvc_macout_ddm_inst_mapping_table, macout_ddm_instance, 0) == SORTED_LIST_OK);
                        ddmw_set_data(&macout_ddm_inst->params_items[i].ddm2_item, macout_linked, no_of_macout_linked * sizeof(macout_linked[0]));
                    }
                }
                /* Insert the MACOUT class descriptor in the specific MACOUT list */
                ddm_class_desc_insert(macout_ddm_inst, &l_macout);

                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MACOUT instance %d to PROD%uCLIST", macout_ddm_instance, prod_instance);
                        uint32_t macout_instance = MACOUT0 | DDM2_PARAMETER_INSTANCE(macout_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &macout_instance, sizeof(macout_instance), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                /* Link MACOUT DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&macout_prod_inst_mapping_table, macout_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);
            }
            else
            {
                ddm_class_desc_delete(macout_ddm_inst);
            }
        }
        else
        {
            LOG(E, "MACOUT0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MACOUT DDM instance related to a specific DC instance
            check whether the RVCDCSRC<X> is linked to the MACOUT DDM instance and if not, link it */
        ddm_class_desc_t *macout_ddm_inst;
        SORTED_LIST_KEY_TYPE macout_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_macout_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(macout_linked, &no_of_macout_linked, &rvc_macout_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

        for (int i = 0; i < no_of_macout_linked; i++)
        {
            if (macout_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            macout_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_macout, (uint8_t)value);
            if (macout_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MACOUT instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_macout_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);
                no_of_macout_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                TRUE_CHECK(sorted_list_get_keys(macout_linked, &no_of_macout_linked, &rvc_macout_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
                ddmw_set_data(&macout_ddm_inst->params_items[macout_linked_idx].ddm2_item, macout_linked, no_of_macout_linked * sizeof(macout_linked[0]));
            }
        }
    }

    if (minvert_ddm_instance != 0xFF)
    {
        SORTED_LIST_VALUE_TYPE temp_macout_ddm_instance = macout_ddm_instance;
        if (sorted_list_unique_get(&temp_macout_ddm_instance, &inv_inst_macout_ddm_inst_mapping_table, inv_inst, 0) == SORTED_LIST_OK)
        {
            LOG(D, "Linking existing MACOUT instance %d to MINVERT instance %d", temp_macout_ddm_instance, minvert_ddm_instance);
            /* Add the MACOUT class instance to the list of linked instances to MINVERT */
            if (sorted_list_unique_add(&rvc_minvert_ddm_inst_mapping_table, (MACOUT0 | DDM2_PARAMETER_INSTANCE(temp_macout_ddm_instance)), minvert_ddm_instance) == SORTED_LIST_ENTRY_INSERTED)
            {
                /* Get all previously linked RVC instances */
                SORTED_LIST_KEY_TYPE minvert_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                int no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                if (sorted_list_get_keys(minvert_linked, &no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, minvert_ddm_instance, 0) == SORTED_LIST_OK)
                {
                    /* Prepare a buffer to hold all previous RVC instances and the macout class instance */
                    uint32_t linked_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int idx = 0;
                    /* Copy existing instances */
                    for (int i = 0; i < no_of_minvert_linked; i++)
                    {
                        linked_instances[idx++] = minvert_linked[i];
                        LOG(D, "Adding existing linked instance %x to MINVERT instance %d", minvert_linked[i], minvert_ddm_instance);
                    }
                    ddm_class_desc_t *minvert_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_minvert, minvert_ddm_instance);
                    if (minvert_ddm_inst != NULL)
                    {
                        ddmw_set_data(&minvert_ddm_inst->params_items[minvert_linked_idx].ddm2_item, linked_instances, idx * sizeof(linked_instances[0]));
                    }
                }
            }
        }
    }
}

static void manage_mchrg_classes(uint32_t class_instance, uint8_t chrg_inst, uint8_t prod_instance, uint8_t minvert_ddm_instance)
{
    uint32_t value;
    int32_t mchrg_ddm_instance = INVALID_DDM_INSTANCE;
    /* If there is no MCHRG DDM instance related to a specific charger instance */
    if (sorted_list_unique_get(&value, &chrg_inst_mchrg_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *mchrg_ddm_inst;

        /* Create a new MCHRG class descriptor*/
        mchrg_ddm_inst = ddm_class_desc_create(MCHRG0);
        if (mchrg_ddm_inst != NULL)
        {
            /* Register new MCHRG instance in the system */
            mchrg_ddm_instance = ddmw_register(&ddm_container, MCHRG0);
            if (mchrg_ddm_instance != -1)
            {
                LOG(D, "Created MCHRG instance %d for CHRG instance %d", mchrg_ddm_instance, chrg_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mchrg_ddm_inst_mapping_table, class_instance, mchrg_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&chrg_inst_mchrg_ddm_inst_mapping_table, chrg_inst, mchrg_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                /* Initialize the MCHRG class descriptor */
                ddm_class_desc_init(mchrg_ddm_instance, MCHRG0, mchrg_ddm_inst);

                /* Add all the items except AVL in the DDMW container and update specific items */
                for (uint32_t i = 1; i < mchrg_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &mchrg_ddm_inst->params_items[i].ddm2_item, mchrg_ddm_inst->params_items[i].ddm2_param, mchrg_ddm_instance);

                    if (DDM2_PARAMETER_BASE_INSTANCE(mchrg_ddm_inst->params_items[i].ddm2_param) == MCHRG0INST)
                    {
                        ddmw_set_i32(&mchrg_ddm_inst->params_items[i].ddm2_item, chrg_inst);
                    }
                    else if (DDM2_PARAMETER_BASE_INSTANCE(mchrg_ddm_inst->params_items[i].ddm2_param) == MCHRG0LINKED)
                    {
                        mchrg_linked_idx = i;
                        SORTED_LIST_KEY_TYPE mchrg_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        /* Find all the RVC instances that are linked to the specific MCHRG DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(mchrg_linked, &no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, mchrg_ddm_instance, 0) == SORTED_LIST_OK);
                        ddmw_set_data(&mchrg_ddm_inst->params_items[i].ddm2_item, mchrg_linked, no_of_mchrg_linked * sizeof(mchrg_linked[0]));
                    }
                }
                /* Insert the MCHRG class descriptor in the specific MCHRG list */
                ddm_class_desc_insert(mchrg_ddm_inst, &l_mchrg);

                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MCHRG instance %d to PROD%uCLIST", mchrg_ddm_instance, prod_instance);
                        uint32_t mchrg_instance = MCHRG0 | DDM2_PARAMETER_INSTANCE(mchrg_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &mchrg_instance, sizeof(mchrg_instance), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                /* Link MCHRG DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&mchrg_prod_inst_mapping_table, mchrg_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Check if there are existing MACIN and MDCPROFILE classes for this charger instance */
                SORTED_LIST_VALUE_TYPE macin_ddm_instance;
                if (sorted_list_unique_get(&macin_ddm_instance, &chrg_inst_macin_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_OK)
                {
                    /* MACIN exists, check if it's already linked */
                    bool macin_already_linked = false;
                    SORTED_LIST_KEY_TYPE mchrg_linked_check[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mchrg_linked_check = MAX_NO_OF_LINKED_RVC_CLASSES;

                    if (sorted_list_get_keys(mchrg_linked_check, &no_of_mchrg_linked_check, &rvc_mchrg_ddm_inst_mapping_table, mchrg_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mchrg_linked_check; i++)
                        {
                            if (mchrg_linked_check[i] == (MACIN0 | DDM2_PARAMETER_INSTANCE(macin_ddm_instance)))
                            {
                                macin_already_linked = true;
                                break;
                            }
                        }
                    }

                    if (!macin_already_linked)
                    {
                        LOG(D, "Linking existing MACIN instance %d to MCHRG instance %d", macin_ddm_instance, mchrg_ddm_instance);
                        TRUE_CHECK(sorted_list_unique_add(&rvc_mchrg_ddm_inst_mapping_table, (MACIN0 | DDM2_PARAMETER_INSTANCE(macin_ddm_instance)), mchrg_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                        /* Update MCHRG linked list parameter */
                        SORTED_LIST_KEY_TYPE updated_mchrg_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int updated_no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        if (sorted_list_get_keys(updated_mchrg_linked, &updated_no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, mchrg_ddm_instance, 0) == SORTED_LIST_OK)
                        {
                            ddmw_set_data(&mchrg_ddm_inst->params_items[mchrg_linked_idx].ddm2_item, updated_mchrg_linked, updated_no_of_mchrg_linked * sizeof(updated_mchrg_linked[0]));
                        }
                    }
                }

                SORTED_LIST_VALUE_TYPE mdcprofile_ddm_instance;
                if (sorted_list_unique_get(&mdcprofile_ddm_instance, &chrg_inst_mdcprofile_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_OK)
                {
                    /* MDCPROFILE exists, check if it's already linked */
                    bool mdcprofile_already_linked = false;
                    SORTED_LIST_KEY_TYPE mchrg_linked_check[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int no_of_mchrg_linked_check = MAX_NO_OF_LINKED_RVC_CLASSES;

                    if (sorted_list_get_keys(mchrg_linked_check, &no_of_mchrg_linked_check, &rvc_mchrg_ddm_inst_mapping_table, mchrg_ddm_instance, 0) == SORTED_LIST_OK)
                    {
                        for (int i = 0; i < no_of_mchrg_linked_check; i++)
                        {
                            if (mchrg_linked_check[i] == (MCHRG0 | DDM2_PARAMETER_INSTANCE(mchrg_ddm_instance)))
                            {
                                mdcprofile_already_linked = true;
                                break;
                            }
                        }
                    }

                    if (!mdcprofile_already_linked)
                    {
                        LOG(D, "Linking existing MDCPROFILE instance %d to MCHRG instance %d", mdcprofile_ddm_instance, mchrg_ddm_instance);
                        TRUE_CHECK(sorted_list_unique_add(&rvc_mchrg_ddm_inst_mapping_table, (MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(mchrg_ddm_instance)), mchrg_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);

                        /* Update MCHRG linked list parameter */
                        SORTED_LIST_KEY_TYPE updated_mchrg_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int updated_no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        if (sorted_list_get_keys(updated_mchrg_linked, &updated_no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, mchrg_ddm_instance, 0) == SORTED_LIST_OK)
                        {
                            ddmw_set_data(&mchrg_ddm_inst->params_items[mchrg_linked_idx].ddm2_item, updated_mchrg_linked, updated_no_of_mchrg_linked * sizeof(updated_mchrg_linked[0]));
                        }
                    }
                }
            }
            else
            {
                ddm_class_desc_delete(mchrg_ddm_inst);
            }
        }
        else
        {
            LOG(E, "MCHRG0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MCHRG DDM instance related to a specific DC instance
            check whether the RVC is linked to the MCHRG DDM instance and if not, link it */
        ddm_class_desc_t *mchrg_ddm_inst;
        SORTED_LIST_KEY_TYPE mchrg_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(mchrg_linked, &no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);

        for (int i = 0; i < no_of_mchrg_linked; i++)
        {
            if (mchrg_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            mchrg_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mchrg, (uint8_t)value);
            if (mchrg_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MCHRG instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_mchrg_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);
                no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                TRUE_CHECK(sorted_list_get_keys(mchrg_linked, &no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
                ddmw_set_data(&mchrg_ddm_inst->params_items[mchrg_linked_idx].ddm2_item, mchrg_linked, no_of_mchrg_linked * sizeof(mchrg_linked[0]));
            }
        }
    }

    if (minvert_ddm_instance != 0xFF)
    {
        SORTED_LIST_VALUE_TYPE temp_mchrg_ddm_instance;
        if (sorted_list_unique_get(&temp_mchrg_ddm_instance, &chrg_inst_mchrg_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_OK)
        {
            LOG(D, "Linking existing MCHRG instance %d to MINVERT instance %d", temp_mchrg_ddm_instance, minvert_ddm_instance);
            /* Add the MCHRG class instance to the list of linked instances to MINVERT */
            if (sorted_list_unique_add(&rvc_minvert_ddm_inst_mapping_table, (MCHRG0 | DDM2_PARAMETER_INSTANCE(temp_mchrg_ddm_instance)), minvert_ddm_instance) == SORTED_LIST_ENTRY_INSERTED)
            {
                /* Get all previously linked RVC instances */
                SORTED_LIST_KEY_TYPE minvert_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                int no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                if (sorted_list_get_keys(minvert_linked, &no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, minvert_ddm_instance, 0) == SORTED_LIST_OK)
                {
                    /* Prepare a buffer to hold all previous RVC instances and the macout class instance */
                    uint32_t linked_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int idx = 0;
                    /* Copy existing instances */
                    for (int i = 0; i < no_of_minvert_linked; i++)
                    {
                        linked_instances[idx++] = minvert_linked[i];
                        LOG(D, "Adding existing linked instance %x to MINVERT instance %d", minvert_linked[i], minvert_ddm_instance);
                    }
                    ddm_class_desc_t *minvert_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_minvert, minvert_ddm_instance);
                    if (minvert_ddm_inst != NULL)
                    {
                        ddmw_set_data(&minvert_ddm_inst->params_items[minvert_linked_idx].ddm2_item, linked_instances, idx * sizeof(linked_instances[0]));
                    }
                }
            }
        }
    }
}

static void manage_macin_classes(uint32_t class_instance, uint8_t chrg_inst, uint8_t prod_instance, uint8_t mchrg_ddm_instance)
{
    uint32_t value;
    int32_t macin_ddm_instance = INVALID_DDM_INSTANCE;
    /* If there is no MACIN DDM instance related to a specific charger instance */
    if (sorted_list_unique_get(&value, &chrg_inst_macin_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *macin_ddm_inst;

        /* Create a new MACIN class descriptor*/
        macin_ddm_inst = ddm_class_desc_create(MACIN0);
        if (macin_ddm_inst != NULL)
        {
            /* Register new MACIN instance in the system */
            macin_ddm_instance = ddmw_register(&ddm_container, MACIN0);
            if (macin_ddm_instance != -1)
            {
                LOG(D, "Created MACIN instance %d for CHRG instance %d", macin_ddm_instance, chrg_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_macin_ddm_inst_mapping_table, class_instance, macin_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&chrg_inst_macin_ddm_inst_mapping_table, chrg_inst, macin_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                /* Initialize the MACIN class descriptor */
                ddm_class_desc_init(macin_ddm_instance, MACIN0, macin_ddm_inst);

                /* Add all the items except AVL in the DDMW container and update specific items */
                for (uint32_t i = 1; i < macin_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &macin_ddm_inst->params_items[i].ddm2_item, macin_ddm_inst->params_items[i].ddm2_param, macin_ddm_instance);
                    if (DDM2_PARAMETER_BASE_INSTANCE(macin_ddm_inst->params_items[i].ddm2_param) == MACIN0LINKED)
                    {
                        macin_linked_idx = i;
                        SORTED_LIST_KEY_TYPE macin_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_macin_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        /* Find all the RVC instances that are linked to the specific MACIN DDM instance */
                        TRUE_CHECK(sorted_list_get_keys(macin_linked, &no_of_macin_linked, &rvc_macin_ddm_inst_mapping_table, macin_ddm_instance, 0) == SORTED_LIST_OK);
                        ddmw_set_data(&macin_ddm_inst->params_items[i].ddm2_item, macin_linked, no_of_macin_linked * sizeof(macin_linked[0]));
                    }
                }
                /* Insert the MACIN class descriptor in the specific MACIN list */
                ddm_class_desc_insert(macin_ddm_inst, &l_macin);
                /* Find the associated prod instance in order to update the PROD<X>CLIST parameter*/
                ddm_class_desc_t *prod_inst = NULL;
                prod_inst = ddm_class_desc_find_by_ddm_instance(&l_prod, prod_instance);
                if (prod_inst != NULL)
                {
                    ddmw_item_t *prod_clist = NULL;
                    prod_clist = ddmw_find_item(&ddm_container, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance));
                    if (prod_clist != NULL)
                    {
                        LOG(D, "Adding MACIN instance %d to PROD%uCLIST", macin_ddm_instance, prod_instance);
                        uint32_t macin_instance = MACIN0 | DDM2_PARAMETER_INSTANCE(macin_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &macin_instance, sizeof(macin_instance), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                /* Link MACIN DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&macin_prod_inst_mapping_table, macin_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);
            }
            else
            {
                ddm_class_desc_delete(macin_ddm_inst);
            }
        }
        else
        {
            LOG(E, "MACIN0 class descriptor cannot be allocated.");
        }
    }
    else
    {
        /* If there is an MACIN DDM instance related to a specific CHRG instance
            check whether the RVC is linked to the MACIN DDM instance and if not, link it */
        ddm_class_desc_t *macin_ddm_inst;
        SORTED_LIST_KEY_TYPE macin_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_macin_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        bool is_linked_class_found = false;

        TRUE_CHECK(sorted_list_get_keys(macin_linked, &no_of_macin_linked, &rvc_macin_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
        for (int i = 0; i < no_of_macin_linked; i++)
        {
            if (macin_linked[i] == class_instance)
            {
                is_linked_class_found = true;
            }
        }

        if (!is_linked_class_found)
        {
            macin_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_macin, (uint8_t)value);
            if (macin_ddm_inst != NULL)
            {
                LOG(D, "Linking RVC class instance %x to MACIN instance %d", class_instance, value);
                TRUE_CHECK(sorted_list_unique_add(&rvc_macin_ddm_inst_mapping_table, class_instance, value) == SORTED_LIST_ENTRY_INSERTED);
                no_of_macin_linked = MAX_NO_OF_LINKED_RVC_CLASSES;

                TRUE_CHECK(sorted_list_get_keys(macin_linked, &no_of_macin_linked, &rvc_macin_ddm_inst_mapping_table, value, 0) == SORTED_LIST_OK);
                ddmw_set_data(&macin_ddm_inst->params_items[macin_linked_idx].ddm2_item, macin_linked, no_of_macin_linked * sizeof(macin_linked[0]));
            }
        }
    }

    if (mchrg_ddm_instance != 0xFF)
    {
        SORTED_LIST_VALUE_TYPE temp_macin_ddm_instance;
        if (sorted_list_unique_get(&temp_macin_ddm_instance, &chrg_inst_macin_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_OK)
        {
            LOG(D, "Linking existing MACIN instance %d to MCHRG instance %d", temp_macin_ddm_instance, mchrg_ddm_instance);
            /* Add the MACIN class instance to the list of linked instances to MCHRG */
            if (sorted_list_unique_add(&rvc_mchrg_ddm_inst_mapping_table, (MACIN0 | DDM2_PARAMETER_INSTANCE(temp_macin_ddm_instance)), mchrg_ddm_instance) == SORTED_LIST_ENTRY_INSERTED)
            {
                /* Get all previously linked RVC instances */
                SORTED_LIST_KEY_TYPE mchrg_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                int no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                if (sorted_list_get_keys(mchrg_linked, &no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, mchrg_ddm_instance, 0) == SORTED_LIST_OK)
                {
                    /* Prepare a buffer to hold all previous RVC instances and the macout class instance */
                    uint32_t linked_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int idx = 0;
                    /* Copy existing instances */
                    for (int i = 0; i < no_of_mchrg_linked; i++)
                    {
                        linked_instances[idx++] = mchrg_linked[i];
                        LOG(D, "Adding existing linked instance %x to MCHRG instance %d", mchrg_linked[i], mchrg_ddm_instance);
                    }
                    ddm_class_desc_t *mchrg_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mchrg, mchrg_ddm_instance);
                    if (mchrg_ddm_inst != NULL)
                    {
                        ddmw_set_data(&mchrg_ddm_inst->params_items[mchrg_linked_idx].ddm2_item, linked_instances, idx * sizeof(linked_instances[0]));
                    }
                }
            }
        }
    }
}

static void manage_mdcprofile_classes(uint32_t class_instance, uint8_t chrg_inst, uint8_t prod_instance, uint8_t mchrg_ddm_instance)
{
    uint32_t value;
    int32_t mdcprofile_ddm_instance = INVALID_DDM_INSTANCE;
    /* If there is no MDCPROFILE DDM instance related to a specific charger instance */
    if (sorted_list_unique_get(&value, &chrg_inst_mdcprofile_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_FAIL)
    {
        ddm_class_desc_t *mdcprofile_ddm_inst;

        /* Create a new MDCPROFILE class descriptor*/
        mdcprofile_ddm_inst = ddm_class_desc_create(MDCPROFILE0);
        if (mdcprofile_ddm_inst != NULL)
        {
            /* Register new MDCPROFILE instance in the system */
            mdcprofile_ddm_instance = ddmw_register(&ddm_container, MDCPROFILE0);
            if (mdcprofile_ddm_instance != -1)
            {
                LOG(D, "Created MDCPROFILE instance %d for CHRG instance %d", mdcprofile_ddm_instance, chrg_inst);
                /* Add the mappings in the sorted lists accordingly */
                TRUE_CHECK(sorted_list_unique_add(&rvc_mdcprofile_ddm_inst_mapping_table, class_instance, mdcprofile_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                TRUE_CHECK(sorted_list_unique_add(&chrg_inst_mdcprofile_ddm_inst_mapping_table, chrg_inst, mdcprofile_ddm_instance) == SORTED_LIST_ENTRY_INSERTED);
                /* Initialize the MDCPROFILE class descriptor */
                ddm_class_desc_init(mdcprofile_ddm_instance, MDCPROFILE0, mdcprofile_ddm_inst);

                /* Add all the items except AVL in the DDMW container */
                for (uint32_t i = 1; i < mdcprofile_ddm_inst->ddm_class_desc_size; i++)
                {
                    ddmw_add(&ddm_container, &mdcprofile_ddm_inst->params_items[i].ddm2_item, mdcprofile_ddm_inst->params_items[i].ddm2_param, mdcprofile_ddm_instance);
                    if (DDM2_PARAMETER_BASE_INSTANCE(mdcprofile_ddm_inst->params_items[i].ddm2_param) == MDCPROFILE0LINKED)
                    {
                        mdcprofile_linked_idx = i;
                        SORTED_LIST_KEY_TYPE mdcprofile_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                        int no_of_mdcprofile_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                        /* Find all the RVC instances that are linked to the specific MDCPROFILE DDM instance */
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
                        uint32_t mdcprofile_instance = MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_instance);
                        connector_send_frame_to_broker(DDMP2_CONTROL_SET, PROD0CLIST | DDM2_PARAMETER_INSTANCE(prod_instance), &mdcprofile_instance, sizeof(mdcprofile_instance), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
                    }
                }

                /* Link MDCPROFILE DDM instance with PROD instance */
                TRUE_CHECK(sorted_list_unique_add(&mdcprofile_prod_inst_mapping_table, mdcprofile_ddm_instance, prod_instance) == SORTED_LIST_ENTRY_INSERTED);

                /* Add the MDCPROFILE mapping */
                add_mdcprofile_mapping(mdcprofile_ddm_instance, prod_instance);
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
        /* If there is an MDCPROFILE DDM instance related to a specific CHRG instance
            check whether the RVC is linked to the MDCPROFILE DDM instance and if not, link it */
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

    if (mchrg_ddm_instance != 0xFF)
    {
        SORTED_LIST_VALUE_TYPE temp_mdcprofile_ddm_instance;
        if (sorted_list_unique_get(&temp_mdcprofile_ddm_instance, &chrg_inst_mdcprofile_ddm_inst_mapping_table, chrg_inst, 0) == SORTED_LIST_OK)
        {
            LOG(D, "Linking existing MDCPROFILE instance %d to MCHRG instance %d", temp_mdcprofile_ddm_instance, mchrg_ddm_instance);
            /* Add the MDCPROFILE class instance to the list of linked instances to MCHRG */
            if (sorted_list_unique_add(&rvc_mchrg_ddm_inst_mapping_table, (MDCPROFILE0 | DDM2_PARAMETER_INSTANCE(temp_mdcprofile_ddm_instance)), mchrg_ddm_instance) == SORTED_LIST_ENTRY_INSERTED)
            {
                /* Get all previously linked RVC instances */
                SORTED_LIST_KEY_TYPE mchrg_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
                int no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
                if (sorted_list_get_keys(mchrg_linked, &no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, mchrg_ddm_instance, 0) == SORTED_LIST_OK)
                {
                    /* Prepare a buffer to hold all previous RVC instances and the macout class instance */
                    uint32_t linked_instances[MAX_NO_OF_LINKED_RVC_CLASSES];
                    int idx = 0;
                    /* Copy existing instances */
                    for (int i = 0; i < no_of_mchrg_linked; i++)
                    {
                        linked_instances[idx++] = mchrg_linked[i];
                        LOG(D, "Adding existing linked instance %x to MCHRG instance %d", mchrg_linked[i], mchrg_ddm_instance);
                    }
                    ddm_class_desc_t *mchrg_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mchrg, mchrg_ddm_instance);
                    if (mchrg_ddm_inst != NULL)
                    {
                        ddmw_set_data(&mchrg_ddm_inst->params_items[mchrg_linked_idx].ddm2_item, linked_instances, idx * sizeof(linked_instances[0]));
                    }
                }
            }
        }
    }
}

static void subscribe_to_prop_classes(uint32_t parameter)
{

    switch (DDM2_PARAMETER_CLASS(parameter))
    {
    case RVCINVERT0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvert, parameter);
    }
    break;
    case RVCINVERTCFG0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertcfg, parameter);
    }
    break;

    case RVCINVERTCFGTWO0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertcfgtwo, parameter);
    }
    break;

    case RVCINVERTCFGTHREE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertcfgthree, parameter);
    }
    break;

    case RVCINVERTCFGFOUR0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertcfgfour, parameter);
    }
    break;

    case RVCINVERTDC0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertdc, parameter);
    }
    break;

    case RVCINVERTTEMP0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinverttemp, parameter);
    }
    break;

    case RVCINVERTTEMPTWO0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinverttemptwo, parameter);
    }
    break;

    case RVCINVERTAC0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertac, parameter);
    }
    break;

    case RVCINVERTACTWO0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertactwo, parameter);
    }
    break;

    case RVCINVERTACTHREE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcinvertacthree, parameter);
    }
    break;

    case RVCCHRG0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrg, parameter);
    }
    break;

    case RVCCHRGTWO0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgtwo, parameter);
    }
    break;

    case RVCCHRGCFG0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgcfg, parameter);
    }
    break;

    case RVCCHRGAC0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgac, parameter);
    }
    break;

    case RVCCHRGACTHREE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgacthree, parameter);
    }
    break;

    case RVCCHRGCFGTWO0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgcfgtwo, parameter);
    }
    break;

    case RVCCHRGACFAULTCFG0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgacfaultcfg, parameter);
    }
    break;

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

    case RVCCHRGCFGTHREE0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgcfgthree, parameter);
    }
    break;

    case RVCCHRGCFGFOUR0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgcfgfour, parameter);
    }
    break;

    case RVCCHRGEQCFG0:
    {
        manage_subscriptions_for_prop_class(&l_rvcchrgeqcfg, parameter);
    }
    break;

    default:
        break;
    }
}

static void remove_macout_instance(uint8_t macout_instance, uint8_t prod_instance)
{
    LOG(D, "Removing MACOUT%d from PROD%d", macout_instance, prod_instance);

    ddm_class_desc_t *macout_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_macout, macout_instance);
    if (macout_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < macout_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &macout_ddm_inst->params_items[i].ddm2_item);
        }

        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MACOUT0 | DDM2_PARAMETER_INSTANCE(macout_instance),
                                       &Zero, sizeof(Zero),
                                       connector_ic_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MACOUT mappings */
        SORTED_LIST_KEY_TYPE macout_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_macout_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(macout_linked, &no_of_macout_linked, &rvc_macout_ddm_inst_mapping_table, macout_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_macout_linked; i++)
            {
                sorted_list_unique_remove(&rvc_macout_ddm_inst_mapping_table, macout_linked[i]);
                LOG(D, "Removed RVC class 0x%08X from MACOUT%d mapping", macout_linked[i], macout_instance);
            }
        }
        /* Remove all RVC class instances linked to MACOUT from their descriptors and DDMW container */
        for (int i = 0; i < no_of_macout_linked; i++)
        {
            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(macout_linked[i]);
            switch (DDM2_PARAMETER_CLASS(macout_linked[i]))
            {
            case RVCINVERTAC0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertac, rvc_instance);
                break;
            case RVCINVERTACTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertactwo, rvc_instance);
                break;
            case RVCINVERTACTHREE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertacthree, rvc_instance);
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
                LOG(D, "Removed RVC class 0x%08X instance %d and its DDMW items", DDM2_PARAMETER_CLASS(macout_linked[i]), rvc_instance);
            }
        }

        /* Remove INV->MACOUT mapping */
        SORTED_LIST_KEY_TYPE inv_inst;
        int no_of_inv = 1;
        if (sorted_list_get_keys(&inv_inst, &no_of_inv, &inv_inst_macout_ddm_inst_mapping_table, macout_instance, 0) == SORTED_LIST_OK)
        {
            sorted_list_unique_remove(&inv_inst_macout_ddm_inst_mapping_table, inv_inst);
            LOG(D, "Removed INV instance %d -> MACOUT%d mapping", inv_inst, macout_instance);
        }

        /* Remove MACOUT->PROD mapping */
        sorted_list_unique_remove(&macout_prod_inst_mapping_table, macout_instance);

        /* Delete descriptor */
        ddm_class_desc_delete(macout_ddm_inst);
        LOG(D, "MACOUT%d successfully removed", macout_instance);
    }
}

static void remove_minvert_instance(uint8_t minvert_instance, uint8_t prod_instance)
{
    LOG(D, "Removing MINVERT%d from PROD%d", minvert_instance, prod_instance);

    ddm_class_desc_t *minvert_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_minvert, minvert_instance);
    if (minvert_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < minvert_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &minvert_ddm_inst->params_items[i].ddm2_item);
        }
        LOG(D, "MINVERT instance %d: All DDMW items removed", minvert_instance);
        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MINVERT0 | DDM2_PARAMETER_INSTANCE(minvert_instance),
                                       &Zero, sizeof(Zero),
                                       connector_ic_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MINVERT mappings (includes MACOUT class instance) */
        SORTED_LIST_KEY_TYPE minvert_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_minvert_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(minvert_linked, &no_of_minvert_linked, &rvc_minvert_ddm_inst_mapping_table, minvert_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_minvert_linked; i++)
            {
                sorted_list_unique_remove(&rvc_minvert_ddm_inst_mapping_table, minvert_linked[i]);
                LOG(D, "Removed class 0x%08X from MINVERT%d mapping", minvert_linked[i], minvert_instance);
            }
        }

        /* Remove all RVC class descriptors and their DDMW items that were linked to this MINVERT */
        for (int i = 0; i < no_of_minvert_linked; i++)
        {
            uint32_t linked_class = minvert_linked[i];

            /* Skip MACOUT and MCHRG as they are handled separately */
            if ((DDM2_PARAMETER_CLASS(linked_class) == MACOUT0) || (DDM2_PARAMETER_CLASS(linked_class) == MCHRG0))
            {
                continue;
            }

            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(linked_class);

            switch (DDM2_PARAMETER_CLASS(linked_class))
            {
            case RVCINVERT0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvert, rvc_instance);
                break;
            case RVCINVERTCFG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfg, rvc_instance);
                break;
            case RVCINVERTCFGTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfgtwo, rvc_instance);
                break;
            case RVCINVERTCFGTHREE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfgthree, rvc_instance);
                break;
            case RVCINVERTCFGFOUR0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertcfgfour, rvc_instance);
                break;
            case RVCINVERTDC0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertdc, rvc_instance);
                break;
            case RVCINVERTTEMP0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinverttemp, rvc_instance);
                break;
            case RVCINVERTTEMPTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinverttemptwo, rvc_instance);
                break;
            case RVCINVERTACTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcinvertactwo, rvc_instance);
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

        /* Remove INV->MINVERT mapping */
        SORTED_LIST_KEY_TYPE inv_inst;
        int no_of_minvert = 1;
        if (sorted_list_get_keys(&inv_inst, &no_of_minvert, &inv_inst_minvert_ddm_inst_mapping_table, minvert_instance, 0) == SORTED_LIST_OK)
        {
            sorted_list_unique_remove(&inv_inst_minvert_ddm_inst_mapping_table, inv_inst);
            LOG(D, "Removed INV instance %d -> MINVERT%d mapping", inv_inst, minvert_instance);
        }

        /* Remove MINVERT->PROD mapping */
        sorted_list_unique_remove(&minvert_prod_inst_mapping_table, minvert_instance);

        /* Delete descriptor */
        ddm_class_desc_delete(minvert_ddm_inst);

        /* Remove from mapping table */
        remove_minvert_mapping(minvert_instance);
        LOG(D, "MINVERT%d successfully removed", minvert_instance);
    }
}

static void remove_mdcprofile_instance(uint8_t mdcprofile_instance, uint8_t prod_instance)
{
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
                                       connector_ic_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MDC mappings */
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
            uint32_t linked_class = mdcprofile_linked[i];

            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(mdcprofile_linked[i]);
            switch (DDM2_PARAMETER_CLASS(mdcprofile_linked[i]))
            {
            case RVCDCSRC0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrg, rvc_instance);
                break;
            case RVCDCSRCTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgtwo, rvc_instance);
                break;
            case RVCCHRGCFG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfg, rvc_instance);
                break;
            case RVCCHRGCFGTHREE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfgthree, rvc_instance);
                break;
            case RVCCHRGCFGFOUR0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfgfour, rvc_instance);
                break;
            case RVCCHRGEQCFG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgeqcfg, rvc_instance);
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

        /* Remove MDCPROFILE->PROD mapping */
        sorted_list_unique_remove(&mdcprofile_prod_inst_mapping_table, mdcprofile_instance);
        /* Delete descriptor */
        ddm_class_desc_delete(mdcprofile_ddm_inst);
        /* Remove from mapping table */
        remove_mdcprofile_mapping(mdcprofile_instance);

        LOG(D, "MDCPROFILE%d successfully removed", mdcprofile_instance);
    }
}

static void remove_mchrg_instance(uint8_t mchrg_instance, uint8_t prod_instance)
{
    LOG(D, "Removing MCHRG%d from PROD%d", mchrg_instance, prod_instance);

    ddm_class_desc_t *mchrg_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_mchrg, mchrg_instance);
    if (mchrg_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < mchrg_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &mchrg_ddm_inst->params_items[i].ddm2_item);
        }

        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MCHRG0 | DDM2_PARAMETER_INSTANCE(mchrg_instance),
                                       &Zero, sizeof(Zero),
                                       connector_ic_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MCHRG mappings */
        SORTED_LIST_KEY_TYPE mchrg_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_mchrg_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(mchrg_linked, &no_of_mchrg_linked, &rvc_mchrg_ddm_inst_mapping_table, mchrg_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_mchrg_linked; i++)
            {
                sorted_list_unique_remove(&rvc_mchrg_ddm_inst_mapping_table, mchrg_linked[i]);
                LOG(D, "Removed RVC class 0x%08X from MCHRG%d mapping", mchrg_linked[i], mchrg_instance);
            }
        }

        /* Remove all RVC class instances linked to MCHRG from their descriptors and DDMW container */
        for (int i = 0; i < no_of_mchrg_linked; i++)
        {
            uint32_t linked_class = mchrg_linked[i];

            /* Skip MACIN and MDCPROFILE as they are handled separately */
            if ((DDM2_PARAMETER_CLASS(linked_class) == MACIN0) || (DDM2_PARAMETER_CLASS(linked_class) == MDCPROFILE0))
            {
                continue;
            }

            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(mchrg_linked[i]);
            switch (DDM2_PARAMETER_CLASS(mchrg_linked[i]))
            {
            case RVCCHRG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrg, rvc_instance);
                break;
            case RVCCHRGTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgtwo, rvc_instance);
                break;
            case RVCCHRGCFG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfg, rvc_instance);
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
        /* Remove MCHRG->PROD mapping */
        sorted_list_unique_remove(&mchrg_prod_inst_mapping_table, mchrg_instance);
        /* Delete descriptor */
        ddm_class_desc_delete(mchrg_ddm_inst);
        LOG(D, "MCHRG%d successfully removed", mchrg_instance);
    }
}

static void remove_macin_instance(uint8_t macin_instance, uint8_t prod_instance)
{
    ddm_class_desc_t *macin_ddm_inst = ddm_class_desc_find_by_ddm_instance(&l_macin, macin_instance);
    if (macin_ddm_inst != NULL)
    {
        /* Remove all DDMW items */
        for (unsigned int i = 0; i < macin_ddm_inst->ddm_class_desc_size; i++)
        {
            ddmw_remove(&ddm_container, &macin_ddm_inst->params_items[i].ddm2_item);
        }

        /* Publish unavailable */
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                       MACIN0 | DDM2_PARAMETER_INSTANCE(macin_instance),
                                       &Zero, sizeof(Zero),
                                       connector_ic_mgmt_service.connector_id,
                                       portMAX_DELAY);

        /* Remove all RVC->MACIN mappings */
        SORTED_LIST_KEY_TYPE macin_linked[MAX_NO_OF_LINKED_RVC_CLASSES];
        int no_of_macin_linked = MAX_NO_OF_LINKED_RVC_CLASSES;
        if (sorted_list_get_keys(macin_linked, &no_of_macin_linked, &rvc_macin_ddm_inst_mapping_table, macin_instance, 0) == SORTED_LIST_OK)
        {
            for (int i = 0; i < no_of_macin_linked; i++)
            {
                sorted_list_unique_remove(&rvc_macin_ddm_inst_mapping_table, macin_linked[i]);
                LOG(D, "Removed RVC class 0x%08X from MACIN%d mapping", macin_linked[i], macin_instance);
            }
        }

        /* Remove all RVC class instances linked to MACIN from their descriptors and DDMW container */
        for (int i = 0; i < no_of_macin_linked; i++)
        {
            /* Find and remove the RVC class descriptor */
            ddm_class_desc_t *rvc_class = NULL;
            uint8_t rvc_instance = DDM2_PARAMETER_INSTANCE_FIELD(macin_linked[i]);
            switch (DDM2_PARAMETER_CLASS(macin_linked[i]))
            {
            case RVCCHRGAC0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgac, rvc_instance);
                break;
            case RVCCHRGACTHREE0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgacthree, rvc_instance);
                break;
            case RVCCHRGCFGTWO0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgcfgtwo, rvc_instance);
                break;
            case RVCCHRGACFAULTCFG0:
                rvc_class = ddm_class_desc_find_by_ddm_instance(&l_rvcchrgacfaultcfg, rvc_instance);
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
                LOG(D, "Removed RVC class 0x%08X instance %d and its DDMW items", DDM2_PARAMETER_CLASS(macin_linked[i]), rvc_instance);
            }
        }
        /* Remove MACIN->PROD mapping */
        sorted_list_unique_remove(&macin_prod_inst_mapping_table, macin_instance);
        /* Delete descriptor */
        ddm_class_desc_delete(macin_ddm_inst);
        LOG(D, "MACIN%d successfully removed", macin_instance);
    }
}

static bool is_rvc_class_macout_class_related(uint32_t class_instance)
{
    bool is_rvc_macout_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCINVERTAC0:
    case RVCINVERTACTWO0:
    case RVCINVERTACTHREE0:
        is_rvc_macout_related = true;
        break;

    default:
        is_rvc_macout_related = false;
        break;
    }
    return is_rvc_macout_related;
}

static bool is_rvc_class_minvert_class_related(uint32_t class_instance)
{
    bool is_rvc_minvert_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCINVERT0:
    case RVCINVERTCFG0:
    case RVCINVERTCFGTWO0:
    case RVCINVERTCFGTHREE0:
    case RVCINVERTCFGFOUR0:
    case RVCINVERTDC0:
    case RVCINVERTTEMP0:
    case RVCINVERTTEMPTWO0:
    case RVCINVERTACTWO0:
        is_rvc_minvert_related = true;
        break;

    default:
        is_rvc_minvert_related = false;
        break;
    }
    return is_rvc_minvert_related;
}

static bool is_rvc_class_mchrg_class_related(uint32_t class_instance)
{
    bool is_rvc_mchrg_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCCHRG0:
    case RVCCHRGTWO0:
    case RVCCHRGCFG0:
        is_rvc_mchrg_related = true;
        break;

    default:
        is_rvc_mchrg_related = false;
        break;
    }
    return is_rvc_mchrg_related;
}

static bool is_rvc_class_macin_class_related(uint32_t class_instance)
{
    bool is_rvc_macin_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCCHRGAC0:
    case RVCCHRGACTHREE0:
    case RVCCHRGCFGTWO0:
    case RVCCHRGACFAULTCFG0:
        is_rvc_macin_related = true;
        break;

    default:
        is_rvc_macin_related = false;
        break;
    }
    return is_rvc_macin_related;
}

static bool is_rvc_class_mdcprofile_class_related(uint32_t class_instance)
{
    bool is_rvc_mdcprofile_related = false;
    switch (DDM2_PARAMETER_BASE_INSTANCE(class_instance))
    {
    case RVCDCSRC0:
    case RVCDCSRCTWO0:
    case RVCCHRGCFG0:
    case RVCCHRGCFGTHREE0:
    case RVCCHRGCFGFOUR0:
    case RVCCHRGEQCFG0:
        is_rvc_mdcprofile_related = true;
        break;

    default:
        is_rvc_mdcprofile_related = false;
        break;
    }
    return is_rvc_mdcprofile_related;
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

static void process_prop_data(RVCPROP0DATA_T *prop_data, uint8_t prop_addr)
{
    TRUE_CHECK(prop_data != NULL);
    prop_inv_chrg_oper_check_t *prop_msg = (prop_inv_chrg_oper_check_t *)prop_data->data;

    if (prop_msg->operation == PROP_INTERNAL_INV_TEMPS_OP_REPORT)
    {
        prop_inv_internal_temp_report_t *temp_report = (prop_inv_internal_temp_report_t *)prop_data->data;

        int32_t transf_temp = (int32_t)(temp_report->transformer_temp - PROP_INTERNAL_INV_TEMPS_OFFSET) * Ddm2_unit_factor_list[DDM2_UNIT_DEGC];
        int32_t fet1_temp = (int32_t)(temp_report->fet_temp - PROP_INTERNAL_INV_TEMPS_OFFSET) * Ddm2_unit_factor_list[DDM2_UNIT_DEGC];

        uint8_t minvert_ddm_inst = get_minvert_instance_from_addr(prop_addr);

        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MINVERT0TRANSFTEMP | DDM2_PARAMETER_INSTANCE(minvert_ddm_inst), &transf_temp, sizeof(transf_temp), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MINVERT0FET1TEMP | DDM2_PARAMETER_INSTANCE(minvert_ddm_inst), &fet1_temp, sizeof(fet1_temp), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
    }
    else if (prop_msg->operation == PROP_CHRG_CFG_RESP_OP_REPORT)
    {
        prop_chrg_cfg_req_cmd_resp_t *chrg_cfg_resp = (prop_chrg_cfg_req_cmd_resp_t *)prop_data->data;

        int32_t ovpr_limit_volt = (int32_t)((chrg_cfg_resp->ovpr_limit_voltage / PROP_CHRG_CFG_OVPR_VOLTAGE_SCALE_FACTOR) * Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);
        int32_t uvpr_limit_volt = (int32_t)((chrg_cfg_resp->uvpr_limit_voltage / PROP_CHRG_CFG_OVPR_VOLTAGE_SCALE_FACTOR) * Ddm2_unit_factor_list[DDM2_UNIT_VOLT]);

        uint8_t mdcprofile_ddm_inst = get_mdcprofile_instance_from_addr(prop_addr);

        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MDCPROFILE0HVDISCONN | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_inst), &ovpr_limit_volt, sizeof(ovpr_limit_volt), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MDCPROFILE0LVDISCONN | DDM2_PARAMETER_INSTANCE(mdcprofile_ddm_inst), &uvpr_limit_volt, sizeof(uvpr_limit_volt), connector_ic_mgmt_service.connector_id, portMAX_DELAY);
    }
    return;
}

static void add_minvert_mapping(uint8_t minvert_instance, uint8_t prod_instance)
{
    uint8_t addr = 0xFF;
    PROD0PROP_T prodprop;
    size_t prop_size = sizeof(PROD0PROP_T);

    ProdDBReadCache(FIELD_PROP, prod_instance, &prodprop, &prop_size);
    addr = prodprop.addr;

    for (size_t i = 0; i < ELEMENTS(minvert_inst_addr_map); i++)
    {
        if (minvert_inst_addr_map[i].addr == 0)
        {
            minvert_inst_addr_map[i].addr = addr;
            minvert_inst_addr_map[i].minvert_instance = minvert_instance;
            LOG(D, "Added INV address 0x%02X -> MINVERT%d mapping", addr, minvert_instance);
            break;
        }
    }
}

static void remove_minvert_mapping(uint8_t minvert_instance)
{
    for (size_t i = 0; i < ELEMENTS(minvert_inst_addr_map); i++)
    {
        if (minvert_inst_addr_map[i].minvert_instance == minvert_instance)
        {
            LOG(D, "Removed address 0x%02X -> MINVERT%d mapping", minvert_inst_addr_map[i].addr, minvert_inst_addr_map[i].minvert_instance);
            minvert_inst_addr_map[i].addr = 0;
            minvert_inst_addr_map[i].minvert_instance = 0;
            break;
        }
    }
}

static void add_mdcprofile_mapping(uint8_t mdcprofile_instance, uint8_t prod_instance)
{
    uint8_t addr = 0xFF;
    PROD0PROP_T prodprop;
    size_t prop_size = sizeof(PROD0PROP_T);
    ProdDBReadCache(FIELD_PROP, prod_instance, &prodprop, &prop_size);
    addr = prodprop.addr;

    for (size_t i = 0; i < ELEMENTS(mdcprofile_inst_addr_map); i++)
    {
        if (mdcprofile_inst_addr_map[i].addr == 0)
        {
            mdcprofile_inst_addr_map[i].addr = addr;
            mdcprofile_inst_addr_map[i].mdcprofile_instance = mdcprofile_instance;
            LOG(D, "Added address 0x%02X -> MDCPROFILE%d mapping", addr, mdcprofile_instance);
            break;
        }
    }
}

static void remove_mdcprofile_mapping(uint8_t mdcprofile_instance)
{
    for (size_t i = 0; i < ELEMENTS(mdcprofile_inst_addr_map); i++)
    {
        if (mdcprofile_inst_addr_map[i].mdcprofile_instance == mdcprofile_instance)
        {
            LOG(D, "Removed address 0x%02X -> MDCPROFILE%d mapping", mdcprofile_inst_addr_map[i].addr, mdcprofile_inst_addr_map[i].mdcprofile_instance);
            mdcprofile_inst_addr_map[i].addr = 0;
            mdcprofile_inst_addr_map[i].mdcprofile_instance = 0;
            break;
        }
    }
}

static uint8_t get_minvert_instance_from_addr(uint8_t addr)
{
    uint8_t minvert_inst = 0xFF;
    for (size_t i = 0; i < ELEMENTS(minvert_inst_addr_map); i++)
    {
        if (minvert_inst_addr_map[i].addr == addr)
        {
            minvert_inst = minvert_inst_addr_map[i].minvert_instance;
            break;
        }
    }
    return minvert_inst;
}

static uint8_t get_mdcprofile_instance_from_addr(uint8_t addr)
{
    uint8_t mdcprofile_inst = 0xFF;
    for (size_t i = 0; i < ELEMENTS(mdcprofile_inst_addr_map); i++)
    {
        if (mdcprofile_inst_addr_map[i].addr == addr)
        {
            mdcprofile_inst = mdcprofile_inst_addr_map[i].mdcprofile_instance;
            break;
        }
    }
    return mdcprofile_inst;
}

static void check_and_update_addr_mdcprofile_inst_mapping(uint8_t prod_instance)
{
    uint8_t addr = 0xFF;
    PROD0PROP_T prodprop;
    size_t prop_size = sizeof(PROD0PROP_T);

    ProdDBReadCache(FIELD_PROP, prod_instance, &prodprop, &prop_size);
    addr = prodprop.addr;

    SORTED_LIST_KEY_TYPE mdcprofile_instance;
    int no_of_mdcprofile = 1;
    if (sorted_list_get_keys(&mdcprofile_instance, &no_of_mdcprofile, &mdcprofile_prod_inst_mapping_table, prod_instance, 0) != SORTED_LIST_OK)
    {
        LOG(E, "No MDCPROFILE instance found for PROD%d", prod_instance);
        return;
    }

    for (size_t i = 0; i < ELEMENTS(mdcprofile_inst_addr_map); i++)
    {
        if (mdcprofile_inst_addr_map[i].mdcprofile_instance == mdcprofile_instance)
        {
            if (mdcprofile_inst_addr_map[i].addr != addr)
            {
                LOG(D, "Updated CHRG address 0x%02X -> MCHRG%d mapping to address 0x%02X", mdcprofile_inst_addr_map[i].addr, mdcprofile_instance, addr);
                mdcprofile_inst_addr_map[i].addr = addr;
            }
            break;
        }
    }
}

static void check_and_update_addr_minvert_inst_mapping(uint8_t prod_instance)
{
    uint8_t addr = 0xFF;
    PROD0PROP_T prodprop;
    size_t prop_size = sizeof(PROD0PROP_T);

    ProdDBReadCache(FIELD_PROP, prod_instance, &prodprop, &prop_size);
    addr = prodprop.addr;

    SORTED_LIST_KEY_TYPE minvert_instance;
    int no_of_minvert = 1;
    if (sorted_list_get_keys(&minvert_instance, &no_of_minvert, &minvert_prod_inst_mapping_table, prod_instance, 0) != SORTED_LIST_OK)
    {
        LOG(E, "No MINVERT instance found for PROD%d", prod_instance);
        return;
    }

    for (size_t i = 0; i < ELEMENTS(minvert_inst_addr_map); i++)
    {
        if (minvert_inst_addr_map[i].minvert_instance == minvert_instance)
        {
            if (minvert_inst_addr_map[i].addr != addr)
            {
                LOG(D, "Updated INV address 0x%02X -> MINVERT%d mapping to address 0x%02X", minvert_inst_addr_map[i].addr, minvert_instance, addr);
                minvert_inst_addr_map[i].addr = addr;
            }
            break;
        }
    }
}

CONNECTOR connector_ic_mgmt_service = {
    .name = "Inverter charger management service connector",
    .initialize = connector_ic_mgmt_init,
    .process_event = connector_ic_mgmt_service_process_task,
};

COMPILE_TIME_ASSERT(sizeof(inverter_cmd_t) == 8);
COMPILE_TIME_ASSERT(sizeof(inverter_cfg_cmd_1_t) == 8);
COMPILE_TIME_ASSERT(sizeof(inverter_cfg_cmd_2_t) == 8);
COMPILE_TIME_ASSERT(sizeof(inverter_cfg_cmd_3_t) == 8);
COMPILE_TIME_ASSERT(sizeof(inverter_cfg_cmd_4_t) == 8);
COMPILE_TIME_ASSERT(sizeof(charger_cmd_t) == 8);
COMPILE_TIME_ASSERT(sizeof(charger_cfg_cmd_t) == 8);
COMPILE_TIME_ASSERT(sizeof(charger_cfg_cmd_2_t) == 8);
COMPILE_TIME_ASSERT(sizeof(charger_cfg_cmd_3_t) == 8);
COMPILE_TIME_ASSERT(sizeof(charger_cfg_cmd_4_t) == 8);
COMPILE_TIME_ASSERT(sizeof(charger_eq_cfg_cmd_t) == 8);
COMPILE_TIME_ASSERT(sizeof(prop_chrg_cfg_req_cmd_resp_t) == 8);
COMPILE_TIME_ASSERT(sizeof(prop_req_internal_inv_temps_t) == 8);
COMPILE_TIME_ASSERT(sizeof(prop_inv_internal_temp_report_t) == 8);
