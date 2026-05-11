//------------------------------------------------------------------------------
// Module:      MsgCAN.c
//
//------------------------------------------------------------------------------
// Description: Provide mailboxes for the RV-C network.
//              Handle decoding of incoming message parameters which are written
//              into the process image. Handle packaging of process image 
//              variables into the transmitted messages.
//------------------------------------------------------------------------------
// COPYRIGHT:   SeaStar Solutions
//              3831, No 6 Road
//              Richmond, BC
//              Canada V6V 1P6
//
//              This source file and the information contained in it are 
//              confidential and proprietary to SeaStar Solutions. 
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

// Libraries
#include <string.h>
#include "RVCDGN.h"
#include "NMEA2K.h"

// Platform
#include "HALCAN.h"

// Application
#include "PImage.h"
#include "PropDGN.h"
#include "MsgCAN.h"
#include "rvc_to_ddm.h"

#ifdef CONNECTOR_PREM_RVC

//------------------------------------------------------------------------------
// Local constants
//------------------------------------------------------------------------------
#define MSGCAN_ARBITRARY_ADDR_CAPABLE   TRUE                     // Can use arbitrary SA to resolve a conflict
#define MSGCAN_INDUSTRY_GROUP           1                        // J1939 TABLE B1: 1 = On-Highway Equipment
#define MSGCAN_VEHICLE_SYSTEM_INSTANCE  0                        // First u8Instance
#define MSGCAN_VEHICLE_SYSTEM           0                        // Non specific system
#define MSGCAN_RESERVED_FIELD           0                        // Reserved by SAE, should be set to 0
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
#define MSGCAN_FUNCTION                 50                       // Auxiliary Heater #1 (J1939, not RV-C)
#else
#define MSGCAN_FUNCTION                 21                       // Not defined in RV-C. Used J1939 cab climate controller (used for thermostat)
#endif
#define MSGCAN_FUNCTION_INSTANCE        1                        // First u8Instance
#define MSGCAN_ECU_INSTANCE             0                        // First u8Instance
#define MSGCAN_SERIAL_NUMBER            0                        // Serial number
#define MSGCAN_MANUFACTURER_CODE        103						 // RVC TABLE 7.1
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
#define MSGCAN_PREFERRED_ADDRESS	    94                       // Preferred source address
#else
#define MSGCAN_PREFERRED_ADDRESS	    88                       // Preferred source address
#endif
#define MSGCAN_DSA              	    MSGCAN_PREFERRED_ADDRESS // Default source address
#define MSGCAN_RX_MAILBOX_SIZE          DIM(msgcan_zRxMailbox)   // EST receive mailbox size.
#define MSGCAN_TX_MAILBOX_SIZE          DIM(msgcan_zTxMailbox)   // EST transmit mailbox size.

#define MAX_SW_INDEX 4
#define DGN_131071_SECOND_PUBLISH_INTERVAL 5 /* In seconds */

#define PREM_RVC_LOG 0 /* 1- Logs enabled */

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
const int CB_TARGET_TEMP_NOT_BLOCKED = 0x00000001;
EventGroupHandle_t callback_accept_handle = NULL;

//------------------------------------------------------------------------------
// Local variables.
//------------------------------------------------------------------------------
static uint8 msgcan_u8PreferredSA;       // Preferred source address
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static uint16 msgcan_u16HeartBeatHeater;   // Heart beat  timer for heater messages
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static uint16 msgcan_u16HeartBeatHMI;      // Heart beat  timer for HMI messages
#endif
static uint8_t SWId[MAX_SW_INDEX][RVCDGN_DGN_65242_FIELD_SIZE] = {0};

//------------------------------------------------------------------------------
// Mailboxes
//------------------------------------------------------------------------------
// Receive buffers
static uint8 MsgCan_DGN_65242_u8RxData[ RVCDGN_DGN_65242_SIZE ];
static uint8 MsgCan_DGN_65259_u8RxData[ RVCDGN_DGN_65259_SIZE ];
static uint8 MsgCan_DGN_130762_u8RxData[ RVCDGN_DGN_130762_SIZE];
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static uint8 MsgCan_DGN_130554_u8RxData[ PROPDGN_DGN_130554_SIZE ];
static uint8 MsgCan_DGN_130556_u8RxData[ PROPDGN_DGN_130556_SIZE ];
static uint8 MsgCan_DGN_130558_u8RxData[ PROPDGN_DGN_130559_SIZE ];
static uint8 MsgCan_DGN_130972_u8RxData[ RVCDGN_DGN_130972_SIZE ];
static uint8 MsgCan_DGN_131070_u8RxData[ RVCDGN_DGN_130972_SIZE ];
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static uint8 MsgCan_DGN_130552_u8RxData[ PROPDGN_DGN_130552_SIZE ];
static uint8 MsgCan_DGN_130553_u8RxData[ PROPDGN_DGN_130553_SIZE ];
static uint8 MsgCan_DGN_130555_u8RxData[ PROPDGN_DGN_130555_SIZE ];
static uint8 MsgCan_DGN_130557_u8RxData[ PROPDGN_DGN_130557_SIZE ];
static uint8 MsgCan_DGN_130559_u8RxData[ PROPDGN_DGN_130559_SIZE ];
static uint8 MsgCan_DGN_131071_u8RxData[ RVCDGN_DGN_131071_SIZE ];
#endif

//------------------------------------------------------------------------------
// Receive handlers
static void  MsgCan_DGN_65242_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_65259_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130762_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void  MsgCan_DGN_130554_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130556_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130558_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130972_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_131070_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void  MsgCan_DGN_130552_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130553_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130555_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130557_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_130559_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
static void  MsgCan_DGN_131071_RxHandle( uint8 u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox );
#endif

//------------------------------------------------------------------------------
// Receive mailbox definition
static NMEA2K_RxMsg_Struct msgcan_zRxMailbox[] =
{
//    Enable Trunk Rx_InProg  DgnType    DGN     AgeCtr   MsgSize   OrgAddr OrgFil  DstAddr  BufferSize                DataBuffer                   Rx Handler
	{ 1,     0,    0,         0,         65242,  0,       0,        0,      0x5E,   0,        RVCDGN_DGN_65242_SIZE,   MsgCan_DGN_65242_u8RxData,   MsgCan_DGN_65242_RxHandle,   },
	{ 1,     0,    0,         0,         65259,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_65259_SIZE,   MsgCan_DGN_65259_u8RxData,   MsgCan_DGN_65259_RxHandle,   },
	{ 1,     0,    0,         0,        130762,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_130762_SIZE,  MsgCan_DGN_130762_u8RxData,  MsgCan_DGN_130762_RxHandle,  },
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
	{ 1,     0,    0,         0,        130554,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130554_SIZE,  MsgCan_DGN_130554_u8RxData,  MsgCan_DGN_130554_RxHandle,  },
	{ 1,     0,    0,         0,        130556,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130556_SIZE,  MsgCan_DGN_130556_u8RxData,  MsgCan_DGN_130556_RxHandle,  },
	{ 1,     0,    0,         0,        130558,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130558_SIZE,  MsgCan_DGN_130558_u8RxData,  MsgCan_DGN_130558_RxHandle,  },
	{ 1,     0,    0,         0,        130972,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_130972_SIZE,  MsgCan_DGN_130972_u8RxData,  MsgCan_DGN_130972_RxHandle,  },
	{ 1,     0,    0,         0,        131070,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_131070_SIZE,  MsgCan_DGN_131070_u8RxData,  MsgCan_DGN_131070_RxHandle,  },
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
	{ 1,     0,    0,         0,        130552,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130552_SIZE,  MsgCan_DGN_130552_u8RxData,  MsgCan_DGN_130552_RxHandle,  },
	{ 1,     0,    0,         0,        130553,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130553_SIZE,  MsgCan_DGN_130553_u8RxData,  MsgCan_DGN_130553_RxHandle,  },
	{ 1,     0,    0,         0,        130555,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130555_SIZE,  MsgCan_DGN_130555_u8RxData,  MsgCan_DGN_130555_RxHandle,  },
	{ 1,     0,    0,         0,        130557,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130557_SIZE,  MsgCan_DGN_130557_u8RxData,  MsgCan_DGN_130557_RxHandle,  },
	{ 1,     0,    0,         0,        130559,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130559_SIZE,  MsgCan_DGN_130559_u8RxData,  MsgCan_DGN_130559_RxHandle,  },
	{ 1,     0,    0,         0,        131071,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_131071_SIZE,  MsgCan_DGN_131071_u8RxData,  MsgCan_DGN_131071_RxHandle,  },
#endif
};

