//------------------------------------------------------------------------------
// Module:      RVCDGN.h
//------------------------------------------------------------------------------
// Description: Provides  NMEA 2000 Standard PGN structure definitions.
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
#ifndef RVCDGN_H
#define RVCDGN_H
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "StdDefs.h"

//------------------------------------------------------------------------------
// General definitions
//------------------------------------------------------------------------------
#define RVCDGN_SINGLE_FRAME_SIZE  8
#define RVCDGN_ISO_REQUEST_SIZE   3
#define RVCDGN_SERIAL_NUMBER_MASK 0x1FFFFF

#define RVCDGN_FUNC_INSTANCE_ALL 0x00
#define RVCDGN_FUNC_INSTANCE_1   0x01
#define RVCDGN_FUNC_INSTANCE_2   0x02

// No data definitions
#define RVCDGN_INT8_NO_DATA   0x7F
#define RVCDGN_INT16_NO_DATA  0x7FFF
#define RVCDGN_INT32_NO_DATA  0x7FFFFFFF
#define RVCDGN_UINT1_NO_DATA  0x1
#define RVCDGN_UINT2_NO_DATA  0x3
#define RVCDGN_UINT3_NO_DATA  0x7
#define RVCDGN_UINT4_NO_DATA  0xF
#define RVCDGN_UINT5_NO_DATA  0x1F
#define RVCDGN_UINT6_NO_DATA  0x3F
#define RVCDGN_UINT7_NO_DATA  0x7F
#define RVCDGN_UINT8_NO_DATA  0xFF
#define RVCDGN_UINT16_NO_DATA 0xFFFF
#define RVCDGN_UINT32_NO_DATA 0xFFFFFFFF

// Error data definitions
#define RVCDGN_INT8_ERROR_DATA   0x7E
#define RVCDGN_INT16_ERROR_DATA  0x7FFE
#define RVCDGN_INT32_ERROR_DATA  0x7FFFFFFE
#define RVCDGN_UINT2_ERROR_DATA  0x2
#define RVCDGN_UINT3_ERROR_DATA  0x6
#define RVCDGN_UINT4_ERROR_DATA  0xE
#define RVCDGN_UINT5_ERROR_DATA  0x1E
#define RVCDGN_UINT6_ERROR_DATA  0x3E
#define RVCDGN_UINT7_ERROR_DATA  0x7E
#define RVCDGN_UINT8_ERROR_DATA  0xFE
#define RVCDGN_UINT16_ERROR_DATA 0xFFFE
#define RVCDGN_UINT32_ERROR_DATA 0xFFFFFFFE

#define RVCDGN_SCHEDULE_MODE_SLEEP  0
#define RVCDGN_SCHEDULE_MODE_WAKE   1
#define RVCDGN_SCHEDULE_MODE_AWAY   2
#define RVCDGN_SCHEDULE_MODE_RETURN 3

#define RVCDGN_VOLT_TO_U8(x)         ((int8)((x)*10))
#define RVCDGN_TEMPERATURE_TO_S8(x)  ((int8)((x)*2))
#define RVCDGN_TEMPERATURE_TO_S16(x) ((int16)((x)*100))
#define RVCDGN_TEMPERATURE_TO_U16(x) ((uint16)(((x) + 273) * 32))

// SPN definitions
#define RVCDGN_SPN_PROPRIETARY_MIN 0x7F000  // Proprietary SPN minimum J1939-73 5.7.1.9 and Appendix F
#define RVCDGN_SPN_PROPRIETARY_MAX 0x7FFFF  // Proprietary SPN maximum J1939-73 5.7.1.9 and Appendix F

// Extraction macros
#define DGN_TO_BITS(dest, src, start, size) dest = (uint8)(((uint8)src >> start) & ((uint8)0xFF >> (8 - size)))
#define DGN_TO_WORD(dest, src)              dest = (((uint16)(*(&(src) + 1)) << 8) + (uint16)src)
#define DGN_TO_DWRD(dest, src)              dest = (((uint32)(*(&(src) + 3)) << 24) + ((uint32)(*(&(src) + 2)) << 16) + ((uint32)(*(&(src) + 1)) << 8) + (uint32)src)

// Stuffing macros (MAKE SURE TO CLEAR THE BUFFER BEFORE STUFFING)
#define BITS_TO_DGN(dest, src, start, size) dest |= (uint8)(((uint8)src << start) & (((uint8)0xFF >> (8 - size)) << start));
#define WORD_TO_DGN(dest, src)                          \
    {                                                   \
        dest = ((uint8)src);                            \
        (*(&(dest) + 1)) = ((uint8)((uint16)src >> 8)); \
    }
#define DWRD_TO_DGN(dest, src)                           \
    {                                                    \
        dest = ((uint8)src);                             \
        (*(&(dest) + 1)) = ((uint8)((uint16)src >> 8));  \
        (*(&(dest) + 2)) = ((uint8)((uint32)src >> 16)); \
        (*(&(dest) + 3)) = ((uint8)((uint32)src >> 24)); \
    }

// Failure Mode Identification codes (FMI). See RV-C / J1939
#define RVCDGN_FMI_ABOVE_NORMAL_RANGE               0
#define RVCDGN_FMI_BELOW_NORMAL_RANGE               1
#define RVCDGN_FMI_DATA_INTERMITTENT                2
#define RVCDGN_FMI_DATA_ERRATIC                     2
#define RVCDGN_FMI_DATA_INCORRECT                   2
#define RVCDGN_FMI_VOLTAGE_ABOVE_NORMAL             3
#define RVCDGN_FMI_VOLTAGE_SHORTED_TO_HIGH_SRC      3
#define RVCDGN_FMI_VOLTAGE_BELOW_NORMAL             4
#define RVCDGN_FMI_VOLTAGE_SHORTED_TO_LOW_SRC       4
#define RVCDGN_FMI_CURRENT_BELOW_NORMAL             5
#define RVCDGN_FMI_CURRENT_OPEN_CIRCUIT             5
#define RVCDGN_FMI_CURRENT_ABOVE_NORMAL             6
#define RVCDGN_FMI_GROUNDED_CIRCUIT                 6
#define RVCDGN_FMI_MECHANICAL_SYSTEM_NOT_RESPONDING 7
#define RVCDGN_FMI_MECHANICAL_SYSTEM_OUT_OF_ADJUST  7
#define RVCDGN_FMI_ABNORMAL_FREQUENCY               8
#define RVCDGN_FMI_ABNORMAL_PULSE_WIDTH             8
#define RVCDGN_FMI_ABNORMAL_PERIOD                  8
#define RVCDGN_FMI_ABNORMAL_UPDATE_RATE             9
#define RVCDGN_FMI_ABNORMAL_RATE_OF_CHANGE          10
#define RVCDGN_FMI_ROOT_CAUSE_NOT_KNOWN             11
#define RVCDGN_FMI_BAD_INTELLIGENT_DEVICE           12
#define RVCDGN_FMI_BAD_INTELLIGENT_COMPONENT        12
#define RVCDGN_FMI_OUT_OF_CALIBRATION               13
#define RVCDGN_FMI_SPECIAL_INSTRUCTIONS             14
#define RVCDGN_FMI_ABOVE_NORMAL_RANGE_LEAST_SEVERE  15
#define RVCDGN_FMI_ABOVE_NORMAL_RANGE_MODERATE_SEV  16
#define RVCDGN_FMI_BELOW_NORMAL_RANGE_LEAST_SEVERE  17
#define RVCDGN_FMI_BELOW_NORMAL_RANGE_MODERATE_SEV  18
#define RVCDGN_FMI_RECEIVED_NETWORK_DATA_IN_ERROR   19
#define RVCDGN_FMI_DATA_DRIFTED_HIGH                20
#define RVCDGN_FMI_DATA_DRIFTED_LOW                 21
#define RVCDGN_FMI_CONDITION_EXIST                  31

//------------------------------------------------------------------------------
// DGN Number   : 65242 (0xFEDA)
// Description  : Product Identification Message
// Data Length  : Variable
// Tx Rate      : On Request
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_DGN_65242_FIELD_SIZE 32
typedef struct
{
    char cId1[RVCDGN_DGN_65242_FIELD_SIZE];
    char cId2[RVCDGN_DGN_65242_FIELD_SIZE];
    char cId3[RVCDGN_DGN_65242_FIELD_SIZE];
    char cId4[RVCDGN_DGN_65242_FIELD_SIZE];
} RVCDGN_zDGN_65242;

#define RVCDGN_DGN_65242_SIZE 64
extern void RVCDGN_DGN_65242_Set(RVCDGN_zDGN_65242 *pzDgn65242, const char *pcId1, const char *pcId2, const char *pcId3, const char *pcId4);
extern void RVCDGN_DGN_65242_Extract(RVCDGN_zDGN_65242 *pzDgn65242, uint8 *pu8Data, uint16 u16Size);
extern uint16 RVCDGN_DGN_65242_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_65242 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 65259 (0xFEEB)
// Description  : Product Identification Message
// Data Length  : 8
// Tx Rate      : On Request
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_DGN_65259_FIELD_SIZE 32
typedef struct
{
    char cMake[RVCDGN_DGN_65259_FIELD_SIZE];
    char cModel[RVCDGN_DGN_65259_FIELD_SIZE];
    char cSerialNumber[RVCDGN_DGN_65259_FIELD_SIZE];
    char cUnitNumber[RVCDGN_DGN_65259_FIELD_SIZE];
} RVCDGN_zDGN_65259;

#define RVCDGN_DGN_65259_SIZE 128
extern void RVCDGN_DGN_65259_Set(RVCDGN_zDGN_65259 *pzDgn65259, const char *pcMake, const char *pcModel, const char *pcSerialNumber, const char *pcUnitNumber);
extern void RVCDGN_DGN_65259_Extract(RVCDGN_zDGN_65259 *pzDgn65259, uint8 *pu8Data, uint16 u16Size);
extern uint16 RVCDGN_DGN_65259_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_65259 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 59904 (0xEA00)
// Description  : ISO Request
// Data Length  : 8
// Tx Rate      :
// Priority     :
//------------------------------------------------------------------------------
typedef struct
{
    uint32 u32DGN;  // requested dgn
    uint8 da;       // destination address, 0xff for global request
} RVCDGN_zDGN_59904;

