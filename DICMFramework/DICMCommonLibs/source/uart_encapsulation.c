/*! \file uart_encapsulation.c
    \brief Encapsulation layer source.

    Definitions and functions relating to UART encapsulation of frames.

    2020-07-13 - Fixed parse position when fragmented frame is received (JB)
    2020-09-27 - Moved to shared library directory (JB)
    2020-11-21 - Fixed buffer overflow in ddmp2_extract_frame (JB)
    2021-05-11 - Merged (SH)
    2024-12-17 - Fixed size check in ddmp2_uart_byteunstuff() (JB)
    2025-10-29 - Added ddmp2_uart_status_string() (JB)
*/

#ifdef __IAR_SYSTEMS_ICC__
#include "stm8s.h"
#else
#include <stdint.h>
#endif

#include "ddm2.h"
#include "uart_encapsulation.h"
#include <string.h>

#ifdef CRC16_TABLE
// clang-format off
static const uint16_t Crc16_table[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};
// clang-format on

uint16_t crc16(const void *data, size_t size)
{
    const uint8_t *src = data;
    uint16_t crc = XMODEM_CRC16_INITIAL;
    int entry;

    while (size--)
    {
        entry = (crc >> 8) ^ *src++;
        crc = (crc << 8) ^ Crc16_table[entry];
    }

    return crc;
}
#else
uint16_t crc16(const void *data, size_t size)
{
    const uint8_t *src = data;
    uint16_t crc = XMODEM_CRC16_INITIAL;

    while (size--)
    {
        crc = crc ^ *src++ << 8;
        for (int i = 0; i < 8; i++)
        {
            if (crc & 0x8000)
            {
                crc = crc << 1 ^ XMODEM_CRC16_POLY;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }

    return crc;
}
#endif

/*! \brief Get string representation of DDMP2 UART encapsulation status code
    \param status Status code
    \return Pointer to string representation
*/
const char *ddmp2_uart_status_string(const DDMP2_ENCAPSULATION_STATUS_ENUM status)
{
    switch (status)
    {
    case DDMP2_ENCAPSULATION_NO_FRAME:
        return "Buffer processed, but no frame yet";
    case DDMP2_ENCAPSULATION_FRAME:
        return "A complete frame received, frame is valid";
    case DDMP2_ENCAPSULATION_ERROR_S_WITHOUT_E:
        return "Sync byte encountered, expected end of previous frame";
    case DDMP2_ENCAPSULATION_ERROR_E_WITHOUT_S:
        return "End byte encountered, but no frame started";
    case DDMP2_ENCAPSULATION_ERROR_BUFFER_OVERFLOW:
        return "UART buffer size exceeded";
    case DDMP2_ENCAPSULATION_ERROR_CRC:
        return "CRC calculation does not agree with supplied signature";
    case DDMP2_ENCAPSULATION_ERROR_SHORT_FRAME:
        return "Enc Frame too short to be valid (<4 bytes)";
    case DDMP2_ENCAPSULATION_ERROR_FRAME_SIZE:
        return "DDMP2 frame has an invalid size";
    case DDMP2_ENCAPSULATION_ERROR_UNPACK:
        return "Unpack of frame was unsuccessful";
    case DDMP2_ENCAPSULATION_ERROR_BUFFER_TOO_SMALL:
        return "Frame too big for UART buffer";
    case DDMP2_ENCAPSULATION_ERROR_ARGUMENT:
        return "Illegal argument to function";
    default:
        return "Unknown status code";
    }
}

/*! \brief Bytestuff bytes
    \param dst Destination encapsulation frame data buffer
    \param src Pointer to source data to bytestuff
    \param size Size of data
    \return Size of bytestuffed frame
*/
static size_t bytestuff(uint8_t *dst, const uint8_t *src, size_t size)
{
    size_t outsize = 0;

    while (size--)
    {
        switch (*src)
        {
        case DDMP2_UART_ESCAPE:
        case DDMP2_UART_SYNC:
        case DDMP2_UART_END:
            *dst++ = DDMP2_UART_ESCAPE;
            outsize++;
            *dst++ = *src++ - DDMP2_UART_ESCAPE;
            break;
        default:
            *dst++ = *src++;
            break;
        }
        outsize++;
    }

    return outsize;
}

/*! \brief Bytestuff DDMP2 frame data for UART transmission
    \param destination Destination encapsulation frame data buffer
    \param data Pointer to source data to bytestuff
    \param data_size Size of data
    \return Size of bytestuffed frame
*/
size_t ddmp2_uart_bytestuff(void *destination, const void *data, const size_t data_size)
{
    uint8_t *dst = destination;
    const uint8_t *src = data;
    size_t outsize = 0;
    uint16_t crc;
    size_t data_stuffsize, crc_stuffsize;

    if (!dst || !src || !data_size)
    {
        return 0;
    }

    crc = crc16(data, data_size);  // calculate CRC16 of data before stuffing

    if (IS_BIG_ENDIAN)
    {
        crc = SWAP2(crc);
    }

    *dst++ = DDMP2_UART_SYNC;
    outsize++;

    data_stuffsize = bytestuff(dst, src, data_size);
    dst += data_stuffsize;
    outsize += data_stuffsize;

    crc_stuffsize = bytestuff(dst, (uint8_t *)&crc, sizeof(crc));
    dst += crc_stuffsize;
    outsize += crc_stuffsize;

    *dst++ = DDMP2_UART_END;
    outsize++;

    return outsize;
}

/*! \brief Bytestuff DDMP2 frame for UART transmission
    \param destination Destination encapsulation frame data buffer
    \param frame DDMP2 frame to encapsulate
    \return Size of bytestuffed frame
*/
size_t ddmp2_uart_bytestuff_frame(void *destination, const DDMP2_FRAME *frame)
{
    return ddmp2_uart_bytestuff(destination, &frame->frame, frame->frame_size);
}

/*! \brief unbytestuff encapsulation frame
    \param destination Destination frame buffer
    \param data Pointer to source data to unbytestuff
    \return Size of output frame
*/
static size_t ddmp2_uart_byteunstuff(void *destination, const void *data)
{
    uint8_t *dst = destination;
    const uint8_t *src = data;
    size_t outsize = 0;

    if (!dst || !src)
    {
        return 0;
    }

    if (*src++ != DDMP2_UART_SYNC)
    {
        return 0;
    }

    while ((*src != DDMP2_UART_END) && (outsize < (DDMP2_MAX_FRAME_SIZE + 2)))  //+2 = CRC
    {
        switch (*src)
        {
        case DDMP2_UART_ESCAPE:
            src++;
            *dst++ = DDMP2_UART_ESCAPE + *src++;
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
    \param pframe Destination frame
    \param sync Pointer to sync byte of encapsulation frame
    \param end Pointer to end byte of encapsulation frame
    \param connector Connector ID to tag frame with
*/
static DDMP2_ENCAPSULATION_STATUS_ENUM ddmp2_extract_frame(DDMP2_FRAME *pframe, const uint8_t *sync, const uint8_t *end, const uint8_t connector)
{
    uint16_t calculated_crc, supplied_crc;
    DDMP2_UART_FRAME_BUFFER buffer;
    size_t unstuffed_size;
    uint8_t frame_size;

    if ((end - sync) < 4)  // e1 + data + CRC(2) + e2
    {
        return DDMP2_ENCAPSULATION_ERROR_SHORT_FRAME;
    }

    unstuffed_size = ddmp2_uart_byteunstuff(&buffer, sync);
    frame_size = (uint8_t)unstuffed_size - 2;

    calculated_crc = crc16(buffer, frame_size);
    supplied_crc = *(uint16_t *)(buffer + frame_size);

    if (IS_BIG_ENDIAN)
    {
        calculated_crc = SWAP2(calculated_crc);
    }

    if (calculated_crc != supplied_crc)
    {
        return DDMP2_ENCAPSULATION_ERROR_CRC;
    }

    pframe->frame_size = frame_size;

    if (!ddmp2_create_raw_frame(pframe, buffer, pframe->frame_size, connector))
    {
        return DDMP2_ENCAPSULATION_ERROR_UNPACK;
    }

    return DDMP2_ENCAPSULATION_FRAME;
}

/*! \brief Receive DDMP data bytes from UART
    \param pframe Output frame
    \param uart_buffer UART working buffer
    \param uart_buffer_size UART working buffer size in bytes
    \param data Pointer to data to input
    \param data_size Size of data to input
    \param connector Connector ID to tag frame with
    \return Amount of data left in uart buffer
    \ingroup Interface
*/
DDMP2_ENCAPSULATION_STATUS_ENUM ddmp2_uart_receive(DDMP2_FRAME *pframe, uint8_t *uart_buffer, const size_t uart_buffer_size, const void *data, const size_t data_size, const uint8_t connector)
{
    DDMP2_ENCAPSULATION_STATUS_ENUM extract_result;
    const uint8_t *src = data;
    static int uart_parse_pos = 0;
    static int uart_data_pos = 0;
    static int uart_sync_pos = -1;

    if (data_size > uart_buffer_size)
    {
        return DDMP2_ENCAPSULATION_ERROR_BUFFER_TOO_SMALL;
    }

    if (data_size > (uart_buffer_size - uart_data_pos))
    {
        uart_data_pos = 0;  // reset buffer
        uart_parse_pos = 0;
        uart_sync_pos = -1;  // reset state

        return DDMP2_ENCAPSULATION_ERROR_BUFFER_OVERFLOW;
    }

    memmove(&uart_buffer[uart_data_pos], src, data_size);
    uart_data_pos += (int)data_size;

    while (uart_parse_pos < uart_data_pos)
    {
        if (uart_sync_pos >= 0)  // sync
        {
            switch (uart_buffer[uart_parse_pos])
            {
            case DDMP2_UART_SYNC:
                uart_sync_pos = uart_parse_pos++;
                return DDMP2_ENCAPSULATION_ERROR_S_WITHOUT_E;
            case DDMP2_UART_END:
                extract_result = ddmp2_extract_frame(pframe, &uart_buffer[uart_sync_pos], &uart_buffer[uart_parse_pos++], connector);
                uart_sync_pos = -1;
                return extract_result;
            default:
                uart_parse_pos++;
                break;
            }
        }
        else  // no sync
        {
            switch (uart_buffer[uart_parse_pos])
            {
            case DDMP2_UART_SYNC:
                uart_sync_pos = uart_parse_pos++;
                continue;
            case DDMP2_UART_END:
                uart_parse_pos++;
                return DDMP2_ENCAPSULATION_ERROR_E_WITHOUT_S;
            default:
                uart_parse_pos++;
                break;
            }
        }
    }

    if (uart_sync_pos >= 0)  // sync?
    {
        if (uart_sync_pos)  // yes, ensure sync is at start of buffer
        {
            uart_parse_pos = uart_data_pos -= uart_sync_pos;
            memmove(uart_buffer, &uart_buffer[uart_sync_pos], uart_data_pos);
            uart_sync_pos = 0;
        }
    }
    else
    {
        uart_data_pos = 0;  // throw away all data if not in sync
        uart_parse_pos = 0;
    }

    return DDMP2_ENCAPSULATION_NO_FRAME;
}
