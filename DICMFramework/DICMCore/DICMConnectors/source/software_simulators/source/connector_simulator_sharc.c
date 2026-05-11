/**
 * \file
 * \date        2022-06-08
 * \author      Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \authors     (AP) Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Connector software simulator for Sharc device implementation
 *
 * Implementation of Connector software simulator for Sharc device.
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

#include "connector_simulator_sharc.h"

#if defined(CONNECTOR_SIMULATOR_SHARC_HEATER)                    || \
	defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL)           || \
	defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER) || \
	defined(__DOXYGEN__)

#include "software_simulator.h"
#include "software_simulator_configuration.h"

#if defined(CONNECTOR_SIMULATOR_SHARC_HEATER) || defined(__DOXYGEN__)

/*
 * This code section simulates HTR (Heater) DDM parameter class
 */

#if !defined(SIMULATOR_HTR)
#error "This connector requires a HTR DDM class simulator. Enable a HTR class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__sharc_heater_simulator;

static int initialize_heater_connector(void)
{
	return software_simulator__init(&s__sharc_heater_simulator, &connector_simulator_sharc_heater, &SIMULATOR_HTR);
}

CONNECTOR connector_simulator_sharc_heater=
{
	.name = "SHARC simulator Heater",
	.initialize = initialize_heater_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_SHARC_HEATER) || defined(__DOXYGEN__) */

#if defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL) || defined(__DOXYGEN__)

/*
 * This code section simulates CC (Climate Control) DDM parameter class
 */

#if !defined(SIMULATOR_CC)
#error "This connector requires a CC DDM class simulator. Enable a CC class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__sharc_climate_control_simulator;

static int initialize_climate_control_connector(void)
{
	return software_simulator__init(&s__sharc_climate_control_simulator,
		&connector_simulator_sharc_climate_control,
		&SIMULATOR_CC);
}

CONNECTOR connector_simulator_sharc_climate_control=
{
	.name = "SHARC simulator Climate Control",
	.initialize = initialize_climate_control_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL) || defined(__DOXYGEN__) */

#if defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER) || defined(__DOXYGEN__)

/*
 * This code section simulates CCS (Climate Control Scheduler) DDM parameter class
 */

#if !defined(SIMULATOR_CCS)
#error "This connector requires a CCS DDM class simulator. Enable a CCS class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__sharc_climate_control_scheduler_simulator;

static int initialize_climate_control_scheduler_connector(void)
{
	return software_simulator__init(&s__sharc_climate_control_scheduler_simulator,
		&connector_simulator_sharc_climate_control_scheduler,
		&SIMULATOR_CCS);
}

CONNECTOR connector_simulator_sharc_climate_control_scheduler=
{
	.name = "SHARC simulator Climate Control Scheduler",
	.initialize = initialize_climate_control_scheduler_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER) || defined(__DOXYGEN__) */
#endif /* defined(CONNECTOR_SIMULATOR_SHARC_HEATER) || defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL) || defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER) || defined(__DOXYGEN__) */
