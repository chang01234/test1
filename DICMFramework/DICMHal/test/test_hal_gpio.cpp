/*
 * test_hal_mem.cpp
 *
 *  Created on: 22 sep. 2023
 *      Author: Andlun
 */


extern "C" {
#include "hal_gpio.h"
#include "linux_driver_setup.h"
#include "Mockgpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
}
#include "DICMFrameworkTestFixture.hpp"

class HalGpioTestFixture : public DICMFrameworkTestFixture {
protected:
	void SetUp() override
    {
		DICMFrameworkTestFixture::SetUp();
    	linux_driver_setup();
    }

};

TEST_F(HalGpioTestFixture, hal_gpio_init)
{
	EXPECT_TRUE(HAL_GPIO_OK == hal_gpio_init()) << "hal_gpio_init() should return HAL_GPIO_OK";
	Mockgpio_Verify();
}
