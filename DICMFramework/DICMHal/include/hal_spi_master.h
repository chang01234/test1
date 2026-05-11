/*! \file hal_spi_master.h
	\brief SPI Master Hardware Abstraction Layer
*/

#ifndef HAL_SPI_MASTER_H_
#define HAL_SPI_MASTER_H_
/*****************************************************************************/
#include "driver/spi_master.h" // Fixme: need abstraction
#include "hal_types.h"

#define HAL_SPI_HOST1     0
#define HAL_SPI_HOST2     1
#define HAL_SPI_HOST3     2

#define IO_NUM_CS_FLASH     27
#define IO_NUM_CS_WIZNET    5

/*! \brief Init a given SPI device (SPI_HOST, HSPI_HOST or VSPI_HOST)
	\param spi_master_port SPI master port
	\param miso pin number of MISO
	\param mosi pin number of MOSI
	\param clk  pin number of SPI Clock
	\param freq default frequency (IMPORTANT: use 0 in order to use device_config)
	\return FALSE if successful
 */
int hal_spi_master_init(int master_port, int miso, int mosi, int clk, int freq);

/*! \brief Add a device on a SPI bus.
	\param spi_master_port SPI master port
	\param spi_device_handle SPI device handler
	\param freq the clock speed of SPI device 
	\param command_bits The command length in this transaction, in bits.
	\param address_bits The address length in this transaction, in bits.
	\return FALSE if successful
 */
int hal_spi_master_add_device(int master_port, spi_device_handle_t *device_handle, int cs, int freq, int command_bits, int address_bits);

/*! \brief Require exclusive access to specified port.
	\param spi_master_port SPI master port
 */
int hal_spi_master_acquire(int master_port);

/*! \brief Release exclusive access to specified port
	\param spi_master_port SPI master port
 */
void hal_spi_master_release(int master_port);

// Fixme: remove
extern spi_device_handle_t spi3;

#endif /* HAL_SPI_MASTER_H_ */