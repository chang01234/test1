/**
 * @file lin_server_scheduler.c
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server scheduler implementation
 * @date 2023-12-28
 */

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "configuration.h"

#include "lin_server_scheduler.h"

/* Private Macros */

#define LIN_SERVER_SCHEDULER_INVALID_TALBE_ITEM_INDEX (UINT32_MAX)

/**
 * @brief Scheduler table
 */
typedef struct lin_sever_scheduler_table
{
    size_t table_items_size;
    lin_server_scheduler_table_item_def_t *table_items;
    /* private */
    size_t current_item_index;
} lin_server_scheduler_table_t;

/**
 * @brief Scheduler descriptor
 */
typedef struct lin_server_scheduler_descriptor
{
    lin_server_scheduler_table_t scheduler_table;
    TimerHandle_t timer_handle;
    void (*execution_function)(void);
} lin_server_scheduler_descriptor_t;

/* Private members */
static lin_server_scheduler_descriptor_t lin_server_scheduler;

/* Private functions */
static void lin_server_scheduler_tick_timer_cb(TimerHandle_t timer);
static bool lin_server_scheduler_schedule_get_next_scheduled_signal_frame(void);

/**
 * @brief Scheduler initialization
 *
 * @param        table_items Table items which keeps frames definition located in FLASH
 * @param        table_items_def Table items defintion which stores modifiable/non-modifiable data of the frames
 * @param        table_items_size Number of table items
 */
void lin_server_scheduler_init(const lin_server_scheduler_table_item_t *const table_items, lin_server_scheduler_table_item_def_t *const table_items_def, size_t table_items_size)
{
    lin_server_scheduler.scheduler_table.table_items = table_items_def;
    lin_server_scheduler.scheduler_table.table_items_size = table_items_size;
    lin_server_scheduler.scheduler_table.current_item_index = LIN_SERVER_SCHEDULER_INVALID_TALBE_ITEM_INDEX;
    for (size_t i = 0; i < lin_server_scheduler.scheduler_table.table_items_size; i++)
    {
        lin_server_scheduler.scheduler_table.table_items[i].item = &table_items[i];
        lin_server_scheduler.scheduler_table.table_items[i].item_context.is_scheduled = SCHEDULE_FRAME_TYPE_NONE;
    }

    TRUE_CHECK_RETURN(lin_server_scheduler.timer_handle = xTimerCreate("LIN Server scheduler tick", pdMS_TO_TICKS(LIN_SERVER_TICK_TIMER_PERIOD_MS), pdTRUE, NULL, lin_server_scheduler_tick_timer_cb));
}

/**
 * @brief Execution callback function
 *
 * @param       execution_function Callback function executed and driven by FreeRTOS timer
 */
void lin_server_scheduler_set_execution_callback(void (*execution_function)(void))
{
    lin_server_scheduler.execution_function = execution_function;
}

/**
 * @brief       Get frame type for specific device
 *
 * @param       frame_type Frame type i.e. CTRL/INFO
 * @param       device_type Device type e.g. LIN_SERVER_DEVICE_TYPE_FRESHJET_2200_3000
 *
 * @note        TODO: extend/modify function to support multiple ctrl/info frames for same device.
 *              Depends on how the data will be linked in the "schedule_table_t"
 */
lin_server_scheduler_table_item_def_t *lin_server_scheduler_get_frame_by_device_type_and_id(lin_server_device_type_t device_type, lin_frame_type_t frame_type, uint8_t frame_id)
{
    for (size_t i = 0; i < lin_server_scheduler.scheduler_table.table_items_size; i++)
    {
        if (lin_server_scheduler.scheduler_table.table_items[i].item->slave_device_type == device_type)
        {
            if (lin_server_scheduler.scheduler_table.table_items[i].item->frame_type == frame_type)
            {
                if (lin_server_scheduler.scheduler_table.table_items[i].item->frame_id == frame_id)
                {
                    return &lin_server_scheduler.scheduler_table.table_items[i];
                }
            }
        }
    }

    return NULL;
}

