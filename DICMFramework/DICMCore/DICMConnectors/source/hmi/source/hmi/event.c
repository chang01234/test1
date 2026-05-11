/*! \file event.c
 *	\brief Generating menu events based on predefined time period or
 *	input events received by the input manager
 *
 *	Some events rely on the user not providing any inputs for a specified period
 *	of time. This is tracked by the idle_ticks timer. input_manager module will
 *	provide reset idle timers event on any user input. Other code modules are also
 *	allowed to reset the idle timers when needed by calling	event_reset_idle_timers().
 *
 *	This module also provides event_delayed_start() and event_delayed_stop() to
 *	be used when other code modules needs to trigger a menu action.
 *
 *	Module state initialization is handled by event_init() which must be called
 *	between boot and any input device changes.
 *
 *	Event definitions consist of a menu event, a state index, a type, and a set
 *	of type-specific data. The type is used to select an event handler which
 *	then checks if the event is valid based on the event data. Each call to
 *	event_update() will return the menu_event specified by the highest priority
 *	valid input event. This usually puts the event in a triggered state meaning
 *	it will not trigger again until further user input has occurred. State index
 *	is used to store runtime data for each event (if necessary) and must be
 *	unique for each event of a specific type (Events of differing types may have
 *	the same state index).
 *
 *	Events priority is determined by the order in which they are updated.
 *
 *	For details on each event type see the associated handler and data structure
 *	definitions.
 */

/** Includes ******************************************************************/
#include "configuration.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "freertos/timers.h"
#include "esp_adc_cal.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "event.h"
#include "varstate.h"
#include "screen.h"

#include "connector.h"
#include "hmi_dispatcher.h"
#include "input_manager.h"

/** Defines *******************************************************************/
#define MAX_DELAYED_EVENTS		10		//!< \~ Maximum number of delayed events that can run simultaneously.
#define MS_TO_TICKS(delay)		((pdMS_TO_TICKS(delay)) != 0 ? (pdMS_TO_TICKS(delay)) : 1)

/** Variables *****************************************************************/
static const EVENT_DEF * event_list;	//!< \~ Pointer to event list.
static uint8_t num_events;				//!< \~ Number of events in event list.

typedef struct _EVENT_TIMER
{
	TimerHandle_t timer;			//!< \~ Idle timer
	StaticTimer_t timer_buffer;		//!< \~ Timer's state
} EVENT_TIMER;

//! \~ Holds runtime state for an idle event.
typedef struct IDLE_STATE
{
	EVENT_TIMER idle_timer;			//!< \~ Idle timer
} IDLE_STATE;

//! \~ Holds runtime state for a delayed event.
typedef struct DELAYED_EVENT_STATE
{
	EVENT_TIMER delayed_timer;		//!< \~ Delayed timer
} DELAYED_EVENT_STATE;

//! \~ Holds runtime state for a delayed menu event.
typedef struct DELAYED_MENU_EVENT_STATE
{
	uint8_t menu_event;				//!< \~ Menu event to trigger.
	EVENT_TIMER delayed_timer;		//!< \~ Delayed timer
} DELAYED_MENU_EVENT_STATE;

static IDLE_STATE idle_events[EVENT_MAX_IDLE];
static DELAYED_EVENT_STATE delay_event_states[EVENT_MAX_DELAYED_EVENTS];
static DELAYED_MENU_EVENT_STATE delayed_menu_events[EVENT_MAX_DELAYED_MENU_EVENTS];

#ifdef HMI_EVENT_DEBUG_NAMES
static const char * const event_type_names[] =
{
	[EVENT_TYPE_KEY_PRESS] = "EVENT_TYPE_KEY_PRESS",
	[EVENT_TYPE_KEY_RELEASE] = "EVENT_TYPE_KEY_RELEASE",
	[EVENT_TYPE_KEY_HOLD] = "EVENT_TYPE_KEY_HOLD",
	[EVENT_TYPE_ROTATION] = "EVENT_TYPE_ROTATION",
	[EVENT_TYPE_VAR_CHANGE] = "EVENT_TYPE_VAR_CHANGE",
	[EVENT_TYPE_STATE_ENTRY] = "EVENT_TYPE_STATE_ENTRY",
	[EVENT_TYPE_STATE_EXIT] = "EVENT_TYPE_STATE_EXIT",
	[EVENT_TYPE_IDLE] = "EVENT_TYPE_IDLE",
	[EVENT_TYPE_TRIGGERED] = "EVENT_TYPE_TRIGGERED",
	[EVENT_TYPE_DELAY] = "EVENT_TYPE_DELAY",
};

