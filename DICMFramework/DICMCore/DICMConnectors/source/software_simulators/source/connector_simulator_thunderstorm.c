/**
 * \file
 * \date        2024-12-24
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Thunderstorm device
 *
 * Enable this connector with CONNECTOR_SIMULATOR_THUNDERSTORM in configuration.
 *
 * \li          2024-12-24  (NR) Initial implementation
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

#include "connector_simulator_thunderstorm.h"

#if defined(CONNECTOR_SIMULATOR_THUNDERSTORM)

#include "software_simulator.h"
#include "software_simulator_configuration.h"

#if !defined(SIMULATOR_HMI)
#error "This connector requires an HMI DDM class simulator. Enable an HMI class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__thunderstorm_hmi_simulator;

static int initialize_hmi_connector(void)
{
	return software_simulator__init(&s__thunderstorm_hmi_simulator, &connector_simulator_thunderstorm_hmi, &SIMULATOR_HMI);
}

CONNECTOR connector_simulator_thunderstorm_hmi =
{
    .name = "Thunderstorm HMI simulator",
	.initialize = initialize_hmi_connector
};

#if !defined(SIMULATOR_SPIR)
#error "This connector requires an SPIR DDM class simulator. Enable an SPIR class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__thunderstorm_spir_simulator;

static int initialize_spir_connector(void)
{
	return software_simulator__init(&s__thunderstorm_spir_simulator, &connector_simulator_thunderstorm_spir, &SIMULATOR_SPIR);
}

CONNECTOR connector_simulator_thunderstorm_spir =
{
    .name = "Thunderstorm SPIR simulator",
	.initialize = initialize_spir_connector
};

#if !defined(SIMULATOR_RFAN)
#error "This connector requires an RFAN DDM class simulator. Enable an RFAN class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__thunderstorm_rfan_simulator;

static int initialize_rfan_connector(void)
{
	return software_simulator__init(&s__thunderstorm_rfan_simulator, &connector_simulator_thunderstorm_rfan, &SIMULATOR_RFAN);
}

CONNECTOR connector_simulator_thunderstorm_rfan =
{
    .name = "Thunderstorm RFAN simulator",
	.initialize = initialize_rfan_connector
};
#endif /* defined(CONNECTOR_SIMULATOR_CFX2) */
/** \} */
