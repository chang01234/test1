//------------------------------------------------------------------------------
// Module:      ProDGN.h
//------------------------------------------------------------------------------
// Description: Provides extraction and stuffing functions for proprietary DGNs.
//              All functions herein are MCU and compiler independent.
//
//              Stuffing method in proprietary DGN frames:
//              
//                     Byte0             Byte 1            Byte 2
//              | 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0 | ......
//                <-------------*   <-------------          ...<----
//                |                 |             |                 |
//                 -------------------------------                  |
//                                  |                               |
//                                   -------------------------------
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
#ifndef PROPDGN_H
#define PROPDGN_H
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "StdDefs.h"

//------------------------------------------------------------------------------
// Global constants
//------------------------------------------------------------------------------

// Define air command modes
typedef enum
{
	PROPDGN_eAIRCMD_OFF = 0,
	PROPDGN_eAIRCMD_ON,
	PROPDGN_eAIRCMD_COUNT,
}
PROPDGN_eAIRCMD;

// Define air heater modes
typedef enum
{
	PROPDGN_eAIRMODE_AUTO = 0,
	PROPDGN_eAIRMODE_SILENT,
	PROPDGN_eAIRMODE_VENT,
	PROPDGN_eAIRMODE_COUNT
}
PROPDGN_eAIRMODE;

// Define water command modes
typedef enum
{
	PROPDGN_eWTRCMD_OFF = 0,
	PROPDGN_eWTRCMD_ON,
	PROPDGN_eWTRCMD_COUNT,
}
PROPDGN_eWTRCMD;

// Define water heater modes
typedef enum
{
	PROPDGN_eWTRMODE_ECO = 0,
	PROPDGN_eWTRMODE_HOT,
	PROPDGN_eWTRMODE_BOOST,
	PROPDGN_eWTRMODE_COUNT
}
PROPDGN_eWTRMODE;

// Define water heater modes
typedef enum
{
	PROPDGN_eENERGY_LPG_ONLY=0,
	PROPDGN_eENERGY_LPG_LOW_ELEC,
	PROPDGN_eENERGY_LPG_HIGH_ELEC,
	PROPDGN_eENERGY_LOW_ELEC_ONLY,
	PROPDGN_eENERGY_HIGH_ELEC_ONLY,
	PROPDGN_eENERGY_COUNT
}
PROPDGN_eENERGY;

// Define system units
typedef enum
{
	PROPDGN_eUNITS_METRIC=0,
	PROPDGN_eUNITS_IMPERIAL,
	PROPDGN_eUNITS_COUNT
}
PROPDGN_eUNITS;

// Define AC heater
typedef enum
{
	PROPDGN_eAC_OFF=0,
	PROPDGN_eAC_LOW,
	PROPDGN_eAC_MED,
	PROPDGN_eAC_HIGH
}
PROPDGN_eAC_HEATER;

// Define Gas heater
typedef enum
{
	PROPDGN_eGAS_OFF=0,
	PROPDGN_eGAS_PURGE,
	PROPDGN_eGAS_LOW,
	PROPDGN_eGAS_MED,
	PROPDGN_eGAS_HIGH,
}
PROPDGN_eGAS_HEATER;

// Define water temp
typedef enum
{
	PROPDGN_eWTR_TEMP_COLD=0,
	PROPDGN_eWTR_TEMP_WARM=1,
	PROPDGN_eWTR_TEMP_HOT=2,
	PROPDGN_eWTR_TEMP_NOT_AVAILABLE=6,
	PROPDGN_eWTR_TEMP_IGNORE=7
}
PROPDGN_eWTR_TEMP_HEATER;

//------------------------------------------------------------------------------
// DGN Number   : 65535 (0xFFFF)
// Description  : Blower PID diagnostic message 1 (PCM -> CANalyser)
// Data Length  : 8
// Tx Rate      : 10 ms
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint16 u16RawRpmSetPoint;       // Raw RPM set point
    uint16 u16SlewedRpmSetPoint;    // Slewed RPM set point
    int16  i16RpmAffTerm;           // Acceleration feedforward term
    int16  i16RpmVffTerm;           // Velocity feedforward term
}
PROPDGN_zDGN_65535;

