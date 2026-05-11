/**
 * \file
 * \date        2023-10-25
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for DIM DDM2 class - variant SHAPEX implementation
 *
 * Implementation of tables to simulate DIM DDM2 class - variant `SHAPEX`
 *
 * \li          2022-10-25  (NR) Initial implementation
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

#ifndef SIMULATOR_DIM_SHAPEX_H_
#define SIMULATOR_DIM_SHAPEX_H_

#include "software_simulator.h"

#define SIMULATOR_DIM_A                      g__class_dim_a_shapex_descriptor
#define SIMULATOR_DIM_B                      g__class_dim_b_shapex_descriptor

extern const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_dim_a_shapex_descriptor;
extern const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_dim_b_shapex_descriptor;

#endif /* SIMULATOR_DIM_SHAPEX_H_ */