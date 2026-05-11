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
#include <stddef.h>

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
#ifndef NULL
    #define NULL ((void *)0)
#endif

#ifndef NULL_PTR
    #define NULL_PTR   ((void*)0)
#endif

#ifndef FALSE
    #define FALSE           0
#endif

#ifndef TRUE
    #define TRUE            1
#endif

#ifndef NO
    #define NO 0
#endif

#ifndef YES
    #define YES 1
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

// No data definitions
#define INT8_NO_DATA   0x7F
#define INT16_NO_DATA  0x7FFF
#define INT32_NO_DATA  0x7FFFFFFF
#define UINT2_NO_DATA  0x3
#define UINT3_NO_DATA  0x7
#define UINT4_NO_DATA  0xF
#define UINT5_NO_DATA  0x1F
#define UINT6_NO_DATA  0x3F
#define UINT7_NO_DATA  0x7F
#define UINT8_NO_DATA  0xFF
#define UINT16_NO_DATA 0xFFFF
#define UINT32_NO_DATA 0xFFFFFFFF

// Error data definitions
#define INT8_ERROR_DATA   0x7E
#define INT16_ERROR_DATA  0x7FFE
#define INT32_ERROR_DATA  0x7FFFFFFE
#define UINT2_ERROR_DATA  0x2
#define UINT3_ERROR_DATA  0x6
#define UINT4_ERROR_DATA  0xE
#define UINT5_ERROR_DATA  0x1E
#define UINT6_ERROR_DATA  0x3E
#define UINT7_ERROR_DATA  0x7E
#define UINT8_ERROR_DATA  0xFE
#define UINT16_ERROR_DATA 0xFFFE
#define UINT32_ERROR_DATA 0xFFFFFFFE

