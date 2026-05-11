/**
 * \file
 * \date        2021-10-08
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (AP) Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \brief       Connector software simulator for ShapeX device implementation
 *
 * Implementation of Connector software simulator for ShapeX device.
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

#include "connector_simulator_shapex.h"

#if defined(CONNECTOR_SIMULATOR_SHAPEX)

#include "software_simulator.h"
#include "software_simulator_configuration.h"

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__shapex_ac_simulator;

static int initialize_ac_connector(void)
{
    return software_simulator__init(&s__shapex_ac_simulator, &connector_simulator_shapex_ac, &SIMULATOR_AC);
}

CONNECTOR connector_simulator_shapex_ac =
{
    .name = "SHAPEX AC simulator",
    .initialize = initialize_ac_connector
};

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__shapex_prod_simulator;

static int initialize_prod_connector(void)
{
    return software_simulator__init(&s__shapex_prod_simulator, &connector_simulator_shapex_prod, &SIMULATOR_PROD);
}

CONNECTOR connector_simulator_shapex_prod =
{
    .name = "SHAPEX PROD simulator",
    .initialize = initialize_prod_connector
};

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__shapex_dim_a_simulator;

static int initialize_dim_a_connector(void)
{
    return software_simulator__init(&s__shapex_dim_a_simulator, &connector_simulator_shapex_dim_a, &SIMULATOR_DIM_A);
}

CONNECTOR connector_simulator_shapex_dim_a =
{
    .name = "SHAPEX DIM A simulator",
    .initialize = initialize_dim_a_connector
};

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__shapex_dim_b_simulator;

static int initialize_dim_b_connector(void)
{
    return software_simulator__init(&s__shapex_dim_b_simulator, &connector_simulator_shapex_dim_b, &SIMULATOR_DIM_B);
}

CONNECTOR connector_simulator_shapex_dim_b =
{
    .name = "SHAPEX DIM B simulator",
    .initialize = initialize_dim_b_connector
};

static EXT_RAM_ATTR SOFTWARE_SIMULATOR s__shapex_hmi_simulator;

static int initialize_hmi_connector(void)
{
    return software_simulator__init(&s__shapex_hmi_simulator, &connector_simulator_shapex_hmi, &SIMULATOR_HMI);
}

CONNECTOR connector_simulator_shapex_hmi =
{
    .name = "SHAPEX HMI simulator",
    .initialize = initialize_hmi_connector
};

#endif /* defined(CONNECTOR_SIMULATOR_SHAPE) */
/** \} */
