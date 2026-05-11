/**
 * \brief Feature database library which stores and caches GROUP<X> parameters
 *        that are used as features.
 */

#include "feature_database.h"
#include "broker.h"
#include "connector.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "freertos/semphr.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#include "nvs.h"
#include "product_database.h"
#include "uid_generator.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/queue.h>

#define NVS_FEAT_DB_NAMESPACE            "feat_db"
#define MAX_GROUP_NAME_LENGTH            32
#define NUM_OF_INTERFACES                10
#define INVALID_GROUP_ID                 -1
#define SMART_ECO_GROUP_FEATURE_NAME     "SMART ECO FEATURE"
#define DEFAULT_SMART_ECO_GROUP_NAME     "SmartECO in Zone "
#define CLIMATE_ZONE_GROUP_FEATURE_NAME  "CLIMATE ZONE FEATURE"
#define DEFAULT_CLIMATE_ZONE_GROUP_NAME  "Zone "
#define DEFAULT_MOBILE_COOLER_GROUP_NAME "Mobile cooler"
#define INVALID_CLASS_INSTANCE           -1

typedef struct feature_database
{
    int32_t class_instance;
    uint8_t connector_id;
    int32_t type;
    int32_t active;
    int32_t enable;
    int32_t id;
    char name[MAX_GROUP_NAME_LENGTH];
    uint8_t num_interfaces_uid;
    uint8_t num_interfaces_class_inst;
    char interface_uid[NUM_OF_INTERFACES][DICM_UID_KEY_STR_LEN];
    uint32_t interface_class_inst[NUM_OF_INTERFACES];
    char uid[DICM_UID_KEY_STR_LEN];
    uint8_t num_rules;
    uint32_t rules[NUM_OF_INTERFACES];
    LIST_ENTRY(feature_database) list_node;
} feature_database_t;

#define MAX_BUFFER_SIZE (sizeof((feature_database_t *)0)->type +               \
                         sizeof((feature_database_t *)0)->enable +             \
                         sizeof((feature_database_t *)0)->id +                 \
                         sizeof((feature_database_t *)0)->num_interfaces_uid + \
                         NUM_OF_INTERFACES * DICM_UID_KEY_STR_LEN +            \
                         MAX_GROUP_NAME_LENGTH + 1)

static int32_t feat_db_update_nvs(feature_database_t *fdb_entry);
static feature_database_t *feat_db_find_by_class_instance(int32_t class_instance);
static bool feat_db_cache_entry_update_interfaces(feature_database_t *fdb_entry, const void *data, size_t data_size);
static void feat_db_cache_entry_update_rules(feature_database_t *fdb_entry, const void *data, size_t data_size);
static int32_t feat_db_find_next_id(void);

typedef LIST_HEAD(list_head_feat_db, feature_database) l_feat_db_t;

static nvs_handle_t nvs_feat_db;

static SemaphoreHandle_t feat_db_mutex;

static EXT_RAM_ATTR l_feat_db_t feat_db_cache;

int32_t feat_db_init(void)
{
    static bool is_fdb_initialized = false;
    esp_err_t err = ESP_OK;
    if (!is_fdb_initialized)
    {
        is_fdb_initialized = true;
        feat_db_mutex = xSemaphoreCreateRecursiveMutex();
        if (feat_db_mutex == NULL)
        {
            LOG(E, "Feature database mutex cannot be created");
            err = FEAT_DB_ERR_MUTEX_NOT_CREATED;
        }
        if (err == ESP_OK)
        {
            err = nvs_open(NVS_FEAT_DB_NAMESPACE, NVS_READWRITE, &nvs_feat_db);
            if (err == ESP_OK)
            {
                LIST_INIT(&feat_db_cache);
                LOG(I, "Feature database initialized.");
            }
            else
            {
                is_fdb_initialized = false;
                vSemaphoreDelete(feat_db_mutex);
                LOG(E, "Feature database not initialized.");
            }
        }
    }
    return err;
}

