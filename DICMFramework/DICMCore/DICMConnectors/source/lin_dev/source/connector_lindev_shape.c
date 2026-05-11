/*! \file
    \brief Connector LINDEV for Shape product
 */

#include "configuration.h"

#ifdef CONNECTOR_LINDEV_SHAPE

#include "connector_lindev.h"
#include "ddm2.h"  // ELEMENTS
#include "ddm2_parameter_list.h"
#include "ddm_store.h"
#include "dometic.h"
#include "esp_attr.h"  // EXT_RAM_ATTR
#include "lindev_generic.h"

#define WORKER_STACK_SIZE         3094
#define WORKER_PRIORITY           xTASK_PRIORITY_NORMAL
#define LIN_TEST_TIME             (2000u) /* set timer time to 2 seconds */
#define LIN_RESET_ERROR_CODE_MASK (0x8000)
#define LIN_TEST_START_REQUESTED  1
#define LIN_TEST_STOP_REQUESTED   0

#define AC0MDL_CURRENT_LIMIT_FEATURE_NOT_REQUIRED -1  // Model discriminates without AC0FEATURE
#define AC0MDL_CURRENT_LIMIT_FEATURE_UNSUPPORTED  0   // AC0FEATURE = 0 (current limit not supported)
#define AC0MDL_CURRENT_LIMIT_FEATURE_SUPPORTED    1   // AC0FEATURE = 1 (current limit supported)

typedef struct lindev_shape_discriminator_table
{
    uint32_t parameter;
    int32_t value;
} lindev_shape_discriminator_table_t;

static int connector_lindev_shape_initialize(void);
static void shape_ac_ctrl_extract(void *bundle, const uint8_t *data);
static void shape_ac_info_stuff(const void *bundle, uint8_t *data);
static void shape_ac2_ctrl_extract(void *bundle, const uint8_t *data);
static void shape_ac2_info_stuff(const void *bundle, uint8_t *data);
static void shape_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index);
static void shape_transceiver_sleep(void *context);
static void shape_transceiver_wakeup(void *context);
static void shape_set_ci_error(bool state);
static void shape_set_not_init(bool state);
static void shape_ddm_has_started(void);
static void shape_ddm_entry_has_changed(const ddm_entry_t *ddm_entry);
static void lindev_timer_timeout(TimerHandle_t xTimer);

static void discriminators_resolve(lindev_shape_discriminator_table_t *table, size_t table_size, uint32_t parameter, int32_t parameter_value);
static const lindev_generic_descriptor_t *shape_device_discriminate(const lindev_shape_discriminator_table_t *table, size_t table_size);
static const lindev_generic_descriptor_t *shape_discriminate_model_with_custom_profile(const lindev_generic_t *lindev_generic, const uint32_t parameter, const int32_t parameter_value);

static uint16_t recv_lin;
static TimerHandle_t xTimer;
static int32_t lin_test_request = LIN_TEST_STOP_REQUESTED;

