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
#ifdef CONNECTOR_PREM_RVC

#include "RVCDGN.h"

// Application
#include "PropDGN.h"     // Interface to Proprietary RVC definitions.
#include "PImage.h"		 // Process Image interface.

// Added by Dometic Solna
#include "configuration.h"
// End added

//------------------------------------------------------------------------------
// Global definitions
//------------------------------------------------------------------------------
#define MSGCAN_TEST_MODE           FALSE                    // Keep it FALSE
#define MSGCAN_HEARTBEAT_TIMEOUT   5000                     // TODO: Select node heartbeat timeout [ms]
#define MSGCAN_NODE_TYPE_HMI       1
#define MSGCAN_NODE_TYPE_HEATER    2
#define MSGCAN_NODE_TYPE_SELF      MSGCAN_NODE_TYPE_HMI     // TODO: Select node type (optional: remove code for the unused node)

// TODO: Set MSGCAN_PRODINFO to production information configuration
#define MSGCAN_PRODINFO_MAKE       "MyMake"
#define MSGCAN_PRODINFO_MODEL      "MyModel"
#define MSGCAN_PRODINFO_SERIAL     "MySN"
#define MSGCAN_PRODINFO_UNIT       "MyUnitNumber"

#define MSGCAN_SWINFO_ID1          "MySW1"
#define MSGCAN_SWINFO_ID2          "MySW2"
#define MSGCAN_SWINFO_ID3          "MySW3"
#define MSGCAN_SWINFO_ID4          "MySW4"


// Added by Dometic SOLNA
extern EventGroupHandle_t callback_accept_handle;

extern const int CB_TARGET_TEMP_NOT_BLOCKED;
// End Added

//------------------------------------------------------------------------------
// Global types
//------------------------------------------------------------------------------
typedef void (*parameter_changed_cb_t)(PIMAGE_eTABLE eIndex, int32 i32Value);

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
	uint8  u8SrcAddr,    // in : Preferred source address
	uint32 u32NameId,    // in : NMEA 2000 identity number
	uint8  u8Instance,    // in : Function instance
	parameter_changed_cb_t funcCb // in: Application function cb
);

//-----------------------------------------------------------------------------
// Function:    MSGCAN_Process
//-----------------------------------------------------------------------------
// Description: Process all transmit function handlers and detect faults.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
extern void MSGCAN_Process
(
	uint16 u16Elapsed   // in : Elapsed time since the last call [ms]
);

//-----------------------------------------------------------------------------
// Function:    MSGCAN_UpdateSWId
//-----------------------------------------------------------------------------
// Description: Update SWId for index 0-3.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
extern void MSGCAN_UpdateSWId(int index, void *data, int length);

#endif // CONNECTOR_PREM_RVC
#endif  // MSGCAN_H
