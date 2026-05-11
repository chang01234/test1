//------------------------------------------------------------------------------
// Module:      std_dometic.h
//------------------------------------------------------------------------------
// Description: Portable types and macro definitions
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
#ifndef STD_DOMETIC_H
#define STD_DOMETIC_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stddef.h>
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
typedef signed char     int8;
typedef signed char     sint8;
typedef unsigned char   uint8;
typedef signed short    int16;
typedef signed short    sint16;
typedef unsigned short  uint16;
typedef signed long     int32;
typedef signed long     sint32;
typedef unsigned long   uint32;
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
// Legacy stuff... do not use for new code
//------------------------------------------------------------------------------
#define STDDEFS_CAPMAX(x,max)          if(x>max)x=max

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------
LOCAL_INLINE uint16_t BYTES_TO_U16(uint8_t MSB, uint8_t LSB)                           
{ 
    return (((uint16_t)(MSB)<<8) + (LSB)); 
}
LOCAL_INLINE int16_t BYTES_TO_S16(uint8_t MSB, uint8_t LSB)                           
{ 
    return (int16_t)(((uint16_t)(MSB)<<8) + (LSB)); 
}
LOCAL_INLINE uint32_t BYTES_TO_U32(uint8_t MSB, uint8_t B2, uint8_t B3, uint8_t LSB)   
{ 
    return (((uint32_t)MSB<<24) +
            ((uint32_t)B2 <<16) +
            ((uint32_t)B3 << 8) +
            ((uint32_t)LSB<< 0) );
}
LOCAL_INLINE int32_t BYTES_TO_S32(uint8_t MSB, uint8_t B2, uint8_t B3, uint8_t LSB)   
{ 
    return (int32_t)(((uint32_t)MSB<<24) +
                     ((uint32_t)B2 <<16) +
                     ((uint32_t)B3 << 8) +
                     ((uint32_t)LSB<< 0) );
}

#endif  // STD_DOMETIC_H