#define RVCDGN_DGN_59904_SIZE 8
extern void RVCDGN_DGN_59904_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_59904 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 60928 (0xEE00)
// Description  : ISO Address Claim
// Data Length  : 8
// Tx Rate      : On Request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint32 u32UniqueNumber;           // Required if multiple nodes from the same manufacturer may be present on the network
    uint16 u16ManufCode;              // Manufacturer Code
    bit u3NodeInstance : 3;           // For devices implementing multiple RV-C nodes (normally 0)
    bit u5FuncInstance : 5;           // Intended to allow multiple instances of the same RV-C node, normally 0
    uint8 u8Function;                 // Function
    bit u1Reserved : 1;               // Reserved bit
    bit u7VehicleSystem : 7;          // Vehicle System
    bit u4VehicleSystemInstance : 4;  // Vehicle System Instance
    bit u3IndustryGroup : 3;          // Industry Group
    bit u1ArbAddrCapable : 1;         // Arbitrary Address capable (0:No 1:Yes)
} RVCDGN_zDGN_60928;

#define RVCDGN_DGN_60928_SIZE     8
#define RVCDGN_GROUP_GLOBAL       0
#define RVCDGN_GROUP_HIGHWAY      1
#define RVCDGN_GROUP_AGRICULTURAL 2
#define RVCDGN_GROUP_FORESTRY     2
#define RVCDGN_GROUP_CONSTRUCTION 3
#define RVCDGN_GROUP_MARINE       4
#define RVCDGN_GROUP_INDUSTRIAL   5

extern void RVCDGN_DGN_60928_Extract(RVCDGN_zDGN_60928 *pzDgn60928, uint8 *pu8Data);
extern void RVCDGN_DGN_60928_Stuff(uint8 *pu8Data, RVCDGN_zDGN_60928 *pzDgn60928);

//------------------------------------------------------------------------------
// DGN Number   : 130762 (0x1FECA)
// Description  : Diagnostic message (DM_RV).
// Data Length  : 8
// Tx Rate      : 1000ms
// Priority     : 6
//------------------------------------------------------------------------------

typedef struct
{
    bit u2OperatingStatus1 : 2;  // Operating Status (0=Disabled, 1=Enabled, 3=Ignore)
    bit u2OperatingStatus2 : 2;  // Operating Status (0=Standby,  1=Ready,   3=Ignore)
    bit u2YellowLampStatus : 2;  // Yellow lamp status
    bit u2RedLampStatus : 2;     // Red lamp status
    uint8 u8DSA;                 // Default Source Address
    uint8 u8SPN_MSB;             // Service Point Number MSB
    uint8 u8SPN_ISB;             // Service Point Number ISB
    bit u3SPN_LSB : 3;           // Service Point Number LSB
    bit u5FMI : 5;               // Failure Mode Identifier.
    bit u7OccurCount : 7;        // Occurence count (0-126)
    uint8 u8DSAExtension;        // DSA extension
    bit u4BankSelect : 4;        // Bank select
} RVCDGN_zDGN_130762;

#define RVCDGN_DGN_130762_SIZE 8
extern void RVCDGN_DGN_130762_Extract(RVCDGN_zDGN_130762 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_130762_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130762 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 59392 (0xE800)
// Description  : Acknowledgement
// Data Length  : 8
// Tx Rate      :
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Code;            // Acknowledgement code
    uint8 u8Instance;        // Instance of the transmitter, if multi-instanced. FFh if not multi-instanced.
    bit u4instanceBank : 4;  // Instance Bank of transmitter, if multi-banked.
    bit u4Reserved : 4;      // Reserved for compatibility with SAE J1939. Do not parse
    uint8 u8Reserved;        // Reserved for compatibility with SAE J1939. Do not parse
    uint8 u8SourceAddress;   // Source Address being Acknowledged
    uint32 u24Dgn;           // DGN being Acknowledged. LSB first
} RVCDGN_zDGN_59392;

#define RVCDGN_DGN_59392_SIZE 8
extern void RVCDGN_DGN_59392_Extract(RVCDGN_zDGN_59392 *pzDgn, uint8 *pu8Data);

//------------------------------------------------------------------------------
// DGN Number   : 98048 (0x17F00)
// Description  : General purpose reset
// Data Length  : 8
// Tx Rate      :
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    bit u2Reboot : 2;                 // 0=No action, 1=Reboot
    bit u2ClearFaults : 2;            // 0=No action, 1=Clear faults
    bit u2ResetToDefault : 2;         // 0=No action, 1=Restore settings to default values
    bit u2ResetStatistics : 2;        // 0=No action, 1=Reset communication status statistics
    bit u2TestMode : 2;               // 0=Quit testing node, 1=Initiate testing node
    bit u2ResetToOEMSettings : 2;     // 0=No action, 1=Restore settings to default values
    bit u2RebootEnterBootloader : 2;  // 0=No action, 1=Reboot or enter bootloader/programming mode
    bit u2dummy : 2;
    uint8 u8dummy1;
    uint8 u8dummy2;
    uint8 u8dummy3;
    uint8 u8dummy4;
    uint8 u8dummy5;
    uint8 da;
} RVCDGN_zDGN_98048;

#define RVCDGN_DGN_98048_SIZE 8
extern void RVCDGN_DGN_98048_Extract(RVCDGN_zDGN_98048 *pzDgn, uint8 *pu8Data);
extern void RVCDGN_DGN_98048_Stuff(uint8 *pu8Data, RVCDGN_zDGN_98048 *pzDgn);

//------------------------------------------------------------------------------
// DGN Number   : 97536 (0x17D00)
// Description  : Download
// Data Length  : 8
// Tx Rate      :
// Priority     : 7
//------------------------------------------------------------------------------
#define RVCDGN_DGN_97536_SIZE 8

typedef struct
{
    uint8 u8Data[RVCDGN_DGN_97536_SIZE];
} RVCDGN_zDGN_97536;

extern void RVCDGN_DGN_97536_Extract(RVCDGN_zDGN_97536 *pzDgn, uint8 *pu8Data);
extern void RVCDGN_DGN_97536_Stuff(uint8 *pu8Data, RVCDGN_zDGN_97536 *pzDgn);

//------------------------------------------------------------------------------
// DGN Number   : 130972 (0x1FF9C)
// Description  : Ambient Temperature
// Data Length  : 8
// Tx Rate      : 5000ms / On Request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;
    uint16 u16AmbientTemp;
    uint8 u8Reserved1;
    uint16 u16Reserved2;
    uint16 u16Reserved3;
} RVCDGN_zDGN_130972;

#define RVCDGN_DGN_130972_SIZE 8
extern void RVCDGN_DGN_130972_Extract(RVCDGN_zDGN_130972 *pzDgn, uint8 *pu8Data);
extern void RVCDGN_DGN_130972_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130972 *pzDgn);

//------------------------------------------------------------------------------
// DGN Number   : 131070 / 131071 (0x1FFFF)
// Description  : Date Time Command / Status
// Data Length  : 8
// Tx Rate      : 1000ms / On Request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Year;       // Offset: 2000 AD (range = 2000 to 2250)
    uint8 u8Month;      // 1 to 12
    uint8 u8Day;        // 1 to 31
    uint8 u8DayOfWeek;  // 1=Sunday, 2�Monday, ... 7-Saturday
    uint8 u8Hour;       // Hours 0-23 (Local Time)
    uint8 u8Minute;     // 0 to 59
    uint8 u8Second;     // 0 to 59
    uint8 u8TimeZone;   // 0 - Greenwich Mean Time
                        // 4 - Eastern Daylight Time
                        // 5 - Eastern Standard Time
                        // 7 - Pacific Daylight Time
                        // 8 - Pacific Standard Time
                        // 0 - Western European Time
                        // 22 - Central European Summer Time
} RVCDGN_zDGN_131070;

#define RVCDGN_DGN_131070_SIZE 8
extern void RVCDGN_DGN_131070_Extract(RVCDGN_zDGN_131070 *pzDgn, uint8 *pu8Data);
extern void RVCDGN_DGN_131070_Stuff(uint8 *pu8Data, RVCDGN_zDGN_131070 *pzDgn);

//------------------------------------------------------------------------------
typedef RVCDGN_zDGN_131070 RVCDGN_zDGN_131071;
#define RVCDGN_DGN_131071_SIZE    RVCDGN_DGN_131070_SIZE
#define RVCDGN_DGN_131071_Extract RVCDGN_DGN_131070_Extract
#define RVCDGN_DGN_131071_Stuff   RVCDGN_DGN_131070_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130530 (0x1FDE2)
// Description  : Roof Fan Command 2 (ROOF_FAN_COMMAND_2).
// Data Length  : 8
// Tx Rate      : On change/Request
// Priority     : 6
//------------------------------------------------------------------------------

// Roof Fan Command 2
typedef struct
{
    uint8 u8Instance;                 // Instance
    uint8 u8DomeCommand;              // Dome Command (0 = Stop, 1 = Open (Raise), 2 = Close (Lower))
    uint8 u8DesiredDomePosition;      // Desired Dome Position (0 - 200 = Desired Position with 0.5% resolution, 250 = Momentary Operation)
    bit u2RainSensorOverride : 2;     // Rain Sensor Override (0 = Use Rain Sensor,  1 = Override Rain Sensor)
    bit u2SetpointCtrldDome : 2;      // Desired Setpoint Controlled Dome (0 = Dome will not be automatically controlled,  1 = Dome will be automatically controlled)
    bit u2AutoCloseDomeonFanOff : 2;  // Auto Close Dome on Fan Off (0 = Leave dome open when fan shuts off, 1 = Automatically close dome when fan shuts off)
    bit u2AutoFanOffonDomeClose : 2;  // Auto Fan Off on Dome Close (0 = Leave fan on when dome is closed, 1 = Automatically shut fan off when dome is closed)
    bit u2FanSpeedIncDec : 2;         // Fan Speed Increment/Decrement (0 = Decrement fan speed, 1 = Increment fan speed)
    bit u6FanSpeedIncDecStep : 6;     // Fan Speed Increment/Decrement Step (0 = Directly to 0%/100%, 1 = One step, 2-50 = Number of steps for increment/decrement command, 63 = One step)
    uint8 u8Reserved1[3];             // Reserved
} RVCDGN_zDGN_130530;

#define RVCDGN_DGN_130530_SIZE 8
extern void RVCDGN_DGN_130530_Extract(RVCDGN_zDGN_130530 *pzDgn, uint8 *pu8Data);
extern void RVCDGN_DGN_130530_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130530 *pzDgn);
//------------------------------------------------------------------------------
// DGN Number   : 130531 (0x1FDE3)
// Description  : Roof Fan Status 2 (ROOF_FAN_STATUS_2).
// Data Length  : 8
// Tx Rate      : 5000ms or On change
// Priority     : 6
//------------------------------------------------------------------------------

