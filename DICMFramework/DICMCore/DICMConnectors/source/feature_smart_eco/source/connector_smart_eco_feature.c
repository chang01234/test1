/*! \file
 *  \brief Smart ECO feature service
 *
 *  The service is responsible for offering the feature interface of Smart ECO.
 *  Requirements to create the feature group is that both a active climate zone and a battery
 *  product is detected. If both requirements are fulfilled, SmartECO feature will be enabled
 *  and can be activated.
 *
 */
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>

#include "broker.h"
#include "cJSON.h"
#include "configuration.h"
#include "connector.h"
#include "connector_smart_eco_feature.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "feature_database.h"
#include "freertos/FreeRTOS.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#include "inventory_handler.h"
#include "product_conf_manager.h"
#include "product_database.h"

#define INVENTORY_HANDLER_STORAGE_SIZE              3
#define PRODXFWVER_MAX_STRING_LENGTH                16
#define PRODXFWVER_NUMBER_OF_VERSION_COMPONENTS     3
#define PRODXFWVER_MINIMUM_REQUIRED_VERSION         "2.0.0"
#define PRODXFWVER_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define PRODXMANUF_MAX_STRING_LENGTH                32
#define NUM_OF_WHITELISTED_MANUF                    2
#ifdef CONNECTOR_SMART_ECO_SPIFFS_FILENAME
#define CONFIG_SPIFFS_FILENAME CONNECTOR_SMART_ECO_SPIFFS_FILENAME
#else
#define CONFIG_SPIFFS_FILENAME "/spiffs/features/smarteco.json"
#endif

#define SMARTECO_CONFIG_CLIMATE 0
#define SMARTECO_CONFIG_COOLER  1

#define SMARTECO_CONFIG_FW_VER_STRING_SIZE PRODXFWVER_MAX_STRING_LENGTH
typedef struct _smarteco_config_t
{
    bool bat_detect;
    bool bat_manuf;
    bool cli_fwver;
    char cli_fwver_string[SMARTECO_CONFIG_FW_VER_STRING_SIZE];
} smarteco_config_t;

static EXT_RAM_ATTR smarteco_config_t smarteco_config[2];

static bool prod_batt_found = false;
static uint32_t prod_batt_instance = 255;

static bool is_ver_supported = false;
static bool is_prodXmanuf_valid = false;

static void check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_ENUM type);
static void get_version(const char *version_in_string, uint8_t *version_out_numbers, uint8_t version_size);
static bool version_comparison(const char *version_string, const char *minimum_requried_version_string);

static int initialize_connector(void);
static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available);
static void connector_process_task(void *Parameter);

static void load_smart_eco_config(void);

DECLARE_SORTED_LIST_EXTRAM(l_inventory_list, INVENTORY_HANDLER_STORAGE_SIZE);

static EXT_RAM_ATTR inventory_handler_t l_ih;

// TODO: This shoud be taken from configuration file
static bool MAYBE_UNUSED is_auto_detect_active = true;

CONNECTOR connector_smart_eco_feature = {
    .name = "Smart ECO feature",         // connector name
    .initialize = initialize_connector,  // connector initialize function
};

