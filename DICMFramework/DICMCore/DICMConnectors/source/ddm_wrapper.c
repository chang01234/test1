/*****************************************************************************
 * \file       ddm_wrapper.c
 * \brief      Wrapper for DDM
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/
#include "ddm_wrapper.h"
#include "broker.h"
#include "configuration.h"
#include "hal_cpu.h"
#include <string.h>

/**********************************************************
 * Private functions
 *********************************************************/
static ddmw_item_t *ddmw_find(ddmw_t *wrapper, uint32_t parameter);
static void ddmw_acquire(const ddmw_t *const wrapper);
static void ddmw_release(const ddmw_t *const wrapper);
static void ddmw_process_message(ddmw_t *wrapper, const DDMP2_FRAME *message);
static void ddmw_process_publish_data(ddmw_item_t *item);

/**********************************************************
 * Private variables
 *********************************************************/

/**********************************************************
 * Implementation
 *********************************************************/

/**********************************************************
 * Function:    ddmw_init
 * Description: Create wrapper object
 *********************************************************/
void ddmw_init(ddmw_t *wrapper, const CONNECTOR *connector)
{
    // Init wrapper
    memset(wrapper, 0, sizeof(ddmw_t));
    wrapper->connector = connector;
    wrapper->itemsize = DDMW_ITEM_DATA_CAPACITY;
    // Init mutex
    TRUE_CHECK((wrapper->mutex_handle = xSemaphoreCreateRecursiveMutexStatic(&wrapper->mutex_storage)) != NULL);
}

/**********************************************************
 * Function:    ddmw_init
 * Description: Create wrapper object
 *********************************************************/
void ddmw_init_size(ddmw_t *wrapper, const CONNECTOR *const connector, int itemsize)
{
    // Init wrapper
    memset(wrapper, 0, sizeof(ddmw_t));
    wrapper->connector = connector;
    wrapper->itemsize = itemsize;
    // Init mutex
    TRUE_CHECK((wrapper->mutex_handle = xSemaphoreCreateRecursiveMutexStatic(&wrapper->mutex_storage)) != NULL);
}

/**********************************************************
 * Function:    ddmw_acquire
 * Description: Acquire mutex
 *********************************************************/
void ddmw_acquire(const ddmw_t *const wrapper)
{
    // Enter critical
    TRUE_CHECK(xSemaphoreTakeRecursive(wrapper->mutex_handle, portMAX_DELAY));
}

/**********************************************************
 * Function:    ddmw_release
 * Description: Release mutex
 *********************************************************/
void ddmw_release(const ddmw_t *const wrapper)
{
    // Exit critical
    xSemaphoreGiveRecursive(wrapper->mutex_handle);
}

/**********************************************************
 * Function:    ddmw_add
 * Description: Add a parameter (empty)
 *********************************************************/
void ddmw_add(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance)
{
    // Add instance to parameter
    parameter = DDM2_PARAMETER_BASE_INSTANCE(parameter);
    parameter |= DDM2_PARAMETER_INSTANCE(instance);

    // Remove from list (if it exists)
    ddmw_remove(wrapper, item);

    // Initialize new item
    ddmw_acquire(wrapper);
    item->wrapper = wrapper;
    item->connector = wrapper->connector;
    item->parameter = parameter;
    item->next = NULL;
    item->size = 0;
    item->maxsize = wrapper->itemsize;
    item->subscribed = false;
    item->modified = false;
    item->updated = false;
    item->has_no_value = false;
    item->is_initialized = false;
    item->requires_immediate_processing = false;
    item->type = DDMW_ACTION_PUBLISH;

    // Add to list
    if (wrapper->last_item)
    {
        wrapper->last_item->next = item;
    }
    else
    {
        wrapper->items = item;
    }
    wrapper->last_item = item;

    // Multi-thread
    ddmw_release(wrapper);
}