// Roof Fan Status 2
typedef struct
{
    uint8 u8Instance;                  // Instance
    uint8 u8DomeMode;                  // Dome Mode (0 = Stopping, 1 = Opening (Raising), 2 = Closing(Lowering))
    uint8 u8DomePosition;              // Dome Position (0 - 250 = Desired Position with 0.5% resolution)
    bit u2RainSensor : 2;              // Rain Sensor (00b = No Rain Detected, 01b = Rain Detected, 10b = Sensor Error, 11b = Rain Sensor Not Installed)
    bit u2RainSensorOverride : 2;      // Rain Sensor Override (0 = Rain Sensor Used,  1 = Rain Sensor Overridden)
    bit u2SetpointCtrldDomeState : 2;  // Setpoint Controlled Dome State (0 = Dome is not automatically controlled,  1 = Dome is automatically controlled)
    bit u2AutoCloseDomeonFanOff : 2;   // Auto Close Dome on Fan Off (0 = Dome will stay open when fan shuts off, 1 = Dome will automatically close when fan shuts off)
    bit u2AutoFanOffonDomeClose : 2;   // Auto Fan Off on Dome Close (0 = Fan will not automatically shut off when dome closes, 1 = Fan will automatically shut off when dome closes)
    bit u6FanStepsSupported : 6;       // Fan Steps (Speeds) Supported (0 = 200 step resolution (0.5%), 1 = 1 step resolution (0%/100% only), 2 - 50 = Number of steps/speeds supported by fan, 63 = Fan Speed Increment/Decrement not supported)
    uint8 u8Reserved1[3];              // Reserved
} RVCDGN_zDGN_130531;

#define RVCDGN_DGN_130531_SIZE 8
extern void RVCDGN_DGN_130531_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130531 *pzDgn);
extern void RVCDGN_DGN_130531_Extract(RVCDGN_zDGN_130531 *pzDgn, uint8 *pu8Data);

//------------------------------------------------------------------------------
// DGN Number   : 130726 (0x1FEA6)
// Description  : Roof Fan Command 1 (ROOF_FAN_COMMAND_1).
// Data Length  : 8
// Tx Rate      : On change/Request
// Priority     : 6
//------------------------------------------------------------------------------

// Roof Fan Command 1
typedef struct
{
    uint8 u8Instance;               // Instance
    bit u2SystemStatus : 2;         // System Status (00b – Off, 01b – On)
    bit u2FanMode : 2;              // Fan Mode (00b – Auto, 01b – Force On)
    bit u2SpeedMode : 2;            // Speed Mode (00b – Auto (Variable), 01b – Manual)
    bit u2Light : 2;                // Light (00b – Off, 01b – On)
    uint8 u8FanSpeedSetting;        // Fan Speed Setting (0 - 250 = Fan Speed Setting with 0.5% resolution)
    bit u2WindDirectionSwitch : 2;  // Wind Direction Switch (00b – Air Out, 01b – Air In)
    bit u4Reserved1 : 4;            // Deprecated
    bit u2Reserved2 : 2;            // Deprecated
    uint16 u16AmbientTemp;          // Ambient Temperature
    uint16 u16SetPoint;             // Set Point
} RVCDGN_zDGN_130726;

#define RVCDGN_DGN_130726_SIZE 8
extern void RVCDGN_DGN_130726_Extract(RVCDGN_zDGN_130726 *pzDgn, uint8 *pu8Data);
extern void RVCDGN_DGN_130726_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130726 *pzDgn);
//------------------------------------------------------------------------------
// DGN Number   : 130727 (0x1FEA7)
// Description  : Roof Fan Status 1 (ROOF_FAN_STATUS_1).
// Data Length  : 8
// Tx Rate      : 5000ms or On change
// Priority     : 6
//------------------------------------------------------------------------------

// Roof Fan Status 1
typedef struct
{
    uint8 u8Instance;               // Instance
    bit u2SystemStatus : 2;         // System Status (00b – Off, 01b – On)
    bit u2FanMode : 2;              // Fan Mode (00b – Auto, 01b – Force On)
    bit u2SpeedMode : 2;            // Speed Mode (00b – Auto (Variable), 01b – Manual)
    bit u2Light : 2;                // Light (00b – Off, 01b – On)
    uint8 u8FanSpeedSetting;        // Fan Speed Setting (0 - 250 = Fan Speed Setting with 0.5% resolution)
    bit u2WindDirectionSwitch : 2;  // Wind Direction Switch (00b – Air Out, 01b – Air In)
    bit u4DomePosition : 4;         // Dome Position (0000b = Closed, 0001b = ¼ Open, 0010b = ½ Open, 0011b = ¾ Open, 0100b = Open)
    bit u2Reserved1 : 2;            // Deprecated
    uint16 u16AmbientTemp;          // Ambient Temperature
    uint16 u16SetPoint;             // Set Point
} RVCDGN_zDGN_130727;

#define RVCDGN_DGN_130727_SIZE 8
extern void RVCDGN_DGN_130727_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130727 *pzDgn);
extern void RVCDGN_DGN_130727_Extract(RVCDGN_zDGN_130727 *pzDgn, uint8 *pu8Data);

//------------------------------------------------------------------------------
// DGN Number   : 130515 (0x1FDD3h)
// Description  : REFRIGERATOR_STATUS
// Data Length  : 8
// Tx Rate      : 500ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    bit u6Instance : 6;  //  Corresponds to “Zones” in user terminology.
    bit u2Cavity : 2;
    bit u2Light : 2;
    bit u2DoorSwitch : 2;
    bit u4Reserved : 4;
    uint16 u16CurrentTemperature;
    uint16 u16SetTemperature;
    bit u4FuelSource : 4;
    bit u4RefrigeratorMode : 4;
    uint8 u8CompressorSpeed;
} RVCDGN_zDGN_130515;

//------------------------------------------------------------------------------
// DGN Number   : 130514 (0x1FDD2)
// Description  : REFRIGERATOR_COMMAND
// Data Length  : 8
// Tx Rate      : on change/as needed
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    bit u6Instance : 6;  //  Corresponds to “Zones” in user terminology.
    bit u2Cavity : 2;
    bit u2Light : 2;
    bit u6Reserved : 6;
    uint16 u16SetTemperature;
    bit u4FuelSource : 4;
    bit u4RefrigeratorMode : 4;
} RVCDGN_zDGN_130514;

#define RVC_DGN_130515_SIZE 8
#define RVC_DGN_130514_SIZE 8
extern void RVC_DGN_130514_Extract(RVCDGN_zDGN_130514 *pzDest, uint8 *pu8Src);  // cmd
extern void RVC_DGN_130515_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130515 *pzSrc);    // status

//------------------------------------------------------------------------------
// DGN Number   : 131069 (0x1FFFD)
// Description  : DC Source Status 1
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16DCVoltage;
    uint32 u32DCCurrent;
} RVCDGN_zDGN_131069;

#define RVCDGN_DGN_131069_SIZE 8
extern void RVCDGN_DGN_131069_Extract(RVCDGN_zDGN_131069 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131068 (0x1FFFC)
// Description  : DC Source Status 2
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16SourceTemperature;
    uint8 u8StateOfCharge;
    uint16 u16TimeRemaining;
    bit u2TimeRemainingInterpr : 2;
} RVCDGN_zDGN_131068;

#define RVCDGN_DGN_131068_SIZE 8
extern void RVCDGN_DGN_131068_Extract(RVCDGN_zDGN_131068 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131067 (0x1FFFB)
// Description  : DC Source Status 3
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint8 u8StateOfHealth;
    uint16 u16CapacityRemaining;
    uint8 u8RelativeCapacity;
    uint16 u16ACRMSRipple;
} RVCDGN_zDGN_131067;

#define RVCDGN_DGN_131067_SIZE 8
extern void RVCDGN_DGN_131067_Extract(RVCDGN_zDGN_131067 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130761 (0x1FEC9)
// Description  : DC Source Status 4
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint8 u8DesiredChargeState;
    uint16 u16DesiredDCVoltage;
    uint16 u16DesiredDCCurrent;
    uint8 u8BatteryType;
} RVCDGN_zDGN_130761;

#define RVCDGN_DGN_130761_SIZE 8
extern void RVCDGN_DGN_130761_Extract(RVCDGN_zDGN_130761 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130760 (0x1FEC8)
// Description  : DC Source Status 5
// Data Length  : 8
// Tx Rate      : 500 ms in normal operation, 100 ms when over-voltage or fluctuating voltage conditions are active
// Priority     : 6 in normal operation, 2 when over-voltage or fluctuating voltage conditions are active
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;      // DC Instance, see table 6.5.2b
    uint8 u8DevicePriority;  // Device priority, see table 6.5.2b
    uint32 u32HPDCVoltage;   // HP DC voltage, Precision = 0.001 V
    // Bytes 4-7 are deprecated
} RVCDGN_zDGN_130760;

#define RVCDGN_DGN_130760_SIZE 8
extern void RVCDGN_DGN_130760_Extract(RVCDGN_zDGN_130760 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130759 (0x1FEC7)
// Description  : DC Source Status 6
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    bit u2HighVoltageLimitStatus : 2;
    bit u2HighVoltageDisconnectStatus : 2;
    bit u2LowVoltageLimitStatus : 2;
    bit u2LowVoltageDisconnectStatus : 2;
    bit u2LowSOCLimitStatus : 2;
    bit u2LowSOCDisconnectStatus : 2;
    bit u2LowDCSourceTempLimitStatus : 2;
    bit u2LowDCSourceTempDisconnectStatus : 2;
    bit u2HighDCSourceTempLimitStatus : 2;
    bit u2HighDCSourceTempDisconnectStatus : 2;
    bit u2HighCurrentDCSourceLimit : 2;
    bit u2HighCurrentDCSourceDisconnect : 2;
} RVCDGN_zDGN_130759;

#define RVCDGN_DGN_130759_SIZE 8
extern void RVCDGN_DGN_130759_Extract(RVCDGN_zDGN_130759 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130732 (0x1FEAC)
// Description  : DC Source Status 7
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16TodayInputAmpHours;
    uint16 u16TodayOutputAmpHours;
} RVCDGN_zDGN_130732;

#define RVCDGN_DGN_130732_SIZE 8
extern void RVCDGN_DGN_130732_Extract(RVCDGN_zDGN_130732 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130731 (0x1FEAB)
// Description  : DC Source Status 8
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16YesterdayInputAmpHours;
    uint16 u16YesterdayOutputAmpHours;
} RVCDGN_zDGN_130731;

#define RVCDGN_DGN_130731_SIZE 8
extern void RVCDGN_DGN_130731_Extract(RVCDGN_zDGN_130731 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130730 (0x1FEAA)
// Description  : DC Source Status 9
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16DayBeforeYestInputAmpHours;
    uint16 u16DayBeforeYestOutputAmpHours;
} RVCDGN_zDGN_130730;

