#ifndef DDM_LOG_SPEC_PARSER_H_
#define DDM_LOG_SPEC_PARSER_H_

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#define DDM_LOG__PARAM_MAX_NAME_LEN		32
#define DDM_LOG_SPEC_PARSER_MAX_NUM_CONDITIONS     2

/* Forward declaration */

typedef enum ddm_log_spec_parser__spec_storage
{
    DDM_LOG_SPEC_PARSER__RAM,
    DDM_LOG_SPEC_PARSER__FLASH,
    DDM_LOG_SPEC_PARSER__MEMORIES,
} ddm_log_spec_parser__spec_storage_t;

/**
 * @brief Keeps the interval information set by user.
 *
 * Possible format: 20d; 30h 15min; 5h 65min; 5m 2d etc.
 *
 */
struct ddm_log_spec_parser__interval
{
    uint32_t secs;
    uint32_t minutes;
    uint32_t hours;
    uint32_t days;
    uint32_t weeks;
    uint32_t months;
    uint32_t years;
    bool valid;
};

enum ddm_log_spec_parser__time_unit
{
    DDM_LOG_SPEC_PARSER__TU_NONE,
    DDM_LOG_SPEC_PARSER__TU_HOUR,
    DDM_LOG_SPEC_PARSER__TU_DAY,
    DDM_LOG_SPEC_PARSER__TU_WEEK,
    DDM_LOG_SPEC_PARSER__TU_MONTH,
    DDM_LOG_SPEC_PARSER__TU_YEAR
};

/**
 * @brief Retention
 *
 * Store max number of log events specified with \a frequency
 * in the selected \a time_unit, and keep them for the selected
 * \a retention_period. When the \a retenetion_period passes, new
 * set of log events can be stored.
 *
 * Possible format: 20d#3, 15h#5, 1y#20
 *
 * - time_unit: year/month/week/day/hour
 * - frequency: sampling frequency (number of samples per \a time_unit)
 * - retention_period: in selected \a time_unit; year/month/week/day/hour
 *
 * Example: 20d#3 - Each day store max up to 5 log events,
 *                   and keep them for 20 days
 */
struct ddm_log_spec_parser__retention
{
    enum ddm_log_spec_parser__time_unit time_unit;
    uint32_t frequency;
    uint32_t retention_period;
};

/**
 * @brief Log Specificaiton data information for each DDM
 *        parameter stored in flash
 *
 * - ddm_id      Parameter ID
 * - activate    If the log specification is activated or not
 * - storage     Destination where the log events will be stored
 * - interval    Criteria for when the log events should be stored
 * - filter      Input filtering rules
 *
 */
struct ddm_log_spec_parser__spec_data
{
    uint32_t ddm_id;
    uint32_t activate;
    ddm_log_spec_parser__spec_storage_t storage;
    struct ddm_log_spec_parser__interval interval;
    //struct spec_parser__filter filter;
    //struct spec_parser__condition condition;
};

typedef enum DDM_LOG_SPEC_PARSER_COMPARISON_TOKEN
{
    COMPARISON_NONE = 0, COMPARISON_EQ, COMPARISON_NEQ, COMPARISON_LT, COMPARISON_LTEQ, COMPARISON_GT, COMPARISON_GTEQ,
} DDM_LOG_SPEC_PARSER_COMPARISON_TOKEN;

typedef enum ddm_log_destination_t
{
    DDM_LOG_DESTINATION_FLASH,
    DDM_LOG_DESTINATION_RAM,
    DDM_LOG_DESTINATION_NONE
} ddm_log_destination_t;

typedef struct ddm_log_data_t
{
    uint32_t parameter;										// parameter id, ffffffff if literal
    int literal;
    int offset;
    int size;
} ddm_log_data_t;

typedef struct ddm_log_specification_data_t
{
    int instance;                                           // Corresponding class instance (LSCFG<X>)
    char name[DDM_LOG__PARAM_MAX_NAME_LEN];					// log specification name
    ddm_log_data_t data;									// parameter data to log (required)
    ddm_log_destination_t destination;						// log storage medium (default: flash)
    int log_interval;										// log interval in seconds (default: 0, log every publish)
    int sample_interval;									// sample interval in seconds (default: 0, use log interval)
    struct condition_t										// log condition (optional)
    {
        ddm_log_data_t lhs;									// left-hand side of comparison
        ddm_log_data_t rhs;									// right-hand side of comparison
        DDM_LOG_SPEC_PARSER_COMPARISON_TOKEN comparison;	// comparison operator (default: TOKEN_NONE, no comparison)
    } conditions[DDM_LOG_SPEC_PARSER_MAX_NUM_CONDITIONS];
    int num_conditions;
    struct hysteresis_t										// hysteresis (optional)
    {
        int lower;											// lower threshold (default: 0, no lower threshold)
        int upper;											// upper threshold (default: same as lower threshold)
    } hysteresis;
    StaticTimer_t log_timer_buffer;							// storage buffer for log timer
    TimerHandle_t log_timer;								// log timer handle
    StaticTimer_t sample_timer_buffer;						// storage buffer for log timer
    TimerHandle_t sample_timer;								// log timer handle
    int64_t accumulator;                                    // Accumulator of sampled data
    int64_t sample_counter;                                 // Number of accumulated sampled data
    int64_t prev_data;                                      // Storage of previously logged data (accumulator/sample_counter)
    uint16_t repeat;                                        // Number of skipped log events due to hysteresis
} ddm_log_specification_data_t;


