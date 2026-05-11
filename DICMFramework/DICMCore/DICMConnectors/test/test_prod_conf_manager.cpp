#include "DICMFrameworkTestFixture.hpp"
#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>

extern "C" {
#include "cJSON.h"
#include "connector_climate_zone_feature.h"
#include "connector_smart_eco_feature.h"
#include "connector_unittest.h"
#include "esp_log.h"
#include "product_conf_manager.h"
#include "product_database.h"
}

class ProductConfManagerFixture : public DICMFrameworkTestFixture
{
  protected:
    const char *conf_dir = "/spiffs/prod_conf";
    const char *conf_file = "/spiffs/prod_conf/conf.json";

    void SetUp() override
    {
        connector_unittest_enable(NULL, NULL);
        DICMFrameworkTestFixture::SetUp();

        setConnectorId(&connector_unittest.connector_id);
        mkdir("/spiffs", 0777);
        mkdir(conf_dir, 0777);
        remove(conf_file);
        DICMFrameworkTestFixture::SetupFramework();
        esp_log_level_set("product_conf_manager.c", ESP_LOG_DEBUG);
    }

    void TearDown() override
    {
        remove(conf_file);
        DICMFrameworkTestFixture::TearDown();
    }

    void write_json_to_conf_file(cJSON *json)
    {
        FILE *f = fopen(conf_file, "w");
        ASSERT_TRUE(f != nullptr);
        char *json_str = cJSON_PrintUnformatted(json);
        fwrite(json_str, 1, strlen(json_str), f);
        fclose(f);
        cJSON_free(json_str);
    }
    void add_Dometic_manuf_item_to_array(cJSON *entry)
    {
        const char *const manuf[] = {"Dometic"};
        cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "manuf"), cJSON_CreateString(manuf[0]));
    }
    cJSON *create_basic_conf_entry()
    {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddItemToObject(entry, "manuf", cJSON_CreateArray());
        cJSON_AddItemToObject(entry, "model", cJSON_CreateArray());
        cJSON_AddItemToObject(entry, "sn", cJSON_CreateArray());
        //        cJSON_AddItemToObject(entry, "fwid", cJSON_CreateArray());
        //        cJSON_AddItemToObject(entry, "version_patterns", cJSON_CreateArray());
        return entry;
    }

    cJSON *create_version_pattern(const char *fwver_regex, const char *hwver_regex = nullptr)
    {
        cJSON *pattern = cJSON_CreateObject();
        cJSON_AddStringToObject(pattern, "fwver_regex", fwver_regex);
        if (hwver_regex != nullptr)
        {
            cJSON_AddStringToObject(pattern, "hwver_regex", hwver_regex);
        }
        return pattern;
    }

    int create_product_with_model(const char *mdl, const char *sn, const char *name)
    {
        prod_database_t *pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        EXPECT_TRUE(pd_data != NULL);
        memset(pd_data, 0, sizeof(prod_database_t));

        strncpy(pd_data->mdl, mdl, strlen(mdl));
        strncpy(pd_data->sn, sn, strlen(sn));
        strncpy(pd_data->name, name, strlen(name));

        int err = ProdDBInit();
        EXPECT_TRUE(err == ESP_OK);

        int ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id);
        return ddm_inst;
    }

    void verify_version(int ddm_inst, ProdClassDescField_t field, const char *expected_value)
    {
        char buffer[PROD_DB_MAX_FIELD_SIZE] = {0};
        size_t buffer_size = sizeof(buffer);

        ProdDBReadCache(field, ddm_inst, buffer, &buffer_size);

        if (expected_value != nullptr)
        {
            EXPECT_STREQ(buffer, expected_value) << "Expected: " << expected_value << ", Got: " << buffer;
            EXPECT_EQ(buffer_size, strlen(expected_value));
        }
    }

    void allocate_fwid_result(fwid_query_result_t *result, int32_t capacity)
    {
        result->fwids = (char **)hal_mem_malloc_prefer(sizeof(char *) * capacity, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        ASSERT_TRUE(result->fwids != nullptr);

        for (int32_t i = 0; i < capacity; i++)
        {
            result->fwids[i] = (char *)hal_mem_malloc_prefer(PRODUCT_CONF_MANAGER_MAX_FWID_LEN, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            ASSERT_TRUE(result->fwids[i] != nullptr);
        }
        result->count = capacity;  // Set capacity
    }

    void free_fwid_result(fwid_query_result_t *result, int32_t capacity)
    {
        if (result->fwids != nullptr)
        {
            for (int32_t i = 0; i < capacity; i++)
            {
                if (result->fwids[i] != nullptr)
                {
                    hal_mem_free(result->fwids[i]);
                }
            }
            hal_mem_free(result->fwids);
            result->fwids = nullptr;
        }
        result->count = 0;
    }

    void cleanup_product(int ddm_inst)
    {
        if (ddm_inst > 0)
        {
            ProdDBProdClassNodeDelete(ddm_inst);
            ProdDBDeInit();
        }
    }
};

TEST_F(ProductConfManagerFixture, FileNotFound)
{
    remove(conf_file);
    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_FILE_NOT_FOUND);
}