#define RVCDGN_DGN_130730_SIZE 8
extern void RVCDGN_DGN_130730_Extract(RVCDGN_zDGN_130730 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130729 (0x1FEA9)
// Description  : DC Source Status 10
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16LastSevenDaysInputAmpHours;
    uint16 u16LastSevenDaysOutputAmpHours;
} RVCDGN_zDGN_130729;

#define RVCDGN_DGN_130729_SIZE 8
extern void RVCDGN_DGN_130729_Extract(RVCDGN_zDGN_130729 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130725 (0x1FEA5)
// Description  : DC Source Status 11
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    bit u2DischargeStatus : 2;
    bit u2ChargeStatus : 2;
    bit u2ChargeDetected : 2;
    bit u2ReserveStatus : 2;
    uint16 u16FullCapacity;
    uint16 u16DCPower;
} RVCDGN_zDGN_130725;

#define RVCDGN_DGN_130725_SIZE 8
extern void RVCDGN_DGN_130725_Extract(RVCDGN_zDGN_130725 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130552 (0x1FDF8)
// Description  : DC Source Status 12
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16Cycles;
    uint16 u16DeepsetDischargeDepth;
    uint16 u16AverageDischargeDepth;
} RVCDGN_zDGN_130552;

#define RVCDGN_DGN_130552_SIZE 8
extern void RVCDGN_DGN_130552_Extract(RVCDGN_zDGN_130552 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130535 (0x1FDE7)
// Description  : DC Source Status 13
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8DCInstance;
    uint8 u8DevicePriority;
    uint16 u16LowestDCSourceVoltage;
    uint16 u16HighestDCSourceVoltage;
} RVCDGN_zDGN_130535;

#define RVCDGN_DGN_130535_SIZE 8
extern void RVCDGN_DGN_130535_Extract(RVCDGN_zDGN_130535 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130551 (0x1FDF7)
// Description  : DC Source Configuration Status 1
// Data Length  : 8
// Tx Rate      : on request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8DCInstance;        // Byte 0
    uint8_t u8PeukertExponent;   // Byte 1 (0.01 per bit, 0–253)
    uint8_t u8TempCoefficient;   // Byte 2 (0.1 %cap/°C per bit, 0–200)
    uint8_t u8ChargeEfficiency;  // Byte 3 (%)
    uint8_t u8TimeRemAveraging;  // Byte 4 (minutes)
    uint16_t u16FullCapacity;    // Bytes 5-6 (A-h)
    uint8_t u8TailCurrent;       // Byte 7
} RVCDGN_zDGN_130551;

#define RVCDGN_DGN_130551_SIZE 8
extern void RVCDGN_DGN_130551_Extract(RVCDGN_zDGN_130551 *pzDest, uint8_t *pu8Src);
extern void RVCDGN_DGN_130551_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130551 *pzDgn);

//------------------------------------------------------------------------------
typedef RVCDGN_zDGN_130551 RVCDGN_zDGN_130550;
#define RVCDGN_DGN_130550_SIZE    RVCDGN_DGN_130551_SIZE
#define RVCDGN_DGN_130550_Extract RVCDGN_DGN_130551_Extract
#define RVCDGN_DGN_130550_Stuff   RVCDGN_DGN_130551_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130512 (0x1FDD0)
// Description  : DC Source Connection Status
// Data Length  : 8
// Tx Rate      : on request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8DeviceInstance;
    uint8_t u8DeviceDSA;
    bit u4Function : 4;
    uint8_t u8PrimaryDCInstance;
    uint8_t u8SecondaryDCInstance;
} RVCDGN_zDGN_130512;

#define RVCDGN_DGN_130512_SIZE 8
extern void RVCDGN_DGN_130512_Extract(RVCDGN_zDGN_130512 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130724 (0x1FEA4)
// Description  : DC Source Command (DC_SOURCE_COMMAND)
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8DCInstance;
    bit u2DesiredPowerOnOffStatus : 2;
    bit u2DesiredChargeOnOffStatus : 2;
} RVCDGN_zDGN_130724;

#define RVCDGN_DGN_130724_SIZE 8
extern void RVCDGN_DGN_130724_Extract(RVCDGN_zDGN_130724 *pzDest, uint8_t *pu8Src);
extern void RVCDGN_DGN_130724_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130724 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130548 (0x1FDF4)
// Description  : DC Source Configuration Command 2 (DC_SOURCE_CONFIGURATION_COMMAND_2)
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8DCInstance;          // Byte 0: DC Instance
    bit u2ClearHistory : 2;        // Byte 1, bits 0-1: Clear history (00b = No action, 01b = Clear history)
    bit u2SetCapacity100 : 2;      // Byte 1, bits 2-3: Set capacity to 100% (00b = No action, 01b = Set capacity to 100%)
    bit u2ResetBatteryHealth : 2;  // Byte 1, bits 4-5: Reset Battery Health (00b = No action, 01b = Reset battery health to 100%)
    uint16_t u16ChargedVoltage;    // Bytes 2-3: Charged Voltage (Vdc)
    uint8_t u8ShuntVoltage;        // Byte 4: Shunt Voltage (mV)
    uint16_t u16ShuntCurrent;      // Bytes 5-6: Shunt Current (Adc)
    bit u4BatteryType : 4;         // Byte 7, bits 0-3: Battery Type
} RVCDGN_zDGN_130548;

#define RVCDGN_DGN_130548_SIZE 8
extern void RVCDGN_DGN_130548_Extract(RVCDGN_zDGN_130548 *pzDest, uint8_t *pu8Src);
extern void RVCDGN_DGN_130548_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130548 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130549 (0x1FDF5)
// Description  : DC Source Configuration Status 2 (Same structure as DGN 130548)
// Data Length  : 8
// Tx Rate      : On request
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130549        RVCDGN_zDGN_130548
#define RVCDGN_DGN_130549_SIZE    RVCDGN_DGN_130548_SIZE
#define RVCDGN_DGN_130549_Extract RVCDGN_DGN_130548_Extract

//------------------------------------------------------------------------------
// DGN Number   : 130526 (0x1FDDE)
// Description  : DC Source Configuration Command 3 (DC_SOURCE_CONFIGURATION_COMMAND_3)
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8DeviceInstance;       // Byte 0
    uint8_t u8DeviceDSA;            // Byte 1
    bit u4Function : 4;             // Byte 2, bits 0-3
    uint8_t u8PrimaryDCInstance;    // Byte 3
    uint8_t u8SecondaryDCInstance;  // Byte 4
} RVCDGN_zDGN_130526;

#define RVCDGN_DGN_130526_SIZE 8
extern void RVCDGN_DGN_130526_Extract(RVCDGN_zDGN_130526 *pzDest, uint8_t *pu8Src);
extern void RVCDGN_DGN_130526_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130526 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130768 (0x1FED0)
// Description  : DC Disconnect Status (DC_DISCONNECT_STATUS)
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;             // Byte 0
    bit u2CircuitStatus : 2;        // Byte 1, bits 0-1
    bit u2LastCommand : 2;          // Byte 1, bits 2-3
    bit u2BypassDetect : 2;         // Byte 1, bits 4-5
    bit u2Reserved : 2;             // Byte 1, bits 6-7
    uint16_t u16DCSwitchedVoltage;  // Bytes 2-3
    uint32_t u32DCSwitchedCurrent;  // Bytes 4-7
} RVCDGN_zDGN_130768;

#define RVCDGN_DGN_130768_SIZE 8
extern void RVCDGN_DGN_130768_Extract(RVCDGN_zDGN_130768 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130767 (0x1FECF)
// Description  : DC Disconnect Command (DC_DISCONNECT_COMMAND)
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;      // Byte 0
    bit u2Command : 2;       // Byte 1, bits 0-1
    bit u6Reserved : 6;      // Byte 1, bits 2-7
    uint8_t u8Reserved2[6];  // Bytes 2-7
} RVCDGN_zDGN_130767;

#define RVCDGN_DGN_130767_SIZE 8
extern void RVCDGN_DGN_130767_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130767 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130545 (0x1FDF1)
// Description  : Battery Summary
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8BatteryInstance;
    uint8 u8DCInstance;
    uint8 u8SeriesString;
    uint8 u8ModuleCount;
    uint8 u8CellsPerModule;
    bit u4VoltageStatus : 4;
    bit u4TemperatureStatus : 4;
} RVCDGN_zDGN_130545;

#define RVCDGN_DGN_130545_SIZE 8
extern void RVCDGN_DGN_130545_Extract(RVCDGN_zDGN_130545 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131042 (0x1FFE2)
// Description  : Thermostat Status 1
// Data Length  : 8
// Tx Rate      : 500ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;         //  Corresponds to “Zones” in user terminology.
    bit u4OperatingMode : 4;  //  OperatingMode(0=off,1=cool,2=heat,3=autoHeat/Cool,4=FanOnly,5=AuxHeat,6=WindowDeforest/Humidify)
    bit u2FanMode : 2;        // FanMode (0=Auto,1=ON)
    bit u2SheduleMode : 2;    // SheduleMode (0=Disable,1=Enable)
    uint8 u8FanSpeed;         // One-speed fans shall interpret any nonzero value as “On”.
    uint16 u16SetPointTempHeat;
    uint16 u16SetPointTempCool;
} RVCDGN_zDGN_131042;

#define RVCDGN_DGN_131042_SIZE 8
extern void RVCDGN_DGN_131042_Extract(RVCDGN_zDGN_131042 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_131042_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_131042 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130810 (0x1FEFA)
// Description  : Thermostat Status 2
// Data Length  : 3
// Tx Rate      : 500ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;  //  Corresponds to “Zones” in user terminology.
    uint8 u8CurrentScheduleInstance;
    uint8 u8NumberOfScheduleInstance;
    bit u2ReducedNoiseMode : 2;  // 00-Disabled //01-Enabled
} RVCDGN_zDGN_130810;

#define RVCDGN_DGN_130810_SIZE 8
extern void RVCDGN_DGN_130810_Extract(RVCDGN_zDGN_130810 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_130810_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130810 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130808 (0x1FEF8)
// Description  : Thermostat Command 2
// Data Length  : 2
// Tx Rate      : 500ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;  //  Corresponds to “Zones” in user terminology.
    uint8 u8CurrentScheduleInstance;
    bit u2ReducedNoiseMode : 2;
} RVCDGN_zDGN_130808;

