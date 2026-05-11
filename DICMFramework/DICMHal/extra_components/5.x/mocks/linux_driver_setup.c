/*
 * linux_driver_setup.c
 *
 *  Created on: 2 okt. 2023
 *      Author: Andlun
 */


#include "driver/gpio.h"
#include "Mockgpio.h"
#include "linux_driver_setup.h"

void linux_driver_setup(void)
{
    Mockgpio_Init();
    gpio_install_isr_service_IgnoreAndReturn(0);
    gpio_isr_register_IgnoreAndReturn(0);
    gpio_config_IgnoreAndReturn(0);
    gpio_set_level_IgnoreAndReturn(0);
    gpio_isr_handler_add_IgnoreAndReturn(0);
}
