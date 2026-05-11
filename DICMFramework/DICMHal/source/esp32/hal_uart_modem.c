/*! \file hal_uart.c
	\brief UART Hardware Abstraction Layer ESP32 implementation
 */

#include "dicm_framework_config.h"
#ifdef MODEM_UART
#include "iGeneralDefinitions.h"
#include "hal_uart_modem.h"
#include "driver/uart.h"
#include "esp32/rom/uart.h"

extern char *uart_rx_buffer;

//! \~ Write data to UART device
int hal_uart_write (void* uart_port,  const char *data_wr, const size_t length)
{
	int result;
	NONNEG_CHECK(result = uart_write_bytes(*(int *)uart_port, data_wr, length));
	return (!(result >= 0));
}

//! \~ Read data from UART device
int hal_uart_read (void* uart_port, char *const data_rd, size_t length)
{
	//uint16_t urxlen;
#if 1
	int rx_count;
	NONNEG_CHECK(rx_count = uart_read_bytes(*(int *)uart_port, (uint8_t *)data_rd, length, 200 / portTICK_RATE_MS));
	return rx_count;
#endif
//FIXME: For Interrupt Check
#if 0
	uint16_t rx_fifo_len, status;
	uint16_t i=0;

	status = UART1.int_st.val; // read UART interrupt Status
	(void)status;
	rx_fifo_len = UART1.status.rxfifo_cnt; // read number of bytes in UART buffer
	urxlen = rx_fifo_len;
	uart_rx_buffer = data_rd;

	while(rx_fifo_len){
		uart_rx_buffer[i++] = UART1.fifo.rw_byte; // read all bytes 
		rx_fifo_len--;
	}
	return urxlen;
#endif

}

//! \~ Initialize UART HAL
int hal_uart_init(void* uart_port, const int tx_pin, const int rx_pin, const int rts_pin, const int cts_pin, int baudrate)
{
	int port = *(int *)uart_port;
	const uart_config_t uart_config =
	{
		.baud_rate = baudrate,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
		.rx_flow_ctrl_thresh = UART_FIFO_LEN - 1,
		.source_clk = UART_SCLK_APB,
	};

	LOG(I,"Tx: GPIO%d Rx: GPIO%d RTS: GPIO%d CTS: GPIO%d", tx_pin, rx_pin, rts_pin, cts_pin);

	ZERO_CHECK(uart_param_config(port, &uart_config));
	ZERO_CHECK(uart_set_pin(port, tx_pin, rx_pin, rts_pin, cts_pin));
	ZERO_CHECK(uart_driver_install(port, RX_BUF_SIZE, 0, 0, NULL, 0));

	return 0;
}

#endif // MODEM_UART
