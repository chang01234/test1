/**
 * \file        load_configurations_rule_engine.h
 * \date        2022-09-27
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Implementation of Rule Engine's rules loading functionality.
 * 
 * This implementation will load all the desired rules stated in
 * load_configurations_rule_engine_xxx.h header files, by setting
 * the correct macros in configuration.h.
 * 
 * Selecting the Rule Engine descriptor for execution is done by defining
 * specifc macro in this header file: LOAD_CONFIGURATIONS__DESCRIPTOR_FC_RULE_ENGINE
 * 
 * Example:
 *  configuration.h
 *      #define LOAD_CONFIGURATIONS
 *      #define LOAD_CONFIGURATIONS__FC_RULE_ENGINE_CONFIGURATION
 * 
 *  load_configurations_rule_engine.c
 *      #include "load_configurations_rule_engine_configuration_matrix.h"
 *
 * \li          2022-09-27  (BB) Initial implementation
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

#ifndef LOAD_CONFIGURATIONS__FC_RULE_ENGINE_H
#define LOAD_CONFIGURATIONS__FC_RULE_ENGINE_H

#include "load_configurations.h"

#define LOAD_CONFIGURATIONS__DESCRIPTOR_FC_RULE_ENGINE
#define LOAD_CONFIGURATIONS__DESCRIPTOR_FC_RULE_ENGINE_NAME fc_rule_engine__descriptor

extern const struct load_configurations__descriptor fc_rule_engine__descriptor;
int connector_fc_rule_engine_load_configurations(const struct load_configurations__configuration *config);

#endif //LOAD_CONFIGURATIONS__FC_RULE_ENGINE_H
