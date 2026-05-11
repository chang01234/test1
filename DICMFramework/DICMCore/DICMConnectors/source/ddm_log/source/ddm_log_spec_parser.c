#include <string.h>

#include "esp_heap_caps.h"
#include "argtable3/argtable3.h"
#include "ctype.h"

#include "ddm2.h"
#include "configuration.h"

#include "ddm_log.h"
#include "ddm_log_spec_parser.h"

#define SPEC_PARSER_MAX_ARGUMENTS 16

typedef struct TOKEN
{
	char *string;
	DDM_LOG_SPEC_PARSER_COMPARISON_TOKEN id;
} TOKEN;

static const TOKEN Comparisons[] =
{
	{"==",		COMPARISON_EQ},
	{"!=",		COMPARISON_NEQ},
	{"<",		COMPARISON_LT},
	{"<=",		COMPARISON_LTEQ},
	{">",		COMPARISON_GT},
	{">=",		COMPARISON_GTEQ},
};

typedef enum PARSE_DATA_STRING_RESULT
{
	PARSE_DATA_STRING_ERROR = -1,
	PARSE_DATA_STRING_DATA = 0,
	PARSE_DATA_STRING_LITERAL = 1,
} PARSE_DATA_STRING_RESULT;

static void * argtable[9];	// global argument table
static struct arg_str *option_parameter, *option_destination, *option_log_interval, *option_sample_interval, *option_condition, *option_hysteresis;
static struct arg_str *option_filter, *option_time;
static struct arg_end *option_end;

// parse an interval string
static uint32_t parse_interval(const char * const interval_string)
{
	uint32_t interval = 0;
	uint32_t field = 0;
	const char * ptr = interval_string;
	int alpha = 1;

	while (*ptr)
	{
		if (isdigit(*ptr))			// currently in number field
		{
			if (alpha)				// first digit of a new field
			{
				interval += field;  // add the previous field to total interval
				field = 0;          // reset the field
				alpha = 0;          // no longer in an alphabetic field
			}
			field = field * 10 + (*ptr - '0');  // convert the character to an integer
		}
		else
		{
			if (alpha)  // no consecutive alphabetic characters
			{
				return 0;
			}
			alpha = 1;
			switch (*ptr)
			{
			/*case 'Y':
				field *= 60 * 60 * 24 * 7 * 30 * 365;    //note: this will overflow 32 bits
				break;*/
			case 'M':   // month
				field *= 60 * 60 * 24 * 7 * 30;
				break;
			case 'W':   // week
				field *= 60 * 60 * 24 * 7;
				break;
			case 'd':   // day
				field *= 60 * 60 * 24;
				break;
			case 'h':   // hour
				field *= 60 * 60;
				break;
			case 'm':   // minute
				field *= 60;
				break;
			case 's':   // second
				break;
			default:    // invalid character
				return 0;
			}
		}

		ptr++;
	}

	return interval + field; // add the last field
}

// match a string to a token
static DDM_LOG_SPEC_PARSER_COMPARISON_TOKEN match_string(void * token_string, const TOKEN * const token_list, size_t token_list_size)
{
	for (size_t n = 0 ; n < token_list_size ; n++)
	{
		if (!strcmp(token_string, token_list[n].string))    // compare the string to the token list
		{
			return token_list[n].id;    // return the corresponding token if a match is found
		}
	}

	return COMPARISON_NONE;  // return COMPARISON_NONE if no match is found
}

static int parse_data_string(const char * parameter_data_string, uint32_t * const parameter, int * const offset, int * const size, int * const literal)
{
	*offset = 0;    // default offset is 0
	*size = 4;      // default size is 4

	const int Valid_literal = sscanf(parameter_data_string, "%d", literal); // check if the data string is a literal

	if (Valid_literal)
	{
		return PARSE_DATA_STRING_LITERAL;   // if it was a literal, we are done
	}

	char parameter_string[32];
	const int Valid_fields = sscanf(parameter_data_string, "%[^:]:%d:%d", parameter_string, offset, size);  // parse the data string

	if (Valid_fields < 1)   // no fields successfully parsed, need at least one; the parameter
	{
		return PARSE_DATA_STRING_ERROR;
	}

	int parameter_parse_result = ddm2_parse_parameter_string(parameter, parameter_string, sizeof(parameter_string));    // look up the parameter string

	if (parameter_parse_result < 0) // no valid parameter found
	{
		return PARSE_DATA_STRING_ERROR;
	}

	return PARSE_DATA_STRING_DATA;
}