TEST_F(ProductConfManagerFixture, EmptyFile)
{
    FILE *f = fopen(conf_file, "w");
    ASSERT_TRUE(f != nullptr);
    fclose(f);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_INVALID_FILE_SIZE);
}

TEST_F(ProductConfManagerFixture, InvalidJSON)
{
    FILE *f = fopen(conf_file, "w");
    ASSERT_TRUE(f != nullptr);
    const char *invalid_json = "{invalid json}";
    fwrite(invalid_json, 1, strlen(invalid_json), f);
    fclose(f);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_JSON_PARSE);
}

TEST_F(ProductConfManagerFixture, RootNotArray)
{
    cJSON *root = cJSON_CreateObject();
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_INVALID_FORMAT);
}

TEST_F(ProductConfManagerFixture, EmptyArray)
{
    cJSON *root = cJSON_CreateArray();
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, MultipleLoadsSameFile)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, SingleManufacturerOnly)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
    EXPECT_TRUE(product_conf_manager_find_manufacturer("Dometic") != NULL);
    EXPECT_FALSE(product_conf_manager_find_manufacturer("Unknown") != NULL);
}

TEST_F(ProductConfManagerFixture, MultipleManufacturers)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    const char *manufacturers[] = {"GOPOWER", "Go Power!", "Dometic"};
    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    for (const char *manuf : manufacturers)
    {
        cJSON_AddItemToArray(manuf_array, cJSON_CreateString(manuf));
    }
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    for (const char *manuf : manufacturers)
    {
        EXPECT_TRUE(product_conf_manager_find_manufacturer(manuf) != NULL);
    }
}

TEST_F(ProductConfManagerFixture, SingleModelOnly)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    const char *const model[] = {"GP-DISPLAY"};
    cJSON *modelentry = cJSON_CreateObject();
    cJSON_AddItemToObject(modelentry, model[0], cJSON_CreateObject());
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelentry);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
    EXPECT_TRUE(product_conf_manager_find_model("Dometic", model[0]) != NULL);
    EXPECT_FALSE(product_conf_manager_find_model("Dometic", "Unknown") != NULL);
}

TEST_F(ProductConfManagerFixture, SingleSerialNumberOnly)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);

    const char *const sn[] = {"SN123456"};
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "sn"), cJSON_CreateString(sn[0]));
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
    EXPECT_TRUE(product_conf_manager_manufacturer_supports_sn("Dometic", sn[0]));
    EXPECT_FALSE(product_conf_manager_manufacturer_supports_sn("Dometic", "Unknown"));
}

TEST_F(ProductConfManagerFixture, AllFieldsPresent)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    const char *const manuf[] = {"Dometic"};
    const char *const model[] = {"GP-DISPLAY"};
    const char *const sn[] = {"SN123"};
    cJSON *modelentry = cJSON_CreateObject();
    cJSON_AddItemToObject(modelentry, model[0], cJSON_CreateObject());

    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "manuf"), cJSON_CreateString(manuf[0]));
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelentry);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "sn"), cJSON_CreateString(sn[0]));
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
    EXPECT_TRUE(product_conf_manager_find_manufacturer(manuf[0]) != NULL);
    EXPECT_TRUE(product_conf_manager_find_model(manuf[0], model[0]) != NULL);
    EXPECT_TRUE(product_conf_manager_manufacturer_supports_sn(manuf[0], sn[0]));
}

