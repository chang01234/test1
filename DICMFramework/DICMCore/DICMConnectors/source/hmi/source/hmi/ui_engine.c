/*! \file ui_engine.c
	\brief Responsible for managing the data of the model.
    Manipulates the data according the controller output or internal(delayed) events.
*/

#include "configuration.h"

#include "ui_engine.h"

#include "connector_hmi.h"
#include "hmi_dispatcher.h"

#include "event_data.h"

// Extern declare required function to be defined in product specific repository.
extern void product_update_init(void);

// Private functions
static uint8_t ui_engine_process_ddm2_events(uint32_t event_id, const void * const data, size_t data_size);
static uint8_t ui_engine_process_internal_events(uint32_t event_id, const void * const data, size_t data_size);
static uint8_t ui_engine_process_input_events(uint32_t event_id, const void * const data, size_t data_size);

/*! \brief Initializes ui engine.
 *
 *	\param data_event_list	Pointer to event list in definition data.
 *	\param data_num_events	Number of events in definition data event list.
 */
void ui_engine_init(const EVENT_DEF * data_event_list, uint8_t data_num_events, const MENU_STATE * menu_boot_state, void(ddmp_set_cb)(uint32_t, uint8_t))
{
    event_init(data_event_list, data_num_events);

    menu_init(menu_boot_state, ddmp_set_cb);
    product_update_init();
}

//!< \brief Process ddm2 parameters changes
static uint8_t ui_engine_process_ddm2_events(uint32_t event_id, const void * const data, size_t data_size)
{
    return event_update(EVENT_TYPE_VAR_CHANGE, data, data_size);
}

//!< \brief Process internal events received by Dispatcher that are supported by UI Engine
static uint8_t ui_engine_process_internal_events(uint32_t event_id, const void * const data, size_t data_size)
{
    TRUE_CHECK_RETURN0(HMI_TYPE_IS_INTERNAL_EVENT(event_id));

    DEBUG_EVENT_NAME(event_id);
    switch (HMI_TYPE_EVENT_FIELD(event_id))
    {
    case HMI_TYPE_INTERNAL_EVENTS_IDLE_TIMER:
        return event_update(EVENT_TYPE_IDLE, data, data_size);
    case HMI_TYPE_INTERNAL_EVENTS_DELAYED_TIMER:
        return event_update_delayed_event(data, data_size);
    case HMI_TYPE_INTERNAL_EVENTS_START_TIMER:
        return event_update(EVENT_TYPE_DELAY, data, data_size);
    case HMI_TYPE_INTERNAL_EVENTS_PRODUCT_UPDATE_TIMER:
        hmi_product_update_task();
        break;
    default:
        LOG(E,"UI Engine internal event type(%02X) not supported!", HMI_TYPE_EVENT_FIELD(event_id));
        break;
    }

    return MENU_EVENT_NONE;
}

//!< \brief Process input events received from InputManager that are supported by UI Engine
static uint8_t ui_engine_process_input_events(uint32_t event_id, const void * const data, size_t data_size)
{
    TRUE_CHECK_RETURN0(HMI_TYPE_IS_INPUT_EVENT(event_id));

    DEBUG_EVENT_NAME(event_id);
    switch (HMI_TYPE_EVENT_FIELD(event_id))
    {
    case HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD:
        return event_update(EVENT_TYPE_KEY_HOLD, data, data_size);
    case HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS:
        return event_update(EVENT_TYPE_KEY_PRESS, data, data_size);
    case HMI_TYPE_INPUT_EVENTS_BUTTON_RELEASE:
        return event_update(EVENT_TYPE_KEY_RELEASE, data, data_size);
    case HMI_TYPE_INPUT_EVENTS_ROTARIES:
        return event_update(EVENT_TYPE_ROTATION, data, data_size);
    case HMI_TYPE_INPUT_EVENTS_RESET_IDLE_TIMERS:
        event_reset_idle_timers();
        break;
    default:
        LOG(E,"UI Engine input event type(%02X) not supported!", HMI_TYPE_EVENT_FIELD(event_id));
        break;
    }

    return MENU_EVENT_NONE;
}

//!< \brief Get model's definition data for the input events that is be used by the InputManager
void ui_engine_get_event_definition_data(uint32_t event_id, void * const data, size_t data_size)
{
    switch (HMI_TYPE_EVENT_FIELD(event_id))
    {
        case HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD:
            event_get_event_definition_data(EVENT_TYPE_KEY_HOLD, data, data_size);
            break;
        case HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS:
            event_get_event_definition_data(EVENT_TYPE_KEY_PRESS, data, data_size);
            break;
        case HMI_TYPE_INPUT_EVENTS_BUTTON_RELEASE:
            event_get_event_definition_data(EVENT_TYPE_KEY_RELEASE, data, data_size);
            break;
        case HMI_TYPE_INPUT_EVENTS_ROTARIES:
            event_get_event_definition_data(EVENT_TYPE_ROTATION, data, data_size);
            break;
        default:
            LOG(E, "UI Engine get event definition type(%02X) not supported!", HMI_TYPE_EVENT_FIELD(event_id));
            break;
    }
}

//!< \brief Process events received from Dispatcher or InputManager that are supported by UI Engine
void ui_engine_process_event(uint32_t event_id, const void * const data, size_t data_size)
{
    uint8_t menu_event = MENU_EVENT_NONE;

    TRUE_CHECK_RETURN(HMI_TYPE_IS_GROUP_FIELD(event_id));

    DEBUG_GROUP_EVENT_NAME(event_id);
    switch (HMI_TYPE_GROUP_FIELD(event_id))
    {
    case HMI_TYPE_DDMP2:
        menu_event = ui_engine_process_ddm2_events(event_id, data, data_size);
        break;
    case HMI_TYPE_INPUT_EVENTS:
        menu_event = ui_engine_process_input_events(event_id, data, data_size);
        break;
    case HMI_TYPE_INTERNAL_EVENTS:
        menu_event = ui_engine_process_internal_events(event_id, data, data_size);
        break;
    default:
        LOG(E,"UI Engine group event type(%02X) not supported!", HMI_TYPE_GROUP_FIELD(event_id));
        return;
    }

    menu_update(menu_event);
}
