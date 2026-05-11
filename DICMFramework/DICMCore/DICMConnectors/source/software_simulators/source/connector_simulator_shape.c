/**
 * \file
 * \date        2021-10-08
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (AP) Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \brief       Connector software simulator for Shape device implementation
 *
 * Implementation of Connector software simulator for Shape device.
 *
 * \li          2021-10-08  (NR) Initial implementation
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

#include "connector_simulator_shape.h"

#if defined(CONNECTOR_SIMULATOR_SHAPE)

#include "software_simulator.h"
#include "software_simulator_configuration.h"

#if !defined(SIMULATOR_AC)
#error "This connector requires an AC DDM class simulator. Enable an AC class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__shape_simulator;

static int initialize_connector(void)
{
	return software_simulator__init(&s__shape_simulator, &connector_simulator_shape, &SIMULATOR_AC);
}

CONNECTOR connector_simulator_shape =
{
    .name = "SHAPE simulator",
	.initialize = initialize_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_SHAPE) */
/** \} */