TEST_F(ProductConfManagerFixture, NullStringCheck)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
    EXPECT_FALSE(product_conf_manager_find_manufacturer(nullptr) != NULL);
}

TEST_F(ProductConfManagerFixture, ManufNotArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = cJSON_CreateObject();
    cJSON_AddItemToObject(entry, "manuf", cJSON_CreateString("Dometic"));
    cJSON_AddItemToObject(entry, "model", cJSON_CreateArray());
    cJSON_AddItemToObject(entry, "sn", cJSON_CreateArray());
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_INVALID_PARAMS);
}

TEST_F(ProductConfManagerFixture, ModelNotArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    cJSON_ReplaceItemInObject(entry, "model", cJSON_CreateNumber(123));
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_INVALID_PARAMS);
}

TEST_F(ProductConfManagerFixture, SnNotArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    cJSON_ReplaceItemInObject(entry, "sn", cJSON_CreateBool(true));
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_INVALID_PARAMS);
}

TEST_F(ProductConfManagerFixture, ArrayItemNotString)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "manuf"), cJSON_CreateNumber(123));
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);  // Non-string items are skipped, loop break only on memory allocation error
}

TEST_F(ProductConfManagerFixture, ArrayItemNull)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "manuf"), cJSON_CreateNull());
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);  // Null items are skipped, loop break only on memory allocation error
}

TEST_F(ProductConfManagerFixture, FieldIsNull)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = cJSON_CreateObject();
    cJSON_AddItemToObject(entry, "manuf", cJSON_CreateNull());
    cJSON_AddItemToObject(entry, "model", cJSON_CreateArray());
    cJSON_AddItemToObject(entry, "sn", cJSON_CreateArray());
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_INVALID_PARAMS);
}

TEST_F(ProductConfManagerFixture, FwidSingleString)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);

    // Add model
    const char *const model[] = {"GP-DISPLAY"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    fwid_query_result_t result;
    allocate_fwid_result(&result, 5);  // Allocate capacity for 5 FWIDs
    bool found = product_conf_manager_get_model_fwids("Dometic", "GP-DISPLAY", &result);
    EXPECT_TRUE(found);
    EXPECT_EQ(result.count, 1);
    EXPECT_STREQ(result.fwids[0], "MP1");

    free_fwid_result(&result, 5);
}

TEST_F(ProductConfManagerFixture, FwidArrayOfStrings)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);

    // Add model
    const char *const model[] = {"SC-DB-MPPT-30"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC1"));
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    fwid_query_result_t result;
    allocate_fwid_result(&result, 5);  // Allocate capacity for 5 FWIDs

    bool found = product_conf_manager_get_model_fwids("Dometic", "SC-DB-MPPT-30", &result);
    EXPECT_TRUE(found);
    EXPECT_EQ(result.count, 2);
    EXPECT_STREQ(result.fwids[0], "MS1");
    EXPECT_STREQ(result.fwids[1], "MS1-MC1");
    free_fwid_result(&result, 5);
}

TEST_F(ProductConfManagerFixture, FwidMultipleModels)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);

    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    modelcontainer = cJSON_CreateObject();
    modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[1], modelItem);
    fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MH1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    fwid_query_result_t result1;
    allocate_fwid_result(&result1, 5);  // Allocate capacity for 5 FWIDs
    bool found1 = product_conf_manager_get_model_fwids("Dometic", "GP-DISPLAY", &result1);
    EXPECT_TRUE(found1);
    EXPECT_EQ(result1.count, 1);
    EXPECT_STREQ(result1.fwids[0], "MP1");

    fwid_query_result_t result2;
    allocate_fwid_result(&result2, 5);  // Allocate capacity for 5 FWIDs
    bool found2 = product_conf_manager_get_model_fwids("Dometic", "GP-Shunt", &result2);
    EXPECT_TRUE(found2);
    EXPECT_EQ(result2.count, 1);
    EXPECT_STREQ(result2.fwids[0], "MH1");

    free_fwid_result(&result1, 5);
    free_fwid_result(&result2, 5);
}