//------------------------------------------------------------------------------
// Transmit buffers
static uint8 MsgCan_DGN_65242_u8TxData[ RVCDGN_DGN_65242_SIZE    ];
static uint8 MsgCan_DGN_65259_u8TxData[ RVCDGN_DGN_65259_SIZE    ];
static uint8 MsgCan_DGN_130762_u8TxData[ RVCDGN_DGN_130762_SIZE  ];
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static uint8 MsgCan_DGN_130553_u8TxData[ PROPDGN_DGN_130553_SIZE ];
static uint8 MsgCan_DGN_130555_u8TxData[ PROPDGN_DGN_130555_SIZE ];
static uint8 MsgCan_DGN_130557_u8TxData[ PROPDGN_DGN_130557_SIZE ];
static uint8 MsgCan_DGN_130559_u8TxData[ PROPDGN_DGN_130559_SIZE ];
static uint8 MsgCan_DGN_131071_u8TxData[ RVCDGN_DGN_131071_SIZE  ];
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static uint8 MsgCan_DGN_130554_u8TxData[ PROPDGN_DGN_130556_SIZE ];
static uint8 MsgCan_DGN_130556_u8TxData[ PROPDGN_DGN_130557_SIZE ];
static uint8 MsgCan_DGN_130558_u8TxData[ PROPDGN_DGN_130557_SIZE ];
static uint8 MsgCan_DGN_130972_u8TxData[ RVCDGN_DGN_130972_SIZE  ];
static uint8 MsgCan_DGN_131070_u8TxData[ RVCDGN_DGN_131070_SIZE  ];
#endif

//------------------------------------------------------------------------------
// Transmit handlers
static void  MsgCan_DGN_65242_TxHandle(  uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_65259_TxHandle(  uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_130762_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void  MsgCan_DGN_130553_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_130555_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_130557_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_130559_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_131071_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void  MsgCan_DGN_130554_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_130556_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_130558_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_130972_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
static void  MsgCan_DGN_131070_TxHandle( uint8 u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox );
#endif
//------------------------------------------------------------------------------
// Transmit mailbox definition
static NMEA2K_TxMsg_Struct msgcan_zTxMailbox[] =
{
    //  TxReq   TxReady Period  TxInProg DgnType Inst  CntrSet  Cntr  DGN     Prio  DestAddr  Req_Addr  DataSize                     Data                       Handler
	{   0,      0,        0,    0,       0,      0,    500,      0,    65242,  6,    0xFF,     0xFF,       RVCDGN_DGN_65242_SIZE,   MsgCan_DGN_65242_u8TxData,   MsgCan_DGN_65242_TxHandle, },
	{   0,      0,        1,    0,       0,      0,    500,      0,    65259,  6,    0xFF,     0xFF,       RVCDGN_DGN_65259_SIZE,   MsgCan_DGN_65259_u8TxData,   MsgCan_DGN_65259_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    100,      0,   130762,  6,    0xFF,     0xFF,      RVCDGN_DGN_130762_SIZE,   MsgCan_DGN_130762_u8TxData,  MsgCan_DGN_130762_TxHandle, },
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
	{   0,      0,        1,    0,       0,      0,    100,      0,   130553,  6,    0xFF,     0xFF,     PROPDGN_DGN_130553_SIZE,   MsgCan_DGN_130553_u8TxData,  MsgCan_DGN_130553_TxHandle, },
	{   0,      0,        1,    0,       0,      0,    100,      0,   130555,  6,    0xFF,     0xFF,     PROPDGN_DGN_130555_SIZE,   MsgCan_DGN_130555_u8TxData,  MsgCan_DGN_130555_TxHandle, },
	{   0,      0,        1,    0,       0,      0,    100,      0,   130557,  6,    0xFF,     0xFF,     PROPDGN_DGN_130557_SIZE,   MsgCan_DGN_130557_u8TxData,  MsgCan_DGN_130557_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    100,      0,   130559,  6,    0xFF,     0xFF,     PROPDGN_DGN_130559_SIZE,   MsgCan_DGN_130559_u8TxData,  MsgCan_DGN_130559_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    100,      0,   131071,  6,    0xFF,     0xFF,      RVCDGN_DGN_131071_SIZE,   MsgCan_DGN_131071_u8TxData,  MsgCan_DGN_131071_TxHandle, },
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
	{   0,      0,        1,    0,       0,      0,    100,      0,   130554,  6,    0xFF,     0xFF,     PROPDGN_DGN_130554_SIZE,   MsgCan_DGN_130554_u8TxData,  MsgCan_DGN_130554_TxHandle, },
	{   0,      0,        1,    0,       0,      0,    100,      0,   130556,  6,    0xFF,     0xFF,     PROPDGN_DGN_130556_SIZE,   MsgCan_DGN_130556_u8TxData,  MsgCan_DGN_130556_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    100,      0,   130558,  6,    0xFF,     0xFF,     PROPDGN_DGN_130558_SIZE,   MsgCan_DGN_130558_u8TxData,  MsgCan_DGN_130558_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    500,      0,   130972,  6,    0xFF,     0xFF,      RVCDGN_DGN_130972_SIZE,   MsgCan_DGN_130972_u8TxData,  MsgCan_DGN_130972_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    500,      0,   131070,  6,    0xFF,     0xFF,      RVCDGN_DGN_131070_SIZE,   MsgCan_DGN_131070_u8TxData,  MsgCan_DGN_131070_TxHandle, },
#endif
};

static parameter_changed_cb_t parameterFuncCb = NULL;

//------------------------------------------------------------------------------
// NMEA2K communication parameter definition
static NMEA2K_Parameter_Struct msgcan_NMEA2KParameter =
{
    BSP_CAN_RVC,                        // RVC CAN port index
    msgcan_zRxMailbox,                  // RxMailbox Address
    MSGCAN_RX_MAILBOX_SIZE,             // RxMailbox Size
    msgcan_zTxMailbox,                  // TxMailbox Address
    MSGCAN_TX_MAILBOX_SIZE,             // TxMailbox Size
    &msgcan_u8PreferredSA,              // Preferred Source Address
    { // Name fields
        MSGCAN_ARBITRARY_ADDR_CAPABLE,  // NMEA2K name field 1
        MSGCAN_INDUSTRY_GROUP,          // NMEA2K name field 2
        MSGCAN_VEHICLE_SYSTEM_INSTANCE, // NMEA2K name field 3
        MSGCAN_VEHICLE_SYSTEM,          // NMEA2K name field 4
        MSGCAN_RESERVED_FIELD,          // NMEA2K name field 5
        MSGCAN_FUNCTION,                // NMEA2K name field 6
        MSGCAN_FUNCTION_INSTANCE,       // NMEA2K name field 7
        MSGCAN_ECU_INSTANCE,            // NMEA2K name field 8
        MSGCAN_MANUFACTURER_CODE,       // NMEA2K name field 9
        MSGCAN_SERIAL_NUMBER,           // NMEA2K name field 10
    }
};

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------
static void MsgCan_Update_PImage(PIMAGE_eTABLE eIndex, int32 i32Value);

//------------------------------------------------------------------------------
// Local functions.
//------------------------------------------------------------------------------
static void MsgCan_DGN_130553_Reset( void );
static void MsgCan_DGN_130554_Reset( void );
static void MsgCan_DGN_130555_Reset( void );
static void MsgCan_DGN_130556_Reset( void );
static void MsgCan_DGN_130557_Reset( void );
static void MsgCan_DGN_130558_Reset( void );
static void MsgCan_DGN_130559_Reset( void );
static void MsgCan_DGN_130972_Reset( void );
static void MsgCan_DGN_131070_Reset( void );
static void MsgCan_DGN_131071_Reset( void );

static bool MsgCan_FilterMsg( HALCAN_zMSG *zMsg );

//-----------------------------------------------------------------------------
// Function:    MSGCAN_UpdateSWId
//-----------------------------------------------------------------------------
// Description: Update SWId for index 0-3.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void MSGCAN_UpdateSWId(int index, void *data, int length)
{
	if (index >= MAX_SW_INDEX)
		return;

	memset(SWId[index], 0, RVCDGN_DGN_65242_FIELD_SIZE);
	memcpy(SWId[index], data, length);
}

//-----------------------------------------------------------------------------
// Function:    MSGCAN_Initialize
//-----------------------------------------------------------------------------
// Description: Initialization of receive and transmit mail boxes.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void MSGCAN_Initialize
( 
	uint8  u8SrcAddr,    // in : Preferred source address
	uint32 u32NameId,    // in : NMEA 2000 identity number
	uint8  u8Instance,    // in : Function instance
	parameter_changed_cb_t funcCb // in: Application function cb
)
{
    // Initialize RVC instance and preferred address
    msgcan_NMEA2KParameter.Name_Fields.Function_Instance = u8Instance;
    msgcan_u8PreferredSA = u8SrcAddr + u8Instance - 1;

    // Assign identity number
    msgcan_NMEA2KParameter.Name_Fields.Identity_Number = u32NameId & RVCDGN_SERIAL_NUMBER_MASK;

    // Set HALCAN filter for messages
    HALCAN_InitAppFilter( BSP_CAN_RVC, MsgCan_FilterMsg );

    // Initialize a new instance of the communication protocol
    (void)NMEA2K_Initialize( &msgcan_NMEA2KParameter );

	// Reset heart beat timers
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
    msgcan_u16HeartBeatHMI = 0;
#endif
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
    msgcan_u16HeartBeatHeater    = 0;
#endif
	
	parameterFuncCb = funcCb;

	// Reset DGNs value to not available
    MsgCan_DGN_130553_Reset();
    MsgCan_DGN_130554_Reset();
    MsgCan_DGN_130555_Reset();
	MsgCan_DGN_130556_Reset();
	MsgCan_DGN_130557_Reset();
	MsgCan_DGN_130558_Reset();
	MsgCan_DGN_130559_Reset();
	MsgCan_DGN_130972_Reset();
	MsgCan_DGN_131070_Reset();
	MsgCan_DGN_131071_Reset();

    // Test mode (demo)
#if MSGCAN_TEST_MODE==TRUE
	PIMAGE_SetValue( VAR_DGN_130556_ROOM_TEMP,         RVCDGN_TEMPERATURE_TO_S16(27) );
	PIMAGE_SetValue( VAR_DGN_130558_TARGET_ROOM_TEMP,  RVCDGN_TEMPERATURE_TO_S16(-23.5) );
	PIMAGE_SetValue( VAR_DGN_130558_UNDERVOLT_THRES,   RVCDGN_VOLT_TO_U8(11.4) );
	PIMAGE_SetValue( VAR_DGN_130559_TARGET_ROOM_TEMP,  RVCDGN_TEMPERATURE_TO_S16(-21.0) );
    PIMAGE_SetValue( VAR_DGN_130559_UNDERVOLT_THRES,   RVCDGN_VOLT_TO_U8(-10.6) );
	PIMAGE_SetValue( VAR_DGN_130972_AMBIENT_TEMP,      RVCDGN_TEMPERATURE_TO_U16(-22.5)  );
	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_FAV,  0);
	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_MENU, 1);
	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_HOME, 2);
	PIMAGE_SetValue( VAR_DGN_130558_AIR_HEATER_CMD,  0);
	PIMAGE_SetValue( VAR_DGN_130558_WTR_HEATER_CMD,  1);
	PIMAGE_SetValue( VAR_DGN_130558_AIR_HEATER_MODE, 2);
	PIMAGE_SetValue( VAR_DGN_130558_WTR_HEATER_MODE, 3);
	PIMAGE_SetValue( VAR_DGN_130558_SYSTEM_UNITS, 1);

	PIMAGE_SetValue( VAR_DGN_130557_AC_HEATER_AIR,   1   );
	PIMAGE_SetValue( VAR_DGN_130557_AC_HEATER_WTR,   2   );
	PIMAGE_SetValue( VAR_DGN_130557_GAS_HEATER_AIR,  3   );
	PIMAGE_SetValue( VAR_DGN_130557_GAS_HEATER_WTR,  4   );

