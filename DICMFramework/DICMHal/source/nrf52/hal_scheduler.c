/*! \file hal_scheduler.c
	\brief Scheduler Hardware Abstraction Layer nRF52 implementation
*/

/** Includes ******************************************************************/
#include "hal_scheduler.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app_error.h"
#include "app_util_platform.h"
#include "app_timer.h"
#include "nrf_atfifo.h"
#include <stdlib.h>

/** Defines *******************************************************************/
#define MAX_SCHEDULED_TASKS   60    //!< Maximum number of tasks.
#define MAX_DATA_SIZE         200   //!< Maximum size of scheduled task data.

typedef struct TASK
{
    app_timer_id_t timer;
    uint32_t delay_ticks;
    HAL_SCHEDULER_CB *cb;
    size_t data_size;
    void *data;
    bool is_active;
    bool is_repeating;
    bool is_executing;
} TASK; 

/** Private Function prototypes ***********************************************/
static void scheduler_timer_cb(void *p_context);
static int schedule_add(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size, uint32_t delay, bool is_repeating);

/** Variables *****************************************************************/
static bool init_complete = false;  //!< True if scheduler HAL has been initialized.

static volatile bool run_schedule = true;    //!< Only execute next task while true.

NRF_ATFIFO_DEF(scheduler_fifo, TASK *, MAX_SCHEDULED_TASKS);

// App timers used when task should be triggered after a delay.
APP_TIMER_DEF(scheduler_timer_1);
APP_TIMER_DEF(scheduler_timer_2);
APP_TIMER_DEF(scheduler_timer_3);
APP_TIMER_DEF(scheduler_timer_4);
APP_TIMER_DEF(scheduler_timer_5);
APP_TIMER_DEF(scheduler_timer_6);
APP_TIMER_DEF(scheduler_timer_7);
APP_TIMER_DEF(scheduler_timer_8);
APP_TIMER_DEF(scheduler_timer_9);
APP_TIMER_DEF(scheduler_timer_10);
APP_TIMER_DEF(scheduler_timer_11);
APP_TIMER_DEF(scheduler_timer_12);
APP_TIMER_DEF(scheduler_timer_13);
APP_TIMER_DEF(scheduler_timer_14);
APP_TIMER_DEF(scheduler_timer_15);
APP_TIMER_DEF(scheduler_timer_16);
APP_TIMER_DEF(scheduler_timer_17);
APP_TIMER_DEF(scheduler_timer_18);
APP_TIMER_DEF(scheduler_timer_19);
APP_TIMER_DEF(scheduler_timer_20);
APP_TIMER_DEF(scheduler_timer_21);
APP_TIMER_DEF(scheduler_timer_22);
APP_TIMER_DEF(scheduler_timer_23);
APP_TIMER_DEF(scheduler_timer_24);
APP_TIMER_DEF(scheduler_timer_25);
APP_TIMER_DEF(scheduler_timer_26);
APP_TIMER_DEF(scheduler_timer_27);
APP_TIMER_DEF(scheduler_timer_28);
APP_TIMER_DEF(scheduler_timer_29);
APP_TIMER_DEF(scheduler_timer_30);
APP_TIMER_DEF(scheduler_timer_31);
APP_TIMER_DEF(scheduler_timer_32);
APP_TIMER_DEF(scheduler_timer_33);
APP_TIMER_DEF(scheduler_timer_34);
APP_TIMER_DEF(scheduler_timer_35);
APP_TIMER_DEF(scheduler_timer_36);
APP_TIMER_DEF(scheduler_timer_37);
APP_TIMER_DEF(scheduler_timer_38);
APP_TIMER_DEF(scheduler_timer_39);
APP_TIMER_DEF(scheduler_timer_40);
APP_TIMER_DEF(scheduler_timer_41);
APP_TIMER_DEF(scheduler_timer_42);
APP_TIMER_DEF(scheduler_timer_43);
APP_TIMER_DEF(scheduler_timer_44);
APP_TIMER_DEF(scheduler_timer_45);
APP_TIMER_DEF(scheduler_timer_46);
APP_TIMER_DEF(scheduler_timer_47);
APP_TIMER_DEF(scheduler_timer_48);
APP_TIMER_DEF(scheduler_timer_49);
APP_TIMER_DEF(scheduler_timer_50);
APP_TIMER_DEF(scheduler_timer_51);
APP_TIMER_DEF(scheduler_timer_52);
APP_TIMER_DEF(scheduler_timer_53);
APP_TIMER_DEF(scheduler_timer_54);
APP_TIMER_DEF(scheduler_timer_55);
APP_TIMER_DEF(scheduler_timer_56);
APP_TIMER_DEF(scheduler_timer_57);
APP_TIMER_DEF(scheduler_timer_58);
APP_TIMER_DEF(scheduler_timer_59);
APP_TIMER_DEF(scheduler_timer_60);

