/*
 * inverter_charger.h
 *
 *  Created on: 11 Nov. 2025
 *      Author: Leo
 */

#ifndef INVERTER_CHARGER_H_
#define INVERTER_CHARGER_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "dgnnode.h"

#include "ddm2.h"

#define INVERTER_AC_STATUS_1_DGN                       131031  // 0x1FFD7
#define INVERTER_AC_STATUS_2_DGN                       131030  // 0x1FFD6
#define INVERTER_AC_STATUS_3_DGN                       131029  // 0x1FFD5
#define INVERTER_AC_STATUS_4_DGN                       130959  // 0x1FF8F
#define INVERTER_DC_STATUS_DGN                         130792  // 0x1FEE8
#define INVERTER_STATUS_DGN                            131028  // 0x1FFD4
#define INVERTER_COMMAND_DGN                           131027  // 0x1FFD3
#define INVERTER_CONFIGURATION_STATUS_1_DGN            131026  // 0x1FFD2
#define INVERTER_CONFIGURATION_COMMAND_1_DGN           131024  // 0x1FFD0
#define INVERTER_CONFIGURATION_STATUS_2_DGN            131025  // 0x1FFD1
#define INVERTER_CONFIGURATION_COMMAND_2_DGN           131023  // 0x1FFCF
#define INVERTER_CONFIGURATION_STATUS_3_DGN            130766  // 0x1FECE
#define INVERTER_CONFIGURATION_COMMAND_3_DGN           130765  // 0x1FECD
#define INVERTER_CONFIGURATION_STATUS_4_DGN            130715  // 0x1FE9B
#define INVERTER_CONFIGURATION_COMMAND_4_DGN           130714  // 0x1FE9A
#define INVERTER_TEMPERATURE_STATUS_DGN                130749  // 0x1FEBD
#define INVERTER_TEMPERATURE_STATUS_2_DGN              130507  // 0x1FDCB
#define CHARGER_AC_STATUS_1_DGN                        131018  // 0x1FFCA
#define CHARGER_AC_STATUS_3_DGN                        131016  // 0x1FFC8
#define CHARGER_STATUS_DGN                             131015  // 0x1FFC7
#define CHARGER_COMMAND_DGN                            131013  // 0x1FFC5
#define CHARGER_STATUS_2_DGN                           130723  // 0x1FEA3
#define CHARGER_CONFIGURATION_STATUS_DGN               131014  // 0x1FFC6
#define CHARGER_CONFIGURATION_COMMAND_DGN              131012  // 0x1FFC4
#define CHARGER_CONFIGURATION_STATUS_2_DGN             130966  // 0x1FF96
#define CHARGER_CONFIGURATION_COMMAND_2_DGN            130965  // 0x1FF95
#define CHARGER_CONFIGURATION_STATUS_3_DGN             130764  // 0x1FECC
#define CHARGER_CONFIGURATION_COMMAND_3_DGN            130763  // 0x1FECB
#define CHARGER_CONFIGURATION_STATUS_4_DGN             130751  // 0x1FEBF
#define CHARGER_CONFIGURATION_COMMAND_4_DGN            130750  // 0x1FEBE
#define CHARGER_EQUALIZATION_CONFIGURATION_STATUS_DGN  130968  // 0x1FF98
#define CHARGER_EQUALIZATION_CONFIGURATION_COMMAND_DGN 130967  // 0x1FF97
#define CHARGER_ACFAULT_CONFIG_STATUS_1_DGN            130953  // 0x1FF89
#define CHARGER_ACFAULT_CONFIG_COMMAND_1_DGN           130951  // 0x1FF87

void inverter_charger_init(uint8_t connector_id, list_t *p_prod);
void remove_inverter_charger_nodes(uint8_t sa);

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_1
bool receive131031Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTAC0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_2
bool receive131030Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTACTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_3
bool receive131029Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTACTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_4
bool receive130959Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTACFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_DC_STATUS
bool receive130792Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTDC0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_STATUS
bool transmit131027Dgn(uint8_t instance, uint8_t *p_data);
bool receive131028Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERT0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_1
bool transmit131024Dgn(uint8_t instance, uint8_t *p_data);
bool receive131026Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_2
bool transmit131023Dgn(uint8_t instance, uint8_t *p_data);
bool receive131025Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_3
bool transmit130765Dgn(uint8_t instance, uint8_t *p_data);
bool receive130766Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTCFGTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_4
bool transmit130714Dgn(uint8_t instance, uint8_t *p_data);
bool receive130715Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTCFGFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS
bool receive130749Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTTEMP0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS_2
bool receive130507Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCINVERTTEMPTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_1
bool receive131018Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGAC0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_3
bool receive131016Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGACTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS
bool transmit131013Dgn(uint8_t instance, uint8_t *p_data);
bool receive131015Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS_2
bool receive130723Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS
bool transmit131012Dgn(uint8_t instance, uint8_t *p_data);
bool receive131014Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_2
bool transmit130965Dgn(uint8_t instance, uint8_t *p_data);
bool receive130966Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_3
bool transmit130763Dgn(uint8_t instance, uint8_t *p_data);
bool receive130764Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGCFGTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_4
bool transmit130750Dgn(uint8_t instance, uint8_t *p_data);
bool receive130751Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGCFGFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_EQUALIZATION_CONFIGURATION_STATUS
bool transmit130967Dgn(uint8_t instance, uint8_t *p_data);
bool receive130968Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGEQCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_ACFAULT_CONFIG_STATUS_1
bool transmit130951Dgn(uint8_t instance, uint8_t *p_data);
bool receive130953Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCCHRGACFAULTCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#endif /* INVERTER_CHARGER_H_ */
