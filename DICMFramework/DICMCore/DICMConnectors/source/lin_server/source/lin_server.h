/**
 * @file lin_server.h
 * @author Borjan Bozhinovski(borjan.bozhinovski@qinshift.com)
 * @date 2023-12-28
 */

#ifndef LIN_SERVER__
#define LIN_SERVER__

#include <stdint.h>
#include <string.h>

#include "lin_server_device_definition.h"

typedef enum lin_server_state_request
{
    LIN_SERVER_STATE_SLEEP_REQUEST_NONE,
    LIN_SERVER_STATE_SLEEP_REQUEST_TIMER,
    LIN_SERVER_STATE_SLEEP_REQUEST_BROKER,
    LIN_SERVER_STATE_WAKEUP_REQUEST_TIMER,
    LIN_SERVER_STATE_WAKEUP_REQUEST_BROKER,
    LIN_SERVER_STATE_WAKEUP_REQUEST_SLAVE,
} lin_server_state_request_t;

typedef enum lin_master_state
{
    LIN_SERVER_STATE_INVALID,
    LIN_SERVER_STATE_OPERATIONAL,
    LIN_SERVER_STATE_SLEEPING,
    LIN_SERVER_STATE_WAKEUP_PENDING,
} lin_server_state_t;

typedef enum lin_server_frame_selection
{
    LIN_SERVER_FRAME_SELECTION_NONE,
    LIN_SERVER_FRAME_SELECTION_SIGNAL,
    LIN_SERVER_FRAME_SELECTION_DIAG_SLEEP,
    LIN_SERVER_FRAME_SELECTION_DIAG_DEVICE_DETECTION,
} lin_server_frame_selection_t;

/* Lin Server defined functions */
void lin_server_init(void);
int lin_server_register_device(const lin_server_slave_device_t * slave_device);
void lin_server_remove_device(const lin_server_slave_device_t * slave_device);
void lin_server_handle_set(const lin_server_slave_device_t * const device, uint32_t parameter, const void * const value, size_t value_size);
void lin_server_handle_subscribe(const lin_server_slave_device_t * const device, uint32_t parameter);
void lin_server_handle_generic(uint32_t id, const void * const value, size_t value_size);
void lin_server_ddm2_to_lin_handle_logic_post_send(const lin_server_slave_device_t * device, const lin_server_device_frame_def_t * ctrl_frame_def);
void lin_server_handle_lin_info_frame_received(const lin_server_slave_device_t * device, const lin_server_device_frames_bundle_def_t * bundle_frame_def);
void lin_server_lock_device(const lin_server_slave_device_t * device);
void lin_server_unlock_device(const lin_server_slave_device_t * device);
/* CTRL/INFO frames bundle */
const lin_server_device_frames_bundle_def_t * lin_server_get_frame_bundle_definition(const lin_server_slave_device_t * device, lin_frame_type_t frame_type, uint8_t frame_id);
/* CTRL frame validation */
bool lin_server_has_any_ctrl_frame_been_sent(const lin_server_slave_device_t * device);
bool lin_server_has_ctrl_frame_been_sent(lin_server_device_frame_t * ctrl_frame);
bool lin_server_has_ctrl_frame_verification_attempts_left(lin_server_device_frame_t * ctrl_frame);
bool lin_server_has_ctrl_frame_verification_local_change_ack_attempts_left(lin_server_device_frame_t * ctrl_frame);
void lin_server_set_ctrl_frame_verification(lin_server_device_frame_t * ctrl_frame, int ctrl_frame_type);
int  lin_server_get_ctrl_frame_verification_type(lin_server_device_frame_t * ctrl_frame);
int  lin_server_get_ctrl_frame_verification_attempts_left(lin_server_device_frame_t * ctrl_frame);
void lin_server_reset_ctrl_frame_verification(lin_server_device_frame_t * ctrl_frame);
void lin_server_reset_ctrl_frame_verification_counter(lin_server_device_frame_t * ctrl_frame, int counter);
void lin_server_reset_ctrl_frame_verification_local_change_ack_counter(lin_server_device_frame_t * ctrl_frame);

/* Sync protocol (None, LocalChange, etc...) */
bool lin_server_state_evaluate(void);
bool lin_server_sync_protocol_local_change_is_local_change_bit_set(const lin_server_device_frame_def_t * frame_def, const lin_server_device_frame_t * info_frame);
bool lin_server_sync_protocol_local_change_is_sync_frame_bit_set(const lin_server_device_frame_def_t * frame_def, const lin_server_device_frame_t * ctrl_frame);
void lin_server_sync_protocol_local_change_set_sync_frame_bit(const lin_server_device_frame_def_t * frame_def, lin_server_device_frame_t * ctrl_frame);
void lin_server_sync_protocol_local_change_clear_sync_frame_bit(const lin_server_device_frame_def_t * frame_def, lin_server_device_frame_t * ctrl_frame);
lin_server_sync_protocol_type_t lin_server_get_sync_protocol(const lin_server_slave_device_t * device);
/* Go-to-sleep helper functions */
void lin_server_trigger_go_to_sleep_event(lin_server_state_request_t sleep_request);
void lin_server_trigger_wakeup_event(lin_server_state_request_t sleep_request);
void lin_server_switch_state(lin_server_state_t state);
/* Lin Server Connector defined functions */
int  lin_server_publish(uint32_t parameter, uint32_t instance, const void * const value, size_t value_size);
void lin_server_generic_event(uint32_t event_id, const void * const value, size_t value_size);
int  lin_server_register_ddm_class_instance(uint32_t device_class);
void lin_server_delete_ddm_class_instance(uint32_t device_class, uint32_t instance);
size_t lin_server_get_number_of_slave_devices(void);
const lin_server_slave_device_t * lin_server_get_slave_device(lin_server_device_type_t slave_device_type);
const lin_server_slave_device_t * lin_server_get_slave_device_by_index(uint32_t slave_device_index);
#endif //LIN_SERVER__