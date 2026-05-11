/*
 * test_hal_mem.cpp
 *
 *  Created on: 22 sep. 2023
 *      Author: Andlun
 */


extern "C" {
#include "hal_mem.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}
#include "DICMFrameworkTestFixture.hpp"

class HalMemTestFixture : public DICMFrameworkTestFixture {
protected:

	void SetUp() override
    {
		DICMFrameworkTestFixture::SetUp();
    }

    void TearDown() override
    {
    	DICMFrameworkTestFixture::TearDown();
    }
};

TEST_F(HalMemTestFixture, hal_mem_malloc)
{
    void *ptr = NULL;
    ptr = hal_mem_malloc(10000, HAL_MEM_SPIRAM);
    EXPECT_TRUE(NULL != ptr) << "Should be possible to use SPIRAM on linux";
    free(ptr);
    ptr = hal_mem_malloc(10000, HAL_MEM_INTERNAL_RAM);
    EXPECT_TRUE(NULL != ptr) << "Should be possible to use SPIRAM on linux";
    free(ptr);
}

const char constteststring[] = "constteststring";
TEST_F(HalMemTestFixture, hal_mem_is_flash_ptr_true)
{
    EXPECT_TRUE(hal_mem_is_flash_ptr((void*)constteststring)) << "constteststring should be a flash pointer";
}

TEST_F(HalMemTestFixture, hal_mem_is_flash_ptr_false)
{
	char ramteststring[20];
    EXPECT_FALSE(hal_mem_is_flash_ptr((void*)ramteststring)) << "ramteststring should not be a flash pointer";
}

