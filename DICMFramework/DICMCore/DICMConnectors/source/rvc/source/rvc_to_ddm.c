/*
 * rvc_to_ddm.c
 *
 *  Created on: 4th June 2020
 *      Author: Stefan.Henningsohn
 */

#include "rvc_to_ddm.h"
#include "PImage.h"
#include "configuration.h"
#include "ddm2_parameter_list.h"
#include <stdint.h>

typedef struct
{
    int32_t domain_value_min;
    int32_t domain_value_max;
    int32_t system_value;
} domain_to_system_t;

#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)

static const domain_to_system_t rvc_htr_elsel_to_system_value[] = {
    {0, 0, HTR0ESEL_GAS},
    {1, 1, HTR0ESEL_GAS_PLUSEL_900_W},
    {2, 2, HTR0ESEL_GAS_PLUSEL_1800_W},
    {3, 3, HTR0ESEL_EL_900_W},
    {4, 4, HTR0ESEL_EL_1800_W}};

static const domain_to_system_t rvc_htr_esrc_to_system_value[] = {
    {0, 0, RVCHTR0ESRC_LPG_ONLY},
    {1, 1, RVCHTR0ESRC_LPG__PLUS_LOW_ELECTRIC_POWER},
    {2, 2, RVCHTR0ESRC_LPG__PLUS_HIGH_ELECTRIC_POWER},
    {3, 3, RVCHTR0ESRC_LOW_ELECTRIC_POWER_ONLY},
    {4, 4, RVCHTR0ESRC_HIGH_ELECTRIC_POWER_ONLY},
    {5, 15, RVCHTR0ESRC_IGNORE}};

static const domain_to_system_t rvc_htr_mode_to_system_value[] = {
    {0, 0, HTR0WTRTEMP_ECO},
    {1, 1, HTR0WTRTEMP_HOT},
    {2, 2, HTR0WTRTEMP_BOOST}};

static const domain_to_system_t rvc_htr_system_units_to_system_value[] = {
    {0, 0, HTR0SYSU_METRIC},
    {1, 1, HTR0SYSU_IMPERIAL}};

static const domain_to_system_t rvc_htr_fan_speed_to_system_value[] = {
    {0, 0, FANSPEED_LEVEL0},
    {1, 1, FANSPEED_LEVEL1},
    {2, 2, FANSPEED_LEVEL2},
    {3, 3, FANSPEED_LEVEL3},
    {4, 4, FANSPEED_LEVEL4},
    {5, 5, FANSPEED_LEVEL5},
    {6, 6, FANSPEED_LEVEL6},
    {7, 7, FANSPEED_LEVEL7}};

static const domain_to_system_t rvc_gas_wtr_heating_to_system_value[] = {
    {0, 0, 0},  // Off
    {1, 1, 0},  // Purge,  still not on
    {2, 2, 0},  // Ignite, still not on
    {3, 3, 1},  // On
};

static const domain_to_system_t rvc_ac_wtr_heating_to_system_value[] = {
    {0, 0, 0},  // Off
    {1, 1, 1},  // Low, on
    {2, 2, 1},  // Med, not used but on in case of other
    {3, 3, 1},  // High, on
};
static const domain_to_system_t rvc_htr_wtr_temp_to_system_value[] = {
    {0, 0, HTR0WTRTS_COLD},
    {1, 1, HTR0WTRTS_WARM},
    {2, 2, HTR0WTRTS_HOT},
};
#endif
const domain_to_system_t rvc_ac_operating_mode_to_system_value[] = {
    // 0 CZ / AC
    // 1 CZ 0 AC
    // 2 CZ 1 AC
    // 3 CZ 3 AC
    // 4 CZ 2 AC
    // 5 CZ 5 AC
    // 6 CZ 4 AC
    {0, 0, 6},  // operating mode 0 means ac0on 0 in Shape
    {1, 1, 0},
    {2, 2, 1},
    {3, 3, 3},
    {4, 4, 2},
    {5, 5, 5},
    {6, 6, 4},
};

const domain_to_system_t rvc_ac_fan_speed_to_system_value[] = {
    {0, 0, 5},
    {1, 50, 0},
    {51, 100, 1},
    {101, 150, 2},
    {151, 200, 3},
};

const domain_to_system_t rvc_ac_dim_level_to_system_value[] = {
    {0, 0, 0},
    {1, 100, 50},
    {101, 200, 100}};

const domain_to_system_t rvc_ac_dim_command_to_sytem_value[] = {
    {0, 0, 1},
    {4, 4, 0}};

typedef struct
{
    int32_t variable;                      // Variable in system parameter store
    uint32_t ddm_parameter;                // DDM parameter
    uint32_t bitmask;                      // Bit mask in a DDM parameter, zero if not used
    uint32_t shift;                        // Shift length of masked data
    int32_t min;                           // Min value for converting to a DDM parameter
    int32_t max;                           // Max value for converting to a DDM parameter
    int32_t def;                           // Default value, when outside min and max value
    int32_t gain;                          // Gain of RV-C value = 1/precision,
    int32_t offset;                        // Offset of RV-C value
    int32_t enum_size;                     // Enum size if value should be converted to an enum, must be 0 if not used
    const domain_to_system_t *enum_table;  // Pointer to enum table, NULL if not used
} conversion_table_t;

typedef struct
{
    rvc_data_type_t rvc_type;
    uint32_t max_rvc_val;
    int32_t rvc_gain;
    int32_t rvc_offset;
    uint32_t default_value;
} rvc_unit_table_t;

