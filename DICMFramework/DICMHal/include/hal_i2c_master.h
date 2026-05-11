/*****************************************************************************
* \file        hal_i2c_master.h
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
#ifndef HAL_I2C_MASTER_H_
#define HAL_I2C_MASTER_H_

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "hal_types.h"

/*****************************************************************************
 * Public defines
 *****************************************************************************/

/*****************************************************************************
 * Public function
 *****************************************************************************/

/*! \brief Initialize I2C master
	\param i2c_master_port I2C port number
	\param sda_pin SDA pin on host
	\param scl_pin SCL pin on host
	\param freq Bitrate to use
	\return FALSE if successful
 */
int hal_i2c_master_init(const int i2c_master_port, const int sda_pin, const int scl_pin, uint32_t freq);
/*! \brief Require exclusive access to specified port
 */
int hal_i2c_master_acquire(const int i2c_master_port);

/*! \brief Release exclusive access to specified port
 */
void hal_i2c_master_release(const int i2c_master_port);

/*! \brief Write data to I2C device
	\param i2c_master_port I2C port number
	\param device_address Slave address to communicate with
	\param data_wr Data to send to slave
	\param size Size of data to send to slave
	\return FALSE if successful
 */
int hal_i2c_master_write(const int i2c_master_port, const uint16_t device_address, const uint8_t *data_wr, const size_t size);

/*! \brief Read data from I2C device
	\param i2c_master_port I2C port number
	\param device_address Slave address to communicate with
	\param data_rd Data read from slave
	\param size Size of data to read from slave
	\return FALSE if successful
 */
int hal_i2c_master_read(const int i2c_master_port, const uint16_t device_address, uint8_t *const data_rd, const size_t size);

/*! \brief Write and read data from I2C device in one transaction using restart bit
	\param i2c_master_port I2C port number
	\param device_address Slave address to communicate with
	\param data_wr Data to send to slave
	\param size_wr Size of data to send to slave
	\param data_rd Data read from slave
	\param size_rd Size of data to read from slave
	\return FALSE if successful
 */
int hal_i2c_master_writeread(const int i2c_master_port, const uint16_t device_address, const uint8_t *data_wr, const size_t size_wr, uint8_t *const data_rd, const size_t size_rd);


/*****************************************************************************/
#endif /* HAL_I2C_MASTER_H_ */