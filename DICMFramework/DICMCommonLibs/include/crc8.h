//------------------------------------------------------------------------------
// Project:     Standard Library
// Module:      Crc8
//------------------------------------------------------------------------------
// Description: This function calculates CRC based on the SAE-J1850 CRC8 Standard
//              See AUTOSAR or SAE-J1850.
//              https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf
// Details:
//              Result width:          8 bits
//              Polynomial:            1Dh
//              Initial value:         FFh
//              Input data reflected:  No
//              Result data reflected: No
//              XOR value:             FFh
//              Check:                 4Bh
//              Magic check:           C4h
//------------------------------------------------------------------------------
// Copyright:   Dometic Group
//
//              This source file and the information contained in it are
//              confidential and proprietary to Dometic. The reproduction or
//              disclosure, in whole or in part, to anyone outside of Dometic
//              without the written approval of a Dometic officer under a
//              Non-Disclosure Agreement is expressly prohibited.
//
//              All rights reserved
//------------------------------------------------------------------------------
#ifndef CRC8_H_
#define CRC8_H_
#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Public Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------
// Note: Module is a combination of lib_crc8, lib_crc16, lib_crc32
//       They are combined for convenience but we consider them
//       as if they were separate modules when it comes to naming convention
//------------------------------------------------------------------------------
uint8_t crc8_start(void);
uint8_t crc8_process(uint8_t crc8, const void* data, uint16_t size);
uint8_t crc8_finalize(uint8_t crc8);

uint8_t crc8_get(const void* data, uint16_t size);
bool    crc8_self_test(void);

//------------------------------------------------------------------------------
#ifdef __cplusplus
}      // extern "C"
#endif
#endif // CRC8_H_

