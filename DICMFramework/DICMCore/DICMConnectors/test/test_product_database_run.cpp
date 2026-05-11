/*
 * test_product_database_run.cpp
 *
 *  Created on: 12 feb. 2025
 *      Author: Kire Janev
 */

extern "C" {
#include "broker.h"
#include "configuration.h"
#include "connector_climate_zone_feature.h"
#include "connector_smart_eco_feature.h"
#include "connector_unittest.h"
#include "ddm2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "product_database.h"
#include "sorted_list.h"
static void task_handler_prod(DDMP2_FRAME *pframe);
static void prod_reset_handler(int32_t new_reset, int ddm_instance);
static void prod_indicate_handler(bool indicate, int ddm_instance);
}
#include "DICMFrameworkTestFixture.hpp"
using ::testing::Test;
class ProductDatabaseTestRunFixture : public DICMFrameworkTestFixture
{
  public:
    static ProductDatabaseTestRunFixture *getInstance()
    {
        return mTestInstance;
    }
    int data_line_owner = 0;
    int data_line_sub = 1;
    static inline ProductDatabaseTestRunFixture *mTestInstance = NULL;
    ProductDatabaseTestRunFixture() : DICMFrameworkTestFixture()
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
        // esp_log_level_set("product_database.c", ESP_LOG_DEBUG);
    }

    void TearDown() override
    {
        ProdDBDeInit();
        DICMFrameworkTestFixture::TearDown();
    }
};

extern "C" void task_handler_prod(DDMP2_FRAME *pframe)
{
    ProdDBFrameHandler(pframe);
}

TEST_F(ProductDatabaseTestRunFixture, SubscribeToProdParams)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char name[] = "Dometic Product";
    int ddm_inst = -1;

    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, name, strlen(name));

    int err = -1;
    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    ddm_inst = ProdDBProdClassNodeCreate(NULL, 0, connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst == PROD_DB_ERR_INVALID_DATA);

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst > 0);

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0SN | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0SN | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0NAME | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0NAME | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0SKU | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a SUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0SKU | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0PNC | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PNC | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0FWVER | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0FWVER | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0HWVER | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0HWVER | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0MDL | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0MDL | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0EAN | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0EAN | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0DESCRIPTION | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0DESCRIPTION | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0MANUF | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0MANUF | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0UID | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0UID | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst)), NULL, 0, connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;

    ProdDBProdClassNodeDelete(ddm_inst);
}

extern "C" void prod_reset_handler(int32_t new_reset, int ddm_instance)
{
    // Simply publish the new value
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_instance)), &new_reset, sizeof(new_reset), connector_unittest.connector_id + ProductDatabaseTestRunFixture::getInstance()->data_line_owner, (TickType_t)portMAX_DELAY);
}

extern "C" void prod_indicate_handler(bool indicate, int ddm_instance)
{
    // Simply publish the new value
    EXPECT_TRUE(true);
}

