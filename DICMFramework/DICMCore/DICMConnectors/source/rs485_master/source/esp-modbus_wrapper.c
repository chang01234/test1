/*! \file esp-modbus_wrapper.c
    \brief Modbus RTU Frame Header Wrapper
 * This file implements wrapper functions for mb_port_ser_send_data and mb_port_ser_recv_data
 * to add/remove 0xFA frame header for all RTU data frames.
*/

#include "configuration.h"
// System includes
#include <string.h>
// IDF includes
#include "esp_log.h"
#include "port_common.h"

#define MB_FRAME_HEADER       0xFA  // Topband battery frame header byte
#define MB_FRAME_HEADER_SIZE  1     // Frame header size in byte
#define MB_RTU_MIN_FRAME_SIZE 5     // [head(1)][addr(1)][func code(1)][data(0+)][CRC16(2)] = 5 bytes minimum
#define MB_MAX_FRAME_SIZE     256   // RTU message max length
#define MB_CRC16_SIZE         2     // CRC16 size in bytes

// Module static variables
static EXT_RAM_ATTR uint8_t recv_frame_buffer[MB_MAX_FRAME_SIZE];

// Original function declarations
extern bool __real_mb_port_ser_send_data(mb_port_base_t *inst, const uint8_t *ser_frame, uint16_t ser_length);
extern bool __real_mb_port_ser_recv_data(mb_port_base_t *inst, uint8_t **ser_frame, uint16_t *p_ser_length);

/* Modbus RTU CRC16 lookup table (CRC-16-IBM reflected, polynomial 0xA001) */
static const uint16_t mb_crc_tab[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};

/**
 * @brief Calculate Modbus RTU CRC16 (CRC-16-IBM, polynomial 0x8005)
 *   - Parameter model: Init=0xFFFF, RefIn=true, RefOut=true, XorOut=0x0000.
 *   - The returned 16-bit value matches Modbus RTU byte order on the wire
 * @param frame_ptr Pointer to frame data
 * @param len_buf   Length of frame data
 * @return uint16_t Calculated CRC16 value
 */
static uint16_t mb_crc16_calculate(const uint8_t *frame_ptr, uint16_t len_buf)
{
    uint16_t crc = 0xFFFF;
    while (len_buf--)
    {
        uint8_t idx = (uint8_t)((crc ^ *frame_ptr++) & 0xFF);
        crc = (uint16_t)((crc >> 8) ^ mb_crc_tab[idx]);
    }
    return crc;
}

/**
 * @brief Verify CRC16 of a Modbus RTU frame
 * @param frame Pointer to frame data (including CRC16 at the end)
 * @param length Total frame length including CRC16
 * @return true if CRC16 is valid, false otherwise
 */
static bool mb_crc16_verify(const uint8_t *frame, uint16_t length)
{
    if (frame == NULL || length < MB_CRC16_SIZE)
    {
        return false;
    }

    // Calculate CRC16 for frame excluding the last 2 bytes (CRC16 itself)
    uint16_t calculated_crc = mb_crc16_calculate(frame, length - MB_CRC16_SIZE);

    // Extract CRC16 from frame (low byte first, then high byte)
    uint16_t frame_crc = (uint16_t)frame[length - 2] | ((uint16_t)frame[length - 1] << 8);

    // CRC16 verification: calculated CRC should match frame CRC
    return (calculated_crc == frame_crc);
}

/**
 * @brief Wrapper for mb_port_ser_send_data - adds 0xFA frame header
 * This function is called instead of the original mb_port_ser_send_data.
 * It prepends 0xFA to the frame before sending.
 * @param inst Pointer to port instance
 * @param ser_frame Pointer to serial frame data (without header)
 * @param ser_length Length of serial frame data
 * @return true if successful, false otherwise
 */