static const char * event_type_name(const EVENT_TYPE type);
#endif //HMI_EVENT_DEBUG_NAMES

/** Private prototypes ********************************************************/
static uint8_t update_var_change(const EVENT_DEF *event);

static void idle_timers_callback(TimerHandle_t xTimer);
static void delayed_timers_callback(TimerHandle_t xTimer);
static void delay_timers_callback(TimerHandle_t xTimer);

static void start_idle_timer(const EVENT_DEF * event);
static void start_delay_timer(const EVENT_DEF * event, uint8_t menu_event);

static uint8_t get_menu_event_from_event_state_index(const EVENT_DEF * event, uint8_t state_index);
static void get_event_buttons_definition(const EVENT_TYPE type, void * data, size_t data_size);

#ifdef HMI_EVENT_DEBUG_NAMES
/*! \brief Converts event type id to string
	\param type event type id
	\return String of event type
 */
static const char * event_type_name(const EVENT_TYPE type)
{
	if ((type >= EVENT_TYPE_KEY_PRESS) || (type < EVENT_TYPE_DELAY))
		return event_type_names[type];
	else
		return "Unknown";
}
#endif //HMI_EVENT_DEBUG_NAMES

static void idle_timers_callback(TimerHandle_t xTimer)
{
	uint8_t event_state_index = *(uint8_t *)pvTimerGetTimerID(xTimer);
	hmi_dispatch_event_to_connector(HMI_SET_MODULE_INTERNAL_EVENT(HMI_MODULE_TYPE_UI_ENGINE, HMI_TYPE_INTERNAL_EVENTS_IDLE_TIMER), &event_state_index, sizeof(event_state_index), portMAX_DELAY);
}

static void delay_timers_callback(TimerHandle_t xTimer)
{
	uint8_t event_state_index = *(uint8_t *)pvTimerGetTimerID(xTimer);
	hmi_dispatch_event_to_connector(HMI_SET_MODULE_INTERNAL_EVENT(HMI_MODULE_TYPE_UI_ENGINE, HMI_TYPE_INTERNAL_EVENTS_START_TIMER), &event_state_index, sizeof(event_state_index), portMAX_DELAY);
}

static void delayed_timers_callback(TimerHandle_t xTimer)
{
	uint8_t menu_event = *(uint8_t *)pvTimerGetTimerID(xTimer);
	hmi_dispatch_event_to_connector(HMI_SET_MODULE_INTERNAL_EVENT(HMI_MODULE_TYPE_UI_ENGINE, HMI_TYPE_INTERNAL_EVENTS_DELAYED_TIMER), &menu_event, sizeof(menu_event), portMAX_DELAY);
}

/*! \brief Initializes event generator.
 *
 *	\param data_event_list	Pointer to event list in definition data (event_data.c).
 *	\param data_num_events	Number of events in definition data event list.
 */
void event_init(const EVENT_DEF * data_event_list, uint8_t data_num_events)
{
	event_list = data_event_list;
	num_events = data_num_events;

	ZERO(idle_events);
	ZERO(delay_event_states);
	ZERO(delayed_menu_events);

	event_reset_idle_timers();
}

/*! \brief Get correct menu event for the event
 *	definition with the corresponding state_index
 *
 *	\param event		Pointer to an event listed in event_list
 *	\param state_index	Unique index for each event of a specific type
 *
 *	\return		Menu event to trigger defined by the \a event, if the \a event's state index corresponds
 *				with \a state_index of interest. Otherwise MENU_EVENT_NONE is returned.
 */
static uint8_t get_menu_event_from_event_state_index(const EVENT_DEF * event, uint8_t state_index)
{
	uint8_t menu_event = MENU_EVENT_NONE;

	if (event->state_index == state_index)
	{
#ifdef HMI_EVENT_DEBUG_NAMES
		LOG(D, "%s: menu_event: %s (%d)", event_type_name(event->type), (const char *)HMI_DATA_ADDRESS(event->name), event->menu_event);
#endif //HMI_EVENT_DEBUG_NAMES
		menu_event = event->menu_event;
	}

	return menu_event;
}

