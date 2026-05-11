/**
 * \file
 * \date        2022-07-05
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Configuration header file for software simulator
 *
 * This header contains common defines and macros for software simulators.
 *
 * \li          2022-07-05  (NR) Initial implementation
 * \li          2024-12-24  (NR) Added Thunderstorm support
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

#ifndef SOFTWARE_SIMULATOR_CONFIGURATION_H_
#define SOFTWARE_SIMULATOR_CONFIGURATION_H_

#include "configuration.h"

/*
 * NOTE:
 *
 * Enable only one of the following macros in your project configuration header file.
 */

#if defined(SOFTWARE_SIMULATOR_SHAPE)
#include "simulator_ac_none.h"
#elif defined(SOFTWARE_SIMULATOR_SHARC)
#include "simulator_htr_none.h"
#include "simulator_ccs_none.h"
#include "simulator_cc_none.h"
#elif defined(SOFTWARE_SIMUALATOR_INVENTILATE)
#include "simulator_iv_none.h"
#elif defined(SOFTWARE_SIMULATOR_FULL_CLIMATE)
#include "simulator_ac_full_climate.h"
#include "simulator_htr_full_climate.h"
#include "simulator_iv_full_climate.h"
#include "simulator_cc_full_climate.h"
#include "simulator_ccs_full_climate.h"
#elif defined(SOFTWARE_SIMULATOR_CFX2)
#include "simulator_mccc_cfxx.h"
#include "simulator_prod_cfxx.h"
#include "simulator_hmi_cfxx.h"
#elif defined(SOFTWARE_SIMULATOR_SHAPEX)
#include "simulator_ac_shapex.h"
#include "simulator_prod_shapex.h"
#include "simulator_dim_shapex.h"
#include "simulator_hmi_cfxx.h"
#elif defined(SOFTWARE_SIMULATOR_EVENT_NOTIFICATION)
#include "simulator_ac_event_notification.h"
#include "simulator_rule_event_notification.h"
#elif defined(SOFTWARE_SIMULATOR_THUNDERSTORM)
#include "simulator_spir_none.h"
#include "simulator_hmi_none.h"
#include "simulator_rfan_none.h"
#endif
#if defined(SOFTWARE_SIMULATOR_PROD_MBAT)
#include "simulator_mbat.h"
#include "simulator_prod_mbat.h"
#endif

#endif /* SOFTWARE_SIMULATOR_CONFIGURATION_H_ */