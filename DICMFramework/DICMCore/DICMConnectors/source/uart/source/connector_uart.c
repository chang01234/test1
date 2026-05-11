/*! \file connector_uart.c
    \brief DDMP2 UART connector
*/

#include <stdint.h>
#include <string.h>

#include "esp_idf_version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#include "driver/uart.h"  // UART port and configuration definition
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/myuart.h"  // UART port and configuration definition
#else
#include "driver/uart.h"  // UART port and configuration definition
#endif
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "configuration.h"
#include "connector_uart.h"
#include "ddm2.h"
#include "uart_encapsulation.h"

#if (CONFIG_UART_ISR_IN_IRAM && CONFIG_RINGBUF_PLACE_FUNCTIONS_INTO_FLASH)
#error "Ringbuffer ISR functions in flash are incompatible with UART ISR in IRAM."
#endif
#ifndef CONNECTOR_UART_CPU_CORE
#define CONNECTOR_UART_CPU_CORE ((BaseType_t)1)  // Default to CPU1
#endif
#ifndef CONNECTOR_UART_PROCESS_CPU_CORE
#define CONNECTOR_UART_PROCESS_CPU_CORE CONNECTOR_UART_CPU_CORE
#endif
#ifndef CONNECTOR_UART_STACK_SIZE
#define CONNECTOR_UART_STACK_SIZE 3072
#endif
#ifndef CONNECTOR_UART_PROCESS_STACK_SIZE
#define CONNECTOR_UART_PROCESS_STACK_SIZE 3072
#endif
#ifndef CONNECTOR_UART_FRAME_RX_EXTENDED_LOGS
#define CONNECTOR_UART_FRAME_RX_EXTENDED_LOGS (0)
#endif
#ifndef CONNECTOR_UART_FRAME_TX_EXTENDED_LOGS
#define CONNECTOR_UART_FRAME_TX_EXTENDED_LOGS (0)
#endif

#ifndef CONNECTOR_UART_DRIVER_QUEUE_SIZE
#define CONNECTOR_UART_DRIVER_QUEUE_SIZE (0x1000)  //!< \~ Size of UART driver event queue
#endif

#define UART_TX_BUFFER_SIZE (0x10000)  // UART driver TX buffer size
#define UART_RX_BUFFER_SIZE (0x10000)  // UART driver RX buffer size

#ifndef CONNECTOR_UART_BUF_SIZE
#define CONNECTOR_UART_BUF_SIZE (0x10000)  // Size of the DDM2 parse buffer and temporary buffer
#endif

#ifndef CONNECTOR_UART_QUEUE_POLLING_INTERVAL_MS
#define CONNECTOR_UART_QUEUE_POLLING_INTERVAL_MS 0  // Default to blocking
#endif

#if (CONNECTOR_UART_QUEUE_POLLING_INTERVAL_MS <= 0)  // Interval to poll UART event queue (>0 = polling, 0 = blocking)
#define UART_QUEUE_POLLING portMAX_DELAY
#else
#define UART_QUEUE_POLLING pdMS_TO_TICKS(CONNECTOR_UART_QUEUE_POLLING_INTERVAL_MS)
#endif

static QueueHandle_t uart_queue;  //!< \~ Handles to UART1 event queue
static EXT_RAM_ATTR uint8_t ddmp2_uart_buffer[CONNECTOR_UART_BUF_SIZE];

/**
 * @brief   UART configuration
 * Configure UART. Note that REF_TICK is used so that the baud rate remains
 * correct while APB frequency is changing in light sleep mode.
 */

static const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    .source_clk = UART_SCLK_REF_TICK,
#else
    .source_clk = UART_SCLK_XTAL,
#endif
#endif
};

static int ddmp2_send_uart(DDMP2_FRAME *pframe);
static void connector_uart_process_task(void *Parameter);