static int initialize_connector(void)  // initialize connector
{
    // Default check requirements
    smarteco_config[SMARTECO_CONFIG_CLIMATE].bat_detect = true;
    smarteco_config[SMARTECO_CONFIG_COOLER].bat_detect = true;
    smarteco_config[SMARTECO_CONFIG_CLIMATE].bat_manuf = true;
    smarteco_config[SMARTECO_CONFIG_COOLER].bat_manuf = false;
    smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver = true;
    smarteco_config[SMARTECO_CONFIG_COOLER].cli_fwver = false;
    memset(smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver_string, 0, SMARTECO_CONFIG_FW_VER_STRING_SIZE);
    memset(smarteco_config[SMARTECO_CONFIG_COOLER].cli_fwver_string, 0, SMARTECO_CONFIG_FW_VER_STRING_SIZE);
    strcpy(smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver_string, PRODXFWVER_MINIMUM_REQUIRED_VERSION);
    strcpy(smarteco_config[SMARTECO_CONFIG_COOLER].cli_fwver_string, PRODXFWVER_MINIMUM_REQUIRED_VERSION);
    // Read config file
    load_smart_eco_config();
    LOG(D, "smarteco_config[SMARTECO_CONFIG_CLIMATE].bat_detect = %d", smarteco_config[SMARTECO_CONFIG_CLIMATE].bat_detect);
    LOG(D, "smarteco_config[SMARTECO_CONFIG_COOLER].bat_detect = %d", smarteco_config[SMARTECO_CONFIG_COOLER].bat_detect);
    LOG(D, "smarteco_config[SMARTECO_CONFIG_CLIMATE].bat_manuf = %d", smarteco_config[SMARTECO_CONFIG_CLIMATE].bat_manuf);
    LOG(D, "smarteco_config[SMARTECO_CONFIG_COOLER].bat_manuf = %d", smarteco_config[SMARTECO_CONFIG_COOLER].bat_manuf);
    LOG(D, "smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver = %d", smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver);
    LOG(D, "smarteco_config[SMARTECO_CONFIG_COOLER].cli_fwver = %d", smarteco_config[SMARTECO_CONFIG_COOLER].cli_fwver);
    LOG(D, "smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver_string = %s", smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver_string);
    LOG(D, "smarteco_config[SMARTECO_CONFIG_COOLER].cli_fwver_string = %s", smarteco_config[SMARTECO_CONFIG_COOLER].cli_fwver_string);
    // Init inventory handler
    inventory_handler_init(&l_ih, &l_inventory_list, inventory_handler_cb, NULL);
    int32_t err = -1;
    int32_t ret = 1;
    err = feat_db_init();
    if (err == ESP_OK)
    {
        feat_db_load_cache(GROUP0TYPE_SMARTECO, connector_smart_eco_feature.connector_id);
        int32_t class_instance = -1;
        uint8_t type = GROUP0TYPE_SMARTECO;
        class_instance = feat_db_find_first_by_id_and_type(0, type);
        if (class_instance == -1)
        {
            err = feat_db_cache_entry_create(GROUP0TYPE_SMARTECO, connector_smart_eco_feature.connector_id, 0, GROUP0ENABLE_ON, GROUP0ACTIVE_OFF, &class_instance);
            if ((err == ESP_OK) && (class_instance > -1))
            {
                LOG(D, "Smart Eco class instance %d", class_instance);
            }
            else if (err != ESP_OK)
            {
                LOG(E, "Error %d", err);
            }
        }
        err = product_conf_manager_init();
        if (err != 0)
        {
            LOG(E, "product_conf_manager_init returned error %d", err);
        }

        TRUE_CHECK(xTaskCreate(connector_process_task, connector_smart_eco_feature.name, 3584, 0, xTASK_PRIORITY_NORMAL, NULL));
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
        if (DDMP2_INVENTORY_CLASS(device_class_instance) == PROD0)
        {
            if (DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance) == 0)
            {
                // Ignore PROD0
            }
            else
            {
                LOG(D, "A new product has been detected PROD%d", DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance));
                connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PROP), NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
            }
        }
        else if (DDMP2_INVENTORY_CLASS(device_class_instance) == GROUP0)
        {
            // Check the group type in order to see whether it is climate zone or cooler
            int32_t group_type;
            size_t group_type_size = sizeof(group_type);
            feat_db_read_cache(FEAT_DB_FIELD_TYPE, DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance), &group_type, &group_type_size);
            if (group_type_size > 0)
            {
                // Need to filter out our own first
                if (group_type == GROUP0TYPE_SMARTECO)
                {
                    LOG(D, "Ignore our own GROUP%d detected", DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance));
                }
                else if ((group_type == GROUP0TYPE_CLIMATEZONE) || (group_type == GROUP0TYPE_MOBILECOOLER))
                {
                    // If it is climate zone or mobile cooler
                    LOG(D, "New GROUP%d detected", DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance));
                    int32_t group_id = -1;
                    size_t group_id_size = sizeof(group_id);
                    feat_db_read_cache(FEAT_DB_FIELD_ID, DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance), &group_id, &group_id_size);
                    if (group_id_size > 0)
                    {
                        if (group_id > 0)
                        {
                            // Check the group ID
                            char group_uid[DICM_UID_KEY_STR_LEN];
                            size_t group_uid_size = DICM_UID_KEY_STR_LEN;
                            int32_t group_se_instance = -1;
                            // Get the group UID
                            feat_db_read_cache(FEAT_DB_FIELD_UID, DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance), &group_uid, &group_uid_size);
                            if (group_uid_size > 0)
                            {
                                // Check whether the climate zone/mobile cooler group is part of a SmartEco interface
                                // Here we assume that a climate zone / Mobile cooler can only be referenced in one SmartECO instance
                                group_se_instance = feat_db_find_first_by_uid_interface(group_uid);
                                if (group_se_instance == -1)
                                {
                                    // If it is not, proceed with creating a new SmartEco group instance
                                    int32_t err = 0;
                                    err = feat_db_cache_entry_create(GROUP0TYPE_SMARTECO, connector_smart_eco_feature.connector_id, group_id, GROUP0ENABLE_OFF, GROUP0ACTIVE_OFF, &group_se_instance);
                                    if ((err == 0) && (group_se_instance > 0))
                                    {
                                        if (group_type == GROUP0TYPE_MOBILECOOLER)
                                        {
                                            feat_db_update_cache("SmartECO with Cooler", strlen("SmartECO with Cooler"), FEAT_DB_FIELD_NAME, group_se_instance);
                                        }
                                        feat_db_update_cache(&device_class_instance, sizeof(device_class_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, group_se_instance);
                                    }
                                }
                                else
                                {
                                    // If it is part of the UID interface, proceed with updating the class instance interface only
                                    feat_db_update_cache(&device_class_instance, sizeof(device_class_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, group_se_instance);
                                }

                                // Check whether there is battery product detected and add it to the interface of the new SmartEco group instance
                                if (prod_batt_found)
                                {
                                    feat_db_update_cache(&prod_batt_instance, sizeof(prod_batt_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, group_se_instance);
                                }
                            }

                            // Check prerequisites for SmartEco - battery detected, manufacturer valid, version supported and climate zone group active
                            if (group_type == GROUP0TYPE_CLIMATEZONE)
                            {
                                check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_CLIMATEZONE);
                            }
                            // Check prerequisites for SmartEco - battery detected and group active
                            if (group_type == GROUP0TYPE_MOBILECOOLER)
                            {
                                check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_MOBILECOOLER);
                            }
                        }
                    }
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(GROUP0ID), NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(GROUP0ACTIVE), NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                }
            }
        }
    }
    else
    {
        if (DDMP2_INVENTORY_CLASS(device_class_instance) == PROD0)
        {
            if (DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance) == 0)
            {
                // Ignore PROD0
            }
            else
            {
                // A product is removed
                UPDLINKEDCLASS_T *remove_prod = hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (remove_prod != NULL)
                {
                    remove_prod->update[0] = 0;
                    remove_prod->updclass = device_class_instance;
                    if (prod_batt_instance == DDM2_PARAMETER_CLASS_INSTANCE(device_class_instance))
                    {
                        prod_batt_found = false;  // Assume that this is the only product we care about
                        is_prodXmanuf_valid = false;
                        feat_db_update_interface_all(remove_prod, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), GROUP0TYPE_SMARTECO);
                    }
                    int32_t enabled_group_inst[10];
                    size_t num_enabled = 10;
                    feat_db_get_all_enabled_instances_of_type(enabled_group_inst, &num_enabled, GROUP0TYPE_SMARTECO);
                    for (size_t i = 0; i < num_enabled; i++)
                    {
                        feat_db_update_cache(&Zero, sizeof(Zero), FEAT_DB_FIELD_ENABLE, enabled_group_inst[i]);
                    }
                    hal_mem_free(remove_prod);
                }
            }
        }
        else if (DDMP2_INVENTORY_CLASS(device_class_instance) == GROUP0)
        {
            // Check the group type in order to see whether it is climate zone
            int32_t group_type;
            size_t group_type_size = sizeof(group_type);
            feat_db_read_cache(FEAT_DB_FIELD_TYPE, DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance), &group_type, &group_type_size);
            if (group_type_size > 0)
            {
                // Need to filter out our own first
                if (group_type == GROUP0TYPE_SMARTECO)
                {
                    LOG(D, "Ignore our own GROUP%d detected", DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance));
                }
                else
                {
                    // Check if we have this stored and linked
                    int32_t group_se_inst = feat_db_find_first_by_class_instance_interface(device_class_instance);
                    if (group_se_inst != -1)
                    {
                        UPDLINKEDCLASS_T *remove_prod = hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                        if (remove_prod != NULL)
                        {
                            remove_prod->update[0] = 0;
                            remove_prod->updclass = device_class_instance;
                            feat_db_update_cache(remove_prod, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), FEAT_DB_FIELD_INTERFACE_CLASS_INST, group_se_inst);
                            feat_db_update_cache(&Zero, sizeof(Zero), FEAT_DB_FIELD_ENABLE, group_se_inst);
                            hal_mem_free(remove_prod);
                        }
                    }
                }
            }
        }
    }
}