#define RVCDGN_DGN_130808_SIZE 8
extern void RVCDGN_DGN_130808_Extract(RVCDGN_zDGN_130808 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_130808_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130808 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130809 (0x1FEF9)
// Description  : Thermostat Command 1 (Same structure as DGN 131042)
// Data Length  : 8
// Tx Rate      : 500 ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130809        RVCDGN_zDGN_131042
#define RVCDGN_DGN_130809_SIZE    RVCDGN_DGN_131042_SIZE
#define RVCDGN_DGN_130809_Extract RVCDGN_DGN_131042_Extract
#define RVCDGN_DGN_130809_Stuff   RVCDGN_DGN_131042_Stuff

// //------------------------------------------------------------------------------
// // DGN Number   : 130807 (0x1FEF7)
// // Description  : Thermostat Schedule Status 1
// // Data Length  : 8
// // Tx Rate      : 500ms and on change
// // Priority     : 6
// //------------------------------------------------------------------------------
// typedef struct
// {
//     uint8   u8Instance;                        // Corresponds to “Zones” in user terminology.
//     uint8   u8SheduleModeInstance;             // (0=sleep,1=awake,2=away,3=return,4to249=AdditionalInstances,250=Storage)
//     uint8   u8Start_Hour;                      // Precision = 1h, Value range = 0 to 23, 0 - 12:00 AM, 12 – 12:00 Noon ,23 – 11:00 PM, This shall be in Local Time
//     uint8   u8StartMinute;                     // Precision = 1 min, Value range = 0 to 59
//     uint16  u16SetPointTempHeat;               // Deg C
//     uint16  u16SetPointTempCool;               // Deg C
// }
// RVCDGN_zDGN_130807;

// #define     RVCDGN_DGN_130807_SIZE 8
// extern void RVCDGN_DGN_130807_Extract( RVCDGN_zDGN_130807* pzDest, uint8* pu8Src );
// extern void RVCDGN_DGN_130807_Stuff( uint8* pu8Dest, RVCDGN_zDGN_130807* pzSrc );

// //------------------------------------------------------------------------------
// // DGN Number   : 130805 (0x1FEF5)
// // Description  : Thermostat Schedule Command 1
// // Data Length  : 8
// // Tx Rate      : 500ms and on change
// // Priority     : 6
// //------------------------------------------------------------------------------
// #define RVCDGN_zDGN_130805             RVCDGN_zDGN_130807
// #define RVCDGN_DGN_130805_SIZE         RVCDGN_DGN_130807_SIZE
// #define RVCDGN_DGN_130805_Extract      RVCDGN_DGN_130807_Extract
// #define RVCDGN_DGN_130805_Stuff        RVCDGN_DGN_130807_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130778 (0x1FEDA)
// Description  : DC Dimmer Status 3
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;                   //  Corresponds to “Zones” in user terminology.
    uint8 u8Group;                      // Indicates group membership.01111110 – Group 1, 01111101 – Group 2, 00000000b- All groups, 11111111b- No data
    uint8 u8OperatingStatusBrightness;  // 251 = Value is changing (ramp command), 252 = Output is Flashing
    bit u2LockStatus : 2;               // 00b – Load is unlocked,01b – Load is locked, 11b – Lock command is not supported.
    bit u2OvercurrentStatus : 2;        // 00b – Load is not in overcurrent, 01b – Load is in overcurrent, 11b – Overcurrent status is unavailable or not supported.
    bit u2OverrideStatus : 2;           // 00b – External override is inactive, 01b – External override is active, 11b – Override status is unavailable or not supported.
    bit u2EnableStatus : 2;             // 00b – Load is enabled. 01b – Load is disabled. 11b – Enable status is unavailable or not supported.
    uint8 u8DelayDuration;              // 0 = delay/duration has expired, 240 = 240 or more seconds remaining (as in the case of theminute increment values), 253 = out of range (more then 240 seconds remaining),255 = no delay/duration active.
    uint8 u8LastCommand;                // Indicates last command (function) executed by this instance(DC_DIMMER_COMMAND_2).
    bit u2InterlockStatus : 2;          // 00b – Interlock command is not active,01b – Interlock command is active,11b – Interlock command is not supported.
    bit u2LoadStatus : 2;               // 00b – Operating status is zero. 01b – Operating status is non-zero or flashing.
    bit u2ReservedField1 : 2;           // Reserved
    bit u2UnderCurrent : 2;             // 00b – Undercurrent not active 01b – Undercurrent active 10b – Undercurrent status timeout (Error) 11b – Undercurrent not supported
    uint8 u8MasterMemoryValue;          // % Note: This is the last saved brightness that, if the load is currently off, can be restored when it is enabled again.
} RVCDGN_zDGN_130778;

#define RVCDGN_DGN_130778_SIZE 8
extern void RVCDGN_DGN_130778_Extract(RVCDGN_zDGN_130778 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_130778_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130778 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130779 (0x1FEDB)
// Description  : DC Dimmer Command 2
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;                // Instance number the command applies to. Valid = 1 to 250. Set to 0xFF for group commands.
    uint8 u8Group;                   // If bit 7 = 1 and bit 6 = 0, it is a node group.10000001 – Node Group 1,10111111 – Node Group 63, 11111111 – For non-group commands
    uint8 u8DesiredLevelBrightness;  // 250 selects the Dimmed Memory Value, 251 selects the Master Memory Value.
    uint8 u8Command;                 // See Table 6.24.4c for a list of possible commands and explanations.
    uint8 u8DelayDuration;           // Max 240 seconds. Additional minute increment values: 241 = 5 min,242 = 6 min. . ., 250 = 14 min
    bit u2InterlockStatus : 2;       // 00b – no Interlock active, 01b – Interlock A ,10b – Interlock B.
    bit u6ReservedField1 : 6;        // Reserved
    uint8 u8RampTime;                // 0-250 Full ramp time from current brightness to new brightness in 0.1 second increments (0 to 25.0s)
    uint8 u8ReservedField2;          // Reserved

} RVCDGN_zDGN_130779;

#define RVCDGN_DGN_130779_SIZE 8
extern void RVCDGN_DGN_130779_Extract(RVCDGN_zDGN_130779 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_130779_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130779 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 131041 (0x1FFE1)
// Description  : Air conditioner Status
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;
    uint8 u8OperatingMode;
    uint8 u8MaxFanSpeed;
    uint8 u8MaxAirConOutLvl;
    uint8 u8FanSpeed;
    uint8 u8AirConOutLvl;
    uint8 u8DeadBand;
    uint8 u8SecondStageDeadBand;

} RVCDGN_zDGN_131041;

#define RVCDGN_DGN_131041_SIZE 8
extern void RVCDGN_DGN_131041_Extract(RVCDGN_zDGN_131041 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_131041_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_131041 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 131040 (0x1FFE0)
// Description  : Air conditioner Command
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_131040        RVCDGN_zDGN_131041
#define RVCDGN_DGN_131040_SIZE    RVCDGN_DGN_131041_SIZE
#define RVCDGN_DGN_131040_Extract RVCDGN_DGN_131041_Extract
#define RVCDGN_DGN_131040_Stuff   RVCDGN_DGN_131041_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130505 (0x1FDC9)
// Description  : Air conditioner Status 2
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;
    bit u4CompressorStatus : 4;
    bit NotInUseB1 : 4;
    bit NotInUseB21 : 3;
    bit u2ReducedNoiseMode : 2;
    bit NotInUseB22 : 3;
    uint16 u16ExtTemp;
    uint16 u16CoilTemp;
    bit u2CoilTempError : 2;
    bit u2CoilFreezeDetected : 2;
    bit u2ExtTempError : 2;
    bit u2DefrostCycleActive : 2;

} RVCDGN_zDGN_130505;

#define RVCDGN_DGN_130505_SIZE 8
// extern void RVCDGN_DGN_130505_Extract( RVCDGN_zDGN_130972* pzDest, uint8* pu8Src );
extern void RVCDGN_DGN_130505_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130505 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130805 (0x1FEF5)
// Description  : Thermostat schedule command 1
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;              //  Corresponds to “Zones” in user terminology.
    uint8 u8ScheduleModeInstance;  //
    uint8 u8StartHour;
    uint8 u8StartMinute;
    uint16 u16SetpointTempHeat;
    uint16 u16SetpointTempCool;
} RVCDGN_zDGN_130805;

#define RVCDGN_DGN_130805_SIZE 8
extern void RVCDGN_DGN_130805_Extract(RVCDGN_zDGN_130805 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_130805_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130805 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130807 (0x1FEF7)
// Description  : Thermostat schedule status 1
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130807        RVCDGN_zDGN_130805
#define RVCDGN_DGN_130807_SIZE    RVCDGN_DGN_130805_SIZE
#define RVCDGN_DGN_130807_Extract RVCDGN_DGN_130805_Extract
#define RVCDGN_DGN_130807_Stuff   RVCDGN_DGN_130805_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130804 (0x1FEF4)
// Description  : Thermostat schedule command 2
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;              //  Corresponds to “Zones” in user terminology.
    uint8 u8ScheduleModeInstance;  //
    bit u2Sunday : 2;
    bit u2Monday : 2;
    bit u2Tuesday : 2;
    bit u2Wednesday : 2;
    bit u2Thursday : 2;
    bit u2Friday : 2;
    bit u2Saturday : 2;
} RVCDGN_zDGN_130804;

#define RVCDGN_DGN_130804_SIZE 8
extern void RVCDGN_DGN_130804_Extract(RVCDGN_zDGN_130804 *pzDest, uint8 *pu8Src);
extern void RVCDGN_DGN_130804_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130804 *pzSrc);

//------------------------------------------------------------------------------
// DGN Number   : 130806 (0x1FEF6)
// Description  : Thermostat schedule status 2
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130806        RVCDGN_zDGN_130804
#define RVCDGN_DGN_130806_SIZE    8
#define RVCDGN_DGN_130806_Extract RVCDGN_DGN_130804_Extract
#define RVCDGN_DGN_130806_Stuff   RVCDGN_DGN_130804_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130971 (0x1FF9B)
// Description  : Heat Pump Status
// Data Length  : 8
// Tx Rate      : 2000ms and on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8 u8Instance;
    uint8 u8OperatingMode;
    uint8 u8MaxHeatOutputLevel;
    uint8 u8HeatOutputLevel;
    uint8 u8DeadBand;
    uint8 u8SecondStageDeadBand;
    uint8 u8FanSpeed;
} RVCDGN_zDGN_130971;

