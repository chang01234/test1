/**
 * @file lin_server_uart.c
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server UART implementation
 * @date 2023-12-28
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"

#include "driver/uart.h"  // UART port and configuration definition
#include "esp_attr.h"     // IRAM_ATTR definition
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"   // Queue implementation
#include "freertos/semphr.h"  // Mutex implementation
#include "fsm.h"              // FSM definition
#include "hal/uart_ll.h"      // The LL layer for UART register operations
#include "hal_cpu.h"
#include "soc/uart_struct.h"  // UART device structure definition

#include "lin_server.h"
#include "lin_server_scheduler.h"
#include "lin_server_uart.h"

#ifdef CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif /* CONFIG_PM_ENABLE */

#define LIN_SERVER_FRAME_LEN(frame_len)         (LIN_FRAME_START_LEN + (frame_len) + LIN_FRAME_CHECKSUM_LEN)
#define LIN_SERVER_CHECKSUM_DATA_LEN(frame_len) ((frame_len) + 1)  // +1 for PID
#define LIN_SERVER_CHECSUM_POSITION(frame_len)  (LIN_FRAME_START_LEN + (frame_len))

// #define LIN_SERVER_UART_TIMING_METRICS
#ifdef LIN_SERVER_UART_TIMING_METRICS
typedef struct lin_server_uart_timing_metrics
{
    bool is_timing_error_detected;

    int64_t timer_tick;
    int64_t sm_idle_start;
    int64_t sm_idle_end;
    int64_t sm_send_start;
    int64_t sm_send_end;
    int64_t sm_receive_start;
    int64_t sm_receive_end;
    int64_t sm_feedback_start;
    int64_t sm_feedback_end;
} lin_server_uart_timing_metrics_t;

#define LIN_SERVER_UART_TIMING_METRICS_GET_TIME(x)                                       ((x) = hal_cpu_get_micros())
#define LIN_SERVER_UART_TIMING_METRICS_SET_TIMING_ERROR(x)                               ((x) = true)
#define LIN_SERVER_UART_TIMING_METRICS_CALCULATE_ACTUAL_TIME(end_time_us, start_time_us) (LIN_SERVER_CALCULATE_ACTUAL_BREAK_TIME(end_time_us, start_time_us))
#define LIN_SERVER_UART_TIMING_METRICS_TRACK_STATE_TIME(lin_server_uart)                 lin_server_uart_track_state_execution_time(lin_server_uart)
#define LIN_SERVER_UART_TIMING_METRICS_TRACK_STATE_TIME_INIT(timer_tick, timer_event_us) ((timer_tick) = (timer_event_us))

#else  // LIN_SERVER_UART_TIMING_METRICS

#define LIN_SERVER_UART_TIMING_METRICS_GET_TIME(x)
#define LIN_SERVER_UART_TIMING_METRICS_SET_TIMING_ERROR(x)
#define LIN_SERVER_UART_TIMING_METRICS_TRACK_STATE_TIME(lin_server_uart)
#define LIN_SERVER_UART_TIMING_METRICS_TRACK_STATE_TIME_INIT(timer_tick, timer_event_us)
#endif  // LIN_SERVER_UART_TIMING_METRICS

#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr)-offsetof(type, member)))

/* LIN helper macros */
#if CONFIG_UART_ISR_IN_IRAM
#warning "Placing UART ISR in IRAM section is forbidden when caching is enabled."
#endif

#if (LIN_SERVER_IS_ISR_IN_IRAM == 1)
#define LIN_SERVER_ISR_ATTR IRAM_ATTR
#else /* (LINDEV_IS_ISR_IN_IRAM == 1) */
#define LIN_SERVER_ISR_ATTR
#endif /* !(LINDEV_IS_ISR_IN_IRAM == 1) */

#define LIN_SERVER_EVENT_LOOP_TASK_STACK_SIZE (3 * 1024u)

/* UART helper macros */
#define BAUD_RATE                      (19200)
#define BREAK_SIGNAL_LENGHT_IN_BITS    (13.0)
#define BREAK_SIGNAL_DELIMITER_IN_BITS (2.0) /* LIN 2.2A: 2.3.1.1 Break field: The break delimiter shall be at    \
                                              * least one nominal bit time long.                                  \
                                              * Note 1: An UART can only handle complete bits, so it can occur    \
                                              * on the physical layer that the break delimiter is shorter than    \
                                              * one bit time. It is recommend using delimiter that is longer than \
                                              * one nominal bit-time.                                             \
                                              */

#define LIN_SERVER_CALCULATE_BIT_TRANSMITION_TIME_IN_US(baud_rate, number_of_bits) \
    (uint32_t)((((number_of_bits) / (float)(baud_rate)) * 1000.0) * 1000.0)

#define LIN_SERVER_BREAK_SIGNAL_LENGHT_IN_US    LIN_SERVER_CALCULATE_BIT_TRANSMITION_TIME_IN_US(BAUD_RATE, BREAK_SIGNAL_LENGHT_IN_BITS)     // break length(19200bps ~ 678us)
#define LIN_SERVER_BREAK_SIGNAL_DELIMITER_IN_US LIN_SERVER_CALCULATE_BIT_TRANSMITION_TIME_IN_US(BAUD_RATE, BREAK_SIGNAL_DELIMITER_IN_BITS)  // break delimiter(19200bps ~ 52us)

#define LIN_SERVER_WAKEUP_SIGNAL_PULSE_LENGTH_US (250)
#define LIN_SERVER_WAKEUP_SIGNAL_PENDING_TIME_MS (100)

#define LIN_SERVER_CALCULATE_ACTUAL_BREAK_TIME(end_time_us, start_time_us) \
    (int32_t)(((end_time_us) >= (start_time_us)) ? ((end_time_us) - (start_time_us)) : ((INT64_MAX - (start_time_us)) + (end_time_us) + 1))

/**
 * @brief       Receive frame status
 */
typedef enum receive_frame_status
{
    RECEIVE_FRAME_STATUS_NOT_FOR_US,                             //!< Not supported frame type
    RECEIVE_FRAME_STATUS_GOT_RESPONSE,                           //!< We got response from slave (INFO frame)
    RECEIVE_FRAME_STATUS_WAIT,                                   //!< We got a partial response, wait for more bytes
    RECEIVE_FRAME_STATUS_ERROR_CHECKSUM,                         //!< A frame error was detected by checksum
    RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_RSID,                 //!< Invalid RSID received
    RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_NAD,          //!< Invalid NAD received
    RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_PCI,          //!< Invalid PCI received
    RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_SUPPLIER_ID,  //!< Invalid Supplier ID received
    RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_IDENTIFIER,   //!< Invalid ID received
    RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_CLASS_REGISTRATION,   //!< Broker class registration failure
} receive_frame_status_t;

/**
 * @brief       Receive frame status
 */
typedef enum feedback_frame_status
{
    FEEDBACK_FRAME_STATUS_WAIT,                       //!< We got a partial response, wait for more bytes
    FEEDBACK_FRAME_STATUS_GOT_CTRL_FRAME,             //!< Feedback line(TX->RX) for ctrl frame. No response from slave expected.
    FEEDBACK_FRAME_STATUS_GOT_INFO_FRAME,             //!< Feedback line(TX->RX) for info frame. Response from slave expected.
    FEEDBACK_FRAME_STATUS_GOT_BUS_PROBING_REQ_FRAME,  //!< Feedback line(TX->RX) for diag request frame. No response from slave expected.
    FEEDBACK_FRAME_STATUS_GOT_BUS_PROBING_RES_FRAME,  //!< Feedback line(TX->RX) for diag response frame. Response from slave expected.
    FEEDBACK_FRAME_STATUS_ERROR,                      //!< A frame error was detected by checksum
} feedback_frame_status_t;

typedef enum send_frame_status
{
    SEND_FRAME_STATUS_SENT,                  //!< Frame sent
    SEND_FRAME_STATUS_ERROR_STUFF_FUNCTION,  //!< Stuff function not successful
    SEND_FRAME_STATUS_ERROR_INVALID_FRAME,   //!< Not supported frame type (lin_server_frame_type_t)
} send_frame_status_t;

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
    SM_EVENTS_SCHEDULER_TIMER,           //!< Scheduler timer request: Move to next frame defined in the scheduler table
    SM_EVENTS_GO_TO_SLEEP,               //!< Request: Go to sleep cluster
    SM_EVENTS_WAKE_UP,                   //!< Request: Wake-up sleeping master
    SM_EVENTS_RESPONSE_TOUT,             //!< Timeout waiting for slave
    SM_EVENTS_IRQ_UNKNOWN,               //!< Unknown UART interrupt request source event
} sm_events_t;

typedef struct lin_server_fsm_event_data
{
    uint8_t buffer_length;
    uint8_t buffer[LIN_FRAME_RAW_LEN];
} lin_server_fsm_event_data_t;

// ISR and Timer events
typedef struct lin_server_fsm_event
{
#ifdef LIN_SERVER_UART_TIMING_METRICS
    int64_t timer_event_us;
#endif  // LIN_SERVER_UART_TIMING_METRICS
    fsm_event_t fsm_event;
    lin_server_fsm_event_data_t data;
} lin_server_fsm_event_t;

typedef struct lin_server_uart_bus_probing
{
    bool are_all_devices_on_bus_detected;
    lin_server_device_type_t current_device_type;
} lin_server_uart_bus_probing_t;

typedef struct lin_server_uart_state
{
    bool go_to_sleep_request;
    lin_server_state_request_t requested_by;
} lin_server_uart_state_t;

typedef struct lin_server_uart_bus_managemenet
{
    uint8_t diagnostic_frame_nad;
    lin_frame_type_t current_frame_type;
    lin_diagnostic_frame_t diag_frame_request;

    lin_server_uart_state_t bus_state;
    lin_server_uart_bus_probing_t bus_probing;
} lin_server_uart_bus_management_t;

typedef struct lin_server_uart_process_frames
{
    /* frame bundle */
    const lin_server_device_frames_bundle_def_t *frames_bundle_def;
    /* scheduled frame */
    lin_server_scheduler_table_item_def_t *scheduled_frame_def;
} lin_server_uart_process_frames_t;

typedef struct lin_server_uart
{
    uart_dev_t *uart_dev;
    uart_port_t uart_port;
    fsm_t sm;                                                              //!< State machine handling the UART reception/transmission and function calls
    lin_server_fsm_event_t isr_event;                                      //!< Local copy of isr_event (avoiding using stack allocation in ISR)
    lin_server_fsm_event_data_t rx_buffer;                                 //!< RX buffer of UART
    lin_server_fsm_event_data_t tx_buffer;                                 //!< TX buffer of UART
    QueueHandle_t sm_event_queue;                                          //!< Queue handle of events which are sent to loop
    StaticQueue_t sm_event_queue_instance;                                 //!< Queue structure instance
    lin_server_fsm_event_t sm_event_queue_entries[LIN_EVENT_QUEUE_DEPTH];  //!< Queue data storage
    lin_server_uart_bus_management_t bus_management;
    lin_server_uart_process_frames_t process_frame;
#ifdef LIN_SERVER_UART_TIMING_METRICS
    lin_server_uart_timing_metrics_t timing;
#endif  // LIN_SERVER_UART_TIMING_METRICS
} lin_server_uart_t;

/* State machine handler private functions */
static void sm_handle_uart_reset_fifo(lin_server_uart_t *lin_server_uart);
static void sm_handle_enable_irq_all(lin_server_uart_t *lin_server_uart);
static void sm_handle_disable_irq_rx(lin_server_uart_t *lin_server_uart);