// Blocks used to store data for tasks.
static TASK tasks[MAX_SCHEDULED_TASKS] =
{
    { .timer = scheduler_timer_1 },
    { .timer = scheduler_timer_2 },
    { .timer = scheduler_timer_3 },
    { .timer = scheduler_timer_4 },
    { .timer = scheduler_timer_5 },
    { .timer = scheduler_timer_6 },
    { .timer = scheduler_timer_7 },
    { .timer = scheduler_timer_8 },
    { .timer = scheduler_timer_9 },
    { .timer = scheduler_timer_10 },   
    { .timer = scheduler_timer_11 },
    { .timer = scheduler_timer_12 },
    { .timer = scheduler_timer_13 },
    { .timer = scheduler_timer_14 },
    { .timer = scheduler_timer_15 },
    { .timer = scheduler_timer_16 },
    { .timer = scheduler_timer_17 },
    { .timer = scheduler_timer_18 },
    { .timer = scheduler_timer_19 },
    { .timer = scheduler_timer_20 },
    { .timer = scheduler_timer_21 },
    { .timer = scheduler_timer_22 },
    { .timer = scheduler_timer_23 },
    { .timer = scheduler_timer_24 },
    { .timer = scheduler_timer_25 },
    { .timer = scheduler_timer_26 },
    { .timer = scheduler_timer_27 },
    { .timer = scheduler_timer_28 },
    { .timer = scheduler_timer_29 },
    { .timer = scheduler_timer_30 },
    { .timer = scheduler_timer_31 },
    { .timer = scheduler_timer_32 },
    { .timer = scheduler_timer_33 },
    { .timer = scheduler_timer_34 },
    { .timer = scheduler_timer_35 },
    { .timer = scheduler_timer_36 },
    { .timer = scheduler_timer_37 },
    { .timer = scheduler_timer_38 },
    { .timer = scheduler_timer_39 },
    { .timer = scheduler_timer_40 },
    { .timer = scheduler_timer_41 },
    { .timer = scheduler_timer_42 },
    { .timer = scheduler_timer_43 },
    { .timer = scheduler_timer_44 },
    { .timer = scheduler_timer_45 },
    { .timer = scheduler_timer_46 },
    { .timer = scheduler_timer_47 },
    { .timer = scheduler_timer_48 },
    { .timer = scheduler_timer_49 },
    { .timer = scheduler_timer_50 },
    { .timer = scheduler_timer_51 },
    { .timer = scheduler_timer_52 },
    { .timer = scheduler_timer_53 },
    { .timer = scheduler_timer_54 },
    { .timer = scheduler_timer_55 },
    { .timer = scheduler_timer_56 },
    { .timer = scheduler_timer_57 },
    { .timer = scheduler_timer_58 },
    { .timer = scheduler_timer_59 },
    { .timer = scheduler_timer_60 },                                     
};

/** Private functions *********************************************************/

/*! \brief Scheduler app_timer callback.
 *
 *  Called when a delayed task is ready to be scheduled.
 *
 *  \param task_ptr Pointer to task which is to be scheduled.
 */
static void scheduler_timer_cb(void *p_context)
{
    TASK *task = (TASK *)p_context;
    APP_ERROR_CHECK(nrf_atfifo_alloc_put(scheduler_fifo, &task, sizeof(task), NULL));

    if(task->is_repeating)
    {
        APP_ERROR_CHECK(app_timer_start(task->timer, task->delay_ticks , task));
    }
}

