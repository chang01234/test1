/*! \file drv_tca6424a.h
	\brief TI TCA6424A IO driver

    Remote 24-bit I/O expander for I2C-bus with interrupt
*/

#ifndef DRV_TCA6424A_H_
#define DRV_TCA6424A_H_

#include <stdint.h>
#include "hal_gpio.h"

//! \~ Configure pin, all pins are inputs at power on
int tca6424_configure_pin(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, HAL_GPIO_PINMODE_ENUM read_write);

//! \~ Response to an interrupt; read levels and compare to previous levels to determine changed pin(s)
int tca6424_changedpins(const int i2c_port, const uint8_t i2c_address, uint32_t *current_levels, uint32_t *changed_pins);

//! \~ Set one of the levels
int tca6424_setlevel(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, const int level);

//! \~ Get one of the levels; get all levels and mask out bit
int tca6424_getlevel(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, int *level);

//! \~ Set all 16 GPIO levels in one go
int tca6424_setlevels(const int i2c_port, const uint8_t i2c_address, const uint16_t levels);

//! \~ Get all 16 levels in one go
int tca6424_getlevels(const int i2c_port, const uint8_t i2c_address, uint32_t *levels);

#endif /* DRV_TCA6424A_H_ */