//! \brief Updates delayed menu event when HMI_TYPE_INTERNAL_EVENTS_DELAYED_TIMER event is generated
uint8_t event_update_delayed_event(const void * const menu_event, size_t menu_event_size)
{
	TRUE_CHECK_RETURN0(menu_event);
	TRUE_CHECK_RETURN0(menu_event_size == sizeof(uint8_t));

	event_delayed_stop(*(uint8_t *)menu_event);
	return *(uint8_t *)menu_event;
}

//! \brief Populates input_manager's data structres for input events defined within event_data.c
static void get_event_buttons_definition(const EVENT_TYPE type, void * data, size_t data_size)
{
	for (size_t i = 0; i < num_events; ++i)
	{
		const EVENT_DEF * event = &event_list[i];

		if (event->type == type)
		{
			switch(event->type)
			{
			case EVENT_TYPE_KEY_PRESS:
			{
				TRUE_CHECK_RETURN(sizeof(KEY_PRESS_EVENT_DEF) == data_size);

				KEY_PRESS_EVENT_DEF * key_press_def = (KEY_PRESS_EVENT_DEF *)data;
				uint8_t n_events = key_press_def->num_key_press_events;
				TRUE_CHECK_RETURN(n_events < EVENT_MAX_KEY_PRESS);

				key_press_def->key_press_event[n_events].key_press_data.state_index = event->state_index;
				key_press_def->key_press_event[n_events].key_press_data.key_press = &event->key_press;

				key_press_def->num_key_press_events++;
				break;
			}
			case EVENT_TYPE_KEY_RELEASE:
			{
				TRUE_CHECK_RETURN(sizeof(KEY_RELEASE_EVENT_DEF) == data_size);

				KEY_RELEASE_EVENT_DEF * key_release_def = (KEY_RELEASE_EVENT_DEF *)data;
				uint8_t n_events = key_release_def->num_key_release_events;
				TRUE_CHECK_RETURN(n_events < EVENT_MAX_KEY_RELEASE);

				key_release_def->key_release_event[n_events].key_release_data.state_index = event->state_index;
				key_release_def->key_release_event[n_events].key_release_data.key_release = &event->key_press;

				key_release_def->num_key_release_events++;
				break;
			}
			case EVENT_TYPE_KEY_HOLD:
			{
				TRUE_CHECK_RETURN(sizeof(KEY_HOLD_EVENT_DEF) == data_size);

				KEY_HOLD_EVENT_DEF * key_hold_def = (KEY_HOLD_EVENT_DEF *)data;
				uint8_t n_events = key_hold_def->num_key_hold_events;
				TRUE_CHECK_RETURN(n_events < EVENT_MAX_KEY_HOLD);

				key_hold_def->key_hold_event[n_events].key_hold_data.state_index = event->state_index;
				key_hold_def->key_hold_event[n_events].key_hold_data.key_hold = &event->key_hold;

				key_hold_def->num_key_hold_events++;
				break;
			}	
			case EVENT_TYPE_ROTATION:
			{
				TRUE_CHECK_RETURN(sizeof(ROTARY_EVENT_DEF) == data_size);

				EVENT_ROTARY_INDEX rotary = event->rotation.encoder;
				ROTARY_EVENT_DEF * rotary_def = (ROTARY_EVENT_DEF *)data;
				TRUE_CHECK_RETURN(rotary < EVENT_NUM_ROTARIES);

				ROTARY_DATA * rotary_data = NULL;
				if (event->rotation.steps > 0)
				{
					rotary_data = &rotary_def[rotary].rotary_data[ROTARY_DIRECTION_CW];
				}
				else if (event->rotation.steps < 0)
				{
					rotary_data = &rotary_def[rotary].rotary_data[ROTARY_DIRECTION_CCW];
				}

				TRUE_CHECK_RETURN(rotary_data);
				rotary_data->state_index = event->state_index;
				rotary_data->rotation = &event->rotation;

				break;
			}
			default:
				assert(0);	// Event type does not have buttons.
			}
		}
	}
}

