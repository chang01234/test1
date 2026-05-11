/**
 * \file        load_configurations_supervisor.c
 */

#include "rule_engine.h"
#include "load_configurations.h"

/**********************************************************
 * Configuration Matrix
 *********************************************************/

#ifndef LOAD_CONFIGURATIONS__SUPERVISOR_RULE_ENGINE_SPIFFS_FILE_NAME
static const char configuration_spiffs_file_name[] = "/spiffs/rules/supervisor.json";
#else
static const char configuration_spiffs_file_name[] = LOAD_CONFIGURATIONS__SUPERVISOR_RULE_ENGINE_SPIFFS_FILE_NAME;
#endif
const struct load_configurations__static_configuration supervisor_config_spiffs_rules =
{
    .configuration = configuration_spiffs_file_name,
    .configuration_size = ELEMENTS(configuration_spiffs_file_name),
    .configuration_name = "Supervisor (Rule engine) configuration SPIFFS json file",
};
