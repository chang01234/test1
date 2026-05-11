//------------------------------------------------------------------------------
// Module:      HALCAN.c
//------------------------------------------------------------------------------
// Description: Hardware Abstraction Layer for CAN port services.
//
//              This driver controls access to the CAN peripherals
//              of the NXP MKV42 MCU.
//
//              The CAN port numbers used throughout this module represent
//              the available physical peripherals defined in bsp.h. Even tough
//              bsp.h defines the CAN port(s) in an enumeration, we decided to
//              use integers to ensure that API services don't need to be
//              recompiled if a new bsp is defined.
//
//              The CAN interface can operate in callback or polling mode
//              for read and write operations. It is possible for the read
//              interface to use callbacks and the write interface to use polling,
//              or visa versa.
//
//              POLLING MODE:
//
//              In polling mode, the user has to periodically call
//              HALCAN_RxProcess() or HALCAN_TxProcess() to copy the CAN
//              messages from the software RX/TX buffers to the CAN peripherals.
//
//              EVENT DRIVEN MODE:
//
//              In event driven mode, the user can trigger the HALCAN_RxFullISR()
//              or HALCAN_TxEmptyISR() methods on RX full and TX empty interrupts.
//------------------------------------------------------------------------------
// Copyright:   SeaStar Solutions Inc.
//              3831, No 6 Road
//              Richmond, BC
//              Canada V6V 1P6
//
//              This source file and the information contained in it are
//              confidential and proprietary to SeaStar Solutions Inc.
//              The reproduction or disclosure, in whole or in part,
//              to anyone outside of SeaStar Solutions without the written
//              approval of a SeaStar Solutions officer under a Non-Disclosure
//              Agreement is expressly prohibited.
//
//              All rights reserved
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <string.h>
#include "hal_can.h"
#include "HALCAN.h"
#include "configuration.h"

//------------------------------------------------------------------------------
#define HMI_COMMUNICATION_ERROR (8)

//------------------------------------------------------------------------------
static can_error_cb_t errorFuncCb = NULL;

