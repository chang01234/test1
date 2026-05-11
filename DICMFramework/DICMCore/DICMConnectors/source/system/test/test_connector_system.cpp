/*
 * test_connector_system.cpp
 *
 *  Created on: 14 nov. 2023
 *      Author: Andlun
 */


extern "C" {
#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal_initialize.h"
#include "hal_cpu.h"
#include "broker.h"
#include "connector_system.h"
}
#include "DICMFrameworkTestFixture.hpp"

class ConnectorSystemTestFixture : public DICMFrameworkTestFixture {
protected:

	void SetUp() override
    {
		DICMFrameworkTestFixture::SetUp();
		setConnectorId(&connector_system.connector_id);
		DICMFrameworkTestFixture::SetupFramework();
    }

    void TearDown() override
    {
    	DICMFrameworkTestFixture::TearDown();
    }
};

TEST_F(ConnectorSystemTestFixture, init_connector)
{
	DDMP2_FRAME myFrame;
	size_t frame_size = 0;
	printf("Pausing scheduler\n");
	vPortPauseScheduler();
	//vTaskDelay(1);
	vPortExecuteTick();
	int res = getNumSentDDMP2Frames();
	EXPECT_EQ(res, 1) << "We should have sent more than 0 frames during startup: " << res << std::endl;
	res = getNextSentDDMP2Frame(&myFrame, &frame_size);
	EXPECT_FALSE(res) << "We should have sent a PUB for svc0avl" << std::endl;
	EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_PUBLISH) << "We should have sent a PUB for svc0avl" << std::endl;
	EXPECT_TRUE(myFrame.frame.publish.parameter == SVC0AVL) << "We should have sent a PUB for svc0avl" << std::endl;
	EXPECT_TRUE(myFrame.frame.publish.value.int32 == 1) << "We should have sent a PUB for svc0avl" << std::endl;

	// No more sent messages

	//vPortResumeScheduler();
	//vTaskDelay(pdMS_TO_TICKS(1000)-1);
	//vPortPauseScheduler();
	TickType_t loop_ticks = 0;
	while (loop_ticks++ < pdMS_TO_TICKS(1000)-1)
	{
		vPortExecuteTick();
	}
	res = getNumReceivedDDMP2Frames();
	EXPECT_EQ(res, 1) << "We should have received 1 frames 1000ms: " << res << std::endl;

	res = getNextReceivedDDMP2Frame(&myFrame, &frame_size);
	EXPECT_EQ(res, 0) << "We should have received a frame: " << res << std::endl;
	EXPECT_TRUE(myFrame.frame.control == DDMP2_CONTROL_GENERIC) << "We should have received a DDMP2_CONTROL_GENERIC" << std::endl;
	EXPECT_EQ(myFrame.frame.generic.id, (uint32_t)1000) << "We should have received DDMP2_CONTROL_GENERIC:id 1000" << std::endl;
	res = getNumReceivedDDMP2Frames();
	EXPECT_EQ(res, 0) << "We should have received no more frames: " << res << std::endl;

	res = getNextSentDDMP2Frame(&myFrame, &frame_size);
	EXPECT_TRUE(res < 0) << "We should not have sent more frames" << std::endl;

	vPortResumeScheduler();
}
