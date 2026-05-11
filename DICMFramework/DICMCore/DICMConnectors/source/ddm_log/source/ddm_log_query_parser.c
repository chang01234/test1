#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "ddm2.h"
#include "configuration.h"

#include "ddm_log_query_parser.h"


/* Private members */
#define KB                    1024
#define MB                    ( KB * 1024 )
#define NUMB_OF_CMDS          5
#define MAX_CMD_LEN           3
#define MAX_CMD_VALUE_LEN     DDMP2_MAX_VALUE_SIZE

struct command_definition
{
    char cmd[MAX_CMD_LEN]; // Command string
    int (*cmd_function)(struct ddm_log_query_parser__query_data * query_data, void *);  // Command execution function
};

struct commands
{
    struct command_definition * command;
    char value[MAX_CMD_VALUE_LEN]; // command argument
};

static int parse_size(struct ddm_log_query_parser__query_data * query_data, void * size);
static int parse_ddm_ids(struct ddm_log_query_parser__query_data * query_data, void * ddm_ids);
static int parse_timespan(struct ddm_log_query_parser__query_data * query_data, void * timestamp);

static int parse_commands(char * command, struct commands * cmdStr, size_t * numbOfCmds);


// Supported commands
struct command_definition cmds[] =
{
    { .cmd = "-s", .cmd_function = parse_size },
    { .cmd = "-d", .cmd_function = parse_ddm_ids },
    { .cmd = "-t", .cmd_function = parse_timespan }
};

static int parse_size(struct ddm_log_query_parser__query_data * query_data, void * sz)
{
    char * size = (char *)sz;

    // Find integral number
    char *ptr;
    long val = strtol(size, &ptr, 10);
    if (!val)
    {
        return -1;
    }

    long bytesValue = 0;
    if (!strcmp(ptr, "B"))
    {
        bytesValue = val;
    }
    else if (!strcmp(ptr, "kB"))
    {
        bytesValue = val * KB;
    }
    else if (!strcmp(ptr, "MB"))
    {
        bytesValue = val * MB;
    }
    else // unhandled string
    {
        return 0;
    }

    query_data->size.usrSize = bytesValue;

    return 1;
}

static int parse_ddm_ids(struct ddm_log_query_parser__query_data * query_data, void * ddm_ids)
{
    char parameter_name[DDM_LOG__PARAM_MAX_NAME_LEN];
    int numbOfParamters = 0;
    int maxNumbOfParamters = 0;
    char * parameters = (char *)ddm_ids;
    char * readParameters = NULL;

    maxNumbOfParamters = ELEMENTS(query_data->ddm_ids.ddm_id);

    while ((readParameters = strtok_r(parameters, ",", &parameters)))
    {
        memset(parameter_name, 0, sizeof(parameter_name));

        int length = strnlen(readParameters, sizeof(parameter_name) - 1);
        strncpy(parameter_name, readParameters, length);
        parameter_name[length] = 0;

        if (numbOfParamters >= maxNumbOfParamters)
        {
            return 0;
        }

        int index = ddm2_parse_parameter_string(&query_data->ddm_ids.ddm_id[numbOfParamters++], parameter_name, length);
        if (index == -1)
        {
            return 0;
        }
    }

    query_data->ddm_ids.numb_of_parameters = numbOfParamters;

    return 1;
}

static int parse_timespan(struct ddm_log_query_parser__query_data * query_data, void * timespan)
{
    time_t time;
    struct tm timeinfo;
    char date[DDMP2_MAX_VALUE_SIZE];
    char * ts = (char *)timespan;

    // delimiter
    char *pos = strchr(ts, '=');
    if (pos != NULL)
    {
        char * startDate = ts;
        char * endDate = pos + 1;

        memcpy(date, startDate, pos - startDate);
        if (strptime(date, "%Y-%m-%d:%H:%M:%S", &timeinfo) == NULL)
        {
            return 0;
        }

        time = mktime(&timeinfo);
        if (time == -1)
        {
            return 0;
        }

        query_data->timespan.start_date = time;

        memset(date, 0, sizeof(date));
        memcpy(date, endDate, startDate + strlen(ts) - endDate);
        if (strptime(date, "%Y-%m-%d:%H:%M:%S", &timeinfo) == NULL)
        {
            return 0;
        }

        time = mktime(&timeinfo);
        if (time == -1)
        {
            return 0;
        }

        query_data->timespan.end_date = time;

        return 1;
    }

    return 0;
}