int32_t feat_db_frame_handler(const DDMP2_FRAME *const p_frame)
{
    int32_t ret = 0;

    if (p_frame == NULL)
    {
        return 0;
    }
    TRUE_CHECK(xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY));

    feature_database_t *fdb_entry;

    switch (p_frame->frame.control)
    {
    case DDMP2_CONTROL_SUBSCRIBE:
    {
        if (DDM2_PARAMETER_CLASS(p_frame->frame.subscribe.parameter) == GROUP0)
        {
            int32_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
            fdb_entry = feat_db_find_by_class_instance(instance);
            if (fdb_entry != NULL)
            {
                switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
                {
                case GROUP0ACTIVE:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &fdb_entry->active, sizeof(fdb_entry->active), p_frame->destination_connector, portMAX_DELAY);
                    break;
                case GROUP0ENABLE:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &fdb_entry->enable, sizeof(fdb_entry->enable), p_frame->destination_connector, portMAX_DELAY);
                    break;
                case GROUP0ID:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &fdb_entry->id, sizeof(fdb_entry->id), p_frame->destination_connector, portMAX_DELAY);
                    break;
                case GROUP0UID:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, fdb_entry->uid, strlen(fdb_entry->uid), p_frame->destination_connector, portMAX_DELAY);
                    break;
                case GROUP0TYPE:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &fdb_entry->type, sizeof(fdb_entry->type), p_frame->destination_connector, portMAX_DELAY);
                    break;
                case GROUP0NAME:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, fdb_entry->name, strlen(fdb_entry->name), p_frame->destination_connector, portMAX_DELAY);
                    break;
                case GROUP0INTERFACE:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, fdb_entry->interface_class_inst, fdb_entry->num_interfaces_class_inst * sizeof(fdb_entry->interface_class_inst[0]), p_frame->destination_connector, portMAX_DELAY);
                    break;
                case GROUP0RULES:
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, fdb_entry->rules, fdb_entry->num_rules * sizeof(fdb_entry->rules[0]), p_frame->destination_connector, portMAX_DELAY);
                    break;
                default:
                    break;
                }
            }
        }
        ret = 1;
    }
    break;
    case DDMP2_CONTROL_SET:
    {
        if (DDM2_PARAMETER_CLASS(p_frame->frame.set.parameter) == GROUP0)
        {
            int32_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
            fdb_entry = feat_db_find_by_class_instance(instance);

            if (fdb_entry != NULL)
            {
                if (fdb_entry->id == 0)
                {
                    // Manager group class
                    switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
                    {
                    case GROUP0ADDGROUP:
                    {
                        int32_t err = 0;
                        int32_t class_instance = INVALID_CLASS_INSTANCE;
                        uint32_t group_class_instance;
                        err = feat_db_cache_entry_create(fdb_entry->type, p_frame->destination_connector, INVALID_GROUP_ID, GROUP0ENABLE_OFF, GROUP0ACTIVE_OFF, &class_instance);
                        if (class_instance < 0)
                        {
                            LOG(E, "Failed to request a GROUP0 class instance, error %d", err);
                            // Return back invalid group instance
                            group_class_instance = DDMP2_INVALID_INSTANCE;
                            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.set.parameter, &group_class_instance, sizeof(group_class_instance), p_frame->destination_connector, portMAX_DELAY);
                        }
                        else
                        {
                            group_class_instance = GROUP0 | DDM2_PARAMETER_INSTANCE(class_instance);
                            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.set.parameter, &group_class_instance, sizeof(group_class_instance), p_frame->destination_connector, portMAX_DELAY);
                        }
                    }
                    break;
                    default:
                        break;
                    }
                }
                else
                {
                    // Created group class
                    switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
                    {
                    case GROUP0DELGROUP:
                    {
                        feat_db_cache_entry_delete(instance);
                    }
                    break;

                    case GROUP0NAME:
                    {
                        char group_name[MAX_GROUP_NAME_LENGTH] = {0};
                        if (ddmp2_value_size(p_frame) == 0)
                        {
                            if (fdb_entry->type == GROUP0TYPE_CLIMATEZONE)
                            {
                                strcpy(group_name, DEFAULT_CLIMATE_ZONE_GROUP_NAME);
                            }
                            else if (fdb_entry->type == GROUP0TYPE_SMARTECO)
                            {
                                strcpy(group_name, DEFAULT_SMART_ECO_GROUP_NAME);
                            }
                        }
                        else
                        {
                            ddmp2_extract_string_from_frame(p_frame, group_name, MAX_GROUP_NAME_LENGTH - 1);
                        }
                        feat_db_update_cache(group_name, strlen(group_name), FEAT_DB_FIELD_NAME, instance);
                    }
                    break;

                    case GROUP0ID:
                    {
                        feat_db_update_cache(&p_frame->frame.set.value.int32, sizeof(fdb_entry->id), FEAT_DB_FIELD_ID, instance);
                    }
                    break;

                    case GROUP0ACTIVE:
                    {
                        feat_db_update_cache(&p_frame->frame.set.value.int32, sizeof(fdb_entry->active), FEAT_DB_FIELD_ACTIVE, instance);
                    }
                    break;
                    case GROUP0ENABLE:
                    {
                        feat_db_update_cache(&p_frame->frame.set.value.int32, sizeof(fdb_entry->enable), FEAT_DB_FIELD_ENABLE, instance);
                    }
                    break;
                    case GROUP0RULES:
                    {
                        feat_db_update_cache(p_frame->frame.set.value.raw, ddmp2_value_size(p_frame), FEAT_DB_FIELD_RULES, instance);
                    }
                    break;
                    case GROUP0INTERFACE:
                    {
                        feat_db_update_cache(p_frame->frame.set.value.raw, ddmp2_value_size(p_frame), FEAT_DB_FIELD_INTERFACE_CLASS_INST, instance);
                    }
                    break;
                    default:
                        break;
                    }
                }
            }
            ret = 1;
        }
    }
    break;
    default:
        break;
    }

    xSemaphoreGiveRecursive(feat_db_mutex);
    return ret;
}

