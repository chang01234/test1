//------------------------------------------------------------------------------
// Project:      Standard Library
// Module:       Crc16
//------------------------------------------------------------------------------
// Description:  Miscellaneous CRC functions.
//
//------------------------------------------------------------------------------
// Copyright:    Mixed (see sections below)
//------------------------------------------------------------------------------
#ifndef CRC16_H_
#define CRC16_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

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
bool     crc16_init(void);
uint16_t crctable (uint8_t const * p, uint16_t len);
uint16_t crctablefast (uint8_t const * p, uint16_t len);

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
uint16_t crc16_start(void);
uint16_t crc16_process( uint16_t crc16, const void* data, uint16_t size);
uint16_t crc16_finalize(uint16_t crc16);
uint16_t crc16_get(const void* data, uint16_t size);

uint16_t crc16_dataschalt(const void* data, uint16_t size); // Same as DATASCHALT implementation... but simple without using RAM
bool     crc16_self_test(void);

/****************************************************************************/
#ifdef __cplusplus
}      // extern "C"
#endif
#endif // CRC16_H_