/*! \file
    \brief Supporting functions for LIN connector over UART
    \author ManiramLTTS
    \author shenningsohn
    \author Andreas Lundeen
    \author Nenad Radulovic (nenad.b.radulovic@gmail.com)
*/

#include "lindev_uart.h"
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "esp_idf_version.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hal/uart_ll.h"
#include "hal/uart_types.h"
#include "soc/uart_reg.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#define MAX_UART_NUM SOC_UART_NUM
#else
#define MAX_UART_NUM UART_NUM_MAX
#endif

#if (LINDEV_ENABLE_PROFILING == 1)
#include "hal_cpu.h"  // High precision time
#endif

#if CONFIG_UART_ISR_IN_IRAM
#warning "Placing UART ISR in IRAM section is forbidden when caching is enabled."
#endif

#if (LINDEV_IS_ISR_IN_IRAM == 1)
#define LINDEV_ISR_ATTR IRAM_ATTR
#else /* (LINDEV_IS_ISR_IN_IRAM == 1) */
#define LINDEV_ISR_ATTR
#endif /* !(LINDEV_IS_ISR_IN_IRAM == 1) */

#if (LINDEV_VERBOSE_LOG == 1)
#define LINDEV_LOG(level, format, ...) LOG(level, format, ##__VA_ARGS__)
#else
#define LINDEV_LOG(level, format, ...)
#endif

#if ((LINDEV_ENABLE_PROFILING == 1) && (LINDEV_VERBOSE_LOG == 1))
#warning "Enabling profiling and verbose logging is not recommended. Verbose logging will impact profiling results."
#endif

/**
 * @brief   Use this to define how many samples are taken for average code execution duration
 */
#define LINDEV_PROFILING_AVG_SAMPLES 256u

#define LINDEV_TICK_TIMER_PERIOD_MS 1000u

#define LINDEV_EVENT_NULL_FRAME_INDEX     SIZE_MAX
#define LINDEV_EVENT_LOOP_TASK_STACK_SIZE (3 * 1024u)

#define CONTAINER_OF(ptr, type, member) ((type *)((char *)ptr - offsetof(type, member)))

#define LIN_SLEEP_TIMEOUT_SEC 5u

/**
 * @brief   Internal state machine events
 *
 * Events that can be processed by internal state machine
 */
typedef enum sm_events
{
    SM_EVENTS_IRQ_BRK = FSM_USER_EVENT,  //!< UART interrupt request: Break detection event
    SM_EVENTS_IRQ_TOUT,                  //!< UART interrupt request: RX FIFO timeout event interrupt
    SM_EVENTS_IRQ_FULL,                  //!< UART interrupt request: RX FIFO full (default: 120 bytes) event interrupt
    SM_EVENTS_IRQ_OVF,                   //!< UART interrupt request: RX FIFO overflow (256 bytes) event interrupt
    SM_EVENTS_IRQ_UNKNOWN,               //!< Unknown UART interrupt request source event
    SM_EVENTS_SLEEP,                     //!< Sleep request, start putting LIN transceiver in sleep mode
    SM_EVENTS_DISABLE,                   //!< Request to disable LIN
    SM_EVENTS_ENABLE,                    //!< Request to enable LIN
} sm_events_t;

/**
 * @brief       Receive frame status
 */
typedef enum receive_frame_status
{
    RECEIVE_FRAME_STATUS_NOT_FOR_US,        //!< The received request does not match our frame
    RECEIVE_FRAME_STATUS_GOT_WRITE,         //!< We got a request for write (Control frame)
    RECEIVE_FRAME_STATUS_GOT_READ,          //!< We got a request for read (Info frame)
    RECEIVE_FRAME_STATUS_GOT_DIAG_REQ,      //!< We got a request for diagnostic
    RECEIVE_FRAME_STATUS_GOT_DIAG_RESP,     //!< We got a response for diagnostic
    RECEIVE_FRAME_STATUS_WAIT,              //!< We got a partial request, wait for more bytes
    RECEIVE_FRAME_STATUS_ERROR_OVERFLOW,    //!< We got more bytes then what we expected
    RECEIVE_FRAME_STATUS_ERROR_HEADER,      //!< A frame error was detected in header
    RECEIVE_FRAME_STATUS_ERROR_CHECKSUM,    //!< A frame error was detected by checksum
    RECEIVE_FRAME_STATUS_ERROR_INVLD_TYPE,  //!< A frame was found, but type doesn't match
} receive_frame_status_t;

/**
 * @brief       Validate response status
 */
typedef enum validate_response_status
{
    VALIDATE_RESPONSE_STATUS_OK,              //!< The response was sent correctly
    VALIDATE_RESPONSE_STATUS_WAIT,            //!< We got a partial response, waith for more bytes
    VALIDATE_RESPONSE_STATUS_ERROR_CHECKSUM,  //!< A frame error was detected
    VALIDATE_RESPONSE_STATUS_OVERFLOW,
} validate_response_status_t;

static void tick_timer_cb(TimerHandle_t timer);

static LINDEV_ISR_ATTR void lindev_fsm_event_init(lindev_fsm_event_t *event, uint32_t event_id);

/* Helper functions */
static bool is_frame_start_valid(const lindev_frame_fields_t *frame_fields);
static size_t find_frame_by_current_pid(const lindev_t *lindev, uint_fast8_t frame_pid);
static size_t find_frame_by_initial_id(const lindev_t *lindev, uint_fast8_t frame_id);
static lindev_t *isr_sm_to_lindev(fsm_t *sm);
static receive_frame_status_t receive_frame(lindev_t *lindev, const fsm_event_t *event);
static validate_response_status_t validate_response(lindev_t *lindev, const fsm_event_t *event);

/* State machine handler functions */
static void sm_handle_uart_reset_fifo(lindev_t *lindev);
static void sm_handle_enable_irq_all(lindev_t *lindev);
static void sm_handle_disable_irq_rx(lindev_t *lindev);
static void sm_handle_disable_irq_all(lindev_t *lindev);
static void sm_handle_send_event(lindev_t *lindev, lindev_event_id_t event_id, size_t frame_index);

