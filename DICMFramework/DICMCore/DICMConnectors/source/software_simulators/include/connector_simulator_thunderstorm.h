/**
 * \file
 * \date        2024-12-24
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Thunderstorm device
 *
 * This connector(s) simulate THUNDERSTORM device, just enable it with:
 * CONNECTOR_SIMULATOR_THUNDERSTORM.
 *
 * \li          2024-12-24  (AP) Initial implementation
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

#ifndef CONNECTOR_SIMULATOR_THUNDERSTORM_H_
#define CONNECTOR_SIMULATOR_THUNDERSTORM_H_

#include "configuration.h"
#include "connector.h"

#if defined(CONNECTOR_SIMULATOR_THUNDERSTORM)
extern CONNECTOR connector_simulator_thunderstorm_hmi;
extern CONNECTOR connector_simulator_thunderstorm_spir;
extern CONNECTOR connector_simulator_thunderstorm_rfan;
#endif /* defined(CONNECTOR_SIMULATOR_THUNDERSTORM) */

#endif /* CONNECTOR_SIMULATOR_THUNDERSTORM_H_ */