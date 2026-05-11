/*
 * test_feature_database_run.cpp
 *
 *  Created on: 28 march 2025
 *      Author: Kire Janev
 */

extern "C" {
#include "broker.h"
#include "configuration.h"
#include "connector_climate_zone_feature.h"
#include "connector_smart_eco_feature.h"
#include "connector_unittest.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "feature_database.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "product_database.h"
#include "sorted_list.h"
static void task_handler_prod(DDMP2_FRAME *pframe);
}
#include "DICMFrameworkTestFixture.hpp"
using ::testing::Test;
class FeatureDatabaseTestRunFixture : public DICMFrameworkTestFixture
{
  public:
    static FeatureDatabaseTestRunFixture *getInstance()
    {
        return mTestInstance;
    }
    int data_line_owner = 0;
    int data_line_sub = 1;
    static inline FeatureDatabaseTestRunFixture *mTestInstance = NULL;
    FeatureDatabaseTestRunFixture() : DICMFrameworkTestFixture()
    {
        mTestInstance = this;
    }

  protected:
    void SetUp() override
    {
        connector_unittest_enable_indexed_connector(NULL, task_handler_prod, data_line_owner);
        connector_unittest_enable_indexed_connector(NULL, NULL, data_line_sub);
        DICMFrameworkTestFixture::SetUp();
        setConnectorId(&connector_unittest.connector_id);
        DICMFrameworkTestFixture::SetupFramework();
        // esp_log_level_set("feature_database.c", ESP_LOG_DEBUG);
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};

extern "C" void task_handler_prod(DDMP2_FRAME *pframe)
{
    feat_db_frame_handler(pframe);
}

TEST_F(FeatureDatabaseTestRunFixture, FeatureDatabaseAPIs)
{
    // Create product database entry so the interfaces of the groups can be updated
    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char prod_name[] = "Dometic Product";
    int prod_instance = -1;

    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, prod_name, strlen(prod_name));

    int err = -1;
    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    prod_instance = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(prod_instance > 0);

    char uid[16];
    size_t uid_size = 16;
    ProdDBReadCache(FIELD_UID, prod_instance, uid, &uid_size);
    EXPECT_TRUE(uid_size == 15);