__attribute__((used, visibility("default"))) bool __wrap_mb_port_ser_send_data(mb_port_base_t *inst, const uint8_t *ser_frame, uint16_t ser_length)
{
    if (ser_frame == NULL || ser_length == 0)
    {
        LOG(E, "Invalid parameters: ser_frame=%p, ser_length=%d", ser_frame, ser_length);
        return false;
    }

    uint8_t send_frame_buffer[MB_MAX_FRAME_SIZE] = {0};

    // Check if frame already has header (avoid double header)
    if (ser_frame[0] == MB_FRAME_HEADER)
    {
        // Frame already has header, send as-is using original function
        LOG(W, "Frame already has header 0xFA, sending as-is");
        return __real_mb_port_ser_send_data(inst, ser_frame, ser_length);
    }

    // Validate frame size (must have at least address + function code + CRC16)
    if (ser_length < (MB_RTU_MIN_FRAME_SIZE - MB_FRAME_HEADER_SIZE))
    {
        LOG(E, "Frame too small: %d bytes (min %d)", ser_length, (MB_RTU_MIN_FRAME_SIZE - MB_FRAME_HEADER_SIZE));
        return false;
    }

    // Check buffer size (original frame + header + CRC16 will be recalculated)
    if (ser_length >= MB_MAX_FRAME_SIZE)
    {
        LOG(E, "Frame too large: %d bytes (max %d)", ser_length, MB_MAX_FRAME_SIZE - MB_FRAME_HEADER_SIZE);
        return false;
    }

    // Build new frame with 0xFA header
    uint16_t new_crc = 0;
    uint16_t new_frame_length = 0;
    /* Address(decimal)   Content                      Size   Unit Remark
     *  30206(hex:0x75FE)  Design capacity 4 Bytes mAh
     *  30292(hex:0x7654)  Charging voltage value       2 Bytes 0.1V
     *  30293(hex:0x7655)  Charging current limit value 2 Bytes 0.1A unsigned
     *  When the register address in the request frame is 0x75FE or 0x7654 or 0x7655,
     *  the system information parameters are sent.
     */
    if ((ser_frame[2] == 0x75 && (ser_frame[3] == 0xFE)) || (ser_frame[2] == 0x76 && (ser_frame[3] == 0x54 || ser_frame[3] == 0x55)))
    {
        LOG(D, "Sending System information parameters:%02X", ser_frame[3]);
        send_frame_buffer[0] = MB_FRAME_HEADER;
        send_frame_buffer[1] = 0xF2;          // Fixed slave address for system info
        send_frame_buffer[2] = 0x15;          // Fixed function code for system info
        send_frame_buffer[3] = ser_frame[2];  // Register addr-hi from original frame
        send_frame_buffer[4] = ser_frame[3];  // Register addr-low from original frame
        send_frame_buffer[5] = ser_frame[0];  // Slave addr from original frame
        send_frame_buffer[6] = ser_frame[4];  // Quantity-hi from original frame
        send_frame_buffer[7] = ser_frame[5];  // Quantity-low from original frame
        new_crc = mb_crc16_calculate(send_frame_buffer, 8);
        new_frame_length = 10;
        send_frame_buffer[new_frame_length - 2] = (uint8_t)(new_crc & 0xFF);         // Low byte
        send_frame_buffer[new_frame_length - 1] = (uint8_t)((new_crc >> 8) & 0xFF);  // High byte
    }
    else
    {
        // Format: [0xFA][addr][func code][data...]
        send_frame_buffer[0] = MB_FRAME_HEADER;
        memcpy(&send_frame_buffer[1], ser_frame, ser_length - MB_CRC16_SIZE);  // Copy frame without old CRC16
        if (send_frame_buffer[2] == 0x02)                                      // Function code 0x02 (Read Discrete Inputs)
        {
            // Topband batteries read 1, will return 2 bytes of data
            // Do not match the standard Modbus behavior, so we force quantity to 1
            send_frame_buffer[6] = 0x01;  // Force quantity to 1
        }

        // Calculate new CRC16 covering [0xFA][addr][func code][data]
        new_crc = mb_crc16_calculate(send_frame_buffer, ser_length - MB_CRC16_SIZE + MB_FRAME_HEADER_SIZE);

        // Append new CRC16 (low byte first, then high byte)
        new_frame_length = ser_length - MB_CRC16_SIZE + MB_FRAME_HEADER_SIZE + MB_CRC16_SIZE;
        send_frame_buffer[new_frame_length - 2] = (uint8_t)(new_crc & 0xFF);         // Low byte
        send_frame_buffer[new_frame_length - 1] = (uint8_t)((new_crc >> 8) & 0xFF);  // High byte
    }

    LOG(D, "Sending frame with header: length=%d (original=%d), new CRC16=0x%04X",
        new_frame_length, ser_length, new_crc);
    ESP_LOG_BUFFER_HEXDUMP("MODBUS_WRAPPER", send_frame_buffer, new_frame_length, ESP_LOG_DEBUG);

    // Call original function with frame including header and new CRC16
    bool result = __real_mb_port_ser_send_data(inst, send_frame_buffer, new_frame_length);

    if (!result)
    {
        LOG(E, "Failed to send frame with header");
    }

    return result;
}

/**
 * @brief Wrapper for mb_port_ser_recv_data - removes 0xFA frame header
 * This function is called instead of the original mb_port_ser_recv_data.
 * It removes the 0xFA header from received frames.
 * @param inst Pointer to port instance
 * @param ser_frame Pointer to pointer that will receive frame data (without header)
 * @param p_ser_length Pointer to length variable (will be updated to length without header)
 * @return true if successful, false otherwise
 */