static int32_t feat_db_update_nvs(feature_database_t *fdb_entry)
{
    esp_err_t err = ESP_OK;
    TRUE_CHECK_RETURNX(FEAT_DB_ERR_INVALID_DATA, fdb_entry != NULL);

    uint8_t buffer[MAX_BUFFER_SIZE] = {0};

    size_t offset = 0;
    memcpy(buffer + offset, &fdb_entry->type, sizeof(fdb_entry->type));
    offset += sizeof(fdb_entry->type);

    memcpy(buffer + offset, &fdb_entry->enable, sizeof(fdb_entry->enable));
    offset += sizeof(fdb_entry->enable);

    memcpy(buffer + offset, &fdb_entry->id, sizeof(fdb_entry->id));
    offset += sizeof(fdb_entry->id);

    memcpy(buffer + offset, &fdb_entry->num_interfaces_uid, sizeof(fdb_entry->num_interfaces_uid));
    offset += sizeof(fdb_entry->num_interfaces_uid);

    memcpy(buffer + offset, fdb_entry->name, strlen(fdb_entry->name));
    offset += strlen(fdb_entry->name) + 1;

    for (int32_t i = 0; i < fdb_entry->num_interfaces_uid; i++)
    {
        memcpy(buffer + offset, fdb_entry->interface_uid[i], DICM_UID_KEY_STR_LEN);
        offset += DICM_UID_KEY_STR_LEN;
    }
    err = nvs_set_blob(nvs_feat_db, fdb_entry->uid, buffer, offset);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_feat_db);
        if (err != ESP_OK)
        {
            LOG(E, "Cannot commit to NVS %d", err);
        }
    }
    else
    {
        LOG(E, "Cannot set NVS blob %d", err);
    }
    return err;
}

void feat_db_update_cache(const void *feat_db_field, size_t feat_db_field_size, feature_database_field_t field, int32_t class_instance)
{
    feature_database_t *fdb_entry = NULL;
    bool update_nvs = false;
    bool publish = false;

    TRUE_CHECK(xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY));
    fdb_entry = feat_db_find_by_class_instance(class_instance);

    if (fdb_entry != NULL)
    {
        uint32_t param_id = 0;
        uint8_t value[NUM_OF_INTERFACES * sizeof(uint32_t)] = {0};
        size_t value_size = 0;

        if (feat_db_field != NULL)
        {
            switch (field)
            {
            case FEAT_DB_FIELD_ACTIVE:
            {
                if (fdb_entry->active != *(const int32_t *)feat_db_field)
                {
                    fdb_entry->active = *(const int32_t *)feat_db_field;
                    LOG(D, "Update field ACTIVE %d", *(int32_t *)feat_db_field);
                    publish = true;
                }
                param_id = GROUP0ACTIVE;
                memcpy(value, &fdb_entry->active, sizeof(fdb_entry->active));
                value_size = sizeof(fdb_entry->active);
            }
            break;
            case FEAT_DB_FIELD_ENABLE:
            {
                if (fdb_entry->enable != *(const int32_t *)feat_db_field)
                {
                    fdb_entry->enable = *(const int32_t *)feat_db_field;
                    LOG(D, "Update field ENABLE %d", *(int32_t *)feat_db_field);
                    update_nvs = true;
                    publish = true;
                }
                param_id = GROUP0ENABLE;
                memcpy(value, &fdb_entry->enable, sizeof(fdb_entry->enable));
                value_size = sizeof(fdb_entry->enable);
            }
            break;
            case FEAT_DB_FIELD_ID:
            {
                if (fdb_entry->id != *(const int32_t *)feat_db_field)
                {
                    fdb_entry->id = *(const int32_t *)feat_db_field;
                    LOG(D, "Update field ID %d", *(int32_t *)feat_db_field);
                    update_nvs = true;
                    publish = true;
                }
                param_id = GROUP0ID;
                memcpy(value, &fdb_entry->id, sizeof(fdb_entry->id));
                value_size = sizeof(fdb_entry->id);
            }
            break;
            case FEAT_DB_FIELD_NAME:
            {
                if (strcmp((const char *)feat_db_field, fdb_entry->name) != 0)
                {
                    memset(fdb_entry->name, 0, MAX_GROUP_NAME_LENGTH);
                    memcpy(fdb_entry->name, (const char *)feat_db_field, feat_db_field_size);
                    memset(fdb_entry->name + (MAX_GROUP_NAME_LENGTH - 1), 0, 1);
                    LOG(D, "Update field NAME %s", (const char *)feat_db_field);
                    update_nvs = true;
                    publish = true;
                }
                param_id = GROUP0NAME;
                memcpy(value, fdb_entry->name, strlen(fdb_entry->name));
                value_size = strlen(fdb_entry->name);
            }
            break;
            case FEAT_DB_FIELD_INTERFACE_CLASS_INST:
            {
                // This can add a class instance to uid and interface lists, but only remove from interface list
                update_nvs = feat_db_cache_entry_update_interfaces(fdb_entry, feat_db_field, feat_db_field_size);
                LOG(D, "Update field INTERFACE CLASS INST");
                publish = true;
                param_id = GROUP0INTERFACE;
                memcpy(value, fdb_entry->interface_class_inst, fdb_entry->num_interfaces_class_inst * sizeof(fdb_entry->interface_class_inst[0]));
                value_size = fdb_entry->num_interfaces_class_inst * sizeof(fdb_entry->interface_class_inst[0]);
            }
            break;
            case FEAT_DB_FIELD_RULES:
            {
                /* If we update this field, it means that we always add or remove classes, so the changes
                   for this field should always be published */
                feat_db_cache_entry_update_rules(fdb_entry, feat_db_field, feat_db_field_size);
                LOG(D, "Update field INTERFACE RULES");
                publish = true;
                param_id = GROUP0RULES;
                memcpy(value, fdb_entry->rules, fdb_entry->num_rules * sizeof(fdb_entry->rules[0]));
                value_size = fdb_entry->num_rules * sizeof(fdb_entry->rules[0]);
            }
            break;
            default:
                break;
            }
            if (update_nvs)
            {
                int32_t err = ESP_OK;
                err = feat_db_update_nvs(fdb_entry);
                if (err != ESP_OK)
                {
                    LOG(E, "Cannot update NVS, err: %d", err);
                }
            }
            if (publish)
            {
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, param_id | DDM2_PARAMETER_INSTANCE(fdb_entry->class_instance), value, value_size, fdb_entry->connector_id, (TickType_t)portMAX_DELAY);
            }
        }
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
}

