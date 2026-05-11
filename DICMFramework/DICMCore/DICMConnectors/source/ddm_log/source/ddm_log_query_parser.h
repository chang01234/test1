#ifndef DDM_LOG_QUERY_PARSER_H_
#define DDM_LOG_QUERY_PARSER_H_

#include <stdint.h>

#include "ddm_log.h"

struct ddm_log_query_parser__time_span
{
    uint32_t start_date;
    uint32_t end_date;
};

struct ddm_log_query_parser__query_data
{
    struct log_events_size
    {
        uint32_t usrSize;  // set by user
        uint32_t cntSize;  // counted size
    } size;

    struct ddm
    {
        uint8_t numb_of_parameters;
        uint32_t ddm_id[DDM_LOG__SPEC_MAX];
    } ddm_ids;
    struct ddm_log_query_parser__time_span timespan;
};

/**
 * @brief Parse the input string provided by the connector
 *
 *  Input string should provide DDM IDs and timestamp of intrested.
 *  The information prased, will be stored in \ref query_data structure.
 *  Format:
 *      "-d AC0ON,AC0TMP"
 *      "-t 15.03.2022=17.04.2022"
 *      "-t 15.03.2022=17.04.2022 -d AC0ON,AC0TMP"
 *      "-d AC0ON,AC0TMP -t 15.03.2022=17.04.2022"
 *
 * @param     query_data Points to a \a ddm_log_query_parser__query_data structure.
 * @param     query_str Points to a \a query_str NULL terminated string with
 *            information about the data being parsed
 *
 * \pre       Parameter \a query_data must be a non-NULL pointer
 * \pre       Parameter \a query_str must be a non-NULL pointer
 *
 * \return  string will be erased in \a query_str if the format is invalid
 * \return  true - If query parser succesfully parse and populate query_data
 */
int ddm_log_query_parser__parse(struct ddm_log_query_parser__query_data * query_data, char * query_str);

#endif // DDM_LOG_QUERY_PARSER_H_
