/**
 * \brief Product database library, stores, caches and handles DDMP2 frames
 *        related to PROD<X> classes.
 */

// System includes
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/queue.h>
// IDF includes
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
// Framework
#include "broker.h"
#include "connector.h"
#include "ddm2_parameter_list.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#include "product_conf_manager.h"
#include "product_database.h"

#define NVS_PROD_DB_NAMESPACE "prod_db"

typedef struct ProdClassDesc
{
    int32_t reset;
    prod_reset_handler_t reset_handle;
    prod_indicate_handler_t indicate_handle;
    int ddm_instance;
    uint8_t connector_id;
    char name[PROD_DB_MAX_FIELD_SIZE];
    char sn[PROD_DB_MAX_FIELD_SIZE];
    char sku[PROD_DB_MAX_FIELD_SIZE];
    char pnc[PROD_DB_MAX_FIELD_SIZE];
    char fwver[PROD_DB_MAX_FIELD_SIZE];
    char hwver[PROD_DB_MAX_FIELD_SIZE];
    char mdl[PROD_DB_MAX_FIELD_SIZE];
    char ean[PROD_DB_MAX_FIELD_SIZE];
    char desc[PROD_DB_MAX_FIELD_SIZE];
    char manuf[PROD_DB_MAX_FIELD_SIZE];
    char fwid[PROD_DB_MAX_FIELD_SIZE];
    char uid[PROD_DB_MAX_FIELD_SIZE];
    uint32_t clist[PROD_DB_MAX_FIELD_SIZE];
    PROD0PROP_T *prop;
    uint8_t no_of_linked_classes;
    uint8_t no_of_linked_prop_classes;
    ProdDBMetaData_t metadata;
    int32_t defined_type;
    LIST_ENTRY(ProdClassDesc) list_node;
} ProdClassDesc_t;

static void ProdDBProdClassNodeStrParamsUpdate(ProdClassDesc_t *prod_class_node, const char *param, ProdClassDescField_t field);
static void ProdDBProdClassNodeStructSAFieldUpdate(ProdClassDesc_t *prod_class_node, uint8_t sa);
static void ProdDBProdClassNodeStructTypeFieldUpdate(ProdClassDesc_t *prod_class_node, uint8_t type);
static void ProdDBProdClassNodeStructInstFieldUpdate(ProdClassDesc_t *prod_class_node, uint8_t instance);
static void ProdDBProdClassNodeUpdLinkedClassUpdate(ProdClassDesc_t *prod_node, const void *data, size_t data_size);
static void ProdDBProdClassNodeStructClassesFieldUpdate(ProdClassDesc_t *prod_class_node, const void *data, size_t data_size);
static ProdClassDesc_t *ProdDBProdClassNodeFindByDdmInstance(uint8_t ddm_instance);
static ProdClassDesc_t *ProdDBProdClassNodeFindByProdField(const void *prod_field, ProdClassDescField_t field);
static int ProdDBUpdateNVS(const char *prod_field, ProdClassDescField_t field, const char *uid);
static void ProdDBLoadCache(void);

typedef LIST_HEAD(list_head_prod_class, ProdClassDesc) list_prod_class_t;

static nvs_handle_t nvs_prod_db;

static SemaphoreHandle_t prod_db_mutex;

static EXT_RAM_ATTR list_prod_class_t prod_db_cache;

int ProdDBInit(void)
{
    static bool is_pd_initialized = false;
    esp_err_t err = ESP_OK;
    if (!is_pd_initialized)
    {
        is_pd_initialized = true;
        prod_db_mutex = xSemaphoreCreateRecursiveMutex();
        if (prod_db_mutex == NULL)
        {
            LOG(E, "Product database mutex cannot be created");
            err = PROD_DB_ERR_MUTEX_NOT_CREATED;
        }
        if (err == ESP_OK)
        {
            err = nvs_open(NVS_PROD_DB_NAMESPACE, NVS_READWRITE, &nvs_prod_db);
            if (err == ESP_OK)
            {
                LIST_INIT(&prod_db_cache);
                ProdDBLoadCache();
                LOG(I, "Product database initialized.");
            }
            else
            {
                is_pd_initialized = false;
                vSemaphoreDelete(prod_db_mutex);
                LOG(E, "Product database not initialized.");
            }
        }
    }
    else
    {
        err = ESP_OK;
    }
    return err;
}

void ProdDBDeInit(void)
{
    nvs_close(nvs_prod_db);
    vSemaphoreDelete(prod_db_mutex);
    LOG(I, "Product database deinitialized.");
}

int ProdDBFrameHandler(const DDMP2_FRAME *const p_frame)
{
    int frame_handled = 0;

    if (p_frame == NULL)
    {
        return 0;
    }

    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));

    ProdClassDesc_t *prod_node;

    switch (p_frame->frame.control)
    {
    case DDMP2_CONTROL_SUBSCRIBE:
        if (DDM2_PARAMETER_CLASS(p_frame->frame.subscribe.parameter) == PROD0)
        {
            const uint8_t Instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
            prod_node = ProdDBProdClassNodeFindByDdmInstance(Instance);

            if (prod_node == NULL)
            {
                return 0;
            }

            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
            {
            case PROD0NAME:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->name, strlen(prod_node->name), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0SN:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->sn, strlen(prod_node->sn), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0SKU:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->sku, strlen(prod_node->sku), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0PNC:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->pnc, strlen(prod_node->pnc), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0FWVER:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->fwver, strlen(prod_node->fwver), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0HWVER:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->hwver, strlen(prod_node->hwver), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0MDL:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->mdl, strlen(prod_node->mdl), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0EAN:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->ean, strlen(prod_node->ean), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0DESCRIPTION:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->desc, strlen(prod_node->desc), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0CLIST:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->clist, prod_node->no_of_linked_classes * sizeof(uint32_t), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0MANUF:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->manuf, strlen(prod_node->manuf), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0PROP:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->prop, sizeof(PROD0PROP_T) + prod_node->no_of_linked_prop_classes * sizeof(uint32_t), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0UID:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->uid, strlen(prod_node->uid), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0RESET:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &(prod_node->reset), sizeof(prod_node->reset), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            case PROD0FWID:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, prod_node->fwid, strlen(prod_node->fwid), p_frame->destination_connector, portMAX_DELAY);
                frame_handled = 1;
                break;
            default:
                break;
            }
        }
        break;
    case DDMP2_CONTROL_SET:
        if (DDM2_PARAMETER_CLASS(p_frame->frame.set.parameter) == PROD0)
        {
            char prod_field[PROD_DB_MAX_FIELD_SIZE] = {0};
            const int Instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
            prod_node = ProdDBProdClassNodeFindByDdmInstance(Instance);

            if (prod_node == NULL)
            {
                return 0;
            }

            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case PROD0NAME:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_NAME, Instance);
                frame_handled = 1;
                break;
            case PROD0SN:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_SN, Instance);
                frame_handled = 1;
                break;
            case PROD0SKU:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_SKU, Instance);
                frame_handled = 1;
                break;
            case PROD0PNC:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_PNC, Instance);
                frame_handled = 1;
                break;
            case PROD0FWVER:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_FWVER, Instance);
                frame_handled = 1;
                break;
            case PROD0HWVER:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_HWVER, Instance);
                frame_handled = 1;
                break;
            case PROD0MDL:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_MDL, Instance);
                frame_handled = 1;
                break;
            case PROD0EAN:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_EAN, Instance);
                frame_handled = 1;
                break;
            case PROD0CLIST:
                ProdDBUpdateCache(p_frame->frame.set.value.raw, ddmp2_value_size(p_frame), FIELD_CLIST, Instance);
                frame_handled = 1;
                break;
            case PROD0MANUF:
                ddmp2_extract_string_from_frame(p_frame, prod_field, sizeof(prod_field));
                ProdDBUpdateCache(prod_field, strlen(prod_field), FIELD_MANUF, Instance);
                frame_handled = 1;
                break;
            case PROD0RESET:
                ProdDBUpdateCache(&(p_frame->frame.set.value.int32), sizeof(int32_t), FIELD_RESET, Instance);
                frame_handled = 1;
                break;
            case PROD0IND:
                ProdDBUpdateCache(&(p_frame->frame.set.value.int32), sizeof(int32_t), FIELD_INDICATE, Instance);
                frame_handled = 1;
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }

    xSemaphoreGiveRecursive(prod_db_mutex);

    return frame_handled;
}

