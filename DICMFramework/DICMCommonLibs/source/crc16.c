//------------------------------------------------------------------------------
// Project:      Standard Library
// Module:       Crc16
//------------------------------------------------------------------------------
// Description:  Miscellaneous CRC functions.
//
//------------------------------------------------------------------------------
// Copyright:    Mixed (see header file)
//------------------------------------------------------------------------------
#include "crc16.h"


/***************************************************************************
* Implementation by DATASCHALT
* Date 13/10/17
* Brief  CRC16 Module
* Author
*      DATASCHALT engineering GmbH
*      www.dataschalt.com
*      eMail: christian.pohl@dataschalt.com
*
* Copyright (c) 2013 DATASCHALT engineering GmbH
* An der Huelshorst 7-9, 23568 Luebeck, Germany
* All rights reserved.
*
* This software is the confidential and proprietary information of
* DATASCHALT engineering GmbH ("Confidential Information"). You shall not
* disclose such Confidential Information and shall use it only in
* accordance with the terms of the license agreement you entered into
* with DATASCHALT engineering GmbH.
*****************************************************************************/
/*
This module is inspired from:
http://stackoverflow.com/questions/11481782/how-to-calculate-crc-16-from-hex-values.
Another nice CRC calculation website is found here: http://zorc.breitbandkatze.de/crc.html.
*/

// forward declarations:
uint16_t reflect (uint16_t crc, uint8_t bitnum);
void generate_crc_table(void);
uint16_t crctablefast (uint8_t const * p, uint16_t len);


// local variables
static bool is_initialized_ = false;

// 'order' [1..32] is the CRC polynom order, counted without the leading '1' bit
// 'polynom' is the CRC polynom without leading '1' bit
// 'direct' [0,1] specifies the kind of algorithm: 1=direct, no augmented zero bits
// 'crcinit' is the initial CRC value belonging to that algorithm
// 'crcxor' is the final XOR value
// 'refin' [0,1] specifies if a data byte is reflected before processing (UART) or not
// 'refout' [0,1] specifies if the CRC will be reflected before XOR
static const uint8_t order = 16;
static const uint16_t polynom = 0x4976;
static const uint8_t direct = 0;
static const uint16_t crcinit = 0x0000;
static const uint16_t crcxor = 0;
static const uint8_t refin = 0;
static const uint8_t refout = 0;

static uint16_t crcmask;
static uint16_t crchighbit;
static uint16_t crcinit_direct;
static uint16_t crcinit_nondirect;
static uint16_t crctab[256];


bool crc16_init(void){
	is_initialized_ = false;

    uint16_t i;
    uint16_t bit, crc;

    crcmask = ((((uint16_t)1<<(order-1))-1)<<1)|1;
    crchighbit = (uint16_t)1<<(order-1);

    generate_crc_table();

    if (!direct){

        crcinit_nondirect = crcinit;
        crc = crcinit;
        for (i=0; i<order; i++){

            bit = crc & crchighbit;
            crc<<= 1;
            if (bit){
                crc^= polynom;
            }
        }
        crc&= crcmask;
        crcinit_direct = crc;
    } else {

        crcinit_direct = crcinit;
        crc = crcinit;
        for (i=0; i<order; i++){

            bit = crc & 1;
            if (bit) crc^= polynom;
            crc >>= 1;
            if (bit){
                crc|= crchighbit;
            }
        }
        crcinit_nondirect = crc;
    }


/*
    // Some algorithm check according to http://srecord.sourceforge.net/crc16-ccitt.html.

    uint16_t crc_calc = 0xaa55;
    crc_calc = crctablefast("", 0);
    crc_calc = crctablefast("A", 1);
    crc_calc = crctablefast("123456789", 9);
    crc_calc = crctablefast("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 256);
*/

	is_initialized_ = true;

	return is_initialized_;
}

//##############################################################################

uint16_t reflect (uint16_t crc, uint8_t bitnum) {

    // reflects the lower 'bitnum' bits of 'crc'

    uint16_t i, j=1, crcout=0;

    for (i=(uint32_t)1<<(bitnum-1); i; i>>=1) {
        if (crc & i){
            crcout|=j;
        }
        j<<= 1;
    }
    return (crcout);
}

//##############################################################################

