/*! \file uart_encapsulation.h
    \brief Encapsulation layer header.

    Definitions and functions relating to UART encapsulation of frames.
*/

#ifndef UART_ENCAPSULATION_H_
#define UART_ENCAPSULATION_H_

#ifdef __IAR_SYSTEMS_ICC__
#include "stm8s.h"
#else
#include <stdint.h>
#endif

#include "ddm2.h"
#include <stddef.h>

#define CRC16_TABLE  //!< \~ Use table for CRC16 calculation

#define DDMP2_UART_ESCAPE 0xe0  //!< \~ Escape byte for byte stuffing of encapsulation frame
#define DDMP2_UART_SYNC   0xe1  //!< \~ Sync byte for encapsulation frame
#define DDMP2_UART_END    0xe2  //!< \~ End byte for encapsulation frame

#define XMODEM_CRC16_POLY    0x1021
#define XMODEM_CRC16_INITIAL 0x0000

#define LOWBYTE(w)  ((uint8_t)(((uint16_t)(w)) & 0xff))
#define HIGHBYTE(w) ((uint8_t)((((uint16_t)(w)) >> 8) & 0xff))

#define DDMP2_ENC_ERR_CLASS(x) ((x) & 0xf0)
#define DDMP2_ENC_OK           0
#define DDMP2_ENC_FRAME        0x10
#define DDMP2_ENC_WARNING      0x20
#define DDMP2_ENC_ERROR        0x30

//! \~ Buffer guaranteed to being able to hold a complete UART encapsulation frame (sync byte, 2xframe data, CRC, end byte)
typedef uint8_t DDMP2_UART_FRAME_BUFFER[1 + DDMP2_MAX_FRAME_SIZE * 2 + 2 + 1];

//! \~ DDMP2 uart encapsulation errors
typedef enum DDMP2_ENCAPSULATION_STATUS_ENUM
{
    DDMP2_ENCAPSULATION_NO_FRAME = 0x00,                //!< \~ Buffer processed, but no frame yet
    DDMP2_ENCAPSULATION_FRAME = 0x10,                   //!< \~ A complete frame received, frame is valid
    DDMP2_ENCAPSULATION_ERROR_S_WITHOUT_E = 0x20,       //!< \~ Sync byte encountered, expected end of previous frame
    DDMP2_ENCAPSULATION_ERROR_E_WITHOUT_S,              //!< \~ End byte encountered, but no frame started
    DDMP2_ENCAPSULATION_ERROR_BUFFER_OVERFLOW,          //!< \~ UART buffer size exceeded
    DDMP2_ENCAPSULATION_ERROR_CRC,                      //!< \~ CRC calculation does not agree with supplied signature
    DDMP2_ENCAPSULATION_ERROR_SHORT_FRAME,              //!< \~ Enc Frame too short to be valid (<4 bytes)
    DDMP2_ENCAPSULATION_ERROR_FRAME_SIZE,               //!< \~ DDMP2 frame has an invalid size
    DDMP2_ENCAPSULATION_ERROR_UNPACK,                   //!< \~ Unpack of frame was unsuccessful
    DDMP2_ENCAPSULATION_ERROR_BUFFER_TOO_SMALL = 0x30,  //!< \~ Frame too big for UART buffer
    DDMP2_ENCAPSULATION_ERROR_ARGUMENT,                 //!< \~ Illegal argument to function
} DDMP2_ENCAPSULATION_STATUS_ENUM;

const char *ddmp2_uart_status_string(const DDMP2_ENCAPSULATION_STATUS_ENUM status);
size_t ddmp2_uart_bytestuff_frame(void *destination, const DDMP2_FRAME *frame);
size_t ddmp2_uart_bytestuff(void *destination, const void *data, const size_t data_size);
DDMP2_ENCAPSULATION_STATUS_ENUM ddmp2_uart_receive(DDMP2_FRAME *pframe, uint8_t *uart_buffer, const size_t uart_buffer_size, const void *data, const size_t data_size, const uint8_t connector);

#endif /* UART_ENCAPSULATION_H_ */
