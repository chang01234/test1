/**
 * \file
 * \date        2025-02-03
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Rule engine implementation
 *
 * Implementation of Connector software simulator for Rule engine.
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

#include "connector_simulator_rule.h"

#if defined(CONNECTOR_SIMULATOR_RULE)
#include "software_simulator.h"
#include "software_simulator_configuration.h"

#if !defined(SIMULATOR_RULE)
#error "This connector requires an RULE DDM class simulator. Enable an RULE class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__rule_simulator;

static int initialize_rule_connector(void)
{
	return software_simulator__init(&s__rule_simulator, &connector_simulator_rule, &SIMULATOR_RULE);
}

CONNECTOR connector_simulator_rule =
{
    .name = "RULE simulator",
    .initialize = initialize_rule_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_RULE_ENGINE) */
/** \} */