static const struct ddm_store_ddm shape_ddm_other_initial_values[] = {
    {.ddm_parameter = AC0ON,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0FSPD,
     .value = {.storage = {.i32 = AC0FSPD_LOW}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0MD,
     .value = {.storage = {.i32 = AC0MD_COOL}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0TTEMP,
     .value = {.storage = {.i32 = 16}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0LGT,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0DMR,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0ITEMP,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0ELGT,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0TONA,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0TONM,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0TOFFA,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0TOFFM,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0STATUS,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_STRUCT}},
    {.ddm_parameter = AC0SLEEP,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0HFAVL,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0LFAVL,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0ACTEXT,
     .value = {.storage = {.u32 = 0}, .type = DDM2_TYPE_UINT32_T}},
    {.ddm_parameter = AC0REMCTRL,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0TEST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = AC0CURRLIM,
     .value = {.storage = {.i32 = AC0CURRLIM_UNLIMITED}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = GW0DSN,
     .value = {.storage = {.str = "0000"}, .type = DDM2_TYPE_STRING}}};

DDM_STORE__DECLARE_EXTRAM(shape_ddm_other_store, ELEMENTS(shape_ddm_other_initial_values));

static const uint32_t shape_inventory_list[] = {
    AC0AVL};

/* Sorted list to hold the states of DDM classes we want to subscribe to. */
DECLARE_SORTED_LIST_EXTRAM(shape_inventory_sortedlist, ELEMENTS(shape_inventory_list));

static const uint16_t function_ids[] = {
    0x0006,  // Default (new) Function ID
    0x0004   // Legacy function ID value
};

static const uint8_t shape_fix_variant_ids[] = {
    0x00,  // Default (new) Variant ID
    0x06   // Legacy Variant ID
};

static const uint8_t shape_var_variant_ids[] = {
    0x01,  // Default (new) Variant ID
    0x07   // Legacy Variant ID
};

static const lin_device_config_data_t shape_fix_device_config = {
    .nad = 0x18,
    .supplier_id = 0x1234,
    .function_ids = function_ids,
    .variant_ids = shape_fix_variant_ids,
    .function_variant_ids_size = ELEMENTS(function_ids),
};

static const lin_device_config_data_t shape_inverter_device_config = {
    .nad = 0x18,
    .supplier_id = 0x1234,
    .function_ids = function_ids,
    .variant_ids = shape_var_variant_ids,
    .function_variant_ids_size = ELEMENTS(function_ids),
};

static const lin_device_config_data_t shape_inverter_facelift_device_config = {
    .nad = 0x18,
    .supplier_id = 0x1234,
    .function_id = 0x0007,
    .variant_id = 0x01,
};

/* Shape frame defintions */
static const lindev_frame_def_t shape_fjx_lindev_frame_defs[] = {
    {.initial_id = DOMETIC_SHAPE_AC_CTRL_FRAME_ID, .type = LIN_CONTROL_FRAME},
    {.initial_id = DOMETIC_SHAPE_AC_INFO_FRAME_ID, .type = LIN_INFO_FRAME},
};

/* CTRL frame DOMETIC_SHAPE_AC_CTRL_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_shape_ac_ctrl_bundle_t shape_ac_ctrl_bundle_buffer;

static const lindev_table_ctrl_bundle_t shape_ac_ctrl_bundle =
    LINDEV_TABLE_CTRL_BUNDLE(DOMETIC_SHAPE_AC_CTRL_FRAME_ID, dometic_shape_ac_ctrl_bundle_t, rd, &shape_ac_ctrl_bundle_buffer, NULL, shape_ac_ctrl_extract);

/* INFO frame DOMETIC_SHAPE_AC_INFO_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_shape_ac_info_bundle_t shape_ac_info_bundle_buffer;

static const lindev_table_info_bundle_t shape_ac_info_bundle =
    LINDEV_TABLE_INFO_BUNDLE(DOMETIC_SHAPE_AC_INFO_FRAME_ID, dometic_shape_ac_info_bundle_t, rd, &shape_ac_info_bundle_buffer, NULL, shape_ac_info_stuff);

/* Bundle map */
static const lindev_table_bundle_map_entry_t shape_fjx_bundle_map[] = {
    {.ctrl = &shape_ac_ctrl_bundle, .info = NULL},
    {.ctrl = NULL, .info = &shape_ac_info_bundle}};

/* Shape Facelift frame defintions */
static const lindev_frame_def_t shape_fjz_lindev_frame_defs[] = {
    {.initial_id = DOMETIC_SHAPE_AC_CTRL_FRAME_ID, .type = LIN_CONTROL_FRAME},
    {.initial_id = DOMETIC_SHAPE_AC_INFO_FRAME_ID, .type = LIN_INFO_FRAME},
    {.initial_id = DOMETIC_SHAPE_AC2_CTRL_FRAME_ID, .type = LIN_CONTROL_FRAME},
    {.initial_id = DOMETIC_SHAPE_AC2_INFO_FRAME_ID, .type = LIN_INFO_FRAME},
};

/* CTRL frame DOMETIC_SHAPE_AC2_CTRL_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_shape_ac2_ctrl_bundle_t shape_ac2_ctrl_bundle_buffer;

static const lindev_table_ctrl_bundle_t shape_ac2_ctrl_bundle =
    LINDEV_TABLE_CTRL_BUNDLE(DOMETIC_SHAPE_AC2_CTRL_FRAME_ID, dometic_shape_ac2_ctrl_bundle_t, icl, &shape_ac2_ctrl_bundle_buffer, NULL, shape_ac2_ctrl_extract);

/* INFO frame DOMETIC_SHAPE_AC2_INFO_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_shape_ac2_info_bundle_t shape_ac2_info_bundle_buffer;

static const lindev_table_info_bundle_t shape_ac2_info_bundle =
    LINDEV_TABLE_INFO_BUNDLE(DOMETIC_SHAPE_AC2_INFO_FRAME_ID, dometic_shape_ac2_info_bundle_t, icl, &shape_ac2_info_bundle_buffer, NULL, shape_ac2_info_stuff);

/* Bundle map */
static const lindev_table_bundle_map_entry_t shape_fjz_bundle_map[] = {
    {.ctrl = &shape_ac_ctrl_bundle, .info = NULL},
    {.ctrl = NULL, .info = &shape_ac_info_bundle},
    {.ctrl = &shape_ac2_ctrl_bundle, .info = NULL},
    {.ctrl = NULL, .info = &shape_ac2_info_bundle},
};

/* Shape frames buffer */
static lindev_frame_t shape_lindev_frames[MAX(ELEMENTS(shape_fjx_lindev_frame_defs), ELEMENTS(shape_fjz_lindev_frame_defs))];

/* FJX Ctrl LINK map */
static const lindev_table_ctrl_link_map_entry_t shape_ctrl_link_map[] = {
    /*                      data type                        data member         bundle                    convert function                                  DDM*/
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.md, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_md_to_ac0md, AC0MD),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.sleep, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_sleep_to_ac0sleep, AC0SLEEP),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.toffa, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_toffa_to_ac0toffa, AC0TOFFA),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.tona, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_tona_to_ac0tona, AC0TONA),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.on, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_on_to_ac0on, AC0ON),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.elgt, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_elgt_to_ac0elgt, AC0ELGT),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.fspd, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_fspd_to_ac0fspd, AC0FSPD),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.ttemp, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_ttemp_to_ac0ttemp, AC0TTEMP),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.tonm, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_tonm_to_ac0tonm, AC0TONM),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.toffm, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_toffm_to_ac0toffm, AC0TOFFM),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.lgt_dmr, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_lgt_dmr_to_ac0lgt, AC0LGT),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, rd.lgt_dmr, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_lgt_dmr_to_ac0dmr, AC0DMR),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, st.remctrl, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_st_remctrl_to_ac0remctrl, AC0REMCTRL),
    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac_ctrl_bundle_t, st.actext, &shape_ac_ctrl_bundle, dometic_shape_ac_conv_st_actext_to_ac0actext, AC0ACTEXT),

    LINDEV_TABLE_CTRL_ENTRY(dometic_shape_ac2_ctrl_bundle_t, icl.input_current_limit, &shape_ac2_ctrl_bundle, dometic_shape_ac2_conv_currlim_to_ac0currlim, AC0CURRLIM),
};