static int parse_condition(const char * condition_string, char * lhs, DDM_LOG_SPEC_PARSER_COMPARISON_TOKEN * comparison, char * rhs)
{
	char comparison_string[32];

	const int Valid_fields = sscanf(condition_string, "%[^=!<>]%[=!<>]%s", lhs, comparison_string, rhs);    // parse the condition string

	if (Valid_fields < 3)   // not enough fields provided, all three are mandatory
	{
		return -1;
	}

	*comparison = match_string(comparison_string, Comparisons, ELEMENTS(Comparisons));  //match the comparison function string to a comparison function token

	if (*comparison == COMPARISON_NONE)  // no valid comparison function found
	{
		return -1;
	}

	return 0;
}

static int parse_hysteresis(const char * hysteresis_string, int * lower, int * upper)
{
	const int Valid_fields = sscanf(hysteresis_string, "%d:%d", lower, upper);  // parse the hysteresis string

	if (Valid_fields < 1)   // no fields provided
	{
		return -1;
	}

	if (Valid_fields < 2)   // use same value for both if only one field was provided
	{
		*upper = *lower;
	}

	return 0;
}

static int parse_time_string(const char *time_string, uint32_t *start, uint32_t *end)
{
    char tmpstr[strlen(time_string) + 1];
    strcpy(tmpstr, time_string);
    char *save_ptr;
    char *check;
    check = strstr(tmpstr, ":");
    if (NULL == check)
    {
        LOG(E, "Found no : in time string");
        return -1;
    }
    char *token;
    if (check == tmpstr)
    {
        token = NULL;
    }
    else
    {
        token = strtok_r(tmpstr, ":", &save_ptr);
    }
    if (token == NULL)
    {
        *start = 0;
        token = tmpstr;
    }
    else
    {
        *start = parse_interval(token);
        token = NULL;
    }
    char *token2 = strtok_r(token, ":", &save_ptr);
    if ((NULL == token2) && (*start == 0))
    {
        return -1;
    }
    else if (NULL == token2)
    {
        *end = 0;
    }
    else
    {
        *end = parse_interval(token2);
    }
    return 0;
}

// callback function for the specification parser
static int parse_cb(ddm_log_specification_data_t * const p_specification_data)
{
	TRUE_CHECK_RETURN0(p_specification_data);

	memset(p_specification_data, 0, sizeof(ddm_log_specification_data_t)); // clear the specification data

	uint32_t parameter;
	int offset, size, literal;

	int parameter_index = parse_data_string(*option_parameter->sval, &parameter, &offset, &size, &literal); // parse the parameter data string

	if (parameter_index != PARSE_DATA_STRING_DATA)  // can't have a literal as a parameter
	{
		LOG(E, "No valid parameter data");
		return -1;
	}

	p_specification_data->data.parameter = parameter;
	p_specification_data->data.offset = offset;
	p_specification_data->data.size = size;

	if (option_destination->count > 0)
	{
	    p_specification_data->destination = *option_destination->sval[0] == 'f' ? DDM_LOG_DESTINATION_FLASH : DDM_LOG_DESTINATION_RAM;
	}
	else
	{
	    // Default to No logging destination. Log read configs as part of log configs would still be working
	    p_specification_data->destination = DDM_LOG_DESTINATION_NONE;
	}

	if (option_log_interval->count)
	{
		const uint32_t Interval = parse_interval(*option_log_interval->sval);		// parse the log interval
		p_specification_data->log_interval = Interval;
	}

	if (option_sample_interval->count)
	{
		const uint32_t Interval = parse_interval(*option_sample_interval->sval);	// parse the sample interval
		p_specification_data->sample_interval = Interval;
	}


	p_specification_data->num_conditions = option_condition->count;
	assert(p_specification_data->num_conditions <= DDM_LOG_SPEC_PARSER_MAX_NUM_CONDITIONS);
	if (option_condition->count)
	{
	    // Loop all possible conditions
	    char lhs[32], rhs[32];
	    DDM_LOG_SPEC_PARSER_COMPARISON_TOKEN comparison;
	    int num_conditions = 0;
	    while (num_conditions < option_condition->count)
	    {
	        if (parse_condition(option_condition->sval[num_conditions], lhs, &comparison, rhs))        // parse the condition
	        {
	            LOG(E, "Condition parse error");
	            return -1;
	        }
	        p_specification_data->conditions[num_conditions].comparison = comparison;
	        uint32_t lhs_parameter, rhs_parameter;
	        int lhs_offset, lhs_size, lhs_literal, rhs_offset, rhs_size, rhs_literal;

	        const PARSE_DATA_STRING_RESULT Lhs_parse_result = parse_data_string(lhs, &lhs_parameter, &lhs_offset, &lhs_size, &lhs_literal); // parse the LHS

	        switch (Lhs_parse_result)
	        {
	        case PARSE_DATA_STRING_ERROR:
	            LOG(E, "LHS parse error");
	            return -1;
	            break;
	        case PARSE_DATA_STRING_DATA:    // string was a parameter
	            p_specification_data->conditions[num_conditions].lhs.parameter = lhs_parameter;
	            p_specification_data->conditions[num_conditions].lhs.offset = lhs_offset;
	            p_specification_data->conditions[num_conditions].lhs.size = lhs_size;
	            break;
	        case PARSE_DATA_STRING_LITERAL: // string was a literal
	            p_specification_data->conditions[num_conditions].lhs.parameter = DDMP2_INVALID_PARAMETER;
	            p_specification_data->conditions[num_conditions].lhs.literal = lhs_literal;
	            break;
	        }

	        const PARSE_DATA_STRING_RESULT Rhs_parse_result = parse_data_string(rhs, &rhs_parameter, &rhs_offset, &rhs_size, &rhs_literal); // parse the RHS

	        switch (Rhs_parse_result)
	        {
	        case PARSE_DATA_STRING_ERROR:
	            LOG(E, "RHS parse error");
	            return -1;
	            break;
	        case PARSE_DATA_STRING_DATA:    // string was a parameter
	            p_specification_data->conditions[num_conditions].rhs.parameter = rhs_parameter;
	            p_specification_data->conditions[num_conditions].rhs.offset = rhs_offset;
	            p_specification_data->conditions[num_conditions].rhs.size = rhs_size;
	            break;
	        case PARSE_DATA_STRING_LITERAL: // string was a literal
	            p_specification_data->conditions[num_conditions].rhs.parameter = DDMP2_INVALID_PARAMETER;
	            p_specification_data->conditions[num_conditions].rhs.literal = rhs_literal;
	            break;
	        }
	        num_conditions++;
	    }

	}

	int lower, upper;

	if (option_hysteresis->count)
	{
		if (parse_hysteresis(*option_hysteresis->sval, &lower, &upper)) // parse the hysteresis
		{
			LOG(E, "Hysteresis parse error");
			return -1;
		}

		p_specification_data->hysteresis.lower = lower;
		p_specification_data->hysteresis.upper = upper;
	}
	if (option_time->count > 0)
	{
	    LOG(E, "Parse error: Time not a valid option");
	    return -1;
	}
    if (option_filter->count > 0)
    {
        LOG(E, "Parse error: Filter not a valid option");
        return -1;
    }
	return 0;
}

