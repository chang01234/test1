/*****************************************************************************
 * \file       hal_can.h
 * \brief      CAN Hardware Abstraction
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/
#ifndef HAL_CAN_H_
#define HAL_CAN_H_

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "hal_types.h"

/*****************************************************************************
 * Public Definitions
 *****************************************************************************/

/*****************************************************************************
 * Public Types
 *****************************************************************************/
typedef enum 
{
    HAL_CAN_DEVICE_TWAI    = 0,
    HAL_CAN_DEVICE_MCP2518,

    HAL_CAN_DEVICE_DIM // always last
} HAL_CAN_DEVICE;

typedef enum 
{
    HAL_CAN_BAUD_250K,
    HAL_CAN_BAUD_500K,
    HAL_CAN_BAUD_1M
} HAL_CAN_BAUD;

typedef enum
{
    // Service status
    HAL_CAN_NOT_INITIALIZED = 0, // Port not initialized
    HAL_CAN_INITIALIZED,         // Port initialized but not yet synchronized
    HAL_CAN_SLEEPING,            // Peripheral is in sleep mode
    HAL_CAN_SYNCHRONIZED,        // Peripheral synchronized to the CAN bus
    HAL_CAN_ERROR_PASSIVE,       // Peripheral is in Error Passive state 
    HAL_CAN_BUS_OFF,             // Peripheral is in Bus Off state (No more communicating)
} HAL_CAN_STATUS;

//------------------------------------------------------------------------------
// Public Types
//------------------------------------------------------------------------------
typedef enum
{
    HAL_CAN_ID_STD = 0,
    HAL_CAN_ID_EXT = 1,
}
HAL_CAN_ID_TYPE;

typedef struct
{
    uint32_t        id;         // CAN ID
    HAL_CAN_ID_TYPE id_type;	// CAN ID type
    uint8_t         length;     // Data Length [0 - 8]
    uint8_t         data[8];    // Data Bytes  [0..8]
}
hal_can_msg_t;

typedef struct
{
    // BUS
    struct
    {
        bool bus_off;
        bool warning;
    } status;

    // FIFO Usage
    struct
    {
        uint16_t  tx_used;
        uint16_t  tx_capacity;
        uint16_t  tx_overflow;
        uint16_t  rx_used;
        uint16_t  rx_capacity;
        uint16_t  rx_overflow;
    } queue;

    // operational state
    HAL_CAN_STATUS eStatus;
}
hal_can_info_t;

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
hal_err_t hal_can_init(HAL_CAN_DEVICE device, HAL_CAN_BAUD baud);
hal_err_t hal_can_deinit(HAL_CAN_DEVICE device);
hal_err_t hal_can_get_info(HAL_CAN_DEVICE device, hal_can_info_t *info);
HAL_CAN_STATUS hal_can_get_stat(HAL_CAN_DEVICE device);
hal_err_t hal_can_read(HAL_CAN_DEVICE device, hal_can_msg_t *msg);
hal_err_t hal_can_write(HAL_CAN_DEVICE device, hal_can_msg_t *msg);
hal_err_t hal_can_recover(HAL_CAN_DEVICE device);
hal_err_t hal_can_resume(HAL_CAN_DEVICE device, bool bStart);
hal_err_t hal_can_pause(HAL_CAN_DEVICE device, bool bStop);
hal_err_t hal_can_start(HAL_CAN_DEVICE device);
hal_err_t hal_can_stop(HAL_CAN_DEVICE device);

// Utilities (derived from the above functions)
bool hal_can_get_is_bus_off(HAL_CAN_DEVICE device);
uint16_t hal_can_get_tx_free(HAL_CAN_DEVICE device);

/*****************************************************************************/
#endif // HAL_CAN_H_
