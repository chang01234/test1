/**
 * \file        event_type.c
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Type structure implementation
 *
 * Implementation of Event Type structure.
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

/* Implements */
#include "event_type.h"

/* Depends */
#include <string.h>

void event_type_init(event_type_t *et)
{
    et->type = 0;
    et->config = EVENT_TYPE_CONFIG_DISABLED;
    et->data_size = 0u;
    memset(et->data, 0, sizeof(et->data));
}

void event_type_terminate(event_type_t *et)
{
    event_type_init(et);
}

extern inline void event_type_set_type(event_type_t *et, uint32_t type);
extern inline int32_t event_type_get_type(const event_type_t *et);
extern inline void event_type_set_config(event_type_t *et, event_type_config_t config);
extern inline bool event_type_is_disabled(const event_type_t *et);
extern inline bool event_type_is_local(const event_type_t *et);
extern inline bool event_type_is_local(const event_type_t *et);
extern inline bool event_type_is_cloud(const event_type_t *et);
extern inline void event_type_add_parameter_data(event_type_t *et, uint32_t ddm_parameter, size_t f_offset, size_t f_size);
extern inline void event_type_add_trigger_data(event_type_t *et, uint32_t trigger_id, size_t f_offset, size_t f_size);
extern inline size_t event_type_get_data_size(const event_type_t *et);
extern inline bool event_type_is_data_parameter(const event_type_t *et, size_t index);
extern inline bool event_type_is_data_trigger(const event_type_t *et, size_t index);
extern inline uint32_t event_type_get_parameter_id(const event_type_t *et, size_t index);
extern inline size_t event_type_get_parameter_field_offset(const event_type_t *et, size_t index);
extern inline size_t event_type_get_parameter_field_size(const event_type_t *et, size_t index);
extern inline uint32_t event_type_get_trigger_id(const event_type_t *et, size_t index);
extern inline size_t event_type_get_trigger_field_offset(const event_type_t *et, size_t index);
extern inline size_t event_type_get_trigger_field_size(const event_type_t *et, size_t index);

void event_type_copy(const event_type_t *from, event_type_t *to)
{
    memcpy(to, from, sizeof(*to));
}
