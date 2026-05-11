/**
 * \file        load_configurations_supervisor.c
 *
 *              All rights reserved
 */

#include "configuration.h"
#include "load_configurations.h"
#include "load_configurations_supervisor.h"
#include "connector_supervisor.h"

// Configurations
#include "load_configurations_supervisor_configuration.h"


const struct load_configurations__configuration supervisor_configurations[] =
{
#if defined(LOAD_CONFIGURATIONS__SUPERVISOR_RULE_ENGINE_CONFIGURATION)
    {
        .configuration_location_type = LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM,
        .static_config = &supervisor_config_spiffs_rules,
    }
#endif
};

const struct load_configurations__descriptor supervisor__descriptor =
{
    .custom_configurations = supervisor_configurations,
    .custom_configurations_size = ELEMENTS(supervisor_configurations),
    .load_custom_configurations = connector_supervisor_load_configurations,
    .descriptor_name = "Supervisor (Rule Engine)",
};