static void initialize_uart(void)
{
    LOG(I, "Tx: GPIO%d Rx: GPIO%d", CONNECTOR_UART_TX, CONNECTOR_UART_RX);
    ZERO_CHECK(uart_param_config(CONNECTOR_UART_NUM, &uart_config));
    ZERO_CHECK(uart_set_pin(CONNECTOR_UART_NUM, CONNECTOR_UART_TX, CONNECTOR_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ZERO_CHECK(uart_driver_install(CONNECTOR_UART_NUM, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, CONNECTOR_UART_DRIVER_QUEUE_SIZE, &uart_queue, 0));
    ZERO_CHECK(uart_set_rx_full_threshold(CONNECTOR_UART_NUM, 64));
};

static void receive_from_uart(const void *const buffer, size_t size)
{
    DDMP2_FRAME frame;
    DDMP2_ENCAPSULATION_STATUS_ENUM result;
    int done = 0;

    while (!done)
    {
        result = ddmp2_uart_receive(&frame, ddmp2_uart_buffer, sizeof(ddmp2_uart_buffer), buffer, size, connector_uart.connector_id);
        size = 0;  // don't resubmit data next call

        switch (DDMP2_ENC_ERR_CLASS(result))
        {
        case DDMP2_ENC_OK:
            done = 1;  // no error, finished
            break;

        case DDMP2_ENC_FRAME:  // received frame, handle and continue
#if CONNECTOR_UART_FRAME_RX_EXTENDED_LOGS
            ESP_LOG_BUFFER_HEXDUMP("UART RX frame", &frame, frame.frame_size + DDMP2_METADATA_SIZE, ESP_LOG_INFO);
#endif
            switch (frame.frame.control)
            {
            default:
                connector_forward_frame_to_broker(&frame);
                break;

            case DDMP2_CONTROL_NOP:
                break;

            case DDMP2_CONTROL_MESSAGE:
                switch (frame.frame.message.id)  // which message id?
                {
                case DDMP2_MESSAGE_RESET:  // reset message, client has just started up
                    LOG(I, "Forwarding reset to broker");
                    connector_forward_frame_to_broker(&frame);
                    break;

                case DDMP2_MESSAGE_PING:  // reply to ping message
                    LOG(D, "Replying to ping message from UART connector (%d)", connector_uart.connector_id);

                    DECLARE_DDMP2_MESSAGE_FRAME(Ping_reply, DDMP2_MESSAGE_PINGREPLY, connector_uart.connector_id);

                    ddmp2_send_uart((DDMP2_FRAME *)&Ping_reply);
                    break;

                case DDMP2_MESSAGE_ERROR:
                    LOG(W, "UART Protocol error");
                    break;
                }
                break;
            }
            break;

        case DDMP2_ENC_ERROR:  // critical error, abort
            LOG(E, "ddmp2_uart_receive() returned error %d", result);
            done = 1;
            break;

        case DDMP2_ENC_WARNING:  // non-critical error, continue
            LOG(W, "ddmp2_uart_receive() returned warning %d", result);
            break;

        default:
            LOG(W, "ddmp2_uart_receive() unhandled returned %d", result);
            break;
        }
    }
}

static void uart_queue_task(void *pvParameter)
{
    int uart_num = (int)pvParameter;
    uart_event_t event;
    static EXT_RAM_ATTR uint8_t rx_buffer[CONNECTOR_UART_BUF_SIZE];
    int read_size;

    // Initialize uart driver in task
    initialize_uart();
    // Start process task on "same" core (configurable)
    TRUE_CHECK(xTaskCreatePinnedToCore(connector_uart_process_task, (char *)connector_uart.name, CONNECTOR_UART_PROCESS_STACK_SIZE, NULL, xTASK_PRIORITY_NORMAL, NULL, CONNECTOR_UART_PROCESS_CPU_CORE));

    while (1)
    {
        TRUE_CHECK(xQueueReceive(uart_queue, (void *)&event, (TickType_t)UART_QUEUE_POLLING));

        switch (event.type)
        {
        case UART_DATA:
            do
            {
                NONNEG_CHECK(read_size = uart_read_bytes(uart_num, rx_buffer, sizeof(rx_buffer), 0));
                receive_from_uart(rx_buffer, read_size);
            } while (read_size == sizeof(rx_buffer));
            break;

        case UART_FIFO_OVF:
            LOG(E, "HW FIFO overflow, flushing");
            uart_flush_input(uart_num);
            break;

        case UART_BUFFER_FULL:
            LOG(E, "Ring buffer full, flushing");
            uart_flush_input(uart_num);
            break;

        case UART_BREAK:
#if CONNECTOR_UART_FRAME_RX_EXTENDED_LOGS
            LOG(I, "UART RX break");
#endif
            break;

        case UART_PARITY_ERR:
            LOG(W, "UART parity error");
            break;

        case UART_FRAME_ERR:
            LOG(W, "UART frame error");
            break;

        default:
            LOG(I, "UART event type: %d", event.type);
            break;
        }
    }
}

static int ddmp2_send_uart(DDMP2_FRAME *pframe)
{
    ASSERT(pframe);

    int result;
    DDMP2_UART_FRAME_BUFFER uart_frame_buffer;
    size_t uart_frame_size;

    ASSERT(pframe->frame_size);

    uart_frame_size = ddmp2_uart_bytestuff_frame(uart_frame_buffer, pframe);
#if CONNECTOR_UART_FRAME_TX_EXTENDED_LOGS
    ESP_LOG_BUFFER_HEXDUMP("UART TX frame", uart_frame_buffer, uart_frame_size, ESP_LOG_INFO);
#endif
    NONNEG_CHECK(result = uart_write_bytes(CONNECTOR_UART_NUM, (char *)uart_frame_buffer, uart_frame_size));

    return (result >= 0);
}

static void connector_uart_process_task(void *Parameter)
{
    DDMP2_FRAME *pframe;
    size_t frame_size;

    DDMP2_FRAME init;
    ddmp2_create_message(&init, DDMP2_MESSAGE_RESET, connector_uart.connector_id);
    ddmp2_send_uart(&init);

    while (1)
    {
        TRUE_CHECK(pframe = xRingbufferReceive(connector_uart.to_connector, &frame_size, portMAX_DELAY));

        switch (pframe->frame.control)
        {
        case DDMP2_CONTROL_SUBSCRIBE:
        case DDMP2_CONTROL_SET:
        case DDMP2_CONTROL_REG:
        case DDMP2_CONTROL_PUBLISH:
            ddmp2_send_uart(pframe);
            break;

        default:
            LOG(W, "UART connector recived UNHANDLED frame %02x from broker!", pframe->frame.control);
        }
        vRingbufferReturnItem(connector_uart.to_connector, pframe);
    }
}

static int initialize_connector_uart(void)
{
#ifdef EOL_PASS /* Disable RS232 UART for SHARC project - used for EOL testing */
    /* Don't initialize RS232 if EOL test passed */
    if (get_eolpass() != EOL_PASS)
    {
#endif /* EOL_PASS */

        TRUE_CHECK(xTaskCreatePinnedToCore(uart_queue_task, "uart" STR(CONNECTOR_UART_NUM), CONNECTOR_UART_STACK_SIZE, (void *)CONNECTOR_UART_NUM, xTASK_PRIORITY_HIGH, NULL, CONNECTOR_UART_CPU_CORE));

#ifdef CONFIG_DICM_TARGET_MARINE_GW_1
        // DICI_ENABLE(1);
#endif
#ifdef CONFIG_DICM_TARGET_DISP_MODEM_V1
        ESP_FORCEOFF(1);
#endif
#ifdef EOL_PASS /* Disable RS232 UART for SHARC project - used for EOL testing */
    }
#endif /* EOL_PASS */
    return 1;
}

CONNECTOR connector_uart = {
    .name = "UART connector",
    .initialize = initialize_connector_uart,
};
