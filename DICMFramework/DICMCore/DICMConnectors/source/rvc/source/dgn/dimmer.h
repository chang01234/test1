/*
 * dimmer.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef DIMMER_H_
#define DIMMER_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define DC_DIMMER_STATUS_3_DGN  130778  // 0x1FEDA
#define DC_DIMMER_COMMAND_2_DGN 130779  // 0x1FEDB

#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1) || defined(RVC_CONFIG_INTERF_DC_DIMMER_1)
void dimmer_init(uint8_t connector_id);
void handleRVC3DIM0(uint32_t dgn, DDMP2_FRAME *p_frame);
void handleRVC2DIM0(uint32_t dgn, DDMP2_FRAME *p_frame);
#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1)
bool receive130779Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit130778Dgn(uint8_t instance, uint8_t *p_data);
#endif
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_1)
bool transmit130779Dgn(uint8_t instance, uint8_t *p_data);
bool receive130778Dgn(uint8_t *p_data, uint8_t sa, size_t size);
#endif

#endif
#endif /* DIMMER_H_ */
