/*****************************************************************************
 * \file       hal_i2c_master.c
 * \brief      I2C Master Hardware Abstraction Layer ESP32 implementation
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"
#include "hal_i2c_master.h"
#include "freertos/FreeRTOS.h"

#include "driver/i2c.h"

/*****************************************************************************
 * Private Defines
 *****************************************************************************/
#define I2C_MASTER_PORT_COUNT  2

/*****************************************************************************
 * Private Variables
 *****************************************************************************/
static struct
{
	volatile bool initialized;
	StaticSemaphore_t instance;
	SemaphoreHandle_t handle;
} hal_i2c_mutex[I2C_MASTER_PORT_COUNT] = {0};

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

//! \~ Initialize I2C device
int hal_i2c_master_init(const int i2c_master_port, const int sda, const int scl, uint32_t freq)
{
	i2c_config_t conf=
	{
		.mode = I2C_MODE_MASTER,
		.sda_io_num = sda,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = scl,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = freq,
	};

	ZERO_CHECK(i2c_param_config(i2c_master_port, &conf));
	ZERO_CHECK(i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0));

	return 0;
}

//! \~ Require exclusive access to specified port
int hal_i2c_master_acquire(const int i2c_master_port)
{
	// Check port
	if (i2c_master_port < I2C_MASTER_PORT_COUNT)
	{
		// Create mutex?
		if (hal_i2c_mutex[i2c_master_port].initialized != true)
		{
			hal_i2c_mutex[i2c_master_port].handle      = xSemaphoreCreateRecursiveMutexStatic(&hal_i2c_mutex[i2c_master_port].instance);
			hal_i2c_mutex[i2c_master_port].initialized = true;
		}

		// Acquire
		xSemaphoreTakeRecursive(hal_i2c_mutex[i2c_master_port].handle, portMAX_DELAY);
		return HAL_E_OK;
	}
	return HAL_E_DEVICE;
}

//! \~ Release exclusive access to specified port
void hal_i2c_master_release(const int i2c_master_port)
{
	// Check port
	if (i2c_master_port < I2C_MASTER_PORT_COUNT)
	{
		// Release
		xSemaphoreGiveRecursive(hal_i2c_mutex[i2c_master_port].handle);
	}
}

//! \~ Write data to I2C device
int hal_i2c_master_write(const int i2c_num, const uint16_t device_address, const uint8_t *data_wr, const size_t size)
{
	esp_err_t result;
	i2c_cmd_handle_t cmd;

	TRUE_CHECK(cmd = i2c_cmd_link_create());

	ZERO_CHECK(i2c_master_start(cmd));
	ZERO_CHECK(i2c_master_write_byte(cmd, device_address<<1, 1));
	ZERO_CHECK(i2c_master_write(cmd, (uint8_t*)data_wr, size, 1));
	ZERO_CHECK(i2c_master_stop(cmd));

	result = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));

	i2c_cmd_link_delete(cmd);

    if ( result != ESP_OK )
    {
        LOG(E, "I2C write failed result = %d", result);
    }

	return result;
}

//! \~ Read data from I2C device
int hal_i2c_master_read(const int i2c_num, const uint16_t device_address, uint8_t *const data_rd, const size_t size)
{
	esp_err_t result;
	i2c_cmd_handle_t cmd;

	TRUE_CHECK(cmd = i2c_cmd_link_create());

	ZERO_CHECK(i2c_master_start(cmd));
	ZERO_CHECK(i2c_master_write_byte(cmd, (device_address<<1) + 1, 0));
	if (size > 1)
	{
		ZERO_CHECK(i2c_master_read(cmd, data_rd, size-1, 0));
	}
	ZERO_CHECK(i2c_master_read_byte(cmd, data_rd + size-1, 1));
	ZERO_CHECK(i2c_master_stop(cmd));

	result=i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));

	i2c_cmd_link_delete(cmd);

    if ( result != ESP_OK )
    {
        LOG(E, " I2C read failed result = %d ", result);
    }

	return result;
}


/*! \brief Write and read data from I2C device in one transaction using restart bit
  \param i2c_master_port I2C port number
  \param device_address Slave address to communicate with
  \param data_wr Data to send to slave
  \param size_wr Size of data to send to slave
  \param data_rd Data read from slave
  \param size_rd Size of data to read from slave
  \return FALSE if successful
  */
int hal_i2c_master_writeread(const int i2c_master_port, const uint16_t device_address, const uint8_t *data_wr, const size_t size_wr, uint8_t *const data_rd, const size_t size_rd)
{
	esp_err_t result;
	i2c_cmd_handle_t cmd;

	TRUE_CHECK(cmd = i2c_cmd_link_create());

	ZERO_CHECK(i2c_master_start(cmd));
	ZERO_CHECK(i2c_master_write_byte(cmd, device_address << 1 | 0 /*Write bit*/, 1 ));
    ZERO_CHECK(i2c_master_write(cmd, data_wr, size_wr, 1));

	ZERO_CHECK(i2c_master_start(cmd));
	ZERO_CHECK(i2c_master_write_byte(cmd, device_address << 1 | 1/*READ_BIT*/, 1 ));
	if (size_rd > 1)
	{
		ZERO_CHECK(i2c_master_read(cmd, data_rd, size_rd - 1, 0));
	}
	ZERO_CHECK(i2c_master_read_byte(cmd, data_rd + size_rd - 1, 1/*NACK_VAL*/));

	ZERO_CHECK(i2c_master_stop(cmd));

	result=i2c_master_cmd_begin(i2c_master_port, cmd, pdMS_TO_TICKS(1000));

	i2c_cmd_link_delete(cmd);

    if ( result != ESP_OK )
    {
        LOG(E, " I2C wr/read failed result = %d ", result);
    }

	return result;
}


