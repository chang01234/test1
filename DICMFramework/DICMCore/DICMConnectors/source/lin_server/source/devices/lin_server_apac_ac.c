/**
 * @file lin_server_apac_ac.c
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief APAC AC implementation
 * @date 2024-06-25
 */

#include "lin_server_apac_ac.h"
#include "configuration.h"
#include "fsm.h"
#include "lin_server.h"
#include "lin_server_device_definition.h"
#include "lin_server_scheduler.h"

/* Private macros */
#define DOMETIC_APAC_AC_AC0STATUS_NUMBER_OF_SUPPORTED_ERRORS 3

#define APAC_TIMERS_STATE_LOG() LOG(D, "State: %s, event: %s", __func__, timers_decode_event_id(event->id));

#define DOMETIC_APAC_AC_FUNCTION_ID_HARRIER 0x8001
#define DOMETIC_APAC_AC_FUNCTION_ID_IBIS_4  0x8002
#define DOMETIC_APAC_AC_FUNCTION_ID_CK_LITE 0x8003

#define DOMETIC_APAC_AC_VARIANT_ID 0x00

/* Private definitions */
typedef enum sm_timers_event_id
{
    SM_TIMERS_EVENT_ID_AC0TONA = FSM_USER_EVENT,
    SM_TIMERS_EVENT_ID_AC0TONM,
    SM_TIMERS_EVENT_ID_AC0TOFFA,
    SM_TIMERS_EVENT_ID_AC0TOFFM,
    SM_TIMERS_EVENT_ID_POST_RECEIVE,  // UART
    SM_TIMERS_EVENT_ID_POST_SEND,     // Broker
} sm_timers_event_id_t;

typedef struct sm_timers_data
{
    const void *value;                        // Value of parameter
    size_t value_size;                        // Paramter value size
    void *ptr;                                // INFO/CTRL frame reference
    ddm_store_t *store;                       // Storage to update parameters
    lin_server_logic_func_response_t status;  // Status of the SM
} sm_timers_data_t;

typedef struct apac_ac_timers
{
    fsm_t sm_timer_on;
    fsm_event_t sm_timer_on_event;
    fsm_t sm_timer_off;
    fsm_event_t sm_timer_off_event;

    sm_timers_event_id_t event_id;  // Pass trigger event of pre_send to post_send
} apac_ac_timers_t;

static EXT_RAM_ATTR apac_ac_timers_t lin_server_apac_timers;

/* Private functions */
static int dometic_apac_ac_init_function(const lin_server_slave_device_t *slave_device);
static int dometic_apac_ac_stuff_function(const lin_server_slave_device_t *slave_device, const lin_server_device_frames_bundle_def_t *frames_bundle_defs, uint8_t *data, size_t *data_size, lin_scheduler_frame_type_requests_t scheduled_frame_type_requests);
static int dometic_apac_ac_extract_function(const lin_server_slave_device_t *slave_device, const lin_server_device_frames_bundle_def_t *frames_bundle_defs, const uint8_t *data, size_t data_size, lin_server_slave_info_response_t *errors);