static void connector_process_task(void *Parameter)
{
    size_t frame_size = 0;
    DDMP2_FRAME *pframe;

    int32_t ret = inventory_handler_add_any(&l_ih, PROD0);  // Add to inventory handler
    assert(ret == 0);
    ret = inventory_handler_add_any(&l_ih, GROUP0);  // Add to inventory handler
    assert(ret == 0);
    ret = inventory_handler_start(&l_ih, &connector_smart_eco_feature);
    assert(ret == 0);

    while (1)
    {
        frame_size = 0;
        pframe = (DDMP2_FRAME *)xRingbufferReceive(connector_smart_eco_feature.to_connector, &frame_size, portMAX_DELAY);
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
                        case PROD0CLIST:
                        {
                            LOG(D, "PROD%dCLIST received: %d", DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter), ddmp2_value_size(pframe));
                            for (size_t i = 0; i < ddmp2_value_size(pframe) / sizeof(uint32_t); i += sizeof(uint32_t))
                            {
                                uint32_t class_inst = *((uint32_t *)(&pframe->frame.publish.value.raw[i]));
                                if (DDM2_PARAMETER_CLASS(class_inst) == MBAT0)
                                {
                                    prod_batt_found = true;
                                    prod_batt_instance = DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter);
                                    if (is_auto_detect_active)
                                    {
                                        UPDLINKEDCLASS_T prod_batt_upd;
                                        prod_batt_upd.updclass = (uint32_t)prod_batt_instance;
                                        feat_db_update_interface_all(&prod_batt_upd, sizeof(UPDLINKEDCLASS_T), GROUP0TYPE_SMARTECO);
                                    }

                                    // Battery has been added to all SmartEco interfaces. Proceed with checking active CZ instances and activate SmartEco instances with same id.
                                    check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_CLIMATEZONE);
                                    check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_MOBILECOOLER);

                                    // Unsubscribe since the battery is detected and we are no longer interested in the updates for this parameter
                                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, pframe->frame.publish.parameter, NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                                    // Subscribe to PRODXMANUF for battery
                                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, prod_batt_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0MANUF), NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                                    break;
                                }
                                else if (DDM2_PARAMETER_CLASS(class_inst) == AC0)
                                {
                                    // Subscribe to PRODXFWVER for AC
                                    uint32_t prod_AC_instance = DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter);
                                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, prod_AC_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0FWVER), NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                                    // Unsubscribe since the battery is detected and we are no longer interested in the updates for this parameter
                                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, pframe->frame.publish.parameter, NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                                }
                            }
                            break;
                        }
                        case PROD0PROP:
                        {
                            LOG(D, "PROD%dPROP received: %d", DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter), ddmp2_value_size(pframe));
                            PROD0PROP_T *p_propprop = (PROD0PROP_T *)pframe->frame.publish.value.raw;
                            prodxprop_type_t type = {0};
                            type.data = p_propprop->type;
                            if ((type.type.cls == PRODXPROP_TYPE_CLASS_POWER) || (type.type.cls == PRODXPROP_TYPE_CLASS_CLIMATE))
                            {
                                // We are only interested in power (battery) or climate (ac)
                                // Check clist
                                connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, DDM2_PARAMETER_INSTANCE_PART(pframe->frame.publish.parameter) | PROD0CLIST, NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                            }
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, pframe->frame.publish.parameter, NULL, 0, connector_smart_eco_feature.connector_id, portMAX_DELAY);
                            break;
                        }
                        case GROUP0ID:
                        {
                            // Change ID for SmartEco if it exists
                            int32_t group_se_inst = -1;
                            group_se_inst = feat_db_find_first_by_class_instance_interface(DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter));
                            if (group_se_inst != -1)
                            {
                                feat_db_update_cache(&pframe->frame.publish.value.int32, sizeof(int32_t), FEAT_DB_FIELD_ID, group_se_inst);
                            }
                            break;
                        }
                        case GROUP0ACTIVE:
                        {
                            // Check whether this group is part of SE interface
                            // Change enable status depending on the value of the active status of CZ instance
                            // NOTE: Assumes that a climate zone or mobile cooler can only be referenced in one SmartEco group instance
                            int32_t group_se_inst = -1;
                            group_se_inst = feat_db_find_first_by_class_instance_interface(DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter));
                            LOG(D, "GROUP%dACTIVE - %d", DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter), group_se_inst);
                            if (group_se_inst != -1)
                            {
                                if (pframe->frame.publish.value.int32 == GROUP0ACTIVE_OFF)
                                {
                                    feat_db_update_cache(&pframe->frame.publish.value.int32, sizeof(int32_t), FEAT_DB_FIELD_ENABLE, group_se_inst);
                                }
                                else if (pframe->frame.publish.value.int32 == GROUP0ACTIVE_ON)
                                {
                                    // get Type
                                    int32_t group_type = -1;
                                    size_t group_type_size = sizeof(group_type);
                                    feat_db_read_cache(FEAT_DB_FIELD_TYPE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter), &group_type, &group_type_size);
                                    LOG(D, "group_type: %d is_ver_supported: %d is_prodXmanuf_valid: %d prod_batt_found: %d", group_type, is_ver_supported, is_prodXmanuf_valid, prod_batt_found);
                                    const int Index = (GROUP0TYPE_ENUM)group_type == GROUP0TYPE_CLIMATEZONE ? SMARTECO_CONFIG_CLIMATE : SMARTECO_CONFIG_COOLER;
                                    if ((smarteco_config[Index].bat_detect ? prod_batt_found : true) &&
                                        (smarteco_config[Index].bat_manuf ? is_prodXmanuf_valid : true) &&
                                        (smarteco_config[Index].cli_fwver ? is_ver_supported : true))
                                    {
                                        feat_db_update_cache(&pframe->frame.publish.value.int32, sizeof(int32_t), FEAT_DB_FIELD_ENABLE, group_se_inst);
                                    }
                                    else
                                    {
                                        feat_db_update_cache(&Zero, sizeof(Zero), FEAT_DB_FIELD_ENABLE, group_se_inst);
                                    }
                                }
                            }
                            break;
                        }
                        case PROD0FWVER:
                        {
                            char version_string[PRODXFWVER_MAX_STRING_LENGTH];
                            ddmp2_extract_string_from_frame(pframe, version_string, PRODXFWVER_MAX_STRING_LENGTH);
                            LOG(D, "version_string", version_string);
                            is_ver_supported = version_comparison(version_string, smarteco_config[SMARTECO_CONFIG_CLIMATE].cli_fwver_string);
                            LOG(D, "version_string %s - %d", version_string, is_ver_supported);
                            check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_CLIMATEZONE);
                            break;
                        }
                        case PROD0MANUF:
                        {
                            // Mobile power product manufacturer
                            char prodXmanuf[PRODXMANUF_MAX_STRING_LENGTH];
                            ddmp2_extract_string_from_frame(pframe, prodXmanuf, PRODXMANUF_MAX_STRING_LENGTH);
                            LOG(D, "prodXmanuf: %s", prodXmanuf);
                            is_prodXmanuf_valid = product_conf_manager_find_manufacturer(prodXmanuf) != NULL;
                            check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_CLIMATEZONE);
                            check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_MOBILECOOLER);
                            break;
                        }
                        default:
                            break;
                        }
                        break;
                    }
                    default:
                        LOG(W, "Unexpected frame control type %d", pframe->frame.control);
                        break;
                    }
                }
            }
            vRingbufferReturnItem(connector_smart_eco_feature.to_connector, pframe);
        }
    }
}