void feat_db_read_cache(feature_database_field_t field, int32_t class_instance, void *data, size_t *data_size)
{
    feature_database_t *group_node = NULL;

    TRUE_CHECK(xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY));
    group_node = feat_db_find_by_class_instance(class_instance);
    if (group_node != NULL)
    {
        if (data != NULL)
        {
            switch (field)
            {
            case FEAT_DB_FIELD_TYPE:
            {
                if (sizeof(group_node->type) <= *data_size)
                {
                    *(int32_t *)data = group_node->type;
                    *data_size = sizeof(group_node->type);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            case FEAT_DB_FIELD_ACTIVE:
            {
                if (sizeof(group_node->active) <= *data_size)
                {
                    *(int32_t *)data = group_node->active;
                    *data_size = sizeof(group_node->active);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            case FEAT_DB_FIELD_ENABLE:
            {
                if (sizeof(group_node->enable) <= *data_size)
                {
                    *(int32_t *)data = group_node->enable;
                    *data_size = sizeof(group_node->enable);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            case FEAT_DB_FIELD_ID:
            {
                if (sizeof(group_node->id) <= *data_size)
                {
                    *(int32_t *)data = group_node->id;
                    *data_size = sizeof(group_node->id);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            case FEAT_DB_FIELD_NAME:
            {
                if ((strlen(group_node->name) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, group_node->name, strlen(group_node->name));
                    *data_size = strlen(group_node->name);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            case FEAT_DB_FIELD_INTERFACE_CLASS_INST:
            {
                if ((group_node->num_interfaces_class_inst * sizeof(group_node->interface_class_inst[0])) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, group_node->interface_class_inst, group_node->num_interfaces_class_inst * sizeof(uint32_t));
                    *data_size = group_node->num_interfaces_class_inst * sizeof(uint32_t);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            case FEAT_DB_FIELD_RULES:
            {
                if ((group_node->num_rules * sizeof(group_node->rules[0])) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, group_node->rules, group_node->num_rules * sizeof(uint32_t));
                    *data_size = group_node->num_rules * sizeof(uint32_t);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            case FEAT_DB_FIELD_UID:
            {
                if ((strlen(group_node->uid) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, group_node->uid, strlen(group_node->uid));
                    *data_size = strlen(group_node->uid);
                }
                else
                {
                    *data_size = 0;
                }
            }
            break;

            default:
                break;
            }
        }
        else
        {
            *data_size = 0;
        }
    }
    else
    {
        *data_size = 0;
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
}

void feat_db_load_cache(GROUP0TYPE_ENUM group_type, uint8_t connector_id)
{
    esp_err_t err;
    nvs_iterator_t it;

    TRUE_CHECK(xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY));

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    err = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_FEAT_DB_NAMESPACE, NVS_TYPE_BLOB, &it);
#else
    it = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_FEAT_DB_NAMESPACE, NVS_TYPE_BLOB);
#endif
    if (it == NULL)
    {
        LOG(D, "No entries found in flash");
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    while (err == ESP_OK)
#else
    while (it != NULL)
#endif
    {
        feature_database_t *fdb_entry = NULL;
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        uint8_t blob[MAX_BUFFER_SIZE] = {0};
        size_t max_blob_size = MAX_BUFFER_SIZE;
        fdb_entry = hal_mem_malloc_prefer(sizeof(feature_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (fdb_entry != NULL)
        {
            memset(fdb_entry, 0, sizeof(feature_database_t));
            err = nvs_get_blob(nvs_feat_db, info.key, blob, &max_blob_size);
            if (err == ESP_OK)
            {
                size_t offset = 0;
                int32_t type;
                memcpy(&type, blob + offset, sizeof(fdb_entry->type));
                if (type == (int)group_type)
                {
                    memcpy(&fdb_entry->type, blob + offset, sizeof(fdb_entry->type));
                    offset += sizeof(fdb_entry->type);
                    LOG(D, "Type: %d", fdb_entry->type);

                    memcpy(&fdb_entry->enable, blob + offset, sizeof(fdb_entry->enable));
                    offset += sizeof(fdb_entry->enable);
                    LOG(D, "Enable: %d", fdb_entry->enable);

                    memcpy(&fdb_entry->id, blob + offset, sizeof(fdb_entry->id));
                    offset += sizeof(fdb_entry->id);
                    LOG(D, "ID: %d", fdb_entry->id);

                    memcpy(&fdb_entry->num_interfaces_uid, blob + offset, sizeof(fdb_entry->num_interfaces_uid));
                    offset += sizeof(fdb_entry->num_interfaces_uid);
                    LOG(D, "Num Interfaces: %d", fdb_entry->num_interfaces_uid);

                    size_t name_len = strlen((char *)(blob + offset)) + 1;
                    memset(fdb_entry->name, 0, name_len);
                    memcpy(fdb_entry->name, blob + offset, name_len);
                    offset += name_len;
                    LOG(D, "Name: %s", fdb_entry->name);

                    for (int32_t i = 0; i < fdb_entry->num_interfaces_uid; i++)
                    {
                        memcpy(fdb_entry->interface_uid[i], blob + offset, DICM_UID_KEY_STR_LEN);
                        LOG(D, "Interface %d: %s", i, fdb_entry->interface_uid[i]);
                        offset += DICM_UID_KEY_STR_LEN;
                    }

                    strncpy(fdb_entry->uid, info.key, DICM_UID_KEY_STR_LEN - 1);
                    fdb_entry->uid[DICM_UID_KEY_STR_LEN - 1] = '\0';
                    LOG(D, "UID: %s", fdb_entry->uid);
                    fdb_entry->class_instance = INVALID_CLASS_INSTANCE;
                    fdb_entry->list_node.le_next = NULL;
                    fdb_entry->list_node.le_prev = NULL;

                    uint32_t device_class = GROUP0;
                    int32_t class_instance = broker_register_instance(&device_class, connector_id);
                    if (class_instance > INVALID_CLASS_INSTANCE)
                    {
                        fdb_entry->class_instance = class_instance;
                        fdb_entry->connector_id = connector_id;
                        LOG(D, "Class instance: %d", fdb_entry->class_instance);
                        LOG(D, "Connector ID: %d", fdb_entry->connector_id);
                    }
                    if ((fdb_entry->id == 0) && (fdb_entry->type == GROUP0TYPE_CLIMATEZONE))
                    {
                        fdb_entry->active = true;
                        LOG(D, "Active: %d", fdb_entry->active);
                    }

                    /* Insert the cache entry in the cache list */
                    LIST_INSERT_HEAD(&feat_db_cache, fdb_entry, list_node);
                }
            }
        }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        err = nvs_entry_next(&it);
#else
        it = nvs_entry_next(it);
#endif
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    nvs_release_iterator(it);
#endif
    xSemaphoreGiveRecursive(feat_db_mutex);
}

int32_t feat_db_cache_entry_create(GROUP0TYPE_ENUM group_type, uint8_t connector_id, int32_t id, int32_t enable, int32_t active, int32_t *group_class_instance)
{
    feature_database_t *fdb_entry = NULL;
    int32_t err = ESP_OK;

    TRUE_CHECK(xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY));
    // Allocate memory for the cache entry
    fdb_entry = hal_mem_malloc_prefer(sizeof(feature_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (fdb_entry == NULL)
    {
        err = FEAT_DB_ERR_MEM_ALLOC_FAILED;
    }
    else
    {
        memset(fdb_entry, 0, sizeof(feature_database_t));

        fdb_entry->list_node.le_next = NULL;
        fdb_entry->list_node.le_prev = NULL;

        /* Proceed with updating the cache entry */
        fdb_entry->type = group_type;
        if (id == INVALID_GROUP_ID)
        {
            fdb_entry->id = feat_db_find_next_id();
        }
        else
        {
            fdb_entry->id = id;
        }
        fdb_entry->active = active;
        fdb_entry->enable = enable;

        uint32_t device_class = GROUP0;
        int32_t class_instance = INVALID_CLASS_INSTANCE;
        class_instance = broker_register_instance(&device_class, connector_id);
        if (class_instance == INVALID_CLASS_INSTANCE)
        {
            LOG(E, "Registration failed for GROUP class");
            hal_mem_free(fdb_entry);
            err = FEAT_DB_ERR_INVALID_CLASS_INSTANCE;
        }
        else
        {
            fdb_entry->class_instance = class_instance;
            fdb_entry->connector_id = connector_id;

            char uid[DICM_UID_KEY_STR_LEN] = {0};
            int32_t ret = dicm_generate_uid_key_str(uid, sizeof(uid));
            /* The UID is 15 characters + 1 NULL terminator */
            if (ret != DICM_UID_KEY_STR_LEN - 1)
            {
                LOG(E, "Cannot generate UID for GROUP class");
                hal_mem_free(fdb_entry);
                err = FEAT_DB_ERR_UID_NOT_GENERATED;
            }
            else
            {
                strncpy(fdb_entry->uid, uid, DICM_UID_KEY_STR_LEN - 1);
                if (group_type == GROUP0TYPE_CLIMATEZONE)
                {
                    if (id != 0)
                    {
                        strcpy(fdb_entry->name, DEFAULT_CLIMATE_ZONE_GROUP_NAME);
                        snprintf(fdb_entry->name + strlen(fdb_entry->name), MAX_GROUP_NAME_LENGTH, "%d", fdb_entry->id);
                    }
                    else
                    {
                        strcpy(fdb_entry->name, CLIMATE_ZONE_GROUP_FEATURE_NAME);
                    }
                }
                else if (group_type == GROUP0TYPE_SMARTECO)
                {
                    if (id != 0)
                    {
                        strcpy(fdb_entry->name, DEFAULT_SMART_ECO_GROUP_NAME);
                        snprintf(fdb_entry->name + strlen(fdb_entry->name), MAX_GROUP_NAME_LENGTH, "%d", fdb_entry->id);
                    }
                    else
                    {
                        strcpy(fdb_entry->name, SMART_ECO_GROUP_FEATURE_NAME);
                    }
                }
                else if (group_type == GROUP0TYPE_MOBILECOOLER)
                {
                    strcpy(fdb_entry->name, DEFAULT_MOBILE_COOLER_GROUP_NAME);
                }

                LIST_INSERT_HEAD(&feat_db_cache, fdb_entry, list_node);
                feat_db_update_nvs(fdb_entry);
                *group_class_instance = fdb_entry->class_instance;
            }
        }
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
    return err;
}

void feat_db_cache_entry_delete(int32_t class_instance)
{
    feature_database_t *fdb_entry = NULL;

    TRUE_CHECK(xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY));
    fdb_entry = feat_db_find_by_class_instance(class_instance);
    if (fdb_entry != NULL)
    {
        // De-register the GROUP instance
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, GROUP0 | DDM2_PARAMETER_INSTANCE(class_instance), &Zero, sizeof(Zero), fdb_entry->connector_id, portMAX_DELAY);
        // Erase key
        nvs_erase_key(nvs_feat_db, fdb_entry->uid);
        nvs_commit(nvs_feat_db);
        // Remove from a list
        LIST_REMOVE(fdb_entry, list_node);
        hal_mem_free(fdb_entry);
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
}

static feature_database_t *feat_db_find_by_class_instance(int32_t class_instance)
{
    feature_database_t *current_node;

    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if (current_node->class_instance == class_instance)
        {
            return current_node;
        }
    }
    return NULL;
}

int32_t feat_db_find_first_by_uid_interface(const char *uid)
{
    TRUE_CHECK_RETURNX(INVALID_CLASS_INSTANCE, uid != NULL);
    int32_t ret = INVALID_CLASS_INSTANCE;
    feature_database_t *current_node = NULL;

    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        for (int32_t i = 0; i < current_node->num_interfaces_uid; i++)
        {
            if (strcmp(current_node->interface_uid[i], uid) == 0)
            {
                ret = current_node->class_instance;
                break;
            }
        }
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
    return ret;
}

int32_t feat_db_find_first_by_class_instance_interface(uint32_t class_instance)
{
    int32_t ret = INVALID_CLASS_INSTANCE;
    feature_database_t *current_node;

    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        for (int32_t i = 0; i < current_node->num_interfaces_class_inst; i++)
        {
            if (current_node->interface_class_inst[i] == class_instance)
            {
                ret = current_node->class_instance;
                break;
            }
        }
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
    return ret;
}

int32_t feat_db_find_first_by_id_and_type(int32_t id, GROUP0TYPE_ENUM group_type)
{
    int32_t ret = INVALID_CLASS_INSTANCE;
    feature_database_t *current_node;

    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if ((current_node->id == id) && (current_node->type == (int)group_type))
        {
            ret = current_node->class_instance;
            break;
        }
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
    return ret;
}

static int32_t feat_db_find_next_id(void)
{
    feature_database_t *current_node;
    int32_t id = 1;

    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if (current_node->id > id)
        {
            id = current_node->id;
        }
    }
    id++;
    xSemaphoreGiveRecursive(feat_db_mutex);
    return id;
}

static bool feat_db_cache_entry_update_interfaces(feature_database_t *fdb_entry, const void *data, size_t data_size)
{
    uint32_t group_interface_class_inst[NUM_OF_INTERFACES] = {0};
    char group_interface_uid[NUM_OF_INTERFACES][DICM_UID_KEY_STR_LEN] = {0};
    bool update_nvs = false;
    memcpy(group_interface_class_inst, fdb_entry->interface_class_inst, fdb_entry->num_interfaces_class_inst * sizeof(uint32_t));
    memcpy(group_interface_uid, fdb_entry->interface_uid, fdb_entry->num_interfaces_uid * DICM_UID_KEY_STR_LEN);
    UPDLINKEDCLASS_T *p_update_interface = (UPDLINKEDCLASS_T *)data;
    bool add_class = true;  // Default is true

    // Find the UID of the specific PROD or GROUP interface
    char uid_interface[DICM_UID_KEY_STR_LEN];
    switch (DDM2_PARAMETER_CLASS(p_update_interface->updclass))
    {
    case PROD0:
    {
        size_t uid_interface_size = DICM_UID_KEY_STR_LEN;
        ProdDBReadCache(FIELD_UID, DDM2_PARAMETER_INSTANCE_FIELD(p_update_interface->updclass), uid_interface, &uid_interface_size);  // returns null terminated
        if (uid_interface_size == 0)
        {
            LOG(W, "Could not read uid of PROD%d. Can only remove from interface_class_inst", DDM2_PARAMETER_INSTANCE_FIELD(p_update_interface->updclass));
        }
    }
    break;
    case GROUP0:
    {
        feature_database_t *fdb_upd_entry = feat_db_find_by_class_instance(DDM2_PARAMETER_INSTANCE_FIELD(p_update_interface->updclass));
        memcpy(uid_interface, fdb_upd_entry->uid, DICM_UID_KEY_STR_LEN);
    }
    break;
    default:
        break;
    }

    // Check length of value to see if extra param is used to add or remove the param. If this is missing, adding is assumed.
    if (data_size > sizeof(UPDLINKEDCLASS_T))
    {
        add_class = (bool)p_update_interface->update[0];
    }
    if (add_class)
    {
        // Verify that this is a PROD or GROUP instance that already exists (there is a valid UID)
        if (strlen(uid_interface) == DICM_UID_KEY_STR_LEN - 1)
        {
            // Check whether the UID exists in the UID interface of the group instance, so we only need to update class interface
            bool uid_exists = false;
            bool class_inst_exists = false;
            for (int32_t i = 0; i < fdb_entry->num_interfaces_uid; i++)
            {
                if (strcmp(fdb_entry->interface_uid[i], uid_interface) == 0)
                {
                    // UID exsists, update only class instance
                    uid_exists = true;
                    break;
                }
            }

            for (int32_t i = 0; i < fdb_entry->num_interfaces_class_inst; i++)
            {
                if (fdb_entry->interface_class_inst[i] == p_update_interface->updclass)
                {
                    // Class instance exsists as well, do not update anything
                    class_inst_exists = true;
                    break;
                }
            }

            if ((uid_exists) && (!class_inst_exists))
            {

                if (fdb_entry->num_interfaces_class_inst < NUM_OF_INTERFACES)
                {
                    // We only need to update the class interface (on subsequent boots)
                    LOG(D, "Num class interfaces before adding: %d", fdb_entry->num_interfaces_class_inst);
                    group_interface_class_inst[fdb_entry->num_interfaces_class_inst] = p_update_interface->updclass;
                    LOG(D, "Interface class inst added: %x", group_interface_class_inst[fdb_entry->num_interfaces_class_inst]);
                    fdb_entry->num_interfaces_class_inst++;
                    LOG(D, "Num class interfaces after adding: %d", fdb_entry->num_interfaces_class_inst);
                }
                update_nvs = false;
            }
            else if ((!uid_exists) && (!class_inst_exists))
            {
                if ((fdb_entry->num_interfaces_class_inst < NUM_OF_INTERFACES) && (fdb_entry->num_interfaces_uid < NUM_OF_INTERFACES))
                {
                    // We need to update both UID and class interfaces
                    LOG(D, "Num UID interfaces before adding: %d", fdb_entry->num_interfaces_uid);
                    LOG(D, "Num class interfaces before adding: %d", fdb_entry->num_interfaces_class_inst);

                    memcpy(group_interface_uid[fdb_entry->num_interfaces_class_inst], uid_interface, DICM_UID_KEY_STR_LEN);
                    group_interface_class_inst[fdb_entry->num_interfaces_class_inst] = p_update_interface->updclass;

                    LOG(D, "Interface UID added: %s", fdb_entry->interface_uid[fdb_entry->num_interfaces_uid]);
                    LOG(D, "Interface class inst added: %x", group_interface_class_inst[fdb_entry->num_interfaces_class_inst]);
                    fdb_entry->num_interfaces_uid++;
                    fdb_entry->num_interfaces_class_inst++;

                    LOG(D, "Num UID interfaces after adding: %d", fdb_entry->num_interfaces_uid);
                    LOG(D, "Num class interfaces after adding: %d", fdb_entry->num_interfaces_class_inst);
                }
                update_nvs = true;
            }
        }
    }
    else
    {
        // Find which one to remove
        LOG(D, "Num UID interfaces before removing %d", fdb_entry->num_interfaces_uid);
        LOG(D, "Num class interfaces before removing %d", fdb_entry->num_interfaces_class_inst);
        if (strlen(uid_interface) == DICM_UID_KEY_STR_LEN - 1)
        {
            for (uint8_t i = 0; i < fdb_entry->num_interfaces_uid; ++i)
            {
                if (strcmp(fdb_entry->interface_uid[i], uid_interface) == 0)
                {
                    // Remove this entry
                    memmove(&group_interface_uid[i], &group_interface_uid[i + 1], sizeof(group_interface_uid[0][0] * (fdb_entry->num_interfaces_uid - i - 1)));
                    --fdb_entry->num_interfaces_uid;
                    break;
                }
            }
        }
        for (uint8_t j = 0; j < fdb_entry->num_interfaces_class_inst; ++j)
        {
            if (group_interface_class_inst[j] == p_update_interface->updclass)
            {
                // Remove this entry
                memmove(&group_interface_class_inst[j], &group_interface_class_inst[j + 1], sizeof(group_interface_class_inst[0] * (fdb_entry->num_interfaces_class_inst - j - 1)));
                --fdb_entry->num_interfaces_class_inst;
                break;
            }
        }
        LOG(D, "Num UID interfaces after removing %d", fdb_entry->num_interfaces_uid);
        LOG(D, "Num class interfaces after removing %d", fdb_entry->num_interfaces_class_inst);
        update_nvs = true;
    }
    memcpy(fdb_entry->interface_class_inst, group_interface_class_inst, fdb_entry->num_interfaces_class_inst * sizeof(uint32_t));
    memcpy(fdb_entry->interface_uid, group_interface_uid, fdb_entry->num_interfaces_uid * DICM_UID_KEY_STR_LEN);
    return update_nvs;
}

static void feat_db_cache_entry_update_rules(feature_database_t *fdb_entry, const void *data, size_t data_size)
{
    uint32_t group_interface_rules[NUM_OF_INTERFACES] = {0};

    memcpy(group_interface_rules, fdb_entry->rules, fdb_entry->num_rules * sizeof(uint32_t));
    UPDLINKEDCLASS_T *p_update_rules = (UPDLINKEDCLASS_T *)data;
    bool add_class = true;  // Default is true

    // Check length of value to see if extra param is used to add or remove the param. If this is missing, adding is assumed.
    if (data_size > sizeof(UPDLINKEDCLASS_T))
    {
        add_class = (bool)p_update_rules->update[0];
    }
    if (add_class)
    {
        group_interface_rules[fdb_entry->num_rules++] = p_update_rules->updclass;
    }
    else
    {
        // Find which one to remove
        for (uint8_t i = 0; i < fdb_entry->num_rules; ++i)
        {
            if (group_interface_rules[i] == p_update_rules->updclass)
            {
                // Remove this entry
                memmove(&group_interface_rules[i], &group_interface_rules[i + 1], sizeof(group_interface_rules[0] * (fdb_entry->num_rules - i - 1)));
                --fdb_entry->num_rules;
                break;
            }
        }
    }
    memcpy(fdb_entry->rules, group_interface_rules, fdb_entry->num_rules * sizeof(uint32_t));
}

void feat_db_update_interface_all(const UPDLINKEDCLASS_T *data, size_t data_size, GROUP0TYPE_ENUM group_type)
{
    TRUE_CHECK_RETURN(data != NULL)
    feature_database_t *current_node;
    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if ((current_node->id != 0) && (current_node->type == (int)group_type))
        {
            feat_db_update_cache(data, data_size, FEAT_DB_FIELD_INTERFACE_CLASS_INST, current_node->class_instance);
        }
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
}

void feat_db_get_all_active_ids_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type)
{
    TRUE_CHECK_RETURN(data != NULL);
    feature_database_t *current_node;
    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    size_t num_inst = 0;
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if ((current_node->active == GROUP0ACTIVE_ON) && (current_node->type == (int)group_type) && (current_node->id != 0))
        {
            if (num_inst <= (*size - 1))
            {
                data[num_inst] = current_node->id;
                num_inst++;
            }
            else
            {
                break;
            }
        }
    }
    *size = num_inst;
    xSemaphoreGiveRecursive(feat_db_mutex);
}

void feat_db_get_all_active_instances_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type)
{
    TRUE_CHECK_RETURN(data != NULL);
    feature_database_t *current_node;
    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    size_t num_inst = 0;
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if ((current_node->active == GROUP0ACTIVE_ON) && (current_node->type == (int)group_type) && (current_node->id != 0))
        {
            if (num_inst <= (*size - 1))
            {
                data[num_inst] = current_node->class_instance;
                num_inst++;
            }
            else
            {
                break;
            }
        }
    }
    *size = num_inst;
    xSemaphoreGiveRecursive(feat_db_mutex);
}

void feat_db_get_all_enabled_ids_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type)
{
    TRUE_CHECK_RETURN(data != NULL);
    feature_database_t *current_node;
    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    size_t num_inst = 0;
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if ((current_node->enable == GROUP0ENABLE_ON) && (current_node->type == (int)group_type) && (current_node->id != 0))
        {
            if (num_inst <= (*size - 1))
            {
                data[num_inst] = current_node->id;
                num_inst++;
            }
            else
            {
                break;
            }
        }
    }
    *size = num_inst;
    xSemaphoreGiveRecursive(feat_db_mutex);
}

void feat_db_get_all_enabled_instances_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type)
{
    TRUE_CHECK_RETURN(data != NULL);
    feature_database_t *current_node;
    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    size_t num_inst = 0;
    LIST_FOREACH(current_node, &feat_db_cache, list_node)
    {
        if ((current_node->enable == GROUP0ENABLE_ON) && (current_node->type == (int)group_type) && (current_node->id != 0))
        {
            if (num_inst <= (*size - 1))
            {
                data[num_inst] = current_node->class_instance;
                num_inst++;
            }
            else
            {
                break;
            }
        }
    }
    *size = num_inst;
    xSemaphoreGiveRecursive(feat_db_mutex);
}

void feat_db_get_all_by_class_instance_interfaces_of_type(uint32_t *data, int32_t *outdata, size_t *size, GROUP0TYPE_ENUM group_type)
{
    feature_database_t *current_node;
    size_t num_matches = 0;
    if ((data == NULL) || (size == NULL))
    {
        LOG(E, "No valid input");
        if (size != NULL)
        {
            *size = 0;
        }
        return;
    }
    xSemaphoreTakeRecursive(feat_db_mutex, portMAX_DELAY);
    for (size_t j = 0; j < *size; ++j)
    {
        LIST_FOREACH(current_node, &feat_db_cache, list_node)
        {
            if (current_node->type == (int32_t)group_type)
            {

                for (int32_t i = 0; i < current_node->num_interfaces_class_inst; i++)
                {
                    // check if class instance is referenced
                    if (current_node->interface_class_inst[i] == data[j])
                    {
                        if (outdata)
                        {
                            outdata[num_matches++] = current_node->class_instance;
                        }
                    }
                }
            }
        }
    }
    if (size != NULL)
    {
        *size = num_matches;
    }
    xSemaphoreGiveRecursive(feat_db_mutex);
    return;
}
