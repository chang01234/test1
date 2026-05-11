/**
 * @file lin_server_bridge_nrx.c
 * @author Felix Qin (felix.qin@dometic.com)
 * @brief BRIDGE NRX implementation
 * @date 2024-07-31
 */

#include "configuration.h"
#include "lin_server_bridge_nrx.h"
#include "lin_server.h"
#include "lin_server_scheduler.h"
#include "lin_server_device_definition.h"

#define DOMETIC_BRIDGE_NRX_FUNCTION_ID                             0x0C03
#define DOMETIC_BRIDGE_NRX_VARIANT_ID                              0x00

/* Private functions */
static int dometic_bridge_nrx_stuff_function(const lin_server_slave_device_t * slave_device, const lin_server_device_frames_bundle_def_t * frames_bundle_defs, uint8_t * data, size_t * data_size, lin_scheduler_frame_type_requests_t scheduled_frame_type_requests);
static int dometic_bridge_nrx_extract_function(const lin_server_slave_device_t * slave_device, const lin_server_device_frames_bundle_def_t * frames_bundle_defs, const uint8_t * data, size_t data_size, lin_server_slave_info_response_t * errors);

/* DDM to LIN conversion functions */
/* Remote Data 1..6 */
static int dometic_bridge_nrx_conv_nrx0mode_ddm_to_lin(const uint8_t * ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t * slave_device_frame);
static int dometic_bridge_nrx_conv_nrx0lvl_ddm_to_lin(const uint8_t * ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t * slave_device_frame);
static int dometic_bridge_nrx_conv_nrx0pwron_ddm_to_lin(const uint8_t * ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t * slave_device_frame);


/* LIN to DDM conversion functions */
/* Remote Data 1..6 */
static int dometic_bridge_nrx_conv_nrx0mode_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_bridge_nrx_conv_nrx0compstat_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_bridge_nrx_conv_nrx0lvl_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_bridge_nrx_conv_nrx0temp_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_bridge_nrx_conv_nrx0errst_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_bridge_nrx_conv_nrx0pwron_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);

/* Private types */
static EXT_RAM_ATTR lin_server_device_frame_t lin_server_bridge_nrx_ctrl_frame;
static EXT_RAM_ATTR lin_server_device_frame_t lin_server_bridge_nrx_info_frame;

static const lin_server_device_frame_def_t lin_server_bridge_nrx_ctrl_frame_def =
	{ .frame_id = DOMETIC_BRIDGE_NRX_CTRL_FRAME_ID,		.frame_type = LIN_CONTROL_FRAME,	.frame_len = DOMETIC_BRIDGE_NRX_CTRL_FRAME_LEN, .frame = &lin_server_bridge_nrx_ctrl_frame, };

static const lin_server_device_frame_def_t lin_server_bridge_nrx_info_frame_def =
	{ .frame_id = DOMETIC_BRIDGE_NRX_INFO_FRAME_ID,		.frame_type = LIN_INFO_FRAME,		.frame_len = DOMETIC_BRIDGE_NRX_INFO_FRAME_LEN, .frame = &lin_server_bridge_nrx_info_frame, };

static const lin_server_device_frames_bundle_def_t lin_server_bridge_nrx_frames_bundle_defs[] =
{
	{ .ctrl_frame_def = &lin_server_bridge_nrx_ctrl_frame_def, .info_frame_def = &lin_server_bridge_nrx_info_frame_def, },
};

static const lin_device_config_data_t bridge_nrx_config_data =
{
	.nad = 0x02,
	.supplier_id = 0x1234,
	.function_id = DOMETIC_BRIDGE_NRX_FUNCTION_ID, //NRX
};

static const uint8_t bridge_nrx_variant_ids[] = { DOMETIC_BRIDGE_NRX_VARIANT_ID };

static const lin_server_function_specific_config_data_t bridge_nrx_function_specific_config_data[] =
{
	{
		// Harrier
		.function_id = DOMETIC_BRIDGE_NRX_FUNCTION_ID,

		.variant_ids = bridge_nrx_variant_ids,
		.variant_ids_size = ELEMENTS(bridge_nrx_variant_ids),
	},
};

static const lin_server_generic_profile_entry_t bridge_nrx_profile_entry[ELEMENTS(bridge_nrx_function_specific_config_data)] =
{
	{ .ddm_model = 0, 		.function_id = DOMETIC_BRIDGE_NRX_FUNCTION_ID, .variant_id = DOMETIC_BRIDGE_NRX_VARIANT_ID },
};

