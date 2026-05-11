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
 * This connector simulates an CFX2 product MCCC and PROD connectors. The
 * simulated parameters are specified in the document:
 * (sharepoint) `Team - Dometic Interact` into directory:
 * `Connectivity Module (DICM)/01_Technical_Specification/versions`
 * name: `Technical Specification DICM SHAPE Integration.docx`
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

#ifndef CONNECTOR_SIMULATOR_CFXX_H_
#define CONNECTOR_SIMULATOR_CFXX_H_

#include "configuration.h"
#include "connector.h"

#if defined(CONNECTOR_SIMULATOR_CFX2)

extern CONNECTOR connector_simulator_cfx2_mccc;

extern CONNECTOR connector_simulator_cfx2_prod;

extern CONNECTOR connector_simulator_cfx2_hmi;

#endif /* CONNECTOR_SIMULATOR_CFX2 */
#endif /* CONNECTOR_SIMULATOR_CFXX_H_ */