#endif


}

//-----------------------------------------------------------------------------
// Function:    MSGCAN_Process
//-----------------------------------------------------------------------------
// Description: Manage message timeouts and fault detection.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void MSGCAN_Process( uint16 u16Elapsed )
{
    // Update CAN interface status
    PIMAGE_SetValue( VAR_RVC_RX_QUEUE_PEAK     , HALCAN_GetRxPeak(    BSP_CAN_RVC ) );
    PIMAGE_SetValue( VAR_RVC_RX_QUEUE_DROP_CNT , HALCAN_GetRxDropped( BSP_CAN_RVC ) );
    PIMAGE_SetValue( VAR_RVC_TX_QUEUE_PEAK     , HALCAN_GetTxPeak(    BSP_CAN_RVC ) );
    PIMAGE_SetValue( VAR_RVC_TX_QUEUE_DROP_CNT , HALCAN_GetTxDropped( BSP_CAN_RVC ) );

    //-----------------------------------------------------
    // Check Heater communication (heartbeat)
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
    if (msgcan_u16HeartBeatHeater < MSGCAN_HEARTBEAT_TIMEOUT )
    {
    	// Clear heart beat fault
    	PIMAGE_SetValue( VAR_FAULT_HEARTBEAT_LOST, FALSE );

    	// Increment timer
    	msgcan_u16HeartBeatHeater = MIN(msgcan_u16HeartBeatHeater+u16Elapsed, MSGCAN_HEARTBEAT_TIMEOUT);
    }
    else
    {
    	// Set heart beat fault
    	PIMAGE_SetValue( VAR_FAULT_HEARTBEAT_LOST, TRUE );

    	// Reset values
    	MsgCan_DGN_130553_Reset();
    	MsgCan_DGN_130555_Reset();
    	MsgCan_DGN_130557_Reset();
    	MsgCan_DGN_130559_Reset();
    	MsgCan_DGN_131071_Reset();
    }
#endif

    //-----------------------------------------------------
    // Check HMI communication (heartbeat)
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
    if (msgcan_u16HeartBeatHMI < MSGCAN_HEARTBEAT_TIMEOUT )
    {
    	// Clear heart beat fault
    	PIMAGE_SetValue( VAR_FAULT_HEARTBEAT_LOST, FALSE );

    	// Increment timer
    	msgcan_u16HeartBeatHMI = MIN(msgcan_u16HeartBeatHMI+u16Elapsed, MSGCAN_HEARTBEAT_TIMEOUT);
    }
    else
    {
    	// Set heart beat fault
    	PIMAGE_SetValue( VAR_FAULT_HEARTBEAT_LOST, TRUE );

    	// Reset values
    	MsgCan_DGN_130554_Reset();
    	MsgCan_DGN_130556_Reset();
    	MsgCan_DGN_130558_Reset();
    	MsgCan_DGN_130972_Reset();
    	MsgCan_DGN_131070_Reset();
    }
#endif
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_FilterMsg
//-----------------------------------------------------------------------------
// Description: Filter messages from the network.
//              This function is called back from within the HALCAN layer every
//              time a CAN frame is received and allows filtering before the
//              message is passed onto the NMEA2K protocol stack.
//-----------------------------------------------------------------------------
// Return:      TRUE: Receive message. FALSE: Discard message
//-----------------------------------------------------------------------------
static bool MsgCan_FilterMsg
(
    HALCAN_zMSG *zMsg  // in : CAN message received
)
{
	// TODO: Optional: Create filter (select messages to receive)
	return TRUE; // Receive all messages
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130553_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130553 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130553_Reset( void )
{
		PIMAGE_SetValue(VAR_DGN_130553_WARNING_FAULT_ACTIVE,	NMEA2K_UINT2_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_CRITICAL_FAULT_ACTIVE,	NMEA2K_UINT2_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_RESERVED_FIELD_1,		NMEA2K_UINT2_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_RESERVED_FIELD_2,		NMEA2K_UINT2_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_RESERVED_FIELD_3,		NMEA2K_UINT8_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_1, 	NMEA2K_UINT10_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_2, 	NMEA2K_UINT10_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_3, 	NMEA2K_UINT10_NO_DATA);
		PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_4, 	NMEA2K_UINT10_NO_DATA);

}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130554_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130554 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130554_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_ON_STAT,      NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_ON_HOUR,      NMEA2K_UINT6_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_ON_MIN,       NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_OFF_STAT,     NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_OFF_HOUR,     NMEA2K_UINT6_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_OFF_MIN,      NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_ON_STAT,      NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_ON_HOUR,      NMEA2K_UINT6_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_ON_MIN,       NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_KEEP_ON_TIME, NMEA2K_UINT8_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130555_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130555 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130555_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_130555_AIR_HTR_ON_STAT,      NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_AIR_HTR_ON_HOUR,      NMEA2K_UINT6_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_AIR_HTR_ON_MIN,       NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_AIR_HTR_OFF_STAT,     NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_AIR_HTR_OFF_HOUR,     NMEA2K_UINT6_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_AIR_HTR_OFF_MIN,      NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_WTR_HTR_ON_STAT,      NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_WTR_HTR_ON_HOUR,      NMEA2K_UINT6_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_WTR_HTR_ON_MIN,       NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME, NMEA2K_UINT8_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130556_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130556 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130556_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_130556_ROOM_TEMP,      NMEA2K_INT16_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130556_HEATER_COMM,    NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130556_INPUT_VOLT,     NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130556_INST_STATUS,    NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130556_INT_CIRCUITERY, NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_FAV,     NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_MENU,    NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_HOME,    NMEA2K_UINT2_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130557_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130557 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130557_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_130557_ROOM_TEMP,         NMEA2K_INT16_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130557_WATER_TEMP,        NMEA2K_UINT3_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130557_GAS_HEATER_AIR,    NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130557_GAS_HEATER_WTR,    NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130557_AC_PRESENT,        NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130557_AC_HEATER_AIR,     NMEA2K_UINT3_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130557_AC_HEATER_WTR,     NMEA2K_UINT3_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130558_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130558 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130558_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_130558_ENERGY_SOURCE,          NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_AIR_HEATER_CMD,         NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_WTR_HEATER_CMD,         NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_AIR_HEATER_MODE,        NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_WTR_HEATER_MODE,        NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_TARGET_ROOM_TEMP,       NMEA2K_INT16_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_SILENT_FAN_MAX,         NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_VENT_FAN_MIN,           NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_UNDERVOLT_THRES,        NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130558_SYSTEM_UNITS,           NMEA2K_UINT2_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130559_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130559 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130559_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_130559_ENERGY_SOURCE,          NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_AIR_HEATER_CMD,         NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_WTR_HEATER_CMD,         NMEA2K_UINT2_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_AIR_HEATER_MODE,        NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_WTR_HEATER_MODE,        NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_TARGET_ROOM_TEMP,       NMEA2K_INT16_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_SILENT_FAN_MAX,         NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_VENT_FAN_MIN,           NMEA2K_UINT4_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_UNDERVOLT_THRES,        NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_130559_SYSTEM_UNITS,           NMEA2K_UINT2_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_130972_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 130972 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_130972_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_130972_AMBIENT_TEMP,   NMEA2K_UINT16_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_131070_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 131070 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_131070_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_131070_YEAR,        NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131070_MONTH,       NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131070_DAY,         NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131070_DAY_OF_WEEK, NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131070_HOUR,        NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131070_MINUTE,      NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131070_SECOND,      NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131070_TIMEZONE,    NMEA2K_UINT8_NO_DATA );
}

