/*!
    \file   mps.c
    \brief  Mobile Power conversion functions, source
    \author Jens Björnhager
    \author Nenad Radulović
*/

#include <stdint.h>
#include <string.h>

#include "mps.h"

#include "ddm2_parameter_list.h"
#include "ddm_entry.h"
#include "ddm_store.h"
#include "iGeneralDefinitions.h"
#include "sorted_list_ptr.h"
#include "utils.h"

#define MBAT0CHGDET_CUMULATIVE    1
#define MBAT0CAPREM_IS_CALCULATED 1

#define MBAT0CHGDET_DEFAULT 0

#define MCHRG0ALGO_DEFAULT    MCHRG0ALGO_LIFEPO4_1                   // B0 (MPS_B0_CHARGING_PROFILE_LIFEPO4_1)
#define MCHRG0SLNT_DEFAULT    0                                      // B0 (false)
#define MBAT0CAPACITY_DEFAULT 0                                      // B2 (0Ah)
#define MBAT0TYPE_DEFAULT     RVC4DCSRC0TYPE_LITHIUM_IRON_PHOSPHATE  // B2 (MPS_B2_BATTERY_TECHNOLOGY_LITHIO)
#define MCHRG0BVOLT_DEFAULT   16000                                  // B4 (16V)
#define MCHRG0FVOLT_DEFAULT   13600                                  // B4 (13.6V)
#define MCHRG0EQVOLT_DEFAULT  0                                      // B5 (0V)
#define MCHRG0MAXCURR_DEFAULT 105000                                 // B5 (105A)
#define MCHRG0CRVOLT_DEFAULT  0                                      // B5 (0V)

/**
 * \brief       Result of the sum of consecutive entries of a sequence
 *
 * This structure holds the result of sum and the number of summed entries.
 */
typedef struct INT64_SUM
{
    int64_t result;
    size_t nbr_of_entries;
} INT64_SUM;

/**
 * \brief       Parameter calculate callback type
 * \param       mps_context Pointer to MPS context structure
 * \param       parameter Parameter that triggered the conversion
 * \param       instance_start Pointer to array of instances associated with trigger parameter
 */
typedef void (*MPS_PARAMETER_CALCULATE_CB)(const MPS_CONTEXT *const mps_context, const uint32_t parameter, const uint32_t *const instance_start);

static RVC4DCSRC0TYPE_ENUM nbus_b2_to_rvc4dcsrc0type(const B2_BATTERY_TECHNOLOGY b2_battery_technology)
{
    switch (b2_battery_technology)
    {
    case MPS_B2_BATTERY_TECHNOLOGY_WET:
        return RVC4DCSRC0TYPE_FLOODED;
    case MPS_B2_BATTERY_TECHNOLOGY_GEL:
        return RVC4DCSRC0TYPE_GEL;
    case MPS_B2_BATTERY_TECHNOLOGY_AGM:
        return RVC4DCSRC0TYPE_AGM;
    case MPS_B2_BATTERY_TECHNOLOGY_LITHIO:
        return RVC4DCSRC0TYPE_LITHIUM_IRON_PHOSPHATE;
    default:
        return -1;
    }
}

static B2_BATTERY_TECHNOLOGY rvc4dcsrc0type_to_nbus_b2(const RVC4DCSRC0TYPE_ENUM mbatXtype)
{
    switch (mbatXtype)
    {
    case RVC4DCSRC0TYPE_FLOODED:
        return MPS_B2_BATTERY_TECHNOLOGY_WET;
    case RVC4DCSRC0TYPE_GEL:
        return MPS_B2_BATTERY_TECHNOLOGY_GEL;
    case RVC4DCSRC0TYPE_AGM:
        return MPS_B2_BATTERY_TECHNOLOGY_AGM;
    case RVC4DCSRC0TYPE_LITHIUM_IRON_PHOSPHATE:
    default:
        return MPS_B2_BATTERY_TECHNOLOGY_LITHIO;
    }
}

static MCHRG0ALGO_ENUM nbus_b0_to_mchrgXalgo(const MPS_B0_CHARGING_PROFILE b2_charging_profile)
{
    switch (b2_charging_profile)
    {
    case MPS_B0_CHARGING_PROFILE_GEL:
        return MCHRG0ALGO_GEL;
    case MPS_B0_CHARGING_PROFILE_WET:
        return MCHRG0ALGO_WET;
    case MPS_B0_CHARGING_PROFILE_AGM1:
        return MCHRG0ALGO_AGM1;
    case MPS_B0_CHARGING_PROFILE_AGM2:
        return MCHRG0ALGO_AGM2;
    case MPS_B0_CHARGING_PROFILE_LIFEPO4_1:
        return MCHRG0ALGO_LIFEPO4_1;
    case MPS_B0_CHARGING_PROFILE_LIFEPO4_2:
        return MCHRG0ALGO_LIFEPO4_2;
    case MPS_B0_CHARGING_PROFILE_LIFEPO4_3:
        return MCHRG0ALGO_LIFEPO4_3;
    case MPS_B0_CHARGING_PROFILE_LIFEPO4_4:
        return MCHRG0ALGO_LIFEPO4_4;
    case MPS_B0_CHARGING_PROFILE_CUSTOM:
        return MCHRG0ALGO_CUSTOM;
    default:
        return -1;
    }
}

static MCHRG0ALGO_ENUM nbus_26_to_mchrgXalgo(const MPS_26_CHARGING_PROFILE mps_26_charging_profile)
{
    return nbus_b0_to_mchrgXalgo(mps_26_charging_profile);
}

static MPS_B0_CHARGING_PROFILE mchrgXalgo_to_nbus_b0(const MCHRG0ALGO_ENUM mchrgXalgo)
{
    switch (mchrgXalgo)
    {
    case MCHRG0ALGO_GEL:
        return MPS_B0_CHARGING_PROFILE_GEL;
    case MCHRG0ALGO_WET:
        return MPS_B0_CHARGING_PROFILE_WET;
    case MCHRG0ALGO_AGM1:
        return MPS_B0_CHARGING_PROFILE_AGM1;
    case MCHRG0ALGO_AGM2:
        return MPS_B0_CHARGING_PROFILE_AGM2;
    case MCHRG0ALGO_LIFEPO4_1:
        return MPS_B0_CHARGING_PROFILE_LIFEPO4_1;
    case MCHRG0ALGO_LIFEPO4_2:
        return MPS_B0_CHARGING_PROFILE_LIFEPO4_2;
    case MCHRG0ALGO_LIFEPO4_3:
        return MPS_B0_CHARGING_PROFILE_LIFEPO4_3;
    case MCHRG0ALGO_LIFEPO4_4:
        return MPS_B0_CHARGING_PROFILE_LIFEPO4_4;
    case MCHRG0ALGO_CUSTOM:
        return MPS_B0_CHARGING_PROFILE_CUSTOM;
    default:
        return MPS_B0_CHARGING_PROFILE_LIFEPO4_1;
    }
}

