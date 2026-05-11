/**
 * \file
 * \date        2025-03-25
 * \author      Andreas Lundeen
 * \brief       Connector software simulator for SHAPE with MBAT device
 *
 * This connector(s) simulate product with MBAT device, just enable it with:
 * CONNECTOR_SIMULATOR_PROD_MBAT.
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

#ifndef CONNECTOR_SIMULATOR_PROD_MBAT_H_
#define CONNECTOR_SIMULATOR_PROD_MBAT_H_

#include "configuration.h"
#include "connector.h"

#if defined(CONNECTOR_SIMULATOR_PROD_MBAT)
extern CONNECTOR connector_simulator_mbat;
extern CONNECTOR connector_simulator_mbat_prod;
#endif /* defined(CONNECTOR_SIMULATOR_PROD_MBAT) */

#endif /* CONNECTOR_SIMULATOR_PROD_MBAT_H_ */