#define     PROPDGN_DGN_65535_SIZE  8
extern void PROPDGN_DGN_65535_Extract( PROPDGN_zDGN_65535* pzDgn65535, uint8* pu8Data );
extern void PROPDGN_DGN_65535_Stuff( uint8* pu8Data, PROPDGN_zDGN_65535* pzDgn65535 );

//------------------------------------------------------------------------------
// DGN Number   : 65534 (0xFFFF)
// Description  : Blower PID diagnostic message 2 (PCM -> CANalyser)
// Data Length  : 8
// Tx Rate      : 10 ms
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct 
{
    int16  i16RpmError;   // RPM error
    int16  i16RpmPTerm;   // P-term
    int16  i16RpmITerm;   // I-term
    int16  i16RpmDTerm;   // D-term
}
PROPDGN_zDGN_65534;

#define     PROPDGN_DGN_65534_SIZE  8
extern void PROPDGN_DGN_65534_Extract( PROPDGN_zDGN_65534* pzDgn65534, uint8* pu8Data );
extern void PROPDGN_DGN_65534_Stuff( uint8* pu8Data, PROPDGN_zDGN_65534* pzDgn65534 );

//------------------------------------------------------------------------------
// DGN Number   : 130559 (0x1FDFF)
// Description  : Heater operation feedback
// Data Length  : 8
// Tx Rate      : 1000 ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8  u8HeaterInstance;               // Heater instance (1 - 253). 0 applies to all heater.
    bit    u4EnergySource              :4; // 0=LPG, 1=LPG+Electric Low, 2=LPG+Electric High, 3=Electric Low, 4=Electric High, 14=Error, 15=Ignored
    bit    u2AirHeaterCmd              :2; // 0=Off, 1=On, 2=Reserved, 3=Ignore
    bit    u2WaterHeaterCmd            :2; // 0=Off, 1=On, 2=Reserved, 3=Ignore
    bit    u4AirHeaterMode             :4; // 0=Auto, 1=Silent, 2=Vent, 14=Error, 15=Ignore
    bit    u4WaterHeaterMode           :4; // 0=Eco, 1=Hot, 2=Boost, 14=Error, 15=Ignore
    int16  i16TargetRoomTemp;              // 0.01 Deg/bit, range=--50..150, 327.66=Error, 327.67=Ignore
    bit    u4SilentModeMaxFan          :4; // 0-7=fan speed select, 14=Error, 15=Ignore
    bit    u4VentModeFanMin            :4; // 0-7=fan speed select, 14=Error, 15=Ignore
    uint8  u8UnderVoltThreshold;           // 0.1V/bit, range=0..25.3, 25.5=Ignore
    bit    u2SystemUnits               :2; // 0 = Metric, 1 = Imperial, 2 = Reserved, 3 = Ignore
    bit    u6Reserved1                 :6;
}
PROPDGN_zDGN_130559;

#define     PROPDGN_DGN_130559_SIZE 8
extern void PROPDGN_DGN_130559_Extract( PROPDGN_zDGN_130559 *pzDest, uint8 *pu8Src );
extern void PROPDGN_DGN_130559_Stuff( uint8 *pu8Dest, PROPDGN_zDGN_130559 *pzSrc );