TEST_F(ProductDatabaseTestRunFixture, SetProdParams_ReadCache)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    const char uid[] = "abcdef123456789";
    const char sn[] = "123456789";
    const char sn_updated[] = "987654321";
    const char mdl[] = "DICMProduct";
    const char mdl_updated[] = "DICM_Product";
    const char name[] = "Dometic Product";
    const char name_updated[] = "Dometic_Product";
    const char sku[] = "abcd123456";
    const char pnc[] = "9876fghj";
    const char manuf[] = "Dometic";
    const char hwver[] = "1.3.0";
    const char fwver[] = "v2.0.0";
    const char ean[] = "1234567891234";
    const char desc[] = "DometicDICMProduct";
    int32_t reset_cmd = PROD0RESET_IDLE;

    UPDLINKEDCLASS_T *clist = (UPDLINKEDCLASS_T *)hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(clist != NULL) << "clist should be a valid pointer " << std::endl;
    clist->updclass = MBAT0;
    clist->update[0] = 1;

    UPDLINKEDCLASS_T *clist_remove = (UPDLINKEDCLASS_T *)hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(clist_remove != NULL) << "clist_remove should be a valid pointer " << std::endl;
    clist_remove->updclass = MBAT0;
    clist_remove->update[0] = 0;

    int ddm_inst = -1;
    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, name, strlen(name));

    // Test multiple calls to ProdDBInit()
    int err = -1;
    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);
    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst != -1);

    uint8_t sa = 88;
    uint8_t type = 2;
    uint8_t inst = 1;
    uint32_t class_inst = AC0;

    vPortPauseScheduler();

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0SN | DDM2_PARAMETER_INSTANCE(ddm_inst)), sn, strlen(sn), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0SN | DDM2_PARAMETER_INSTANCE(ddm_inst)), sn_updated, strlen(sn_updated), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0SN | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0NAME | DDM2_PARAMETER_INSTANCE(ddm_inst)), name, strlen(name), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0NAME | DDM2_PARAMETER_INSTANCE(ddm_inst)), name_updated, strlen(name_updated), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0NAME | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0SKU | DDM2_PARAMETER_INSTANCE(ddm_inst)), sku, strlen(sku), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a SUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0SKU | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0SKU | DDM2_PARAMETER_INSTANCE(ddm_inst)), sku, strlen(sku), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0PNC | DDM2_PARAMETER_INSTANCE(ddm_inst)), pnc, strlen(pnc), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PNC | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0PNC | DDM2_PARAMETER_INSTANCE(ddm_inst)), pnc, strlen(pnc), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0FWVER | DDM2_PARAMETER_INSTANCE(ddm_inst)), fwver, strlen(fwver), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0FWVER | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0FWVER | DDM2_PARAMETER_INSTANCE(ddm_inst)), fwver, strlen(fwver), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0HWVER | DDM2_PARAMETER_INSTANCE(ddm_inst)), hwver, strlen(hwver), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0HWVER | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0HWVER | DDM2_PARAMETER_INSTANCE(ddm_inst)), hwver, strlen(hwver), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0MDL | DDM2_PARAMETER_INSTANCE(ddm_inst)), mdl, strlen(mdl), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0MDL | DDM2_PARAMETER_INSTANCE(ddm_inst)), mdl_updated, strlen(mdl_updated), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0MDL | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0EAN | DDM2_PARAMETER_INSTANCE(ddm_inst)), ean, strlen(ean), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0EAN | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0EAN | DDM2_PARAMETER_INSTANCE(ddm_inst)), ean, strlen(ean), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0DESCRIPTION | DDM2_PARAMETER_INSTANCE(ddm_inst)), desc, strlen(desc), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    // Test adding a class instance to PROD<X>CLIST without UPDLINKEDCLASS_T format
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst)), &class_inst, sizeof(class_inst), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.uint32 == AC0) << "The value for the CLIST should be AC0" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    // Test adding a class instance to PROD<X>CLIST with UPDLINKEDCLASS_T format
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst)), clist, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.uint32 == AC0) << "The value for the CLIST should be AC0" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    // Remove class instance from PROD<X>CLIST
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst)), clist_remove, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    // Test removal again with the same class instance
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst)), clist_remove, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst)), clist, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.uint32 == AC0) << "The value for the CLIST should be AC0" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0MANUF | DDM2_PARAMETER_INSTANCE(ddm_inst)), manuf, strlen(manuf), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0MANUF | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    // Now install reset handler
    ProdDBProdClassNodeAddIndicateHandler(ddm_inst, prod_indicate_handler);
    int32_t ind = 1;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0IND | DDM2_PARAMETER_INSTANCE(ddm_inst)), &ind, sizeof(ind), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent any DDMP2 frames, " << getNumSentDDMP2Frames() << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    reset_cmd = PROD0RESET_RESTART;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst)), &reset_cmd, sizeof(reset_cmd), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 2) << "We should have sent two DDMP2 frames, " << getNumSentDDMP2Frames() << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB " << res << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB " << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_NOT_SUPPORTED) << "We should have sent a PUB, " << myFrame.frame.publish.value.int32 << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB " << res << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB " << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_IDLE) << "We should have sent a PUB, " << myFrame.frame.publish.value.int32 << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;
    // Now install reset handler
    ProdDBProdClassNodeAddResetHandler(ddm_inst, prod_reset_handler);
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst)), &reset_cmd, sizeof(reset_cmd), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame, " << getNumSentDDMP2Frames() << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB " << res << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB " << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_RESTART) << "We should have sent a PUB, " << myFrame.frame.publish.value.int32 << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    reset_cmd = PROD0RESET_IDLE;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst)), &reset_cmd, sizeof(reset_cmd), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame, " << getNumSentDDMP2Frames() << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB " << res << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB " << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_IDLE) << "We should have sent a PUB, " << myFrame.frame.publish.value.int32 << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0MANUF | DDM2_PARAMETER_INSTANCE(ddm_inst)), manuf, strlen(manuf), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent PUB frame" << std::endl;

    connector_send_frame_to_broker(DDMP2_CONTROL_SET, (PROD0UID | DDM2_PARAMETER_INSTANCE(ddm_inst)), uid, strlen(uid), connector_unittest.connector_id + data_line_sub, (TickType_t)portMAX_DELAY);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not sent DDMP2 frame for UID" << std::endl;
    bool updated = false;
    updated = ProdDBUpdateCache(&sa, sizeof(uint8_t), FIELD_INVALID, ddm_inst);
    EXPECT_FALSE(updated) << "Expected no update for FIELD_INVALID" << std::endl;
    updated = ProdDBUpdateCache(&sa, sizeof(uint8_t), FIELD_PROP_SA, ddm_inst);
    EXPECT_TRUE(updated) << "PRODDB should be updated for FIELD_PROP_SA" << std::endl;
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent one DDMP2 frame" << std::endl;
    ProdDBUpdateCache(&sa, sizeof(uint8_t), FIELD_PROP, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    updated = ProdDBUpdateCache(&type, sizeof(uint8_t), FIELD_PROP_TYPE, ddm_inst);
    EXPECT_TRUE(updated) << "PRODDB should be updated for FIELD_PROP_TYPE" << std::endl;
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent one DDMP2 frame" << std::endl;
    ProdDBUpdateCache(&type, sizeof(uint8_t), FIELD_PROP, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    updated = ProdDBUpdateCache(&inst, sizeof(uint8_t), FIELD_PROP_INST, ddm_inst);
    EXPECT_TRUE(updated) << "PRODDB should be updated for FIELD_PROP_INST" << std::endl;
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent one DDMP2 frame" << std::endl;
    ProdDBUpdateCache(&inst, sizeof(uint8_t), FIELD_PROP, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    updated = ProdDBUpdateCache(&class_inst, sizeof(uint32_t), FIELD_PROP_CLASS, ddm_inst);
    EXPECT_TRUE(updated) << "PRODDB should be updated for FIELD_PROP_CLASS" << std::endl;
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent one DDMP2 frame" << std::endl;
    ProdDBUpdateCache(&class_inst, sizeof(uint32_t), FIELD_PROP, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    class_inst = MBAT0;
    updated = ProdDBUpdateCache(&class_inst, sizeof(uint32_t), FIELD_PROP_CLASS, ddm_inst);
    EXPECT_TRUE(updated) << "PRODDB should be updated for FIELD_PROP_CLASS" << std::endl;
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not have sent one DDMP2 frame" << std::endl;
    ProdDBUpdateCache(&class_inst, sizeof(uint32_t), FIELD_PROP, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    ProdDBUpdateCache(desc, strlen(desc), FIELD_DESC, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0DESCRIPTION | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    // Now clear reset handler
    ProdDBProdClassNodeAddResetHandler(ddm_inst, static_cast<prod_reset_handler_t>(NULL));

    // No reset handled installed. Testing default behaviour
    reset_cmd = PROD0RESET_IDLE;
    ProdDBUpdateCache(&reset_cmd, sizeof(reset_cmd), FIELD_RESET, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 0) << "We should not sent any DDMP2 frame, " << getNumSentDDMP2Frames() << std::endl;
    reset_cmd = PROD0RESET_RESET_TO_DEFAULT_SETTINGS;
    ProdDBUpdateCache(&reset_cmd, sizeof(reset_cmd), FIELD_RESET, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 2) << "We should have sent two DDMP2 frames, " << getNumSentDDMP2Frames() << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_NOT_SUPPORTED) << "We should have sent PROD0RESET_NOT_SUPPORTED" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_IDLE) << "We should have sent PROD0RESET_IDLE" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    reset_cmd = PROD0RESET_RESET_TO_FACTORY_SETTINGS;
    ProdDBUpdateCache(&reset_cmd, sizeof(reset_cmd), FIELD_RESET, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 2) << "We should have sent two DDMP2 frames" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_NOT_SUPPORTED) << "We should have sent PROD0RESET_NOT_SUPPORTED" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_IDLE) << "We should have sent PROD0RESET_IDLE" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    reset_cmd = PROD0RESET_RESTART;
    ProdDBUpdateCache(&reset_cmd, sizeof(reset_cmd), FIELD_RESET, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 2) << "We should have sent two DDMP2 frames" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_NOT_SUPPORTED) << "We should have sent PROD0RESET_NOT_SUPPORTED" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_IDLE) << "We should have sent PROD0RESET_IDLE" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    reset_cmd = PROD0RESET_CLEAR_FAULTS;
    ProdDBUpdateCache(&reset_cmd, sizeof(reset_cmd), FIELD_RESET, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 2) << "We should have sent two DDMP2 frames" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_NOT_SUPPORTED) << "We should have sent PROD0RESET_NOT_SUPPORTED" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_IDLE) << "We should have sent PROD0RESET_IDLE" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    // Now install reset handler
    ProdDBProdClassNodeAddResetHandler(ddm_inst, prod_reset_handler);
    reset_cmd = PROD0RESET_CLEAR_FAULTS;
    ProdDBUpdateCache(&reset_cmd, sizeof(reset_cmd), FIELD_RESET, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_CLEAR_FAULTS) << "We should have sent PROD0RESET_CLEAR_FAULTS" << std::endl;
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    reset_cmd = PROD0RESET_RESET_TO_DEFAULT_SETTINGS;
    ProdDBUpdateCache(&reset_cmd, sizeof(reset_cmd), FIELD_RESET, ddm_inst);
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB" << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0RESET | DDM2_PARAMETER_INSTANCE(ddm_inst))) << "We should have sent a PUB of PROD0RESET" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == PROD0RESET_RESET_TO_DEFAULT_SETTINGS) << "We should have sent PROD0RESET_RESET_TO_DEFAULT_SETTINGS" << std::endl;

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    res = -1;

    void *buffer = NULL;
    buffer = hal_mem_malloc_prefer(PROD_DB_MAX_FIELD_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(buffer != NULL);
    size_t buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_INVALID, ddm_inst, buffer, &buffer_size);
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    int invalid_ddm_instance = 255;
    ProdDBReadCache(FIELD_INVALID, invalid_ddm_instance, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_NAME, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, name_updated)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(name_updated));
    EXPECT_TRUE(buffer_size == strlen(name_updated));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_SN, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, sn_updated)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(sn_updated));
    EXPECT_TRUE(buffer_size == strlen(sn_updated));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_SKU, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, sku)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(sku));
    EXPECT_TRUE(buffer_size == strlen(sku));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_PNC, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, pnc)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(pnc));
    EXPECT_TRUE(buffer_size == strlen(pnc));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_FWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, fwver)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(fwver));
    EXPECT_TRUE(buffer_size == strlen(fwver));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_HWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, hwver)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(hwver));
    EXPECT_TRUE(buffer_size == strlen(hwver));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_MDL, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, mdl_updated)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(mdl_updated));
    EXPECT_TRUE(buffer_size == strlen(mdl_updated));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_EAN, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, ean)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(ean));
    EXPECT_TRUE(buffer_size == strlen(ean));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_DESC, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, desc)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(desc));
    EXPECT_TRUE(buffer_size == strlen(desc));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_MANUF, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, manuf)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(manuf));
    EXPECT_TRUE(buffer_size == strlen(manuf));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_UID, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(strlen((const char *)buffer) == 15);
    EXPECT_TRUE(buffer_size == 15);
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_PROP, ddm_inst, buffer, &buffer_size);
    PROD0PROP_T *prop_data = (PROD0PROP_T *)hal_mem_malloc_prefer(sizeof(PROD0PROP_T) + buffer_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    prop_data = (PROD0PROP_T *)buffer;
    EXPECT_TRUE(prop_data->addr == sa);
    EXPECT_TRUE(prop_data->type == type);
    EXPECT_TRUE(prop_data->inst == inst);
    uint32_t prop_instances[PROD_DB_MAX_FIELD_SIZE];
    memcpy(prop_instances, prop_data->classes, buffer_size);
    EXPECT_TRUE(prop_instances[0] == AC0);
    EXPECT_TRUE(prop_instances[1] == MBAT0);
    EXPECT_TRUE(buffer_size == (sizeof(PROD0PROP_T) + 2 * sizeof(uint32_t)));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_CLIST, ddm_inst, buffer, &buffer_size);
    uint32_t clist_instances[PROD_DB_MAX_FIELD_SIZE];
    memcpy(clist_instances, buffer, buffer_size);
    EXPECT_TRUE(clist_instances[0] == AC0);
    EXPECT_TRUE(buffer_size = sizeof(uint32_t));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_RESET, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(*(int32_t *)buffer == (int32_t)PROD0RESET_RESET_TO_DEFAULT_SETTINGS);
    EXPECT_TRUE(buffer_size == sizeof(int32_t));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    // Test with invalid data size
    buffer_size = 1;
    ProdDBReadCache(FIELD_MDL, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_SN, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_NAME, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_SKU, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_MANUF, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_HWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_FWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_EAN, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_DESC, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_CLIST, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_PROP, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_PNC, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_UID, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_RESET, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    // Test removing a node with invalid DDM instance
    ProdDBProdClassNodeDelete(invalid_ddm_instance);
    ProdDBProdClassNodeDelete(ddm_inst);
}