/* State machine states */
static void sm_state_init(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_idle(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_sleeping(fsm_t *fsm, const fsm_event_t *event);
/* CTRL/INFO frame handing states */
static void sm_state_send_next_frame(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_validate_feedback(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_receive_response(fsm_t *fsm, const fsm_event_t *event);
/* Bus probing handing states */
static void sm_state_diag_initiate_device_detection(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_diag_send_request(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_diag_send_response(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_diag_validate_feedback(fsm_t *fsm, const fsm_event_t *event);
static void sm_state_diag_receive_response(fsm_t *fsm, const fsm_event_t *event);

/* LIN server helper functions */
static void lin_server_uart_keep_tx_line_level(int32_t time_us);
static void lin_server_uart_generate_tx_signal(uart_port_t uart_port, int32_t level_low, int32_t level_high);
static void lin_server_uart_generate_break_signal(uart_port_t uart_port);
static void lin_server_uart_send_wakeup_signal(uart_port_t uart_port);
static bool lin_server_uart_prepare_frames(lin_server_uart_t *lin_server_uart);
/* LIN server go-to-sleep/wakeup helper functions */
static bool lin_server_uart_sleep_is_requested(lin_server_uart_t *lin_server_uart);
static void lin_server_uart_sleep_register_request(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event);
static void lin_server_uart_sleep_reset_request(lin_server_uart_t *lin_server_uart);
static void lin_server_uart_wakeup_register_request(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event);
static void lin_server_uart_wakeup_reset_request(lin_server_uart_t *lin_server_uart);
static lin_server_state_request_t lin_server_uart_state_change_requested_by(lin_server_uart_t *lin_server_uart);
/* Callback function invoked by scheduler's timer */
static void lin_server_uart_trigger_next_frame_by_scheduler(void);

/* LIN server state machine helper functions */
/* CTRL/INFO frame helper functions */
static int lin_server_uart_send_next_frame(lin_server_uart_t *lin_server);
static int lin_server_uart_validate_feedback(lin_server_uart_t *lin_server, lin_server_fsm_event_t *server_fsm_event);
static int lin_server_uart_receive_response(lin_server_uart_t *lin_server, lin_server_fsm_event_t *server_fsm_event);
static void lin_server_uart_responose_not_received(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event);
/* Bus probing handing helper functions */
static int lin_server_uart_diag_receive_response(lin_server_uart_t *lin_server, lin_server_fsm_event_t *server_fsm_event);
static int lin_server_uart_diag_validate_feedback(lin_server_uart_t *lin_server, lin_server_fsm_event_t *server_fsm_event);
static int lin_server_uart_diag_send_next_frame(lin_server_uart_t *lin_server);
static bool lin_server_uart_bus_probing_next_device_on_bus_detection(lin_server_uart_bus_probing_t *active_device);

static LIN_SERVER_ISR_ATTR void lin_server_fsm_event_init(lin_server_fsm_event_t *event, uint32_t event_id);

/* Private members */
static lin_server_uart_t lin_server_uart;
/**
 * @brief   Default UART configuration
 * Configure UART. Note that REF_TICK is used so that the baud rate remains
 * correct while APB frequency is changing in light sleep mode.
 */
static const uart_config_t default_uart_config =
    {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
        .source_clk = UART_SCLK_REF_TICK,
#else
        .source_clk = UART_SCLK_XTAL,
#endif
};

/******* Implementation *******/
#ifdef CONFIG_PM_ENABLE
static esp_pm_lock_handle_t lin_pm_lock = NULL;
static bool lock_status = false;
TimerHandle_t timer_response_wait;

static void lin_wait_slave_response(TimerHandle_t timer)
{
    BaseType_t is_a_task_woken = pdFALSE;
    lin_server_fsm_event_init(&lin_server_uart.isr_event, SM_EVENTS_RESPONSE_TOUT);
    xQueueSendFromISR(lin_server_uart.sm_event_queue, &lin_server_uart.isr_event, &is_a_task_woken);

    if (is_a_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

static void lin_pm_lock_acquire(void)
{
    if (lock_status == false)
    {
        LIN_MSTR_SLEEP(!LIN_MSTR_SLP_N);
        esp_pm_lock_acquire(lin_pm_lock);
        lock_status = true;
    }
}

static void lin_pm_lock_release(void)
{
    if (lock_status == true)
    {
        lock_status = false;
        uart_ll_disable_intr_mask(lin_server_uart.uart_dev, UART_INTR_RXFIFO_TOUT);
        uart_ll_clr_intsts_mask(lin_server_uart.uart_dev, UART_RXFIFO_TOUT_INT_CLR_M);
        uart_ll_rxfifo_rst(lin_server_uart.uart_dev);

        LIN_MSTR_SLEEP(LIN_MSTR_SLP_N);
        esp_pm_lock_release(lin_pm_lock);
    }
}

bool lock_is_acquired(void)
{
    return lock_status;
}
#endif /* CONFIG_PM_ENABLE */

#ifdef LIN_SERVER_UART_TIMING_METRICS
static void lin_server_uart_track_state_execution_time(lin_server_uart_t *lin_server_uart)
{
    if (lin_server_uart->timing.is_timing_error_detected)
    {
        int32_t sm_idle_queue_transfer = LIN_SERVER_UART_TIMING_METRICS_CALCULATE_ACTUAL_TIME(lin_server_uart->timing.sm_idle_start, lin_server_uart->timing.timer_tick);
        LOG(E, "difference between timer kick and receiving of the timer event: %dus", sm_idle_queue_transfer);

        LOG(E, "sm_idle time:     %dus",
            LIN_SERVER_UART_TIMING_METRICS_CALCULATE_ACTUAL_TIME(lin_server_uart->timing.sm_idle_end, lin_server_uart->timing.sm_idle_start));
        LOG(E, "sm_send time:     %dus, len=%d",
            LIN_SERVER_UART_TIMING_METRICS_CALCULATE_ACTUAL_TIME(lin_server_uart->timing.sm_send_end, lin_server_uart->timing.sm_send_start),
            lin_server_uart->tx_buffer.buffer_length);
        LOG(E, "sm_feedback time: %dus",
            LIN_SERVER_UART_TIMING_METRICS_CALCULATE_ACTUAL_TIME(lin_server_uart->timing.sm_feedback_end, lin_server_uart->timing.sm_feedback_start));
        LOG(E, "sm_receive time:  %dus, len=%d",
            LIN_SERVER_UART_TIMING_METRICS_CALCULATE_ACTUAL_TIME(lin_server_uart->timing.sm_receive_end, lin_server_uart->timing.sm_receive_start),
            lin_server_uart->rx_buffer.buffer_length - 3);
        LOG(E, "time left %lld", 100 * 1000 - (hal_cpu_get_micros() - lin_server_uart->timing.timer_tick));
    }

    // reset time
    memset(&lin_server_uart->timing, 0, sizeof(lin_server_uart->timing));
}
#endif  // LIN_SERVER_UART_TIMING_METRICS

/**
 * @brief       Keep TX line high or low for predefined time
 *
 * @param       time_us Time in microseconds
 *
 * @note        We have to make multiple attempts to keep the line on desired level
 *              since 'esp_rom_delay_us' can exit earlier then the preset @ref time_us
 */
static void lin_server_uart_keep_tx_line_level(int32_t time_us)
{
    int32_t time_delay_us;
    int64_t start_timer_us;
    int64_t end_timer_us;

    time_delay_us = time_us;
    while (time_delay_us > 0)
    {
        start_timer_us = hal_cpu_get_micros();
        hal_cpu_wait_us(time_delay_us);
        end_timer_us = hal_cpu_get_micros();
        time_delay_us -= LIN_SERVER_CALCULATE_ACTUAL_BREAK_TIME(end_timer_us, start_timer_us);
    }
}

static void lin_server_uart_generate_tx_signal(uart_port_t uart_port, int32_t level_low, int32_t level_high)
{
    uart_set_line_inverse(uart_port, UART_SIGNAL_TXD_INV);
    lin_server_uart_keep_tx_line_level(level_low);
    uart_set_line_inverse(uart_port, UART_SIGNAL_INV_DISABLE);
    lin_server_uart_keep_tx_line_level(level_high);
}

/**
 * @brief       Generate break signal
 *
 * @param       uart_port Uart port
 */
static void lin_server_uart_generate_break_signal(uart_port_t uart_port)
{
    lin_server_uart_generate_tx_signal(uart_port, LIN_SERVER_BREAK_SIGNAL_LENGHT_IN_US, LIN_SERVER_BREAK_SIGNAL_DELIMITER_IN_US);
}

/**
 * @brief       Generate wakeup signal
 *
 * @param       uart_port Uart port
 */
static void lin_server_uart_send_wakeup_signal(uart_port_t uart_port)
{
    lin_server_uart_generate_tx_signal(uart_port, LIN_SERVER_WAKEUP_SIGNAL_PULSE_LENGTH_US, 0);
}

/**
 * @brief       Send LIN frame
 *
 * The LIN Master break field is generated and transmitted separately from the rest of the frame.
 * It must be sent consecutively, with the break delimiter immediately followed by the sync byte field,
 * allowing to create the nominal break signal duration(13bits), nominal brake delimiter duration and with
 * that to ensure having the nominal header duration. To achieve this, the break signal generation and
 * transmission of the subsequent frame bytes must occur without RTOS interference between the two operations.
 *
 * @param       uart_dev   Uart device
 * @param       buffer     frame buffer
 * @param       buffer_sie frame buffer size
 */
static void lin_server_uart_send_frame(lin_server_uart_t *lin_server, uint8_t *buffer, size_t buffer_size)
{
    static portMUX_TYPE lm_spinlock = portMUX_INITIALIZER_UNLOCKED;

#ifdef CONFIG_PM_ENABLE
    lin_pm_lock_acquire();
    hal_cpu_wait_us(10); /* wait for UART and tranceiver to wake */
#endif                   /* CONFIG_PM_ENABLE */
    portENTER_CRITICAL(&lm_spinlock);
    {
#ifdef CONFIG_PM_ENABLE
        uart_ll_rxfifo_rst(lin_server->uart_dev);
        uart_ll_clr_intsts_mask(lin_server->uart_dev, UART_RXFIFO_TOUT_INT_CLR_M);
        uart_ll_ena_intr_mask(lin_server->uart_dev, UART_INTR_RXFIFO_TOUT);
#endif /* CONFIG_PM_ENABLE */
        lin_server_uart_generate_break_signal(lin_server->uart_port);
        uart_ll_write_txfifo(lin_server->uart_dev, buffer, buffer_size);
    }
    portEXIT_CRITICAL(&lm_spinlock);
}

static lin_server_uart_t *isr_sm_to_lindev(fsm_t *sm)
{
    return CONTAINER_OF(sm, lin_server_uart_t, sm);
}

/**
 * @brief       Reset UART peripheral FIFO
 *
 * @param       lin_server_uart Pointer to initialized lin_server_uart instance
 */
static void sm_handle_uart_reset_fifo(lin_server_uart_t *lin_server_uart)
{
    uart_ll_rxfifo_rst(lin_server_uart->uart_dev);
}

/**
 * @brief       Disable only UART RX FIFO IRQ
 *
 * @param       lin_server_uart Pointer to initialized lin_server_uart instance
 */
static void sm_handle_disable_irq_rx(lin_server_uart_t *lin_server_uart)
{
    uart_ll_disable_intr_mask(lin_server_uart->uart_dev, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_OVF);
}

/**
 * @brief       Enable UART Break and RX FIFO IRQ
 *
 * @param       lin_server_uart Pointer to initialized lin_server_uart instance
 */
static void sm_handle_enable_irq_all(lin_server_uart_t *lin_server_uart)
{
    uart_ll_clr_intsts_mask(lin_server_uart->uart_dev, UART_INTR_BRK_DET | UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_OVF);
    uart_ll_ena_intr_mask(lin_server_uart->uart_dev, UART_INTR_BRK_DET | UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_OVF);
}

static bool lin_server_uart_has_info_frame_changed(const uint8_t *current_info_frame, const uint8_t *new_info_frame, size_t size)
{
    return (memcmp(current_info_frame, new_info_frame, size) != 0) ? true : false;
}

static void lin_server_uart_sleep_register_request(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event)
{
    lin_server_uart->bus_management.bus_state.go_to_sleep_request = true;
    lin_server_uart->bus_management.bus_state.requested_by = (lin_server_state_request_t)server_fsm_event->data.buffer[0];
}

static void lin_server_uart_sleep_reset_request(lin_server_uart_t *lin_server_uart)
{
    lin_server_uart->bus_management.bus_state.go_to_sleep_request = false;
    lin_server_uart->bus_management.bus_state.requested_by = LIN_SERVER_STATE_SLEEP_REQUEST_NONE;
}

static void lin_server_uart_wakeup_register_request(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event)
{
    lin_server_uart->bus_management.bus_state.go_to_sleep_request = false;
    lin_server_uart->bus_management.bus_state.requested_by = (lin_server_state_request_t)server_fsm_event->data.buffer[0];
}

static void lin_server_uart_wakeup_reset_request(lin_server_uart_t *lin_server_uart)
{
    lin_server_uart->bus_management.bus_state.go_to_sleep_request = false;
    lin_server_uart->bus_management.bus_state.requested_by = LIN_SERVER_STATE_SLEEP_REQUEST_NONE;
}

static lin_server_state_request_t lin_server_uart_state_change_requested_by(lin_server_uart_t *lin_server_uart)
{
    return lin_server_uart->bus_management.bus_state.requested_by;
}

static bool lin_server_uart_sleep_is_requested(lin_server_uart_t *lin_server_uart)
{
    return lin_server_uart->bus_management.bus_state.go_to_sleep_request;
}

static void lin_server_uart_set_diagnostic_nad(lin_server_uart_t *lin_server_uart, uint32_t nad)
{
    lin_server_uart->bus_management.diagnostic_frame_nad = nad;
}

static uint32_t lin_server_uart_get_diagnostic_nad(const lin_server_uart_t *lin_server_uart)
{
    return lin_server_uart->bus_management.diagnostic_frame_nad;
}

static bool lin_server_uart_is_sleep_command(const lin_server_uart_t *lin_server_uart)
{
    uint8_t nad = lin_server_uart_get_diagnostic_nad(lin_server_uart);
    return LIN_IS_NAD_GO_TO_SLEEP_COMMAND(nad);
}

static bool lin_server_uart_prepare_frames(lin_server_uart_t *lin_server_uart)
{
    lin_server_scheduler_table_item_def_t *scheduler_table_active_frame = lin_server_scheduler_get_current_scheduled_frame();
    const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(scheduler_table_active_frame->item->slave_device_type);

    lin_server_uart->process_frame.frames_bundle_def = lin_server_get_frame_bundle_definition(
        slave_device,
        scheduler_table_active_frame->item->frame_type,
        scheduler_table_active_frame->item->frame_id);

    /* True check if the frame id & type for particular device listed in the scheduling table
     * is actually defined in the lin_server_device_frames_bundle_def_t for that device */
    TRUE_CHECK_RETURNX(false, lin_server_uart->process_frame.frames_bundle_def);

    lin_server_uart->process_frame.scheduled_frame_def = scheduler_table_active_frame;

    return true;
}

/*** START BUS PROBING IMPLEMENTATION ***/
static bool lin_server_uart_bus_probing_next_device_on_bus_detection(lin_server_uart_bus_probing_t *bus_probing)
{
    bool is_end_of_device_list = false;
    static size_t next_device_index = 0;
    static uint8_t active_config_index = 0;
    size_t numb_of_slave_devices = lin_server_get_number_of_slave_devices();

    size_t device_index = next_device_index;
    for (; device_index < numb_of_slave_devices; device_index++)
    {
        const lin_server_slave_device_t *device = lin_server_get_slave_device_by_index(device_index);
        if (device->data->is_on_bus_detected == false)
        {
            next_device_index = device_index;

            if (active_config_index >= device->function_specific_config_data_size)
            {
                /* All configs for current device tried - move to next undetected device */
                next_device_index++;
                active_config_index = 0;
            }
            else
            {
                /* Try next function_specific_config for current device */
                device->data->active_config_data = &device->function_specific_config_data[active_config_index];
                bus_probing->current_device_type = device->device_type;
                active_config_index++;

                break;
            }
        }
    }

    if (device_index >= numb_of_slave_devices)
    {
        /* End of device list. Reset it to first member, so we can start
         * iterrating from the beggining. Also inform SM that the end of the
         * list has been reached, so it can switch to signal frames handling
         */
        next_device_index = 0;
        active_config_index = 0;
        is_end_of_device_list = true;
    }
    else
    {
        is_end_of_device_list = false;
    }

    return is_end_of_device_list;
}

static bool lin_server_uart_bus_probing_is_any_device_detection_requried(lin_server_uart_bus_probing_t *bus_probing)
{
    bool is_any_device_on_bus_detection_required = false;
    size_t numb_of_slave_devices = lin_server_get_number_of_slave_devices();

    /* Check if any device is not detected on bus, so we can start
     * sending bus probing diag frames to establish connection/reconection
     * with the device on bus.
     */
    for (uint32_t device_index = 0; device_index < numb_of_slave_devices; device_index++)
    {
        const lin_server_slave_device_t *device = lin_server_get_slave_device_by_index(device_index);
        if (device->data->is_on_bus_detected == false)
        {
            is_any_device_on_bus_detection_required = true;
            break;
        }
    }

    if (is_any_device_on_bus_detection_required == false)
    {
        /* Set devices detection status. All devices are detected on the bus.
         * The SM will swith to the signal scheduling table and continue sending
         * new frames for all of the devices
         */
        bus_probing->are_all_devices_on_bus_detected = true;
    }
    else
    {
        /* Set devices detection status. If at least one device is not detected on bus,
         * then all devices on bus detection flag should be updated correspondingly.
         */
        bus_probing->are_all_devices_on_bus_detected = false;

        bool is_end_of_device_list_reached = lin_server_uart_bus_probing_next_device_on_bus_detection(bus_probing);
        if (is_end_of_device_list_reached == true)
        {
            /* If end of the list has been reached, allow the SM to switch
             * to the signal scheduling table and continue sending new frames
             * for the detected devices
             */
            is_any_device_on_bus_detection_required = false;
        }
    }

    return is_any_device_on_bus_detection_required;
}

static int lin_server_uart_diag_receive_response(lin_server_uart_t *lin_server, lin_server_fsm_event_t *server_fsm_event)
{
    lin_server_device_type_t device_type = lin_server->bus_management.bus_probing.current_device_type;
    lin_frame_type_t active_frame_type = lin_server->bus_management.current_frame_type;
    const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(device_type);

    /* Copy event bytes to working buffer */
    memcpy(&lin_server->rx_buffer.buffer[lin_server->rx_buffer.buffer_length],
           server_fsm_event->data.buffer,
           server_fsm_event->data.buffer_length);
    lin_server->rx_buffer.buffer_length += server_fsm_event->data.buffer_length;

    if (active_frame_type == LIN_DIAG_RES_FRAME)
    {
        if (lin_server->rx_buffer.buffer_length >= LIN_FRAME_RAW_LEN)
        {
            // Prepare Parity to verify the received frame checksum
            uint_fast8_t frame_id = LIN_FRAME_ID_DIAG_RESPONSE;
            uint_fast8_t parity = lin_calculate_parity(frame_id) << LIN_PID_FIELD_PARITY_POS;
            uint_fast8_t frame_pid = parity | frame_id;

            lin_server->rx_buffer.buffer[LIN_PID_FIELD_BYTE] = frame_pid;
            uint8_t checksum = lin_calculate_checksum(
                frame_id,
                &lin_server->rx_buffer.buffer[LIN_PID_FIELD_BYTE],
                LIN_FRAME_DATA_LEN + 1);

            if (checksum == lin_server->rx_buffer.buffer[LIN_CHECKSUM_FIELD_BYTE])
            {
                lin_diagnostic_frame_t diag_identifier_frame_response;
                lin_diagnostic_frame_t *diag_identifier_frame_resquest = &lin_server->bus_management.diag_frame_request;

                memcpy(&diag_identifier_frame_response,
                       &lin_server->rx_buffer.buffer[LIN_FRAME_START_LEN],
                       MIN(LIN_FRAME_DATA_LEN, sizeof(diag_identifier_frame_response)));

                LOG(D, "[%s]: [0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X] - [0x%X]",
                    slave_device->name,
                    diag_identifier_frame_response.pdu_header.nad,
                    diag_identifier_frame_response.pdu_header.pci,
                    diag_identifier_frame_response.pdu_data.pdu_sf.rsid,
                    diag_identifier_frame_response.pdu_data.pdu_sf.d1,
                    diag_identifier_frame_response.pdu_data.pdu_sf.d2,
                    diag_identifier_frame_response.pdu_data.pdu_sf.d3,
                    diag_identifier_frame_response.pdu_data.pdu_sf.d4,
                    diag_identifier_frame_response.pdu_data.pdu_sf.d5,
                    checksum);

                uint8_t sid = diag_identifier_frame_resquest->pdu_data.pdu_sf.sid;
                uint8_t rsid = diag_identifier_frame_response.pdu_data.pdu_sf.rsid;
                if (LIN_CALCULATE_RSID(sid) != rsid)
                {
                    LOG(E, "[%s]: Invalid RSID. SID+0x40[0x%02X] != RSID[0x%02X]", slave_device->name, sid, rsid);
                    return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_RSID;
                }

                if (sid == LIN_SID_READ_BY_IDENTIFIER)
                {
                    /* Verify NAD between requested and recevied frame */
                    if (diag_identifier_frame_resquest->pdu_header.nad != diag_identifier_frame_response.pdu_header.nad)
                    {
                        LOG(E, "[%s]: Invalid NAD[0x%02X] received, expected NAD[0x%02X]",
                            slave_device->name,
                            diag_identifier_frame_resquest->pdu_header.nad,
                            diag_identifier_frame_response.pdu_header.nad);
                        return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_NAD;
                    }

                    uint16_t function_id = 0;
                    uint16_t variant_id = 0;
                    /* Take identifier request and validate response */
                    uint8_t identifier = diag_identifier_frame_resquest->pdu_data.pdu_sf.d1;
                    if (identifier == LIN_SID_READ_BY_IDENTIFIER_PRODUCT_IDENTIFICATION)
                    {
                        uint8_t pci = diag_identifier_frame_response.pdu_header.pci;
                        if (pci != LIN_PCI_6)
                        {
                            LOG(E, "[%s]: Invalid PCI[0x%02X] received, expected PCI[0x%02X]", slave_device->name, pci, LIN_PCI_6);
                            return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_PCI;
                        }

                        uint16_t supplier_id_response = diag_identifier_frame_response.pdu_data.pdu_sf.d2 << 8 | diag_identifier_frame_response.pdu_data.pdu_sf.d1;
                        uint16_t supplier_id_request = diag_identifier_frame_resquest->pdu_data.pdu_sf.d3 << 8 | diag_identifier_frame_resquest->pdu_data.pdu_sf.d2;
                        if (supplier_id_request != supplier_id_response)
                        {
                            LOG(E, "[%s]: Invalid supplier id[0x%04X] received, expected supplier id[0x%04X]", slave_device->name, supplier_id_response, supplier_id_request);
                            return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_SUPPLIER_ID;
                        }

                        bool is_function_id_detected = false;
                        bool is_variant_id_detected = false;
                        uint16_t function_id_response = diag_identifier_frame_response.pdu_data.pdu_sf.d4 << 8 | diag_identifier_frame_response.pdu_data.pdu_sf.d3;
                        uint8_t variant_id_response = diag_identifier_frame_response.pdu_data.pdu_sf.d5;

                        uint16_t function_id_request = slave_device->data->active_config_data->function_id;
                        if (function_id_request == function_id_response)
                        {
                            function_id = function_id_request;
                            is_function_id_detected = true;

                            for (size_t j = 0; j < slave_device->data->active_config_data->variant_ids_size; ++j)
                            {
                                uint8_t variant_id_request = slave_device->data->active_config_data->variant_ids[j];
                                if (variant_id_request == variant_id_response)
                                {
                                    variant_id = variant_id_request;
                                    is_variant_id_detected = true;

                                    // store active configuration and variant id so we can easily access in the conversion functions
                                    slave_device->data->active_variant_id = variant_id;

                                    break;
                                }
                            }
                        }

                        if (is_function_id_detected == false)
                        {
                            LOG(E, "[%s]: Invalid function id[0x%04X] received!", slave_device->name, function_id_response);
                            return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_PCI;
                        }

                        if (is_variant_id_detected == false)
                        {
                            LOG(E, "[%s]: Invalid varaint id[0x%02X] received!", slave_device->name, variant_id_response);
                            return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_PCI;
                        }
                    }
                    else
                    {
                        LOG(W, "[%s]: Identifier[0x%02X] not yet handled", slave_device->name, identifier);
                        return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_IDENTIFIER;
                    }

                    /* First time registration, else just enable INFO/CTRL frames for the device */
                    if (slave_device->data->is_initialized == false)
                    {
                        /* Initialize the slave device and inform broker about it's existance */
                        int status = lin_server_register_device(slave_device);
                        if (status == 0)
                        {
                            return RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_CLASS_REGISTRATION;
                        }
                    }

                    /* Take device info frames from scheduling signal table for the corresponding device */
                    for (size_t i = 0; i < slave_device->frames_bundle_defs_size; ++i)
                    {
                        if (slave_device->frames_bundle_defs[i].info_frame_def)
                        {
                            lin_server_scheduler_table_item_def_t *device_info_frame;
                            uint8_t frame_id = slave_device->frames_bundle_defs[i].info_frame_def->frame_id;
                            if ((device_info_frame = lin_server_scheduler_get_frame_by_device_type_and_id(device_type, LIN_INFO_FRAME, frame_id)) != NULL)
                            {
                                /* Schedule INFO frame for corresponding device */
                                lin_server_scheduler_schedule_frame(device_info_frame, SCHEDULE_FRAME_TYPE_INFO_REQUEST);
                            }
                            else
                            {
                                LOG(E, "[%s]: No INFO frame listed in the signal scheduling table for the device, with frame id = %d",
                                    slave_device->name,
                                    frame_id);
                            }
                        }
                    }

                    /* Device is detected on the bus. Make the slave device aware of it. */
                    slave_device->data->is_on_bus_detected = true;

                    LOG(I, "[%s]: device detected on bus! Function ID: 0x%04X; Variant: 0x%02X", slave_device->name, function_id, variant_id);

                    return RECEIVE_FRAME_STATUS_GOT_RESPONSE;
                }
                else
                {
                    LOG(E, "[%s]: SID[%d] request not yet supported!", slave_device->name, sid);
                }
            }
            else
            {
                LOG(W, "[%s]: [0x%02X]-[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]-[0x%02X]",
                    slave_device->name,
                    lin_server->rx_buffer.buffer[2],
                    lin_server->rx_buffer.buffer[3], lin_server->rx_buffer.buffer[4], lin_server->rx_buffer.buffer[5],
                    lin_server->rx_buffer.buffer[6], lin_server->rx_buffer.buffer[7], lin_server->rx_buffer.buffer[8],
                    lin_server->rx_buffer.buffer[9], lin_server->rx_buffer.buffer[10], lin_server->rx_buffer.buffer[11]);
                LOG(E, "[%s]: Invalid checksum[0x%02X]:[0x%02X]. Discard frame.",
                    slave_device->name,
                    checksum,
                    lin_server->rx_buffer.buffer[LIN_CHECKSUM_FIELD_BYTE]);
                return RECEIVE_FRAME_STATUS_ERROR_CHECKSUM;
            }
        }
        else
        {
            // Still waiting for bytes
            return RECEIVE_FRAME_STATUS_WAIT;
        }
    }
    else
    {
        LOG(E, "[%s]: Only diagnostic response frame should be handled! Frame handled[0x%02X]", slave_device->name, active_frame_type);
    }

    return RECEIVE_FRAME_STATUS_NOT_FOR_US;
}

static int lin_server_uart_diag_validate_feedback(lin_server_uart_t *lin_server, lin_server_fsm_event_t *server_fsm_event)
{
    lin_frame_type_t active_frame_type = lin_server->bus_management.current_frame_type;

    /* Copy event bytes to working buffer */
    memcpy(&lin_server->rx_buffer.buffer[lin_server->rx_buffer.buffer_length],
           server_fsm_event->data.buffer,
           server_fsm_event->data.buffer_length);
    lin_server->rx_buffer.buffer_length += server_fsm_event->data.buffer_length;

    if (active_frame_type == LIN_DIAG_REQ_FRAME)
    {
        if (lin_server->rx_buffer.buffer_length >= LIN_FRAME_RAW_LEN)
        {
            size_t lenght = MIN(lin_server->rx_buffer.buffer_length, lin_server->tx_buffer.buffer_length);
            int status = memcmp(
                lin_server->rx_buffer.buffer,
                lin_server->tx_buffer.buffer,
                lenght);

            if (status != 0)
            {
                LOG(E, "[0x%02X][0x%02X][0x%02X]-[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]-[0x%02X]",
                    lin_server->rx_buffer.buffer[0], lin_server->rx_buffer.buffer[1], lin_server->rx_buffer.buffer[2],
                    lin_server->rx_buffer.buffer[3], lin_server->rx_buffer.buffer[4], lin_server->rx_buffer.buffer[5],
                    lin_server->rx_buffer.buffer[6], lin_server->rx_buffer.buffer[7], lin_server->rx_buffer.buffer[8],
                    lin_server->rx_buffer.buffer[9], lin_server->rx_buffer.buffer[10], lin_server->rx_buffer.buffer[11]);
                LOG(E, "[0x%02X][0x%02X][0x%02X]-[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]-[0x%02X]",
                    lin_server->tx_buffer.buffer[0], lin_server->tx_buffer.buffer[1], lin_server->tx_buffer.buffer[2],
                    lin_server->tx_buffer.buffer[3], lin_server->tx_buffer.buffer[4], lin_server->tx_buffer.buffer[5],
                    lin_server->tx_buffer.buffer[6], lin_server->tx_buffer.buffer[7], lin_server->tx_buffer.buffer[8],
                    lin_server->tx_buffer.buffer[9], lin_server->tx_buffer.buffer[10], lin_server->tx_buffer.buffer[11]);
            }

            return status == 0 ? FEEDBACK_FRAME_STATUS_GOT_BUS_PROBING_REQ_FRAME : FEEDBACK_FRAME_STATUS_ERROR;
        }
    }
    else if (active_frame_type == LIN_DIAG_RES_FRAME)
    {
        if (lin_server->rx_buffer.buffer_length >= LIN_FRAME_START_LEN)
        {
            size_t lenght = MIN(lin_server->rx_buffer.buffer_length, lin_server->tx_buffer.buffer_length);
            int status = memcmp(
                lin_server->rx_buffer.buffer,
                lin_server->tx_buffer.buffer,
                lenght);

            if (status != 0)
            {
                LOG(E, "[0x%02X][0x%02X][0x%02X] == [0x%02X][0x%02X][0x%02X]",
                    lin_server->rx_buffer.buffer[0],
                    lin_server->rx_buffer.buffer[1],
                    lin_server->rx_buffer.buffer[2],
                    lin_server->tx_buffer.buffer[0],
                    lin_server->tx_buffer.buffer[1],
                    lin_server->tx_buffer.buffer[2]);
            }

            return status == 0 ? FEEDBACK_FRAME_STATUS_GOT_BUS_PROBING_RES_FRAME : FEEDBACK_FRAME_STATUS_ERROR;
        }
    }
    else
    {
        return FEEDBACK_FRAME_STATUS_ERROR;
    }

    return FEEDBACK_FRAME_STATUS_WAIT;
}

static int lin_server_uart_diag_send_next_frame(lin_server_uart_t *lin_server)
{
    lin_frame_type_t active_frame_type = lin_server->bus_management.current_frame_type;

    /* Prepare header(0x00, 0x55, PID) */
    uint_fast8_t frame_id = (active_frame_type == LIN_DIAG_REQ_FRAME ? LIN_FRAME_ID_DIAG_REQUEST : LIN_FRAME_ID_DIAG_RESPONSE);
    uint_fast8_t parity = lin_calculate_parity(frame_id) << LIN_PID_FIELD_PARITY_POS;
    uint_fast8_t frame_pid = parity | frame_id;
    /* Break signal does not need to be send since we generate the break signal seperately. */
    lin_server->tx_buffer.buffer[LIN_SYNC_FIELD_BYTE] = LIN_SYNC_FIELD_DATA;
    lin_server->tx_buffer.buffer[LIN_PID_FIELD_BYTE] = frame_pid;

    if (active_frame_type == LIN_DIAG_REQ_FRAME)
    {
        if (lin_server_uart_is_sleep_command(lin_server) == true)
        {
            LOG(D, "[LIN Master]: Send frame: [GO_TO_SLEEP][DIAG REQUEST]");

            lin_server->bus_management.diag_frame_request.pdu_header.nad = LIN_NAD_GO_TO_SLEEP_COMMAND;
            lin_server->bus_management.diag_frame_request.pdu_header.pci = 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.sid = 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d1 = 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d2 = 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d3 = 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d4 = 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d5 = 0xFF;
        }
        else
        {
            lin_server_device_type_t device_type = lin_server->bus_management.bus_probing.current_device_type;
            const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(device_type);

            LOG(D, "[%s]: Send frame: frame_id[0x%02X], NAD[0x%02X], supplier_id[0x%04X], function_id[0x%04X], [DIAG REQUEST]",
                slave_device->name,
                frame_id,
                slave_device->device_config->nad,
                slave_device->device_config->supplier_id,
                slave_device->data->active_config_data->function_id);

            lin_server->bus_management.diag_frame_request.pdu_header.nad = slave_device->device_config->nad;
            lin_server->bus_management.diag_frame_request.pdu_header.pci = LIN_PCI_6;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.sid = LIN_SID_READ_BY_IDENTIFIER;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d1 = LIN_SID_READ_BY_IDENTIFIER_PRODUCT_IDENTIFICATION;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d2 = slave_device->device_config->supplier_id & 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d3 = (slave_device->device_config->supplier_id >> 8) & 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d4 = slave_device->data->active_config_data->function_id & 0xFF;
            lin_server->bus_management.diag_frame_request.pdu_data.pdu_sf.d5 = (slave_device->data->active_config_data->function_id >> 8) & 0xFF;
        }

        /* Copy diagnostic frame payload */
        memcpy(&lin_server->tx_buffer.buffer[LIN_FRAME_START_LEN],
               &lin_server->bus_management.diag_frame_request,
               MIN(LIN_FRAME_DATA_LEN, sizeof(lin_server->bus_management.diag_frame_request)));

        /* Prepare checksum */
        uint8_t checksum = lin_calculate_checksum(
            frame_pid,
            &lin_server->tx_buffer.buffer[LIN_PID_FIELD_BYTE],
            LIN_FRAME_DATA_LEN + LIN_FRAME_CHECKSUM_LEN);
        lin_server->tx_buffer.buffer[LIN_CHECKSUM_FIELD_BYTE] = checksum;

        // -1 for the Break signal as we do not send it as part of this array.
        lin_server->tx_buffer.buffer_length = LIN_FRAME_RAW_LEN - 1;

        lin_server_uart_send_frame(
            lin_server,
            &lin_server->tx_buffer.buffer[LIN_SYNC_FIELD_BYTE],
            lin_server->tx_buffer.buffer_length);

        return SEND_FRAME_STATUS_SENT;
    }
    else if (active_frame_type == LIN_DIAG_RES_FRAME)
    {
        lin_server_device_type_t device_type = lin_server->bus_management.bus_probing.current_device_type;
        const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(device_type);

        LOG(D, "[%s]: Send frame: [0x%02X][%s]", slave_device->name, frame_id, "DIAG RESPONSE");

        // -1 for the Break signal as we do not send it as part of this array.
        lin_server->tx_buffer.buffer_length = LIN_FRAME_START_LEN - 1;

        lin_server_uart_send_frame(
            lin_server,
            &lin_server->tx_buffer.buffer[LIN_SYNC_FIELD_BYTE],
            lin_server->tx_buffer.buffer_length);

        return SEND_FRAME_STATUS_SENT;
    }

    LOG(E, "Invalid bus probing data frame[%d]", active_frame_type);
    return SEND_FRAME_STATUS_ERROR_INVALID_FRAME;
}

static void sm_state_diag_receive_response(fsm_t *fsm, const fsm_event_t *event)
{
    receive_frame_status_t status;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:

        LOG(D, "<- state");
        /* buffer clear + lenght = 0 */
        memset(&lin_server_uart->rx_buffer, 0, sizeof(lin_server_uart->rx_buffer));
        /* skip 0x00, 0x55, PID, as we do not get them as part of the slave response */
        lin_server_uart->rx_buffer.buffer_length = LIN_FRAME_START_LEN;
#ifdef CONFIG_PM_ENABLE
        TRUE_CHECK(xTimerStart(timer_response_wait, portMAX_DELAY));
#endif /* CONFIG_PM_ENABLE */
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
    {
        const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(lin_server_uart->bus_management.bus_probing.current_device_type);
        LOG(D, "[%s]: device did not reply!", slave_device->name);
#ifdef CONFIG_PM_ENABLE
        lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
        fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
        break;
    }
    case SM_EVENTS_RESPONSE_TOUT:
        /* If we are waiting for response, this state will timeout faster so we don't wait for next scheduler cycle,
       this will allow to go to sleep earlier in case of response is not received */
#ifdef CONFIG_PM_ENABLE
        lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
        fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
        break;
    case SM_EVENTS_IRQ_TOUT:
        status = lin_server_uart_diag_receive_response(lin_server_uart, server_fsm_event);
        switch (status)
        {
        case RECEIVE_FRAME_STATUS_WAIT:
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStart(timer_response_wait, portMAX_DELAY));
#endif /* CONFIG_PM_ENABLE */
            /* do nothing, still waiting for full buffer reception */
            break;
        case RECEIVE_FRAME_STATUS_GOT_RESPONSE:
            /* we have received and processed the diagnostic response frame. */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        case RECEIVE_FRAME_STATUS_NOT_FOR_US:
            /* we have received internal request by the state machine to try
             * to get response on an non diagnostic response frame.
             */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStop(timer_response_wait, portMAX_DELAY));
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        case RECEIVE_FRAME_STATUS_ERROR_CHECKSUM:
            /* Invalid checksum of the recieved diagnostic response frame. */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStop(timer_response_wait, portMAX_DELAY));
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        case RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_RSID:
        case RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_NAD:
        case RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_PCI:
        case RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_SUPPLIER_ID:
        case RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_INVALID_IDENTIFIER:
        case RECEIVE_FRAME_STATUS_ERROR_BUS_PROBING_CLASS_REGISTRATION:
            /* Error detected in received diagnostic response frame */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStop(timer_response_wait, portMAX_DELAY));
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        default:
            break;
        }
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
#ifdef CONFIG_PM_ENABLE
        lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
        break;
    default:
        break;
    }
}

static void sm_state_diag_validate_feedback(fsm_t *fsm, const fsm_event_t *event)
{
    feedback_frame_status_t status;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LOG(D, "<- state");

        /* buffer clear + lenght = 0 */
        memset(&lin_server_uart->rx_buffer, 0, sizeof(lin_server_uart->rx_buffer));
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
        LOG(E, "SM state should not receive TIMER event! We ended up in inbetween state!");
        fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
        break;
    case SM_EVENTS_IRQ_TOUT:
        status = lin_server_uart_diag_validate_feedback(lin_server_uart, server_fsm_event);
        switch (status)
        {
        case FEEDBACK_FRAME_STATUS_WAIT:
            /* do nothing, still waiting for full buffer reception */
            break;
        case FEEDBACK_FRAME_STATUS_GOT_BUS_PROBING_REQ_FRAME:
            if (lin_server_uart_is_sleep_command(lin_server_uart) == true)
            {
                /* Valid feedback. Transit to sleep state */
                fsm_state_change(fsm, sm_state_sleeping);
#ifdef CONFIG_PM_ENABLE
                lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            }
            else
            {
                /* Valid feedback. What we sent is what we received. Move to diagnostic response frame send state. */
                fsm_state_change(fsm, sm_state_diag_send_response);
#ifdef CONFIG_PM_ENABLE
                lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            }
            break;
        case FEEDBACK_FRAME_STATUS_GOT_BUS_PROBING_RES_FRAME:
            /* Valid feedback. What we sent is what we received. Move to receive response state. */
            fsm_state_change(fsm, sm_state_diag_receive_response);
            break;
        case FEEDBACK_FRAME_STATUS_ERROR:
            if (lin_server_uart_is_sleep_command(lin_server_uart) == true)
            {
                /* Valid feedback. Transit to sleep state */
                fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
                lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            }
            else
            {
                /* Invalid feedback. What we sent is not what we received. Possible collision? */
                fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
#ifdef CONFIG_PM_ENABLE
                lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            }
            break;
        default:
            break;
        }
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    default:
        break;
    }
}

static void sm_state_diag_send_response(fsm_t *fsm, const fsm_event_t *event)
{
    send_frame_status_t status;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LOG(D, "<- state");

        /* buffer clear + lenght = 0 */
        memset(&lin_server_uart->tx_buffer, 0, sizeof(lin_server_uart->tx_buffer));

        lin_server_uart->bus_management.current_frame_type = LIN_DIAG_RES_FRAME;
        lin_server_scheduler_change_time_period(LIN_SERVER_TICK_TIMER_DIAG_PERIOD_MS);
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
        status = lin_server_uart_diag_send_next_frame(lin_server_uart);
        switch (status)
        {
        case SEND_FRAME_STATUS_SENT:
            /* Validate RX/TX buffers trough feedback line */
            fsm_state_change(fsm, sm_state_diag_validate_feedback);
            break;
        case SEND_FRAME_STATUS_ERROR_STUFF_FUNCTION:
            /* Stuff function uncessfull, we didn't send any data to slave.*/
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
            break;
        case SEND_FRAME_STATUS_ERROR_INVALID_FRAME:
            /* Not supported frame type by send_next_frame(), we didn't send any data to slave */
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
            break;
        }
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    case FSM_EXIT_EVENT:
        lin_server_scheduler_change_time_period(LIN_SERVER_TICK_TIMER_PERIOD_MS);
        break;
    default:
        break;
    }
}

static void sm_state_diag_send_request(fsm_t *fsm, const fsm_event_t *event)
{
    send_frame_status_t status;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LOG(D, "<- state");
        /* buffer clear + lenght = 0 */
        memset(&lin_server_uart->tx_buffer, 0, sizeof(lin_server_uart->tx_buffer));

        lin_server_uart->bus_management.current_frame_type = LIN_DIAG_REQ_FRAME;
        status = lin_server_uart_diag_send_next_frame(lin_server_uart);
        switch (status)
        {
        case SEND_FRAME_STATUS_SENT:
            /* Validate RX/TX buffers trough feedback line */
            fsm_state_change(fsm, sm_state_diag_validate_feedback);
            break;
        case SEND_FRAME_STATUS_ERROR_STUFF_FUNCTION:
            /* Stuff function uncessfull, we didn't send any data to slave.*/
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
            break;
        case SEND_FRAME_STATUS_ERROR_INVALID_FRAME:
            /* Not supported frame type by send_next_frame(), we didn't send any data to slave */
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
            break;
        }
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
        LOG(E, "SM state should not receive TIMER event! We ended up in inbetween state!");
#ifdef CONFIG_PM_ENABLE
        lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
        fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    default:
        break;
    }
}

static void sm_state_diag_initiate_device_detection(fsm_t *fsm, const fsm_event_t *event)
{
    bool is_any_device_on_bus_detection_required = false;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LOG(D, "<- state");
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
        is_any_device_on_bus_detection_required = lin_server_uart_bus_probing_is_any_device_detection_requried(&lin_server_uart->bus_management.bus_probing);
        if (is_any_device_on_bus_detection_required == true)
        {
            /* We have undetected device on bus. Try to detect. */
            const lin_server_slave_device_t *device = lin_server_get_slave_device(lin_server_uart->bus_management.bus_probing.current_device_type);
            lin_server_uart_set_diagnostic_nad(lin_server_uart, device->device_config->nad);
            fsm_state_change(fsm, sm_state_diag_send_request);
        }
        else
        {
            /* All devices are detected or we have reached the end of devices list. */
            fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
        }
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    default:
        break;
    }
}

