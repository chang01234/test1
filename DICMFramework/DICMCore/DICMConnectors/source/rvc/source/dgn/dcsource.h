/*
 * dcsource.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef DCSOURCE_H_
#define DCSOURCE_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "dgnnode.h"

#include "ddm2.h"

#define DC_SOURCE_STATUS_1_DGN                131069  // 0x1FFFD
#define DC_SOURCE_STATUS_2_DGN                131068  // 0x1FFFC
#define DC_SOURCE_STATUS_3_DGN                131067  // 0x1FFFB
#define DC_SOURCE_STATUS_4_DGN                130761  // 0x1FEC9
#define DC_SOURCE_STATUS_5_DGN                130760  // 0x1FEC8
#define DC_SOURCE_STATUS_6_DGN                130759  // 0x1FEC7
#define DC_SOURCE_STATUS_7_DGN                130732  // 0x1FEAC
#define DC_SOURCE_STATUS_8_DGN                130731  // 0x1FEAB
#define DC_SOURCE_STATUS_9_DGN                130730  // 0x1FEAA
#define DC_SOURCE_STATUS_10_DGN               130729  // 0x1FEA9
#define DC_SOURCE_STATUS_11_DGN               130725  // 0x1FEA5
#define DC_SOURCE_STATUS_12_DGN               130552  // 0x1FDF8
#define DC_SOURCE_STATUS_13_DGN               130535  // 0x1FDE7
#define DC_SOURCE_CONFIGURATION_COMMAND_1_DGN 130550  // 0x1FDF6
#define DC_SOURCE_CONFIGURATION_STATUS_1_DGN  130551  // 0x1FDF7
#define DC_SOURCE_CONFIGURATION_COMMAND_2_DGN 130548  // 0x1FDF4
#define DC_SOURCE_CONFIGURATION_STATUS_2_DGN  130549  // 0x1FDF5
#define DC_SOURCE_CONFIGURATION_COMMAND_3_DGN 130526  // 0x1FDDE
#define DC_SOURCE_CONNECTION_STATUS_DGN       130512  // 0x1FDD0
#define DC_SOURCE_COMMAND_DGN                 130724  // 0x1FEA4
#define DC_DISCONNECT_COMMAND_DGN             130767  // 0x1FECF
#define DC_DISCONNECT_STATUS_DGN              130768  // 0x1FED0

void dcsource_init(uint8_t connector_id, list_t *p_prod);
void remove_dcsource_nodes(uint8_t sa);

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
bool receive131069Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRC0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
bool receive131068Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
bool receive131067Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCTHR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
bool receive130761Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
bool receive130760Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCFIVE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
bool receive130759Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCSIX0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
bool receive130732Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCSEV0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
bool receive130731Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCEIG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
bool receive130730Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCNINE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
bool receive130729Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCTEN0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
bool receive130725Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCELE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
bool receive130552Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCTWE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
bool receive130535Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCTHI0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
bool transmit130550Dgn(uint8_t instance, uint8_t *p_data);
bool receive130551Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
bool transmit130548Dgn(uint8_t instance, uint8_t *p_data);
bool receive130549Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
bool receive130512Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCSRCCONN0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
bool transmit130724Dgn(uint8_t instance, uint8_t *p_data);
void handleRVCDCSRCCMD0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
bool transmit130526Dgn(uint8_t instance, uint8_t *p_data);
void handleRVCDCSRCCFGTHR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#ifdef RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS
bool transmit130767Dgn(uint8_t instance, uint8_t *p_data);
bool receive130768Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCDCDISCONN0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif
#endif /* DCSOURCE_H_ */
