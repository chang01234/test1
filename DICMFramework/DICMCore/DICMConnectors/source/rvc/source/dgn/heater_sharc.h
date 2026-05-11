/*
 * heater_sharc.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef HEATER_SHARC_H_
#define HEATER_SHARC_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define HEATER_SHARC_VERSION_DGN        130552  // 0x1FDF8
#define HEATER_SHARC_FAULTS_DGN         130553  // 0x1FDF9
#define HEATER_SHARC_SCHED_COMMAND_DGN  130554  // 0x1FDFA
#define HEATER_SHARC_SCHED_FEEDBACK_DGN 130555  // 0x1FDFB
#define HEATER_SHARC_HMI_STATUS_DGN     130556  // 0x1FDFC
#define HEATER_SHARC_STATUS_DGN         130557  // 0x1FDFD
#define HEATER_SHARC_OPER_COMMAND_DGN   130558  // 0x1FDFE
#define HEATER_SHARC_OPER_FEEDBACK_DGN  130559  // 0x1FDFF

#ifdef RVC_CONFIG_INTERF_SHARC_HTR
void heater_sharc_init(uint8_t connector_id);
bool transmit130558PropDgn(uint8_t instance, uint8_t *p_data);
bool receive130559PropDgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCHTR0(uint32_t dgn, DDMP2_FRAME *p_frame);
bool receive130552PropDgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCHTRVER0(uint32_t dgn, DDMP2_FRAME *p_frame);
bool receive130553PropDgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCHTRFAULT0(uint32_t dgn, DDMP2_FRAME *p_frame);
bool transmit130554PropDgn(uint8_t instance, uint8_t *p_data);
bool receive130555PropDgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCHTRSCHED0(uint32_t dgn, DDMP2_FRAME *p_frame);
bool receive130557PropDgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCHTRST0(uint32_t dgn, DDMP2_FRAME *p_frame);
bool transmit130556PropDgn(uint8_t instance, uint8_t *p_data);
void handleRVCHMI0(uint32_t dgn, DDMP2_FRAME *p_frame);

#endif
#endif /* HEATER_SHARC_H_ */
