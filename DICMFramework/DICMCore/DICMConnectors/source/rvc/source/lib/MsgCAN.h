//------------------------------------------------------------------------------
// Module:      MsgCAN.h
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
#ifndef MSGCAN_H
#define MSGCAN_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
// Libraries
#include "configuration.h"

#include "RVCDGN.h"

// Application
#include "PropDGN.h"     // Interface to Proprietary RVC definitions.
#include "PImage.h"		 // Process Image interface.

// End added

//------------------------------------------------------------------------------
// Global definitions
//------------------------------------------------------------------------------
#define MSGCAN_TEST_MODE           FALSE                    // Keep it FALSE
#define MSGCAN_HEARTBEAT_TIMEOUT   5000                     // TODO: Select node heartbeat timeout [ms]
#define MSGCAN_NODE_TYPE_HMI       1
#define MSGCAN_NODE_TYPE_HEATER    2
#ifndef MSGCAN_NODE_TYPE_SELF
#define MSGCAN_NODE_TYPE_SELF      MSGCAN_NODE_TYPE_HMI     // TODO: Select node type (optional: remove code for the unused node)
#endif

// TODO: Set MSGCAN_PRODINFO to production information configuration
#ifndef MSGCAN_PRODINFO_MAKE
#define MSGCAN_PRODINFO_MAKE       "MyMake"
#endif
#ifndef MSGCAN_PRODINFO_MODEL
#define MSGCAN_PRODINFO_MODEL      "MyModel"
#endif
#ifndef MSGCAN_PRODINFO_SERIAL
#define MSGCAN_PRODINFO_SERIAL     "MySN"
#endif
#ifndef MSGCAN_PRODINFO_UNIT
#define MSGCAN_PRODINFO_UNIT       "MyUnitNumber"
#endif

#ifndef MSGCAN_SWINFO_ID1
#define MSGCAN_SWINFO_ID1          "MySW1"
#endif
#ifndef MSGCAN_SWINFO_ID2
#define MSGCAN_SWINFO_ID2          "MySW2"
#endif
#ifndef MSGCAN_SWINFO_ID3
#define MSGCAN_SWINFO_ID3          "MySW3"
#endif
#ifndef MSGCAN_SWINFO_ID4
#define MSGCAN_SWINFO_ID4          "MySW4"
#endif

// Operation codes for 0xEF00 - 61184
typedef enum
{
	DGN61184_Oper0x42 = 0x42,
	DGN61184_Oper0x43 = 0x43,
	DGN61184_Oper0x55 = 0x55
} DGN61184_OperID;

typedef enum
{
    MSGCAN_LIB_STATUS_NONE = 0,
    MSGCAN_LIB_STATUS_ADDRESS_CLAIMED_UPDATE = 1,
	MSGCAN_LIB_STATUS_PROP_DGN_TX_READY = 2
} msgcan_lib_status_type_t;

// Added by Dometic SOLNA
extern EventGroupHandle_t callback_accept_handle;

extern const int CB_TARGET_TEMP_NOT_BLOCKED;
// End Added

//------------------------------------------------------------------------------
// Global types
//------------------------------------------------------------------------------
typedef void (*parameter_changed_cb_t)(PIMAGE_eTABLE eIndex, int32 i32Value);
typedef void (*status_changed_cb_t)(msgcan_lib_status_type_t status_type, uint32_t value);

typedef bool (*rx_cb_t)(uint32_t pgn, uint8_t sa, uint8_t *p_data, size_t size);
typedef bool (*tx_cb_t)(uint32_t pgn, uint8_t instance, uint8_t *p_data);

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Global function prototypes
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function:    MSGCAN_Initialize
//-----------------------------------------------------------------------------
// Description: Initialization of EPS receive and transmit mail boxes.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
extern void MSGCAN_Initialize
( 
	uint8_t  u8SrcAddr,    // in : Preferred source address
	uint32_t u32NameId,    // in : NMEA 2000 identity number
	uint8_t  u8Instance,    // in : Function instance
	status_changed_cb_t status_changed_cb // in: Application function cb
);

/**
 * @brief Sets a global callback that will be called for every received dgn
 *
 * @param rx_callback Callback function
 */
void MSGCAN_SetRxCallback(rx_cb_t rx_callback);

/**
 * @brief Sets a global callback that will be called for every transmitted dgn
 *
 * @param tx_callback Callback function
 */
void MSGCAN_SetTxCallback(tx_cb_t tx_callback);

//-----------------------------------------------------------------------------
// Function:    MSGCAN_Process
//-----------------------------------------------------------------------------
// Description: Process all transmit function handlers and detect faults.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void MSGCAN_Process(uint16 u16Elapsed);   // in : Elapsed time since the last call [ms]

//-----------------------------------------------------------------------------
// Function:    MSGCAN_UpdateSWId
//-----------------------------------------------------------------------------
// Description: Update SWId for index 0-3.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void MSGCAN_UpdateSWId(int index, void *data, int length);

void MsgCan_Address_Claimed_Update(void);
void MsgCan_DGN_TxReady(uint32_t DGN, uint8_t msg_type_ready);

#endif  // MSGCAN_H