static const lin_server_generic_profile_t bridge_nrx_generic_profile =
{
	.discriminator_ddm = DISCRIMINATOR_DDM_PARAMETER_MODEL_NONE,
	.profile_entry = bridge_nrx_profile_entry,
	.profile_entry_size = ELEMENTS(bridge_nrx_profile_entry),
};

static lin_server_slave_device_mutable_data_t data =
{
	.class_instance = -1,
	.prod_instance = -1,
	.is_initialized = false,
	.is_on_bus_detected = false,
	.bus_detection_counter = LIN_SERVER_BUS_DETECTION_COUNTER_INIT_VALUE,
	.lock = NULL,
};
static const struct ddm_store_ddm bridge_nrx_ddm_production_initial_values[] =
{
	{
		.ddm_parameter = PROD0AVL,
		.value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}
	},
	{
		.ddm_parameter = PROD0NAME,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0SN,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0SKU,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0PNC,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0FWVER,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0HWVER,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0MDL,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0EAN,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0DESCRIPTION,
		.value = {.storage = {.str = ""}, .type = DDM2_TYPE_STRING}
	},
	{
		.ddm_parameter = PROD0CLIST,
		.value = {.storage = {.structure = NULL}, .size = 0, .type = DDM2_TYPE_STRUCT}
	},
};
DDM_STORE__DECLARE_EXTRAM(bridge_nrx_production_ddm_owned_store, ELEMENTS(bridge_nrx_ddm_production_initial_values));