/* FJX Info LINK map */
static const lindev_table_info_link_map_entry_t shape_info_link_map[] = {
    /*                      DDM0            DDM1     DDM2   convert function                                   data type                        data member           bundle */
    LINDEV_TABLE_INFO_ENTRY(AC0MD, 0, 0, dometic_shape_ac_conv_ac0md_to_md, dometic_shape_ac_info_bundle_t, rd.md, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0SLEEP, 0, 0, dometic_shape_ac_conv_ac0sleep_to_sleep, dometic_shape_ac_info_bundle_t, rd.sleep, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0TOFFA, 0, 0, dometic_shape_ac_conv_ac0toffa_to_toffa, dometic_shape_ac_info_bundle_t, rd.toffa, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0TONA, 0, 0, dometic_shape_ac_conv_ac0tona_to_tona, dometic_shape_ac_info_bundle_t, rd.tona, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0ON, 0, 0, dometic_shape_ac_conv_ac0on_to_on, dometic_shape_ac_info_bundle_t, rd.on, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0ELGT, 0, 0, dometic_shape_ac_conv_ac0elgt_to_elgt, dometic_shape_ac_info_bundle_t, rd.elgt, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0FSPD, 0, 0, dometic_shape_ac_conv_ac0fspd_to_fspd, dometic_shape_ac_info_bundle_t, rd.fspd, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0TTEMP, 0, 0, dometic_shape_ac_conv_ac0ttemp_to_ttemp, dometic_shape_ac_info_bundle_t, rd.ttemp, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0TONM, 0, 0, dometic_shape_ac_conv_ac0tonm_to_tonm, dometic_shape_ac_info_bundle_t, rd.tonm, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0TOFFM, 0, 0, dometic_shape_ac_conv_ac0toffm_to_toffm, dometic_shape_ac_info_bundle_t, rd.toffm, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0LGT, AC0DMR, 0, dometic_shape_ac_conv_ac0lgt_ac0dmr_to_lgt_dmr, dometic_shape_ac_info_bundle_t, rd.lgt_dmr, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0HFAVL, 0, 0, dometic_shape_ac_conv_ac0hfavl_to_hfavl, dometic_shape_ac_info_bundle_t, rd.hfavl, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0LFAVL, 0, 0, dometic_shape_ac_conv_ac0lfavl_to_lfavl, dometic_shape_ac_info_bundle_t, rd.lfavl, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0REMCTRL, 0, 0, dometic_shape_ac_conv_ac0remctrl_to_rd_remctrl, dometic_shape_ac_info_bundle_t, rd.remctrl, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0ACTEXT, 0, 0, dometic_shape_ac_conv_ac0actext_to_st_actext, dometic_shape_ac_info_bundle_t, st.actext, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0ITEMP, 0, 0, dometic_shape_ac_conv_ac0itemp_to_itemp, dometic_shape_ac_info_bundle_t, st.itemp, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0STATUS, 0, 0, dometic_shape_ac_conv_ac0status_to_status, dometic_shape_ac_info_bundle_t, st.status, &shape_ac_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(AC0REMCTRL, 0, 0, dometic_shape_ac_conv_ac0remctrl_to_st_remctrl, dometic_shape_ac_info_bundle_t, st.remctrl, &shape_ac_info_bundle),

    LINDEV_TABLE_INFO_ENTRY(AC0CURRLIM, 0, 0, dometic_shape_ac2_conv_ac0currlim_to_currlim, dometic_shape_ac2_info_bundle_t, icl.input_current_limit, &shape_ac2_info_bundle),
};

static const lindev_table_config_t shape_fjx_lindev_table_config = {
    .sync_method = LINDEV_TABLE_SYNC_METHOD_SIMPLE,
    .bundle_map = shape_fjx_bundle_map,
    .bundle_map_length = ELEMENTS(shape_fjx_bundle_map),
    .ctrl_link_map = shape_ctrl_link_map,
    .ctrl_link_map_length = ELEMENTS(shape_ctrl_link_map),
    .info_link_map = shape_info_link_map,
    .info_link_map_length = ELEMENTS(shape_info_link_map),
};

static const lindev_table_config_t shape_fjz_lindev_table_config = {
    .sync_method = LINDEV_TABLE_SYNC_METHOD_SIMPLE,
    .bundle_map = shape_fjz_bundle_map,
    .bundle_map_length = ELEMENTS(shape_fjz_bundle_map),
    .ctrl_link_map = shape_ctrl_link_map,
    .ctrl_link_map_length = ELEMENTS(shape_ctrl_link_map),
    .info_link_map = shape_info_link_map,
    .info_link_map_length = ELEMENTS(shape_info_link_map),
};

lindev_generic_t shape_lindev_generic;

static const lindev_uart_config_t shape_lindev_uart_config = {
    .context = &shape_lindev_generic,
    .process_event = shape_process_event,
    .sleep = shape_transceiver_sleep,
    .wakeup = shape_transceiver_wakeup,
    .sleep_timeout_ms = 4000,
    .start_after_ms = 2000,
};

static const lindev_generic_config_t shape_lindev_generic_config = {
    .worker_stack_size = WORKER_STACK_SIZE,
    .worker_priority = WORKER_PRIORITY,
    .inventory_list = shape_inventory_list,
    .inventory_size = ELEMENTS(shape_inventory_list),
    .inventory_sortedlist = &shape_inventory_sortedlist,
};

static const lindev_generic_descriptor_t shape_fix_descriptor = {
    .name = "Shape FJX4/FJZ4",
    .ddm_other_store = &shape_ddm_other_store,
    .ddm_other_initial_values = shape_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(shape_ddm_other_initial_values),
    .device_config = &shape_fix_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = shape_lindev_frames,
    .lindev_frame_defs = shape_fjx_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(shape_fjx_lindev_frame_defs),
    .lindev_table_config = &shape_fjx_lindev_table_config,
    .cb_ddm_loop_has_started = shape_ddm_has_started,
    .cb_ddm_entry_has_changed = shape_ddm_entry_has_changed,
};

static const lindev_generic_descriptor_t shape_inverter_descriptor = {
    .name = "Shape FJX7/FJZ7",
    .ddm_other_store = &shape_ddm_other_store,
    .ddm_other_initial_values = shape_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(shape_ddm_other_initial_values),
    .device_config = &shape_inverter_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = shape_lindev_frames,
    .lindev_frame_defs = shape_fjx_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(shape_fjx_lindev_frame_defs),
    .lindev_table_config = &shape_fjx_lindev_table_config,
    .cb_ddm_loop_has_started = shape_ddm_has_started,
    .cb_ddm_entry_has_changed = shape_ddm_entry_has_changed,
};

static const lindev_generic_descriptor_t shape_inverter_facelift_descriptor = {
    .name = "Shape FJZ7",
    .ddm_other_store = &shape_ddm_other_store,
    .ddm_other_initial_values = shape_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(shape_ddm_other_initial_values),
    .device_config = &shape_inverter_facelift_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = shape_lindev_frames,
    .lindev_frame_defs = shape_fjz_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(shape_fjz_lindev_frame_defs),
    .lindev_table_config = &shape_fjz_lindev_table_config,
    .cb_ddm_loop_has_started = shape_ddm_has_started,
    .cb_ddm_entry_has_changed = shape_ddm_entry_has_changed,
};

static const uint32_t shape_discriminator_list[] = {
    AC0MDL,
    AC0FEATURE};

static const lindev_generic_profile_t shape_profile = {
    .profile_type = LINDEV_GENERIC_PROFILE_TYPE_CUSTOM,
    .discriminator_list = shape_discriminator_list,
    .discriminator_list_size = ELEMENTS(shape_discriminator_list),
    .custom_profile = {
        .cb_discriminate_model = shape_discriminate_model_with_custom_profile,
    }};

static const struct shape_profile_entry
{
    int32_t model;
    int32_t current_limit_feature;
    const lindev_generic_descriptor_t *descriptor;
} shape_profile_entries[] = {
    // Models that don't need AC0FEATURE for discrimination
    {.model = AC0MDL_DOMETIC_FJX4000_SERIES, .current_limit_feature = AC0MDL_CURRENT_LIMIT_FEATURE_NOT_REQUIRED, .descriptor = &shape_fix_descriptor},
    {.model = AC0MDL_DOMETIC_FJX7000_SERIES, .current_limit_feature = AC0MDL_CURRENT_LIMIT_FEATURE_NOT_REQUIRED, .descriptor = &shape_inverter_descriptor},
    {.model = AC0MDL_DOMETIC_FJZ4000_SERIES, .current_limit_feature = AC0MDL_CURRENT_LIMIT_FEATURE_NOT_REQUIRED, .descriptor = &shape_fix_descriptor},
    // FJZ7000 models that need AC0FEATURE for discrimination
    {.model = AC0MDL_DOMETIC_FJZ7000_SERIES, .current_limit_feature = AC0MDL_CURRENT_LIMIT_FEATURE_UNSUPPORTED, .descriptor = &shape_inverter_descriptor},
    {.model = AC0MDL_DOMETIC_FJZ7000_SERIES, .current_limit_feature = AC0MDL_CURRENT_LIMIT_FEATURE_SUPPORTED, .descriptor = &shape_inverter_facelift_descriptor},
};

CONNECTOR connector_lindev_shape = {
    .name = "LIN device",
    .initialize = connector_lindev_shape_initialize};

static void discriminators_resolve(lindev_shape_discriminator_table_t *table, size_t table_size, uint32_t parameter, int32_t parameter_value)
{
    for (size_t n_discriminators = 0; n_discriminators < table_size; n_discriminators++)
    {
        if (table[n_discriminators].parameter == parameter)
        {
            /* NOTE: We work under the assumption that the first publish of AC0FEATURE was with the CURRENTLIMIT bit updated */
            table[n_discriminators].value = parameter_value;
            break;
        }
    }
}

static const lindev_generic_descriptor_t *shape_device_discriminate(const lindev_shape_discriminator_table_t *table, size_t table_size)
{
    bool can_discriminate = false;
    int32_t model_value = -1;
    int32_t current_limit_feature_value = -1;
    const lindev_generic_descriptor_t *descriptor = NULL;

    if (table[0].value != -1)  // model
    {
        model_value = table[0].value;
        if (model_value == AC0MDL_DOMETIC_FJZ7000_SERIES)
        {
            if (table[1].value != -1)  // current limit feature
            {
                /* For FJZ7000 series CURRENTLIMIT feature is mandatory discriminator */
                current_limit_feature_value = AC0FEATURE_CURRENTLIMIT_GET(table[1].value);
                can_discriminate = true;
            }
            else
            {
                /* still waiting for current limit feature discriminator */
                can_discriminate = false;
            }
        }
        else
        {
            /* For other models CURRENTLIMIT feature is not supported */
            current_limit_feature_value = AC0MDL_CURRENT_LIMIT_FEATURE_NOT_REQUIRED;
            can_discriminate = true;
        }
    }
    else
    {
        /* still waiting for model discriminator */
        can_discriminate = false;
    }

    if (can_discriminate)
    {
        for (size_t n_shape_profiles = 0; n_shape_profiles < ELEMENTS(shape_profile_entries); n_shape_profiles++)
        {
            if ((shape_profile_entries[n_shape_profiles].model == model_value) &&
                (shape_profile_entries[n_shape_profiles].current_limit_feature == current_limit_feature_value))
            {
                descriptor = shape_profile_entries[n_shape_profiles].descriptor;
                break;
            }
        }
        if (descriptor == NULL)
        {
            LOG(E, "%s: Shape AC model(%d) with Current Limit(%d) feature not supported!",
                connector_lindev_shape.name,
                model_value,
                current_limit_feature_value);
        }
        else
        {
            LOG(I, "%s: Shape AC model(%d)(%s) with Current Limit(%d) feature detected.",
                connector_lindev_shape.name,
                model_value,
                descriptor->name,
                current_limit_feature_value);
        }
    }

    return descriptor;
}

static const lindev_generic_descriptor_t *shape_discriminate_model_with_custom_profile(const lindev_generic_t *lindev_generic, const uint32_t parameter, const int32_t parameter_value)
{
    const lindev_generic_descriptor_t *descriptor = NULL;
    static lindev_shape_discriminator_table_t discriminator_table[] = {
        {.parameter = AC0MDL, .value = -1},
        {.parameter = AC0FEATURE, .value = -1},
    };  // This table should match shape_discriminator_list

    discriminators_resolve(discriminator_table, ELEMENTS(discriminator_table), parameter, parameter_value);
    descriptor = shape_device_discriminate(discriminator_table, ELEMENTS(discriminator_table));

    return descriptor;
}

static int connector_lindev_shape_initialize(void)
{
    shape_set_ci_error(false);
    shape_set_not_init(true);

    return lindev_generic_init_with_profile(&shape_lindev_generic,
                                            &connector_lindev_shape,
                                            &shape_lindev_uart_config,
                                            &shape_lindev_generic_config,
                                            &shape_profile);
}

static void handle_lin_test_counter(void)
{
    /* check if LIN test is requested */
    if (lin_test_request == LIN_TEST_START_REQUESTED)
    {
        /* increment received LIN frames counter */
        recv_lin++;
    }
    else
    {
        /* LIN test is not requested */
    }
}

static void lindev_timer_timeout(TimerHandle_t xTimer)
{
    int32_t status;
    /* check if received any frames during test time */
    if (recv_lin > 0)
    {
        status = (LIN_RESET_ERROR_CODE_MASK | GENERIC_CI_BUS_SELFTEST_ERROR); /* set no error for test */
    }
    else
    {
        status = GENERIC_CI_BUS_SELFTEST_ERROR; /* set error for test */
    }
    LOG(I, "LIN test status - 0x%04x, frames received - %d", status, recv_lin);
    recv_lin = 0; /* reset received LIN frames counter */

    /*Write updated value to DDMP */
    connector_send_frame_to_broker(
        /* control */ DDMP2_CONTROL_SET,
        /* parameter */ AC0STATUS,
        /* value */ &status,
        /* value_size */ sizeof(status),
        /* connector */ connector_lindev_shape.connector_id,
        /* timeout */ portMAX_DELAY);
}

static void handle_lin_test_request(void)
{
    static bool lin_test_start = true;

    /* check if test is requested */
    if (lin_test_request == LIN_TEST_START_REQUESTED)
    {
        if (lin_test_start)
        {
            TRUE_CHECK(xTimer = xTimerCreate(NULL, pdMS_TO_TICKS(LIN_TEST_TIME), pdTRUE, NULL, lindev_timer_timeout));
            if (xTimerStart(xTimer, 0) != pdPASS)
            {
                LOG(E, "Timer start attempt failed");
            }
            LOG(I, "LIN test timer started");
            lin_test_start = false;
        }
    }
    else if (lin_test_request == LIN_TEST_STOP_REQUESTED)
    {
        recv_lin = 0; /* reset received LIN frames counter */
        lin_test_start = true;
        /* LIN test is not requested */
        if (xTimer != NULL)
        {
            if (xTimerStop(xTimer, 0) != pdPASS)
            {
                LOG(E, "Timer stop attempt failed");
            }
            else
            {
                LOG(I, "LIN test timer stopped");
            }
            xTimerDelete(xTimer, portMAX_DELAY);
        }
        else
        {
            /* timer is not created or was already stoped */
        }
    }
    else
    {
        LOG(W, "Wrong LIN selftest request - %d", lin_test_request);
    }
}

static void shape_ac_ctrl_extract(void *bundle, const uint8_t *data)
{
    dometic_shape_ac_ctrl_extract(bundle, data);
}

static void shape_ac_info_stuff(const void *bundle, uint8_t *data)
{
    dometic_shape_ac_info_stuff(bundle, data);
}

static void shape_ac2_ctrl_extract(void *bundle, const uint8_t *data)
{
    dometic_shape_ac2_ctrl_extract(bundle, data);
}

static void shape_ac2_info_stuff(const void *bundle, uint8_t *data)
{
    dometic_shape_ac2_info_stuff(bundle, data);
}

static void shape_transceiver_sleep(void *context)
{
    (void)context;  // Not used
    LIN_SLEEP(0);
}

static void shape_transceiver_wakeup(void *context)
{
    (void)context;  // Not used
    LIN_SLEEP(1);
}

static void shape_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index)
{
    switch (event_id)
    {
    case LINDEV_EVENT_ID_INFO_FRAME:
        shape_set_ci_error(false);
        /* FALLTHROUGH */
    case LINDEV_EVENT_ID_CONTROL_FRAME:
    case LINDEV_EVENT_ID_DIAG_REQ_FRAME:
    case LINDEV_EVENT_ID_DIAG_RESP_FRAME:
        lindev_generic_process_event(context, event_id, frame_index);
        break;
    default:
        shape_set_ci_error(true);
    }

    if ((event_id == LINDEV_EVENT_ID_INFO_FRAME) ||
        (event_id == LINDEV_EVENT_ID_CONTROL_FRAME) ||
        (event_id == LINDEV_EVENT_ID_DIAG_REQ_FRAME) ||
        (event_id == LINDEV_EVENT_ID_DIAG_RESP_FRAME) ||
        (event_id == LINDEV_EVENT_ID_NOT_FOR_US))
    {
        handle_lin_test_counter();
    }
}

