/*! \file
    \brief Supporting functions for LIN connector over UART
    \author ManiramLTTS
    \author shenningsohn
    \author Andreas Lundeen
    \author Nenad Radulovic (nenad.b.radulovic@gmail.com)
*/

#ifndef LINDEV_UART_H_
#define LINDEV_UART_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_idf_version.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"   // Queue implementation
#include "freertos/semphr.h"  // Mutex implementation
#include "soc/uart_struct.h"  // UART device structure definition
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#include "driver/uart.h"  // UART port and configuration definition
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/myuart.h"  // UART port and configuration definition
#else
#include "driver/uart.h"  // UART port and configuration definition
#endif
#include "esp_attr.h"  // IRAM_ATTR definition
#include "esp_pm.h"    // PM lock handle
#include "fsm.h"       // FSM definition

#include "lin_common.h"

/**
 * @brief       LINDEV UART profiling
 *
 * By default, LIN profiling is disabled. On new products it is good practice to enable LIN
 * profiling code which will measure the critical code execution.
 *
 * @note        Disable this macro by setting to 0 (zero) when building for production.
 * @note        When this macro is not defined in product configuration header file the
 *              implementation will default to disabled (profiling is not turned on).
 */
#ifndef LINDEV_ENABLE_PROFILING
#define LINDEV_ENABLE_PROFILING 0
#endif

/**
 * @brief       LINDEV UART verbose logging
 *
 * By default, LINDEV UART will not print any message. Define this macro in local project
 * configuration file to enable verbose logging.
 *
 * @note        This will generate a lot of logs.
 * @note        Do not leave this option enabled in firmware production releases.
 * @note        When logging is enabled, 30ms is the minimal LIN period.
 */
#ifndef LINDEV_VERBOSE_LOG
#define LINDEV_VERBOSE_LOG 0
#endif

/**
 * @brief       LINDEV module event types
 *
 * LINDEV module generates events which are to be processed at task level context.
 */
typedef enum lindev_event_id
{
    LINDEV_EVENT_ID_CONTROL_FRAME = 0,         //!< Control frame was successfully received from master
    LINDEV_EVENT_ID_INFO_FRAME,                //!< Info frame was successfully sent to master
    LINDEV_EVENT_ID_DIAG_REQ_FRAME,            //!< Diagnostic request frame
    LINDEV_EVENT_ID_DIAG_RESP_FRAME,           //!< Diagnostic response frame
    LINDEV_EVENT_ID_NOT_FOR_US,                //!< We received a frame, but its PID does not match our serviceable frames
    LINDEV_EVENT_ID_ERROR_RX_CHECKSUM,         //!< Received frame checksum is invalid
    LINDEV_EVENT_ID_ERROR_RX_HEADER,           //!< Received frame header is invalid
    LINDEV_EVENT_ID_ERROR_RX_FIFO_FULL,        //!< While receiving, detected that the internal FIFO is full
    LINDEV_EVENT_ID_ERROR_RX_FIFO_OVF,         //!< While receiving, detected that the internal FIFO overflowed
    LINDEV_EVENT_ID_ERROR_TX_CHECKSUM,         //!< While transmitting, detected that checksum is invalid
    LINDEV_EVENT_ID_ERROR_TX_LENGTH,           //!< While transmitting, detected that all bytes were not sent
    LINDEV_EVENT_ID_ERROR_TX_FIFO_FULL,        //!< While transmitting, detected that the internal FIFO is full
    LINDEV_EVENT_ID_ERROR_TX_FIFO_OVF,         //!< While transmitting, detected that the internal FIFO overflowed
    LINDEV_EVENT_ID_ERROR_TX_BRK,              //!< While transmitting, detected a BREAK
    LINDEV_EVENT_ID_ERROR_BRK_FIFO_FULL,       //!< While waiting for BREAK, detected that the internal FIFO is full
    LINDEV_EVENT_ID_ERROR_BRK_FIFO_OVF,        //!< While waiting for BREAK, detected that the internal FIFO overflowed
    LINDEV_EVENT_ID_ERROR_UNKNOWN_IRQ,         //!< Unknown UART IRQ occured
    LINDEV_EVENT_ID_ERROR_INTERNAL_OVF,        //!< Internal buffer overflow
    LINDEV_EVENT_ID_ERROR_INVALID_FRAME_TYPE,  //!< Found a frame with invalid frame type
    LINDEV_EVENT_ID_MAX,                       //!< Used to mark maximum value in event_id
} lindev_event_id_t;

