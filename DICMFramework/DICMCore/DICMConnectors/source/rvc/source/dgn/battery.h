/*
 * battery.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef BATTERY_H_
#define BATTERY_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "dgnnode.h"

#include "ddm2.h"

#define BATTERY_SUMMARY_DGN 130545  // 0x1FDF1
void battery_init(uint8_t connector_id, list_t *p_prod);
void remove_battery_nodes(uint8_t sa);

#ifdef RVC_CONFIG_INTERF_BATTERY_SUMMARY
bool receive130545Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCBATSUM0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#endif /* BATTERY_H_ */
