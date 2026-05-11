//------------------------------------------------------------------------------
// Module:      nmea_types.h
//------------------------------------------------------------------------------
// Description: Types and definition related to NMEA
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
#ifndef NMEA_TYPES_H
#define NMEA_TYPES_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Public Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Defines
//------------------------------------------------------------------------------
// No data definitions
#define NMEA_INT8_NO_DATA                         0x7F
#define NMEA_INT16_NO_DATA                        0x7FFF
#define NMEA_INT32_NO_DATA                        0x7FFFFFFF
#define NMEA_UINT2_NO_DATA                        0x3
#define NMEA_UINT3_NO_DATA                        0x7
#define NMEA_UINT4_NO_DATA                        0xF
#define NMEA_UINT5_NO_DATA                        0x1F
#define NMEA_UINT6_NO_DATA                        0x3F
#define NMEA_UINT7_NO_DATA                        0x7F
#define NMEA_UINT8_NO_DATA                        0xFF
#define NMEA_UINT16_NO_DATA                       0xFFFF
#define NMEA_UINT32_NO_DATA                       0xFFFFFFFF

// Error data definitions
#define NMEA_INT8_ERROR_DATA                      0x7E
#define NMEA_INT16_ERROR_DATA                     0x7FFE
#define NMEA_INT32_ERROR_DATA                     0x7FFFFFFE
#define NMEA_UINT2_ERROR_DATA                     0x2
#define NMEA_UINT3_ERROR_DATA                     0x6
#define NMEA_UINT4_ERROR_DATA                     0xE
#define NMEA_UINT5_ERROR_DATA                     0x1E
#define NMEA_UINT6_ERROR_DATA                     0x3E
#define NMEA_UINT7_ERROR_DATA                     0x7E
#define NMEA_UINT8_ERROR_DATA                     0xFE
#define NMEA_UINT16_ERROR_DATA                    0xFFFE
#define NMEA_UINT32_ERROR_DATA                    0xFFFFFFFE

// Data macros
#define NMEA_IS_DATA_VALID_I16(x)                 ((x)<0x7FFE)?1:0
#define NMEA_IS_DATA_VALID_U16(x)                 ((x)<0xFFFE)?1:0
#define NMEA_IS_DATA_VALID_I32(x)                 ((x)<0x7FFFFFFEL)?1:0
#define NMEA_IS_DATA_VALID_U32(x)                 ((x)<0xFFFFFFFEUL)?1:0

//------------------------------------------------------------------------------
// Public Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#endif  // NMEA_TYPES_H
