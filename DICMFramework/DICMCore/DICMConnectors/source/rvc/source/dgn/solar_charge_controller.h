/*
 * solar_charge_controller.h
 *
 *  Created on: 30 Oct. 2025
 *      Author: Leo
 */

#ifndef SOLAR_CHARGE_CONTROLLER_H_
#define SOLAR_CHARGE_CONTROLLER_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "dgnnode.h"

#include "ddm2.h"

#define SOLAR_CONTROLLER_STATUS_DGN                  130739  // 0x1FEB3
#define SOLAR_CONTROLLER_STATUS_2_DGN                130693  // 0x1FE85
#define SOLAR_CONTROLLER_STATUS_3_DGN                130692  // 0x1FE84
#define SOLAR_CONTROLLER_STATUS_4_DGN                130691  // 0x1FE83
#define SOLAR_CONTROLLER_STATUS_5_DGN                130690  // 0x1FE82
#define SOLAR_CONTROLLER_STATUS_6_DGN                130689  // 0x1FE81
#define SOLAR_CONTROLLER_BATTERY_STATUS_DGN          130688  // 0x1FE80
#define SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS_DGN      130559  // 0x1FDFF
#define SOLAR_CONTROLLER_CONFIGURATION_STATUS_DGN    130738  // 0x1FEB2
#define SOLAR_CONTROLLER_CONFIGURATION_STATUS_2_DGN  130558  // 0x1FDFE
#define SOLAR_CONTROLLER_CONFIGURATION_STATUS_3_DGN  130556  // 0x1FDFC
#define SOLAR_CONTROLLER_CONFIGURATION_STATUS_4_DGN  130554  // 0x1FDFA
#define SOLAR_CONTROLLER_CONFIGURATION_STATUS_5_DGN  130511  // 0x1FDCF
#define SOLAR_EQUALIZATION_STATUS_DGN                130735  // 0x1FEAF
#define SOLAR_EQUALIZATION_CONFIGURATION_STATUS_DGN  130734  // 0x1FEAE
#define SOLAR_CONTROLLER_COMMAND_DGN                 130737  // 0x1FEB1
#define SOLAR_CONTROLLER_CONFIGURATION_COMMAND_DGN   130736  // 0x1FEB0
#define SOLAR_CONTROLLER_CONFIGURATION_COMMAND_2_DGN 130557  // 0x1FDFD
#define SOLAR_CONTROLLER_CONFIGURATION_COMMAND_3_DGN 130555  // 0x1FDFB
#define SOLAR_CONTROLLER_CONFIGURATION_COMMAND_4_DGN 130553  // 0x1FDF9
#define SOLAR_CONTROLLER_CONFIGURATION_COMMAND_5_DGN 130510  // 0x1FDCE
#define SOLAR_EQUALIZATION_CONFIGURATION_COMMAND_DGN 130733  // 0x1FEAD

void solar_charge_controller_init(uint8_t connector_id, list_t *p_prod);
void remove_solar_charge_controller_nodes(uint8_t sa);

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS
bool transmit130737Dgn(uint8_t instance, uint8_t *p_data);
bool receive130739Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLAR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_2
bool receive130693Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_3
bool receive130692Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARTHR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_4
bool receive130691Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_5
bool receive130690Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARFIVE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_6
bool receive130689Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARSIX0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_BATTERY_STATUS
bool receive130688Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARBAT0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS
bool receive130559Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARARR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS
bool transmit130736Dgn(uint8_t instance, uint8_t *p_data);
bool receive130738Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_2
bool transmit130557Dgn(uint8_t instance, uint8_t *p_data);
bool receive130558Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_3
bool transmit130555Dgn(uint8_t instance, uint8_t *p_data);
bool receive130556Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARCFGTHR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_4
bool transmit130553Dgn(uint8_t instance, uint8_t *p_data);
bool receive130554Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARCFGFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_5
bool transmit130510Dgn(uint8_t instance, uint8_t *p_data);
bool receive130511Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLARCFGFIVE0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_STATUS
bool receive130735Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLAREQ0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_CONFIGURATION_STATUS
bool transmit130733Dgn(uint8_t instance, uint8_t *p_data);
bool receive130734Dgn(uint8_t *p_data, uint8_t sa, size_t size);
void handleRVCSOLAREQCFG0(uint32_t dgn, DDMP2_FRAME *p_frame);
#endif

#endif /* SOLAR_CHARGE_CONTROLLER_H_ */