/* State machine states */
static void sm_state_init(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_idle(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_receive_request(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_validate_response(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_disabled(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_sleeping(fsm_t *fsm, const fsm_event_t *event);

/* Miscellaneous functions */
static void LINDEV_ISR_ATTR uart_intr_handler(void *parameter);
static uart_dev_t *uart_port_to_dev(uart_port_t uart_port);
static void event_loop_task(void *parameter);
static void start_tasks(lindev_t *lindev);

static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief       Tick timer callback
 *
 * @param       timer Instance of FreeRTOS timer that is controlling this callback
 *
 * Tick timer callback is used to:
 * 1. notify about LIN errors in human readable form. The printing to logging facility is done here
 *    in order to avoid logging in time critical LIN code.
 * 2. Manage sleep timeout
 * 3. If profiling is enabled, the results will also be printed by this timer.
 */
static void tick_timer_cb(TimerHandle_t timer)
{
    lindev_t *lindev;
    uint32_t event_mask;
    uint32_t error_mask;
    static uint32_t sleep_ticks;

    lindev = pvTimerGetTimerID(timer);
    /*
     * We are getting the event_mask and clearing it. There might be a moment that new data is
     * available in event_mask during this fetching and clearing, but since this is for only human
     * readable text and sleeping we may allow for some skips in event reporting. The point of this
     * code is to propagate errors to logging facility only when some persistent error occurs and
     * for sleeping. By 2.2a standard we need to sleep between 4 to 10s. If we miss 1s most probably
     * we will catch it next time.
     */
    event_mask = lindev->event_mask;
    lindev->event_mask = 0u;

    // LINDEV_LOG(I, "event: %u", event_mask);
    /* Handle sleeping of LIN bus */
    if (event_mask == 0u)
    {
        sleep_ticks++;
        // LINDEV_LOG(I, "no event, sleep tick: %u", sleep_ticks);

        if (sleep_ticks == ((LIN_SLEEP_TIMEOUT_SEC * 1000u) / LINDEV_TICK_TIMER_PERIOD_MS))
        {
            sleep_ticks = 0u;
            lindev_sleep(lindev);
        }
    }
    else
    {
        sleep_ticks = 0u;
    }
    /* Get only errors in error_mask which is used for reporting */
    error_mask = event_mask & ~((0x1u << LINDEV_EVENT_ID_CONTROL_FRAME) |
                                (0x1u << LINDEV_EVENT_ID_INFO_FRAME) |
                                (0x1u << LINDEV_EVENT_ID_DIAG_REQ_FRAME) |
                                (0x1u << LINDEV_EVENT_ID_DIAG_RESP_FRAME) |
                                (0x1u << LINDEV_EVENT_ID_NOT_FOR_US));

    if (error_mask != 0u)
    {
        /* Go through all defined bits in error_mask */
        for (uint32_t error_bit = 0u; error_bit < LINDEV_EVENT_ID_MAX; error_bit++)
        {
            if (error_mask & (0x1u << error_bit))
            {
                /* Error bit index actually corresponds to event_id */
                lindev_event_id_t event_id = error_bit;

                LOG(W, "LIN error detected: %u (%s)", event_id, lindev_event_to_text(event_id));
            }
        }
    }
#if (LINDEV_ENABLE_PROFILING == 1)
    LOG(I, "ISR & TASK duration [us]: avg %u, max %u, avg %u, max %u",
        lindev->prof_isr_avg_duration_us,
        lindev->prof_isr_max_duration_us,
        lindev->prof_loop_avg_duration_us,
        lindev->prof_loop_max_duration_us);
    LOG(I, "Minimum space left in sm_event_queue: %u", lindev->min_queue_space_left)
#endif
}

static LINDEV_ISR_ATTR void lindev_fsm_event_init(lindev_fsm_event_t *event, uint32_t event_id)
{
    event->fsm_event.id = event_id;
    event->fsm_event.p_data = &event->data;
}

/**
 * @brief       Is frame BREAK, SYNC and PID valid?
 *
 * @param       lindev Pointer to lindev instance
 * @return      true Frame start field is valid
 * @return      false Frame start field is not valid
 */
static bool is_frame_start_valid(const lindev_frame_fields_t *frame_fields)
{
    return (frame_fields->brk == LIN_BREAK_FIELD_DATA) &&
           (frame_fields->sync == LIN_SYNC_FIELD_DATA);
}

/**
 * @brief       Get the LINDEV instance from SM instance
 *
 * @param       sm Pointer to ISR SM instance
 * @return      lindev_isr_state_t* Pointer to containing LINDEV instance
 */
static lindev_t *isr_sm_to_lindev(fsm_t *sm)
{
    return CONTAINER_OF(sm, lindev_t, sm);
}

/**
 * @brief       Find a frame which matching PID (protected ID).
 *
 * @param       lindev Pointer to context structure
 * @param       frame_pid Current frame PID value
 * @return      size_t Returns an index into frame table. When this index is equal to SIZE_MAX then
 *              it means we don't have such a frame with given PID.
 */
static size_t find_frame_by_current_pid(const lindev_t *lindev, uint_fast8_t frame_pid)
{
    for (size_t i = 0; i < lindev->num_frame_defs; i++)
    {
        if (lindev->frame_db[i].frame_raw.view.fields.pid == frame_pid)
        {
            return i;
        }
    }
    return SIZE_MAX;
}

static size_t find_frame_by_initial_id(const lindev_t *lindev, uint_fast8_t frame_id)
{
    for (size_t i = 0; i < lindev->num_frame_defs; i++)
    {
        if (lindev->frame_defs[i].initial_id == frame_id)
        {
            return i;
        }
    }
    return SIZE_MAX;
}

static receive_frame_status_t receive_frame_process_diag_request(lindev_t *lindev)
{
    uint8_t calculated_checksum;

    /* State that this frame is not mapped by user code. */
    lindev->current_frame_index = SIZE_MAX;
    /* See if we have enough bytes to start interpretting them */
    if (lindev->rx_buffer_length != LIN_FRAME_RAW_LEN)
    {
        /* We are still waiting for more bytes */
        return RECEIVE_FRAME_STATUS_WAIT;
    }
    // Verify checksum.
    calculated_checksum = lin_calculate_checksum(
        LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(LIN_FRAME_PID_DIAG_REQUEST),
        &lindev->rx_buffer.view.bytes[LIN_PID_FIELD_BYTE],
        LIN_FRAME_DATA_LEN + 1);
    if (lindev->rx_buffer.view.fields.checksum != calculated_checksum)
    {
        LINDEV_LOG(W,
                   "failed checksum in DIAG request: PID [%02x] %02x %02x %02x %02x %02x %02x %02x %02x [%02x]",
                   lindev->rx_buffer.view.fields.pid,
                   lindev->rx_buffer.view.fields.data.bytes[0],
                   lindev->rx_buffer.view.fields.data.bytes[1],
                   lindev->rx_buffer.view.fields.data.bytes[2],
                   lindev->rx_buffer.view.fields.data.bytes[3],
                   lindev->rx_buffer.view.fields.data.bytes[4],
                   lindev->rx_buffer.view.fields.data.bytes[5],
                   lindev->rx_buffer.view.fields.data.bytes[6],
                   lindev->rx_buffer.view.fields.data.bytes[7],
                   lindev->rx_buffer.view.fields.checksum);
        return RECEIVE_FRAME_STATUS_ERROR_CHECKSUM;
    }
    /*
     * Diagnostic request frames are processed synchronous to the state machine execution, therefore,
     * no mutex guard is needed here.
     */
    // NOTE: xSemaphoreTake(lindev->lock, portMAX_DELAY); is not needed here
    memcpy(
        lindev->diagnostic_request.frame_raw.view.bytes,
        lindev->rx_buffer.view.bytes,
        sizeof(lindev->diagnostic_request.frame_raw.view.bytes));
    // NOTE: xSemaphoreGive(lindev->lock); is not needed here
    // Reset a flag so LINDEV UART user needs to update the response
    lindev->is_diagnostic_response_set = false;
    return RECEIVE_FRAME_STATUS_GOT_DIAG_REQ;
}

static receive_frame_status_t receive_frame_process_diag_response(lindev_t *lindev)
{
    /* State that this frame is not mapped by user code. */
    lindev->current_frame_index = SIZE_MAX;

    /* Send the diagnostic response only when LINDEV UART user sets the response. */
    if (lindev->is_diagnostic_response_set == false)
    {
        return RECEIVE_FRAME_STATUS_NOT_FOR_US;
    }
    // Queue frame data and checksum for transmission.
    /*
     * Since the diagnosti response frame data can be written asynchronous to state machine
     * execution we need to serialize the access using a mutex.
     */
    xSemaphoreTake(lindev->lock, portMAX_DELAY);
    uart_ll_write_txfifo(
        lindev->uart_dev,
        &lindev->diagnostic_response.frame_raw.view.bytes[LIN_FRAME_START_LEN],
        LIN_FRAME_DATA_LEN + LIN_FRAME_CHECKSUM_LEN);
    xSemaphoreGive(lindev->lock);
    // Reset a flag so LINDEV UART user needs to update the response
    lindev->is_diagnostic_response_set = false;
    return RECEIVE_FRAME_STATUS_GOT_DIAG_RESP;
}

/**
 * @brief       Process incoming request messages from master
 *
 * @note        This function relies on `lindev->rx_buffer_length` being reset when it called for
 *              the first time and then the `lindev->rx_buffer_length` must be preserved when the
 *              function returns @ref RECEIVE_FRAME_STATUS_WAIT value.
 * @param       lindev
 * @return      receive_frame_status_t Status of operation. See @ref receive_frame_status_t for
 *              details.
 */
static receive_frame_status_t receive_frame(lindev_t *lindev, const fsm_event_t *event)
{
    const lindev_fsm_event_data_t *event_data = event->p_data;

    uint32_t able_to_receive_length = sizeof(lindev->rx_buffer.view.bytes) - lindev->rx_buffer_length;

    /* See if we can fit event bytes to working buffer */
    if (event_data->rx_buffer_length > able_to_receive_length)
    {
        LINDEV_LOG(E, "working buffer overflow: %u, %u", event_data->rx_buffer_length, able_to_receive_length);
        return RECEIVE_FRAME_STATUS_ERROR_OVERFLOW;
    }
    /* Copy event bytes to working buffer */
    memcpy(&lindev->rx_buffer.view.bytes[lindev->rx_buffer_length],
           event_data->rx_buffer.view.bytes,
           event_data->rx_buffer_length);
    lindev->rx_buffer_length += event_data->rx_buffer_length;

    /* See if we have enough bytes to start interpretting them */
    if ((lindev->rx_buffer_length != LIN_FRAME_START_LEN) &&
        (lindev->rx_buffer_length != LIN_FRAME_RAW_LEN))
    {
        /* We are still waiting for more bytes */
        return RECEIVE_FRAME_STATUS_WAIT;
    }
    /* See if frame start header is valid */
    if (!is_frame_start_valid(&lindev->rx_buffer.view.fields))
    {
        /* Header fields (BRK, SYNC) are invalid */
        LINDEV_LOG(W,
                   "header invalid: [BRK] %02x [SYNC] %02x [PID] %02x, %u byte(s)",
                   lindev->rx_buffer.view.fields.brk,
                   lindev->rx_buffer.view.fields.sync,
                   lindev->rx_buffer.view.fields.pid,
                   lindev->rx_buffer_length);
        return RECEIVE_FRAME_STATUS_ERROR_HEADER;
    }
    /* Extract frame identifier from PID field */
    uint8_t frame_pid = lindev->rx_buffer.view.fields.pid;
    if (LIN_IS_FRAME_PID_DIAG_REQUEST(frame_pid))
    {
        /* Process diagnostic request frame */
        return receive_frame_process_diag_request(lindev);
    }
    if (LIN_IS_FRAME_PID_DIAG_RESPONSE(frame_pid))
    {
        /* Process diagnostic response frame */
        return receive_frame_process_diag_response(lindev);
    }
    /*
     * We got a complete frame header and the address parity is correct. Got a proper info/ctrl
     * frame. Look for info frames with a matching indentifier.
     *
     * NOTE: Frame PID field is changed only by us, so we don't need to protect it with mutex
     */
    lindev->current_frame_index = find_frame_by_current_pid(lindev, frame_pid);
    if (lindev->current_frame_index == SIZE_MAX)
    {
        /* INFO/CTRL frame was sent by master but it is not for us. */
        LINDEV_LOG(I, "not for us, Frame PID: 0x%x", frame_pid);
        return RECEIVE_FRAME_STATUS_NOT_FOR_US;
    }
    /* Based on number of bytes assume this is an INFO or a CTRL frame */
    if (lindev->rx_buffer_length == LIN_FRAME_START_LEN)
    {
        /*
         * This is probably an INFO frame (concluded by the number of bytes) but we need to check
         * that.
         */
        /*
         * This might be CTRL frame but we only received 3 bytes of it, leading us to assume that
         * this is an INFO frame (which is wrong). If this is the case we need to wait for more
         * bytes.
         */
        if (lindev->frame_defs[lindev->current_frame_index].type == LIN_CONTROL_FRAME)
        {
            return RECEIVE_FRAME_STATUS_WAIT;
        }
        // Queue frame data and checksum for transmission.
        /*
         * Since info frame data can be written to async to state machine execution we need to
         * serialize the access using a mutex.
         */
        xSemaphoreTake(lindev->lock, portMAX_DELAY);
        uart_ll_write_txfifo(
            lindev->uart_dev,
            &lindev->frame_db[lindev->current_frame_index].frame_raw.view.bytes[LIN_FRAME_START_LEN],
            LIN_FRAME_DATA_LEN + LIN_FRAME_CHECKSUM_LEN);
        xSemaphoreGive(lindev->lock);
        return RECEIVE_FRAME_STATUS_GOT_READ;
    }
    else
    {
        uint8_t calculated_checksum;
        /*
         * This is probably a CTRL frame.
         *
         * We got a complete frame header and the address parity is correct. Look for control frames
         * with a matching identifier.
         */
        if (lindev->frame_defs[lindev->current_frame_index].type != LIN_CONTROL_FRAME)
        {
            /*
             * When we reach this point it means we have this frame mapped as an INFO frame which
             * suggest to us that we have wrong mappings in frame definitions.
             */
            LINDEV_LOG(W, "wrong frame mapping, index: %u", lindev->current_frame_index);
            return RECEIVE_FRAME_STATUS_ERROR_INVLD_TYPE;
        }
        // Verify checksum.
        calculated_checksum = lin_calculate_checksum(
            LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(frame_pid),
            &lindev->rx_buffer.view.bytes[LIN_PID_FIELD_BYTE],
            LIN_FRAME_DATA_LEN + 1);
        if (lindev->rx_buffer.view.fields.checksum != calculated_checksum)
        {
            LINDEV_LOG(W,
                       "failed checksum: PID [%02x] %02x %02x %02x %02x %02x %02x %02x %02x [%02x]",
                       lindev->rx_buffer.view.fields.pid,
                       lindev->rx_buffer.view.fields.data.bytes[0],
                       lindev->rx_buffer.view.fields.data.bytes[1],
                       lindev->rx_buffer.view.fields.data.bytes[2],
                       lindev->rx_buffer.view.fields.data.bytes[3],
                       lindev->rx_buffer.view.fields.data.bytes[4],
                       lindev->rx_buffer.view.fields.data.bytes[5],
                       lindev->rx_buffer.view.fields.data.bytes[6],
                       lindev->rx_buffer.view.fields.data.bytes[7],
                       lindev->rx_buffer.view.fields.checksum);
            return RECEIVE_FRAME_STATUS_ERROR_CHECKSUM;
        }
        // Success! Copy data to frame table
        /*
         * Control frames are processed synchronous to the state machine execution, therefore, no
         * mutex guard is needed here.
         */
        // NOTE: xSemaphoreTake(lindev->lock, portMAX_DELAY); is not needed here
        memcpy(
            lindev->frame_db[lindev->current_frame_index].frame_raw.view.bytes,
            lindev->rx_buffer.view.bytes,
            sizeof(lindev->frame_db[0].frame_raw.view.bytes));
        // NOTE: xSemaphoreGive(lindev->lock); is not needed here
        return RECEIVE_FRAME_STATUS_GOT_WRITE;
    }
}

/**
 * @brief       Validate the response
 *
 * @note        This function relies on `lindev->buffer_len` being set by @ref receive_frame
 *              function.
 * @param       lindev Pointer to initialized lindev instance
 * @return      validate_response_status_t Operation status, see @ref validate_response_status_t for
 *              details.
 */
static validate_response_status_t validate_response(lindev_t *lindev, const fsm_event_t *event)
{
    /*
     * This handler handles RX timeout detection, it will receive the bytes it sent while
     * processing INFO frame and check them.
     */
    const lindev_fsm_event_data_t *event_data = event->p_data;
    uint8_t calculated_checksum;

    uint32_t able_to_receive_length = sizeof(lindev->rx_buffer.view.bytes) - lindev->rx_buffer_length;

    /* See if we can fit event bytes to working buffer */
    if (event_data->rx_buffer_length > able_to_receive_length)
    {
        LINDEV_LOG(E,
                   "working buffer overflow: %u, %u",
                   event_data->rx_buffer_length,
                   able_to_receive_length);
        return VALIDATE_RESPONSE_STATUS_OVERFLOW;
    }
    /* Copy event bytes to working buffer */
    memcpy(&lindev->rx_buffer.view.bytes[lindev->rx_buffer_length],
           event_data->rx_buffer.view.bytes,
           event_data->rx_buffer_length);
    lindev->rx_buffer_length += event_data->rx_buffer_length;

    if (lindev->rx_buffer_length != LIN_FRAME_RAW_LEN)
    {
        return VALIDATE_RESPONSE_STATUS_WAIT;
    }
    calculated_checksum = lin_calculate_checksum(
        LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(lindev->rx_buffer.view.fields.pid),
        &lindev->rx_buffer.view.bytes[LIN_PID_FIELD_BYTE],
        LIN_FRAME_DATA_LEN + 1);
    if (lindev->rx_buffer.view.fields.checksum != calculated_checksum)
    {
        LINDEV_LOG(W,
                   "failed checksum: PID [%02x] %02x %02x %02x %02x %02x %02x %02x %02x [%02x]",
                   lindev->rx_buffer.view.fields.pid,
                   lindev->rx_buffer.view.fields.data.bytes[0],
                   lindev->rx_buffer.view.fields.data.bytes[1],
                   lindev->rx_buffer.view.fields.data.bytes[2],
                   lindev->rx_buffer.view.fields.data.bytes[3],
                   lindev->rx_buffer.view.fields.data.bytes[4],
                   lindev->rx_buffer.view.fields.data.bytes[5],
                   lindev->rx_buffer.view.fields.data.bytes[6],
                   lindev->rx_buffer.view.fields.data.bytes[7],
                   lindev->rx_buffer.view.fields.checksum);
        return VALIDATE_RESPONSE_STATUS_ERROR_CHECKSUM;
    }
    return VALIDATE_RESPONSE_STATUS_OK;
}

/**
 * @brief       Reset UART peripheral FIFO
 *
 * @param       lindev Poinnter to initialized lindev instance
 */
static void sm_handle_uart_reset_fifo(lindev_t *lindev)
{
    uart_ll_rxfifo_rst(lindev->uart_dev);
}

/**
 * @brief       Enable UART Break and RX FIFO IRQ
 *
 * @param       lindev Poinnter to initialized lindev instance
 */
static void sm_handle_enable_irq_all(lindev_t *lindev)
{
    uart_ll_clr_intsts_mask(lindev->uart_dev, UART_INTR_BRK_DET | UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_OVF);
    uart_ll_ena_intr_mask(lindev->uart_dev, UART_INTR_BRK_DET | UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_OVF);
}

/**
 * @brief       Disable all UART IRQ
 *
 * @param       lindev Poinnter to initialized lindev instance
 */
static void sm_handle_disable_irq_all(lindev_t *lindev)
{
    uart_ll_disable_intr_mask(lindev->uart_dev, UART_LL_INTR_MASK);
}

/**
 * @brief       Disable only UART RX FIFO IRQ
 *
 * @param       lindev Poinnter to initialized lindev instance
 */
static void sm_handle_disable_irq_rx(lindev_t *lindev)
{
    uart_ll_disable_intr_mask(lindev->uart_dev, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_OVF);
}

/**
 * @brief       Send the event to event loop
 *
 * @param       lindev Pointer to initialized lindev instance
 * @param       event_id Event identifier
 * @param       frame_index Index of frame in frame array
 */
static void sm_handle_send_event(lindev_t *lindev, lindev_event_id_t event_id, size_t frame_index)
{
    /* Report event in event_mask */
    lindev->event_mask |= (0x1u << event_id);
    lindev->config->process_event(lindev->config->context, event_id, frame_index);
}

/**
 * @brief       Initialization state of state machine
 *
 * This state on entry:
 * 1. resets the UART peripheral FIFO registers.
 * 2. wakes up the transceiver
 *
 * When the transceiver has waked up, the Break and RX interrupts are enabled.
 *
 * @param       fsm  Pointer to FSM object
 * @param       event Pointer to event
 */
static void sm_state_init(fsm_t *fsm, const fsm_event_t *event)
{
    lindev_t *lindev = isr_sm_to_lindev(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        sm_handle_uart_reset_fifo(lindev);
        sm_handle_enable_irq_all(lindev);
        // Wake up LIN transceiver so we are able to receive the first message, if any.
#if CONFIG_PM_ENABLE
        esp_pm_lock_acquire(lindev->pm_lock);
#endif
        lindev->config->wakeup(lindev->config->context);
        fsm_state_change(fsm, sm_state_idle);
        break;
    default:
        break;
    }
}

/**
 * @brief       Idle LIN state
 *
 * @param       fsm  Pointer to FSM object
 * @param       event Pointer to event
 */
static void sm_state_idle(fsm_t *fsm, const fsm_event_t *event)
{
    lindev_t *lindev = isr_sm_to_lindev(fsm);

    switch (event->id)
    {
    case SM_EVENTS_IRQ_BRK:
        fsm_state_change(fsm, sm_state_receive_request);
        break;
    case SM_EVENTS_IRQ_FULL:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_BRK_FIFO_FULL, LINDEV_EVENT_NULL_FRAME_INDEX);
        break;
    case SM_EVENTS_IRQ_OVF:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_BRK_FIFO_OVF, LINDEV_EVENT_NULL_FRAME_INDEX);
        break;
    case SM_EVENTS_IRQ_TOUT:
        /* LINDEV_EVENT_ID_ERROR_BRK_FIFO_TOUT is not needed because of:
         * - we receive TOUT while other slaves are communicating, just throw away data.
         */
        break;
    case SM_EVENTS_SLEEP:
        fsm_state_change(fsm, sm_state_sleeping);
        break;
    case SM_EVENTS_DISABLE:
        sm_handle_disable_irq_all(lindev);
        lindev->config->sleep(lindev->config->context);
        fsm_state_change(fsm, sm_state_disabled);
        break;
    case SM_EVENTS_IRQ_UNKNOWN:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_UNKNOWN_IRQ, LINDEV_EVENT_NULL_FRAME_INDEX);
    default:
        break;
    }
}

/**
 * @brief       Receive request LIN state
 *
 * @param       fsm  Pointer to FSM object
 * @param       event Pointer to event
 */
static void sm_state_receive_request(fsm_t *fsm, const fsm_event_t *event)
{
    lindev_t *lindev = isr_sm_to_lindev(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        lindev->rx_buffer_length = 0;  // buffer_clear()
        break;
    case SM_EVENTS_IRQ_BRK:
        /* LINDEV_EVENT_ID_ERROR_RX_BRK is not needed, because:
         * - we receive another BREAK during WAKE-UP
         */
        fsm_state_change(fsm, sm_state_receive_request);
        break;
    case SM_EVENTS_IRQ_FULL:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_RX_FIFO_FULL, LINDEV_EVENT_NULL_FRAME_INDEX);
        fsm_state_change(fsm, sm_state_idle);
        break;
    case SM_EVENTS_IRQ_OVF:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_RX_FIFO_OVF, LINDEV_EVENT_NULL_FRAME_INDEX);
        fsm_state_change(fsm, sm_state_idle);
        break;
    case SM_EVENTS_IRQ_TOUT:
    {
        receive_frame_status_t status;

        status = receive_frame(lindev, event);
        switch (status)
        {
        case RECEIVE_FRAME_STATUS_NOT_FOR_US:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_NOT_FOR_US, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        case RECEIVE_FRAME_STATUS_GOT_WRITE:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_CONTROL_FRAME, lindev->current_frame_index);
            fsm_state_change(fsm, sm_state_idle);
            break;
        case RECEIVE_FRAME_STATUS_GOT_DIAG_REQ:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_DIAG_REQ_FRAME, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        case RECEIVE_FRAME_STATUS_GOT_READ:
        case RECEIVE_FRAME_STATUS_GOT_DIAG_RESP:
            fsm_state_change(fsm, sm_state_validate_response);
            break;
        case RECEIVE_FRAME_STATUS_WAIT:
            /* Do not change SM state here, we are still waiting for more bytes */
            break;
        case RECEIVE_FRAME_STATUS_ERROR_HEADER:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_RX_HEADER, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        case RECEIVE_FRAME_STATUS_ERROR_CHECKSUM:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_RX_CHECKSUM, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        case RECEIVE_FRAME_STATUS_ERROR_INVLD_TYPE:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_INVALID_FRAME_TYPE, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        default:  // RECEIVE_FRAME_STATUS_ERROR_OVERFLOW:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_INTERNAL_OVF, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        }
        break;
    }
    case SM_EVENTS_SLEEP:
        LINDEV_LOG(W, "Sleep ignored.");
        break;
    case SM_EVENTS_DISABLE:
        sm_handle_disable_irq_all(lindev);
        lindev->config->sleep(lindev->config->context);
        fsm_state_change(fsm, sm_state_disabled);
        break;
    case SM_EVENTS_IRQ_UNKNOWN:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_UNKNOWN_IRQ, LINDEV_EVENT_NULL_FRAME_INDEX);
        break;
    default:
        break;
    }
}