TEST_F(ProductConfManagerFixture, FwidNotFound)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    fwid_query_result_t result;
    allocate_fwid_result(&result, 5);  // Allocate capacity for 5 FWIDs
    bool found = product_conf_manager_get_model_fwids("Dometic", "Unknown", &result);
    EXPECT_FALSE(found);
    free_fwid_result(&result, 5);
}

TEST_F(ProductConfManagerFixture, FwidEmptyArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, FwidNotArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON_AddItemToObject(modelItem, "fwid", cJSON_CreateString("Invalid"));
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_INVALID_PARAMS);  // Non-array FWID field is skipped
}

TEST_F(ProductConfManagerFixture, FwidInvalidItemInArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateNull());
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);  // Covers line 614 (invalid fwid item warning)
}

TEST_F(ProductConfManagerFixture, FwidEmptyArrayInObject)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);  // Covers line 584 (empty array warning)
}

TEST_F(ProductConfManagerFixture, FwidValueNotStringOrArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();
    cJSON *fwid_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(fwid_obj, "GP-DISPLAY", cJSON_CreateNumber(123));  // Invalid: number
    cJSON_AddItemToArray(fwid_array, fwid_obj);
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);  // Should skip this invalid entry
}

TEST_F(ProductConfManagerFixture, FwidObjectWithNullChild)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();
    cJSON *fwid_obj = cJSON_CreateObject();
    // Don't add any children to the object
    cJSON_AddItemToArray(fwid_array, fwid_obj);
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);

    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);  // Should handle empty object gracefully
}

TEST_F(ProductConfManagerFixture, FwidObjectInvalid)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("NotAnObject"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, FwidArrayContainsNonString)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateNumber(123));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);

    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, FwidGetWithNullParams)
{
    fwid_query_result_t result;
    allocate_fwid_result(&result, 5);
    bool found = product_conf_manager_get_model_fwids(NULL, NULL, &result);
    EXPECT_FALSE(found);

    found = product_conf_manager_get_model_fwids(NULL, "GP-DISPLAY", nullptr);
    EXPECT_FALSE(found);

    free_fwid_result(&result, 5);
}

TEST_F(ProductConfManagerFixture, FwidPartialModelNameMatch)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);

    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    // Test partial match (contains)
    fwid_query_result_t result;
    allocate_fwid_result(&result, 5);
    bool found = product_conf_manager_get_model_fwids("Dometic", "GP-DISPLAY-v1.0", &result);
    EXPECT_TRUE(found);
    EXPECT_EQ(result.count, 1);
    EXPECT_STREQ(result.fwids[0], "MP1");
    free_fwid_result(&result, 5);
}

TEST_F(ProductConfManagerFixture, MultipleConfigurationEntries)
{
    cJSON *root = cJSON_CreateArray();

    // First entry
    cJSON *entry1 = create_basic_conf_entry();
    const char *const manuf1[] = {"GOPOWER"};
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry1, "manuf"), cJSON_CreateString(manuf1[0]));
    cJSON_AddItemToArray(root, entry1);

    // Second entry
    cJSON *entry2 = create_basic_conf_entry();
    const char *const manuf2[] = {"Dometic"};
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry2, "manuf"), cJSON_CreateString(manuf2[0]));
    cJSON_AddItemToArray(root, entry2);

    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    EXPECT_TRUE(product_conf_manager_find_manufacturer(manuf1[0]) != NULL);
    EXPECT_TRUE(product_conf_manager_find_manufacturer(manuf2[0]) != NULL);
}

