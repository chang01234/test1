/*
 * roof_fan.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef ROOF_FAN_H_
#define ROOF_FAN_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define ROOF_FAN_STATUS_1_DGN  130727  // 0x1FEA7
#define ROOF_FAN_COMMAND_1_DGN 130726  // 0x1FEA6
#define ROOF_FAN_STATUS_2_DGN  130531  // 0x1FDE3
#define ROOF_FAN_COMMAND_2_DGN 130530  // 0x1FDE2

#if defined(RVC_CONFIG_IMPL_ROOF_FAN) || defined(RVC_CONFIG_INTERF_ROOF_FAN)
void roof_fan_init(uint8_t connector_id);
void handleRVCRFAN0(uint32_t dgn, DDMP2_FRAME *p_frame);
void handleRVCRFANTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_IMPL_ROOF_FAN
bool receive130726Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130530Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit130727Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130531Dgn(uint8_t instance, uint8_t *p_data);
#endif
#ifdef RVC_CONFIG_INTERF_ROOF_FAN
bool receive130727Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130531Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit130726Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130530Dgn(uint8_t instance, uint8_t *p_data);
#endif

#endif /* ROOF_FAN_H_ */