/**********************************************************
 * Function:    ddmw_add_i32
 * Description: Add a parameter as int32
 *********************************************************/
void ddmw_add_i32(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance, int32_t value)
{
    ddmw_add(wrapper, item, parameter, instance);
    ddmw_set_i32(item, value);
}

/**********************************************************
 * Function:    ddmw_add_u32
 * Description: Add a parameter as uint32
 *********************************************************/
void ddmw_add_u32(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance, uint32_t value)
{
    ddmw_add(wrapper, item, parameter, instance);
    ddmw_set_u32(item, value);
}

/**********************************************************
 * Function:    ddmw_add_str
 * Description: Add a parameter as string
 *********************************************************/
void ddmw_add_str(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance, const char *value)
{
    ddmw_add(wrapper, item, parameter, instance);
    ddmw_set_str(item, value);
}

/**********************************************************
 * Function:    ddmw_add_void
 * Description: Add empty parameter (same as)
 *********************************************************/
void ddmw_add_void(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance)
{
    ddmw_add(wrapper, item, parameter, instance);
}

/**********************************************************
 * Function:    ddmw_add_zero
 * Description: Add a parameter set to 0
 *********************************************************/
void ddmw_add_zero(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance)
{
    ddmw_add_i32(wrapper, item, parameter, instance, 0);
}

/**********************************************************
 * Function:    ddmw_add_one
 * Description: Add a parameter set to 1
 *********************************************************/
void ddmw_add_one(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance)
{
    ddmw_add_i32(wrapper, item, parameter, instance, 1);
}

/**********************************************************
 * Function:    ddmw_exists
 * Description: Return true if item is part of the list
 *********************************************************/
bool ddmw_exists(ddmw_t *wrapper, ddmw_item_t *item)
{
    // Multi-thread
    ddmw_acquire(wrapper);

    // Scan
    bool found = false;
    ddmw_item_t *node = wrapper->items;
    while (node)
    {
        // Match?
        if (node == item)
        {
            found = true;
            break;
        }

        // Next
        node = node->next;
    }

    // Done
    ddmw_release(wrapper);
    return found;
}

/**********************************************************
 * Function:    ddmw_remove
 * Description: Remove a parameter
 *********************************************************/
bool ddmw_remove(ddmw_t *wrapper, ddmw_item_t *item)
{
    // Multi-thread
    ddmw_acquire(wrapper);

    // Scan
    bool found = false;
    ddmw_item_t *parent = NULL;
    ddmw_item_t *node = wrapper->items;
    while (node)
    {
        // Match?
        if (node == item)
        {
            // Remove from list
            if (parent == NULL)
            {
                wrapper->items = node->next;  // First item
            }
            else
            {
                parent->next = node->next;
            }

            // Update last item
            if (wrapper->last_item == node)
            {
                wrapper->last_item = parent;
            }
            // Done
            node->next = NULL;
            found = true;
            break;
        }

        // Next
        parent = node;
        node = node->next;
    }

    // Done
    ddmw_release(wrapper);
    return found;
}

/**********************************************************
 * Function:    ddmw_find
 * Description: Initialize ddm item
 *********************************************************/
static ddmw_item_t *ddmw_find(ddmw_t *wrapper, uint32_t parameter)
{
    // Multi-thread
    ddmw_acquire(wrapper);

    // Scan
    ddmw_item_t *item = wrapper->items;
    ddmw_item_t *result = NULL;
    while (item)
    {
        if (item->parameter == parameter)
        {
            // Found
            result = item;
            break;
        }
        item = item->next;
    }

    // Done
    ddmw_release(wrapper);
    return result;
}

/**********************************************************
 * Function:    ddmw_set_type
 * Description: Set type (Publish or Set)
 *********************************************************/
uint8_t ddmw_set_type(ddmw_item_t *item, ddmw_action_t t)
{
    return item->type = t;
}

