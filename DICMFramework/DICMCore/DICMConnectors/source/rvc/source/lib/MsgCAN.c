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
#include <stdbool.h>
#include <string.h>

#include "configuration.h"

#include "NMEA2K.h"
#include "RVCDGN.h"

// Platform
#include "HALCAN.h"

// Application
#include "MsgCAN.h"
#include "PImage.h"
#include "PropDGN.h"
#include "rvc_int.h"
//------------------------------------------------------------------------------
// Local constants
//------------------------------------------------------------------------------
#define MSGCAN_ARBITRARY_ADDR_CAPABLE  TRUE  // Can use arbitrary SA to resolve a conflict
#define MSGCAN_INDUSTRY_GROUP          1     // J1939 TABLE B1: 1 = On-Highway Equipment
#define MSGCAN_VEHICLE_SYSTEM_INSTANCE 0     // First u8Instance
#define MSGCAN_VEHICLE_SYSTEM          0     // Non specific system
#define MSGCAN_RESERVED_FIELD          0     // Reserved by SAE, should be set to 0
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
#define MSGCAN_FUNCTION 50  // Auxiliary Heater #1 (J1939, not RV-C)
#else
#define MSGCAN_FUNCTION 21  // Not defined in RV-C. Used J1939 cab climate controller (used for thermostat)
#endif
#define MSGCAN_FUNCTION_INSTANCE 1    // First u8Instance
#define MSGCAN_ECU_INSTANCE      0    // First u8Instance
#define MSGCAN_SERIAL_NUMBER     0    // Serial number
#define MSGCAN_MANUFACTURER_CODE 103  // RVC TABLE 7.1
#ifndef MSGCAN_PREFERRED_ADDRESS
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
#define MSGCAN_PREFERRED_ADDRESS 94  // Preferred source address
#else
#define MSGCAN_PREFERRED_ADDRESS 88  // Preferred source address
#endif
#endif
#define MSGCAN_DSA             MSGCAN_PREFERRED_ADDRESS  // Default source address
#define MSGCAN_RX_MAILBOX_SIZE DIM(msgcan_zRxMailbox)    // EST receive mailbox size.
#define MSGCAN_TX_MAILBOX_SIZE DIM(msgcan_zTxMailbox)    // EST transmit mailbox size.

#define MAX_SW_INDEX 4

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
const int CB_TARGET_TEMP_NOT_BLOCKED = 0x00000001;
EventGroupHandle_t callback_accept_handle = NULL;

//------------------------------------------------------------------------------
// Local variables.
//------------------------------------------------------------------------------
static uint8_t msgcan_u8PreferredSA;  // Preferred source address
static uint16_t msgcan_u16HeartBeat;  // Heart beat  timer for heater messages
static uint8_t SWId[MAX_SW_INDEX][RVCDGN_DGN_65242_FIELD_SIZE] = {0};