//------------------------------------------------------------------------------
// DGN Number   : 130558 (0x1FDFE)
// Description  : Heater operation commands (Same structure as DGN 130559)
// Data Length  : 8
// Tx Rate      : 1000 ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
#define PROPDGN_zDGN_130558             PROPDGN_zDGN_130559
#define PROPDGN_DGN_130558_SIZE         PROPDGN_DGN_130559_SIZE
#define PROPDGN_DGN_130558_Extract      PROPDGN_DGN_130559_Extract
#define PROPDGN_DGN_130558_Stuff        PROPDGN_DGN_130559_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130557 (0x1FDFD)
// Description  : Heater status
// Data Length  : 8
// Tx Rate      : 1000 ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8  u8HeaterInstance;              // Heater instance (1 - 253). 0 applies to all heater.
    uint8  u8Reserved1;                   // 0xFF
    uint8  u8Reserved2;                   // 0xFF
    sint16 i16RoomTemp;                   // 0.01 Deg/bit, range=--50..150, 327.66=Error, 327.67=Ignore
    bit    u3WaterTemp             : 3;   // 0=Cold, 1=Warm, 2=Hot, 7=Ignore
    bit    u3Reserved5             : 5;
    bit    u4GasHeaterAir          : 4;   // 0=Off, 1=Purge, 2=Low, 3=Med, 4=High, 15=Ignore
    bit    u4GasHeaterWater        : 4;   // 0=Off, 1=Purge, 2=Low, 3=Med, 4=High, 15=Ignore
    bit    u2ACPresent             : 2;   // 0=No, 1=Yes, 2=Error, 3=Ignore
    bit    u3ACHeaterAir           : 3;   // 0=Off, 1=Low, 2=Med, 3=High, 7=Ignore
    bit    u3ACHeaterWater         : 3;   // 0=Off, 1=Low, 2=Med, 3=High, 7=Ignore
}
PROPDGN_zDGN_130557;

#define     PROPDGN_DGN_130557_SIZE 8
extern void PROPDGN_DGN_130557_Extract( PROPDGN_zDGN_130557* pzDest, uint8* pu8Src );
extern void PROPDGN_DGN_130557_Stuff( uint8* pu8Dest, PROPDGN_zDGN_130557* pzSrc );

//------------------------------------------------------------------------------
// DGN Number   : 130556 (0x1FDFC)
// Description  : HMI Status
// Data Length  : 8
// Tx Rate      : 1000 ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8  u8HMIInstance;                    // HMI instance (1 - 253).
    int16  i16RoomTemperature;               // 0.01 per bit, -50.00 to 150.00, 327.66 = Error, 327.67 = Ignore
    bit    u2HeaterCommunication      : 2;   // 1=Normal, 2=Error, 3=Ignore
    bit    u2InputVoltage             : 2;   // 0=Low, 1=Normal, 2=High, 3=Ignore
    bit    u2HMIInstanceStatus        : 2;   // 1=Normal, 2=Duplicate, 3=Ignore
    bit    u2InternalCircuitry        : 2;   // 1=Normal, 2=Error, 3=Ignore
    bit    u2FavoriteButton           : 2;   // 0=Released, 1=Pressed, 2=Error, 3=Ignore
    bit    u2MenuButton               : 2;   // 0=Released, 1=Pressed, 2=Error, 3=Ignore
    bit    u2HomeButton               : 2;   // 0=Released, 1=Pressed, 2=Error, 3=Ignore
    bit    u2Reserved1                : 2;   // 0x3
    uint8  u8Reserved2;                      // 0xFF
    uint16 u16Reserved3;                     // 0xFFFF
}
PROPDGN_zDGN_130556;

#define     PROPDGN_DGN_130556_SIZE 8
extern void PROPDGN_DGN_130556_Extract( PROPDGN_zDGN_130556* pzDest, uint8* pu8Src );
extern void PROPDGN_DGN_130556_Stuff( uint8* pu8Dest, PROPDGN_zDGN_130556* pzSrc );

//------------------------------------------------------------------------------
// DGN Number   : 130555 (0x1FDFB)
// Description  : Heater scheduling feedback
// Data Length  : 8
// Tx Rate      : 1000ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8  u8HeaterInstance;                 // Heater instance (1 - 253).
    bit    u2AirHtrTimerOffStat      : 2;     // 0=Inactive, 1=Active, 2=Reserved, 3=Ignore
    bit    u6AirHtrTimerOffHour      : 6;     // 0-23, 0x3F = Ignore
    uint8  u8AirHtrTimerOffMin;               // 0-59, 0xFF = Ignore
    bit    u2AirHtrTimerOnStat       : 2;     // 0=Inactive, 1=Active, 2=Reserved, 3=Ignore
    bit    u6AirHtrTimerOnHour       : 6;     // 0-23, 0x3F = Ignore
    uint8  u8AirHtrTimerOnMin;                // 0-59, 0xFF = Ignore
    bit    u2WtrHtrTimerOnStat       : 2;     // 0=Inactive, 1=Active, 2=Reserved, 3=Ignore
    bit    u6WtrHtrTimerOnHour       : 6;     // 0-23, 0x3F = Ignore
    uint8  u8WtrHtrTimerOnMin;                // 0-59, 0xFF = Ignore
    uint8  u8WtrHtrKeepOnTime;                // 0=Infinite. 5-120 minutes. 0xFF = Ignore
}
PROPDGN_zDGN_130555;