void generate_crc_table(void) {

    // make CRC lookup table used by table algorithms

    uint16_t i, j;
    uint16_t bit, crc;

    for (i=0; i<256; i++) {

        crc=(uint16_t)i;
        if (refin){
            crc = reflect(crc, 8);
        }
        crc<<= order-8;

        for (j=0; j<8; j++){
            bit = crc & crchighbit;
            crc<<= 1;
            if (bit){
                crc^= polynom;
            }
        }

        if (refin){
            crc = reflect(crc, order);
        }
        crc&= crcmask;
        crctab[i]= crc;
    }
}

//##############################################################################

uint16_t crctablefast (uint8_t const * p, uint16_t len) {

    // fast lookup table algorithm without augmented zero bytes, e.g. used in pkzip.
    // only usable with polynom orders of 8, 16, 24 or 32.

    uint16_t crc = crcinit_direct;

    if (refin){
        crc = reflect(crc, order);
    }

    if (!refin){
        while (len--){
            crc = (crc << 8) ^ crctab[ ((crc >> (order-8)) & 0xff) ^ *p++];
        }
    } else {
        while (len--){
            crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ *p++];
        }
    }

    if (refout^refin){
        crc = reflect(crc, order);
    }
    crc^= crcxor;
    crc&= crcmask;

    return(crc);
}

//##############################################################################

uint16_t crctable (uint8_t const * p, uint16_t len) {

    // normal lookup table algorithm with augmented zero bytes.
    // only usable with polynom orders of 8, 16, 24 or 32.

    uint16_t crc = crcinit_nondirect;

    if (refin){
        crc = reflect(crc, order);
    }

    if (!refin){
        while (len--){
            crc = ((crc << 8) | *p++) ^ crctab[ (crc >> (order-8))  & 0xff];
        }
    } else {
        while (len--){
            crc = ((crc >> 8) | (*p++ << (order-8))) ^ crctab[ crc & 0xff];
        }
    }

    if (!refin){
        while (++len < order/8){
            crc = (crc << 8) ^ crctab[ (crc >> (order-8))  & 0xff];
        }
    } else {
        while (++len < order/8){
            crc = (crc >> 8) ^ crctab[crc & 0xff];
        }
    }

    if (refout^refin){
        crc = reflect(crc, order);
    }
    crc^= crcxor;
    crc&= crcmask;

    return(crc);
}

//##############################################################################

uint16_t crcbitbybit(uint8_t* p, uint16_t len) {

    // bit by bit algorithm with augmented zero bytes.
    // does not use lookup table, suited for polynom orders between 1...32.

    uint16_t i, j, c, bit;
    uint16_t crc = crcinit_nondirect;

    for (i=0; i<len; i++) {

        c = (uint16_t)*p++;
        if (refin){
            c = reflect(c, 8);
        }

        for (j=0x80; j; j>>=1){

            bit = crc & crchighbit;
            crc<<= 1;
            if (c & j) crc|= 1;
            if (bit){
                crc^= polynom;
            }
        }
    }

    for (i=0; i<order; i++) {

        bit = crc & crchighbit;
        crc<<= 1;
        if (bit){
            crc^= polynom;
        }
    }

    if (refout){
        crc=reflect(crc, order);
    }
    crc^= crcxor;
    crc&= crcmask;

    return(crc);
}

//##############################################################################

uint16_t crcbitbybitfast(uint8_t* p, uint16_t len) {

    // fast bit by bit algorithm without augmented zero bytes.
    // does not use lookup table, suited for polynom orders between 1...32.

    uint16_t i, j, c, bit;
    uint16_t crc = crcinit_direct;

    for (i=0; i<len; i++) {

        c = (uint16_t)*p++;
        if (refin) c = reflect(c, 8);

        for (j=0x80; j; j>>=1) {

            bit = crc & crchighbit;
            crc<<= 1;
            if (c & j){
                bit^= crchighbit;
            }
            if (bit){
                crc^= polynom;
            }
        }
    }

    if (refout){
        crc=reflect(crc, order);
    }
    crc^= crcxor;
    crc&= crcmask;

    return(crc);
}

