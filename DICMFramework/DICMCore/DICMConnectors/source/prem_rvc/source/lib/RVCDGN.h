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
#define RVCDGN_SINGLE_FRAME_SIZE    8
#define RVCDGN_ISO_REQUEST_SIZE   	3
#define RVCDGN_SERIAL_NUMBER_MASK   0x1FFFFF

#define RVCDGN_FUNC_INSTANCE_ALL    0x00
#define RVCDGN_FUNC_INSTANCE_1      0x01

// No data definitions
#define RVCDGN_INT8_NO_DATA       	0x7F
#define RVCDGN_INT16_NO_DATA      	0x7FFF
#define RVCDGN_INT32_NO_DATA      	0x7FFFFFFF
#define RVCDGN_UINT1_NO_DATA      	0x1
#define RVCDGN_UINT2_NO_DATA      	0x3
#define RVCDGN_UINT3_NO_DATA      	0x7
#define RVCDGN_UINT4_NO_DATA      	0xF
#define RVCDGN_UINT5_NO_DATA      	0x1F
#define RVCDGN_UINT6_NO_DATA      	0x3F
#define RVCDGN_UINT7_NO_DATA      	0x7F
#define RVCDGN_UINT8_NO_DATA      	0xFF
#define RVCDGN_UINT16_NO_DATA     	0xFFFF
#define RVCDGN_UINT32_NO_DATA     	0xFFFFFFFF

// Error data definitions
#define RVCDGN_INT8_ERROR_DATA    	0x7E
#define RVCDGN_INT16_ERROR_DATA   	0x7FFE
#define RVCDGN_INT32_ERROR_DATA   	0x7FFFFFFE
#define RVCDGN_UINT2_ERROR_DATA   	0x2
#define RVCDGN_UINT3_ERROR_DATA   	0x6
#define RVCDGN_UINT4_ERROR_DATA   	0xE
#define RVCDGN_UINT5_ERROR_DATA   	0x1E
#define RVCDGN_UINT6_ERROR_DATA   	0x3E
#define RVCDGN_UINT7_ERROR_DATA   	0x7E
#define RVCDGN_UINT8_ERROR_DATA   	0xFE
#define RVCDGN_UINT16_ERROR_DATA  	0xFFFE
#define RVCDGN_UINT32_ERROR_DATA  	0xFFFFFFFE

#define RVCDGN_SCHEDULE_MODE_SLEEP    0
#define RVCDGN_SCHEDULE_MODE_WAKE     1
#define RVCDGN_SCHEDULE_MODE_AWAY     2
#define RVCDGN_SCHEDULE_MODE_RETURN   3

#define RVCDGN_VOLT_TO_U8(x)          ((int8)((x)*10))
#define RVCDGN_TEMPERATURE_TO_S8(x)   ((int8)((x)*2))
#define RVCDGN_TEMPERATURE_TO_S16(x)  ((int16)((x)*100))
#define RVCDGN_TEMPERATURE_TO_U16(x)  ((uint16)(((x)+273)*32))

// SPN definitions
#define RVCDGN_SPN_PROPRIETARY_MIN	0x7F000 	// Proprietary SPN minimum J1939-73 5.7.1.9 and Appendix F
#define RVCDGN_SPN_PROPRIETARY_MAX	0x7FFFF 	// Proprietary SPN maximum J1939-73 5.7.1.9 and Appendix F

// Extraction macros
#define DGN_TO_BITS(dest,src,start,size)  dest=(uint8)(((uint8)src >> start) & ((uint8)0xFF >> (8 - size)))
#define DGN_TO_WORD(dest,src)             dest=(((uint16)(*(&(src)+1)) << 8) + (uint16)src)
#define DGN_TO_DWRD(dest,src)             dest=(((uint32)(*(&(src)+3)) << 24) + ((uint32)(*(&(src)+2)) << 16) + ((uint32)(*(&(src)+1)) << 8 ) + (uint32)src)

// Stuffing macros (MAKE SURE TO CLEAR THE BUFFER BEFORE STUFFING)
#define BITS_TO_DGN(dest,src,start,size)  dest|=(uint8)(((uint8)src << start) & (((uint8)0xFF >> (8 - size)) << start));
#define WORD_TO_DGN(dest,src)             {dest=((uint8)src);(*(&(dest)+1))=((uint8)((uint16)src >> 8));}
#define DWRD_TO_DGN(dest,src)             {dest=((uint8)src);(*(&(dest)+1))=((uint8)((uint16)src >> 8));(*(&(dest)+2))=((uint8)((uint32)src >> 16));(*(&(dest)+3))=((uint8)((uint32)src >> 24));}

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
	char cId1[ RVCDGN_DGN_65242_FIELD_SIZE ];
	char cId2[ RVCDGN_DGN_65242_FIELD_SIZE ];
	char cId3[ RVCDGN_DGN_65242_FIELD_SIZE ];
	char cId4[ RVCDGN_DGN_65242_FIELD_SIZE ];
}
RVCDGN_zDGN_65242;