#define     PROPDGN_DGN_130555_SIZE 8
extern void PROPDGN_DGN_130555_Extract( PROPDGN_zDGN_130555* pzDest, uint8* pu8Src );
extern void PROPDGN_DGN_130555_Stuff( uint8* pu8Dest, PROPDGN_zDGN_130555* pzSrc );

//------------------------------------------------------------------------------
// DGN Number   : 130554 (0x1FDFA)
// Description  : Heater scheduling commands (Same structure as DGN 130555)
// Data Length  : 8
// Tx Rate      : 1000 ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
#define PROPDGN_zDGN_130554             PROPDGN_zDGN_130555
#define PROPDGN_DGN_130554_SIZE         PROPDGN_DGN_130555_SIZE
#define PROPDGN_DGN_130554_Extract      PROPDGN_DGN_130555_Extract
#define PROPDGN_DGN_130554_Stuff        PROPDGN_DGN_130555_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130553 (0x1FDF9)
// Description  : Heater active faults
// Data Length  : 8
// Tx Rate      : 1000ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8  u8HeaterInstance;                 // Heater instance (1 - 253).
    bit    u2WarningFault             :  2;  // Warning fault active (0=No,1=Yes,3=Ignore)
    bit    u2CriticalFault            :  2;  // Critical fault active (0=No,1=Yes,3=Ignore)
    bit    u2Reserved1                :  2;
    bit    u2Reserved2                :  2;
    uint8  u8Reserved3;
    int    u10FaultCode1;                    // 0 – 1021, 1022 = Reserved, 1023 = Ignore
    int    u10FaultCode2;
    int    u10FaultCode3;
    int    u10FaultCode4;
}
PROPDGN_zDGN_130553;

#define     PROPDGN_DGN_130553_SIZE 8
extern void PROPDGN_DGN_130553_Extract( PROPDGN_zDGN_130553* pzDest, uint8* pu8Src );
extern void PROPDGN_DGN_130553_Stuff( uint8* pu8Dest, PROPDGN_zDGN_130553* pzSrc );


//------------------------------------------------------------------------------
// DGN Number   : 130552 (0x1FDF8)
// Description  : Heater version numbers
// Data Length  : 8
// Tx Rate      : On request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8  u8HeaterInstance;      // Heater instance (1 - 253).
    uint8  u8ComfortSwMajor;      // 0 – 253 = version number, 255 = Ignore
    uint8  u8ComfortSwMinor;      // 0 – 253 = version number, 255 = Ignore
    uint8  u8BurnerSwMajor;       // 0 – 253 = version number, 255 = Ignore
    uint8  u8BurnerSwMinor;       // 0 – 253 = version number, 255 = Ignore
    uint8  u8PcbaVersion;         // 0 – 253 = version number, 255 = Ignore
    uint8  u8ProtocolMajor;       // 0 – 253 = version number, 255 = Ignore
    uint8  u8ProtocolMinor;       // 0 – 253 = version number, 255 = Ignore
}
PROPDGN_zDGN_130552;

#define     PROPDGN_DGN_130552_SIZE  8
extern void PROPDGN_DGN_130552_Extract( PROPDGN_zDGN_130552* pzDest, uint8* pu8Src );
extern void PROPDGN_DGN_130552_Stuff( uint8* pu8Dest, PROPDGN_zDGN_130552* pzSrc );


//------------------------------------------------------------------------------
#endif  // PROPDGN_H


