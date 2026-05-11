/**
 * \file
 * \date        2022-07-05
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       State structures for Full Climate system simulation
 *
 * Implementation of state structures for Full Climate system simulation.
 *
 * \li          2022-07-05  (NR) Initial implementation
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
#include "configuration.h"

#if defined(SOFTWARE_SIMULATOR_FULL_CLIMATE)
#include "full_climate_state.h"

FULL_CLIMATE_STATE g__full_climate_state =
{
    .ambient_temperature = FULL_CLIMATE_STATE__INITIAL_INTERNAL_TEMPERATURE
};
#endif