    err = feat_db_init();
    EXPECT_TRUE(err == ESP_OK);
    feat_db_load_cache(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id);
    int32_t class_instance = -1;
    // Try to find manager class for climate zone
    class_instance = feat_db_find_first_by_id_and_type(0, GROUP0TYPE_CLIMATEZONE);
    EXPECT_TRUE(class_instance == -1);
    // Create manager and standard group classes for SmartEco
    err = feat_db_cache_entry_create(GROUP0TYPE_SMARTECO, connector_unittest.connector_id, 0, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);
    err = feat_db_cache_entry_create(GROUP0TYPE_SMARTECO, connector_unittest.connector_id, 1, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);
    // Create manager and standard group classes for ClimateZone
    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id, 0, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);
    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id, 1, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);

    // Store the group class instance for Zone 1 to include it in the interface of Zone 2
    uint32_t group_class_instance = GROUP0 | DDM2_PARAMETER_INSTANCE(class_instance);

    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id, 2, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);

    int32_t active_off = 0;
    int32_t enable_off = 0;
    int32_t active_on = 1;
    int32_t enable_on = 1;
    int32_t id = 3;
    const char name[] = "Zone 3";
    uint32_t rule_instance = RULE0;
    uint32_t prod_class_instance = PROD0 | DDM2_PARAMETER_INSTANCE(prod_instance);

    // Update cache entry for Zone 2 with different active/enable flags, different ID and different name
    feat_db_update_cache(&prod_class_instance, sizeof(prod_class_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance);
    feat_db_update_cache(&group_class_instance, sizeof(group_class_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance);
    feat_db_update_cache(&active_off, sizeof(active_off), FEAT_DB_FIELD_ACTIVE, class_instance);
    feat_db_update_cache(&enable_off, sizeof(enable_off), FEAT_DB_FIELD_ENABLE, class_instance);
    feat_db_update_cache(&active_on, sizeof(active_on), FEAT_DB_FIELD_ACTIVE, class_instance);
    feat_db_update_cache(&enable_on, sizeof(enable_on), FEAT_DB_FIELD_ENABLE, class_instance);
    feat_db_update_cache(&id, sizeof(id), FEAT_DB_FIELD_ID, class_instance);
    feat_db_update_cache(name, strlen(name), FEAT_DB_FIELD_NAME, class_instance);
    feat_db_update_cache(&rule_instance, sizeof(rule_instance), FEAT_DB_FIELD_RULES, class_instance);

    // Use any value to try and update an INVALID field
    feat_db_update_cache(&rule_instance, sizeof(rule_instance), FEAT_DB_FIELD_INVALID, class_instance);

    // Update with same values
    feat_db_update_cache(&active_on, sizeof(active_on), FEAT_DB_FIELD_ACTIVE, class_instance);
    feat_db_update_cache(&enable_on, sizeof(enable_on), FEAT_DB_FIELD_ENABLE, class_instance);
    feat_db_update_cache(&id, sizeof(id), FEAT_DB_FIELD_ID, class_instance);
    feat_db_update_cache(name, strlen(name), FEAT_DB_FIELD_NAME, class_instance);
    feat_db_update_cache(&prod_class_instance, sizeof(prod_class_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance);

    int32_t enabled_ids[10];
    int32_t active_ids[10];
    size_t enabled_size = 10;
    size_t active_size = 10;
    feat_db_get_all_active_ids_of_type(active_ids, &active_size, GROUP0TYPE_CLIMATEZONE);
    feat_db_get_all_enabled_ids_of_type(enabled_ids, &enabled_size, GROUP0TYPE_CLIMATEZONE);
    EXPECT_TRUE(enabled_size == 2);
    EXPECT_TRUE(active_size == 2);

    // Store the value of instance number for Zone 3 (updated Zone 2)
    int32_t class_instance_check = class_instance;

    class_instance = feat_db_find_first_by_class_instance_interface(prod_class_instance);
    EXPECT_TRUE(class_instance == class_instance_check);
    class_instance = feat_db_find_first_by_uid_interface(uid);
    EXPECT_TRUE(class_instance == class_instance_check);
    class_instance = feat_db_find_first_by_id_and_type(id, GROUP0TYPE_CLIMATEZONE);
    EXPECT_TRUE(class_instance == class_instance_check);

    // Create new cache entry with invalid group ID
    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id, -1, GROUP0ENABLE_OFF, GROUP0ACTIVE_OFF, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);

    // Remove class instances from interface and rules
    UPDLINKEDCLASS_T *remove_prod = (UPDLINKEDCLASS_T *)hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(remove_prod != NULL);
    remove_prod->update[0] = 0;
    remove_prod->updclass = prod_class_instance;
    feat_db_update_interface_all(remove_prod, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), GROUP0TYPE_CLIMATEZONE);
    feat_db_update_interface_all(remove_prod, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), GROUP0TYPE_CLIMATEZONE);
    hal_mem_free(remove_prod);

    UPDLINKEDCLASS_T *remove_rule = (UPDLINKEDCLASS_T *)hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(remove_rule != NULL);
    remove_rule->update[0] = 0;
    remove_rule->updclass = rule_instance;
    feat_db_update_cache(remove_rule, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), FEAT_DB_FIELD_RULES, class_instance_check);
    feat_db_update_cache(remove_rule, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), FEAT_DB_FIELD_RULES, class_instance_check);
    feat_db_update_cache(NULL, 0, FEAT_DB_FIELD_RULES, class_instance_check);
    feat_db_update_cache(remove_rule, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), FEAT_DB_FIELD_INVALID, INVALID_DDM_INSTANCE);
    hal_mem_free(remove_rule);

    // Delete the cache entry for valid and invalid class instances
    feat_db_cache_entry_delete(class_instance);
    feat_db_cache_entry_delete(INVALID_DDM_INSTANCE);

    // Load the cache again for Climate Zone and Smart Eco instances
    feat_db_load_cache(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id);
    feat_db_load_cache(GROUP0TYPE_SMARTECO, connector_unittest.connector_id);

    // Add class instance that doesn't have UID in the interface
    uint32_t invalid_AC_class = AC0;
    feat_db_update_cache(&invalid_AC_class, sizeof(invalid_AC_class), FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance_check);

    // Find group class instance with invalid class instance intarface, invalid UID and of type that doesn't exist
    class_instance = feat_db_find_first_by_class_instance_interface(invalid_AC_class);
    EXPECT_TRUE(class_instance == -1);
    char invalid_uid[] = "12345678912345";
    class_instance = feat_db_find_first_by_uid_interface(invalid_uid);
    EXPECT_TRUE(class_instance == -1);
    class_instance = feat_db_find_first_by_id_and_type(5, GROUP0TYPE_CLIMATECONTROL);
    EXPECT_TRUE(class_instance == -1);
    class_instance = feat_db_find_first_by_uid_interface(NULL);
    EXPECT_TRUE(class_instance == -1);
    // Get active and enabled IDs for group instances with the number of instances set to invalid size
    enabled_size = 1;
    active_size = 1;
    feat_db_get_all_active_ids_of_type(active_ids, &active_size, GROUP0TYPE_CLIMATEZONE);
    EXPECT_TRUE(active_size > 0);
    feat_db_get_all_enabled_ids_of_type(enabled_ids, &enabled_size, GROUP0TYPE_CLIMATEZONE);
    EXPECT_TRUE(enabled_size > 0);
    feat_db_get_all_active_ids_of_type(NULL, &active_size, GROUP0TYPE_CLIMATEZONE);
    feat_db_get_all_enabled_ids_of_type(NULL, &enabled_size, GROUP0TYPE_CLIMATEZONE);

    // Update the interface of all group instances of specific type with invalid pointer to data
    feat_db_update_interface_all(NULL, 0, GROUP0TYPE_CLIMATEZONE);
}

