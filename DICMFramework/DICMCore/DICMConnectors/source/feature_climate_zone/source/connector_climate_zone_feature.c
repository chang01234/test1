/*! \file
 *  \brief Climate zone feature service
 *
 *  Service is responsible for managing climates zone feature and its groupX classes. The service will be able
 *  to autodetect "climate" products and map to climate zones.
 *  Service should be rewritten when service database library exists and product databasee library exists
 */

#include <stdint.h>
#include <string.h>
#include <sys/queue.h>

#include "configuration.h"

#include "broker.h"
#include "connector.h"
#include "connector_climate_zone_feature.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "feature_database.h"
#include "freertos/FreeRTOS.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#include "inventory_handler.h"
#include "product_database.h"

#define INVENTORY_HANDLER_STORAGE_SIZE 3

static int initialize_connector(void);
static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available);
static void connector_process_task(void *Parameter);

DECLARE_SORTED_LIST_EXTRAM(l_inventory_list, INVENTORY_HANDLER_STORAGE_SIZE);

static EXT_RAM_ATTR inventory_handler_t l_ih;

// TODO: This shoud be taken from configuration file
static bool MAYBE_UNUSED is_auto_detect_active = true;

CONNECTOR connector_climate_zone_feature = {
    .name = "Climate zone feature",      // connector name
    .initialize = initialize_connector,  // connector initialize function
};

static int initialize_connector(void)  // initialize connector
{
    // Init inventory handler
    inventory_handler_init(&l_ih, &l_inventory_list, inventory_handler_cb, NULL);
    int ret = 1;
    int err = -1;
    err = feat_db_init();
    if (err == ESP_OK)
    {
        feat_db_load_cache(GROUP0TYPE_CLIMATEZONE, connector_climate_zone_feature.connector_id);
        int32_t class_instance = -1;
        class_instance = feat_db_find_first_by_id_and_type(0, GROUP0TYPE_CLIMATEZONE);
        if (class_instance == -1)
        {
            err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_climate_zone_feature.connector_id, 0, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
            if ((err == ESP_OK) && (class_instance > -1))
            {
                LOG(D, "Climate zone class instance %d", class_instance);
            }
            else if (err != ESP_OK)
            {
                LOG(E, "Error %d", err);
            }
        }
        TRUE_CHECK(xTaskCreate(connector_process_task, connector_climate_zone_feature.name, 3584, 0, xTASK_PRIORITY_NORMAL, NULL));
    }
    else
    {
        LOG(E, "Feature database init returned error %d", err);
        ret = 0;
    }

    return ret;
}

static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available)
{
    // Detect if any new PRODs are created
    // If so, subscribe to its params to get its interface
    if (is_available)
    {
        if (DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance) == 0)
        {
            // Ignore PROD0
        }
        else
        {
            LOG(D, "A new product has been detected (0x%08x)", device_class_instance);
            connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PROP), NULL, 0, connector_climate_zone_feature.connector_id, portMAX_DELAY);
        }
    }
    else
    {
        if (DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance) == 0)
        {
            // Ignore PROD0
        }
        else
        {
            uint32_t prod_instance[1] = {device_class_instance};
            int32_t ref_group_inst[10];
            size_t num_refs = 1;

            // A product is removed
            // Check if we have this stored and linked
            feat_db_get_all_by_class_instance_interfaces_of_type(prod_instance, ref_group_inst, &num_refs, GROUP0TYPE_CLIMATEZONE);
            if (num_refs > 0)
            {
                LOG(D, "A climate zone product has been removed (0x%08x)", device_class_instance);
                UPDLINKEDCLASS_T *remove_prod = hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (remove_prod != NULL)
                {
                    remove_prod->update[0] = 0;
                    remove_prod->updclass = device_class_instance;
                    feat_db_update_interface_all(remove_prod, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), GROUP0TYPE_CLIMATEZONE);

                    hal_mem_free(remove_prod);
                }
                for (size_t i = 0; i < num_refs; ++i)
                {
                    // Set to not active
                    feat_db_update_cache(&Zero, sizeof(Zero), FEAT_DB_FIELD_ACTIVE, ref_group_inst[i]);
                }
            }
        }
    }
}

