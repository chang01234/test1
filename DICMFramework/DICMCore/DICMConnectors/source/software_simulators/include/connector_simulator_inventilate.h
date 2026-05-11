/**
 * \file
 * \date        2022-07-06
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Inventilate device
 *
 * Enable this connector with CONNECTOR_SIMULATOR_INVENTILATE in configuration.
 *
 * \li          2021-07-06  (NR) Initial implementation
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

#ifndef CONNECTOR_SIMULATOR_INVENTILATE_H_
#define CONNECTOR_SIMULATOR_INVENTILATE_H_

#include "configuration.h"

#if defined(CONNECTOR_SIMULATOR_INVENTILATE)

#include "connector.h"

extern CONNECTOR connector_simulator_inventilate;

#endif /* CONNECTOR_SIMULATOR_INVENTILATE */
#endif /* CONNECTOR_SIMULATOR_INVENTILATE_H_ */