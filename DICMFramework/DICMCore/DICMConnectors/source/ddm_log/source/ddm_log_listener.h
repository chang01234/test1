#ifndef DDM_LOG_LISTENER_H_
#define DDM_LOG_LISTENER_H_

#include <stdint.h>
#include "ddm_entry.h"
#include "ddm_log_event.h"

struct ddm_log__spec;

/**
 * @brief Creates new log entry and run it trough filtering rules
 *
 * @param spec      Log Specification of the parameter
 * @param value     Parameter's value
 * @param size      Paramater's value size
 *
 * \pre    Parameter \a spec must be a non_NULL pointer.
 * \pre    Parameter \a value must be a non-NULL pointer.
 *
 * @return ddm_log_event__data_t*
 */
ddm_log_event__data_t *ddm_log_listener__new_entry(struct ddm_log__spec * spec, const void * value, size_t size);

/**
 * @brief Creates new log event data
 *
 * Allocates memory and populates the \a event_data.
 *
 * @param event_data    Pointer to the log event that will be created
 * @param spec          Specification for the parameter being logged
 * @param value         Value of the parameter
 * @param size          Size of the \a value
 *
 * \pre    Parameter \a spec must be a non_NULL pointer.
 * \pre    Parameter \a value must be a non-NULL pointer.
 *
 * @return 1 - if memory has been allocated successfully
 */
int ddm_log_listener__create_event(ddm_log_event__data_t ** event_data, struct ddm_log__spec * spec, const void * value, size_t size);

#endif // DDM_LOG_LISTENER_H_
