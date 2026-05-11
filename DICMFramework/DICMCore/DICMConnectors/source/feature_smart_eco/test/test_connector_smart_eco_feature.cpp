/*
 * test_connector_climate_zone.cpp
 *
 *  Created on: 22 aug. 2025
 *      Author: Andlun
 */

#include <cstddef>
extern "C" {
#include <stdint.h>

#include "configuration.h"

#include "broker.h"
#include "connector_climate_zone_feature.h"
#include "connector_fc_rule_engine.h"
#include "connector_smart_eco_feature.h"
#include "connector_unittest.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "feature_database.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "product_database.h"
static void task_handler_prod(DDMP2_FRAME *pframe);
}
#include "DICMFrameworkTestFixture.hpp"

using ::testing::Test;

class ConnectorSmartEcoFeatureTestFixture : public DICMFrameworkTestFixture
{
  protected:
    void SetUp() override
    {
        DICMFrameworkTestFixture::SetUp();
        esp_log_level_set("feature_database.c", ESP_LOG_DEBUG);
        connector_unittest_enable(NULL, task_handler_prod);
        setConnectorId(&connector_smart_eco_feature.connector_id);
        DICMFrameworkTestFixture::SetupFramework();
        esp_log_level_set("product_conf_manager.c", ESP_LOG_DEBUG);
        esp_log_level_set("product_database.c", ESP_LOG_DEBUG);
        esp_log_level_set("feature_database.c", ESP_LOG_DEBUG);
        esp_log_level_set("connector_smart_eco_feature.c", ESP_LOG_DEBUG);
        ProdDBInit();
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};
extern "C" void task_handler_prod(DDMP2_FRAME *pframe)
{
    static int ddm_inst = -1;
    static int ac_ddm_inst = -1;
    static int group_inst = -1;
    if (!ProdDBFrameHandler(pframe))
    {
        if (!feat_db_frame_handler(pframe))
        {
            switch (pframe->frame.control)
            {
            case DDMP2_CONTROL_GENERIC:
                switch (pframe->frame.generic.id)
                {
                case 0:
                {
                    // Create a new product
                    const char sn[] = "123456789";
                    const char mdl[] = "DICMProduct";
                    const char name[] = "Dometic NDS";
                    const char uid[] = "123456789012345";

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
                    uint8_t sa = 2;
                    uint8_t inst = 4;
                    prodxprop_type_t type;
                    type.data = 0;
                    type.type.cls = PRODXPROP_TYPE_CLASS_POWER;
                    type.type.intf = PRODXPROP_TYPE_INTERFACE_UNKNOWN;
                    ProdDBUpdateCache((const void *)&name, strlen(name), FIELD_MANUF, ddm_inst);
                    ProdDBUpdateCache((const void *)&sa, sizeof(uint8_t), FIELD_PROP_SA, ddm_inst);
                    ProdDBUpdateCache((const void *)&inst, sizeof(uint8_t), FIELD_PROP_INST, ddm_inst);
                    ProdDBUpdateCache((const void *)&type, sizeof(uint8_t), FIELD_PROP_TYPE, ddm_inst);
                    ProdDBUpdateCache((const void *)&type, sizeof(uint8_t), FIELD_PROP, ddm_inst);
                    break;
                }
                case 1:
                {
                    // Add new class to clist
                    uint32_t new_class = *(uint32_t *)pframe->frame.generic.data;
                    ProdDBUpdateCache((const void *)&new_class, sizeof(new_class), FIELD_CLIST, ddm_inst);
                    break;
                }
                case 2:
                {
                    // Create a new product
                    const char sn[] = "923456781";
                    const char mdl[] = "DICM AC Product";
                    const char name[] = "Dometic";
                    const char uid[] = "923456789012349";

                    prod_database_t *pd_data = NULL;
                    pd_data = (prod_database_t *)hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                    EXPECT_TRUE(pd_data != NULL);
                    strncpy(pd_data->mdl, mdl, strlen(mdl));
                    strncpy(pd_data->sn, sn, strlen(sn));
                    strncpy(pd_data->name, name, strlen(name));
                    strncpy(pd_data->uid, name, strlen(uid));

                    ac_ddm_inst = ProdDBProdClassNodeCreate(pd_data, sizeof(prod_database_t), connector_unittest.connector_id);
                    hal_mem_free(pd_data);
                    EXPECT_TRUE(ac_ddm_inst > 0);
                    uint8_t sa = 1;
                    uint8_t inst = 1;
                    prodxprop_type_t type;
                    type.data = 0;
                    type.type.cls = PRODXPROP_TYPE_CLASS_CLIMATE;
                    type.type.intf = PRODXPROP_TYPE_INTERFACE_UNKNOWN;
                    ProdDBUpdateCache((const void *)&name, strlen(name), FIELD_MANUF, ac_ddm_inst);
                    ProdDBUpdateCache((const void *)&sa, sizeof(uint8_t), FIELD_PROP_SA, ac_ddm_inst);
                    ProdDBUpdateCache((const void *)&inst, sizeof(uint8_t), FIELD_PROP_INST, ac_ddm_inst);
                    ProdDBUpdateCache((const void *)&type, sizeof(uint8_t), FIELD_PROP_TYPE, ac_ddm_inst);
                    ProdDBUpdateCache((const void *)&type, sizeof(uint8_t), FIELD_PROP, ac_ddm_inst);
                    ProdDBUpdateCache((const void *)"2.0.0", strlen("2.0.0"), FIELD_FWVER, ac_ddm_inst);

                    break;
                }
                case 3:
                {
                    // Add new group climate zone to
                    uint32_t new_class = *(uint32_t *)pframe->frame.generic.data;
                    ProdDBUpdateCache((const void *)&new_class, sizeof(new_class), FIELD_CLIST, ac_ddm_inst);
                    break;
                }
                case 4:
                {
                    // Add/create the active climate zone
                    feat_db_cache_entry_create(GROUP0TYPE_CLIMATEZONE, connector_unittest.connector_id, 1, true, true, &group_inst);
                    break;
                }
                case 100:
                {
                    // Set new prio of unittest task (xTASK_PRIORITY_NORMAL)
                    vTaskPrioritySet(NULL, (UBaseType_t)pframe->frame.generic.data[0]);
                    break;
                }
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }
}

// Make sure we have created the id == 0 group type and subscribed to inventory
TEST_F(ConnectorSmartEcoFeatureTestFixture, init_connector)
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
TEST_F(ConnectorSmartEcoFeatureTestFixture, no_zone_valid_bat_prod_detected_at_highprio)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int ddm_inst = 1;

    vPortPauseScheduler();

    clearReceivedDDMP2Frames();
    uint8_t newprio = xTASK_PRIORITY_NORMAL + 1;
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 100, &newprio, 1, connector_unittest.connector_id, portMAX_DELAY);
    clearSentDDMP2Frames();
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 0, NULL, 0, connector_unittest.connector_id, portMAX_DELAY);
    // A new prod should create a SUB on PROD<ddm_inst>PROP
    // reception of PROD1PROP should generate a SUB on PROD1CLIST and UNSUB PROD1PROP
    int res = getNumSentDDMP2Frames();
    EXPECT_EQ(res, 3) << "We should have sent three frames " << res << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a SUB for PROD1PROP" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE) << "We should have sent a SUB for PROD1PROP" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(1))) << "We should have sent a SUB for PROD1PROP" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a SUB for PROD1CLIST" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE) << "We should have sent a SUB for PROD1CLIST" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(1))) << "We should have sent a SUB for PROD1CLIST" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a UNSUB for PROD1PROP" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_UNSUBSCRIBE) << "We should have sent a UNSUB for PROD1PROP" << std::endl;
    EXPECT_TRUE(myFrame.frame.unsubscribe.parameter == (PROD0PROP | DDM2_PARAMETER_INSTANCE(1))) << "We should have sent a UNSUB for PROD1PROP" << std::endl;

    vPortExecuteTick();
    // Now add the mbat product
    clearReceivedDDMP2Frames();
    clearSentDDMP2Frames();
    uint32_t new_class = MBAT0;
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 1, &new_class, sizeof(new_class), connector_unittest.connector_id, portMAX_DELAY);
    res = getNumReceivedDDMP2Frames();
    // PRODxCLIST
    EXPECT_EQ(res, 2) << "We should have received two frame" << res << std::endl;
    res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
    EXPECT_EQ(res, 0) << "We should have received a frame: " << res << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have received a PUB for PROD1CLIST" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(1))) << "We should have received a PUB for PROD1CLIST" << std::endl;
    EXPECT_EQ(ddmp2_value_size(&myFrame), 4) << "We should have received a PUB for PROD1CLIST of size 4" << std::endl;
    EXPECT_EQ(myFrame.frame.publish.value.uint32, MBAT0) << "We should have received a PUB for PROD1CLIST with data MBAT0" << std::endl;

    vPortExecuteTick();

    // PRODxMANUF
    res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
    EXPECT_EQ(res, 0) << "We should have received a frame: " << res << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have received a PUB for PROD0MANUF" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0MANUF | DDM2_PARAMETER_INSTANCE(1))) << "We should have received a PUB for PROD0MANUF" << std::endl;

    vPortExecuteTick();

    res = getNumSentDDMP2Frames();
    EXPECT_EQ(res, 2) << "We should have sent two frames " << res << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a SUB for PROD1CLIST" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_UNSUBSCRIBE) << "We should have sent a UNSUB for PROD1CLIST" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(1))) << "We should have sent a UNSUB for PROD1CLIST" << std::endl;
    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a SUB for PROD1MANUF" << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_SUBSCRIBE) << "We should have sent a SUB for PROD1MANUF" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == (PROD0MANUF | DDM2_PARAMETER_INSTANCE(1))) << "We should have sent a SUB for PROD1MANUF" << std::endl;

    vPortResumeScheduler();
}

