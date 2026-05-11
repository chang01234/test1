#include <stdint.h>

#include <string.h>
#include "configuration.h"

#include "hal_mem.h"
#include "hal_cpu.h"
#include "ddm_log.h"
#include "ddm_log_event.h"
#include "ddm_log_event_ram.h"
#include "ddm_log_event_flash.h"
#include "ddm_log_query_parser.h"
#include "ddm_log_query_event_filter.h"

static ddm_log_event__descriptor_t *descriptor[DDM_LOG_SPEC_PARSER__MEMORIES];

uint32_t ddm_log_event__init(uint32_t memory, uint32_t size)
{
    uint32_t size_s = 0;

    if (memory == DDM_LOG_SPEC_PARSER__RAM)
    {
        if ((descriptor[DDM_LOG_SPEC_PARSER__RAM] = ddm_log_event_ram__init(&size)) == NULL)
        {
            LOG(E, "Failed to allocate ram buffer");
        }
        size_s = size;
    }

    if (memory == DDM_LOG_SPEC_PARSER__FLASH)
    {
        if ((descriptor[DDM_LOG_SPEC_PARSER__FLASH] = ddm_log_event_flash__init(&size)) == NULL)
        {
            LOG(E, "Failed to allocate flash partition");
        }
    }

    return size_s;
}

int ddm_log_event__read(const uint32_t memory, struct ddm_log_query_parser__query_data * qdata, struct ddm_log_event__data ** events, uint32_t n_events)
{
	uint32_t n_entries = 0;

    if (!events)
    {
        return 0;
    }

    if (memory >= DDM_LOG_SPEC_PARSER__MEMORIES)
    {
        return 0;
    }

    if (descriptor[memory] == NULL)
    {
        return 0;
    }

    if (!ddm_log_event__size_exceeded(qdata))
    {
        while (n_entries < n_events)
        {
            struct ddm_log_event__data value;
            memset(&value, 0, sizeof(value));

            if (descriptor[memory]->funcs.ddm_log__all_entries_read(descriptor[memory]))
            {
                break;
            }

            if (descriptor[memory]->funcs.ddm_log__read(descriptor[memory], &value.val.header, ENTRY_HEADER_SIZE, ENTRY_HEADER) == 0)
            {
                break;
            }

            descriptor[memory]->read_pos += ENTRY_HEADER_SIZE;

            // if filter pass, read value
            if (ddm_log_query_event_filter__header(qdata, value.val))
            {
                struct ddm_log_event__data * event_log = ddm_log_event__create(ENTRY_VALUE_SIZE(&value));
                if (event_log == NULL)
                {
                    descriptor[memory]->read_pos -= ENTRY_HEADER_SIZE;
                    break;
                }

                // Copy header info
                memcpy(&event_log->val.header, &value.val.header, ENTRY_HEADER_SIZE);

                // Read value
                memset(&event_log->val.value, 0, ENTRY_VALUE_SIZE(&value));
                if (descriptor[memory]->funcs.ddm_log__read(descriptor[memory], &event_log->val.value, ENTRY_VALUE_SIZE(&value), ENTRY_VALUE) == 0)
                {
                    break;
                }

                events[n_entries] = event_log;
                n_entries++;
            }

            //move to next entry
            descriptor[memory]->read_pos += ENTRY_VALUE_SIZE(&value);
        }
    }

    if (n_entries == 0)
    {
        descriptor[memory]->funcs.ddm_log__set_read_position(descriptor[memory]);
    }

    return n_entries;
}

int ddm_log_event__write(uint32_t memory, struct ddm_log_event__data * data)
{
    if (!data)
    {
        return 0;
    }

    if (memory >= DDM_LOG_SPEC_PARSER__MEMORIES)
    {
        return 0;
    }

    if (descriptor[memory] == NULL)
    {
        return 0;
    }

    if (!descriptor[memory]->funcs.ddm_log__write(descriptor[memory], &data->val, ENTRY_SIZE(data)))
    {
        LOG(W, "Unable to store");
        return 0;
    }

    descriptor[memory]->write_pos += ENTRY_SIZE(data);

    return 1;
}

int ddm_log_event__size_exceeded(const struct ddm_log_query_parser__query_data * qdata)
{
    return ddm_log_query_event_filter__size_exceeded(qdata, qdata->size.cntSize);
}

struct ddm_log_event__data * ddm_log_event__create(size_t value_size)
{
    struct ddm_log_event__data *p_event_log = hal_mem_malloc_prefer(value_size + sizeof(struct ddm_log_event__data), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (p_event_log)
    {
        struct tm tm_time;
        p_event_log->val.header.magic_num = MAGIC_NUM;
        p_event_log->val.header.value_size = value_size;
        p_event_log->val.header.offset = 0;
        p_event_log->val.header.repeat = 0;
        p_event_log->val.header.timestamp = hal_cpu_get_millis();
        if (ddm_log__get_localtime(&tm_time))
        {
            p_event_log->val.header.timestamp = mktime(&tm_time);
        }
    }
    return p_event_log;
}

void ddm_log_event__free(struct ddm_log_event__data * event_log)
{
    if (event_log)
    {
        hal_mem_free(event_log);
    }
}