TEST_F(ProductConfManagerFixture, ComplexConfiguration)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    // Add manufacturers
    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("GOPOWER"));
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("Go Power!"));

    // Add models
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt", "SC-DB-MPPT-30"};

    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    modelcontainer = cJSON_CreateObject();
    modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[1], modelItem);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    modelcontainer = cJSON_CreateObject();
    modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[1], modelItem);
    fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    EXPECT_TRUE(product_conf_manager_find_manufacturer("GOPOWER") != NULL);
    EXPECT_TRUE(product_conf_manager_find_manufacturer("Go Power!") != NULL);
    EXPECT_TRUE(product_conf_manager_find_model("GOPOWER", "GP-DISPLAY") != NULL);
    EXPECT_TRUE(product_conf_manager_find_model("Go Power!", "GP-Shunt") != NULL);
}

TEST_F(ProductConfManagerFixture, VersionPatternValidEntry)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, VersionPatternWithHwver)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-AIC", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-[^-]*-[^-]*-[^-]*-([^_]*)",
                                            "^[^_]*_(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, VersionPatternNotObject)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-AIC", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON_AddItemToObject(modelItem, "version_patterns", cJSON_CreateString("NotAnObject"));
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, VersionPatternMissingFwverRegex)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-AIC", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = cJSON_CreateObject();
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, VersionPatternInvalidFwverRegex)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-AIC", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("[invalid(regex");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, VersionPatternInvalidHwverRegex)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-AIC", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-(.*)$",
                                            "[invalid(regex");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);
}

TEST_F(ProductConfManagerFixture, UpdateVersionsGPAIC)
{

    // Load configuration with version patterns
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-AIC", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-[^-]*-[^-]*-[^-]*-([^_]*)",
                                            "^[^_]*_(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    const char mdl[] = "GP-AIC-2000-12-SL-1.00.00-4.67_C00GM00G";

    //    int ddm_inst = create_product_with_model(mdl, sn, name);
    //    ASSERT_TRUE(ddm_inst > 0);
    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    // bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    EXPECT_TRUE(result == 0);
    EXPECT_STREQ("1.00.00-4.67", result_string);
    // verify_version(ddm_inst, FIELD_FWVER, "1.00.00-4.67");
    result = product_conf_manager_extract_hw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    // verify_version(ddm_inst, FIELD_HWVER, "C00GM00G");
    EXPECT_TRUE(result == 0);
    EXPECT_STREQ("C00GM00G", result_string);

    // cleanup_product(ddm_inst);
}