TEST_F(ConnectorSmartEcoFeatureTestFixture, enabled_zone_valid_bat_prod_detected_at_highprio)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int ddm_inst = 1;
    int res = 0;

    vPortPauseScheduler();

    clearReceivedDDMP2Frames();
    uint8_t newprio = xTASK_PRIORITY_NORMAL + 1;
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 100, &newprio, 1, connector_unittest.connector_id, portMAX_DELAY);
    clearSentDDMP2Frames();
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 0, NULL, 0, connector_unittest.connector_id, portMAX_DELAY);
    vPortExecuteTick();
    // Now add the mbat product
    uint32_t new_class = MBAT0;
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 1, &new_class, sizeof(new_class), connector_unittest.connector_id, portMAX_DELAY);
    vPortExecuteTick();
    clearReceivedDDMP2Frames();
    clearSentDDMP2Frames();
    // Now we need to receive a valid climate zone with an AC0 class
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 2, NULL, 0, connector_unittest.connector_id, portMAX_DELAY);
    res = getNumSentDDMP2Frames();
    EXPECT_EQ(res, 3) << "We should have sent three frames " << res << std::endl;
    clearReceivedDDMP2Frames();
    clearSentDDMP2Frames();

    // PRODxCLIST
    new_class = AC0;
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 3, &new_class, sizeof(new_class), connector_unittest.connector_id, portMAX_DELAY);
    res = getNumReceivedDDMP2Frames();
    EXPECT_EQ(res, 2) << "We should have received two frame" << res << std::endl;
    res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
    EXPECT_EQ(res, 0) << "We should have received a frame: " << res << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have received a PUB for PROD2CLIST" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0CLIST | DDM2_PARAMETER_INSTANCE(2))) << "We should have received a PUB for PROD2CLIST" << std::endl;
    EXPECT_EQ(ddmp2_value_size(&myFrame), 4) << "We should have received a PUB for PROD2CLIST of size 4" << std::endl;
    EXPECT_EQ(myFrame.frame.publish.value.uint32, AC0) << "We should have received a PUB for PROD2CLIST with data MBAT0" << std::endl;

    // PRODxFWVER
    res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
    EXPECT_EQ(res, 0) << "We should have received a frame: " << res << std::endl;
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "We should have received a PUB for PROD2FWVER" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == (PROD0FWVER | DDM2_PARAMETER_INSTANCE(2))) << "We should have received a PUB for PROD2FWVER" << std::endl;
    char buffer[30];
    ddmp2_extract_string_from_frame(&myFrame, buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "2.0.0") << "We should have received a PUB for PROD2FWVER" << std::endl;

    // GROUPxENABLE and GROUPxACTIVE
    vPortExecuteTick();
    clearReceivedDDMP2Frames();
    clearSentDDMP2Frames();
    // Create CLIMATE zone
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, 4, NULL, 0, connector_unittest.connector_id, portMAX_DELAY);
    vPortExecuteTick();
    res = getNumReceivedDDMP2Frames();
    // Now we should have created our own SMARTECO feature with interface classes (CLIMATE_ZONE group instance and PROD (mbat) instance)
    int32_t groups[2];
    size_t num_groups = 2;
    feat_db_get_all_enabled_instances_of_type(groups, &num_groups, GROUP0TYPE_CLIMATEZONE);
    EXPECT_EQ(num_groups, 1) << "We should have one enabled climate zone: " << num_groups << std::endl;
    if (num_groups)
    {
        EXPECT_EQ(groups[0], 1) << "We should have one enabled climate zone: " << groups[0] << std::endl;
    }
    // Now we should have created our own SMARTECO feature with interface classes (CLIMATE_ZONE group instance and PROD (mbat) instance)
    num_groups = 2;
    feat_db_get_all_enabled_instances_of_type(groups, &num_groups, GROUP0TYPE_SMARTECO);
    EXPECT_EQ(num_groups, 1) << "We should have one enabled Smart ECO feature: " << num_groups << std::endl;
    if (num_groups == 1)
    {
        EXPECT_EQ(groups[0], 2) << "We should have one enabled Smart ECO feature: " << groups[0] << std::endl;

        // Validate the linkage
        uint32_t classes[5];
        size_t num_classes = 5 * sizeof(uint32_t);

        feat_db_read_cache(FEAT_DB_FIELD_INTERFACE_CLASS_INST, 2, classes, &num_classes);
        EXPECT_EQ(num_classes, 2 * sizeof(uint32_t)) << "We should have two linked classes: " << num_classes << std::endl;
        if (num_classes == 2 * sizeof(uint32_t))
        {
            // order is not important here
            if (classes[0] == (GROUP0 | DDM2_PARAMETER_INSTANCE(1)))
            {
                EXPECT_EQ(classes[0], GROUP0 | DDM2_PARAMETER_INSTANCE(1)) << "We should have linked class GROUP1: " << classes[0] << std::endl;
                EXPECT_EQ(classes[1], PROD0 | DDM2_PARAMETER_INSTANCE(1)) << "We should have linked class PROD1: " << classes[1] << std::endl;
            }
            else
            {
                EXPECT_EQ(classes[1], GROUP0 | DDM2_PARAMETER_INSTANCE(1)) << "We should have linked class GROUP1: " << classes[0] << std::endl;
                EXPECT_EQ(classes[0], PROD0 | DDM2_PARAMETER_INSTANCE(1)) << "We should have linked class PROD1: " << classes[1] << std::endl;
            }
        }
        int32_t enabled = false;
        size_t enable_size = sizeof(enabled);
        feat_db_read_cache(FEAT_DB_FIELD_ENABLE, 2, &enabled, &enable_size);
        EXPECT_EQ(enabled, true) << "Smart ECO feature shall be true: " << enabled << std::endl;
    }

    vPortResumeScheduler();
}

extern "C" void disable_connectors(void)
{
    LOG(I, "Disable Climate zone connector");
    connector_climate_zone_feature.disabled = 1;
    LOG(I, "Disable FC rule engine connector");
    connector_fc_rule_engine.disabled = 1;
}