TEST_F(ProductDatabaseTestRunFixture, NodeCreate_MultipleTimes)
{
    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char name[] = "Dometic Product";

    int ddm_inst = -1;
    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    memset(pd_data, 0, sizeof(prod_database_t));
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, name, strlen(name));

    int err = -1;
    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst > 0) << "ddm instance should be valid" << std::endl;
    ddm_inst = -1;

    memset(pd_data->sn, 0, PROD_DB_MAX_FIELD_SIZE);

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst == PROD_DB_ERR_PRODUCT_ALREADY_EXISTS) << "ddm instance should not be valid" << std::endl;
    ddm_inst = -1;

    memset(pd_data->mdl, 0, PROD_DB_MAX_FIELD_SIZE);

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst == PROD_DB_ERR_PRODUCT_ALREADY_EXISTS) << "ddm instance should not be valid" << ddm_inst << std::endl;
    ddm_inst = -1;

    memset(pd_data->name, 0, PROD_DB_MAX_FIELD_SIZE);

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst == PROD_DB_ERR_NO_VALID_FIELD) << "ddm instance should not be valid" << std::endl;
    ddm_inst = -1;

    strncpy(pd_data->sn, sn, strlen(sn));

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst == PROD_DB_ERR_PRODUCT_ALREADY_EXISTS) << "ddm instance should not be valid" << std::endl;
}