static void check_active_and_update_enable_of_referenced_group_type(GROUP0TYPE_ENUM type)
{
    int32_t active_group_insts[10];
    uint32_t active_group_class_insts[10];
    int32_t active2_group_insts[10];
    size_t num_active = 10;
    feat_db_get_all_active_instances_of_type(active_group_insts, &num_active, type);
    for (size_t i = 0; i < num_active; i++)
    {
        LOG(D, "Active group type (%d) %s group instance %d", type, type == GROUP0TYPE_CLIMATEZONE ? "Climate zone" : "Mobile cooler", active_group_insts[i]);
        active_group_class_insts[i] = DDM2_PARAMETER_INSTANCE(active_group_insts[i]) | GROUP0;
    }
    feat_db_get_all_by_class_instance_interfaces_of_type(active_group_class_insts, active2_group_insts, &num_active, GROUP0TYPE_SMARTECO);
    for (size_t i = 0; i < num_active; i++)
    {
        const int Index = type == GROUP0TYPE_CLIMATEZONE ? SMARTECO_CONFIG_CLIMATE : SMARTECO_CONFIG_COOLER;
        LOG(D, "Referenced Smart ECO group instance %d", active2_group_insts[i]);
        LOG(D, "is_ver_supported: %d is_prodXmanuf_valid: %d prod_batt_found: %d", is_ver_supported, is_prodXmanuf_valid, prod_batt_found);
        if ((smarteco_config[Index].bat_detect ? prod_batt_found : true) &&
            (smarteco_config[Index].bat_manuf ? is_prodXmanuf_valid : true) &&
            (smarteco_config[Index].cli_fwver ? is_ver_supported : true))
        {
            feat_db_update_cache(&One, sizeof(One), FEAT_DB_FIELD_ENABLE, active2_group_insts[i]);
        }
    }
}

