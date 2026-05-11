//------------------------------------------------------------------------------
// Module:      HALCAN.h
//------------------------------------------------------------------------------
// Description: This driver controls access to the CAN peripherals
//              of the NXP KV58 MCU.
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
#ifndef HALCAN_H
#define HALCAN_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "hal_types.h"
#include "StdDefs.h"
// #include "CanTypes.h"
//------------------------------------------------------------------------------
// Public Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Types
//------------------------------------------------------------------------------
typedef enum
{
    BSP_CAN_NMEA2K = 0,      // twai
    BSP_CAN_COUNT
}
BSP_eCAN;


typedef enum
{
    HALCAN_IDTYPE_STANDARD = 0,
    HALCAN_IDTYPE_EXTENDED,
}
HALCAN_eIDTYPE;

typedef struct
{
    uint32         u32Id;	   // Message ID
    HALCAN_eIDTYPE eIdType;	   // CAN ID type
    uint8          u8Length;   // Data Length [0 - 8]
    uint8          au8Data[8]; // Data Bytes  [0..8]
}
HALCAN_zMSG;

// Function pointer type for application level filter
typedef bool (*halcan_tMsgFilter)( HALCAN_zMSG *pzMsg );

// Function pointer type for error callback
typedef void (*can_error_cb_t)(int32 error_code);

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------
extern void   HALCAN_Initialize( can_error_cb_t pFunc );
extern void   HALCAN_InitAppFilter( uint8 u8CanPort, halcan_tMsgFilter pMsgFilter );
extern bool   HALCAN_TxProcess( uint8 u8Port );
extern bool   HALCAN_RxProcess( uint8 u8Port );
extern bool   HALCAN_Start( uint8 u8Port );
extern bool   HALCAN_Stop( uint8 u8Port );
extern short int HALCAN_Read( uint8 u8Port, HALCAN_zMSG *pMsg );
extern short int HALCAN_Write( uint8 u8Port, HALCAN_zMSG *pMsg );
extern uint16  HALCAN_TxFree( uint8 u8Port );
extern uint8  HALCAN_RxFree( uint8 u8Port );
extern uint8  HALCAN_TxSize( uint8 u8Port );
extern uint8  HALCAN_RxSize( uint8 u8Port );
extern uint8  HALCAN_GetRxPeak( uint8 u8Port );
extern uint16 HALCAN_GetRxDropped( uint8 u8Port );
extern uint8  HALCAN_GetTxPeak( uint8 u8Port );
extern uint16 HALCAN_GetTxDropped( uint8 u8Port );

//------------------------------------------------------------------------------
#endif /* HALCAN_H */