/**********************************************************
 * Function:    ddmw_get_type
 * Description: Return type (Publish or Set)
 *********************************************************/
ddmw_action_t ddmw_get_type(ddmw_item_t *item)
{
    return item->type;
}

/**********************************************************
 * Function:    ddmw_get_instance
 * Description: Get parameter instance
 *********************************************************/
int ddmw_get_instance(ddmw_item_t *item)
{
    return DDM2_PARAMETER_INSTANCE_FIELD(item->parameter);
}

/**********************************************************
 * Function:    ddmw_sizeof
 * Description: Get item size
 *********************************************************/
int32_t ddmw_sizeof(ddmw_item_t *item)
{
    return item->size;
}

/**********************************************************
 * Function:    ddmw_subscribe
 * Description: Suscribe (means we are not the owner)
 *********************************************************/
void ddmw_subscribe(ddmw_item_t *item)
{
    // Mark as suscribed (publish will set value)
    item->subscribed = true;

    // Send command
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, item->parameter, NULL, 0, item->connector->connector_id, (TickType_t)portMAX_DELAY);
}

/**********************************************************
 * Function:    ddmw_unsubscribe
 * Description: Unsubscribe (means we are not the owner)
 *********************************************************/
void ddmw_unsubscribe(ddmw_item_t *item)
{
    // Mark as suscribed (publish will set value)
    item->subscribed = false;

    // Send command
    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, item->parameter, NULL, 0, item->connector->connector_id, (TickType_t)portMAX_DELAY);
}

/**********************************************************
 * Function:    ddmw_set_data
 * Description: Set parameter data
 *********************************************************/
void ddmw_set_data(ddmw_item_t *item, const void *data, int size)
{
    ddmw_acquire(item->wrapper);

    // New size?
    if (item->size != size)
    {
        item->size = MIN(item->maxsize, size);
        memcpy(item->data, data, item->size);
        item->modified = true;
        item->is_initialized = true;
    }

    // New content?
    else if (memcmp(item->data, data, item->size) != 0)
    {
        memcpy(item->data, data, item->size);
        item->modified = true;
        item->is_initialized = true;
    }

    if (ddmw_requires_immediate_processing(item))
    {
        // Mark as modified, for items requiring immediate processing to trigger immediate set/publish
        item->modified = true;  // will be reset in ddmw_process_publish_data, so it will
                                // not be set/published again trough ddmw_process_publish()
        ddmw_process_publish_data(item);
    }

    // Clear updated flag (set from external)
    item->updated = false;

    ddmw_release(item->wrapper);
}

/**********************************************************
 * Function:    ddmw_set_i32
 * Description: Set parameter data as int32
 *********************************************************/
void ddmw_set_i32(ddmw_item_t *item, int32_t value)
{
    ddmw_set_data(item, &value, 4);
}

/**********************************************************
 * Function:    ddmw_set_u32
 * Description: Set parameter data as uint32
 *********************************************************/
void ddmw_set_u32(ddmw_item_t *item, uint32_t value)
{
    ddmw_set_data(item, &value, 4);
}

/**********************************************************
 * Function:    ddmw_set_str
 * Description: Set parameter data as string
 *********************************************************/
void ddmw_set_str(ddmw_item_t *item, const char *value)
{
    ddmw_set_data(item, value, strlen(value));
}

/**********************************************************
 * Function:    ddmw_get_data
 * Description: Get parameter data
 *********************************************************/
int ddmw_get_data(ddmw_item_t *item, void *data, int size)
{
    // Copy available data
    int available = MIN(size, item->size);
    memcpy(data, item->data, available);

    // Padding?
    if (available < size)
    {
        memset(&((uint8_t *)data)[available], 0, size - available);
    }
    return available;
}

/**********************************************************
 * Function:    ddmw_get_i32
 * Description: Get parameter data as int32
 *********************************************************/