// Bit field definitions
#define _b00000000 0x00
#define _b00000001 0x01
#define _b00000010 0x02
#define _b00000011 0x03
#define _b00000100 0x04
#define _b00000101 0x05
#define _b00000110 0x06
#define _b00000111 0x07
#define _b00001000 0x08
#define _b00001001 0x09
#define _b00001010 0x0A
#define _b00001011 0x0B
#define _b00001100 0x0C
#define _b00001101 0x0D
#define _b00001110 0x0E
#define _b00001111 0x0F
#define _b00010000 0x10
#define _b00010001 0x11
#define _b00010010 0x12
#define _b00010011 0x13
#define _b00010100 0x14
#define _b00010101 0x15
#define _b00010110 0x16
#define _b00010111 0x17
#define _b00011000 0x18
#define _b00011001 0x19
#define _b00011010 0x1A
#define _b00011011 0x1B
#define _b00011100 0x1C
#define _b00011101 0x1D
#define _b00011110 0x1E
#define _b00011111 0x1F
#define _b00100000 0x20
#define _b00100001 0x21
#define _b00100010 0x22
#define _b00100011 0x23
#define _b00100100 0x24
#define _b00100101 0x25
#define _b00100110 0x26
#define _b00100111 0x27
#define _b00101000 0x28
#define _b00101001 0x29
#define _b00101010 0x2A
#define _b00101011 0x2B
#define _b00101100 0x2C
#define _b00101101 0x2D
#define _b00101110 0x2E
#define _b00101111 0x2F
#define _b00110000 0x30
#define _b00110001 0x31
#define _b00110010 0x32
#define _b00110011 0x33
#define _b00110100 0x34
#define _b00110101 0x35
#define _b00110110 0x36
#define _b00110111 0x37
#define _b00111000 0x38
#define _b00111001 0x39
#define _b00111010 0x3A
#define _b00111011 0x3B
#define _b00111100 0x3C
#define _b00111101 0x3D
#define _b00111110 0x3E
#define _b00111111 0x3F
#define _b01000000 0x40
#define _b01000001 0x41
#define _b01000010 0x42
#define _b01000011 0x43
#define _b01000100 0x44
#define _b01000101 0x45
#define _b01000110 0x46
#define _b01000111 0x47
#define _b01001000 0x48
#define _b01001001 0x49
#define _b01001010 0x4A
#define _b01001011 0x4B
#define _b01001100 0x4C
#define _b01001101 0x4D
#define _b01001110 0x4E
#define _b01001111 0x4F
#define _b01010000 0x50
#define _b01010001 0x51
#define _b01010010 0x52
#define _b01010011 0x53
#define _b01010100 0x54
#define _b01010101 0x55
#define _b01010110 0x56
#define _b01010111 0x57
#define _b01011000 0x58
#define _b01011001 0x59
#define _b01011010 0x5A
#define _b01011011 0x5B
#define _b01011100 0x5C
#define _b01011101 0x5D
#define _b01011110 0x5E
#define _b01011111 0x5F
#define _b01100000 0x60
#define _b01100001 0x61
#define _b01100010 0x62
#define _b01100011 0x63
#define _b01100100 0x64
#define _b01100101 0x65
#define _b01100110 0x66
#define _b01100111 0x67
#define _b01101000 0x68
#define _b01101001 0x69
#define _b01101010 0x6A
#define _b01101011 0x6B
#define _b01101100 0x6C
#define _b01101101 0x6D
#define _b01101110 0x6E
#define _b01101111 0x6F
#define _b01110000 0x70
#define _b01110001 0x71
#define _b01110010 0x72
#define _b01110011 0x73
#define _b01110100 0x74
#define _b01110101 0x75
#define _b01110110 0x76
#define _b01110111 0x77
#define _b01111000 0x78
#define _b01111001 0x79
#define _b01111010 0x7A
#define _b01111011 0x7B
#define _b01111100 0x7C
#define _b01111101 0x7D
#define _b01111110 0x7E
#define _b01111111 0x7F
#define _b10000000 0x80
#define _b10000001 0x81
#define _b10000010 0x82
#define _b10000011 0x83
#define _b10000100 0x84
#define _b10000101 0x85
#define _b10000110 0x86
#define _b10000111 0x87
#define _b10001000 0x88
#define _b10001001 0x89
#define _b10001010 0x8A
#define _b10001011 0x8B
#define _b10001100 0x8C
#define _b10001101 0x8D
#define _b10001110 0x8E
#define _b10001111 0x8F
#define _b10010000 0x90
#define _b10010001 0x91
#define _b10010010 0x92
#define _b10010011 0x93
#define _b10010100 0x94
#define _b10010101 0x95
#define _b10010110 0x96
#define _b10010111 0x97
#define _b10011000 0x98
#define _b10011001 0x99
#define _b10011010 0x9A
#define _b10011011 0x9B
#define _b10011100 0x9C
#define _b10011101 0x9D
#define _b10011110 0x9E
#define _b10011111 0x9F
#define _b10100000 0xA0
#define _b10100001 0xA1
#define _b10100010 0xA2
#define _b10100011 0xA3
#define _b10100100 0xA4
#define _b10100101 0xA5
#define _b10100110 0xA6
#define _b10100111 0xA7
#define _b10101000 0xA8
#define _b10101001 0xA9
#define _b10101010 0xAA
#define _b10101011 0xAB
#define _b10101100 0xAC
#define _b10101101 0xAD
#define _b10101110 0xAE
#define _b10101111 0xAF
#define _b10110000 0xB0
#define _b10110001 0xB1
#define _b10110010 0xB2
#define _b10110011 0xB3
#define _b10110100 0xB4
#define _b10110101 0xB5
#define _b10110110 0xB6
#define _b10110111 0xB7
#define _b10111000 0xB8
#define _b10111001 0xB9
#define _b10111010 0xBA
#define _b10111011 0xBB
#define _b10111100 0xBC
#define _b10111101 0xBD
#define _b10111110 0xBE
#define _b10111111 0xBF
#define _b11000000 0xC0
#define _b11000001 0xC1
#define _b11000010 0xC2
#define _b11000011 0xC3
#define _b11000100 0xC4
#define _b11000101 0xC5
#define _b11000110 0xC6
#define _b11000111 0xC7
#define _b11001000 0xC8
#define _b11001001 0xC9
#define _b11001010 0xCA
#define _b11001011 0xCB
#define _b11001100 0xCC
#define _b11001101 0xCD
#define _b11001110 0xCE
#define _b11001111 0xCF
#define _b11010000 0xD0
#define _b11010001 0xD1
#define _b11010010 0xD2
#define _b11010011 0xD3
#define _b11010100 0xD4
#define _b11010101 0xD5
#define _b11010110 0xD6
#define _b11010111 0xD7
#define _b11011000 0xD8
#define _b11011001 0xD9
#define _b11011010 0xDA
#define _b11011011 0xDB
#define _b11011100 0xDC
#define _b11011101 0xDD
#define _b11011110 0xDE
#define _b11011111 0xDF
#define _b11100000 0xE0
#define _b11100001 0xE1
#define _b11100010 0xE2
#define _b11100011 0xE3
#define _b11100100 0xE4
#define _b11100101 0xE5
#define _b11100110 0xE6
#define _b11100111 0xE7
#define _b11101000 0xE8
#define _b11101001 0xE9
#define _b11101010 0xEA
#define _b11101011 0xEB
#define _b11101100 0xEC
#define _b11101101 0xED
#define _b11101110 0xEE
#define _b11101111 0xEF
#define _b11110000 0xF0
#define _b11110001 0xF1
#define _b11110010 0xF2
#define _b11110011 0xF3
#define _b11110100 0xF4
#define _b11110101 0xF5
#define _b11110110 0xF6
#define _b11110111 0xF7
#define _b11111000 0xF8
#define _b11111001 0xF9
#define _b11111010 0xFA
#define _b11111011 0xFB
#define _b11111100 0xFC
#define _b11111101 0xFD
#define _b11111110 0xFE
#define _b11111111 0xFF

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

