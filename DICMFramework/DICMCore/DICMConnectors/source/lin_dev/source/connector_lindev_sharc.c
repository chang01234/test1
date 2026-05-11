/*! \file
    \brief Connector LINDEV for Sharc product
 */

#include "configuration.h"

#ifdef CONNECTOR_LINDEV_SHARC

#include "connector_lindev.h"
#include "ddm2.h"  // ELEMENTS
#include "ddm2_parameter_list.h"
#include "ddm_store.h"
#include "dometic.h"
#include "esp_attr.h"  // EXT_RAM_ATTR
#include "lindev_generic.h"

/*
 * Sharc product has multiple models. Models are switched dynamically based on a DDM parameter value.
 *
 * Since more work remains on DDM parameter side, this switching is disabled by default and it is
 * hardcoded to a single Sharc model. In future, when DDM parameter side work is completed this
 * model switching will be enabled and relevant pre-processor code will be removed from this file.
 */
#ifndef CONNECTOR_LINDEV_SHARC_MODEL_DISCRIMINATION
#define CONNECTOR_LINDEV_SHARC_MODEL_DISCRIMINATION 0
#endif

#define WORKER_STACK_SIZE 3094
#define WORKER_PRIORITY   xTASK_PRIORITY_NORMAL

static int connector_lindev_sharc_initialize(void);
static void sharc_wtr_ctrl_extract(void *bundle, const uint8_t *data);
static void sharc_air_ctrl_extract(void *bundle, const uint8_t *data);
static void sharc_wtr_info_stuff(const void *bundle, uint8_t *data);
static void sharc_air_info_stuff(const void *bundle, uint8_t *data);
static void sharc_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index);
static void sharc_transceiver_sleep(void *context);
static void sharc_transceiver_wakeup(void *context);
static void sharc_set_ci_error(bool state);
static void sharc_ddm_entry_has_changed(const ddm_entry_t *ddm_entry);

