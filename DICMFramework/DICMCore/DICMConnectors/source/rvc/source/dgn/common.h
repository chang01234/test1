/*
 * common.h
 *
 *  Created on: 1 sep. 2025
 *      Author: Andlun
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

#define INFORMATION_REQUEST_DGN 59904  // 0xEA00 request for dgn

#define PROP_MSG_DGN       61184   // 0xEF00 Propriatary message, global not allowed
#define PRODUCT_ID_MSG_DGN 65259   // 0xFEEB
#define DMRV_MSG_DGN       130762  // 0x1FECA
#define GENERAL_RESET_DGN  98048   // 0x17F00
#define DOWNLOAD_DGN       97536   // 0x17D00
#define ACKNOWLEDGMENT_DGN 59392   // 0xE800
#define GENERIC_CONFIGURATION_STATUS_DGN 130776  // 0x1FED8

void request_info_request_dgn(uint32_t dgn, uint8_t da);

#endif /* COMMON_H_ */
