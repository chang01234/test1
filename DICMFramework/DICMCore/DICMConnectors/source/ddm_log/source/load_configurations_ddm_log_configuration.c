/**
 * \file        load_configurations_ddm_log_configuration.c
 */

#include "load_configurations.h"

/**********************************************************
 * Configuration Matrix
 *********************************************************/

#ifndef LOAD_CONFIGURATIONS__DDM_LOG_SPIFFS_FILE_NAME
static const char configuration_spiffs_file_name[] = "/spiffs/logcfgs/logconfigs.json";
#else
static const char configuration_spiffs_file_name[] = LOAD_CONFIGURATIONS__DDM_LOG_SPIFFS_FILE_NAME;
#endif
const struct load_configurations__static_configuration ddm_log_config_spiffs_rules =
{
    .configuration = configuration_spiffs_file_name,
    .configuration_size = ELEMENTS(configuration_spiffs_file_name),
    .configuration_name = "DDM Event Log configuration SPIFFS json file",
};