#define RVCDGN_DGN_130971_SIZE 8
extern void RVCDGN_DGN_130971_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130971 *pzSrc);
extern void RVCDGN_DGN_130971_Extract(RVCDGN_zDGN_130971 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130970 (0x1FF9A)
// Description  : Heat Pump Command
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130970        RVCDGN_zDGN_130971
#define RVCDGN_DGN_130970_SIZE    8
#define RVCDGN_DGN_130970_Extract RVCDGN_DGN_130971_Extract
#define RVCDGN_DGN_130970_Stuff   RVCDGN_DGN_130971_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130776 (0x1FED8)
// Description  : Generic Configuration Status
// Data Length  : 8
// Tx Rate      : 2000ms when configuration required, or on request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8ManufacturerCodeLSB;  // Byte 0: Manufacturer Code (LSB)
    bit u3ManufacturerCodeMSB : 3;  // Byte 1: bits 0-2 Manufacturer Code (MS bits), bits 3-7 Function Instance
    bit u5FunctionInstance : 5;
    uint8_t u8Function;          // Byte 2: Function
    uint8_t u8FirmwareRevision;  // Byte 3: Manufacturer specific firmware revision number
    uint8_t u8ConfigTypeLSB;     // Byte 4: Configuration Type (LSB)
    uint8_t u8ConfigType;        // Byte 5: Configuration Type
    uint8_t u8ConfigTypeMSB;     // Byte 6: Configuration Type (MSB)
    uint8_t u8ConfigRevision;    // Byte 7: Manufacturer specific configuration revision number
} RVCDGN_zDGN_130776;

#define RVCDGN_DGN_130776_SIZE 8
extern void RVCDGN_DGN_130776_Extract(RVCDGN_zDGN_130776 *pzDest, uint8 *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130739 (0x1FEB3)
// Description  : Solar Controller Status
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16ChargeVoltage;
    uint16_t u16ChargeCurrent;
    uint8_t u8ChargeCurrentPercentOfMaximum;
    uint8_t u8OperatingState;
    uint8_t u2PowerUpState : 2;
    uint8_t u2ClearHistory : 2;
    uint8_t u4ForceCharge : 4;
} RVCDGN_zDGN_130739;

#define RVCDGN_DGN_130739_SIZE 8
extern void RVCDGN_DGN_130739_Extract(RVCDGN_zDGN_130739 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130737 (0x1FEB1)
// Description  : Solar Controller Command
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8SolarChargeControllerStatus;
    uint8_t u2DefaultStateOnPowerUp : 2;
    uint8_t u2ClearHistory : 2;
    uint8_t u4ForceCharge : 4;
    uint8_t u8Reserved1[5];
} RVCDGN_zDGN_130737;

#define RVCDGN_DGN_130737_SIZE    8
extern void RVCDGN_DGN_130737_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130737 *pzSrc);
extern void RVCDGN_DGN_130737_Extract(RVCDGN_zDGN_130737 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130693 (0x1FE85)
// Description  : Solar Controller Status 2
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16RatedBatteryVoltage;
    uint16_t u16RatedChargingCurrent;
    uint16_t u16SupportedBatteryTypes;
    uint8_t u2VendorDefinedProprietaryType1 : 2;
    uint8_t u2VendorDefinedProprietaryType2 : 2;
    uint8_t u4Reserved1 : 4;
} RVCDGN_zDGN_130693;

#define RVCDGN_DGN_130693_SIZE 8
extern void RVCDGN_DGN_130693_Extract(RVCDGN_zDGN_130693 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130692 (0x1FE84)
// Description  : Solar Controller Status 3
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16RatedSolarInputVoltage;
    uint16_t u16RatedSolarInputCurrent;
    uint16_t u16RatedSolarOverPower;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130692;

#define RVCDGN_DGN_130692_SIZE 8
extern void RVCDGN_DGN_130692_Extract(RVCDGN_zDGN_130692 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130691 (0x1FE83)
// Description  : Solar Controller Status 4
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16TodayAmpHoursToBattery;
    uint16_t u16YesterdayAmpHoursToBattery;
    uint16_t u16DayBeforeYesterdayAmpHoursToBattery;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130691;

#define RVCDGN_DGN_130691_SIZE 8
extern void RVCDGN_DGN_130691_Extract(RVCDGN_zDGN_130691 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130690 (0x1FE82)
// Description  : Solar Controller Status 5
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16Last7DaysAmpHoursToBattery;
    uint32_t u32CumulativePowerGeneration;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130690;

#define RVCDGN_DGN_130690_SIZE 8
extern void RVCDGN_DGN_130690_Extract(RVCDGN_zDGN_130690 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130689 (0x1FE81)
// Description  : Solar Controller Status 6
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16TotalNumberOfOperatingDays;
    uint16_t u16SolarChargeControllerMeasuredTemperature;
    uint8_t u8Reserved1[3];
} RVCDGN_zDGN_130689;

#define RVCDGN_DGN_130689_SIZE 8
extern void RVCDGN_DGN_130689_Extract(RVCDGN_zDGN_130689 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130688 (0x1FE80)
// Description  : Solar Controller Battery Status
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8Reserved1;
    uint8_t u8Reserved2;
    uint16_t u16MeasuredVoltage;
    uint16_t u16MeasuredCurrent;
    uint8_t u8MeasuredTemperature;
} RVCDGN_zDGN_130688;

#define RVCDGN_DGN_130688_SIZE 8
extern void RVCDGN_DGN_130688_Extract(RVCDGN_zDGN_130688 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130559 (0x1FDFF)
// Description  : Solar Charge Controller Solar Array Status
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16SolarArrayMeasuredVoltage;
    uint16_t u16SolarArrayMeasuredInputCurrent;
    uint8_t u8Reserved1[3];
} RVCDGN_zDGN_130559;

#define RVCDGN_DGN_130559_SIZE 8
extern void RVCDGN_DGN_130559_Extract(RVCDGN_zDGN_130559 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130738 (0x1FEB2)
// Description  : Solar Controller Configuration Status
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8ChargingAlgorithm;
    uint8_t u8ControllerMode;
    uint8_t u2BatterySensorPresent : 2;
    uint8_t u2LinkageMode : 2;
    uint8_t u4BatteryType : 4;
    uint16_t u16BatteryBankSize;
    uint8_t u8Reserved1;
    uint8_t u8MaximumChargingCurrent;
} RVCDGN_zDGN_130738;

#define RVCDGN_DGN_130738_SIZE 8
extern void RVCDGN_DGN_130738_Extract(RVCDGN_zDGN_130738 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130736 (0x1FEB0)
// Description  : Solar Controller Configuration Command
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8ChargingAlgorithm;
    uint8_t u8ControllerMode;
    uint8_t u2BatterySensorPresent : 2;
    uint8_t u2LinkageMode : 2;
    uint8_t u4BatteryType : 4;
    uint16_t u16BatteryBankSize;
    uint8_t u8Reserved1;
    uint8_t u8MaximumChargingCurrent;
} RVCDGN_zDGN_130736;

#define RVCDGN_DGN_130736_SIZE    8
extern void RVCDGN_DGN_130736_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130736 *pzSrc);
extern void RVCDGN_DGN_130736_Extract(RVCDGN_zDGN_130736 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130558 (0x1FDFE)
// Description  : Solar Controller Configuration Status 2
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16BulkAbsorptionVoltage;
    uint16_t u16FloatVoltage;
    uint16_t u16ChargeReturnVoltage;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130558;

#define RVCDGN_DGN_130558_SIZE 8
extern void RVCDGN_DGN_130558_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130558 *pzSrc);
extern void RVCDGN_DGN_130558_Extract(RVCDGN_zDGN_130558 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130557 (0x1FDFD)
// Description  : Solar Controller Configuration Command 2
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130557          RVCDGN_zDGN_130558
#define RVCDGN_DGN_130557_SIZE      RVCDGN_DGN_130558_SIZE
#define RVCDGN_DGN_130557_Stuff     RVCDGN_DGN_130558_Stuff
#define RVCDGN_DGN_130557_Extract   RVCDGN_DGN_130558_Extract

//------------------------------------------------------------------------------
// DGN Number   : 130556 (0x1FDFC)
// Description  : Solar Controller Configuration Status 3
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16UnderVoltageWarningVoltage;
    uint16_t u16BatteryHighVoltageLimitVoltage;
    uint16_t u16BatteryLowVoltageLimitVoltage;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130556;

#define RVCDGN_DGN_130556_SIZE 8
extern void RVCDGN_DGN_130556_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130556 *pzSrc);
extern void RVCDGN_DGN_130556_Extract(RVCDGN_zDGN_130556 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130555 (0x1FDFB)
// Description  : Solar Controller Configuration Command 3
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130555          RVCDGN_zDGN_130556
#define RVCDGN_DGN_130555_SIZE      RVCDGN_DGN_130556_SIZE
#define RVCDGN_DGN_130555_Stuff     RVCDGN_DGN_130556_Stuff
#define RVCDGN_DGN_130555_Extract   RVCDGN_DGN_130556_Extract

//------------------------------------------------------------------------------
// DGN Number   : 130554 (0x1FDFA)
// Description  : Solar Controller Configuration Status 4
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16BatteryHighVoltageLimitReturnVoltage;
    uint16_t u16BatteryLowVoltageLimitReturnVoltage;
    uint8_t u8BatteryLowVoltageLimitTimeDelay;
    uint8_t u8AbsorptionDuration;
    uint8_t u8TemperatureCompensationFactor;
} RVCDGN_zDGN_130554;

#define RVCDGN_DGN_130554_SIZE 8
extern void RVCDGN_DGN_130554_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130554 *pzSrc);
extern void RVCDGN_DGN_130554_Extract(RVCDGN_zDGN_130554 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130553 (0x1FDF9)
// Description  : Solar Controller Configuration Command 4
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130553          RVCDGN_zDGN_130554
#define RVCDGN_DGN_130553_SIZE      RVCDGN_DGN_130554_SIZE
#define RVCDGN_DGN_130553_Stuff     RVCDGN_DGN_130554_Stuff
#define RVCDGN_DGN_130553_Extract   RVCDGN_DGN_130554_Extract

//------------------------------------------------------------------------------
// DGN Number   : 130511 (0x1FDCF)
// Description  : Solar Controller Configuration Status 5
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8ChargerPriority;
    uint8_t u8ExternalTemperatureSensorHighTemperatureLimit;
    uint8_t u8ExternalTemperatureSensorLowTemperatureLimit;
    uint8_t u8Reserved1[4];
} RVCDGN_zDGN_130511;

#define RVCDGN_DGN_130511_SIZE 8
extern void RVCDGN_DGN_130511_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130511 *pzSrc);
extern void RVCDGN_DGN_130511_Extract(RVCDGN_zDGN_130511 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130510 (0x1FDCE)
// Description  : Solar Controller Configuration Command 5
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130510          RVCDGN_zDGN_130511
#define RVCDGN_DGN_130510_SIZE      RVCDGN_DGN_130511_SIZE
#define RVCDGN_DGN_130510_Stuff     RVCDGN_DGN_130511_Stuff
#define RVCDGN_DGN_130510_Extract   RVCDGN_DGN_130511_Extract

//------------------------------------------------------------------------------
// DGN Number   : 130735 (0x1FEAF)
// Description  : Solar Equalization Status
// Data Length  : 8
// Tx Rate      : 1000ms if active
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16TimeRemaining;
    uint8_t u2PreChargingStatus : 2;
    uint8_t u6Reserved1 : 6;
    uint8_t u8TimeSinceLastEqualization;
    uint8_t u8Reserved2[3];
} RVCDGN_zDGN_130735;