/* PRODXFWVER dependencies implementation */
static void get_version(const char *version_in_string, uint8_t *version_out_numbers, uint8_t version_size)
{
    uint8_t version_id = 0;
    const char *p_version_in_string = version_in_string;
    /* Loop through version string and take first "version_size" version numbers */
    while (*p_version_in_string)
    {
        if (version_id >= version_size)
        {
            break;
        }
        if (isdigit((uint8_t)(*p_version_in_string)))
        {
            /* Found a number, read it*/
            long val = strtol(p_version_in_string, (char **)&p_version_in_string, 10);
            version_out_numbers[version_id] = (uint8_t)val;
            version_id++;
        }
        else
        {
            /* Otherwise, move on to the next character. */
            p_version_in_string++;
        }
    }
}

static bool version_comparison(const char *version_string, const char *minimum_requried_version_string)
{
    bool is_version_compatible = false;
    uint8_t version_data[PRODXFWVER_NUMBER_OF_VERSION_COMPONENTS];
    uint8_t minimum_requried_version_data[PRODXFWVER_NUMBER_OF_VERSION_COMPONENTS];

    memset(version_data, 0, sizeof(version_data));
    memset(minimum_requried_version_data, 0, sizeof(minimum_requried_version_data));

    // Get version as array of integers
    get_version(version_string, version_data, sizeof(version_data));
    uint8_t ver_major = version_data[0];
    uint8_t ver_minor = version_data[1];
    uint8_t ver_patch = version_data[2];

    get_version(minimum_requried_version_string, minimum_requried_version_data, sizeof(minimum_requried_version_data));
    uint8_t min_req_major = minimum_requried_version_data[0];
    uint8_t min_req_minor = minimum_requried_version_data[1];
    uint8_t min_req_patch = minimum_requried_version_data[2];

    if (PRODXFWVER_VERSION_VAL(min_req_major, min_req_minor, min_req_patch) <= PRODXFWVER_VERSION_VAL(ver_major, ver_minor, ver_patch))
    {
        is_version_compatible = true;
    }

    return is_version_compatible;
}

