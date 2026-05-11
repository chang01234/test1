/**
 * \file
 * \date        2025-03-25
 * \author      Andreas Lundeen
 * \brief       Connector software simulator for Battery MBAT device
 *
 * Enable this connector with CONNECTOR_SIMULATOR_PROD_MBAT in configuration.
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

#include "connector_simulator_prod_mbat.h"

#if defined(CONNECTOR_SIMULATOR_PROD_MBAT)

#include "software_simulator.h"
#include "software_simulator_configuration.h"
#include "product_database.h"
#if !defined(SIMULATOR_MBAT)
#error "This connector requires an MBAT DDM class simulator. Enable an MBAT class simulator; refer to software_simulator_configuration.h"
#endif

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__shape_mbat;

static int initialize_mbat_connector(void)
{
    return software_simulator__init(&s__shape_mbat, &connector_simulator_mbat, &SIMULATOR_MBAT);
}

CONNECTOR connector_simulator_mbat =
{
    .name = "MBAT simulator",
    .initialize = initialize_mbat_connector
};

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__mbat_prod_simulator;

static int initialize_prod_connector(void)
{
    ProdDBInit();
    return software_simulator__init(&s__mbat_prod_simulator, &connector_simulator_mbat_prod, &SIMULATOR_PROD);
}

CONNECTOR connector_simulator_mbat_prod =
{
    .name = "SHAPE-MBAT PROD simulator",
    .initialize = initialize_prod_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_PROD_MBAT) */
/** \} */
