/*
 * date_time.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef DATE_TIME_H_
#define DATE_TIME_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define SET_DATE_TIME_COMMAND_DGN 131070  // 0x1FFFE
#define DATE_TIME_STATUS          131071  // 0x1FFFF

#if defined(RVC_CONFIG_BUS_TIME_USE) || defined(RVC_CONFIG_BUS_TIME_KEEP)
void date_time_init(uint8_t connector_id);
#ifdef RVC_CONFIG_BUS_TIME_USE
bool receive131071Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit131070Dgn(uint8_t instance, uint8_t *p_data);
#endif
void handleRVCTIME0(uint32_t dgn, DDMP2_FRAME *p_frame);
#ifdef RVC_CONFIG_BUS_TIME_KEEP
bool receive131070Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit131071Dgn(uint8_t instance, uint8_t *p_data);
#endif
#endif

#endif /* DATE_TIME_H_ */