//------------------------------------------------------------------------------
// Mailboxes
//------------------------------------------------------------------------------
// Receive buffers
static EXT_RAM_ATTR uint8_t MsgCan_DGN_65242_u8RxData[RVCDGN_DGN_65242_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_65259_u8RxData[RVCDGN_DGN_65259_SIZE + 1];    // last null
static EXT_RAM_ATTR uint8_t MsgCan_DGN_65259_u8RxData_2[RVCDGN_DGN_65259_SIZE + 1];  // last null
static EXT_RAM_ATTR uint8_t MsgCan_DGN_65259_u8RxData_3[RVCDGN_DGN_65259_SIZE + 1];  // last null
static EXT_RAM_ATTR uint8_t MsgCan_DGN_65259_u8RxData_4[RVCDGN_DGN_65259_SIZE + 1];  // last null
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130762_u8RxData[RVCDGN_DGN_130762_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130776_u8RxData[RVCDGN_DGN_130776_SIZE];
#ifdef RVC_CONFIG_IMPL_DOWNLOAD
static EXT_RAM_ATTR uint8_t MsgCan_DGN_97536_u8RxData[RVCDGN_DGN_97536_SIZE];  // DOWNLOAD - 0x17D00
#endif
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130554_u8RxData[PROPDGN_DGN_130554_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130556_u8RxData[PROPDGN_DGN_130556_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130558_u8RxData[PROPDGN_DGN_130559_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130972_u8RxData[RVCDGN_DGN_130972_SIZE];
#endif
#ifdef RVC_CONFIG_BUS_TIME_KEEP
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131070_u8RxData[RVCDGN_DGN_130972_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_SHARC_HTR
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130552_u8RxData[PROPDGN_DGN_130552_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130553_u8RxData[PROPDGN_DGN_130553_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130555_u8RxData[PROPDGN_DGN_130555_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130557_u8RxData[PROPDGN_DGN_130557_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130559_u8RxData[PROPDGN_DGN_130559_SIZE];
#endif
#ifdef RVC_CONFIG_BUS_TIME_USE
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131071_u8RxData[RVCDGN_DGN_131071_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131042_u8RxData[RVCDGN_DGN_131042_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130810_u8RxData[RVCDGN_DGN_130810_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130807_u8RxData[RVCDGN_DGN_130807_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130806_u8RxData[RVCDGN_DGN_130806_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130809_u8RxData[RVCDGN_DGN_130809_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130808_u8RxData[RVCDGN_DGN_130808_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130805_u8RxData[RVCDGN_DGN_130805_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130804_u8RxData[RVCDGN_DGN_130804_SIZE];
#endif
#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1) || defined(RVC_CONFIG_IMPL_DC_DIMMER_2)
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130779_u8RxData[RVCDGN_DGN_130779_SIZE];
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_1) || defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130778_u8RxData[RVCDGN_DGN_130778_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_PROPRIATARY
static EXT_RAM_ATTR uint8_t MsgCan_DGN_61184_u8RxData[PROPDGN_DGN_61184_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_ROOF_FAN
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130530_u8RxData[RVCDGN_DGN_130530_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130726_u8RxData[RVCDGN_DGN_130726_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_ROOF_FAN
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130531_u8RxData[RVCDGN_DGN_130531_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130727_u8RxData[RVCDGN_DGN_130727_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131069_u8RxData[RVCDGN_DGN_131069_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131068_u8RxData[RVCDGN_DGN_131068_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131067_u8RxData[RVCDGN_DGN_131067_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130761_u8RxData[RVCDGN_DGN_130761_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130760_u8RxData[RVCDGN_DGN_130760_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130759_u8RxData[RVCDGN_DGN_130759_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130732_u8RxData[RVCDGN_DGN_130732_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130731_u8RxData[RVCDGN_DGN_130731_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130730_u8RxData[RVCDGN_DGN_130730_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130729_u8RxData[RVCDGN_DGN_130729_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130725_u8RxData[RVCDGN_DGN_130725_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130552_u8RxData[RVCDGN_DGN_130552_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130535_u8RxData[RVCDGN_DGN_130535_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130551_u8RxData[RVCDGN_DGN_130551_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130549_u8RxData[RVCDGN_DGN_130549_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130512_u8RxData[RVCDGN_DGN_130512_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_BATTERY_SUMMARY
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130545_u8RxData[RVCDGN_DGN_130545_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131041_u8RxData[RVCDGN_DGN_131041_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130971_u8RxData[RVCDGN_DGN_130971_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_REFRIGERATOR
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130514_u8RxData[RVC_DGN_130514_SIZE];
#endif
static EXT_RAM_ATTR uint8_t MsgCan_DGN_Common_u8RxData[8];

// clang-format off
//------------------------------------------------------------------------------
// Receive handlers
static void MsgCan_DGN_Common_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
#ifdef RVC_CONFIG_IMPL_PROPRIATARY
static void MsgCan_DGN_61184_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
#endif
static void MsgCan_DGN_65242_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
static void MsgCan_DGN_65259_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
static void MsgCan_DGN_130554_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
static void MsgCan_DGN_130556_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
static void MsgCan_DGN_130558_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
#endif
#ifdef RVC_CONFIG_BUS_TIME_KEEP
static void MsgCan_DGN_131070_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
static void MsgCan_DGN_131042_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
static void MsgCan_DGN_130810_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
static void MsgCan_DGN_130807_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
static void MsgCan_DGN_130806_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
static void MsgCan_DGN_130972_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox);
#endif

//------------------------------------------------------------------------------
// Receive mailbox definition
static NMEA2K_RxMsg_Struct msgcan_zRxMailbox[] =
{
//    Enable Trunk Rx_InProg  DgnType    DGN     AgeCtr   MsgSize   OrgAddr OrgFil  DstAddr  BufferSize                DataBuffer                   Rx Handler
	{ 1,     0,    0,         0,         65242,  0,       0,        0,      0x5E,   0,        RVCDGN_DGN_65242_SIZE,   MsgCan_DGN_65242_u8RxData,   MsgCan_DGN_65242_RxHandle,   },
	{ 1,     0,    0,         0,         65259,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_65259_SIZE,   MsgCan_DGN_65259_u8RxData,   MsgCan_DGN_65259_RxHandle,   },
    { 1,     0,    0,         0,         65259,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_65259_SIZE,   MsgCan_DGN_65259_u8RxData_2,   MsgCan_DGN_65259_RxHandle,   },
    { 1,     0,    0,         0,         65259,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_65259_SIZE,   MsgCan_DGN_65259_u8RxData_3,   MsgCan_DGN_65259_RxHandle,   },
    { 1,     0,    0,         0,         65259,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_65259_SIZE,   MsgCan_DGN_65259_u8RxData_4,   MsgCan_DGN_65259_RxHandle,   },
	{ 1,     0,    0,         0,        130762,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_130762_SIZE,  MsgCan_DGN_130762_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
    { 1,     0,    0,         0,        130776,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_130776_SIZE,  MsgCan_DGN_130776_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#ifdef RVC_CONFIG_IMPL_DOWNLOAD
    { 1,     0,    0,         0,        97536,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_97536_SIZE,  MsgCan_DGN_97536_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
	
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
	{ 1,     0,    0,         0,        130554,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130554_SIZE,  MsgCan_DGN_130554_u8RxData,  MsgCan_DGN_130554_RxHandle,  },
	{ 1,     0,    0,         0,        130556,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130556_SIZE,  MsgCan_DGN_130556_u8RxData,  MsgCan_DGN_130556_RxHandle,  },
	{ 1,     0,    0,         0,        130558,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130558_SIZE,  MsgCan_DGN_130558_u8RxData,  MsgCan_DGN_130558_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
    { 1,     0,    0,         0,        130972,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_130972_SIZE,  MsgCan_DGN_130972_u8RxData,  MsgCan_DGN_130972_RxHandle,  },
#endif
#ifdef RVC_CONFIG_BUS_TIME_KEEP
    { 1,     0,    0,         0,        131070,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_131070_SIZE,  MsgCan_DGN_131070_u8RxData,  MsgCan_DGN_131070_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_SHARC_HTR
	{ 1,     0,    0,         0,        130552,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130552_SIZE,  MsgCan_DGN_130552_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
	{ 1,     0,    0,         0,        130553,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130553_SIZE,  MsgCan_DGN_130553_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
	{ 1,     0,    0,         0,        130555,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130555_SIZE,  MsgCan_DGN_130555_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
	{ 1,     0,    0,         0,        130557,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130557_SIZE,  MsgCan_DGN_130557_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
	{ 1,     0,    0,         0,        130559,  0,       0,        0,      0xFF,   0,       PROPDGN_DGN_130559_SIZE,  MsgCan_DGN_130559_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_BUS_TIME_USE
    { 1,     0,    0,         0,        131071,  0,       0,        0,      0xFF,   0,        RVCDGN_DGN_131071_SIZE,  MsgCan_DGN_131071_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
	{ 1,     0,    0,         0,        131042,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_131042_SIZE,  MsgCan_DGN_131042_u8RxData,  MsgCan_DGN_131042_RxHandle,  },
    { 1,     0,    0,         0,        130810,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130810_SIZE,  MsgCan_DGN_130810_u8RxData,  MsgCan_DGN_130810_RxHandle,  },
    { 1,     0,    0,         0,        130807,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130807_SIZE,  MsgCan_DGN_130807_u8RxData,  MsgCan_DGN_130807_RxHandle,  },
    { 1,     0,    0,         0,        130806,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130806_SIZE,  MsgCan_DGN_130806_u8RxData,  MsgCan_DGN_130806_RxHandle,  },
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
	{ 1,     0,    0,         0,        130809,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130809_SIZE,  MsgCan_DGN_130809_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
    { 1,     0,    0,         0,        130808,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130808_SIZE,  MsgCan_DGN_130808_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
    { 1,     0,    0,         0,        130805,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130805_SIZE,  MsgCan_DGN_130805_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
    { 1,     0,    0,         0,        130804,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130804_SIZE,  MsgCan_DGN_130804_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1) || defined(RVC_CONFIG_IMPL_DC_DIMMER_2)
	{ 1,     0,    0,         0,        130779,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130779_SIZE,  MsgCan_DGN_130779_u8RxData, MsgCan_DGN_Common_RxHandle,  },
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_1) || defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
	{ 1,     0,    0,         0,        130778,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130778_SIZE,  MsgCan_DGN_130778_u8RxData, MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_IMPL_ROOF_FAN
    { 1,     0,    0,         0,        130530,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130530_SIZE,  MsgCan_DGN_130530_u8RxData, MsgCan_DGN_Common_RxHandle,  },
    { 1,     0,    0,         0,        130726,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130726_SIZE,  MsgCan_DGN_130726_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_ROOF_FAN
    { 1,     0,    0,         0,        130531,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130531_SIZE,  MsgCan_DGN_130531_u8RxData, MsgCan_DGN_Common_RxHandle,  },
    { 1,     0,    0,         0,        130727,  0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130727_SIZE,  MsgCan_DGN_130727_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_IMPL_PROPRIATARY
    { 1,     0,    0,         0,        61184,   0,      0,      0,       0xFF,   0,      PROPDGN_DGN_61184_SIZE,   MsgCan_DGN_61184_u8RxData,  MsgCan_DGN_61184_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
    { 1,     0,    0,         0,        131069,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_131069_SIZE,   MsgCan_DGN_131069_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
    { 1,     0,    0,         0,        131068,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_131068_SIZE,   MsgCan_DGN_131068_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
    { 1,     0,    0,         0,        131067,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_131067_SIZE,   MsgCan_DGN_131067_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
    { 1,     0,    0,         0,        130761,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130761_SIZE,   MsgCan_DGN_130761_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
    { 1,     0,    0,         0,        130760,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130760_SIZE,   MsgCan_DGN_130760_u8RxData,  MsgCan_DGN_Common_RxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
    { 1,     0,    0,         0,        130759,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130759_SIZE,   MsgCan_DGN_130759_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
    { 1,     0,    0,         0,        130732,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130732_SIZE,   MsgCan_DGN_130732_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
    { 1,     0,    0,         0,        130731,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130731_SIZE,   MsgCan_DGN_130731_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
    { 1,     0,    0,         0,        130730,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130730_SIZE,   MsgCan_DGN_130730_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
    { 1,     0,    0,         0,        130729,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130729_SIZE,   MsgCan_DGN_130729_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
    { 1,     0,    0,         0,        130725,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130725_SIZE,   MsgCan_DGN_130725_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
    { 1,     0,    0,         0,        130552,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130552_SIZE,   MsgCan_DGN_130552_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
    { 1,     0,    0,         0,        130535,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130535_SIZE,   MsgCan_DGN_130535_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
    { 1,     0,    0,         0,        130551,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130551_SIZE,   MsgCan_DGN_130551_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
    { 1,     0,    0,         0,        130549,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130549_SIZE,   MsgCan_DGN_130549_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
    { 1,     0,    0,         0,        130512,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130512_SIZE,   MsgCan_DGN_130512_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_BATTERY_SUMMARY
    { 1,     0,    0,         0,        130545,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130545_SIZE,   MsgCan_DGN_130545_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
    { 1,     0,    0,         0,        131041,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_131041_SIZE,   MsgCan_DGN_131041_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
    { 1,     0,    0,         0,        130971,   0,      0,      0,       0xFF,   0,      RVCDGN_DGN_130971_SIZE,   MsgCan_DGN_130971_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
#ifdef RVC_CONFIG_IMPL_REFRIGERATOR 
    { 1,     0,    0,         0,        130514,   0,      0,      0,       0xFF,   0,      RVC_DGN_130515_SIZE,   MsgCan_DGN_130514_u8RxData,  MsgCan_DGN_Common_RxHandle,  },
#endif
    { 1,     0,     0,      0,  0x0003FFFF,     0,  0,  0,      0xFF,   0,  8,      MsgCan_DGN_Common_u8RxData,     MsgCan_DGN_Common_RxHandle,  },
};
// clang-format on
//------------------------------------------------------------------------------
// Transmit buffers
static EXT_RAM_ATTR uint8_t MsgCan_DGN_65242_u8TxData[RVCDGN_DGN_65242_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_65259_u8TxData[RVCDGN_DGN_65259_SIZE + 1];  // Extra null
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130762_u8TxData[RVCDGN_DGN_130762_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_59904_u8TxData[RVCDGN_DGN_59904_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_98048_u8TxData[RVCDGN_DGN_98048_SIZE];  // general reset 0x17F00
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130553_u8TxData[PROPDGN_DGN_130553_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130555_u8TxData[PROPDGN_DGN_130555_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130557_u8TxData[PROPDGN_DGN_130557_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130559_u8TxData[PROPDGN_DGN_130559_SIZE];
#endif
#ifdef RVC_CONFIG_BUS_TIME_KEEP
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131071_u8TxData[RVCDGN_DGN_131071_SIZE];
#endif
#ifdef RVC_CONFIG_BUS_TIME_USE
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131070_u8TxData[RVCDGN_DGN_131070_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_SHARC_HTR
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130554_u8TxData[PROPDGN_DGN_130556_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130556_u8TxData[PROPDGN_DGN_130557_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130558_u8TxData[PROPDGN_DGN_130557_SIZE];
#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130809_u8TxData[RVCDGN_DGN_130809_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130808_u8TxData[RVCDGN_DGN_130808_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130804_u8TxData[RVCDGN_DGN_130804_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130805_u8TxData[RVCDGN_DGN_130805_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130809_2_u8TxData[RVCDGN_DGN_130809_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130808_2_u8TxData[RVCDGN_DGN_130808_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130804_2_u8TxData[RVCDGN_DGN_130804_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130805_2_u8TxData[RVCDGN_DGN_130805_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130809_3_u8TxData[RVCDGN_DGN_130809_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130808_3_u8TxData[RVCDGN_DGN_130808_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130804_3_u8TxData[RVCDGN_DGN_130804_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130805_3_u8TxData[RVCDGN_DGN_130805_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130809_4_u8TxData[RVCDGN_DGN_130809_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130808_4_u8TxData[RVCDGN_DGN_130808_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130804_4_u8TxData[RVCDGN_DGN_130804_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130805_4_u8TxData[RVCDGN_DGN_130805_SIZE];
#endif

#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131042_u8TxData[RVCDGN_DGN_131042_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130810_u8TxData[RVCDGN_DGN_130810_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130972_u8TxData[RVCDGN_DGN_130972_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130807_u8TxData[RVCDGN_DGN_130807_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130806_u8TxData[RVCDGN_DGN_130806_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z2
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131042_2_u8TxData[RVCDGN_DGN_131042_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130810_2_u8TxData[RVCDGN_DGN_130810_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130972_2_u8TxData[RVCDGN_DGN_130972_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130807_2_u8TxData[RVCDGN_DGN_130807_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130806_2_u8TxData[RVCDGN_DGN_130806_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z3
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131042_3_u8TxData[RVCDGN_DGN_131042_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130810_3_u8TxData[RVCDGN_DGN_130810_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130972_3_u8TxData[RVCDGN_DGN_130972_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130807_3_u8TxData[RVCDGN_DGN_130807_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130806_3_u8TxData[RVCDGN_DGN_130806_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z4
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131042_4_u8TxData[RVCDGN_DGN_131042_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130810_4_u8TxData[RVCDGN_DGN_130810_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130972_4_u8TxData[RVCDGN_DGN_130972_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130807_4_u8TxData[RVCDGN_DGN_130807_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130806_4_u8TxData[RVCDGN_DGN_130806_SIZE];
#endif

#ifdef RVC_CONFIG_IMPL_DC_DIMMER_1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130778_u8TxData[RVCDGN_DGN_130778_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_DC_DIMMER_2
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130778_1_u8TxData[RVCDGN_DGN_130778_SIZE];
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_1)
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130779_u8TxData[RVCDGN_DGN_130779_SIZE];
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130779_1_u8TxData[RVCDGN_DGN_130779_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_AIR_CONDITIONER
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131041_u8TxData[RVCDGN_DGN_131041_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130505_u8TxData[RVCDGN_DGN_130505_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
static EXT_RAM_ATTR uint8_t MsgCan_DGN_131040_u8TxData[RVCDGN_DGN_131040_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_PROPRIATARY
static EXT_RAM_ATTR uint8_t MsgCan_DGN_61184_u8TxData[PROPDGN_DGN_61184_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_ROOF_FAN
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130531_u8TxData[RVCDGN_DGN_130531_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130727_u8TxData[RVCDGN_DGN_130727_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_ROOF_FAN
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130530_u8TxData[RVCDGN_DGN_130530_SIZE];
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130726_u8TxData[RVCDGN_DGN_130726_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130970_u8TxData[RVCDGN_DGN_130970_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_REFRIGERATOR
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130515_u8TxData[RVC_DGN_130515_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130724_u8TxData[RVCDGN_DGN_130724_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130550_u8TxData[RVCDGN_DGN_130550_SIZE];
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130548_u8TxData[RVCDGN_DGN_130548_SIZE];
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
static EXT_RAM_ATTR uint8_t MsgCan_DGN_130526_u8TxData[RVCDGN_DGN_130526_SIZE];
#endif

//------------------------------------------------------------------------------
// Transmit handlers
static void MsgCan_DGN_65242_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_65259_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_59904_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_98048_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_Common_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
static void MsgCan_DGN_130553_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130555_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130557_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130559_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif
#ifdef RVC_CONFIG_BUS_TIME_USE
static void MsgCan_DGN_131070_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
static void MsgCan_DGN_130809_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130808_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130804_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130805_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
static void MsgCan_DGN_130809_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130808_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130804_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130805_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
static void MsgCan_DGN_130809_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130808_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130804_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130805_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
static void MsgCan_DGN_130809_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130808_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130804_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130805_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif

#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
static void MsgCan_DGN_130806_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
static void MsgCan_DGN_130807_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif
#ifdef RVC_CONFIG_IMPL_PROPRIATARY
static void MsgCan_DGN_61184_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox);
#endif
// clang-format off
//------------------------------------------------------------------------------
// Transmit mailbox definition
static NMEA2K_TxMsg_Struct msgcan_zTxMailbox[] =
{
    //  TxReq   TxReady Period  TxInProg DgnType Inst  CntrSet  Cntr  DGN     Prio  DestAddr  Req_Addr  DataSize                     Data                       Handler
	{   0,      0,        0,    0,       0,      0,    500,      0,    65242,  6,    0xFF,     0xFF,       RVCDGN_DGN_65242_SIZE,   MsgCan_DGN_65242_u8TxData,   MsgCan_DGN_65242_TxHandle, },
	{   0,      0,        0,    0,       0,      0,    500,      0,    65259,  6,    0xFF,     0xFF,       RVCDGN_DGN_65259_SIZE,   MsgCan_DGN_65259_u8TxData,   MsgCan_DGN_65259_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    500,      0,   130762,  6,    0xFF,     0xFF,      RVCDGN_DGN_130762_SIZE,   MsgCan_DGN_130762_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        0,    0,       0,      0,    500,      0,    59904,  6,    0xFF,     0xFF,       RVCDGN_DGN_59904_SIZE,    MsgCan_DGN_59904_u8TxData,  MsgCan_DGN_59904_TxHandle, },
    {   0,      0,        0,    0,       0,      0,    500,      0,    98048,  6,    0xFF,     0xFF,       RVCDGN_DGN_98048_SIZE,    MsgCan_DGN_98048_u8TxData,  MsgCan_DGN_98048_TxHandle, }, // General reset
    
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
	{   0,      0,        1,    0,       0,      0,    100,      0,   130553,  6,    0xFF,     0xFF,     PROPDGN_DGN_130553_SIZE,   MsgCan_DGN_130553_u8TxData,  MsgCan_DGN_130553_TxHandle, },
	{   0,      0,        1,    0,       0,      0,    100,      0,   130555,  6,    0xFF,     0xFF,     PROPDGN_DGN_130555_SIZE,   MsgCan_DGN_130555_u8TxData,  MsgCan_DGN_130555_TxHandle, },
	{   0,      0,        1,    0,       0,      0,    100,      0,   130557,  6,    0xFF,     0xFF,     PROPDGN_DGN_130557_SIZE,   MsgCan_DGN_130557_u8TxData,  MsgCan_DGN_130557_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    100,      0,   130559,  6,    0xFF,     0xFF,     PROPDGN_DGN_130559_SIZE,   MsgCan_DGN_130559_u8TxData,  MsgCan_DGN_130559_TxHandle, },
#endif
#ifdef RVC_CONFIG_BUS_TIME_KEEP
    {   0,      0,        1,    0,       0,      0,    100,      0,   131071,  6,    0xFF,     0xFF,      RVCDGN_DGN_131071_SIZE,   MsgCan_DGN_131071_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_BUS_TIME_USE
    {   0,      0,        0,    0,       0,      0,    500,      0,   131070,  6,    0xFF,     0xFF,      RVCDGN_DGN_131070_SIZE,   MsgCan_DGN_131070_u8TxData,  MsgCan_DGN_131070_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_SHARC_HTR
	{   0,      0,        1,    0,       0,      0,    100,      0,   130554,  6,    0xFF,     0xFF,     PROPDGN_DGN_130554_SIZE,   MsgCan_DGN_130554_u8TxData,  MsgCan_DGN_Common_TxHandle, },
	{   0,      0,        1,    0,       0,      0,    100,      0,   130556,  6,    0xFF,     0xFF,     PROPDGN_DGN_130556_SIZE,   MsgCan_DGN_130556_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,    0,       0,      0,    100,      0,   130558,  6,    0xFF,     0xFF,     PROPDGN_DGN_130558_SIZE,   MsgCan_DGN_130558_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
    {   0,      0,        0,      0,       0,      0,    500,     0,   130809,  6,    0xFF,     0xFF,     RVCDGN_DGN_130809_SIZE,  MsgCan_DGN_130809_u8TxData,  MsgCan_DGN_130809_TxHandle, },
    {   0,      0,        0,      0,       0,      0,    500,     0,   130808,  6,    0xFF,     0xFF,     RVCDGN_DGN_130808_SIZE,  MsgCan_DGN_130808_u8TxData,  MsgCan_DGN_130808_TxHandle, },
    {   0,      0,        0,      0,       0,      0,    500,     0,   130804,  6,    0xFF,     0xFF,     RVCDGN_DGN_130804_SIZE,  MsgCan_DGN_130804_u8TxData,  MsgCan_DGN_130804_TxHandle, },
    {   0,      0,        0,      0,       0,      0,    500,     0,   130805,  6,    0xFF,     0xFF,     RVCDGN_DGN_130805_SIZE,  MsgCan_DGN_130805_u8TxData,  MsgCan_DGN_130805_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
    {   0,      0,        0,      0,       0,      1,    500,     0,   130809,  6,    0xFF,     0xFF,     RVCDGN_DGN_130809_SIZE,  MsgCan_DGN_130809_2_u8TxData,  MsgCan_DGN_130809_2_TxHandle, },
    {   0,      0,        0,      0,       0,      1,    500,     0,   130808,  6,    0xFF,     0xFF,     RVCDGN_DGN_130808_SIZE,  MsgCan_DGN_130808_2_u8TxData,  MsgCan_DGN_130808_2_TxHandle, },
    {   0,      0,        0,      0,       0,      1,    500,     0,   130804,  6,    0xFF,     0xFF,     RVCDGN_DGN_130804_SIZE,  MsgCan_DGN_130804_2_u8TxData,  MsgCan_DGN_130804_2_TxHandle, },
    {   0,      0,        0,      0,       0,      1,    500,     0,   130805,  6,    0xFF,     0xFF,     RVCDGN_DGN_130805_SIZE,  MsgCan_DGN_130805_2_u8TxData,  MsgCan_DGN_130805_2_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
    {   0,      0,        0,      0,       0,      2,    500,     0,   130809,  6,    0xFF,     0xFF,     RVCDGN_DGN_130809_SIZE,  MsgCan_DGN_130809_3_u8TxData,  MsgCan_DGN_130809_3_TxHandle, },
    {   0,      0,        0,      0,       0,      2,    500,     0,   130808,  6,    0xFF,     0xFF,     RVCDGN_DGN_130808_SIZE,  MsgCan_DGN_130808_3_u8TxData,  MsgCan_DGN_130808_3_TxHandle, },
    {   0,      0,        0,      0,       0,      2,    500,     0,   130804,  6,    0xFF,     0xFF,     RVCDGN_DGN_130804_SIZE,  MsgCan_DGN_130804_3_u8TxData,  MsgCan_DGN_130804_3_TxHandle, },
    {   0,      0,        0,      0,       0,      2,    500,     0,   130805,  6,    0xFF,     0xFF,     RVCDGN_DGN_130805_SIZE,  MsgCan_DGN_130805_3_u8TxData,  MsgCan_DGN_130805_3_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
    {   0,      0,        0,      0,       0,      3,    500,     0,   130809,  6,    0xFF,     0xFF,     RVCDGN_DGN_130809_SIZE,  MsgCan_DGN_130809_4_u8TxData,  MsgCan_DGN_130809_4_TxHandle, },
    {   0,      0,        0,      0,       0,      3,    500,     0,   130808,  6,    0xFF,     0xFF,     RVCDGN_DGN_130808_SIZE,  MsgCan_DGN_130808_4_u8TxData,  MsgCan_DGN_130808_4_TxHandle, },
    {   0,      0,        0,      0,       0,      3,    500,     0,   130804,  6,    0xFF,     0xFF,     RVCDGN_DGN_130804_SIZE,  MsgCan_DGN_130804_4_u8TxData,  MsgCan_DGN_130804_4_TxHandle, },
    {   0,      0,        0,      0,       0,      3,    500,     0,   130805,  6,    0xFF,     0xFF,     RVCDGN_DGN_130805_SIZE,  MsgCan_DGN_130805_4_u8TxData,  MsgCan_DGN_130805_4_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
    {   0,      0,        1,      0,       0,      0,    200,     0,   131042,  6,    0xFF,     0xFF,     RVCDGN_DGN_131042_SIZE,  MsgCan_DGN_131042_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      0,    500,     0,   130972,  6,    0xFF,     0xFF,     RVCDGN_DGN_130972_SIZE,   MsgCan_DGN_130972_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      0,    500,     0,   130810,  6,    0xFF,     0xFF,     RVCDGN_DGN_130810_SIZE,  MsgCan_DGN_130810_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        0,      0,       0,      0,    500,     0,   130807,  6,    0xFF,     0xFF,     RVCDGN_DGN_130807_SIZE,  MsgCan_DGN_130807_u8TxData,  MsgCan_DGN_130807_TxHandle, },
    {   0,      0,        0,      0,       0,      0,    500,     0,   130806,  6,    0xFF,     0xFF,     RVCDGN_DGN_130806_SIZE,  MsgCan_DGN_130806_u8TxData,  MsgCan_DGN_130806_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z2
    {   0,      0,        1,      0,       0,      1,    500,     0,   131042,  6,    0xFF,     0xFF,     RVCDGN_DGN_131042_SIZE,  MsgCan_DGN_131042_2_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      1,    500,     0,   130972,  6,    0xFF,     0xFF,     RVCDGN_DGN_130972_SIZE,   MsgCan_DGN_130972_2_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      1,    500,     0,   130810,  6,    0xFF,     0xFF,     RVCDGN_DGN_130810_SIZE,  MsgCan_DGN_130810_2_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      1,    500,     0,   130807,  6,    0xFF,     0xFF,     RVCDGN_DGN_130807_SIZE,  MsgCan_DGN_130807_2_u8TxData,  MsgCan_DGN_130807_TxHandle, },
    {   0,      0,        1,      0,       0,      1,    500,     0,   130806,  6,    0xFF,     0xFF,     RVCDGN_DGN_130806_SIZE,  MsgCan_DGN_130806_2_u8TxData,  MsgCan_DGN_130806_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z3
    {   0,      0,        1,      0,       0,      2,    500,     0,   131042,  6,    0xFF,     0xFF,     RVCDGN_DGN_131042_SIZE,  MsgCan_DGN_131042_3_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      2,    500,     0,   130972,  6,    0xFF,     0xFF,     RVCDGN_DGN_130972_SIZE,   MsgCan_DGN_130972_3_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      2,    500,     0,   130810,  6,    0xFF,     0xFF,     RVCDGN_DGN_130810_SIZE,  MsgCan_DGN_130810_3_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      2,    500,     0,   130807,  6,    0xFF,     0xFF,     RVCDGN_DGN_130807_SIZE,  MsgCan_DGN_130807_3_u8TxData,  MsgCan_DGN_130807_TxHandle, },
    {   0,      0,        1,      0,       0,      2,    500,     0,   130806,  6,    0xFF,     0xFF,     RVCDGN_DGN_130806_SIZE,  MsgCan_DGN_130806_3_u8TxData,  MsgCan_DGN_130806_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z4
    {   0,      0,        1,      0,       0,      3,    500,     0,   131042,  6,    0xFF,     0xFF,     RVCDGN_DGN_131042_SIZE,  MsgCan_DGN_131042_4_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      3,    500,     0,   130972,  6,    0xFF,     0xFF,     RVCDGN_DGN_130972_SIZE,   MsgCan_DGN_130972_4_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      3,    500,     0,   130810,  6,    0xFF,     0xFF,     RVCDGN_DGN_130810_SIZE,  MsgCan_DGN_130810_4_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      3,    500,     0,   130807,  6,    0xFF,     0xFF,     RVCDGN_DGN_130807_SIZE,  MsgCan_DGN_130807_4_u8TxData,  MsgCan_DGN_130807_TxHandle, },
    {   0,      0,        1,      0,       0,      3,    500,     0,   130806,  6,    0xFF,     0xFF,     RVCDGN_DGN_130806_SIZE,  MsgCan_DGN_130806_4_u8TxData,  MsgCan_DGN_130806_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_DC_DIMMER_1
    {   0,      0,        0,      0,       0,      0,    500,     0,   130778,  6,    0xFF,     0xFF,     RVCDGN_DGN_130778_SIZE,  MsgCan_DGN_130778_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_DC_DIMMER_2
    {   0,      0,        0,      0,       0,      1,    500,     0,   130778,  6,    0xFF,     0xFF,     RVCDGN_DGN_130778_SIZE,  MsgCan_DGN_130778_1_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_1)
    {   0,      0,        0,      0,       0,      0,    100,     0,   130779,  6,    0xFF,     0xFF,     RVCDGN_DGN_130779_SIZE,  MsgCan_DGN_130779_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
    {   0,      0,        0,      0,       0,      1,    100,     0,   130779,  6,    0xFF,     0xFF,     RVCDGN_DGN_130779_SIZE,  MsgCan_DGN_130779_1_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_AIR_CONDITIONER
    {   0,      0,        1,      0,       0,      0,    200,     0,   131041,  6,    0xFF,     0xFF,     RVCDGN_DGN_131041_SIZE,  MsgCan_DGN_131041_u8TxData,  MsgCan_DGN_Common_TxHandle, },
	{   0,      0,        1,      0,       0,      0,    200,     0,   130505,  6,    0xFF,     0xFF,     RVCDGN_DGN_130505_SIZE,  MsgCan_DGN_130505_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
    {   0,      0,        1,      0,       0,      0,    200,     0,   131040,  6,    0xFF,     0xFF,     RVCDGN_DGN_131040_SIZE,  MsgCan_DGN_131040_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
    {   0,      0,        0,      0,       0,      0,    200,     0,   130970,  6,    0xFF,     0xFF,     RVCDGN_DGN_130970_SIZE,  MsgCan_DGN_130970_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        0,      0,       0,      1,    200,     0,   130970,  6,    0xFF,     0xFF,     RVCDGN_DGN_130970_SIZE,  MsgCan_DGN_130970_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        0,      0,       0,      2,    200,     0,   130970,  6,    0xFF,     0xFF,     RVCDGN_DGN_130970_SIZE,  MsgCan_DGN_130970_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        0,      0,       0,      3,    200,     0,   130970,  6,    0xFF,     0xFF,     RVCDGN_DGN_130970_SIZE,  MsgCan_DGN_130970_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_ROOF_FAN
    {   0,      0,        1,      0,       0,      0,    500,      0,   130531,  6,    0xFF,     0xFF,     RVCDGN_DGN_130531_SIZE,   MsgCan_DGN_130531_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        1,      0,       0,      0,    500,      0,   130727,  6,    0xFF,     0xFF,     RVCDGN_DGN_130727_SIZE,   MsgCan_DGN_130727_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_ROOF_FAN
    {   0,      0,        0,      0,       0,      0,    500,      0,   130530,  6,    0xFF,     0xFF,     RVCDGN_DGN_130530_SIZE,   MsgCan_DGN_130530_u8TxData,  MsgCan_DGN_Common_TxHandle, },
    {   0,      0,        0,      0,       0,      0,    500,      0,   130726,  6,    0xFF,     0xFF,     RVCDGN_DGN_130726_SIZE,   MsgCan_DGN_130726_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_REFRIGERATOR 
    {   0,      0,        0,      0,       0,      1,    500,     0,   130515,  6,    0xFF,     0xFF,     RVC_DGN_130515_SIZE,  MsgCan_DGN_130515_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
    {   0,      0,        0,      0,       0,      0,    500,     0,   130724,  6,    0xFF,     0xFF,     RVCDGN_DGN_130724_SIZE,  MsgCan_DGN_130724_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
    {   0,      0,        0,      0,       0,      0,    500,     0,   130550,  6,    0xFF,     0xFF,     RVCDGN_DGN_130550_SIZE,  MsgCan_DGN_130550_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
    {   0,      0,        0,      0,       0,      0,    500,     0,   130548,  6,    0xFF,     0xFF,     RVCDGN_DGN_130548_SIZE,  MsgCan_DGN_130548_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
    {   0,      0,        0,      0,       0,      0,    500,     0,   130526,  6,    0xFF,     0xFF,     RVCDGN_DGN_130526_SIZE,  MsgCan_DGN_130526_u8TxData,  MsgCan_DGN_Common_TxHandle, },
#endif
#ifdef RVC_CONFIG_IMPL_PROPRIATARY
    {   0,      0,        0,      0,       0,      0,    100,     0,   61184,   6,    0xFF,     0xFF,     PROPDGN_DGN_61184_SIZE,  MsgCan_DGN_61184_u8TxData,  MsgCan_DGN_61184_TxHandle, },
#endif
};
static bool rxNullCb(uint32_t a, uint8_t sa, uint8_t *p, size_t size)
// clang-format on
{
    // Do nothing per default
    return false;
}
static bool txNullCb(uint32_t pgn, uint8_t instance, uint8_t *p_data)
{
    return false;
}
static parameter_changed_cb_t __attribute__((unused)) parameterFuncCb = NULL;
static status_changed_cb_t l_status_changed_cb = NULL;

static rx_cb_t rxFuncCb = rxNullCb;
static tx_cb_t txFuncCb = txNullCb;

//------------------------------------------------------------------------------
// NMEA2K communication parameter definition
static NMEA2K_Parameter_Struct msgcan_NMEA2KParameter =
    {
        BSP_CAN_RVC,             // RVC CAN port index
        msgcan_zRxMailbox,       // RxMailbox Address
        MSGCAN_RX_MAILBOX_SIZE,  // RxMailbox Size
        msgcan_zTxMailbox,       // TxMailbox Address
        MSGCAN_TX_MAILBOX_SIZE,  // TxMailbox Size
        &msgcan_u8PreferredSA,   // Preferred Source Address
        {
            // Name fields
            MSGCAN_ARBITRARY_ADDR_CAPABLE,   // NMEA2K name field 1
            MSGCAN_INDUSTRY_GROUP,           // NMEA2K name field 2
            MSGCAN_VEHICLE_SYSTEM_INSTANCE,  // NMEA2K name field 3
            MSGCAN_VEHICLE_SYSTEM,           // NMEA2K name field 4
            MSGCAN_RESERVED_FIELD,           // NMEA2K name field 5
            MSGCAN_FUNCTION,                 // NMEA2K name field 6
            MSGCAN_FUNCTION_INSTANCE,        // NMEA2K name field 7
            MSGCAN_ECU_INSTANCE,             // NMEA2K name field 8
            MSGCAN_MANUFACTURER_CODE,        // NMEA2K name field 9
            MSGCAN_SERIAL_NUMBER,            // NMEA2K name field 10
        }};

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------
static void MsgCan_Update_PImage(PIMAGE_eTABLE eIndex, int32 i32Value) __attribute__((unused));
//------------------------------------------------------------------------------
// Local functions.
//------------------------------------------------------------------------------
static void MsgCan_DGN_130553_Reset(void);
static void MsgCan_DGN_130554_Reset(void);
static void MsgCan_DGN_130555_Reset(void);
static void MsgCan_DGN_130556_Reset(void);
static void MsgCan_DGN_130557_Reset(void);
static void MsgCan_DGN_130558_Reset(void);
static void MsgCan_DGN_130559_Reset(void);
static void MsgCan_DGN_130972_Reset(void);
static void MsgCan_DGN_131070_Reset(void);
static void MsgCan_DGN_131071_Reset(void);
static void MsgCan_DGN_131042_Reset(void);
static void MsgCan_DGN_130809_Reset(void);
static void MsgCan_DGN_130808_Reset(void);
static void MsgCan_DGN_130807_Reset(void);
static void MsgCan_DGN_130804_Reset(void);
static void MsgCan_DGN_130805_Reset(void);
static void MsgCan_DGN_130806_Reset(void);
static void MsgCan_DGN_130778_Reset(void);
static void MsgCan_DGN_130779_Reset(void);
static void MsgCan_DGN_130810_Reset(void);
static void MsgCan_DGN_61184_Reset(void);

static bool MsgCan_FilterMsg(HALCAN_zMSG *zMsg);

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
    {
        return;
    }

    memset(SWId[index], 0, RVCDGN_DGN_65242_FIELD_SIZE);
    memcpy(SWId[index], data, length);
}

/**
 * @brief Initialization of receive and transmit mail boxes.
 *
 * @param u8SrcAddr[in] Preferred source address
 * @param u32NameId[in] NMEA 2000 identity number
 * @param u8Instance[in] Function instance
 * @param funcCb[in] Application function cb
 */
void MSGCAN_Initialize(uint8_t u8SrcAddr, uint32_t u32NameId, uint8_t u8Instance, status_changed_cb_t status_changed_cb)
{
    // Initialize RVC instance and preferred address
    msgcan_NMEA2KParameter.Name_Fields.Function_Instance = u8Instance;
    msgcan_u8PreferredSA = u8SrcAddr + u8Instance - 1;

    // Assign identity number
    msgcan_NMEA2KParameter.Name_Fields.Identity_Number = u32NameId & RVCDGN_SERIAL_NUMBER_MASK;

    // Set HALCAN filter for messages
    HALCAN_InitAppFilter(BSP_CAN_RVC, MsgCan_FilterMsg);

    // Initialize a new instance of the communication protocol
    (void)NMEA2K_Initialize(&msgcan_NMEA2KParameter);

    l_status_changed_cb = status_changed_cb;

    // Install callback for address claim notification
    NMEA2K_SetAddressClaimedCallback(MsgCan_Address_Claimed_Update);

    // Install callback for TxReady flag for DGN notification
    NMEA2K_SetDGNTxReadyCallback(MsgCan_DGN_TxReady);
    // Reset heart beat timers
    msgcan_u16HeartBeat = 0;

    // parameterFuncCb = funcCb;

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
    // For SHAPE

    MsgCan_DGN_130810_Reset();
    MsgCan_DGN_131042_Reset();
    MsgCan_DGN_130809_Reset();
    MsgCan_DGN_130808_Reset();
    MsgCan_DGN_130807_Reset();
    MsgCan_DGN_130804_Reset();
    MsgCan_DGN_130805_Reset();
    MsgCan_DGN_130806_Reset();
    MsgCan_DGN_130778_Reset();
    MsgCan_DGN_130779_Reset();
    MsgCan_DGN_130972_Reset();
    MsgCan_DGN_61184_Reset();

    // Test mode (demo)
#if MSGCAN_TEST_MODE == TRUE
    PIMAGE_SetValue(VAR_DGN_130556_ROOM_TEMP, RVCDGN_TEMPERATURE_TO_S16(27));
    PIMAGE_SetValue(VAR_DGN_130558_TARGET_ROOM_TEMP, RVCDGN_TEMPERATURE_TO_S16(-23.5));
    PIMAGE_SetValue(VAR_DGN_130558_UNDERVOLT_THRES, RVCDGN_VOLT_TO_U8(11.4));
    PIMAGE_SetValue(VAR_DGN_130559_TARGET_ROOM_TEMP, RVCDGN_TEMPERATURE_TO_S16(-21.0));
    PIMAGE_SetValue(VAR_DGN_130559_UNDERVOLT_THRES, RVCDGN_VOLT_TO_U8(-10.6));
    PIMAGE_SetValue(VAR_DGN_130972_AMBIENT_TEMP, RVCDGN_TEMPERATURE_TO_U16(-22.5));
    PIMAGE_SetValue(VAR_DGN_130556_BUTTON_FAV, 0);
    PIMAGE_SetValue(VAR_DGN_130556_BUTTON_MENU, 1);
    PIMAGE_SetValue(VAR_DGN_130556_BUTTON_HOME, 2);
    PIMAGE_SetValue(VAR_DGN_130558_AIR_HEATER_CMD, 0);
    PIMAGE_SetValue(VAR_DGN_130558_WTR_HEATER_CMD, 1);
    PIMAGE_SetValue(VAR_DGN_130558_AIR_HEATER_MODE, 2);
    PIMAGE_SetValue(VAR_DGN_130558_WTR_HEATER_MODE, 3);
    PIMAGE_SetValue(VAR_DGN_130558_SYSTEM_UNITS, 1);

    PIMAGE_SetValue(VAR_DGN_130557_AC_HEATER_AIR, 1);
    PIMAGE_SetValue(VAR_DGN_130557_AC_HEATER_WTR, 2);
    PIMAGE_SetValue(VAR_DGN_130557_GAS_HEATER_AIR, 3);
    PIMAGE_SetValue(VAR_DGN_130557_GAS_HEATER_WTR, 4);

#endif
}

void MSGCAN_SetRxCallback(rx_cb_t rx_callback)
{
    rxFuncCb = rx_callback;
}

void MSGCAN_SetTxCallback(tx_cb_t tx_callback)
{
    txFuncCb = tx_callback;
}

/**
 * @brief Manage message timeouts and fault detection.
 *
 * @param u16Elapsed elapsed time since last run
 */
void MSGCAN_Process(uint16 u16Elapsed)
{
    // Update CAN interface status
    PIMAGE_SetValue(VAR_RVC_RX_QUEUE_PEAK, HALCAN_GetRxPeak(BSP_CAN_RVC));
    PIMAGE_SetValue(VAR_RVC_RX_QUEUE_DROP_CNT, HALCAN_GetRxDropped(BSP_CAN_RVC));
    PIMAGE_SetValue(VAR_RVC_TX_QUEUE_PEAK, HALCAN_GetTxPeak(BSP_CAN_RVC));
    PIMAGE_SetValue(VAR_RVC_TX_QUEUE_DROP_CNT, HALCAN_GetTxDropped(BSP_CAN_RVC));

    //-----------------------------------------------------
    // Check Heater communication (heartbeat)
#ifdef RVC_CONFIG_INTERF_SHARC_HTR
    if (msgcan_u16HeartBeat < MSGCAN_HEARTBEAT_TIMEOUT)
    {
        // Clear heart beat fault
        PIMAGE_SetValue(VAR_FAULT_HEARTBEAT_LOST, FALSE);

        // Increment timer
        msgcan_u16HeartBeat = MIN(msgcan_u16HeartBeat + u16Elapsed, MSGCAN_HEARTBEAT_TIMEOUT);
    }
    else
    {
        // Set heart beat fault
        PIMAGE_SetValue(VAR_FAULT_HEARTBEAT_LOST, TRUE);

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
#ifdef RVC_CONFIG_IMPL_SHARC_HTR
    if (msgcan_u16HeartBeat < MSGCAN_HEARTBEAT_TIMEOUT)
    {
        // Clear heart beat fault
        PIMAGE_SetValue(VAR_FAULT_HEARTBEAT_LOST, FALSE);

        // Increment timer
        msgcan_u16HeartBeat = MIN(msgcan_u16HeartBeat + u16Elapsed, MSGCAN_HEARTBEAT_TIMEOUT);
    }
    else
    {
        // Set heart beat fault
        PIMAGE_SetValue(VAR_FAULT_HEARTBEAT_LOST, TRUE);

        // Reset values
        MsgCan_DGN_130554_Reset();
        MsgCan_DGN_130556_Reset();
        MsgCan_DGN_130558_Reset();
        MsgCan_DGN_130972_Reset();
        MsgCan_DGN_131070_Reset();
    }
#endif
    // MsgCan_DGN_130810_Reset();
    // MsgCan_DGN_131042_Reset();
    // MsgCan_DGN_130809_Reset();
    // MsgCan_DGN_130808_Reset();
    // MsgCan_DGN_130807_Reset();
    // MsgCan_DGN_130804_Reset();
    // MsgCan_DGN_130805_Reset();
    // MsgCan_DGN_130806_Reset();
    // MsgCan_DGN_130778_Reset();
    // MsgCan_DGN_130779_Reset();
    // MsgCan_DGN_130972_Reset();
    // MsgCan_DGN_61184_Reset();
    // MsgCan_DGN_61184_Oper0x42_Reset();
    // MsgCan_DGN_61184_Oper0x43_Reset();
}

/**
 * @brief Filter messages from the network.
 *
 * This function is called back from within the HALCAN layer every
 * time a CAN frame is received and allows filtering before the
 * message is passed onto the NMEA2K protocol stack.
 *
 * @todo Optional: Create filter (select messages to receive)
 * @param zMsg[in] CAN message received
 * @return true: Receive message. false: Discard message
 */
static bool MsgCan_FilterMsg(HALCAN_zMSG *zMsg)
{
    return true;  // Receive all messages
}

/**
 * @brief Reset DGN 130553 (Heater active fault codes) parameters to not available.
 *
 */
static void MsgCan_DGN_130553_Reset(void)
{
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    PIMAGE_SetValue(VAR_DGN_130553_WARNING_FAULT_ACTIVE, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_CRITICAL_FAULT_ACTIVE, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_RESERVED_FIELD_1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_RESERVED_FIELD_2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_RESERVED_FIELD_3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_1, NMEA2K_UINT10_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_2, NMEA2K_UINT10_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_3, NMEA2K_UINT10_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_4, NMEA2K_UINT10_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130554 (Heater scheduling command) parameters to not available.
 *
 */
static void MsgCan_DGN_130554_Reset(void)
{
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_ON_STAT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_ON_HOUR, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_ON_MIN, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_OFF_STAT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_OFF_HOUR, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_OFF_MIN, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_ON_STAT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_ON_HOUR, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_ON_MIN, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_KEEP_ON_TIME, NMEA2K_UINT8_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130555 (Heater scheduling status) parameters to not available.
 *
 */
static void MsgCan_DGN_130555_Reset(void)
{
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    PIMAGE_SetValue(VAR_DGN_130555_AIR_HTR_ON_STAT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_AIR_HTR_ON_HOUR, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_AIR_HTR_ON_MIN, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_AIR_HTR_OFF_STAT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_AIR_HTR_OFF_HOUR, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_AIR_HTR_OFF_MIN, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_WTR_HTR_ON_STAT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_WTR_HTR_ON_HOUR, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_WTR_HTR_ON_MIN, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME, NMEA2K_UINT8_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130556 (HMI status) parameters to not available.
 *
 */
static void MsgCan_DGN_130556_Reset(void)
{
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    PIMAGE_SetValue(VAR_DGN_130556_ROOM_TEMP, NMEA2K_INT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130556_HEATER_COMM, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130556_INPUT_VOLT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130556_INST_STATUS, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130556_INT_CIRCUITERY, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130556_BUTTON_FAV, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130556_BUTTON_MENU, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130556_BUTTON_HOME, NMEA2K_UINT2_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130557 (Heater status) parameters to not available.
 *
 */
static void MsgCan_DGN_130557_Reset(void)
{
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    PIMAGE_SetValue(VAR_DGN_130557_ROOM_TEMP, NMEA2K_INT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130557_WATER_TEMP, NMEA2K_UINT3_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130557_GAS_HEATER_AIR, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130557_GAS_HEATER_WTR, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130557_AC_PRESENT, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130557_AC_HEATER_AIR, NMEA2K_UINT3_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130557_AC_HEATER_WTR, NMEA2K_UINT3_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130558 (Heater operation command) parameters to not available.
 *
 */
static void MsgCan_DGN_130558_Reset(void)
{
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    PIMAGE_SetValue(VAR_DGN_130558_ENERGY_SOURCE, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_AIR_HEATER_CMD, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_WTR_HEATER_CMD, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_AIR_HEATER_MODE, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_WTR_HEATER_MODE, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_TARGET_ROOM_TEMP, NMEA2K_INT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_SILENT_FAN_MAX, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_VENT_FAN_MIN, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_UNDERVOLT_THRES, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130558_SYSTEM_UNITS, NMEA2K_UINT2_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130559 (Heater operation status) parameters to not available.
 *
 */
static void MsgCan_DGN_130559_Reset(void)
{
#if defined(RVC_CONFIG_INTERF_SHARC_HTR) || defined(RVC_CONFIG_IMPL_SHARC_HTR)
    PIMAGE_SetValue(VAR_DGN_130559_ENERGY_SOURCE, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_AIR_HEATER_CMD, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_WTR_HEATER_CMD, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_AIR_HEATER_MODE, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_WTR_HEATER_MODE, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_TARGET_ROOM_TEMP, NMEA2K_INT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_SILENT_FAN_MAX, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_VENT_FAN_MIN, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_UNDERVOLT_THRES, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130559_SYSTEM_UNITS, NMEA2K_UINT2_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130972 (Thermostat Ambient status) parameters to not available.
 *
 */
static void MsgCan_DGN_130972_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    PIMAGE_SetValue(VAR_DGN_130972_AMBIENT_TEMP_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130972_ZONE_INSTANCE_Z1, 1);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    PIMAGE_SetValue(VAR_DGN_130972_AMBIENT_TEMP_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130972_ZONE_INSTANCE_Z2, 2);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    PIMAGE_SetValue(VAR_DGN_130972_AMBIENT_TEMP_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130972_ZONE_INSTANCE_Z3, 3);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    PIMAGE_SetValue(VAR_DGN_130972_AMBIENT_TEMP_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130972_ZONE_INSTANCE_Z4, 4);
#endif
}

/**
 * @brief Reset DGN 131070 (Date Time command) parameters to not available.
 *
 */
static void MsgCan_DGN_131070_Reset(void)
{
#if defined(RVC_CONFIG_BUS_TIME_USE) || defined(RVC_CONFIG_BUS_TIME_KEEP)
    PIMAGE_SetValue(VAR_DGN_131070_YEAR, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131070_MONTH, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131070_DAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131070_DAY_OF_WEEK, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131070_HOUR, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131070_MINUTE, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131070_SECOND, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131070_TIMEZONE, NMEA2K_UINT8_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 131071 (Data Time status) parameters to not available.
 *
 */
static void MsgCan_DGN_131071_Reset(void)
{
#if defined(RVC_CONFIG_BUS_TIME_USE) || defined(RVC_CONFIG_BUS_TIME_KEEP)
    PIMAGE_SetValue(VAR_DGN_131071_YEAR, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_MONTH, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_DAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_DAY_OF_WEEK, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_HOUR, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_MINUTE, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_SECOND, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_TIMEZONE, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131071_SYNC, 0);
#endif
}

/**
 * @brief Reset DGN 130810 (Thermostat status 2) parameters to not available.
 *
 */
static void MsgCan_DGN_130810_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    // Zone 1
    PIMAGE_SetValue(VAR_DGN_130810_SYNC_Z1, 0);
    PIMAGE_SetValue(VAR_DGN_130810_ZONE_INSTANCE_Z1, 1);
    PIMAGE_SetValue(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z1, 0);
    PIMAGE_SetValue(VAR_DGN_130810_REDUCED_NOISE_MODE_Z1, NMEA2K_UINT2_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    // Zone 2
    PIMAGE_SetValue(VAR_DGN_130810_SYNC_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_130810_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_130810_REDUCED_NOISE_MODE_Z2, NMEA2K_UINT2_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    // Zone 3
    PIMAGE_SetValue(VAR_DGN_130810_SYNC_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_130810_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_130810_REDUCED_NOISE_MODE_Z3, NMEA2K_UINT2_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    // Zone 4
    PIMAGE_SetValue(VAR_DGN_130810_SYNC_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_130810_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_130810_REDUCED_NOISE_MODE_Z4, NMEA2K_UINT2_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 131042 (Thermostat status 1) parameters to not available.
 *
 */
static void MsgCan_DGN_131042_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    PIMAGE_SetValue(VAR_DGN_131042_SYNC_Z1, 0);
    // Zone 1
    PIMAGE_SetValue(VAR_DGN_131042_SYNC_Z1, 0);
    PIMAGE_SetValue(VAR_DGN_131042_ZONE_INSTANCE_Z1, 1);
    PIMAGE_SetValue(VAR_DGN_131042_OPERATING_MODE_Z1, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_MODE_Z1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SCHEDULE_MODE_Z1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_SPEED_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    // Zone 2
    PIMAGE_SetValue(VAR_DGN_131042_SYNC_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_131042_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_131042_OPERATING_MODE_Z2, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_MODE_Z2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SCHEDULE_MODE_Z2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_SPEED_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    // Zone 3
    PIMAGE_SetValue(VAR_DGN_131042_SYNC_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_131042_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_131042_OPERATING_MODE_Z3, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_MODE_Z3, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SCHEDULE_MODE_Z3, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_SPEED_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    // Zone 4
    PIMAGE_SetValue(VAR_DGN_131042_SYNC_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_131042_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_131042_OPERATING_MODE_Z4, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_MODE_Z4, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SCHEDULE_MODE_Z4, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_FAN_SPEED_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130809 (Thermostat command 1) parameters to not available.
 *
 */
static void MsgCan_DGN_130809_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    PIMAGE_SetValue(VAR_DGN_130809_SYNC, 0);
    PIMAGE_SetValue(VAR_DGN_130809_ZONE_INSTANCE, 1);
    PIMAGE_SetValue(VAR_DGN_130809_OPERATING_MODE, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_MODE, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SHEDULE_MODE, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_SPEED, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL, NMEA2K_UINT16_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    PIMAGE_SetValue(VAR_DGN_130809_SYNC_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_130809_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_130809_OPERATING_MODE_Z2, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_MODE_Z2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SHEDULE_MODE_Z2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_SPEED_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    PIMAGE_SetValue(VAR_DGN_130809_SYNC_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_130809_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_130809_OPERATING_MODE_Z3, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_MODE_Z3, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SHEDULE_MODE_Z3, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_SPEED_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    PIMAGE_SetValue(VAR_DGN_130809_SYNC_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_130809_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_130809_OPERATING_MODE_Z4, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_MODE_Z4, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SHEDULE_MODE_Z4, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_FAN_SPEED_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130808 (Thermostat 2 command) parameters to not available.
 *
 */
static void MsgCan_DGN_130808_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    PIMAGE_SetValue(VAR_DGN_130808_SYNC, 0);
    PIMAGE_SetValue(VAR_DGN_130808_ZONE_INSTANCE, 1);
    PIMAGE_SetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130808_REDUCED_NOISE_MODE, NMEA2K_UINT2_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    PIMAGE_SetValue(VAR_DGN_130808_SYNC_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_130808_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130808_REDUCED_NOISE_MODE_Z2, NMEA2K_UINT2_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    PIMAGE_SetValue(VAR_DGN_130808_SYNC_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_130808_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130808_REDUCED_NOISE_MODE_Z3, NMEA2K_UINT2_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    PIMAGE_SetValue(VAR_DGN_130808_SYNC_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_130808_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130808_REDUCED_NOISE_MODE_Z4, NMEA2K_UINT2_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130807 (Thermostat Schedule status) parameters to not available.
 *
 */
static void MsgCan_DGN_130807_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    // Zone 1
    PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z1, 0);
    PIMAGE_SetValue(VAR_DGN_130807_ZONE_INSTANCE_Z1, 1);
    PIMAGE_SetValue(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
#endif
    // Zone 2
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_130807_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
#endif
    // Zone 3
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_130807_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
#endif
    // Zone 4
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    PIMAGE_SetValue(VAR_DGN_130807_SYNC_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_130807_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130804 (Thermostat Schedule 2 command) parameters to not available.
 *
 */
static void MsgCan_DGN_130804_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    PIMAGE_SetValue(VAR_DGN_130804_SYNC_Z1, 0);
    PIMAGE_SetValue(VAR_DGN_130804_ZONE_INSTANCE_Z1, 1);
    PIMAGE_SetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SUNDAY_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_MONDAY_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_TUESDAY_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_WEDNESDAY_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_THURSDAY_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_FRIDAY_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SATURDAY_Z1, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    PIMAGE_SetValue(VAR_DGN_130804_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SUNDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_MONDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_TUESDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_WEDNESDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_THURSDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_FRIDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SATURDAY_Z2, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    PIMAGE_SetValue(VAR_DGN_130804_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SUNDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_MONDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_TUESDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_WEDNESDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_THURSDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_FRIDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SATURDAY_Z3, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    PIMAGE_SetValue(VAR_DGN_130804_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SUNDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_MONDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_TUESDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_WEDNESDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_THURSDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_FRIDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130804_SATURDAY_Z4, NMEA2K_UINT8_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130806 (Thermostat Schedule 2 status) parameters to not available.
 *
 */
static void MsgCan_DGN_130806_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    PIMAGE_SetValue(VAR_DGN_130806_SYNC, 0);
    PIMAGE_SetValue(VAR_DGN_130806_ZONE_INSTANCE, 1);
    PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SUNDAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_MONDAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_TUESDAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_WEDNESDAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_THURSDAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_FRIDAY, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SATURDAY, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    PIMAGE_SetValue(VAR_DGN_130806_SYNC_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_130806_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SUNDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_MONDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_TUESDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_WEDNESDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_THURSDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_FRIDAY_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SATURDAY_Z2, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    PIMAGE_SetValue(VAR_DGN_130806_SYNC_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_130806_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SUNDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_MONDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_TUESDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_WEDNESDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_THURSDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_FRIDAY_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SATURDAY_Z3, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    PIMAGE_SetValue(VAR_DGN_130806_SYNC_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_130806_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SUNDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_MONDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_TUESDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_WEDNESDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_THURSDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_FRIDAY_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130806_SATURDAY_Z4, NMEA2K_UINT8_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130805 (Thermostat Schedule command) parameters to not available.
 *
 */
static void MsgCan_DGN_130805_Reset(void)
{
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
    PIMAGE_SetValue(VAR_DGN_130805_SYNC_Z1, 0);
    PIMAGE_SetValue(VAR_DGN_130805_ZONE_INSTANCE_Z1, 1);
    PIMAGE_SetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z1, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z1, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    PIMAGE_SetValue(VAR_DGN_130805_SYNC_Z2, 0);
    PIMAGE_SetValue(VAR_DGN_130805_ZONE_INSTANCE_Z2, 2);
    PIMAGE_SetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z2, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z2, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    PIMAGE_SetValue(VAR_DGN_130805_SYNC_Z3, 0);
    PIMAGE_SetValue(VAR_DGN_130805_ZONE_INSTANCE_Z3, 3);
    PIMAGE_SetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z3, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z3, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z3, NMEA2K_UINT8_NO_DATA);
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    PIMAGE_SetValue(VAR_DGN_130805_SYNC_Z4, 0);
    PIMAGE_SetValue(VAR_DGN_130805_ZONE_INSTANCE_Z4, 4);
    PIMAGE_SetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z4, NMEA2K_UINT16_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z4, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z4, NMEA2K_UINT8_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130778 (DC Dimmer 3 status) parameters to not available.
 *
 */
static void MsgCan_DGN_130778_Reset(void)
{
#ifdef RVC_CONFIG_IMPL_DC_DIMMER_1
    // Light 1
    PIMAGE_SetValue(VAR_DGN_130778_INST_L1, 1);
    PIMAGE_SetValue(VAR_DGN_130778_GROUP_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_OPERATING_STATUS_BRIGHTNESS_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_LOCK_STATUS_L1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_OVER_CURRENT_STATUS_L1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_OVER_RIDE_STATUS_L1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_ENABLE_STATUS_L1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_DELAY_DURATION_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_LSTCMD_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_INTER_LOCK_STATUS_L1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_LOAD_STATUS_L1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_RESERVED_FIELD_1_L1, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_RESERVED_FIELD_2_L1, NMEA2K_UINT8_NO_DATA);
#endif
    // Light 2
#ifdef RVC_CONFIG_IMPL_DC_DIMMER_2
    PIMAGE_SetValue(VAR_DGN_130778_INST_L1, 2);
    PIMAGE_SetValue(VAR_DGN_130778_GROUP_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_OPERATING_STATUS_BRIGHTNESS_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_LOCK_STATUS_L2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_OVER_CURRENT_STATUS_L2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_OVER_RIDE_STATUS_L2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_ENABLE_STATUS_L2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_DELAY_DURATION_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_LSTCMD_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_INTER_LOCK_STATUS_L2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_LOAD_STATUS_L2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_RESERVED_FIELD_1_L2, NMEA2K_UINT4_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130778_RESERVED_FIELD_2_L2, NMEA2K_UINT8_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 130779 (DC Dimmer 3 command) parameters to not available.
 *
 */
static void MsgCan_DGN_130779_Reset(void)
{
#ifdef RVC_CONFIG_IMPL_DC_DIMMER_1
    PIMAGE_SetValue(VAR_DGN_130779_INST_L1, 1);
    PIMAGE_SetValue(VAR_DGN_130779_GROUP_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_DESIRED_LEVEL_BRIGHTNESS_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_COMMAND_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_DELAY_DURATION_L1, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_INTER_LOCK_L1, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_RESERVED_FIELD_1_L1, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_RESERVED_FIELD_2_L1, NMEA2K_UINT16_NO_DATA);
#endif
#ifdef RVC_CONFIG_IMPL_DC_DIMMER_2
    PIMAGE_SetValue(VAR_DGN_130779_INST_L1, 2);
    PIMAGE_SetValue(VAR_DGN_130779_GROUP_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_DESIRED_LEVEL_BRIGHTNESS_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_COMMAND_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_DELAY_DURATION_L2, NMEA2K_UINT8_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_INTER_LOCK_L2, NMEA2K_UINT2_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_RESERVED_FIELD_1_L2, NMEA2K_UINT6_NO_DATA);
    PIMAGE_SetValue(VAR_DGN_130779_RESERVED_FIELD_2_L2, NMEA2K_UINT16_NO_DATA);
#endif
}

/**
 * @brief Reset DGN 61184 parameters to not available.
 *
 */
static void MsgCan_DGN_61184_Reset(void)
{
}

/**
 * @brief Common process received RVC DGN
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_Common_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    LOG(D, "%d received", pzRxMailbox->PGN);
    // Call global rx callback
    if (true == rxFuncCb(pzRxMailbox->PGN, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize))
    {
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
    }
}

/**
 * @brief Process received RVC DGN 65242 (software info)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_65242_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_65242 zDGN;
    RVCDGN_DGN_65242_Extract(&zDGN, pzRxMailbox->Data, pzRxMailbox->MsgSize);

    // TODO: Process received product information (see zDGN members)
}

/**
 * @brief Process received RVC DGN 65259 (product info)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_65259_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    LOG(D, "PIM %d received", pzRxMailbox->PGN);

    // Call global rx callback
    if (true == rxFuncCb(pzRxMailbox->PGN, pzRxMailbox->Org_Addr, (uint8 *)pzRxMailbox->Data, pzRxMailbox->MsgSize))
    {
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
    }
}

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Process received RVC DGN 130554 (Heater scheduling commands)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130554_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    PROPDGN_zDGN_130554 zDGN;
    PROPDGN_DGN_130554_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance?
    if (zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_1)
    {
        // Copy content to the application process image
        PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_ON_STAT, zDGN.u2AirHtrTimerOnStat);
        PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_ON_HOUR, zDGN.u6AirHtrTimerOnHour);
        PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_ON_MIN, zDGN.u8AirHtrTimerOnMin);
        PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_OFF_STAT, zDGN.u2AirHtrTimerOffStat);
        PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_OFF_HOUR, zDGN.u6AirHtrTimerOffHour);
        PIMAGE_SetValue(VAR_DGN_130554_AIR_HTR_OFF_MIN, zDGN.u8AirHtrTimerOffMin);
        PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_ON_STAT, zDGN.u2AirHtrTimerOnStat);
        PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_ON_HOUR, zDGN.u6AirHtrTimerOnHour);
        PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_ON_MIN, zDGN.u8AirHtrTimerOnMin);
        PIMAGE_SetValue(VAR_DGN_130554_WTR_HTR_KEEP_ON_TIME, zDGN.u8WtrHtrKeepOnTime);

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
    }
}
#endif

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Process received RVC DGN 130556 (HMI status)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130556_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    PROPDGN_zDGN_130556 zDGN;
    PROPDGN_DGN_130556_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance?
    if (zDGN.u8HMIInstance == RVCDGN_FUNC_INSTANCE_1)
    {
        // Copy content to the application process image
        PIMAGE_SetValue(VAR_DGN_130556_ROOM_TEMP, zDGN.i16RoomTemperature);
        PIMAGE_SetValue(VAR_DGN_130556_HEATER_COMM, zDGN.u2HeaterCommunication);
        PIMAGE_SetValue(VAR_DGN_130556_INPUT_VOLT, zDGN.u2InputVoltage);
        PIMAGE_SetValue(VAR_DGN_130556_INST_STATUS, zDGN.u2HMIInstanceStatus);
        PIMAGE_SetValue(VAR_DGN_130556_INT_CIRCUITERY, zDGN.u2InternalCircuitry);
        PIMAGE_SetValue(VAR_DGN_130556_BUTTON_FAV, zDGN.u2FavoriteButton);
        PIMAGE_SetValue(VAR_DGN_130556_BUTTON_MENU, zDGN.u2MenuButton);
        PIMAGE_SetValue(VAR_DGN_130556_BUTTON_HOME, zDGN.u2HomeButton);

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
    }
}
#endif

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Process received RVC DGN 130558 (heater operation commands)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130558_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    PROPDGN_zDGN_130558 zDGN;
    PROPDGN_DGN_130558_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance and valid message type?
    if ((zDGN.u8HeaterInstance == RVCDGN_FUNC_INSTANCE_ALL) ||
        (zDGN.u8HeaterInstance == msgcan_NMEA2KParameter.Name_Fields.Function_Instance))

    {
        // Copy content to the application process image
        PIMAGE_SetValue(VAR_DGN_130558_ENERGY_SOURCE, zDGN.u4EnergySource);
        PIMAGE_SetValue(VAR_DGN_130558_TARGET_ROOM_TEMP, zDGN.i16TargetRoomTemp);
        PIMAGE_SetValue(VAR_DGN_130558_SILENT_FAN_MAX, zDGN.u4SilentModeMaxFan);
        PIMAGE_SetValue(VAR_DGN_130558_VENT_FAN_MIN, zDGN.u4VentModeFanMin);
        PIMAGE_SetValue(VAR_DGN_130558_UNDERVOLT_THRES, zDGN.u8UnderVoltThreshold);
        PIMAGE_SetValue(VAR_DGN_130558_AIR_HEATER_CMD, zDGN.u2AirHeaterCmd);
        PIMAGE_SetValue(VAR_DGN_130558_AIR_HEATER_MODE, zDGN.u4AirHeaterMode);
        PIMAGE_SetValue(VAR_DGN_130558_WTR_HEATER_CMD, zDGN.u2WaterHeaterCmd);
        PIMAGE_SetValue(VAR_DGN_130558_WTR_HEATER_MODE, zDGN.u4WaterHeaterMode);
        PIMAGE_SetValue(VAR_DGN_130558_SYSTEM_UNITS, zDGN.u2SystemUnits);

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
    }
}
#endif

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Process received RVC DGN 130972 (Thermostat ambient status)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130972_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_130972 zDGN;
    RVCDGN_DGN_130972_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance == RVCDGN_FUNC_INSTANCE_1)
    {
        // Get signals (application inputs)
        PIMAGE_SetValue(VAR_DGN_130972_AMBIENT_TEMP, zDGN.u16AmbientTemp);
    }

    // Reload heart beat timer
    msgcan_u16HeartBeat = 0;
}
#endif

#ifdef RVC_CONFIG_BUS_TIME_KEEP
/**
 * @brief Process received RVC DGN 131070 (Date time command)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_131070_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_131070 zDGN;
    RVCDGN_DGN_131070_Extract(&zDGN, pzRxMailbox->Data);

    // Get signals (application inputs)
    PIMAGE_SetValue(VAR_DGN_131070_YEAR, zDGN.u8Year);
    PIMAGE_SetValue(VAR_DGN_131070_MONTH, zDGN.u8Month);
    PIMAGE_SetValue(VAR_DGN_131070_DAY, zDGN.u8Day);
    PIMAGE_SetValue(VAR_DGN_131070_DAY_OF_WEEK, zDGN.u8DayOfWeek);
    PIMAGE_SetValue(VAR_DGN_131070_HOUR, zDGN.u8Hour);
    PIMAGE_SetValue(VAR_DGN_131070_MINUTE, zDGN.u8Minute);
    PIMAGE_SetValue(VAR_DGN_131070_SECOND, zDGN.u8Second);
    PIMAGE_SetValue(VAR_DGN_131070_TIMEZONE, zDGN.u8TimeZone);

    // Reload heart beat timer
    msgcan_u16HeartBeat = 0;
    // Call global rx callback
    rxFuncCb(131070, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
}
#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
/**
 * @brief Process received RVC DGN 131042 (Thermostat status 1)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_131042_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_131042 zDGN;
    RVCDGN_DGN_131042_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_131042_ZONE_INSTANCE_Z1))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_131042_ZONE_INSTANCE_Z1, zDGN.u8Instance);
        MsgCan_Update_PImage(VAR_DGN_131042_OPERATING_MODE_Z1, zDGN.u4OperatingMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_MODE_Z1, zDGN.u2FanMode);
        MsgCan_Update_PImage(VAR_DGN_131042_SCHEDULE_MODE_Z1, zDGN.u2SheduleMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_SPEED_Z1, zDGN.u8FanSpeed);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z1, zDGN.u16SetPointTempHeat);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z1, zDGN.u16SetPointTempCool);
        MsgCan_Update_PImage(VAR_DGN_131042_SYNC_Z1, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(131042, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_131042_ZONE_INSTANCE_Z2))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_131042_ZONE_INSTANCE_Z2, zDGN.u8Instance);
        MsgCan_Update_PImage(VAR_DGN_131042_OPERATING_MODE_Z2, zDGN.u4OperatingMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_MODE_Z2, zDGN.u2FanMode);
        MsgCan_Update_PImage(VAR_DGN_131042_SCHEDULE_MODE_Z2, zDGN.u2SheduleMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_SPEED_Z2, zDGN.u8FanSpeed);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z2, zDGN.u16SetPointTempHeat);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z2, zDGN.u16SetPointTempCool);
        MsgCan_Update_PImage(VAR_DGN_131042_SYNC_Z2, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(131042, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_131042_ZONE_INSTANCE_Z3))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_131042_ZONE_INSTANCE_Z3, zDGN.u8Instance);
        MsgCan_Update_PImage(VAR_DGN_131042_OPERATING_MODE_Z3, zDGN.u4OperatingMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_MODE_Z3, zDGN.u2FanMode);
        MsgCan_Update_PImage(VAR_DGN_131042_SCHEDULE_MODE_Z3, zDGN.u2SheduleMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_SPEED_Z3, zDGN.u8FanSpeed);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z3, zDGN.u16SetPointTempHeat);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z3, zDGN.u16SetPointTempCool);
        MsgCan_Update_PImage(VAR_DGN_131042_SYNC_Z3, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(131042, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_131042_ZONE_INSTANCE_Z4))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_131042_ZONE_INSTANCE_Z4, zDGN.u8Instance);
        MsgCan_Update_PImage(VAR_DGN_131042_OPERATING_MODE_Z4, zDGN.u4OperatingMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_MODE_Z4, zDGN.u2FanMode);
        MsgCan_Update_PImage(VAR_DGN_131042_SCHEDULE_MODE_Z4, zDGN.u2SheduleMode);
        MsgCan_Update_PImage(VAR_DGN_131042_FAN_SPEED_Z4, zDGN.u8FanSpeed);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z4, zDGN.u16SetPointTempHeat);
        MsgCan_Update_PImage(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z4, zDGN.u16SetPointTempCool);
        MsgCan_Update_PImage(VAR_DGN_131042_SYNC_Z4, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(131042, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif

#if DGN_DATA_RX_TESTING == 1
    LOG(I, " Invoked DGN 131042 RxHandle");
    ESP_LOG_BUFFER_HEXDUMP("Received data:: ", pzRxMailbox->Data, pzRxMailbox->MsgSize, ESP_LOG_INFO);
    LOG(I, "operating mode zone 1: %d", PIMAGE_u8GetValue(VAR_DGN_131042_OPERATING_MODE_Z1));
    LOG(I, "fan mode zone 1: %d", PIMAGE_u8GetValue(VAR_DGN_131042_FAN_MODE_Z1));
    LOG(I, "schedule mode zone 1: %d", PIMAGE_u8GetValue(VAR_DGN_131042_SCHEDULE_MODE_Z1));
    LOG(I, "fan speed zone 1: %d", PIMAGE_u8GetValue(VAR_DGN_131042_FAN_SPEED_Z1));
    LOG(I, "set point temp heat zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_131042_SET_POINT_TEMP_HEAT_Z1));
    LOG(I, "set point temp cool zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_131042_SET_POINT_TEMP_COOL_Z1));
#endif
#ifdef MSGCAN_EXTENDED_LOG
    LOG(I, "fan speed 131042 zone 1: %d", PIMAGE_u8GetValue(VAR_DGN_131042_FAN_SPEED_Z1));
#endif
}
#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
/**
 * @brief Process received RVC DGN 130810 (Thermostat status 2)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130810_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_130810 zDGN;
    RVCDGN_DGN_130810_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130810_ZONE_INSTANCE_Z1))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z1, zDGN.u8CurrentScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z1, zDGN.u8NumberOfScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_REDUCED_NOISE_MODE_Z1, zDGN.u2ReducedNoiseMode);
        MsgCan_Update_PImage(VAR_DGN_130810_SYNC_Z1, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130810, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130810_ZONE_INSTANCE_Z2))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z2, zDGN.u8CurrentScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z2, zDGN.u8NumberOfScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_REDUCED_NOISE_MODE_Z2, zDGN.u2ReducedNoiseMode);
        MsgCan_Update_PImage(VAR_DGN_130810_SYNC_Z2, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130810, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }

#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130810_ZONE_INSTANCE_Z3))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z3, zDGN.u8CurrentScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z3, zDGN.u8NumberOfScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_REDUCED_NOISE_MODE_Z3, zDGN.u2ReducedNoiseMode);
        MsgCan_Update_PImage(VAR_DGN_130810_SYNC_Z3, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130810, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }

#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130810_ZONE_INSTANCE_Z4))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z4, zDGN.u8CurrentScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z4, zDGN.u8NumberOfScheduleInstance);
        MsgCan_Update_PImage(VAR_DGN_130810_REDUCED_NOISE_MODE_Z4, zDGN.u2ReducedNoiseMode);
        MsgCan_Update_PImage(VAR_DGN_130810_SYNC_Z4, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130810, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }

#endif
    else
    {
        /* code */
    }

#if DGN_DATA_RX_TESTING == 1
    LOG(I, " Invoked DGN 130810 RxHandle");
    ESP_LOG_BUFFER_HEXDUMP("Received data:: ", pzRxMailbox->Data, pzRxMailbox->MsgSize, ESP_LOG_INFO);
    LOG(I, "Current_Scheduling_instance zone 1: %d", PIMAGE_u8GetValue(VAR_DGN_130810_CURRENT_SCHEDULE_INSTANCE_Z1));
    LOG(I, "No_Of_Instance zone 1: %d", PIMAGE_u8GetValue(VAR_DGN_130810_NUMBER_OF_SCHEDULE_INSTANCE_Z1));
#endif
}
#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
/**
 * @brief Process received RVC DGN 130807 (Thermostat Schedule Status 1)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130807_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_130807 zDGN;
    RVCDGN_DGN_130807_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130807_ZONE_INSTANCE_Z1))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z1, zDGN.u8ScheduleModeInstance);

        if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_HOUR_Z1, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_MINUTE_Z1, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z1, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z1, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_HOUR_Z1, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_MINUTE_Z1, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z1, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z1, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_HOUR_Z1, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_MINUTE_Z1, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z1, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z1, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_HOUR_Z1, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_MINUTE_Z1, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z1, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z1, zDGN.u16SetpointTempCool);
        }
        else
        {
            LOG(E, "Do not support this schedule mode instance value: %d", zDGN.u8ScheduleModeInstance);
        }
        MsgCan_Update_PImage(VAR_DGN_130807_SYNC_Z1, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130807, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130807_ZONE_INSTANCE_Z2))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z2, zDGN.u8ScheduleModeInstance);

        if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_HOUR_Z2, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_MINUTE_Z2, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z2, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z2, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_HOUR_Z2, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_MINUTE_Z2, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z2, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z2, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_HOUR_Z2, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_MINUTE_Z2, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z2, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z2, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_HOUR_Z2, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_MINUTE_Z2, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z2, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z2, zDGN.u16SetpointTempCool);
        }
        else
        {
            LOG(E, "Do not support this schedule mode instance value: %d", zDGN.u8ScheduleModeInstance);
        }
        MsgCan_Update_PImage(VAR_DGN_130807_SYNC_Z2, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130807, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }

#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130807_ZONE_INSTANCE_Z3))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z3, zDGN.u8ScheduleModeInstance);

        if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_HOUR_Z3, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_MINUTE_Z3, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z3, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z3, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_HOUR_Z3, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_MINUTE_Z3, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z3, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z3, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_HOUR_Z3, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_MINUTE_Z3, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z3, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z3, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_HOUR_Z3, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_MINUTE_Z3, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z3, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z3, zDGN.u16SetpointTempCool);
        }
        else
        {
            LOG(E, "Do not support this schedule mode instance value: %d", zDGN.u8ScheduleModeInstance);
        }
        MsgCan_Update_PImage(VAR_DGN_130807_SYNC_Z3, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130807, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130807_ZONE_INSTANCE_Z4))
    {
        // Copy content to the application process image
        MsgCan_Update_PImage(VAR_DGN_130807_SCHEDULE_MODE_INSTANCE_Z4, zDGN.u8ScheduleModeInstance);

        if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_HOUR_Z4, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_START_MINUTE_Z4, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z4, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z4, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_HOUR_Z4, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_START_MINUTE_Z4, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_HEAT_Z4, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_1_SET_POINT_TEMP_COOL_Z4, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_HOUR_Z4, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_START_MINUTE_Z4, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_HEAT_Z4, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_2_SET_POINT_TEMP_COOL_Z4, zDGN.u16SetpointTempCool);
        }
        else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
        {
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_HOUR_Z4, zDGN.u8StartHour);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_START_MINUTE_Z4, zDGN.u8StartMinute);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_HEAT_Z4, zDGN.u16SetpointTempHeat);
            MsgCan_Update_PImage(VAR_DGN_130807_MODE_3_SET_POINT_TEMP_COOL_Z4, zDGN.u16SetpointTempCool);
        }
        else
        {
            LOG(E, "Do not support this schedule mode instance value: %d", zDGN.u8ScheduleModeInstance);
        }
        MsgCan_Update_PImage(VAR_DGN_130807_SYNC_Z4, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130807, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
    else
    {
        /* Do Nothing */
    }

#if DGN_DATA_RX_TESTING == 1
    LOG(I, " Invoked DGN 130807 RxHandle");
    ESP_LOG_BUFFER_HEXDUMP("Received data:: ", pzRxMailbox->Data, pzRxMailbox->MsgSize, ESP_LOG_INFO);
    if (zDGN.u8SheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
    {
        LOG(I, "sleep start hour zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_MODE_0_START_HOUR_Z1))
        LOG(I, "sleep start minute zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_MODE_0_START_MINUTE_Z1))
        LOG(I, "sleep set point temp heat zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_HEAT_Z1));
        LOG(I, "sleep set point temp heat zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_MODE_0_SET_POINT_TEMP_COOL_Z1));
    }
    if (zDGN.u8SheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
    {
        LOG(I, "wake start hour zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_WAKE_START_HOUR_Z1))
        LOG(I, "wake minute zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_WAKE_START_MINUTE_Z1))
        LOG(I, "wake set point temp heat zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_WAKE_SET_POINT_TEMP_HEAT_Z1));
        LOG(I, "wake set point temp heat zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_WAKE_SET_POINT_TEMP_COOL_Z1));
    }
    if (zDGN.u8SheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
    {
        LOG(I, "Away set point temp heat zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_AWAY_SET_POINT_TEMP_HEAT_Z1));
        LOG(I, "Away set point temp heat zone 1: %d", PIMAGE_u16GetValue(VAR_DGN_130807_AWAY_SET_POINT_TEMP_COOL_Z1));
    }
#endif
}
/**
 * @brief Process received RVC DGN 130806 (Thermostat Schedule Status 2)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzRxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130806_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_130806 zDGN;
    RVCDGN_DGN_130806_Extract(&zDGN, pzRxMailbox->Data);

    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return;
    }

    // Valid instance and valid message type?
    if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130806_ZONE_INSTANCE))
    {
        // Copy content to the application process image
        PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE, zDGN.u8ScheduleModeInstance);
        uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
        uint8_t value;
        // Check Sunday
        if (zDGN.u2Sunday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SUNDAY);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Sunday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_SUNDAY, value);
        }
        // Check Monday
        if (zDGN.u2Monday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_MONDAY);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Monday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_MONDAY, value);
        }

        // Check Tuesday
        if (zDGN.u2Tuesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_TUESDAY);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Tuesday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_TUESDAY, value);
        }

        // Check Wednesday
        if (zDGN.u2Wednesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_WEDNESDAY);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Wednesday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_WEDNESDAY, value);
        }

        // Check Thursday
        if (zDGN.u2Thursday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_THURSDAY);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Thursday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_THURSDAY, value);
        }

        // Check Friday
        if (zDGN.u2Friday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_FRIDAY);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Friday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_FRIDAY, value);
        }

        // Check Saturday
        if (zDGN.u2Saturday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SATURDAY);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Saturday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_SATURDAY, value);
        }
        MsgCan_Update_PImage(VAR_DGN_130806_SYNC, 1);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130806, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130806_ZONE_INSTANCE_Z2))
    {
        // Copy content to the application process image
        PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE_Z2, zDGN.u8ScheduleModeInstance);
        uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
        uint8_t value;
        // Check Sunday
        if (zDGN.u2Sunday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SUNDAY_Z2);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Sunday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_SUNDAY_Z2, value);
        }

        // Check Monday
        if (zDGN.u2Monday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_MONDAY_Z2);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Monday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_MONDAY_Z2, value);
        }

        // Check Tuesday
        if (zDGN.u2Tuesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_TUESDAY_Z2);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Tuesday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_TUESDAY_Z2, value);
        }

        // Check Wednesday
        if (zDGN.u2Wednesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_WEDNESDAY_Z2);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Wednesday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_WEDNESDAY_Z2, value);
        }

        // Check Thursday
        if (zDGN.u2Thursday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_THURSDAY_Z2);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Thursday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_THURSDAY_Z2, value);
        }

        // Check Friday
        if (zDGN.u2Friday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_FRIDAY_Z2);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Friday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_FRIDAY_Z2, value);
        }

        if (zDGN.u2Saturday != NMEA2K_UINT2_NO_DATA)
        {
            // Check Saturday
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SATURDAY_Z2);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Saturday << 2 * zDGN.u8ScheduleModeInstance);
            MsgCan_Update_PImage(VAR_DGN_130806_SATURDAY_Z2, value);
        }
        MsgCan_Update_PImage(VAR_DGN_130806_SYNC, 2);
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130806, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130806_ZONE_INSTANCE_Z3))
    {
        // Copy content to the application process image
        PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE_Z3, zDGN.u8ScheduleModeInstance);
        uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
        uint8_t value;
        // Check Sunday
        if (zDGN.u2Sunday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SUNDAY_Z3);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Sunday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_SUNDAY_Z3, value);
        }

        // Check Monday
        if (zDGN.u2Monday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_MONDAY_Z3);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Monday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_MONDAY_Z3, value);
        }

        // Check Tuesday
        if (zDGN.u2Tuesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_TUESDAY_Z3);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Tuesday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_TUESDAY_Z3, value);
        }

        // Check Wednesday
        if (zDGN.u2Wednesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_WEDNESDAY_Z3);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Wednesday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_WEDNESDAY_Z3, value);
        }

        // Check Thursday
        if (zDGN.u2Thursday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_THURSDAY_Z3);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Thursday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_THURSDAY_Z3, value);
        }

        // Check Friday
        if (zDGN.u2Friday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_FRIDAY_Z3);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Friday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_FRIDAY_Z3, value);
        }

        // Check Saturday
        if (zDGN.u2Saturday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SATURDAY_Z3);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Saturday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_SATURDAY_Z3, value);
        }
        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130806, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130806_ZONE_INSTANCE_Z4))
    {
        // Copy content to the application process image
        PIMAGE_SetValue(VAR_DGN_130806_SCHEDULE_MODE_INSTANCE_Z4, zDGN.u8ScheduleModeInstance);
        uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
        uint8_t value;
        // Check Sunday
        if (zDGN.u2Sunday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SUNDAY_Z4);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Sunday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_SUNDAY_Z4, value);
        }

        // Check Monday
        if (zDGN.u2Monday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_MONDAY_Z4);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Monday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_MONDAY_Z4, value);
        }

        // Check Tuesday
        if (zDGN.u2Tuesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_TUESDAY_Z4);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Tuesday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_TUESDAY_Z4, value);
        }

        // Check Wednesday
        if (zDGN.u2Wednesday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_WEDNESDAY_Z4);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Wednesday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_WEDNESDAY_Z4, value);
        }

        // Check Thursday
        if (zDGN.u2Thursday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_THURSDAY_Z4);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Thursday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_THURSDAY_Z4, value);
        }

        // Check Friday
        if (zDGN.u2Friday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_FRIDAY_Z4);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Friday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_FRIDAY_Z4, value);
        }

        // Check Saturday
        if (zDGN.u2Saturday != NMEA2K_UINT2_NO_DATA)
        {
            value = PIMAGE_u8GetValue(VAR_DGN_130806_SATURDAY_Z4);
            // Mask out current value
            value &= (uint8_t)~mask;
            // Mask in new value
            value |= (uint8_t)(zDGN.u2Saturday << 2 * zDGN.u8ScheduleModeInstance);
            PIMAGE_SetValue(VAR_DGN_130806_SATURDAY_Z4, value);
        }

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130806, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
}
#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
/**
 * @brief Process received RVC DGN 130972 (Thermostat Ambient temp Status)
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzTxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_130972_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // Extract message content
    RVCDGN_zDGN_130972 zDGN;
    RVCDGN_DGN_130972_Extract(&zDGN, pzRxMailbox->Data);

    // Valid instance and valid message type?
    LOG(I, " Invoked DGN 130972 RxHandle_out");
    if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130972_ZONE_INSTANCE_Z1))
    {
        // Copy content to the application process image
        LOG(I, " Invoked DGN 130972 RxHandle");
        MsgCan_Update_PImage(VAR_DGN_130972_AMBIENT_TEMP_Z1, zDGN.u16AmbientTemp);

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130972, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130972_ZONE_INSTANCE_Z2))
    {
        // Copy content to the application process image
        LOG(I, " Invoked DGN 130972 zone 2 RxHandle");
        MsgCan_Update_PImage(VAR_DGN_130972_AMBIENT_TEMP_Z2, zDGN.u16AmbientTemp);

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130972, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130972_ZONE_INSTANCE_Z3))
    {
        // Copy content to the application process image
        LOG(I, " Invoked DGN 130972 zone 3 RxHandle");
        MsgCan_Update_PImage(VAR_DGN_130972_AMBIENT_TEMP_Z3, zDGN.u16AmbientTemp);

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130972, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
    else if (zDGN.u8Instance == PIMAGE_u8GetValue(VAR_DGN_130972_ZONE_INSTANCE_Z4))
    {
        // Copy content to the application process image
        LOG(I, " Invoked DGN 130972 zone 4 RxHandle");
        MsgCan_Update_PImage(VAR_DGN_130972_AMBIENT_TEMP_Z4, zDGN.u16AmbientTemp);

        // Reload heart beat timer
        msgcan_u16HeartBeat = 0;
        // Call global rx callback
        rxFuncCb(130972, pzRxMailbox->Org_Addr, pzRxMailbox->Data, pzRxMailbox->MsgSize);
    }
#endif
    else
    {
        /* Do Nothing */
    }
#if DGN_DATA_RX_TESTING == 1
    LOG(I, " Invoked DGN 130972 RxHandle");
    ESP_LOG_BUFFER_HEXDUMP("Received data:: ", pzRxMailbox->Data, pzRxMailbox->MsgSize, ESP_LOG_INFO);
#endif
}
#endif

#ifdef RVC_CONFIG_IMPL_PROPRIATARY

/**
 * @brief Prepare transmission of RVC DGN 61184 (Proprietary message)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_61184_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    if (txFuncCb(pzTxMailbox->PGN, pzTxMailbox->Instance, pzTxMailbox->Data))
    {
        // Need special handling of specific "operation" code 0x55 (RVC tunneling protocol)
        pzTxMailbox->DataSize = pzTxMailbox->Data[PROPDGN_DGN_61184_SIZE - 1];   // last byte is size
        pzTxMailbox->Dest_Addr = pzTxMailbox->Data[PROPDGN_DGN_61184_SIZE - 2];  // next last byte is destination address
        // min size is 8
        if (pzTxMailbox->DataSize < 8)
        {
            pzTxMailbox->DataSize = 8;
        }
        // Flag message as ready to send
        pzTxMailbox->TxReady = TRUE;
    }
}

/**
 * @brief Process received RVC DGN 61184 (Proprietary message)
 *
 * Filter out messages not addressed to us. Global address 0xFF is not allowed.
 *
 * @param u8CanPort[in] CAN port the message was received from
 * @param pzTxMailbox[in] Receive mailbox pointer
 */
static void MsgCan_DGN_61184_RxHandle(uint8_t u8CanPort, NMEA2K_RxMsg_Struct *pzRxMailbox)
{
    // We need to check that this is addressed to us. Global messages are not allowed
    if (NMEA2K_GetDestAddrProp() != NMEA2K_GetSourceAddr(u8CanPort))
    {
        // Not for us, ignore
        return;
    }
    MsgCan_DGN_Common_RxHandle(u8CanPort, pzRxMailbox);
}
#endif

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Prepare transmission of RVC DGN 130555 (Heater scheduling feedback)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130553_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    PROPDGN_zDGN_130553 zDGN;
    zDGN.u2WarningFault = FALSE;
    zDGN.u2CriticalFault = TRUE;
    zDGN.u10FaultCode1 = 1001;
    zDGN.u10FaultCode2 = 1022;
    zDGN.u10FaultCode3 = 1023;
    zDGN.u10FaultCode4 = 1004;

    // Stuff message data
    PROPDGN_DGN_130553_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Prepare transmission of RVC DGN 130555 (Heater scheduling feedback)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130555_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    PROPDGN_zDGN_130555 zDGN;
    zDGN.u8HeaterInstance = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u2AirHtrTimerOnStat = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_AIR_HTR_ON_STAT);
    zDGN.u6AirHtrTimerOnHour = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_AIR_HTR_ON_HOUR);
    zDGN.u8AirHtrTimerOnMin = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_AIR_HTR_ON_MIN);
    zDGN.u2AirHtrTimerOffStat = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_AIR_HTR_OFF_STAT);
    zDGN.u6AirHtrTimerOffHour = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_AIR_HTR_OFF_HOUR);
    zDGN.u8AirHtrTimerOffMin = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_AIR_HTR_OFF_MIN);
    zDGN.u2WtrHtrTimerOnStat = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_WTR_HTR_ON_STAT);
    zDGN.u6WtrHtrTimerOnHour = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_WTR_HTR_ON_HOUR);
    zDGN.u8WtrHtrTimerOnMin = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_WTR_HTR_ON_MIN);
    zDGN.u8WtrHtrKeepOnTime = (uint8_t)PIMAGE_GetValue(VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME);

    // Stuff message data
    PROPDGN_DGN_130555_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

/**
 * @brief Prepare transmission of RVC DGN. This is the common implementation that should be able to be used by all DGNs
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_Common_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    if (txFuncCb(pzTxMailbox->PGN, pzTxMailbox->Instance, pzTxMailbox->Data))
    {
        // Flag message as ready to send
        pzTxMailbox->TxReady = TRUE;
    }
}

/**
 * @brief Prepare transmission of J1939(RVC) DGN 65242 (Software identification)
 * Only used in Sharc application. Not full RVC
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_65242_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    RVCDGN_zDGN_65242 zDGN;

    // Prepare signals
    RVCDGN_DGN_65242_Set(&zDGN, (const char *)&SWId[0][0], (const char *)&SWId[1][0], (const char *)&SWId[2][0], (const char *)&SWId[3][0]);

    // Stuff message data
    pzTxMailbox->DataSize = RVCDGN_DGN_65242_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}

/**
 * @brief Prepare transmission of RVC DGN 65259 (Product identification)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_65259_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    if (txFuncCb(pzTxMailbox->PGN, pzTxMailbox->Instance, pzTxMailbox->Data))
    {
        pzTxMailbox->DataSize = (uint16_t)strlen((const char *)pzTxMailbox->Data);
        // min size is 8
        if (pzTxMailbox->DataSize < 8)
        {
            pzTxMailbox->DataSize = 8;
        }
        // Flag message as ready to send
        pzTxMailbox->TxReady = TRUE;
    }
}

/**
 * @brief Prepare transmission of RVC DGN 59904 (ISO Request)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_59904_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    MsgCan_DGN_Common_TxHandle(u8CanPort, pzTxMailbox);
    if (pzTxMailbox->TxReady)
    {
        // Set destination address for this request and clear temp data
        pzTxMailbox->Dest_Addr = pzTxMailbox->Data[RVCDGN_DGN_59904_SIZE - 1];
        pzTxMailbox->Data[RVCDGN_DGN_59904_SIZE - 1] = 0xFF;
    }
}

/**
 * @brief Prepare transmission of RVC DGN 98048 (General reset)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_98048_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    MsgCan_DGN_Common_TxHandle(u8CanPort, pzTxMailbox);
    if (pzTxMailbox->TxReady)
    {
        // Set destination address for this request and clear temp data
        pzTxMailbox->Dest_Addr = pzTxMailbox->Data[RVCDGN_DGN_98048_SIZE - 1];
        pzTxMailbox->Data[RVCDGN_DGN_98048_SIZE - 1] = 0xFF;
    }
}

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Prepare transmission of RVC DGN 130557 (Heater status)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130557_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    PROPDGN_zDGN_130557 zDGN;

    // Prepare signals (application outputs)
    zDGN.u8HeaterInstance = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u2GasHeaterAir = PIMAGE_u8GetValue(VAR_DGN_130557_GAS_HEATER_AIR);
    zDGN.u2GasHeaterWater = PIMAGE_u8GetValue(VAR_DGN_130557_GAS_HEATER_WTR);
    zDGN.u2ACPresent = PIMAGE_u8GetValue(VAR_DGN_130557_AC_PRESENT);
    zDGN.u2ACHeaterAir = PIMAGE_u8GetValue(VAR_DGN_130557_AC_HEATER_AIR);
    zDGN.u2ACHeaterWater = PIMAGE_u8GetValue(VAR_DGN_130557_AC_HEATER_WTR);

    // Stuff message data
    PROPDGN_DGN_130557_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

#ifdef RVC_CONFIG_IMPL_SHARC_HTR
/**
 * @brief Prepare transmission of RVC DGN 130559 (heater operation feedback)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130559_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    PROPDGN_zDGN_130559 zDGN;
    zDGN.u8HeaterInstance = msgcan_NMEA2KParameter.Name_Fields.Function_Instance;
    zDGN.u4EnergySource = PIMAGE_u8GetValue(VAR_DGN_130559_ENERGY_SOURCE);
    zDGN.u2AirHeaterCmd = PIMAGE_u8GetValue(VAR_DGN_130559_AIR_HEATER_CMD);
    zDGN.u2WaterHeaterCmd = PIMAGE_u8GetValue(VAR_DGN_130559_WTR_HEATER_CMD);
    zDGN.u4AirHeaterMode = PIMAGE_u8GetValue(VAR_DGN_130559_AIR_HEATER_MODE);
    zDGN.u4WaterHeaterMode = PIMAGE_u8GetValue(VAR_DGN_130559_WTR_HEATER_MODE);
    zDGN.i16TargetRoomTemp = PIMAGE_s16GetValue(VAR_DGN_130559_TARGET_ROOM_TEMP);
    zDGN.u4SilentModeMaxFan = PIMAGE_u8GetValue(VAR_DGN_130559_SILENT_FAN_MAX);
    zDGN.u4VentModeFanMin = PIMAGE_u8GetValue(VAR_DGN_130559_VENT_FAN_MIN);
    zDGN.u8UnderVoltThreshold = PIMAGE_u8GetValue(VAR_DGN_130559_UNDERVOLT_THRES);
    zDGN.u2SystemUnits = PIMAGE_u8GetValue(VAR_DGN_130559_SYSTEM_UNITS);

    // Stuff message data
    PROPDGN_DGN_130559_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

#ifdef RVC_CONFIG_BUS_TIME_USE
/**
 * @brief Prepare transmission of RVC DGN 131070 (Date Time Command)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_131070_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    if (txFuncCb(pzTxMailbox->PGN, pzTxMailbox->Instance, pzTxMailbox->Data))
    {
        // Transmitted: reset
        MsgCan_DGN_131070_Reset();

        // Flag message as ready to send
        pzTxMailbox->TxReady = TRUE;
    }
}
#endif

/**
 * @brief Passed to NMEA2K library and will be called when there is an update in address claimed status and/or local source address.
 * Calling upper layer callback if existing.
 *
 */
void MsgCan_Address_Claimed_Update(void)
{

    if (l_status_changed_cb)
    {
        uint32_t value = (uint32_t)(NMEA2K_IsAddressClaimed(BSP_CAN_RVC) ? 1 << 31 : 0);
        value |= (uint32_t)NMEA2K_GetSourceAddr(BSP_CAN_RVC);
        l_status_changed_cb(MSGCAN_LIB_STATUS_ADDRESS_CLAIMED_UPDATE, value);
    }
}

/**
 * @brief Passed to NMEA2K library and will be called when the TxReq or TxReq/TxInProgress/xReady flags for a TxMailbox entry
 * are false.
 * Calling upper layer callback if existing.
 *
 */
void MsgCan_DGN_TxReady(uint32_t DGN, uint8_t msg_type_ready)
{
    if (l_status_changed_cb)
    {
        switch (DGN)
        {
        case 61184:
            l_status_changed_cb(MSGCAN_LIB_STATUS_PROP_DGN_TX_READY, msg_type_ready);
            break;

        default:
            break;
        }
    }
}

/**
 * @brief Update PImage if value has changed and call callback function to further
 * process changed value
 *
 * @param eIndex[in] variable index to be updated
 * @param i32Value[in] variable value to be updated
 * @note Added by Dometic Sweden.
 */
static void MsgCan_Update_PImage(PIMAGE_eTABLE eIndex, int32 i32Value)
{
    EventBits_t __attribute__((unused)) event;
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
#if defined(RVC_CONFIG_INTERF_SHARC_HTR)
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
#endif
    {
        evaluate = 1;
    }

    // Check if values differ
    if (evaluate && (i32Value != curVal))
    {
        LOG(I, "MsgCan_Update_PImage differs, 0x%x 0x%x", (uint32_t)i32Value, (uint32_t)curVal);
        // Value has changed so set new value
        PIMAGE_SetValue(eIndex, i32Value);

        // Check if callback is registered
        if (parameterFuncCb != NULL)
        {
            // LOG(I, "MsgCan_Update_PImage calling callback function");
            //  Call registered callback function
            parameterFuncCb(eIndex, i32Value);
        }
    }
}

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
/**
 * @brief Prepare transmission of RVC DGN 130809 (Thermostat Command 1)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130809_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130809 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130809_ZONE_INSTANCE);
    zDGN.u4OperatingMode = PIMAGE_u8GetValue(VAR_DGN_130809_OPERATING_MODE);
    zDGN.u2FanMode = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_MODE);
    zDGN.u2SheduleMode = PIMAGE_u8GetValue(VAR_DGN_130809_SHEDULE_MODE);
    zDGN.u8FanSpeed = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_SPEED);
    zDGN.u16SetPointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT);
    zDGN.u16SetPointTempCool = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL);

    // Stuff message data
    RVCDGN_DGN_130809_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
static void MsgCan_DGN_130809_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130809 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130809_ZONE_INSTANCE_Z2);
    zDGN.u4OperatingMode = PIMAGE_u8GetValue(VAR_DGN_130809_OPERATING_MODE_Z2);
    zDGN.u2FanMode = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_MODE_Z2);
    zDGN.u2SheduleMode = PIMAGE_u8GetValue(VAR_DGN_130809_SHEDULE_MODE_Z2);
    zDGN.u8FanSpeed = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_SPEED_Z2);
    zDGN.u16SetPointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z2);
    zDGN.u16SetPointTempCool = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z2);

    // Stuff message data
    RVCDGN_DGN_130809_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
static void MsgCan_DGN_130809_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130809 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130809_ZONE_INSTANCE_Z3);
    zDGN.u4OperatingMode = PIMAGE_u8GetValue(VAR_DGN_130809_OPERATING_MODE_Z3);
    zDGN.u2FanMode = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_MODE_Z3);
    zDGN.u2SheduleMode = PIMAGE_u8GetValue(VAR_DGN_130809_SHEDULE_MODE_Z3);
    zDGN.u8FanSpeed = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_SPEED_Z3);
    zDGN.u16SetPointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z3);
    zDGN.u16SetPointTempCool = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z3);

    // Stuff message data
    RVCDGN_DGN_130809_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
static void MsgCan_DGN_130809_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130809 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130809_ZONE_INSTANCE_Z4);
    zDGN.u4OperatingMode = PIMAGE_u8GetValue(VAR_DGN_130809_OPERATING_MODE_Z4);
    zDGN.u2FanMode = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_MODE_Z4);
    zDGN.u2SheduleMode = PIMAGE_u8GetValue(VAR_DGN_130809_SHEDULE_MODE_Z4);
    zDGN.u8FanSpeed = PIMAGE_u8GetValue(VAR_DGN_130809_FAN_SPEED_Z4);
    zDGN.u16SetPointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_HEAT_Z4);
    zDGN.u16SetPointTempCool = PIMAGE_u16GetValue(VAR_DGN_130809_SET_POINT_TEMP_COOL_Z4);

    // Stuff message data
    RVCDGN_DGN_130809_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
/**
 * @brief Prepare transmission of RVC DGN 130808 (Thermostat Command 2)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130808_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130808 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130808_ZONE_INSTANCE);
    zDGN.u8CurrentScheduleInstance = PIMAGE_u8GetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE);
    zDGN.u2ReducedNoiseMode = PIMAGE_u8GetValue(VAR_DGN_130808_REDUCED_NOISE_MODE);

    // Stuff message data
    RVCDGN_DGN_130808_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
/**
 * @brief Prepare transmission of RVC DGN 130804 (Thermostat Schedule Command 2)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130804_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130804 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130804_ZONE_INSTANCE_Z1);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z1);
    uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Sunday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SUNDAY_Z1) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Monday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_MONDAY_Z1) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Tuesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_TUESDAY_Z1) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Wednesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_WEDNESDAY_Z1) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Thursday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_THURSDAY_Z1) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Friday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_FRIDAY_Z1) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Saturday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SATURDAY_Z1) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return;
    }

    // Stuff message data
    RVCDGN_DGN_130804_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
static void MsgCan_DGN_130808_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130808 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130808_ZONE_INSTANCE_Z2);
    zDGN.u8CurrentScheduleInstance = PIMAGE_u8GetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE_Z2);
    zDGN.u2ReducedNoiseMode = PIMAGE_u8GetValue(VAR_DGN_130808_REDUCED_NOISE_MODE_Z2);

    // Stuff message data
    RVCDGN_DGN_130808_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
static void MsgCan_DGN_130804_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130804 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130804_ZONE_INSTANCE_Z2);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z2);
    uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Sunday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SUNDAY_Z2) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Monday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_MONDAY_Z2) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Tuesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_TUESDAY_Z2) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Wednesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_WEDNESDAY_Z2) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Thursday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_THURSDAY_Z2) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Friday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_FRIDAY_Z2) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Saturday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SATURDAY_Z2) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return;
    }

    // Stuff message data
    RVCDGN_DGN_130804_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
static void MsgCan_DGN_130808_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130808 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130808_ZONE_INSTANCE_Z3);
    zDGN.u8CurrentScheduleInstance = PIMAGE_u8GetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE_Z3);
    zDGN.u2ReducedNoiseMode = PIMAGE_u8GetValue(VAR_DGN_130808_REDUCED_NOISE_MODE_Z3);

    // Stuff message data
    RVCDGN_DGN_130808_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
static void MsgCan_DGN_130804_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130804 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130804_ZONE_INSTANCE_Z3);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z3);
    uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Sunday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SUNDAY_Z3) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Monday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_MONDAY_Z3) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Tuesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_TUESDAY_Z3) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Wednesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_WEDNESDAY_Z3) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Thursday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_THURSDAY_Z3) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Friday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_FRIDAY_Z3) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Saturday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SATURDAY_Z3) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return;
    }

    // Stuff message data
    RVCDGN_DGN_130804_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
static void MsgCan_DGN_130808_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130808 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130808_ZONE_INSTANCE_Z4);
    zDGN.u8CurrentScheduleInstance = PIMAGE_u8GetValue(VAR_DGN_130808_CURRENT_SCHEDULE_INSTANCE_Z4);
    zDGN.u2ReducedNoiseMode = PIMAGE_u8GetValue(VAR_DGN_130808_REDUCED_NOISE_MODE_Z4);

    // Stuff message data
    RVCDGN_DGN_130808_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
static void MsgCan_DGN_130804_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130804 zDGN;
    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130804_ZONE_INSTANCE_Z4);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130804_SCHEDULE_MODE_INSTANCE_Z4);
    uint8_t mask = (uint8_t)(NMEA2K_UINT2_NO_DATA << 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Sunday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SUNDAY_Z4) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Monday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_MONDAY_Z4) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Tuesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_TUESDAY_Z4) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Wednesday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_WEDNESDAY_Z4) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Thursday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_THURSDAY_Z4) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Friday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_FRIDAY_Z4) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    zDGN.u2Saturday = (uint8_t)((PIMAGE_u8GetValue(VAR_DGN_130804_SATURDAY_Z4) & mask) >> 2 * zDGN.u8ScheduleModeInstance);
    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return;
    }

    // Stuff message data
    RVCDGN_DGN_130804_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
/**
 * @brief Prepare transmission of RVC DGN 130805 (Thermostat Schedule Command 1)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130805_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130805 zDGN;

    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130805_ZONE_INSTANCE_Z1);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z1);

    if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z1);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z1);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z1);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z1);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z1);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z1);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z1);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z1);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z1);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z1);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z1);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z1);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z1);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z1);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z1);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z1);
    }

    // Stuff message data
    RVCDGN_DGN_130805_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z2
static void MsgCan_DGN_130805_2_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130805 zDGN;

    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130805_ZONE_INSTANCE_Z2);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z2);

    if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z2);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z2);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z2);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z2);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z2);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z2);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z2);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z2);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z2);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z2);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z2);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z2);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z2);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z2);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z2);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z2);
    }

    // Stuff message data
    RVCDGN_DGN_130805_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z3
static void MsgCan_DGN_130805_3_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130805 zDGN;

    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130805_ZONE_INSTANCE_Z3);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z3);

    if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z3);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z3);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z3);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z3);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z3);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z3);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z3);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z3);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z3);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z3);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z3);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z3);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z3);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z3);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z3);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z3);
    }

    // Stuff message data
    RVCDGN_DGN_130805_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z4
static void MsgCan_DGN_130805_4_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    // Prepare signals (application outputs)
    RVCDGN_zDGN_130805 zDGN;

    zDGN.u8Instance = PIMAGE_u8GetValue(VAR_DGN_130805_ZONE_INSTANCE_Z4);
    zDGN.u8ScheduleModeInstance = PIMAGE_u8GetValue(VAR_DGN_130805_SCHEDULE_MODE_INSTANCE_Z4);

    if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_SLEEP)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_HOUR_Z4);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_0_START_MINUTE_Z4);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_HEAT_Z4);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_0_SET_POINT_TEMP_COOL_Z4);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_WAKE)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_HOUR_Z4);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_1_START_MINUTE_Z4);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_HEAT_Z4);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_1_SET_POINT_TEMP_COOL_Z4);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_AWAY)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_HOUR_Z4);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_2_START_MINUTE_Z4);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_HEAT_Z4);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_2_SET_POINT_TEMP_COOL_Z4);
    }
    else if (zDGN.u8ScheduleModeInstance == RVCDGN_SCHEDULE_MODE_RETURN)
    {
        zDGN.u8StartHour = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_HOUR_Z4);
        zDGN.u8StartMinute = PIMAGE_u8GetValue(VAR_DGN_130805_MODE_3_START_MINUTE_Z4);
        zDGN.u16SetpointTempHeat = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_HEAT_Z4);
        zDGN.u16SetpointTempCool = PIMAGE_u16GetValue(VAR_DGN_130805_MODE_3_SET_POINT_TEMP_COOL_Z4);
    }

    // Stuff message data
    RVCDGN_DGN_130805_Stuff(pzTxMailbox->Data, &zDGN);

    // Flag message as ready to send
    pzTxMailbox->TxReady = TRUE;
}
#endif

#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
/**
 * @brief Prepare transmission of RVC DGN 130806 (Thermostat Schedule Status 2)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130806_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    static uint8_t periodic_schedule_instance = 0;
    uint8_t requested_instance = pzTxMailbox->Instance;

    // Prepare signals (application outputs)
    // Periodic scheduling of all schedule mode instances
    if (pzTxMailbox->TxReq == NMEA2K_TXREQ_PERIODIC)
    {
        // We should schedule to send next u8ScheduleModeInstance
        requested_instance |= ((++periodic_schedule_instance) << 4);
        if (periodic_schedule_instance <= RVCDGN_SCHEDULE_MODE_RETURN)
        {
            pzTxMailbox->Cntr = 2;
        }
        else
        {
            // Reset
            periodic_schedule_instance = 0;
        }
    }
    if (true == txFuncCb(pzTxMailbox->PGN, requested_instance, pzTxMailbox->Data))
    {
        // Flag message as ready to send
        pzTxMailbox->TxReady = TRUE;
    }
}
#endif

#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
/**
 * @brief Prepare transmission of RVC DGN 130807 (Thermostat Schedule Status 1)
 *
 * @param u8CanPort[in] CAN port the message must be sent to
 * @param pzTxMailbox[in] Transmission mailbox pointer
 */
static void MsgCan_DGN_130807_TxHandle(uint8_t u8CanPort, NMEA2K_TxMsg_Struct *pzTxMailbox)
{
    static uint8_t periodic_schedule_instance = 0;
    uint8_t requested_instance = pzTxMailbox->Instance;
    // Prepare signals (application outputs)
    // Periodic scheduling of all schedule mode instances
    if (pzTxMailbox->TxReq == NMEA2K_TXREQ_PERIODIC)
    {
        // We should schedule to send next u8ScheduleModeInstance
        requested_instance |= ((++periodic_schedule_instance) << 4);
        if (periodic_schedule_instance <= RVCDGN_SCHEDULE_MODE_RETURN)
        {
            pzTxMailbox->Cntr = 2;
        }
        else
        {
            // Reset
            periodic_schedule_instance = 0;
        }
    }
    if (true == txFuncCb(pzTxMailbox->PGN, requested_instance, pzTxMailbox->Data))
    {
        // Flag message as ready to send
        pzTxMailbox->TxReady = TRUE;
    }
}
#endif
