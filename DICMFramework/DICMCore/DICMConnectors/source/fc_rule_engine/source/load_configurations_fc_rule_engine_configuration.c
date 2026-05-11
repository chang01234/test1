/**
 * \file        load_configurations_fc_rule_engine_configuration.c
 * \date        2022-09-27
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Rule Engine configuration rules definition.
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

#include "rule_engine.h"
#include "load_configurations.h"

/**********************************************************
 * Configuration Matrix
 *********************************************************/

#ifndef LOAD_CONFIGURATIONS__FC_RULE_ENGINE_SPIFFS_FILE_NAME
const char configuration_spiffs_file_name[] = "/spiffs/rules/rule_engine.json";
#else
const char configuration_spiffs_file_name[] = LOAD_CONFIGURATIONS__FC_RULE_ENGINE_SPIFFS_FILE_NAME;
#endif
const struct load_configurations__static_configuration config_spiffs_rules =
{
    .configuration = configuration_spiffs_file_name,
    .configuration_size = ELEMENTS(configuration_spiffs_file_name),
    .configuration_name = "Full climate (Rule engine) configuration SPIFFS json file",
};
