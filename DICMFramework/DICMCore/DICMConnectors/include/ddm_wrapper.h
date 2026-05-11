/*****************************************************************************
 * \file       ddm_wrapper.h
 * \brief      Make use of the ddm more easy
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
#ifndef DDM_WRAPPER_H_
#define DDM_WRAPPER_H_
#include "connector.h"
#include "ddm2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/**********************************************************
 * Public defines
 *********************************************************/

/**
 * @brief Override locally if needed
 */
#ifndef DDMW_ITEM_DATA_CAPACITY
#define DDMW_ITEM_DATA_CAPACITY DDMP2_MAX_VALUE_SIZE
#endif
#ifndef DDMW_EVENT_DATA_CAPACITY
#define DDMW_EVENT_DATA_CAPACITY DDMP2_MAX_VALUE_SIZE
#endif
#define DDMW_INVALID_UID UINT32_MAX

/**********************************************************
 * Public types
 *********************************************************/

// Forward declaration to ddm wrapper
typedef struct ddmw_t ddmw_t;

// DDM Wrapper action type
typedef enum
{
    DDMW_ACTION_PUBLISH,
    DDMW_ACTION_SET
} ddmw_action_t;

// DDM wrapper object
typedef struct _ddmw_item_t
{
    // Internal
    const ddmw_t *wrapper;
    const CONNECTOR *connector;
    struct _ddmw_item_t *next;
    // Properties
    uint32_t parameter;
    int size;
    int maxsize;          // Will be set to DDMW_ITEM_DATA_CAPACITY when added
    bool is_initialized;  // True means that parameters has been initialized internally by the connector, or someone has SET/PUBLISHED the parameter
    bool subscribed;      // True means that own subscription has started or someone has started the subscription
    bool modified;
    bool updated;
    bool has_no_value;        // True means that there is no value to subscribe to, data is only generated at a SET
    bool requires_immediate_processing;  // True means that this item requires immediate processing
    ddmw_action_t type;
    uint8_t data[DDMW_ITEM_DATA_CAPACITY];  // Storage needs to be last or references in items will be wrong in common library functions
} ddmw_item_t;

typedef struct _ddmw_event_t
{
    uint32_t event_id;
    uint8_t data[DDMW_EVENT_DATA_CAPACITY];
    uint32_t data_size;
    bool updated;
} ddmw_event_t;

// DDM item object
typedef struct ddmw_t
{
    const CONNECTOR *connector;
    ddmw_item_t *items;
    ddmw_item_t *last_item;
    ddmw_event_t event_item;
    void (*inventory_cb)(uint32_t parameter);
    void (*inventory_context_cb)(uint32_t parameter, void *inventory_context);
    void *inventory_context;
    SemaphoreHandle_t mutex_handle;
    StaticSemaphore_t mutex_storage;
    int itemsize;
} ddmw_t;

/**********************************************************
 * Public variables
 *********************************************************/
void ddmw_init(ddmw_t *wrapper, const CONNECTOR *const connector);
void ddmw_init_size(ddmw_t *wrapper, const CONNECTOR *const connector, int item_size);
void ddmw_process(ddmw_t *wrapper, const DDMP2_FRAME *const frame);
void ddmw_process_publish(ddmw_t *wrapper);
int ddmw_register(ddmw_t *wrapper, uint32_t dev_class);
void ddmw_add(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance);
void ddmw_add_one(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance);
void ddmw_add_zero(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance);
void ddmw_add_i32(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance, int32_t value);
void ddmw_add_u32(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance, uint32_t value);
void ddmw_add_str(ddmw_t *wrapper, ddmw_item_t *item, uint32_t parameter, int instance, const char *value);
bool ddmw_exists(ddmw_t *wrapper, ddmw_item_t *item);
bool ddmw_remove(ddmw_t *wrapper, ddmw_item_t *item);
void ddmw_get_inventory(ddmw_t *wrapper, void (*inventory_cb)(uint32_t parameter));
void ddmw_get_inventory_with_context(ddmw_t *wrapper, void (*inventory_context_cb)(uint32_t parameter, void *inventory_context), void *inventory_context);
void ddmw_send_set_i32(ddmw_t *wrapper, uint32_t parameter, int instance, int32_t value);
ddmw_item_t *ddmw_find_item(ddmw_t *wrapper, uint32_t parameter);

// Item operations
int32_t ddmw_sizeof(ddmw_item_t *item);
void ddmw_subscribe(ddmw_item_t *item);
void ddmw_unsubscribe(ddmw_item_t *item);
bool ddmw_is_subscribed(ddmw_item_t *item);
bool ddmw_is_updated(ddmw_item_t *item);
void ddmw_set_has_no_value(ddmw_item_t *item, bool has_no_value);
void ddmw_set_requires_immediate_processing(ddmw_item_t *item, bool requires_immediate_processing);
bool ddmw_requires_immediate_processing(ddmw_item_t *item);
void ddmw_set_data(ddmw_item_t *item, const void *data, int size);
void ddmw_set_i32(ddmw_item_t *item, int32_t value);
void ddmw_set_u32(ddmw_item_t *item, uint32_t value);
void ddmw_set_str(ddmw_item_t *item, const char *value);
int ddmw_get_data(ddmw_item_t *item, void *data, int size);
int32_t ddmw_get_i32(ddmw_item_t *item);
uint32_t ddmw_get_u32(ddmw_item_t *item);
int32_t ddmw_get_i32_def(ddmw_item_t *item, int default_value);
uint8_t ddmw_set_type(ddmw_item_t *item, ddmw_action_t t);
ddmw_action_t ddmw_get_type(ddmw_item_t *item);
int ddmw_get_instance(ddmw_item_t *item);

// Generic events
void ddmw_send_generic_event_data(const ddmw_t *const wrapper, const uint32_t event_id, const void *value, const size_t value_size);
bool ddmw_is_generic_event_updated(ddmw_t *const wrapper);
uint32_t ddmw_get_generic_event_id(const ddmw_t *const wrapper);
int ddmw_get_generic_event_data(const ddmw_t *const wrapper, void *const data, const uint32_t size);

// NVS
uint32_t ddmw_load_uid32(const char *class_name, int instance);
void ddmw_save_uid32(const char *class_name, int instance, uint32_t uid);
void ddmw_load_i32(ddmw_item_t *item, const char *class_name, int instance, const char *param, int32_t default_value);
void ddmw_save_i32(ddmw_item_t *item, const char *class_name, int instance, const char *param);
void ddmw_save_blob(ddmw_item_t *item, const char *class_name, int instance, const char *param);
int ddmw_save_if_updated_i32(ddmw_item_t *item, const char *class_name, int instance, const char *param);
int ddmw_save_if_updated_str(ddmw_item_t *item, const char *class_name, int instance, const char *param);
void ddmw_load_str(ddmw_item_t *item, const char *class_name, int instance, const char *param, char *default_str);
int ddmw_save_str(const char *class_name, int instance, const char *param, const char *str);

//---------------------------------------------------------
#endif