static const struct ddm_store_ddm sharc_ddm_other_initial_values[] = {
    // Needed to get current Device Serial Number
    {.ddm_parameter = GW0DSN,
     .value = {.storage = {.str = "0000"}, .type = DDM2_TYPE_STRING}},
    {.ddm_parameter = HTR0AON,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ATEMP,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ESEL,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0AMD,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0SMAXFAN,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0VMINFAN,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0RTS,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0WTRTEMP,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0WTRON,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0TIMES,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0TIMEM,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0TIMEH,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0DATED,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0DATEM,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0DATEY,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ERRCD1,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ERRCD2,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ERRCD3,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ERRCD4,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0WTRTS,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ACWTRHST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0GASWTRHST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ACST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0AHTOFFST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0AHTONST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0WTRTST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HTR0ERRST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
};

DDM_STORE__DECLARE_EXTRAM(sharc_ddm_other_store, ELEMENTS(sharc_ddm_other_initial_values));

static const uint32_t sharc_inventory_list[] = {
    HTR0AVL};

/* Sorted list to hold the states of DDM classes we want to subscribe to. */
DECLARE_SORTED_LIST_EXTRAM(sharc_inventory_sortedlist, ELEMENTS(sharc_inventory_list));

static const lin_device_config_data_t sharc_ch4000_device_config = {
    .nad = 0x10,
    .supplier_id = 0x1234,
    .function_id = 0x8001,
    .variant_id = 0x0,
};

#if (CONNECTOR_LINDEV_SHARC_MODEL_DISCRIMINATION == 1)
static const lin_device_config_data_t sharc_ch4000e_device_config = {
    .nad = 0x10,
    .supplier_id = 0x1234,
    .function_id = 0x8001,
    .variant_id = 0x1,
};

static const lin_device_config_data_t sharc_ch6000_device_config = {
    .nad = 0x10,
    .supplier_id = 0x1234,
    .function_id = 0x8000,
    .variant_id = 0x0,
};
static const lin_device_config_data_t sharc_ch6000e_device_config = {
    .nad = 0x10,
    .supplier_id = 0x1234,
    .function_id = 0x8000,
    .variant_id = 0x1,
};
#endif /* CONNECTOR_LINDEV_SHARC_MODEL_DISCRIMINATION == 1 */

static const lindev_frame_def_t sharc_lindev_frame_defs[] = {
    {.initial_id = DOMETIC_SHARC_WTR_CTRL_ID, .type = LIN_CONTROL_FRAME},
    {.initial_id = DOMETIC_SHARC_WTR_INFO_ID, .type = LIN_INFO_FRAME},
    {.initial_id = DOMETIC_SHARC_AIR_CTRL_ID, .type = LIN_CONTROL_FRAME},
    {.initial_id = DOMETIC_SHARC_AIR_INFO_ID, .type = LIN_INFO_FRAME},
};

static lindev_frame_t sharc_lindev_frames[ELEMENTS(sharc_lindev_frame_defs)];

/* CTRL frame DOMETIC_SHARC_WTR_CTRL_ID bundle structures */
static EXT_RAM_ATTR dometic_sharc_wtr_ctrl_bundle_t sharc_wtr_ctrl_bundle_buffer;

static const lindev_table_ctrl_protocol_t sharc_wtr_ctrl_protocol =
    LINDEV_TABLE_PAGED_CTRL_PROTOCOL_NO_SYNC(dometic_sharc_wtr_ctrl_bundle_t, protocol.is_sync_frame, protocol.ctrl_page, protocol.info_page);

static const lindev_table_ctrl_bundle_t sharc_wtr_ctrl_bundle =
    LINDEV_TABLE_PAGED_CTRL_BUNDLE_2(DOMETIC_SHARC_WTR_CTRL_ID, dometic_sharc_wtr_ctrl_bundle_t, rd.page0, rd.page1, &sharc_wtr_ctrl_bundle_buffer, &sharc_wtr_ctrl_protocol, sharc_wtr_ctrl_extract);

/* INFO frame DOMETIC_SHARC_WTR_INFO_ID bundle structures */
static EXT_RAM_ATTR dometic_sharc_wtr_info_bundle_t sharc_wtr_info_bundle_buffer;

static const lindev_table_info_protocol_t sharc_wtr_info_protocol =
    LINDEV_TABLE_PAGED_INFO_PROTOCOL_NO_SYNC(dometic_sharc_wtr_info_bundle_t, protocol.is_local_change_frame, protocol.info_page, 3);

static const lindev_table_info_bundle_t sharc_wtr_info_bundle =
    LINDEV_TABLE_PAGED_INFO_BUNDLE_2(DOMETIC_SHARC_WTR_INFO_ID, dometic_sharc_wtr_info_bundle_t, rd.page0, rd.page1, &sharc_wtr_info_bundle_buffer, &sharc_wtr_info_protocol, sharc_wtr_info_stuff);

/* CTRL frame DOMETIC_SHARC_AIR_CTRL_ID bundle structures */
static EXT_RAM_ATTR dometic_sharc_air_ctrl_bundle_t sharc_air_ctrl_bundle_buffer;

static const lindev_table_ctrl_protocol_t sharc_air_ctrl_protocol =
    LINDEV_TABLE_PAGED_CTRL_PROTOCOL_NO_SYNC(dometic_sharc_air_ctrl_bundle_t, protocol.is_sync_frame, protocol.ctrl_page, protocol.info_page);

static const lindev_table_ctrl_bundle_t sharc_air_ctrl_bundle =
    LINDEV_TABLE_PAGED_CTRL_BUNDLE_1(DOMETIC_SHARC_AIR_CTRL_ID, dometic_sharc_air_ctrl_bundle_t, rd.page0, &sharc_air_ctrl_bundle_buffer, &sharc_air_ctrl_protocol, sharc_air_ctrl_extract);

/* INFO frame DOMETIC_SHARC_AIR_INFO_ID bundle structures */
static EXT_RAM_ATTR dometic_sharc_air_info_bundle_t sharc_air_info_bundle_buffer;

static const lindev_table_info_protocol_t sharc_air_info_protocol =
    LINDEV_TABLE_PAGED_INFO_PROTOCOL_NO_SYNC(dometic_sharc_air_info_bundle_t, protocol.is_local_change_frame, protocol.info_page, 0);

static const lindev_table_info_bundle_t sharc_air_info_bundle =
    LINDEV_TABLE_PAGED_INFO_BUNDLE_1(DOMETIC_SHARC_AIR_INFO_ID, dometic_sharc_air_info_bundle_t, rd.page0, &sharc_air_info_bundle_buffer, &sharc_air_info_protocol, sharc_air_info_stuff);

/* Bundle map */
static const lindev_table_bundle_map_entry_t sharc_bundle_map[] = {
    {.ctrl = &sharc_wtr_ctrl_bundle, .info = &sharc_wtr_info_bundle},
    {.ctrl = &sharc_air_ctrl_bundle, .info = &sharc_air_info_bundle},
};

/* Ctrl LINK map */
static const lindev_table_ctrl_link_map_entry_t sharc_ctrl_link_map[] = {
    /* DOMETIC_SHARC_WTR_CTRL_ID - Page 0 */
    /*                            data type                        data member           page    bundle                  convert function                                DDM*/
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page0.wtrtemp, 0, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_wtrtemp_to_htr0wtrtemp, HTR0WTRTEMP),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page0.wtron, 0, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_wtron_to_htr0wtron, HTR0WTRON),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page0.esel, 0, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_esel_to_htr0esel, HTR0ESEL),
    /* DOMETIC_SHARC_WTR_CTRL_ID - Page 1 */
    /*                            data type                        data member           page    bundle                  convert function                                DDM*/
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page1.times, 1, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_times_to_htr0times, HTR0TIMES),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page1.timem, 1, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_timem_to_htr0timem, HTR0TIMEM),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page1.timeh, 1, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_timeh_to_htr0timeh, HTR0TIMEH),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page1.datey, 1, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_datey_to_htr0datey, HTR0DATEY),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page1.datem, 1, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_datem_to_htr0datem, HTR0DATEM),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_wtr_ctrl_bundle_t, rd.page1.dated, 1, &sharc_wtr_ctrl_bundle, dometic_sharc_wtr_conv_dated_to_htr0dated, HTR0DATED),
    /* DOMETIC_SHARC_WTR_CTRL_ID - Page 2 */
    /* Nothing mapped on this frame & page */
    /* DOMETIC_SHARC_AIR_CTRL_ID - Page 0 */
    /*                            data type                        data member           page    bundle                  convert function                                DDM*/
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_air_ctrl_bundle_t, rd.page0.atemp, 0, &sharc_air_ctrl_bundle, dometic_sharc_air_conv_atemp_to_htr0atemp, HTR0ATEMP),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_air_ctrl_bundle_t, rd.page0.esel, 0, &sharc_air_ctrl_bundle, dometic_sharc_air_conv_esel_to_htr0esel, HTR0ESEL),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_air_ctrl_bundle_t, rd.page0.amd, 0, &sharc_air_ctrl_bundle, dometic_sharc_air_conv_amd_to_htr0amd, HTR0AMD),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_air_ctrl_bundle_t, rd.page0.smaxfan, 0, &sharc_air_ctrl_bundle, dometic_sharc_air_conv_smaxfan_to_htr0smaxfan, HTR0SMAXFAN),
    LINDEV_TABLE_PAGED_CTRL_ENTRY(dometic_sharc_air_ctrl_bundle_t, rd.page0.vminfan, 0, &sharc_air_ctrl_bundle, dometic_sharc_air_conv_vminfan_to_htr0vminfan, HTR0VMINFAN),
};