TEST_F(FeatureDatabaseTestRunFixture, ReadCacheAPI)
{
    int err = -1;

    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char prod_name[] = "Dometic Product";
    int prod_instance = -1;

    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, prod_name, strlen(prod_name));

    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    prod_instance = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(prod_instance > 0);

    char uid[16];
    size_t uid_size = 16;
    ProdDBReadCache(FIELD_UID, prod_instance, uid, &uid_size);
    EXPECT_TRUE(uid_size == 15);

    err = feat_db_init();
    EXPECT_TRUE(err == ESP_OK);
    int32_t class_instance = -1;
    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id, 1, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);
    uint32_t prod_class_instance = PROD0 | DDM2_PARAMETER_INSTANCE(prod_instance);
    uint32_t rule_class_instance = RULE0;

    feat_db_update_cache(&prod_class_instance, sizeof(prod_class_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance);
    feat_db_update_cache(&rule_class_instance, sizeof(rule_class_instance), FEAT_DB_FIELD_RULES, class_instance);

    int32_t data;
    size_t data_size = sizeof(data);
    size_t invalid_data_size = 1;
    feat_db_read_cache(FEAT_DB_FIELD_INVALID, class_instance, NULL, &invalid_data_size);
    feat_db_read_cache(FEAT_DB_FIELD_INVALID, class_instance, &data, &invalid_data_size);
    feat_db_read_cache(FEAT_DB_FIELD_INVALID, INVALID_DDM_INSTANCE, NULL, &invalid_data_size);

    feat_db_read_cache(FEAT_DB_FIELD_ACTIVE, class_instance, &data, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_ACTIVE, class_instance, &data, &data_size);
    EXPECT_TRUE(data_size == sizeof(int32_t));
    EXPECT_TRUE(data == GROUP0ACTIVE_ON);

    feat_db_read_cache(FEAT_DB_FIELD_ENABLE, class_instance, &data, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_ENABLE, class_instance, &data, &data_size);
    EXPECT_TRUE(data_size == sizeof(int32_t));
    EXPECT_TRUE(data == GROUP0ENABLE_ON);

    feat_db_read_cache(FEAT_DB_FIELD_TYPE, class_instance, &data, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_TYPE, class_instance, &data, &data_size);
    EXPECT_TRUE(data_size == sizeof(int32_t));
    EXPECT_TRUE(data == GROUP0TYPE_CLIMATEZONE);

    feat_db_read_cache(FEAT_DB_FIELD_ID, class_instance, &data, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_ID, class_instance, &data, &data_size);
    EXPECT_TRUE(data_size == sizeof(int32_t));
    EXPECT_TRUE(data == 1);

    char name[32];
    size_t name_len = 32;
    feat_db_read_cache(FEAT_DB_FIELD_NAME, class_instance, name, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_NAME, class_instance, name, &name_len);
    EXPECT_TRUE(name_len == 6);

    uint32_t class_inst[10];
    size_t class_inst_size = 10 * sizeof(uint32_t);

    feat_db_read_cache(FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance, class_inst, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance, class_inst, &class_inst_size);
    EXPECT_TRUE(class_inst_size == sizeof(uint32_t));
    EXPECT_TRUE(class_inst[0] == prod_class_instance);

    memset(class_inst, 0, 10 * sizeof(class_inst[0]));

    feat_db_read_cache(FEAT_DB_FIELD_RULES, class_instance, class_inst, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_RULES, class_instance, class_inst, &class_inst_size);
    EXPECT_TRUE(class_inst_size == sizeof(uint32_t));
    EXPECT_TRUE(class_inst[0] == RULE0);

    char uid_group[32];
    size_t uid_group_len = DICM_UID_KEY_STR_LEN;
    feat_db_read_cache(FEAT_DB_FIELD_UID, class_instance, uid_group, &invalid_data_size);
    EXPECT_TRUE(invalid_data_size == 0);
    feat_db_read_cache(FEAT_DB_FIELD_UID, class_instance, uid_group, &uid_group_len);
    EXPECT_TRUE(uid_group_len == DICM_UID_KEY_STR_LEN - 1);
}

