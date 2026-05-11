/*! \file hal_spi_master.c
	\brief SPI Master Hardware Abstraction Layer
*/

#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hal_spi_master.h"
#include "driver/spi_master.h"

spi_device_handle_t spi3;


/* ESP32 supports 3 SPI master: SPI_HOST, HSPI_HOST, VSPI_HOST */
#define SPI_MASTER_PORT_COUNT        3
static struct
{
    volatile bool initialized;
    StaticSemaphore_t instance;
    SemaphoreHandle_t handle;
} hal_spi_mutex[SPI_MASTER_PORT_COUNT] = {0};


//! \~ Configure master
int hal_spi_master_init(int master_port, int miso, int mosi, int clk, int freq)
{
    // Initialize bus
    const spi_bus_config_t spicfg=
    {
        .miso_io_num   = miso,
        .mosi_io_num   = mosi,
        .sclk_io_num   = clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .flags=SPICOMMON_BUSFLAG_NATIVE_PINS,
    };
    ZERO_CHECK(spi_bus_initialize(master_port, &spicfg, 0));

    // Initialize Flash device (legacy)
    if (freq != 0)
    {
        hal_spi_master_add_device(master_port, &spi3, IO_NUM_CS_FLASH, freq, 8, 24);
    }
    return HAL_E_OK;
}

//! \~ Configure device
int hal_spi_master_add_device(int master_port, spi_device_handle_t *device_handle, int cs, int freq, int command_bits, int address_bits)
{
    const spi_device_interface_config_t spidevcfg=
    {
        .clock_speed_hz = freq,
        .mode         =  3,
        .spics_io_num = cs,
        .queue_size   =  7,
        .pre_cb       = NULL,
        .command_bits = command_bits,
        .address_bits = address_bits,
        .flags=SPI_DEVICE_HALFDUPLEX,
    };

    // Check port
    ZERO_CHECK(spi_bus_add_device(master_port, &spidevcfg, device_handle));
    return HAL_E_OK;
}

//! \~ Require exclusive access to specified port
int hal_spi_master_acquire(int master_port)
{
    // Check port
    if (master_port < SPI_MASTER_PORT_COUNT)
    {
        // Create mutex?
        if (hal_spi_mutex[master_port].initialized != true)
        {
            hal_spi_mutex[master_port].handle      = xSemaphoreCreateRecursiveMutexStatic(&hal_spi_mutex[master_port].instance);
            hal_spi_mutex[master_port].initialized = true;
        }

        // Acquire
        xSemaphoreTakeRecursive(hal_spi_mutex[master_port].handle, portMAX_DELAY);
        return HAL_E_OK;
    }
    return HAL_E_DEVICE;
}

//! \~ Release exclusive access to specified port
void hal_spi_master_release(const int master_port)
{
    // Check port
    if (master_port < SPI_MASTER_PORT_COUNT)
    {
        // Release
        xSemaphoreGiveRecursive(hal_spi_mutex[master_port].handle);
    }
}