/* Info LINK map */
static const lindev_table_info_link_map_entry_t sharc_info_link_map[] = {
    /* DOMETIC_SHARC_WTR_INFO_ID - Page 0 */
    /*                            DDM0          DDM1     DDM2   convert function                                    data type                        data member        page  bundle */
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0WTRTEMP, 0, 0, dometic_sharc_wtr_conv_htr0wtrtemp_to_wtrtemp, dometic_sharc_wtr_info_bundle_t, rd.page0.wtrtemp, 0, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0WTRON, 0, 0, dometic_sharc_wtr_conv_htr0wtron_to_wtron, dometic_sharc_wtr_info_bundle_t, rd.page0.wtron, 0, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0AON, 0, 0, dometic_sharc_wtr_conv_htr0aon_to_aon, dometic_sharc_wtr_info_bundle_t, st.page0.aon, 0, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ESEL, 0, 0, dometic_sharc_wtr_conv_htr0esel_to_esel, dometic_sharc_wtr_info_bundle_t, rd.page0.esel, 0, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0WTRTS, 0, 0, dometic_sharc_wtr_conv_htr0wtrts_to_wtrts, dometic_sharc_wtr_info_bundle_t, st.page0.wtrts, 0, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ACWTRHST, 0, 0, dometic_sharc_wtr_conv_htr0acwtrhst_to_acwtrhst, dometic_sharc_wtr_info_bundle_t, st.page0.acwtrhst, 0, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0GASWTRHST, 0, 0, dometic_sharc_wtr_conv_htr0gaswtrhst_to_gaswtrhst, dometic_sharc_wtr_info_bundle_t, st.page0.gaswtrhst, 0, &sharc_wtr_info_bundle),
    /* DOMETIC_SHARC_WTR_INFO_ID - Page 1 */
    /*                            DDM0          DDM1     DDM2   convert function                                data type                        data member        page  bundle */
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0TIMES, 0, 0, dometic_sharc_wtr_conv_htr0times_to_times, dometic_sharc_wtr_info_bundle_t, rd.page1.times, 1, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0TIMEM, 0, 0, dometic_sharc_wtr_conv_htr0timem_to_timem, dometic_sharc_wtr_info_bundle_t, rd.page1.timem, 1, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0TIMEH, 0, 0, dometic_sharc_wtr_conv_htr0timeh_to_timeh, dometic_sharc_wtr_info_bundle_t, rd.page1.timeh, 1, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0DATED, 0, 0, dometic_sharc_wtr_conv_htr0dated_to_dated, dometic_sharc_wtr_info_bundle_t, rd.page1.dated, 1, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0DATEM, 0, 0, dometic_sharc_wtr_conv_htr0datem_to_datem, dometic_sharc_wtr_info_bundle_t, rd.page1.datem, 1, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0DATEY, 0, 0, dometic_sharc_wtr_conv_htr0datey_to_datey, dometic_sharc_wtr_info_bundle_t, rd.page1.datey, 1, &sharc_wtr_info_bundle),
    /* DOMETIC_SHARC_WTR_INFO_ID - Page 2 */
    /*                            DDM0          DDM1     DDM2   convert function                                data type                        data member        page  bundle */
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ERRCD1, 0, 0, dometic_sharc_wtr_conv_htr0errcd1_to_errcd1, dometic_sharc_wtr_info_bundle_t, st.page2.errcd1, 2, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ERRCD2, 0, 0, dometic_sharc_wtr_conv_htr0errcd2_to_errcd2, dometic_sharc_wtr_info_bundle_t, st.page2.errcd2, 2, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ERRCD3, 0, 0, dometic_sharc_wtr_conv_htr0errcd3_to_errcd3, dometic_sharc_wtr_info_bundle_t, st.page2.errcd3, 2, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ERRCD4, 0, 0, dometic_sharc_wtr_conv_htr0errcd4_to_errcd4, dometic_sharc_wtr_info_bundle_t, st.page2.errcd4, 2, &sharc_wtr_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ERRST, 0, 0, dometic_sharc_wtr_conv_htr0errst_to_errst, dometic_sharc_wtr_info_bundle_t, st.page2.errst, 2, &sharc_wtr_info_bundle),
    /* DOMETIC_SHARC_AIR_INFO_ID - Page 0 */
    /*                            DDM0          DDM1     DDM2   convert function                                data type                        data member        page  bundle */
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ATEMP, 0, 0, dometic_sharc_air_conv_htr0atemp_to_atemp, dometic_sharc_air_info_bundle_t, rd.page0.atemp, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0AON, 0, 0, dometic_sharc_air_conv_htr0aon_to_aon, dometic_sharc_air_info_bundle_t, st.page0.aon, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ESEL, 0, 0, dometic_sharc_air_conv_htr0esel_to_esel, dometic_sharc_air_info_bundle_t, rd.page0.esel, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0AMD, 0, 0, dometic_sharc_air_conv_htr0amd_to_amd, dometic_sharc_air_info_bundle_t, rd.page0.amd, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0SMAXFAN, 0, 0, dometic_sharc_air_conv_htr0smaxfan_to_smaxfan, dometic_sharc_air_info_bundle_t, rd.page0.smaxfan, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0VMINFAN, 0, 0, dometic_sharc_air_conv_htr0vminfan_to_vminfan, dometic_sharc_air_info_bundle_t, rd.page0.vminfan, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0RTS, 0, 0, dometic_sharc_air_conv_htr0rts_to_st_rts, dometic_sharc_air_info_bundle_t, st.page0.rts, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0ACST, 0, 0, dometic_sharc_air_conv_htr0acst_to_acst, dometic_sharc_air_info_bundle_t, st.page0.acst, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0AHTOFFST, 0, 0, dometic_sharc_air_conv_htr0ahtoffst_to_ahtoffst, dometic_sharc_air_info_bundle_t, st.page0.ahtoffst, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0AHTONST, 0, 0, dometic_sharc_air_conv_htr0ahtonst_to_ahtonst, dometic_sharc_air_info_bundle_t, st.page0.ahtonst, 0, &sharc_air_info_bundle),
    LINDEV_TABLE_PAGED_INFO_ENTRY(HTR0WTRTST, 0, 0, dometic_sharc_air_conv_htr0wtrtst_to_wtrtst, dometic_sharc_air_info_bundle_t, st.page0.wtrtst, 0, &sharc_air_info_bundle),
};