/*** END BUS PROBING IMPLEMENTATION ***/

/*** START SIGNAL FRAME IMPLEMENTATION  ***/
static bool lin_server_uart_receive_info_response(lin_server_uart_t *lin_server_uart, const lin_server_slave_device_t *slave_device)
{
    bool should_apply_slave_response = false;
    lin_server_uart_process_frames_t *process_frame = &lin_server_uart->process_frame;
    const lin_server_device_frame_def_t *info_frame_def = process_frame->frames_bundle_def->info_frame_def;
    const lin_server_device_frame_def_t *ctrl_frame_def = process_frame->frames_bundle_def->ctrl_frame_def;

    switch (lin_server_get_sync_protocol(slave_device))
    {
    case LIN_SERVER_SYNC_PROTOCOL_LOCAL_CHANGE:
    {
        lin_server_device_frame_t info_reponse;
        memcpy(&info_reponse.frame_signals.raw_frame, &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN], info_frame_def->frame_len);
        bool is_local_change_bit_set = lin_server_sync_protocol_local_change_is_local_change_bit_set(
            info_frame_def,
            &info_reponse);

        if (is_local_change_bit_set == true)
        {
            should_apply_slave_response = true;
            /* Prepare for LocalChange ACK, and continue reading info frames.
             * Do not scheudule the ACK frame yet, as there might be new updates
             * incoming from slave's side (e.g. user is constantly changing the temperature)
             *
             * Also, some slaves can raise LocalChange bit to 1 even before they update the
             * actual signal that they want to update. So, scheduling and sending an ACK frame
             * with old values will override the slave's new updated value as the slave might accept
             * ACK frame with the older state, i.e. APAC AC.
             */
            lin_server_set_ctrl_frame_verification(ctrl_frame_def->frame, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST);
            lin_server_reset_ctrl_frame_verification_counter(ctrl_frame_def->frame, LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_RESET_COUNTER);
        }
        else
        {
            /* Compare stored signal data with the new info frame */
            if (lin_server_uart_has_info_frame_changed(info_frame_def->frame->frame_signals.raw_frame,
                                                       &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN],
                                                       info_frame_def->frame_len) == true)
            {
                should_apply_slave_response = true;
            }
        }

        break;
    }
    case LIN_SERVER_SYNC_PROTOCOL_NONE:
    {
        /* Compare stored signal data with the new info frame */
        if (lin_server_uart_has_info_frame_changed(info_frame_def->frame->frame_signals.raw_frame,
                                                   &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN],
                                                   info_frame_def->frame_len) == true)
        {
            should_apply_slave_response = true;
        }

        break;
    }
    default:
        break;
    }

    return should_apply_slave_response;
}

