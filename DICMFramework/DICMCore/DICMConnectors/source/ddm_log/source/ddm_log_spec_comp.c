#include <string.h>

#include "configuration.h"

#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "ddm_log_spec_comp.h"
#include "ddm_log_spec_parser.h"

char log_spec_item[DDMP2_MAX_VALUE_SIZE];

int ddm_log_spec_composer__comp_ddm_id(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len)
{
    size_t log_spec_item_size = sizeof(log_spec_item);

    TRUE_CHECK_RETURNX(-1, spec_data);
    TRUE_CHECK_RETURNX(-1, len);

    ZERO(log_spec_item);
    *value = log_spec_item;

    ddm2_parameter_name(spec_data->ddm_id, *value, &log_spec_item_size);
    *len = log_spec_item_size + 1;

    return 1;
}

int ddm_log_spec_composer__comp_ddm_storage(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len)
{
    TRUE_CHECK_RETURNX(-1, spec_data);
    TRUE_CHECK_RETURNX(-1, len);

    ZERO(log_spec_item);
    *value = log_spec_item;

    if (spec_data->storage == DDM_LOG_SPEC_PARSER__RAM)
    {
        strcpy(*value, "RAM");
    }
    else if (spec_data->storage == DDM_LOG_SPEC_PARSER__FLASH)
    {
        strcpy(*value, "FLASH");
    }

    *len = strlen(*value) + 1;

    return 1;
}

static void compose_timeunit(const uint32_t interval, char * str, char ** startPtr, char * endPtr)
{
    int numbOfChar;

    if (*startPtr < endPtr)
    {
        numbOfChar = snprintf(*startPtr, endPtr - *startPtr, str, interval);
        *startPtr += numbOfChar;
    }
}

int ddm_log_spec_composer__comp_ddm_interval(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len)
{
    char *ptr;
    char *endPtr;

    TRUE_CHECK_RETURNX(-1, spec_data);
    TRUE_CHECK_RETURNX(-1, len);

    ZERO(log_spec_item);
    ptr = *value = log_spec_item;
    endPtr = *value + sizeof(log_spec_item) - 1;

    if (spec_data->interval.years)    compose_timeunit(spec_data->interval.years,   "%dY ",    &ptr, endPtr);
    if (spec_data->interval.months)   compose_timeunit(spec_data->interval.months,  "%dM ",    &ptr, endPtr);
    if (spec_data->interval.weeks)    compose_timeunit(spec_data->interval.weeks,   "%dW ",    &ptr, endPtr);
    if (spec_data->interval.days)     compose_timeunit(spec_data->interval.days,    "%dd ",    &ptr, endPtr);
    if (spec_data->interval.hours)    compose_timeunit(spec_data->interval.hours,   "%dh ",    &ptr, endPtr);
    if (spec_data->interval.minutes)  compose_timeunit(spec_data->interval.minutes, "%dmin ",  &ptr, endPtr);
    if (spec_data->interval.secs)     compose_timeunit(spec_data->interval.secs,    "%dsec ",  &ptr, endPtr);

    *len = MIN((uint32_t)(ptr - *value), (uint32_t)(sizeof(log_spec_item) - 1));

    return 1;
}

int ddm_log_spec_composer__comp_ddm_filter(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len)
{
    TRUE_CHECK_RETURNX(-1, spec_data);
    TRUE_CHECK_RETURNX(-1, len);

    ZERO(log_spec_item);
    *value = log_spec_item;

    return 1;
}
