/*! \file dcp_encapsulation.h
	\brief Encapsulation layer header.

	Definitions and functions relating to UART encapsulation of frames for the DCP.
*/

#ifndef ENCAPSULATION_H_
#define ENCAPSULATION_H_

#define STX    (0x02)
#define ETX    (0x03)
#define DLE    (0x10)
#define CMD    (0xFF)
#define ACK    (0x06)
#define NACK   (0x15)

//! \~ Escape byte for byte stuffing of encapsulation frame
#define DCP_ESCAPE	DLE
//! \~ Sync byte for encapsulation frame
#define DCP_SYNC	STX
//! \~ End byte for encapsulation frame
#define DCP_END     ETX

#define DCP_CMD     CMD
#define DCP_FRAME_SIZE_MAX 256

//! \~ Size of incoming UART frame buffer
#define UART_BUFFER_SIZE 512

typedef void (*uart_msg_cb_t)(const char *data, uint8_t length);

int dcp_encapsulation_init(void);
size_t dcp_stuff(void* destination, const void* data, size_t size);
size_t dcp_receive_uart(const void* data, size_t size, uart_msg_cb_t msg_cb);

#endif /* ENCAPSULATION_H_ */

