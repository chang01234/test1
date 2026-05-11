/**
 * \file
 * \date        2022-07-05
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Configuration header file for software simulator
 *
 * This header contains common defines and macros for software simulators.
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

#ifndef SOFTWARE_SIMULATOR_DEFAULTS_H_
#define SOFTWARE_SIMULATOR_DEFAULTS_H_

#include "configuration.h"

#ifndef SOFTWARE_SIMULATOR_DEFAULTS__PRIO
#define SOFTWARE_SIMULATOR_DEFAULTS__PRIO       3
#endif

#ifndef SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK
#define SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK  2048
#endif

#endif /* SOFTWARE_SIMULATOR_DEFAULTS_H_ */