TEST_F(FeatureDatabaseTestRunFixture, SubscribeToGroupParameters)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    int err = -1;

    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char prod_name[] = "Dometic Product";
    int prod_instance = -1;

    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, prod_name, strlen(prod_name));

    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    prod_instance = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(prod_instance > 0);

    char uid[16];
    size_t uid_size = 16;
    ProdDBReadCache(FIELD_UID, prod_instance, uid, &uid_size);
    EXPECT_TRUE(uid_size == 15);

    err = feat_db_init();
    EXPECT_TRUE(err == ESP_OK);
    int32_t class_instance = -1;
    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id + data_line_owner, 1, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);
    uint32_t prod_class_instance = PROD0 | DDM2_PARAMETER_INSTANCE(prod_instance);
    uint32_t rule_class_instance = RULE0;

    feat_db_update_cache(&prod_class_instance, sizeof(prod_class_instance), FEAT_DB_FIELD_INTERFACE_CLASS_INST, class_instance);
    feat_db_update_cache(&rule_class_instance, sizeof(rule_class_instance), FEAT_DB_FIELD_RULES, class_instance);

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0UID | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0UID | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0ID | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0ID | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a SUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0TYPE | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0TYPE | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0RULES | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0RULES | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0ENABLE | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0ENABLE | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0ACTIVE | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0ACTIVE | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (GROUP0DELGROUP | DDM2_PARAMETER_INSTANCE(class_instance)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent one DDMP2 frame" << std::endl;

    ProdDBProdClassNodeDelete(prod_instance);
    feat_db_cache_entry_delete(class_instance);
}

TEST_F(FeatureDatabaseTestRunFixture, SetGroupParams)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    int err = -1;

    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char prod_name[] = "Dometic Product";
    int prod_instance = -1;

    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, prod_name, strlen(prod_name));

    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    prod_instance = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(prod_instance > 0);

    char uid[16];
    size_t uid_size = 16;
    ProdDBReadCache(FIELD_UID, prod_instance, uid, &uid_size);
    EXPECT_TRUE(uid_size == 15);

    err = feat_db_init();
    EXPECT_TRUE(err == ESP_OK);
    int32_t class_instance = -1;
    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id + data_line_owner, 1, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);
    uint32_t prod_class_instance = PROD0 | DDM2_PARAMETER_INSTANCE(prod_instance);
    uint32_t rule_class_instance = RULE0;

    int32_t active = 0;
    int32_t enable = 0;
    int32_t id = 2;
    const char name[] = "Zone 2";

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0ACTIVE | DDM2_PARAMETER_INSTANCE(class_instance)), &active, sizeof(active), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0ACTIVE | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0ENABLE | DDM2_PARAMETER_INSTANCE(class_instance)), &enable, sizeof(enable), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0ENABLE | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0ID | DDM2_PARAMETER_INSTANCE(class_instance)), &id, sizeof(id), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0ID | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance)), name, strlen(name), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance)), name, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(class_instance)), &prod_class_instance, sizeof(prod_class_instance), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0RULES | DDM2_PARAMETER_INSTANCE(class_instance)), &rule_class_instance, sizeof(rule_class_instance), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0RULES | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0TYPE | DDM2_PARAMETER_INSTANCE(class_instance)), &One, sizeof(One), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent one DDMP2 frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0DELGROUP | DDM2_PARAMETER_INSTANCE(class_instance)), &One, sizeof(One), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0AVL | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    // After deleting the group, try load the cache again. Nothing should be found as part of GROUP0DELGROUP events before.
    feat_db_load_cache(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent any DDMP2 frame" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    ProdDBProdClassNodeDelete(prod_instance);

    class_instance = -1;
    err = feat_db_cache_entry_create(GROUP0TYPE_SMARTECO, connector_unittest.connector_id + data_line_owner, 1, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance)), name, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0NAME | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;
}

TEST_F(FeatureDatabaseTestRunFixture, SetGroupManagerParam)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    int err = -1;

    err = feat_db_init();
    EXPECT_TRUE(err == ESP_OK);
    int32_t class_instance = -1;
    err = feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id + data_line_owner, 0, GROUP0ENABLE_ON, GROUP0ACTIVE_ON, &class_instance);
    EXPECT_TRUE(err == ESP_OK);
    EXPECT_TRUE(class_instance > -1);

    int32_t active = 0;

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0ACTIVE | DDM2_PARAMETER_INSTANCE(class_instance)), &active, sizeof(active), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should have sent one DDMP2 frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (GROUP0ADDGROUP | DDM2_PARAMETER_INSTANCE(class_instance)), &One, sizeof(One), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 2) << "We should have sent two DDMP2 frame" << std::endl;
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0ADDGROUP | DDM2_PARAMETER_INSTANCE(class_instance))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    feat_db_cache_entry_delete(class_instance);
}
extern "C" void disable_connectors(void)
{
    LOG(I, "Disable smarteco connector and climate zone connector");
    connector_smart_eco_feature.disabled = 1;
    connector_climate_zone_feature.disabled = 1;
}