//! \~ MPS Device Firmware ID strings
static const SORTED_LIST_PTR_ENTRY Mps_firmware_id__to_desc_strings_list_data[] = {
    {0x010001, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Sun Control 2 - SCE320"},
    {0x010002, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Sun Control 2 with Bluetooth - SCE320B"},
    {0x010003, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Sun Control 2 - SCE360"},
    {0x010004, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Sun Control 2 with Bluetooth - SCE360B"},
    {0x010005, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Sun Control - SC330"},
    {0x010006, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Sun Control - SC480"},
    {0x020001, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Display - DTB"},
    {0x020002, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Display - TD283"},
    {0x030101, (SORTED_LIST_PTR_VALUE_TYPE) "NDS PSB - PSB12-40"},
    {0x030102, (SORTED_LIST_PTR_VALUE_TYPE) "NDS PSB - PSB12-80"},
    {0x030108, (SORTED_LIST_PTR_VALUE_TYPE) "NDS PSB - PSB24/12-80"},
    {0x030182, (SORTED_LIST_PTR_VALUE_TYPE) "Büttner LB - LB12-80"},
    {0x030188, (SORTED_LIST_PTR_VALUE_TYPE) "Büttner LB - LB24/12-80"},
    {0x050001, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 150Ah - TLB150"},
    {0x050002, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 120Ah - TLB120"},
    {0x050003, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 100Ah - TLB100"},
    {0x050004, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 150Ah with heater - TLB150F"},
    {0x050005, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 120Ah with heater - TLB120F"},
    {0x050006, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 100Ah with heater - TLB100F"},
    {0x050008, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Tempra 150Ah - TLB150"},
    {0x050009, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Tempra 150Ah with heater - TLB150F"},
    {0x05000A, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Tempra 100Ah Dometic - TLB100"},
    {0x05000B, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Tempra 100Ah Dometic with heater - TLB100F"},
    {0x05000C, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Tempra 120Ah - TLB120"},
    {0x05000D, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Tempra 120Ah with heater - TLB120F"},
    {0x05000F, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Tempra 500Ah Dometic - TLB500"},
    {0x050010, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 100Ah - TLB100V - VAVE"},
    {0x050011, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 100Ah - TLB100VF - VAVE"},
    {0x050012, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 100Ah - TLB100S - L3"},
    {0x050013, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Tempra 100Ah - TLB100SF - L3"},
    {0x060001, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic MCA+"},
    {0x070001, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Inverter - SPB1000i"},
    {0x070002, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Inverter - SPB1500i"},
    {0x070003, (SORTED_LIST_PTR_VALUE_TYPE) "NDS Inverter - SPB2000i"},
    {0x070007, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Inverter C"},
    {0x070080, (SORTED_LIST_PTR_VALUE_TYPE) "Dometic Inverter E"},
    {0xc6ffff, (SORTED_LIST_PTR_VALUE_TYPE) "AC Charger (CI-BUS)"},
};

static SORTED_LIST_PTR Mps_firmware_id_to_desc_strings_list = {
    .pdata = (SORTED_LIST_PTR_ENTRY *)Mps_firmware_id__to_desc_strings_list_data,
    .capacity = ELEMENTS(Mps_firmware_id__to_desc_strings_list_data),
    .entry_count = ELEMENTS(Mps_firmware_id__to_desc_strings_list_data),
};

/**
 * \brief       Lookup description string from MPS firmware ID
 * \param       firmware_id MPS Firmware ID to lookup
 * \return      Pointer to Description string or NULL if not found
 */
const char *lookup_description_from_mps_firmware_id_string(const uint32_t firmware_id)
{
    const char *desc_firmware_id_string = "";

    sorted_list_ptr_unique_get((SORTED_LIST_PTR_VALUE_TYPE *)&desc_firmware_id_string, &Mps_firmware_id_to_desc_strings_list, firmware_id, 0);

    return desc_firmware_id_string;
}

//! \~ MPS Device Firmware ID strings
static const SORTED_LIST_PTR_ENTRY Mps_firmware_id_conv_list_data[] = {
    {0x010001, (SORTED_LIST_PTR_VALUE_TYPE) "ABM"},
    {0x010002, (SORTED_LIST_PTR_VALUE_TYPE) "ABN"},
    {0x010003, (SORTED_LIST_PTR_VALUE_TYPE) "ABO"},
    {0x010004, (SORTED_LIST_PTR_VALUE_TYPE) "ABP"},
    {0x010005, (SORTED_LIST_PTR_VALUE_TYPE) "ACD"},
    {0x010006, (SORTED_LIST_PTR_VALUE_TYPE) "ACE"},
    {0x020001, (SORTED_LIST_PTR_VALUE_TYPE) "ABQ"},
    {0x020002, (SORTED_LIST_PTR_VALUE_TYPE) "ACC"},
    {0x030101, (SORTED_LIST_PTR_VALUE_TYPE) "ABR"},
    {0x030102, (SORTED_LIST_PTR_VALUE_TYPE) "ABS"},
    {0x030108, (SORTED_LIST_PTR_VALUE_TYPE) "ABY"},
    {0x030182, (SORTED_LIST_PTR_VALUE_TYPE) "KAG"},
    {0x030188, (SORTED_LIST_PTR_VALUE_TYPE) "KAH"},
    {0x050001, (SORTED_LIST_PTR_VALUE_TYPE) "HAE"},
    {0x050002, (SORTED_LIST_PTR_VALUE_TYPE) "HAC"},
    {0x050003, (SORTED_LIST_PTR_VALUE_TYPE) "HAA"},
    {0x050004, (SORTED_LIST_PTR_VALUE_TYPE) "HAF"},
    {0x050005, (SORTED_LIST_PTR_VALUE_TYPE) "HAD"},
    {0x050006, (SORTED_LIST_PTR_VALUE_TYPE) "HAB"},
    {0x050008, (SORTED_LIST_PTR_VALUE_TYPE) "KAA"},
    {0x050009, (SORTED_LIST_PTR_VALUE_TYPE) "KAB"},
    {0x05000A, (SORTED_LIST_PTR_VALUE_TYPE) "KAE"},
    {0x05000B, (SORTED_LIST_PTR_VALUE_TYPE) "KAF"},
    {0x05000C, (SORTED_LIST_PTR_VALUE_TYPE) "KAC"},
    {0x05000D, (SORTED_LIST_PTR_VALUE_TYPE) "KAD"},
    {0x05000F, (SORTED_LIST_PTR_VALUE_TYPE) "KAX"},
    {0x050010, (SORTED_LIST_PTR_VALUE_TYPE) "HAH"},
    {0x050011, (SORTED_LIST_PTR_VALUE_TYPE) "HAI"},
    {0x050012, (SORTED_LIST_PTR_VALUE_TYPE) "HAJ"},
    {0x050013, (SORTED_LIST_PTR_VALUE_TYPE) "HAK"},
    {0x060001, (SORTED_LIST_PTR_VALUE_TYPE) "MPA"},
    {0x070001, (SORTED_LIST_PTR_VALUE_TYPE) "BBA"},
    {0x070002, (SORTED_LIST_PTR_VALUE_TYPE) "BBB"},
    {0x070003, (SORTED_LIST_PTR_VALUE_TYPE) "BBC"},
    {0x070007, (SORTED_LIST_PTR_VALUE_TYPE) "BCV"},
    {0x070080, (SORTED_LIST_PTR_VALUE_TYPE) "BCA"},
    {0xc6ffff, (SORTED_LIST_PTR_VALUE_TYPE) "IAA"},
};

static SORTED_LIST_PTR Mps_firmware_id_conv_string_list = {
    .pdata = (SORTED_LIST_PTR_ENTRY *)Mps_firmware_id_conv_list_data,
    .capacity = ELEMENTS(Mps_firmware_id_conv_list_data),
    .entry_count = ELEMENTS(Mps_firmware_id_conv_list_data),
};

/**
 * \brief       Lookup Dometic firmware ID string from MPS firmware ID
 * \param       firmware_id MPS Firmware ID to lookup
 * \return      Pointer to Firmware ID string or NULL if not found
 */
const char *lookup_mps_firmware_id_string(const uint32_t firmware_id)
{
    const char *firmware_id_string = "";

    sorted_list_ptr_unique_get((SORTED_LIST_PTR_VALUE_TYPE *)&firmware_id_string, &Mps_firmware_id_conv_string_list, firmware_id, 0);

    return firmware_id_string;
}

// Simulated/system setting
static void f_80b0(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);
    (void)instance_start;  // NULL pointers

    DDMP2_FRAME frame;
    const MPS_NBUS_B0 *Nbus_b0 = &nbus_realtime_data->nbus_b0;

    const int32_t Ddm2_profile = nbus_b0_to_mchrgXalgo(Nbus_b0->charging_profile);
    MPS_GENERIC_FRAME_DATA generic_frame = {
        .device_address = nbus_realtime_data->device_address << 8 | nbus_realtime_data->sub_address,
        .i32data[0] = Ddm2_profile,
    };

    ddmp2_create_generic(&frame, MPS_GENERIC_FRAME_SETTING_CHARGE_PROFILE, &generic_frame, sizeof(MPS_GENERIC_FRAME_DATA), -1);

    complete_cb(&frame, context);
}

static void f_80b3(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);
    (void)instance_start;  // NULL pointers

    DDMP2_FRAME frame;
    const MPS_NBUS_B3 *const Nbus_b3 = &nbus_realtime_data->nbus_b3;
    const int32_t Ddm2_power = (Nbus_b3->panel_power1) + (Nbus_b3->panel_power2);

    MPS_GENERIC_FRAME_DATA generic_frame = {
        .device_address = nbus_realtime_data->device_address << 8 | nbus_realtime_data->sub_address,
        .i32data[0] = Ddm2_power,
    };
    ddmp2_create_generic(&frame, MPS_GENERIC_FRAME_SETTING_PANEL_POWER, &generic_frame, sizeof(MPS_GENERIC_FRAME_DATA), -1);
    complete_cb(&frame, context);
}

// 81
static void f_8101(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_01 *const Nbus_01 = &nbus_realtime_data->nbus_01;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0IVOLT;

    if (Nbus_01->voltage == 0xffff)  // No voltage available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const int32_t Ddm2_v = Nbus_01->voltage * (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 100);
        ddmp2_create_publish(&frame, instance, &Ddm2_v, sizeof(Ddm2_v), -1);
    }

    complete_cb(&frame, context);
}

static void f_8102(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_02 *const Nbus_02 = &nbus_realtime_data->nbus_02;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0CVOLT;

    if (Nbus_02->voltage == 0xffff)  // No voltage available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const int32_t Ddm2_v = Nbus_02->voltage * (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 100);
        ddmp2_create_publish(&frame, instance, &Ddm2_v, sizeof(Ddm2_v), -1);
    }

    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0CCURR;

    if (Nbus_02->current == 0xffff)  // No current available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const uint16_t Nbus_i = Nbus_02->current;
        const int32_t Sign = Nbus_i & 0x8000;
        const int32_t Magnitude = Nbus_i & 0x7fff;
        const int32_t Scale_differece = Ddm2_unit_factor_list[DDM2_UNIT_AMPERE] / 100;
        const int32_t Ddm2_i = (Sign ? -Scale_differece : Scale_differece) * Magnitude;
        ddmp2_create_publish(&frame, instance, &Ddm2_i, sizeof(Ddm2_i), -1);
    }

    complete_cb(&frame, context);
}

static void f_810c(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_0C *const Nbus_0c = &nbus_realtime_data->nbus_0c;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0EXTTEMP;

    if (Nbus_0c->temperature == 0xffff)  // No temperature available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const int32_t Ddm2_temperature = (Nbus_0c->temperature - 500) * (Ddm2_unit_factor_list[DDM2_UNIT_DEGC] / 10);
        ddmp2_create_publish(&frame, instance, &Ddm2_temperature, sizeof(Ddm2_temperature), -1);
    }

    complete_cb(&frame, context);
}

static void f_8110(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_10 *const Nbus_10 = &nbus_realtime_data->nbus_10;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0TEMP;

    if (Nbus_10->temperature == 0xffff)  // No temperature available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const int32_t Ddm2_temperature = (Nbus_10->temperature - 500) * (Ddm2_unit_factor_list[DDM2_UNIT_DEGC] / 10);
        ddmp2_create_publish(&frame, instance, &Ddm2_temperature, sizeof(Ddm2_temperature), -1);
    }

    complete_cb(&frame, context);
}

static void f_8126(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_26 *const Nbus_26 = &nbus_realtime_data->nbus_26;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0ALGO;
    const int32_t Ddm2_charging_profile = nbus_26_to_mchrgXalgo(Nbus_26->charging_profile);
    ddmp2_create_publish(&frame, instance, &Ddm2_charging_profile, sizeof(Ddm2_charging_profile), -1);
    complete_cb(&frame, context);
}

static void f_8135(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_35 *const Nbus_35 = &nbus_realtime_data->nbus_35;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRGH].value | MCHRGH0TODAY;
    const int32_t Ddm2_wh = Nbus_35->energy;
    ddmp2_create_publish(&frame, instance, &Ddm2_wh, sizeof(Ddm2_wh), -1);
    complete_cb(&frame, context);
}

static void f_8154(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    const MPS_NBUS_54 *const Nbus_54 = &nbus_realtime_data->nbus_54;

    MPS_GENERIC_FRAME_DATA generic_frame = {
        .device_address = nbus_realtime_data->device_address << 8 | nbus_realtime_data->sub_address,
    };

    generic_frame.string[0] = Nbus_54->serial_number_prefix[2];
    generic_frame.string[1] = Nbus_54->serial_number_prefix[1];
    generic_frame.string[2] = Nbus_54->serial_number_prefix[0];
    generic_frame.string[3] = '\0';
    DDMP2_FRAME frame;

    ddmp2_create_generic(&frame, MPS_GENERIC_FRAME_DATA_SERIAL_NUMBER_PREFIX, &generic_frame, sizeof(MPS_GENERIC_FRAME_DATA), -1);
    complete_cb(&frame, context);
}

static void f_8155(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    const MPS_NBUS_55 *const Nbus_55 = &nbus_realtime_data->nbus_55;

    MPS_GENERIC_FRAME_DATA generic_frame = {
        .device_address = nbus_realtime_data->device_address << 8 | nbus_realtime_data->sub_address,
    };

    snprintf(generic_frame.string, ELEMENTS(generic_frame.string), "%d", Nbus_55->serial_number);

    DDMP2_FRAME frame;
    ddmp2_create_generic(&frame, MPS_GENERIC_FRAME_DATA_SERIAL_NUMBER, &generic_frame, sizeof(MPS_GENERIC_FRAME_DATA), -1);
    complete_cb(&frame, context);
}

static void f_8160(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_60 *const Nbus_60 = &nbus_realtime_data->nbus_60;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0ENA;
    const int32_t Ddm2_charger_enabled = Nbus_60->status;
    ddmp2_create_publish(&frame, instance, &Ddm2_charger_enabled, sizeof(Ddm2_charger_enabled), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0SLNT;
    const int32_t Ddm2_silent_mode = Nbus_60->silent_mode;
    ddmp2_create_publish(&frame, instance, &Ddm2_silent_mode, sizeof(Ddm2_silent_mode), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0OPER;
    const int32_t Ddm2_charging_phase = Nbus_60->charging_phase;
    ddmp2_create_publish(&frame, instance, &Ddm2_charging_phase, sizeof(Ddm2_charging_phase), -1);
    complete_cb(&frame, context);
}

static void f_81b0(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_B0 *Nbus_b0 = &nbus_realtime_data->nbus_b0;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MCHRG].value | MCHRG0ALGO;
    const int32_t Ddm2_profile = nbus_b0_to_mchrgXalgo(Nbus_b0->charging_profile);
    ddmp2_create_publish(&frame, instance, &Ddm2_profile, sizeof(Ddm2_profile), -1);
    complete_cb(&frame, context);
}

static void f_81b3(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_B3 *const Nbus_b3 = &nbus_realtime_data->nbus_b3;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_81_MSOLAR].value | MSOLAR0PPWR;
    const int32_t Ddm2_power = ((Nbus_b3->panel_power1) + (Nbus_b3->panel_power2)) * Ddm2_unit_factor_list[DDM2_UNIT_WATT];
    ddmp2_create_publish(&frame, instance, &Ddm2_power, sizeof(Ddm2_power), -1);
    complete_cb(&frame, context);
}

// 85

static void f_8502(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_02 *const Nbus_02 = &nbus_realtime_data->nbus_02;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0VOLT;

    if (Nbus_02->voltage == 0xffff)  // No voltage available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const int32_t Ddm2_v = Nbus_02->voltage * (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 100);
        ddmp2_create_publish(&frame, instance, &Ddm2_v, sizeof(Ddm2_v), -1);
    }

    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0CURR;

    if (Nbus_02->current == 0xffff)  // No current available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const uint16_t Nbus_i = Nbus_02->current;
        const int32_t Sign = Nbus_i & 0x8000;
        const int32_t Magnitude = Nbus_i & 0x7fff;
        const int32_t Scale_differece = Ddm2_unit_factor_list[DDM2_UNIT_AMPERE] / 100;
        const int32_t Ddm2_i = (Sign ? -Scale_differece : Scale_differece) * Magnitude;
        ddmp2_create_publish(&frame, instance, &Ddm2_i, sizeof(Ddm2_i), -1);
    }

    complete_cb(&frame, context);
}

static void f_8507(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    // When this is received I will not get b2, so type is set from here to Litium (valid for the Tempra)
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_07 *const Nbus_07 = &nbus_realtime_data->nbus_07;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0CAPACITY;
    const int32_t Nbus_Ah = Nbus_07->leisure_capacity;
    ddmp2_create_publish(&frame, instance, &Nbus_Ah, sizeof(Nbus_Ah), -1);
    complete_cb(&frame, context);
    instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0TYPE;
    const int32_t Batt_type = MBAT0TYPE_DEFAULT;
    ddmp2_create_publish(&frame, instance, &Batt_type, sizeof(Batt_type), -1);
    complete_cb(&frame, context);
}

static void f_850b(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_0B *const Nbus_0b = &nbus_realtime_data->nbus_0b;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0SOC;
    const int32_t Ddm2_soc = Nbus_0b->soc;
    ddmp2_create_publish(&frame, instance, &Ddm2_soc, sizeof(Ddm2_soc), -1);
    complete_cb(&frame, context);
}

static void f_850c(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_0C *const Nbus_0c = &nbus_realtime_data->nbus_0c;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0TEMP;

    if (Nbus_0c->temperature == 0xffff)  // No temperature available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const int32_t Ddm2_temperature = (Nbus_0c->temperature - 500) * Ddm2_unit_factor_list[DDM2_UNIT_DEGC] / 10;
        ddmp2_create_publish(&frame, instance, &Ddm2_temperature, sizeof(Ddm2_temperature), -1);
    }

    complete_cb(&frame, context);
}

static void f_850e(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_0E *const Nbus_0e = &nbus_realtime_data->nbus_0e;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0SOH;
    const int32_t Ddm2_battery_health = Nbus_0e->battery_health;
    ddmp2_create_publish(&frame, instance, &Ddm2_battery_health, sizeof(Ddm2_battery_health), -1);
    complete_cb(&frame, context);
}

#if (MBAT0CAPREM_IS_CALCULATED != 1)
static void f_8536(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_36 *const Nbus_36 = &nbus_realtime_data->nbus_36;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0CAPREM;
    const int32_t Ddm2_wh = Nbus_36->capacity_remaining;
    ddmp2_create_publish(&frame, instance, &Ddm2_wh, sizeof(Ddm2_wh), -1);
    complete_cb(&frame, context);
}
#endif

static void f_8560(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_8560 *const Nbus_8560 = &nbus_realtime_data->nbus_8560;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0POLES;
    const int32_t Ddm2_status = (Nbus_8560->poles & 0x1) ^ (Nbus_8560->poles & 0x2);
    ddmp2_create_publish(&frame, instance, &Ddm2_status, sizeof(Ddm2_status), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0DCDC;

    const int32_t Ddm2_regulator_status = Nbus_8560->dcdc;
    ddmp2_create_publish(&frame, instance, &Ddm2_regulator_status, sizeof(Ddm2_regulator_status), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0HTR;

    const int32_t Ddm2_heater_status = Nbus_8560->htr;
    ddmp2_create_publish(&frame, instance, &Ddm2_heater_status, sizeof(Ddm2_heater_status), -1);
    complete_cb(&frame, context);
}

static void f_85b2(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_B2 *const Nbus_b2 = &nbus_realtime_data->nbus_b2;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0CAPACITY;
    const int32_t Ddm2_capacity = Nbus_b2->battery_capacity * Ddm2_unit_factor_list[DDM2_UNIT_AMPHOURS];
    ddmp2_create_publish(&frame, instance, &Ddm2_capacity, sizeof(Ddm2_capacity), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_85_MBAT].value | MBAT0TYPE;
    const int32_t Ddm2_battery_type = nbus_b2_to_rvc4dcsrc0type(Nbus_b2->battery_technology);

    ddmp2_create_publish(&frame, instance, &Ddm2_battery_type, sizeof(Ddm2_battery_type), -1);
    complete_cb(&frame, context);
}

// 87

static void f_8702(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_02 *const Nbus_02 = &nbus_realtime_data->nbus_02;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_87_MINVERT].value | MINVERT0VOLT;

    if (Nbus_02->voltage == 0xffff)  // No voltage available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const int32_t Ddm2_v = Nbus_02->voltage * (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 100);
        ddmp2_create_publish(&frame, instance, &Ddm2_v, sizeof(Ddm2_v), -1);
    }

    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_87_MINVERT].value | MINVERT0CURR;

    if (Nbus_02->current == 0xffff)  // No current available
    {
        ddmp2_create_publish(&frame, instance, NULL, 0, -1);
    }
    else
    {
        const uint16_t Nbus_i = Nbus_02->current;
        const int32_t Sign = Nbus_i & 0x8000;
        const int32_t Magnitude = Nbus_i & 0x7fff;
        const int32_t Scale_differece = Ddm2_unit_factor_list[DDM2_UNIT_AMPERE] / 100;
        const int32_t Ddm2_i = (Sign ? -Scale_differece : Scale_differece) * Magnitude;
        ddmp2_create_publish(&frame, instance, &Ddm2_i, sizeof(Ddm2_i), -1);
    }

    complete_cb(&frame, context);
}

static void f_8720(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_20 *const Nbus_20 = &nbus_realtime_data->nbus_20;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_87_MINVERT].value | MINVERT0OUTVOLT;
    const int32_t Ddm2_v = (Nbus_20->output_voltage_ac + 102) * Ddm2_unit_factor_list[DDM2_UNIT_VOLT];
    ddmp2_create_publish(&frame, instance, &Ddm2_v, sizeof(Ddm2_v), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_87_MINVERT].value | MINVERT0LOADSENSE;
    const int32_t Ddm2_eco_mode = Nbus_20->eco_mode;
    ddmp2_create_publish(&frame, instance, &Ddm2_eco_mode, sizeof(Ddm2_eco_mode), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_87_MINVERT].value | MINVERT0WRKLD;
    const int32_t Ddm2_workload = Nbus_20->workload * Ddm2_unit_factor_list[DDM2_UNIT_PART];
    ddmp2_create_publish(&frame, instance, &Ddm2_workload, sizeof(Ddm2_workload), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_87_MINVERT].value | MINVERT0PASSTHROUGH;
    const int32_t Ddm2_passthrough = Nbus_20->passthrough;
    ddmp2_create_publish(&frame, instance, &Ddm2_passthrough, sizeof(Ddm2_passthrough), -1);
    complete_cb(&frame, context);
}

static void f_8760(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_60 *const Nbus_60 = &nbus_realtime_data->nbus_60;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_87_MINVERT].value | MINVERT0ENABLE;
    const int32_t Ddm2_inverter_enabled = Nbus_60->status;
    ddmp2_create_publish(&frame, instance, &Ddm2_inverter_enabled, sizeof(Ddm2_inverter_enabled), -1);
    complete_cb(&frame, context);
}

// c6

static void f_c699(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    DDMP2_FRAME frame;
    const MPS_NBUS_99 *const Nbus_99 = &nbus_realtime_data->nbus_99;

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_C6_MCHRG].value | MCHRG0SLNT;
    const int32_t Ddm2_silent_mode = Nbus_99->silent_mode;
    ddmp2_create_publish(&frame, instance, &Ddm2_silent_mode, sizeof(Ddm2_silent_mode), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_C6_MCHRG].value | MCHRG0DERST;
    const int32_t Ddm2_reduced_power = Nbus_99->reduced_power;
    ddmp2_create_publish(&frame, instance, &Ddm2_reduced_power, sizeof(Ddm2_reduced_power), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_C6_MCHRG].value | MCHRG0OPER;
    const int32_t Ddm2_error = Nbus_99->error;
    ddmp2_create_publish(&frame, instance, &Ddm2_error, sizeof(Ddm2_error), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_C6_MCHRG].value | MCHRG0ENA;
    const int32_t Ddm2_charging_active = Nbus_99->charging_active;
    ddmp2_create_publish(&frame, instance, &Ddm2_charging_active, sizeof(Ddm2_charging_active), -1);
    complete_cb(&frame, context);

    instance = instance_start[MPS_INSTANCE_INDEX_C6_MCHRG].value | MCHRG0CCURR;
    const int32_t Ddm2_i = Nbus_99->current * (Ddm2_unit_factor_list[DDM2_UNIT_AMPERE] / 5);
    ddmp2_create_publish(&frame, instance, &Ddm2_i, sizeof(Ddm2_i), -1);
    complete_cb(&frame, context);
}

// 82
static void f_82a1(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    const MPS_NBUS_A1 *const Nbus_a1 = &nbus_realtime_data->nbus_a1;

    MPS_GENERIC_FRAME_DATA generic_frame = {
        .device_address = nbus_realtime_data->device_address << 8 | nbus_realtime_data->sub_address,
    };

    snprintf(generic_frame.string, ELEMENTS(generic_frame.string), "%d.%d.0", (Nbus_a1->firmware_version >> 8) & 0xFF, Nbus_a1->firmware_version & 0xFF);
    DDMP2_FRAME frame;
    ddmp2_create_generic(&frame, MPS_GENERIC_FRAME_DATA_FIRMWARE_VERSION, &generic_frame, sizeof(MPS_GENERIC_FRAME_DATA), -1);
    complete_cb(&frame, context);
}

static void f_82a2(const MPS_NBUS_REALTIME_DATA *const nbus_realtime_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(complete_cb != NULL);

    const MPS_NBUS_A2 *const Nbus_a2 = &nbus_realtime_data->nbus_a2;

    char version[8];
    const int Version_length = snprintf(version, sizeof(version), "%d.%d.0", (Nbus_a2->firmware_version >> 8) & 0xFF, Nbus_a2->firmware_version & 0xFF);

    uint32_t instance = instance_start[MPS_INSTANCE_INDEX_82_HMI].value | HMI0VER;

    DDMP2_FRAME frame;
    ddmp2_create_publish(&frame, instance, version, Version_length, -1);
    complete_cb(&frame, context);
}

//! \~ N-BUS to DDM2 conversion callback lookup list
static const SORTED_LIST_PTR_ENTRY Data_type_lookup_list_data[] = {
    {0x80b0, (SORTED_LIST_PTR_VALUE_TYPE)f_80b0},
    {0x80b3, (SORTED_LIST_PTR_VALUE_TYPE)f_80b3},
    {0x8101, (SORTED_LIST_PTR_VALUE_TYPE)f_8101},
    {0x8102, (SORTED_LIST_PTR_VALUE_TYPE)f_8102},
    {0x810c, (SORTED_LIST_PTR_VALUE_TYPE)f_810c},
    {0x8110, (SORTED_LIST_PTR_VALUE_TYPE)f_8110},
    {0x8126, (SORTED_LIST_PTR_VALUE_TYPE)f_8126},
    {0x8135, (SORTED_LIST_PTR_VALUE_TYPE)f_8135},
    {0x8154, (SORTED_LIST_PTR_VALUE_TYPE)f_8154},
    {0x8155, (SORTED_LIST_PTR_VALUE_TYPE)f_8155},
    {0x8160, (SORTED_LIST_PTR_VALUE_TYPE)f_8160},
    {0x81b0, (SORTED_LIST_PTR_VALUE_TYPE)f_81b0},
    {0x81b3, (SORTED_LIST_PTR_VALUE_TYPE)f_81b3},
    {0x8254, (SORTED_LIST_PTR_VALUE_TYPE)f_8154},
    {0x8255, (SORTED_LIST_PTR_VALUE_TYPE)f_8155},
    {0x82a1, (SORTED_LIST_PTR_VALUE_TYPE)f_82a1},
    {0x82a2, (SORTED_LIST_PTR_VALUE_TYPE)f_82a2},
    {0x8301, (SORTED_LIST_PTR_VALUE_TYPE)f_8101},
    {0x8302, (SORTED_LIST_PTR_VALUE_TYPE)f_8102},
    {0x8310, (SORTED_LIST_PTR_VALUE_TYPE)f_8110},
    {0x8311, (SORTED_LIST_PTR_VALUE_TYPE)f_8110},
    {0x8326, (SORTED_LIST_PTR_VALUE_TYPE)f_8126},
    {0x8335, (SORTED_LIST_PTR_VALUE_TYPE)f_8135},
    {0x8354, (SORTED_LIST_PTR_VALUE_TYPE)f_8154},
    {0x8355, (SORTED_LIST_PTR_VALUE_TYPE)f_8155},
    {0x8360, (SORTED_LIST_PTR_VALUE_TYPE)f_8160},
    {0x8502, (SORTED_LIST_PTR_VALUE_TYPE)f_8502},
    {0x8507, (SORTED_LIST_PTR_VALUE_TYPE)f_8507},
    {0x850b, (SORTED_LIST_PTR_VALUE_TYPE)f_850b},
    {0x850c, (SORTED_LIST_PTR_VALUE_TYPE)f_850c},
    {0x850e, (SORTED_LIST_PTR_VALUE_TYPE)f_850e},
#if (MBAT0CAPREM_IS_CALCULATED != 1)
    {0x8536, (SORTED_LIST_PTR_VALUE_TYPE)f_8536},
#endif
    {0x8554, (SORTED_LIST_PTR_VALUE_TYPE)f_8154},
    {0x8555, (SORTED_LIST_PTR_VALUE_TYPE)f_8155},
    {0x8560, (SORTED_LIST_PTR_VALUE_TYPE)f_8560},
    {0x85b2, (SORTED_LIST_PTR_VALUE_TYPE)f_85b2},
    {0x8602, (SORTED_LIST_PTR_VALUE_TYPE)f_8102},
    {0x8626, (SORTED_LIST_PTR_VALUE_TYPE)f_8126},
    {0x8654, (SORTED_LIST_PTR_VALUE_TYPE)f_8154},
    {0x8655, (SORTED_LIST_PTR_VALUE_TYPE)f_8155},
    {0x8660, (SORTED_LIST_PTR_VALUE_TYPE)f_8160},
    {0x8702, (SORTED_LIST_PTR_VALUE_TYPE)f_8702},
    {0x8720, (SORTED_LIST_PTR_VALUE_TYPE)f_8720},
    {0x8754, (SORTED_LIST_PTR_VALUE_TYPE)f_8154},
    {0x8755, (SORTED_LIST_PTR_VALUE_TYPE)f_8155},
    {0x8760, (SORTED_LIST_PTR_VALUE_TYPE)f_8760},
    {0xc699, (SORTED_LIST_PTR_VALUE_TYPE)f_c699},
};

static SORTED_LIST_PTR Data_type_lookup_list = {
    // [nbus address, data type] -> [NBUS->DDM2 conversion callback]
    .pdata = (SORTED_LIST_PTR_ENTRY *)Data_type_lookup_list_data,
    .capacity = ELEMENTS(Data_type_lookup_list_data),
    .entry_count = ELEMENTS(Data_type_lookup_list_data),
};

/*! \brief Perform NBUS to DDM2 conversion
    \param nbus Pointer to NBUS data
    \param instance_start Pointer to DDM2 instance start
    \param complete_cb Callback function for completion
    \param context Callback context
    \return 0
*/
int mps_nbus_to_ddm2(const MPS_NBUS_REALTIME_DATA *const nbus_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context)
{
    const SORTED_LIST_PTR_KEY_TYPE Data_type = (nbus_data->device_address << 8) | nbus_data->data_type;
    SORTED_LIST_PTR_VALUE_TYPE frame_cb;

    const SORTED_LIST_PTR_RETURN_VALUE Get_result = sorted_list_ptr_unique_get(&frame_cb, &Data_type_lookup_list, Data_type, 0);  // Find conversion function for N-BUS frame data type

    if (((SORTED_LIST_PTR_RETURN_VALUE)Get_result == SORTED_LIST_PTR_OK) && (frame_cb != 0))
    {
        ((NBUS_DDM2_CONVERT_CB)frame_cb)(nbus_data, mps_context, instance_start, complete_cb, context);  // Convert N-BUS frame to DDM2 frame
    }

    return 0;
};

// SET

static void f_b0(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(mps_context != NULL);
    TRUE_CHECK_RETURN(complete_cb != NULL);

    ddm_entry_t *mchrgXslnt, *mchrgXalgo;
    int32_t mchrgXslnt_value, mchrgXalgo_value;
    const int32_t Instance = DDM2_PARAMETER_INSTANCE_PART(pframe->frame.set.parameter);

    switch (pframe->frame.set.parameter)
    {
    case MCHRG0SLNT:
        mchrgXslnt_value = pframe->frame.set.value.int32;
        mchrgXalgo = ddm_store__access(mps_context->mps_store, MCHRG0ALGO | Instance);
        mchrgXalgo_value = mchrgXalgo ? ddm_entry__value_i32(mchrgXalgo) : MCHRG0ALGO_DEFAULT;
        break;
    case MCHRG0ALGO:
        mchrgXalgo_value = pframe->frame.set.value.int32;
        mchrgXslnt = ddm_store__access(mps_context->mps_store, MCHRG0SLNT | Instance);
        mchrgXslnt_value = mchrgXslnt ? ddm_entry__value_i32(mchrgXslnt) : MCHRG0SLNT_DEFAULT;
        break;
    default:
        return;
    }

    MPS_NBUS_WRITE_FRAME write_imp_b0 = {
        .frame_type = MPS_NBUS_WRITE_FRAME_TYPE_IMP,
        .imp_frame = {
            .command = "APP+IMP=",
            .data_type = 0xb0,
            .nbus_b0 = {
                .ble_on = 1,
                .language = MPS_B0_LANGUAGE_ENGLISH,
                .device_state = MPS_B0_DEVICE_STATE_RUN,
                .silent_mode = mchrgXslnt_value,
                .charging_profile = mchrgXalgo_to_nbus_b0(mchrgXalgo_value),
            },
        },
    };

    complete_cb(&write_imp_b0, context);
}

// b1 has no input parameter

static void f_b2(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(mps_context != NULL);
    TRUE_CHECK_RETURN(complete_cb != NULL);

    ddm_entry_t *mbatXtype, *mbatXcapacity;
    int32_t mbatXtype_value, mbatXcapacity_value;
    const int32_t Instance = DDM2_PARAMETER_INSTANCE_PART(pframe->frame.set.parameter);

    switch (pframe->frame.set.parameter)
    {
    case MBAT0TYPE:
        mbatXtype_value = pframe->frame.set.value.int32;
        mbatXcapacity = ddm_store__access(mps_context->mps_store, MBAT0CAPACITY | Instance);
        mbatXcapacity_value = (mbatXcapacity != NULL) ? ddm_entry__value_i32(mbatXcapacity) : MBAT0CAPACITY_DEFAULT;
        break;
    case MBAT0CAPACITY:
        mbatXcapacity_value = pframe->frame.set.value.int32;
        mbatXtype = ddm_store__access(mps_context->mps_store, MBAT0TYPE | Instance);
        mbatXtype_value = (mbatXtype != NULL) ? ddm_entry__value_i32(mbatXtype) : MBAT0TYPE_DEFAULT;
        break;
    default:
        return;
    }

    MPS_NBUS_WRITE_FRAME write_imp_b2 = {
        .frame_type = MPS_NBUS_WRITE_FRAME_TYPE_IMP,
        .imp_frame = {
            .command = "APP+IMP=",
            .data_type = 0xb2,
            .nbus_b2 = {
                .battery_technology = rvc4dcsrc0type_to_nbus_b2(mbatXtype_value),
                .battery_capacity = mbatXcapacity_value / Ddm2_unit_factor_list[DDM2_UNIT_AMPHOURS],
            },
        },
    };

    complete_cb(&write_imp_b2, context);
}

static void f_b3(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(complete_cb != NULL);

    const unsigned int Average_power = (unsigned int)(pframe->frame.set.value.int32 / (2 * Ddm2_unit_factor_list[DDM2_UNIT_WATT]));

    MPS_NBUS_WRITE_FRAME write_imp_b3 = {
        .frame_type = MPS_NBUS_WRITE_FRAME_TYPE_IMP,
        .imp_frame = {
            .command = "APP+IMP=",
            .data_type = 0xb3,
            .nbus_b3 = {
                .panel_power1 = Average_power,
                .panel_power2 = Average_power,
            },
        },
    };

    complete_cb(&write_imp_b3, context);
}

static void f_b4(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(mps_context != NULL);
    TRUE_CHECK_RETURN(complete_cb != NULL);

    ddm_entry_t *mchrg0bvolt, *mchrg0fvolt;
    int32_t mchrg0bvolt_value, mchrg0fvolt_value;
    const int32_t Instance = DDM2_PARAMETER_INSTANCE_PART(pframe->frame.set.parameter);

    switch (pframe->frame.set.parameter)
    {
    case MCHRG0BVOLT:
        mchrg0bvolt_value = pframe->frame.set.value.int32;
        mchrg0fvolt = ddm_store__access(mps_context->mps_store, MCHRG0FVOLT | Instance);
        mchrg0fvolt_value = (mchrg0fvolt != NULL) ? ddm_entry__value_i32(mchrg0fvolt) : MCHRG0FVOLT_DEFAULT;
        break;
    case MCHRG0FVOLT:
        mchrg0fvolt_value = pframe->frame.set.value.int32;
        mchrg0bvolt = ddm_store__access(mps_context->mps_store, MCHRG0BVOLT | Instance);
        mchrg0bvolt_value = (mchrg0bvolt != NULL) ? ddm_entry__value_i32(mchrg0bvolt) : MCHRG0BVOLT_DEFAULT;
        break;
    default:
        return;
    }

    MPS_NBUS_WRITE_FRAME write_imp_b4 = {
        .frame_type = MPS_NBUS_WRITE_FRAME_TYPE_IMP,
        .imp_frame = {
            .command = "APP+IMP=",
            .data_type = 0xb4,
            .nbus_b4 = {
                .vabs = mchrg0bvolt_value / (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 10),
                .vfloat = mchrg0fvolt_value / (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 10),
            },
        },
    };

    complete_cb(&write_imp_b4, context);
}

static void f_b5(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(mps_context != NULL);
    TRUE_CHECK_RETURN(complete_cb != NULL);

    ddm_entry_t *mchrgXcrvolt, *mchrgXeqvolt, *mchrgXmaxcurr;
    int32_t mchrgXcrvolt_value, mchrgXeqvolt_value, mchrgXmaxcurr_value;
    const int32_t Instance = DDM2_PARAMETER_INSTANCE_PART(pframe->frame.set.parameter);

    switch (pframe->frame.set.parameter)
    {
    case MCHRG0CRVOLT:
        mchrgXcrvolt_value = pframe->frame.set.value.int32;
        mchrgXeqvolt = ddm_store__access(mps_context->mps_store, MCHRG0EQVOLT | Instance);
        mchrgXmaxcurr = ddm_store__access(mps_context->mps_store, MCHRG0MAXCURR | Instance);
        mchrgXeqvolt_value = (mchrgXeqvolt != NULL) ? ddm_entry__value_i32(mchrgXeqvolt) : MCHRG0EQVOLT_DEFAULT;
        mchrgXmaxcurr_value = (mchrgXmaxcurr != NULL) ? ddm_entry__value_i32(mchrgXmaxcurr) : MCHRG0MAXCURR_DEFAULT;
        break;
    case MCHRG0EQVOLT:
        mchrgXeqvolt_value = pframe->frame.set.value.int32;
        mchrgXcrvolt = ddm_store__access(mps_context->mps_store, MCHRG0CRVOLT | Instance);
        mchrgXmaxcurr = ddm_store__access(mps_context->mps_store, MCHRG0MAXCURR | Instance);
        mchrgXcrvolt_value = (mchrgXcrvolt != NULL) ? ddm_entry__value_i32(mchrgXcrvolt) : MCHRG0CRVOLT_DEFAULT;
        mchrgXmaxcurr_value = (mchrgXmaxcurr != NULL) ? ddm_entry__value_i32(mchrgXmaxcurr) : MCHRG0MAXCURR_DEFAULT;
        break;
    case MCHRG0MAXCURR:
        mchrgXmaxcurr_value = pframe->frame.set.value.int32;
        mchrgXcrvolt = ddm_store__access(mps_context->mps_store, MCHRG0CRVOLT | Instance);
        mchrgXeqvolt = ddm_store__access(mps_context->mps_store, MCHRG0EQVOLT | Instance);
        mchrgXcrvolt_value = (mchrgXcrvolt != NULL) ? ddm_entry__value_i32(mchrgXcrvolt) : MCHRG0CRVOLT_DEFAULT;
        mchrgXeqvolt_value = (mchrgXeqvolt != NULL) ? ddm_entry__value_i32(mchrgXeqvolt) : MCHRG0EQVOLT_DEFAULT;
        break;
    default:
        return;
    }

    MPS_NBUS_WRITE_FRAME write_imp_b5 = {
        .frame_type = MPS_NBUS_WRITE_FRAME_TYPE_IMP,
        .imp_frame = {
            .command = "APP+IMP=",
            .data_type = 0xb5,
            .nbus_b5 = {
                .vrestart = mchrgXcrvolt_value / (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 10),
                .desulfation = mchrgXeqvolt_value / (Ddm2_unit_factor_list[DDM2_UNIT_VOLT] / 10),
                .imax = mchrgXmaxcurr_value / (Ddm2_unit_factor_list[DDM2_UNIT_AMPERE] / 10),
                .maintenance = 0,
            },
        },
    };

    complete_cb(&write_imp_b5, context);
}

static void f_pwr(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(mps_context != NULL);
    TRUE_CHECK_RETURN(complete_cb != NULL);

    const int32_t Instance = DDM2_PARAMETER_INSTANCE_PART(pframe->frame.set.parameter);

    ddm_entry_t *minvertXinst;
    int32_t minvertXinst_value;
    minvertXinst = ddm_store__access(mps_context->mps_store, MINVERT0INST | Instance);

    if (minvertXinst == NULL)
    {
        LOG(W, "No N-BUS instance for %08x", MINVERT0INST | Instance);
        return;
    }

    minvertXinst_value = ddm_entry__value_i32(minvertXinst);

    uint8_t device_address = (uint8_t)(minvertXinst_value >> 8);
    uint8_t sub_address = (uint8_t)(minvertXinst_value & 0xff);

    MPS_NBUS_WRITE_FRAME write_pwr = {
        .frame_type = MPS_NBUS_WRITE_FRAME_TYPE_PWR,
        .pwr_frame = {
            .command = "APP+PWR=",
            .device_address = device_address,
            .sub_address = sub_address,
            .data = pframe->frame.set.value.int32 ? 2 : 1,
        },
    };

    complete_cb(&write_pwr, context);
}

static void f_rpa(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(mps_context != NULL);
    TRUE_CHECK_RETURN(complete_cb != NULL);

    MPS_NBUS_WRITE_FRAME write_rpa = {
        .frame_type = MPS_NBUS_WRITE_FRAME_TYPE_RPA,
        .rpa_frame = {
            .command = "APP+RPA",
        },
    };

    complete_cb(&write_rpa, context);
}

//! \~ DDM2 to N-BUS conversion callback lookup list, settable parameters
static const SORTED_LIST_PTR_ENTRY Parameter_lookup_list_data[] = {
    {MCHRG0MAXCURR, (SORTED_LIST_PTR_VALUE_TYPE)f_b5},    // 0x1e000010
    {MCHRG0BVOLT, (SORTED_LIST_PTR_VALUE_TYPE)f_b4},      // 0x1e000012
    {MCHRG0FVOLT, (SORTED_LIST_PTR_VALUE_TYPE)f_b4},      // 0x1e000013
    {MCHRG0EQVOLT, (SORTED_LIST_PTR_VALUE_TYPE)f_b5},     // 0x1e000014
    {MCHRG0CRVOLT, (SORTED_LIST_PTR_VALUE_TYPE)f_b5},     // 0x1e000018
    {MCHRG0SLNT, (SORTED_LIST_PTR_VALUE_TYPE)f_b0},       // 0x1e000021
    {MCHRG0ALGO, (SORTED_LIST_PTR_VALUE_TYPE)f_b0},       // 0x1e000022
    {MBAT0TYPE, (SORTED_LIST_PTR_VALUE_TYPE)f_b2},        // 0x1e020004
    {MBAT0CAPACITY, (SORTED_LIST_PTR_VALUE_TYPE)f_b2},    // 0x1e02001c
    {MSOLAR0PPWR, (SORTED_LIST_PTR_VALUE_TYPE)f_b3},      // 0x1e05000b
    {MINVERT0ENABLE, (SORTED_LIST_PTR_VALUE_TYPE)f_pwr},  // 0x1e070006
    {MPS0RSTPWD, (SORTED_LIST_PTR_VALUE_TYPE)f_rpa},      // 0x1e090000
};

// [DDM2 parameter base instance] -> [DDM2->N-BUS conversion callback]
static SORTED_LIST_PTR Parameter_lookup_list = {
    .pdata = (SORTED_LIST_PTR_ENTRY *)Parameter_lookup_list_data,
    .capacity = ELEMENTS(Parameter_lookup_list_data),
    .entry_count = ELEMENTS(Parameter_lookup_list_data),
};

/*! \brief Perform NBUS to DDM2 conversion
    \param nbus Pointer to NBUS data
    \param instance_start Pointer to DDM2 instance start
    \param complete_cb Callback function for completion
    \param context Callback context
    \return TRUE is handled, FALSE if not handled
*/
int mps_ddm2_to_nbus(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context)
{
    SORTED_LIST_PTR_VALUE_TYPE frame_cb;
    const uint32_t Base_instance = DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.set.parameter);

    const SORTED_LIST_PTR_RETURN_VALUE Get_result = sorted_list_ptr_unique_get(&frame_cb, &Parameter_lookup_list, Base_instance, 0);  // Find conversion function for DDM2 parameter

    if (((SORTED_LIST_PTR_RETURN_VALUE)Get_result == SORTED_LIST_PTR_OK) && (frame_cb != 0))
    {
        ((DDM2_NBUS_CONVERT_CB)frame_cb)(pframe, mps_context, complete_cb, context);  // Convert DDM2 frame to N-BUS frame
        return 1;
    }
    else
    {
        LOG(W, "No conversion function for DDM2 parameter %08x", pframe->frame.set.parameter);
    }

    return 0;
};

static void int64_sum_map_fn(ddm_entry_t *ddm_entry, void *map_context)
{
    INT64_SUM *sum_context = map_context;

    if (ddm_entry__is_value_int32(ddm_entry))
    {
        sum_context->result += ddm_entry__value_i32(ddm_entry);
    }
    sum_context->nbr_of_entries++;
}

static void int32_set_map_fn(ddm_entry_t *ddm_entry, void *map_context)
{
    int32_t value = *(int32_t *)map_context;
    bool has_changed = ddm_entry__set__value_i32(ddm_entry, value);
    ddm_entry__set__has_changed_conditionally(ddm_entry, has_changed);
}

static size_t parameter_sum_int64(ddm_store_t *ddm_store, uint32_t parameter, int64_t *result)
{
    INT64_SUM sum_context = {0, 0};

    ddm_store__filter_then_map(ddm_store, ddm_store__condition_by_parameter_any_instance, &parameter, int64_sum_map_fn, &sum_context);
    *result = sum_context.result;
    return sum_context.nbr_of_entries;
}

static void parameter_set_all_instances_i32(ddm_store_t *ddm_store, uint32_t parameter, int32_t value)
{
    ddm_store__filter_then_map(ddm_store, ddm_store__condition_by_parameter_any_instance, &parameter, int32_set_map_fn, &value);
}

/**
 * \brief       Calculate MBATxCHGDET
 *
 * It is expected that the parameter \a instance_start defines instances for MBATxSOC and MBATxSOH, in that order.
 *
 * When MBAT0CHGDET_CUMULATIVE is set to 1, \a instance_start is not used.
 */
static void calculate_mbatXchgdet(const MPS_CONTEXT *const mps_context, const uint32_t parameter, const uint32_t *const instance_start)
{
#if (MBAT0CHGDET_CUMULATIVE == 0)
    bool has_changed = false;
    ddm_entry_t *mbatXcurr;
    ddm_entry_t *mbatXchgdet;
    const uint32_t mbatXchgdet_parameter = MBAT0CHGDET | DDM2_PARAMETER_INSTANCE_PART(parameter);

    // Access or create MBATxCHGDET
    mbatXchgdet = ddm_store__new_entry(mps_context->mps_store, mbatXchgdet_parameter);
    TRUE_CHECK_RETURN(mbatXchgdet != NULL);
    if (ddm_entry__is_value_none(mbatXchgdet))
    {
        ddm_entry__set__value_i32(mbatXchgdet, MBAT0CHGDET_DEFAULT);
        ddm_entry__set__has_changed(mbatXchgdet, true);  // Force publish of this parameter since we just created it
    }
    // Access MBATxCURR
    mbatXcurr = ddm_store__access(mps_context->mps_store, MBAT0CURR | DDM2_PARAMETER_INSTANCE_PART(parameter));
    // TRUE_CHECK_RETURN(mbatXcurr != NULL); // This should not happen since we got triggered by this parameter

    // Calculate MBATxCHGDET value
    if (ddm_entry__value_i32(mbatXcurr) > (0.5 * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]))
    {
        has_changed = ddm_entry__set__value_i32(mbatXchgdet, 1);
    }
    else if (ddm_entry__value_i32(mbatXcurr) < (-0.5 * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]))
    {
        has_changed = ddm_entry__set__value_i32(mbatXchgdet, 0);
    }
    ddm_entry__set__has_changed_conditionally(mbatXchgdet, has_changed);
#else
    ddm_entry_t *mbatXchgdet;
    const uint32_t mbatXchgdet_parameter = MBAT0CHGDET | DDM2_PARAMETER_INSTANCE_PART(parameter);
    // Iterate over mbatXcurr instances and calculate the sum
    int64_t sum_mbatXcurr;
    (void)parameter_sum_int64(mps_context->mps_store, MBAT0CURR, &sum_mbatXcurr);

    // Access or create MBATxCHGDET
    mbatXchgdet = ddm_store__access(mps_context->mps_store, mbatXchgdet_parameter);
    if (mbatXchgdet == NULL)
    {
        mbatXchgdet = ddm_store__new_entry(mps_context->mps_store, mbatXchgdet_parameter);
        TRUE_CHECK_RETURN(mbatXchgdet != NULL);
    }
    // If the sum is greater than 0.5 set mbatXchgdet to 1 (which mbatXchgdet?)
    // If the sum is less than 0.5 set mbatXchgdet to 0 (which mbatXchgdet?)
    // Calculate MBATxCHGDET value
    if (sum_mbatXcurr > (0.5 * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]))
    {
        parameter_set_all_instances_i32(mps_context->mps_store, MBAT0CHGDET, 1);
    }
    else if (sum_mbatXcurr < (-0.5 * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]))
    {
        parameter_set_all_instances_i32(mps_context->mps_store, MBAT0CHGDET, 0);
    }
#endif
}

/**
 * \brief       Calculate MBATxCAPREL
 */
static void calculate_mbatXcaprel(const MPS_CONTEXT *const mps_context, const uint32_t parameter, const uint32_t *const instance_start)
{
    ddm_entry_t *mbatXsoh;
    ddm_entry_t *mbatXsoc;
    ddm_entry_t *mbatXcaprel;
    const uint32_t mbatXcaprel_parameter = MBAT0CAPREL | DDM2_PARAMETER_INSTANCE_PART(parameter);

    // Access MBATxSOH or MBATxSOC, in order to calculate MBATxCAPREL both params needs to be present
    mbatXsoc = ddm_store__access(mps_context->mps_store, MBAT0SOC | DDM2_PARAMETER_INSTANCE_PART(parameter));
    mbatXsoh = ddm_store__access(mps_context->mps_store, MBAT0SOH | DDM2_PARAMETER_INSTANCE_PART(parameter));
    if ((mbatXsoc != NULL) && (mbatXsoh != NULL) &&
        ddm_entry__is_value_int32(mbatXsoc) && ddm_entry__is_value_int32(mbatXsoh))
    {
        // Access or create MBATxCAPREL
        mbatXcaprel = ddm_store__access(mps_context->mps_store, mbatXcaprel_parameter);
        if (mbatXcaprel == NULL)
        {
            mbatXcaprel = ddm_store__new_entry(mps_context->mps_store, mbatXcaprel_parameter);
            TRUE_CHECK_RETURN(mbatXcaprel != NULL);
        }
        // Calculate MBATxCAPREL value
        int32_t mbatXcaprel_value = ddm_entry__value_i32(mbatXsoh) * ddm_entry__value_i32(mbatXsoc) / 100;
        bool has_changed = ddm_entry__set__value_i32(mbatXcaprel, mbatXcaprel_value);
        ddm_entry__set__has_changed(mbatXcaprel, has_changed);
    }
}

typedef struct CALCULATE_TOC_CONTEXT
{
    int64_t mbatXcurr_sum;
    int64_t ah_to_charge_sum;
    int64_t ah_to_discharge_sum;
    ddm_store_t *mps_store;
    int32_t toc;
    bool is_toc_updated;
} CALCULATE_TOC_CONTEXT;

/**
 * \brief       Implements the core of TOC calculation
 *
 * On each iteration the `is_toc_updated` flag is changed. Only the last iteration is taken into
 * account by caller function where the final TOC value is available.
 */
static void calculate_toc(ddm_entry_t *trigger_parameter, void *map_context)
{
    CALCULATE_TOC_CONTEXT *toc_context = map_context;

    uint32_t base_instance = DDM2_PARAMETER_INSTANCE_PART(ddm_entry__parameter_id(trigger_parameter));
    ddm_entry_t *mbatXsoc = ddm_store__access(toc_context->mps_store, MBAT0SOC | base_instance);
    ddm_entry_t *mbatXcurr = ddm_store__access(toc_context->mps_store, MBAT0CURR | base_instance);
    ddm_entry_t *mbatXcapacity = ddm_store__access(toc_context->mps_store, MBAT0CAPACITY | base_instance);
    // Only when all the parameters of the same class are available we can calculate MBATxTIME
    if ((mbatXsoc != NULL) && (mbatXcapacity != NULL) && (mbatXcurr != NULL) &&
        ddm_entry__is_value_int32(mbatXsoc) && ddm_entry__is_value_int32(mbatXcapacity) && ddm_entry__is_value_int32(mbatXcurr))
    {
        int32_t mbatXsoc_value = ddm_entry__value_i32(mbatXsoc);
        int32_t mbatXcapacity_value = ddm_entry__value_i32(mbatXcapacity);
        toc_context->mbatXcurr_sum += ddm_entry__value_i32(mbatXcurr);
        toc_context->ah_to_charge_sum += (mbatXcapacity_value * (100 - mbatXsoc_value)) / 100;
        toc_context->ah_to_discharge_sum += (mbatXcapacity_value * mbatXsoc_value) / 100;
        if (toc_context->mbatXcurr_sum > (0.5 * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]))
        {
            toc_context->is_toc_updated = true;
            toc_context->toc = (toc_context->ah_to_charge_sum * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]) / toc_context->mbatXcurr_sum * 60 * Ddm2_unit_factor_list[DDM2_UNIT_MINUTE];
        }
        else if (toc_context->mbatXcurr_sum < (-0.5 * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]))
        {
            toc_context->is_toc_updated = true;
            toc_context->toc = (toc_context->ah_to_discharge_sum * Ddm2_unit_factor_list[DDM2_UNIT_AMPERE]) / toc_context->mbatXcurr_sum * 60 * Ddm2_unit_factor_list[DDM2_UNIT_MINUTE];
        }
        else
        {
            // Do not update toc
            toc_context->is_toc_updated = false;
        }
    }
}

/**
 * \brief       Calculate MBATxTIME based on MBATxSOC, MBATxCAPACITY or MBATxCURR.
 */
static void calculate_mbatXtime(const MPS_CONTEXT *const mps_context, const uint32_t trigger_parameter, const uint32_t *const instance_start)
{
    CALCULATE_TOC_CONTEXT calculate_toc_context = {
        .mps_store = mps_context->mps_store,
        .mbatXcurr_sum = 0,
        .ah_to_charge_sum = 0,
        .ah_to_discharge_sum = 0,
        .toc = 0,
        .is_toc_updated = false,
    };
    ddm_store__filter_then_map(mps_context->mps_store, ddm_store__condition_by_parameter_any_instance, &trigger_parameter, calculate_toc, &calculate_toc_context);
    if (calculate_toc_context.is_toc_updated)
    {
        const uint32_t mbatXtime_parameter = MBAT0TIME | DDM2_PARAMETER_INSTANCE_PART(trigger_parameter);
        const uint32_t mbatXtimests_parameter = MBAT0TIMESTS | DDM2_PARAMETER_INSTANCE_PART(trigger_parameter);
        ddm_entry_t *p_entry;
        // Access or create MBATxTIME
        p_entry = ddm_store__access(mps_context->mps_store, mbatXtime_parameter);
        if (p_entry == NULL)
        {
            p_entry = ddm_store__new_entry(mps_context->mps_store, mbatXtime_parameter);
            TRUE_CHECK_RETURN(p_entry != NULL);
            ddm_entry__set__value_i32(p_entry, 0);       // This value will be published when TOC isn't updated and it was calculated for the 1st time.
            ddm_entry__set__has_changed(p_entry, true);  // Force publish of this parameter since we just created it
        }
        // Access or create MBATxTIMESTS
        p_entry = ddm_store__access(mps_context->mps_store, mbatXtimests_parameter);
        if (p_entry == NULL)
        {
            p_entry = ddm_store__new_entry(mps_context->mps_store, mbatXtimests_parameter);
            TRUE_CHECK_RETURN(p_entry != NULL);
            ddm_entry__set__value_i32(p_entry, 0);       // Default is time-to-empty
            ddm_entry__set__has_changed(p_entry, true);  // Force publish of this parameter since we just created it
        }
        if (calculate_toc_context.toc <= 0)
        {
            // Meaning discharging - time-to-empty
            parameter_set_all_instances_i32(mps_context->mps_store, MBAT0TIMESTS, 0);
            parameter_set_all_instances_i32(mps_context->mps_store, MBAT0TIME, -calculate_toc_context.toc);
        }
        else
        {
            // Meaning Charging - time-to-full
            parameter_set_all_instances_i32(mps_context->mps_store, MBAT0TIMESTS, 1);
            parameter_set_all_instances_i32(mps_context->mps_store, MBAT0TIME, calculate_toc_context.toc);
        }
    }
}

typedef struct CALCULATE_SOC_CONTEXT
{
    int64_t system_capacity_sum;
    int64_t system_remaining_capacity_sum;
    ddm_store_t *mps_store;
    int32_t soc;
    bool soc_is_calculated;
} CALCULATE_SOC_CONTEXT;

static void calculate_soc(ddm_entry_t *trigger_parameter, void *map_context)
{
    CALCULATE_SOC_CONTEXT *soc_context = map_context;
    uint32_t base_instance = DDM2_PARAMETER_INSTANCE_PART(ddm_entry__parameter_id(trigger_parameter));
    ddm_entry_t *mbatXsoc = ddm_store__access(soc_context->mps_store, MBAT0SOC | base_instance);
    ddm_entry_t *mbatXsoh = ddm_store__access(soc_context->mps_store, MBAT0SOH | base_instance);
    ddm_entry_t *mbatXcapacity = ddm_store__access(soc_context->mps_store, MBAT0CAPACITY | base_instance);
    // Only when all the parameters of the same class are available we can calculate SOC
    if ((mbatXsoc != NULL) && (mbatXcapacity != NULL) && (mbatXsoh != NULL) &&
        ddm_entry__is_value_int32(mbatXsoc) && ddm_entry__is_value_int32(mbatXcapacity) && ddm_entry__is_value_int32(mbatXsoh))
    {
        int32_t mbatXsoc_value = ddm_entry__value_i32(mbatXsoc);
        int32_t mbatXsoh_value = ddm_entry__value_i32(mbatXsoh);
        int32_t mbatXcapacity_value = ddm_entry__value_i32(mbatXcapacity);
        soc_context->system_capacity_sum += mbatXcapacity_value * mbatXsoh_value / 100;
        soc_context->system_remaining_capacity_sum += mbatXcapacity_value * mbatXsoc_value / 100;
        if (soc_context->system_capacity_sum != 0)
        {
            soc_context->soc = soc_context->system_remaining_capacity_sum * 100 / soc_context->system_capacity_sum;
        }
        else
        {
            soc_context->soc = 0;
        }
        soc_context->soc_is_calculated = true;
    }
}

/**
 * \brief       Calculate MPSxSOC based on MBATxSOC, MBATxCAPACITY or MBATxSOH.
 */
static void calculate_mpsXsoc(const MPS_CONTEXT *const mps_context, const uint32_t trigger_parameter, const uint32_t *const instance_start)
{
    CALCULATE_SOC_CONTEXT calculate_soc_context = {
        .mps_store = mps_context->mps_store,
        .system_capacity_sum = 0,
        .system_remaining_capacity_sum = 0,
        .soc = 0,
        .soc_is_calculated = false,
    };
    ddm_store__filter_then_map(mps_context->mps_store, ddm_store__condition_by_parameter_any_instance, &trigger_parameter, calculate_soc, &calculate_soc_context);
    if (calculate_soc_context.soc_is_calculated)
    {
        uint32_t mpsXsoc_parameter = MPS0SOC | DDM2_PARAMETER_INSTANCE_PART(mps_context->mps_class_instance);
        // Access or create MPSxSOC
        ddm_entry_t *mpsXsoc = ddm_store__access(mps_context->mps_store, mpsXsoc_parameter);
        if (mpsXsoc == NULL)
        {
            mpsXsoc = ddm_store__new_entry(mps_context->mps_store, mpsXsoc_parameter);
            TRUE_CHECK_RETURN(mpsXsoc != NULL);
        }
        bool has_changed = ddm_entry__set__value_i32(mpsXsoc, calculate_soc_context.soc);
        ddm_entry__set__has_changed_conditionally(mpsXsoc, has_changed);
    }
}

#if (MBAT0CAPREM_IS_CALCULATED == 1)
/**
 * \brief       Calculate MBATXxCAPREM based on MBATxSOC, MBATxCAPACITY and MBATxSOH.
 */
static void calculate_mbatXcaprem(const MPS_CONTEXT *const mps_context, const uint32_t trigger_parameter, const uint32_t *const instance_start)
{
    ddm_entry_t *mbatXsoh;
    ddm_entry_t *mbatXsoc;
    ddm_entry_t *mbatXcapacity;
    ddm_entry_t *mbatXcaprem;
    const uint32_t mbatXcaprem_parameter = MBAT0CAPREM | DDM2_PARAMETER_INSTANCE_PART(trigger_parameter);

    // Access MBATxSOH, MBATxSOC and MBATxCAPACITY, in order to calculate MBATxCAPREM all params needs to be present
    mbatXsoc = ddm_store__access(mps_context->mps_store, MBAT0SOC | DDM2_PARAMETER_INSTANCE_PART(trigger_parameter));
    mbatXsoh = ddm_store__access(mps_context->mps_store, MBAT0SOH | DDM2_PARAMETER_INSTANCE_PART(trigger_parameter));
    mbatXcapacity = ddm_store__access(mps_context->mps_store, MBAT0CAPACITY | DDM2_PARAMETER_INSTANCE_PART(trigger_parameter));
    if ((mbatXsoc != NULL) && (mbatXsoh != NULL) && (mbatXcapacity != NULL) &&
        ddm_entry__is_value_int32(mbatXsoc) && ddm_entry__is_value_int32(mbatXsoh) && ddm_entry__is_value_int32(mbatXcapacity))
    {
        // Access or create MBATxCAPREM
        mbatXcaprem = ddm_store__access(mps_context->mps_store, mbatXcaprem_parameter);
        if (mbatXcaprem == NULL)
        {
            mbatXcaprem = ddm_store__new_entry(mps_context->mps_store, mbatXcaprem_parameter);
            TRUE_CHECK_RETURN(mbatXcaprem != NULL);
        }
        // Calculate MBATxCAPREL value
        int32_t mbatXcaprem_value = ddm_entry__value_i32(mbatXcapacity) * ddm_entry__value_i32(mbatXsoh) * ddm_entry__value_i32(mbatXsoc) / 10000;
        bool has_changed = ddm_entry__set__value_i32(mbatXcaprem, mbatXcaprem_value);
        ddm_entry__set__has_changed(mbatXcaprem, has_changed);
    }
}
#endif /* (MBAT0CAPREM_IS_CALCULATED == 1) */

static const SORTED_LIST_PTR_ENTRY Parameter_calculation_lookup_list_data[] = {
    {MBAT0CURR, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXchgdet},  // 0x1e02000a
    {MBAT0CURR, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXtime},    // 0x1e02000a
    {MBAT0SOC, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXtime},     // 0x1e02000c
    {MBAT0SOC, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXcaprel},   // 0x1e02000c
#if (MBAT0CAPREM_IS_CALCULATED == 1)
    {MBAT0SOC, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXcaprem},  // 0x1e02000c
#endif
    {MBAT0SOC, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mpsXsoc},      // 0x1e02000c
    {MBAT0SOH, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXcaprel},  // 0x1e02000f
#if (MBAT0CAPREM_IS_CALCULATED == 1)
    {MBAT0SOH, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXcaprem},  // 0x1e02000f
#endif
    {MBAT0SOH, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mpsXsoc},         // 0x1e02000f
    {MBAT0CAPACITY, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXtime},  // 0x1e02001c
#if (MBAT0CAPREM_IS_CALCULATED == 1)
    {MBAT0CAPACITY, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mbatXcaprem},  // 0x1e02001c
#endif
    {MBAT0CAPACITY, (SORTED_LIST_PTR_VALUE_TYPE)calculate_mpsXsoc},  // 0x1e02001c
};

// [DDM2 parameter base instance] -> [DDM2 calculation callback]
static SORTED_LIST_PTR Parameter_calculation_lookup_list = {
    .pdata = (SORTED_LIST_PTR_ENTRY *)Parameter_calculation_lookup_list_data,
    .capacity = ELEMENTS(Parameter_calculation_lookup_list_data),
    .entry_count = ELEMENTS(Parameter_calculation_lookup_list_data),
};

void mps_calculate(const MPS_CONTEXT *const mps_context, uint32_t parameter)
{
    SORTED_LIST_PTR_VALUE_TYPE parameter_calculate_cbs[ELEMENTS(Parameter_calculation_lookup_list_data)];
    int value_count = ELEMENTS(parameter_calculate_cbs);
    const uint32_t Base_instance = DDM2_PARAMETER_BASE_INSTANCE(parameter);

    const SORTED_LIST_PTR_RETURN_VALUE Get_result = sorted_list_ptr_multiple_get(&parameter_calculate_cbs[0], &value_count, &Parameter_calculation_lookup_list, Base_instance, 0);  // Find conversion function for DDM2 parameter

    if (Get_result == SORTED_LIST_PTR_OK)
    {
        for (int i = 0; i < value_count; i++)
        {
            MPS_PARAMETER_CALCULATE_CB cb = (MPS_PARAMETER_CALCULATE_CB)parameter_calculate_cbs[i];
            if (cb != NULL)
            {
                cb(mps_context, parameter, NULL);  // NOTE: The third argument is reserved for future use
            }
        }
    }
}