int32_t ddmw_get_i32(ddmw_item_t *item)
{
    int32_t result = 0;
    if (item->size == 4)
    {
        ddmw_get_data(item, &result, 4);
    }
    return result;
}

/**********************************************************
 * Function:    ddmw_get_u32
 * Description: Get parameter data as uint32
 *********************************************************/
uint32_t ddmw_get_u32(ddmw_item_t *item)
{
    uint32_t result = 0;
    if (item->size == 4)
    {
        ddmw_get_data(item, &result, 4);
    }
    return result;
}

/**********************************************************
 * Function:    ddmw_get_i32_def
 * Description: Get parameter data as int32
 *********************************************************/
int32_t ddmw_get_i32_def(ddmw_item_t *item, int default_value)
{
    int32_t result = default_value;
    if (item->size == 4)
    {
        ddmw_get_data(item, &result, 4);
    }
    return result;
}

/**********************************************************
 * Function:    ddmw_get_generic_event_id
 * Description: Get generic event id
 *********************************************************/
uint32_t ddmw_get_generic_event_id(const ddmw_t *const wrapper)
{
    uint32_t event_id;

    ddmw_acquire(wrapper);
    event_id = wrapper->event_item.event_id;
    ddmw_release(wrapper);

    return event_id;
}

/**********************************************************
 * Function:    ddmw_get_generic_event_data
 * Description: Get generic event data
 *********************************************************/
int ddmw_get_generic_event_data(const ddmw_t *const wrapper, void *const data, const uint32_t size)
{
    TRUE_CHECK_RETURN0(data != NULL);
    TRUE_CHECK_RETURN0(size);

    ddmw_acquire(wrapper);

    uint32_t data_available = MIN(wrapper->event_item.data_size, size);
    memcpy(data, wrapper->event_item.data, data_available);

    ddmw_release(wrapper);

    return data_available;
}

/**********************************************************
 * Function:    ddmw_process
 * Description: Process frame
 *********************************************************/
void ddmw_process(ddmw_t *wrapper, const DDMP2_FRAME *const frame)
{
    // Multi-thread
    ddmw_acquire(wrapper);

    // Process incoming messages
    ddmw_process_message(wrapper, frame);

    // Done
    ddmw_release(wrapper);
}

/**********************************************************
 * Function:    ddmw_send_set_i32
 * Description: Send a single direct set command
 *********************************************************/
void ddmw_send_set_i32(ddmw_t *wrapper, uint32_t parameter, int instance, int32_t value)
{
    // Build parameter
    parameter = DDM2_PARAMETER_BASE_INSTANCE(parameter);
    parameter |= DDM2_PARAMETER_INSTANCE(instance);

    // Send to broker
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, parameter, &value, sizeof(value), wrapper->connector->connector_id, (TickType_t)portMAX_DELAY);
}

/**
 * Find the item that matches the given parameter value.
 *
 * @param[in] wrapper Wrapper object
 * @param[in] parameter Parameter of item to find
 * @return Pointer to found item if successful, NULL otherwise.
 */
ddmw_item_t *ddmw_find_item(ddmw_t *wrapper, uint32_t parameter)
{
    // Scan
    ddmw_item_t *result = ddmw_find(wrapper, parameter);
    return result;
}

/**********************************************************
 * Function:    ddmw_send_generic_event_data
 * Description: Send a generic event to source connector
 *********************************************************/
void ddmw_send_generic_event_data(const ddmw_t *const wrapper, const uint32_t event_id, const void *value, const size_t value_size)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, event_id, value, value_size, wrapper->connector->connector_id, portMAX_DELAY);
}

/**********************************************************
 * Function:    ddmw_get_inventory
 * Description: Get inventory by subscribing to it
 *********************************************************/
void ddmw_get_inventory(ddmw_t *wrapper, void (*inventory_cb)(uint32_t parameter))
{
    // Set callback
    wrapper->inventory_cb = inventory_cb;

    // Send command
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, GW0INV, NULL, 0, wrapper->connector->connector_id, portMAX_DELAY);
}

