/**
 * \file
 * \date        2022-07-07
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       State structures for Full Climate system simulation
 *
 * This file contains structures and functions for holding system state when simulating Full Climate
 * system.
 *
 * \li          2021-07-07  (NR) Initial implementation
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

#ifndef FULL_CLIMATE_STATE_H_
#define FULL_CLIMATE_STATE_H_

#include <stdint.h>
#include <stdbool.h>

#define FULL_CLIMATE_STATE__INITIAL_INTERNAL_TEMPERATURE    21000
#define FULL_CLIMATE_STATE__OUTSIDE_TEMPERATURE             21000

typedef struct full_climate_state
{
    /* */
    int32_t ac_temp_increment;
    int32_t htr_temp_increment;
    int32_t iv_temp_increment;
    int32_t ambient_temperature;
} FULL_CLIMATE_STATE;

extern FULL_CLIMATE_STATE g__full_climate_state;

#endif /* FULL_CLIMATE_STATE_H_ */