static bool lin_server_uart_verify_ctrl_request(lin_server_uart_t *lin_server_uart, const lin_server_slave_device_t *slave_device)
{
    bool should_apply_slave_response = false;
    bool is_ctrl_frame_verified = false;
    lin_server_uart_process_frames_t *process_frame = &lin_server_uart->process_frame;
    const lin_server_device_frame_def_t *info_frame_def = process_frame->frames_bundle_def->info_frame_def;
    const lin_server_device_frame_def_t *ctrl_frame_def = process_frame->frames_bundle_def->ctrl_frame_def;

    // Get correspoding CTRL frame so we can mark for ACK or INIT response to slave
    lin_server_scheduler_table_item_def_t *schedule_table_ctrl_schedule_frame =
        lin_server_scheduler_get_frame_by_device_type_and_id(slave_device->device_type, ctrl_frame_def->frame_type, ctrl_frame_def->frame_id);

    switch (lin_server_get_sync_protocol(slave_device))
    {
    case LIN_SERVER_SYNC_PROTOCOL_LOCAL_CHANGE:
    {
        lin_server_device_frame_t info_reponse;
        memcpy(&info_reponse.frame_signals.raw_frame, &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN], info_frame_def->frame_len);
        bool is_local_change_bit_set = lin_server_sync_protocol_local_change_is_local_change_bit_set(
            info_frame_def,
            &info_reponse);

        switch (lin_server_get_ctrl_frame_verification_type(ctrl_frame_def->frame))
        {
        case SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST:
        {
            if (lin_server_uart_has_info_frame_changed(info_frame_def->frame->frame_signals.raw_frame,
                                                       &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN],
                                                       info_frame_def->frame_len) == true)
            {
                LOG(D, "[%s]: Update recevied, reset ACK counter.", slave_device->name);
                /* Apply slave's changes immediately in master, but do not start sending ACK yet */
                should_apply_slave_response = true;
                /* reset counter as we have received new change before we schedule ACK frame */
                lin_server_reset_ctrl_frame_verification_local_change_ack_counter(ctrl_frame_def->frame);
            }
            else
            {
                if (is_local_change_bit_set == true)
                {
                    /* No new updates has been registered, start with ACK process */
                    if (lin_server_has_ctrl_frame_verification_local_change_ack_attempts_left(ctrl_frame_def->frame) == false)
                    {
                        if (lin_server_has_ctrl_frame_verification_attempts_left(ctrl_frame_def->frame) == false)
                        {
                            /* There is possibility that the transition from LocalChange 0 to 1 to
                             * not be detected by the device. Unless we force another 0 -> 1 transition
                             * in the next iterration, the slave device will never accept our ACK frame.
                             */
                            LOG(D, "[%s]: Reset LocalChange ACK process.", slave_device->name);
                            lin_server_scheduler_schedule_frame(schedule_table_ctrl_schedule_frame, SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST);
                            lin_server_reset_ctrl_frame_verification_counter(ctrl_frame_def->frame, LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_RESET_COUNTER);
                        }
                        else
                        {
                            /* ACK frame */
                            LOG(D, "[%s]: LocalChange ACK frame sent.", slave_device->name);
                            lin_server_scheduler_schedule_frame(schedule_table_ctrl_schedule_frame, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST);
                            lin_server_reset_ctrl_frame_verification_local_change_ack_counter(ctrl_frame_def->frame);
                        }
                    }
                    else
                    {
                        /* Do nothing, stil reading info frames
                         * before we start with ACK process.
                         */
                    }
                }
                else
                {
                    LOG(D, "[%s]: LocalChange ACK frame accepted.", slave_device->name);
                    is_ctrl_frame_verified = true;
                    lin_server_reset_ctrl_frame_verification_local_change_ack_counter(ctrl_frame_def->frame);
                }
            }

            break;
        }
        case SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST:
        {
            /* Compare stored signal data with the new info frame. */
            if (lin_server_uart_has_info_frame_changed(info_frame_def->frame->frame_signals.raw_frame,
                                                       &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN],
                                                       info_frame_def->frame_len) == true)
            {
                if (is_local_change_bit_set == true)  // while confirming CTRL frame acceptance from slave side, slave has changed on its own
                {
                    should_apply_slave_response = true;
                    lin_server_set_ctrl_frame_verification(ctrl_frame_def->frame, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST);
                    lin_server_reset_ctrl_frame_verification_counter(ctrl_frame_def->frame, LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_RESET_COUNTER);
                }
                else
                {
                    if (lin_server_has_ctrl_frame_verification_attempts_left(ctrl_frame_def->frame) == false)
                    {
                        LOG(D, "[%s]: Change request has been accepted", slave_device->name);

                        /* has been accepted, proceed */
                        should_apply_slave_response = true;
                        is_ctrl_frame_verified = true;
                    }
                }
            }
            else
            {
                if (lin_server_has_ctrl_frame_verification_attempts_left(ctrl_frame_def->frame) == false)
                {
                    LOG(D, "[%s]: Applying info frame data! Change request was not accepted!", slave_device->name);

                    /* Regardless of whether the change matches our request, we should adhere to the slave's state */
                    should_apply_slave_response = true;
                    is_ctrl_frame_verified = true;
                }
            }

            break;
        }
        default:
            LOG(E, "Unhandled ctrl frame verification type: %d", lin_server_get_ctrl_frame_verification_type(ctrl_frame_def->frame));
            break;
        }
        break;
    }
    case LIN_SERVER_SYNC_PROTOCOL_NONE:
    {
        /* Compare stored signal data with the new info frame. */
        if (lin_server_uart_has_info_frame_changed(info_frame_def->frame->frame_signals.raw_frame,
                                                   &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN],
                                                   info_frame_def->frame_len) == true)
        {
            if (lin_server_has_ctrl_frame_verification_attempts_left(ctrl_frame_def->frame) == false)
            {
                LOG(D, "[%s]: Change request has been accepted", slave_device->name);

                is_ctrl_frame_verified = true;
                should_apply_slave_response = true;
            }
        }
        else
        {
            if (lin_server_has_ctrl_frame_verification_attempts_left(ctrl_frame_def->frame) == false)
            {
                LOG(D, "[%s]: Applying info frame data! Change request was not accepted!", slave_device->name);
                /* Regardless of whether the change matches our request, we should adhere to the slave's state */
                should_apply_slave_response = true;
                is_ctrl_frame_verified = true;
            }
        }

        break;
    }
    default:
        break;
    }

    if (is_ctrl_frame_verified == true)
    {
        /* Switch back to reading info frames */
        lin_server_reset_ctrl_frame_verification(ctrl_frame_def->frame);
    }

    return should_apply_slave_response;
}

