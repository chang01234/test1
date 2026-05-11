/*! \file drv_mcp2518fd.h
	\brief Microchip MCP2518 driver

    This header file provides the API function prototypes for the CAN FD SPI
    controller.
*/

// DOM-IGNORE-BEGIN
// DOM-IGNORE-END

#ifndef _DRV_CANFDSPI_API_H
#define _DRV_CANFDSPI_API_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files

#include <stdint.h>
#include <stdbool.h>
#include "drv_mcp2518fd_register.h"
#include "hal_spi_master.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif
// DOM-IGNORE-END

typedef struct _CAN_TX_CONFIG {
    CAN_FIFO_CHANNEL can_tx_fifo;
    CAN_FIFO_PLSIZE payload_size;
    uint16_t fifo_size : 5;
    uint16_t tx_priority : 5;
    uint16_t unimplemented1 : 6;
}CAN_TX_CONFIG;

typedef struct _CAN_RX_CONFIG {
    CAN_FIFO_CHANNEL can_rx_fifo;
    CAN_FIFO_PLSIZE payload_size;
    uint8_t fifo_size : 5;
    uint8_t unimplemented1 : 3;
}CAN_RX_CONFIG;

typedef struct _CAN_FILTER_MASK_CONFIG {
    CAN_FILTER filterId;
    uint32_t SID : 11;
    uint32_t EID : 18;
    uint32_t SID11 : 1;
    uint32_t EXIDE : 1;
    uint32_t unimplemented1 : 1;

    uint32_t MSID : 11;
    uint32_t MEID : 18;
    uint32_t MSID11 : 1;
    uint32_t MIDE : 1;
    uint32_t unimplemented2 : 1;
}CAN_FILTER_MASK_CONFIG;

/* Initializer macro for filter configuration to accept all IDs  */
#define CAN_FILTER_CONFIG_ACCEPT_ALL()  {.MIDE = 0, .MSID = 0, .MEID = 0, .MSID11 = 0}

typedef struct _CAN_TIMING_CONFIG {
    CAN_SYSCLK_SPEED sys_clk;
    CAN_SSP_MODE ssp_mode;
    CAN_BITTIME_SETUP bit_time;
}CAN_TIMING_CONFIG;

typedef struct _CAN_CONFIGURATION {
    CAN_TX_CONFIG txCfg;
    CAN_RX_CONFIG rxCfg;
    CAN_FILTER_MASK_CONFIG fltMaskCfg;
    CAN_TIMING_CONFIG timeCfg;
}CAN_CONFIGURATION;

typedef spi_device_handle_t MCP2518_Device;

int8_t DRV_MCP2518FD_Read(uint32_t *u32Id, bool *bExt, uint8_t *u8Dlc, uint8_t *au8Data);
int8_t DRV_MCP2518FD_Write(uint32_t u32Id, bool bExt, uint8_t  u8Dlc, uint8_t  *au8Data);
int8_t DRV_MCP2518FD_Init(MCP2518_Device spi_device, CAN_TX_CONFIG txCfg, CAN_RX_CONFIG rxCfg, CAN_FILTER_MASK_CONFIG fltMaskCfg, CAN_TIMING_CONFIG timeCfg);

#endif // _DRV_CANFDSPI_API_H
