/**
 * \file        load_configurations_ddm_log.c
 *
 *              All rights reserved
 */

#include "configuration.h"
#include "load_configurations.h"
#include "load_configurations_ddm_log.h"
#include "connector_ddm_log.h"

// Configurations
#include "load_configurations_ddm_log_configuration.h"

const struct load_configurations__configuration ddm_log_configurations[] =
{
#if defined(LOAD_CONFIGURATIONS__DDM_LOG_CONFIGURATION)
    {
        .configuration_location_type = LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM,
        .static_config = &ddm_log_config_spiffs_rules,
    },
#endif
};

const struct load_configurations__descriptor ddm_log__descriptor =
{
    .custom_configurations = ddm_log_configurations,
    .custom_configurations_size = ELEMENTS(ddm_log_configurations),
    .load_custom_configurations = connector_ddm_log_load_configurations,
    .descriptor_name = "DDM Event Log",
};