static int lin_server_uart_receive_response(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event)
{
    lin_server_uart_process_frames_t *process_frame = &lin_server_uart->process_frame;
    const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(process_frame->scheduled_frame_def->item->slave_device_type);
    const lin_server_device_frame_def_t *ctrl_frame_def = process_frame->frames_bundle_def->ctrl_frame_def;
    const lin_server_device_frame_def_t *info_frame_def = process_frame->frames_bundle_def->info_frame_def;

    /* Copy event bytes to working buffer */
    memcpy(&lin_server_uart->rx_buffer.buffer[lin_server_uart->rx_buffer.buffer_length],
           server_fsm_event->data.buffer,
           server_fsm_event->data.buffer_length);
    lin_server_uart->rx_buffer.buffer_length += server_fsm_event->data.buffer_length;

    if (process_frame->scheduled_frame_def->item->frame_type == LIN_INFO_FRAME)
    {
        // since we start populating the buffer from .buffer[LIN_FRAME_START_LEN]
        if (lin_server_uart->rx_buffer.buffer_length >= LIN_SERVER_FRAME_LEN(info_frame_def->frame_len))
        {
            lin_server_slave_info_response_t slave_info_response;

            // Prepare Parity to verify the received frame checksum
            lin_server_device_type_t device_type = process_frame->scheduled_frame_def->item->slave_device_type;
            uint_fast8_t frame_id = process_frame->scheduled_frame_def->item->frame_id;
            uint_fast8_t parity = lin_calculate_parity(frame_id) << LIN_PID_FIELD_PARITY_POS;
            uint_fast8_t frame_pid = parity | frame_id;

            lin_server_uart->rx_buffer.buffer[LIN_PID_FIELD_BYTE] = frame_pid;
            uint8_t checksum = lin_calculate_checksum(
                frame_id,
                &lin_server_uart->rx_buffer.buffer[LIN_PID_FIELD_BYTE],
                LIN_SERVER_CHECKSUM_DATA_LEN(info_frame_def->frame_len));

            if (checksum == lin_server_uart->rx_buffer.buffer[LIN_SERVER_CHECSUM_POSITION(info_frame_def->frame_len)])
            {
                // Callback to slave device extract function
                memset(&slave_info_response, 0, sizeof(slave_info_response));
                slave_device->extract_function(
                    slave_device,
                    process_frame->frames_bundle_def,
                    &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN],
                    info_frame_def->frame_len,
                    &slave_info_response);

                LOG(D, "[%s]: <- [0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]-[0x%02X]",
                    slave_device->name,
                    lin_server_uart->rx_buffer.buffer[3], lin_server_uart->rx_buffer.buffer[4], lin_server_uart->rx_buffer.buffer[5],
                    lin_server_uart->rx_buffer.buffer[6], lin_server_uart->rx_buffer.buffer[7], lin_server_uart->rx_buffer.buffer[8],
                    lin_server_uart->rx_buffer.buffer[9], lin_server_uart->rx_buffer.buffer[10], lin_server_uart->rx_buffer.buffer[11]);

                if (slave_info_response.ci_error)
                {
                    LOG(E, "[%s]: CIError.", slave_device->name);
                }
                if (slave_info_response.error)
                {
                    LOG(E, "[%s]: Error.", slave_device->name);
                }
                if (slave_info_response.no_main)
                {
                    LOG(E, "[%s]: NoMain.", slave_device->name);
                }
                if (slave_info_response.no_init)
                {
                    LOG(W, "[%s]: NoInit. Set CTRL frame request.", slave_device->name);

                    lin_server_scheduler_table_item_def_t *schedule_table_ctrl_schedule_frame;
                    if ((schedule_table_ctrl_schedule_frame = lin_server_scheduler_get_frame_by_device_type_and_id(device_type, LIN_CONTROL_FRAME, ctrl_frame_def->frame_id)))
                    {
                        // Get correspoding CTRL frame so we can mark for ACK or INIT response to slave
                        lin_server_scheduler_schedule_frame(schedule_table_ctrl_schedule_frame, SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);
                    }
                }

                bool should_apply_slave_response = false;
                if (slave_device->data->is_initialized == false)
                {
                    should_apply_slave_response = true;
                    slave_device->data->is_initialized = true;
                }
                else
                {
                    if (ctrl_frame_def && (lin_server_has_ctrl_frame_been_sent(ctrl_frame_def->frame) == true))
                    {
                        should_apply_slave_response = lin_server_uart_verify_ctrl_request(lin_server_uart, slave_device);
                    }
                    else
                    {
                        should_apply_slave_response = lin_server_uart_receive_info_response(lin_server_uart, slave_device);
                    }
                }

                if (should_apply_slave_response == true)
                {
                    // Copy RX to LIN info frame
                    memcpy(info_frame_def->frame->frame_signals.raw_frame, &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN], info_frame_def->frame_len);

                    // Slave requested changes, update Master's parameters and frames
                    lin_server_handle_lin_info_frame_received(slave_device, process_frame->frames_bundle_def);
                }
                else
                {
                    LOG(D, "No updates in the received info frame. Discard frame.");
                }

                return RECEIVE_FRAME_STATUS_GOT_RESPONSE;
            }
            else
            {
                LOG(W, "[%s]: [0x%02X]-[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]-[0x%02X]",
                    slave_device->name,
                    lin_server_uart->rx_buffer.buffer[2],
                    lin_server_uart->rx_buffer.buffer[3], lin_server_uart->rx_buffer.buffer[4], lin_server_uart->rx_buffer.buffer[5],
                    lin_server_uart->rx_buffer.buffer[6], lin_server_uart->rx_buffer.buffer[7], lin_server_uart->rx_buffer.buffer[8],
                    lin_server_uart->rx_buffer.buffer[9], lin_server_uart->rx_buffer.buffer[10], lin_server_uart->rx_buffer.buffer[11]);
                LOG(E, "[%s]: Invalid checksum[0x%02X]:[0x%02X]. Discard frame.",
                    slave_device->name,
                    checksum,
                    lin_server_uart->rx_buffer.buffer[LIN_SERVER_CHECSUM_POSITION(info_frame_def->frame_len)]);
                return RECEIVE_FRAME_STATUS_ERROR_CHECKSUM;
            }
        }
        // Still waiting for bytes
        return RECEIVE_FRAME_STATUS_WAIT;
    }

    // If LIN_CTRL_FRAME, we do not expect to get any
    // reponse back, just return and move to sm_state_idle
    return RECEIVE_FRAME_STATUS_NOT_FOR_US;
}