//-----------------------------------------------------------------------------
// Function:    MsgCan_DGN_131071_Reset
//-----------------------------------------------------------------------------
// Description: Reset DGN 131071 parameters to not available.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgCan_DGN_131071_Reset( void )
{
	PIMAGE_SetValue( VAR_DGN_131071_YEAR,        NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131071_MONTH,       NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131071_DAY,         NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131071_DAY_OF_WEEK, NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131071_HOUR,        NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131071_MINUTE,      NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131071_SECOND,      NMEA2K_UINT8_NO_DATA );
	PIMAGE_SetValue( VAR_DGN_131071_TIMEZONE,    NMEA2K_UINT8_NO_DATA );
}

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_65242_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 65242 (software info)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
static void  MsgCan_DGN_65242_RxHandle
(
	uint8                u8CanPort,   // in: CAN port the message was received from
	NMEA2K_RxMsg_Struct* pzRxMailbox  // in: Receive mailbox pointer
)
{
    // Extract message content
	RVCDGN_zDGN_65242 zDGN;
    RVCDGN_DGN_65242_Extract( &zDGN,  pzRxMailbox->Data, pzRxMailbox->MsgSize);

    // TODO: Process received product information (see zDGN members)
}

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_65259_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 65259 (product info)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
static void  MsgCan_DGN_65259_RxHandle
(
	uint8                u8CanPort,   // in: CAN port the message was received from
	NMEA2K_RxMsg_Struct* pzRxMailbox  // in: Receive mailbox pointer
)
{
    // Extract message content
	RVCDGN_zDGN_65259 zDGN;
    RVCDGN_DGN_65259_Extract( &zDGN,  pzRxMailbox->Data, pzRxMailbox->MsgSize);
	LOG(I, "DGN 65259 received model - %s", zDGN.cModel);
    PIMAGE_SetString(STRINGS_DGN_65259_MODEL, zDGN.cModel);
	PIMAGE_SetString(STRINGS_DGN_65259_SN, zDGN.cSerialNumber);
}

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130554_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130554 (Heater scheduling commands)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130554_RxHandle
(
    uint8                u8CanPort,     // in: CAN port the message was received from
    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
	PROPDGN_zDGN_130554 zDGN;
    PROPDGN_DGN_130554_Extract( &zDGN,  pzRxMailbox->Data );

    // Valid instance?
    if ( zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_1 )
    {
    	// Copy content to the application process image
    	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_ON_STAT,      zDGN.u2AirHtrTimerOnStat  );
    	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_ON_HOUR,      zDGN.u6AirHtrTimerOnHour  );
    	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_ON_MIN,       zDGN.u8AirHtrTimerOnMin   );
    	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_OFF_STAT,     zDGN.u2AirHtrTimerOffStat );
    	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_OFF_HOUR,     zDGN.u6AirHtrTimerOffHour );
    	PIMAGE_SetValue( VAR_DGN_130554_AIR_HTR_OFF_MIN,      zDGN.u8AirHtrTimerOffMin  );
    	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_ON_STAT,      zDGN.u2AirHtrTimerOnStat  );
    	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_ON_HOUR,      zDGN.u6AirHtrTimerOnHour  );
    	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_ON_MIN,       zDGN.u8AirHtrTimerOnMin   );
    	PIMAGE_SetValue( VAR_DGN_130554_WTR_HTR_KEEP_ON_TIME, zDGN.u8WtrHtrKeepOnTime   );

        // Reload heart beat timer
    	msgcan_u16HeartBeatHMI = 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130552_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130552 (Heater firmware versions)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130552_RxHandle
