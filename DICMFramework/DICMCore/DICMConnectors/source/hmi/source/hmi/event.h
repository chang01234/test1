/*! \file event.h
 *	\brief Event generator public defines and function headers.
 *
 *  Make sure HMI_EVENT_DEBUG_NAMES is defined globally when building both hmi_data
 *  and fw for getting user friendly names of events.
 */
 
#ifndef EVENT_H_
#define EVENT_H_

/** Includes ******************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Defines *******************************************************************/
#define EVENT_MAX_KEY_PRESS             10	//!< Maximum number of key_press events.
#define EVENT_MAX_KEY_RELEASE           10	//!< Maximum number of key_release events.
#define EVENT_MAX_KEY_HOLD              10	//!< Maximum number of key_hold events.
#define EVENT_MAX_ROTATION              10	//!< Maximum number of rotation events.
#define EVENT_MAX_IDLE                  10	//!< Maximum number of idle events.
#define EVENT_MAX_DELAYED_EVENTS        10	//!< Maximum number of delayed events.
#define EVENT_MAX_DELAYED_MENU_EVENTS   10	//!< Maximum number of delayed menu events.

#include "hmi_data_def.h"

/** Function prototypes *******************************************************/
void event_init(const EVENT_DEF *data_event_list, uint8_t data_num_events);
uint8_t event_update(enum EVENT_TYPE type, const void * data, size_t data_size);
uint8_t event_update_start_timer(const void * data, size_t data_size);
void event_delayed_start(uint8_t event, uint32_t delay);
void event_delayed_stop(uint8_t event);
void event_restart_delay_timer(uint8_t event);
void event_stop_delay_timer(uint8_t event);
uint8_t event_update_delayed_event(const void * const menu_event, size_t menu_event_size);
void event_reset_idle_timers(void);
uint8_t event_get_menu_event_from_type_and_index(EVENT_TYPE type, uint8_t state_index);
void event_get_event_definition_data(enum EVENT_TYPE type, void * const data, size_t data_size);
#endif /* EVENT_H_ */