static const struct ddm_store_ddm bridge_nrx_ddm_owned_initial_values[] =
{
    {
        .ddm_parameter = NRX0DSN,
		.value = { .storage = { .str = "12345678" },	.type = DDM2_TYPE_STRING }
    },
    {
        .ddm_parameter = NRX0SKU,
		.value = { .storage = { .str = "12345612" },	.type = DDM2_TYPE_STRING }
    },
    {
        .ddm_parameter = NRX0PNC,
		.value = { .storage = { .str = "12345678" },	.type = DDM2_TYPE_STRING }
    },
    {
        .ddm_parameter = NRX0VER,
		.value = { .storage = { .str = "1.30" },	.type = DDM2_TYPE_STRING }
    },
    {
        .ddm_parameter = NRX0LGTON,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
    {
        .ddm_parameter = NRX0MODE,
		.value = { .storage = { .i32 = NRX0MODE_PERFORMANCE_MODE },	.type = DDM2_TYPE_INT32_T }
    },
	{
        .ddm_parameter = NRX0COMPSTAT,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
    {
        .ddm_parameter = NRX0LVL,
		.value = { .storage = { .i32 = NRX0LEVEL_LEVEL3 },	.type = DDM2_TYPE_INT32_T }
    },
	{
        .ddm_parameter = NRX0TEMP,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
    {
        .ddm_parameter = NRX0ERRST,
		.value = { .storage = { .i32 = NRX0ERRST_NO_ERRORS },	.type = DDM2_TYPE_INT32_T }
    },
	{
        .ddm_parameter = NRX0PWRON,
		.value = { .storage = { .i32 = 0 },	.type = DDM2_TYPE_INT32_T }
    },
};
DDM_STORE__DECLARE_EXTRAM(bridge_nrx_ddm_owned_store, ELEMENTS(bridge_nrx_ddm_owned_initial_values));
//to ctrl
static const lin_server_map_ddm_to_lin_t lin_server_bridge_nrx_ddm_to_lin[] =
{
	/* remote data 1 */
	{ .ddm = { NRX0MODE, 0, 0, },		.convert_ddm_to_lin = dometic_bridge_nrx_conv_nrx0mode_ddm_to_lin,		.ctrl_frame_def = &lin_server_bridge_nrx_ctrl_frame_def},
	/* remote data 2 */
	{ .ddm = { NRX0LVL, 0, 0 },			.convert_ddm_to_lin = dometic_bridge_nrx_conv_nrx0lvl_ddm_to_lin,		.ctrl_frame_def = &lin_server_bridge_nrx_ctrl_frame_def},
	/* remote data 7 */
	{ .ddm = { NRX0PWRON, 0, 0 },		.convert_ddm_to_lin = dometic_bridge_nrx_conv_nrx0pwron_ddm_to_lin,		.ctrl_frame_def = &lin_server_bridge_nrx_ctrl_frame_def},
};

//from sync status
static const lin_server_map_lin_to_ddm_t lin_server_bridge_nrx_lin_to_ddm[] =
{
	/* remote data 1 */
	{ .convert_lin_to_ddm = dometic_bridge_nrx_conv_nrx0mode_lin_to_ddm,		.ddm = { NRX0MODE, 0, 0 },		.info_frame_def = &lin_server_bridge_nrx_info_frame_def, },
	{ .convert_lin_to_ddm = dometic_bridge_nrx_conv_nrx0compstat_lin_to_ddm,	.ddm = { NRX0COMPSTAT, 0, 0 },	.info_frame_def = &lin_server_bridge_nrx_info_frame_def, },
	/* remote data 2 */
	{ .convert_lin_to_ddm = dometic_bridge_nrx_conv_nrx0lvl_lin_to_ddm,			.ddm = { NRX0LVL, 0, 0 },		.info_frame_def = &lin_server_bridge_nrx_info_frame_def, },
	/* remote data 5-6 */
	{ .convert_lin_to_ddm = dometic_bridge_nrx_conv_nrx0temp_lin_to_ddm,		.ddm = { NRX0TEMP, 0, 0 },		.info_frame_def = &lin_server_bridge_nrx_info_frame_def, },
	/* remote data 7 */
	{ .convert_lin_to_ddm = dometic_bridge_nrx_conv_nrx0errst_lin_to_ddm,		.ddm = { NRX0ERRST, 0, 0 },		.info_frame_def = &lin_server_bridge_nrx_info_frame_def, },
	/* remote data 8 */
	{ .convert_lin_to_ddm = dometic_bridge_nrx_conv_nrx0pwron_lin_to_ddm,		.ddm = { NRX0PWRON, 0, 0 },		.info_frame_def = &lin_server_bridge_nrx_info_frame_def, },
};

const lin_server_slave_device_t lin_server_bridge_nrx_device =
{
	.device_class = NRX0,
	.data = &data,
	.name = "BRIDGE_NRX",
	.device_type = LIN_SERVER_DEVICE_TYPE_BRIDGE_NRX,
	.ddm_owned_store = &bridge_nrx_ddm_owned_store,
	.ddm_owned_initial_values = bridge_nrx_ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(bridge_nrx_ddm_owned_initial_values),
	.ddm_production_store = &bridge_nrx_production_ddm_owned_store,
	.ddm_production_initial_values = bridge_nrx_ddm_production_initial_values,
	.ddm_production_initial_values_size = ELEMENTS(bridge_nrx_ddm_production_initial_values),
	.frames_bundle_defs = lin_server_bridge_nrx_frames_bundle_defs,
	.frames_bundle_defs_size = ELEMENTS(lin_server_bridge_nrx_frames_bundle_defs),
	.map_ddm_to_lin = lin_server_bridge_nrx_ddm_to_lin,
	.map_ddm_to_lin_size = ELEMENTS(lin_server_bridge_nrx_ddm_to_lin),
	.map_lin_to_ddm = lin_server_bridge_nrx_lin_to_ddm,
	.map_lin_to_ddm_size = ELEMENTS(lin_server_bridge_nrx_lin_to_ddm),
	.function_specific_config_data = bridge_nrx_function_specific_config_data,
	.function_specific_config_data_size = ELEMENTS(bridge_nrx_function_specific_config_data),
	.device_config = &bridge_nrx_config_data,
	.protocol = LIN_SERVER_SYNC_PROTOCOL_NONE,
	.generic_profile = &bridge_nrx_generic_profile,
	.ddm2_to_lin_ctrl_logic = NULL,
	.ddm2_to_lin_ctrl_logic_size = 0,
	.lin_to_ddm2_info_logic = NULL,
	.lin_to_ddm2_info_logic_size = 0,
	.init_function = NULL,
	.stuff_function = dometic_bridge_nrx_stuff_function,
	.extract_function = dometic_bridge_nrx_extract_function,
};

/* Executed as part of the UART task */
static int dometic_bridge_nrx_stuff_function(const lin_server_slave_device_t * slave_device, const lin_server_device_frames_bundle_def_t * frames_bundle_defs, uint8_t * data, size_t * data_size, lin_scheduler_frame_type_requests_t scheduled_frame_type_requests)
{
	(void)(slave_device);
	const lin_server_device_frame_t * ctrl_frame = frames_bundle_defs->ctrl_frame_def->frame;

	TRUE_CHECK_RETURN0(LIN_SCHEDULE_FRAME_TYPE_GET(scheduled_frame_type_requests, SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST) ||
		LIN_SCHEDULE_FRAME_TYPE_GET(scheduled_frame_type_requests, SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST));

	*data_size = LIN_FRAME_DATA_LEN;
	memcpy(data, ctrl_frame->frame_signals.bridge_nrx_frame.ctrl_frame.ctrl_frame, LIN_FRAME_DATA_LEN);

	return 1;
}

/* Executed as part of the UART task */
static int dometic_bridge_nrx_extract_function(const lin_server_slave_device_t * slave_device, const lin_server_device_frames_bundle_def_t * frames_bundle_defs, const uint8_t * data, size_t data_size, lin_server_slave_info_response_t * info_status)
{
	(void)(slave_device);
	(void)(frames_bundle_defs);
	lin_server_device_frame_bridge_nrx_t * bridge_nrx_info_frame = (lin_server_device_frame_bridge_nrx_t *)data;

	info_status->error = bridge_nrx_info_frame->info_frame.remote_data_7.Error;

	return 1;
}

/* DDM to LIN conversion functions */
/* Remote Data 1..8 */
static int dometic_bridge_nrx_conv_nrx0mode_ddm_to_lin(const uint8_t * ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t * slave_device_frame)
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	const int32_t * nrx0mode = (const int32_t *)ddm2_parameters_value[0];
	lin_server_device_frame_bridge_nrx_t * bridge_nrx_ctrl_frame = &slave_device_frame->bridge_nrx_frame;

	bridge_nrx_ctrl_frame->ctrl_frame.remote_data_1.Mode = *nrx0mode;

	return 1;
}

static int dometic_bridge_nrx_conv_nrx0lvl_ddm_to_lin(const uint8_t * ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t * slave_device_frame)
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	const int32_t * nrx0lvl = (const int32_t *)ddm2_parameters_value[0];
	lin_server_device_frame_bridge_nrx_t * bridge_nrx_ctrl_frame = &slave_device_frame->bridge_nrx_frame;

	bridge_nrx_ctrl_frame->ctrl_frame.remote_data_2.Temperature = *nrx0lvl;

	return 1;
}

static int dometic_bridge_nrx_conv_nrx0pwron_ddm_to_lin(const uint8_t * ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t * slave_device_frame)
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	const int32_t * nrx0pwron = (const int32_t *)ddm2_parameters_value[0];
	lin_server_device_frame_bridge_nrx_t * bridge_nrx_ctrl_frame = &slave_device_frame->bridge_nrx_frame;

	bridge_nrx_ctrl_frame->ctrl_frame.remote_data_8.Power = *nrx0pwron;

	return 1;
}
/* LIN to DDM conversion functions */
/* Remote Data 1..8 */
static int dometic_bridge_nrx_conv_nrx0mode_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	int32_t * nrx0mode = (int32_t *)ddm2_parameters_value[0];
	const lin_server_device_frame_bridge_nrx_t * bridge_nrx_info_frame = &slave_device_frame->bridge_nrx_frame;

	*nrx0mode = bridge_nrx_info_frame->info_frame.remote_data_1.Mode;

	return 1;
}

