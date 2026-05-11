/**
 * \file        make_json.c
 * \date        2024-07-09
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Create JSON from event record.
 *
 * \li          2024-07-09  (NR) Initial implementation
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
#include "make_json.h"

/* Depends */
#include <stdio.h>

#include "ddm_entry.h"
#include "event_record.h"

int make_json_event_record(const event_record_t *er, const char *key, const char *device_id, char *buffer, size_t buffer_size)
{
    int status;
    int index = 0;
    int free_space = (int)buffer_size;
    status = snprintf(&buffer[index],
                      free_space,
                      "{\"ts\":%u,\"id\":%u,\"type\":%d,\"ack\":%d,\"device_id\":\"%s\",\"key\":\"%s\",\"values\":[",
                      event_record_get_ts(er),
                      event_record_get_id(er),
                      event_record_get_type(er),
                      event_record_is_app_acknowledged(er) ? 1 : 0,
                      device_id,
                      key);
    TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
    index += status;
    free_space -= status;
    TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
    size_t record_data_size = event_record_get_data_size(er);
    if (record_data_size != 0)
    {
        for (size_t i = 0u; i < record_data_size; i++)
        {
            const ddm_entry_t *ddm_entry;

            if (event_record_is_data_parameter(er, i))
            {
                ddm_entry = event_record_get_parameter_data(er, i);
                if (ddm_entry__is_value_none(ddm_entry))
                {
                    status = snprintf(&buffer[index], free_space, "\"\",");
                    TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                    index += status;
                    free_space -= status;
                    TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                }
                else
                {
                    switch (ddm_entry__out_type(ddm_entry))
                    {
                    case DDM2_TYPE_INT32_T:
                    {
                        int32_t int32_value;

                        int32_value = ddm_entry__value_i32(ddm_entry);
                        status = snprintf(&buffer[index], free_space, "\"%d\",", int32_value);
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        break;
                    }
                    case DDM2_TYPE_UINT32_T:
                    {
                        uint32_t uint32_value;

                        uint32_value = ddm_entry__value_u32(ddm_entry);
                        status = snprintf(&buffer[index], free_space, "\"%u\",", uint32_value);
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        break;
                    }
                    case DDM2_TYPE_STRING:
                    {
                        const char *str_value;

                        str_value = ddm_entry__value_str(ddm_entry);
                        status = snprintf(&buffer[index], free_space, "\"%s\",", str_value);
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        break;
                    }
                    case DDM2_TYPE_OTHER:
                    {
                        const void *other_value;
                        size_t other_value_size;

                        ddm_entry__read__value(ddm_entry, &other_value, &other_value_size);
                        // Encode the bytes as a hexadecimal string in JSON
                        const uint8_t *bytes = (const uint8_t *)other_value;
                        status = snprintf(&buffer[index], free_space, "\"");
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);

                        for (size_t j = 0; j < other_value_size; ++j)
                        {
                            status = snprintf(&buffer[index], free_space, "%02X", bytes[j]);
                            TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                            index += status;
                            free_space -= status;
                            TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        }
                        status = snprintf(&buffer[index], free_space, "\",");
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        break;
                    }
                    case DDM2_TYPE_STRUCT:
                    {
                        const void *struct_value;
                        size_t struct_value_size;

                        ddm_entry__read__value(ddm_entry, &struct_value, &struct_value_size);
                        // Encode the bytes as a hexadecimal string in JSON

                        const uint8_t *bytes = (const uint8_t *)struct_value;
                        status = snprintf(&buffer[index], free_space, "\"");
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);

                        for (size_t j = 0; j < struct_value_size; ++j)
                        {
                            status = snprintf(&buffer[index], free_space, "%02X", bytes[j]);
                            TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                            index += status;
                            free_space -= status;
                            TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        }
                        status = snprintf(&buffer[index], free_space, "\",");
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        break;
                    }
                    default:
                    {
                        status = snprintf(&buffer[index], free_space, "\"\",");
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                        index += status;
                        free_space -= status;
                        TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
                        break;
                    }
                    }
                }
            }
            else if (event_record_is_data_trigger(er, i))
            {
                int32_t trigger_value;

                trigger_value = event_record_get_trigger_data(er, i);
                status = snprintf(&buffer[index], free_space, "\"%d\",", trigger_value);
                TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
                index += status;
                free_space -= status;
                TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
            }
        }
        index--;  // Remove the trailing `,` symbol
    }
    status = snprintf(&buffer[index], free_space, "]}");
    TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_FAILURE, status >= 0);
    index += status;
    free_space -= status;
    TRUE_CHECK_RETURNX(MAKE_JSON_ERROR_NO_MEMORY, free_space > 0);
    return MAKE_JSON_NO_ERROR;
}
