/**
 * \file        event_record.c
 * \date        2024-06-27
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Record structure implementation
 *
 * Implementation of Event Record structure.
 *
 * \li          2024-06-27  (NR) Initial implementation
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
#include "event_record.h"

/* Depends */
#include <string.h>
#include <sys/time.h>

static size_t expected_bytes_for_type(const ddm_entry_t *entry);

static size_t expected_bytes_for_type(const ddm_entry_t *entry)
{
    size_t expected_size;
    DDM2_TYPE_ENUM data_type = ddm_entry__out_type(entry);

    switch (data_type)
    {
    case DDM2_TYPE_INT32_T:
        expected_size = sizeof(int32_t);
        break;
    case DDM2_TYPE_UINT32_T:
        expected_size = sizeof(uint32_t);
        break;
    default:
        expected_size = 0u;  // Variable-length types
        break;
    }
    return expected_size;
}

void event_record_init(event_record_t *er, event_id_t id, int32_t type)
{
    struct timeval current_time;
    int status;

    status = gettimeofday(&current_time, NULL);
    /* When we fail to get current time, use 0 timestamp */
    er->ts = status == 0 ? (current_time.tv_sec * 1000u) + (current_time.tv_usec / 1000ULL) : 0;
    er->id = id;
    er->type = type;
    er->flags = 0u;
    er->data_size = 0u;
    /* This array contains ddm_entry structures. When a record is created it
     * doesn't know which DDM parameters it should store, they are added later.
     * For now, just initialize the structures to 0 as if they are statically
     * allocated.
     */
    memset(er->data, 0, sizeof(er->data));
}

void event_record_terminate(event_record_t *er)
{
    for (size_t i = 0u; i < er->data_size; i++)
    {
        switch (er->data[i].data_type)
        {
        case EVENT_RECORD_DATA_TYPE_PARAMETER:
            ddm_entry__terminate(&er->data[i].parameter);
            break;
        case EVENT_RECORD_DATA_TYPE_TRIGGER:
            er->data[i].trigger = 0;
            break;
        default:
            /* Should not happen */
            break;
        }
    }
}

int event_record_add_parameter_data(event_record_t *er, const ddm_entry_t *source, size_t f_offset, size_t f_size)
{
    // There are the following edge cases to consider:
    // 1. If f_offset >= source_data_size, return empty data
    // 2. If f_size is 0, return all data from offset to end
    // 3. If f_size exceeds available bytes from offset to end, return all available data

    int retval;

    if (er->data_size < ELEMENTS(er->data))
    {
        ddm_entry__init(&er->data[er->data_size].parameter, ddm_entry__parameter_id(source));
        if ((f_size != 0) || (f_offset != 0))
        {
            const void *source_data;
            size_t source_data_size;
            const void *new_source_data;
            size_t new_source_data_size;
            const void *final_data;
            size_t final_size;

            ddm_entry__read__value(source, &source_data, &source_data_size);
            // Edge case 1: If offset >= source_size, return empty data
            if (f_offset >= source_data_size)
            {
                new_source_data = source_data;
                new_source_data_size = 0;
            }
            else
            {
                // Calculate available bytes from offset to end
                size_t available_bytes = source_data_size - f_offset;

                // Edge case 2 & 3: If f_size is 0 or exceeds available bytes, use all available bytes
                if ((f_size == 0) || (f_size > available_bytes))
                {
                    new_source_data_size = available_bytes;
                }
                else
                {
                    new_source_data_size = f_size;
                }
                new_source_data = (const uint8_t *)source_data + f_offset;
            }
            // Determine the final data pointer and size based on type requirements
            size_t expected_size = expected_bytes_for_type(source);
            if (expected_size > 0)
            {
                static uint8_t padded_buffer[DDMP2_MAX_VALUE_SIZE];

                // Fixed-size type (integers): pad to expected type size
                memset(padded_buffer, 0, expected_size);
                if (new_source_data_size > 0)
                {
                    size_t copy_size = (new_source_data_size <= expected_size) ? new_source_data_size : expected_size;
                    memcpy(padded_buffer, new_source_data, copy_size);
                }
                final_data = padded_buffer;
                final_size = expected_size;
            }
            else
            {
                // Variable-length type (strings/structs): use extracted data as-is
                final_data = new_source_data;
                final_size = new_source_data_size;
            }
            // Set the value with the prepared data
            ddm_entry__set__value(&er->data[er->data_size].parameter, final_data, final_size);
        }
        else
        {
            ddm_entry__copy__value(&er->data[er->data_size].parameter, source);
        }
        er->data[er->data_size].data_type = EVENT_RECORD_DATA_TYPE_PARAMETER;
        er->data_size++;
        retval = EVENT_RECORD_OP_SUCCESS;
    }
    else
    {
        retval = EVENT_RECORD_OP_FAILURE;
    }
    return retval;
}

