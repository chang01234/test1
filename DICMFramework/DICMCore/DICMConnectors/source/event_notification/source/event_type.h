/**
 * \file        event_type.h
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event type structure interface
 *
 * \li          2024-07-05  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef EVENT_TYPE_H_
#define EVENT_TYPE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "connector_event_notification_defaults.h"

#define EVENT_TYPE_DATA_IS_TRIGGER   0x1
#define EVENT_TYPE_DATA_IS_PARAMETER 0x2
#define EVENT_TYPE_MAX_DATA_ELEMENTS CONNECTOR_EVENT_NOTIFICATION_MAX_DATA_IN_EVENT_RECORD

typedef enum event_type_data_type
{
    EVENT_TYPE_DATA_TYPE_TRIGGER = EVENT_TYPE_DATA_IS_TRIGGER,
    EVENT_TYPE_DATA_TYPE_PARAMETER = EVENT_TYPE_DATA_IS_PARAMETER,
} event_type_data_type_t;

/**
 * @brief       Event type configuration
 *
 * It seems that in specification Event types can be disabled (whatever that
 * meant), can be local and on cloud.
 */
typedef enum event_type_config
{
    EVENT_TYPE_CONFIG_DISABLED = 0x0,
    EVENT_TYPE_CONFIG_LOCAL = (0x1 << 0),
    EVENT_TYPE_CONFIG_CLOUD = (0x1 << 1),
    EVENT_TYPE_CONFIG_LOCAL_CLOUD = (EVENT_TYPE_CONFIG_LOCAL | EVENT_TYPE_CONFIG_CLOUD)
} event_type_config_t;

/**
 * @brief   Event type structure
 *
 * This structure represents one type of events that can occur within the system.
 *
 * After parsing NVS stored JSON file the information is stored in event_type
 * structures to conserve memory and allow easy data manipulation.
 */
typedef struct event_type
{
    int32_t type;                //!< Global definition
    event_type_config_t config;  //!< 0 - disable, 1 - local, 2 - cloud, 3 - both local and cloud
    size_t data_size;            //!< Event data number of data entries (DDM parameters, trigger data, etc).
    struct event_type_data
    {
        event_type_data_type_t data_type;
        union
        {
            struct event_type_trigger_data
            {
                uint32_t trigger_id;
                size_t field_size;
                size_t field_offset;
            } trigger;
            struct event_type_parameter_data
            {
                uint32_t parameter_id;
                size_t field_size;
                size_t field_offset;
            } parameter;
        };
    } data[EVENT_TYPE_MAX_DATA_ELEMENTS];
} event_type_t;

/**
 * @brief       Initialize an event type structure.
 *
 * When created the type is initialized to empty values. Using other
 * `event_type_set_xxx` function other fields are defined. By default, event
 * type is initialized with the following member values:
 * - type is set to zero
 * - config is set to @ref EVENT_TYPE_CONFIG_DISABLED
 * - no data elements are defined.
 *
 * @param       et is a pointer to event_type instance that needs to be
 *              initialized.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
void event_type_init(event_type_t *et);

/**
 * @brief       Terminate an previosly initialized event_type structure.
 *
 * @param       et is a pointer to event_type instance that was previosly
 *              initialized.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
void event_type_terminate(event_type_t *et);

/**
 * @brief       Set type of an event type instance.
 *
 * @param       et is a pointer to event_type instance.
 * @param       type is a new event type value.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline void event_type_set_type(event_type_t *et, uint32_t type)
{
    et->type = type;
}

/**
 * @brief       Get type of an event type instance.
 *
 * @param       et is a pointer to event_type instance.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline int32_t event_type_get_type(const event_type_t *et)
{
    return et->type;
}

/**
 * @brief       Set configuration of an event type instance.
 *
 * @param       et is a pointer to event_type instance.
 * @param       config is a new event config value. Event type config can be
 *              @ref EVENT_TYPE_CONFIG_DISABLED, @ref EVENT_TYPE_CONFIG_LOCAL
 *              and @ref EVENT_TYPE_CONFIG_LOCAL_CLOUD. See
 *              @ref event_type_config_t.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline void event_type_set_config(event_type_t *et, event_type_config_t config)
{
    et->config = config;
}

/**
 * @brief       Is event_type disabled?
 *
 * @param       et is a pointer to event_type instance.
 * @return      Event type configuration.
 * @retval      true if event type is disabled.
 * @retval      false if event type is enabled.
 */
inline bool event_type_is_disabled(const event_type_t *et)
{
    return et->config == EVENT_TYPE_CONFIG_DISABLED;
}

/**
 * @brief       Is event_type local?
 *
 * @param       et is a pointer to event_type instance.
 * @return      Event type configuration.
 * @retval      true if event type is local.
 * @retval      false if event type is not local (cloud only or disabled).
 */
inline bool event_type_is_local(const event_type_t *et)
{
    return (et->config == EVENT_TYPE_CONFIG_LOCAL) || (et->config == EVENT_TYPE_CONFIG_LOCAL_CLOUD);
}

/**
 * @brief       Is event_type cloud?
 *
 * @param       et is a pointer to event_type instance.
 * @return      Event type configuration.
 * @retval      true if event type is cloud.
 * @retval      false if event type is not cloud (local only or disabled).
 */
inline bool event_type_is_cloud(const event_type_t *et)
{
    return (et->config == EVENT_TYPE_CONFIG_CLOUD) || (et->config == EVENT_TYPE_CONFIG_LOCAL_CLOUD);
}