(
    uint8                u8CanPort,     // in: CAN port the message was received from
    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
	PROPDGN_zDGN_130552 zDGN;
    PROPDGN_DGN_130552_Extract( &zDGN,  pzRxMailbox->Data );

    // Valid instance?
    if ( zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_1 )
    {
		MsgCan_Update_PImage(VAR_DGN_130552_COMFORT_SW_VER_MA, 	zDGN.u8ComfortSwMajor);
		MsgCan_Update_PImage(VAR_DGN_130552_COMFORT_SW_VER_MI,  zDGN.u8ComfortSwMinor);
		MsgCan_Update_PImage(VAR_DGN_130552_BURNER_SW_VER_MA, 	zDGN.u8BurnerSwMajor);
		MsgCan_Update_PImage(VAR_DGN_130552_BURNER_SW_VER_MI, 	zDGN.u8BurnerSwMinor);
		MsgCan_Update_PImage(VAR_DGN_130552_PCBA_VER,			zDGN.u8PcbaVersion); 
		MsgCan_Update_PImage(VAR_DGN_130552_PROTOCOL_VER_MA, 	zDGN.u8ProtocolMajor);
		MsgCan_Update_PImage(VAR_DGN_130552_PROTOCOL_VER_MI, 	zDGN.u8ProtocolMinor);

        // Reload heart beat timer
    	msgcan_u16HeartBeatHeater = 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130553_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130553 (Heater faults)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130553_RxHandle
(
    uint8                u8CanPort,     // in: CAN port the message was received from
    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
	PROPDGN_zDGN_130553 zDGN;
    PROPDGN_DGN_130553_Extract( &zDGN,  pzRxMailbox->Data );

    // Valid instance?
    if ( zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_1 )
    {
		MsgCan_Update_PImage(VAR_DGN_130553_WARNING_FAULT_ACTIVE, 	zDGN.u2WarningFault);
		MsgCan_Update_PImage(VAR_DGN_130553_CRITICAL_FAULT_ACTIVE,  zDGN.u2CriticalFault);
		MsgCan_Update_PImage(VAR_DGN_130553_ACTIVE_FAULT_CODE_1, 	zDGN.u10FaultCode1);
		MsgCan_Update_PImage(VAR_DGN_130553_ACTIVE_FAULT_CODE_2, 	zDGN.u10FaultCode2);
		MsgCan_Update_PImage(VAR_DGN_130553_ACTIVE_FAULT_CODE_3, 	zDGN.u10FaultCode3);
		MsgCan_Update_PImage(VAR_DGN_130553_ACTIVE_FAULT_CODE_4, 	zDGN.u10FaultCode4);

        // Reload heart beat timer
    	msgcan_u16HeartBeatHeater = 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130555_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130555 (Heater scheduling feedback)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130555_RxHandle
(
    uint8                u8CanPort,     // in: CAN port the message was received from
    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
	PROPDGN_zDGN_130555 zDGN;
    PROPDGN_DGN_130555_Extract( &zDGN,  pzRxMailbox->Data );

    // Valid instance?
    if ( zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_1 )
    {
    	// Copy content to the application process image
    	MsgCan_Update_PImage( VAR_DGN_130555_AIR_HTR_ON_STAT,      zDGN.u2AirHtrTimerOnStat  );
    	MsgCan_Update_PImage( VAR_DGN_130555_AIR_HTR_ON_HOUR,      zDGN.u6AirHtrTimerOnHour  );
    	MsgCan_Update_PImage( VAR_DGN_130555_AIR_HTR_ON_MIN,       zDGN.u8AirHtrTimerOnMin   );
    	MsgCan_Update_PImage( VAR_DGN_130555_AIR_HTR_OFF_STAT,     zDGN.u2AirHtrTimerOffStat );
    	MsgCan_Update_PImage( VAR_DGN_130555_AIR_HTR_OFF_HOUR,     zDGN.u6AirHtrTimerOffHour );
    	MsgCan_Update_PImage( VAR_DGN_130555_AIR_HTR_OFF_MIN,      zDGN.u8AirHtrTimerOffMin  );
    	MsgCan_Update_PImage( VAR_DGN_130555_WTR_HTR_ON_STAT,      zDGN.u2WtrHtrTimerOnStat  );
    	MsgCan_Update_PImage( VAR_DGN_130555_WTR_HTR_ON_HOUR,      zDGN.u6WtrHtrTimerOnHour  );
    	MsgCan_Update_PImage( VAR_DGN_130555_WTR_HTR_ON_MIN,       zDGN.u8WtrHtrTimerOnMin   );
    	MsgCan_Update_PImage( VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME, zDGN.u8WtrHtrKeepOnTime   );

        // Reload heart beat timer
    	msgcan_u16HeartBeatHeater = 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130556_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130556 (HMI status)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130556_RxHandle
(
	uint8                u8CanPort,     // in: CAN port the message was received from
	NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
	PROPDGN_zDGN_130556 zDGN;
    PROPDGN_DGN_130556_Extract( &zDGN,  pzRxMailbox->Data );

    // Valid instance?
    if ( zDGN.u8HMIInstance == RVCDGN_FUNC_INSTANCE_1 )
    {
        // Copy content to the application process image
    	PIMAGE_SetValue( VAR_DGN_130556_ROOM_TEMP,      zDGN.i16RoomTemperature    );
    	PIMAGE_SetValue( VAR_DGN_130556_HEATER_COMM,    zDGN.u2HeaterCommunication );
    	PIMAGE_SetValue( VAR_DGN_130556_INPUT_VOLT,     zDGN.u2InputVoltage        );
    	PIMAGE_SetValue( VAR_DGN_130556_INST_STATUS,    zDGN.u2HMIInstanceStatus   );
    	PIMAGE_SetValue( VAR_DGN_130556_INT_CIRCUITERY, zDGN.u2InternalCircuitry   );
    	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_FAV,     zDGN.u2FavoriteButton      );
    	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_MENU,    zDGN.u2MenuButton          );
    	PIMAGE_SetValue( VAR_DGN_130556_BUTTON_HOME,    zDGN.u2HomeButton          );

        // Reload heart beat timer
    	msgcan_u16HeartBeatHMI= 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130557_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130557 (Heater status)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130557_RxHandle
(
    uint8                u8CanPort,     // in: CAN port the message was received from
    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
	PROPDGN_zDGN_130557 zDGN;
    PROPDGN_DGN_130557_Extract( &zDGN,  pzRxMailbox->Data );

	//LOG(I, "MsgCan_DGN_130557_RxHandle");

    // Valid instance?
    if ( zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_1 )
    {
        // Copy content to the application process image
    	MsgCan_Update_PImage( VAR_DGN_130557_ROOM_TEMP,       zDGN.i16RoomTemp      );
    	MsgCan_Update_PImage( VAR_DGN_130557_WATER_TEMP,      zDGN.u3WaterTemp      );
    	PIMAGE_SetValue( VAR_DGN_130557_GAS_HEATER_AIR,       zDGN.u4GasHeaterAir   ); // Not used in HMI
    	MsgCan_Update_PImage( VAR_DGN_130557_GAS_HEATER_WTR,  zDGN.u4GasHeaterWater );
    	MsgCan_Update_PImage( VAR_DGN_130557_AC_PRESENT,      zDGN.u2ACPresent      );
    	PIMAGE_SetValue( VAR_DGN_130557_AC_HEATER_AIR,        zDGN.u3ACHeaterAir    ); // Not used in HMI
    	MsgCan_Update_PImage( VAR_DGN_130557_AC_HEATER_WTR,   zDGN.u3ACHeaterWater  );

        // Reload heart beat timer
    	msgcan_u16HeartBeatHeater = 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130558_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130558 (heater operation commands)
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130558_RxHandle
(
    uint8                u8CanPort,     // in: CAN port the message was received from
    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
	PROPDGN_zDGN_130558 zDGN;
    PROPDGN_DGN_130558_Extract( &zDGN,  pzRxMailbox->Data );

    // Valid instance and valid message type?
    if ( ( zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_ALL) ||
         ( zDGN.u8HeaterInstance == msgcan_NMEA2KParameter.Name_Fields.Function_Instance) )

    {
        // Copy content to the application process image
    	PIMAGE_SetValue( VAR_DGN_130558_ENERGY_SOURCE,          zDGN.u4EnergySource           );
    	PIMAGE_SetValue( VAR_DGN_130558_TARGET_ROOM_TEMP,       zDGN.i16TargetRoomTemp        );
    	PIMAGE_SetValue( VAR_DGN_130558_SILENT_FAN_MAX,         zDGN.u4SilentModeMaxFan       );
    	PIMAGE_SetValue( VAR_DGN_130558_VENT_FAN_MIN,           zDGN.u4VentModeFanMin         );
    	PIMAGE_SetValue( VAR_DGN_130558_UNDERVOLT_THRES,        zDGN.u8UnderVoltThreshold     );
    	PIMAGE_SetValue( VAR_DGN_130558_AIR_HEATER_CMD,         zDGN.u2AirHeaterCmd           );
    	PIMAGE_SetValue( VAR_DGN_130558_AIR_HEATER_MODE,        zDGN.u4AirHeaterMode          );
    	PIMAGE_SetValue( VAR_DGN_130558_WTR_HEATER_CMD,         zDGN.u2WaterHeaterCmd         );
    	PIMAGE_SetValue( VAR_DGN_130558_WTR_HEATER_MODE,        zDGN.u4WaterHeaterMode        );
    	PIMAGE_SetValue( VAR_DGN_130558_SYSTEM_UNITS,           zDGN.u2SystemUnits            );

        // Reload heart beat timer
    	msgcan_u16HeartBeatHMI = 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130559_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130559 (heater operation feedback)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130559_RxHandle
( 
    uint8                u8CanPort,     // in: CAN port the message was received from
    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    PROPDGN_zDGN_130559 zDGN;

    // Extract message content
    PROPDGN_DGN_130559_Extract( &zDGN,  pzRxMailbox->Data );

	//LOG(I, "MsgCan_DGN_130559_RxHandle");

    // Valid instance and valid message type?
    if ( zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_1 )
    {
        // Copy content to the application process image
    	MsgCan_Update_PImage( VAR_DGN_130559_ENERGY_SOURCE,          zDGN.u4EnergySource              );
    	MsgCan_Update_PImage( VAR_DGN_130559_AIR_HEATER_CMD,         zDGN.u2AirHeaterCmd              );
    	MsgCan_Update_PImage( VAR_DGN_130559_WTR_HEATER_CMD,         zDGN.u2WaterHeaterCmd            );
    	MsgCan_Update_PImage( VAR_DGN_130559_AIR_HEATER_MODE,        zDGN.u4AirHeaterMode             );
    	MsgCan_Update_PImage( VAR_DGN_130559_WTR_HEATER_MODE,        zDGN.u4WaterHeaterMode           );
    	MsgCan_Update_PImage( VAR_DGN_130559_TARGET_ROOM_TEMP,       zDGN.i16TargetRoomTemp           );
    	MsgCan_Update_PImage( VAR_DGN_130559_SILENT_FAN_MAX,         zDGN.u4SilentModeMaxFan          );
    	MsgCan_Update_PImage( VAR_DGN_130559_VENT_FAN_MIN,           zDGN.u4VentModeFanMin            );
    	MsgCan_Update_PImage( VAR_DGN_130559_UNDERVOLT_THRES,        zDGN.u8UnderVoltThreshold        );
    	MsgCan_Update_PImage( VAR_DGN_130559_SYSTEM_UNITS,           zDGN.u2SystemUnits               );

        // Reload heart beat timer
    	msgcan_u16HeartBeatHeater = 0;
    }
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130762_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130762 (DM_RV)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
static void MsgCan_DGN_130762_RxHandle
(
	uint8                u8CanPort,     // in: CAN port the message was received from
	NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
	RVCDGN_zDGN_130762_Session zSession;
	RVCDGN_zDGN_130762_Header  zHeader;
	RVCDGN_zDGN_130762_DTC     zDTC;
	uint8                      u8Count;

    // Extract message content
	RVCDGN_DGN_130762_ExtractInit( &zSession, pzRxMailbox->Data, pzRxMailbox->MsgSize );
	RVCDGN_DGN_130762_ExtractHeader( &zSession, &zHeader );

	// TODO: If needed, process received operating status & lamps from DM_RV (see zHeader members)
	u8Count = 0;
	while ( RVCDGN_DGN_130762_ExtractDTC( &zSession, &zDTC ) )
	{
		// TODO: If needed, process DTC information (see zDTC members)
	    u8Count++;
	}
}

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130972_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 130972 (Thermostat ambient status)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130972_RxHandle
(
	uint8                u8CanPort,     // in: CAN port the message was received from
	NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
    RVCDGN_zDGN_130972 zDGN;
    RVCDGN_DGN_130972_Extract( &zDGN,  pzRxMailbox->Data );

    // Valid instance and valid message type?
    if ( zDGN.u8Instance == RVCDGN_FUNC_INSTANCE_1 )
    {
    	// Get signals (application inputs)
    	PIMAGE_SetValue( VAR_DGN_130972_AMBIENT_TEMP, zDGN.u16AmbientTemp );
    }

    // Reload heart beat timer
    msgcan_u16HeartBeatHMI = 0;

}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_131070_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 131070 (Date time command)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_131070_RxHandle
(
	    uint8                u8CanPort,     // in: CAN port the message was received from
	    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
    RVCDGN_zDGN_131070 zDGN;
    RVCDGN_DGN_131070_Extract( &zDGN,  pzRxMailbox->Data );

    // Get signals (application inputs)
	PIMAGE_SetValue( VAR_DGN_131070_YEAR,        zDGN.u8Year       );
	PIMAGE_SetValue( VAR_DGN_131070_MONTH,       zDGN.u8Month      );
	PIMAGE_SetValue( VAR_DGN_131070_DAY,         zDGN.u8Day        );
	PIMAGE_SetValue( VAR_DGN_131070_DAY_OF_WEEK, zDGN.u8DayOfWeek  );
	PIMAGE_SetValue( VAR_DGN_131070_HOUR,        zDGN.u8Hour       );
	PIMAGE_SetValue( VAR_DGN_131070_MINUTE,      zDGN.u8Minute     );
	PIMAGE_SetValue( VAR_DGN_131070_SECOND,      zDGN.u8Second     );
	PIMAGE_SetValue( VAR_DGN_131070_TIMEZONE,    zDGN.u8TimeZone   );

    // Reload heart beat timer
	msgcan_u16HeartBeatHMI = 0;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_131071_RxHandle
//------------------------------------------------------------------------------
// Description: Process received RVC DGN 131071 (Date time status)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_131071_RxHandle
(
	    uint8                u8CanPort,     // in: CAN port the message was received from
	    NMEA2K_RxMsg_Struct* pzRxMailbox    // in: Receive mailbox pointer
)
{
    // Extract message content
    RVCDGN_zDGN_131071 zDGN;
    RVCDGN_DGN_131071_Extract( &zDGN,  pzRxMailbox->Data );

    // Get signals (application inputs)
	MsgCan_Update_PImage( VAR_DGN_131071_YEAR,        zDGN.u8Year       );
	MsgCan_Update_PImage( VAR_DGN_131071_MONTH,       zDGN.u8Month      );
	MsgCan_Update_PImage( VAR_DGN_131071_DAY,         zDGN.u8Day        );
	MsgCan_Update_PImage( VAR_DGN_131071_DAY_OF_WEEK, zDGN.u8DayOfWeek  );
	MsgCan_Update_PImage( VAR_DGN_131071_HOUR,        zDGN.u8Hour       );
	MsgCan_Update_PImage( VAR_DGN_131071_MINUTE,      zDGN.u8Minute     );

	if ((zDGN.u8Second % DGN_131071_SECOND_PUBLISH_INTERVAL) == 0)
	{	/* Publish new seconds value with interval of DGN_131071_SECOND_PUBLISH_INTERVAL */
		MsgCan_Update_PImage( VAR_DGN_131071_SECOND,  zDGN.u8Second     );
	}
	else
	{
		/* Just store seconds value don't publish */
		PIMAGE_SetValue ( VAR_DGN_131071_SECOND,      zDGN.u8Second     );
	}

	MsgCan_Update_PImage( VAR_DGN_131071_TIMEZONE,    zDGN.u8TimeZone   );

	// Reload heart beat timer
	msgcan_u16HeartBeatHeater = 0;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130554_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130554 (Heater scheduling command)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130554_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    PROPDGN_zDGN_130554 zDGN;
    zDGN.u8HeaterInstance     = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u2AirHtrTimerOnStat  = (uint8)PIMAGE_GetValue( VAR_DGN_130554_AIR_HTR_ON_STAT  );
	zDGN.u6AirHtrTimerOnHour  = (uint8)PIMAGE_GetValue( VAR_DGN_130554_AIR_HTR_ON_HOUR  );
	zDGN.u8AirHtrTimerOnMin   = (uint8)PIMAGE_GetValue( VAR_DGN_130554_AIR_HTR_ON_MIN   );
	zDGN.u2AirHtrTimerOffStat = (uint8)PIMAGE_GetValue( VAR_DGN_130554_AIR_HTR_OFF_STAT );
	zDGN.u6AirHtrTimerOffHour = (uint8)PIMAGE_GetValue( VAR_DGN_130554_AIR_HTR_OFF_HOUR );
	zDGN.u8AirHtrTimerOffMin  = (uint8)PIMAGE_GetValue( VAR_DGN_130554_AIR_HTR_OFF_MIN  );
	zDGN.u2WtrHtrTimerOnStat  = (uint8)PIMAGE_GetValue( VAR_DGN_130554_WTR_HTR_ON_STAT  );
	zDGN.u6WtrHtrTimerOnHour  = (uint8)PIMAGE_GetValue( VAR_DGN_130554_WTR_HTR_ON_HOUR  );
	zDGN.u8WtrHtrTimerOnMin   = (uint8)PIMAGE_GetValue( VAR_DGN_130554_WTR_HTR_ON_MIN   );
	zDGN.u8WtrHtrKeepOnTime   = (uint8)PIMAGE_GetValue( VAR_DGN_130554_WTR_HTR_KEEP_ON_TIME  );

	// Stuff message data
    PROPDGN_DGN_130554_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130553_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130555 (Heater scheduling feedback)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130553_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    PROPDGN_zDGN_130553 zDGN;
    zDGN.u2WarningFault   = FALSE;
    zDGN.u2CriticalFault  = TRUE;
    zDGN.u10FaultCode1    = 1001;
    zDGN.u10FaultCode2    = 1022;
    zDGN.u10FaultCode3    = 1023;
    zDGN.u10FaultCode4    = 1004;

	// Stuff message data
    PROPDGN_DGN_130553_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130555_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130555 (Heater scheduling feedback)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130555_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    PROPDGN_zDGN_130555 zDGN;
    zDGN.u8HeaterInstance     = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u2AirHtrTimerOnStat  = (uint8)PIMAGE_GetValue( VAR_DGN_130555_AIR_HTR_ON_STAT  );
	zDGN.u6AirHtrTimerOnHour  = (uint8)PIMAGE_GetValue( VAR_DGN_130555_AIR_HTR_ON_HOUR  );
	zDGN.u8AirHtrTimerOnMin   = (uint8)PIMAGE_GetValue( VAR_DGN_130555_AIR_HTR_ON_MIN   );
	zDGN.u2AirHtrTimerOffStat = (uint8)PIMAGE_GetValue( VAR_DGN_130555_AIR_HTR_OFF_STAT );
	zDGN.u6AirHtrTimerOffHour = (uint8)PIMAGE_GetValue( VAR_DGN_130555_AIR_HTR_OFF_HOUR );
	zDGN.u8AirHtrTimerOffMin  = (uint8)PIMAGE_GetValue( VAR_DGN_130555_AIR_HTR_OFF_MIN  );
	zDGN.u2WtrHtrTimerOnStat  = (uint8)PIMAGE_GetValue( VAR_DGN_130555_WTR_HTR_ON_STAT  );
	zDGN.u6WtrHtrTimerOnHour  = (uint8)PIMAGE_GetValue( VAR_DGN_130555_WTR_HTR_ON_HOUR  );
	zDGN.u8WtrHtrTimerOnMin   = (uint8)PIMAGE_GetValue( VAR_DGN_130555_WTR_HTR_ON_MIN   );
	zDGN.u8WtrHtrKeepOnTime   = (uint8)PIMAGE_GetValue( VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME  );

	// Stuff message data
    PROPDGN_DGN_130555_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130556_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130556 (HMI status)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130556_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    PROPDGN_zDGN_130556 zDGN;
    zDGN.u8HMIInstance          = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.i16RoomTemperature     = PIMAGE_GetValue( VAR_DGN_130556_ROOM_TEMP      );
	zDGN.u2HeaterCommunication  = PIMAGE_GetValue( VAR_DGN_130556_HEATER_COMM    );
	zDGN.u2InputVoltage         = PIMAGE_GetValue( VAR_DGN_130556_INPUT_VOLT     );
	zDGN.u2HMIInstanceStatus    = PIMAGE_GetValue( VAR_DGN_130556_INST_STATUS    );
	zDGN.u2InternalCircuitry    = PIMAGE_GetValue( VAR_DGN_130556_INT_CIRCUITERY );
	zDGN.u2FavoriteButton       = PIMAGE_GetValue( VAR_DGN_130556_BUTTON_FAV );
	zDGN.u2MenuButton           = PIMAGE_GetValue( VAR_DGN_130556_BUTTON_MENU );
	zDGN.u2HomeButton           = PIMAGE_GetValue( VAR_DGN_130556_BUTTON_HOME );

	// Stuff message data
    PROPDGN_DGN_130556_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_65242_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 65242 (Software identification)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
static void MsgCan_DGN_65242_TxHandle
(
	uint8                u8CanPort,  // in: CAN port the message must be sent to
	NMEA2K_TxMsg_Struct* pzTxMailbox // in: Transmission mailbox pointer
)
{
	RVCDGN_zDGN_65242 zDGN;

	// Prepare signals
	RVCDGN_DGN_65242_Set( &zDGN, (const char *)&SWId[0][0], (const char *)&SWId[1][0], (const char *)&SWId[2][0],  (const char *)&SWId[3][0]);

	// Stuff message data
	pzTxMailbox->DataSize =  RVCDGN_DGN_65242_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_65259_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 65259 (Product identification)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
static void MsgCan_DGN_65259_TxHandle
(
	uint8                u8CanPort,  // in: CAN port the message must be sent to
	NMEA2K_TxMsg_Struct* pzTxMailbox // in: Transmission mailbox pointer
)
{
	RVCDGN_zDGN_65259 zDGN;

	// Prepare signals
	RVCDGN_DGN_65259_Set( &zDGN, MSGCAN_PRODINFO_MAKE, MSGCAN_PRODINFO_MODEL, MSGCAN_PRODINFO_SERIAL, MSGCAN_PRODINFO_UNIT);

	// Stuff message data
	pzTxMailbox->DataSize =  RVCDGN_DGN_65259_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130557_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130557 (Heater status)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130557_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
    PROPDGN_zDGN_130557 zDGN;

	// Prepare signals (application outputs)
    zDGN.u8HeaterInstance = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u2GasHeaterAir   = PIMAGE_u8GetValue( VAR_DGN_130557_GAS_HEATER_AIR  );
    zDGN.u2GasHeaterWater = PIMAGE_u8GetValue( VAR_DGN_130557_GAS_HEATER_WTR  );
    zDGN.u2ACPresent      = PIMAGE_u8GetValue( VAR_DGN_130557_AC_PRESENT      );
    zDGN.u2ACHeaterAir    = PIMAGE_u8GetValue( VAR_DGN_130557_AC_HEATER_AIR   );
    zDGN.u2ACHeaterWater  = PIMAGE_u8GetValue( VAR_DGN_130557_AC_HEATER_WTR   );

    // Stuff message data
    PROPDGN_DGN_130557_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130558_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130558 (heater operation command)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130558_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    PROPDGN_zDGN_130558 zDGN;
    zDGN.u8HeaterInstance            = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u4EnergySource              = PIMAGE_u8GetValue(  VAR_DGN_130558_ENERGY_SOURCE    );
    zDGN.u2AirHeaterCmd              = PIMAGE_u8GetValue(  VAR_DGN_130558_AIR_HEATER_CMD   );
    zDGN.u2WaterHeaterCmd            = PIMAGE_u8GetValue(  VAR_DGN_130558_WTR_HEATER_CMD   );
    zDGN.u4AirHeaterMode             = PIMAGE_u8GetValue(  VAR_DGN_130558_AIR_HEATER_MODE  );
    zDGN.u4WaterHeaterMode           = PIMAGE_u8GetValue(  VAR_DGN_130558_WTR_HEATER_MODE  );
    zDGN.i16TargetRoomTemp           = PIMAGE_s16GetValue( VAR_DGN_130558_TARGET_ROOM_TEMP );
    zDGN.u4SilentModeMaxFan          = PIMAGE_u8GetValue(  VAR_DGN_130558_SILENT_FAN_MAX   );
    zDGN.u4VentModeFanMin            = PIMAGE_u8GetValue(  VAR_DGN_130558_VENT_FAN_MIN     );
    zDGN.u8UnderVoltThreshold        = PIMAGE_u8GetValue(  VAR_DGN_130558_UNDERVOLT_THRES  );
    zDGN.u2SystemUnits               = PIMAGE_u8GetValue(  VAR_DGN_130558_SYSTEM_UNITS     );

    // Stuff message data
    PROPDGN_DGN_130558_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130559_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130559 (heater operation feedback)
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_130559_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    PROPDGN_zDGN_130559 zDGN;
    zDGN.u8HeaterInstance         = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u4EnergySource           = PIMAGE_u8GetValue(  VAR_DGN_130559_ENERGY_SOURCE          );
    zDGN.u2AirHeaterCmd           = PIMAGE_u8GetValue(  VAR_DGN_130559_AIR_HEATER_CMD         );
    zDGN.u2WaterHeaterCmd         = PIMAGE_u8GetValue(  VAR_DGN_130559_WTR_HEATER_CMD         );
    zDGN.u4AirHeaterMode          = PIMAGE_u8GetValue(  VAR_DGN_130559_AIR_HEATER_MODE        );
    zDGN.u4WaterHeaterMode        = PIMAGE_u8GetValue(  VAR_DGN_130559_WTR_HEATER_MODE        );
    zDGN.i16TargetRoomTemp        = PIMAGE_s16GetValue( VAR_DGN_130559_TARGET_ROOM_TEMP       );
    zDGN.u4SilentModeMaxFan       = PIMAGE_u8GetValue(  VAR_DGN_130559_SILENT_FAN_MAX         );
    zDGN.u4VentModeFanMin         = PIMAGE_u8GetValue(  VAR_DGN_130559_VENT_FAN_MIN           );
    zDGN.u8UnderVoltThreshold     = PIMAGE_u8GetValue(  VAR_DGN_130559_UNDERVOLT_THRES        );
    zDGN.u2SystemUnits            = PIMAGE_u8GetValue(  VAR_DGN_130559_SYSTEM_UNITS           );

    // Stuff message data
    PROPDGN_DGN_130559_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130762_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130762 (DM_RV)
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
static void MsgCan_DGN_130762_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	RVCDGN_zDGN_130762_Session zDGN;
	RVCDGN_zDGN_130762_Header  zHeader;
	RVCDGN_zDGN_130762_DTC     zDTC[RVCDGN_DGN_130762_DTC_CAPACITY];
	uint8 u8Index, u8DtcCount;

	// Prepare header
	// TODO: Set operating status & lamps using the application faults
	zHeader.u2OperatingStatus1 = 1;
	zHeader.u2OperatingStatus2 = 1;
	zHeader.u2RedLampStatus    = 1;
	zHeader.u2YellowLampStatus = 1;
	zHeader.u8DSA              = MSGCAN_DSA;

	// Prepare faults
	// TODO: Set faults according to the application
	zDTC[0].u32SPN         = 1234;
	zDTC[0].u8DSAExtension = 0xFF;
	zDTC[0].u8FMI          = RVCDGN_FMI_ABOVE_NORMAL_RANGE;
	zDTC[0].u8OccurCount   = 1;
	zDTC[1].u32SPN         = 2345;
	zDTC[1].u8DSAExtension = 0xFF;
	zDTC[1].u8FMI          = RVCDGN_FMI_ABOVE_NORMAL_RANGE;
	zDTC[1].u8OccurCount   = 1;
	// [...]
	u8DtcCount             = 2;

    // Stuff message data
	RVCDGN_DGN_130762_StuffInit(   &zDGN, pzTxMailbox->Data, RVCDGN_DGN_130762_SIZE );
	RVCDGN_DGN_130762_StuffHeader( &zDGN, &zHeader);
	for (u8Index=0; u8Index<u8DtcCount; u8Index++)
	{
		RVCDGN_DGN_130762_StuffDTC( &zDGN, &zDTC[u8Index] );
	}
	RVCDGN_DGN_130762_StuffFooter(&zDGN);

    // Flag message as ready to send
	pzTxMailbox->DataSize = RVCDGN_DGN_130762_GetSize( &zDGN );
    pzTxMailbox->TxReady  = TRUE;
}


//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_130972_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 130972 (Thermostat Ambient Temperature)
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_130972_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    RVCDGN_zDGN_130972 zDGN;
    zDGN.u8Instance     = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u16AmbientTemp = PIMAGE_u16GetValue( VAR_DGN_130972_AMBIENT_TEMP );

    // Stuff message data
    RVCDGN_DGN_130972_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_131070_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 131070 (Date Time Command)
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HMI
static void MsgCan_DGN_131070_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	/* This is workaround, heater will update it's second value only if minute or hour is changed in the same frame.
	 * So here we update minutes value to last heater minute value sent if seconds are changed.
	 */
	if ((PIMAGE_u8GetValue( VAR_DGN_131070_SECOND) != 0xff)                &&
	    (PIMAGE_u8GetValue( VAR_DGN_131070_MINUTE) == NMEA2K_UINT8_NO_DATA) &&
	    (PIMAGE_u8GetValue( VAR_DGN_131070_HOUR)   == NMEA2K_UINT8_NO_DATA))
	{
		PIMAGE_SetValue(VAR_DGN_131070_MINUTE, PIMAGE_u8GetValue( VAR_DGN_131071_MINUTE));
		
	}

	// Prepare signals (application outputs)
    RVCDGN_zDGN_131070 zDGN;
    zDGN.u8Year      = PIMAGE_u8GetValue( VAR_DGN_131070_YEAR        );
    zDGN.u8Month     = PIMAGE_u8GetValue( VAR_DGN_131070_MONTH       );
    zDGN.u8Day       = PIMAGE_u8GetValue( VAR_DGN_131070_DAY         );
    zDGN.u8DayOfWeek = PIMAGE_u8GetValue( VAR_DGN_131070_DAY_OF_WEEK );
    zDGN.u8Hour      = PIMAGE_u8GetValue( VAR_DGN_131070_HOUR        );
    zDGN.u8Minute    = PIMAGE_u8GetValue( VAR_DGN_131070_MINUTE      );
    zDGN.u8Second    = PIMAGE_u8GetValue( VAR_DGN_131070_SECOND      );
    zDGN.u8TimeZone  = PIMAGE_u8GetValue( VAR_DGN_131070_TIMEZONE    );

    // Stuff message data
    RVCDGN_DGN_131071_Stuff( pzTxMailbox->Data, &zDGN );

    // Transmitted: reset
    MsgCan_DGN_131070_Reset();

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_DGN_131071_TxHandle
//------------------------------------------------------------------------------
// Description: Prepare transmission of RVC DGN 131071 (Date Time Status)
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
#if MSGCAN_NODE_TYPE_SELF==MSGCAN_NODE_TYPE_HEATER
static void MsgCan_DGN_131071_TxHandle
(
    uint8                u8CanPort,   // in: CAN port the message must be sent to
    NMEA2K_TxMsg_Struct* pzTxMailbox  // in: Transmission mailbox pointer
)
{
	// Prepare signals (application outputs)
    RVCDGN_zDGN_131071 zDGN;
    zDGN.u8Year      = PIMAGE_u8GetValue( VAR_DGN_131071_YEAR        );
    zDGN.u8Month     = PIMAGE_u8GetValue( VAR_DGN_131071_MONTH       );
    zDGN.u8Day       = PIMAGE_u8GetValue( VAR_DGN_131071_DAY         );
    zDGN.u8DayOfWeek = PIMAGE_u8GetValue( VAR_DGN_131071_DAY_OF_WEEK );
    zDGN.u8Hour      = PIMAGE_u8GetValue( VAR_DGN_131071_HOUR        );
    zDGN.u8Minute    = PIMAGE_u8GetValue( VAR_DGN_131071_MINUTE      );
    zDGN.u8Second    = PIMAGE_u8GetValue( VAR_DGN_131071_SECOND      );
    zDGN.u8TimeZone  = PIMAGE_u8GetValue( VAR_DGN_131071_TIMEZONE    );

    // Stuff message data
    RVCDGN_DGN_131071_Stuff( pzTxMailbox->Data, &zDGN );

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgCan_Update_PImage
//------------------------------------------------------------------------------
// Description: Update PImage if value has changed and call callback function
//              to further process changed value
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
static void MsgCan_Update_PImage(PIMAGE_eTABLE eIndex, int32 i32Value)
{
	EventBits_t event;
	int evaluate = 0;

	// Get parameter value
	int32 curVal = PIMAGE_GetValue(eIndex);

#if 0
	if (eIndex >= 4 && eIndex <= 12)
	{
		LOG(I, "MsgCan_Update_PImage, idx=%d, cur=%d, new=%d", eIndex, (int)curVal, (int)i32Value);
	}
#endif

	// Special handling for target room temp.
	// Callback is blocked by connector until 3 seconds has passed from last change.
	// If callback is not blocked then it can be called.
	if (eIndex == VAR_DGN_130559_TARGET_ROOM_TEMP)
	{
		// Check for event but do not block (ticks to wait = 0)
		event = xEventGroupWaitBits(callback_accept_handle, CB_TARGET_TEMP_NOT_BLOCKED, pdFALSE, pdFALSE, 0);
		if ((event & CB_TARGET_TEMP_NOT_BLOCKED) == CB_TARGET_TEMP_NOT_BLOCKED)
		{
			evaluate = 1;
		}
	}
	else
	{
		evaluate = 1;
	}
	
	// Check if values differ
	if (evaluate && (i32Value != curVal))
	{
#if PREM_RVC_LOG
		int32_t ddm_parameter;
		uint8_t name_buffer[32];
		size_t name_length = sizeof(name_buffer);

		ddm_parameter = get_parameter_by_pimage_index(eIndex);
		LOG(I, "MsgCan_Update_PImage differs for %s, from %d to %d",ddm2_parameter_name(ddm_parameter, name_buffer, &name_length), \
																	(uint32_t)i32Value, (uint32_t)curVal);
#endif /* PREM_RVC_LOG */
		// Value has changed so set new value
		PIMAGE_SetValue(eIndex, i32Value);

		// Check if callback is registered
		if (parameterFuncCb != NULL)
		{
			//LOG(I, "MsgCan_Update_PImage calling callback function");
			// Call registered callback function
			parameterFuncCb(eIndex, i32Value);
		}
	}
}

#endif // CONNECTOR_PREM_RVC