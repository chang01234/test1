//------------------------------------------------------------------------------
// Module:      PropDGN.c
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

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <string.h>
#include "PropDGN.h"
#include "RVCDGN.h"


//------------------------------------------------------------------------------
// Local defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local prototypes
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local functions prototypes
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_65534_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 65534 frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_65534_Extract
(
	PROPDGN_zDGN_65534* pzDgn65534,     // out: Extracted parameters
    uint8*              pu8Data         // in : DGN frame buffer
)
{
    //           dest                          src
    DGN_TO_WORD( pzDgn65534->i16RpmError,      pu8Data[0]  );
    DGN_TO_WORD( pzDgn65534->i16RpmPTerm,      pu8Data[2]  );
    DGN_TO_WORD( pzDgn65534->i16RpmITerm,      pu8Data[4]  );
    DGN_TO_WORD( pzDgn65534->i16RpmDTerm,      pu8Data[6]  );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_65534_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff DGN 65534 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_65534_Stuff
(
    uint8*              pu8Data,    // out: DGN frame buffer
	PROPDGN_zDGN_65534* pzDgn65534  // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, PROPDGN_DGN_65534_SIZE );

	// Fill
    //           dest         source
    WORD_TO_DGN( pu8Data[0],  pzDgn65534->i16RpmError  );
    WORD_TO_DGN( pu8Data[2],  pzDgn65534->i16RpmPTerm  );
    WORD_TO_DGN( pu8Data[4],  pzDgn65534->i16RpmITerm  );
    WORD_TO_DGN( pu8Data[6],  pzDgn65534->i16RpmDTerm  );
}


//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_65535_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 65535 frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_65535_Extract
(
	PROPDGN_zDGN_65535* pzDgn65535,     // out: Extracted parameters
    uint8*              pu8Data         // in : DGN frame buffer
)
{
    //           dest                                src
    DGN_TO_WORD( pzDgn65535->u16RawRpmSetPoint,      pu8Data[0] );
    DGN_TO_WORD( pzDgn65535->u16SlewedRpmSetPoint,   pu8Data[2] );
    DGN_TO_WORD( pzDgn65535->i16RpmAffTerm,          pu8Data[4] );
    DGN_TO_WORD( pzDgn65535->i16RpmVffTerm,          pu8Data[6] );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_65535_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff DGN 65535 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_65535_Stuff
(
    uint8*              pu8Data,    // out: DGN frame buffer
	PROPDGN_zDGN_65535* pzDgn65535  // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, PROPDGN_DGN_65535_SIZE );

	// Fill
    //           dest         source
    WORD_TO_DGN( pu8Data[0],  pzDgn65535->u16RawRpmSetPoint     );
    WORD_TO_DGN( pu8Data[2],  pzDgn65535->u16SlewedRpmSetPoint  );
    WORD_TO_DGN( pu8Data[4],  pzDgn65535->i16RpmAffTerm         );
    WORD_TO_DGN( pu8Data[6],  pzDgn65535->i16RpmVffTerm         );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130559_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130559 (or 130558).
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130559_Extract
(
	PROPDGN_zDGN_130559* pzDest,  // out: Destination structure.
    uint8*               pu8Src   // in:  Source buffer.
)
{    
    //            dest                          src        start size
    DGN_TO_BITS(  pzDest->u8HeaterInstance,     pu8Src[0],     0,   8 );
    DGN_TO_BITS(  pzDest->u4EnergySource,       pu8Src[1],     0,   4 );
    DGN_TO_BITS(  pzDest->u2AirHeaterCmd,       pu8Src[1],     4,   2 );
    DGN_TO_BITS(  pzDest->u2WaterHeaterCmd,     pu8Src[1],     6,   2 );
    DGN_TO_BITS(  pzDest->u4AirHeaterMode,      pu8Src[2],     0,   4 );
    DGN_TO_BITS(  pzDest->u4WaterHeaterMode,    pu8Src[2],     4,   4 );
    DGN_TO_WORD(  pzDest->i16TargetRoomTemp,    pu8Src[3]             );
    DGN_TO_BITS(  pzDest->u4SilentModeMaxFan,   pu8Src[5],     0,   4 );
    DGN_TO_BITS(  pzDest->u4VentModeFanMin,     pu8Src[5],     4,   4 );
    DGN_TO_BITS(  pzDest->u8UnderVoltThreshold, pu8Src[6],     0,   8 );
    DGN_TO_BITS(  pzDest->u2SystemUnits,        pu8Src[7],     0,   2 );
    DGN_TO_BITS(  pzDest->u6Reserved1,          pu8Src[7],     2,   6 );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130559_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters from DGN 130559 (or 130558).
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130559_Stuff
(
    uint8*               pu8Dest,   // out: Destination buffer.
	PROPDGN_zDGN_130559* pzSrc      // in:  Source structure.
)
{
    // Clear
    (void)memset( pu8Dest, 0, PROPDGN_DGN_130559_SIZE );

	// Fill
    //           dest        src                           start size
    BITS_TO_DGN( pu8Dest[0], pzSrc->u8HeaterInstance,          0,   8 );
    BITS_TO_DGN( pu8Dest[1], pzSrc->u4EnergySource,            0,   4 );
    BITS_TO_DGN( pu8Dest[1], pzSrc->u2AirHeaterCmd,            4,   2 );
    BITS_TO_DGN( pu8Dest[1], pzSrc->u2WaterHeaterCmd,          6,   2 );
    BITS_TO_DGN( pu8Dest[2], pzSrc->u4AirHeaterMode,           0,   4 );
    BITS_TO_DGN( pu8Dest[2], pzSrc->u4WaterHeaterMode,         4,   4 );
    WORD_TO_DGN( pu8Dest[3], pzSrc->i16TargetRoomTemp                 );
    BITS_TO_DGN( pu8Dest[5], pzSrc->u4SilentModeMaxFan,        0,   4 );
    BITS_TO_DGN( pu8Dest[5], pzSrc->u4VentModeFanMin,          4,   4 );
    BITS_TO_DGN( pu8Dest[6], pzSrc->u8UnderVoltThreshold,      0,   8 );
    BITS_TO_DGN( pu8Dest[7], pzSrc->u2SystemUnits,             0,   2 );
    BITS_TO_DGN( pu8Dest[7], 0x3F,                             2,   6 );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130557_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130557.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130557_Extract
(
	PROPDGN_zDGN_130557* pzDest,   // out: Destination structure
    uint8*               pu8Src    // in:  Source buffer
)
{    
    //           dest                             src       start size
    DGN_TO_BITS( pzDest->u8HeaterInstance,        pu8Src[0],    0,   8 );
    DGN_TO_BITS( pzDest->u8Reserved1,             pu8Src[1],    0,   8 );
    DGN_TO_BITS( pzDest->u8Reserved2,             pu8Src[2],    0,   8 );
    DGN_TO_WORD( pzDest->i16RoomTemp,             pu8Src[3]            );
    DGN_TO_BITS( pzDest->u3WaterTemp,             pu8Src[5],    0,   3 );
    DGN_TO_BITS( pzDest->u3Reserved5,             pu8Src[5],    3,   5 );
    DGN_TO_BITS( pzDest->u4GasHeaterAir,          pu8Src[6],    0,   4 );
    DGN_TO_BITS( pzDest->u4GasHeaterWater,        pu8Src[6],    4,   4 );
    DGN_TO_BITS( pzDest->u2ACPresent,             pu8Src[7],    0,   2 );
    DGN_TO_BITS( pzDest->u3ACHeaterAir,           pu8Src[7],    2,   3 );
    DGN_TO_BITS( pzDest->u3ACHeaterWater,         pu8Src[7],    5,   3 );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130557_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters from DGN 130557.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130557_Stuff
(
    uint8*               pu8Dest, // out:  Destination buffer
	PROPDGN_zDGN_130557* pzSrc    // in: Source structure
)
{
    // Clear
    (void)memset( pu8Dest, 0, PROPDGN_DGN_130557_SIZE );

	// Fill
    //           src         dest                          start size
    BITS_TO_DGN( pu8Dest[0], pzSrc->u8HeaterInstance,          0,   8 );
    BITS_TO_DGN( pu8Dest[1], 0xFF,                             0,   8 );
    BITS_TO_DGN( pu8Dest[2], 0xFF,                             0,   8 );
	WORD_TO_DGN( pu8Dest[3], pzSrc->i16RoomTemp                       );
    BITS_TO_DGN( pu8Dest[5], pzSrc->u3WaterTemp,               0,   3 );
    BITS_TO_DGN( pu8Dest[5], 0x1F,                             3,   5 );
    BITS_TO_DGN( pu8Dest[6], pzSrc->u4GasHeaterAir,            0,   4 );
    BITS_TO_DGN( pu8Dest[6], pzSrc->u4GasHeaterWater,          4,   4 );
    BITS_TO_DGN( pu8Dest[7], pzSrc->u2ACPresent,               0,   2 );
    BITS_TO_DGN( pu8Dest[7], pzSrc->u3ACHeaterAir,             2,   3 );
    BITS_TO_DGN( pu8Dest[7], pzSrc->u3ACHeaterWater,           5,   3 );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130556_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130556.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130556_Extract
(
	PROPDGN_zDGN_130556* pzDest,   // out: Destination structure
    uint8*               pu8Src    // in:  Source buffer
)
{
    //           dest                           src       start size
    DGN_TO_BITS( pzDest->u8HMIInstance,         pu8Src[0],    0,   8 );
    DGN_TO_WORD( pzDest->i16RoomTemperature,    pu8Src[1]);
    DGN_TO_BITS( pzDest->u2HeaterCommunication, pu8Src[3],    0,   2 );
    DGN_TO_BITS( pzDest->u2InputVoltage,        pu8Src[3],    2,   2 );
    DGN_TO_BITS( pzDest->u2HMIInstanceStatus,   pu8Src[3],    4,   2 );
    DGN_TO_BITS( pzDest->u2InternalCircuitry,   pu8Src[3],    6,   2 );
    DGN_TO_BITS( pzDest->u2FavoriteButton,      pu8Src[4],    0,   2 );
    DGN_TO_BITS( pzDest->u2MenuButton,          pu8Src[4],    2,   2 );
    DGN_TO_BITS( pzDest->u2HomeButton,          pu8Src[4],    4,   2 );
    DGN_TO_BITS( pzDest->u2Reserved1,           pu8Src[4],    6,   2 );
    DGN_TO_BITS( pzDest->u8Reserved2,           pu8Src[5],    0,   8 );
    DGN_TO_WORD( pzDest->u16Reserved3,          pu8Src[6]);
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130556_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130556
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130556_Stuff
(
    uint8*               pu8Dest,   // out:  Destination buffer
	PROPDGN_zDGN_130556* pzSrc      // in:   Source structure
)
{
    // Clear
    (void)memset( pu8Dest, 0, PROPDGN_DGN_130556_SIZE );

	// Fill
    //           dest         source                       start size
    BITS_TO_DGN( pu8Dest[0],  pzSrc->u8HMIInstance,            0,   8 );
	WORD_TO_DGN( pu8Dest[1],  pzSrc->i16RoomTemperature               );
    BITS_TO_DGN( pu8Dest[3],  pzSrc->u2HeaterCommunication,    0,   2 );
    BITS_TO_DGN( pu8Dest[3],  pzSrc->u2InputVoltage,           2,   2 );
    BITS_TO_DGN( pu8Dest[3],  pzSrc->u2HMIInstanceStatus,      4,   2 );
    BITS_TO_DGN( pu8Dest[3],  pzSrc->u2InternalCircuitry,      6,   2 );
    BITS_TO_DGN( pu8Dest[4],  pzSrc->u2FavoriteButton,         0,   2 );
    BITS_TO_DGN( pu8Dest[4],  pzSrc->u2MenuButton,             2,   2 );
    BITS_TO_DGN( pu8Dest[4],  pzSrc->u2HomeButton,             4,   2 );
    BITS_TO_DGN( pu8Dest[4],  0x03,                            6,   2 );
    BITS_TO_DGN( pu8Dest[5],  0xFF,                            0,   8 );
    WORD_TO_DGN( pu8Dest[6],  0xFFFF                                  );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130555_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130555 (or 130554).
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130555_Extract
(
	PROPDGN_zDGN_130555* pzDest,   // out: Destination structure
    uint8*               pu8Src    // in:  Source buffer
)
{
    //           dest                           src       start size
    DGN_TO_BITS( pzDest->u8HeaterInstance,      pu8Src[0],    0,   8 );
    DGN_TO_BITS( pzDest->u2AirHtrTimerOffStat,  pu8Src[1],    0,   2 );
    DGN_TO_BITS( pzDest->u6AirHtrTimerOffHour,  pu8Src[1],    2,   6 );
    DGN_TO_BITS( pzDest->u8AirHtrTimerOffMin,   pu8Src[2],    0,   8 );
    DGN_TO_BITS( pzDest->u2AirHtrTimerOnStat,   pu8Src[3],    0,   2 );
    DGN_TO_BITS( pzDest->u6AirHtrTimerOnHour,   pu8Src[3],    2,   6 );
    DGN_TO_BITS( pzDest->u8AirHtrTimerOnMin,    pu8Src[4],    0,   8 );
    DGN_TO_BITS( pzDest->u2WtrHtrTimerOnStat,   pu8Src[5],    0,   2 );
    DGN_TO_BITS( pzDest->u6WtrHtrTimerOnHour,   pu8Src[5],    2,   6 );
    DGN_TO_BITS( pzDest->u8WtrHtrTimerOnMin,    pu8Src[6],    0,   8 );
    DGN_TO_BITS( pzDest->u8WtrHtrKeepOnTime,    pu8Src[7],    0,   8 );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130555_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130555 (or 130554)
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130555_Stuff
(
    uint8*               pu8Dest,   // out:  Destination buffer
	PROPDGN_zDGN_130555* pzSrc      // in:   Source structure
)
{
    // Clear
    (void)memset( pu8Dest, 0, PROPDGN_DGN_130555_SIZE );

	// Fill
    //           dest         source                      start size
    BITS_TO_DGN( pu8Dest[0],  pzSrc->u8HeaterInstance,        0,   8 );
    BITS_TO_DGN( pu8Dest[1],  pzSrc->u2AirHtrTimerOffStat,    0,   2 );
    BITS_TO_DGN( pu8Dest[1],  pzSrc->u6AirHtrTimerOffHour,    2,   6 );
    BITS_TO_DGN( pu8Dest[2],  pzSrc->u8AirHtrTimerOffMin,     0,   8 );
    BITS_TO_DGN( pu8Dest[3],  pzSrc->u2AirHtrTimerOnStat,     0,   2 );
    BITS_TO_DGN( pu8Dest[3],  pzSrc->u6AirHtrTimerOnHour,     2,   6 );
    BITS_TO_DGN( pu8Dest[4],  pzSrc->u8AirHtrTimerOnMin,      0,   8 );
    BITS_TO_DGN( pu8Dest[5],  pzSrc->u2WtrHtrTimerOnStat,     0,   2 );
    BITS_TO_DGN( pu8Dest[5],  pzSrc->u6WtrHtrTimerOnHour,     2,   6 );
    BITS_TO_DGN( pu8Dest[6],  pzSrc->u8WtrHtrTimerOnMin,      0,   8 );
    BITS_TO_DGN( pu8Dest[7],  pzSrc->u8WtrHtrKeepOnTime,      0,   8 );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130553_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130553
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130553_Extract
(
	PROPDGN_zDGN_130553* pzDest,   // out: Destination structure
    uint8*               pu8Src    // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS( pzDest->u8HeaterInstance,           pu8Src[0],    0,   8 );
    DGN_TO_BITS( pzDest->u2WarningFault,             pu8Src[1],    0,   2 );
    DGN_TO_BITS( pzDest->u2CriticalFault,            pu8Src[1],    2,   2 );
    DGN_TO_BITS( pzDest->u2Reserved1,                pu8Src[1],    4,   2 );
    DGN_TO_BITS( pzDest->u2Reserved2,                pu8Src[1],    6,   2 );
    DGN_TO_BITS( pzDest->u8Reserved3,                pu8Src[2],    0,   8 );
    pzDest->u10FaultCode1  = (pu8Src[3]>>0);
    pzDest->u10FaultCode1 |= (pu8Src[4]<<8)&0x3FF;
    pzDest->u10FaultCode2  = (pu8Src[4]>>2);
    pzDest->u10FaultCode2 |= (pu8Src[5]<<6)&0x3FF;
    pzDest->u10FaultCode3  = (pu8Src[5]>>4);
    pzDest->u10FaultCode3 |= (pu8Src[6]<<4)&0x3FF;
    pzDest->u10FaultCode4  = (pu8Src[6]>>6);
    pzDest->u10FaultCode4 |= (pu8Src[7]<<2);
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130553_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130553
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130553_Stuff
(
    uint8*               pu8Dest,   // out:  Destination buffer
	PROPDGN_zDGN_130553* pzSrc      // in:   Source structure
)
{
    // Clear
    (void)memset( pu8Dest, 0, PROPDGN_DGN_130553_SIZE );

	// Fill
    //           dest       source                          start size
    BITS_TO_DGN( pu8Dest[0], pzSrc->u8HeaterInstance,           0,   8 );
    BITS_TO_DGN( pu8Dest[1], pzSrc->u2WarningFault,             0,   2 );
    BITS_TO_DGN( pu8Dest[1], pzSrc->u2CriticalFault,            2,   2 );
    BITS_TO_DGN( pu8Dest[1], 0x03,                              4,   2 );
    BITS_TO_DGN( pu8Dest[1], 0x03,                              6,   2 );
    BITS_TO_DGN( pu8Dest[2], 0xFF,                              0,   8 );
    pu8Dest[3]  = (pzSrc->u10FaultCode1);
    pu8Dest[4]  = (pzSrc->u10FaultCode1>>8) & 0x03;
    pu8Dest[4] |= (pzSrc->u10FaultCode2<<2);
    pu8Dest[5]  = (pzSrc->u10FaultCode2>>6) & 0x0F;
    pu8Dest[5] |= (pzSrc->u10FaultCode3<<4) & 0xF0;
    pu8Dest[6]  = (pzSrc->u10FaultCode3>>4) & 0x3F;
    pu8Dest[6] |= (pzSrc->u10FaultCode4<<6) & 0xC0;
    pu8Dest[7]  = (pzSrc->u10FaultCode4>>2);
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130552_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130552
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130552_Extract
(
	PROPDGN_zDGN_130552* pzDest,   // out: Destination structure
    uint8*               pu8Src    // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS( pzDest->u8HeaterInstance,           pu8Src[0],    0,   8 );
    DGN_TO_BITS( pzDest->u8ComfortSwMajor,           pu8Src[1],    0,   8 );
    DGN_TO_BITS( pzDest->u8ComfortSwMinor,           pu8Src[2],    0,   8 );
    DGN_TO_BITS( pzDest->u8BurnerSwMajor,            pu8Src[3],    0,   8 );
    DGN_TO_BITS( pzDest->u8BurnerSwMinor,            pu8Src[4],    0,   8 );
    DGN_TO_BITS( pzDest->u8PcbaVersion,              pu8Src[5],    0,   8 );
    DGN_TO_BITS( pzDest->u8ProtocolMajor,            pu8Src[6],    0,   8 );
    DGN_TO_BITS( pzDest->u8ProtocolMinor,            pu8Src[7],    0,   8 );
}

//-----------------------------------------------------------------------------
// Function:    PROPDGN_DGN_130552_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130552
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PROPDGN_DGN_130552_Stuff
(
    uint8*               pu8Dest,   // out:  Destination buffer
	PROPDGN_zDGN_130552* pzSrc      // in:   Source structure
)
{
    // Clear
    (void)memset( pu8Dest, 0, PROPDGN_DGN_130552_SIZE );

	// Fill
    //           dest       source                          start size
    BITS_TO_DGN( pu8Dest[0], pzSrc->u8HeaterInstance,           0,   8 );
    BITS_TO_DGN( pu8Dest[1], pzSrc->u8ComfortSwMajor,           0,   8 );
    BITS_TO_DGN( pu8Dest[2], pzSrc->u8ComfortSwMinor,           0,   8 );
    BITS_TO_DGN( pu8Dest[3], pzSrc->u8BurnerSwMajor,            0,   8 );
    BITS_TO_DGN( pu8Dest[4], pzSrc->u8BurnerSwMinor,            0,   8 );
    BITS_TO_DGN( pu8Dest[5], pzSrc->u8PcbaVersion,              0,   8 );
    BITS_TO_DGN( pu8Dest[6], pzSrc->u8ProtocolMajor,            0,   8 );
    BITS_TO_DGN( pu8Dest[7], pzSrc->u8ProtocolMinor,            0,   8 );
}