TEST_F(ProductDatabaseTestRunFixture, LoadCache)
{
    const char mdl[] = "Dometic Product";
    const char sn[] = "123456789";
    const char name[] = "DICM Product";
    const char uid[] = "abcdefg12345678";

    size_t offset = 0;
    int err = -1;
    nvs_handle_t load_cache_handle;
    offset = 3 * PROD_DB_MAX_FIELD_SIZE;
    uint8_t *blob_data_to_set = (uint8_t *)hal_mem_malloc_prefer(offset, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(blob_data_to_set != NULL);

    memcpy(blob_data_to_set, mdl, strlen(mdl) + 1);
    memcpy(blob_data_to_set + strlen(mdl) + 1, name, strlen(name));
    memcpy(blob_data_to_set + strlen(mdl) + 1 + strlen(name) + 1, sn, strlen(sn));

    err = nvs_open("prod_db", NVS_READWRITE, &load_cache_handle);
    EXPECT_TRUE(err == 0);

    err = nvs_set_blob(load_cache_handle, uid, blob_data_to_set, offset);
    EXPECT_TRUE(err == 0);

    err = nvs_commit(load_cache_handle);
    EXPECT_TRUE(err == 0);

    hal_mem_free(blob_data_to_set);

    nvs_close(load_cache_handle);

    err = ProdDBInit();
    EXPECT_TRUE(err == 0);
    prod_database_t *pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);

    memset(pd_data, 0, sizeof(prod_database_t));
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, name, strlen(name));

    int ddm_inst = -1;
    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id + data_line_owner);
    EXPECT_TRUE(ddm_inst > 0);
    hal_mem_free(pd_data);
}