static int parse_read_config_cb(ddm_log_read_specification_data_t * const p_specification_data)
{
    TRUE_CHECK_RETURN0(p_specification_data);

    uint32_t parameter;
    int offset, size, literal;

    int parameter_index = parse_data_string(*option_parameter->sval, &parameter, &offset, &size, &literal); // parse the parameter data string

    if (parameter_index != PARSE_DATA_STRING_DATA)  // can't have a literal as a parameter
    {
        LOG(E, "No valid parameter data");
        return -1;
    }

    p_specification_data->data.parameter = parameter;
    p_specification_data->data.offset = offset;
    p_specification_data->data.size = size;

    if (option_time->count > 0)
    {
        uint32_t start = 0;
        uint32_t end = 0;
        if (parse_time_string(*option_time->sval, &start, &end))
        {
            LOG(E, "Failed to parse time option");
            return -1;
        }
        p_specification_data->log_read_time.start = start;
        p_specification_data->log_read_time.end = end;
        LOG(D, "time_start: %d time_end: %d", start, end);
    }
    else
    {
        LOG(E, "We need to specify time option");
        return -1;
    }
    if (option_filter->count > 0)
    {
        uint32_t filter_value = parse_interval(*option_filter->sval);
        p_specification_data->log_read_time.interval = filter_value;
        LOG(D, "filter_value(interval): %d", filter_value);
    }
    else
    {
        // We use default from logconfig
        p_specification_data->log_read_time.interval = p_specification_data->p_logconfig->log_interval;
        ASSERT(p_specification_data->log_read_time.interval);
        LOG(D, "filter_value(interval): %d", p_specification_data->log_read_time.interval);
    }
    // Allocate required buffers
    p_specification_data->data_buffer_length = (p_specification_data->log_read_time.end - p_specification_data->log_read_time.start) / p_specification_data->log_read_time.interval;
    ASSERT(((p_specification_data->log_read_time.end - p_specification_data->log_read_time.start) % p_specification_data->log_read_time.interval) == 0);
    LOG(D, "Allocating %d buffers", p_specification_data->data_buffer_length);
    p_specification_data->data_buffer = (int32_t*)hal_mem_malloc_prefer(sizeof(int32_t) * p_specification_data->data_buffer_length, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    TRUE_CHECK_RETURN1(p_specification_data->data_buffer != NULL);
    for (int i = 0; i < p_specification_data->data_buffer_length; ++i)
    {
        p_specification_data->data_buffer[i] = INT32_MIN;
    }
    return 0;
}

// Prepare specification parsing engine
int ddm_log_spec_parser__prepare(void)
{
	argtable[0] = option_parameter = arg_str1(NULL, NULL, "parameter[:offset[:size]]", NULL);   // define the specification line grammar
	argtable[1] = option_destination = arg_str0("d", "destination", "r|f", NULL);
	argtable[2] = option_log_interval = arg_str0("i", "interval", NULL, NULL);
	argtable[3] = option_sample_interval = arg_str0("s", "sample interval", NULL, NULL);
	argtable[4] = option_condition = arg_strn("c", "condition", "XcondY", 0, 2, NULL);
	argtable[5] = option_hysteresis = arg_str0("h", "hysteresis", "lower[:upper]", NULL);
    argtable[6] = option_filter = arg_str0("f", "filter", NULL, NULL);
    argtable[7] = option_time = arg_str0("t", "time", "<start>:<end>", NULL);
	argtable[8] = option_end = arg_end(20);

	if (arg_nullcheck(argtable))    // check for memory allocation errors during argument table creation
	{
		printf("insufficient memory\n");
		arg_freetable(argtable, ELEMENTS(argtable));

		return 1;
	}

	//arg_print_syntaxv(stdout, argtable, "\n"); // print the syntax of the specification line

	return 0;
};

static int split_argv(char ** argv, char * parse_string, const int max_args)
{
	int i = 0;
	char *pch;
	char *saveptr;
	pch = strtok_r(parse_string, " ", &saveptr);

	while (pch != NULL && i < max_args)
	{
		argv[i] = pch;
		pch = strtok_r(NULL, " ", &saveptr);
		i++;
	}

	argv[i] = NULL;
	return i;
}

int ddm_log_spec_parser__parse(ddm_log_specification_data_t * const p_specification_data, char *p_option_string)
{
	char *argv[SPEC_PARSER_MAX_ARGUMENTS];
	char tmp_option_string[200];    // Need a temp buffer as split_argv() destroys string

	strcpy(tmp_option_string, p_option_string);
	argv[0] = "";   // empty command name
	//size_t argc = 1 + esp_console_split_argv(option_string, &argv[1], SPEC_PARSER_MAX_ARGUMENTS);   // split the specification line into an argc/argv pair
	size_t argc = 1 + split_argv(&argv[1], tmp_option_string, SPEC_PARSER_MAX_ARGUMENTS);   // split the specification line into an argc/argv pair

	const int errors = arg_parse(argc, argv, argtable); // Parse the split specification line

	if (errors > 0)
	{
		arg_print_errors(stdout, option_end, "");   // If there were errors, print them

		return 1;
	}

	return parse_cb(p_specification_data);  // If there were no errors, call the callback function
}

int ddm_log_read_spec_parser__parse(ddm_log_read_specification_data_t * const p_specification_data, char *p_option_string)
{
    char *argv[SPEC_PARSER_MAX_ARGUMENTS];
    char tmp_option_string[200];    // Need a temp buffer as split_argv() destroys string

    strcpy(tmp_option_string, p_option_string);
    argv[0] = "";   // empty command name
    //size_t argc = 1 + esp_console_split_argv(option_string, &argv[1], SPEC_PARSER_MAX_ARGUMENTS);   // split the specification line into an argc/argv pair
    size_t argc = 1 + split_argv(&argv[1], tmp_option_string, SPEC_PARSER_MAX_ARGUMENTS);   // split the specification line into an argc/argv pair

    const int errors = arg_parse(argc, argv, argtable); // Parse the split specification line

    if (errors > 0)
    {
        arg_print_errors(stdout, option_end, "");   // If there were errors, print them

        return 1;
    }

    return parse_read_config_cb(p_specification_data);  // If there were no errors, call the callback function

}

int ddm_log_spec_parser__parse_activate(struct ddm_log_spec_parser__spec_data * spec_data, const uint32_t activate)
{
	return spec_data != NULL ? spec_data->activate = activate : 0;
}

int ddm_log_spec_parser__parse_ddm_storage(struct ddm_log_spec_parser__spec_data * spec_data, const ddm_log_spec_parser__spec_storage_t storage)
{
	int isValid = 1;

	TRUE_CHECK_RETURN0(spec_data);

	switch (storage)
	{
	case DDM_LOG_SPEC_PARSER__RAM:
		spec_data->storage = DDM_LOG_SPEC_PARSER__RAM;
		break;
	case DDM_LOG_SPEC_PARSER__FLASH:
		spec_data->storage = DDM_LOG_SPEC_PARSER__FLASH;
		break;
	default:
		isValid = 0;
		break;
	}

	return isValid;
}

int ddm_log_spec_parser__parse_ddm_filter(struct ddm_log_spec_parser__spec_data * spec_data, const char * filter, size_t size)
{
	 return 1;
}
