/*
 * heatpump.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef HEATPUMP_H_
#define HEATPUMP_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define HEAT_PUMP_COMMAND_DGN 130970  // 0x1FF9A
#define HEAT_PUMP_STATUS_DGN  130971  // 0x1FF9B

#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
void heatpump_init(uint8_t connector_id);
bool receive130971Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit130970Dgn(uint8_t instance, uint8_t *p_data);
void handleRVCHPUMP0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#endif /* HEATPUMP_H_ */