TEST_F(ProductDatabaseTestRunFixture, VirtualProdClassNodeCreate)
{
    char uid[] = "abcdefg12345678";
    char invalid_uid[] = "1234567890123456789";  // More than 15 characters

    int err = -1;
    err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK);

    int ddm_inst = 1;
    int non_existing_ddm_inst = 3;

    err = ProdDBVirtualProdClassNodeCreate(uid, ddm_inst);
    EXPECT_TRUE(err == ESP_OK) << "Virtual product class node should be created successfully" << std::endl;

    // Try to create the same virtual product class node again
    err = ProdDBVirtualProdClassNodeCreate(uid, ddm_inst);
    EXPECT_TRUE(err == PROD_DB_ERR_PROD_CLASS_ALREADY_EXISTS) << "Virtual product class node should not be created again" << std::endl;

    // Try to create a virtual product class node with an invalid DDM instance
    err = ProdDBVirtualProdClassNodeCreate(uid, -1);
    EXPECT_TRUE(err == PROD_DB_ERR_INVALID_DDM_INST) << "Virtual product class node should not be created with an invalid DDM instance" << std::endl;

    // Try to create a virtual product class node with an invalid UID
    err = ProdDBVirtualProdClassNodeCreate(invalid_uid, ddm_inst);
    EXPECT_TRUE(err == PROD_DB_ERR_INVALID_UID) << "Virtual product class node should not be created with an invalid UID"
                                                << " err: " << err << std::endl;

    // Check if the virtual product class node exists
    bool exists = ProdDBProdClassNodeExists(ddm_inst);
    EXPECT_TRUE(exists) << "Virtual product class node should exist" << std::endl;

    bool exists_invalid = ProdDBProdClassNodeExists(non_existing_ddm_inst);
    EXPECT_FALSE(exists_invalid) << "Virtual product class node should not exist for invalid DDM instance" << std::endl;

    int cached_ddm_inst = -1;
    size_t cached_ddm_inst_size = sizeof(cached_ddm_inst);

    int cached_conn_id = -1;
    size_t cached_conn_id_size = sizeof(cached_conn_id);

    // Read the cached DDM instance
    ProdDBReadCache(FIELD_DDM_INST, ddm_inst, &cached_ddm_inst, &cached_ddm_inst_size);
    EXPECT_TRUE(cached_ddm_inst == ddm_inst) << "Cached DDM instance should match the created one" << std::endl;

    // Read the cached connection ID
    ProdDBReadCache(FIELD_CONN_ID, ddm_inst, &cached_conn_id, &cached_conn_id_size);
    EXPECT_TRUE(cached_conn_id == INVALID_CONNECTOR_ID) << "Cached connection ID should be invalid for virtual prod class" << std::endl;

    // Remove the virtual product class node
    ProdDBProdClassNodeDelete(ddm_inst);

    // Check if the virtual product class node exists
    exists = ProdDBProdClassNodeExists(ddm_inst);
    EXPECT_FALSE(exists) << "Virtual product class node should not exist" << std::endl;
}