TEST_F(ProductConfManagerFixture, UpdateVersionsSCDBMPPT)
{
    // Load configuration
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"SC-DB-MPPT", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-[^-]*-[^-]*-(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    const char mdl[] = "SC-DB-MPPT-30-1.00.01-8";

    // int ddm_inst = create_product_with_model(mdl, sn, name);
    // ASSERT_TRUE(ddm_inst > 0);

    // bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    // EXPECT_TRUE(result);
    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    // bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    EXPECT_TRUE(result == 0);
    EXPECT_STREQ("1.00.01-8", result_string);

    // verify_version(ddm_inst, FIELD_FWVER, "1.00.01-8");

    // cleanup_product(ddm_inst);
}

TEST_F(ProductConfManagerFixture, UpdateVersionsTM505)
{
    // Load configuration
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"TM505", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-([^(]*)\\(");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    const char mdl[] = "TM505-1.19E(x110-01)";

    // int ddm_inst = create_product_with_model(mdl, sn, name);
    // ASSERT_TRUE(ddm_inst > 0);

    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    EXPECT_TRUE(result == 0);
    EXPECT_STREQ("1.19E", result_string);
    // bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    // EXPECT_TRUE(result);

    // verify_version(ddm_inst, FIELD_FWVER, "1.19E");

    // cleanup_product(ddm_inst);
}

TEST_F(ProductConfManagerFixture, UpdateVersionsGPShunt)
{
    // Load configuration
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"TM505", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[1], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    const char mdl[] = "GP-Shunt-0.00.01.1";

    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    EXPECT_TRUE(result == 0);
    EXPECT_STREQ("0.00.01.1", result_string);

    // int ddm_inst = create_product_with_model(mdl, sn, name);
    // ASSERT_TRUE(ddm_inst > 0);

    // bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    // EXPECT_TRUE(result);

    // verify_version(ddm_inst, FIELD_FWVER, "0.00.01.1");

    // cleanup_product(ddm_inst);
}

TEST_F(ProductConfManagerFixture, UpdateVersionsGPRVCSSW)
{
    // Load configuration
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-RVC-SSW", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-[^-]*-[^-]*-(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    const char mdl[] = "GP-RVC-SSW-2000-1.00.00";

    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    EXPECT_TRUE(result == 0);
    EXPECT_STREQ("1.00.00", result_string);

    // int ddm_inst = create_product_with_model(mdl, sn, name);
    // ASSERT_TRUE(ddm_inst > 0);

    // bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    // EXPECT_TRUE(result);

    // verify_version(ddm_inst, FIELD_FWVER, "1.00.00");

    // cleanup_product(ddm_inst);
}

TEST_F(ProductConfManagerFixture, UpdateVersionsGPDisplay)
{
    // Load configuration
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    add_Dometic_manuf_item_to_array(entry);
    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    const char mdl[] = "GP-DISPLAY-1.00.00";

    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    EXPECT_TRUE(result == 0);
    EXPECT_STREQ("1.00.00", result_string);
    /*    int ddm_inst = create_product_with_model(mdl, sn, name);
        ASSERT_TRUE(ddm_inst > 0);

        bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
        EXPECT_TRUE(result);

        verify_version(ddm_inst, FIELD_FWVER, "1.00.00");

        cleanup_product(ddm_inst);
    */
}

TEST_F(ProductConfManagerFixture, UpdateVersionsInvalidModel)
{
    // Load configuration (no patterns for this model)
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();
    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    const char mdl[] = "GP-Dometic";

    // int ddm_inst = create_product_with_model(mdl, sn, name);
    // ASSERT_TRUE(ddm_inst > 0);
    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));

    //    bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    EXPECT_FALSE(result == 0);

    //    cleanup_product(ddm_inst);
}

TEST_F(ProductConfManagerFixture, UpdateVersionsNoPatternLoaded)
{
    // Don't load any configuration
    const char mdl[] = "GP-DISPLAY-1.00.00";

    //    int ddm_inst = create_product_with_model(mdl, sn, name);
    //    ASSERT_TRUE(ddm_inst > 0);

    // bool result = prod_conf_mgr_update_versions_from_model(ddm_inst);
    char result_string[64];
    int result = product_conf_manager_extract_fw_version("Dometic", mdl, mdl, result_string, sizeof(result_string));
    EXPECT_FALSE(result == 0);  // Should fail because no patterns are loaded

    // cleanup_product(ddm_inst);
}

TEST_F(ProductConfManagerFixture, ComplexConfigurationWithVersionPatterns)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    // Add manufacturers
    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("GOPOWER"));
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("Go Power!"));

    // Add models
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt", "SC-DB-MPPT-30"};

    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON *pattern = create_version_pattern("^[^-]*-[^-]*-(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern);
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    modelcontainer = cJSON_CreateObject();
    modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[1], modelItem);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    modelcontainer = cJSON_CreateObject();
    modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[2], modelItem);
    fwid_array = cJSON_CreateArray();
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON *pattern2 = create_version_pattern("^[^-]*-[^-]*-[^-]*-[^-]*-(.*)$");
    cJSON_AddItemToObject(modelItem, "version_patterns", pattern2);

    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    // Verify whitelist
    EXPECT_TRUE(product_conf_manager_find_manufacturer("GOPOWER") != NULL);
    EXPECT_TRUE(product_conf_manager_find_manufacturer("Go Power!") != NULL);
    EXPECT_TRUE(product_conf_manager_find_model("GOPOWER", "GP-DISPLAY") != NULL);
    EXPECT_TRUE(product_conf_manager_find_model("Go Power!", "GP-Shunt") != NULL);

    // Verify FWID mapping
    fwid_query_result_t result;
    allocate_fwid_result(&result, 5);  // Allocate capacity for 5 FWIDs
    bool found = product_conf_manager_get_model_fwids("Go Power!", "GP-DISPLAY", &result);
    EXPECT_TRUE(found);
    EXPECT_EQ(result.count, 1);
    EXPECT_STREQ(result.fwids[0], "MP1");

    // Verify version extraction works
    const char mdl[] = "GP-DISPLAY-1.00.00";

    // int ddm_inst = create_product_with_model(mdl, sn, name);
    // ASSERT_TRUE(ddm_inst > 0);

    // bool result_ver = prod_conf_mgr_update_versions_from_model(ddm_inst);
    // EXPECT_TRUE(result_ver);
    char result_string[64];
    int result_ver = product_conf_manager_extract_fw_version("GOPOWER", mdl, mdl, result_string, sizeof(result_string));
    EXPECT_TRUE(result_ver == 0);
    EXPECT_STREQ("1.00.00", result_string);

    // verify_version(ddm_inst, FIELD_FWVER, "1.00.00");

    // cleanup_product(ddm_inst);
    free_fwid_result(&result, 5);
}

