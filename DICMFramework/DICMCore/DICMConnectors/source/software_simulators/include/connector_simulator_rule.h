/**
 * \file
 * \date        2025-02-03
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Rule engine
 *
 * Enable this connector with CONNECTOR_SIMULATOR_RULE in configuration.
 *
 * \li          2025-02-03  (NR) Initial implementation
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

#ifndef CONNECTOR_SIMULATOR_RULE_H_
#define CONNECTOR_SIMULATOR_RULE_H_

#include "configuration.h"

#if defined(CONNECTOR_SIMULATOR_RULE)

#include "connector.h"

extern CONNECTOR connector_simulator_rule;

#endif /* CONNECTOR_SIMULATOR_RULE */
#endif /* CONNECTOR_SIMULATOR_RULE_H_ */