/**
 * \file
 * \date        2022-07-06
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Inventilate device implementation
 *
 * Implementation of Connector software simulator for Inventilate device.
 *
 * \li          2022-07-06  (NR) Initial implementation
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

#include "connector_simulator_inventilate.h"

#if defined(CONNECTOR_SIMULATOR_INVENTILATE)

#include "software_simulator.h"
#include "software_simulator_configuration.h"

#if !defined(SIMULATOR_IV)
#error "This connector requires an IV DDM class simulator. Enable an IV class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__inventilate_simulator;

static int initialize_connector(void)
{
	return software_simulator__init(&s__inventilate_simulator, &connector_simulator_inventilate, &SIMULATOR_IV);
}

CONNECTOR connector_simulator_inventilate =
{
    .name = "INVENTILATE simulator",
	.initialize = initialize_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_INVENTILATE) */
/** \} */
