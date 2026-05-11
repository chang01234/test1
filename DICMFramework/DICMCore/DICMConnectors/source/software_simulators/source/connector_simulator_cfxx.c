/**
 * \file
 * \date        2023-10-25
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (AP) Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \brief       Connector software simulator for CFX2 device
 *
 * Enable this connector with CONNECTOR_SIMULATOR_CFX2 in configuration.
 *
 * \li          2021-10-25  (NR) Initial implementation
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

#include "connector_simulator_cfxx.h"

#if defined(CONNECTOR_SIMULATOR_CFX2)

#include "software_simulator.h"
#include "software_simulator_configuration.h"

#if !defined(SIMULATOR_MCCC)
#error "This connector requires an MCCC DDM class simulator. Enable an MCCC class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__cfx2_mccc_simulator;

static int initialize_mccc_connector(void)
{
	return software_simulator__init(&s__cfx2_mccc_simulator, &connector_simulator_cfx2_mccc, &SIMULATOR_MCCC);
}

CONNECTOR connector_simulator_cfx2_mccc =
{
    .name = "CFX2 MCCC simulator",
	.initialize = initialize_mccc_connector
};

#if !defined(SIMULATOR_PROD)
#error "This connector requires an PROD DDM class simulator. Enable an PROD class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__cfx2_prod_simulator;

static int initialize_prod_connector(void)
{
	return software_simulator__init(&s__cfx2_prod_simulator, &connector_simulator_cfx2_prod, &SIMULATOR_PROD);
}

CONNECTOR connector_simulator_cfx2_prod =
{
    .name = "CFX2 PROD simulator",
	.initialize = initialize_prod_connector
};

#if !defined(SIMULATOR_HMI)
#error "This connector requires an HMI DDM class simulator. Enable an HMI class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__cfx2_hmi_simulator;

static int initialize_hmi_connector(void)
{
	return software_simulator__init(&s__cfx2_hmi_simulator, &connector_simulator_cfx2_hmi, &SIMULATOR_HMI);
}

CONNECTOR connector_simulator_cfx2_hmi =
{
    .name = "CFX2 HMI simulator",
	.initialize = initialize_hmi_connector
};
#endif /* defined(CONNECTOR_SIMULATOR_CFX2) */
/** \} */