static const lindev_table_config_t sharc_lindev_table_config = {
    .sync_method = LINDEV_TABLE_SYNC_METHOD_SIMPLE,
    .bundle_map = sharc_bundle_map,
    .bundle_map_length = ELEMENTS(sharc_bundle_map),
    .ctrl_link_map = sharc_ctrl_link_map,
    .ctrl_link_map_length = ELEMENTS(sharc_ctrl_link_map),
    .info_link_map = sharc_info_link_map,
    .info_link_map_length = ELEMENTS(sharc_info_link_map),
};

lindev_generic_t sharc_lindev_generic;

static const lindev_uart_config_t sharc_lindev_uart_config = {
    .context = &sharc_lindev_generic,
    .process_event = sharc_process_event,
    .sleep = sharc_transceiver_sleep,
    .wakeup = sharc_transceiver_wakeup,
    .sleep_timeout_ms = 4000,
    .start_after_ms = 2000,
};

static const lindev_generic_config_t sharc_lindev_generic_config = {
    .worker_stack_size = WORKER_STACK_SIZE,
    .worker_priority = WORKER_PRIORITY,
    .inventory_list = sharc_inventory_list,
    .inventory_size = ELEMENTS(sharc_inventory_list),
    .inventory_sortedlist = &sharc_inventory_sortedlist,
};