/*! \brief Populates input_manager's data structres for input events defined within event_data.c
*
*	\param type			Type of the event that \a data should be populated with
*	\param data			Buffer that should be populated with the corresponding event's data for each event
*						of type \a type defined in event_data.c
*	\param data_size	Size of \a data buffer
*
*	\pre		Parameter \a data must be a non-NULL pointer.
*	\pre		Parameter \a data_size must note be 0.
*/
void event_get_event_definition_data(enum EVENT_TYPE type, void * const data, size_t data_size)
{
	TRUE_CHECK_RETURN(data);
	TRUE_CHECK_RETURN(data_size);

	switch (type)
	{
	case EVENT_TYPE_KEY_PRESS:
		get_event_buttons_definition(EVENT_TYPE_KEY_PRESS, data, data_size);
		break;
	case EVENT_TYPE_KEY_RELEASE:
		get_event_buttons_definition(EVENT_TYPE_KEY_RELEASE, data, data_size);
		break;
	case EVENT_TYPE_KEY_HOLD:
		get_event_buttons_definition(EVENT_TYPE_KEY_HOLD, data, data_size);
		break;
	case EVENT_TYPE_ROTATION:
		get_event_buttons_definition(EVENT_TYPE_ROTATION, data, data_size);
		break;
	default:
		LOG(E, "Get event definition data type(%02X) not supported!", type);
		break;
	}
}

/*! \brief Updates event generator state. Returns highest priority pending menu
 *	event or MENU_EVENT_NONE if no pending menu events.
 *
 *	Returns the menu event associated with whichever was the highest priority valid
 *	delayed or input event. Returns 0 (MENU_EVENT_NONE) if no valid event was found.
 *
 *	Note that this function assumes that the menu event returned will be acted
 *	upon (or intentionally ignored). It will only ever return a certain menu
 *	event once for every triggering of the associated input event.
 *
 *	\param type			Type of the event that should trigger associated menu event
 *	\param data			Keeps additional data used for determinig the correct event definition.
 *						It can hold the state_index, used as unique index for each event of a specific
 *						type, or it can be NULL if update of variable is detected when DDM2 parameter has
 *						been published.
 *	\param data_size	Size of \a data buffer.
 *
 *	\pre		Parameter \a data might be a non-NULL pointer.
 *	\pre		Parameter \a data_size must not be 0, if \a data is a non-NULL pointer.
 *
 *	\returns	Highest priority valid menu event or MENU_EVENT_NONE if no valid menu events.
 */
uint8_t event_update(enum EVENT_TYPE type, const void * data, size_t data_size)
{
	uint8_t menu_event = MENU_EVENT_NONE;

	for (size_t i = 0; i < num_events; ++i)
	{
		const EVENT_DEF * event = &event_list[i];

		if (menu_event != MENU_EVENT_NONE)
		{
			break;
		}

		if (event->type == type)
		{
			switch (event->type)
			{
			case EVENT_TYPE_KEY_PRESS:
			case EVENT_TYPE_KEY_RELEASE:
			case EVENT_TYPE_KEY_HOLD:
			case EVENT_TYPE_ROTATION:
			case EVENT_TYPE_DELAY:
			case EVENT_TYPE_IDLE:
			{
				TRUE_CHECK_RETURN0(data);
				TRUE_CHECK_RETURN0(data_size == sizeof(event->state_index));

				uint8_t state_index = *(uint8_t *)data;
				menu_event = get_menu_event_from_event_state_index(event, state_index);
				break;
			}
			case EVENT_TYPE_VAR_CHANGE:
			{
				menu_event = update_var_change(event);
				break;
			}
			case EVENT_TYPE_STATE_ENTRY:
				// Do nothing. Handled at state_change()
				break;
			case EVENT_TYPE_STATE_EXIT:
				// Do nothing. Handled at state_change()
				break;
			case EVENT_TYPE_TRIGGERED:
				break;
			default:
				LOG(E, "%d", event->type);
				assert(0); // Invalid event type.
				break;
			}
		}
	}

	return menu_event;
}

//! \brief Get menu event for the event type assosicated with the correct state_index
uint8_t event_get_menu_event_from_type_and_index(EVENT_TYPE type, uint8_t state_index)
{
	uint8_t menu_event = 0;

	// Check and update events in order of priority.
	for (size_t i = 0; i < num_events; ++i)
	{
		const EVENT_DEF *event = &event_list[i];

		if ((event->type == type) && (event->state_index == state_index))
		{
			menu_event = event->menu_event;
			break;
		}
	}

	return menu_event;
}

/*! \brief Waits for delay sys ticks and then makes next call to event_update()
 * 	return menu_event.
 *
 * 	Delay may not be set to 0. Smallest possible delay is 1 tick.
 *
 * 	\param menu_event	Index of menu event to trigger.
 * 	\param delay		Number of milliseconds after which to trigger delay.
 */