#define       RVCDGN_DGN_65242_SIZE  64
extern void   RVCDGN_DGN_65242_Set( RVCDGN_zDGN_65242* pzDgn65242, const char* pcId1, const char* pcId2, const char* pcId3, const char* pcId4 );
extern void   RVCDGN_DGN_65242_Extract( RVCDGN_zDGN_65242* pzDgn65242, uint8* pu8Data, uint16 u16Size );
extern uint16 RVCDGN_DGN_65242_Stuff( uint8* pu8Dest, RVCDGN_zDGN_65242* pzSrc );

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
	char cMake[         RVCDGN_DGN_65259_FIELD_SIZE ];
	char cModel[        RVCDGN_DGN_65259_FIELD_SIZE ];
	char cSerialNumber[ RVCDGN_DGN_65259_FIELD_SIZE ];
	char cUnitNumber[   RVCDGN_DGN_65259_FIELD_SIZE ];
}
RVCDGN_zDGN_65259;

#define       RVCDGN_DGN_65259_SIZE  128
extern void   RVCDGN_DGN_65259_Set( RVCDGN_zDGN_65259* pzDgn65259, const char* pcMake, const char* pcModel, const char* pcSerialNumber, const char* pcUnitNumber );
extern void   RVCDGN_DGN_65259_Extract( RVCDGN_zDGN_65259* pzDgn65259, uint8* pu8Data, uint16 u16Size );
extern uint16 RVCDGN_DGN_65259_Stuff( uint8* pu8Dest, RVCDGN_zDGN_65259* pzSrc );

//------------------------------------------------------------------------------
// DGN Number   : 59904 (0xEA00)
// Description  : ISO Request
// Data Length  : 8
// Tx Rate      :
// Priority     :
//------------------------------------------------------------------------------
typedef struct
{
    uint32 u32DGN;
}
RVCDGN_zDGN_59904;

#define     RVCDGN_DGN_59904_SIZE 3
extern void RVCDGN_DGN_59904_Stuff( uint8 *pu8Dest, RVCDGN_zDGN_59904 *pzSrc );

//------------------------------------------------------------------------------
// DGN Number   : 60928 (0xEE00)
// Description  : ISO Address Claim
// Data Length  : 8
// Tx Rate      : On Request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
    uint32 u32UniqueNumber;             // Required if multiple nodes from the same manufacturer may be present on the network
    uint16 u16ManufCode;                // Manufacturer Code
    bit    u3NodeInstance          :3;  // For devices implementing multiple RV-C nodes (normally 0)
    bit    u5FuncInstance          :5;  // Intended to allow multiple instances of the same RV-C node, normally 0
    uint8  u8Function;                  // Function
    bit    u1Reserved               :1; // Reserved bit
    bit    u7VehicleSystem          :7; // Vehicle System
    bit    u4VehicleSystemInstance  :4; // Vehicle System Instance
    bit    u3IndustryGroup          :3; // Industry Group
    bit    u1ArbAddrCapable         :1; // Arbitrary Address capable (0:No 1:Yes)
}
RVCDGN_zDGN_60928;

#define RVCDGN_DGN_60928_SIZE       8
#define RVCDGN_GROUP_GLOBAL         0
#define RVCDGN_GROUP_HIGHWAY        1
#define RVCDGN_GROUP_AGRICULTURAL   2
#define RVCDGN_GROUP_FORESTRY       2
#define RVCDGN_GROUP_CONSTRUCTION   3
#define RVCDGN_GROUP_MARINE         4
#define RVCDGN_GROUP_INDUSTRIAL     5

extern void RVCDGN_DGN_60928_Extract( RVCDGN_zDGN_60928* pzDgn60928, uint8* pu8Data );
extern void RVCDGN_DGN_60928_Stuff( uint8* pu8Data, RVCDGN_zDGN_60928* pzDgn60928 );

//------------------------------------------------------------------------------
// DGN Number   : 130762 (0x1FECA)
// Description  : Diagnostic message (DM_RV).
// Data Length  : 8
// Tx Rate      : 1000ms
// Priority     : 6
//------------------------------------------------------------------------------

// DMRV Extracting /Stuffing session instance
typedef struct
{
    uint8*  pu8Data;       // Buffer address
    uint16  u16Index;      // Buffer index
    uint16  u16Capacity;   // Buffer capacity
}
RVCDGN_zDGN_130762_Session;

// RV_DM lamp status
typedef struct
{
    bit   u2OperatingStatus1 :2;     // Operating Status (0=Disabled, 1=Enabled, 3=Ignore)
    bit   u2OperatingStatus2 :2;     // Operating Status (0=Standby,  1=Ready,   3=Ignore)
    bit   u2YellowLampStatus :2;     // Yellow lamp status
    bit   u2RedLampStatus    :2;     // Red lamp status
    uint8 u8DSA;                     // Default Source Address
}
RVCDGN_zDGN_130762_Header;