#define RVCDGN_DGN_130735_SIZE 8
extern void RVCDGN_DGN_130735_Extract(RVCDGN_zDGN_130735 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130734 (0x1FEAE)
// Description  : Solar Equalization Configuration Status
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16EqualizationVoltage;
    uint16_t u16EqualizationTime;
    uint8_t u8EqualizationInterval;
    uint8_t u8Reserved1[2];
} RVCDGN_zDGN_130734;

#define RVCDGN_DGN_130734_SIZE 8
extern void RVCDGN_DGN_130734_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130734 *pzSrc);
extern void RVCDGN_DGN_130734_Extract(RVCDGN_zDGN_130734 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130733 (0x1FEAD)
// Description  : Solar Equalization Configuration Command
// Data Length  : 8
// Tx Rate      : As needed
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130733          RVCDGN_zDGN_130734
#define RVCDGN_DGN_130733_SIZE      RVCDGN_DGN_130734_SIZE
#define RVCDGN_DGN_130733_Stuff     RVCDGN_DGN_130734_Stuff
#define RVCDGN_DGN_130733_Extract   RVCDGN_DGN_130734_Extract

//------------------------------------------------------------------------------
// DGN Number   : 131031 (0x1FFD7)
// Description  : Inverter AC Status 1
// Data Length  : 8
// Tx Rate      : 500ms
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u4Instance : 4;
    uint8_t u2Line : 2;
    uint8_t u2InputOutput : 2;
    uint16_t u16RMSVoltage;
    uint16_t u16RMSCurrent;
    uint16_t u16Frequency;
    uint8_t u2FaultOpenGround : 2;
    uint8_t u2FaultOpenNeutral : 2;
    uint8_t u2FaultReversePolarity : 2;
    uint8_t u2FaultGroundCurrent : 2;
} RVCDGN_zDGN_131031;

#define RVCDGN_DGN_131031_SIZE 8
extern void RVCDGN_DGN_131031_Extract(RVCDGN_zDGN_131031 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131030 (0x1FFD6)
// Description  : Inverter AC Status 2
// Data Length  : 8
// Tx Rate      : 500ms
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u4Instance : 4;
    uint8_t u2Line : 2;
    uint8_t u2InputOutput : 2;
    uint16_t u16PeakVoltage;
    uint16_t u16PeakCurrent;
    uint16_t u16GroundCurrent;
    uint8_t u8Capacity;
} RVCDGN_zDGN_131030;

#define RVCDGN_DGN_131030_SIZE 8
extern void RVCDGN_DGN_131030_Extract(RVCDGN_zDGN_131030 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131029 (0x1FFD5)
// Description  : Inverter AC Status 3
// Data Length  : 8
// Tx Rate      : 500ms
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u4Instance : 4;
    uint8_t u2Line : 2;
    uint8_t u2InputOutput : 2;
    uint8_t u2Waveform : 2;
    uint8_t u4PhaseStatus : 4;
    uint8_t u2Reserved1 : 2;
    uint16_t u16RealPower;
    uint16_t u16ReactivePower;
    uint8_t u8HarmonicDistortion;
    uint8_t u8ComplementaryLeg;
} RVCDGN_zDGN_131029;

#define RVCDGN_DGN_131029_SIZE 8
extern void RVCDGN_DGN_131029_Extract(RVCDGN_zDGN_131029 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130959 (0x1FF8F)
// Description  : Inverter AC Status 4
// Data Length  : 8
// Tx Rate      : 500ms
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u4Instance : 4;
    uint8_t u2Line : 2;
    uint8_t u2InputOutput : 2;
    uint8_t u8VoltageFault;
    uint8_t u2FaultSurgeProtection : 2;
    uint8_t u2FaultHighFrequency : 2;
    uint8_t u2FaultLowFrequency : 2;
    uint8_t u2BypassModeActive : 2;
    uint8_t u4QualificationStatus : 4;
    uint8_t u4Reserved1 : 4;
    uint8_t u8Reserved2[4];
} RVCDGN_zDGN_130959;

#define RVCDGN_DGN_130959_SIZE 8
extern void RVCDGN_DGN_130959_Extract(RVCDGN_zDGN_130959 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130792 (0x1FEE8)
// Description  : Inverter DC Status
// Data Length  : 8
// Tx Rate      : 5000ms on request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16DCVoltage;
    uint16_t u16DCAmperage;
    uint8_t u8Reserved1[3];
} RVCDGN_zDGN_130792;

#define RVCDGN_DGN_130792_SIZE 8
extern void RVCDGN_DGN_130792_Extract(RVCDGN_zDGN_130792 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131028 (0x1FFD4)
// Description  : Inverter Status
// Data Length  : 8
// Tx Rate      : 500ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8Status;
    uint8_t u2BatteryTemperatureSensorPresent : 2;
    uint8_t u2LoadSenseEnabled : 2;
    uint8_t u2InverterEnabled : 2;
    uint8_t u2PassThroughEnable : 2;
    uint8_t u2GeneratorSupportEnabled : 2;
    uint8_t u6Reserved1 : 6;
    uint8_t u8Reserved2[4];
} RVCDGN_zDGN_131028;

#define RVCDGN_DGN_131028_SIZE 8
extern void RVCDGN_DGN_131028_Extract(RVCDGN_zDGN_131028 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131027 (0x1FFD3)
// Description  : Inverter Command
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u2InverterEnabled : 2;
    uint8_t u2LoadSenseEnabled : 2;
    uint8_t u2PassThroughEnable : 2;
    uint8_t u2GeneratorSupportEnabled : 2;
    uint8_t u8Reserved1[5];
    uint8_t u2InverterEnableOnStartup : 2;
    uint8_t u2LoadSenseEnableOnStartup : 2;
    uint8_t u2ACPassThroughEnableOnStartup : 2;
    uint8_t u2GeneratorSupportEnableOnStartup : 2;
} RVCDGN_zDGN_131027;

#define RVCDGN_DGN_131027_SIZE    8
extern void RVCDGN_DGN_131027_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131027 *pzSrc);
extern void RVCDGN_DGN_131027_Extract(RVCDGN_zDGN_131027 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131026 (0x1FFD2)
// Description  : Inverter Configuration Status 1
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16LoadSensePowerThreshold;
    uint16_t u16LoadSenseInterval;
    uint16_t u16DCSourceShutdownVoltageMinimum;
    uint8_t u2InverterEnableOnStartup : 2;
    uint8_t u2LoadSenseEnableOnStartup : 2;
    uint8_t u2ACPassThroughEnableOnStartup : 2;
    uint8_t u2Reserved1 : 2;
} RVCDGN_zDGN_131026;

#define RVCDGN_DGN_131026_SIZE 8
extern void RVCDGN_DGN_131026_Extract(RVCDGN_zDGN_131026 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131024 (0x1FFD0)
// Description  : Inverter Configuration Command 1
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16LoadSensePowerThreshold;
    uint16_t u16LoadSenseInterval;
    uint16_t u16DCSourceShutdownVoltageMinimum;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_131024;

#define RVCDGN_DGN_131024_SIZE    8
extern void RVCDGN_DGN_131024_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131024 *pzSrc);
extern void RVCDGN_DGN_131024_Extract(RVCDGN_zDGN_131024 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131025 (0x1FFD1)
// Description  : Inverter Configuration Status 2
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16DCSourceShutdownVoltageMaximum;
    uint16_t u16DCSourceWarningVoltageMinimum;
    uint16_t u16DCSourceWarningVoltageMaximum;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_131025;

#define RVCDGN_DGN_131025_SIZE 8
extern void RVCDGN_DGN_131025_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131025 *pzSrc);
extern void RVCDGN_DGN_131025_Extract(RVCDGN_zDGN_131025 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131023 (0x1FFCF)
// Description  : Inverter Configuration Command 2
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_131023        RVCDGN_zDGN_131025
#define RVCDGN_DGN_131023_SIZE    8
#define RVCDGN_DGN_131023_Extract RVCDGN_DGN_131025_Extract
#define RVCDGN_DGN_131023_Stuff   RVCDGN_DGN_131025_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130766 (0x1FECE)
// Description  : Inverter Configuration Status 3
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16DCSourceShutdownDelay;
    uint8_t u8StackMode;
    uint16_t u16DCSourceShutdownRecoveryLevel;
    uint16_t u16GeneratorSupportEngageCurrent;
} RVCDGN_zDGN_130766;

#define RVCDGN_DGN_130766_SIZE 8
extern void RVCDGN_DGN_130766_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130766 *pzSrc);
extern void RVCDGN_DGN_130766_Extract(RVCDGN_zDGN_130766 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130765 (0x1FECD)
// Description  : Inverter Configuration Command 3
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130765        RVCDGN_zDGN_130766
#define RVCDGN_DGN_130765_SIZE    8
#define RVCDGN_DGN_130765_Extract RVCDGN_DGN_130766_Extract
#define RVCDGN_DGN_130765_Stuff   RVCDGN_DGN_130766_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130715 (0x1FE9B)
// Description  : Inverter Configuration Status 4
// Data Length  : 8
// Tx Rate      : On change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16OutputACVoltage;
    uint8_t u8OutputFrequency;
    uint16_t u16ACOutputPowerLimit;
    uint16_t u16ACOutputTimeLimit;
} RVCDGN_zDGN_130715;