typedef struct ddm_log_read_specification_data_t
{
    int instance;                                           // Corresponding class instance (LSCFG<X>)
    char name[DDM_LOG__PARAM_MAX_NAME_LEN];                 // log specification name
    ddm_log_data_t data;                                    // parameter data to log (required)
    struct log_read_time_t
    {
        uint32_t start;                                          // start time in seconds (default: 0, now)
        uint32_t end;                                            // end time in seconds
        uint32_t interval;                                       // interval in seconds between requested logs (default: 0, get all)
        uint32_t counter;
    } log_read_time;
    int64_t accumulator;                                    // Accumulator of sampled data
    int64_t sample_counter;                                 // Number of accumulated sampled data
    int32_t *data_buffer;
    int data_buffer_length;
    const ddm_log_specification_data_t *p_logconfig;              // Parent log spec for this log read spec
} ddm_log_read_specification_data_t;

/**
 * @brief Parse parameter name provided by the broker
 *
 * \a parameter_name parsed will be stored as parameter id in \a parameter.
 *
 * @param parameter_name Points to a name of the parameter.
 * @param parameter      Points to the prameter's id
 *
 * \pre       Parameter \a parameter_name must be a non-NULL pointer
 * \pre       Parameter \a parameter must be a non-NULL pointer
 *
 * \retval    0 invalid \a parameter_name, was not found in ddm2 parameter list
 * \retval    1 valid \a parameter_name, was found in ddm2 parameter list
 */
int ddm_log_spec_parser__parse_ddm_id(const char *parameter_name, uint32_t *parameter);

/**
 * @brief Parse activate item provided by the broker
 *
 * The information parsed, will be stored in \a ddm_log_spec_parser__spec_data structure.
 *
 * @param spec_data    Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param activate     Request to activate the log specification
 *
 * \pre       Parameter \a spec_data must be a non-NULL pointer
 *
 * \return    Returns whether the specification is activated
 */
int ddm_log_spec_parser__parse_activate(struct ddm_log_spec_parser__spec_data *spec_data, const uint32_t activate);

/**
 * @brief Parse storage provided by the broker
 *
 * The information parsed, will be stored in \a ddm_log_spec_parser__spec_data structure.
 *
 * @param spec_data    Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param storage      Memory in which the log events will be stored
 *
 * \pre       Parameter \a spec_data must be a non-NULL pointer
 *
 * \retval    0 invalid enum constant for \a storage has been provided
 * \retval    1 valid enum constant for \a storage has been provided
 */
int ddm_log_spec_parser__parse_ddm_storage(struct ddm_log_spec_parser__spec_data *spec_data, const ddm_log_spec_parser__spec_storage_t storage);

/**
 * @brief Parse interval provided by the broker
 *
 * The information parsed, will be stored in \a ddm_log_spec_parser__spec_data structure.
 * Accepted Format: 1h 15min; 75min; 1d 2h
 *
 * @param spec_data   Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param interval    NULL terminated string
 *
 * \pre       Parameter \a spec_data must be a non-NULL pointer
 * \pre       Parameter \a interval must be a non-NULL pointer
 *
 * \retval    0 \a interval string format is invalid
 * \retval    1 \a interval string format isvalid, string successfully parsed
 */
int ddm_log_spec_parser__parse_ddm_interval(struct ddm_log_spec_parser__spec_data *spec_data, char *interval);

int ddm_log_spec_parser__parse_ddm_filter(struct ddm_log_spec_parser__spec_data *spec_data, const char *filter, size_t size);

int ddm_log_spec_parser__prepare(void);
int ddm_log_spec_parser__parse(ddm_log_specification_data_t *const p_specification_data, char *p_option_string);
int ddm_log_read_spec_parser__parse(ddm_log_read_specification_data_t *const p_specification_data, char *p_option_string);

#endif // DDM_LOG_SPEC_PARSER_H_