/**
 * @brief       Data type representing fixed size array of frame data (payload) bits.
 *
 * @note        All members of this structure are private to LINDEV.
 */
typedef struct LIN_PACKED lindev_frame_data
{
    uint8_t bytes[LIN_FRAME_DATA_LEN];
} lindev_frame_data_t;

/**
 * @brief       Definition of fields within one LIN frame.
 *
 * @note        All members of this structure are private to LINDEV.
 * @note        Type definition is for internal LINDEV module usage.
 */
typedef struct LIN_PACKED lindev_frame_fields
{
    uint8_t brk;               //!< Break field (always 0x00)
    uint8_t sync;              //!< SYNC field (always 0x55)
    uint8_t pid;               //!< PID, Parity and identifier field
    lindev_frame_data_t data;  //!< Data (payload), 8 bytes
    uint8_t checksum;          //!< Data and PID field checksum
} lindev_frame_fields_t;

/**
 * @brief       Data type representing fixed size array of frame raw bits.
 *
 * The structure offers two views of raw frame data:
 * - Bytes view is a classic view as array of bytes. This view is used by UART code to serialize the
 *   frame data.
 * - Fields view is used by higher level code to access individual members of the frame.
 *
 * Both views are overlayed over the same data in memory.
 *
 * @note        All members of this structure are private to LINDEV.
 * @note        Type definition is for internal LINDEV module usage.
 */
typedef struct lindev_frame_raw
{
    union lindev_frame_raw_view
    {
        uint8_t bytes[LIN_FRAME_RAW_LEN];  //!< Plain array view of raw frame data
        lindev_frame_fields_t fields;      //!< Structured view with data fields raw frame data
    } view;                                //!< Different views of the same data
} lindev_frame_raw_t;

/**
 * @brief       LINDEV frame
 *
 * This is used to store frame data. The data can be accesed by LINDEV user using
 * @ref lindev_set_frame_data and @ref lindev_get_frame_data.
 *
 * @note        All members of this structure are private to LINDEV.
 * @note        Type definition is for internal LINDEV module usage.
 */
typedef struct lindev_frame_t
{
    lindev_frame_raw_t frame_raw;  //!< Complete frame data
} lindev_frame_t;

/**
 * \brief      LIN frame definition
 *
 * This structure describes each LIN frame. Besides describing the frame, it also stores intermedia
 * data like flags and data payloads.
 */
typedef struct lindev_frame_def
{
    uint_fast8_t initial_id;  //!< Initial frame identifier
    lin_frame_type_t type;    //!< Control or info frame
} lindev_frame_def_t;

/**
 * @brief       LINDEV module configuration structure
 *
 * Allocate this structure in read-only memory. Use it to configure the LINDEV module during
 * initialization.
 *
 * @note        Callback functions receive the context pointer. Some callback functions,
 *              like sleep/wakeup might be called early, during the preinit stage. It is
 *              up to user to make sure that context pointer is properly initialized.
 */
