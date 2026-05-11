/*! \file drv_pca9675.c
	\brief NXP PCA9675 IO driver

	Remote 16-bit I/O expander for Fm+ I2C-bus with interrupt
*/

#include "drv_tca6424a.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "freertos/FreeRTOS.h"
#pragma GCC diagnostic pop

#include "dicm_framework_config.h"
#include "hal_i2c_master.h"

#define TCA_6424A_PORT0_READ_CMD (0x00)
#define TCA_6424A_PORT0_OUTPUT_CMD (0x04)
#define TCA_6424A_PORT0_CONFIG_CMD (0x0c)

//! \~ Configure pin, all pins are inputs at power on
int tca6424_configure_pin(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, HAL_GPIO_PINMODE_ENUM read_write)
{
	static uint32_t configured_pins = 0x00ffffff;
	const uint32_t pin_in_port = ((1 << pin) << (port * 8));
	const uint32_t pin_mask=~(pin_in_port);
	portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
	uint8_t data[2];
	
	portENTER_CRITICAL(&myMutex);
	{
		if (read_write == HAL_GPIO_PINMODE_WRITE)
		{
			// Clear to mark the pin as output
			configured_pins &= pin_mask;
		}
		else if (read_write == HAL_GPIO_PINMODE_READ)
		{
			// Set to mark it as input
			configured_pins |= pin_mask;
		}
		else
		{
			portEXIT_CRITICAL(&myMutex);
			return ESP_FAIL;
		}
	}
	portEXIT_CRITICAL(&myMutex);

	data[0] = TCA_6424A_PORT0_CONFIG_CMD + (port * 1);
	data[1] = (uint8_t)((configured_pins & (0xff << (port * 8))) >> (port * 8));

	//LOG(I, "Config: Wrote data[0]=0x%x, data[1]=0x%x", data[0], data[1]);

	return hal_i2c_master_write(i2c_port, i2c_address, data, sizeof(data));
}

//! \~ Response to an interrupt; read levels and compare to previous levels to determine changed pin(s)
int tca6424_changedpins(const int i2c_port, const uint8_t i2c_address, uint32_t *current_levels, uint32_t *changed_pins)
{
	uint32_t new_levels;

    // Read all pins
    hal_err_t result = tca6424_getlevels(i2c_port, i2c_address, &new_levels);

    if (result == HAL_E_OK)
    {
        // Compare them
        *changed_pins   = new_levels ^ *current_levels;
        *current_levels = new_levels;
    }
    return result;
}

//! \~ Set one of the levels
int tca6424_setlevel(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, const int level)
{
	static uint32_t set_levels = 0x00ffffff;
	const uint32_t pin_in_port = ((1 << pin) << (port * 8));
	const uint32_t pin_mask    = ~(pin_in_port);
 
	portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
	uint8_t data[2];

	portENTER_CRITICAL(&myMutex);
	{
		if (!level)
		{
			// Clear
			set_levels &= pin_mask;
		}
		else
		{
			// Set 
			set_levels |= pin_in_port;
		}
	}
	portEXIT_CRITICAL(&myMutex);

	data[0] = TCA_6424A_PORT0_OUTPUT_CMD + (port * 1);
	data[1] = (uint8_t)((set_levels & (0xff << (port * 8))) >> (port * 8));

	//LOG(I, "Set: Wrote data[0]=0x%x, data[1]=0x%x", data[0], data[1]);

	return hal_i2c_master_write(i2c_port, i2c_address, data, sizeof(data));
}

//! \~ Get one of the levels
int tca6424_getlevel(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, int *level)
{
	uint8_t data;
	uint8_t cmd;
	esp_err_t ret;

	cmd = TCA_6424A_PORT0_READ_CMD + (port * 1);

	ret = hal_i2c_master_write(i2c_port, i2c_address, &cmd, sizeof(cmd));
	ret |= hal_i2c_master_read(i2c_port, i2c_address, &data, sizeof(data));

	//LOG(I, "Get: From cmd=0x%x, data=0x%x", cmd, data);

	data = !!(data & (1 << pin));
	*level = (int)data;

	return ret;
}

//! \~ Set all 16 GPIO levels in one go
int tca6424_setlevels(const int i2c_port, const uint8_t i2c_address, const uint16_t levels)
{
	return 0;
}

//! \~ Get all 16 levels in one go
int tca6424_getlevels(const int i2c_port, const uint8_t i2c_address, uint32_t *levels)
{
	uint8_t cmd = 0x80;
	esp_err_t ret;
	ret = hal_i2c_master_writeread(i2c_port, i2c_address, &cmd, 1, (uint8_t*)levels, 3);
	//printf("IO EX input pin value : 0x%x\n", *levels);

	return ret;
}