/**
 * @brief Schedule frame for specific device
 *
 * @param       frame_type Frame type i.e. CTRL/INFO
 * @param       frame_id Frame id
 * @param       device_type Device type e.g. LIN_SERVER_DEVICE_TYPE_FRESHJET_2200_3000
 * @param       schedule_request CTRL(CTRL_BROKER_REQUEST, CTRL_LOCAL_CHANGE_REQUEST, CTRL_INIT_REQUEST) or INFO(INFO_REQUEST)
 *
 * @return      true, if the frame has been successfully scheduled for selected device. Otherwise, false.
 */
bool lin_server_scheduler_schedule_frame_by_device_type_and_id(lin_frame_type_t frame_type, uint8_t frame_id, lin_server_device_type_t device_type, lin_scheduler_frame_type_requests_t schedule_request)
{
    for (size_t i = 0; i < lin_server_scheduler.scheduler_table.table_items_size; i++)
    {
        lin_server_scheduler_table_item_def_t *table_item_def = &lin_server_scheduler.scheduler_table.table_items[i];
        if (table_item_def->item->slave_device_type == device_type)
        {
            if (table_item_def->item->frame_type == frame_type)
            {
                if (table_item_def->item->frame_id == frame_id)
                {
                    return lin_server_scheduler_schedule_frame(table_item_def, schedule_request);
                }
            }
        }
    }

    return false;
}

void lin_server_scheduler_unschedule_frames_by_device_type(lin_server_device_type_t device_type)
{
    for (size_t i = 0; i < lin_server_scheduler.scheduler_table.table_items_size; i++)
    {
        lin_server_scheduler_table_item_def_t *table_item_def = &lin_server_scheduler.scheduler_table.table_items[i];
        if (table_item_def->item->slave_device_type == device_type)
        {
            lin_server_scheduler_schedule_frame(table_item_def, SCHEDULE_FRAME_TYPE_NONE);
        }
    }
}

/**
 * @brief Schedule frame
 *
 * @param       table_item_def Frame definition listed in the scheduling table
 * @param       schedule_request CTRL(CTRL_BROKER_REQUEST, CTRL_LOCAL_CHANGE_REQUEST, CTRL_INIT_REQUEST) or INFO(INFO_REQUEST)
 *
 * @return      true, if the frame has been successfully scheduled. Otherwise, false.
 */
bool lin_server_scheduler_schedule_frame(lin_server_scheduler_table_item_def_t *table_item_def, lin_scheduler_frame_type_requests_t schedule_request)
{
    switch (table_item_def->item->frame_type)
    {
    case LIN_CONTROL_FRAME:
        if (LIN_SCHEDULE_FRAME_TYPE_IS_CTRL_FRAME_REQUEST(schedule_request) == false)
        {
            LOG(E, "Invalid schedule request[%d] for CONTROL frame. Frame not scheduled.", schedule_request);
            return false;
        }
        break;
    case LIN_INFO_FRAME:
        if (LIN_SCHEDULE_FRAME_TYPE_IS_INFO_FRAME_REQUEST(schedule_request) == false)
        {
            LOG(E, "Invalid schedule request[%d] for INFO frame. Frame not scheduled.", schedule_request);
            return false;
        }
        break;
    default:
        LOG(E, "Not supported frame type[%d]. Only INFO or CTRL can be scheduled.", table_item_def->item->frame_type);
        return false;
    }

    LIN_SCHEDULE_FRAME_TYPE_SET(table_item_def->item_context.is_scheduled, schedule_request);
    return true;
}

/**
 * @brief Check if frame has been scheduled
 *
 * @param       table_item_def Frame definition listed in the scheduling table
 * @param       schedule_request CTRL(CTRL_BROKER_REQUEST, CTRL_LOCAL_CHANGE_REQUEST, CTRL_INIT_REQUEST) or INFO(INFO_REQUEST)
 *
 * @return      true, if the frame is scheduled with the requested scheduling request. Otherwise, false.
 */