int event_record_add_trigger_data(event_record_t *er, int32_t trigger_value, size_t f_offset, size_t f_size)
{
    // There are the following edge cases to consider:
    // 1. If f_offset >= source_data_size, return empty data
    // 2. If f_size is 0, return all data from offset to end
    // 3. If f_size exceeds available bytes from offset to end, return all available data
    int retval;

    if (er->data_size < ELEMENTS(er->data))
    {
        er->data[er->data_size].data_type = EVENT_RECORD_DATA_TYPE_TRIGGER;
        if ((f_size != 0) || (f_offset != 0))
        {
            // Edge case 1: If offset >= source_size, return empty data
            if (f_offset >= sizeof(trigger_value))
            {
                er->data[er->data_size].trigger = 0;
            }
            else
            {
                // Calculate available bytes from offset to end
                size_t available_bytes = sizeof(trigger_value) - f_offset;

                // Edge case 2 & 3: If f_size is 0 or exceeds available bytes, use all available bytes
                if ((f_size == 0) || (f_size > available_bytes))
                {
                    // Use all available bytes - safe shift since f_offset < sizeof(trigger_value)
                    uint32_t shift_bits = f_offset * 8u;
                    er->data[er->data_size].trigger = trigger_value >> shift_bits;
                }
                else
                {
                    // Use exactly f_size bytes - create mask safely
                    uint32_t shift_bits = f_offset * 8u;
                    uint32_t mask_bits = f_size * 8u;

                    // Since f_size <= available_bytes and available_bytes <= sizeof(trigger_value)
                    // mask_bits will be <= 32, but shifting by 32 is undefined in C, so handle that case explicitly
                    uint32_t mask = (mask_bits == 32) ? 0xFFFFFFFFu : ((1u << mask_bits) - 1u);
                    er->data[er->data_size].trigger = (trigger_value >> shift_bits) & mask;
                }
            }
        }
        else
        {
            er->data[er->data_size].trigger = trigger_value;
        }
        er->data_size++;
        retval = EVENT_RECORD_OP_SUCCESS;
    }
    else
    {
        retval = EVENT_RECORD_OP_FAILURE;
    }
    return retval;
}

extern inline void event_record_set_ts(event_record_t *er, event_timestamp_t ts);
extern inline event_timestamp_t event_record_get_ts(const event_record_t *er);
extern inline void event_record_set_app_acknowledged(event_record_t *er, bool is_acknowledged);
extern inline bool event_record_is_app_acknowledged(const event_record_t *er);
extern inline void event_record_set_cloud_sent(event_record_t *er, bool is_sent);
extern inline bool event_record_is_cloud_sent(const event_record_t *er);
extern inline void event_record_set_cloud_acknowledged(event_record_t *er, bool is_acknowledged);
extern inline bool event_record_is_cloud_acknowledged(const event_record_t *er);
extern inline void event_record_set_flags(event_record_t *er, uint32_t flags);
extern inline uint32_t event_record_get_flags(const event_record_t *er);
extern inline event_id_t event_record_get_id(const event_record_t *er);
extern inline int32_t event_record_get_type(const event_record_t *er);
extern inline uint32_t event_record_get_data_size(const event_record_t *er);
extern inline const ddm_entry_t *event_record_get_parameter_data(const event_record_t *er, size_t index);
extern inline int32_t event_record_get_trigger_data(const event_record_t *er, size_t index);
extern inline int32_t event_record_get_data_type(const event_record_t *er, size_t index);
extern inline bool event_record_is_data_parameter(const event_record_t *er, size_t index);
extern inline bool event_record_is_data_trigger(const event_record_t *er, size_t index);

void event_record_copy(const event_record_t *from, event_record_t *to)
{
    to->id = from->id;
    to->ts = from->ts;
    to->type = from->type;
    to->flags = from->flags;
    to->data_size = from->data_size;
    for (size_t i = 0u; i < from->data_size; i++)
    {
        switch (from->data[i].data_type)
        {
        case EVENT_RECORD_DATA_TYPE_TRIGGER:
            to->data[i].data_type = EVENT_RECORD_DATA_TYPE_TRIGGER;
            to->data[i].trigger = from->data[i].trigger;
            break;
        case EVENT_RECORD_DATA_TYPE_PARAMETER:
            to->data[i].data_type = EVENT_RECORD_DATA_TYPE_PARAMETER;
            ddm_entry__init_copy(&to->data[i].parameter, &from->data[i].parameter);
            break;
        default:
            /* Should not happen */
            break;
        }
    }
}
