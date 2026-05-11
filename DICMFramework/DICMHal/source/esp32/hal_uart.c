/*****************************************************************************
 * \file       hal_uart.c
 * \brief      UART Hardware Abstraction Layer ESP32 implementation
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "dicm_framework_config.h"
#ifndef MODEM_UART
#include "iGeneralDefinitions.h"
#include "esp_idf_version.h"

#include "hal_uart.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#include "driver/uart.h"                    // UART port and configuration definition
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/myuart.h"                    // UART port and configuration definition
#else
#include "driver/uart.h"                    // UART port and configuration definition
#endif

/*****************************************************************************
 * Private Defines
 *****************************************************************************/
#define UART_PORT_COUNT  UART_NUM_MAX

/*****************************************************************************
 * Private Variables
 *****************************************************************************/
static struct
{
    volatile bool initialized;
    StaticSemaphore_t instance;
    SemaphoreHandle_t handle;
} hal_uart_mutex[UART_PORT_COUNT] = {0};

//! \~ Initialize UART HAL
int hal_uart_init(int uart_port, int tx_pin, int rx_pin, int baudrate, int rx_fifo, int tx_fifo)
{
	const uart_config_t uart_config =
	{
		.baud_rate  = baudrate,
		.data_bits  = UART_DATA_8_BITS,
		.parity     = UART_PARITY_DISABLE,
		.stop_bits  = UART_STOP_BITS_1,
		.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    #if CONFIG_IDF_TARGET_ESP32C2
        .source_clk = UART_SCLK_PLL_F40M,
    #else
		.source_clk = UART_SCLK_APB,
    #endif
	};

	LOG(I,"Tx=GPIO%d Rx=GPIO%d Fifo=%d:%d", tx_pin, rx_pin, rx_fifo, tx_fifo);
	ZERO_CHECK(uart_param_config(uart_port, &uart_config));
	ZERO_CHECK(uart_set_pin(uart_port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ZERO_CHECK(uart_driver_install(uart_port, rx_fifo, tx_fifo, 0, NULL, 0));

	return ESP_OK;
}

//! \~ Read data from UART device
int hal_uart_read(int uart_port, char *const data_rd, size_t length, uint16_t timeout_ticks)
{
	int rx_count = uart_read_bytes(uart_port, (uint8_t *)data_rd, length, timeout_ticks);
	//NONNEG_CHECK(rx_count);
	return rx_count;
}

//! \~ Write data to UART device
int hal_uart_write(int uart_port,  const char *data_wr, const size_t length)
{
	int tx_count = uart_write_bytes(uart_port, data_wr, length);
	//NONNEG_CHECK(tx_count);	
	return tx_count;
}

//! \~ Write data to UART device
int hal_uart_wait_tx(int uart_port)
{
	return uart_wait_tx_done(uart_port, portMAX_DELAY);
}

//! \~ Require exclusive access to specified port
int hal_uart_acquire(int uart_port)
{
    // Check port
    if (uart_port < UART_PORT_COUNT)
    {
        // Create mutex?
        if (hal_uart_mutex[uart_port].initialized != true)
        {
            hal_uart_mutex[uart_port].handle      = xSemaphoreCreateRecursiveMutexStatic(&hal_uart_mutex[uart_port].instance);
            hal_uart_mutex[uart_port].initialized = true;
        }

        // Acquire
        xSemaphoreTakeRecursive(hal_uart_mutex[uart_port].handle, portMAX_DELAY);
        return HAL_E_OK;
    }
    return HAL_E_DEVICE;
}

//! \~ Release exclusive access to specified port
void hal_uart_release(int uart_port)
{
    // Check port
    if (uart_port < UART_PORT_COUNT)
    {
        // Release
        xSemaphoreGiveRecursive(hal_uart_mutex[uart_port].handle);
    }
}

#endif // ifndef MODEM_UART
