/*! \file udp_logger.h
	\brief Remote logger

	Logger that sends data to a remote host.
 */

#ifndef UDP_LOGGER_H_
#define UDP_LOGGER_H_

#include "configuration.h"
#include "lwip/sockets.h"

extern int udp_logger_registered;

typedef enum
{
	UL_LIN_0_RX	= 0x10,
	UL_LIN_0_TX	= 0x11,
	UL_LIN_1_RX	= 0x12,
	UL_LIN_1_TX	= 0x13,
	UL_LIN_2_RX	= 0x14,
	UL_LIN_2_TX	= 0x15,
	UL_CAN_RX	= 0x20,
	UL_CAN_TX	= 0x21,
	UL_DCP_RX	= 0x30,
	UL_DCP_TX	= 0x31,
} UL_SOURCE_ENUM;

#ifdef UDP_LOGGER
void ul_log_data(UL_SOURCE_ENUM source, const void *data, size_t datalen);
#else
#define ul_log_data(source, data, len) do { } while (0)
#endif

void initialize_udp_logger(void);

int udp_printf(const char *fmt, ...);
void register_addr_udp_logger (int socket, struct sockaddr_in *addr);

#endif /* UDP_LOGGER_H_ */