bool lin_server_scheduler_is_frame_scheduled_type_set(lin_server_scheduler_table_item_def_t *table_item_def, lin_scheduler_frame_type_requests_t schedule_request)
{
    return LIN_SCHEDULE_FRAME_TYPE_GET(table_item_def->item_context.is_scheduled, schedule_request) != 0 ? true : false;
}

/**
 * @brief Clear scheduling status for the specifc scheduling request
 *
 * @param       table_item_def Frame definition listed in the scheduling table
 * @param       schedule_request CTRL(CTRL_BROKER_REQUEST, CTRL_LOCAL_CHANGE_REQUEST, CTRL_INIT_REQUEST) or INFO(INFO_REQUEST)
 */
void lin_server_scheduler_unschedule_frame(lin_server_scheduler_table_item_def_t *table_item_def, lin_scheduler_frame_type_requests_t schedule_request)
{
    LIN_SCHEDULE_FRAME_TYPE_CLEAR(table_item_def->item_context.is_scheduled, schedule_request);
}

/**
 * @brief Check if frame has been scheduled
 *
 * @param       table_item_def Frame definition listed in the scheduling table
 *
 * @return      true, if the frame is scheduled. Otherwise, false.
 */
bool lin_server_scheduler_is_frame_scheduled(lin_server_scheduler_table_item_def_t *table_item_def)
{
    bool is_frame_scheduled = false;

    switch (table_item_def->item->frame_type)
    {
    case LIN_CONTROL_FRAME:
        is_frame_scheduled = LIN_SCHEDULE_FRAME_TYPE_IS_CTRL_FRAME_SCHEDULED(table_item_def->item_context.is_scheduled) != 0 ? true : false;
        break;
    case LIN_INFO_FRAME:
        is_frame_scheduled = LIN_SCHEDULE_FRAME_TYPE_IS_INFO_FRAME_SCHEDULED(table_item_def->item_context.is_scheduled) != 0 ? true : false;
        break;
    default:
        LOG(E, "Not supported frame type[%d]. Only INFO or CTRL should be scheduled.", table_item_def->item->frame_type);
        break;
    }

    return is_frame_scheduled;
}

/**
 * @brief Get current scheduled frame
 *
 * @return      Frame's definition of the currently scheduled frame.
 */
lin_server_scheduler_table_item_def_t *lin_server_scheduler_get_current_scheduled_frame(void)
{
    return &lin_server_scheduler.scheduler_table.table_items[lin_server_scheduler.scheduler_table.current_item_index];
}

/**
 * @brief       Iterrate to next scheduled frame in the table
 *              and update the table index correspondingly
 *
 * @return      true, if it found next scheduled frame.
 * @return      false, if there is no scheduled frame or if it reached the end of the table
 */
static bool lin_server_scheduler_schedule_get_next_scheduled_signal_frame(void)
{
    bool is_signal_frame_scheduled = false;
    static size_t index = 0;

    /* Take current index and start from there, since we
     * have updated to the next table index last time we
     * exited from this function.
     */
    lin_server_scheduler.scheduler_table.current_item_index = index;
    while (lin_server_scheduler.scheduler_table.current_item_index < lin_server_scheduler.scheduler_table.table_items_size)
    {
        size_t current_item_index = lin_server_scheduler.scheduler_table.current_item_index;
        const lin_server_scheduler_table_item_t *current_table_item = lin_server_scheduler.scheduler_table.table_items[current_item_index].item;
        lin_server_scheduler_table_item_context_t *current_table_item_context = &lin_server_scheduler.scheduler_table.table_items[current_item_index].item_context;
        if (current_table_item->frame_type == LIN_CONTROL_FRAME)
        {
            if (LIN_SCHEDULE_FRAME_TYPE_IS_CTRL_FRAME_SCHEDULED(current_table_item_context->is_scheduled))
            {
                is_signal_frame_scheduled = true;
                break;
            }
        }
        else if (current_table_item->frame_type == LIN_INFO_FRAME)
        {
            if (LIN_SCHEDULE_FRAME_TYPE_IS_INFO_FRAME_SCHEDULED(current_table_item_context->is_scheduled))
            {
                is_signal_frame_scheduled = true;
                break;
            }
        }

        index = ++lin_server_scheduler.scheduler_table.current_item_index;
    }

    if (is_signal_frame_scheduled)
    {
        index++;  // Move to next index
    }
    else
    {
        index = 0;  // reset index to the begginig of the table as we have reached
                    // this state when there is no scheduled frame in the table or
                    // we are at the end of the table and need switching to bus probing.
    }

    return is_signal_frame_scheduled;
}