static void sm_state_validate_response(fsm_t *fsm, const fsm_event_t *event)
{
    lindev_t *lindev = isr_sm_to_lindev(fsm);

    switch (event->id)
    {
    case SM_EVENTS_IRQ_BRK:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_TX_BRK, LINDEV_EVENT_NULL_FRAME_INDEX);
        fsm_state_change(fsm, sm_state_receive_request);
        break;
    case SM_EVENTS_IRQ_FULL:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_TX_FIFO_FULL, LINDEV_EVENT_NULL_FRAME_INDEX);
        fsm_state_change(fsm, sm_state_idle);
        break;
    case SM_EVENTS_IRQ_OVF:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_TX_FIFO_OVF, LINDEV_EVENT_NULL_FRAME_INDEX);
        fsm_state_change(fsm, sm_state_idle);
        break;
    case SM_EVENTS_IRQ_TOUT:
    {
        validate_response_status_t status;

        status = validate_response(lindev, event);
        switch (status)
        {
        case VALIDATE_RESPONSE_STATUS_OK:
            /* Distinguish between mapped INFO frame and a diagnostic response frame. */
            if (lindev->current_frame_index != SIZE_MAX)
            {
                /* When we have a frame index then that is an INFO frame */
                sm_handle_send_event(lindev, LINDEV_EVENT_ID_INFO_FRAME, lindev->current_frame_index);
            }
            else
            {
                sm_handle_send_event(lindev, LINDEV_EVENT_ID_DIAG_RESP_FRAME, LINDEV_EVENT_NULL_FRAME_INDEX);
            }
            fsm_state_change(fsm, sm_state_idle);
            break;
        case VALIDATE_RESPONSE_STATUS_WAIT:
            break;
        case VALIDATE_RESPONSE_STATUS_ERROR_CHECKSUM:
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_TX_CHECKSUM, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        default:  // VALIDATE_RESPONSE_STATUS_OVERFLOW
            sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_INTERNAL_OVF, LINDEV_EVENT_NULL_FRAME_INDEX);
            fsm_state_change(fsm, sm_state_idle);
            break;
        }
        break;
    }
    case SM_EVENTS_SLEEP:
        LINDEV_LOG(W, "Sleep ignored.");
        break;
    case SM_EVENTS_DISABLE:
        sm_handle_disable_irq_rx(lindev);
        lindev->config->sleep(lindev->config->context);
        fsm_state_change(fsm, sm_state_disabled);
        break;
    case SM_EVENTS_IRQ_UNKNOWN:
        sm_handle_send_event(lindev, LINDEV_EVENT_ID_ERROR_UNKNOWN_IRQ, LINDEV_EVENT_NULL_FRAME_INDEX);
        break;
    default:
        break;
    }
}

