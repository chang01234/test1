/*! \file hal_uart_interface.h
 *  \brief UART Hardware Abstraction Layer
 */
#ifndef HAL_UART_MODEM_H_
#define HAL_UART_MODEM_H_
//-----------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>

//-----------------------------------------------------------------
#include "dicm_framework_config.h"
#ifdef MODEM_UART

/*! \brief Initialize UART
 *	\param uart_port UART port number
 *	\param tx_pin UART TX pin on host
 *	\param rx_pin UART RX pin on host
 *	\param baudrate to set speed of UART port
 *  \return 0 if successful
 */
int hal_uart_init(void *uart_port, const int tx_pin, const int rx_pin, const int rts_pin, const int cts_pin, int baudrate);

/*! \brief Write data to UART device
 *	\param uart_port UART port number
 *  \param data_wr Data to send
 *  \param lenght Size of data to send 
 *  \return 0 if successful
 */
int hal_uart_write (void *uart_port,  const char *data_wr, const size_t length);

/*! \brief Read data from UART device
 *	\param uart_port UART port number
 *  \param data_rd Data to read
 *  \param length Size of data to be read
 *  \return number of bytes read from RX_FIFO
 */
int hal_uart_read (void *uart_port, char *const data_rd, size_t length);

#endif // MODEM_UART
#endif // HAL_UART_MODEM_H_