#define RVCDGN_DGN_130715_SIZE 8
extern void RVCDGN_DGN_130715_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130715 *pzSrc);
extern void RVCDGN_DGN_130715_Extract(RVCDGN_zDGN_130715 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130714 (0x1FE9A)
// Description  : Inverter Configuration Command 4
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130714        RVCDGN_DGN_130715_SIZE
#define RVCDGN_DGN_130714_SIZE    8
#define RVCDGN_DGN_130714_Extract RVCDGN_DGN_130715_Extract
#define RVCDGN_DGN_130714_Stuff   RVCDGN_DGN_130715_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130749 (0x1FEBD)
// Description  : Inverter Temperature Status
// Data Length  : 8
// Tx Rate      : 500 ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16FET1Temperature;
    uint16_t u16TransformerTemperature;
    uint16_t u16FET2Temperature;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130749;

#define RVCDGN_DGN_130749_SIZE 8
extern void RVCDGN_DGN_130749_Extract(RVCDGN_zDGN_130749 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130507 (0x1FDCB)
// Description  : Inverter Temperature Status 2
// Data Length  : 8
// Tx Rate      : 500 ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16ControlPowerBoardTemperature;
    uint16_t u16CapacitorTemperature;
    uint16_t u16AmbientTemperature;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130507;

#define RVCDGN_DGN_130507_SIZE 8
extern void RVCDGN_DGN_130507_Extract(RVCDGN_zDGN_130507 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131018 (0x1FFCA)
// Description  : Charger AC Status 1
// Data Length  : 8
// Tx Rate      : 500ms when charging, 5000ms when not charging
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u4Instance : 4;
    uint8_t u2Line : 2;
    uint8_t u2InputOutput : 2;
    uint16_t u16RMSVoltage;
    uint16_t u16RMSCurrent;
    uint16_t u16Frequency;
    uint8_t u2FaultOpenGround : 2;
    uint8_t u2FaultOpenNeutral : 2;
    uint8_t u2FaultReversePolarity : 2;
    uint8_t u2FaultGroundCurrent : 2;
  
} RVCDGN_zDGN_131018;

#define RVCDGN_DGN_131018_SIZE 8
extern void RVCDGN_DGN_131018_Extract(RVCDGN_zDGN_131018 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131016 (0x1FFC8)
// Description  : Charger AC Status 3
// Data Length  : 8
// Tx Rate      : 500ms when charging, 5000ms when not charging
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u4Instance : 4;
    uint8_t u2Line : 2;
    uint8_t u2InputOutput : 2;
    uint8_t u2Waveform : 2;
    uint8_t u4PhaseStatus : 4;
    uint8_t u2Reserved1 : 2;
    uint16_t u16RealPower;
    uint16_t u16ReactivePower;
    uint8_t u8HarmonicDistortion;
    uint8_t u8ComplementaryLeg;
} RVCDGN_zDGN_131016;

#define RVCDGN_DGN_131016_SIZE 8
extern void RVCDGN_DGN_131016_Extract(RVCDGN_zDGN_131016 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131015 (0x1FFC7)
// Description  : Charger Status
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16ChargeVoltage;
    uint16_t u16ChargeCurrent;
    uint8_t u8ChargeCurrentPercentOfMaximum;
    uint8_t u8OperatingState;
    uint8_t u2DefaultStateOnPowerUp : 2;
    uint8_t u2AutoRechargeEnable : 2;
    uint8_t u4ForceCharge : 4;
} RVCDGN_zDGN_131015;

#define RVCDGN_DGN_131015_SIZE 8
extern void RVCDGN_DGN_131015_Extract(RVCDGN_zDGN_131015 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131013 (0x1FFC5)
// Description  : Charger Command
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8Status;
    uint8_t u2DefaultStateOnPowerUp : 2;
    uint8_t u2AutoRechargeEnable : 2;
    uint8_t u4ForceCharge : 4;
    uint16_t u16ControlVoltageForCCCVMode;
    uint16_t u16ControlCurrentForCCCVMode;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_131013;

#define RVCDGN_DGN_131013_SIZE    8
extern void RVCDGN_DGN_131013_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131013 *pzSrc);
extern void RVCDGN_DGN_131013_Extract(RVCDGN_zDGN_131013 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130723 (0x1FEA3)
// Description  : Charger Status 2
// Data Length  : 8
// Tx Rate      : 5000ms or on change
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8Reserved1;
    uint8_t u8ChargerPriority;
    uint16_t u16ChargingVoltage;
    uint16_t u16ChargingCurrent;
    uint8_t u8ChargerTemperature;
} RVCDGN_zDGN_130723;

#define RVCDGN_DGN_130723_SIZE 8
extern void RVCDGN_DGN_130723_Extract(RVCDGN_zDGN_130723 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131014 (0x1FFC6)
// Description  : Charger Configuration Status
// Data Length  : 8
// Tx Rate      : On charge
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8ChargingAlgorithm;
    uint8_t u8ChargerMode;
    uint8_t u2BatterySensorPresent : 2;
    uint8_t u2ChargerInstallationLine : 2;
    uint8_t u4BatteryType : 4;
    uint16_t u16BatteryBankSize;
    uint16_t u16MaximumChargingCurrent;
} RVCDGN_zDGN_131014;

#define RVCDGN_DGN_131014_SIZE 8
extern void RVCDGN_DGN_131014_Extract(RVCDGN_zDGN_131014 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 131012 (0x1FFC4)
// Description  : Charger Configuration Command
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8ChargingAlgorithm;
    uint8_t u8ChargerMode;
    uint8_t u2BatterySensorPresent : 2;
    uint8_t u2ChargerInstallationLine : 2;
    uint8_t u4Reserved1 : 4;
    uint16_t u16BatteryBankSize;
    uint8_t u4BatteryType : 4;
    uint8_t u4Reserved2 : 4;
    uint8_t u8MaximumChargingCurrent;
} RVCDGN_zDGN_131012;

#define RVCDGN_DGN_131012_SIZE    8
extern void RVCDGN_DGN_131012_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131012 *pzSrc);
extern void RVCDGN_DGN_131012_Extract(RVCDGN_zDGN_131012 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130966 (0x1FF96)
// Description  : Charger Configuration Status 2
// Data Length  : 8
// Tx Rate      : On charge
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8MaximumChargeCurrentAsPercent;
    uint8_t u8ChargeRateLimitAsPercentOfBankSize;
    uint8_t u8ShoreBreakerSize;
    uint8_t u8DefaultBatteryTemperature;
    uint16_t u16RechargeVoltage;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130966;

#define RVCDGN_DGN_130966_SIZE 8
extern void RVCDGN_DGN_130966_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130966 *pzSrc);
extern void RVCDGN_DGN_130966_Extract(RVCDGN_zDGN_130966 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130965 (0x1FF95)
// Description  : Charger Configuration Command 2
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130965        RVCDGN_zDGN_130966
#define RVCDGN_DGN_130965_SIZE    8
#define RVCDGN_DGN_130965_Extract RVCDGN_DGN_130966_Extract
#define RVCDGN_DGN_130965_Stuff   RVCDGN_DGN_130966_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130764 (0x1FECC)
// Description  : Charger Configuration Status 3
// Data Length  : 8
// Tx Rate      : On charge
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16BulkVoltage;
    uint16_t u16AbsorptionVoltage;
    uint16_t u16FloatVoltage;
    uint8_t u8TemperatureCompensationConstant;
} RVCDGN_zDGN_130764;

#define RVCDGN_DGN_130764_SIZE 8
extern void RVCDGN_DGN_130764_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130764 *pzSrc);
extern void RVCDGN_DGN_130764_Extract(RVCDGN_zDGN_130764 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130763 (0x1FECB)
// Description  : Charger Configuration Command 3
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130763        RVCDGN_zDGN_130764
#define RVCDGN_DGN_130763_SIZE    8
#define RVCDGN_DGN_130763_Extract RVCDGN_DGN_130764_Extract
#define RVCDGN_DGN_130763_Stuff   RVCDGN_DGN_130764_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130751 (0x1FEBF)
// Description  : Charger Configuration Status 4
// Data Length  : 8
// Tx Rate      : On charge
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16BulkTime;
    uint16_t u16AbsorptionTime;
    uint16_t u16FloatTime;
    uint8_t u8Reserved1;
} RVCDGN_zDGN_130751;

#define RVCDGN_DGN_130751_SIZE 8
extern void RVCDGN_DGN_130751_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130751 *pzSrc);
extern void RVCDGN_DGN_130751_Extract(RVCDGN_zDGN_130751 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130750 (0x1FEBE)
// Description  : Charger Configuration Command 4
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130750        RVCDGN_zDGN_130751
#define RVCDGN_DGN_130750_SIZE    8
#define RVCDGN_DGN_130750_Extract RVCDGN_DGN_130751_Extract
#define RVCDGN_DGN_130750_Stuff   RVCDGN_DGN_130751_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130968 (0x1FF98)
// Description  : Charger Equalization Configuration Status
// Data Length  : 8
// Tx Rate      : On charge
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint16_t u16EqualizationVoltage;
    uint16_t u16EqualizationTime;
    uint8_t u8Reserved1[3];
} RVCDGN_zDGN_130968;

#define RVCDGN_DGN_130968_SIZE 8
extern void RVCDGN_DGN_130968_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130968 *pzSrc);
extern void RVCDGN_DGN_130968_Extract(RVCDGN_zDGN_130968 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130967 (0x1FF97)
// Description  : Charger Equalization Configuration Command
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130967        RVCDGN_zDGN_130968
#define RVCDGN_DGN_130967_SIZE    8
#define RVCDGN_DGN_130967_Extract RVCDGN_DGN_130968_Extract
#define RVCDGN_DGN_130967_Stuff   RVCDGN_DGN_130968_Stuff

//------------------------------------------------------------------------------
// DGN Number   : 130953 (0x1FF89)
// Description  : Charger AC FAULT Configuration Status 1
// Data Length  : 8
// Tx Rate      : On charge
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t u8Instance;
    uint8_t u8ExtremeLowVoltageLevel;
    uint8_t u8LowVoltageLevel;
    uint8_t u8HighVoltageLevel;
    uint8_t u8ExtremeHighVoltageLevel;
    uint8_t u8QualificationTime;
    uint8_t u2BypassMode : 2;
    uint8_t u6Reserved1 : 6;
    uint8_t u8Reserved2;
} RVCDGN_zDGN_130953;

#define RVCDGN_DGN_130953_SIZE 8
extern void RVCDGN_DGN_130953_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130953 *pzSrc);
extern void RVCDGN_DGN_130953_Extract(RVCDGN_zDGN_130953 *pzDest, uint8_t *pu8Src);

//------------------------------------------------------------------------------
// DGN Number   : 130951 (0x1FF87)
// Description  : Charger ACFAULT Configuration Command 1
// Data Length  : 8
// Tx Rate      : NA
// Priority     : 6
//------------------------------------------------------------------------------
#define RVCDGN_zDGN_130951        RVCDGN_zDGN_130953
#define RVCDGN_DGN_130951_SIZE    8
#define RVCDGN_DGN_130951_Extract RVCDGN_DGN_130953_Extract
#define RVCDGN_DGN_130951_Stuff   RVCDGN_DGN_130953_Stuff
//------------------------------------------------------------------------------
#endif