void event_delayed_start(uint8_t menu_event, uint32_t delay)
{
	assert(delay != 0); // Delay must not be 0.

	for (size_t i = 0; i < MAX_DELAYED_EVENTS; ++i)
	{
		// Free timer's slot
		if ((delayed_menu_events[i].delayed_timer.timer == NULL) && (delayed_menu_events[i].menu_event == 0))
		{
			size_t delay_ticks = MS_TO_TICKS(delay);
			TimerHandle_t * delayed_timer = &delayed_menu_events[i].delayed_timer.timer;
			StaticTimer_t * delayed_timer_buffer = &delayed_menu_events[i].delayed_timer.timer_buffer;
			delayed_menu_events[i].menu_event = menu_event;

			TRUE_CHECK_RETURN(*delayed_timer = xTimerCreateStatic(NULL, delay_ticks, false, &delayed_menu_events[i].menu_event, delayed_timers_callback, delayed_timer_buffer));
			TRUE_CHECK_RETURN(xTimerStart(*delayed_timer, portMAX_DELAY));

			return;
		}
	}

	assert(0);	// Too many delayed events.
}

/*! \brief Removes any running delay for menu_event. Does nothing if no delay
 * 	for menu_event is running.
 *
 * 	\param menu_event	Index of delayed menu event to cancel.
 */
void event_delayed_stop(uint8_t menu_event)
{
	for (size_t i = 0; i < MAX_DELAYED_EVENTS; ++i)
	{
		if (delayed_menu_events[i].menu_event == menu_event)
		{
			TRUE_CHECK_RETURN(delayed_menu_events[i].delayed_timer.timer);
			TRUE_CHECK_RETURN(xTimerStop(delayed_menu_events[i].delayed_timer.timer, portMAX_DELAY));
			TRUE_CHECK_RETURN(xTimerDelete(delayed_menu_events[i].delayed_timer.timer, portMAX_DELAY));

			delayed_menu_events[i].menu_event = 0;
			delayed_menu_events[i].delayed_timer.timer = NULL;

			break;
		}
	}
}

/*! \brief Starts/restarts a timer delay for menu_event.
 *
 * 	\param menu_event	Index of delay timer menu event to start/restart.
 */
void event_restart_delay_timer(uint8_t menu_event)
{
	// Check and update events in order of priority.
	for (size_t i = 0; i < num_events; ++i)
	{
		const EVENT_DEF * event = &event_list[i];

		switch (event->type)
		{
			case EVENT_TYPE_DELAY:
				start_delay_timer(event, menu_event);
				break;
			default:
				break;
		}
	}
}

static void start_delay_timer(const EVENT_DEF * event, uint8_t menu_event)
{
	if ((event->type == EVENT_TYPE_DELAY) && (event->menu_event == menu_event))
	{
		uint32_t delay = 0; // in ticks
		bool repeat = event->delay_event.repeat;
		TimerHandle_t * delayed_timer = &delay_event_states[event->state_index].delayed_timer.timer;

		// if last bit is set, we treat this as a variable reference, leaving UINT32_MAX/2 as max for a delay
		if (event->delay_event.var.is_var)
		{
			uint32_t local_var = event->delay_event.var.index;
			delay = MS_TO_TICKS(varstate_get(local_var) * 1000);	// local_var in seconds
#ifdef HMI_EVENT_DEBUG_NAMES
			LOG(D, "Arm DELAY_EVENT: menu_event: %s (%d), varstate: %d, ticks: %d", (const char *)HMI_DATA_ADDRESS(event->name), menu_event, local_var, delay);
#endif
		}
		else
		{
			delay = MS_TO_TICKS(event->delay_event.delay);
#ifdef HMI_EVENT_DEBUG_NAMES
			LOG(D, "Arm DELAY_EVENT: menu_event: %s (%d), ticks: %d", (const char *)HMI_DATA_ADDRESS(event->name), menu_event, delay);
#endif
		}

		// Create delay timer, if it is not already created
		if (*delayed_timer == NULL)
		{
			uint8_t * state_index = (uint8_t *)&event->state_index;
			TRUE_CHECK_RETURN(*delayed_timer = xTimerCreateStatic(NULL, delay, repeat, state_index, delay_timers_callback, &delay_event_states[event->state_index].delayed_timer.timer_buffer));
		}

		// If the timer is in dormant state then xTimerReset() == xTimerStart() API function.
		TRUE_CHECK_RETURN(xTimerReset(*delayed_timer, portMAX_DELAY));
	}
}