/**********************************************************
 * Function:    ddmw_get_inventory_with_context
 * Description: Get inventory by subscribing to it
 *********************************************************/
void ddmw_get_inventory_with_context(ddmw_t *wrapper, void (*inventory_context_cb)(uint32_t parameter, void *inventory_context), void *inventory_context)
{
    // Set callback
    wrapper->inventory_context_cb = inventory_context_cb;
    wrapper->inventory_context = inventory_context;

    // Send command
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, GW0INV, NULL, 0, wrapper->connector->connector_id, portMAX_DELAY);
}

/**********************************************************
 * Function:    ddmw_process_publish_data
 * Description: Process publish data
 *********************************************************/
static void ddmw_process_publish_data(ddmw_item_t *item)
{
    // Is modified?
    if (item->modified)
    {
        // Only publish if anyone has subscribed to parameter or if AVL property
        if ((item->type == DDMW_ACTION_PUBLISH) && ((item->subscribed) || DDM2_IS_AVAIL_PROPERTY(item->parameter)))
        {
            // Publish
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, item->parameter, item->data, item->size, item->connector->connector_id, portMAX_DELAY);
        }
        // Type is set
        else if (item->type == DDMW_ACTION_SET)
        {
            // Set
            connector_send_frame_to_broker(DDMP2_CONTROL_SET, item->parameter, item->data, item->size, item->connector->connector_id, portMAX_DELAY);
        }

        // Clear
        item->modified = false;
    }
}

/**********************************************************
 * Function:    ddmw_process_publish
 * Description: Process modified parameters
 *********************************************************/
void ddmw_process_publish(ddmw_t *wrapper)
{
    // Scan parameters
    ddmw_acquire(wrapper);
    ddmw_item_t *item = wrapper->items;

    while (item)
    {
        ddmw_process_publish_data(item);
        item = item->next;
    }
    ddmw_release(wrapper);
}

/**********************************************************
 * Function:    ddmw_process_message
 * Description: Process received ddm message
 *********************************************************/
static void ddmw_process_message(ddmw_t *wrapper, const DDMP2_FRAME *message)
{
    ddmw_item_t *item;

    if (message->frame.control == DDMP2_CONTROL_SET)
    {
        item = ddmw_find(wrapper, message->frame.set.parameter);
        if (item != NULL)
        {
            ddmw_set_data(item, message->frame.set.value.raw, ddmp2_value_size(message));
            item->updated = true;
        }
    }
    else if (message->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        item = ddmw_find(wrapper, message->frame.subscribe.parameter);
        if (item != NULL)
        {
            // Only publish if value has been initialzied(internally by the connector or by SET frame)
            if ((!item->has_no_value) && (item->is_initialized))
            {
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, item->parameter, item->data, item->size, item->connector->connector_id, (TickType_t)portMAX_DELAY);
                item->modified = false;
            }
            item->subscribed = true;
        }
    }
    else if (message->frame.control == DDMP2_CONTROL_PUBLISH)
    {
        item = ddmw_find(wrapper, message->frame.publish.parameter);
        if (item != NULL)
        {
            if (item->subscribed)
            {
                ddmw_set_data(item, message->frame.publish.value.raw, ddmp2_value_size(message));
                item->modified = false;
                item->updated = true;
            }
        }

        // Received inventory?
        if ((wrapper->inventory_cb != NULL) || (wrapper->inventory_context_cb != NULL))
        {
            if (message->frame.publish.parameter == GW0INV)
            {
                // Decode
                uint32_t *params = (uint32_t *)&message->frame.publish.value.raw;
                uint32_t count = ddmp2_value_size(message) / 4;
                for (uint32_t i = 0; i < count; i++)
                {
                    if (wrapper->inventory_cb != NULL)
                    {
                        wrapper->inventory_cb(params[i]);
                    }
                    if (wrapper->inventory_context_cb != NULL)
                    {
                        wrapper->inventory_context_cb(params[i], wrapper->inventory_context);
                    }
                }
            }
        }
    }
    else if (message->frame.control == DDMP2_CONTROL_GENERIC)
    {
        uint32_t data_available = MIN(DDMW_EVENT_DATA_CAPACITY, ddmp2_value_size(message));

        wrapper->event_item.event_id = message->frame.generic.id;

        wrapper->event_item.data_size = data_available;
        memset(wrapper->event_item.data, 0, DDMW_EVENT_DATA_CAPACITY);
        memcpy(&wrapper->event_item.data, message->frame.generic.data, data_available);

        wrapper->event_item.updated = true;
    }
}

