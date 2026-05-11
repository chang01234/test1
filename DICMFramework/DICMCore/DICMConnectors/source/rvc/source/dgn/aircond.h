/*
 * aircond.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef AIRCOND_H_
#define AIRCOND_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define AIR_CONDITIONER_COMMAND_DGN 131040  // 0x1FFE0
#define AIR_CONDITIONER_STATUS_DGN  131041  // 0x1FFE1
#define AIR_CONDITIONING_STATUS_2   130505  // 0x1FDC9

#if defined(RVC_CONFIG_IMPL_AIR_CONDITIONER) || defined(RVC_CONFIG_INTERF_AIR_CONDITIONER)
void aircond_init(uint8_t connector_id);
#ifdef RVC_CONFIG_IMPL_AIR_CONDITIONER
bool receive131040Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit131041Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130505Dgn(uint8_t instance, uint8_t *p_data);
void handleRVC2AC0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
bool receive131041Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit131040Dgn(uint8_t instance, uint8_t *p_data);
#endif
void handleRVCAC0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#endif /* AIRCOND_H_ */