// Extraction macros
// (BUFFLE means Buffer in little endian format)
// (BUFFBE means Buffer in big endian format)
#define BUFFER_TO_BITS(dest, src, start, size) \
    dest = (uint8)(((uint8)src >> start) & ((uint8)0xFF >> (8 - size)))
#define BUFFLE_TO_WORD(dest, src) \
    dest = (((uint16)(*(&(src) + 1)) << 8) + (uint16)src)
#define BUFFBE_TO_WORD(dest, src) \
    dest = (((uint16)(*(&(src) + 1))) + ((uint16)src << 8))
#define BUFFLE_TO_LONG(dest, src)                                             \
    dest = (((uint32)(*(&(src) + 3)) << 24) + ((uint32)(*(&(src) + 2)) << 16) \
            + ((uint32)(*(&(src) + 1)) << 8) + (uint32)src)
#define BUFFBE_TO_LONG(dest, src)                                      \
    dest = (((uint32)(*(&(src) + 3))) + ((uint32)(*(&(src) + 2)) << 8) \
            + ((uint32)(*(&(src) + 1)) << 16) + ((uint32)src << 24))

// Stuffing macros (MAKE SURE TO INIT BUFFER WITH ZEROS(o) BEFORE STUFFING)
#define BITS_TO_BUFFER(dest, src, start, size) \
    dest |= (uint8)(((uint8)src << start)      \
                    & (((uint8)0xFF >> (8 - size)) << start));
#define WORD_TO_BUFFLE(dest, src)                       \
    {                                                   \
        dest = ((uint8)src);                            \
        (*(&(dest) + 1)) = ((uint8)((uint16)src >> 8)); \
    }
#define WORD_TO_BUFFBE(dest, src)           \
    {                                       \
        dest = ((uint8)((uint16)src >> 8)); \
        (*(&(dest) + 1)) = ((uint8)src);    \
    }
#define LONG_TO_BUFFLE(dest, src)                        \
    {                                                    \
        dest = ((uint8)src);                             \
        (*(&(dest) + 1)) = ((uint8)((uint32)src >> 8));  \
        (*(&(dest) + 2)) = ((uint8)((uint32)src >> 16)); \
        (*(&(dest) + 3)) = ((uint8)((uint32)src >> 24)); \
    }
#define LONG_TO_BUFFBE(dest, src)                        \
    {                                                    \
        dest = ((uint8)((uint32)src >> 24));             \
        (*(&(dest) + 1)) = ((uint8)((uint32)src >> 16)); \
        (*(&(dest) + 2)) = ((uint8)((uint32)src >> 8));  \
        (*(&(dest) + 3)) = ((uint8)src);                 \
    }

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#endif  // STDDEFS_H