/**********************************************************
 * Function:    ddmw_register
 * Description: Register new instance for selected class
 *********************************************************/
int ddmw_register(ddmw_t *wrapper, uint32_t dev_class)
{
    // register if AVL property
    uint32_t device_class = dev_class;
    if (DDM2_IS_AVAIL_PROPERTY(device_class) == 0)
    {
        LOG(E, "Invalid AVL parameter[%08x]", DDM2_PARAMETER_PROPERTY_FIELD(device_class));
        return -1;
    }

    // request instance and register
    int instance = broker_register_instance(&device_class, wrapper->connector->connector_id);
    if (instance == -1)
    {
        LOG(E, "Registration failed for class %08x", device_class);
        return -1;
    }

    return instance;
}

/**********************************************************
 * Function:    ddmw_is_subscribed
 * Description: Return true if item is suscribed
 *********************************************************/
bool ddmw_is_subscribed(ddmw_item_t *item)
{
    return item->subscribed;
}

/**********************************************************
 * Function:    ddmw_is_updated
 * Description: Return true if item was updated for broker
 *********************************************************/
bool ddmw_is_updated(ddmw_item_t *item)
{
    bool updated = item->updated;
    item->updated = false;
    return updated;
}

/**********************************************************
 * Function:    ddmw_is_generic_event_updated
 * Description: Return true if an event was updated
 *********************************************************/
bool ddmw_is_generic_event_updated(ddmw_t *const wrapper)
{
    ddmw_acquire(wrapper);
    bool updated = wrapper->event_item.updated;
    wrapper->event_item.updated = false;
    ddmw_release(wrapper);

    return updated;
}

/**********************************************************
 * Function:    ddmw_set_has_no_value
 * Description: Set has_no_value flag
 *********************************************************/
void ddmw_set_has_no_value(ddmw_item_t *item, bool has_no_value)
{
    item->has_no_value = has_no_value;
}

/**********************************************************
 * Function:    ddmw_set_requires_immediate_processing
 * Description: Set requires immediate processing flag
 *********************************************************/
void ddmw_set_requires_immediate_processing(ddmw_item_t *item, bool requires_immediate_processing)
{
    TRUE_CHECK_RETURN(!DDM2_IS_AVAIL_PROPERTY(item->parameter));
    item->requires_immediate_processing = requires_immediate_processing;
}

/**********************************************************
 * Function:    ddmw_requires_immediate_processing
 * Description: Return true if item requires immediate processing
 *********************************************************/
bool ddmw_requires_immediate_processing(ddmw_item_t *item)
{
    return item->requires_immediate_processing;
}

/**********************************************************
 * Function:    ddmw_load_str
 * Description: Load string for specified class instance
 *********************************************************/
void ddmw_load_str(ddmw_item_t *item, const char *class_name, int instance, const char *param, char *default_str)
{
    size_t str_length = item->maxsize;
    char str[item->maxsize];
    char key[item->maxsize];
    char *p = str;

    // Build key
    snprintf(key, sizeof(key), "%s%d%s", class_name, instance, param);

    // Try to read it
    int result = config_get_str(key, str, &str_length);
    if (result != ESP_OK)
    {
        // Use default
        p = default_str;
    }

    // Set DDM
    ddmw_set_str(item, p);
}

