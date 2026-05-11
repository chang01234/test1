#ifndef DDM_LOG_QUERY_EVENT_FILTER_H_
#define DDM_LOG_QUERY_EVENT_FILTER_H_

#include <stdint.h>
#include "ddm_log_event.h"

struct ddm_log_query_parser__query_data;
struct ddm_log__spec;

/**
 * @brief Validates whether the log entry read from memory passes the
 *        output filtering rules set by the user in \a qdata
 *
 * @param qdata       Points to a \a ddm_log_query_parser__query_data structure.
 *                    Stores the output filtering rules set by the user.
 * @param value       Log entry read from memory.
 *
 * @return true - if all of the output filtering rules pass
 */
int ddm_log_query_event_filter__header(const struct ddm_log_query_parser__query_data * qdata, const ddm_log_event__data_val_t value);

/**
 * @brief Validates whether the \a timestamp of the log entry read
 *        from memory matches the timespan set by user in \a qdata.
 *
 * @param qdata       Points to a \a ddm_log_query_parser__query_data structure.
 * @param timestamp   UTC timestamp.
 *
 * @return true - if \a timestamp matches the timespan set by user in \a qdata.
 */
int ddm_log_query_event_filter__timespan(const struct ddm_log_query_parser__query_data * qdata, const uint32_t timestamp);

/**
 * @brief Validates whether the \a ddm_id of the log entry read
 *        from memory matches any of the ddm_ids set by user in \a qdata.
 *
 * @param qdata       Points to a \a ddm_log_query_parser__query_data structure.
 * @param ddm_id      DDM parameter ID
 *
 * @return true - if ddm_id matches any of the ddm_ids set by user in \a qdata.
 */
int ddm_log_query_event_filter__ddm_id(const struct ddm_log_query_parser__query_data * qdata, const uint32_t ddm_id);

/**
 * @brief Validates whether the \a size of the log entries being read
 *        from memory does not exceed the size set by user in \a qdata.
 *
 * @param qdata       Points to a \a ddm_log_query_parser__query_data structure.
 * @param size        Size of the log entires
 *
 * @return false - if \a size exceeds the \a qdata->size.usrSize
 */
int ddm_log_query_event_filter__size_exceeded(const struct ddm_log_query_parser__query_data * qdata, const uint32_t size);

#endif //DDM_LOG_QUERY_EVENT_FILTER_H_
