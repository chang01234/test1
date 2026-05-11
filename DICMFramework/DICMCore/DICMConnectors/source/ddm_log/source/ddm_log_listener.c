/*
 * ddm_log_listener.c
 *
 */
/**
 * \file    ddm_log_listener.c
 * \brief   DDM Log implementation.
 *
 * \{
 */

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "ddm2.h"
#include "utils.h"
#include "configuration.h"

#include "ddm_log.h"
#include "ddm_log_event.h"
#include "ddm_log_spec_parser.h"
#include "ddm_log_listener.h"

//static int filter_new_value(struct ddm_entry * entry, const void * value, size_t size);
//static int filter_timeinterval(struct ddm_log__spec * spec);

struct ddm_log_event__data * ddm_log_listener__new_entry(struct ddm_log__spec * spec, const void * value, size_t size)
{
    struct ddm_log_event__data * event = NULL;

    TRUE_CHECK_RETURNX(NULL, spec);
    TRUE_CHECK_RETURNX(NULL, spec->data);
    TRUE_CHECK_RETURNX(NULL, value);

    // Already set
#if 0
    if (!filter_new_value(spec->spec_entry, value, size))
    {
        return NULL;
    }
    if (!filter_timeinterval(spec))
    {
        return NULL;
    }
#endif

    if (!ddm_log_listener__create_event(&event, spec, value, size))
    {
        return NULL;
    }

    return event;
}

/*static int filter_new_value(struct ddm_entry * entry, const void * value, size_t size)
{
    int differ = 0;
    struct ddm_entry new_entry;

    // create default ddm entry for same parameter entry
    ddm_entry__init(&new_entry, ddm_entry__parameter_id(entry));
    switch (ddm_entry__out_type(entry))
    {
    case DDM2_TYPE_INT32_T:
    {
        ddm_entry__set__value_i32(&new_entry, *(uint32_t *)value);
        if (ddm_entry__value_i32(entry) != ddm_entry__value_i32(&new_entry))
        {
            differ = 1;
        }
        break;
    }
    case DDM2_TYPE_STRING:
    {
        ddm_entry__set__value_str(&new_entry, (const char *)value, size);
        if (strcmp(ddm_entry__value_str(entry), ddm_entry__value_str(&new_entry)) != 0)
        {
            differ = 1;
        }
        break;
    }
    case DDM2_TYPE_OTHER:
    case DDM2_TYPE_VOID:
        differ = 1;
        break;
    default:
        break;
    }

    // free memory
    ddm_entry__delete__value(&new_entry);

    return differ;
}*/

/*static int filter_timeinterval(struct ddm_log__spec * spec)
{
    struct tm time_tm;

    // time interval has not been set, allow storing the log event
    if (is_zero(&spec->data->interval, sizeof(spec->data->interval)))
    {
        return 1;
    }

    if (ddm_log__get_localtime(&time_tm))
    {
        if (difftime(spec->time_interval, mktime(&time_tm)) <= 0)
        {
            return 1;
        }
        else
        {
            LOG(I, "Time left until next log event[%ds]", (uint32_t)difftime(spec->time_interval, mktime(&time_tm)));
        }
    }

    return 0;
}*/

int ddm_log_listener__create_event(struct ddm_log_event__data ** event_data, struct ddm_log__spec * spec, const void * value, size_t size)
{
    struct tm time_tm;

    *event_data = ddm_log_event__create(size);
    if (*event_data)
    {
        if (ddm_log__get_localtime(&time_tm))
        {
            (*event_data)->val.header.magic_num = MAGIC_NUM;
            (*event_data)->val.header.ddm_id = spec->data->ddm_id;
            (*event_data)->val.header.offset = 0;
            (*event_data)->val.header.repeat = 0;
            (*event_data)->val.header.timestamp = mktime(&time_tm);

            (*event_data)->val.header.value_size = size;
            memcpy((*event_data)->val.value, value, size);

            return 1;
        }
    }

    return 0;
}