/**********************************************************
 * Function:    ddmw_save_str
 * NOTE: Strings will be limited to to have a max size of 32-1.
 * Description: Save string for specified class instance
 *********************************************************/
int ddmw_save_str(const char *class_name, int instance, const char *param, const char *str)
{
    // Build key
    char key[32];
    char local_str[DDMW_ITEM_DATA_CAPACITY];
    const char *p = str;

    snprintf(key, sizeof(key), "%s%d%s", class_name, instance, param);

    // Limit saved strings
    if (strlen(str) > (DDMW_ITEM_DATA_CAPACITY - 1))
    {
        strncpy(local_str, str, DDMW_ITEM_DATA_CAPACITY - 1);
        local_str[DDMW_ITEM_DATA_CAPACITY - 1] = '\0';
        p = local_str;
    }

    // Save string
    return config_set_str(key, p);
}

/**********************************************************
 * Function:    ddmw_load_uid32
 * Description: Return unique id (uint32) for specified class instance
 *********************************************************/
uint32_t ddmw_load_uid32(const char *class_name, int instance)
{
    // Build uid key
    char key[32];
    snprintf(key, sizeof(key), "%s%duid", class_name, instance);

    // Try to read it
    return (uint32_t)config_get_i32_def(key, DDMW_INVALID_UID);
}

/**********************************************************
 * Function:    ddmw_save_uid32
 * Description: Save unique id (uint32) for specified class instance
 *********************************************************/
void ddmw_save_uid32(const char *class_name, int instance, uint32_t uid)
{
    // Build key
    char key[32];
    snprintf(key, sizeof(key), "%s%duid", class_name, instance);

    // Save value
    config_set_i32(key, uid);
}

/**********************************************************
 * Function:    ddmw_load_i32
 * Description: Load specified parameter from configuration
 *********************************************************/
void ddmw_load_i32(ddmw_item_t *item, const char *class_name, int instance, const char *param, int32_t default_value)
{
    // Build key
    char key[32];
    snprintf(key, sizeof(key), "%s%d%s", class_name, instance, param);

    // Load value
    int32_t value;
    if (config_get_i32(key, &value) != ESP_OK)
    {
        // Use default
        value = default_value;
    }

    // Set DDM
    ddmw_set_i32(item, value);
}

/**********************************************************
 * Function:    ddmw_save_i32
 * Description: Save specified parameter to configuration
 *********************************************************/
void ddmw_save_i32(ddmw_item_t *item, const char *class_name, int instance, const char *param)
{
    // Build key
    char key[32];
    snprintf(key, sizeof(key), "%s%d%s", class_name, instance, param);

    // Save value
    int32_t value = ddmw_get_i32(item);
    config_set_i32(key, value);
}

/**********************************************************
 * Function:    ddmw_save_if_updated_i32
 * Description: Save specified parameter to configuration if updated
 * Return non zero if updated.
 *********************************************************/
int ddmw_save_if_updated_i32(ddmw_item_t *item, const char *class_name, int instance, const char *param)
{
    if (ddmw_is_updated(item))
    {
        ddmw_save_i32(item, class_name, instance, param);
        return 1;
    }

    return 0;
}

/**********************************************************
 * Function:    ddmw_save_if_updated_str
 * Description: Save specified parameter to configuration if updated
 * Return non zero if updated.
 *********************************************************/
int ddmw_save_if_updated_str(ddmw_item_t *item, const char *class, int instance, const char *param)
{
    char str[item->maxsize];

    if (ddmw_is_updated(item))
    {
        ddmw_get_data(item, str, item->maxsize);
        ddmw_save_str(class, instance, param, str);
        return 1;
    }

    return 0;
}