static int dometic_bridge_nrx_conv_nrx0compstat_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	int32_t * nrx0compstat = (int32_t *)ddm2_parameters_value[0];
	const lin_server_device_frame_bridge_nrx_t * bridge_nrx_info_frame = &slave_device_frame->bridge_nrx_frame;

	*nrx0compstat = bridge_nrx_info_frame->info_frame.remote_data_1.CompressorStatus;

	return 1;
}

static int dometic_bridge_nrx_conv_nrx0lvl_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	int32_t * nrx0lvl = (int32_t *)ddm2_parameters_value[0];
	const lin_server_device_frame_bridge_nrx_t * bridge_nrx_info_frame = &slave_device_frame->bridge_nrx_frame;

	*nrx0lvl = bridge_nrx_info_frame->info_frame.remote_data_2.ActualFridge;

	return 1;
}

static int dometic_bridge_nrx_conv_nrx0temp_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	int32_t * nrx0temp = (int32_t *)ddm2_parameters_value[0];
	const lin_server_device_frame_bridge_nrx_t * bridge_nrx_info_frame = &slave_device_frame->bridge_nrx_frame;

	*nrx0temp = (bridge_nrx_info_frame->info_frame.remote_data_5.FreshTemp - 128) * 500; /* (res / 2 -64) * 1000 */

	return 1;
}

static int dometic_bridge_nrx_conv_nrx0errst_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	int32_t * nrx0errst = (int32_t *)ddm2_parameters_value[0];
	const lin_server_device_frame_bridge_nrx_t * bridge_nrx_info_frame = &slave_device_frame->bridge_nrx_frame;

	*nrx0errst = bridge_nrx_info_frame->info_frame.remote_data_7.Error;

	return 1;
}

static int dometic_bridge_nrx_conv_nrx0pwron_lin_to_ddm(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
	TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

	int32_t * nrx0pwron = (int32_t *)ddm2_parameters_value[0];
	const lin_server_device_frame_bridge_nrx_t * bridge_nrx_info_frame = &slave_device_frame->bridge_nrx_frame;

	*nrx0pwron = bridge_nrx_info_frame->info_frame.remote_data_8.Power;

	return 1;
}