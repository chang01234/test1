//------------------------------------------------------------------------------
// Module:      PImage.h
//------------------------------------------------------------------------------
// Description: Process Image

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
#ifndef PIMAGE_H
#define PIMAGE_H

//------------------------------------------------------------------------------
// Include files
//------------------------------------------------------------------------------
#include "configuration.h"
#ifdef CONNECTOR_PREM_RVC
#include "StdDefs.h"

//------------------------------------------------------------------------------
// Public Constants
//------------------------------------------------------------------------------
typedef enum _PIMAGE_eTABLE
{
	// CAN Bus RVC
	VAR_RVC_RX_QUEUE_PEAK = 0,
	VAR_RVC_RX_QUEUE_DROP_CNT,
	VAR_RVC_TX_QUEUE_PEAK,
	VAR_RVC_TX_QUEUE_DROP_CNT,

	// DGN 130552 (Heater versions)
	VAR_DGN_130552_COMFORT_SW_VER_MA,
	VAR_DGN_130552_COMFORT_SW_VER_MI,
	VAR_DGN_130552_BURNER_SW_VER_MA,
	VAR_DGN_130552_BURNER_SW_VER_MI,
	VAR_DGN_130552_PCBA_VER,
	VAR_DGN_130552_PROTOCOL_VER_MA,
	VAR_DGN_130552_PROTOCOL_VER_MI,

	// DGN 130553 (Heater active fault codes)
	VAR_DGN_130553_WARNING_FAULT_ACTIVE,
	VAR_DGN_130553_CRITICAL_FAULT_ACTIVE,
	VAR_DGN_130553_RESERVED_FIELD_1,
	VAR_DGN_130553_RESERVED_FIELD_2,
	VAR_DGN_130553_RESERVED_FIELD_3,
	VAR_DGN_130553_ACTIVE_FAULT_CODE_1,
	VAR_DGN_130553_ACTIVE_FAULT_CODE_2,
	VAR_DGN_130553_ACTIVE_FAULT_CODE_3,
	VAR_DGN_130553_ACTIVE_FAULT_CODE_4,

	// DGN 130554 (Heater scheduling commands)
	VAR_DGN_130554_AIR_HTR_ON_STAT,
	VAR_DGN_130554_AIR_HTR_ON_HOUR,
	VAR_DGN_130554_AIR_HTR_ON_MIN,
	VAR_DGN_130554_AIR_HTR_OFF_STAT,
	VAR_DGN_130554_AIR_HTR_OFF_HOUR,
	VAR_DGN_130554_AIR_HTR_OFF_MIN,
	VAR_DGN_130554_WTR_HTR_ON_STAT,
	VAR_DGN_130554_WTR_HTR_ON_HOUR,
	VAR_DGN_130554_WTR_HTR_ON_MIN,
	VAR_DGN_130554_WTR_HTR_KEEP_ON_TIME,

	// DGN 130555 (Heater scheduling feedback)
	VAR_DGN_130555_AIR_HTR_ON_STAT,
	VAR_DGN_130555_AIR_HTR_ON_HOUR,
	VAR_DGN_130555_AIR_HTR_ON_MIN,
	VAR_DGN_130555_AIR_HTR_OFF_STAT,
	VAR_DGN_130555_AIR_HTR_OFF_HOUR,
	VAR_DGN_130555_AIR_HTR_OFF_MIN,
	VAR_DGN_130555_WTR_HTR_ON_STAT,
	VAR_DGN_130555_WTR_HTR_ON_HOUR,
	VAR_DGN_130555_WTR_HTR_ON_MIN,
	VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME,

	// DGN 130556 (HMI Status)
	VAR_DGN_130556_ROOM_TEMP,
	VAR_DGN_130556_HEATER_COMM,
	VAR_DGN_130556_INPUT_VOLT,
	VAR_DGN_130556_INST_STATUS,
	VAR_DGN_130556_INT_CIRCUITERY,
	VAR_DGN_130556_BUTTON_FAV,
	VAR_DGN_130556_BUTTON_MENU,
	VAR_DGN_130556_BUTTON_HOME,

	// DGN 130557 (Heater status)
	VAR_DGN_130557_ROOM_TEMP,
	VAR_DGN_130557_WATER_TEMP,
	VAR_DGN_130557_GAS_HEATER_AIR,
	VAR_DGN_130557_GAS_HEATER_WTR,
	VAR_DGN_130557_AC_PRESENT,
	VAR_DGN_130557_AC_HEATER_AIR,
	VAR_DGN_130557_AC_HEATER_WTR,

	// DGN 130558 (Heater operation commands)
	VAR_DGN_130558_ENERGY_SOURCE,
	VAR_DGN_130558_AIR_HEATER_CMD,
	VAR_DGN_130558_WTR_HEATER_CMD,
	VAR_DGN_130558_AIR_HEATER_MODE,
	VAR_DGN_130558_WTR_HEATER_MODE,
	VAR_DGN_130558_TARGET_ROOM_TEMP,
	VAR_DGN_130558_SILENT_FAN_MAX,
	VAR_DGN_130558_VENT_FAN_MIN,
	VAR_DGN_130558_UNDERVOLT_THRES,
	VAR_DGN_130558_SYSTEM_UNITS,

	// DGN 130559 (Heater operation feedback)
	VAR_DGN_130559_ENERGY_SOURCE,
	VAR_DGN_130559_AIR_HEATER_CMD,
	VAR_DGN_130559_WTR_HEATER_CMD,
	VAR_DGN_130559_AIR_HEATER_MODE,
	VAR_DGN_130559_WTR_HEATER_MODE,
	VAR_DGN_130559_TARGET_ROOM_TEMP,
	VAR_DGN_130559_SILENT_FAN_MAX,
	VAR_DGN_130559_VENT_FAN_MIN,
	VAR_DGN_130559_UNDERVOLT_THRES,
	VAR_DGN_130559_SYSTEM_UNITS,

	// DGN 130972 (Ambient Temperature)
	VAR_DGN_130972_AMBIENT_TEMP,

	// DGN 131070 (System Date Time Command)
	VAR_DGN_131070_YEAR,
	VAR_DGN_131070_MONTH,
	VAR_DGN_131070_DAY,
	VAR_DGN_131070_DAY_OF_WEEK,
	VAR_DGN_131070_HOUR,
	VAR_DGN_131070_MINUTE,
	VAR_DGN_131070_SECOND,
	VAR_DGN_131070_TIMEZONE,

	// DGN 131071 (System Date Time Status)
	VAR_DGN_131071_YEAR,
	VAR_DGN_131071_MONTH,
	VAR_DGN_131071_DAY,
	VAR_DGN_131071_DAY_OF_WEEK,
	VAR_DGN_131071_HOUR,
	VAR_DGN_131071_MINUTE,
	VAR_DGN_131071_SECOND,
	VAR_DGN_131071_TIMEZONE,

	// DGN 65259 (product identifier)
	VAR_DGN_65259_MODEL,

	// Faults
	VAR_FAULT_HEARTBEAT_LOST,

	VAR_ENUM_COUNT

} PIMAGE_eTABLE;