/* DDM to LIN conversion functions */
/* Remote Data 1..6 */
static int dometic_apac_ac_conv_ac0md_to_md(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0lgt_to_lgt(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0on_to_on(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0ttemp_to_ttemp(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0sleep_to_sleep(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0toffa_to_toffa(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0tona_to_tona(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0tonm_to_tonm(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0toffm_to_toffm(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_conv_ac0sysu_to_cf(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
/* Remote data 1..6 && Ctrl frame */
static int dometic_apac_ac_ac0remctrl_to_rem_ctrl_dis(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);

/* LIN to DDM conversion functions */
/* Remote Data 1..6 */
static int dometic_apac_ac_conv_md_to_ac0md(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_lgt_to_ac0lgt(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_on_to_ac0on(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_ttemp_to_ac0ttemp(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_sleep_to_ac0sleep(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_toffa_to_ac0toffa(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_tona_to_ac0tona(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_tonm_to_ac0tonm(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_toffm_to_ac0toffm(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_cf_to_ac0sysu(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
/* System status */
static int dometic_apac_ac_conv_comprun_to_ac0actext(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_conv_itemp_to_ac0itemp(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
/* Info status */
static int dometic_apac_ac_conv_errors_to_ac0status(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);

/* CK-Lite */
/* DDM to LIN conversion functions */
static int dometic_apac_ac_ck_lite_conv_ac0fspd_to_fspd(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
/* LIN to DDM conversion functions */
static int dometic_apac_ac_ck_lite_conv_fspd_to_ac0fspd(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);

/* Harrier */
/* DDM to LIN conversion functions */
static int dometic_apac_ac_harrier_conv_ac0pur_to_pur(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
static int dometic_apac_ac_harrier_conv_ac0flaps_to_flap(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
/* LIN to DDM conversion functions */
static int dometic_apac_ac_harrier_conv_pure_to_ac0pur(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
static int dometic_apac_ac_harrier_conv_flap_to_ac0flaps(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);

/* IBIS4 && Harrier */
/* DDM to LIN conversion functions */
static int dometic_apac_ac_ibis4_harrier_conv_ac0fspd_to_fspd(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame);
/* LIN to DDM conversion functions */
static int dometic_apac_ac_ibis4_harrier_conv_fspd_to_ac0fspd(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);

/* DDM logic functions */
static lin_server_logic_func_response_t dometic_apac_ac_logic_ac0fspd_pre_send(const lin_server_slave_device_t *slave_device, const ddm_entry_t *const updated_parameter, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def);

/* Timer ON logic implementation */
static lin_server_logic_func_response_t dometic_apac_logic_timer_on_pre_send(const lin_server_slave_device_t *const device, const ddm_entry_t *const updated_parameter, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def);
static lin_server_logic_func_response_t dometic_apac_logic_timer_on_post_send(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *ctrl_frame_def);
static lin_server_logic_func_response_t dometic_apac_logic_timer_on_post_receive(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *info_frame_def, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def);

static bool timer_on_info_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *info_frame);
static bool timer_on_ctrl_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *ctrl_frame);
static bool timer_on_info_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *info_frame);
static bool timer_on_ctrl_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *ctrl_frame);
static bool timer_on_info_frame_is_timeout_set(const lin_server_device_frame_apac_ac_t *info_frame);

static void sm_state_timer_off_deactivate(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_off_active(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_off_activate(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_off_min_pending(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_off_en_pending(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_off_idle(fsm_t *const fsm, fsm_event_t const *const event);

/* Timer OFF logic implementation */
static lin_server_logic_func_response_t dometic_apac_logic_timer_off_pre_send(const lin_server_slave_device_t *const device, const ddm_entry_t *const updated_parameter, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def);
static lin_server_logic_func_response_t dometic_apac_logic_timer_off_post_send(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *ctrl_frame_def);
static lin_server_logic_func_response_t dometic_apac_logic_timer_off_post_receive(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *info_frame_def, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def);

static bool timer_off_info_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *info_frame);
static bool timer_off_ctrl_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *ctrl_frame);
static bool timer_off_info_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *info_frame);
static bool timer_off_ctrl_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *ctrl_frame);
static bool timer_off_info_frame_is_timeout_set(const lin_server_device_frame_apac_ac_t *info_frame);

static void sm_state_timer_on_deactivate(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_on_active(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_on_activate(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_on_min_pending(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_on_en_pending(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_state_timer_on_idle(fsm_t *const fsm, fsm_event_t const *const event);

/* Private types */
static EXT_RAM_ATTR lin_server_device_frame_t lin_server_apac_ac_ctrl_frame;
static EXT_RAM_ATTR lin_server_device_frame_t lin_server_apac_ac_info_frame;

static const lin_server_slave_device_sync_protocol_local_change_t info_frame_sync_protocol =
    {
        .local_change =
            {
                .local_change_byte_pos = offsetof(lin_server_device_frame_t, frame_signals.apac_ac_frame.info_frame.info_status.info_status),
                .local_change_bit_pos = DOMETIC_APAC_AC_LOCAL_CHANGE_BIT_POSITION,
            },
};

static const lin_server_slave_device_sync_protocol_local_change_t ctrl_frame_sync_protocol =
    {
        .sync_frame =
            {
                .sync_frame_byte_pos = offsetof(lin_server_device_frame_t, frame_signals.apac_ac_frame.ctrl_frame.ctrl_status.ctrl_status),
                .sync_frame_bit_pos = DOMETIC_APAC_AC_SYNC_FRAME_BIT_POSITION,
            },
};

static const lin_server_device_frame_def_t lin_server_apac_ac_ctrl_frame_def =
    {.frame_id = DOMETIC_APAC_AC_CTRL_FRAME_ID, .frame_type = LIN_CONTROL_FRAME, .frame_len = DOMETIC_APAC_AC_CTRL_FRAME_LEN, .frame = &lin_server_apac_ac_ctrl_frame, .protocol_definition = {
                                                                                                                                                                           .local_change = &ctrl_frame_sync_protocol,
                                                                                                                                                                       }};

static const lin_server_device_frame_def_t lin_server_apac_ac_info_frame_def =
    {.frame_id = DOMETIC_APAC_AC_INFO_FRAME_ID, .frame_type = LIN_INFO_FRAME, .frame_len = DOMETIC_APAC_AC_INFO_FRAME_LEN, .frame = &lin_server_apac_ac_info_frame, .protocol_definition = {
                                                                                                                                                                        .local_change = &info_frame_sync_protocol,
                                                                                                                                                                    }};

static const lin_server_device_frames_bundle_def_t lin_server_apac_ac_frames_bundle_defs[] =
    {
        {
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
};

static const lin_device_config_data_t apac_ac_config_data =
    {
        .nad = 0x10,
        .supplier_id = 0x1234,
};

static const uint8_t apac_ac_variant_ids[] = {DOMETIC_APAC_AC_VARIANT_ID};

/* CK-Lite */
static const lin_server_map_ddm_to_lin_t lin_server_apac_ac_ck_lite_specific_ddm_to_lin[] =
    {
        {
            .ddm = {AC0FSPD, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_ck_lite_conv_ac0fspd_to_fspd,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
};

static const lin_server_map_lin_to_ddm_t lin_server_apac_ac_ck_lite_specific_lin_to_ddm[] =
    {
        {
            .convert_lin_to_ddm = dometic_apac_ac_ck_lite_conv_fspd_to_ac0fspd,
            .ddm = {AC0FSPD, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
};

/* IBIS4 */
static const lin_server_map_ddm_to_lin_t lin_server_apac_ac_ibis4_specific_ddm_to_lin[] =
    {
        {
            .ddm = {AC0FSPD, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_ibis4_harrier_conv_ac0fspd_to_fspd,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
};

static const lin_server_map_lin_to_ddm_t lin_server_apac_ac_ibis4_specific_lin_to_ddm[] =
    {
        {
            .convert_lin_to_ddm = dometic_apac_ac_ibis4_harrier_conv_fspd_to_ac0fspd,
            .ddm = {AC0FSPD, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
};

/* Harrier */
static const lin_server_map_ddm_to_lin_t lin_server_apac_ac_harrier_specific_ddm_to_lin[] =
    {
        {
            .ddm = {AC0PUR, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_harrier_conv_ac0pur_to_pur,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        {
            .ddm = {AC0FLAPS, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_harrier_conv_ac0flaps_to_flap,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        {
            .ddm = {AC0FSPD, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_ibis4_harrier_conv_ac0fspd_to_fspd,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
};

static const lin_server_map_lin_to_ddm_t lin_server_apac_ac_harrier_specific_lin_to_ddm[] =
    {
        {
            .convert_lin_to_ddm = dometic_apac_ac_harrier_conv_pure_to_ac0pur,
            .ddm = {AC0PUR, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        {
            .convert_lin_to_ddm = dometic_apac_ac_harrier_conv_flap_to_ac0flaps,
            .ddm = {AC0FLAPS, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        {
            .convert_lin_to_ddm = dometic_apac_ac_ibis4_harrier_conv_fspd_to_ac0fspd,
            .ddm = {AC0FSPD, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
};

static const lin_server_function_specific_config_data_t apac_ac_function_specific_config_data[] =
    {
        {
            // Harrier
            .function_id = DOMETIC_APAC_AC_FUNCTION_ID_HARRIER,

            .variant_ids = apac_ac_variant_ids,
            .variant_ids_size = ELEMENTS(apac_ac_variant_ids),
            .map_function_specific_ddm_to_lin = lin_server_apac_ac_harrier_specific_ddm_to_lin,
            .map_function_specific_ddm_to_lin_size = ELEMENTS(lin_server_apac_ac_harrier_specific_ddm_to_lin),
            .map_function_specific_lin_to_ddm = lin_server_apac_ac_harrier_specific_lin_to_ddm,
            .map_function_specific_lin_to_ddm_size = ELEMENTS(lin_server_apac_ac_harrier_specific_lin_to_ddm),
        },
        {
            // IBIS 4
            .function_id = DOMETIC_APAC_AC_FUNCTION_ID_IBIS_4,

            .variant_ids = apac_ac_variant_ids,
            .variant_ids_size = ELEMENTS(apac_ac_variant_ids),
            .map_function_specific_ddm_to_lin = lin_server_apac_ac_ibis4_specific_ddm_to_lin,
            .map_function_specific_ddm_to_lin_size = ELEMENTS(lin_server_apac_ac_ibis4_specific_ddm_to_lin),
            .map_function_specific_lin_to_ddm = lin_server_apac_ac_ibis4_specific_lin_to_ddm,
            .map_function_specific_lin_to_ddm_size = ELEMENTS(lin_server_apac_ac_ibis4_specific_lin_to_ddm),
        },
        {
            // CK-Lite
            .function_id = DOMETIC_APAC_AC_FUNCTION_ID_CK_LITE,

            .variant_ids = apac_ac_variant_ids,
            .variant_ids_size = ELEMENTS(apac_ac_variant_ids),
            .map_function_specific_ddm_to_lin = lin_server_apac_ac_ck_lite_specific_ddm_to_lin,
            .map_function_specific_ddm_to_lin_size = ELEMENTS(lin_server_apac_ac_ck_lite_specific_ddm_to_lin),
            .map_function_specific_lin_to_ddm = lin_server_apac_ac_ck_lite_specific_lin_to_ddm,
            .map_function_specific_lin_to_ddm_size = ELEMENTS(lin_server_apac_ac_ck_lite_specific_lin_to_ddm),
        }};

static const lin_server_generic_profile_entry_t apac_ac_profile_entry[] =
    {
        {.ddm_model = AC0MDL_DOMETIC_APAC_HARRIER, .function_id = DOMETIC_APAC_AC_FUNCTION_ID_HARRIER, .variant_id = DOMETIC_APAC_AC_VARIANT_ID},
        {.ddm_model = AC0MDL_DOMETIC_APAC_IBIS4, .function_id = DOMETIC_APAC_AC_FUNCTION_ID_IBIS_4, .variant_id = DOMETIC_APAC_AC_VARIANT_ID},
        {.ddm_model = AC0MDL_DOMETIC_APAC_CK_LITE, .function_id = DOMETIC_APAC_AC_FUNCTION_ID_CK_LITE, .variant_id = DOMETIC_APAC_AC_VARIANT_ID},
};

static const lin_server_generic_profile_t apac_ac_generic_profile =
    {
        .discriminator_ddm = AC0MDL,
        .profile_entry = apac_ac_profile_entry,
        .profile_entry_size = ELEMENTS(apac_ac_profile_entry),
};

static lin_server_slave_device_mutable_data_t data =
    {
        .class_instance = -1,
        .prod_instance = -1,
        .is_initialized = false,
        .is_on_bus_detected = false,
        .bus_detection_counter = LIN_SERVER_BUS_DETECTION_COUNTER_INIT_VALUE,
        .lock = NULL,
        .data = (void *)&lin_server_apac_timers,
        .data_size = sizeof(lin_server_apac_timers),
};

static const struct ddm_store_ddm apac_ac_ddm_production_initial_values[] =
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

static uint16_t ac0status_error[DOMETIC_APAC_AC_AC0STATUS_NUMBER_OF_SUPPORTED_ERRORS] = {};

static const struct ddm_store_ddm apac_ac_ddm_owned_initial_values[] =
    {
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
        {.ddm_parameter = AC0MDL,
         .value = {.storage = {.i32 = AC0MDL_UNKNOWN}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ITEMP,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FS,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FMD,
         .value = {.storage = {.i32 = AC0FMD_AUTO}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ELGT,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0PUR,
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
         .value =
             {
                 .storage = {.structure = ac0status_error},
                 .size = sizeof(ac0status_error),
                 .type = DDM2_TYPE_STRUCT,
             }},
        {.ddm_parameter = AC0SLEEP,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0HFAVL,
         .value = {.storage = {.i32 = 1}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0LFAVL,
         .value = {.storage = {.i32 = 1}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ACTEXT,
         .value = {.storage = {.u32 = 0}, .type = DDM2_TYPE_UINT32_T}},
        {.ddm_parameter = AC0REMCTRL,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FLAPS,
         .value = {.storage = {.i32 = AC0FLAPS_FLAPS_STOPPED}, .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0SYSU,
         .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T}},
};

DDM_STORE__DECLARE_EXTRAM(apac_ac_ddm_owned_store, ELEMENTS(apac_ac_ddm_owned_initial_values));
DDM_STORE__DECLARE_EXTRAM(apac_ac_ddm_production_store, ELEMENTS(apac_ac_ddm_production_initial_values));

static const lin_server_ddm2_to_lin_ctrl_logic_t lin_server_apac_ac_parameter_ctrl_logic[] =
    {
        DDM2_TO_LIN_CTRL_LOGIC_ENTRY(((const uint32_t[]){AC0FSPD}), dometic_apac_ac_logic_ac0fspd_pre_send, NULL),
        DDM2_TO_LIN_CTRL_LOGIC_ENTRY(((const uint32_t[]){
                                         AC0TONM,
                                         AC0TONA,
                                     }),
                                     dometic_apac_logic_timer_on_pre_send, dometic_apac_logic_timer_on_post_send),
        DDM2_TO_LIN_CTRL_LOGIC_ENTRY(((const uint32_t[]){AC0TOFFM, AC0TOFFA}), dometic_apac_logic_timer_off_pre_send, dometic_apac_logic_timer_off_post_send),
};

static const lin_server_lin_to_ddm2_info_logic_t lin_server_apac_ac_parameter_info_logic[] =
    {
        LIN_INFO_TO_DDM2_LOGIC_ENTRY(&lin_server_apac_ac_info_frame_def, dometic_apac_logic_timer_on_post_receive),
        LIN_INFO_TO_DDM2_LOGIC_ENTRY(&lin_server_apac_ac_info_frame_def, dometic_apac_logic_timer_off_post_receive),
};

static const lin_server_map_ddm_to_lin_t lin_server_apac_ac_ddm_to_lin[] =
    {
        /* remote data 1 */
        {
            .ddm = {
                AC0MD,
                0,
                0,
            },
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0md_to_md,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        /* AC0FSPD conversion handled in function_id-specific arrays above (CK-Lite, IBIS4, Harrier) */
        {
            .ddm = {AC0LGT, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0lgt_to_lgt,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        {
            .ddm = {AC0ON, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0on_to_on,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        /* remote data 2 */
        {
            .ddm = {AC0TTEMP, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0ttemp_to_ttemp,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        {
            .ddm = {AC0SLEEP, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0sleep_to_sleep,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        {
            .ddm = {AC0TOFFA, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0toffa_to_toffa,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        {
            .ddm = {AC0TONA, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0tona_to_tona,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        /* remote data 3 */
        {
            .ddm = {AC0TONM, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0tonm_to_tonm,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        /* remote data 4 */
        {
            .ddm = {AC0TOFFM, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0toffm_to_toffm,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        /* remote data 5 */
        // 'TimerMinLSBits' handled by 'remote data 3' and 'remote data 4' conversion functions
        /* remote data 6 */
        {
            .ddm = {AC0SYSU, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_conv_ac0sysu_to_cf,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
        /* ctrl status */
        {
            .ddm = {AC0REMCTRL, 0, 0},
            .convert_ddm_to_lin = dometic_apac_ac_ac0remctrl_to_rem_ctrl_dis,
            .ctrl_frame_def = &lin_server_apac_ac_ctrl_frame_def,
        },
};

static const lin_server_map_lin_to_ddm_t lin_server_apac_ac_lin_to_ddm[] =
    {
        /* remote data 1 */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_md_to_ac0md,
            .ddm = {AC0MD, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        /* AC0FSPD conversion handled in function_id-specific arrays above (CK-Lite, IBIS4, Harrier) */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_lgt_to_ac0lgt,
            .ddm = {AC0LGT, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_on_to_ac0on,
            .ddm = {AC0ON, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        /* remote data 2 */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_ttemp_to_ac0ttemp,
            .ddm = {AC0TTEMP, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_sleep_to_ac0sleep,
            .ddm = {AC0SLEEP, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_toffa_to_ac0toffa,
            .ddm = {AC0TOFFA, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_tona_to_ac0tona,
            .ddm = {AC0TONA, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        /* remote data 3 */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_tonm_to_ac0tonm,
            .ddm = {AC0TONM, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        /* remote data 4 */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_toffm_to_ac0toffm,
            .ddm = {AC0TOFFM, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        /* remote data 5 */
        // 'TimerMinLSBits' handled by 'remote data 3' and 'remote data 4' conversion functions
        /* remote data 6 */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_cf_to_ac0sysu,
            .ddm = {AC0SYSU, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        /* system status */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_comprun_to_ac0actext,
            .ddm = {AC0ACTEXT, 0, 0},
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_itemp_to_ac0itemp,
            .ddm = {
                AC0ITEMP,
                0,
                0,
            },
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
        /* info status */
        {
            .convert_lin_to_ddm = dometic_apac_ac_conv_errors_to_ac0status,
            .ddm = {
                AC0STATUS,
                0,
                0,
            },
            .info_frame_def = &lin_server_apac_ac_info_frame_def,
        },
};

const lin_server_slave_device_t lin_server_apac_ac_device =
    {
        .device_class = AC0,
        .data = &data,
        .name = "APAC_AC",
        .device_type = LIN_SERVER_DEVICE_TYPE_APAC_AC,
        .ddm_owned_store = &apac_ac_ddm_owned_store,
        .ddm_owned_initial_values = apac_ac_ddm_owned_initial_values,
        .ddm_owned_initial_values_size = ELEMENTS(apac_ac_ddm_owned_initial_values),
        .ddm_production_store = &apac_ac_ddm_production_store,
        .ddm_production_initial_values = apac_ac_ddm_production_initial_values,
        .ddm_production_initial_values_size = ELEMENTS(apac_ac_ddm_production_initial_values),
        .frames_bundle_defs = lin_server_apac_ac_frames_bundle_defs,
        .frames_bundle_defs_size = ELEMENTS(lin_server_apac_ac_frames_bundle_defs),
        .map_ddm_to_lin = lin_server_apac_ac_ddm_to_lin,
        .map_ddm_to_lin_size = ELEMENTS(lin_server_apac_ac_ddm_to_lin),
        .map_lin_to_ddm = lin_server_apac_ac_lin_to_ddm,
        .map_lin_to_ddm_size = ELEMENTS(lin_server_apac_ac_lin_to_ddm),
        .function_specific_config_data = apac_ac_function_specific_config_data,
        .function_specific_config_data_size = ELEMENTS(apac_ac_function_specific_config_data),
        .device_config = &apac_ac_config_data,
        .protocol = LIN_SERVER_SYNC_PROTOCOL_LOCAL_CHANGE,
        .generic_profile = &apac_ac_generic_profile,
        .ddm2_to_lin_ctrl_logic = lin_server_apac_ac_parameter_ctrl_logic,
        .ddm2_to_lin_ctrl_logic_size = ELEMENTS(lin_server_apac_ac_parameter_ctrl_logic),
        .lin_to_ddm2_info_logic = lin_server_apac_ac_parameter_info_logic,
        .lin_to_ddm2_info_logic_size = ELEMENTS(lin_server_apac_ac_parameter_info_logic),
        .init_function = dometic_apac_ac_init_function,
        .stuff_function = dometic_apac_ac_stuff_function,
        .extract_function = dometic_apac_ac_extract_function,
};

static int dometic_apac_ac_init_function(const lin_server_slave_device_t *slave_device)
{
    TRUE_CHECK_RETURN0(slave_device->data->data);
    TRUE_CHECK_RETURN0(slave_device->data->data_size == sizeof(apac_ac_timers_t));

    apac_ac_timers_t *apac_ac_timers = (apac_ac_timers_t *)slave_device->data->data;

    fsm_initialize(&apac_ac_timers->sm_timer_on, sm_state_timer_on_idle);
    fsm_initialize(&apac_ac_timers->sm_timer_off, sm_state_timer_off_idle);

    return 1;
}

/* Executed as part of the UART task */
static int dometic_apac_ac_stuff_function(const lin_server_slave_device_t *slave_device, const lin_server_device_frames_bundle_def_t *frames_bundle_defs, uint8_t *data, size_t *data_size, lin_scheduler_frame_type_requests_t scheduled_frame_type_requests)
{
    (void)(slave_device);
    lin_server_device_frame_t *ctrl_frame = frames_bundle_defs->ctrl_frame_def->frame;
    const lin_server_device_frame_t *info_frame = frames_bundle_defs->info_frame_def->frame;

    uint8_t size = 0;
    if (LIN_SCHEDULE_FRAME_TYPE_GET(scheduled_frame_type_requests, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST) ||
        LIN_SCHEDULE_FRAME_TYPE_GET(scheduled_frame_type_requests, SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST))
    {
        // Remote 1..6 (first 0-5bytes)
        size = LIN_FRAME_DATA_LEN - 2;
        TRUE_CHECK_RETURN0(size <= *data_size);
        memcpy(data, info_frame->frame_signals.apac_ac_frame.info_frame.info_frame, size);

        // 6byte reserved(0XFF)
        data[size] = ctrl_frame->frame_signals.apac_ac_frame.ctrl_frame.reserverd = 0xFF;

        size++;
        TRUE_CHECK_RETURN0(size <= *data_size);
        data[size] = ctrl_frame->frame_signals.apac_ac_frame.ctrl_frame.ctrl_status.ctrl_status;

        size++;  // 8bytes has been written
    }
    else if (LIN_SCHEDULE_FRAME_TYPE_GET(scheduled_frame_type_requests, SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST))
    {
        size = LIN_FRAME_DATA_LEN;
        memcpy(data, ctrl_frame->frame_signals.apac_ac_frame.ctrl_frame.ctrl_frame, size);
    }

    *data_size = size;

    return 1;
}

/* Executed as part of the UART task */
static int dometic_apac_ac_extract_function(const lin_server_slave_device_t *slave_device, const lin_server_device_frames_bundle_def_t *frames_bundle_defs, const uint8_t *data, size_t data_size, lin_server_slave_info_response_t *info_status)
{
    (void)(slave_device);
    (void)(frames_bundle_defs);
    lin_server_device_frame_apac_ac_t *apac_ac_info_frame = (lin_server_device_frame_apac_ac_t *)data;

    /* Slave should have the RTU shut down all the load */
    if (apac_ac_info_frame->info_frame.info_status.LinError)
    {
        info_status->ci_error = apac_ac_info_frame->info_frame.info_status.LinError;
    }
    if (apac_ac_info_frame->info_frame.info_status.Error)
    {
        info_status->error = apac_ac_info_frame->info_frame.info_status.Error;
    }
    if (apac_ac_info_frame->info_frame.info_status.NoMain)
    {
        info_status->no_main = apac_ac_info_frame->info_frame.info_status.NoMain;
    }
    if (apac_ac_info_frame->info_frame.info_status.NotInit)
    {
        info_status->no_init = apac_ac_info_frame->info_frame.info_status.NotInit;
    }

    return 1;
}

/* DDM logic functions */
static lin_server_logic_func_response_t dometic_apac_ac_logic_ac0fspd_pre_send(const lin_server_slave_device_t *slave_device, const ddm_entry_t *const updated_parameter, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    (void)slave_device;
    (void)ctrl_frame_def;
    int32_t *ac0md_value;
    size_t ac0md_value_size;

    ddm_entry_t *ac0md = ddm_store__access(ddm_store, AC0MD);
    ddm_entry__read__value(ac0md, (const void **)&ac0md_value, &ac0md_value_size);

    if (*ac0md_value == AC0MD_AUTO)
    {
        LOG(W, "Fanspeed changes to speed %d is not supported when AC mode is set to AUTO", ddm_entry__value_i32(updated_parameter));
        return LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
    }

    return LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
}

/* DDM to LIN conversion functions */
/* Remote Data 1..6 */
static int dometic_apac_ac_conv_ac0md_to_md(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0md = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    switch (*ac0md)
    {
    case AC0MD_AUTO:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.Mode = 0;
        break;
    case AC0MD_COOL:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.Mode = 1;
        break;
    case AC0MD_DRY:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.Mode = 2;
        break;
    case AC0MD_HEAT:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.Mode = 3;
        break;
    case AC0MD_FAN:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.Mode = 4;
        break;
    case AC0MD_TURBO:
    default:
        LOG(E, "Mode not supported: %d", *ac0md);
        return 0;
    }

    return 1;
}
static int dometic_apac_ac_conv_ac0lgt_to_lgt(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0lgt = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    apac_ac_ctrl_frame->ctrl_frame.remote_data_1.Light = (uint8_t)*ac0lgt;

    return 1;
}
static int dometic_apac_ac_conv_ac0on_to_on(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0on = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    apac_ac_ctrl_frame->ctrl_frame.remote_data_1.Power = (uint8_t)*ac0on;

    return 1;
}
static int dometic_apac_ac_conv_ac0ttemp_to_ttemp(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0ttemp = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    // Temperature = Temp + 16
    apac_ac_ctrl_frame->ctrl_frame.remote_data_2.Temp = (uint8_t)(*ac0ttemp / Ddm2_unit_factor_list[DDM2_UNIT_DEGC]) - 16u;

    return 1;
}
static int dometic_apac_ac_conv_ac0sleep_to_sleep(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0sleep = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    apac_ac_ctrl_frame->ctrl_frame.remote_data_2.SleepMode = (uint8_t)*ac0sleep;

    return 1;
}
static int dometic_apac_ac_conv_ac0toffa_to_toffa(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0toffa = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    /* Set the timer enable flag */
    apac_ac_ctrl_frame->ctrl_frame.remote_data_2.TimerOffMode = (uint8_t)*ac0toffa;
    /* Set the timer update flag */
    apac_ac_ctrl_frame->ctrl_frame.ctrl_status.TimerUpdate = 1;

    return 1;
}
static int dometic_apac_ac_conv_ac0tona_to_tona(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0tona = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    /* Set the timer enable flag */
    apac_ac_ctrl_frame->ctrl_frame.remote_data_2.TimerOnMode = (uint8_t)*ac0tona;
    /* Set the timer update flag */
    apac_ac_ctrl_frame->ctrl_frame.ctrl_status.TimerUpdate = 1;

    return 1;
}
static int dometic_apac_ac_conv_ac0tonm_to_tonm(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0tonm = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    int8_t timer_msb = (*ac0tonm / 10);
    int8_t timer_lsb = (*ac0tonm % 10);

    /* Set the timer left time to event */
    apac_ac_ctrl_frame->ctrl_frame.remote_data_3.OnTimerMSBits = timer_msb;
    apac_ac_ctrl_frame->ctrl_frame.remote_data_5.TimerMinLSBits = timer_lsb;

    return 1;
}
static int dometic_apac_ac_conv_ac0toffm_to_toffm(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0toffm = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    int8_t timer_msb = (*ac0toffm / 10);
    int8_t timer_lsb = (*ac0toffm % 10);

    /* Set the timer left time to event */
    apac_ac_ctrl_frame->ctrl_frame.remote_data_4.OffTimerMSBits = timer_msb;
    apac_ac_ctrl_frame->ctrl_frame.remote_data_5.TimerMinLSBits = timer_lsb;

    return 1;
}
static int dometic_apac_ac_conv_ac0sysu_to_cf(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0sysu = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    apac_ac_ctrl_frame->ctrl_frame.remote_data_6.CF = (uint8_t)*ac0sysu;

    return 1;
}
// /* Remote data 1..6 && Ctrl frame */
static int dometic_apac_ac_ac0remctrl_to_rem_ctrl_dis(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0remctrl = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    apac_ac_ctrl_frame->ctrl_frame.ctrl_status.RemoteCtrlDIs = (uint8_t)*ac0remctrl;

    return 1;
}

/* LIN to DDM conversion functions */
/* Remote Data 1..6 */
static int dometic_apac_ac_conv_md_to_ac0md(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0md = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    uint32_t mode = apac_ac_info_frame->info_frame.remote_data_1.Mode;

    switch (mode)
    {
    case 0:  // Auto
        *ac0md = AC0MD_AUTO;
        break;
    case 1:  // Cool
        *ac0md = AC0MD_COOL;
        break;
    case 2:  // Dry
        *ac0md = AC0MD_DRY;
        break;
    case 3:  // Heat
        *ac0md = AC0MD_HEAT;
        break;
    case 4:  // Fan
        *ac0md = AC0MD_FAN;
        break;
    case 5:  // reserevd, Fall through
    case 6:  // reserevd, Fall through
    case 7:  // reserevd, Fall through
    default:
        return 0;
    }

    return 1;
}
static int dometic_apac_ac_conv_lgt_to_ac0lgt(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0lgt = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0lgt = apac_ac_info_frame->info_frame.remote_data_1.Light;

    return 1;
}
static int dometic_apac_ac_conv_on_to_ac0on(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0on = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0on = apac_ac_info_frame->info_frame.remote_data_1.Power;

    return 1;
}
static int dometic_apac_ac_conv_ttemp_to_ac0ttemp(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0ttemp = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    // Temperature = Temp + 16
    *ac0ttemp = (int32_t)((apac_ac_info_frame->info_frame.remote_data_2.Temp + 16u) * Ddm2_unit_factor_list[DDM2_UNIT_DEGC]);

    return 1;
}
static int dometic_apac_ac_conv_sleep_to_ac0sleep(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0sleep = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0sleep = apac_ac_info_frame->info_frame.remote_data_2.SleepMode;

    return 1;
}
static int dometic_apac_ac_conv_toffa_to_ac0toffa(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0toffa = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0toffa = apac_ac_info_frame->info_frame.remote_data_2.TimerOffMode;

    return 1;
}
static int dometic_apac_ac_conv_tona_to_ac0tona(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0ton = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0ton = apac_ac_info_frame->info_frame.remote_data_2.TimerOnMode;

    return 1;
}
static int dometic_apac_ac_conv_tonm_to_ac0tonm(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0tonm = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    int8_t timer_msb = apac_ac_info_frame->info_frame.remote_data_3.OnTimerMSBits;
    int8_t timer_lsb = apac_ac_info_frame->info_frame.remote_data_5.TimerMinLSBits;

    *ac0tonm = (timer_msb * 10) + timer_lsb;

    return 1;
}
static int dometic_apac_ac_conv_toffm_to_ac0toffm(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0toffm = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    int8_t timer_msb = apac_ac_info_frame->info_frame.remote_data_4.OffTimerMSBits;
    int8_t timer_lsb = apac_ac_info_frame->info_frame.remote_data_5.TimerMinLSBits;

    *ac0toffm = (timer_msb * 10) + timer_lsb;

    return 1;
}
static int dometic_apac_ac_conv_cf_to_ac0sysu(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0sysu = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0sysu = apac_ac_info_frame->info_frame.remote_data_6.CF;

    return 1;
}
/* System status */
static int dometic_apac_ac_conv_comprun_to_ac0actext(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(uint32_t));

    uint32_t *ac0actext = (uint32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    uint32_t compRun = apac_ac_info_frame->info_frame.system_status.CompRun;

    AC0ACTEXT_OUT_COMPRESSOR_SET(*ac0actext, compRun);

    return 1;
}
static int dometic_apac_ac_conv_itemp_to_ac0itemp(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0itemp = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0itemp = apac_ac_info_frame->info_frame.system_status.IntTemp * Ddm2_unit_factor_list[DDM2_UNIT_DEGC];

    return 1;
}
/* Info status */
static int dometic_apac_ac_conv_errors_to_ac0status(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    /* AC0STATUS is an array of unsigned 16-bit integers */
    uint16_t *ac0status_data = (uint16_t *)ddm2_parameters_value[0];
    size_t *ac0status_data_size = &ddm2_parameters_value_size[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    int errors_count = 0;
    uint16_t ac0status_actual_data_size = 0;
    if (apac_ac_info_frame->info_frame.info_status.LinError)
    {
        /* Set the CI bus communication error code */
        ac0status_data[errors_count] = GENERIC_CI_BUS_COMMUNICATION_ERROR;
        ac0status_actual_data_size += sizeof(ac0status_data[errors_count]);
        errors_count++;
    }
    if (apac_ac_info_frame->info_frame.info_status.Error)
    {
        /* Set the Ambient temperature sensor error code */
        ac0status_data[errors_count] = AIRC_OUTDOOR_TEMP_SENSOR_ERROR;
        ac0status_actual_data_size += sizeof(ac0status_data[errors_count]);
        errors_count++;
    }
    if (apac_ac_info_frame->info_frame.info_status.NoMain)
    {
        /* Set the over voltage error code */
        ac0status_data[errors_count] = GENERIC_AC_OVER_VOLTAGE_ERROR;
        ac0status_actual_data_size += sizeof(ac0status_data[errors_count]);
        errors_count++;
    }

    if (errors_count == 0)
    {
        ac0status_data[errors_count] = GENERIC_NO_ERRORS;
        ac0status_actual_data_size += sizeof(ac0status_data[errors_count]);
    }

    *ac0status_data_size = ac0status_actual_data_size;

    return true;
}

/* CK-Lite */
/* DDM to LIN conversion functions */
static int dometic_apac_ac_ck_lite_conv_ac0fspd_to_fspd(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0fspd = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    switch (*ac0fspd)
    {
    case AC0FSPD_AUTO:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 0;
        break;
    case AC0FSPD_LOW:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 1;
        break;
    case AC0FSPD_MED:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 2;
        break;
    case AC0FSPD_HIGH:  // fall through
    case AC0FSPD_MAX:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 3;
        break;
    default:
        LOG(E, "Fan speed not supported: %d", *ac0fspd);
        return 0;
    }

    return 1;
}
/* LIN to DDM conversion functions */
static int dometic_apac_ac_ck_lite_conv_fspd_to_ac0fspd(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0fspd = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    uint32_t fspd = apac_ac_info_frame->info_frame.remote_data_1.FanSet;

    switch (fspd)
    {
    case 0:
        *ac0fspd = AC0FSPD_AUTO;
        break;
    case 1:
        *ac0fspd = AC0FSPD_LOW;
        break;
    case 2:
        *ac0fspd = AC0FSPD_MED;
        break;
    case 3:
        // CK-Lite ambiguity resolution: LIN value 3 can represent both HIGH and MAX
        // Check current DDM2 value(use it as current state) to preserve user's original intent
        if (*ac0fspd == AC0FSPD_MAX)
        {
            // User originally requested MAX - preserve it (don't overwrite with HIGH)
            // AC internally treats both HIGH and MAX as LIN value 3
        }
        else
        {
            // User originally requested HIGH - set to HIGH
            *ac0fspd = AC0FSPD_HIGH;
        }
        break;
    default:
        LOG(E, "Fan speed not supported: %d", fspd);
        return 0;
    }

    return 1;
}

/* Harrier */
/* DDM to LIN conversion functions */
static int dometic_apac_ac_harrier_conv_ac0pur_to_pur(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0pur = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    apac_ac_ctrl_frame->ctrl_frame.remote_data_2.Pure = (uint8_t)*ac0pur;

    return 1;
}
static int dometic_apac_ac_harrier_conv_ac0flaps_to_flap(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0flaps = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    switch (*ac0flaps)
    {
    case AC0FLAPS_FLAPS_STOPPED:  // Fall through
    case AC0FLAPS_FLAP1_ACTIVE:   // Fall through
    case AC0FLAPS_FLAP2_ACTIVE:   // Fall through
    case AC0FLAPS_FLAPS_ACTIVE:   // Fall through
        apac_ac_ctrl_frame->ctrl_frame.remote_data_5.Flap = (uint8_t)*ac0flaps;
        break;
    default:
        LOG(E, "Unsupported Flap value: %d", *ac0flaps);
        return 0;
    }

    return 1;
}

/* LIN to DDM conversion functions */
static int dometic_apac_ac_harrier_conv_pure_to_ac0pur(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0pur = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0pur = apac_ac_info_frame->info_frame.remote_data_2.Pure;

    return 1;
}
static int dometic_apac_ac_harrier_conv_flap_to_ac0flaps(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0flaps = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    *ac0flaps = apac_ac_info_frame->info_frame.remote_data_5.Flap;

    return 1;
}

/* IBIS4 && Harrier */
/* DDM to LIN conversion functions */
static int dometic_apac_ac_ibis4_harrier_conv_ac0fspd_to_fspd(const uint8_t *ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t *slave_device_frame)
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    const int32_t *ac0fspd = (const int32_t *)ddm2_parameters_value[0];
    lin_server_device_frame_apac_ac_t *apac_ac_ctrl_frame = &slave_device_frame->apac_ac_frame;

    switch (*ac0fspd)
    {
    case AC0FSPD_AUTO:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 0;
        break;
    case AC0FSPD_LOW:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 1;
        break;
    case AC0FSPD_MED:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 3;
        break;
    case AC0FSPD_HIGH:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 5;
        break;
    case AC0FSPD_MAX:
        apac_ac_ctrl_frame->ctrl_frame.remote_data_1.FanSet = 7;
        break;
    default:
        LOG(E, "Fan speed not supported: %d", *ac0fspd);
        return 0;
    }

    return 1;
}
/* LIN to DDM conversion functions */
static int dometic_apac_ac_ibis4_harrier_conv_fspd_to_ac0fspd(const lin_server_device_frame_signals_t *const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY])
{
    TRUE_CHECK_RETURN0(ddm2_parameters_value_size[0] == sizeof(int32_t));

    int32_t *ac0fspd = (int32_t *)ddm2_parameters_value[0];
    const lin_server_device_frame_apac_ac_t *apac_ac_info_frame = &slave_device_frame->apac_ac_frame;

    uint32_t fspd = apac_ac_info_frame->info_frame.remote_data_1.FanSet;

    switch (fspd)
    {
    case 0:
        *ac0fspd = AC0FSPD_AUTO;
        break;
    case 1:  // fall through
    case 2:
        *ac0fspd = AC0FSPD_LOW;
        break;
    case 3:  // fall through
    case 4:
        *ac0fspd = AC0FSPD_MED;
        break;
    case 5:  // fall through
    case 6:
        *ac0fspd = AC0FSPD_HIGH;
        break;
    case 7:
        *ac0fspd = AC0FSPD_MAX;
        break;
    default:
        LOG(E, "Fan speed not supported: %d", fspd);
        return 0;
    }

    return 1;
}

/* Timers helper functions */
static const char *timers_decode_event_id(uint8_t event_id)
{
    switch (event_id)
    {
    case FSM_ENTRY_EVENT:
        return "TIMER_EVENT_ENTRY";
    case FSM_EXIT_EVENT:
        return "TIMER_EVENT_EXIT";
    case SM_TIMERS_EVENT_ID_AC0TONA:
        return "TIMER_EVENT_AC0TONA";
    case SM_TIMERS_EVENT_ID_AC0TOFFM:
        return "TIMER_EVENT_AC0TOFFM";
    case SM_TIMERS_EVENT_ID_AC0TONM:
        return "TIMER_EVENT_AC0TONM";
    case SM_TIMERS_EVENT_ID_AC0TOFFA:
        return "TIMER_EVENT_AC0TOFFA";
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
        return "TIMER_EVENT_POST_RECEIVE";
    case SM_TIMERS_EVENT_ID_POST_SEND:
        return "TIMER_EVENT_POST_SEND";
    default:
        return "NONE";
    }
}

/* Timer ON implementation START */
static bool timer_on_info_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *info_frame)
{
    return ((info_frame->info_frame.remote_data_3.OnTimerMSBits != 0) || (info_frame->info_frame.remote_data_5.TimerMinLSBits) ? true : false);
}

static bool timer_on_ctrl_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *ctrl_frame)
{
    return ((ctrl_frame->ctrl_frame.remote_data_3.OnTimerMSBits != 0) || (ctrl_frame->ctrl_frame.remote_data_5.TimerMinLSBits) ? true : false);
}

static bool timer_on_info_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *info_frame)
{
    return (info_frame->info_frame.remote_data_2.TimerOnMode != 0 ? true : false);
}

static bool timer_on_ctrl_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *ctrl_frame)
{
    return (ctrl_frame->ctrl_frame.remote_data_2.TimerOnMode != 0 ? true : false);
}

static bool timer_on_info_frame_is_timeout_set(const lin_server_device_frame_apac_ac_t *info_frame)
{
    return (info_frame->info_frame.info_status.TimerOnReq != 0 ? true : false);
}

static void sm_state_timer_on_deactivate(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_POST_SEND:
    {
        const lin_server_device_frame_apac_ac_t *ctrl_frame = (const lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_on_ctrl_frame_is_enabled_set(ctrl_frame) == false)
        {
            if (timer_on_ctrl_frame_are_minutes_set(ctrl_frame) == true)
            {
                /* Force reschedule of the ctrl frame, which will endup sending the same AC0TON and AC0TONM with TimerUpdate = 0.
                 * There is a bug on the APAC AC side that prevents the AC0TONM from updating to 0. As a result,
                 * the timer icon remains displayed on the screen, and the countdown continues. When the countdown
                 * finishes, APAC AC will turn on. Forcing AC0TONM to 0 does not help as is not accepted by the APAC AC,
                 * so in the next info frames we will get the time remaing back.
                 *
                 * Forcing another frame with TimerUpdate=0 must be done, so the APAC AC can process the request correctly.
                 */
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
                fsm_state_change(fsm, sm_state_timer_on_en_pending);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE;
                fsm_state_change(fsm, sm_state_timer_on_idle);
            }
        }
        else
        {
            if (timer_on_ctrl_frame_are_minutes_set(ctrl_frame) == true)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
                // You cannot enter in 'Active' state from 'Deactivate' state.
                LOG(E, " Ivalid TimerOn parameter values: tona: %d tonm_lsb: %d tonm_lsb: %d",
                    ctrl_frame->ctrl_frame.remote_data_2.TimerOnMode,
                    ctrl_frame->ctrl_frame.remote_data_3.OnTimerMSBits,
                    ctrl_frame->ctrl_frame.remote_data_5.TimerMinLSBits);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE;
                fsm_state_change(fsm, sm_state_timer_on_min_pending);
            }
        }

        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_on_active(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TONA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tona = *(int32_t *)sm_timers_data->value;
        if (ac0tona != 0)
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_on_deactivate);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TONM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tonm = *(int32_t *)sm_timers_data->value;
        if (ac0tonm != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_on_activate);
        }
        else
        {
            /* Description about this request is added in sm_state_timer_on_deactivate, event SM_TIMERS_EVENT_ID_POST_SEND */
            fsm_state_change(fsm, sm_state_timer_on_deactivate);
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_on_info_frame_is_timeout_set(info_frame) == true)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_on_idle);
        }
        else
        {
            if (timer_on_info_frame_is_enabled_set(info_frame) == true)
            {
                if (timer_on_info_frame_are_minutes_set(info_frame) == true)
                {
                    // still counting
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                }
                else
                {
                    /* NOTE: Enabling of timer i.e. AC0ON = 1 and AC0ONM != 0 e.g. entering in active state
                     * will produce first post receive info frame from APAC AC with AC0ON = 1 and AC0ONM = 0;
                     * after couple of iterrations it will produce info frame with AC0ONM = <value_that_we_set>.
                     */
                    LOG(D, "TimerON in Active state, but APAC AC sent TimerONMIN = 0.");
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                    fsm_state_change(fsm, sm_state_timer_on_min_pending);
                }
            }
            else
            {
                if (timer_on_info_frame_are_minutes_set(info_frame) == true)
                {
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                    fsm_state_change(fsm, sm_state_timer_on_en_pending);
                }
                else
                {
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                    fsm_state_change(fsm, sm_state_timer_on_idle);
                }
            }
        }
        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_on_activate(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_POST_SEND:
    {
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE;
        fsm_state_change(fsm, sm_state_timer_on_active);
        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_on_en_pending(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TONA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tona = *(int32_t *)sm_timers_data->value;
        if (ac0tona == 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_on_idle);
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_on_activate);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TONM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tonm = *(int32_t *)sm_timers_data->value;
        if (ac0tonm != 0)
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_on_idle);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_on_info_frame_is_enabled_set(info_frame) == true)
        {
            if (timer_on_info_frame_are_minutes_set(info_frame) == true)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_on_active);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_on_min_pending);
            }
        }
        else
        {
            if (timer_on_info_frame_are_minutes_set(info_frame) == true)
            {
                /* Force AC0TONM to 0, as if we are in this state, it means that the update for
                 * the residual timer time signal only reflects TIMER OFF, not timer ON.
                 */
                ddm_entry__set__value_i32(ddm_store__access(sm_timers_data->store, AC0TONM), 0);

                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_on_idle);
            }
        }
        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_on_min_pending(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TONA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tona = *(int32_t *)sm_timers_data->value;
        if (ac0tona != 0)
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_on_deactivate);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TONM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tonm = *(int32_t *)sm_timers_data->value;
        if (ac0tonm != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_on_activate);
        }
        else
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_on_info_frame_is_enabled_set(info_frame) == true)
        {
            if (timer_on_info_frame_are_minutes_set(info_frame) == true)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_on_active);
            }
            else
            {
                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
        }
        else
        {
            if (timer_on_info_frame_are_minutes_set(info_frame) == true)
            {
                /* Force AC0TONM to 0, as if we are in this state, it means that the update for
                 * the residual timer time signal only reflects TIMER OFF, not timer ON.
                 */
                ddm_entry__set__value_i32(ddm_store__access(sm_timers_data->store, AC0TONM), 0);

                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_on_idle);
            }
        }

        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_on_idle(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TONA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tona = *(int32_t *)sm_timers_data->value;
        if (ac0tona != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_on_min_pending);
        }
        else
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TONM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0tonm = *(int32_t *)sm_timers_data->value;
        if (ac0tonm != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_on_en_pending);
        }
        else
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_on_info_frame_is_enabled_set(info_frame) == true)
        {
            if (timer_on_info_frame_are_minutes_set(info_frame) == false)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_on_min_pending);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_on_active);
            }
        }
        else
        {
            if (timer_on_info_frame_are_minutes_set(info_frame) == false)
            {
                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
            else
            {
                /* Force AC0TONM to 0, as if we are in this state, it means that the update for
                 * the residual timer time signal only reflects TIMER OFF, not timer ON.
                 */
                ddm_entry__set__value_i32(ddm_store__access(sm_timers_data->store, AC0TONM), 0);

                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
        }

        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static lin_server_logic_func_response_t dometic_apac_logic_timer_on_pre_send(const lin_server_slave_device_t *const device, const ddm_entry_t *const updated_parameter, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    lin_server_device_frame_signals_t *ctrl_frame_signals = &ctrl_frame_def->frame->frame_signals;
    sm_timers_data_t timers_data = {.store = ddm_store, .ptr = (void *)ctrl_frame_signals};
    apac_ac_timers_t *timers = (apac_ac_timers_t *)device->data->data;

    switch (ddm_entry__parameter_id(updated_parameter))
    {
    case AC0TONM:
        ddm_entry__read__value(updated_parameter, &timers_data.value, &timers_data.value_size);
        timers->sm_timer_on_event.p_data = &timers_data;
        timers->sm_timer_on_event.id = timers->event_id = SM_TIMERS_EVENT_ID_AC0TONM;
        fsm_state_dispatch(&timers->sm_timer_on, &timers->sm_timer_on_event);
        break;
    case AC0TONA:
        ddm_entry__read__value(updated_parameter, &timers_data.value, &timers_data.value_size);
        timers->sm_timer_on_event.p_data = &timers_data;
        timers->sm_timer_on_event.id = timers->event_id = SM_TIMERS_EVENT_ID_AC0TONA;
        fsm_state_dispatch(&timers->sm_timer_on, &timers->sm_timer_on_event);
        break;
    default:
        LOG(E, "Invalid parameter 0x%X", ddm_entry__parameter_id(updated_parameter));
        timers_data.status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        break;
    }

    if (timers_data.status == LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE)
    {
        ctrl_frame_signals->apac_ac_frame.ctrl_frame.ctrl_status.TimerUpdate = 1;
    }

    return timers_data.status;
}

static lin_server_logic_func_response_t dometic_apac_logic_timer_on_post_send(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    lin_server_device_frame_signals_t *ctrl_frame_signals = &ctrl_frame_def->frame->frame_signals;
    sm_timers_data_t timers_data = {.ptr = (void *)ctrl_frame_signals};
    apac_ac_timers_t *timers = (apac_ac_timers_t *)device->data->data;

    TRUE_CHECK_RETURN0(timers->event_id >= SM_TIMERS_EVENT_ID_AC0TONA && timers->event_id <= SM_TIMERS_EVENT_ID_AC0TOFFM);

    switch (timers->event_id)
    {
    case SM_TIMERS_EVENT_ID_AC0TONA:
    case SM_TIMERS_EVENT_ID_AC0TONM:
        timers->sm_timer_on_event.p_data = &timers_data;
        timers->sm_timer_on_event.id = SM_TIMERS_EVENT_ID_POST_SEND;
        fsm_state_dispatch(&timers->sm_timer_on, &timers->sm_timer_on_event);
        break;
    default:
        LOG(E, "Invalid post_send event: %d received", timers->event_id);
        return 0;
    }

    ctrl_frame_signals->apac_ac_frame.ctrl_frame.ctrl_status.TimerUpdate = 0;

    return timers_data.status;
}

static lin_server_logic_func_response_t dometic_apac_logic_timer_on_post_receive(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *info_frame_def, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    lin_server_device_frame_signals_t *ctrl_frame_signals = &ctrl_frame_def->frame->frame_signals;
    lin_server_device_frame_signals_t *info_frame_signals = &info_frame_def->frame->frame_signals;
    sm_timers_data_t timers_data = {.store = ddm_store, .ptr = (void *)info_frame_signals};
    apac_ac_timers_t *timers = (apac_ac_timers_t *)device->data->data;

    timers->sm_timer_on_event.p_data = &timers_data;
    timers->sm_timer_on_event.id = SM_TIMERS_EVENT_ID_POST_RECEIVE;
    fsm_state_dispatch(&timers->sm_timer_on, &timers->sm_timer_on_event);

    if (timers_data.status == LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE)
    {
        ctrl_frame_signals->apac_ac_frame.ctrl_frame.ctrl_status.TimerUpdate = 1;
    }

    return timers_data.status;
}
/* Timer ON implementation END */

/* Timer OFF implementation START */
static bool timer_off_info_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *info_frame)
{
    return ((info_frame->info_frame.remote_data_4.OffTimerMSBits != 0) || (info_frame->info_frame.remote_data_5.TimerMinLSBits) ? true : false);
}

static bool timer_off_ctrl_frame_are_minutes_set(const lin_server_device_frame_apac_ac_t *ctrl_frame)
{
    return ((ctrl_frame->ctrl_frame.remote_data_4.OffTimerMSBits != 0) || (ctrl_frame->ctrl_frame.remote_data_5.TimerMinLSBits) ? true : false);
}

static bool timer_off_info_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *info_frame)
{
    return (info_frame->info_frame.remote_data_2.TimerOffMode != 0 ? true : false);
}

static bool timer_off_ctrl_frame_is_enabled_set(const lin_server_device_frame_apac_ac_t *ctrl_frame)
{
    return (ctrl_frame->ctrl_frame.remote_data_2.TimerOffMode != 0 ? true : false);
}

static bool timer_off_info_frame_is_timeout_set(const lin_server_device_frame_apac_ac_t *info_frame)
{
    return (info_frame->info_frame.info_status.TimerOffReq != 0 ? true : false);
}

static void sm_state_timer_off_deactivate(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_POST_SEND:
    {
        const lin_server_device_frame_apac_ac_t *ctrl_frame = (const lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;

        if (timer_off_ctrl_frame_is_enabled_set(ctrl_frame) == false)
        {
            if (timer_off_ctrl_frame_are_minutes_set(ctrl_frame) == true)
            {
                /* Force reschedule of the ctrl frame, which will endup sending the same AC0TOFF and AC0TOFFM with TimerUpdate = 0.
                 * There is a bug on the APAC AC side that prevents the AC0TOFFM from updating to 0. As a result,
                 * the timer icon remains displayed on the screen, and the countdown continues. When the countdown
                 * finishes, APAC AC will turn off. Forcing AC0TOFFM to 0 does not help as is not accepted by the APAC AC,
                 * so in the next info frames we will get the time remaing back.
                 *
                 * Forcing another frame with TimerUpdate=0 must be done, so the APAC AC can process the request correctly.
                 */
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
                fsm_state_change(fsm, sm_state_timer_off_en_pending);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE;
                fsm_state_change(fsm, sm_state_timer_off_idle);
            }
        }
        else
        {
            if (timer_off_ctrl_frame_are_minutes_set(ctrl_frame) == true)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
                // You cannot enter in 'Active' state from 'Deactivate' state.
                LOG(E, " Ivalid TimerOff parameter values: toffa: %d toffm_lsb: %d toffm_lsb: %d",
                    ctrl_frame->ctrl_frame.remote_data_2.TimerOffMode,
                    ctrl_frame->ctrl_frame.remote_data_4.OffTimerMSBits,
                    ctrl_frame->ctrl_frame.remote_data_5.TimerMinLSBits);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE;
                fsm_state_change(fsm, sm_state_timer_off_min_pending);
            }
        }

        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_off_active(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TOFFA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffa = *(int32_t *)sm_timers_data->value;
        if (ac0toffa != 0)
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_off_deactivate);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TOFFM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffm = *(int32_t *)sm_timers_data->value;
        if (ac0toffm != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_off_activate);
        }
        else
        {
            /* Description about this request is added in sm_state_timer_off_deactivate, event SM_TIMERS_EVENT_ID_POST_SEND */
            fsm_state_change(fsm, sm_state_timer_off_deactivate);
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_off_info_frame_is_timeout_set(info_frame) == true)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_off_idle);
        }
        else
        {
            if (timer_off_info_frame_is_enabled_set(info_frame) == true)
            {
                if (timer_off_info_frame_are_minutes_set(info_frame) == true)
                {
                    // still counting
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                }
                else
                {
                    /* NOTE: Enabling of timer i.e. AC0OFFA = 1 and AC0OFFM != 0 e.g. entering in active state
                     * will produce first post receive info frame from APAC AC with AC0OFFA = 1 and AC0OFFM = 0;
                     * after couple of iterrations it will produce info frame with AC0OFFM = <value_that_we_set>.
                     */
                    LOG(D, "TimerOFF in Active state, but APAC AC sent TimerOFFMIN = 0.");
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                    fsm_state_change(fsm, sm_state_timer_off_min_pending);
                }
            }
            else
            {
                if (timer_off_info_frame_are_minutes_set(info_frame) == true)
                {
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                    fsm_state_change(fsm, sm_state_timer_off_en_pending);
                }
                else
                {
                    sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                    fsm_state_change(fsm, sm_state_timer_off_idle);
                }
            }
        }
        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_off_activate(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_POST_SEND:
    {
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE;
        fsm_state_change(fsm, sm_state_timer_off_active);
        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_off_en_pending(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TOFFA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffa = *(int32_t *)sm_timers_data->value;
        if (ac0toffa == 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_off_idle);
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_off_activate);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TOFFM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffm = *(int32_t *)sm_timers_data->value;
        if (ac0toffm != 0)
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_off_idle);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_off_info_frame_is_enabled_set(info_frame) == true)
        {
            if (timer_off_info_frame_are_minutes_set(info_frame) == true)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_off_active);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_off_min_pending);
            }
        }
        else
        {
            if (timer_off_info_frame_are_minutes_set(info_frame) == true)
            {
                /* Force AC0TOFFM to 0, as if we are in this state, it means that the update for
                 * the residual timer time signal only reflects TIMER ON, not timer OFF.
                 */
                ddm_entry__set__value_i32(ddm_store__access(sm_timers_data->store, AC0TOFFM), 0);

                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_off_idle);
            }
        }
        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static void sm_state_timer_off_min_pending(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TOFFA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffa = *(int32_t *)sm_timers_data->value;
        if (ac0toffa != 0)
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }
        else
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_off_deactivate);
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TOFFM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffm = *(int32_t *)sm_timers_data->value;
        if (ac0toffm != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE;
            fsm_state_change(fsm, sm_state_timer_off_activate);
        }
        else
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_off_info_frame_is_enabled_set(info_frame) == true)
        {
            if (timer_off_info_frame_are_minutes_set(info_frame) == true)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_off_active);
            }
            else
            {
                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
        }
        else
        {
            if (timer_off_info_frame_are_minutes_set(info_frame) == true)
            {
                /* Force AC0TOFFM to 0, as if we are in this state, it means that the update for
                 * the residual timer time signal only reflects TIMER ON, not timer OFF.
                 */
                ddm_entry__set__value_i32(ddm_store__access(sm_timers_data->store, AC0TOFFM), 0);

                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_off_idle);
            }
        }

        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        break;
    }
}

static void sm_state_timer_off_idle(fsm_t *const fsm, fsm_event_t const *const event)
{
    APAC_TIMERS_STATE_LOG();

    sm_timers_data_t *sm_timers_data = (sm_timers_data_t *)event->p_data;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        break;
    case FSM_EXIT_EVENT:
        break;
    case SM_TIMERS_EVENT_ID_AC0TOFFA:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffa = *(int32_t *)sm_timers_data->value;
        if (ac0toffa != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_off_min_pending);
        }
        else
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_AC0TOFFM:
    {
        TRUE_CHECK_RETURN(sm_timers_data->value_size == sizeof(int32_t));

        int32_t ac0toffm = *(int32_t *)sm_timers_data->value;
        if (ac0toffm != 0)
        {
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            fsm_state_change(fsm, sm_state_timer_off_en_pending);
        }
        else
        {
            // Do not do anything, this is your state
            sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
        }

        break;
    }
    case SM_TIMERS_EVENT_ID_POST_RECEIVE:
    {
        lin_server_device_frame_apac_ac_t *info_frame = (lin_server_device_frame_apac_ac_t *)sm_timers_data->ptr;
        if (timer_off_info_frame_is_enabled_set(info_frame) == true)
        {
            if (timer_off_info_frame_are_minutes_set(info_frame) == false)
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_off_min_pending);
            }
            else
            {
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
                fsm_state_change(fsm, sm_state_timer_off_active);
            }
        }
        else
        {
            if (timer_off_info_frame_are_minutes_set(info_frame) == false)
            {
                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
            else
            {
                /* Force AC0TOFFM to 0, as if we are in this state, it means that the update for
                 * the residual timer time signal only reflects TIMER ON, not timer OFF.
                 */
                ddm_entry__set__value_i32(ddm_store__access(sm_timers_data->store, AC0TOFFM), 0);

                // Do not do anything, this is your state
                sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH;
            }
        }

        break;
    }
    default:
        sm_timers_data->status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        LOG(E, "Invalid state received: %d", event->id);
        break;
    }
}

static lin_server_logic_func_response_t dometic_apac_logic_timer_off_pre_send(const lin_server_slave_device_t *const device, const ddm_entry_t *const updated_parameter, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    lin_server_device_frame_signals_t *ctrl_frame_signals = &ctrl_frame_def->frame->frame_signals;
    sm_timers_data_t timers_data = {.store = ddm_store, .ptr = (void *)&ctrl_frame_signals->apac_ac_frame};
    apac_ac_timers_t *timers = (apac_ac_timers_t *)device->data->data;

    switch (ddm_entry__parameter_id(updated_parameter))
    {
    case AC0TOFFM:
        ddm_entry__read__value(updated_parameter, &timers_data.value, &timers_data.value_size);
        timers->sm_timer_off_event.p_data = &timers_data;
        timers->sm_timer_off_event.id = timers->event_id = SM_TIMERS_EVENT_ID_AC0TOFFM;
        fsm_state_dispatch(&timers->sm_timer_off, &timers->sm_timer_off_event);
        break;
    case AC0TOFFA:
        ddm_entry__read__value(updated_parameter, &timers_data.value, &timers_data.value_size);
        timers->sm_timer_off_event.p_data = &timers_data;
        timers->sm_timer_off_event.id = timers->event_id = SM_TIMERS_EVENT_ID_AC0TOFFA;
        fsm_state_dispatch(&timers->sm_timer_off, &timers->sm_timer_off_event);
        break;
    default:
        LOG(E, "Invalid parameter 0x%X", ddm_entry__parameter_id(updated_parameter));
        timers_data.status = LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION;
        break;
    }

    if (timers_data.status == LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE)
    {
        ctrl_frame_signals->apac_ac_frame.ctrl_frame.ctrl_status.TimerUpdate = 1;
    }

    return timers_data.status;
}

static lin_server_logic_func_response_t dometic_apac_logic_timer_off_post_send(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    lin_server_device_frame_signals_t *ctrl_frame_signals = &ctrl_frame_def->frame->frame_signals;
    sm_timers_data_t timers_data = {.ptr = (void *)&ctrl_frame_signals->apac_ac_frame};
    apac_ac_timers_t *timers = (apac_ac_timers_t *)device->data->data;

    switch (timers->event_id)
    {
    case SM_TIMERS_EVENT_ID_AC0TOFFA:
    case SM_TIMERS_EVENT_ID_AC0TOFFM:
        timers->sm_timer_off_event.p_data = &timers_data;
        timers->sm_timer_off_event.id = SM_TIMERS_EVENT_ID_POST_SEND;
        fsm_state_dispatch(&timers->sm_timer_off, &timers->sm_timer_off_event);
        break;
    default:
        LOG(E, "Invalid post_send event: %d received", timers->event_id);
        return 0;
    }

    ctrl_frame_signals->apac_ac_frame.ctrl_frame.ctrl_status.TimerUpdate = 0;

    return timers_data.status;
}

static lin_server_logic_func_response_t dometic_apac_logic_timer_off_post_receive(const lin_server_slave_device_t *const device, const lin_server_device_frame_def_t *info_frame_def, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    lin_server_device_frame_signals_t *ctrl_frame_signals = &ctrl_frame_def->frame->frame_signals;
    lin_server_device_frame_signals_t *info_frame_signals = &info_frame_def->frame->frame_signals;
    sm_timers_data_t timers_data = {.store = ddm_store, .ptr = (void *)&info_frame_signals->apac_ac_frame};
    apac_ac_timers_t *timers = (apac_ac_timers_t *)device->data->data;

    timers->sm_timer_off_event.p_data = &timers_data;
    timers->sm_timer_off_event.id = SM_TIMERS_EVENT_ID_POST_RECEIVE;
    fsm_state_dispatch(&timers->sm_timer_off, &timers->sm_timer_off_event);

    if (timers_data.status == LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE)
    {
        ctrl_frame_signals->apac_ac_frame.ctrl_frame.ctrl_status.TimerUpdate = 1;
    }

    return timers_data.status;
}
/* Timer OFF implementation END */