// DMRV Diagnostic Trouble Code (DTC)
typedef struct
{
    uint32 u32SPN;                   // Service Point Number
    uint8  u8FMI;                    // Failure Mode Identifier.
    uint8  u8OccurCount;             // Occurence count (0-126)
    uint8  u8DSAExtension;           // DSA extension
}
RVCDGN_zDGN_130762_DTC;

#define RVCDGN_DGN_130762_HEADER_SIZE             2
#define RVCDGN_DGN_130762_DTC_CAPACITY            8
#define RVCDGN_DGN_130762_DTC_SIZE                5
#define RVCDGN_DGN_130762_FOOTER_SIZE             1
#define RVCDGN_DGN_130762_SIZE_OF_BUFFER(count)  (RVCDGN_DGN_130762_HEADER_SIZE + ((count)*RVCDGN_DGN_130762_DTC_CAPACITY))
#define RVCDGN_DGN_130762_SIZE                    RVCDGN_DGN_130762_SIZE_OF_BUFFER(RVCDGN_DGN_130762_DTC_CAPACITY)

extern void   RVCDGN_DGN_130762_StuffInit(     RVCDGN_zDGN_130762_Session *pzSession, uint8* pu8Data, uint16 u16Size );
extern void   RVCDGN_DGN_130762_StuffHeader(   RVCDGN_zDGN_130762_Session *pzSession, RVCDGN_zDGN_130762_Header *pzHeader);
extern bool   RVCDGN_DGN_130762_StuffDTC(      RVCDGN_zDGN_130762_Session *pzSession, RVCDGN_zDGN_130762_DTC    *pzDTC );
extern void   RVCDGN_DGN_130762_StuffFooter(   RVCDGN_zDGN_130762_Session *pzSession );
extern uint16 RVCDGN_DGN_130762_StuffSize(     RVCDGN_zDGN_130762_Session *pzSession );
extern void   RVCDGN_DGN_130762_ExtractInit(   RVCDGN_zDGN_130762_Session *pzSession, uint8* pu8Data, uint16 u16Capacity );
extern void   RVCDGN_DGN_130762_ExtractHeader( RVCDGN_zDGN_130762_Session *pzSession, RVCDGN_zDGN_130762_Header *pzHeader );
extern bool   RVCDGN_DGN_130762_ExtractDTC(    RVCDGN_zDGN_130762_Session *pzSession, RVCDGN_zDGN_130762_DTC    *pzDTC );

// Backward compatibility
#define RVCDGN_DGN_130762_GetSize RVCDGN_DGN_130762_StuffSize

//------------------------------------------------------------------------------
// DGN Number   : 130972 (0x1FF9C)
// Description  : Ambient Temperature
// Data Length  : 8
// Tx Rate      : 5000ms / On Request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
	uint8   u8Instance;
	uint16  u16AmbientTemp;
	uint8   u8Reserved1;
	uint16  u16Reserved2;
	uint16  u16Reserved3;
}
RVCDGN_zDGN_130972;

#define     RVCDGN_DGN_130972_SIZE   8
extern void RVCDGN_DGN_130972_Extract( RVCDGN_zDGN_130972* pzDgn, uint8* pu8Data);
extern void RVCDGN_DGN_130972_Stuff( uint8* pu8Data, RVCDGN_zDGN_130972* pzDgn );

//------------------------------------------------------------------------------
// DGN Number   : 131070 / 131071 (0x1FFFF)
// Description  : Date Time Command / Status
// Data Length  : 8
// Tx Rate      : 1000ms / On Request
// Priority     : 6
//------------------------------------------------------------------------------
typedef struct
{
	uint8  u8Year;       // Offset: 2000 AD (range = 2000 to 2250)
	uint8  u8Month;      // 1 to 12
	uint8  u8Day;        // 1 to 31
	uint8  u8DayOfWeek;  // 1=Sunday, 2�Monday, ... 7-Saturday
	uint8  u8Hour;       // Hours 0-23 (Local Time)
	uint8  u8Minute;     // 0 to 59
	uint8  u8Second;     // 0 to 59
	uint8  u8TimeZone;   // 0 - Greenwich Mean Time
	                     // 4 - Eastern Daylight Time
                         // 5 - Eastern Standard Time
                         // 7 - Pacific Daylight Time
                         // 8 - Pacific Standard Time
                         // 0 - Western European Time
                         // 22 - Central European Summer Time
}
RVCDGN_zDGN_131070;

#define     RVCDGN_DGN_131070_SIZE   8
extern void RVCDGN_DGN_131070_Extract( RVCDGN_zDGN_131070* pzDgn, uint8* pu8Data );
extern void RVCDGN_DGN_131070_Stuff( uint8* pu8Data, RVCDGN_zDGN_131070* pzDgn );

//------------------------------------------------------------------------------
typedef RVCDGN_zDGN_131070 RVCDGN_zDGN_131071;
#define RVCDGN_DGN_131071_SIZE    RVCDGN_DGN_131070_SIZE
#define RVCDGN_DGN_131071_Extract RVCDGN_DGN_131070_Extract
#define RVCDGN_DGN_131071_Stuff   RVCDGN_DGN_131070_Stuff

//------------------------------------------------------------------------------
#endif