static void sm_state_disabled(fsm_t *fsm, const fsm_event_t *event)
{
    lindev_t *lindev = isr_sm_to_lindev(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        sm_handle_disable_irq_all(lindev);
        lindev->config->sleep(lindev->config->context);
#if CONFIG_PM_ENABLE
        esp_pm_lock_release(lindev->pm_lock);
#endif
        /* NOTE: We can use logging facility since after this the LIN is disabled. */
        LOG(I, "went to disabled");
        break;
    case SM_EVENTS_ENABLE:
        /* NOTE:
         * Do not acquire PM lock yet since we are eventually going into sleeping state.
         */
        fsm_state_change(fsm, sm_state_init);
        LINDEV_LOG(I, "we were enabled");
        break;
    default:
        break;
    }
}

static void sm_state_sleeping(fsm_t *fsm, const fsm_event_t *event)
{
    lindev_t *lindev = isr_sm_to_lindev(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        sm_handle_disable_irq_rx(lindev);
        lindev->config->sleep(lindev->config->context);
#if CONFIG_PM_ENABLE
        esp_pm_lock_release(lindev->pm_lock);
#endif
        /* NOTE: We can use logging facility since after this the LIN is sleeping. */
        LOG(I, "went to sleep");
        break;
    case SM_EVENTS_IRQ_BRK:
#if CONFIG_PM_ENABLE
        esp_pm_lock_acquire(lindev->pm_lock);
#endif
        lindev->config->wakeup(lindev->config->context);
        sm_handle_enable_irq_all(lindev);
        fsm_state_change(fsm, sm_state_receive_request);
        LINDEV_LOG(I, "we have woken up");
        break;
    case SM_EVENTS_DISABLE:
        fsm_state_change(fsm, sm_state_disabled);
        break;
    default:
        break;
    }
}