/***************************************************************************
 * Implementation by Dometic
 * 
 * Description: This function calculates a CRC 16 bits using
 *              CCITT-FALSE CRC16 Standard
 *              See AUTOSAR.
 *              https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf
 * Notes:
 *              CRC result width:      16 bits
 *              Polynomial:            1021h
 *              Initial value:         FFFFh
 *              Input data reflected:  No
 *              Result data reflected: No
 *              XOR value:             0000h
 *              Check:                 29B1h
 *              Magic check:           0000h
 * 
 * Copyright:   Dometic Group
 * 
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic. The reproduction or
 *              disclosure, in whole or in part, to anyone outside of Dometic
 *              without the written approval of a Dometic officer under a
 *              Non-Disclosure Agreement is expressly prohibited.
 *
 *              All rights reserved
 ***************************************************************************/

//------------------------------------------------------------------------------
// Function:    crc16_start
// Description: Return the start value
//------------------------------------------------------------------------------
// Return:      16-bit CRC value
//------------------------------------------------------------------------------
uint16_t crc16_start(void)
{
    return 0xFFFFu;
}

//------------------------------------------------------------------------------
// Function:    crc16_process
// Description: See CRC_GetCrc16
//------------------------------------------------------------------------------
// Return:      16-bit CRC value.
//------------------------------------------------------------------------------
uint16_t crc16_process
(
    uint16_t    crc16,   // in: Value returned by crc16_start or crc16_process
    const void* data,    // in: Pointer to data used for crc computation
    uint16_t    size     // in: Size of data (bytes)
)
{
    // Lookup table
    static const uint16_t lookup[256] =
    {
        0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
        0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
        0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
        0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
        0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
        0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
        0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
        0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
        0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
        0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
        0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
        0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
        0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
        0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
        0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
        0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
        0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
        0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
        0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
        0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
        0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
        0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
        0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
        0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
        0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
        0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
        0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
        0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
        0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
        0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
        0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
        0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
    };

    // Compute
    const uint8_t* bytes = (const uint8_t*)data;
	uint16_t i, result = crc16;
	uint8_t index;
    for (i=0u; i<size; i++)
    {
        index   = ((uint8_t)(result>>8) ^ bytes[i]) & 0xFFu;
        result  = ((uint16_t)result << 8);
        result ^= lookup[index];
    }
    return result;
}

//------------------------------------------------------------------------------
// Function:    crc16_finalize
// Description: Finalize CRC16 computation
//------------------------------------------------------------------------------
// Return:      16-bit CRC value
//------------------------------------------------------------------------------
uint16_t crc16_finalize
(
    uint16_t crc16  // In: Value returned by the last crc16_process() call
)
{
    return crc16; // No mask
}

//------------------------------------------------------------------------------
// Function:    crc16_get
// Description: This function calculates CRC based on the CCITT-FALSE / AUTOSAR standard
//------------------------------------------------------------------------------
// Return:      8-bit CRC value
//------------------------------------------------------------------------------
uint16_t crc16_get
(
    const void* data,  // in: Pointer to data used for crc computation
    uint16_t    size   // in: Size of data (bytes)
)
{
    return crc16_process(0xFFFFu, data, size);
}

//------------------------------------------------------------------------------
// Function:    crc16_self_test
// Description: Perform self-test
//------------------------------------------------------------------------------
// Return:      Test result
//------------------------------------------------------------------------------
bool crc16_self_test(void)
{
    if (crc16_get("Hello world!", 12) != 0xBD22u)
        return false;
    if (crc16_get("123456789", 9) != 0x29B1u)
        return false; 
    return true;    
}

