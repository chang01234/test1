/*
 * thermostat.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef THERMOSTAT_H_
#define THERMOSTAT_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

#define THERMOSTAT_AMBIENT_STATUS_DGN 130972  // 0x1FF9C
#define THERMOSTAT_STATUS_1_DGN       131042  // 0x1FFE2
#define THERMOSTAT_COMMAND_1_DGN      130809  // 0x1FEF9
#define THERMOSTAT_STATUS_2_DGN       130810  // 0x1FEFA
#define THERMOSTAT_COMMAND_2_DGN      130808  // 0x1FEF8
#define THERMOSTAT_SCHEDULE_STATUS_1  130807  // 0x1FEF7
#define THERMOSTAT_SCHEDULE_STATUS_2  130806  // 0x1FEF6
#define THERMOSTAT_SCHEDULE_COMMAND_1 130805  // 0x1FEF5
#define THERMOSTAT_SCHEDULE_COMMAND_2 130804  // 0x1FEF4

#if defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4) || defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4)
#define MAX_NUM_OF_ZONE_INSTANCES (4)
#elif defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3) || defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3)
#define MAX_NUM_OF_ZONE_INSTANCES (3)
#elif defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2) || defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2)
#define MAX_NUM_OF_ZONE_INSTANCES (2)
#elif defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1) || defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1)
#define MAX_NUM_OF_ZONE_INSTANCES (1)
#else
#define MAX_NUM_OF_ZONE_INSTANCES (0)
#endif

#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)
void thermostat_init(uint8_t connector_id);
void handleRVCTHASTAT0(uint32_t dgn, DDMP2_FRAME *p_frame);
void handleRVC2TH0(uint32_t dgn, DDMP2_FRAME *p_frame);
void handleRVCTH0(uint32_t dgn, DDMP2_FRAME *p_frame);
void handleRVC2THSCHED0(uint32_t dgn, DDMP2_FRAME *p_frame);
void handleRVCTHSCHED0(uint32_t dgn, DDMP2_FRAME *p_frame);
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
bool receive130809Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130808Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130805Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130804Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit130972Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130810Dgn(uint8_t instance, uint8_t *p_data);
bool transmit131042Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130806Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130807Dgn(uint8_t instance, uint8_t *p_data);
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
bool receive131042Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130810Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130807Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130806Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool receive130972Dgn(uint8_t *p_data, uint8_t sa, size_t size);
bool transmit130808Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130809Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130804Dgn(uint8_t instance, uint8_t *p_data);
bool transmit130805Dgn(uint8_t instance, uint8_t *p_data);
#endif

#endif

#endif /* THERMOSTAT_H_ */
