//------------------------------------------------------------------------------
// Module:      StdDefs.h
//------------------------------------------------------------------------------
// Description: Portable macro definitions specific to the software platform
//              and the target MCU.
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
#ifndef STDDEFS_H
#define STDDEFS_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Public Types (DOMETIC / AUTOSAR subset)
//------------------------------------------------------------------------------
#ifndef __cplusplus
  #ifndef bool
typedef unsigned char	bool;
  #endif
#endif
typedef unsigned char	boolean;
typedef int8_t     int8;
typedef int8_t     sint8;
typedef uint8_t   uint8;
typedef int16_t    int16;
typedef int16_t    sint16;
typedef uint16_t  uint16;
typedef int32_t     int32;
typedef int32_t     sint32;
typedef uint32_t   uint32;
typedef float           float32;
typedef double          float64;
typedef unsigned char	bit;

//------------------------------------------------------------------------------
// Public Definitions (AUTOSAR subset)
//------------------------------------------------------------------------------
#define CPU_TYPE_8        8
#define CPU_TYPE_16       16
#define CPU_TYPE_32       32

#define MSB_FIRST         0
#define LSB_FIRST         1

#define HIGH_BYTE_FIRST   0
#define LOW_BYTE_FIRST    1

#define HIGH_WORD_FIRST   0
#define LOW_WORD_FIRST    1

#define CPU_TYPE          CPU_TYPE_32
#define CPU_BIT_ORDER     LSB_FIRST
#define CPU_BYTE_ORDER    LOW_BYTE_FIRST

#define INLINE            inline
#define LOCAL_INLINE      static inline

//------------------------------------------------------------------------------
// Public Definitions
//------------------------------------------------------------------------------
#ifndef NULL_PTR
    #define NULL_PTR   ((void*)0)
#endif

#ifndef FALSE
    #define FALSE           0
#endif

#ifndef TRUE
    #define TRUE            1
#endif

#ifndef STD_OFF
    #define STD_OFF         0
#endif

#ifndef STD_ON
    #define STD_ON          1
#endif

#define E_OK 				0
#define E_NOT_OK 			1

// Max value definitions
#ifndef UINT8_MAX
    #define UINT8_MAX      255
#endif
#ifndef UINT16_MAX
    #define UINT16_MAX     65535
#endif
#ifndef UINT32_MAX
    #define UINT32_MAX     4294967295UL
#endif
#ifndef INT8_MAX
    #define INT8_MAX       127
#endif
#ifndef INT8_MIN
    #define INT8_MIN       -128
#endif
#ifndef INT16_MAX
    #define INT16_MAX      32767
#endif
#ifndef INT16_MIN
    #define INT16_MIN      -32768
#endif
#ifndef INT32_MAX
    #define INT32_MAX      2147483647L
#endif
#ifndef INT32_MIN
    #define INT32_MIN      -2147483648L
#endif

//------------------------------------------------------------------------------
// Public Macros
//------------------------------------------------------------------------------
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

// Macros used to get sub-sets of larger data type
#define U16_UPPER_U8(x)     ((uint8)(x>>8))
#define U16_LOWER_U8(x)     ((uint8)x)
#define U32_UPPER_U16(x)    ((uint16)(x>>16))
#define U32_LOWER_U16(x)    ((uint16)x)

// Macro used to determine the number of elements in a struct array
#define DIM(x)              (sizeof(x)/sizeof(x[0]))

// Macro for bitwise operation
// Returns 1 if bit position "pos" in var" is 1, 0 otherwise
#define CHECK_BIT(var,pos)  (((var)>>(pos)) & (1))


//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#endif  // STDDEFS_H
