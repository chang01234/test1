/*! \file encapsulation.h
	\brief Encapsulation layer header.

	Definitions and functions relating to UART encapsulation of frames.
*/

#ifndef ENCAPSULATION_H_
#define ENCAPSULATION_H_

//! \~ Escape byte for byte stuffing of encapsulation frame
#define DDMP_ESCAPE	0xe0
//! \~ Sync byte for encapsulation frame
#define DDMP_SYNC	0xe1
//! \~ End byte for encapsulation frame
#define DDMP_END	0xe2

//! \~ Size of incoming UART frame buffer
#define UART_BUFFER_SIZE 128

size_t ddmp_receive_uart(const int uart, const void* data, size_t size, const int first_uart_interface);

#endif /* ENCAPSULATION_H_ */
