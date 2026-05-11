/**
 * @file lin_server_tempra_battery.c
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief TEMPRA Battery implementation
 * @date 2024-06-25
 */

#include "lin_server_tempra_battery.h"
#include "configuration.h"
#include "lin_server.h"
#include "lin_server_device_definition.h"
#include "lin_server_scheduler.h"

/* Private macros */
#define TEMPRA_DDM_MODEL

#define TEMPRA_INITIAL_NAD 0x05
#define TEMPRA_FUNCTION_ID 0x0005
#define TEMPRA_SUPPLIER_ID 0x2014

#define TEMPRA_VARIANT_ID_TLB150  0x01
#define TEMPRA_VARIANT_ID_TLB120  0x02
#define TEMPRA_VARIANT_ID_TLB100  0x03
#define TEMPRA_VARIANT_ID_TLB150F 0x04
#define TEMPRA_VARIANT_ID_TLB120F 0x05
#define TEMPRA_VARIANT_ID_TLB100F 0x06

#define TEMPRA_MAX_NOMINAL_CAPACITY_RAW 2700  // corresponds to 5400Ah

#define TEMPRA_MBAT0STATUS_MERGE_BATTERIES_ERRORS              0x00
#define TEMPRA_MBAT0STATUS_MERGE_EVENTS                        0x01
#define TEMPRA_MBAT0STATUS_ERROR_SIZE                          (sizeof(((ERROR_T *)0)->error[0]))
#define TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_BATTERIES_ERRORS  (ELEMENTS(mbat0status_handled_batteries_errors))
#define TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_EVENTS            (ELEMENTS(mbat0status_handled_events))
#define TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_ERRORS_AND_EVENTS (TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_BATTERIES_ERRORS + TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_EVENTS)

/* List of all 'Batteries Errors' and 'Events' supported for Tempra battery */
static const uint16_t mbat0status_handled_batteries_errors[] = {MPS_BAT_CELL_FAULT};
static const uint16_t mbat0status_handled_events[] =
    {
        MPS_GEN_DEVICE_OVERTEMP,
        MPS_GEN_DEVICE_FAULT,
        MPS_DC_LOW_STATE_CHARGE_LIMIT_REACHED,
        MPS_DC_HIGH_CURR_LIMIT_REACHED,
        MPS_DC_HIGH_CURR_LIMIT_DISCONNECTED,
        MPS_DC_LOW_VOLT_LIMIT_LOAD_DISCONNECTED,
        MPS_DC_HIGH_VOLT_LIMIT_CHARGE_DISCONNECTED,
        MPS_DC_LOW_TEMP_LIMIT_REACHED,
};

/* Private functions */
static int tempra_extract_function(const lin_server_slave_device_t *slave_device, const lin_server_device_frames_bundle_def_t *frames_bundle_defs, const uint8_t *data, size_t data_size, lin_server_slave_info_response_t *errors);

/* Helper functions */
static int tempra_merge_mbat0status_codes(uint8_t merge_type, const uint16_t *current_mbat0status_data, size_t current_mbat0status_data_size, uint16_t *mbat0status_data, size_t *mbat0status_data_size);

