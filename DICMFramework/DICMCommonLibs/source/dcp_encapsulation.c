/*! \file dcp_encapsulation.c
	\brief Encapsulation layer source.

	Definitions and functions relating to UART encapsulation of frames.
*/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "dcp_encapsulation.h"
#include "crc16.h"

//! \~ Incoming UART frame buffer
//static uint8_t uart_buffer[UART_BUFFER_SIZE];
static uint8_t *uart_buffer;

//! \~ Position in UART buffer
static size_t buffer_pos = 0;


int dcp_encapsulation_init(void)
{
	uart_buffer = malloc(UART_BUFFER_SIZE);

	crc16_init();

	return !!uart_buffer;
}

/*! \brief Bytestuff DDMP parameter for UART transmission
	\param destination Destination encapsulation frame data buffer
	\param data Pointer to source data to bytestuff
	\param size Size of data
	\return Size of bytestuffed frame
*/
size_t dcp_stuff(void* destination, const void *data, size_t size)
{
	uint8_t *dst = destination;
	const uint8_t *src = data;
	uint8_t *const crc_start = (uint8_t *const)dst + 2;
	uint8_t *const datalen_pos = (uint8_t* const)dst + 3;
	size_t outsize = 7;
	uint8_t datalen = 0;
	uint16_t crc;

	if ((!dst) || (!src))
		return 0;

	*dst++ = DCP_SYNC;
	*dst++ = DCP_SYNC;
	*dst++ = DCP_CMD;
	dst++;
	
	while (size--)
	{
		switch (*src)
		{
		case DCP_ESCAPE:
		case DCP_SYNC:
		case DCP_END:
			*dst++ = DCP_ESCAPE;
			datalen++;
			*dst++ = *src++;
			break;
		default:
			*dst++ = *src++;
			break;
		}
		datalen++;
	}

	*datalen_pos = datalen;
	outsize += datalen;

	crc = crctable(crc_start, datalen+2);
	*dst++ = crc >> 8;
	*dst++ = crc & 0xff;
	*dst++ = DCP_END;

	return outsize;
}

/*! \brief unbytestuff encapsulation frame
	\param destination Destination frame buffer
	\param data Pointer to source data to unbytestuff
	\return Size of output frame
*/
size_t dcp_unstuff(void* destination, const void *data)
{
	uint8_t *dst = destination;
	const uint8_t *src = data;
	size_t outsize = 0;
	uint8_t length;

	if ((!dst) || (!src))
		return 0;

	if (*src++ != DCP_SYNC)
		return 0;

	if (*src++ != DCP_SYNC)
        return 0;

    src++; // DCP command
    length = *src++; // DCP length

	while ((length>0) && (outsize < DCP_FRAME_SIZE_MAX))
	{
		switch (*src)
		{
		case DCP_ESCAPE:
			src++;
			*dst++ = *src++;
			break;
		default:
			*dst++ = *src++;
			break;
		}
		outsize++;
		length--;
	}
	return outsize;
}

/*! \brief Extract frame from buffer
	\param sync Pointer to sync byte of encapsulation frame
	\param end Pointer to end byte of encapsulation frame
	\param msg_cb UART message callback
*/
void dcp_extract_frame(const uint8_t *sync, const uint8_t *end, uart_msg_cb_t msg_cb)
{
	uint16_t crc;
	uint8_t size;
	char buffer[80];

	if ((end - sync) < 7)	//minimum 0 data bytes
	{
		return;
	}
	crc = crctable(sync + 2, ((*(sync+3))+2));
	if (((uint8_t)crc != *(end - 1)) && ((uint8_t)(crc >> 8) != *(end - 2)))
    {
		return;
	}
	size = (uint8_t)dcp_unstuff(&buffer, sync);
	if (msg_cb)
	{
		buffer[size] = '\0';
		msg_cb(buffer, size);
	}
}

/*! \brief Receive DDMP data bytes from UART
	\param data Pointer to data to input
	\param size Size of data to input
	\param msg_cb UART message callback
	\return Amount of data left in uart buffer
	\ingroup Interface
*/
size_t dcp_receive_uart(const void* data, size_t size, uart_msg_cb_t msg_cb)
{
	const uint8_t *src = data;
	const uint8_t *parse = &uart_buffer[buffer_pos];
	const uint8_t *end;
	static const uint8_t *sync = NULL;

	if (size > (UART_BUFFER_SIZE - buffer_pos))
	{
		buffer_pos = 0;
		sync = NULL;
	}

	if (size > UART_BUFFER_SIZE)
	{
		return 0;
	}

	memcpy(&uart_buffer[buffer_pos], src, size);
	buffer_pos += size;

	while (size--)
	{
		if (sync)
		{
			switch (*parse)
			{
			case DCP_SYNC:
				// One more SYNC is ok
				parse++;
				continue;
			case DCP_END:
				end = parse++;
				dcp_extract_frame(sync, end, msg_cb);
				sync = NULL;
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
			case DCP_SYNC:
				sync = parse++;
				continue;
			case DCP_END:
				//DDMP_ERROR(DDMP_ERROR_E_WITHOUT_S);
				parse++;
				continue;
			default:
				parse++;
				break;
			}
		}
	}

	if (sync)
	{
		if (sync != uart_buffer)
		{
			buffer_pos = parse - sync;
			memmove(uart_buffer, sync, buffer_pos);
			sync = uart_buffer;
		}
	}
	else
	{
		buffer_pos = 0;
	}

	return buffer_pos;
}