typedef struct lindev_uart_config
{
    /**
     * @brief   Process incoming LIN event
     *
     * This callback implements LIN event handling logic. It receives the generic `context` pointer
     * and pointer to `lindev_event_t`. The event memory is managed by LINDEV module.
     */
    void (*process_event)(void *context, const lindev_event_id_t event_id, size_t frame_index);
    void (*wakeup)(void *context);  //!< Function which is to wake up LIN transceiver from low power state. Must be non-NULL pointer.
    void (*sleep)(void *context);   //!< Function which is to put LIN transceiver into low power state. Must be non-NULL pointer.
    void *context;                  //!< Generic pointer which is passed to function callbacks
    uint32_t sleep_timeout_ms;      //!< How many ms to wait before going to sleep
    uint32_t start_after_ms;        //!< Start-up delay in ms, use this to delay starting of LIN processing
} lindev_uart_config_t;

typedef struct lindev_fsm_event_data
{
    uint32_t rx_buffer_length;
    lindev_frame_raw_t rx_buffer;
} lindev_fsm_event_data_t;

typedef struct isr_event
{
    fsm_event_t fsm_event;
    lindev_fsm_event_data_t data;
} lindev_fsm_event_t;

/**
 * \brief       Lindev structure
 *
 * All members of this structure are private to this module.
 */
typedef struct lindev
{
    SemaphoreHandle_t lock;        //!< Mutex used to protect shared data in frame array `frames`
    fsm_t sm;                      //!< State machine handling the UART reception/transmission and function calls
    lindev_fsm_event_t isr_event;  //!< Local copy of isr_event (avoiding using stack allocation in ISR)
    lindev_frame_raw_t rx_buffer;  //!< RX buffer of UART
    size_t rx_buffer_length;       //!< Number of bytes in UART
    uart_dev_t *uart_dev;          //!< Pointer to UART device structure
#if CONFIG_PM_ENABLE
    esp_pm_lock_handle_t pm_lock;  //!< Power management (PM) lock
#endif
    size_t num_frame_defs;                                             //!< Number of defined frames
    const lindev_frame_def_t *frame_defs;                              //!< Pointer to array of frame definitions
    lindev_frame_t *frame_db;                                          //!< Pointer to array of frames
    size_t current_frame_index;                                        //!< Index of frame which was sent and now we are expecting the response for it.
    const lindev_uart_config_t *config;                                //!< Pointer to module configuration which contains callbacks to inform higher level code about events.
    lindev_fsm_event_t sm_event_queue_entries[LIN_EVENT_QUEUE_DEPTH];  //!< Queue data storage
    QueueHandle_t sm_event_queue;                                      //!< Queue handle of events which are sent to loop
    StaticQueue_t sm_event_queue_instance;                             //!< Queue structure instance
    lindev_frame_t diagnostic_request;                                 //!< Received diagnostic request frame
    lindev_frame_t diagnostic_response;                                //!< Prepared diagnostic response frame
    bool is_diagnostic_response_set;                                   //!< This flag is set to true when we have some response in diagnostic_response
    uint32_t event_mask;                                               //!< Event mask used for periodic tick timer
    enum lindev_state
    {
        LINDEV_STATE_NOT_STARTED_ENABLED,     //!< Initial state
        LINDEV_STATE_NOT_STARTED_DISABLED,    //!< LINDEV is not started, but it was disabled by call to disable function
        LINDEV_STATE_STARTED_ENABLED,         //!< LINDEV is started and is not disabled
        LINDEV_STATE_STARTED_DISABLED,        //!< LINDEV is started and currently is disabled
        LINDEV_STATE_PENDING_START_DISABLED,  //!< LINDEV is not started and is disabled
    } state;                                  //!< Holds current state of LINDEV initialization
#if (LINDEV_ENABLE_PROFILING == 1)
    uint32_t min_queue_space_left;       //!< Minimum spaces left in sm_event_queue
    uint64_t prof_isr_duration_acc;      //!< Profiling ISR duration accumulator
    uint64_t prof_loop_duration_acc;     //!< Profiling loop code duration accumulator
    uint32_t prof_isr_duration_us;       //!< Last ISR duration
    uint32_t prof_isr_avg_duration_us;   //!< Average ISR duration
    uint32_t prof_isr_max_duration_us;   //!< Maximum ISR duration
    uint32_t prof_loop_avg_duration_us;  //!< Loop code average duration
    uint32_t prof_loop_max_duration_us;  //!< Loop code maximum duration
    uint32_t prof_index;                 //!< Index counter used for accumulation and averaging
#endif                                   /* LINDEV_ENABLE_PROFILING == 1 */
} lindev_t;

