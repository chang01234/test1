/**
 * \file        load_configurations_rule_engine.c
 * \date        2022-09-27
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Implementation of Rule Engine's rules loading functionality.
 *
 * For more details please refer to the header file.
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

#include "configuration.h"
#include "load_configurations.h"
#include "load_configurations_fc_rule_engine.h"
#include "connector_fc_rule_engine.h"

#include "cJSON.h"
#include "esp_heap_caps.h"

//#define LOAD_CONFIGURATION_PRINT_JSON // uncomment to print all rules

// Configurations
#include "load_configurations_fc_rule_engine_configuration.h"


const struct load_configurations__configuration fc_configurations[] =
{
#if defined(LOAD_CONFIGURATIONS__FC_RULE_ENGINE_CONFIGURATION)
    {
        .configuration_location_type = LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM,
        .static_config = &config_spiffs_rules,
    }
#endif
};

const struct load_configurations__descriptor fc_rule_engine__descriptor =
{
    .custom_configurations = fc_configurations,
    .custom_configurations_size = ELEMENTS(fc_configurations),
    .load_custom_configurations = connector_fc_rule_engine_load_configurations,
    .descriptor_name = "Full Climate(Rule Engine)",
};
