/*! \file hmi_dispatcher.h
    \brief Responsible for dispatching events to the correct module.
*/

#ifndef HMI_DISPATCHER_H_
#define HMI_DISPATCHER_H_

#include <stdint.h>
#include <stddef.h>

#include "configuration.h"

#define HMI_TYPE_NAME_DEBUG      0 //keep 0 when committing

#define HMI_TYPE_EVENT_FIELD(x)                        ( (x)  & 0x000fffff )
#define HMI_TYPE_GROUP_FIELD(x)                        ( ((x) & 0x0ff00000) >> 20 )
#define HMI_TYPE_MODULE_FIELD(x)                       ( ((x) & 0xf0000000) >> 28 )
#define HMI_TYPE_IS_EVENT_FIELD(x)                     ( (HMI_TYPE_EVENT_FIELD(x)  & 0x000fffff) )
#define HMI_TYPE_IS_GROUP_FIELD(x)                     ( (HMI_TYPE_GROUP_FIELD(x)  & 0xff) )
#define HMI_TYPE_IS_MODULE_FIELD(x)                    ( (HMI_TYPE_MODULE_FIELD(x) & 0xf) )

#define HMI_TYPE_IS_INTERNAL_EVENT(x)   \
    ( ((HMI_TYPE_IS_EVENT_FIELD(x) >= HMI_TYPE_INTERNAL_EVENTS_IDLE_TIMER) && ((HMI_TYPE_IS_EVENT_FIELD(x) <= HMI_TYPE_INTERNAL_EVENTS_END))) )
#define HMI_TYPE_IS_INPUT_EVENT(x)   \
    ( ((HMI_TYPE_IS_EVENT_FIELD(x) >= HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD) && ((HMI_TYPE_IS_EVENT_FIELD(x) <= HMI_TYPE_INPUT_EVENTS_END))) )

// HMI set event withing group/module
#define HMI_TYPE_SET_MODULE_FIELD(x)                   ( (uint32_t)(((uint8_t)x) << 28) )
#define HMI_TYPE_SET_GROUP_FIELD(x)                    ( (uint32_t)(((uint8_t)x) << 20) )
#define HMI_TYPE_SET_INTERNAL_EVENT(x)                 ( HMI_TYPE_SET_GROUP_FIELD(HMI_TYPE_INTERNAL_EVENTS) | (x) )
#define HMI_TYPE_SET_INPUT_EVENT(x)                    ( HMI_TYPE_SET_GROUP_FIELD(HMI_TYPE_INPUT_EVENTS)    | (x) )
#define HMI_TYPE_SET_SCREEN_EVENT(x)                   ( HMI_TYPE_SET_GROUP_FIELD(HMI_TYPE_SCREEN_EVENTS)   | (x) )

// HMI set module + event helper macros
#define HMI_SET_MODULE_INTERNAL_EVENT(module, event)   (HMI_TYPE_SET_MODULE_FIELD(module) | HMI_TYPE_SET_INTERNAL_EVENT(event))
#define HMI_SET_MODULE_INPUT_EVENT(module, event)      (HMI_TYPE_SET_MODULE_FIELD(module) | HMI_TYPE_SET_INPUT_EVENT(event))


// HMI MODULE types
#define HMI_MODULE_TYPE_NONE                           0x00
#define HMI_MODULE_TYPE_UI_ENGINE                      0x01
#define HMI_MODULE_TYPE_INPUT_MANAGER                  0x02
#define HMI_MODULE_TYPE_DISPLAY_MANAGER                0x03

// HMI event group types
#define HMI_TYPE_NONE                                  0x00
#define HMI_TYPE_DDMP2                                 0x01
#define HMI_TYPE_INTERNAL_EVENTS                       0x02
#define HMI_TYPE_INPUT_EVENTS                          0x03
#define HMI_TYPE_SCREEN_EVENTS                         0x04

// HMI Events
#define HMI_TYPE_EVENT_NONE                            0x00
// HMI Internal event types - HMI_TYPE_INTERNAL_EVENTS
#define HMI_TYPE_INTERNAL_EVENTS_IDLE_TIMER            0x01
#define HMI_TYPE_INTERNAL_EVENTS_DELAYED_TIMER         0x02
#define HMI_TYPE_INTERNAL_EVENTS_START_TIMER           0x03
#define HMI_TYPE_INTERNAL_EVENTS_PRODUCT_UPDATE_TIMER  0x04
#define HMI_TYPE_INTERNAL_EVENTS_SCREEN_TIMER_DELAY    0x05
#define HMI_TYPE_INTERNAL_EVENTS_SCREEN_STATE_UPDATED  0x06
// #define HMI_TYPE_INTERNAL_EVENTS_XXX
#define HMI_TYPE_INTERNAL_EVENTS_END                   0x19

// HMI Input event types - HMI_TYPE_INPUT_EVENTS
#define HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD              0x20
#define HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS             0x21
#define HMI_TYPE_INPUT_EVENTS_BUTTON_RELEASE           0x22
#define HMI_TYPE_INPUT_EVENTS_ROTARIES                 0x23
#define HMI_TYPE_INPUT_EVENTS_RESET_IDLE_TIMERS        0x24
#define HMI_TYPE_INPUT_EVENTS_NTC                      0x25
// #define HMI_TYPE_INPUT_EVENTS_XXX
#define HMI_TYPE_INPUT_EVENTS_END                      0x39

// HMI Screen event types - HMI_TYPE_SCREEN_EVENTS

#if HMI_TYPE_NAME_DEBUG
extern const char * const hmi_module_type_id_to_name[];
extern const char * const hmi_group_type_id_to_name[];
extern const char * const hmi_event_type_id_to_name[];
#define DEBUG_MODULE_EVENT_NAME(x) \
    LOG(I, "Module Event ID[%s]", hmi_module_type_id_to_name[HMI_TYPE_MODULE_FIELD(event_id)]);
#define DEBUG_GROUP_EVENT_NAME(x) \
    LOG(I, "Group Event ID[%s]", hmi_group_type_id_to_name[HMI_TYPE_GROUP_FIELD(event_id)]);
#define DEBUG_EVENT_NAME(x) \
    LOG(I, "Event ID[%s]", hmi_event_type_id_to_name[HMI_TYPE_EVENT_FIELD(event_id)]);
#else //HMI_TYPE_NAME_DEBUG
#define DEBUG_MODULE_EVENT_NAME(x)
#define DEBUG_GROUP_EVENT_NAME(x)
#define DEBUG_EVENT_NAME(x)
#endif //HMI_TYPE_NAME_DEBUG


// Public functions
void hmi_dispatch_event_to_module(uint32_t event_id, const void * const data, size_t data_size);
void hmi_dispatch_event_to_connector(uint32_t event_id, const void * const data, size_t data_size, uint32_t timeout);

#endif //HMI_DISPATCHER_H_