//------------------------------------------------------------------------------
// Function:    crc16_dataschalt
// Description: This function calculates CRC based on DATASCHALT parameters
//------------------------------------------------------------------------------
// Return:      16-bit CRC value
//------------------------------------------------------------------------------
uint16_t crc16_dataschalt
(
    const void* data,  // in: Pointer to data used for crc computation
    uint16_t    size   // in: Size of data (bytes)
)
{
    // Lookup table
    static const uint16_t lookup[256] =
    {
        0x0000, 0x4976, 0x92EC, 0xDB9A, 0x6CAE, 0x25D8, 0xFE42, 0xB734, 0xD95C, 0x902A, 0x4BB0, 0x02C6, 0xB5F2, 0xFC84, 0x271E, 0x6E68,
        0xFBCE, 0xB2B8, 0x6922, 0x2054, 0x9760, 0xDE16, 0x058C, 0x4CFA, 0x2292, 0x6BE4, 0xB07E, 0xF908, 0x4E3C, 0x074A, 0xDCD0, 0x95A6,
        0xBEEA, 0xF79C, 0x2C06, 0x6570, 0xD244, 0x9B32, 0x40A8, 0x09DE, 0x67B6, 0x2EC0, 0xF55A, 0xBC2C, 0x0B18, 0x426E, 0x99F4, 0xD082,
        0x4524, 0x0C52, 0xD7C8, 0x9EBE, 0x298A, 0x60FC, 0xBB66, 0xF210, 0x9C78, 0xD50E, 0x0E94, 0x47E2, 0xF0D6, 0xB9A0, 0x623A, 0x2B4C,
        0x34A2, 0x7DD4, 0xA64E, 0xEF38, 0x580C, 0x117A, 0xCAE0, 0x8396, 0xEDFE, 0xA488, 0x7F12, 0x3664, 0x8150, 0xC826, 0x13BC, 0x5ACA,
        0xCF6C, 0x861A, 0x5D80, 0x14F6, 0xA3C2, 0xEAB4, 0x312E, 0x7858, 0x1630, 0x5F46, 0x84DC, 0xCDAA, 0x7A9E, 0x33E8, 0xE872, 0xA104,
        0x8A48, 0xC33E, 0x18A4, 0x51D2, 0xE6E6, 0xAF90, 0x740A, 0x3D7C, 0x5314, 0x1A62, 0xC1F8, 0x888E, 0x3FBA, 0x76CC, 0xAD56, 0xE420,
        0x7186, 0x38F0, 0xE36A, 0xAA1C, 0x1D28, 0x545E, 0x8FC4, 0xC6B2, 0xA8DA, 0xE1AC, 0x3A36, 0x7340, 0xC474, 0x8D02, 0x5698, 0x1FEE,
        0x6944, 0x2032, 0xFBA8, 0xB2DE, 0x05EA, 0x4C9C, 0x9706, 0xDE70, 0xB018, 0xF96E, 0x22F4, 0x6B82, 0xDCB6, 0x95C0, 0x4E5A, 0x072C,
        0x928A, 0xDBFC, 0x0066, 0x4910, 0xFE24, 0xB752, 0x6CC8, 0x25BE, 0x4BD6, 0x02A0, 0xD93A, 0x904C, 0x2778, 0x6E0E, 0xB594, 0xFCE2,
        0xD7AE, 0x9ED8, 0x4542, 0x0C34, 0xBB00, 0xF276, 0x29EC, 0x609A, 0x0EF2, 0x4784, 0x9C1E, 0xD568, 0x625C, 0x2B2A, 0xF0B0, 0xB9C6,
        0x2C60, 0x6516, 0xBE8C, 0xF7FA, 0x40CE, 0x09B8, 0xD222, 0x9B54, 0xF53C, 0xBC4A, 0x67D0, 0x2EA6, 0x9992, 0xD0E4, 0x0B7E, 0x4208,
        0x5DE6, 0x1490, 0xCF0A, 0x867C, 0x3148, 0x783E, 0xA3A4, 0xEAD2, 0x84BA, 0xCDCC, 0x1656, 0x5F20, 0xE814, 0xA162, 0x7AF8, 0x338E,
        0xA628, 0xEF5E, 0x34C4, 0x7DB2, 0xCA86, 0x83F0, 0x586A, 0x111C, 0x7F74, 0x3602, 0xED98, 0xA4EE, 0x13DA, 0x5AAC, 0x8136, 0xC840,
        0xE30C, 0xAA7A, 0x71E0, 0x3896, 0x8FA2, 0xC6D4, 0x1D4E, 0x5438, 0x3A50, 0x7326, 0xA8BC, 0xE1CA, 0x56FE, 0x1F88, 0xC412, 0x8D64,
        0x18C2, 0x51B4, 0x8A2E, 0xC358, 0x746C, 0x3D1A, 0xE680, 0xAFF6, 0xC19E, 0x88E8, 0x5372, 0x1A04, 0xAD30, 0xE446, 0x3FDC, 0x76AA 
    };

    // Process
    const uint8_t* bytes = (const uint8_t*)data;
	uint16_t i, result = 0u;
	uint8_t index;
    for (i=0u; i<size; i++)
    {
        index   = ((uint8_t)(result>>8) ^ bytes[i]) & 0xFFu;
        result  = ((uint16_t)result << 8);
        result ^= lookup[index];
    }    
    return result;
}