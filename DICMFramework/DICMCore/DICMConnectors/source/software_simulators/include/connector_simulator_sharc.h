/**
 * \file
 * \date        2022-06-08
 * \author      Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \authors     (AP) Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Shape device
 *
 * This connector(s) simulate Sharc device. The Sharc supports different class so choose to your
 * needs accordingly:
 *  CONNECTOR_SIMULATOR_SHARC_HEATER - HTR class
 *  CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL  - CC class
 *  CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER - CCS class
 *
 * Or just enable all with:
 *  CONNECTOR_SIMULATOR_SHARC - HTR, CC, CCS classes
 *
 * \li          2022-06-08  (AP) Initial implementation
 * \li          2022-07-05  (NR) Refactored code
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef CONNECTOR_SIMULATOR_SHARC_H_
#define CONNECTOR_SIMULATOR_SHARC_H_

#include "configuration.h"
#include "connector.h"

#if defined(CONNECTOR_SIMULATOR_SHARC)
#define CONNECTOR_SIMULATOR_SHARC_HEATER
#define CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL
#define CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER
#endif

#if defined(CONNECTOR_SIMULATOR_SHARC_HEATER)
extern CONNECTOR connector_simulator_sharc_heater;
#endif /* defined(CONNECTOR_SIMULATOR_SHARC_HEATER) */

#if defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL)
extern CONNECTOR connector_simulator_sharc_climate_control;
#endif /* defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL) */

#if defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER)
extern CONNECTOR connector_simulator_sharc_climate_control_scheduler;
#endif /* defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER) */

#endif /* CONNECTOR_SIMULATOR_SHARC_H_ */