TEST_F(ProductConfManagerFixture, FwidMultipleQueries)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("GOPOWER"));

    // Add models
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt", "SC-DB-MPPT-30"};

    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();

    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    modelcontainer = cJSON_CreateObject();
    modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[1], modelItem);
    fwid_array = cJSON_CreateArray();

    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MH1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    modelcontainer = cJSON_CreateObject();
    modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[2], modelItem);
    fwid_array = cJSON_CreateArray();

    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    // Query multiple times
    fwid_query_result_t result1;
    allocate_fwid_result(&result1, 5);  // Allocate capacity for 5 FWIDs
    bool found1 = product_conf_manager_get_model_fwids("GOPOWER", "GP-DISPLAY", &result1);
    EXPECT_TRUE(found1);
    EXPECT_EQ(result1.count, 1);

    fwid_query_result_t result2;
    allocate_fwid_result(&result2, 5);  // Allocate capacity for 5 FWIDs
    bool found2 = product_conf_manager_get_model_fwids("GOPOWER", "GP-Shunt", &result2);
    EXPECT_TRUE(found2);
    EXPECT_EQ(result2.count, 1);

    fwid_query_result_t result3;
    allocate_fwid_result(&result3, 5);  // Allocate capacity for 5 FWIDs
    bool found3 = product_conf_manager_get_model_fwids("GOPOWER", "SC-DB-MPPT-30", &result3);
    EXPECT_TRUE(found3);
    EXPECT_EQ(result3.count, 2);

    fwid_query_result_t result4;
    allocate_fwid_result(&result4, 5);  // Allocate capacity for 5 FWIDs
    bool found4 = product_conf_manager_get_model_fwids("GOPOWER", "Unknown", &result4);
    EXPECT_FALSE(found4);
    EXPECT_EQ(result4.count, 0);

    // Verify first result is still valid
    EXPECT_STREQ(result1.fwids[0], "MP1");
    free_fwid_result(&result1, 5);
    free_fwid_result(&result2, 5);
    free_fwid_result(&result3, 5);
    free_fwid_result(&result4, 5);
}

TEST_F(ProductConfManagerFixture, FwidCaseSensitiveMatch)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("GOPOWER"));

    // Add models
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt", "SC-DB-MPPT-30"};

    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);

    cJSON *fwid_array = cJSON_CreateArray();

    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    // Exact match should work
    fwid_query_result_t result1;
    allocate_fwid_result(&result1, 5);  // Allocate capacity for 5 FWIDs
    bool found1 = product_conf_manager_get_model_fwids("GOPOWER", "GP-DISPLAY", &result1);
    EXPECT_TRUE(found1);

    // Different case should not match (case-sensitive substring match)
    fwid_query_result_t result2;
    allocate_fwid_result(&result2, 5);  // Allocate capacity for 5 FWIDs
    bool found2 = product_conf_manager_get_model_fwids("GOPOWER", "gp-display", &result2);
    EXPECT_FALSE(found2);
    free_fwid_result(&result1, 5);
    free_fwid_result(&result2, 5);
}

