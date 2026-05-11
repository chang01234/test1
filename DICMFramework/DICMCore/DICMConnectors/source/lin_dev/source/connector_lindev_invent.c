/*! \file connector_lindev_invent.c
    \brief Connector for CIBus in Inventilate
*/

#include "configuration.h"
#ifdef CONNECTOR_LINDEV_INVENT

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

static int connector_lindev_invent_v1_initialize(void);
static void invent_v1_ctrl_extract(void *bundle, const uint8_t *data);
static void invent_v1_info_stuff(const void *bundle, uint8_t *data);
static void invent_v1_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index);
static void invent_v1_transceiver_sleep(void *context);
static void invent_v1_transceiver_wakeup(void *context);
static void invent_v1_set_ci_error(bool state);
static void invent_v1_ddm_entry_has_changed(const ddm_entry_t *ddm_entry);

static const struct ddm_store_ddm invent_v1_ddm_other_initial_values[] = {
#if 0  // Not used
    {
        .ddm_parameter = IV0AVL,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
    {.ddm_parameter = IV0MODE,
     .value = {.storage = {.i32 = IV0MODE_AUTO}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0PWRON,
     .value = {.storage = {.i32 = IV0PWRON_OFF}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0FILST,
     .value = {.storage = {.i32 = IV0FILST_FILTER_CHANGE_NOT_REQ}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0STORAGE,
     .value = {.storage = {.i32 = IV0STORAGE_DEACTIVATE}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0ERRST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0WARN,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0PWRSRC,
     .value = {.storage = {.i32 = IV0PWRSRC_12V_CAR_BATTERY_INPUT}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0AQST,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},

#if 0  // Not used
    {
        .ddm_parameter = IV0PRST,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
    {.ddm_parameter = IV0BLREQ,
     .value = {.storage = {.i32 = IV0BLREQ_IDLE}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = IV0IONST,
     .value = {.storage = {.i32 = IV0IONST_OFF}, .type = DDM2_TYPE_INT32_T}},
#if 0  // Not used
    {
        .ddm_parameter = IV0HMITST,
		.value = { .storage = { .i32 = AC0FMD_AUTO },	.type = DDM2_TYPE_INT32_T }
    },
#endif
#if 0  // Not used
    {
        .ddm_parameter = IVPMGR0AVL,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
    {.ddm_parameter = IVPMGR0STATE,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
#if 0  // Not used
    {
        .ddm_parameter = IVAQR0AVL,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
#if 0  // Not used
    {
        .ddm_parameter = IVAQR0MIN,
		.value = { .storage = { .i32 = HTR0SYSU_METRIC },	.type = DDM2_TYPE_INT32_T }
    },
#endif
#if 0  // Not used
    {
        .ddm_parameter = IVAQR0MAX,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
#if 0  // Not used
    {
        .ddm_parameter = MTR0AVL,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
    {.ddm_parameter = MTR0DEVID,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
#if 0  // Not used
    {
        .ddm_parameter = MTR0SETSPD,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
    {.ddm_parameter = MTR0MINSPD,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = MTR0MAXSPD,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
#if 0  // Not used
    {
        .ddm_parameter = MTR0TACHO,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
    {.ddm_parameter = MTR0DIR,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
#if 0  // Not used
    {
        .ddm_parameter = IV0REMCTRL,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
    {
        .ddm_parameter = IV0TEST,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
#endif
    {.ddm_parameter = DIM0LVL,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = WIFI0STS,
     .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = GW0DSN,
     .value = {.storage = {.str = "0000"}, .type = DDM2_TYPE_STRING}}};

DDM_STORE__DECLARE_EXTRAM(invent_v1_ddm_other_store, ELEMENTS(invent_v1_ddm_other_initial_values));

static const uint32_t invent_v1_inventory_list[] = {
    IV0AVL,
    IVPMGR0AVL,
    MTR0AVL,
    DIM0AVL,
    WIFI0AVL};

/* Sorted list to hold the states of DDM classes we want to subscribe to. */
DECLARE_SORTED_LIST_EXTRAM(invent_v1_inventory_sortedlist, ELEMENTS(invent_v1_inventory_list));

/*
    Reference : https://onedometic.atlassian.net/wiki/spaces/GC/pages/2877489157/Dometic+Generic+CI+Bus+Description
*/
static const lin_device_config_data_t invent_v1_device_config = {
    .nad = 0x20,
    .supplier_id = 0x1234,
    .function_id = 0x0010,
    .variant_id = 0x00,
};

static const lindev_frame_def_t invent_v1_lindev_frame_defs[] = {
    {.initial_id = DOMETIC_INVENT_V1_CTRL_FRAME_ID, .type = LIN_CONTROL_FRAME},
    {.initial_id = DOMETIC_INVENT_V1_INFO_FRAME_ID, .type = LIN_INFO_FRAME},
};

static lindev_frame_t invent_v1_lindev_frames[ELEMENTS(invent_v1_lindev_frame_defs)];

/* CTRL frame DOMETIC_INVENT_V1_CTRL_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_invent_v1_ctrl_bundle_t invent_v1_ctrl_bundle_buffer;

static const lindev_table_ctrl_bundle_t invent_v1_ctrl_bundle =
    LINDEV_TABLE_CTRL_BUNDLE(DOMETIC_INVENT_V1_CTRL_FRAME_ID, dometic_invent_v1_ctrl_bundle_t, rd, &invent_v1_ctrl_bundle_buffer, NULL, invent_v1_ctrl_extract);

/* INFO frame DOMETIC_INVENT_V1_INFO_FRAME_ID bundle structures */
static EXT_RAM_ATTR dometic_invent_v1_info_bundle_t invent_v1_info_bundle_buffer;

static const lindev_table_info_bundle_t invent_v1_info_bundle =
    LINDEV_TABLE_INFO_BUNDLE(DOMETIC_INVENT_V1_INFO_FRAME_ID, dometic_invent_v1_info_bundle_t, rd, &invent_v1_info_bundle_buffer, NULL, invent_v1_info_stuff);

/* Bundle map */
static const lindev_table_bundle_map_entry_t invent_v1_bundle_map[] = {
    {.ctrl = &invent_v1_ctrl_bundle, .info = NULL},
    {.ctrl = NULL, .info = &invent_v1_info_bundle},
};

/* Ctrl LINK map */
static const lindev_table_ctrl_link_map_entry_t invent_v1_ctrl_link_map[] = {
    LINDEV_TABLE_CTRL_ENTRY(dometic_invent_v1_ctrl_bundle_t, rd.inv_pwr, &invent_v1_ctrl_bundle, dometic_invent_v1_conv_pwr_to_iv0pwron, IV0PWRON),
    LINDEV_TABLE_CTRL_ENTRY(dometic_invent_v1_ctrl_bundle_t, rd.inv_mode, &invent_v1_ctrl_bundle, dometic_invent_v1_conv_mode_to_iv0mode, IV0MODE),
    LINDEV_TABLE_CTRL_ENTRY(dometic_invent_v1_ctrl_bundle_t, rd.inv_ledstrip_bright, &invent_v1_ctrl_bundle, dometic_invent_v1_conv_ledbright_to_dim0lvl, DIM0LVL),
    LINDEV_TABLE_CTRL_ENTRY(dometic_invent_v1_ctrl_bundle_t, rd.inv_filtreset, &invent_v1_ctrl_bundle, dometic_invent_v1_conv_filtreset_to_iv0filst, IV0FILST),
    LINDEV_TABLE_CTRL_ENTRY(dometic_invent_v1_ctrl_bundle_t, rd.inv_stormodeavl, &invent_v1_ctrl_bundle, dometic_invent_v1_conv_storage_to_iv0storage, IV0STORAGE),
};

/* Info LINK map */
static const lindev_table_info_link_map_entry_t invent_v1_info_link_map[] = {
    LINDEV_TABLE_INFO_ENTRY(IV0PWRON, 0, 0, dometic_invent_v1_conv_iv0pwrn_to_pwr, dometic_invent_v1_info_bundle_t, rd.inv_pwr, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(IV0MODE, 0, 0, dometic_invent_v1_conv_iv0mode_to_mode, dometic_invent_v1_info_bundle_t, rd.inv_mode, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(DIM0LVL, 0, 0, dometic_invent_v1_conv_dim0lvl_to_ledbright, dometic_invent_v1_info_bundle_t, rd.inv_ledstrip_bright, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(IV0FILST, 0, 0, dometic_invent_v1_conv_iv0filst_to_filtreset, dometic_invent_v1_info_bundle_t, rd.inv_filtreset, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(IV0STORAGE, 0, 0, dometic_invent_v1_conv_iv0storage_to_storage, dometic_invent_v1_info_bundle_t, rd.inv_stormodeavl, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(IV0AQST, 0, 0, dometic_invent_v1_conv_iv0aqst_to_airqua, dometic_invent_v1_info_bundle_t, st.inv_airqua, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(IV0PWRSRC, 0, 0, dometic_invent_v1_conv_iv0pwrsrc_to_pwrsrcstat, dometic_invent_v1_info_bundle_t, st.inv_pwrsrcstat, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(WIFI0STS, 0, 0, dometic_invent_v1_conv_wifi0sts_to_wifistat, dometic_invent_v1_info_bundle_t, st.inv_wifistat, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(IV0IONST, 0, 0, dometic_invent_v1_conv_iv0ionst_to_ionizstat, dometic_invent_v1_info_bundle_t, st.inv_ionizstat, &invent_v1_info_bundle),
    LINDEV_TABLE_INFO_ENTRY(IV0ERRST, 0, 0, dometic_invent_v1_conv_iv0errst_to_errcode, dometic_invent_v1_info_bundle_t, st.inv_errcode, &invent_v1_info_bundle),
};

static const lindev_table_config_t invent_v1_lindev_table_config = {
    .sync_method = LINDEV_TABLE_SYNC_METHOD_SIMPLE,
    .bundle_map = invent_v1_bundle_map,
    .bundle_map_length = ELEMENTS(invent_v1_bundle_map),
    .ctrl_link_map = invent_v1_ctrl_link_map,
    .ctrl_link_map_length = ELEMENTS(invent_v1_ctrl_link_map),
    .info_link_map = invent_v1_info_link_map,
    .info_link_map_length = ELEMENTS(invent_v1_info_link_map),
};

lindev_generic_t invent_lindev_generic;

static const lindev_uart_config_t invent_v1_lindev_uart_config = {
    .context = &invent_lindev_generic,
    .process_event = invent_v1_process_event,
    .sleep = invent_v1_transceiver_sleep,
    .wakeup = invent_v1_transceiver_wakeup,
    .sleep_timeout_ms = 4000,
    .start_after_ms = 2000,
};

static const lindev_generic_config_t invent_v1_lindev_generic_config = {
    .worker_stack_size = WORKER_STACK_SIZE,
    .worker_priority = WORKER_PRIORITY,
    .inventory_list = invent_v1_inventory_list,
    .inventory_size = ELEMENTS(invent_v1_inventory_list),
    .inventory_sortedlist = &invent_v1_inventory_sortedlist,
};

static const lindev_generic_descriptor_t invent_v1_descriptor = {
    .name = "Invent V1",
    .ddm_other_store = &invent_v1_ddm_other_store,
    .ddm_other_initial_values = invent_v1_ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(invent_v1_ddm_other_initial_values),
    .device_config = &invent_v1_device_config,
    .lindev_uart = CONNECTOR_LINDEV_UART_NUM,
    .lindev_frames = invent_v1_lindev_frames,
    .lindev_frame_defs = invent_v1_lindev_frame_defs,
    .lindev_frame_defs_size = ELEMENTS(invent_v1_lindev_frame_defs),
    .lindev_table_config = &invent_v1_lindev_table_config,
    .cb_ddm_loop_has_started = NULL,
    .cb_ddm_entry_has_changed = invent_v1_ddm_entry_has_changed,
};

CONNECTOR connector_lindev_invent = {
    .name = "LIN device",
    .initialize = connector_lindev_invent_v1_initialize,
};

static int connector_lindev_invent_v1_initialize(void)
{
    LOG(I, "LINDEV Init");
    invent_v1_set_ci_error(false);
    return lindev_generic_init(&invent_lindev_generic, &connector_lindev_invent, &invent_v1_lindev_uart_config, &invent_v1_lindev_generic_config, &invent_v1_descriptor);
}

static void invent_v1_ctrl_extract(void *bundle, const uint8_t *data)
{
    dometic_invent_v1_ctrl_extract(bundle, data);
}

static void invent_v1_info_stuff(const void *bundle, uint8_t *data)
{
    dometic_invent_v1_info_stuff(bundle, data);
}

static void invent_v1_transceiver_sleep(void *context)
{
    (void)context;  // Not used
    LIN_SLEEP(0);
}

static void invent_v1_transceiver_wakeup(void *context)
{
    (void)context;  // Not used
    LIN_SLEEP(1);
}

static void invent_v1_process_event(void *context, const lindev_event_id_t event_id, size_t frame_index)
{
    switch (event_id)
    {
    case LINDEV_EVENT_ID_CONTROL_FRAME:
    case LINDEV_EVENT_ID_INFO_FRAME:
    case LINDEV_EVENT_ID_DIAG_REQ_FRAME:
    case LINDEV_EVENT_ID_DIAG_RESP_FRAME:
        lindev_generic_process_event(context, event_id, frame_index);
        invent_v1_set_ci_error(false);
        break;
    default:
        invent_v1_set_ci_error(true);
        break;
    }
}

static void invent_v1_set_ci_error(bool state)
{
    invent_v1_info_bundle_buffer.protocol.response_error = state ? 1 : 0;
}

static void invent_v1_ddm_entry_has_changed(const ddm_entry_t *ddm_entry)
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
        lindev_generic_set_serial_number(&invent_lindev_generic, *(const uint32_t *)dsn);
    }
}

#endif