/**
 * @brief       Pre-initialization procedure
 *
 * @param       lindev Pointer to allocated lindev instance
 * @param       config Configuration structure for LINDEV
 */
void lindev_preinit(lindev_t *lindev, const lindev_uart_config_t *config);

/**
 * @brief       Initialize lindev instance
 *
 * @param       lindev Pointer to allocated lindev instance
 * @param       uart_port Number of UART port, UART_NUM_0, UART_NUM_1, etc.
 * @param       uart_config Configuration for UART port.
 * @param       frame_defs Pointer to frame definitions array
 * @param       frames Pointer to frame array
 * @param       num_frame_defs Number of instances in @a frames array
 */
void lindev_init(lindev_t *lindev,
                 uart_port_t uart_port,
                 const uart_config_t *uart_config,
                 const lindev_frame_def_t *frame_defs,
                 lindev_frame_t *frames,
                 size_t num_frame_defs);

/**
 * @brief       Start LIN operation
 *
 * @param       lindev Pointer to initialized lindev instance.
 *
 * This function will start internal loop task and status timer. The internal loop task is used to
 * process all events that are comming from UART peripheral and events that are comming from
 * firmware calls to lindev_uart functions.
 *
 * The status timer is used to periodically check the LIN errors and prints them to logging facility.
 * If LIN profiling is enabled (@see LINDEV_ENABLE_PROFILING) then the profiling information is also
 * being printed from status timer.
 */
void lindev_start(lindev_t *lindev);

/**
 * @brief       Get data associated with given frame index
 *
 * @param       lindev Pointer to initialized and started lindev instance.
 * @param       Initial_frame_id Initial frame ID of the requested frame.
 * @param       frame_data Frame data buffer which will contain a copy of frame data.
 */
void lindev_get_frame_data(lindev_t *lindev, uint_fast8_t initial_frame_id, lindev_frame_data_t *frame_data);

/**
 * @brief       Get data associated with diagnostic request frame
 *
 * @param       lindev Pointer to initialized and started lindev instance.
 * @param       frame_data Frame data buffer which will contain a copy of diagnostic request frame data.
 */
void lindev_get_diag_req_frame_data(lindev_t *lindev, lindev_frame_data_t *frame_data);

/**
 * @brief       Set data associated with given frame index (safe)
 *
 * This function
 * @param       lindev Pointer to initialized and started lindev instance.
 * @param       initial_frame_id Initial frame ID of the requested frame.
 * @param       frame_data Frame data buffer which is to be set to frame.
 */
void lindev_set_frame_data(lindev_t *lindev, uint_fast8_t initial_frame_id, const lindev_frame_data_t *frame_data);

/**
 * @brief       Set diagnostic response frame data
 *
 * @param       lindev Pointer to initialized and started lindev instance.
 * @param       frame_data Frame data buffer whiich
 */
void lindev_set_diag_resp_frame_data(lindev_t *lindev, const lindev_frame_data_t *frame_data);

/**
 * @brief       Get initial frame identifier
 *
 * @param       lindev Pointer to initialized and started lindev instance.
 * @param       frame_index Index of frame. This index is obtained from @ref lindev_event_t
 *              structure instance or by numbering. The index is determined by frame description
 *              array order (passed to @ref lindev_init function).
 * @return      Frame initial identifier
 */
static inline uint_fast8_t lindev_get_initial_frame_id(const lindev_t *const lindev, uint32_t frame_index)
{
    return lindev->frame_defs[frame_index].initial_id;
}

