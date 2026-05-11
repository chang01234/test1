/*! \file hal_scheduler.h
	\brief Scheduler Hardware Abstraction Layer
        
    HAL used for scheduling function calls. 
    
    Users can use  \p hal_scheduler_add to schedule execution of a function in 
    the main thread from an interrupt. Use \p hal_scheduler_add_delayed to delay
    scheduled execution for a specified number of milliseconds. And use
    \p hal_scheduler_add_periodic to schedule periodic execution.
    
    -----
    
    Implementation notes:
        - Implementation is responsible for ensuring scheduled calls are done
        one at the time. Callback functions for scheduled calls are not expected
        to access shared resources in a thread-safe way.
*/

#ifndef HAL_SCHEDULER_H_
#define HAL_SCHEDULER_H_

/** Includes ******************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** Defines *******************************************************************/

/*! \brief Scheduled function type.
 *
 *  This is the signature of a callback function accepted by 
 *  \p hal_scheduler_add and \p hal_scheduler_remove.
 *
 *  data_ptr is a pointer to a copy of the data provided when scheduling the
 *  function call. data_size is the size of this data in bytes.
 */
typedef void (HAL_SCHEDULER_CB)(void * data_ptr, size_t data_size);

/** Function prototypes *******************************************************/

/*! \brief Scheduler hal init function.
 *
 *  Initializer for scheduler HAL. Must be called before calling
 *  \p hal_scheduler_add. Should only be called once.
 *  
 *  \returns    0 if successful. Any other value otherwise.
 */
int hal_scheduler_init(void);

/*! \brief Execute all scheduled functions that are ready.
 *
 *  Used by the main context to execute scheduled function calls. All scheduled
 *  calls with their delays elapsed will be executed before this function
 *  returns.
 *
 *  \returns    0 if successful. Any other value otherwise.
 */
int hal_scheduler_execute(void);

/*! \brief Schedules a call to cb in main thread.
 *  
 *  This will call the provided callback function with a copy of the provided
 *  data Callback will be run in main thread context so this function can be
 *  used to escape an interrupt context.
 *
 *  Data will be stored as a copy by the scheduler and so does not need to be
 *  retained by the caller. If no data is required data_ptr may be NULL and
 *  data_size should then be 0.
 *
 *  \param cb           Function to be scheduled for execution.
 *  \param data_ptr     Data pointer provided to cb when executing.
 *  \param data_size    Size of data in bytes.
 *
 *  \returns            0 if successful. Any other value otherwise.
 */
int hal_scheduler_add(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size);

/*! \brief Schedules a call to cb after delay milliseconds.
 *  
 *  This will call the provided callback function with a copy of the provided
 *  data after delay milliseconds. Behavior and considerations are otherwise
 *  identical to \p hal_scheduler_add.
 *
 *  \param cb           Function to be scheduled for execution.
 *  \param data_ptr     Data pointer provided to cb when executing.
 *  \param data_size    Size of data in bytes.
 *  \param delay        Delay before scheduled function should be called in milliseconds.
 *
 *  \returns            0 if successful. Any other value otherwise.
 */
int hal_scheduler_add_delayed(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size, uint32_t delay);

/*! \brief Schedules a call to cb every period milliseconds.
 *  
 *  This will call the provided callback function with a copy of the provided
 *  data every period milliseconds. Behavior and considerations are otherwise
 *  identical to \p hal_scheduler_add.
 *
 *  Note that period need to be at least 1 millisecond.
 *
 *  \param cb           Function to be scheduled for execution.
 *  \param data_ptr     Data pointer provided to cb when executing.
 *  \param data_size    Size of data in bytes.
 *  \param period       Period between function calls in milliseconds.
 *
 *  \returns            0 if successful. Any other value otherwise.
 */
int hal_scheduler_add_periodic(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size, uint32_t period);

/*! \brief Removes all scheduled calls to cb.
 *
 *  This will remove all scheduled tasks where the scheduled callback function
 *  is cb. This includes both ready tasks and tasks waiting for their delay to
 *  expire.
 *
 *  Will return success (0) even if there were no scheduled tasks to remove.
 *
 *  \param cb   Previously scheduled function to be unscheduled.
 *
 *  \returns    0 if successful. Any other value otherwise.
 */
int hal_scheduler_remove(HAL_SCHEDULER_CB cb);

/*! \brief Scheduler radio notification callback.
 *
 *  Used by Bluetooth HAL to notify scheduler about radio activity.
 *
 *  Parameter radio_active will be true if radio went from off to on and false
 *  if radio went from on to off state.
 *
 *  \param radio_active Radio state.
 */
void hal_scheduler_radio_notification_handler(bool radio_active);

#endif //HAL_SCHEDULER_H_