static void connector_process_task(void *Parameter)
{
    size_t frame_size = 0;
    DDMP2_FRAME *pframe;

    int ret = inventory_handler_add_any(&l_ih, PROD0);  // Add to inventory handler
    assert(ret == 0);
    ret = inventory_handler_start(&l_ih, &connector_climate_zone_feature);
    assert(ret == 0);

    while (1)
    {
        frame_size = 0;
        pframe = (DDMP2_FRAME *)xRingbufferReceive(connector_climate_zone_feature.to_connector, &frame_size, portMAX_DELAY);
        TRUE_CHECK(NULL != pframe);
        if ((frame_size != 0) && (NULL != pframe))
        {
            if (!inventory_handler_update(&l_ih, pframe))
            {
                if (!feat_db_frame_handler(pframe))
                {
                    switch (pframe->frame.control)
                    {
                    case DDMP2_CONTROL_PUBLISH:
                    {
                        // Handler subscribed parameters
                        switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.publish.parameter))
                        {
                        case PROD0PROP:
                        {
                            PROD0PROP_T *p_propprop = (PROD0PROP_T *)pframe->frame.publish.value.raw;
                            // We are only interested in CLIMATE (AC) type of product with valid instance
                            prodxprop_type_t type = {0};
                            type.data = p_propprop->type;
                            if ((type.type.cls == PRODXPROP_TYPE_CLASS_CLIMATE) && (p_propprop->inst > 0))
                            {
                                if (is_auto_detect_active)
                                {
                                    int32_t group_inst = -1;
                                    int32_t group_class_inst = -1;
                                    char uid[DICM_UID_KEY_STR_LEN] = {0};
                                    size_t uid_size = DICM_UID_KEY_STR_LEN;
                                    // Get the UID from product database for the specific product
                                    ProdDBReadCache(FIELD_UID, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter), uid, &uid_size);
                                    // Check whether there is a group (feature) that has this UID in its UID interface
                                    group_inst = feat_db_find_first_by_uid_interface(uid);
                                    if (group_inst == -1)
                                    {
                                        // If there is no group (feature) that has this UID in its interface
                                        LOG(D, "Group inst invalid");
                                        // Check whether there is a group (feature) with the ID (zone) same as the one for the product
                                        group_class_inst = feat_db_find_first_by_id_and_type(p_propprop->inst, GROUP0TYPE_CLIMATEZONE);
                                        if (group_class_inst > 0)
                                        {
                                            // If the group (feature) with the same zone exists
                                            LOG(D, "Group inst exists");
                                            int prod_class = DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter);
                                            // Check whether the product is part of its class interface, and if it is not, add it
                                            group_class_inst = feat_db_find_first_by_class_instance_interface((uint32_t)prod_class);
                                            if (group_class_inst == -1)
                                            {
                                                LOG(D, "Add product to group interface");
                                                feat_db_update_cache(&prod_class, sizeof(prod_class), FEAT_DB_FIELD_INTERFACE_CLASS_INST, group_class_inst);
                                            }
                                        }
                                        else
                                        {
                                            // If the group (feature) with the same zone doesn't exist
                                            LOG(D, "Group inst doesn't exist");
                                            int err = -1;
                                            // Create a new group (feature) of type climate zone, with the same ID (zone) as the product
                                            err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_climate_zone_feature.connector_id, p_propprop->inst, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &group_class_inst);
                                            if ((err == ESP_OK) && (group_class_inst > 0))
                                            {
                                                // If the group (feature) is successfully created, add the product as a part of its interface
                                                LOG(D, "Cache entry created");
                                                int prod_class = DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter);
                                                feat_db_update_cache(&prod_class, sizeof(prod_class), FEAT_DB_FIELD_INTERFACE_CLASS_INST, group_class_inst);
                                            }
                                            else
                                            {
                                                LOG(E, "Error creating cache entry %d", err);
                                            }
                                        }
                                        // Check whether the group is active, and if it is not, activate it
                                        int is_group_active;
                                        size_t is_group_active_size = sizeof(is_group_active);
                                        feat_db_read_cache(FEAT_DB_FIELD_ACTIVE, group_class_inst, &is_group_active, &is_group_active_size);
                                        if (is_group_active_size != 0)
                                        {
                                            if (is_group_active == 0)
                                            {
                                                feat_db_update_cache(&One, sizeof(One), FEAT_DB_FIELD_ACTIVE, group_class_inst);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        LOG(D, "Group inst valid");
                                        int prod_class = DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter);
                                        // Check whether the product is part of its class interface, and if it is not, add it
                                        group_class_inst = feat_db_find_first_by_class_instance_interface((uint32_t)prod_class);
                                        if (group_class_inst == -1)
                                        {
                                            char uid[DICM_UID_KEY_STR_LEN] = {0};
                                            size_t uid_size = DICM_UID_KEY_STR_LEN;
                                            // Get the UID from product database for the specific product
                                            ProdDBReadCache(FIELD_UID, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter), uid, &uid_size);
                                            // Find the group (feature) that has this UID in its UID interface
                                            group_class_inst = feat_db_find_first_by_uid_interface(uid);
                                            LOG(D, "Add product to group climate zone interface");
                                            feat_db_update_cache(&prod_class, sizeof(prod_class), FEAT_DB_FIELD_INTERFACE_CLASS_INST, group_class_inst);
                                        }
                                        // Check whether the group is active, and if it is not, activate it
                                        int is_group_active;
                                        size_t is_group_active_size = sizeof(is_group_active);
                                        feat_db_read_cache(FEAT_DB_FIELD_ACTIVE, group_class_inst, &is_group_active, &is_group_active_size);
                                        if (is_group_active_size != 0)
                                        {
                                            if (is_group_active == 0)
                                            {
                                                feat_db_update_cache(&One, sizeof(One), FEAT_DB_FIELD_ACTIVE, group_class_inst);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        break;
                        default:
                            break;
                        }
                    }
                    break;
                    default:
                        LOG(W, "Unexpected frame control type %d", pframe->frame.control);
                        break;
                    }
                }
            }
            vRingbufferReturnItem(connector_climate_zone_feature.to_connector, pframe);
        }
    }
}
