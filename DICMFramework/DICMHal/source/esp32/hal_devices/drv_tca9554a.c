/*! \file drv_tca9554a.c
	\brief TI TCA9554A IO driver

	Remote 8-bit I/O expander for I2C-bus with interrupt
*/

#include "dicm_framework_config.h"

#ifdef DEVICE_TCA9554A
#include "iGeneralDefinitions.h"
#include "drv_tca9554a.h"
#include "freertos/FreeRTOS.h"
#include "hal_i2c_master.h"

//#define TCA9554A_DEBUG_LOG

#define TCA_9554A_INPUT_READ_CMD   (0x00)
#define TCA_9554A_OUTPUT_CMD       (0x01)
#define TCA_9554A_POLARITY_INV_CMD (0x02)
#define TCA_9554A_CONFIG_CMD       (0x03) 

uint8_t configured_pins = 0xFF;
uint8_t set_levels = 0xFF;

//! \~ Configure pin, all pins are inputs at power on
int tca9554a_configure_pin(const int i2c_port, const uint8_t i2c_address, const uint8_t pin, HAL_GPIO_PINMODE_ENUM read_write)
{
	uint8_t pin_mask;
	portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
	uint8_t data[2];

#ifdef TCA9554A_DEBUG_LOG
    LOG(I, "Config: pin %d, configured_pins = 0x%x", pin, configured_pins);
#endif

    portENTER_CRITICAL(&myMutex);
	{
		if ( read_write == HAL_GPIO_PINMODE_WRITE )
		{ 
            pin_mask = ~(1 << pin);
			// Clear to mark the pin as output
			configured_pins &= pin_mask;
		}
		else if ( read_write == HAL_GPIO_PINMODE_READ )
		{
            pin_mask = (1 << pin);
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

	data[0] = TCA_9554A_CONFIG_CMD;
	data[1] = configured_pins;

#ifdef TCA9554A_DEBUG_LOG
	LOG(I, "Config: Wrote data[0]=0x%x, data[1]=0x%x", data[0], data[1]);
#endif

	return hal_i2c_master_write(i2c_port, i2c_address, data, sizeof(data));
}

//! \~ Set one of the levels
int tca9554a_setlevel(const int i2c_port, const uint8_t i2c_address, const uint8_t pin, const int level)
{
	uint8_t pin_mask;
	portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
	uint8_t data[2];

	portENTER_CRITICAL(&myMutex);
	{
		if (!level)
		{
			// Clear
            pin_mask = ~(1 << pin);
			set_levels &= pin_mask;
		}
		else
		{
			// Set
            pin_mask = (1 << pin);
			set_levels |= pin_mask;
		}
	}
	portEXIT_CRITICAL(&myMutex);

	data[0] = TCA_9554A_OUTPUT_CMD;
	data[1] = set_levels;

	//LOG(I, "Set: Wrote data[0]=0x%x, data[1]=0x%x", data[0], data[1]);

	int result = hal_i2c_master_acquire(i2c_port);
	if (result == HAL_E_OK)
	{
		result = hal_i2c_master_write(i2c_port, i2c_address, data, sizeof(data));

		hal_i2c_master_release(i2c_port);
	}
	return result;
}

//! \~ Get one of the levels
int tca9554a_getlevel(const int i2c_port, const uint8_t i2c_address, const uint8_t pin, uint8_t *level)
{
	uint8_t data;
	uint8_t cmd;
	esp_err_t ret;

	cmd = TCA_9554A_INPUT_READ_CMD;

	ret = hal_i2c_master_acquire(i2c_port);
	if (ret == HAL_E_OK)
	{
		ret = hal_i2c_master_writeread(i2c_port, i2c_address, &cmd, 1, &data, 1); 

		hal_i2c_master_release(i2c_port);
	}

#ifdef TCA9554A_DEBUG_LOG
    LOG(I, "data=0x%x", data);
#endif
	data   = !!(data & (1 << pin));
	*level = data;

#ifdef TCA9554A_DEBUG_LOG
    LOG(I, "Get: From cmd=0x%x, data=0x%x level=0x%x", cmd, data, *level);
#endif
    return ret;
}

#endif // DEVICE_TCA9554A