/**
 * @brief       Get frame identifier
 *
 * @param       lindev Pointer to initialized lindev instance.
 * @param       frame_index Index of frame. This index is obtained from @ref lindev_event_t
 *              structure instance or by numbering. The index is determined by frame description
 *              array order (passed to @ref lindev_init function).
 * @return      Frame current identifier.
 */
uint32_t lindev_get_frame_id(const lindev_t *const lindev, uint32_t frame_index);

/**
 * @brief       Set new frame identifier.
 *
 * @param       lindev Pointer to initialized lindev instance.
 * @param       frame_index Index of frame. This index is obtained from @ref lindev_event_t
 *              structure instance or by numbering. The index is determined by frame description
 *              array order (passed to @ref lindev_init function).
 * @param       frame_id New frame identifier value.
 */
void lindev_set_frame_id(lindev_t *lindev, uint32_t frame_index, uint32_t frame_id);

/**
 * @brief       Get current frame protected identifier.
 *
 * @param       lindev Pointer to initialized lindev instance.
 * @param       frame_index Index of frame. This index is obtained from @ref lindev_event_t
 *              structure instance or by numbering. The index is determined by frame description
 *              array order (passed to @ref lindev_init function).
 * @return      frame_id Current frame protected identifier value.
 */
uint32_t lindev_get_frame_pid(const lindev_t *const lindev, uint32_t frame_index);

/**
 * @brief       Set new frame protected identifier
 *
 * @param       lindev Pointer to initialized lindev instance.
 * @param       frame_index Index of frame. This index is obtained from @ref lindev_event_t
 *              structure instance or by numbering. The index is determined by frame description
 *              array order (passed to @ref lindev_init function).
 * @param       frame_pid New frame protected identifier.
 */
void lindev_set_frame_pid(lindev_t *lindev, uint32_t frame_index, uint32_t frame_pid);

/**
 * @brief       Get the number of frame definitions
 *
 * @param       lindev Pointer to initialized lindev instance.
 * @return      size_t Number of frame definitions.
 */
static inline size_t lindev_get_num_frame_defs(const lindev_t *const lindev)
{
    return lindev->num_frame_defs;
}

/**
 * @brief       Access frame definition data
 *
 * @param       lindev Pointer to initialized lindev instance.
 * @param       frame_index Index of frame definition. This index is obtained from
 *              @ref lindev_event_t structure instance.
 * @return      const lindev_frame_def_t* Frame definition pointer.
 */
static inline const lindev_frame_def_t *lindev_get_frame_def(const lindev_t *const lindev, size_t frame_index)
{
    return &lindev->frame_defs[frame_index];
}

/**
 * @brief       Disable LIN ISR stack operation
 *
 * This function will disable relevant ISR and set appropriate internal flags to forbid
 * re-activation of ISR. After initialisation the ISR handling is enabled by default.
 *
 * @param       lindev Pointer to already initialized lindev structure.
 * @pre         The pointer @a lindev must be a non-NULL pointer.
 */
void lindev_disable(lindev_t *lindev);

/**
 * @brief       Enable previosly disabled LIN ISR stack operation
 *
 * This function will re-enable previosly disabled ISR and set appropriate internal flags. After
 * initialisation the ISR handling is enabled by default. It is forbidden to call this function
 * without a previos call to lindev_disable function.
 *
 * @param       lindev Pointer to already initialized lindev structure.
 * @pre         The pointer @a lindev must be a non-NULL pointer.
 */
void lindev_enable(lindev_t *lindev);

/**
 * @brief       Put the LIN transceiver and UART (if possible) to sleep
 *
 * @param       lindev Pointer to already initialized lindev structure.
 */
void lindev_sleep(lindev_t *lindev);

/**
 * @brief       Convert event enumerator to human readable text
 *
 * @param       event_type Enumerator of event
 * @return      const char* Pointer to text describing the event.
 */
const char *lindev_event_to_text(lindev_event_id_t event_type);

#endif /* LINDEV_UART_H_ */