/* LIN to DDM conversion functions */
/* IBS_FRM2 */
static int tempra_conv_current_to_mbat0curr(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int tempra_conv_voltage_to_mbat0volt(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int tempra_conv_temperature_to_mbat0temp(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int tempra_conv_errors_to_mbat0status(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
/* IBS_FRM6 */
static int tempra_conv_avlcap_to_mbat0caprel(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int tempra_conv_nomcap_to_mbat0capacity(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
/* IBS_FRM5 */
static int tempra_conv_soc_to_mbat0soc(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int tempra_conv_soh_to_mbat0soh(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int tempra_conv_events_to_mbat0status(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);

/* Private types */
static EXT_RAM_ATTR lin_server_device_frame_t lin_server_tempra_info_ibs_frame2;
static EXT_RAM_ATTR lin_server_device_frame_t lin_server_tempra_info_ibs_frame5;
static EXT_RAM_ATTR lin_server_device_frame_t lin_server_tempra_info_ibs_frame6;

static const lin_device_config_data_t tempra_config_data =
    {
        .nad = TEMPRA_INITIAL_NAD,
        .supplier_id = TEMPRA_SUPPLIER_ID,
};

static const uint8_t tempra_variant_ids[] =
    {
        TEMPRA_VARIANT_ID_TLB150,
        TEMPRA_VARIANT_ID_TLB120,
        TEMPRA_VARIANT_ID_TLB100,
        TEMPRA_VARIANT_ID_TLB150F,
        TEMPRA_VARIANT_ID_TLB120F,
        TEMPRA_VARIANT_ID_TLB100F,
};

static const lin_server_function_specific_config_data_t tempra_function_specific_config_data[] =
    {
        {
            .function_id = TEMPRA_FUNCTION_ID,

            .variant_ids = tempra_variant_ids,
            .variant_ids_size = ELEMENTS(tempra_variant_ids),
            .map_function_specific_ddm_to_lin = NULL,
            .map_function_specific_ddm_to_lin_size = 0,
            .map_function_specific_lin_to_ddm = NULL,
            .map_function_specific_lin_to_ddm_size = 0,
        },
};

static const lin_server_generic_profile_entry_t tempra_profile_entry[] =
    {
        {
            .ddm_model = TLB150,
            .function_id = TEMPRA_FUNCTION_ID,
            .variant_id = TEMPRA_VARIANT_ID_TLB150,
        },
        {
            .ddm_model = TLB120,
            .function_id = TEMPRA_FUNCTION_ID,
            .variant_id = TEMPRA_VARIANT_ID_TLB120,
        },
        {
            .ddm_model = TLB100,
            .function_id = TEMPRA_FUNCTION_ID,
            .variant_id = TEMPRA_VARIANT_ID_TLB100,
        },
        {
            .ddm_model = TLB150F,
            .function_id = TEMPRA_FUNCTION_ID,
            .variant_id = TEMPRA_VARIANT_ID_TLB150F,
        },
        {
            .ddm_model = TLB120F,
            .function_id = TEMPRA_FUNCTION_ID,
            .variant_id = TEMPRA_VARIANT_ID_TLB100F,
        },
        {
            .ddm_model = TLB100F,
            .function_id = TEMPRA_FUNCTION_ID,
            .variant_id = TEMPRA_VARIANT_ID_TLB100F,
        },
};

static const lin_server_generic_profile_t tempra_generic_profile =
    {
        .discriminator_ddm = DISCRIMINATOR_DDM_PARAMETER_MODEL_NONE,  // No discriminator for MBAT?
        .profile_entry = tempra_profile_entry,
        .profile_entry_size = ELEMENTS(tempra_profile_entry),
};

static const lin_server_device_frame_def_t lin_server_tempra_info_ibs_frame2_def =
    {
        .frame_id = TEMPRA_INFO_FRAME_IBS_FRM2_ID,
        .frame_type = LIN_INFO_FRAME,
        .frame_len = TEMPRA_INFO_FRAME_IBS_FRM2_LEN,
        .frame = &lin_server_tempra_info_ibs_frame2,
        .protocol_definition = {.local_change = NULL},
};
static const lin_server_device_frame_def_t lin_server_tempra_info_ibs_frame5_def =
    {
        .frame_id = TEMPRA_INFO_FRAME_IBS_FRM5_ID,
        .frame_type = LIN_INFO_FRAME,
        .frame_len = TEMPRA_INFO_FRAME_IBS_FRM5_LEN,
        .frame = &lin_server_tempra_info_ibs_frame5,
        .protocol_definition = {.local_change = NULL},
};
static const lin_server_device_frame_def_t lin_server_tempra_info_ibs_frame6_def =
    {
        .frame_id = TEMPRA_INFO_FRAME_IBS_FRM6_ID,
        .frame_type = LIN_INFO_FRAME,
        .frame_len = TEMPRA_INFO_FRAME_IBS_FRM6_LEN,
        .frame = &lin_server_tempra_info_ibs_frame6,
        .protocol_definition = {.local_change = NULL},
};

static const lin_server_device_frames_bundle_def_t lin_server_tempra_frames_bundle_defs[] =
    {
        {
            .ctrl_frame_def = NULL,
            .info_frame_def = &lin_server_tempra_info_ibs_frame2_def,
        },
        {
            .ctrl_frame_def = NULL,
            .info_frame_def = &lin_server_tempra_info_ibs_frame5_def,
        },
        {
            .ctrl_frame_def = NULL,
            .info_frame_def = &lin_server_tempra_info_ibs_frame6_def,
        },
};

static lin_server_slave_device_mutable_data_t data =
    {
        .class_instance = -1,
        .prod_instance = -1,
        .is_initialized = false,
        .is_on_bus_detected = false,
        .bus_detection_counter = LIN_SERVER_BUS_DETECTION_COUNTER_INIT_VALUE,
        .lock = NULL,
        .data = NULL,
        .data_size = 0,
};

static const struct ddm_store_ddm tempra_ddm_production_initial_values[] =
    {
        {.ddm_parameter = PROD0AVL,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = PROD0NAME,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0SN,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0SKU,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0PNC,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0FWVER,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0HWVER,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0MDL,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0EAN,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0DESCRIPTION,
         .value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = PROD0CLIST,
         .value = {.storage = {.structure = NULL}, .size = 0, .type = DDM2_TYPE_STRUCT}},
};

static const struct ddm_store_ddm tempra_ddm_owned_initial_values[] =
    {
        /* IBS_FRM2 */
        {
            .ddm_parameter = MBAT0CURR,
            .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = MBAT0VOLT,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = MBAT0TEMP,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = MBAT0STATUS,
         .value = {
             .storage = {.structure = NULL},
             .type = DDM2_TYPE_STRUCT,
         }},
        /* IBS_FRM6 */
        {.ddm_parameter = MBAT0CAPREM,  // Available Capacity
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = MBAT0CAPACITY,  // Nominal Capacity
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        /* IBS_FRM5 */
        {.ddm_parameter = MBAT0SOC, .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = MBAT0SOH, .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
#if 0
    {
        .ddm_paramter = EVENTS
    },
#endif
};

DDM_STORE__DECLARE_EXTRAM(tempra_ddm_owned_store, ELEMENTS(tempra_ddm_owned_initial_values));
DDM_STORE__DECLARE_EXTRAM(tempra_ddm_production_store, ELEMENTS(tempra_ddm_production_initial_values));

static const lin_server_map_lin_to_ddm_t lin_server_tempra_lin_to_ddm[] =
    {
        /* IBS_FRM2 */
        {
            .convert_lin_to_ddm = tempra_conv_current_to_mbat0curr,
            .ddm = {MBAT0CURR, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame2_def,
        },
        {
            .convert_lin_to_ddm = tempra_conv_voltage_to_mbat0volt,
            .ddm = {MBAT0VOLT, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame2_def,
        },
        {
            .convert_lin_to_ddm = tempra_conv_temperature_to_mbat0temp,
            .ddm = {MBAT0TEMP, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame2_def,
        },
        {
            .convert_lin_to_ddm = tempra_conv_errors_to_mbat0status,
            .ddm = {MBAT0STATUS, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame2_def,
        },
        /* IBS_FRM6 */
        {
            .convert_lin_to_ddm = tempra_conv_avlcap_to_mbat0caprel,
            .ddm = {MBAT0CAPREM, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame6_def,
        },
        {
            .convert_lin_to_ddm = tempra_conv_nomcap_to_mbat0capacity,
            .ddm = {MBAT0CAPACITY, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame6_def,
        },
        /* IBS_FRM5 */
        {
            .convert_lin_to_ddm = tempra_conv_soc_to_mbat0soc,
            .ddm = {MBAT0SOC, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame5_def,
        },
        {
            .convert_lin_to_ddm = tempra_conv_soh_to_mbat0soh,
            .ddm = {MBAT0SOH, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame5_def,
        },
        {
            .convert_lin_to_ddm = tempra_conv_events_to_mbat0status,
            .ddm = {MBAT0STATUS, 0, 0},
            .info_frame_def = &lin_server_tempra_info_ibs_frame5_def,
        },
};

const lin_server_slave_device_t lin_server_tempra_device =
    {
        .device_class = MBAT0,
        .data = &data,
        .name = "TEMPRA",
        .device_type = LIN_SERVER_DEVICE_TYPE_TEMPRA,
        .ddm_owned_store = &tempra_ddm_owned_store,
        .ddm_owned_initial_values = tempra_ddm_owned_initial_values,
        .ddm_owned_initial_values_size = ELEMENTS(tempra_ddm_owned_initial_values),
        .ddm_production_store = &tempra_ddm_production_store,
        .ddm_production_initial_values = tempra_ddm_production_initial_values,
        .ddm_production_initial_values_size = ELEMENTS(tempra_ddm_production_initial_values),
        .frames_bundle_defs = lin_server_tempra_frames_bundle_defs,
        .frames_bundle_defs_size = ELEMENTS(lin_server_tempra_frames_bundle_defs),
        .map_ddm_to_lin = NULL,
        .map_ddm_to_lin_size = 0,
        .map_lin_to_ddm = lin_server_tempra_lin_to_ddm,
        .map_lin_to_ddm_size = ELEMENTS(lin_server_tempra_lin_to_ddm),
        .function_specific_config_data = tempra_function_specific_config_data,
        .function_specific_config_data_size = ELEMENTS(tempra_function_specific_config_data),
        .device_config = &tempra_config_data,
        .protocol = LIN_SERVER_SYNC_PROTOCOL_NONE,
        .generic_profile = &tempra_generic_profile,
        .ddm2_to_lin_ctrl_logic = NULL,
        .ddm2_to_lin_ctrl_logic_size = 0,
        .init_function = NULL,
        .stuff_function = NULL,
        .extract_function = tempra_extract_function,
};

/* Helper functions */

/*
 * @brief       Function to merge status codes from more than one source(frame)
 *
 * @param[in]   merge_type                           TEMPRA_MBAT0STATUS_MERGE_BATTERIES_ERRORS or TEMPRA_MBAT0STATUS_MERGE_EVENTS.
 *                                                   Depending on the provided merge type, this function will filter out the existing
 *                                                   status ids from @param mbat0status_data using the @var mbat0status_handled_batteries_errors
 *                                                   or @param mbat0status_handled_events.
 * @param[in]   current_mbat0status_data             Extracted status ids from the currently processed INFO frame
 * @param[in]   current_mbat0status_Data_size        Size of @param current_mbat0status_data in bytes
 * @param[out]  mbat0status_data                     DDM2 parameter storage with the already existing status ids from previosly processed INFO frames
 * @param[out]  mbat0status_data_size                Size of @param mbat0status_data in bytes
 *
 * @retval 1 -> Success
 * @retval 0 -> Fail
 */
static int tempra_merge_mbat0status_codes(uint8_t merge_type, const uint16_t *current_mbat0status_data, size_t current_mbat0status_data_size, uint16_t *mbat0status_data, size_t *mbat0status_data_size)
{
    const uint16_t *mbat0status_other = merge_type == TEMPRA_MBAT0STATUS_MERGE_BATTERIES_ERRORS ? mbat0status_handled_events : mbat0status_handled_batteries_errors;
    size_t mbat0status_other_count = merge_type == TEMPRA_MBAT0STATUS_MERGE_BATTERIES_ERRORS ? TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_EVENTS : TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_BATTERIES_ERRORS;
    /* Temporary storage that keeps the 'other' type of IDs i.e if we are
     * processing new updates from IBS_FRAME2 that defines 'Batteries Errors',
     * then already existing IDs from IBS_FRAME5 that defines the 'Events' should
     * not be lost, but should be kept in the MBAT0STATUS DDM2 parameter. */
    uint16_t mbat0status_merge[TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_ERRORS_AND_EVENTS];
    size_t mbat0status_merge_size = 0;
    size_t mbat0status_merge_count = 0;

    memset(mbat0status_merge, 0, sizeof(mbat0status_merge));

    /* Iterrate trough already existing 'other' code ids and save them in the temporary buffer */
    size_t mbat0status_data_count = *mbat0status_data_size / TEMPRA_MBAT0STATUS_ERROR_SIZE;
    for (size_t i = 0; i < mbat0status_data_count; ++i)
    {
        for (size_t j = 0; j < mbat0status_other_count; ++j)
        {
            /* Batteries Error/Events exists in the current status data. Keep it. */
            if (mbat0status_data[i] == mbat0status_other[j])
            {
                mbat0status_merge[mbat0status_merge_count] = mbat0status_other[j];
                mbat0status_merge_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
                mbat0status_merge_count++;
                break;
            }
        }
    }

    /* If there is no active 'Batteries Errors' or 'Events', set GENERIC_NO_ERRORS */
    if ((mbat0status_merge_size == 0) && (current_mbat0status_data_size == 0))
    {
        mbat0status_merge[mbat0status_merge_count] = GENERIC_NO_ERRORS;
        mbat0status_merge_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_merge_count++;
    }

    /* append currently detected status data */
    TRUE_CHECK_RETURN0((mbat0status_merge_size + current_mbat0status_data_size) <= sizeof(mbat0status_merge));
    memcpy(&mbat0status_merge[mbat0status_merge_count], current_mbat0status_data, current_mbat0status_data_size);
    mbat0status_merge_size += current_mbat0status_data_size;

    /* override DDM2 MBAT0STATUS parameter storage with currently detected + already existing valid erros code ids */
    TRUE_CHECK_RETURN0(mbat0status_merge_size <= LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY);
    memcpy(mbat0status_data, mbat0status_merge, mbat0status_merge_size);
    *mbat0status_data_size = mbat0status_merge_size;

    return 1;
}

/* Private functions */
static int tempra_extract_function(const lin_server_slave_device_t *slave_device, const lin_server_device_frames_bundle_def_t *frames_bundle_defs, const uint8_t *data, size_t data_size, lin_server_slave_info_response_t *errors)
{
    (void)(data);
    (void)(slave_device);
    (void)(frames_bundle_defs);
    const lin_server_device_frame_def_t *info_frame_def = frames_bundle_defs->info_frame_def;

    switch (info_frame_def->frame_id)
    {
    case TEMPRA_INFO_FRAME_IBS_FRM2_ID:
        TRUE_CHECK_RETURN0(data_size == TEMPRA_INFO_FRAME_IBS_FRM2_LEN);
        break;
    case TEMPRA_INFO_FRAME_IBS_FRM5_ID:
        TRUE_CHECK_RETURN0(data_size == TEMPRA_INFO_FRAME_IBS_FRM5_LEN);
        break;
    case TEMPRA_INFO_FRAME_IBS_FRM6_ID:
        TRUE_CHECK_RETURN0(data_size == TEMPRA_INFO_FRAME_IBS_FRM6_LEN);
        break;
    default:
        break;
    }

    return 1;
}

/* LIN to DDM conversion functions */
/* IBS_FRM2 */
static int tempra_conv_current_to_mbat0curr(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t current_offset = -2000000;
    int32_t *mbat0current = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    *mbat0current = (((uint32_t)tempra_info_frame->info_frame_ibs_frm2.Current_MSB << 16) |
                     ((uint32_t)tempra_info_frame->info_frame_ibs_frm2.Current << 8) |
                     (tempra_info_frame->info_frame_ibs_frm2.Current_LSB));

    *mbat0current = (*mbat0current + current_offset) / Ddm2_unit_factor_list[DDM2_UNIT_AMPERE];

    return 1;
}
static int tempra_conv_voltage_to_mbat0volt(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *mbat0volt = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    *mbat0volt = tempra_info_frame->info_frame_ibs_frm2.Voltage;

    return 1;
}
static int tempra_conv_temperature_to_mbat0temp(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *mbat0temp = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    *mbat0temp = ((tempra_info_frame->info_frame_ibs_frm2.temperature_batt_errors.Temperature / 2) - 40) * Ddm2_unit_factor_list[DDM2_UNIT_DEGC];

    return 1;
}
static int tempra_conv_errors_to_mbat0status(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    /* MBAT0STATUS is an array of unsigned 16-bit integers */
    uint16_t *mbat0status_data = &((ERROR_T *)(ddm2_parameters_value[0]))->error[0];
    size_t *mbat0status_data_size = &ddm2_parameters_value_size[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    uint16_t mbat0status_batteries_error[TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_BATTERIES_ERRORS];
    size_t mbat0status_batteries_error_size = 0;
    size_t mbat0status_batteries_error_count = 0;

    if (tempra_info_frame->info_frame_ibs_frm2.temperature_batt_errors.Batteries_Errors > 0x60)
    {
        /* Set the battery cell fault detected */
        mbat0status_batteries_error[mbat0status_batteries_error_count] = MPS_BAT_CELL_FAULT;
        mbat0status_batteries_error_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_batteries_error_count++;
    }

    return tempra_merge_mbat0status_codes(TEMPRA_MBAT0STATUS_MERGE_BATTERIES_ERRORS, mbat0status_batteries_error, mbat0status_batteries_error_size, mbat0status_data, mbat0status_data_size);
}
/* IBS_FRM6 */
static int tempra_conv_avlcap_to_mbat0caprel(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *mbat0caprel = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    *mbat0caprel = (tempra_info_frame->info_frame_ibs_frm6.Available_Capacity * 0.1f) * Ddm2_unit_factor_list[DDM2_UNIT_AMPHOURS];

    return 1;
}
static int tempra_conv_nomcap_to_mbat0capacity(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    uint16_t nominal_capacity_raw;
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *mbat0capacity = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    // For values below 256, D5 is 0xFF and should be treated as 0x00
    if (tempra_info_frame->info_frame_ibs_frm6.Nominal_Capacity_MSB == 0xFF)
    {
        nominal_capacity_raw = tempra_info_frame->info_frame_ibs_frm6.Nominal_Capacity_LSB;  // Use only D4 for values < 256
    }
    else
    {
        // For values >= 256, combine D4 and D5 (little-endian)
        nominal_capacity_raw = (uint16_t)tempra_info_frame->info_frame_ibs_frm6.Nominal_Capacity_LSB | ((uint16_t)tempra_info_frame->info_frame_ibs_frm6.Nominal_Capacity_MSB << 8);
    }

    // Check if the raw value is within the valid range
    if (nominal_capacity_raw > TEMPRA_MAX_NOMINAL_CAPACITY_RAW)
    {
        *mbat0capacity = 0;  // Set to 0 if the value is above the maximum range
        LOG(E, "Nominal Capacity raw value[%d] above maximum range[%d]! LSB=0x%02X, MSB=0x%02X",
            nominal_capacity_raw, TEMPRA_MAX_NOMINAL_CAPACITY_RAW,
            tempra_info_frame->info_frame_ibs_frm6.Nominal_Capacity_LSB,
            tempra_info_frame->info_frame_ibs_frm6.Nominal_Capacity_MSB);
    }
    else
    {
        // Convert raw value to Ah: raw_value * 2 * unit_factor
        *mbat0capacity = (nominal_capacity_raw * 2) * Ddm2_unit_factor_list[DDM2_UNIT_AMPHOURS];
    }

    return 1;
}
/* IBS_FRM5 */
static int tempra_conv_soc_to_mbat0soc(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *mbat0soc = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    *mbat0soc = tempra_info_frame->info_frame_ibs_frm5.SoC / 2;

    return 1;
}
static int tempra_conv_soh_to_mbat0soh(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *mbat0soh = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    *mbat0soh = tempra_info_frame->info_frame_ibs_frm5.SoH / 2;

    return 1;
}
static int tempra_conv_events_to_mbat0status(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    /* MBAT0STATUS is an array of unsigned 16-bit integers */
    uint16_t *mbat0status_data = &((ERROR_T *)(ddm2_parameters_value[0]))->error[0];
    size_t *mbat0status_data_size = &ddm2_parameters_value_size[0];
    const lin_server_device_frame_tempra_t *tempra_info_frame = &slave_device_frame->tempra_frame;

    uint16_t mbat0status_events[TEMPRA_MBAT0STATUS_NUMBER_OF_HANDLED_EVENTS];
    size_t mbat0status_events_size = 0;
    size_t mbat0status_events_count = 0;

    if (tempra_info_frame->info_frame_ibs_frm5.Events.Cell_Temp_Out_Of_Range_On_Charge ||
        tempra_info_frame->info_frame_ibs_frm5.Events.Cell_Temp_Out_Of_Range_On_Discharge ||
        tempra_info_frame->info_frame_ibs_frm5.Events.Cell_BMS_Temp_Out_Of_Range)
    {
        mbat0status_events[mbat0status_events_count] = MPS_GEN_DEVICE_OVERTEMP;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }
    if (tempra_info_frame->info_frame_ibs_frm5.Events.Battery_Pack_Voltage_Less_Than_10_2V ||
        tempra_info_frame->info_frame_ibs_frm5.Events.HW_Error)
    {
        mbat0status_events[mbat0status_events_count] = MPS_GEN_DEVICE_FAULT;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }
    if (tempra_info_frame->info_frame_ibs_frm5.Events.SOC_Too_Low_or_Not_Aligned ||
        tempra_info_frame->info_frame_ibs_frm5.Events.Voltage_Pack_Hihger_Than_15_3V)
    {
        mbat0status_events[mbat0status_events_count] = MPS_DC_LOW_STATE_CHARGE_LIMIT_REACHED;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }
    if (tempra_info_frame->info_frame_ibs_frm5.Events.Current_Between_202_and_206A_or_Above_260A ||
        tempra_info_frame->info_frame_ibs_frm5.Events.Overcurrent_Prealarm)
    {
        mbat0status_events[mbat0status_events_count] = MPS_DC_HIGH_CURR_LIMIT_REACHED;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }
    if (tempra_info_frame->info_frame_ibs_frm5.Events.Short_Circuit_Event)
    {
        mbat0status_events[mbat0status_events_count] = MPS_DC_HIGH_CURR_LIMIT_DISCONNECTED;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }
    if (tempra_info_frame->info_frame_ibs_frm5.Events.Cell_Voltage_below_2200mV)
    {
        mbat0status_events[mbat0status_events_count] = MPS_DC_LOW_VOLT_LIMIT_LOAD_DISCONNECTED;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }
    if (tempra_info_frame->info_frame_ibs_frm5.Events.Cell_Voltage_over_3850mV)
    {
        mbat0status_events[mbat0status_events_count] = MPS_DC_HIGH_VOLT_LIMIT_CHARGE_DISCONNECTED;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }
    if (tempra_info_frame->info_frame_ibs_frm5.Events.Heating_System_Active)
    {
        mbat0status_events[mbat0status_events_count] = MPS_DC_LOW_TEMP_LIMIT_REACHED;
        mbat0status_events_size += TEMPRA_MBAT0STATUS_ERROR_SIZE;
        mbat0status_events_count++;
    }

    return tempra_merge_mbat0status_codes(TEMPRA_MBAT0STATUS_MERGE_EVENTS, mbat0status_events, mbat0status_events_size, mbat0status_data, mbat0status_data_size);
}