static void load_smart_eco_config(void)
{
    struct stat st;

    if (stat(CONFIG_SPIFFS_FILENAME, &st) != 0)
    {
        LOG(W, "Could not find whitelist file: %s", CONFIG_SPIFFS_FILENAME);
    }
    else
    {
        LOG(D, "Trying to load whitelist file: %s (%ld bytes)", CONFIG_SPIFFS_FILENAME, st.st_size);
        FILE *f = fopen(CONFIG_SPIFFS_FILENAME, "r");
        if (f == NULL)
        {
            LOG(W, "Could not open whitelist file: %s", CONFIG_SPIFFS_FILENAME);
        }
        else
        {
            cJSON *root;
            char *buffer = hal_mem_malloc_prefer(st.st_size + 4, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            if (buffer != NULL)
            {
                size_t read_bytes = fread(buffer, 1, st.st_size, f);
                LOG(D, "Read %d bytes", read_bytes);
                fclose(f);
                root = cJSON_ParseWithLength(buffer, read_bytes);
                if (root != NULL)
                {
                    LOG(D, "Successfully parsed JSON: %s", cJSON_Print(root));
                    cJSON *obj_arr[2];
                    obj_arr[SMARTECO_CONFIG_CLIMATE] = cJSON_GetObjectItemCaseSensitive(root, "climate");
                    obj_arr[SMARTECO_CONFIG_COOLER] = cJSON_GetObjectItemCaseSensitive(root, "cooler");

                    for (int i = 0; i < 2; ++i)
                    {
                        if (obj_arr[i] != NULL)
                        {
                            if (cJSON_IsObject(obj_arr[i]))
                            {
                                // Get properties
                                cJSON *obj = cJSON_GetObjectItemCaseSensitive(obj_arr[i], "bat_detect");
                                if ((obj != NULL) && (cJSON_IsBool(obj)))
                                {
                                    smarteco_config[i].bat_detect = cJSON_IsTrue(obj);
                                }
                                else
                                {
                                    smarteco_config[i].bat_detect = false;
                                }
                                obj = cJSON_GetObjectItemCaseSensitive(obj_arr[i], "bat_manuf");
                                if ((obj != NULL) && (cJSON_IsBool(obj)))
                                {
                                    smarteco_config[i].bat_manuf = cJSON_IsTrue(obj);
                                }
                                else
                                {
                                    smarteco_config[i].bat_manuf = false;
                                }
                                obj = cJSON_GetObjectItemCaseSensitive(obj_arr[i], "clim_fw_ver");
                                if ((obj != NULL) && (cJSON_IsBool(obj)))
                                {
                                    smarteco_config[i].cli_fwver = cJSON_IsTrue(obj);
                                }
                                else
                                {
                                    smarteco_config[i].cli_fwver = false;
                                }
                                obj = cJSON_GetObjectItemCaseSensitive(obj_arr[i], "clim_fw_ver_string");
                                if ((obj != NULL) && (cJSON_IsString(obj)))
                                {
                                    strcpy(smarteco_config[i].cli_fwver_string, cJSON_GetStringValue(obj));
                                }
                                else
                                {
                                    smarteco_config[i].cli_fwver_string[0] = '\0';
                                }
                            }
                            else
                            {
                                LOG(W, "Error json format:Json object arr index %d is not an cJSON_IsObject", i);
                                LOG(W, "Error parsing json: %p (%p)", cJSON_GetErrorPtr(), buffer);
                            }
                        }
                        else
                        {
                            // Use default values
                            LOG(W, "Json object arr index %d not found in file. Use defaults", i);
                        }
                    }
                    cJSON_Delete(root);
                }
                hal_mem_free(buffer);
            }
            else
            {
                LOG(E, "Failed to allocate memory for whitelist buffer");
            }
        }
    }
}