static int lin_server_uart_validate_feedback(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event)
{
    lin_server_uart_process_frames_t *process_frame = &lin_server_uart->process_frame;
    const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(process_frame->scheduled_frame_def->item->slave_device_type);
    const lin_server_device_frame_def_t *ctrl_frame_def = process_frame->frames_bundle_def->ctrl_frame_def;

    /* Copy event bytes to working buffer */
    memcpy(&lin_server_uart->rx_buffer.buffer[lin_server_uart->rx_buffer.buffer_length],
           server_fsm_event->data.buffer,
           server_fsm_event->data.buffer_length);
    lin_server_uart->rx_buffer.buffer_length += server_fsm_event->data.buffer_length;

    if (process_frame->scheduled_frame_def->item->frame_type == LIN_CONTROL_FRAME)
    {
        if (lin_server_uart->rx_buffer.buffer_length >= LIN_SERVER_FRAME_LEN(ctrl_frame_def->frame_len))
        {
            size_t lenght = MIN(lin_server_uart->rx_buffer.buffer_length, lin_server_uart->tx_buffer.buffer_length);
            int status = memcmp(
                lin_server_uart->rx_buffer.buffer,
                lin_server_uart->tx_buffer.buffer,
                lenght);

            // proceed only if the frame was successfully sent
            if (status == 0)
            {
                lin_server_lock_device(slave_device);
                {
                    switch (lin_server_get_sync_protocol(slave_device))
                    {
                    case LIN_SERVER_SYNC_PROTOCOL_LOCAL_CHANGE:
                    {
                        if (lin_server_scheduler_is_frame_scheduled_type_set(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST))
                        {
                            // Clear frame 'SyncFrame' bit as we have successfully sent the ACK frame
                            lin_server_sync_protocol_local_change_clear_sync_frame_bit(ctrl_frame_def, ctrl_frame_def->frame);

                            // Clear frame scheduling status
                            lin_server_scheduler_unschedule_frame(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST);
                        }
                        else if (lin_server_scheduler_is_frame_scheduled_type_set(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST))
                        {
                            // Clear frame scheduling status
                            lin_server_scheduler_unschedule_frame(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST);
                        }
                        else  // SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST
                        {
                            /* Reset counter, as you can receive new broker request while verifying previos frame */
                            lin_server_reset_ctrl_frame_verification_counter(ctrl_frame_def->frame, LIN_SERVER_CTRL_FRAME_VERIFICATION_COUNTER);
                            /* Request verification of the broker ctrl frame */
                            lin_server_set_ctrl_frame_verification(ctrl_frame_def->frame, SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);

                            /* It's possible to get new parameter(signal) update from the broker, between 'send' and 'verify' states.
                             * If this happens, we should not unschedule the next BROKER frame, as the new update will not be propagated
                             * to the slave device */
                            if (memcmp(ctrl_frame_def->frame->frame_signals.raw_frame, &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN], ctrl_frame_def->frame_len) == 0)
                            {
                                // Clear frame scheduling status
                                lin_server_scheduler_unschedule_frame(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);
                            }
                        }
                        break;
                    }
                    case LIN_SERVER_SYNC_PROTOCOL_NONE:  // Fall through
                    default:
                    {
                        /* Reset counter, as you can receive new broker request while verifying previos frame */
                        lin_server_reset_ctrl_frame_verification_counter(ctrl_frame_def->frame, LIN_SERVER_CTRL_FRAME_VERIFICATION_COUNTER);
                        /* Request verification of the broker ctrl frame */
                        lin_server_set_ctrl_frame_verification(ctrl_frame_def->frame, SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);

                        /* It's possible to get new parameter(signal) update from the broker, between 'send' and 'verify' states.
                         * If this happens, we should not unschedule the next BROKER frame, as the new update will not be propagated
                         * to the slave device */
                        if (memcmp(ctrl_frame_def->frame->frame_signals.raw_frame, &lin_server_uart->rx_buffer.buffer[LIN_FRAME_START_LEN], ctrl_frame_def->frame_len) == 0)
                        {
                            // Clear frame scheduling status
                            lin_server_scheduler_unschedule_frame(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);
                        }
                        break;
                    }
                    }
                    // Handle CTRL post event
                    lin_server_ddm2_to_lin_handle_logic_post_send(slave_device, ctrl_frame_def);
                }
                lin_server_unlock_device(slave_device);
            }

            if (status != 0)
            {
                LOG(E, "[%s]: -> [0x%02X][0x%02X][0x%02X]-[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]-[0x%02X]",
                    slave_device->name,
                    lin_server_uart->rx_buffer.buffer[0], lin_server_uart->rx_buffer.buffer[1], lin_server_uart->rx_buffer.buffer[2],
                    lin_server_uart->rx_buffer.buffer[3], lin_server_uart->rx_buffer.buffer[4], lin_server_uart->rx_buffer.buffer[5],
                    lin_server_uart->rx_buffer.buffer[6], lin_server_uart->rx_buffer.buffer[7], lin_server_uart->rx_buffer.buffer[8],
                    lin_server_uart->rx_buffer.buffer[9], lin_server_uart->rx_buffer.buffer[10], lin_server_uart->rx_buffer.buffer[11]);
                LOG(E, "[%s]: -> [0x%02X][0x%02X][0x%02X]-[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]-[0x%02X]",
                    slave_device->name,
                    lin_server_uart->tx_buffer.buffer[0], lin_server_uart->tx_buffer.buffer[1], lin_server_uart->tx_buffer.buffer[2],
                    lin_server_uart->tx_buffer.buffer[3], lin_server_uart->tx_buffer.buffer[4], lin_server_uart->tx_buffer.buffer[5],
                    lin_server_uart->tx_buffer.buffer[6], lin_server_uart->tx_buffer.buffer[7], lin_server_uart->tx_buffer.buffer[8],
                    lin_server_uart->tx_buffer.buffer[9], lin_server_uart->tx_buffer.buffer[10], lin_server_uart->tx_buffer.buffer[11]);
            }

            return status == 0 ? FEEDBACK_FRAME_STATUS_GOT_CTRL_FRAME : FEEDBACK_FRAME_STATUS_ERROR;
        }
    }
    else if (process_frame->scheduled_frame_def->item->frame_type == LIN_INFO_FRAME)
    {
        if (lin_server_uart->rx_buffer.buffer_length >= LIN_FRAME_START_LEN)
        {
            size_t lenght = MIN(lin_server_uart->rx_buffer.buffer_length, lin_server_uart->tx_buffer.buffer_length);
            int status = memcmp(
                lin_server_uart->rx_buffer.buffer,
                lin_server_uart->tx_buffer.buffer,
                lenght);

            if (status != 0)
            {
                LOG(E, "-> [0x%02X][0x%02X][0x%02X] == [0x%02X][0x%02X][0x%02X]",
                    lin_server_uart->rx_buffer.buffer[0],
                    lin_server_uart->rx_buffer.buffer[1],
                    lin_server_uart->rx_buffer.buffer[2],
                    lin_server_uart->tx_buffer.buffer[0],
                    lin_server_uart->tx_buffer.buffer[1],
                    lin_server_uart->tx_buffer.buffer[2]);
            }

            return status == 0 ? FEEDBACK_FRAME_STATUS_GOT_INFO_FRAME : FEEDBACK_FRAME_STATUS_ERROR;
        }
    }
    else
    {
        return FEEDBACK_FRAME_STATUS_ERROR;
    }

    return FEEDBACK_FRAME_STATUS_WAIT;
}

static int lin_server_uart_send_next_frame(lin_server_uart_t *lin_server_uart)
{
    lin_server_uart_process_frames_t *process_frame = &lin_server_uart->process_frame;
    const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(process_frame->scheduled_frame_def->item->slave_device_type);
    const lin_server_device_frame_def_t *ctrl_frame_def = process_frame->frames_bundle_def->ctrl_frame_def;

    LOG(D, "[%s]: Send frame: 0x%02X[%s]",
        slave_device->name,
        process_frame->scheduled_frame_def->item->frame_id,
        process_frame->scheduled_frame_def->item->frame_type != LIN_CONTROL_FRAME ? "INFO" : lin_server_scheduler_is_frame_scheduled_type_set(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST) ? "CTRL[ACK]"
                                                                                         : lin_server_scheduler_is_frame_scheduled_type_set(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_INIT_REQUEST)           ? "CTRL[INIT]"
                                                                                                                                                                                                                                 : "CTRL[BROKER]");

    // Prepare header(0x00, 0x55, PID)
    uint_fast8_t parity = lin_calculate_parity(process_frame->scheduled_frame_def->item->frame_id) << LIN_PID_FIELD_PARITY_POS;
    uint_fast8_t frame_pid = parity | process_frame->scheduled_frame_def->item->frame_id;
    // Break signal does not need to be send since we generate the break signal seperately.
    lin_server_uart->tx_buffer.buffer[LIN_SYNC_FIELD_BYTE] = LIN_SYNC_FIELD_DATA;
    lin_server_uart->tx_buffer.buffer[LIN_PID_FIELD_BYTE] = frame_pid;

    // Send next frame(CTRL override request, ACK or INFO request)
    if (process_frame->scheduled_frame_def->item->frame_type == LIN_CONTROL_FRAME)
    {
        int status;
        uint8_t buffer[LIN_FRAME_DATA_LEN];
        size_t buffer_size = sizeof(buffer);

        /* Stuff temporary buffer and ctrl frame depending on CTRL frame request(Broker or Local Change)
         *
         * Use LocalChange with higher priority over Broker.
         *
         * We do not need to check for LOCAL_CHANGE_REQUEST in broker task when SET has happened at the
         * same time as we try to do ACK, since the ACK frame depends on the INFO remote data.
         */
        if (lin_server_scheduler_is_frame_scheduled(process_frame->scheduled_frame_def))
        {
            lin_server_lock_device(slave_device);
            {
                switch (lin_server_get_sync_protocol(slave_device))
                {
                case LIN_SERVER_SYNC_PROTOCOL_LOCAL_CHANGE:
                {
                    if (lin_server_scheduler_is_frame_scheduled_type_set(process_frame->scheduled_frame_def, SCHEDULE_FRAME_TYPE_CTRL_LOCAL_CHANGE_REQUEST))
                    {
                        // Set SyncFrame bit to prepare ACK CTRL frame
                        lin_server_sync_protocol_local_change_set_sync_frame_bit(
                            ctrl_frame_def,
                            ctrl_frame_def->frame);
                    }
                }
                case LIN_SERVER_SYNC_PROTOCOL_NONE:  // Fall through
                default:
                    break;
                }

                status = slave_device->stuff_function(
                    slave_device,
                    process_frame->frames_bundle_def,
                    buffer,
                    &buffer_size,
                    process_frame->scheduled_frame_def->item_context.is_scheduled);
            }
            lin_server_unlock_device(slave_device);

            if (status == 0 || (buffer_size != ctrl_frame_def->frame_len))
            {
                LOG(E, "[%s]: Stuff functions unsuccessful", slave_device->name);
                return SEND_FRAME_STATUS_ERROR_STUFF_FUNCTION;
            }

            // ACK to slave, set TX buffer payload
            memcpy(&lin_server_uart->tx_buffer.buffer[LIN_FRAME_START_LEN], buffer, MIN(ctrl_frame_def->frame_len, buffer_size));
        }
        else
        {
            // We should never end up here
            LOG(E, "[%s]: Invalid CTRL scheduled frame type[%d]", slave_device->name, process_frame->scheduled_frame_def->item_context.is_scheduled);
            return SEND_FRAME_STATUS_ERROR_INVALID_FRAME;
        }

        // Prepare checksum
        uint8_t checksum = lin_calculate_checksum(
            frame_pid,
            &lin_server_uart->tx_buffer.buffer[LIN_PID_FIELD_BYTE],
            LIN_SERVER_CHECKSUM_DATA_LEN(ctrl_frame_def->frame_len));
        lin_server_uart->tx_buffer.buffer[LIN_SERVER_CHECSUM_POSITION(ctrl_frame_def->frame_len)] = checksum;

        // -1 for the Break signal as we do not send it as part of this array.
        lin_server_uart->tx_buffer.buffer_length = LIN_SERVER_FRAME_LEN(ctrl_frame_def->frame_len) - 1;

        lin_server_uart_send_frame(
            lin_server_uart,
            &lin_server_uart->tx_buffer.buffer[LIN_SYNC_FIELD_BYTE],
            lin_server_uart->tx_buffer.buffer_length);

        return SEND_FRAME_STATUS_SENT;
    }
    else if (process_frame->scheduled_frame_def->item->frame_type == LIN_INFO_FRAME)
    {
        // -1 for the Break signal as we do not send it as part of this array.
        lin_server_uart->tx_buffer.buffer_length = LIN_FRAME_START_LEN - 1;

        lin_server_uart_send_frame(
            lin_server_uart,
            &lin_server_uart->tx_buffer.buffer[LIN_SYNC_FIELD_BYTE],
            lin_server_uart->tx_buffer.buffer_length);

        return SEND_FRAME_STATUS_SENT;
    }

    return SEND_FRAME_STATUS_ERROR_INVALID_FRAME;
}