static int schedule_add(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size, uint32_t delay, bool is_repeating)
{
    APP_ERROR_CHECK_BOOL(init_complete);
    APP_ERROR_CHECK_BOOL(cb != NULL);
    APP_ERROR_CHECK_BOOL(data_size <= MAX_DATA_SIZE);
    APP_ERROR_CHECK_BOOL(!(is_repeating && (delay == 0)));
    

    /* Acquire a task slot. 
     *
     * Done inside a critical region to guarantee each task slot can only be
     * given out once.
     */
    TASK *task = NULL;        
    CRITICAL_REGION_ENTER();
    for (int n = 0; n < MAX_SCHEDULED_TASKS; ++n)
    {
        if (!tasks[n].is_executing && !tasks[n].is_active)
        {
            task = &tasks[n];
            break;
        }
    }
    APP_ERROR_CHECK_BOOL(task != NULL); // Delayed task queue full.
    task->is_active = true;
    CRITICAL_REGION_EXIT();
    // At this point the task slot has been reserved so we no longer need the critical region.
    
    // Initialize the task slot with task data.
    task->cb = cb;
    task->data_size = data_size;
    task->is_repeating = is_repeating;
    if (data_ptr != NULL)
    {
        if (task->data == NULL)
        {
            task->data = calloc(data_size, sizeof(uint8_t));
        }

        APP_ERROR_CHECK_BOOL(task->data != NULL);
        memcpy(task->data, data_ptr, data_size);
    }
    
    // If delay is 0 run task as soon as possible. Else start its delay timer and run when timer is done.
    if (delay == 0)
    {
        task->delay_ticks = 0;
        APP_ERROR_CHECK(nrf_atfifo_alloc_put(scheduler_fifo, &task, sizeof(task), NULL));
    }
    else
    {
        task->delay_ticks = APP_TIMER_TICKS(delay);
        APP_ERROR_CHECK(app_timer_start(task->timer, task->delay_ticks , task));
    }
    
    return 0;   // If we reach this point no error occurred.
}

/** Public functions **********************************************************/
int hal_scheduler_init(void)
{
    if (!init_complete)
    {
        NRF_ATFIFO_INIT(scheduler_fifo);
        
        for (int n = 0; n < MAX_SCHEDULED_TASKS; ++n)
        {
            APP_ERROR_CHECK(app_timer_create(&tasks[n].timer, APP_TIMER_MODE_SINGLE_SHOT, scheduler_timer_cb));
        }
        
        init_complete = true;
    }
    
    return 0;   // If we reach this point no error occurred.
}

int hal_scheduler_execute(void)
{
    TASK *task;
    
    while (run_schedule && (nrf_atfifo_get_free(scheduler_fifo, &task, sizeof(task), NULL) == NRF_SUCCESS))
    {
        // Task might have been disabled after it was added to the FIFO so only run if still active.
        if (task->is_active)
        {
            task->is_executing = true;
            task->cb(task->data, task->data_size);
            task->is_executing = false;
            
            if (!task->is_repeating)
            {
                task->is_active = false;
                free(task->data);
                task->data = NULL;
            }
        }
    }   
    
    return 0;   // If we reach this point no error occurred.
}

int hal_scheduler_add(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size)
{
    return schedule_add(cb, data_ptr, data_size, 0, false);
}

int hal_scheduler_add_delayed(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size, uint32_t delay)
{
    return schedule_add(cb, data_ptr, data_size, delay, false);
}

int hal_scheduler_add_periodic(HAL_SCHEDULER_CB cb, void *data_ptr, size_t data_size, uint32_t period)
{
    return schedule_add(cb, data_ptr, data_size, period, true);
}

int hal_scheduler_remove(HAL_SCHEDULER_CB cb)
{
    APP_ERROR_CHECK_BOOL(cb != NULL);
    
    for (int n = 0; n < MAX_SCHEDULED_TASKS; ++n)
    {
        if (tasks[n].is_active && tasks[n].cb == cb)
        {
            APP_ERROR_CHECK(app_timer_stop(tasks[n].timer));
            tasks[n].is_active = false;
            free(tasks[n].data);
            tasks[n].data = NULL;
        }
    }

    return 0;   // If we reach this point no error occurred.
}

void hal_scheduler_radio_notification_handler(bool radio_active)
{
    run_schedule = !radio_active;
}