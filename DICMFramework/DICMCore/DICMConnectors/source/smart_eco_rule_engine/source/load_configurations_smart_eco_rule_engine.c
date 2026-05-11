/**
 * \file        load_configurations_rule_engine.c
 * \date        2024-02-07
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Implementation of Rule Engine's rules loading functionality.
 *
 * For more details please refer to the header file.
 * 
 * \li          2024-02-07  (BB) Initial implementation
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
#include "load_configurations_smart_eco_rule_engine.h"
#include "connector_smart_eco_rule_engine.h"

#include "cJSON.h"
#include "esp_heap_caps.h"

//#define LOAD_CONFIGURATION_PRINT_JSON // uncomment to print all rules

// Configurations
#include "load_configurations_smart_eco_cfg.h"


const struct load_configurations__configuration smart_eco_configurations[] =
{
#if defined(LOAD_CONFIGURATIONS__SMART_ECO_RULE_ENGINE_CONFIGURATION)
    {
        .configuration_location_type = LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM,
        .static_config = &config_spiffs_rules,
    }
#endif
};

const struct load_configurations__descriptor smart_eco_rule_engine__descriptor =
{
    .custom_configurations = smart_eco_configurations,
    .custom_configurations_size = ELEMENTS(smart_eco_configurations),
    .load_custom_configurations = NULL,
    .descriptor_name = "Smart Eco (Rule Engine)",
};
