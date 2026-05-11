#include "configuration.h"

#include "ddm_log_query_event_filter.h"
#include "ddm_log_query_parser.h"
#include "ddm_log_spec_parser.h"
#include "ddm_log_event.h"

int ddm_log_query_event_filter__header(const struct ddm_log_query_parser__query_data * qdata, const ddm_log_event__data_val_t value)
{
    if (qdata->timespan.start_date) // if timespan set
    {
        if (!ddm_log_query_event_filter__timespan(qdata, value.header.timestamp))
        {
            return 0;
        }
    }

    if (qdata->ddm_ids.numb_of_parameters) // if ddm_ids set
    {
        if (!ddm_log_query_event_filter__ddm_id(qdata, value.header.ddm_id))
        {
            return 0;
        }
    }

    return 1;
}

int ddm_log_query_event_filter__size_exceeded(const struct ddm_log_query_parser__query_data * qdata, const uint32_t size)
{
    if (qdata->size.usrSize)
    {
        if (qdata->size.usrSize < size)
        {
            return 1;
        }
    }

    return 0;
}

int ddm_log_query_event_filter__timespan(const struct ddm_log_query_parser__query_data * qdata, const uint32_t timestamp)
{
    return ((timestamp >= qdata->timespan.start_date) && (timestamp <= qdata->timespan.end_date));
}

int ddm_log_query_event_filter__ddm_id(const struct ddm_log_query_parser__query_data * qdata, const uint32_t ddm_id)
{
    for (int i = 0; i < qdata->ddm_ids.numb_of_parameters; i++)
    {
        if (qdata->ddm_ids.ddm_id[i] == ddm_id)
        {
            return 1;
        }
    }

    return 0;
}
