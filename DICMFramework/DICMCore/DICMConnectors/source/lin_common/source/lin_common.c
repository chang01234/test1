/**
 * @file lin_common.c
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN helper functions shared between LIN server and LIN device
 * @date 2024-01-11
 */

#include "iGeneralDefinitions.h"

#include "lin_common.h"

/**
 * @brief       Calculate frame PID parity
 *
 * @param       id Frame ID
 * @return      uint_fast8_t Returns frame PID parity bits
 */
uint_fast8_t lin_calculate_parity(uint_fast8_t id)
{
    /* Create the Lin ID parity */
    /*
     * Calculate parity bits:
     * p0 = id0 ^ id1 ^ id2 ^ id4
     * p1 = ~(id1 ^ id3 ^ id4 ^ id5)
     */
    uint_fast8_t p0 = LIN_EXTRACT_BIT(id, 0) ^ LIN_EXTRACT_BIT(id, 1) ^ LIN_EXTRACT_BIT(id, 2) ^ LIN_EXTRACT_BIT(id, 4);
    uint_fast8_t p1 = (~(LIN_EXTRACT_BIT(id, 1) ^ LIN_EXTRACT_BIT(id, 3) ^ LIN_EXTRACT_BIT(id, 4) ^ LIN_EXTRACT_BIT(id, 5))) & 0x1u;
    return (p1 << 1) | p0;
}

/**
 * @brief       Calculate frame checksum
 *
 * @param       id Frame ID
 * @param       data Pointer to data buffer
 * @param       len Length of data buffer
 * @return      uint8_t Value of checksum
 */
uint8_t lin_calculate_checksum(uint_fast8_t id, const uint8_t * data, size_t len)
{
    uint32_t checksum = 0;
    size_t start_from = 0;

    /* Diagnostic frames use classic checksum (data bytes only). Enhanced checksum (data bytes +
     * PID field) is used for other frame types.
     */
    if (LIN_IS_FRAME_ID_DIAGNOSTIC(id))
    {
        start_from = 1;
    }

    for (size_t i = start_from; i < len; i++)
    {
        checksum = checksum + data[i];
        if (checksum >= 256)
        {
            checksum -= 255;
        }
    }

    return (uint8_t)~checksum;
}

/**
 * @brief Compile time validation if the diagnostic frame definition is 8 bytes
 */
COMPILE_TIME_ASSERT(LIN_FRAME_DATA_LEN == sizeof(lin_diagnostic_frame_t));