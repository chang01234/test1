/**
 * \file        load_configurations_smart_eco_rule_engine_configuration.c
 * \date        2024-02-07
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

#ifndef LOAD_CONFIGURATIONS__SMART_ECO_RULE_ENGINE_SPIFFS_FILE_NAME
const char configuration_spiffs_file_name[] = "/spiffs/rules/smart_eco.json";
#else
const char configuration_spiffs_file_name[] = LOAD_CONFIGURATIONS__SMART_ECO_RULE_ENGINE_SPIFFS_FILE_NAME;
#endif
const struct load_configurations__static_configuration config_spiffs_rules =
{
    .configuration = configuration_spiffs_file_name,
    .configuration_size = ELEMENTS(configuration_spiffs_file_name),
    .configuration_name = "Smart Eco (Rule engine) configuration SPIFFS json file",
};
