/**
 * \file        load_configurations_event_notification.c
 * \date        2025-06-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event manager load configurations.
 *
 * \li          2024-06-05  (NR) Initial implementation
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

#include "load_configurations_event_notification.h"
#include "configuration.h"
#include "load_configurations.h"

// Configurations
#include "load_configurations_event_notification_configuration.h"

const struct load_configurations__configuration event_notification_configurations[] = {
#if defined(LOAD_CONFIGURATIONS__EVENT_NOTIFICATION_CONFIGURATION)
    {
        .configuration_location_type = LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM,
        .static_config = &event_notification_config_spiffs_rules,
    }
#endif
};

const struct load_configurations__descriptor event_notification__descriptor = {
    .custom_configurations = event_notification_configurations,
    .custom_configurations_size = ELEMENTS(event_notification_configurations),
    .load_custom_configurations = connector_event_notification_load_configurations,
    .descriptor_name = "Event notification (Event types)",
};