/*! \brief Stops a timer delay for menu_event.
 *
 * 	\param menu_event	Index of delay timer menu event to cancel.
 */
void event_stop_delay_timer(uint8_t menu_event)
{
	// Check and update events in order of priority.
	for (size_t i = 0; i < num_events; ++i)
	{
		const EVENT_DEF *event = &event_list[i];

		if ((event->type == EVENT_TYPE_DELAY) && (event->menu_event == menu_event))
		{
			TimerHandle_t delayed_timer = delay_event_states[event->state_index].delayed_timer.timer;
#ifdef HMI_EVENT_DEBUG_NAMES
			LOG(D, "Stop DELAY_EVENT: menu_event: %s (%d), ticks: %d", (const char *)HMI_DATA_ADDRESS(event->name), menu_event, 0);
#endif
			TRUE_CHECK_RETURN(xTimerStop(delayed_timer, portMAX_DELAY));
			break;
		}
	}
}

/*! \brief Updates idle event state based on menu_event.
 *
 * 	Idle events trigger if user makes no inputs for the specified period of time
 * 	Each idle event will trigger at most once for each period of no user
 * 	activity.
 *
 * 	If multiple idle events would trigger in the same event_update() only the
 * 	highest priority event will trigger. lower priority events will trigger on
 * 	subsequent calls to event_update().
 *
 * 	Note that the idle timers are reset on user input but may also be reset by
 * 	calling event_reset_idle_timers().
 *
 * 	\param	event		Pointer to idle event whose state should be updated.
 *
 */
static void start_idle_timer(const EVENT_DEF * event)
{
	uint32_t delay = 0; // in ticks
	TimerHandle_t * idle_timer = &idle_events[event->state_index].idle_timer.timer;

	// if last bit is set, we treat this as a variable reference, leaving UINT32_MAX/2 as max for a delay
	if (event->idle_event.var.is_var)
	{
		uint32_t local_var = event->idle_event.var.index;
		delay = MS_TO_TICKS(varstate_get(local_var) * 1000);
#ifdef HMI_EVENT_DEBUG_NAMES
		LOG(D, "Arm IDLE_EVENT: menu_event: %s (%d), varstate: %d, ticks: %d", (const char *)HMI_DATA_ADDRESS(event->name), event->menu_event, local_var, delay);
#endif
	}
	else
	{
		delay = MS_TO_TICKS(event->idle_event.delay);
#ifdef HMI_EVENT_DEBUG_NAMES
		LOG(D, "Arm IDLE_EVENT: menu_event: %s (%d), ticks: %d", (const char *)HMI_DATA_ADDRESS(event->name), event->menu_event, delay);
#endif
	}

	// Create idle timer, if it is not already created
	if (*idle_timer == NULL)
	{
		uint8_t * state_index = (uint8_t *)&event->state_index;
		TRUE_CHECK_RETURN(*idle_timer = xTimerCreateStatic(NULL, delay, false, state_index, idle_timers_callback, &idle_events[event->state_index].idle_timer.timer_buffer));
	}

	// If the timer is in dormant state then xTimerReset() == xTimerStart() API function.
	TRUE_CHECK_RETURN(xTimerReset(*idle_timer, portMAX_DELAY));
}

//! \brief Resets idle timers.
void event_reset_idle_timers(void)
{
	// Check and update idle events
	for (size_t i = 0; i < num_events; ++i)
	{
		const EVENT_DEF *event = &event_list[i];

		switch (event->type)
		{
			case EVENT_TYPE_IDLE:
				start_idle_timer(event);
				break;
			default:
				break;
		}
	}
}

static uint8_t update_var_change(const EVENT_DEF * event)
{
	uint8_t menu_event = MENU_EVENT_NONE;

	for (int var = 0; var < event->var_change.num_vars; var++)
	{
		if (varstate_get_changed(event->var_change.vars[var]))
		{
			varstate_set_changed(event->var_change.vars[var], false);
			menu_event = event->menu_event;
#ifdef HMI_EVENT_DEBUG_NAMES
			LOG(D, "%s: menu_event: %s (%d)", event_type_name(event->type), (const char *)HMI_DATA_ADDRESS(event->name), menu_event);
#endif
		}
	}

	return menu_event;
}