TEST_F(ProductDatabaseTestRunFixture, VirtualUpdateCache)
{
    const char uid[] = "abcdefg12345678";
    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char name[] = "Dometic Product";
    const char sku[] = "abcd123456";
    const char pnc[] = "9876fghj";
    const char manuf[] = "Dometic";
    const char hwver[] = "1.3.0";
    const char fwver[] = "v2.0.0";
    const char ean[] = "1234567891234";
    const char desc[] = "DometicDICMProduct";

    UPDLINKEDCLASS_T *clist = (UPDLINKEDCLASS_T *)hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(clist != NULL) << "clist should be a valid pointer " << std::endl;
    clist->updclass = MBAT0;
    clist->update[0] = 1;

    UPDLINKEDCLASS_T *clist_remove = (UPDLINKEDCLASS_T *)hal_mem_malloc_prefer(sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(clist_remove != NULL) << "clist_remove should be a valid pointer " << std::endl;
    clist_remove->updclass = MBAT0;
    clist_remove->update[0] = 0;

    int ddm_inst = 1;

    uint8_t sa = 88;
    uint8_t type = 2;
    uint8_t inst = 1;
    uint32_t class_inst = AC0;

    int err = ProdDBInit();
    EXPECT_TRUE(err == ESP_OK) << "Product database should be initialized successfully" << std::endl;

    err = ProdDBVirtualProdClassNodeCreate(uid, ddm_inst);
    EXPECT_TRUE(err == ESP_OK) << "Virtual product class node should be created successfully" << std::endl;

    ProdDBVirtualUpdateCache(&sa, sizeof(uint8_t), FIELD_INVALID, ddm_inst);

    ProdDBVirtualUpdateCache(&sa, sizeof(uint8_t), FIELD_PROP_SA, ddm_inst);

    ProdDBVirtualUpdateCache(&type, sizeof(uint8_t), FIELD_PROP_TYPE, ddm_inst);

    ProdDBVirtualUpdateCache(&inst, sizeof(uint8_t), FIELD_PROP_INST, ddm_inst);

    ProdDBVirtualUpdateCache(&class_inst, sizeof(uint32_t), FIELD_PROP_CLASS, ddm_inst);

    class_inst = MBAT0;
    ProdDBVirtualUpdateCache(&class_inst, sizeof(uint32_t), FIELD_PROP_CLASS, ddm_inst);

    ProdDBVirtualUpdateCache(desc, strlen(desc), FIELD_DESC, ddm_inst);

    ProdDBVirtualUpdateCache(name, strlen(name), FIELD_NAME, ddm_inst);

    ProdDBVirtualUpdateCache(sn, strlen(sn), FIELD_SN, ddm_inst);

    ProdDBVirtualUpdateCache(sku, strlen(sku), FIELD_SKU, ddm_inst);

    ProdDBVirtualUpdateCache(pnc, strlen(pnc), FIELD_PNC, ddm_inst);

    ProdDBVirtualUpdateCache(fwver, strlen(fwver), FIELD_FWVER, ddm_inst);

    ProdDBVirtualUpdateCache(hwver, strlen(hwver), FIELD_HWVER, ddm_inst);

    ProdDBVirtualUpdateCache(mdl, strlen(mdl), FIELD_MDL, ddm_inst);

    ProdDBVirtualUpdateCache(ean, strlen(ean), FIELD_EAN, ddm_inst);

    ProdDBVirtualUpdateCache(manuf, strlen(manuf), FIELD_MANUF, ddm_inst);

    ProdDBVirtualUpdateCache(clist, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), FIELD_CLIST, ddm_inst);

    // ProdDBVirtualUpdateCache(clist_remove, sizeof(UPDLINKEDCLASS_T) + sizeof(uint8_t), FIELD_CLIST, ddm_inst);

    void *buffer = NULL;
    buffer = hal_mem_malloc_prefer(PROD_DB_MAX_FIELD_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(buffer != NULL);
    size_t buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_INVALID, ddm_inst, buffer, &buffer_size);
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_NAME, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, name)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(name));
    EXPECT_TRUE(buffer_size == strlen(name));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_SN, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, sn)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(sn));
    EXPECT_TRUE(buffer_size == strlen(sn));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_SKU, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, sku)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(sku));
    EXPECT_TRUE(buffer_size == strlen(sku));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_PNC, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, pnc)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(pnc));
    EXPECT_TRUE(buffer_size == strlen(pnc));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_FWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, fwver)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(fwver));
    EXPECT_TRUE(buffer_size == strlen(fwver));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_HWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, hwver)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(hwver));
    EXPECT_TRUE(buffer_size == strlen(hwver));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_MDL, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, mdl)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(mdl));
    EXPECT_TRUE(buffer_size == strlen(mdl));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_EAN, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, ean)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(ean));
    EXPECT_TRUE(buffer_size == strlen(ean));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_DESC, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, desc)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(desc));
    EXPECT_TRUE(buffer_size == strlen(desc));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_MANUF, ddm_inst, buffer, &buffer_size);
    EXPECT_FALSE(strcmp((const char *)buffer, manuf)) << "Buffer is " << static_cast<char *>(buffer) << std::endl;
    EXPECT_TRUE(strlen((const char *)buffer) == strlen(manuf));
    EXPECT_TRUE(buffer_size == strlen(manuf));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_UID, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(strlen((const char *)buffer) == 15);
    EXPECT_TRUE(buffer_size == 15);
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_PROP, ddm_inst, buffer, &buffer_size);
    PROD0PROP_T *prop_data = (PROD0PROP_T *)hal_mem_malloc_prefer(sizeof(PROD0PROP_T) + buffer_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    prop_data = (PROD0PROP_T *)buffer;
    EXPECT_TRUE(prop_data->addr == sa);
    EXPECT_TRUE(prop_data->type == type);
    EXPECT_TRUE(prop_data->inst == inst);
    uint32_t prop_instances[PROD_DB_MAX_FIELD_SIZE];
    memcpy(prop_instances, prop_data->classes, buffer_size);
    EXPECT_TRUE(prop_instances[0] == AC0);
    EXPECT_TRUE(prop_instances[1] == MBAT0);
    EXPECT_TRUE(buffer_size == (sizeof(PROD0PROP_T) + 2 * sizeof(uint32_t)));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    ProdDBReadCache(FIELD_CLIST, ddm_inst, buffer, &buffer_size);
    uint32_t clist_instances[PROD_DB_MAX_FIELD_SIZE];
    memcpy(clist_instances, buffer, buffer_size);
    EXPECT_TRUE(clist_instances[0] == MBAT0);
    EXPECT_TRUE(buffer_size == sizeof(uint32_t));
    memset(buffer, 0, PROD_DB_MAX_FIELD_SIZE);
    buffer_size = PROD_DB_MAX_FIELD_SIZE;

    // Test with invalid data size
    buffer_size = 1;
    ProdDBReadCache(FIELD_MDL, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_SN, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_NAME, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_SKU, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_MANUF, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_HWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_FWVER, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_EAN, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_DESC, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_CLIST, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_PROP, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_PNC, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    buffer_size = 1;
    ProdDBReadCache(FIELD_UID, ddm_inst, buffer, &buffer_size);
    EXPECT_TRUE(buffer_size == 0);

    ProdDBProdClassNodeDelete(ddm_inst);
}

extern "C" void disable_connectors(void)
{
    LOG(I, "Disable smarteco connector and climate zone connector");
    connector_smart_eco_feature.disabled = 1;
    connector_climate_zone_feature.disabled = 1;
}
