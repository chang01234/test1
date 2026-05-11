/*
 * test_inventory_handler_run.cpp
 *
 *  Created on: 14 nov. 2024
 *      Author: Andlun
 */


extern "C" {
#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "broker.h"
#include "inventory_handler.h"
#include "sorted_list.h"
#include "connector_unittest.h"
#include "ddm2.h"
static void task_handler(DDMP2_FRAME *pframe);
static void init_task_cb(void);
static void inventory_handler_available_cb(void * argument, uint32_t device_class_instance, bool is_available);
}
#include "DICMFrameworkTestFixture.hpp"
using ::testing::Test;
class InventoryHandlerTestRunFixture : public DICMFrameworkTestFixture {
public:
    static InventoryHandlerTestRunFixture *getInstance()
    {
        return mTestInstance;
    }
    static inline InventoryHandlerTestRunFixture *mTestInstance = NULL;
    InventoryHandlerTestRunFixture() : DICMFrameworkTestFixture()
    {
        mTestInstance = this;
    }
    inventory_handler_t ih = { .sorted_list = NULL, .cb_available = NULL, .cb_argument = NULL};
    DECLARE_SORTED_LIST_PUBLIC(sorted_list, 100);
protected:
	void SetUp() override
    {
        connector_unittest_enable(init_task_cb, task_handler);
        sorted_list_clear(&sorted_list);
		DICMFrameworkTestFixture::SetUp();
		setConnectorId(&connector_unittest.connector_id);
		DICMFrameworkTestFixture::SetupFramework();
    }

    void TearDown() override
    {
    	DICMFrameworkTestFixture::TearDown();
    }
};

extern "C" void task_handler(DDMP2_FRAME *pframe)
{
    inventory_handler_update(&InventoryHandlerTestRunFixture::getInstance()->ih, pframe);
}
extern "C" void init_task_cb(void)
{
    uint32_t device_class_instance = AC0MDL;
    EXPECT_TRUE(inventory_handler_init(&InventoryHandlerTestRunFixture::getInstance()->ih, &InventoryHandlerTestRunFixture::getInstance()->sorted_list, inventory_handler_available_cb, NULL) == 0) << "inventory_handler_init() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add(&InventoryHandlerTestRunFixture::getInstance()->ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_start(&InventoryHandlerTestRunFixture::getInstance()->ih, &connector_unittest) == 0) << "inventory_handler_start() should return 0" << std::endl;
}

extern "C" void inventory_handler_available_cb(void * argument, uint32_t device_class_instance, bool is_available)
{
    if (is_available)
    {
        EXPECT_EQ(device_class_instance, AC0) << "device_class_instance AC0 should be available" << std::endl;
    }
    else
    {
        EXPECT_TRUE(false) << "device_class_instance AC0 should be available" << std::endl;        
    }
}

TEST_F(InventoryHandlerTestRunFixture, start)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    vPortPauseScheduler();

    vPortExecuteTick();
    vPortExecuteTick();
  
    // Check that subscribe of inventory has been sent
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "We should have sent a SUB for gw0inv" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_SUBSCRIBE) << "We should have sent a SUB for gw0inv" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == GW0INV) << "We should have sent a SUB for gw0inv" << std::endl;

}

TEST_F(InventoryHandlerTestRunFixture, update)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    uint32_t device_class = AC0;
    vPortPauseScheduler();

    vPortExecuteTick();
    vPortExecuteTick();
  
    // Check that subscribe of inventory has been sent
    EXPECT_TRUE(getNumSentDDMP2Frames() == 1) << "We should have sent one DDMP2 frame" << std::endl;
    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
  
    EXPECT_FALSE(res) << "We should have sent a SUB for gw0inv" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_SUBSCRIBE) << "We should have sent a SUB for gw0inv" << std::endl;
    EXPECT_TRUE(myFrame.frame.subscribe.parameter == GW0INV) << "We should have sent a SUB for gw0inv" << std::endl;
    
    // Now we wait until AC0 becomes available
    vPortExecuteTick();
    vPortExecuteTick();
    vPortExecuteTick();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    vPortExecuteTick();
    vPortExecuteTick();
    int instance = broker_register_instance(&device_class, 0); // Use first connector id that is different from ours
    EXPECT_TRUE(instance == 0) << "We should have requested instance 0" << std::endl;
    EXPECT_TRUE(getNumReceivedDDMP2Frames() == 1) << "We should have received one DDMP2 frame" << std::endl;
    frame_size = 0;
    res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
    EXPECT_TRUE(frame_size != 0) << "We should have received size != 0" << std::endl;
    EXPECT_FALSE(res) << "We should have received a PUB for gw0inv" << std::endl;
    EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have received a PUB for gw0inv: " << (int)myFrame.frame.control << std::endl;
    EXPECT_TRUE(myFrame.frame.publish.parameter == GW0INV) << "We should have received a PUB for gw0inv " << (int)myFrame.frame.publish.parameter << std::endl;
}
