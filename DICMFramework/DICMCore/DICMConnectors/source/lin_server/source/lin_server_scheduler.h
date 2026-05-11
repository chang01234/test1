/**
 * @file lin_server_scheduler.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server scheduler implementation
 * @date 2023-12-28
 */

#ifndef LIN_SERVER_SCHEDULING_TABLE__
#define LIN_SERVER_SCHEDULING_TABLE__

#include "lin_server_device_definition.h"

#include "sorted_container.h"

#define LIN_SERVER_TICK_TIMER_MIN_PERIOD_MS     10u
#define LIN_SERVER_TICK_TIMER_MAX_PERIOD_MS     1000u

#ifndef LIN_SERVER_TICK_TIMER_PERIOD_MS
#define LIN_SERVER_TICK_TIMER_PERIOD_MS         100u
#endif

/* LIN2.2A 5.6 SLAVE DIAGNOSTIC TIMING REQUIREMENTS
 * Time between reception of the last frame of a diagnostic request on the LIN bus and the slave node
   being able to provide data for a response is min=50ms, time_max=500ms
 */
#ifndef LIN_SERVER_TICK_TIMER_DIAG_PERIOD_MS
#define LIN_SERVER_TICK_TIMER_DIAG_PERIOD_MS    450u
#endif

/**
 * @brief       Scheduling frame types
 *
 * @note        It is used for bitwise operations, depending on the frame type.
 *              BROKER, LOCAL_CHANGE(ACK) or INIT(NoInit) requests are used for the CTRL frame.
 *              INFO request is used for the INFO frame.
 */
#define SCHEDULE_FRAME_TYPE_NONE                          0x0
#define SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST           0x1
#define SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST     0x2
#define SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST             0x4
#define SCHEDULE_FRAME_TYPE_INFO_REQUEST                  0x8

#define LIN_SCHEDULE_FRAME_TYPE_SET(storage, request)        ((request == SCHEDULE_FRAME_TYPE_NONE) ? ((storage) = 0) : ((storage) |= (request)))
#define LIN_SCHEDULE_FRAME_TYPE_CLEAR(storage, request)      ((storage)  &= ~(request))
#define LIN_SCHEDULE_FRAME_TYPE_GET(storage, request)        ((storage)  &   (request))

#define LIN_SCHEDULE_FRAME_TYPE_IS_CTRL_FRAME_SCHEDULED(storage)               \
    (((storage) & (SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST))         |         \
      ((storage) & (SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST))  |         \
      ((storage) & (SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST)))

#define LIN_SCHEDULE_FRAME_TYPE_IS_CTRL_FRAME_REQUEST(request)                 \
    (((request) == (SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST))         |        \
      ((request) == (SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST))  |        \
      ((request) == (SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST))          |        \
      ((request) == (SCHEDULE_FRAME_TYPE_NONE)))

#define LIN_SCHEDULE_FRAME_TYPE_IS_INFO_FRAME_SCHEDULED(storage)               \
    ((storage) & (SCHEDULE_FRAME_TYPE_INFO_REQUEST))

#define LIN_SCHEDULE_FRAME_TYPE_IS_INFO_FRAME_REQUEST(request)                 \
    (((request) == (SCHEDULE_FRAME_TYPE_INFO_REQUEST))                |        \
     ((request) == (SCHEDULE_FRAME_TYPE_NONE)))

/**
 * @brief Frame type requests
 *
 * @note To be used together with SCHEDULE_FRAME_TYPE_<X> macros
 *
 */
typedef uint32_t lin_scheduler_frame_type_requests_t;

/**
 * @brief Scheduler table item context
 *
 * Stores modifiable data for INFO/CTRL frames.
 * All members are stored in RAM.
 */
typedef struct lin_server_scheduler_table_item_context
{
    lin_scheduler_frame_type_requests_t is_scheduled;           /*! Status of the frame. Depending of the frame
                                                                * type defined in @ref lin_server_scheduler_table_item_t
                                                                * the status should be updated correspondingly
                                                                */
} lin_server_scheduler_table_item_context_t;

/**
 * @brief Scheduler table item
 *
 * Stores configuration data for INFO/CTRL frames.
 * All members are stored in FLASH.
 */
typedef struct lin_server_scheduler_table_item
{
    uint_fast8_t frame_id;                                      //!< Frame identifier
    lin_frame_type_t frame_type;                                //!< Frame type(LIN_CTRL_FRAME/LIN_INFO_FRAME)
    lin_server_device_type_t slave_device_type;                 //!< Type of device related to the frame_id and frame_type
} lin_server_scheduler_table_item_t;

/**
 * @brief Scheduler table item definition
 *
 * Stores configuration data definition for INFO/CTRL frames.
 */
typedef struct lin_server_scheduler_table_item_def
{
    lin_server_scheduler_table_item_context_t item_context;     //!< modifiable frame data
    const lin_server_scheduler_table_item_t * item;             //!< reference to frame definition data located on flash
} lin_server_scheduler_table_item_def_t;

void lin_server_scheduler_init(const lin_server_scheduler_table_item_t * const table_items, lin_server_scheduler_table_item_def_t * const table_items_def, size_t table_items_size);
void lin_server_scheduler_enable(bool is_enabled);
void lin_server_scheduler_set_execution_callback(void (* execution_function)(void));

bool lin_server_scheduler_get_next_scheduled_frame(void);
bool lin_server_scheduler_is_ctrl_frame_scheduled(void);
lin_server_scheduler_table_item_def_t * lin_server_scheduler_get_current_scheduled_frame(void);
lin_server_scheduler_table_item_def_t * lin_server_scheduler_get_frame_by_device_type_and_id(lin_server_device_type_t device_type, lin_frame_type_t frame_type, uint8_t frame_id);

bool lin_server_scheduler_schedule_frame(lin_server_scheduler_table_item_def_t * table_item_def, lin_scheduler_frame_type_requests_t schedule_request);
void lin_server_scheduler_unschedule_frame(lin_server_scheduler_table_item_def_t * table_item_def, lin_scheduler_frame_type_requests_t schedule_request);
void lin_server_scheduler_unschedule_frames_by_device_type(lin_server_device_type_t device_type);
bool lin_server_scheduler_schedule_frame_by_device_type_and_id(lin_frame_type_t frame_type, uint8_t frame_id, lin_server_device_type_t device_type, lin_scheduler_frame_type_requests_t schedule_request);
bool lin_server_scheduler_is_frame_scheduled_type_set(lin_server_scheduler_table_item_def_t * table_item_def, lin_scheduler_frame_type_requests_t schedule_request);
bool lin_server_scheduler_is_frame_scheduled(lin_server_scheduler_table_item_def_t * table_item_def);
void lin_server_scheduler_change_time_period(uint32_t time_ms);

#endif //LIN_SERVER_SCHEDULING_TABLE__