/**
 * @brief       Add DDM parameters to an event type instance.
 *
 * @param       et is a pointer to event_type instance.
 * @param       ddm_parameter is a new DDM parameter ID.
 * @param       field_offset is offset of the parameter field in DDM data structure.
 * @param       field_size is size of the parameter field in DDM data structure.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 * @pre         Number of added parameters must not exceede number of parameters
 *              defined by @ref EVENT_TYPE_MAX_DATA_ELEMENTS.
 */
inline void event_type_add_parameter_data(event_type_t *et, uint32_t ddm_parameter, size_t field_offset, size_t field_size)
{
    et->data[et->data_size].data_type = EVENT_TYPE_DATA_TYPE_PARAMETER;
    et->data[et->data_size].parameter.parameter_id = ddm_parameter;
    et->data[et->data_size].parameter.field_offset = field_offset;
    et->data[et->data_size].parameter.field_size = field_size;
    et->data_size++;
}

/**
 * @brief       Add trigger to an event type instance.
 *
 * @param       et is a pointer to event_type instance.
 * @param       trigger_id is a new trigger ID.
 * @param       field_offset is offset of the trigger field in trigger data structure.
 * @param       field_size is size of the trigger field in trigger data structure.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 * @pre         Number of added parameters must not exceede number of parameters
 *              defined by @ref EVENT_TYPE_MAX_DATA_ELEMENTS.
 */
inline void event_type_add_trigger_data(event_type_t *et, uint32_t trigger_id, size_t field_offset, size_t field_size)
{
    et->data[et->data_size].data_type = EVENT_TYPE_DATA_TYPE_TRIGGER;
    et->data[et->data_size].trigger.trigger_id = trigger_id;
    et->data[et->data_size].trigger.field_offset = field_offset;
    et->data[et->data_size].trigger.field_size = field_size;
    et->data_size++;
}

/**
 * @brief       Get how many data elements does this event type instance define.
 *
 * @param       et is a pointer to event_type instance.
 *
 * @return      Data element number defined in the event type instance.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline size_t event_type_get_data_size(const event_type_t *et)
{
    return et->data_size;
}

/**
 * @brief       Is data element at given index a DDM parameter?
 *
 * @param       et is a pointer to event_type instance.
 * @param       index is the index of data element to be checked.
 *
 * @return      true if data element at given index is a DDM parameter,
 *              false otherwise.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline bool event_type_is_data_parameter(const event_type_t *et, size_t index)
{
    return et->data[index].data_type == EVENT_TYPE_DATA_TYPE_PARAMETER;
}

/**
 * @brief       Is data element at given index a trigger?
 *
 * @param       et is a pointer to event_type instance.
 * @param       index is the index of data element to be checked.
 *
 * @return      true if data element at given index is a trigger,
 *              false otherwise.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline bool event_type_is_data_trigger(const event_type_t *et, size_t index)
{
    return et->data[index].data_type == EVENT_TYPE_DATA_TYPE_TRIGGER;
}

/**
 * @brief       Get the DDM parameter ID with given index.
 *
 * @param       et is a pointer to event_type instance.
 *
 * @return      DDM parameter ID with index given in @a index argument.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline uint32_t event_type_get_parameter_id(const event_type_t *et, size_t index)
{
    return et->data[index].parameter.parameter_id;
}

/**
 * @brief       Get the DDM parameter field offset with given index.
 *
 * @param       et is a pointer to event_type instance.
 *
 * @return      DDM parameter field offset with index given in @a index argument.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline size_t event_type_get_parameter_field_offset(const event_type_t *et, size_t index)
{
    return et->data[index].parameter.field_offset;
}

/**
 * @brief       Get the DDM parameter field size with given index.
 *
 * @param       et is a pointer to event_type instance.
 *
 * @return      DDM parameter field size with index given in @a index argument.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline size_t event_type_get_parameter_field_size(const event_type_t *et, size_t index)
{
    return et->data[index].parameter.field_size;
}

/**
 * @brief       Get the trigger ID with given index.
 *
 * @param       et is a pointer to event_type instance.
 *
 * @return      Trigger ID with index given in @a index argument.
 *
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline uint32_t event_type_get_trigger_id(const event_type_t *et, size_t index)
{
    return et->data[index].trigger.trigger_id;
}

/**
 * @brief       Get the trigger parameter field offset with given index.
 * 
 * @param       et is a pointer to event_type instance.
 * 
 * @return      Trigger parameter field offset with index given in @a index argument.
 * 
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline size_t event_type_get_trigger_field_offset(const event_type_t *et, size_t index)
{
    return et->data[index].trigger.field_offset;
}

/**
 * @brief       Get the trigger parameter field size with given index.
 * 
 * @param       et is a pointer to event_type instance.
 * 
 * @return      Trigger parameter field size with index given in @a index argument.
 * 
 * @pre         Parameter @a et must be non-NULL pointer.
 */
inline size_t event_type_get_trigger_field_size(const event_type_t *et, size_t index)
{
    return et->data[index].trigger.field_size;
}

/**
 * @brief       Copy event type @a from into event type @a to
 *
 * @param       from is a pointer to event type instance to be copied from
 *              (source).
 * @param       to is a pointer to event type instance which will contain the
 *              source data from @from (destination).
 *
 * @pre         Parameter @a er must be non-NULL pointer.
 * @pre         Parameter @a new_copy must be non-NULL pointer.
 */
void event_type_copy(const event_type_t *from, event_type_t *to);

#endif /* EVENT_TYPE_H_ */