static void shape_set_ci_error(bool state)
{
    shape_ac_info_bundle_buffer.st.ci_error = state ? 1 : 0;
}

static void shape_set_not_init(bool state)
{
    shape_ac_info_bundle_buffer.st.not_init = state ? 1 : 0;
}

static void shape_ddm_has_started(void)
{
    shape_set_not_init(false);
}

static void shape_ddm_entry_has_changed(const ddm_entry_t *ddm_entry)
{
    if (ddm_entry__parameter_id(ddm_entry) == GW0DSN)
    {
        /* The following implementation is done based on old Sharc code. In Shape this was never
         * handled at all.
         */
        /* TODO: Until this gets defined/cleared, use string as 4 bytes to form 32-bit Serial Number,
         * as it was done in old Sharc
         */
        const char *dsn;

        dsn = ddm_entry__value_str(ddm_entry);
        if (dsn != NULL)
        {
            lindev_generic_set_serial_number(&shape_lindev_generic, *(const uint32_t *)dsn);
        }
        else
        {
            lindev_generic_set_serial_number(&shape_lindev_generic, 0u);
        }
    }
    if (ddm_entry__parameter_id(ddm_entry) == AC0TEST)
    {
        if (lin_test_request != ddm_entry->p__value.storage.i32)
        {
            lin_test_request = ddm_entry->p__value.storage.i32;
            LOG(I, "LIN test requested, AC0TEST = %d", ddm_entry->p__value.storage.i32);
            handle_lin_test_request();
        }
        else
        {
            LOG(I, "No change AC0TEST = %d", ddm_entry->p__value.storage.i32);
        }
    }
}

#endif  // CONNECTOR_LINDEV_SHAPE