/**
 * @brief       UART peripheral interrupt handler
 *
 * @param       parameter Pointer to lindev instance
 */
static void LINDEV_ISR_ATTR uart_intr_handler(void *parameter)
{
    lindev_t *lindev = parameter;
    BaseType_t is_a_task_woken = pdFALSE;  //!< A flag used to signal to FreeRTOS that a context switch is needed
    uint32_t uart_intr_status = uart_ll_get_intsts_mask(lindev->uart_dev);

#if (LINDEV_ENABLE_PROFILING == 1)
    int64_t prof_isr_begin = hal_cpu_get_micros();
#endif
    if (uart_intr_status & UART_RXFIFO_FULL_INT_ST_M)
    {
        uart_ll_rxfifo_rst(lindev->uart_dev);
        uart_ll_clr_intsts_mask(lindev->uart_dev, UART_RXFIFO_FULL_INT_CLR_M);
        lindev_fsm_event_init(&lindev->isr_event, SM_EVENTS_IRQ_FULL);
        xQueueSendFromISR(lindev->sm_event_queue, &lindev->isr_event, &is_a_task_woken);
    }
    else if (uart_intr_status & UART_RXFIFO_OVF_INT_ST_M)
    {
        uart_ll_rxfifo_rst(lindev->uart_dev);
        uart_ll_clr_intsts_mask(lindev->uart_dev, UART_RXFIFO_OVF_INT_CLR_M);
        lindev_fsm_event_init(&lindev->isr_event, SM_EVENTS_IRQ_OVF);
        xQueueSendFromISR(lindev->sm_event_queue, &lindev->isr_event, &is_a_task_woken);
    }
    else if (uart_intr_status & UART_BRK_DET_INT_ST_M)
    {
        uart_ll_clr_intsts_mask(lindev->uart_dev, UART_BRK_DET_INT_CLR_M);
        lindev_fsm_event_init(&lindev->isr_event, SM_EVENTS_IRQ_BRK);
        xQueueSendFromISR(lindev->sm_event_queue, &lindev->isr_event, &is_a_task_woken);
    }
    else if (uart_intr_status & UART_RXFIFO_TOUT_INT_ST_M)
    {
        lindev_fsm_event_init(&lindev->isr_event, SM_EVENTS_IRQ_TOUT);
        uint32_t rxfifo_len = uart_ll_get_rxfifo_len(lindev->uart_dev);
        lindev->isr_event.data.rx_buffer_length = MIN(LIN_FRAME_RAW_LEN, rxfifo_len);

        uart_ll_read_rxfifo(lindev->uart_dev, lindev->isr_event.data.rx_buffer.view.bytes, lindev->isr_event.data.rx_buffer_length);
        uart_ll_clr_intsts_mask(lindev->uart_dev, UART_RXFIFO_TOUT_INT_CLR_M);
        xQueueSendFromISR(lindev->sm_event_queue, &lindev->isr_event, &is_a_task_woken);
    }
    else
    {
        lindev_fsm_event_init(&lindev->isr_event, SM_EVENTS_IRQ_UNKNOWN);
        xQueueSendFromISR(lindev->sm_event_queue, &lindev->isr_event, &is_a_task_woken);
    }

#if (LINDEV_ENABLE_PROFILING == 1)
    int64_t prof_isr_end = hal_cpu_get_micros();
    lindev->prof_isr_duration_us = (uint32_t)(prof_isr_end - prof_isr_begin);
    uint32_t min_queue_space_left = uxQueueSpacesAvailable(lindev->sm_event_queue);
    if (lindev->min_queue_space_left > min_queue_space_left)
    {
        lindev->min_queue_space_left = min_queue_space_left;
    }
#endif
    if (is_a_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief       Get UART device instance based on UART port number
 *
 * @param       uart_port UART port number
 * @return      uart_dev_t * Pointer to UART device instance
 */
static uart_dev_t *uart_port_to_dev(uart_port_t uart_port)
{
    switch (uart_port)
    {
#if (MAX_UART_NUM > 0)
    case UART_NUM_0:
    {
        return &UART0;
        break;
    }
#endif
#if (MAX_UART_NUM > 1)
    case UART_NUM_1:
    {
        return &UART1;
        break;
    }
#endif
#if (MAX_UART_NUM > 2)
    case UART_NUM_2:
    {
        return &UART2;
        break;
    }
#endif
#if (MAX_UART_NUM > 3)
    case UART_NUM_3:
    {
        return &UART3;
        break;
    }
#endif
    default:
        return NULL;
        break;
    }
}

/**
 * @brief       LIN transceiver power management task
 *
 * @param       parameter Pointer to lindev instance
 *
 * This is the main event loop task in LINDEV UART. It will fetch the events from event queue and
 * process them using the state machine.
 */
static void event_loop_task(void *parameter)
{
    lindev_t *lindev = parameter;
    lindev_fsm_event_t isr_event;

    /* NOTE:
     * We are adding a delay here because UART shall start after DDM2 owner publishes all parameter
     * values, otherwise, LIN will send wrong data at start-up (zero).
     *
     * This is a crude way to accomplish this and it will be reworked in the future.
     */
    vTaskDelay(pdMS_TO_TICKS(lindev->config->start_after_ms));

    fsm_initialize(&lindev->sm, sm_state_init);

    LINDEV_LOG(I, "Started LIN event loop after %u ms", lindev->config->start_after_ms);
    while (true)
    {
        BaseType_t retval = xQueueReceive(lindev->sm_event_queue, &isr_event, portMAX_DELAY);
        TRUE_CHECK(retval == pdPASS);
        isr_event.fsm_event.p_data = &isr_event.data;
#if (LINDEV_ENABLE_PROFILING == 1)
        int64_t prof_loop_begin = hal_cpu_get_micros();
#endif
        fsm_state_dispatch(&lindev->sm, &isr_event.fsm_event);
#if (LINDEV_ENABLE_PROFILING == 1)
        uint32_t current_loop_duration_us;

        /* Get current loop duration */
        current_loop_duration_us = (uint32_t)(hal_cpu_get_micros() - prof_loop_begin);
        /* Calculate AVG and MAX for loop code */
        if (lindev->prof_loop_max_duration_us < current_loop_duration_us)
        {
            lindev->prof_loop_max_duration_us = current_loop_duration_us;
        }
        lindev->prof_loop_duration_acc += (uint64_t)current_loop_duration_us;
        if (lindev->prof_index == (LINDEV_PROFILING_AVG_SAMPLES - 1u))
        {
            lindev->prof_loop_avg_duration_us = lindev->prof_isr_duration_acc / LINDEV_PROFILING_AVG_SAMPLES;
            lindev->prof_loop_duration_acc = 0u;
        }

        /* Calculate AVG and MAX for ISR code */
        if (lindev->prof_isr_max_duration_us < lindev->prof_isr_duration_us)
        {
            lindev->prof_isr_max_duration_us = lindev->prof_isr_duration_us;
        }
        lindev->prof_isr_duration_acc += (uint64_t)lindev->prof_isr_duration_us;
        if (lindev->prof_index == (LINDEV_PROFILING_AVG_SAMPLES - 1u))
        {
            lindev->prof_isr_avg_duration_us = lindev->prof_isr_duration_acc / LINDEV_PROFILING_AVG_SAMPLES;
            lindev->prof_isr_duration_acc = 0u;
        }
        /* Increment and reset the index counter if needed */
        lindev->prof_index++;
        if (lindev->prof_index == LINDEV_PROFILING_AVG_SAMPLES)
        {
            lindev->prof_index = 0u;
        }
#endif /* (LINDEV_ENABLE_PROFILING == 1) */
    }
}

static void start_tasks(lindev_t *lindev)
{
    BaseType_t status;
    TimerHandle_t status_timer;

    status = xTaskCreate(event_loop_task, "lindev event loop", LINDEV_EVENT_LOOP_TASK_STACK_SIZE, lindev, xTASK_PRIORITY_REALTIME, NULL);
    TRUE_CHECK(status == pdPASS);
    status_timer = xTimerCreate("LIN tick", pdMS_TO_TICKS(LINDEV_TICK_TIMER_PERIOD_MS), pdTRUE, lindev, tick_timer_cb);
    TRUE_CHECK(status_timer != NULL);
    status = xTimerStart(status_timer, portMAX_DELAY);
    TRUE_CHECK(status == pdPASS);
}

void lindev_preinit(lindev_t *lindev, const lindev_uart_config_t *config)
{
    memset(lindev, 0, sizeof(*lindev));
    lindev->config = config;
    config->sleep(config->context);
}

void lindev_init(lindev_t *lindev,
                 uart_port_t uart_port,
                 const uart_config_t *uart_config,
                 const lindev_frame_def_t *frame_defs,
                 lindev_frame_t *frames,
                 size_t num_frame_defs)
{
    uart_dev_t *uart_dev;
#if CONFIG_PM_ENABLE
    int retval;  // Used only for PM return value
#endif

    uart_dev = uart_port_to_dev(uart_port);
    TRUE_CHECK_RETURN(uart_dev);
    lindev->uart_dev = uart_dev;
    lindev->frame_defs = frame_defs;
    lindev->frame_db = frames;
    lindev->num_frame_defs = num_frame_defs;
    lindev->lock = xSemaphoreCreateMutex();
    TRUE_CHECK_RETURN(lindev->lock != NULL);
    lindev->sm_event_queue = xQueueCreateStatic(
        ELEMENTS(lindev->sm_event_queue_entries),
        sizeof(lindev->sm_event_queue_entries[0]),
        (uint8_t *)&lindev->sm_event_queue_entries[0],
        &lindev->sm_event_queue_instance);
    TRUE_CHECK_RETURN(lindev->sm_event_queue != NULL);
#if CONFIG_PM_ENABLE
    retval = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "lin", &lindev->pm_lock);
    TRUE_CHECK_RETURN(retval == ESP_OK);
#endif
    ZERO_CHECK(uart_driver_install(uart_port, 256, 0, 0, NULL, 0));
    ZERO_CHECK(uart_disable_intr_mask(uart_port, UART_LL_INTR_MASK));
    ZERO_CHECK(uart_isr_free(uart_port));
    ZERO_CHECK(uart_isr_register(uart_port, uart_intr_handler, lindev, 0, NULL));
    ZERO_CHECK(uart_disable_pattern_det_intr(uart_port));
    ZERO_CHECK(uart_disable_tx_intr(uart_port));
    ZERO_CHECK(uart_disable_rx_intr(uart_port));
    ZERO_CHECK(uart_set_rx_timeout(uart_port, 3));
    ZERO_CHECK(uart_disable_intr_mask(uart_port, UART_LL_INTR_MASK));
    ZERO_CHECK(uart_param_config(uart_port, uart_config));
    ZERO_CHECK(uart_set_pin(uart_port, CONNECTOR_LINDEV_UART_TX, CONNECTOR_LINDEV_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    for (size_t i = 0; i < lindev->num_frame_defs; i++)
    {
        memset(&lindev->frame_db[i].frame_raw.view.fields.data,
               0xffu,
               sizeof(lindev->frame_db[0].frame_raw.view.fields.data));
        lindev_set_frame_id(lindev, i, lindev->frame_defs[i].initial_id);
    }
#if (LINDEV_ENABLE_PROFILING == 1)
    LOG(I, "event queue depth: %u entries", LIN_EVENT_QUEUE_DEPTH);
    LOG(I, "event queue item size: %u bytes", sizeof(lindev_fsm_event_t));
    LOG(I, "Number of application frames: %u", num_frame_defs);
    LOG(I, "total application frame storage: %u bytes", sizeof(lindev_frame_t) * num_frame_defs);
    lindev->min_queue_space_left = LIN_EVENT_QUEUE_DEPTH;
    lindev->prof_isr_duration_us = 0u;
    lindev->prof_isr_avg_duration_us = 0u;
    lindev->prof_isr_max_duration_us = 0u;
    lindev->prof_loop_avg_duration_us = 0u;
    lindev->prof_loop_max_duration_us = 0u;
    lindev->prof_index = 0u;
#endif
}

void lindev_start(lindev_t *lindev)
{
    taskENTER_CRITICAL(&state_lock);

    switch (lindev->state)
    {
    case LINDEV_STATE_NOT_STARTED_ENABLED:
        lindev->state = LINDEV_STATE_STARTED_ENABLED;
        taskEXIT_CRITICAL(&state_lock);
        start_tasks(lindev);
        break;
    case LINDEV_STATE_NOT_STARTED_DISABLED:
        lindev->state = LINDEV_STATE_PENDING_START_DISABLED;
        taskEXIT_CRITICAL(&state_lock);
        break;
    case LINDEV_STATE_STARTED_ENABLED:
    case LINDEV_STATE_STARTED_DISABLED:
    case LINDEV_STATE_PENDING_START_DISABLED:
    default:
        taskEXIT_CRITICAL(&state_lock);
        break;
    }
}

void lindev_get_frame_data(lindev_t *lindev, uint_fast8_t initial_frame_id, lindev_frame_data_t *frame_data)
{
    size_t frame_index;

    frame_index = find_frame_by_initial_id(lindev, initial_frame_id);
    if (frame_index == SIZE_MAX)
    {
        return;
    }
    memcpy(frame_data->bytes,
           lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes,
           sizeof(frame_data->bytes));
    LINDEV_LOG(I,
               "get data: PID [%02x] %02x %02x %02x %02x %02x %02x %02x %02x [%02x]",
               lindev->frame_db[frame_index].frame_raw.view.fields.pid,
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[0],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[1],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[2],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[3],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[4],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[5],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[6],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[7],
               lindev->frame_db[frame_index].frame_raw.view.fields.checksum);
}

void lindev_get_diag_req_frame_data(lindev_t *lindev, lindev_frame_data_t *frame_data)
{
    memcpy(frame_data->bytes,
           lindev->diagnostic_request.frame_raw.view.fields.data.bytes,
           sizeof(frame_data->bytes));
    LINDEV_LOG(I,
               "get diagnostic request data: PID [%02x] %02x %02x %02x %02x %02x %02x %02x %02x [%02x]",
               lindev->diagnostic_request.frame_raw.view.fields.pid,
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[0],
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[1],
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[2],
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[3],
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[4],
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[5],
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[6],
               lindev->diagnostic_request.frame_raw.view.fields.data.bytes[7],
               lindev->diagnostic_request.frame_raw.view.fields.checksum);
}

void lindev_set_frame_data(lindev_t *lindev, uint_fast8_t initial_frame_id, const lindev_frame_data_t *frame_data)
{
    size_t frame_index;

    frame_index = find_frame_by_initial_id(lindev, initial_frame_id);
    /* Check if we have this frame */
    if (frame_index == SIZE_MAX)
    {
        return;
    }
    /* The given frame must be INFO frame */
    if (lindev->frame_defs[frame_index].type == LIN_CONTROL_FRAME)
    {
        return;
    }
    /* Since INFO frames may be updated asynchronously to the state machine we must protect the data
     * with a mutex lock.
     */
    xSemaphoreTake(lindev->lock, portMAX_DELAY);
    /* Set data */
    memcpy(lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes,
           frame_data->bytes,
           sizeof(lindev->frame_db[0].frame_raw.view.fields.data.bytes));
    /* Update checksum for new data */
    lindev->frame_db[frame_index].frame_raw.view.fields.checksum = lin_calculate_checksum(
        LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(lindev->frame_db[frame_index].frame_raw.view.fields.pid),
        &lindev->frame_db[frame_index].frame_raw.view.bytes[LIN_PID_FIELD_BYTE],
        LIN_FRAME_DATA_LEN + 1);
    xSemaphoreGive(lindev->lock);
    LINDEV_LOG(I,
               "set data: PID [%02x] %02x %02x %02x %02x %02x %02x %02x %02x [%02x]",
               lindev->frame_db[frame_index].frame_raw.view.fields.pid,
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[0],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[1],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[2],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[3],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[4],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[5],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[6],
               lindev->frame_db[frame_index].frame_raw.view.fields.data.bytes[7],
               lindev->frame_db[frame_index].frame_raw.view.fields.checksum);
}

void lindev_set_diag_resp_frame_data(lindev_t *lindev, const lindev_frame_data_t *frame_data)
{
    /* Diagnostic frames are handled in such a way that diagnostic request always comes before
     * diagnostic response. Since the diagnostic response is always created during the synchronous
     * processing of diagnostic requests the data protection mutex is not needed.
     */
    /*
     * When ever we update the data we need to calculate proper checksum as well since the raw
     * buffer will be sent as response.
     */
    /* Frame start fields are always the same for diagnostic response frame */
    lindev->diagnostic_response.frame_raw.view.fields.brk = LIN_BREAK_FIELD_DATA;
    lindev->diagnostic_response.frame_raw.view.fields.sync = LIN_SYNC_FIELD_DATA;
    lindev->diagnostic_response.frame_raw.view.fields.pid = LIN_FRAME_PID_DIAG_RESPONSE;
    /* Set data */
    memcpy(lindev->diagnostic_response.frame_raw.view.fields.data.bytes,
           frame_data->bytes,
           sizeof(lindev->diagnostic_response.frame_raw.view.fields.data.bytes));
    /* Update checksum for new data */
    lindev->diagnostic_response.frame_raw.view.fields.checksum = lin_calculate_checksum(
        LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(LIN_FRAME_PID_DIAG_RESPONSE),
        &lindev->diagnostic_response.frame_raw.view.bytes[LIN_PID_FIELD_BYTE],
        LIN_FRAME_DATA_LEN + 1);
    lindev->is_diagnostic_response_set = true;
}

uint32_t lindev_get_frame_id(const lindev_t *const lindev, uint32_t frame_index)
{
    return LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(lindev->frame_db[frame_index].frame_raw.view.fields.pid);
}

void lindev_set_frame_id(lindev_t *lindev, uint32_t frame_index, uint32_t frame_id)
{
    uint_fast8_t parity = lin_calculate_parity(frame_id) << LIN_PID_FIELD_PARITY_POS;
    uint_fast8_t frame_pid = parity | frame_id;
    lindev->frame_db[frame_index].frame_raw.view.fields.pid = frame_pid;
    lindev->frame_db[frame_index].frame_raw.view.fields.checksum = lin_calculate_checksum(
        frame_id,
        &lindev->frame_db[frame_index].frame_raw.view.bytes[LIN_PID_FIELD_BYTE],
        LIN_FRAME_DATA_LEN + 1);
}

uint32_t lindev_get_frame_pid(const lindev_t *const lindev, uint32_t frame_index)
{
    return lindev->frame_db[frame_index].frame_raw.view.fields.pid;
}

void lindev_set_frame_pid(lindev_t *lindev, uint32_t frame_index, uint32_t frame_pid)
{
    lindev->frame_db[frame_index].frame_raw.view.fields.pid = frame_pid;
    lindev->frame_db[frame_index].frame_raw.view.fields.checksum = lin_calculate_checksum(
        LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(frame_pid),
        &lindev->frame_db[frame_index].frame_raw.view.bytes[LIN_PID_FIELD_BYTE],
        LIN_FRAME_DATA_LEN + 1);
    LINDEV_LOG(I, "frame index: %u, frame PID: %u", frame_index, frame_pid);
}

void lindev_disable(lindev_t *lindev)
{
    taskENTER_CRITICAL(&state_lock);
    switch (lindev->state)
    {
    case LINDEV_STATE_NOT_STARTED_ENABLED:
        lindev->state = LINDEV_STATE_NOT_STARTED_DISABLED;
        taskEXIT_CRITICAL(&state_lock);
        break;
    case LINDEV_STATE_STARTED_ENABLED:
    {
        lindev_fsm_event_t disable_event;

        lindev->state = LINDEV_STATE_STARTED_DISABLED;
        taskEXIT_CRITICAL(&state_lock);
        lindev_fsm_event_init(&disable_event, SM_EVENTS_DISABLE);
        xQueueSend(lindev->sm_event_queue, &disable_event, portMAX_DELAY);
        break;
    }
    case LINDEV_STATE_NOT_STARTED_DISABLED:
    case LINDEV_STATE_STARTED_DISABLED:
    case LINDEV_STATE_PENDING_START_DISABLED:
    default:
        taskEXIT_CRITICAL(&state_lock);
        break;
    }
}

void lindev_enable(lindev_t *lindev)
{
    taskENTER_CRITICAL(&state_lock);
    switch (lindev->state)
    {
    case LINDEV_STATE_NOT_STARTED_DISABLED:
        lindev->state = LINDEV_STATE_NOT_STARTED_ENABLED;
        taskEXIT_CRITICAL(&state_lock);
        break;
    case LINDEV_STATE_STARTED_DISABLED:
    {
        lindev_fsm_event_t enable_event;

        lindev->state = LINDEV_STATE_STARTED_ENABLED;
        taskEXIT_CRITICAL(&state_lock);
        lindev_fsm_event_init(&enable_event, SM_EVENTS_ENABLE);
        xQueueSend(lindev->sm_event_queue, &enable_event, portMAX_DELAY);
        break;
    }
    case LINDEV_STATE_PENDING_START_DISABLED:
    {
        lindev->state = LINDEV_STATE_STARTED_ENABLED;
        taskEXIT_CRITICAL(&state_lock);
        start_tasks(lindev);
        break;
    }
    case LINDEV_STATE_NOT_STARTED_ENABLED:
    case LINDEV_STATE_STARTED_ENABLED:
    default:
        taskEXIT_CRITICAL(&state_lock);
        break;
    }
}

void lindev_sleep(lindev_t *lindev)
{
    lindev_fsm_event_t sleep_event;

    lindev_fsm_event_init(&sleep_event, SM_EVENTS_SLEEP);
    xQueueSend(lindev->sm_event_queue, &sleep_event, portMAX_DELAY);
}

const char *lindev_event_to_text(lindev_event_id_t event_type)
{
    switch (event_type)
    {
    case LINDEV_EVENT_ID_CONTROL_FRAME:
        return "Control frame was successfully received from master";
    case LINDEV_EVENT_ID_INFO_FRAME:
        return "Info frame was successfully sent to master";
    case LINDEV_EVENT_ID_DIAG_REQ_FRAME:
        return "Diagnostic request frame was successfully received from master";
    case LINDEV_EVENT_ID_DIAG_RESP_FRAME:
        return "Diagnostic response frame was successfully sent to master";
    case LINDEV_EVENT_ID_NOT_FOR_US:
        return "Recevied a frame which is not addressed to us";
    case LINDEV_EVENT_ID_ERROR_RX_CHECKSUM:
        return "Received frame checksum is invalid";
    case LINDEV_EVENT_ID_ERROR_RX_HEADER:
        return "Received frame header is invalid";
    case LINDEV_EVENT_ID_ERROR_RX_FIFO_FULL:
        return "While receiving, detected that the internal FIFO is full";
    case LINDEV_EVENT_ID_ERROR_RX_FIFO_OVF:
        return "While receiving, detected that the internal FIFO overflowed";
    case LINDEV_EVENT_ID_ERROR_TX_CHECKSUM:
        return "While transmitting, detected that checksum is invalid";
    case LINDEV_EVENT_ID_ERROR_TX_LENGTH:
        return "While transmitting, detected that all bytes were not sent";
    case LINDEV_EVENT_ID_ERROR_TX_FIFO_FULL:
        return "While transmitting, detected that the internal FIFO is full";
    case LINDEV_EVENT_ID_ERROR_TX_FIFO_OVF:
        return "While transmitting, detected that the internal FIFO overflowed";
    case LINDEV_EVENT_ID_ERROR_TX_BRK:
        return "While transmitting, detected a BREAK";
    case LINDEV_EVENT_ID_ERROR_BRK_FIFO_FULL:
        return "While waiting for BREAK, detected that the internal FIFO is full";
    case LINDEV_EVENT_ID_ERROR_BRK_FIFO_OVF:
        return "While waiting for BREAK, detected that the internal FIFO overflowed";
    case LINDEV_EVENT_ID_ERROR_UNKNOWN_IRQ:
        return "Unknown UART IRQ occured";
    case LINDEV_EVENT_ID_ERROR_INTERNAL_OVF:
        return "Internal buffer overflow was detected while processing IRQ";
    case LINDEV_EVENT_ID_ERROR_INVALID_FRAME_TYPE:
        return "Invalid frame type was specified for a frame in the frame table";
    default:
        return "Unknown event";
    }
}

/* Ensure that brk member is at correct location in packed structure */
COMPILE_TIME_ASSERT(LIN_BREAK_FIELD_BYTE == offsetof(lindev_frame_fields_t, brk));
/* Ensure that sync member is at correct location in packed structure */
COMPILE_TIME_ASSERT(LIN_SYNC_FIELD_BYTE == offsetof(lindev_frame_fields_t, sync));
/* Ensure that pid member is at correct location in packed structure */
COMPILE_TIME_ASSERT(LIN_PID_FIELD_BYTE == offsetof(lindev_frame_fields_t, pid));
/* Ensure that checksum member is at correct location in packed structure */
COMPILE_TIME_ASSERT(LIN_CHECKSUM_FIELD_BYTE == offsetof(lindev_frame_fields_t, checksum));
COMPILE_TIME_ASSERT(LIN_FRAME_RAW_LEN == sizeof(lindev_frame_raw_t));
COMPILE_TIME_ASSERT(LIN_FRAME_RAW_LEN == sizeof(lindev_frame_fields_t));
COMPILE_TIME_ASSERT(LIN_FRAME_DATA_LEN == sizeof(lindev_frame_data_t));