/**
 * @brief
 *
 * @param command           Command string sent as parameter input
 * @param cmdStr            Array of commands that will be populated with parsed command's values
 * @param numbOfCmds        Maximum number of cmds to be parsed. Will be updated with actual commands
 *                          parsed by the function
 *
 * \pre     Parameter \a command must be a non-NULL pointer
 * \pre     Parameter \a cmdStr must be a non-NULL pointer
 * \pre     Parameter \a numbOfCmds must be a non-NULL pointer and its value must be > 0
 * @return int
 */
static int parse_commands(char * command, struct commands * cmdStr, size_t * numbOfCmds)
{
	size_t argc = 0;
    char * readCommand = NULL;

    readCommand = strtok(command, " ");
    while (readCommand != NULL)
    {
        // exceed maximum number of allowed commands
        if (argc >= *numbOfCmds)
        {
            return 0;
        }

        // command is marked with '-'
        if (readCommand[0] != '-')
        {
            return 0;
        }

        int len = strlen(readCommand);
        if (len <= 1 || len > MAX_CMD_VALUE_LEN)
            return 0;

        // validate cmd
        int isValid = 0;
        for (unsigned int sp_cmds = 0; sp_cmds < sizeof(cmds) / sizeof(cmds[0]); sp_cmds++)
        {
            if (strcmp(readCommand, cmds[sp_cmds].cmd) == 0)
            {
                // parse argument
                readCommand = strtok(NULL, " ");
                if (readCommand != NULL)
                {
                    // now set the parameter argument and corresponding parse function
                    if (readCommand[0] != '-')
                    {
                        // Save command
                        cmdStr[argc].command = &cmds[sp_cmds];
                        strncpy(cmdStr[argc].value, readCommand, strnlen(readCommand, sizeof(cmdStr[argc].value)));

                        argc++;
                        isValid = 1;
                    }
                }
                break;
            }
        }

        // command does not exist
        if (!isValid)
        {
            return 0;
        }

        //next command
        readCommand = strtok(NULL, " ");
    }

    // update number of valid commands
    *numbOfCmds = argc;

    return argc;
}

int ddm_log_query_parser__parse(struct ddm_log_query_parser__query_data * query_data, char * query_str)
{
    int isValid = 0;
    size_t argc;
    struct commands argv[NUMB_OF_CMDS];
    char query[DDMP2_MAX_VALUE_SIZE];

    TRUE_CHECK_RETURNX(0, query_data);
    TRUE_CHECK_RETURNX(0, query_str);

    memset(query_data, 0, sizeof(*query_data));

    argc = ELEMENTS(argv);
    memset(argv, 0, sizeof(argv));
    memset(query, 0, sizeof(query));

    // Create temporary interval string
    memcpy(query, query_str, MIN(sizeof(query), strlen(query_str)));

    // parse and validate commands
    isValid = parse_commands(query, argv, &argc);
    if (isValid)
    {
        // parse and validate commands argument
        for (size_t optind = 0; optind < argc; optind++)
        {
            struct command_definition * cmd = argv[optind].command;
            if (!cmd || (cmd->cmd_function(query_data, argv[optind].value) == 0))
            {
                isValid = 0;
                // set to empty string, since it's invalid
                memset(query_str, 0, strlen(query_str));
                break;
            }
        }
    }

    return isValid;
}
