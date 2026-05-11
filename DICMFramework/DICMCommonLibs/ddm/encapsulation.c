/*! \file encapsulation.c
	\brief Encapsulation layer source.

	Definitions and functions relating to UART encapsulation of frames.
*/

#include <stdint.h>
#include <string.h>

#include "encapsulation.h"
#include "ddm.h"

extern DDMP_SEND_ERROR_ENUM _ddmp_enqueue(int intf, DDMP_QUEUE_ENUM q, const DDMP_FRAME *frame);	//ddm.c

//! \~ Incoming UART frame buffer
static uint8_t uart_buffer[DDMP_UART_INTERFACE_COUNT][UART_BUFFER_SIZE];

//! \~ Position in UART buffer
static size_t buffer_pos[DDMP_INTERFACE_COUNT] = { 0 };

//! \~ CRC7 lookup table
static const uint8_t Crc7_table[256] =
{
	0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
	0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
	0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
	0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
	0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
	0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
	0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
	0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
	0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
	0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
	0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
	0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
	0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
	0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
	0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
	0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
	0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
	0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
	0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
	0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
	0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
	0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
	0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
	0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
	0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
	0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
	0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
	0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
	0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
	0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
	0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
	0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79,
};

/*! \brief Calculate CRC7 signature
	\param data Pointer to data
	\param size Size of data
	\param val Initial value of CRC (0)
	\return CRC7 of data
*/
uint8_t _crc7(const void *data, size_t size, uint8_t val)
{
	const uint8_t *src = data;

	while (size--)
	{
		val = Crc7_table[val ^ *src++];
	}

	return val;
}

/*! \brief Bytestuff DDMP parameter for UART transmission
	\param destination Destination encapsulation frame data buffer
	\param data Pointer to source data to bytestuff
	\param size Size of data
	\return Size of bytestuffed frame
*/
size_t _stuff(void* destination, const void *data, size_t size)
{
	uint8_t *dst = destination;
	const uint8_t *src = data;
	size_t outsize = 0;

	if ((!dst) || (!src))
		return 0;

	*dst++ = DDMP_SYNC;
	outsize++;

	while (size--)
	{
		switch (*src)
		{
		case DDMP_ESCAPE:
		case DDMP_SYNC:
		case DDMP_END:
			*dst++ = DDMP_ESCAPE;
			outsize++;
			*dst++ = *src++ - DDMP_ESCAPE;
			break;
		default:
			*dst++ = *src++;
			break;
		}
		outsize++;
	}

	*dst++ = _crc7((uint8_t*)destination + 1, outsize - 1, 0);
	outsize++;

	*dst++ = DDMP_END;
	outsize++;

	return outsize;
}

/*! \brief unbytestuff encapsulation frame
	\param destination Destination frame buffer
	\param data Pointer to source data to unbytestuff
	\return Size of output frame
*/
size_t _unstuff(void* destination, const void *data)
{
	uint8_t *dst = destination;
	const uint8_t *src = data;
	size_t outsize = 0;

	if ((!dst) || (!src))
		return 0;

	if (*src++ != DDMP_SYNC)
		return 0;

	while ((*(src + 1) != DDMP_END) && (outsize < DDMP_FRAME_SIZE_MAX))
	{
		switch (*src)
		{
		case DDMP_ESCAPE:
			src++;
			*dst++ = DDMP_ESCAPE + *src++;
			break;
		default:
			*dst++ = *src++;
			break;
		}
		outsize++;
	}
	return outsize;
}

/*! \brief Extract frame from buffer
	\param intf Interface
	\param sync Pointer to sync byte of encapsulation frame
	\param end Pointer to end byte of encapsulation frame
*/
void _extract_frame(int intf, const uint8_t *sync, const uint8_t *end)
{
	uint8_t crc;
	DDMP_FRAME frame;
	DDMP_FRAME_BUFFER buffer; 

	if ((end - sync) < 3)	//e1 + data + CRC + e2
	{
		_ddmp_enqueue(intf, DDMP_REPLY_QUEUE, &Ddmp_nak_frame);
		DDMP_ERROR(DDMP_ERROR_SHORT_FRAME);
		return;
	}
	crc = _crc7(sync + 1, end - sync - 2, 0);
	if (crc != *(end - 1))
	{
		_ddmp_enqueue(intf, DDMP_REPLY_QUEUE, &Ddmp_nak_frame);
		DDMP_ERROR(DDMP_ERROR_CRC);
		return;
	}
	frame.size = (uint8_t)_unstuff(&buffer, sync);
	ddmp_unpack(&frame, buffer, frame.size);
	ddmp_incoming_frame(intf, &frame);
}

/*! \brief Receive DDMP data bytes from UART
	\param uart Originating UART ID
	\param data Pointer to data to input
	\param size Size of data to input
	\param first_uart_interface Interface ID of first UART
	\return Amount of data left in uart buffer
	\ingroup Interface
*/
size_t ddmp_receive_uart(const int uart, const void* data, size_t size, const int first_uart_interface)
{
	const uint8_t *src = data;
	const uint8_t *parse = &uart_buffer[uart][buffer_pos[uart]];
	const uint8_t *end;
	static const uint8_t *sync[DDMP_UART_INTERFACE_COUNT] = {0};
	int intf = first_uart_interface + uart;

	if (size > (UART_BUFFER_SIZE - buffer_pos[uart]))
	{
		DDMP_ERROR(DDMP_ERROR_BUFFER_OVERFLOW);
		buffer_pos[uart] = 0;
		sync[uart] = NULL;
	}

	if (size > UART_BUFFER_SIZE)
	{
		DDMP_ERROR(DDMP_ERROR_BUFFER_TOO_SMALL);
		return 0;
	}

	memcpy(&uart_buffer[uart][buffer_pos[uart]], src, size);
	buffer_pos[uart] += size;

	while (size--)
	{
		if (sync[uart])
		{
			switch (*parse)
			{
			case DDMP_SYNC:
				DDMP_ERROR(DDMP_ERROR_S_WITHOUT_E);
				sync[uart] = parse++;
				continue;
			case DDMP_END:
				end = parse++;
				_extract_frame(intf, sync[uart], end);
				sync[uart] = NULL;
				continue;
			default:
				parse++;
				break;
			}
		}
		else
		{
			switch (*parse)
			{
			case DDMP_SYNC:
				sync[uart] = parse++;
				continue;
			case DDMP_END:
				DDMP_ERROR(DDMP_ERROR_E_WITHOUT_S);
				parse++;
				continue;
			default:
				parse++;
				break;
			}
		}
	}

	if (sync[uart])
	{
		if (sync[uart] != uart_buffer[uart])
		{
			buffer_pos[uart] = parse - sync[uart];
			memmove(uart_buffer[uart], sync[uart], buffer_pos[uart]);
			sync[uart] = uart_buffer[uart];
		}
	}
	else
	{
		buffer_pos[uart] = 0;
	}

	return buffer_pos[uart];
}