static void lin_server_uart_reponse_received(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event)
{
    lin_server_uart_process_frames_t *process_frame = &lin_server_uart->process_frame;
    const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(process_frame->scheduled_frame_def->item->slave_device_type);

    /* reset device bus detection counter, as the slave has replied with new info frame reponse */
    if (slave_device->data->bus_detection_counter != LIN_SERVER_BUS_DETECTION_COUNTER_INIT_VALUE)
    {
        slave_device->data->bus_detection_counter = LIN_SERVER_BUS_DETECTION_COUNTER_INIT_VALUE;
    }
}

static void lin_server_uart_responose_not_received(lin_server_uart_t *lin_server_uart, lin_server_fsm_event_t *server_fsm_event)
{
    lin_server_uart_process_frames_t *process_frame = &lin_server_uart->process_frame;
    const lin_server_slave_device_t *slave_device = lin_server_get_slave_device(process_frame->scheduled_frame_def->item->slave_device_type);

    slave_device->data->bus_detection_counter--;
    if (slave_device->data->bus_detection_counter <= 0)
    {
        /* Device disconneted. Remove device class instance */
        lin_server_remove_device(slave_device);

        /* Device disconneted? Try to detect it again. */
        lin_server_uart->bus_management.bus_probing.are_all_devices_on_bus_detected = false;

        /* Disable INFO/CTRL frames for dedicated slave device */
        lin_server_scheduler_unschedule_frames_by_device_type(process_frame->scheduled_frame_def->item->slave_device_type);
    }
}

/**
 * @brief       Send next frame LIN state
 *
 * @param       fsm  Pointer to FSM object
 * @param       event Pointer to event
 */
static void sm_state_receive_response(fsm_t *fsm, const fsm_event_t *event)
{
    receive_frame_status_t status;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_receive_start);
        LOG(D, "<- state");
        /* buffer clear + lenght = 0 */
        memset(&lin_server_uart->rx_buffer, 0, sizeof(lin_server_uart->rx_buffer));
        /* skip 0x00, 0x55, PID, as we do not get them as part of the slave response */
        lin_server_uart->rx_buffer.buffer_length = LIN_FRAME_START_LEN;
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
    {
        LIN_SERVER_UART_TIMING_METRICS_SET_TIMING_ERROR(lin_server_uart->timing.is_timing_error_detected);
        /* Reinitiate slave detection (bus probing diagnostic frames) */
        lin_server_uart_responose_not_received(lin_server_uart, server_fsm_event);
        LOG(E, "SM state should not receive TIMER event! We ended up in inbetween state!");
        fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
        lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
        break;
    }
    case SM_EVENTS_RESPONSE_TOUT:
        /* If we are waiting for response, this state will timeout faster so we don't wait for next scheduler cycle,
           this will allow to go to sleep earlier in case if response is not received */
#ifdef CONFIG_PM_ENABLE
        lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
        fsm_state_change(fsm, sm_state_idle);
        break;
    case SM_EVENTS_IRQ_TOUT:
        status = lin_server_uart_receive_response(lin_server_uart, server_fsm_event);
        switch (status)
        {
        case RECEIVE_FRAME_STATUS_WAIT:
            /* do nothing, still waiting for full buffer reception */
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStart(timer_response_wait, portMAX_DELAY));
#endif /* CONFIG_PM_ENABLE */
            break;
        case RECEIVE_FRAME_STATUS_GOT_RESPONSE:
            lin_server_uart_reponse_received(lin_server_uart, server_fsm_event);
            /* we have received and processed the INFO frame. Switch back to idle state */
            fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStop(timer_response_wait, portMAX_DELAY));
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        case RECEIVE_FRAME_STATUS_NOT_FOR_US:
            /* we have received internal request by the state machine to try
             * to get response on an non INFO frame. Switch back to idle state
             */
            fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStop(timer_response_wait, portMAX_DELAY));
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        case RECEIVE_FRAME_STATUS_ERROR_CHECKSUM:
            lin_server_uart_reponse_received(lin_server_uart, server_fsm_event);
            /* Invalid checksum of the recieved INFO frame. Switch back to idle state */
            fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
            TRUE_CHECK(xTimerStop(timer_response_wait, portMAX_DELAY));
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        default:
            break;
        }
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    case FSM_EXIT_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_receive_end);
        break;
    default:
        break;
    }
}

/**
 * @brief       Validate sent frame LIN state
 *
 * Validate if what has been set of TX line, is actually what we have received.
 * LIN transmiter has it's own feedback line between TX and RX which allows us to
 * do the validation.
 *
 * @param       fsm  Pointer to FSM object
 * @param       event Pointer to event
 */
static void sm_state_validate_feedback(fsm_t *fsm, const fsm_event_t *event)
{
    feedback_frame_status_t status;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_feedback_start);
        LOG(D, "<- state");

        /* buffer clear + lenght = 0 */
        memset(&lin_server_uart->rx_buffer, 0, sizeof(lin_server_uart->rx_buffer));
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
        LIN_SERVER_UART_TIMING_METRICS_SET_TIMING_ERROR(lin_server_uart->timing.is_timing_error_detected);
        LOG(E, "SM state should not receive TIMER event! We ended up in inbetween state!");
        fsm_state_change(fsm, sm_state_idle);
        break;
    case SM_EVENTS_IRQ_TOUT:
        status = lin_server_uart_validate_feedback(lin_server_uart, server_fsm_event);
        switch (status)
        {
        case FEEDBACK_FRAME_STATUS_WAIT:
            /* do nothing, still waiting for full buffer reception */
            break;
        case FEEDBACK_FRAME_STATUS_GOT_CTRL_FRAME:
            /* Valid feedback. What we sent is what we received. Return to idle. */
            fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        case FEEDBACK_FRAME_STATUS_GOT_INFO_FRAME:
            /* Valid feedback. What we sent is what we received. Move to receive response. */
            fsm_state_change(fsm, sm_state_receive_response);
            break;
        case FEEDBACK_FRAME_STATUS_ERROR:
            /* Invalid feedback. What we sent is not what we received. Possible collision? */
            fsm_state_change(fsm, sm_state_idle);
#ifdef CONFIG_PM_ENABLE
            lin_pm_lock_release();
#endif /* CONFIG_PM_ENABLE */
            break;
        default:
            break;
        }
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    case FSM_EXIT_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_feedback_end);
        break;
    default:
        break;
    }
}

/**
 * @brief       Send next frame LIN state
 *
 * @param       fsm  Pointer to FSM object
 * @param       event Pointer to event
 */
static void sm_state_send_next_frame(fsm_t *fsm, const fsm_event_t *event)
{
    send_frame_status_t status;
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_send_start);
        LOG(D, "<- state");

        /* buffer clear + lenght = 0 */
        memset(&lin_server_uart->tx_buffer, 0, sizeof(lin_server_uart->tx_buffer));

        status = lin_server_uart_send_next_frame(lin_server_uart);
        switch (status)
        {
        case SEND_FRAME_STATUS_SENT:
            /* Response not expected, return to idle state */
            fsm_state_change(fsm, sm_state_validate_feedback);
            break;
        case SEND_FRAME_STATUS_ERROR_STUFF_FUNCTION:
            /* Stuff function uncessfull, we didn't send any data to slave. Switch to idle state */
            fsm_state_change(fsm, sm_state_idle);
            break;
        case SEND_FRAME_STATUS_ERROR_INVALID_FRAME:
            /* Not supported frame type by send_next_frame(), we didn't send any data to slave. Switch to idle state */
            fsm_state_change(fsm, sm_state_idle);
            break;
        }
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
        LIN_SERVER_UART_TIMING_METRICS_SET_TIMING_ERROR(lin_server_uart->timing.is_timing_error_detected);
        LOG(E, "SM state should not receive TIMER event! We ended up in inbetween state!");
        fsm_state_change(fsm, sm_state_idle);
        break;
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    case FSM_EXIT_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_send_end);
        break;
    default:
        break;
    }
}

static bool lin_server_uart_is_signal_frame_in_progress(lin_server_uart_t *lin_server_uart)
{
    bool is_signal_frame_pending = lin_server_state_evaluate();
    if (is_signal_frame_pending == true)
    {
        if (lin_server_uart_sleep_is_requested(lin_server_uart) == true)
        {
            lin_server_state_request_t requested_by = lin_server_uart_state_change_requested_by(lin_server_uart);
            switch (requested_by)
            {
            case LIN_SERVER_STATE_SLEEP_REQUEST_TIMER:
                LOG(D, "Timer's sleep request removed.");
                lin_server_uart_sleep_reset_request(lin_server_uart);
                break;
            case LIN_SERVER_STATE_SLEEP_REQUEST_BROKER:
                LOG(D, "Broker's sleep request pending.");
                break;
            default:
                LOG(E, "Unsupported sleep request[%d].", requested_by);
                break;
            }
        }
    }
    return is_signal_frame_pending;
}
/*** END SIGNAL FRAME IMPLEMENTATION  ***/

static void sm_state_sleeping(fsm_t *fsm, const fsm_event_t *event)
{
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LOG(D, "<- state");

        /* Disable rx interrupts */
        sm_handle_disable_irq_rx(lin_server_uart);
        /* Transceiver sleep */
#ifndef CONFIG_PM_ENABLE
        LIN_MSTR_SLEEP(0);
#endif /* !CONFIG_PM_ENABLE */
        /* Disable scheduler */
        lin_server_scheduler_enable(false);
        /* Inform lin server that is sleeping */
        lin_server_switch_state(LIN_SERVER_STATE_SLEEPING);
        /* Reset sleeping request */
        lin_server_uart_sleep_reset_request(lin_server_uart);
        LOG(D, "entered sleep mode");
        break;
    case SM_EVENTS_IRQ_BRK:
        lin_server_trigger_wakeup_event(LIN_SERVER_STATE_WAKEUP_REQUEST_SLAVE);
        break;
    case SM_EVENTS_WAKE_UP:
        lin_server_uart_wakeup_register_request(lin_server_uart, server_fsm_event);
        /* Inform lin server that is pending for wakeup */
        lin_server_switch_state(LIN_SERVER_STATE_WAKEUP_PENDING);

        lin_server_state_request_t requested_by = lin_server_uart_state_change_requested_by(lin_server_uart);
        switch (requested_by)
        {
        case LIN_SERVER_STATE_WAKEUP_REQUEST_TIMER:
        case LIN_SERVER_STATE_WAKEUP_REQUEST_BROKER:
            /* Transiver wakeup */
#ifndef CONFIG_PM_ENABLE
            LIN_MSTR_SLEEP(1);
#endif /* !CONFIG_PM_ENABLE */
            /* Generate break signal */
            lin_server_uart_send_wakeup_signal(lin_server_uart->uart_port);
            break;
        case LIN_SERVER_STATE_WAKEUP_REQUEST_SLAVE:
#ifndef CONFIG_PM_ENABLE
            LIN_MSTR_SLEEP(1);
#endif /* !CONFIG_PM_ENABLE */
            /* Do not generate break signal, as it was already generated by the slave */
            break;
        default:
            break;
        }

        fsm_state_change(fsm, sm_state_init);
        break;
    case FSM_EXIT_EVENT:
        break;
    default:
        LOG(E, "Unhandled event: %d", event->id);
        break;
    }
}

static lin_server_frame_selection_t lin_server_frame_selection(lin_server_uart_t *lin_server_uart)
{
    lin_server_frame_selection_t frame_selection_type = LIN_SERVER_FRAME_SELECTION_NONE;

    // Check scheduled signal frames first
    if (lin_server_scheduler_get_next_scheduled_frame())
    {
        frame_selection_type = LIN_SERVER_FRAME_SELECTION_SIGNAL;
    }
    // Check if signal frame is in progress
    else if (lin_server_uart_is_signal_frame_in_progress(lin_server_uart))
    {
        // Try to get next frame again as we have reached the end of the table
        if (lin_server_scheduler_get_next_scheduled_frame())
        {
            frame_selection_type = LIN_SERVER_FRAME_SELECTION_SIGNAL;
        }
        else
        {
            // should never get here as there should always be
            // scheduled frame when a signal frame is in progress
            frame_selection_type = LIN_SERVER_FRAME_SELECTION_NONE;
        }
    }
    // Check for sleep request last
    else if (lin_server_uart_sleep_is_requested(lin_server_uart))
    {
        frame_selection_type = LIN_SERVER_FRAME_SELECTION_DIAG_SLEEP;
    }
    // Check for device detection
    else if (lin_server_uart->bus_management.bus_probing.are_all_devices_on_bus_detected)
    {
        // Try to get next frame again as we have reached the end of the table
        if (lin_server_scheduler_get_next_scheduled_frame())
        {
            frame_selection_type = LIN_SERVER_FRAME_SELECTION_SIGNAL;
        }
        else
        {
            // should never get here as there should always be
            // scheduled frame when all devices are detected on bus
            frame_selection_type = LIN_SERVER_FRAME_SELECTION_NONE;
        }
    }
    else
    {
        frame_selection_type = LIN_SERVER_FRAME_SELECTION_DIAG_DEVICE_DETECTION;
    }

    return frame_selection_type;
}