typedef enum _PIMAGE_eTABLE_SRTING
{
	// DGN 65259 (product identifier)
	STRINGS_DGN_65259_MODEL,
	STRINGS_DGN_65259_SN,

	STRINGS_ENUM_COUNT
} PIMAGE_eTABLE_SRTING;


//------------------------------------------------------------------------------
// Public types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------
void   PIMAGE_Initialize( void );
void   PIMAGE_SetValue(    PIMAGE_eTABLE eIndex, int32 i32Value );
int32  PIMAGE_GetValue(    PIMAGE_eTABLE eIndex );
uint8  PIMAGE_u8GetValue(  PIMAGE_eTABLE eIndex );
int8   PIMAGE_s8GetValue(  PIMAGE_eTABLE eIndex );
uint16 PIMAGE_u16GetValue( PIMAGE_eTABLE eIndex );
int16  PIMAGE_s16GetValue( PIMAGE_eTABLE eIndex );
void PIMAGE_SetString(const PIMAGE_eTABLE_SRTING eIndex, const char * string);
void PIMAGE_GetString(const PIMAGE_eTABLE_SRTING eIndex, char * string);

//------------------------------------------------------------------------------
// Public Macros
//------------------------------------------------------------------------------

#endif // CONNECTOR_PREM_RVC
#endif // PIMAGE_H

