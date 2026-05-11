/*****************************************************************************
 * \file       hal_uart.h
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
#ifndef HAL_UART_H_
#define HAL_UART_H_

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "hal_types.h"

/*****************************************************************************
 * MODEM Implementation
 *****************************************************************************/
#ifdef MODEM_UART
#include "hal_uart_modem.h"
#else

/*****************************************************************************
 * Generic Implementation
 *****************************************************************************/
/*! \brief Initialize UART
 *	\param uart_port UART port number
 *	\param tx_pin UART TX pin on host
 *	\param rx_pin UART RX pin on host
 *	\param baudrate to set speed of UART port
 *  \return 0 if successful
 */
int hal_uart_init(int uart_port, int tx_pin, int rx_pin, int baudrate, int rx_fifo, int tx_fifo);

/*! \brief Write data to UART device
 *	\param uart_port UART port number
 *  \param data_wr Data to send
 *  \param lenght Size of data to send 
 *  \return 0 if successful
 */
int hal_uart_write(int uart_port,  const char *data_wr, const size_t length);

/*! \brief Read data from UART device
 *	\param uart_port UART port number
 *  \param data_rd Data to read
 *  \param length Size of data to be read
 *  \return number of bytes read from RX_FIFO
 */
int hal_uart_read(int uart_port, char *const data_rd, size_t length, uint16_t timeout_ticks);

/*! \brief Wait for UART device to complte transmission
 *	\param uart_port UART port number
 *  \return 0 if successful
 */
int hal_uart_wait_tx(int uart_port);

/*! \brief Acquire exclusive access to the port (when shared)
 *  \return 0 = success
 */
int hal_uart_acquire(int uart_port);

/*! \brief Release exclusive access to the port (when shared)
 *  \return 0 = success
 */
void hal_uart_release(int uart_port);


/*****************************************************************************/
#endif // HAL_UART_H_
#endif // HAL_UART_H_ 