__attribute__((used, visibility("default"))) bool __wrap_mb_port_ser_recv_data(mb_port_base_t *inst, uint8_t **ser_frame, uint16_t *p_ser_length)
{
    if (ser_frame == NULL || p_ser_length == NULL)
    {
        LOG(E, "Invalid parameters: ser_frame=%p, p_ser_length=%p", ser_frame, p_ser_length);
        return false;
    }
    // Call original function to receive frame (may include header)
    bool result = __real_mb_port_ser_recv_data(inst, ser_frame, p_ser_length);

    if (!result || *ser_frame == NULL || *p_ser_length == 0)
    {
        return result;
    }

    // Validate minimum frame size (must have header + address + function code + CRC16)
    if (*p_ser_length < (MB_RTU_MIN_FRAME_SIZE))
    {
        LOG(E, "Received frame too small: %d bytes (min %d)",
            *p_ser_length, MB_RTU_MIN_FRAME_SIZE);
        return false;
    }
    // 0xFA 0xF2 0x15 frame indicates system information response
    if ((*ser_frame)[0] == MB_FRAME_HEADER && (*ser_frame)[1] == 0xF2 && (*ser_frame)[2] == 0x15)
    {
        LOG(D, "Receiving System information parameters");
        // Verify CRC16 for system info frame
        if (!mb_crc16_verify(*ser_frame, *p_ser_length))
        {
            LOG(E, "CRC16 verification failed for received system info frame");
            ESP_LOG_BUFFER_HEXDUMP("MODBUS_WRAPPER", *ser_frame, *p_ser_length, ESP_LOG_ERROR);
            return false;
        }

        // Rebuild original frame format: [addr][func code][reg addr-hi][reg addr-lo][quantity-hi][quantity-lo]
        recv_frame_buffer[0] = (*ser_frame)[3];                            // Slave addr
        recv_frame_buffer[1] = 0x04;                                       // Original function code
        recv_frame_buffer[2] = (*ser_frame)[4];                            // Data length will be quantity * 2
        memcpy(&recv_frame_buffer[3], (*ser_frame) + 5, (*ser_frame)[4]);  // Data
        uint8_t len_temp = 3 + (*ser_frame)[4];
        uint16_t new_crc = mb_crc16_calculate(recv_frame_buffer, len_temp);
        recv_frame_buffer[len_temp] = (uint8_t)(new_crc & 0xFF);             // Low byte
        recv_frame_buffer[len_temp + 1] = (uint8_t)((new_crc >> 8) & 0xFF);  // High byte
        // Update length
        *p_ser_length = len_temp + MB_CRC16_SIZE;

        // Update pointer to point to our buffer
        *ser_frame = recv_frame_buffer;

        LOG(D, "Extracted original system info frame: new length=%d", *p_ser_length);
        ESP_LOG_BUFFER_HEXDUMP("MODBUS_WRAPPER", *ser_frame, *p_ser_length, ESP_LOG_DEBUG);

        return result;
    }

    // Check if frame has header
    else if ((*ser_frame)[0] == MB_FRAME_HEADER)
    {
        LOG(D, "Removing frame header: original length=%d", *p_ser_length);

        // Verify CRC16 before removing header
        // CRC16 should cover [0xFA][addr][func code][data]
        if (!mb_crc16_verify(*ser_frame, *p_ser_length))
        {
            LOG(E, "CRC16 verification failed for received frame");
            ESP_LOG_BUFFER_HEXDUMP("MODBUS_WRAPPER", *ser_frame, *p_ser_length, ESP_LOG_ERROR);
            return false;
        }

        // Check buffer size for frame without header
        uint16_t frame_without_header_len = *p_ser_length - MB_FRAME_HEADER_SIZE;
        if (frame_without_header_len > sizeof(recv_frame_buffer))
        {
            LOG(E, "Received frame too large: %d bytes (max %d)",
                frame_without_header_len, sizeof(recv_frame_buffer));
            return false;
        }

        // Copy frame data without header to our buffer (excluding old CRC16)
        // Format will be: [addr][func code][data] (CRC16 will be recalculated)
        uint16_t data_len = frame_without_header_len - MB_CRC16_SIZE;
        memcpy(recv_frame_buffer, (*ser_frame) + MB_FRAME_HEADER_SIZE, data_len);

        // Recalculate CRC16 for frame without 0xFA header
        // CRC16 should cover [addr][func code][data] (without 0xFA)
        uint16_t new_crc = mb_crc16_calculate(recv_frame_buffer, data_len);

        // Append new CRC16 (low byte first, then high byte)
        recv_frame_buffer[data_len] = (uint8_t)(new_crc & 0xFF);             // Low byte
        recv_frame_buffer[data_len + 1] = (uint8_t)((new_crc >> 8) & 0xFF);  // High byte

        // Update length (same as original frame without 0xFA header)
        (*p_ser_length) = frame_without_header_len;

        // Update pointer to point to our buffer
        *ser_frame = recv_frame_buffer;

        LOG(D, "Frame header removed and CRC16 recalculated: new length=%d, new CRC16=0x%04X",
            *p_ser_length, new_crc);
        ESP_LOG_BUFFER_HEXDUMP("MODBUS_WRAPPER", *ser_frame, *p_ser_length, ESP_LOG_DEBUG);
    }
    else
    {
        // No header found, log warning but continue
        LOG(W, "Received frame without header 0xFA (first byte=0x%02X)",
            (*ser_frame)[0]);
        // Still return the frame, but it may not be valid
    }

    return result;
}