static int ProdDBUpdateNVS(const char *prod_field, ProdClassDescField_t field, const char *uid)
{
    esp_err_t err = -1;
    size_t required_size = 0;
    TRUE_CHECK_RETURNX(PROD_DB_ERR_INVALID_DATA, prod_field != NULL);
    TRUE_CHECK_RETURNX(PROD_DB_ERR_INVALID_DATA, uid != NULL);

    prod_database_t *pd_entry = NULL;
    err = nvs_get_blob(nvs_prod_db, uid, NULL, &required_size);

    if (err == ESP_OK)
    {
        void *blob = hal_mem_malloc_prefer(required_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        TRUE_CHECK_RETURNX(PROD_DB_ERR_MEM_ALLOC_FAILED, blob != NULL);
        pd_entry = hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (pd_entry != NULL)
        {
            memset(pd_entry, 0, sizeof(prod_database_t));
            size_t offset = 0;
            memset(blob, 0, required_size);
            err = nvs_get_blob(nvs_prod_db, uid, blob, &required_size);
            if (err == ESP_OK)
            {
                strncpy(pd_entry->uid, uid, DICM_UID_KEY_STR_LEN - 1);
                strncpy(pd_entry->mdl, (char *)(blob + offset), PROD_DB_MAX_FIELD_SIZE - 1);
                offset += strlen(pd_entry->mdl) + 1;
                LOG(D, "model: %s, offset: %d", pd_entry->mdl, offset);
                strncpy(pd_entry->name, (char *)(blob + offset), PROD_DB_MAX_FIELD_SIZE - 1);
                offset += strlen(pd_entry->name) + 1;
                LOG(D, "name: %s, offset: %d", pd_entry->name, offset);
                strncpy(pd_entry->sn, (char *)(blob + offset), PROD_DB_MAX_FIELD_SIZE - 1);
                offset += strlen(pd_entry->sn) + 1;
                LOG(D, "sn: %s, offset: %d", pd_entry->sn, offset);
                // Copy each string from the blob into the structure's fields
                // Ensure the strings don't exceed the maximum size defined for each field
                if (field == FIELD_MDL)
                {
                    strncpy(pd_entry->mdl, prod_field, PROD_DB_MAX_FIELD_SIZE - 1);
                }
                else if (field == FIELD_NAME)
                {
                    strncpy(pd_entry->name, prod_field, PROD_DB_MAX_FIELD_SIZE - 1);
                }
                else if (field == FIELD_SN)
                {
                    strncpy(pd_entry->sn, prod_field, PROD_DB_MAX_FIELD_SIZE - 1);
                }

                size_t data_to_set_size = strlen(pd_entry->mdl) + strlen(pd_entry->name) + strlen(pd_entry->sn) + 3;
                uint8_t *blob_data_to_set = hal_mem_malloc_prefer(data_to_set_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (blob_data_to_set != NULL)
                {
                    memset(blob_data_to_set, 0, data_to_set_size);
                    memcpy(blob_data_to_set, pd_entry->mdl, strlen(pd_entry->mdl));
                    memcpy(blob_data_to_set + strlen(pd_entry->mdl) + 1, pd_entry->name, strlen(pd_entry->name));
                    memcpy(blob_data_to_set + strlen(pd_entry->mdl) + 1 + strlen(pd_entry->name) + 1, pd_entry->sn, strlen(pd_entry->sn));

                    err = nvs_set_blob(nvs_prod_db, uid, blob_data_to_set, data_to_set_size);
                    err = nvs_commit(nvs_prod_db);
                    hal_mem_free(blob_data_to_set);
                }
                else
                {
                    err = PROD_DB_ERR_MEM_ALLOC_FAILED;
                }
            }
            hal_mem_free(pd_entry);
        }
        else
        {
            err = PROD_DB_ERR_MEM_ALLOC_FAILED;
        }
        hal_mem_free(blob);
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        prod_database_t *pd_entry = NULL;
        uint8_t *blob_data_to_set = NULL;
        size_t offset = 0;
        offset = sizeof(pd_entry->mdl) + sizeof(pd_entry->name) + sizeof(pd_entry->sn);
        blob_data_to_set = hal_mem_malloc_prefer(offset, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        TRUE_CHECK_RETURNX(PROD_DB_ERR_MEM_ALLOC_FAILED, blob_data_to_set != NULL);
        pd_entry = hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (pd_entry != NULL)
        {
            memset(blob_data_to_set, 0, offset);
            memset(pd_entry, 0, sizeof(prod_database_t));
            strncpy(pd_entry->uid, uid, DICM_UID_KEY_STR_LEN - 1);
            if (field == FIELD_MDL)
            {
                strncpy(pd_entry->mdl, prod_field, PROD_DB_MAX_FIELD_SIZE - 1);
            }

            if (field == FIELD_NAME)
            {
                strncpy(pd_entry->name, prod_field, PROD_DB_MAX_FIELD_SIZE - 1);
            }

            if (field == FIELD_SN)
            {
                strncpy(pd_entry->sn, prod_field, PROD_DB_MAX_FIELD_SIZE - 1);
            }

            memcpy(blob_data_to_set, pd_entry->mdl, strlen(pd_entry->mdl) + 1);
            memcpy(blob_data_to_set + strlen(pd_entry->mdl) + 1, pd_entry->name, strlen(pd_entry->name));
            memcpy(blob_data_to_set + strlen(pd_entry->mdl) + 1 + strlen(pd_entry->name) + 1, pd_entry->sn, strlen(pd_entry->sn));

            err = nvs_set_blob(nvs_prod_db, uid, blob_data_to_set, offset);
            if (err == ESP_OK)
            {
                err = nvs_commit(nvs_prod_db);
                if (err != ESP_OK)
                {
                    LOG(E, "Cannot commit to nvs_prod_db %d", err);
                }
            }
            else
            {
                LOG(E, "Cannot write to nvs_prod_db %d", err);
            }
            hal_mem_free(pd_entry);
        }
        else
        {
            err = PROD_DB_ERR_MEM_ALLOC_FAILED;
        }
        hal_mem_free(blob_data_to_set);
    }
    return err;
}

bool ProdDBUpdateCache(const void *prod_field, size_t prod_field_size, ProdClassDescField_t field, int ddm_instance)
{
    ProdClassDesc_t *prod_node = NULL;
    int err = -1;
    uint32_t param_id = 0;
    void *value_copy_ptr = NULL;
    size_t value_size = 0;
    bool publish = false;
    bool updated = false;

    // Try and find the product instance by ddm instance, and if so, proceed with updating the product field
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));

    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);

    if (prod_node != NULL)
    {
        if (prod_field != NULL)
        {
            switch (field)
            {
            case FIELD_SN:
                if (strcmp((char *)prod_field, prod_node->sn) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    err = ProdDBUpdateNVS((char *)prod_field, FIELD_SN, prod_node->uid);
                    if (err != ESP_OK)
                    {
                        LOG(E, "Cannot update NVS, error %d", err);
                    }
                    LOG(D, "Update field SN %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0SN;
                value_copy_ptr = prod_node->sn;
                value_size = strlen(prod_node->sn);
                break;
            case FIELD_NAME:
                if (strcmp((char *)prod_field, prod_node->name) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    err = ProdDBUpdateNVS((char *)prod_field, FIELD_NAME, prod_node->uid);
                    if (err != ESP_OK)
                    {
                        LOG(E, "Cannot update NVS, error %d", err);
                    }
                    LOG(D, "Update field NAME %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0NAME;
                value_copy_ptr = prod_node->name;
                value_size = strlen(prod_node->name);
                break;
            case FIELD_MDL:
                if (strcmp((char *)prod_field, prod_node->mdl) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    err = ProdDBUpdateNVS((char *)prod_field, FIELD_MDL, prod_node->uid);
                    if (err != ESP_OK)
                    {
                        LOG(E, "Cannot update NVS, error %d", err);
                    }
                    LOG(D, "Update field MDL %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0MDL;
                value_copy_ptr = prod_node->mdl;
                value_size = strlen(prod_node->mdl);
                break;
            case FIELD_SKU:
                if (strcmp((char *)prod_field, prod_node->sku) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field SKU %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0SKU;
                value_copy_ptr = prod_node->sku;
                value_size = strlen(prod_node->sku);
                break;
            case FIELD_PNC:
                if (strcmp((char *)prod_field, prod_node->pnc) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field PNC %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0PNC;
                value_copy_ptr = prod_node->pnc;
                value_size = strlen(prod_node->pnc);
                break;
            case FIELD_MANUF:
                if (strcmp((char *)prod_field, prod_node->manuf) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field MANUF %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0MANUF;
                value_copy_ptr = prod_node->manuf;
                value_size = strlen(prod_node->manuf);
                break;
            case FIELD_HWVER:
                if (strcmp((char *)prod_field, prod_node->hwver) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field HWVER %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0HWVER;
                value_copy_ptr = prod_node->hwver;
                value_size = strlen(prod_node->hwver);
                break;
            case FIELD_FWVER:
                if (strcmp((char *)prod_field, prod_node->fwver) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field FWVER %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0FWVER;
                value_copy_ptr = prod_node->fwver;
                value_size = strlen(prod_node->fwver);
                break;
            case FIELD_EAN:
                if (strcmp((char *)prod_field, prod_node->ean) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field EAN %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0EAN;
                value_copy_ptr = prod_node->ean;
                value_size = strlen(prod_node->ean);
                break;
            case FIELD_DESC:
                if (strcmp((char *)prod_field, prod_node->desc) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field DESC %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0DESCRIPTION;
                value_copy_ptr = prod_node->desc;
                value_size = strlen(prod_node->desc);
                break;
            case FIELD_CLIST:
                /* If we update this field, it means that we always add or remove classes, so the changes
                for this field should always be published */
                ProdDBProdClassNodeUpdLinkedClassUpdate(prod_node, prod_field, prod_field_size);
                LOG(D, "Update field CLIST");
                publish = true;
                updated = true;
                param_id = PROD0CLIST;
                value_copy_ptr = prod_node->clist;
                value_size = prod_node->no_of_linked_classes * sizeof(uint32_t);
                break;
            case FIELD_PROP_SA:
                if (prod_node->prop->addr != *(uint8_t *)prod_field)
                {
                    ProdDBProdClassNodeStructSAFieldUpdate(prod_node, *(uint8_t *)prod_field);
                    LOG(D, "Update field PROP_SA");
                    updated = true;
                }
                param_id = PROD0PROP;
                break;
            case FIELD_PROP_TYPE:
                if (prod_node->prop->type != *(uint8_t *)prod_field)
                {
                    ProdDBProdClassNodeStructTypeFieldUpdate(prod_node, *(uint8_t *)prod_field);
                    LOG(D, "Update field PROP_TYPE");
                    updated = true;
                }
                param_id = PROD0PROP;
                break;
            case FIELD_PROP_CLASS:
                /* If we update this field, it means that we always add classes, so the changes
                for this field should always be published */
                ProdDBProdClassNodeStructClassesFieldUpdate(prod_node, prod_field, prod_field_size);
                LOG(D, "Update field PROP_CLASS");
                updated = true;
                param_id = PROD0PROP;
                break;
            case FIELD_PROP_INST:
                if (prod_node->prop->inst != *(uint8_t *)prod_field)
                {
                    ProdDBProdClassNodeStructInstFieldUpdate(prod_node, *(uint8_t *)prod_field);
                    LOG(D, "Update field PROP_INST");
                    updated = true;
                }
                param_id = PROD0PROP;
                break;
            case FIELD_PROP:
                // Used to synchronize updated PROP into a single publish
                LOG(D, "Publish field PROP");
                publish = true;
                param_id = PROD0PROP;
                value_copy_ptr = prod_node->prop;
                value_size = sizeof(PROD0PROP_T) + prod_node->no_of_linked_prop_classes * sizeof(uint32_t);
                break;
            case FIELD_RESET:
                // Only copy to cache, no direct publish
                LOG(D, "Update field RESET");
                if (prod_node->reset != *(int32_t *)prod_field)
                {
                    switch (*(int32_t *)prod_field)
                    {
                    case PROD0RESET_IDLE:
                        // fallthrough
                    case PROD0RESET_RESTART:
                        // fallthrough
                    case PROD0RESET_RESET_TO_FACTORY_SETTINGS:
                        // fallthrough
                    case PROD0RESET_RESET_TO_DEFAULT_SETTINGS:
                        // fallthrough
                    case PROD0RESET_CLEAR_FAULTS:
                        // Allowed values
                        updated = true;
                        prod_node->reset = *(int32_t *)prod_field;
                        break;
                    default:
                        break;
                    }
                    // Updated and might require more processing outside of frame handlers
                    if (updated && prod_node->reset_handle)
                    {
                        // Call external handler if exist. The handler must publish any accepted command
                        prod_node->reset_handle(prod_node->reset, ddm_instance);
                    }
                    else
                    {
                        // Not supported by "application" for this "product"
                        publish = true;
                        param_id = PROD0RESET;
                        prod_node->reset = PROD0RESET_NOT_SUPPORTED;
                        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, param_id | DDM2_PARAMETER_INSTANCE(prod_node->ddm_instance), &(prod_node->reset), sizeof(prod_node->reset), prod_node->connector_id, portMAX_DELAY);
                        // Reset to idle
                        prod_node->reset = PROD0RESET_IDLE;
                        value_copy_ptr = &(prod_node->reset);
                        value_size = sizeof(prod_node->reset);
                    }
                }
                break;
            case FIELD_INDICATE:
                publish = false;
                bool indicate = (*(int32_t *)prod_field != 0);
                if (prod_node->indicate_handle)
                {
                    prod_node->indicate_handle(indicate, ddm_instance);
                }
                break;
            case FIELD_FWID:
                if (strcmp((char *)prod_field, prod_node->fwid) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field FWID %s", (char *)prod_field);
                    publish = true;
                    updated = true;
                }
                param_id = PROD0FWID;
                value_copy_ptr = prod_node->fwid;
                value_size = strlen(prod_node->fwid);
                break;
            case FIELD_METADATA:
                LOG(D, "Update field METADATA");
                if (prod_field_size == sizeof(ProdDBMetaData_t))
                {
                    // Copy value
                    prod_node->metadata = *(ProdDBMetaData_t *)prod_field;
                }
                publish = false;
                break;
            default:
                break;
            }
            if (publish)
            {
                void *value = NULL;
                // Allocate the buffer with the correct size now that we know what's needed. Note: requesting value_size of 0 is still valid
                value = hal_mem_malloc_prefer(value_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if ((value == NULL) && (value_size > 0))
                {
                    LOG(E, "Value cannot be allocated");
                }
                else
                {
                    memcpy(value, value_copy_ptr, value_size);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, param_id | DDM2_PARAMETER_INSTANCE(prod_node->ddm_instance), value, value_size, prod_node->connector_id, portMAX_DELAY);
                    hal_mem_free(value);
                }
            }
        }
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
    return updated;
}

static void ProdDBLoadCache(void)
{
    esp_err_t err;
    size_t required_size = 0;
    nvs_iterator_t it;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    err = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_PROD_DB_NAMESPACE, NVS_TYPE_BLOB, &it);
#else
    it = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_PROD_DB_NAMESPACE, NVS_TYPE_BLOB);
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
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);  // Can omit error check if parameters are guaranteed to be non-NULL

        prod_database_t *pd_entry = NULL;
        err = nvs_get_blob(nvs_prod_db, info.key, NULL, &required_size);
        LOG(D, "Load cache blob required size %d", required_size);
        if (err == ESP_OK)
        {
            void *blob = hal_mem_malloc_prefer(required_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            if (blob)
            {
                memset(blob, 0, required_size);
                pd_entry = hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (pd_entry)
                {
                    memset(pd_entry, 0, sizeof(prod_database_t));
                    err = nvs_get_blob(nvs_prod_db, info.key, blob, &required_size);
                    if (err == ESP_OK)
                    {
                        size_t offset = 0;
                        // Copy each string from the blob into the structure's fields
                        // Ensure the strings don't exceed the maximum size defined for each field
                        strcpy(pd_entry->uid, info.key);
                        LOG(D, "UID %s", pd_entry->uid);
                        strncpy(pd_entry->mdl, (char *)(blob + offset), PROD_DB_MAX_FIELD_SIZE - 1);
                        offset += strlen(pd_entry->mdl) + 1;
                        LOG(D, "MDL %s", pd_entry->mdl);
                        strncpy(pd_entry->name, (char *)(blob + offset), PROD_DB_MAX_FIELD_SIZE - 1);
                        offset += strlen(pd_entry->name) + 1;
                        LOG(D, "NAME %s", pd_entry->name);
                        strncpy(pd_entry->sn, (char *)(blob + offset), PROD_DB_MAX_FIELD_SIZE - 1);
                        offset += strlen(pd_entry->sn) + 1;
                        LOG(D, "SN %s", pd_entry->sn);
                        int prod_class_node_inst = -1;
                        prod_class_node_inst = ProdDBProdClassNodeCreate(pd_entry, sizeof(prod_database_t), INVALID_CONNECTOR_ID);
                        if (prod_class_node_inst == INVALID_DDM_INSTANCE)
                        {
                            LOG(D, "ProdClassNode inserted into cache");
                        }
                        else
                        {
                            LOG(E, "ProdClassNodeCreate returned error %d", prod_class_node_inst);
                        }
                    }
                    hal_mem_free(pd_entry);
                }
                hal_mem_free(blob);
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
}

void ProdDBReadCache(ProdClassDescField_t field, int ddm_instance, void *data, size_t *data_size)
{
    ProdClassDesc_t *prod_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);
    if (prod_node != NULL)
    {
        if (data != NULL)
        {
            switch (field)
            {
            case FIELD_NAME:
                if ((strlen(prod_node->name) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->name, strlen(prod_node->name));
                    *data_size = strlen(prod_node->name);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_SN:
                if ((strlen(prod_node->sn) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->sn, strlen(prod_node->sn));
                    *data_size = strlen(prod_node->sn);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_SKU:
                if ((strlen(prod_node->sku) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->sku, strlen(prod_node->sku));
                    *data_size = strlen(prod_node->sku);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_PNC:
                if ((strlen(prod_node->pnc) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->pnc, strlen(prod_node->pnc));
                    *data_size = strlen(prod_node->pnc);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_FWVER:
                if ((strlen(prod_node->fwver) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->fwver, strlen(prod_node->fwver));
                    *data_size = strlen(prod_node->fwver);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_HWVER:
                if ((strlen(prod_node->hwver) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->hwver, strlen(prod_node->hwver));
                    *data_size = strlen(prod_node->hwver);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_MDL:
                if ((strlen(prod_node->mdl) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->mdl, strlen(prod_node->mdl));
                    *data_size = strlen(prod_node->mdl);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_EAN:
                if ((strlen(prod_node->ean) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->ean, strlen(prod_node->ean));
                    *data_size = strlen(prod_node->ean);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_DESC:
                if ((strlen(prod_node->desc) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->desc, strlen(prod_node->desc));
                    *data_size = strlen(prod_node->desc);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_CLIST:
                if ((prod_node->no_of_linked_classes * sizeof(uint32_t)) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->clist, prod_node->no_of_linked_classes * sizeof(uint32_t));
                    *data_size = prod_node->no_of_linked_classes * sizeof(uint32_t);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_MANUF:
                if ((strlen(prod_node->manuf) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->manuf, strlen(prod_node->manuf));
                    *data_size = strlen(prod_node->manuf);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_PROP:
                if (sizeof(PROD0PROP_T) == *data_size)
                {
                    // Ok to only request PROD0PROP_T
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->prop, sizeof(PROD0PROP_T));
                }
                else if ((sizeof(PROD0PROP_T) + prod_node->no_of_linked_prop_classes * sizeof(uint32_t)) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->prop, sizeof(PROD0PROP_T) + prod_node->no_of_linked_prop_classes * sizeof(uint32_t));
                    *data_size = sizeof(PROD0PROP_T) + prod_node->no_of_linked_prop_classes * sizeof(uint32_t);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_UID:
                if ((strlen(prod_node->uid) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->uid, strlen(prod_node->uid));
                    *data_size = strlen(prod_node->uid);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_CONN_ID:
                if (*data_size >= sizeof(uint8_t))
                {
                    memset(data, 0, *data_size);
                    *((uint8_t *)data) = prod_node->connector_id;
                    *data_size = sizeof(uint8_t);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_DDM_INST:
                if (*data_size >= sizeof(int))
                {
                    memset(data, 0, *data_size);
                    *((int *)data) = prod_node->ddm_instance;
                    *data_size = sizeof(int);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_NO_OF_LINKED_PROP_CLASSES:
                if (*data_size >= sizeof(int))
                {
                    memset(data, 0, *data_size);
                    *((int *)data) = prod_node->no_of_linked_prop_classes;
                    *data_size = sizeof(int);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_NO_OF_LINKED_CLASSES:
                if (*data_size >= sizeof(int))
                {
                    memset(data, 0, *data_size);
                    *((int *)data) = prod_node->no_of_linked_classes;
                    *data_size = sizeof(int);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_RESET:
                if (*data_size >= sizeof(int32_t))
                {
                    memset(data, 0, *data_size);
                    *((int32_t *)data) = prod_node->reset;
                    *data_size = sizeof(int32_t);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            case FIELD_FWID:
                if ((strlen(prod_node->fwid) + 1) <= *data_size)
                {
                    memset(data, 0, *data_size);
                    memcpy(data, prod_node->fwid, strlen(prod_node->fwid));
                    *data_size = strlen(prod_node->fwid);
                }
                else
                {
                    *data_size = 0;
                }
                break;
            case FIELD_METADATA:
                if (*data_size >= sizeof(ProdDBMetaData_t))
                {
                    *((ProdDBMetaData_t *)data) = prod_node->metadata;
                    *data_size = sizeof(ProdDBMetaData_t);
                }
                else
                {
                    *data_size = 0;
                }
                break;

            default:
                break;
            }
        }
    }
    else
    {
        *data_size = 0;
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
}

int ProdDBProdClassNodeCreate(const void *data, size_t data_size, uint8_t connector_id)
{
    ProdClassDesc_t *prod_class_node = NULL;
    bool is_field_valid = false;
    int ret_ddm_instance = PROD_DB_ERR_INVALID_DDM_INST;

    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    if (data != NULL)
    {
        /* Unpack data */
        prod_database_t *pd_field = (prod_database_t *)data;

        /* Search cache entries depending on which parameter is available */
        if (strlen(pd_field->sn) > 0)
        {
            prod_class_node = ProdDBProdClassNodeFindByProdField(pd_field->sn, FIELD_SN);
            is_field_valid = true;
        }
        else if (strlen(pd_field->mdl) > 0)
        {
            prod_class_node = ProdDBProdClassNodeFindByProdField(pd_field->mdl, FIELD_MDL);
            is_field_valid = true;
        }
        else if (strlen(pd_field->name) > 0)
        {
            prod_class_node = ProdDBProdClassNodeFindByProdField(pd_field->name, FIELD_NAME);
            is_field_valid = true;
        }
        /* If there is no cache entry, it means that the product is not in the cache list (on first detection or while loading
            the cache after reboot) */
        if ((prod_class_node == NULL) && (is_field_valid))
        {
            // Allocate memory for the cache entry
            prod_class_node = hal_mem_malloc_prefer(sizeof(ProdClassDesc_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            if (prod_class_node == NULL)
            {
                xSemaphoreGiveRecursive(prod_db_mutex);
                return PROD_DB_ERR_MEM_ALLOC_FAILED;
            }
            memset(prod_class_node, 0, sizeof(ProdClassDesc_t));

            /* Proceed with updating the cache entry product fields first */
            strncpy(prod_class_node->uid, pd_field->uid, PROD_DB_MAX_FIELD_SIZE - 1);
            strncpy(prod_class_node->mdl, pd_field->mdl, PROD_DB_MAX_FIELD_SIZE - 1);
            strncpy(prod_class_node->sn, pd_field->sn, PROD_DB_MAX_FIELD_SIZE - 1);
            strncpy(prod_class_node->name, pd_field->name, PROD_DB_MAX_FIELD_SIZE - 1);

            /* If a valid connector_id is provided:
                - Register a PROD instance (on first detection)
                - Generate UID for the PROD instance

                If an invalid connector_id is provided:
                - Set the ddm_instance and connector_id fields to invalid (when loading the cache after reboot). */

            if (connector_id != INVALID_CONNECTOR_ID)
            {
                uint32_t ddm_class = PROD0;
                int ddm_instance = -1;
                ddm_instance = broker_register_instance(&ddm_class, connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for PROD class");
                    hal_mem_free(prod_class_node);
                    xSemaphoreGiveRecursive(prod_db_mutex);
                    return PROD_DB_ERR_INVALID_DDM_INST;
                }
                prod_class_node->ddm_instance = ddm_instance;
                prod_class_node->connector_id = connector_id;

                /* Generate a UID for the newly registered PROD instance and update the UID field in the cache entry
                    on first detection */
                char uid[DICM_UID_KEY_STR_LEN];
                int ret = dicm_generate_uid_key_str(uid, sizeof(uid));
                /* The UID is 15 characters + 1 NULL terminator */
                if (ret == sizeof(uid) - 1)
                {
                    strncpy(prod_class_node->uid, uid, DICM_UID_KEY_STR_LEN);
                }
                else
                {
                    LOG(E, "Cannot generate UID for PROD class");
                    hal_mem_free(prod_class_node);
                    xSemaphoreGiveRecursive(prod_db_mutex);
                    return PROD_DB_ERR_UID_NOT_GENERATED;
                }
            }
            else
            {
                prod_class_node->ddm_instance = INVALID_DDM_INSTANCE;
                prod_class_node->connector_id = INVALID_CONNECTOR_ID;
            }

            prod_class_node->list_node.le_next = NULL;
            prod_class_node->list_node.le_prev = NULL;

            /* Allocate memory for the proprietary field, since it is a separate structure */
            PROD0PROP_T *prop;
            prop = hal_mem_malloc_prefer(sizeof(PROD0PROP_T), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            if (prop == NULL)
            {
                hal_mem_free(prod_class_node);
                xSemaphoreGiveRecursive(prod_db_mutex);
                return PROD_DB_ERR_MEM_ALLOC_FAILED;
            }
            memset(prop, 0, sizeof(PROD0PROP_T));
            prod_class_node->prop = prop;

            /* Insert the cache entry in the cache list */
            LIST_INSERT_HEAD(&prod_db_cache, prod_class_node, list_node);

            /* If valid connector_id is provided it means that the product is detected and registered for the first
                time, so proceed with updating the flash as well */
            if (connector_id != INVALID_CONNECTOR_ID)
            {
                int err = 0;
                err = ProdDBUpdateNVS(prod_class_node->mdl, FIELD_MDL, prod_class_node->uid);
                if (err != 0)
                {
                    ProdDBProdClassNodeDelete(prod_class_node->ddm_instance);
                    xSemaphoreGiveRecursive(prod_db_mutex);
                    return PROD_DB_ERR_CANNOT_WRITE_TO_NVS;
                }
                err = ProdDBUpdateNVS(prod_class_node->sn, FIELD_SN, prod_class_node->uid);
                if (err != 0)
                {
                    ProdDBProdClassNodeDelete(prod_class_node->ddm_instance);
                    xSemaphoreGiveRecursive(prod_db_mutex);
                    return PROD_DB_ERR_CANNOT_WRITE_TO_NVS;
                }
                err = ProdDBUpdateNVS(prod_class_node->name, FIELD_NAME, prod_class_node->uid);
                if (err != 0)
                {
                    ProdDBProdClassNodeDelete(prod_class_node->ddm_instance);
                    xSemaphoreGiveRecursive(prod_db_mutex);
                    return PROD_DB_ERR_CANNOT_WRITE_TO_NVS;
                }
            }
        }
        else if ((prod_class_node == NULL) && (!is_field_valid))
        {
            xSemaphoreGiveRecursive(prod_db_mutex);
            return PROD_DB_ERR_NO_VALID_FIELD;
        }
        else
        {
            /* There is a cache entry in the cache list (after reboot), so only a new PROD instance needs to be registered
                if it hasn't been registered already */
            if ((connector_id != INVALID_CONNECTOR_ID) && (prod_class_node->ddm_instance == INVALID_DDM_INSTANCE))
            {
                uint32_t ddm_class = PROD0;
                int ddm_instance = -1;
                ddm_instance = broker_register_instance(&ddm_class, connector_id);

                if (ddm_instance == INVALID_DDM_INSTANCE)
                {
                    LOG(E, "Registration failed for PROD0 class");
                    xSemaphoreGiveRecursive(prod_db_mutex);
                    return PROD_DB_ERR_INVALID_DDM_INST;
                }
                prod_class_node->ddm_instance = ddm_instance;
                prod_class_node->connector_id = connector_id;
                if (prod_class_node->prop == NULL)
                {
                    /* Allocate memory for the proprietary field, since it is a separate structure */
                    PROD0PROP_T *prop;
                    prop = hal_mem_malloc_prefer(sizeof(PROD0PROP_T), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                    if (prop == NULL)
                    {
                        hal_mem_free(prod_class_node);
                        xSemaphoreGiveRecursive(prod_db_mutex);
                        return PROD_DB_ERR_MEM_ALLOC_FAILED;
                    }
                    memset(prop, 0, sizeof(PROD0PROP_T));
                    prod_class_node->prop = prop;
                }
            }
            else if ((connector_id != INVALID_CONNECTOR_ID) && (prod_class_node->ddm_instance > 0))
            {
                LOG(E, "A product with same manufacturing information found. Product class cannot be registered again.");
                xSemaphoreGiveRecursive(prod_db_mutex);
                return PROD_DB_ERR_PRODUCT_ALREADY_EXISTS;
            }
        }
    }
    else
    {
        xSemaphoreGiveRecursive(prod_db_mutex);
        return PROD_DB_ERR_INVALID_DATA;
    }
    ret_ddm_instance = prod_class_node->ddm_instance;
    xSemaphoreGiveRecursive(prod_db_mutex);
    return ret_ddm_instance;
}

void ProdDBProdClassNodeDelete(int ddm_instance)
{
    ProdClassDesc_t *prod_class_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    prod_class_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);
    if (prod_class_node != NULL)
    {
        // De-register the PROD instance
        if (prod_class_node->connector_id != INVALID_CONNECTOR_ID)
        {
            // We have a owner. Nodes created by VirtualAPI will be prevented to be sent here with invalid connector_id
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, PROD0AVL | DDM2_PARAMETER_INSTANCE(ddm_instance), &Zero, sizeof(Zero), prod_class_node->connector_id, portMAX_DELAY);
        }
        // Disable node, but keep info in cache
        prod_class_node->no_of_linked_classes = 0;  // Reset
        prod_class_node->no_of_linked_prop_classes = 0;
        prod_class_node->ddm_instance = INVALID_DDM_INSTANCE;
        prod_class_node->connector_id = INVALID_CONNECTOR_ID;
        hal_mem_free(prod_class_node->prop);
        prod_class_node->prop = NULL;
#if 0
        // Remove from a list
        LIST_REMOVE(prod_class_node, list_node);
        hal_mem_free(prod_class_node->prop);
        hal_mem_free(prod_class_node);
#endif
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
}

static void ProdDBProdClassNodeStrParamsUpdate(ProdClassDesc_t *prod_class_node, const char *param, ProdClassDescField_t field)
{
    switch (field)
    {
    case FIELD_NAME:
        strncpy(prod_class_node->name, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_SN:
        strncpy(prod_class_node->sn, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_SKU:
        strncpy(prod_class_node->sku, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_PNC:
        strncpy(prod_class_node->pnc, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_FWVER:
        strncpy(prod_class_node->fwver, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_HWVER:
        strncpy(prod_class_node->hwver, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_MDL:
        strncpy(prod_class_node->mdl, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_EAN:
        strncpy(prod_class_node->ean, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_DESC:
        strncpy(prod_class_node->desc, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_MANUF:
        strncpy(prod_class_node->manuf, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;

    case FIELD_FWID:
        strncpy(prod_class_node->fwid, param, PROD_DB_MAX_FIELD_SIZE - 1);
        break;
    default:
        break;
    }
}

static void ProdDBProdClassNodeUpdLinkedClassUpdate(ProdClassDesc_t *prod_node, const void *data, size_t data_size)
{
    uint32_t prod_node_clist[PROD_DB_MAX_FIELD_SIZE] = {0};

    memcpy(prod_node_clist, prod_node->clist, prod_node->no_of_linked_classes * sizeof(uint32_t));
    UPDLINKEDCLASS_T *p_update_clist = (UPDLINKEDCLASS_T *)data;
    bool add_class = true;  // Default is true

    // Check length of value to see if extra param is used to add or remove the param. If this is missing, adding is assumed.
    if (data_size > sizeof(UPDLINKEDCLASS_T))
    {
        add_class = (bool)p_update_clist->update[0];
    }
    if (add_class)
    {
        prod_node_clist[prod_node->no_of_linked_classes++] = p_update_clist->updclass;
    }
    else
    {
        // Find which one to remove
        for (uint8_t i = 0; i < prod_node->no_of_linked_classes; ++i)
        {
            if (prod_node_clist[i] == p_update_clist->updclass)
            {
                // Remove this entry
                memmove(&prod_node_clist[i], &prod_node_clist[i + 1], (prod_node->no_of_linked_classes - i - 1) * sizeof(prod_node_clist[0]));
                --prod_node->no_of_linked_classes;
                break;
            }
        }
    }
    memcpy(prod_node->clist, prod_node_clist, prod_node->no_of_linked_classes * sizeof(uint32_t));
}

static void ProdDBProdClassNodeStructSAFieldUpdate(ProdClassDesc_t *prod_class_node, uint8_t sa)
{
    prod_class_node->prop->addr = sa;
}

static void ProdDBProdClassNodeStructTypeFieldUpdate(ProdClassDesc_t *prod_class_node, uint8_t type)
{
    prod_class_node->prop->type = type;
}

static void ProdDBProdClassNodeStructInstFieldUpdate(ProdClassDesc_t *prod_class_node, uint8_t instance)
{
    prod_class_node->prop->inst = instance;
}

static void ProdDBProdClassNodeStructClassesFieldUpdate(ProdClassDesc_t *prod_class_node, const void *data, size_t data_size)
{
    if ((prod_class_node == NULL) || (data == NULL))
    {
        return;
    }

    UPDLINKEDCLASS_T *p_update_class = (UPDLINKEDCLASS_T *)data;
    bool add_class = true;  // Default is to add

    // Check if extra parameter is provided to specify add or remove
    if (data_size > sizeof(uint32_t))
    {
        add_class = (bool)p_update_class->update[0];
    }

    if (add_class)
    {
        // Add new class - reallocate memory to accommodate new class
        PROD0PROP_T *prop_reallocated = NULL;
        prop_reallocated = hal_mem_realloc_prefer(prod_class_node->prop, sizeof(PROD0PROP_T) + (prod_class_node->no_of_linked_prop_classes + 1) * sizeof(uint32_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (prop_reallocated == NULL)
        {
            LOG(E, "Cannot reallocate memory for PRODxPROP parameter");
            return;
        }
        prod_class_node->prop = prop_reallocated;

        // Add the new class at the end
        prod_class_node->prop->classes[prod_class_node->no_of_linked_prop_classes] = p_update_class->updclass;
        prod_class_node->no_of_linked_prop_classes++;

        LOG(D, "Added class 0x%08X to PROP, total classes: %d", p_update_class->updclass, prod_class_node->no_of_linked_prop_classes);
    }
    else
    {
        // Remove class - find it first before any memory operations
        bool found = false;
        uint8_t remove_index = 0;

        for (uint8_t i = 0; i < prod_class_node->no_of_linked_prop_classes; i++)
        {
            if (prod_class_node->prop->classes[i] == p_update_class->updclass)
            {
                found = true;
                remove_index = i;
                break;
            }
        }

        if (!found)
        {
            LOG(W, "Class 0x%08X not found in PROP classes", p_update_class->updclass);
            return;
        }

        // Copy existing classes (except the removed one) to a temporary buffer
        uint8_t new_count = prod_class_node->no_of_linked_prop_classes - 1;
        uint32_t *temp_classes = NULL;

        if (new_count > 0)
        {
            temp_classes = hal_mem_malloc_prefer(new_count * sizeof(uint32_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            if (temp_classes == NULL)
            {
                LOG(E, "Cannot allocate temporary buffer for class removal");
                return;
            }

            // Copy classes before the removed one
            if (remove_index > 0)
            {
                memcpy(temp_classes, prod_class_node->prop->classes, remove_index * sizeof(uint32_t));
            }

            // Copy classes after the removed one
            if (remove_index < prod_class_node->no_of_linked_prop_classes - 1)
            {
                memcpy(&temp_classes[remove_index],
                       &prod_class_node->prop->classes[remove_index + 1],
                       (prod_class_node->no_of_linked_prop_classes - remove_index - 1) * sizeof(uint32_t));
            }
        }

        // Reallocate with new size
        PROD0PROP_T *prop_reallocated = NULL;
        size_t new_size = sizeof(PROD0PROP_T) + new_count * sizeof(uint32_t);

        prop_reallocated = hal_mem_realloc_prefer(prod_class_node->prop, new_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (prop_reallocated == NULL)
        {
            LOG(E, "Cannot reallocate memory for PRODxPROP parameter during removal");
            if (temp_classes)
            {
                hal_mem_free(temp_classes);
            }
            return;
        }
        prod_class_node->prop = prop_reallocated;

        // Copy back the classes from temporary buffer
        if ((new_count > 0) && (temp_classes != NULL))
        {
            memcpy(prod_class_node->prop->classes, temp_classes, new_count * sizeof(uint32_t));
            hal_mem_free(temp_classes);
        }

        prod_class_node->no_of_linked_prop_classes = new_count;
        LOG(D, "Removed class 0x%08X from PROP, remaining classes: %d", p_update_class->updclass, new_count);
    }
}

static ProdClassDesc_t *ProdDBProdClassNodeFindByDdmInstance(uint8_t ddm_instance)
{
    ProdClassDesc_t *current_node;

    LIST_FOREACH(current_node, &prod_db_cache, list_node)
    {
        if (current_node->ddm_instance == ddm_instance)
        {
            return current_node;
        }
    }
    return NULL;
}

static ProdClassDesc_t *ProdDBProdClassNodeFindByProdField(const void *prod_field, ProdClassDescField_t field)
{
    ProdClassDesc_t *ret_node = NULL;
    ProdClassDesc_t *current_node;

    LIST_FOREACH(current_node, &prod_db_cache, list_node)
    {
        switch (field)
        {
        case FIELD_MDL:
            if (strcmp(current_node->mdl, (const char *)prod_field) == 0)
            {
                ret_node = current_node;
            }
            break;
        case FIELD_SN:
            if (strcmp(current_node->sn, (const char *)prod_field) == 0)
            {
                ret_node = current_node;
            }
            break;
        case FIELD_NAME:
            if (strcmp(current_node->name, (const char *)prod_field) == 0)
            {
                ret_node = current_node;
            }
            break;
        case FIELD_PROP_SA:
            if (current_node->prop && (current_node->prop->addr == *(uint8_t *)prod_field))
            {
                ret_node = current_node;
            }
            break;
        case FIELD_UID:
            if (strcmp(current_node->uid, (const char *)prod_field) == 0)
            {
                ret_node = current_node;
            }
            break;
        default:
            ret_node = NULL;
            break;
        }
    }
    return ret_node;
}

int ProdDBSearchCache(const void *prod_field, ProdClassDescField_t field)
{
    ProdClassDesc_t *prod_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    if (prod_field != NULL)
    {
        prod_node = ProdDBProdClassNodeFindByProdField(prod_field, field);
        if (prod_node != NULL)
        {
            int prod_ddm_instance = prod_node->ddm_instance;
            xSemaphoreGiveRecursive(prod_db_mutex);
            return prod_ddm_instance;
        }
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
    return INVALID_DDM_INSTANCE;
}

int ProdDBVirtualProdClassNodeCreate(const char *uid, int ddm_instance)
{
    ProdClassDesc_t *prod_class_node = NULL;
    int ret = -1;
    if (ddm_instance < 1)
    {
        LOG(E, "Invalid DDM instance %d", ddm_instance);
        ret = PROD_DB_ERR_INVALID_DDM_INST;
    }
    else
    {
        if ((strlen(uid) != (DICM_UID_KEY_STR_LEN - 1)))
        {
            LOG(E, "Invalid UID length %d", strlen(uid));
            ret = PROD_DB_ERR_INVALID_UID;
        }
        else
        {
            TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
            prod_class_node = ProdDBProdClassNodeFindByProdField(uid, FIELD_UID);
            if (prod_class_node == NULL)
            {
                // Allocate memory for the cache entry
                prod_class_node = hal_mem_malloc_prefer(sizeof(ProdClassDesc_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (prod_class_node != NULL)
                {
                    memset(prod_class_node, 0, sizeof(ProdClassDesc_t));
                    prod_class_node->ddm_instance = ddm_instance;
                    prod_class_node->connector_id = INVALID_CONNECTOR_ID;  // No connector ID for virtual PROD class

                    strncpy(prod_class_node->uid, uid, DICM_UID_KEY_STR_LEN);
                    prod_class_node->list_node.le_next = NULL;
                    prod_class_node->list_node.le_prev = NULL;

                    /* Allocate memory for the proprietary field, since it is a separate structure */
                    PROD0PROP_T *prop;
                    prop = hal_mem_malloc_prefer(sizeof(PROD0PROP_T), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                    if (prop != NULL)
                    {
                        memset(prop, 0, sizeof(PROD0PROP_T));
                        prod_class_node->prop = prop;
                        /* Insert the cache entry in the cache list */
                        LIST_INSERT_HEAD(&prod_db_cache, prod_class_node, list_node);
                        ret = ESP_OK;
                    }
                    else
                    {
                        hal_mem_free(prod_class_node);
                        ret = PROD_DB_ERR_MEM_ALLOC_FAILED;
                    }
                }
                else
                {
                    ret = PROD_DB_ERR_MEM_ALLOC_FAILED;
                }
            }
            else
            {
                LOG(D, "Virtual PROD class with UID %s already exists", uid);
                prod_class_node->ddm_instance = ddm_instance;
                if (prod_class_node->prop == NULL)
                {
                    /* Allocate memory for the proprietary field, since it is a separate structure */
                    PROD0PROP_T *prop;
                    prop = hal_mem_malloc_prefer(sizeof(PROD0PROP_T), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                    if (prop != NULL)
                    {
                        memset(prop, 0, sizeof(PROD0PROP_T));
                        prod_class_node->prop = prop;
                    }
                }
                ret = PROD_DB_ERR_PROD_CLASS_ALREADY_EXISTS;
            }
        }
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
    return ret;
}

void ProdDBVirtualUpdateCache(const void *prod_field, size_t prod_field_size, ProdClassDescField_t field, int ddm_instance)
{
    ProdClassDesc_t *prod_node = NULL;
    int err = -1;

    // Try and find the product instance by ddm instance, and if so, proceed with updating the product field
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));

    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);

    if (prod_node != NULL)
    {
        if (prod_field != NULL)
        {
            switch (field)
            {
            case FIELD_SN:
                if (strcmp((char *)prod_field, prod_node->sn) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    err = ProdDBUpdateNVS((char *)prod_field, FIELD_SN, prod_node->uid);
                    if (err != ESP_OK)
                    {
                        LOG(E, "Cannot update NVS, error %d", err);
                    }
                    LOG(D, "Update field SN %s", (char *)prod_field);
                }
                break;
            case FIELD_NAME:
                if (strcmp((char *)prod_field, prod_node->name) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    err = ProdDBUpdateNVS((char *)prod_field, FIELD_NAME, prod_node->uid);
                    if (err != ESP_OK)
                    {
                        LOG(E, "Cannot update NVS, error %d", err);
                    }
                    LOG(D, "Update field NAME %s", (char *)prod_field);
                }
                break;
            case FIELD_MDL:
                if (strcmp((char *)prod_field, prod_node->mdl) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    err = ProdDBUpdateNVS((char *)prod_field, FIELD_MDL, prod_node->uid);
                    if (err != ESP_OK)
                    {
                        LOG(E, "Cannot update NVS, error %d", err);
                    }
                    LOG(D, "Update field MDL %s", (char *)prod_field);
                }
                break;
            case FIELD_SKU:
                if (strcmp((char *)prod_field, prod_node->sku) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field SKU %s", (char *)prod_field);
                }
                break;
            case FIELD_PNC:
                if (strcmp((char *)prod_field, prod_node->pnc) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field PNC %s", (char *)prod_field);
                }
                break;
            case FIELD_MANUF:
                if (strcmp((char *)prod_field, prod_node->manuf) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field MANUF %s", (char *)prod_field);
                }
                break;
            case FIELD_HWVER:
                if (strcmp((char *)prod_field, prod_node->hwver) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field HWVER %s", (char *)prod_field);
                }
                break;
            case FIELD_FWVER:
                if (strcmp((char *)prod_field, prod_node->fwver) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field FWVER %s", (char *)prod_field);
                }
                break;
            case FIELD_EAN:
                if (strcmp((char *)prod_field, prod_node->ean) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field EAN %s", (char *)prod_field);
                }
                break;
            case FIELD_DESC:
                if (strcmp((char *)prod_field, prod_node->desc) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field DESC %s", (char *)prod_field);
                }
                break;
            case FIELD_CLIST:
                /* If we update this field, it means that we always add or remove classes, so the changes
                for this field should always be published */
                ProdDBProdClassNodeUpdLinkedClassUpdate(prod_node, prod_field, prod_field_size);
                LOG(D, "Update field CLIST");
                break;
            case FIELD_PROP_SA:
                if (prod_node->prop->addr != *(uint8_t *)prod_field)
                {
                    ProdDBProdClassNodeStructSAFieldUpdate(prod_node, *(uint8_t *)prod_field);
                    LOG(D, "Update field PROP_SA");
                }
                break;
            case FIELD_PROP_TYPE:
                if (prod_node->prop->type != *(uint8_t *)prod_field)
                {
                    ProdDBProdClassNodeStructTypeFieldUpdate(prod_node, *(uint8_t *)prod_field);
                    LOG(D, "Update field PROP_TYPE");
                }
                break;
            case FIELD_PROP_CLASS:
                /* If we update this field, it means that we always add classes, so the changes
                for this field should always be published */
                ProdDBProdClassNodeStructClassesFieldUpdate(prod_node, prod_field, prod_field_size);
                LOG(D, "Update field PROP_CLASS");
                break;
            case FIELD_PROP_INST:
                if (prod_node->prop->inst != *(uint8_t *)prod_field)
                {
                    ProdDBProdClassNodeStructInstFieldUpdate(prod_node, *(uint8_t *)prod_field);
                    LOG(D, "Update field PROP_INST");
                }
                break;
            case FIELD_RESET:
                if (prod_node->reset != *(int32_t *)prod_field)
                {
                    prod_node->reset = *(int32_t *)prod_field;
                    LOG(D, "Update field RESET");
                }
                break;
            case FIELD_FWID:
                if (strcmp((char *)prod_field, prod_node->fwid) != 0)
                {
                    ProdDBProdClassNodeStrParamsUpdate(prod_node, (char *)prod_field, field);
                    LOG(D, "Update field FWID %s", (char *)prod_field);
                }
                break;
            case FIELD_METADATA:
                if (prod_field_size == sizeof(ProdDBMetaData_t))
                {
                    // Copy value
                    prod_node->metadata = *(ProdDBMetaData_t *)prod_field;
                    LOG(I, "Update field METADATA 0x%" PRIx32, prod_node->metadata.data_u32);
                }
                break;
            default:
                break;
            }
        }
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
}

bool ProdDBProdClassNodeExists(int ddm_instance)
{
    int ret = false;
    ProdClassDesc_t *prod_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);
    if (prod_node != NULL)
    {
        ret = true;
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
    return ret;
}

void ProdDBProdClassNodeAddResetHandler(int ddm_instance, prod_reset_handler_t handler)
{
    ProdClassDesc_t *prod_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);
    if (prod_node != NULL)
    {
        prod_node->reset_handle = handler;
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
}

void ProdDBProdClassNodeAddIndicateHandler(int ddm_instance, prod_indicate_handler_t handler)
{
    ProdClassDesc_t *prod_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);
    if (prod_node != NULL)
    {
        prod_node->indicate_handle = handler;
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
}

int32_t ProdDBSetProductType(int ddm_instance, int32_t input)
{
    int32_t output_val = 0;  // Unknown
    ProdClassDesc_t *prod_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);
    if (prod_node != NULL)
    {
        // Check if we have set this already
        if (!prod_node->defined_type)
        {
            // Compare with possible content in conf_parser
            int defined_type = 0;
            output_val = input;
            if (product_conf_manager_get_model_type(prod_node->manuf, prod_node->mdl, &defined_type) == ESP_OK)
            {
                LOG(D, "Use config type %d vs input type %d", defined_type, output_val);
                output_val = defined_type;
            }
            prod_node->defined_type = output_val;
        }
        else
        {
            // Just return
            LOG(W, "Trying to reset type: %d", prod_node->defined_type);
            output_val = prod_node->defined_type;
        }
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
    return output_val;
}

int32_t ProdDBGetProductType(int ddm_instance)
{
    int32_t output_val = 0;  // Unknown
    ProdClassDesc_t *prod_node = NULL;
    TRUE_CHECK(xSemaphoreTakeRecursive(prod_db_mutex, portMAX_DELAY));
    prod_node = ProdDBProdClassNodeFindByDdmInstance(ddm_instance);
    if (prod_node != NULL)
    {
        // Just return
        output_val = prod_node->defined_type;
    }
    xSemaphoreGiveRecursive(prod_db_mutex);
    return output_val;
}

/**
 * @brief Validate if a version string is in standard semver format (major.minor.patch without leading zeros)
 *
 * @param version Version string to validate (e.g., "1.2.3")
 * @return true if valid semver format, false otherwise
 */
bool ProdDBIsValidSemverVersion(const char *version)
{
    if (version == NULL || *version == '\0')
    {
        return false;
    }

    int major = -1, minor = -1, patch = -1;
    char extra[2] = {0};  // To detect any extra characters

    // Parse the version string
    int matched = sscanf(version, "%d.%d.%d%1s", &major, &minor, &patch, extra);

    // Should match exactly 3 components, no extra characters
    if (matched != 3)
    {
        LOG(W, "Invalid semver format: %s (matched %d components)", version, matched);
        return false;
    }

    // Check for negative numbers
    if (major < 0 || minor < 0 || patch < 0)
    {
        LOG(W, "Invalid semver: negative version numbers not allowed");
        return false;
    }

    // Check for leading zeros
    char reconstructed[PROD_DB_MAX_FIELD_SIZE];
    snprintf(reconstructed, sizeof(reconstructed), "%d.%d.%d", major, minor, patch);

    if (strcmp(version, reconstructed) != 0)
    {
        LOG(W, "Invalid semver: leading zeros detected. Got '%s', expected '%s'",
            version, reconstructed);
        return false;
    }

    return true;
}
