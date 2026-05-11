/*
 * refrig.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef REFRIG_H_
#define REFRIG_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define REFRIGERATOR_STATUS_DGN  130515  // 0x1FDD3
#define REFRIGERATOR_COMMAND_DGN 130514  // 0x1FDD2

#ifdef RVC_CONFIG_IMPL_REFRIGERATOR
void refrig_init(uint8_t connector_id);
bool receive130514Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit130515Dgn(uint8_t instance, uint8_t *p_data);
void handleRVCREFRIG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#endif /* REFRIG_H_ */
