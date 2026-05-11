/*! \file drv_tca9554a.h
	\brief TI TCA9554A IO driver

	Remote 8-bit I/O expander for I2C-bus with interrupt
*/

#ifndef DRV_TCA9554A_H_
#define DRV_TCA9554A_H_

#include <stdint.h>
#include "hal_gpio.h"

//! \~ Configure pin, all pins are inputs at power on
int tca9554a_configure_pin(const int i2c_port, const uint8_t i2c_address, const uint8_t pin, HAL_GPIO_PINMODE_ENUM read_write);

//! \~ Set one of the levels
int tca9554a_setlevel(const int i2c_port, const uint8_t i2c_address, const uint8_t pin, const int level);

//! \~ Get one of the levels; get all levels and mask out bit
int tca9554a_getlevel(const int i2c_port, const uint8_t i2c_address, const uint8_t pin, uint8_t *level);

#endif /* DRV_TCA9554A_H_ */