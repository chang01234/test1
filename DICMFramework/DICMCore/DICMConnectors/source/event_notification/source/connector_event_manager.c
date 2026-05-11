/**
 * \file        connector_event_notification.c
 * \date        2024-06-27
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Notification connector implementation
 *
 * Implementation of Event Notification connector.
 *
 * \li          2024-06-27  (NR) Initial implementation
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

#include <string.h>
#include <sys/stat.h>

#include "configuration.h"
#include "connector_event_manager.h"
#include "connector_event_notification_defaults.h"
#include "event_manager.h"
#include "load_configurations_event_notification.h"

static int connector_event_manager_init(void);
static void connector_event_manager_process_task(const DDMP2_FRAME *frame);

/**
 * @brief       The event manager context structure instance
 *
 * The connector needs one instance of event_manager structure to hold all
 * relevant data needed for operation.
 */
static event_manager_t event_manager;

/* Sorted container that stores our owned EVM and EVN DDM parameters */
SORTED_CONTAINER__DECLARE_EXTRAM(
    event_manager_owned_ddm_store_storage,
    EVENT_MANAGER_OWNED_DDM_COUNT,
    ddm_entry_t);

/* Inventory used when subscribing to other DDM parameters */
DECLARE_SORTED_LIST_EXTRAM(
    event_manager_inventory_handler_storage,
    CONNECTOR_EVENT_NOTIFICATION_MAX_DDM_TO_SUBSCRIBE);

/* Sorted container that store other DDM parameters */
SORTED_CONTAINER__DECLARE_EXTRAM(
    event_manager_other_ddm_store_storage,
    CONNECTOR_EVENT_NOTIFICATION_MAX_DDM_TO_SUBSCRIBE,
    ddm_entry_t);

/* Storage of event_type structures for event_type database */
SORTED_CONTAINER__DECLARE_EXTRAM(
    event_manager_event_type_db_storage,
    CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_TYPES,
    event_type_t);

/* Storage of event_record structures for event_record database */
SORTED_CONTAINER__DECLARE_EXTRAM(
    event_manager_event_record_db_storage,
    CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS,
    event_record_t);

/* Storage of event_record identifiers for event_record database */
static event_id_t event_manager_event_record_db_fifo_storage[CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS];

static int connector_event_manager_init(void)
{
    static const event_manager_description_t description = {
        .p_connector = &connector_event_manager,
        .p_event_record_db_fifo_storage = &event_manager_event_record_db_fifo_storage[0],
        .p_event_record_db_storage = &event_manager_event_record_db_storage,
        .p_event_type_db_storage = &event_manager_event_type_db_storage,
        .p_inventory_handler_storage = &event_manager_inventory_handler_storage,
        .p_other_ddm_store_storage = &event_manager_other_ddm_store_storage,
        .p_owned_ddm_store_storage = &event_manager_owned_ddm_store_storage,
    };
    return event_manager_connector_init(&event_manager, &description) == EVENT_MANAGER_NO_ERROR ? CONNECTOR_INIT_SUCCESS : CONNECTOR_INIT_FAILURE;
}

static void connector_event_manager_process_task(const DDMP2_FRAME *frame)
{
    event_manager_conector_process_task(&event_manager, frame);
}

int connector_event_notification_load_configurations(const struct load_configurations__configuration *config)
{
    int load_status = 1;  // By default we return load failure
    switch (config->configuration_location_type)
    {
    case LOAD_CONFIGURATION__LOCATION_ROM:
        LOG(E, "Not supported LOAD_CONFIGURATION__LOCATION_ROM");
        break;
    case LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM:
    {
        // Load json file from spiffs. Filename is the config
        struct stat st;
        if (stat(config->static_config->configuration, &st) != 0)
        {
            LOG(W, "Could not find configuration file: %s", (char *)config->static_config->configuration);
        }
        else
        {
            LOG(I, "Trying to load configuration file: %s (%ld bytes)", (char *)config->static_config->configuration, st.st_size);
            FILE *f = fopen((char *)config->static_config->configuration, "r");
            if (f == NULL)
            {
                LOG(W, "Could not open configuration file: %s", (char *)config->static_config->configuration);
            }
            else
            {
                /* NOTE: During buffer allocation add one more byte for NULL terminating byte
                 */
                char *buffer = hal_mem_malloc_prefer(st.st_size + 1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                TRUE_CHECK_RETURNX(1, buffer != NULL);
                size_t read_bytes = fread(buffer, 1, st.st_size, f);
                fclose(f);
                /* NULL terminate the buffer in case it is not NULL terminated (taken into account during buffer allocation) */
                buffer[read_bytes] = '\0';
                int configuration_status = event_manager_load_configuration(&event_manager, buffer);
                hal_mem_free(buffer);
                load_status = configuration_status == EVENT_MANAGER_NO_ERROR ? 0 : 1;
            }
        }
        break;
    }
    default:
        break;
    }
    return load_status;
}

CONNECTOR connector_event_manager = {
    .name = "Event Manager connector",
    .initialize = connector_event_manager_init,
    .process_event = connector_event_manager_process_task,
};
