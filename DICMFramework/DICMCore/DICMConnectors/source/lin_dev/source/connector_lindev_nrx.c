/*! \file connector_lindev_nrx.c
    \brief Connector for CIBus in NRX
*/

#include "configuration.h"
#ifdef CONNECTOR_LINDEV_NRX

#include "connector_lindev.h"

#include "ddm2.h"
#include "ddm2.h"  // ELEMENTS
#include "ddm2_parameter_list.h"
#include "ddm_store.h"
#include "dometic.h"
#include "esp_attr.h"  // EXT_RAM_ATTR
#include "lindev_generic.h"

#define WORKER_STACK_SIZE 3094
#define WORKER_PRIORITY   xTASK_PRIORITY_NORMAL

static int connector_lindev_nrx_v1_initialize(void);
static void nrx_v1_ctrl_extract(void *bundle, const uint8_t *data);
static void nrx_v1_info_stuff(const void *bundle, uint8_t *data);
static void nrx_v1_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index);
static void nrx_v1_transceiver_sleep(void *context);
static void nrx_v1_transceiver_wakeup(void *context);
static void nrx_v1_ddm_entry_has_changed(const ddm_entry_t *ddm_entry);

static const struct ddm_store_ddm nrx_v1_ddm_other_initial_values[] = {
    {.ddm_parameter = NRX0MODE,
     .value = {.storage = {.i32 = NRX0MODE_PERFORMANCE_MODE}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = NRX0LVL,
     .value = {.storage = {.i32 = NRX0LEVEL_LEVEL3}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = NRX0COMPSTAT,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = NRX0TEMP,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = NRX0ERRST,
     .value = {.storage = {.i32 = NRX0ERRST_NO_ERRORS}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = NRX0PWRON,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = GW0DSN,
     .value = {.storage = {.str = "0000"}, .type = DDM2_TYPE_STRING}}};

DDM_STORE__DECLARE_EXTRAM(nrx_v1_ddm_other_store, ELEMENTS(nrx_v1_ddm_other_initial_values));

static const uint32_t nrx_v1_inventory_list[] = {
    NRX0AVL};

/* Sorted list to hold the states of DDM classes we want to subscribe to. */
DECLARE_SORTED_LIST_EXTRAM(nrx_v1_inventory_sortedlist, ELEMENTS(nrx_v1_inventory_list));

/*
    Reference : https://onedometic.atlassian.net/wiki/spaces/GC/pages/2877489157/Dometic+Generic+CI+Bus+Description
*/
static const lin_device_config_data_t nrx_v1_device_config = {
    .nad = 0x02,
    .supplier_id = 0x1234,
    .function_id = 0x0C03,  // NRX
    .variant_id = 0x00,
};

static const lindev_frame_def_t nrx_v1_lindev_frame_defs[] = {
    {.initial_id = DOMETIC_NRX_V1_CTRL_FRAME_ID, .type = LIN_CONTROL_FRAME},
    {.initial_id = DOMETIC_NRX_V1_INFO_FRAME_ID, .type = LIN_INFO_FRAME},
};

static lindev_frame_t nrx_v1_lindev_frames[ELEMENTS(nrx_v1_lindev_frame_defs)];

/* CTRL frame DOMETIC_NRX_V1_CTRL_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_nrx_v1_ctrl_bundle_t nrx_v1_ctrl_bundle_buffer;

static const lindev_table_ctrl_bundle_t nrx_v1_ctrl_bundle =
    LINDEV_TABLE_CTRL_BUNDLE(DOMETIC_NRX_V1_CTRL_FRAME_ID, dometic_nrx_v1_ctrl_bundle_t, rd, &nrx_v1_ctrl_bundle_buffer, NULL, nrx_v1_ctrl_extract);

/* INFO frame DOMETIC_NRX_V1_INFO_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_nrx_v1_info_bundle_t nrx_v1_info_bundle_buffer;

static const lindev_table_info_bundle_t nrx_v1_info_bundle =
    LINDEV_TABLE_INFO_BUNDLE(DOMETIC_NRX_V1_INFO_FRAME_ID, dometic_nrx_v1_info_bundle_t, rd, &nrx_v1_info_bundle_buffer, NULL, nrx_v1_info_stuff);

/* Bundle map */
static const lindev_table_bundle_map_entry_t nrx_v1_bundle_map[] = {
    {.ctrl = &nrx_v1_ctrl_bundle, .info = NULL},
    {.ctrl = NULL, .info = &nrx_v1_info_bundle},
};

/* Ctrl LINK map to ddm */
static const lindev_table_ctrl_link_map_entry_t nrx_v1_ctrl_link_map[] = {
    LINDEV_TABLE_CTRL_ENTRY(dometic_nrx_v1_ctrl_bundle_t, rd.nrx_userMode, &nrx_v1_ctrl_bundle, dometic_nrx_v1_conv_nrx_userMode_to_nrx0mode, NRX0MODE),
    LINDEV_TABLE_CTRL_ENTRY(dometic_nrx_v1_ctrl_bundle_t, rd.nrx_setTemp, &nrx_v1_ctrl_bundle, dometic_nrx_v1_conv_nrx_setTemp_to_nrx0lvl, NRX0LVL),
    LINDEV_TABLE_CTRL_ENTRY(dometic_nrx_v1_ctrl_bundle_t, rd.nrx_power, &nrx_v1_ctrl_bundle, dometic_nrx_v1_conv_nrx_power_to_nrx0pwron, NRX0PWRON),
};

/* Info LINK map to any int*/
static const lindev_table_info_link_map_entry_t nrx_v1_info_link_map[] = {
    LINDEV_TABLE_INFO_ENTRY(NRX0MODE, 0, 0, dometic_nrx_v1_conv_nrx0mode_to_nrx_userMode, dometic_nrx_v1_info_bundle_t, rd.nrx_userMode, &nrx_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(NRX0COMPSTAT, 0, 0, dometic_nrx_v1_conv_nrx0compstat_to_nrx_comprStatus, dometic_nrx_v1_info_bundle_t, st.nrx_comprStatus, &nrx_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(NRX0LVL, 0, 0, dometic_nrx_v1_conv_nrx0lvl_to_nrx_setTemp, dometic_nrx_v1_info_bundle_t, rd.nrx_setTemp, &nrx_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(NRX0TEMP, 0, 0, dometic_nrx_v1_conv_nrx0temp_to_nrx_Fresh_Temp, dometic_nrx_v1_info_bundle_t, st.nrx_Fresh_Temp, &nrx_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(NRX0PWRON, 0, 0, dometic_nrx_v1_conv_nrx0pwron_to_nrx_power, dometic_nrx_v1_info_bundle_t, rd.nrx_power, &nrx_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(NRX0ERRST, 0, 0, dometic_nrx_v1_conv_nrx0errst_to_nrx_Error, dometic_nrx_v1_info_bundle_t, st.nrx_Error, &nrx_v1_info_bundle),
};

static const lindev_table_config_t nrx_v1_lindev_table_config = {
    .sync_method = LINDEV_TABLE_SYNC_METHOD_SIMPLE,
    .bundle_map = nrx_v1_bundle_map,
    .bundle_map_length = ELEMENTS(nrx_v1_bundle_map),
    .ctrl_link_map = nrx_v1_ctrl_link_map,
    .ctrl_link_map_length = ELEMENTS(nrx_v1_ctrl_link_map),
    .info_link_map = nrx_v1_info_link_map,
    .info_link_map_length = ELEMENTS(nrx_v1_info_link_map),
};

lindev_generic_t nrx_lindev_generic;

static const lindev_uart_config_t nrx_v1_lindev_uart_config = {
    .context = &nrx_lindev_generic,
    .process_event = nrx_v1_process_event,
    .sleep = nrx_v1_transceiver_sleep,
    .wakeup = nrx_v1_transceiver_wakeup,
    .sleep_timeout_ms = 4000,
    .start_after_ms = 2000,
};

static const lindev_generic_config_t nrx_v1_lindev_generic_config = {
    .worker_stack_size = WORKER_STACK_SIZE,
    .worker_priority = WORKER_PRIORITY,
    .inventory_list = nrx_v1_inventory_list,
    .inventory_size = ELEMENTS(nrx_v1_inventory_list),
    .inventory_sortedlist = &nrx_v1_inventory_sortedlist,
};

static const lindev_generic_descriptor_t nrx_v1_descriptor = {
    .name = "NRX V1",
    .ddm_other_store = &nrx_v1_ddm_other_store,
    .ddm_other_initial_values = nrx_v1_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(nrx_v1_ddm_other_initial_values),
    .device_config = &nrx_v1_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = nrx_v1_lindev_frames,
    .lindev_frame_defs = nrx_v1_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(nrx_v1_lindev_frame_defs),
    .lindev_table_config = &nrx_v1_lindev_table_config,
    .cb_ddm_loop_has_started = NULL,
    .cb_ddm_entry_has_changed = nrx_v1_ddm_entry_has_changed,
};

CONNECTOR connector_lindev_nrx = {
    .name = "LIN device",
    .initialize = connector_lindev_nrx_v1_initialize};

static int connector_lindev_nrx_v1_initialize(void)
{
    LOG(I, "LINDEV Init");
    return lindev_generic_init(&nrx_lindev_generic, &connector_lindev_nrx, &nrx_v1_lindev_uart_config, &nrx_v1_lindev_generic_config, &nrx_v1_descriptor);
}

static void nrx_v1_ctrl_extract(void *bundle, const uint8_t *data)
{
    dometic_nrx_v1_ctrl_extract(bundle, data);
}

static void nrx_v1_info_stuff(const void *bundle, uint8_t *data)
{
    dometic_nrx_v1_info_stuff(bundle, data);
}

static void nrx_v1_transceiver_sleep(void *context)
{
    (void)context;  // Not used
    LIN_DEV_SLEEP(0);
}

static void nrx_v1_transceiver_wakeup(void *context)
{
    (void)context;  // Not used
    LIN_DEV_SLEEP(1);
}

static void nrx_v1_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index)
{
    switch (event_id)
    {
    case LINDEV_EVENT_ID_CONTROL_FRAME:
    case LINDEV_EVENT_ID_INFO_FRAME:
    case LINDEV_EVENT_ID_DIAG_REQ_FRAME:
    case LINDEV_EVENT_ID_DIAG_RESP_FRAME:
        lindev_generic_process_event(context, event_id, frame_index);
        break;
    default:
    }
}

static void nrx_v1_ddm_entry_has_changed(const ddm_entry_t *ddm_entry)
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
        lindev_generic_set_serial_number(&nrx_lindev_generic, *(const uint32_t *)dsn);
    }
}

#endif
