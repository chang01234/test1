/**
 * @file test_unittest.cpp
 * @brief The intention of this file is to show a very simple template implementation
 * that utilizes the multi connector properties of the unittest connector.
 *
 * @date 14 feb. 2025
 * @author Andreas Lundeen
 */

extern "C" {
#include "broker.h"
#include "configuration.h"
#include "connector_unittest.h"
#include "ddm2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void task_handler_0(DDMP2_FRAME *pframe);
static void init_task_cb_0(void);
static void task_handler_1(DDMP2_FRAME *pframe);
static void init_task_cb_1(void);
}
#include "DICMFrameworkTestFixture.hpp"
using ::testing::Test;
class UnitTestHandlerTestRunFixture : public DICMFrameworkTestFixture
{
  protected:
    void SetUp() override
    {
        DICMFrameworkTestFixture::SetUp();
        setConnectorId(&connector_unittest.connector_id);
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};

extern "C" void task_handler_0(DDMP2_FRAME *pframe)
{
    EXPECT_TRUE(pframe->frame.control == DDMP2_CONTROL_SUBSCRIBE) << "We should have received a DDMP2_CONTROL_SUBSCRIBE" << std::endl;
    EXPECT_TRUE(pframe->frame.subscribe.parameter == AC0MDL) << "We should have received a DDMP2_CONTROL_SUBSCRIBE for AC0MDL" << std::endl;
    LOG(I, "Received SUBSCRIBE of AC0MDL");

    int32_t data = 0x12345678;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, AC0MDL, &data, sizeof(data), connector_unittest.connector_id + 0, portMAX_DELAY);
    LOG(I, "Sent PUBLISH of AC0MDL: 0x12345678");
}

extern "C" void task_handler_1(DDMP2_FRAME *pframe)
{
    EXPECT_TRUE(pframe->frame.control == DDMP2_CONTROL_PUBLISH) << "We should have received a DDMP2_CONTROL_PUBLISH" << std::endl;
    EXPECT_TRUE(pframe->frame.publish.parameter == AC0MDL) << "We should have received a DDMP2_CONTROL_PUBLISH of AC0MDL" << std::endl;
    EXPECT_TRUE(pframe->frame.publish.value.int32 == 0x12345678) << "We should have received a 0x12345678 as data of AC0MDL" << std::endl;
    LOG(I, "Received PUBLISH of AC0MDL: 0x12345678");
}

extern "C" void init_task_cb_0(void)
{
    uint32_t device_class_instance = AC0;
    // Discard return value as we do not care if it fails, it just means that the instance is already registered
    (void)broker_register_instance(&device_class_instance, connector_unittest.connector_id + 0);
}

extern "C" void init_task_cb_1(void)
{
    uint32_t device_class_instance = AC0MDL;
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance, NULL, 0, connector_unittest.connector_id + 1, portMAX_DELAY);
}

TEST_F(UnitTestHandlerTestRunFixture, template)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    connector_unittest_enable(init_task_cb_0, task_handler_0);
    connector_unittest_enable_indexed_connector(init_task_cb_1, task_handler_1, 1);
    DICMFrameworkTestFixture::SetupFramework();

    vPortPauseScheduler();

    vPortExecuteTick();
    vPortExecuteTick();
    // As this API only checks the base connector ID
    EXPECT_TRUE(getNumSentDDMP2Frames() == 2) << "We should have sent two DDMP2 frames" << std::endl;
    // First should be the PUB since registering the AC0 class
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB of AC0AVL" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB of AC0AVL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == AC0AVL) << "We should have sent a PUB of AC0AVL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == One) << "We should have sent a PUB of AC0AVL" << std::endl;

    res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a PUB of AC0MDL" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB of AC0MDL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == AC0MDL) << "We should have sent a PUB of AC0MDL" << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.value.int32 == 0x12345678) << "We should have sent a PUB of AC0MDL" << std::endl;

    EXPECT_TRUE(getNumReceivedDDMP2Frames() == 1) << "We should have received one DDMP2 frame" << std::endl;
    res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a SUB of AC0MDL" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_SUBSCRIBE) << "We should have sent a PUB of AC0MDL" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == AC0MDL) << "We should have sent a PUB of AC0MDL" << std::endl;
}