// Conversion table for Status frames
static const conversion_table_t conv_table[] = {
// DGN 103559/58
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    {.variable = VAR_DGN_130559_ENERGY_SOURCE, .ddm_parameter = RVCHTR0ESRC, .bitmask = 0x0, .min = 0, .max = 4, .def = 0, .gain = 1, .offset = 0, .enum_size = 6, .enum_table = rvc_htr_esrc_to_system_value},
    {.variable = VAR_DGN_130559_TARGET_ROOM_TEMP, .ddm_parameter = RVCHTR0TTEMP, .bitmask = 0x0, .min = -5000, .max = 15000, .def = 0, .gain = 100, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130559_UNDERVOLT_THRES, .ddm_parameter = RVCHTR0UVTHRES, .bitmask = 0x0, .min = 0, .max = 253, .def = 105, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130559_ENERGY_SOURCE, .ddm_parameter = HTR0ESEL, .bitmask = 0x0, .min = 0, .max = 4, .def = 0, .gain = 1, .offset = 0, .enum_size = 5, .enum_table = rvc_htr_elsel_to_system_value},
    {.variable = VAR_DGN_130559_AIR_HEATER_CMD, .ddm_parameter = HTR0AON, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130559_AIR_HEATER_MODE, .ddm_parameter = HTR0AMD, .bitmask = 0x0, .min = 0, .max = 2, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130559_WTR_HEATER_CMD, .ddm_parameter = HTR0WTRON, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130559_TARGET_ROOM_TEMP, .ddm_parameter = HTR0ATEMP, .bitmask = 0x0, .min = -5000, .max = 15000, .def = 0, .gain = 100, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130559_SILENT_FAN_MAX, .ddm_parameter = HTR0SMAXFAN, .bitmask = 0x0, .min = 0, .max = 7, .def = 3, .gain = 1, .offset = 0, .enum_size = 8, .enum_table = rvc_htr_fan_speed_to_system_value},
    {.variable = VAR_DGN_130559_VENT_FAN_MIN, .ddm_parameter = HTR0VMINFAN, .bitmask = 0x0, .min = 0, .max = 7, .def = 4, .gain = 1, .offset = 0, .enum_size = 8, .enum_table = rvc_htr_fan_speed_to_system_value},
    {.variable = VAR_DGN_130559_WTR_HEATER_MODE, .ddm_parameter = HTR0WTRTEMP, .bitmask = 0x0, .min = 0, .max = 2, .def = 0, .gain = 1, .offset = 0, .enum_size = 3, .enum_table = rvc_htr_mode_to_system_value},
    {.variable = VAR_DGN_130559_UNDERVOLT_THRES, .ddm_parameter = HTR0UVTH, .bitmask = 0x0, .min = 0, .max = 253, .def = 105, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130559_SYSTEM_UNITS, .ddm_parameter = HTR0SYSU, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 2, .enum_table = rvc_htr_system_units_to_system_value},
    // DGN 103552
    {.variable = VAR_DGN_130552_COMFORT_SW_VER_MA, .ddm_parameter = HTR0CVER, .bitmask = 0xff, .shift = 8, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130552_COMFORT_SW_VER_MI, .ddm_parameter = HTR0CVER, .bitmask = 0xff, .shift = 0, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130552_BURNER_SW_VER_MA, .ddm_parameter = HTR0BVER, .bitmask = 0xff, .shift = 8, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130552_BURNER_SW_VER_MI, .ddm_parameter = HTR0BVER, .bitmask = 0xff, .shift = 0, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130552_PCBA_VER, .ddm_parameter = HTR0PCBA, .bitmask = 0x0, .shift = 0, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130552_PROTOCOL_VER_MA, .ddm_parameter = HTR0PROT, .bitmask = 0xff, .shift = 8, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130552_PROTOCOL_VER_MI, .ddm_parameter = HTR0PROT, .bitmask = 0xff, .shift = 0, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // DGN 103553
    {.variable = VAR_DGN_130553_WARNING_FAULT_ACTIVE, .ddm_parameter = HTR0ERRST, .bitmask = 0x1, .shift = 0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130553_CRITICAL_FAULT_ACTIVE, .ddm_parameter = HTR0ERRST, .bitmask = 0x1, .shift = 1, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130553_ACTIVE_FAULT_CODE_1, .ddm_parameter = HTR0ERRCD1, .bitmask = 0x0, .min = 0, .max = 1021, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130553_ACTIVE_FAULT_CODE_2, .ddm_parameter = HTR0ERRCD2, .bitmask = 0x0, .min = 0, .max = 1021, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130553_ACTIVE_FAULT_CODE_3, .ddm_parameter = HTR0ERRCD3, .bitmask = 0x0, .min = 0, .max = 1021, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130553_ACTIVE_FAULT_CODE_4, .ddm_parameter = HTR0ERRCD4, .bitmask = 0x0, .min = 0, .max = 1021, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // DGN 130555
    {.variable = VAR_DGN_130555_AIR_HTR_ON_STAT, .ddm_parameter = HTR0AHTONST, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_AIR_HTR_ON_HOUR, .ddm_parameter = HTR0AHTONH, .bitmask = 0x0, .min = 0, .max = 23, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_AIR_HTR_ON_MIN, .ddm_parameter = HTR0AHTONM, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_AIR_HTR_OFF_STAT, .ddm_parameter = HTR0AHTOFFST, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_AIR_HTR_OFF_HOUR, .ddm_parameter = HTR0AHTOFFH, .bitmask = 0x0, .min = 0, .max = 23, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_AIR_HTR_OFF_MIN, .ddm_parameter = HTR0AHTOFFM, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_WTR_HTR_ON_STAT, .ddm_parameter = HTR0WTRTST, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_WTR_HTR_ON_HOUR, .ddm_parameter = HTR0WTRTONH, .bitmask = 0x0, .min = 0, .max = 23, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_WTR_HTR_ON_MIN, .ddm_parameter = HTR0WTRTONM, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME, .ddm_parameter = HTR0WTRTKET, .bitmask = 0x0, .min = 0, .max = 120, .def = 6, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // DGN 130556

    {.variable = VAR_DGN_130556_ROOM_TEMP, .ddm_parameter = RVCHMI0RTEMP, .bitmask = 0x0, .min = -5000, .max = 15000, .gain = 100, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130556_ROOM_TEMP, .ddm_parameter = TH0ITEMP, .bitmask = 0x0, .min = -5000, .max = 15000, .gain = 100, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130556_BUTTON_FAV, .ddm_parameter = TH0BUT0, .bitmask = 0x0, .min = 0, .max = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130556_BUTTON_MENU, .ddm_parameter = TH0BUT1, .bitmask = 0x0, .min = 0, .max = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130556_BUTTON_HOME, .ddm_parameter = TH0BUT2, .bitmask = 0x0, .min = 0, .max = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // DGN 130557
    {.variable = VAR_DGN_130557_ROOM_TEMP, .ddm_parameter = RVCHTRST0RTEMP, .bitmask = 0x0, .min = -5000, .max = 15000, .gain = 100, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130557_ROOM_TEMP, .ddm_parameter = HTR0RTS, .bitmask = 0x0, .min = -5000, .max = 15000, .gain = 100, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130557_WATER_TEMP, .ddm_parameter = HTR0WTRTS, .bitmask = 0x0, .min = 0, .max = 2, .gain = 1, .offset = 0, .enum_size = 3, .enum_table = rvc_htr_wtr_temp_to_system_value},
    {.variable = VAR_DGN_130557_AC_PRESENT, .ddm_parameter = HTR0ACST, .bitmask = 0x0, .min = 0, .max = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130557_GAS_HEATER_WTR, .ddm_parameter = HTR0GASWTRHST, .bitmask = 0x0, .min = 0, .max = 3, .gain = 1, .offset = 0, .enum_size = 4, .enum_table = rvc_gas_wtr_heating_to_system_value},
    {.variable = VAR_DGN_130557_AC_HEATER_WTR, .ddm_parameter = HTR0ACWTRHST, .bitmask = 0x0, .min = 0, .max = 3, .gain = 1, .offset = 0, .enum_size = 4, .enum_table = rvc_ac_wtr_heating_to_system_value},
#endif
#if defined(RVC_CONFIG_BUS_TIME_USE) || defined(RVC_CONFIG_BUS_TIME_KEEP)
    // DGN 131071
    {.variable = VAR_DGN_131071_YEAR, .ddm_parameter = HTR0DATEY, .bitmask = 0x0, .min = 20, .max = 250, .def = 20, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_131071_MONTH, .ddm_parameter = HTR0DATEM, .bitmask = 0x0, .min = 1, .max = 12, .def = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_131071_DAY, .ddm_parameter = HTR0DATED, .bitmask = 0x0, .min = 1, .max = 31, .def = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_131071_HOUR, .ddm_parameter = HTR0TIMEH, .bitmask = 0x0, .min = 0, .max = 23, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_131071_MINUTE, .ddm_parameter = HTR0TIMEM, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_131071_SECOND, .ddm_parameter = HTR0TIMES, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_131071_TIMEZONE, .ddm_parameter = HTR0TTZ, .bitmask = 0x0, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
#endif
// DGN 65259
#if defined(RVC_CONFIG_IMPL_SHARC_HTR) || defined(RVC_CONFIG_INTERF_SHARC_HTR)
    {.variable = VAR_DGN_65259_IN_MODEL, .ddm_parameter = HTR0MDL, .bitmask = 0x0, .min = 0, .max = 65535, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
#endif
#if defined(CONNECTOR_SHAPE_CFG) || defined(CONNECTOR_SHAPE_FIX_CFG)
    {.variable = VAR_DGN_130809_OPERATING_MODE, .ddm_parameter = AC0MD, .bitmask = 0x0, .min = 0, .max = 255, .def = 0, .gain = 1, .offset = 0, .enum_size = 7, .enum_table = rvc_ac_operating_mode_to_system_value},
    {.variable = VAR_DGN_130809_SET_POINT_TEMP_COOL, .ddm_parameter = AC0TTEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130809_FAN_MODE, .ddm_parameter = AC0FMD, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = AC0ON, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130809_FAN_SPEED, .ddm_parameter = AC0FSPD, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 5, .enum_table = rvc_ac_fan_speed_to_system_value},
    {.variable = 0, .ddm_parameter = AC0LGT, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = AC0ELGT, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130779_DESIRED_LEVEL_BRIGHTNESS_L2, .ddm_parameter = AC0DMR, .bitmask = 0x0, .min = 0, .max = 200, .def = 0, .gain = 1, .offset = 0, .enum_size = 3, .enum_table = rvc_ac_dim_level_to_system_value},
    {.variable = VAR_DGN_130779_COMMAND_L1, .ddm_parameter = AC0ELGT, .bitmask = 0x0, .min = 0, .max = 4, .def = 4, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = AC0ITEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_130808_REDUCED_NOISE_MODE, .ddm_parameter = AC0SLEEP, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_Oper42_ActExt, .ddm_parameter = AC0ACTEXT, .bitmask = 0x1, .shift = 4, .min = 0, .max = 255, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = VAR_DGN_Oper42_RemCtrl, .ddm_parameter = AC0REMCTRL, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
#endif
#ifdef RVC_CONFIG_IMPL_REFRIGERATOR
    {.variable = 0, .ddm_parameter = RVCREFRIG0LGT, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCREFRIG0DOOR, .bitmask = 0x0, .min = 0, .max = 1, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // Current Temperature default 20℃
    {.variable = 0, .ddm_parameter = RVCREFRIG0CTEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    // Set Temperature default 5℃
    {.variable = 0, .ddm_parameter = RVCREFRIG0SETTEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 8896, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCREFRIG0MODE, .bitmask = 0x0, .min = 0, .max = 4, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // conversion precision=0.5 %
    {.variable = 0, .ddm_parameter = RVCREFRIG0COMPSPD, .bitmask = 0x0, .min = 0, .max = 200, .def = 0, .gain = 200, .offset = 0, .enum_size = 0, .enum_table = NULL},
#endif

    // RVCTHSCHED0 class conversions
    // RVCTHSCHED0CSET, RVCTHSCHED0HSET: def=20 °C, max=1735 C min=-273 °C precision=0.03125 °C
    {.variable = 0, .ddm_parameter = RVCTHSCHED0CSET, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTHSCHED0HSET, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTHSCHED0HOUR, .bitmask = 0x0, .min = 0, .max = 23, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTHSCHED0MIN, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCTH0 class conversions
    // RVCTH0CSET, RVCTH0HSET: def=20 °C, max=1735 C min=-273 °C precision=0.03125 °C
    {.variable = 0, .ddm_parameter = RVCTH0CSET, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTH0HSET, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    // RVCTH0FSPD % conversion precision=0.5 %
    {.variable = 0, .ddm_parameter = RVCTH0FSPD, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCTHASTAT0 class conversions
    {.variable = 0, .ddm_parameter = RVCTHASTAT0TEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    // RVCTIME0 class conversions
    {.variable = 0, .ddm_parameter = RVCTIME0YEAR, .bitmask = 0x0, .min = 0, .max = 250, .def = 23, .gain = 1, .offset = -2000, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTIME0MONTH, .bitmask = 0x0, .min = 1, .max = 12, .def = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTIME0DAY, .bitmask = 0x0, .min = 0, .max = 31, .def = 1, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTIME0HOUR, .bitmask = 0x0, .min = 0, .max = 23, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTIME0MIN, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTIME0SEC, .bitmask = 0x0, .min = 0, .max = 59, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCTIME0TZ, .bitmask = 0x0, .min = 0, .max = 253, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVC2DIM0 class conversions (DC DIMMER)
    {.variable = 0, .ddm_parameter = RVCDIMTWO0DEL_DUR, .bitmask = 0x0, .min = 0, .max = 229, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDIMTWO0RTIME, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCAC0 class conversions (AIR CONDITIONER %)
    {.variable = 0, .ddm_parameter = RVCAC0MFSPD, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCAC0MOLVL, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCAC0FSPD, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCAC0OLVL, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // AIR CONDITIONER  ^C
    {.variable = 0, .ddm_parameter = RVCAC0DBAND, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCAC0SDBAND, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVC2AC0 class conversions (AIR CONDITIONER ^C)
    {.variable = 0, .ddm_parameter = RVCACTWO0EXTTEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCACTWO0COILTEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRC0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRC0VOLT, .bitmask = 0x0, .min = 0, .max = 64250, .def = 0, .gain = 20, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRC0CURR, .bitmask = 0x0, .min = 0, .max = 4221081200, .def = 0, .gain = 1000, .offset = 2000000, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCTWO0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCTWO0STEMP, .bitmask = 0x0, .min = 0, .max = 64256, .def = 9376, .gain = 32, .offset = 273, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCTWO0SOC, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCTHR0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCTHR0CAP, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCTHR0RIPPLE, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCTHR0SOH, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCTHR0RELCAP, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCFOUR0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCFOUR0VOLT, .bitmask = 0x0, .min = 0, .max = 64250, .def = 0, .gain = 20, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCFOUR0CURR, .bitmask = 0x0, .min = 0, .max = 64250, .def = 0, .gain = 20, .offset = 1600, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCFIVE0 class conversions
    // {.variable = 0, .ddm_parameter = RVCDCSRCFIVE0VOLT, .bitmask = 0x0, .min = 0, .max = INT32_MAX, .def = 0, .gain = 1000, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCSEV0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCSEV0INPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCSEV0OUTPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCEIG0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCEIG0INPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCEIG0OUTPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCNINE0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCNINE0INPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCNINE0OUTPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCTEN0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCTEN0INPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCTEN0OUTPUT, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCELE0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCELE0CAPACITY, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCELE0POWER, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCDCSRCCFG0 class conversions
    {.variable = 0, .ddm_parameter = RVCDCSRCCFG0FACT, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCCFG0CAPACITY, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCCFG0EXP, .bitmask = 0x0, .min = 0, .max = 253, .def = 0, .gain = 100, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCCFG0COEFF, .bitmask = 0x0, .min = 0, .max = 200, .def = 0, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCDCSRCCFG0TAILCURR, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},

    // RVCDCSRCCFGTWO0 class conversions
    //    {.variable = 0, .ddm_parameter = RVCDCSRCCFGTWO0SHTVOLT, .bitmask = 0x0, .min = 0, .max = 253, .def = 0, .gain = 1000, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCBATSUM0 class conversions
    {.variable = 0, .ddm_parameter = RVCBATSUM0TEMPSTS, .bitmask = 0x0, .min = 0, .max = 4, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCBATSUM0VOLTSTS, .bitmask = 0x0, .min = 0, .max = 4, .def = 0, .gain = 1, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCHPUMP0MOLVL, RVCHPUMP0OLVL, RVCHPUMP0FSPD % conversion precision=0.5 %
    {.variable = 0, .ddm_parameter = RVCHPUMP0MOLVL, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCHPUMP0OLVL, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCHPUMP0FSPD, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 2, .offset = 0, .enum_size = 0, .enum_table = NULL},
    // RVCHPUMP0DBAND, RVCHPUMP0SDBAND degC conversion precision=0.1 degC
    {.variable = 0, .ddm_parameter = RVCHPUMP0DBAND, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCHPUMP0SDBAND, .bitmask = 0x0, .min = 0, .max = 250, .def = 0, .gain = 10, .offset = 0, .enum_size = 0, .enum_table = NULL},
#ifdef RVC_CONFIG_INVERTER_CHARGER
    // Precision = 1, offset = 32000
    {.variable = 0, .ddm_parameter = RVCINVERTACTHREE0REACTPOW, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 32000, .enum_size = 0, .enum_table = NULL},
    {.variable = 0, .ddm_parameter = RVCCHRGACTHREE0REACTPOW, .bitmask = 0x0, .min = 0, .max = 65530, .def = 0, .gain = 1, .offset = 32000, .enum_size = 0, .enum_table = NULL},
#endif
};

// RVC unit table conversion (based on Table 5.3 from RV-C specification)
static const rvc_unit_table_t rvc_unit_table_conv[] = {
    {.rvc_type = RVC_PART_UINT8, .max_rvc_val = 250, .rvc_gain = 2, .rvc_offset = 0, .default_value = 255},
    {.rvc_type = RVC_INST_UINT8, .max_rvc_val = 250, .rvc_gain = 1, .rvc_offset = 0, .default_value = 255},
    {.rvc_type = RVC_DEGC_UINT8, .max_rvc_val = 250, .rvc_gain = 1, .rvc_offset = 40, .default_value = 255},
    {.rvc_type = RVC_DEGC_UINT16, .max_rvc_val = 64256, .rvc_gain = 32, .rvc_offset = 8736, .default_value = 65535},
    {.rvc_type = RVC_VACDC_UINT8, .max_rvc_val = 250, .rvc_gain = 1, .rvc_offset = 0, .default_value = 255},
    {.rvc_type = RVC_VACDC_UINT16, .max_rvc_val = 64250, .rvc_gain = 20, .rvc_offset = 0, .default_value = 65535},
    {.rvc_type = RVC_AACDC_UINT8, .max_rvc_val = 250, .rvc_gain = 1, .rvc_offset = 0, .default_value = 255},
    {.rvc_type = RVC_AACDC_UINT16, .max_rvc_val = 64250, .rvc_gain = 20, .rvc_offset = 32000, .default_value = 65535},
    {.rvc_type = RVC_AACDC_UINT32, .max_rvc_val = 4221081200, .rvc_gain = 1000, .rvc_offset = 2000000000, .default_value = UINT32_MAX},
    {.rvc_type = RVC_HZ_UINT8, .max_rvc_val = 250, .rvc_gain = 1, .rvc_offset = 0, .default_value = 255},
    {.rvc_type = RVC_W_UINT16, .max_rvc_val = 65530, .rvc_gain = 1, .rvc_offset = 0, .default_value = 65535},
    {.rvc_type = RVC_AMPHOUR_UINT16, .max_rvc_val = 65530, .rvc_gain = 1, .rvc_offset = 0, .default_value = 65535},
    {.rvc_type = RVC_STD_UINT8, .max_rvc_val = 255, .rvc_gain = 1, .rvc_offset = 0, .default_value = 255},
    {.rvc_type = RVC_STD_UINT8_GAIN1000, .max_rvc_val = 255, .rvc_gain = 1000, .rvc_offset = 0, .default_value = 255},
    {.rvc_type = RVC_STD_UINT16, .max_rvc_val = 65535, .rvc_gain = 1, .rvc_offset = 0, .default_value = 65535},
    {.rvc_type = RVC_STD_UINT32, .max_rvc_val = UINT32_MAX, .rvc_gain = 1, .rvc_offset = 0, .default_value = UINT32_MAX}};

static void convert_rvc_bitmask_to_ddm_system_value(uint32_t ddm_parameter, uint32_t shift, int32_t *i32Value)
{
    const conversion_table_t *ptr = &conv_table[0];
    int32_t sum = 0;
    int32_t value;
    int32_t bitmask = 0;

    for (int i = 0; i < (int)ELEMENTS(conv_table); i++, ptr++)
    {
        if (ptr->ddm_parameter == ddm_parameter)
        {
            value = PIMAGE_GetValue(ptr->variable);
            if ((value < ptr->min) || (value > ptr->max))
            {
                // The value is outside limits, use default value
                value = ptr->def;
            }

            value &= ptr->bitmask;
            value <<= ptr->shift;
            sum += value;
            if (shift == ptr->shift)
            {
                // Store bitmask to be used in later calculations
                bitmask = ptr->bitmask;
            }
        }
    }

    if (*i32Value == 0)
    {
        sum &= (int32_t)(~(bitmask << shift));
    }
    else
    {
        sum |= (*i32Value) << shift;
    }

    *i32Value = sum;
}

/**
 * @brief Convert RVC value to DDM2 parameter value. Assumes that parameters are to be published
 *
 * @param variable[in] RVC variable to convert
 * @param i32Value[in,out]
 * @param type[in] Type of variable
 * @return
 */
int convert_rvc_to_ddm_system_value(const uint32_t parameter, int32_t *i32Value)
{
    int result = 0;
    const conversion_table_t *ptr = &conv_table[0];
    int factor;
    int index;

    if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) == -1)
    {
        // Invalid parameter
        return 0;
    }
    factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].out_unit];

    for (int i = 0; i < (int)ELEMENTS(conv_table); i++, ptr++)
    {
        if (ptr->ddm_parameter == (uint32_t)DDM2_PARAMETER_BASE_INSTANCE(parameter))
        {
            if (((*i32Value) < ptr->min) || ((*i32Value) > ptr->max))
            {
                // The value is outside limits, use default value
                *i32Value = ptr->def;
            }

            if (ptr->enum_size == 0)
            {
                if (ptr->bitmask != 0)
                {
                    // This value is part of a bit mask so collect the bits together
                    convert_rvc_bitmask_to_ddm_system_value(ptr->ddm_parameter, ptr->shift, i32Value);
                }
                else
                {
                    if (!factor)
                    {
                        factor = 1;
                    }

                    int64_t value = (*i32Value * factor);

                    *i32Value = (int32_t)(((value) / ptr->gain) - ptr->offset * factor);
                }
                char name_buffer[30];
                size_t buf_size = 30;
                LOG(D, "%s", ddm2_parameter_name(parameter, name_buffer, &buf_size));
                LOG(D, "factor : %d lookup index : %d gain : %d offset : %d i32Value : %d", factor, index, ptr->gain, ptr->offset, *i32Value);
                result = 1;
            }
            else
            {
                for (int j = 0; j < ptr->enum_size; j++)
                {
                    if ((ptr->enum_table[j].domain_value_min <= (*i32Value)) && (ptr->enum_table[j].domain_value_max >= (*i32Value)))
                    {
                        *i32Value = ptr->enum_table[j].system_value;
                        LOG(I, "system_value: %d domain_value %d\n", ptr->enum_table[j].system_value, ptr->enum_table[j].domain_value_max);
                        result = 1;
                        break;
                    }
                }
            }
            break;
        }
    }
    if (!result)
    {
        // Perform normal conversion, do not require an entry in table above
        int64_t value = *i32Value;
        value = value * factor;
        *i32Value = (int32_t)value;
        result = 1;
    }
    return result;
}

/**
 * @brief Convert from DDM to RVC
 *
 * @param parameter[in] DDM2 parameter to convert
 * @param i32Value[in,out] Input value to convert, store output value
 * @param is_set[in] true if SET parameter, false if PUB parameter
 * @return
 */
int convert_ddm_system_value_to_rvc_value(const uint32_t parameter, int32_t *i32Value, bool is_set)
{
    int result = 0;
    const conversion_table_t *ptr = &conv_table[0];
    int factor;
    int index;
    int64_t value;

    if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) == -1)
    {
        return 0;
    }

    if (is_set)
    {
        factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].in_unit];
    }
    else
    {
        factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].out_unit];
    }

    for (int i = 0; i < (int)ELEMENTS(conv_table); i++, ptr++)
    {
        if (ptr->ddm_parameter == DDM2_PARAMETER_BASE_INSTANCE(parameter))
        {
            if (ptr->enum_size == 0)
            {
                char name_buffer[30];
                size_t buf_size = 30;
                value = *i32Value;
                LOG(D, "%s", ddm2_parameter_name(parameter, name_buffer, &buf_size));
                LOG(D, "[init value: %d Val = %d ]", *i32Value, (int32_t)value);
                LOG(D, "[factor: %d Gain:%d]", factor, ptr->gain);
                if (factor)
                {
                    value += (ptr->offset * factor);
                    // value = (*i32Value) * ptr->gain; // Increase with gain first to avoid loosing accuracy
                    value = (value * ptr->gain) / factor;
                }
                else
                {
                    value += ptr->offset;
                    // value = (*i32Value) * ptr->gain; // Increase with gain first to avoid loosing accuracy
                    value = value * ptr->gain;
                }
                // Check limits
                if (((int32_t)value < ptr->min) || ((int32_t)value > ptr->max))
                {
                    return 0;
                }

                *i32Value = (int32_t)value;
                result = 1;

                LOG(D, "Final value: %d", *i32Value);
            }
            else
            {
                for (int j = 0; j < ptr->enum_size; j++)
                {
                    if (ptr->enum_table[j].system_value == (*i32Value))
                    {
                        LOG(I, "i32Value: %d", *i32Value);
                        *i32Value = ptr->enum_table[j].domain_value_max;
                        result = 1;

                        LOG(I, "system_value: %d domain_value %d\n", ptr->enum_table[j].system_value, ptr->enum_table[j].domain_value_max);
                        break;
                    }
                }
            }
            break;
        }
    }
    if (!result)
    {
        // Perform normal conversion, do not require an entry in table above
        value = *i32Value;
        value = value / factor;
        *i32Value = (int32_t)value;
        result = 2;
    }
    return result;
}

int convert_rvc_to_ddm_rvc_params_gain(const uint32_t parameter, void *value, rvc_data_type_t type, int32_t user_rvc_gain)
{
    int result = 0;
    const rvc_unit_table_t *ptr = &rvc_unit_table_conv[0];
    int factor;
    int index;

    if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) == -1)
    {
        // Invalid parameter
        return 0;
    }
    factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].out_unit];
    char name_buffer[30];
    size_t buf_size = 30;
    LOG(D, "%s", ddm2_parameter_name(parameter, name_buffer, &buf_size));
    for (int i = 0; i < (int)ELEMENTS(rvc_unit_table_conv); i++, ptr++)
    {
        if (ptr->rvc_type == type)
        {
            result = 1;

            if (!factor)
            {
                factor = 1;
            }
            // Since the DDM2 parameters are always defined as int32_t or uint32_t, we can safely cast the value to int32_t or uint32_t
            switch (type)
            {
            case RVC_PART_UINT8:
            case RVC_INST_UINT8:
            case RVC_DEGC_UINT8:
            case RVC_VACDC_UINT8:
            case RVC_AACDC_UINT8:
            case RVC_HZ_UINT8:
            case RVC_STD_UINT8:
            case RVC_STD_UINT8_GAIN1000:
            case RVC_W_UINT16:
            case RVC_AMPHOUR_UINT16:
            case RVC_DEGC_UINT16:
            case RVC_VACDC_UINT16:
            case RVC_AACDC_UINT16:
            case RVC_STD_UINT16:
            {
                int32_t loc_value = *((int32_t *)value);
                if (loc_value > (int32_t)ptr->max_rvc_val)
                {
                    *((int32_t *)value) = (int32_t)ptr->default_value;
                }
                else
                {
                    loc_value -= ptr->rvc_offset;
                    float f_val = ((float)loc_value / (float)user_rvc_gain);
                    *((int32_t *)value) = (int32_t)(f_val * (float)factor);
                }
                LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %d", ptr->rvc_type, factor, user_rvc_gain, ptr->rvc_offset, *((int32_t *)value));
            }
            break;
            case RVC_AACDC_UINT32:
            {
                int64_t loc_value = (int64_t)(*((uint32_t *)value));
                if (loc_value > (int64_t)ptr->max_rvc_val)
                {
                    *((int64_t *)value) = (int64_t)ptr->default_value;
                }
                else
                {
                    loc_value -= ptr->rvc_offset;
                    double f_val = ((double)loc_value / (double)user_rvc_gain);
                    f_val *= (double)factor;
                    if (f_val > INT32_MAX)
                    {
                        *((int32_t *)value) = INT32_MAX;
                    }
                    else if (f_val < INT32_MIN)
                    {
                        *((int32_t *)value) = INT32_MIN;
                    }
                    else
                    {
                        // Value is within int32_t range, safe to cast
                        *((int32_t *)value) = (int32_t)(f_val);
                    }
                }
                LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %lld", ptr->rvc_type, factor, user_rvc_gain, ptr->rvc_offset, *((int64_t *)value));
            }
            break;
            case RVC_STD_UINT32:
            {
                uint32_t loc_value = *((uint32_t *)value);
                if (loc_value > ptr->max_rvc_val)
                {
                    *((uint32_t *)value) = (uint32_t)ptr->default_value;
                }
                else
                {
                    loc_value -= ptr->rvc_offset;
                    float f_val = ((float)loc_value / (float)user_rvc_gain);
                    *((uint32_t *)value) = (uint32_t)(f_val * (float)factor);
                }
                LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %u", ptr->rvc_type, factor, user_rvc_gain, ptr->rvc_offset, *((uint32_t *)value));
            }
            break;
            default:
                LOG(W, "Unsupported RVC data type %d", type);
                result = 0;
                break;
            }

            break;
        }
    }
    return result;
}

int convert_rvc_to_ddm_rvc_params(const uint32_t parameter, void *value, rvc_data_type_t type)
{
    int result = 0;
    const rvc_unit_table_t *ptr = &rvc_unit_table_conv[0];
    int factor;
    int index;

    if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) == -1)
    {
        // Invalid parameter
        return 0;
    }
    factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].out_unit];
    char name_buffer[30];
    size_t buf_size = 30;
    LOG(D, "%s", ddm2_parameter_name(parameter, name_buffer, &buf_size));
    for (int i = 0; i < (int)ELEMENTS(rvc_unit_table_conv); i++, ptr++)
    {
        if (ptr->rvc_type == type)
        {
            result = 1;

            if (!factor)
            {
                factor = 1;
            }
            // Since the DDM2 parameters are always defined as int32_t or uint32_t, we can safely cast the value to int32_t or uint32_t
            switch (type)
            {
            case RVC_PART_UINT8:
            case RVC_INST_UINT8:
            case RVC_DEGC_UINT8:
            case RVC_VACDC_UINT8:
            case RVC_AACDC_UINT8:
            case RVC_HZ_UINT8:
            case RVC_STD_UINT8:
            case RVC_STD_UINT8_GAIN1000:
            case RVC_W_UINT16:
            case RVC_AMPHOUR_UINT16:
            case RVC_DEGC_UINT16:
            case RVC_VACDC_UINT16:
            case RVC_AACDC_UINT16:
            case RVC_STD_UINT16:
            {
                int32_t loc_value = *((int32_t *)value);
                if (loc_value > (int32_t)ptr->max_rvc_val)
                {
                    *((int32_t *)value) = (int32_t)ptr->default_value;
                }
                else
                {
                    loc_value -= ptr->rvc_offset;
                    float f_val = ((float)loc_value / (float)ptr->rvc_gain);
                    *((int32_t *)value) = (int32_t)(f_val * (float)factor);
                }
                LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %d", ptr->rvc_type, factor, ptr->rvc_gain, ptr->rvc_offset, *((int32_t *)value));
            }
            break;
            case RVC_AACDC_UINT32:
            {
                int64_t loc_value = (int64_t)(*((uint32_t *)value));
                if (loc_value > (int64_t)ptr->max_rvc_val)
                {
                    *((int32_t *)value) = (int32_t)ptr->default_value;
                }
                else
                {
                    loc_value -= ptr->rvc_offset;
                    double f_val = ((double)loc_value / (double)ptr->rvc_gain);
                    f_val *= (double)factor;
                    if (f_val > INT32_MAX)
                    {
                        *((int32_t *)value) = INT32_MAX;
                    }
                    else if (f_val < INT32_MIN)
                    {
                        *((int32_t *)value) = INT32_MIN;
                    }
                    else
                    {
                        // Value is within int32_t range, safe to cast
                        *((int32_t *)value) = (int32_t)(f_val);
                    }
                }
                LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %d", ptr->rvc_type, factor, ptr->rvc_gain, ptr->rvc_offset, *((int32_t *)value));
            }
            break;
            case RVC_STD_UINT32:
            {
                uint32_t loc_value = *((uint32_t *)value);
                if (loc_value > ptr->max_rvc_val)
                {
                    *((uint32_t *)value) = (uint32_t)ptr->default_value;
                }
                else
                {
                    loc_value -= ptr->rvc_offset;
                    float f_val = ((float)loc_value / (float)ptr->rvc_gain);
                    *((uint32_t *)value) = (uint32_t)(f_val * (float)factor);
                }
                LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %u", ptr->rvc_type, factor, ptr->rvc_gain, ptr->rvc_offset, *((uint32_t *)value));
            }
            break;
            default:
                LOG(W, "Unsupported RVC data type %d", type);
                result = 0;
                break;
            }

            break;
        }
    }
    return result;
}

// It will try to use special implementation and then revert to generic conversion
int convert_ddm_to_rvc_value(const uint32_t parameter, void *value, rvc_data_type_t type)
{
    int32_t local_value = *(int32_t *)value;
    int result = 0;
    if (convert_ddm_system_value_to_rvc_value(parameter, &local_value, true) == 1)
    {
        // Special conversion found.
        result = 1;
        *(int32_t *)value = local_value;
    }
    else
    {
        int index;
        // Try generic conversion
        if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) == -1)
        {
            // Invalid parameter
            return 0;
        }

        int factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].in_unit];
        const rvc_unit_table_t *ptr = &rvc_unit_table_conv[0];
        for (int i = 0; i < (int)ELEMENTS(rvc_unit_table_conv); i++, ptr++)
        {
            if (ptr->rvc_type == type)
            {
                result = 1;
                if (!factor)
                {
                    factor = 1;
                }
                // Since the DDM2 parameters are always defined as int32_t or uint32_t, we can safely cast the value to int32_t or uint32_t
                switch (type)
                {
                case RVC_PART_UINT8:
                case RVC_INST_UINT8:
                case RVC_DEGC_UINT8:
                case RVC_VACDC_UINT8:
                case RVC_AACDC_UINT8:
                case RVC_HZ_UINT8:
                case RVC_STD_UINT8:
                case RVC_STD_UINT8_GAIN1000:
                case RVC_W_UINT16:
                case RVC_AMPHOUR_UINT16:
                case RVC_DEGC_UINT16:
                case RVC_VACDC_UINT16:
                case RVC_AACDC_UINT16:
                case RVC_STD_UINT16:
                {
                    int32_t loc_value = *((int32_t *)value);  // Including factor
                    float f_val = (((float)loc_value * (float)ptr->rvc_gain) / (float)factor);
                    f_val += (float)ptr->rvc_offset;
                    loc_value = (int32_t)f_val;
                    if ((loc_value > (int32_t)ptr->max_rvc_val) || (loc_value < 0))
                    {
                        LOG(E, "Could not convert within range for parameter 0x%08x", parameter);
                        return 0;
                    }
                    *((int32_t *)value) = (int32_t)(loc_value);
                    LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %d", ptr->rvc_type, factor, ptr->rvc_gain, ptr->rvc_offset, *((int32_t *)value));
                    break;
                }
                case RVC_AACDC_UINT32:
                {
                    int64_t loc_value = (int64_t)(*((int32_t *)value));  // Including factor
                    double f_val = (((double)loc_value * (double)ptr->rvc_gain) / (double)factor);
                    f_val += (double)ptr->rvc_offset;

                    if (((int64_t)f_val > (int64_t)ptr->max_rvc_val) || ((int64_t)f_val < 0))
                    {
                        LOG(E, "Could not convert within range for parameter 0x%08x", parameter);
                        return 0;
                    }
                    // Value is within int32_t range, safe to cast
                    *((int32_t *)value) = (int32_t)(f_val);
                    LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %lld", ptr->rvc_type, factor, ptr->rvc_gain, ptr->rvc_offset, *((int64_t *)value));
                    break;
                }
                case RVC_STD_UINT32:
                {
                    uint32_t loc_value = *((uint32_t *)value);  // Including factor
                    float f_val = (((float)loc_value * (float)ptr->rvc_gain) / (float)factor);
                    f_val += (float)ptr->rvc_offset;
                    loc_value = (uint32_t)f_val;
                    if (loc_value > (uint32_t)ptr->max_rvc_val)
                    {
                        LOG(E, "Could not convert within max range for parameter 0x%08x", parameter);
                        return 0;
                    }
                    *((int32_t *)value) = (int32_t)(loc_value);
                    LOG(D, "type: %d, factor : %d rvc_gain : %d rvc_offset : %d value : %u", ptr->rvc_type, factor, ptr->rvc_gain, ptr->rvc_offset, *((uint32_t *)value));
                    break;
                }
                default:
                    LOG(W, "Unsupported RVC data type %d", type);
                    result = 0;
                    break;
                }
            }
        }
    }
    return result;
}
