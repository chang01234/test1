/*! \file display_manager.c
 *  \brief Renders the presentation of the model according to the screen definition.
*/
#include "configuration.h"
#include "display_manager.h"

#include "hmi_dispatcher.h"

#include "screen.h"


//!< \brief Process internal events received by Dispatcher that are supported by Input Manager
static void diplay_manager_process_internal_events(uint32_t event_id, const void * const data, size_t data_size)
{
    TRUE_CHECK_RETURN(HMI_TYPE_IS_INTERNAL_EVENT(event_id));

    DEBUG_EVENT_NAME(event_id);
    switch (HMI_TYPE_EVENT_FIELD(event_id))
    {
    case HMI_TYPE_INTERNAL_EVENTS_SCREEN_TIMER_DELAY:
        screen_timer_delay(data, data_size);
        break;
    default:
        LOG(E,"Display manager internal event type(%02X) not supported!", HMI_TYPE_EVENT_FIELD(event_id));
        break;
    }
}

//!< \brief Process events received from Dispatcher that are supported by Input Manager
void diplay_manager_process_event(uint32_t event_id, const uint8_t * const data, size_t data_size)
{
    TRUE_CHECK_RETURN(HMI_TYPE_IS_GROUP_FIELD(event_id));

    DEBUG_GROUP_EVENT_NAME(event_id);
    switch (HMI_TYPE_GROUP_FIELD(event_id))
    {
    case HMI_TYPE_INTERNAL_EVENTS:
        diplay_manager_process_internal_events(event_id, data, data_size);
        break;
    default:
        LOG(E,"Display manager group event type(%02X) not supported!", HMI_TYPE_GROUP_FIELD(event_id));
        break;
    }
}