static const lindev_generic_descriptor_t sharc_ch4000_descriptor = {
    .name = "Sharc CH4000",
    .ddm_other_store = &sharc_ddm_other_store,
    .ddm_other_initial_values = sharc_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(sharc_ddm_other_initial_values),
    .device_config = &sharc_ch4000_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = sharc_lindev_frames,
    .lindev_frame_defs = sharc_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(sharc_lindev_frame_defs),
    .lindev_table_config = &sharc_lindev_table_config,
    .cb_ddm_entry_has_changed = sharc_ddm_entry_has_changed,
};

#if (CONNECTOR_LINDEV_SHARC_MODEL_DISCRIMINATION == 1)
static const lindev_generic_descriptor_t sharc_ch4000e_descriptor = {
    .name = "Sharc CH400E",
    .ddm_other_store = &sharc_ddm_other_store,
    .ddm_other_initial_values = sharc_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(sharc_ddm_other_initial_values),
    .device_config = &sharc_ch4000e_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = sharc_lindev_frames,
    .lindev_frame_defs = sharc_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(sharc_lindev_frame_defs),
    .lindev_table_config = &sharc_lindev_table_config,
    .cb_ddm_entry_has_changed = sharc_ddm_entry_has_changed,
};

static const lindev_generic_descriptor_t sharc_ch6000_descriptor = {
    .name = "Sharc CH6000",
    .ddm_other_store = &sharc_ddm_other_store,
    .ddm_other_initial_values = sharc_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(sharc_ddm_other_initial_values),
    .device_config = &sharc_ch6000_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = sharc_lindev_frames,
    .lindev_frame_defs = sharc_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(sharc_lindev_frame_defs),
    .lindev_table_config = &sharc_lindev_table_config,
    .cb_ddm_entry_has_changed = sharc_ddm_entry_has_changed,
};

static const lindev_generic_descriptor_t sharc_ch6000e_descriptor = {
    .name = "Sharc CH6000E",
    .ddm_other_store = &sharc_ddm_other_store,
    .ddm_other_initial_values = sharc_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(sharc_ddm_other_initial_values),
    .device_config = &sharc_ch6000e_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = sharc_lindev_frames,
    .lindev_frame_defs = sharc_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(sharc_lindev_frame_defs),
    .lindev_table_config = &sharc_lindev_table_config,
    .cb_ddm_entry_has_changed = sharc_ddm_entry_has_changed,
};

