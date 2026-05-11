/**
 * @file lin_server_uart.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server UART implementation
 * @date 2023-12-28
 */

#ifndef LIN_SERVER_UART__
#define LIN_SERVER_UART__

/**
 * @brief Lin server UART initialization
 */
void lin_server_uart_init(void);

/**
 * @brief Lin server UART task start
 */
void lin_server_uart_start(void);

#endif //LIN_SERVER_UART__
