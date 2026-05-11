/**
 * \file
 * \date        2022-10-08
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (AP) Anatolie Pocropivnii (anatolie.pocropivnii@seavus.com)
 * \brief       Connector software simulator for ShapeX device
 *
 * Enable this connector with CONNECTOR_SIMULATOR_SHAPEX in configuration.
 *
 * This connector simulates an SHAPEX clases: AC, PROD and DIM
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

#ifndef CONNECTOR_SIMULATOR_SHAPEX_H_
#define CONNECTOR_SIMULATOR_SHAPEX_H_

#include "configuration.h"

#if defined(CONNECTOR_SIMULATOR_SHAPEX)

#include "connector.h"

extern CONNECTOR connector_simulator_shapex_ac;
extern CONNECTOR connector_simulator_shapex_prod;
extern CONNECTOR connector_simulator_shapex_dim_a;
extern CONNECTOR connector_simulator_shapex_dim_b;
extern CONNECTOR connector_simulator_shapex_hmi;

#endif /* CONNECTOR_SIMULATOR_SHAPEX */
#endif /* CONNECTOR_SIMULATOR_SHAPEX_H_ */