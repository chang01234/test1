/*! \file hmi_dispatcher.c
    \brief Responsible for dispatching events to the correct module.
*/

#include "configuration.h"

#include "hmi_dispatcher.h"

#include "connector_hmi.h"

#include "ui_engine.h"
#include "input_manager.h"
#include "display_manager.h"

#if HMI_TYPE_NAME_DEBUG
const char * const hmi_module_type_id_to_name[] =
{
    [HMI_MODULE_TYPE_NONE] = "HMI_MODULE_TYPE_NONE",
    [HMI_MODULE_TYPE_UI_ENGINE] = "HMI_MODULE_TYPE_UI_ENGINE",
    [HMI_TYPE_INTERNAL_EVENTS] = "HMI_TYPE_INTERNAL_EVENTS",
    [HMI_MODULE_TYPE_DISPLAY_MANAGER] = "HMI_MODULE_TYPE_DISPLAY_MANAGER",
};

const char * const hmi_group_type_id_to_name[] =
{
    [HMI_TYPE_NONE] = "HMI_TYPE_NONE",
    [HMI_TYPE_DDMP2] = "HMI_TYPE_DDMP2",
    [HMI_TYPE_INTERNAL_EVENTS] = "HMI_TYPE_INTERNAL_EVENTS",
    [HMI_TYPE_INPUT_EVENTS] = "HMI_TYPE_INPUT_EVENTS",
    [HMI_TYPE_SCREEN_EVENTS] = "HMI_TYPE_SCREEN_EVENTS",
};

const char * const hmi_event_type_id_to_name[] =
{
    [HMI_TYPE_INTERNAL_EVENTS_IDLE_TIMER] = "HMI_TYPE_INTERNAL_EVENTS_IDLE_TIMER",
    [HMI_TYPE_INTERNAL_EVENTS_DELAYED_TIMER] = "HMI_TYPE_INTERNAL_EVENTS_DELAYED_TIMER",
    [HMI_TYPE_INTERNAL_EVENTS_START_TIMER] = "HMI_TYPE_INTERNAL_EVENTS_START_TIMER",
    [HMI_TYPE_INTERNAL_EVENTS_SCREEN_TIMER_DELAY] = "HMI_TYPE_INTERNAL_EVENTS_SCREEN_TIMER_DELAY",
    [HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD] = "HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD",
    [HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS] = "HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS",
    [HMI_TYPE_INPUT_EVENTS_BUTTON_RELEASE] = "HMI_TYPE_INPUT_EVENTS_BUTTON_RELEASE",
    [HMI_TYPE_INPUT_EVENTS_ROTARIES] = "HMI_TYPE_INPUT_EVENTS_ROTARIES",
};
#endif //HMI_TYPE_NAME_DEBUG

static void dispatch_display_manager_events(uint32_t event_id, const uint8_t * const data, size_t data_size)
{
    diplay_manager_process_event(event_id, data, data_size);
}

static void dispatch_input_manager_events(uint32_t event_id, const uint8_t * const data, size_t data_size)
{
    input_manager_process_event(event_id, data, data_size);
}

static void dispatch_ui_engine_events(uint32_t event_id, const uint8_t * const data, size_t data_size)
{
    ui_engine_process_event(event_id, data, data_size);
}


void hmi_dispatch_event_to_module(uint32_t event_id, const void * const data, size_t data_size)
{
    TRUE_CHECK_RETURN(HMI_TYPE_IS_MODULE_FIELD(event_id));

    DEBUG_MODULE_EVENT_NAME(event_id);
    switch (HMI_TYPE_MODULE_FIELD(event_id))
    {
    case HMI_MODULE_TYPE_UI_ENGINE:
        dispatch_ui_engine_events(event_id, data, data_size);
        break;
    case HMI_MODULE_TYPE_INPUT_MANAGER:
        dispatch_input_manager_events(event_id, data, data_size);
        break;
    case HMI_MODULE_TYPE_DISPLAY_MANAGER:
        dispatch_display_manager_events(event_id, data, data_size);
        break;
    case HMI_MODULE_TYPE_NONE:
    default:
        LOG(E, "HMI Module type not provided!");
        break;
    }
}

void hmi_dispatch_event_to_connector(uint32_t event_id, const void * const data, size_t data_size, uint32_t timeout)
{
    send_generic_frame_to_connector(event_id, data, data_size, timeout);
}