/**
 * @brief       Iterrate to next scheduled frame in the table
 *              and update the table index correspondingly
 *
 * @return      true, if it found next scheduled frame.
 * @return      false, if there is no scheduled frame or if it the end of the table has been  eached
 */
bool lin_server_scheduler_get_next_scheduled_frame(void)
{
    return lin_server_scheduler_schedule_get_next_scheduled_signal_frame();
}

bool lin_server_scheduler_is_ctrl_frame_scheduled(void)
{
    bool is_ctrl_frame_scheduled = false;

    for (size_t index = 0; index < lin_server_scheduler.scheduler_table.table_items_size; ++index)
    {
        const lin_server_scheduler_table_item_t *current_table_item = lin_server_scheduler.scheduler_table.table_items[index].item;
        lin_server_scheduler_table_item_context_t *current_table_item_context = &lin_server_scheduler.scheduler_table.table_items[index].item_context;
        if (current_table_item->frame_type == LIN_CONTROL_FRAME)
        {
            if (LIN_SCHEDULE_FRAME_TYPE_IS_CTRL_FRAME_SCHEDULED(current_table_item_context->is_scheduled))
            {
                is_ctrl_frame_scheduled = true;
                break;
            }
        }
    }

    return is_ctrl_frame_scheduled;
}

/**
 * @brief       Enable/Disable scheduler timer
 */
void lin_server_scheduler_enable(bool is_enabled)
{
    if (is_enabled)
    {
        TRUE_CHECK(xTimerStart(lin_server_scheduler.timer_handle, portMAX_DELAY));
    }
    else
    {
        TRUE_CHECK(xTimerStop(lin_server_scheduler.timer_handle, portMAX_DELAY));
    }
}

/**
 * @brief       Change scheduler timer period
 */
void lin_server_scheduler_change_time_period(uint32_t time_ms)
{
    TRUE_CHECK_RETURN(pdMS_TO_TICKS(time_ms)); /* time ms cannot be less than one tick */
    TRUE_CHECK_RETURN((time_ms >= LIN_SERVER_TICK_TIMER_MIN_PERIOD_MS) && (time_ms <= LIN_SERVER_TICK_TIMER_MAX_PERIOD_MS));

    TRUE_CHECK(xTimerChangePeriod(lin_server_scheduler.timer_handle, pdMS_TO_TICKS(time_ms), portMAX_DELAY));
}

/**
 * @brief Timer callback, which kiks each time
 * LIN_SERVER_TICK_TIMER_PERIOD_MS has passed
 *
 * @param        timer RTOS timer handler
 */
static void lin_server_scheduler_tick_timer_cb(TimerHandle_t timer)
{
    lin_server_scheduler.execution_function();
}

COMPILE_TIME_ASSERT(pdMS_TO_TICKS(LIN_SERVER_TICK_TIMER_MIN_PERIOD_MS) != 0);  // minimum requrired frequency cannot be achived with current freeRTOS tick frequency
COMPILE_TIME_ASSERT(LIN_SERVER_TICK_TIMER_PERIOD_MS >= LIN_SERVER_TICK_TIMER_MIN_PERIOD_MS);
COMPILE_TIME_ASSERT(LIN_SERVER_TICK_TIMER_DIAG_PERIOD_MS >= LIN_SERVER_TICK_TIMER_MIN_PERIOD_MS);
COMPILE_TIME_ASSERT(LIN_SERVER_TICK_TIMER_MAX_PERIOD_MS > LIN_SERVER_TICK_TIMER_MIN_PERIOD_MS);