static const lindev_generic_profile_entry_t sharc_profile_entries[] = {
    {.ddm_value = HTR0MDL_SHARC_CH4000, &sharc_ch4000_descriptor},
    {.ddm_value = HTR0MDL_SHARC_CH4000E, &sharc_ch4000e_descriptor},
    {.ddm_value = HTR0MDL_SHARC_CH6000, &sharc_ch6000_descriptor},
    {.ddm_value = HTR0MDL_SHARC_CH6000E, &sharc_ch6000e_descriptor},
};
static const uint32_t sharc_discriminator_list[] = {
    HTR0MDL};

static const lindev_generic_profile_t sharc_profile = {
    .profile_type = LINDEV_GENERIC_PROFILE_TYPE_GENERIC,
    .discriminator_list = sharc_discriminator_list,
    .discriminator_list_size = ELEMENTS(sharc_discriminator_list),
    .generic_profile = {
        .entries = sharc_profile_entries,
        .entries_size = ELEMENTS(sharc_profile_entries),
    }};

#endif /* (CONNECTOR_LINDEV_SHARC_MODEL_DISCRIMINATION == 1) */

CONNECTOR connector_lindev_sharc = {
    .name = "LIN device",
    .initialize = connector_lindev_sharc_initialize};

static int connector_lindev_sharc_initialize(void)
{
    sharc_set_ci_error(false);
#if (CONNECTOR_LINDEV_SHARC_MODEL_DISCRIMINATION == 1)
    return lindev_generic_init_with_profile(&sharc_lindev_generic,
                                            &connector_lindev_sharc,
                                            &sharc_lindev_uart_config,
                                            &sharc_lindev_generic_config,
                                            &sharc_profile);
#else
    return lindev_generic_init(&sharc_lindev_generic,
                               &connector_lindev_sharc,
                               &sharc_lindev_uart_config,
                               &sharc_lindev_generic_config,
                               &sharc_ch4000_descriptor);
#endif
}

static void sharc_wtr_ctrl_extract(void *bundle, const uint8_t *data)
{
    dometic_sharc_wtr_ctrl_extract(bundle, data);
}

static void sharc_air_ctrl_extract(void *bundle, const uint8_t *data)
{
    dometic_sharc_air_ctrl_extract(bundle, data);
}

static void sharc_wtr_info_stuff(const void *bundle, uint8_t *data)
{
    dometic_sharc_wtr_info_stuff(bundle, data);
}

static void sharc_air_info_stuff(const void *bundle, uint8_t *data)
{
    dometic_sharc_air_info_stuff(bundle, data);
}

static void sharc_transceiver_sleep(void *context)
{
    (void)context;  // Not used
    LIN_SLEEP(0);
}

static void sharc_transceiver_wakeup(void *context)
{
    (void)context;  // Not used
    LIN_SLEEP(1);
}

static void sharc_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index)
{
    switch (event_id)
    {
    case LINDEV_EVENT_ID_INFO_FRAME:
        sharc_set_ci_error(false);
        /* FALLTHROUGH */
    case LINDEV_EVENT_ID_CONTROL_FRAME:
    case LINDEV_EVENT_ID_DIAG_REQ_FRAME:
    case LINDEV_EVENT_ID_DIAG_RESP_FRAME:
        lindev_generic_process_event(context, event_id, frame_index);
        break;
    default:
        sharc_set_ci_error(true);
    }
}

static void sharc_set_ci_error(bool state)
{
    sharc_air_info_bundle_buffer.protocol.response_error = (state ? 1 : 0);
    sharc_wtr_info_bundle_buffer.protocol.response_error = (state ? 1 : 0);
}

static void sharc_ddm_entry_has_changed(const ddm_entry_t *ddm_entry)
{
    if (ddm_entry__parameter_id(ddm_entry) == GW0DSN)
    {
        /* The following implementation is done based on old Sharc code.
         */
        /* TODO: Until this gets defined/cleared, use string as 4 bytes to form 32-bit Serial Number,
         * as it was done in old Sharc
         */
        const char *dsn;

        dsn = ddm_entry__value_str(ddm_entry);
        if (dsn != NULL)
        {
            lindev_generic_set_serial_number(&sharc_lindev_generic, *(const uint32_t *)dsn);
        }
        else
        {
            lindev_generic_set_serial_number(&sharc_lindev_generic, 0u);
        }
    }
}

#endif  // CONNECTOR_LINDEV_SHARC
