/**
 * \file        load_configurations_event_notification_configuration.c
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

#include "load_configurations_event_notification_configuration.h"

/**********************************************************
 * Configuration Matrix
 *********************************************************/

#ifndef LOAD_CONFIGURATIONS__EVENT_NOTIFICATION_SPIFFS_FILE_NAME
static const char configuration_spiffs_file_name[] = "/spiffs/event_types/event_types.json";
#else
static const char configuration_spiffs_file_name[] = LOAD_CONFIGURATIONS__EVENT_NOTIFICATION_SPIFFS_FILE_NAME;
#endif

const struct load_configurations__static_configuration event_notification_config_spiffs_rules = {
    .configuration = configuration_spiffs_file_name,
    .configuration_size = ELEMENTS(configuration_spiffs_file_name),
    .configuration_name = "Event types definition SPIFFS json file",
};