TEST_F(ProductConfManagerFixture, FwidWithManufacturerFilter)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    // Add manufacturers
    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("GOPOWER"));
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("Go Power!"));

    // Add models
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt", "SC-DB-MPPT-30"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[0], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();

    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MP1"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    // Test with matching manufacturer
    fwid_query_result_t result1;
    allocate_fwid_result(&result1, 5);  // Allocate capacity for 5 FWIDs
    bool found1 = product_conf_manager_get_model_fwids("GOPOWER", "GP-DISPLAY", &result1);
    EXPECT_TRUE(found1);
    EXPECT_EQ(result1.count, 1);
    EXPECT_STREQ(result1.fwids[0], "MP1");

    // Test with another matching manufacturer
    fwid_query_result_t result2;
    allocate_fwid_result(&result2, 5);  // Allocate capacity for 5 FWIDs
    bool found2 = product_conf_manager_get_model_fwids("Go Power!", "GP-DISPLAY", &result2);
    EXPECT_TRUE(found2);

    // Test with non-matching manufacturer
    fwid_query_result_t result3;
    allocate_fwid_result(&result3, 5);  // Allocate capacity for 5 FWIDs
    bool found3 = product_conf_manager_get_model_fwids("Dometic", "GP-DISPLAY", &result3);
    EXPECT_FALSE(found3);
    free_fwid_result(&result1, 5);
    free_fwid_result(&result2, 5);
    free_fwid_result(&result3, 5);
}

TEST_F(ProductConfManagerFixture, FwidInsufficientCapacity)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("GOPOWER"));

    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt", "SC-DB-MPPT-30"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[2], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();

    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC2"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    // Allocate buffer with capacity for only 2 FWIDs (but there are 3)
    fwid_query_result_t result;
    allocate_fwid_result(&result, 2);

    bool found = product_conf_manager_get_model_fwids("GOPOWER", "SC-DB-MPPT-30", &result);
    EXPECT_TRUE(found);
    EXPECT_EQ(result.count, 2);  // Should only copy 2 (capacity limit)
    EXPECT_STREQ(result.fwids[0], "MS1");
    EXPECT_STREQ(result.fwids[1], "MS1-MC1");
    // MS1-MC2 should be truncated

    // test with zero capacity
    result.count = 0;
    bool found1 = product_conf_manager_get_model_fwids("GOPOWER", "SC-DB-MPPT-30", &result);
    EXPECT_TRUE(found1);
    EXPECT_EQ(result.count, 0);
    free_fwid_result(&result, 2);
}

TEST_F(ProductConfManagerFixture, FwidGetWithNullFwidsArray)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *entry = create_basic_conf_entry();

    cJSON *manuf_array = cJSON_GetObjectItem(entry, "manuf");
    cJSON_AddItemToArray(manuf_array, cJSON_CreateString("GOPOWER"));

    // Add model
    const char *const model[] = {"GP-DISPLAY", "GP-Shunt", "SC-DB-MPPT-30"};
    cJSON *modelcontainer = cJSON_CreateObject();
    cJSON *modelItem = cJSON_CreateObject();
    cJSON_AddItemToObject(modelcontainer, model[2], modelItem);
    cJSON *fwid_array = cJSON_CreateArray();

    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC1"));
    cJSON_AddItemToArray(fwid_array, cJSON_CreateString("MS1-MC2"));
    cJSON_AddItemToObject(modelItem, "fwid", fwid_array);
    cJSON_AddItemToArray(cJSON_GetObjectItem(entry, "model"), modelcontainer);

    cJSON_AddItemToArray(root, entry);
    write_json_to_conf_file(root);
    cJSON_Delete(root);

    int ret = product_conf_manager_load_config_file(conf_file);
    EXPECT_EQ(ret, PROD_CONF_ERR_OK);

    // Test with NULL fwids array (invalid parameter)
    fwid_query_result_t result;
    result.fwids = nullptr;  // Intentionally NULL
    result.count = 5;

    bool found = product_conf_manager_get_model_fwids("GOPOWER", "GP-DISPLAY", &result);
    EXPECT_FALSE(found);  // Should fail due to invalid parameters
}

extern "C" void disable_connectors(void)
{
    LOG(I, "Disable smarteco connector and climate zone connector");
    connector_smart_eco_feature.disabled = 1;
    connector_climate_zone_feature.disabled = 1;
}