//------------------------------------------------------------------------------
// Function:    HALCAN_Initialize
//------------------------------------------------------------------------------
// Description: Initialize driver
//------------------------------------------------------------------------------
extern void HALCAN_Initialize( can_error_cb_t pFunc )
{
    errorFuncCb = pFunc;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_InitAppFilter
//------------------------------------------------------------------------------
// Description: Set application filter (callback)
//------------------------------------------------------------------------------
void HALCAN_InitAppFilter( uint8 u8CanPort, halcan_tMsgFilter pMsgFilter )
{
    (void)u8CanPort;
    (void)pMsgFilter; // TODO: Fix if filtering is needed
}

//------------------------------------------------------------------------------
// Function:    HALCAN_TxProcess
//------------------------------------------------------------------------------
// Description: Polling method of processing CAN transmit frames. Attempts to
//              send any pending CAN frames in the TX FIFO onto the CAN network.
//              This function must be called periodically, preferably as fast
//              as possible.
//------------------------------------------------------------------------------
// Return:      TRUE if transmit successful
//------------------------------------------------------------------------------
bool HALCAN_TxProcess
(
    uint8 u8Port   // CAN Port
)
{
    // Nothing to do
    return FALSE;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_RxProcess
//------------------------------------------------------------------------------
// Description: Polling method of processing CAN receive frames. Attempts to
//              read any pending CAN frames from the device into the RX FIFO.
//
//              This function must be called periodically, preferably as fast
//              as possible.
//------------------------------------------------------------------------------
// Return:      TRUE if successful
//------------------------------------------------------------------------------
bool HALCAN_RxProcess
(
    uint8 u8Port   // CAN Port
)
{
    // Nothing to do.
    return TRUE;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_Start
//------------------------------------------------------------------------------
// Description: Start interface
//------------------------------------------------------------------------------
bool HALCAN_Start( uint8 u8Port )
{
    return hal_can_init(u8Port, HAL_CAN_BAUD_250K) == HAL_E_OK? TRUE:FALSE;  
}

//------------------------------------------------------------------------------
// Function:    HALCAN_TxFree
//------------------------------------------------------------------------------
// Description: Stop interface
//------------------------------------------------------------------------------
bool HALCAN_Stop( uint8 u8Port )
{
return hal_can_deinit(u8Port) == HAL_E_OK? TRUE:FALSE;   
}

//------------------------------------------------------------------------------
// Function:    HALCAN_Read
//------------------------------------------------------------------------------
// Description: Read a message from a CAN Rx Queue.
//------------------------------------------------------------------------------
// Return:      TRUE if a new message was read.
//------------------------------------------------------------------------------
bool HALCAN_Read( uint8 u8Port, HALCAN_zMSG *pMsg )
{
    hal_can_msg_t can_msg;
    if (hal_can_read(u8Port, &can_msg) == HAL_E_OK)
    {
        pMsg->u32Id    = can_msg.id;
        pMsg->eIdType  = (can_msg.id_type == HAL_CAN_ID_EXT)? HALCAN_IDTYPE_EXTENDED : HALCAN_IDTYPE_STANDARD;
        pMsg->u8Length = can_msg.length;
        memcpy(pMsg->au8Data, can_msg.data, 8);
        return TRUE;
    }
    return FALSE;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_Write
//------------------------------------------------------------------------------
// Description: Write a message into a CAN TX queue. The message will be sent
//              the next time HALCAN_Process is called.
//------------------------------------------------------------------------------
// Return:      TRUE if was successfully written
//------------------------------------------------------------------------------
bool HALCAN_Write( uint8 u8Port, HALCAN_zMSG *pMsg )
{
    // Redirect to hal_can
    hal_can_msg_t can_msg;
    hal_err_t     can_result;
    can_msg.id      = pMsg->u32Id;
    can_msg.id_type = (pMsg->eIdType == HALCAN_IDTYPE_EXTENDED)? HAL_CAN_ID_EXT : HAL_CAN_ID_STD;
    can_msg.length  = pMsg->u8Length;
    memcpy(can_msg.data, pMsg->au8Data, 8);
    can_result = hal_can_write(u8Port, &can_msg);

    // Detect status change
    static bool error_active = false;
    if (can_result == HAL_E_OK)
    {
        if (error_active == true)
        {
            LOG(W, "CAN TX restored");
            if (errorFuncCb != NULL)
                errorFuncCb(0);
            error_active = false;
        }
    }
    else
    {
        if (error_active == false)
        {
            LOG(E, "CAN TX failed %d", can_result);
            if (errorFuncCb != NULL)
                errorFuncCb(HMI_COMMUNICATION_ERROR);
            error_active = true;
        }
    }

    // Return result
    return (can_result == HAL_E_OK)? TRUE:FALSE;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_TxFree
//------------------------------------------------------------------------------
// Description: Get transmit queue free space
//------------------------------------------------------------------------------
uint8 HALCAN_TxFree( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.tx_capacity - info.queue.tx_used;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_RxFree
//------------------------------------------------------------------------------
// Description: Get receive queue free space
//------------------------------------------------------------------------------
uint8  HALCAN_RxFree( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.rx_capacity - info.queue.rx_used;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_TxSize
//------------------------------------------------------------------------------
// Description: Get transmit queue capacity
//------------------------------------------------------------------------------
uint8  HALCAN_TxSize( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.tx_capacity;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_RxSize
//------------------------------------------------------------------------------
// Description: Get receive queue capacity
//------------------------------------------------------------------------------
uint8  HALCAN_RxSize( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.rx_capacity;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_GetRxPeak
//------------------------------------------------------------------------------
// Description: Get receive queue maximum usage
//------------------------------------------------------------------------------
extern uint8  HALCAN_GetRxPeak( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.rx_used;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_GetRxDropped
//------------------------------------------------------------------------------
// Description: Get receive queue overflow counter
//------------------------------------------------------------------------------
extern uint16 HALCAN_GetRxDropped( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.rx_overflow;
}

//------------------------------------------------------------------------------
// Function:    HALCAN_GetTxPeak
//------------------------------------------------------------------------------
// Description: Get transmit queue maximum usage
//------------------------------------------------------------------------------
extern uint8  HALCAN_GetTxPeak( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.tx_used; 
}

//------------------------------------------------------------------------------
// Function:    HALCAN_GetTxDropped
//------------------------------------------------------------------------------
// Description: Get transmit queue overflow counter
//------------------------------------------------------------------------------
extern uint16 HALCAN_GetTxDropped( uint8 u8Port )
{
    hal_can_info_t info = {0};
    hal_can_get_info(u8Port, &info);
    return info.queue.tx_overflow;   
}
