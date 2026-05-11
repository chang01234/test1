/*
 * test_connector_climate_zone.cpp
 *
 *  Created on: 22 aug. 2025
 *      Author: Andlun
 */

extern "C" {
#include "broker.h"
#include "configuration.h"
#include "connector_climate_zone_feature.h"
#include "connector_smart_eco_feature.h"
#include "connector_unittest.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "product_database.h"
static void task_handler_prod(DDMP2_FRAME *pframe);
}
#include "DICMFrameworkTestFixture.hpp"

class ConnectorClimateZoneFeatureTestFixture : public DICMFrameworkTestFixture
{
  protected:
    void SetUp() override
    {
        DICMFrameworkTestFixture::SetUp();
        esp_log_level_set("feature_database.c", ESP_LOG_DEBUG);
        connector_unittest_enable(NULL, task_handler_prod);
        setConnectorId(&connector_climate_zone_feature.connector_id);
        DICMFrameworkTestFixture::SetupFramework();
        esp_log_level_set("product_database.c", ESP_LOG_DEBUG);
        esp_log_level_set("feature_database.c", ESP_LOG_DEBUG);
        esp_log_level_set("connector_climate_zone_feature.c", ESP_LOG_DEBUG);
        ProdDBInit();
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};
extern "C" void task_handler_prod(DDMP2_FRAME *pframe)
{
    ProdDBFrameHandler(pframe);
}

// Make sure we have created the id == 0 group type and subscribed to inventory
TEST_F(ConnectorClimateZoneFeatureTestFixture, init_connector)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    vPortPauseScheduler();
    int res = getNumSentDDMP2Frames();
    EXPECT_EQ(res, 2) << "We should have sent 2 frames during startup: " << res << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB for GROUP0AVL" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB for GROUP0AVL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == GROUP0AVL) << "We should have sent a PUB for GROUP0AVL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == 1) << "We should have sent a PUB for GROUP0AVL" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a SUB for GW0INV" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE) << "We should have sent a SUB for GW0INV" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == GW0INV) << "We should have sent a SUB for GW0INV" << std::endl;

    vPortExecuteTick();
    res = getNumReceivedDDMP2Frames();
    EXPECT_EQ(res, 1) << "We should have received 1 frame" << res << std::endl;
    res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
    EXPECT_EQ(res, 0) << "We should have received a frame: " << res << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have received a PUB for GW0INV" << std::endl;
    EXPECT_EQ(myFrame.frame.publish.parameter, GW0INV) << "We should have received a PUB for GW0INV" << std::endl;

    // No more messages
    TickType_t loop_ticks = 0;
    while (loop_ticks++ < pdMS_TO_TICKS(1000) - 1)
    {
        vPortExecuteTick();
    }
    res = getNumReceivedDDMP2Frames();
    EXPECT_EQ(res, 0) << "We should not have received more frames (1000ms)" << res << std::endl;
    res = getNumSentDDMP2Frames();
    EXPECT_EQ(res, 0) << "We should have sent more frames (1000ms)" << res << std::endl;

    // Simulate a new product is detected in the system

    vPortResumeScheduler();
}

// Simulate a new climate product is detected in the system
/*typedef struct prod_database
{
    char mdl[PROD_DB_MAX_FIELD_SIZE];
    char sn[PROD_DB_MAX_FIELD_SIZE];
    char name[PROD_DB_MAX_FIELD_SIZE];
    char uid[DICM_UID_KEY_STR_LEN];
} prod_database_t;
*/
TEST_F(ConnectorClimateZoneFeatureTestFixture, valid_prod_detected)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    vPortPauseScheduler();
    vPortExecuteTick();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    const char sn[] = "123456789";
    const char mdl[] = "DICMProduct";
    const char name[] = "Dometic Product";
    const char uid[] = "123456789012345";
    int ddm_inst = -1;

    prod_database_t *pd_data = NULL;
    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(pd_data != NULL);
    strncpy(pd_data->mdl, mdl, strlen(mdl));
    strncpy(pd_data->sn, sn, strlen(sn));
    strncpy(pd_data->name, name, strlen(name));
    strncpy(pd_data->uid, name, strlen(uid));

    ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id);
    hal_mem_free(pd_data);
    EXPECT_TRUE(ddm_inst > 0);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    uint8_t sa = 2;
    uint8_t inst = 4;
    prodxprop_type_t type;
    type.data = 0;
    type.type.cls = PRODXPROP_TYPE_CLASS_CLIMATE;
    ProdDBUpdateCache((const void *)&sa, sizeof(uint8_t), FIELD_PROP_SA, ddm_inst);
    ProdDBUpdateCache((const void *)&inst, sizeof(uint8_t), FIELD_PROP_INST, ddm_inst);
    ProdDBUpdateCache((const void *)&type, sizeof(uint8_t), FIELD_PROP_TYPE, ddm_inst);
    ProdDBUpdateCache((const void *)&type, sizeof(uint8_t), FIELD_PROP, ddm_inst);
    // A new prod should create a SUB on PROD<ddm_inst>PROP
    // A valid
    int res = getNumSentDDMP2Frames();
    EXPECT_EQ(res, 2) << "We should have sent two frame " << res << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB for GROUP1AVL" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB for GROUP1AVL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0AVL | DDM2_PARAMETER_INSTANCE(1))) << "We should have sent a PUB for GROUP1AVL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == 1) << "We should have sent a PUB for GROUP1AVL" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB for GROUP1INTERFACE" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB for GROUP1INTERFACE" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (GROUP0INTERFACE | DDM2_PARAMETER_INSTANCE(1))) << "We should have sent a PUB for GROUP1INTERFACE" << std::endl;
    EXPECT_EQ(myFrame.frame.publish.value.uint32, PROD0 | DDM2_PARAMETER_INSTANCE(ddm_inst)) << "We should have sent a PROD<ddm_inst> in GROUP1INTERFACE" << std::endl;
    vPortResumeScheduler();
}

extern "C" void disable_connectors(void)
{
    LOG(I, "Disable smarteco connector");
    connector_smart_eco_feature.disabled = 1;
}