/**
 * @brief       Idle LIN state
 *
 * @param       fsm  Pointer to FSM object
 * @param       event Pointer to event
 */
static void sm_state_idle(fsm_t *fsm, const fsm_event_t *event)
{
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);
    lin_server_fsm_event_t *server_fsm_event = event->p_data;

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_TRACK_STATE_TIME(lin_server_uart);
        LOG(D, "<- state");
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
    {
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_idle_start);
        LIN_SERVER_UART_TIMING_METRICS_TRACK_STATE_TIME_INIT(lin_server_uart->timing.timer_tick, ((lin_server_fsm_event_t *)event->p_data)->timer_event_us);

        lin_server_frame_selection_t frame_selection_type = lin_server_frame_selection(lin_server_uart);
        switch (frame_selection_type)
        {
        case LIN_SERVER_FRAME_SELECTION_SIGNAL:
            LOG(D, "LIN_SERVER_FRAME_SELECTION_SIGNAL");
            lin_server_uart_prepare_frames(lin_server_uart);
            fsm_state_change(fsm, sm_state_send_next_frame);
            break;
        case LIN_SERVER_FRAME_SELECTION_DIAG_SLEEP:
            LOG(D, "LIN_SERVER_FRAME_SELECTION_DIAG_SLEEP");
            lin_server_uart_set_diagnostic_nad(lin_server_uart, LIN_NAD_GO_TO_SLEEP_COMMAND);
            fsm_state_change(fsm, sm_state_diag_send_request);
            break;
        case LIN_SERVER_FRAME_SELECTION_DIAG_DEVICE_DETECTION:
            LOG(D, "LIN_SERVER_FRAME_SELECTION_DIAG_DEVICE_DETECTION");
            fsm_state_change(fsm, sm_state_diag_initiate_device_detection);
            break;
        case LIN_SERVER_FRAME_SELECTION_NONE:  // fall through
        default:
            LOG(E, "Usupported frame type: %d", frame_selection_type);
            break;
        }
        break;
    }
    case SM_EVENTS_GO_TO_SLEEP:
        lin_server_uart_sleep_register_request(lin_server_uart, server_fsm_event);
        break;
    case FSM_EXIT_EVENT:
        LIN_SERVER_UART_TIMING_METRICS_GET_TIME(lin_server_uart->timing.sm_idle_end);
        break;
    default:
        break;
    }
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
    lin_server_uart_t *lin_server_uart = isr_sm_to_lindev(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        LOG(D, "<- state");
        /* Avoid immediate wake-up; the cluster requires
         * at least 100ms to be ready to listen on the bus
         */
        lin_server_scheduler_enable(true);
        lin_server_scheduler_change_time_period(LIN_SERVER_WAKEUP_SIGNAL_PENDING_TIME_MS);
        break;
    case SM_EVENTS_SCHEDULER_TIMER:
        /* Reset fifo */
        sm_handle_uart_reset_fifo(lin_server_uart);
        /* Re-enable (RX) interrupts */
        sm_handle_enable_irq_all(lin_server_uart);
        /* Reset wakeup request */
        lin_server_uart_wakeup_reset_request(lin_server_uart);
        /* Inform lin server that has woken up */
        lin_server_switch_state(LIN_SERVER_STATE_OPERATIONAL);

        /* Switch to operational(idle) state */
        fsm_state_change(fsm, sm_state_idle);
        LOG(D, "Woke-up as request from: %d",
            lin_server_uart_state_change_requested_by(lin_server_uart));
        break;
    case SM_EVENTS_IRQ_BRK:
        LOG(D, "[%d] Already waking up, as request from: %d",
            SM_EVENTS_IRQ_BRK,
            lin_server_uart_state_change_requested_by(lin_server_uart));
        break;
    case SM_EVENTS_WAKE_UP:
        LOG(D, "[%d] Already waking up, as request from: %d",
            SM_EVENTS_WAKE_UP,
            lin_server_uart_state_change_requested_by(lin_server_uart));
        break;
    case FSM_EXIT_EVENT:
        lin_server_scheduler_change_time_period(LIN_SERVER_TICK_TIMER_PERIOD_MS);
        break;
    default:
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
    BaseType_t retval;
    lin_server_uart_t *lin_server = parameter;
    lin_server_fsm_event_t server_fsm_event;

    fsm_initialize(&lin_server->sm, sm_state_init);

    LOG(I, "Started LIN event loop after %u ms", pdMS_TO_TICKS(2000));
    for (;;)
    {
        TRUE_CHECK(retval = xQueueReceive(lin_server->sm_event_queue, &server_fsm_event, portMAX_DELAY));
        if (retval == pdPASS)
        {
            server_fsm_event.fsm_event.p_data = &server_fsm_event;
            fsm_state_dispatch(&lin_server->sm, &server_fsm_event.fsm_event);
        }
    }
}

static LIN_SERVER_ISR_ATTR void lin_server_fsm_event_init(lin_server_fsm_event_t *event, uint32_t event_id)
{
    LIN_SERVER_UART_TIMING_METRICS_GET_TIME(event->timer_event_us);
    event->fsm_event.id = event_id;
    event->fsm_event.p_data = &event->data;
}

/**
 * @brief       UART peripheral interrupt handler
 *
 * @param       parameter Pointer to lindev instance
 */
static void LIN_SERVER_ISR_ATTR uart_intr_handler(void *parameter)
{
    lin_server_uart_t *lin_server = parameter;
    BaseType_t is_a_task_woken = pdFALSE;  //!< A flag used to signal to FreeRTOS that a context switch is needed
    uint32_t uart_intr_status = uart_ll_get_intsts_mask(lin_server->uart_dev);

    if (uart_intr_status & UART_RXFIFO_FULL_INT_ST_M)
    {
        uart_ll_rxfifo_rst(lin_server->uart_dev);
        uart_ll_clr_intsts_mask(lin_server->uart_dev, UART_RXFIFO_FULL_INT_CLR_M);
        lin_server_fsm_event_init(&lin_server->isr_event, SM_EVENTS_IRQ_FULL);
        xQueueSendFromISR(lin_server->sm_event_queue, &lin_server->isr_event, &is_a_task_woken);
    }
    else if (uart_intr_status & UART_RXFIFO_OVF_INT_ST_M)
    {
        uart_ll_rxfifo_rst(lin_server->uart_dev);
        uart_ll_clr_intsts_mask(lin_server->uart_dev, UART_RXFIFO_OVF_INT_CLR_M);
        lin_server_fsm_event_init(&lin_server->isr_event, SM_EVENTS_IRQ_OVF);
        xQueueSendFromISR(lin_server->sm_event_queue, &lin_server->isr_event, &is_a_task_woken);
    }
    else if (uart_intr_status & UART_BRK_DET_INT_ST_M)
    {
        uart_ll_clr_intsts_mask(lin_server->uart_dev, UART_BRK_DET_INT_CLR_M);
        lin_server_fsm_event_init(&lin_server->isr_event, SM_EVENTS_IRQ_BRK);
        xQueueSendFromISR(lin_server->sm_event_queue, &lin_server->isr_event, &is_a_task_woken);
    }
    else if (uart_intr_status & UART_RXFIFO_TOUT_INT_ST_M)
    {
#ifdef CONFIG_PM_ENABLE
        /* Get RX data only if the lock is acquired, active only during TX/RX cycle, ignore any bus activity outide TX/RX cycle.
           Because of power manager activation UART peripheral catches noice and interrupt is triggered, that is work arround */
        if (lock_is_acquired())
#else
        /* Default data handling, without power manager */
        if (1)
#endif /* CONFIG_PM_ENABLE */
        {
            uint32_t rxfifo_len = uart_ll_get_rxfifo_len(lin_server->uart_dev);
            lin_server->isr_event.data.buffer_length = MIN(LIN_FRAME_INFO_REQUEST_LEN, rxfifo_len);
            /* Valid data - process normally */
            uart_ll_read_rxfifo(lin_server->uart_dev, lin_server->isr_event.data.buffer, lin_server->isr_event.data.buffer_length);
            lin_server_fsm_event_init(&lin_server->isr_event, SM_EVENTS_IRQ_TOUT);
            uart_ll_clr_intsts_mask(lin_server->uart_dev, UART_RXFIFO_TOUT_INT_CLR_M);
            xQueueSendFromISR(lin_server->sm_event_queue, &lin_server->isr_event, &is_a_task_woken);
        }
        else
        {
            /* Just clear the interrupt and ignore the data */
            uart_ll_rxfifo_rst(lin_server->uart_dev);
            uart_ll_clr_intsts_mask(lin_server->uart_dev, UART_RXFIFO_TOUT_INT_CLR_M);
        }
    }
    else
    {
        lin_server_fsm_event_init(&lin_server->isr_event, SM_EVENTS_IRQ_UNKNOWN);
        xQueueSendFromISR(lin_server->sm_event_queue, &lin_server->isr_event, &is_a_task_woken);
    }

    if (is_a_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

void lin_server_uart_start(void)
{
    BaseType_t status;

    /* Provide callback funtion to the scheduler that will drive the state machine */
    lin_server_scheduler_set_execution_callback(lin_server_uart_trigger_next_frame_by_scheduler);
    /* Create Lin Server Uart task */
    TRUE_CHECK_RETURN(status = xTaskCreate(event_loop_task, "LIN Server event loop", LIN_SERVER_EVENT_LOOP_TASK_STACK_SIZE, &lin_server_uart, xTASK_PRIORITY_REALTIME, NULL));
}

void lin_server_uart_init(void)
{
    memset(&lin_server_uart, 0, sizeof(lin_server_uart));

    lin_server_uart.uart_dev = &CONNECTOR_LIN_SERVER_UART_DEV;
    lin_server_uart.uart_port = CONNECTOR_LIN_SERVER_UART_NUM;
    ZERO_CHECK(uart_driver_install(CONNECTOR_LIN_SERVER_UART_NUM, 256, 0, 0, NULL, 0));
    ZERO_CHECK(uart_isr_free(CONNECTOR_LIN_SERVER_UART_NUM));
    ZERO_CHECK(uart_isr_register(CONNECTOR_LIN_SERVER_UART_NUM, uart_intr_handler, &lin_server_uart, 0, NULL));
    ZERO_CHECK(uart_disable_pattern_det_intr(CONNECTOR_LIN_SERVER_UART_NUM));
    ZERO_CHECK(uart_disable_tx_intr(CONNECTOR_LIN_SERVER_UART_NUM));
    ZERO_CHECK(uart_disable_rx_intr(CONNECTOR_LIN_SERVER_UART_NUM));
#if CONFIG_IDF_TARGET_ESP32S3
    ZERO_CHECK(uart_set_rx_timeout(CONNECTOR_LIN_SERVER_UART_NUM, 1));  // relate with LIN Master receice
#else
    ZERO_CHECK(uart_set_rx_timeout(CONNECTOR_LIN_SERVER_UART_NUM, 3));
#endif
    ZERO_CHECK(uart_disable_intr_mask(CONNECTOR_LIN_SERVER_UART_NUM, UART_LL_INTR_MASK));
    ZERO_CHECK(uart_param_config(CONNECTOR_LIN_SERVER_UART_NUM, &default_uart_config));
    ZERO_CHECK(uart_set_pin(CONNECTOR_LIN_SERVER_UART_NUM, CONNECTOR_LIN_SERVER_UART_TX, CONNECTOR_LIN_SERVER_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    lin_server_uart.sm_event_queue = xQueueCreateStatic(
        ELEMENTS(lin_server_uart.sm_event_queue_entries),
        sizeof(lin_server_uart.sm_event_queue_entries[0]),
        (uint8_t *)&lin_server_uart.sm_event_queue_entries[0],
        &lin_server_uart.sm_event_queue_instance);
    TRUE_CHECK_RETURN(lin_server_uart.sm_event_queue != NULL);
#ifdef CONFIG_PM_ENABLE
    esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "lin", &lin_pm_lock);
    TRUE_CHECK_RETURN(timer_response_wait = xTimerCreate("LIN slave response wait", pdMS_TO_TICKS(2), pdFALSE, NULL, lin_wait_slave_response));
#endif /* CONFIG_PM_ENABLE */
}

static void lin_server_uart_trigger_next_frame_by_scheduler(void)
{
    lin_server_fsm_event_t event;
    lin_server_fsm_event_init(&event, SM_EVENTS_SCHEDULER_TIMER);

    xQueueSend(lin_server_uart.sm_event_queue, &event, portMAX_DELAY);
}

void lin_server_trigger_go_to_sleep_event(lin_server_state_request_t sleep_request)
{
    TRUE_CHECK_RETURN(sleep_request < UINT8_MAX);
    TRUE_CHECK_RETURN(sleep_request >= LIN_SERVER_STATE_SLEEP_REQUEST_TIMER && sleep_request <= LIN_SERVER_STATE_SLEEP_REQUEST_BROKER);
    lin_server_fsm_event_t event = {
        .data = {
            .buffer[0] = (uint8_t)sleep_request,
            .buffer_length = sizeof(uint8_t),
        },
    };
    lin_server_fsm_event_init(&event, SM_EVENTS_GO_TO_SLEEP);

    xQueueSend(lin_server_uart.sm_event_queue, &event, portMAX_DELAY);
}

void lin_server_trigger_wakeup_event(lin_server_state_request_t wakeup_request)
{
    TRUE_CHECK_RETURN(wakeup_request < UINT8_MAX);
    TRUE_CHECK_RETURN(wakeup_request >= LIN_SERVER_STATE_WAKEUP_REQUEST_TIMER && wakeup_request <= LIN_SERVER_STATE_WAKEUP_REQUEST_SLAVE);
    lin_server_fsm_event_t event = {
        .data = {
            .buffer[0] = (uint8_t)wakeup_request,
            .buffer_length = sizeof(uint8_t),
        },
    };
    lin_server_fsm_event_init(&event, SM_EVENTS_WAKE_UP);

    xQueueSend(lin_server_uart.sm_event_queue, &event, portMAX_DELAY);
}
