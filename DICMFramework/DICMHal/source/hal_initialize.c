#include "hal_initialize.h"
#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#ifdef CONFIG_IDF_TARGET_LINUX
#include "linux_driver_setup.h"
#include "linux_esp_timer_setup.h"
#endif
#ifdef HAL_GPIO
#include "hal_gpio.h"
#endif
#include "hal_i2c_master.h"
#include "hal_ledc.h"
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
#include "hal_ethnet.h"
#endif

void hal_initialize(void)
{
#ifdef CONFIG_IDF_TARGET_LINUX
    linux_driver_setup();
    linux_esp_timer_setup();
#endif
#if defined(HAL_I2C_MASTER)
    LOG(I, "Initializing I2C");
#ifdef I2C_MASTER0
    hal_i2c_master_init(I2C_NUM_0, I2C_MASTER0_SDA, I2C_MASTER0_SCL, I2C_MASTER0_FREQ);
#endif
#ifdef I2C_MASTER1
    hal_i2c_master_init(I2C_NUM_1, I2C_MASTER1_SDA, I2C_MASTER1_SCL, I2C_MASTER1_FREQ);
#endif
#endif

#ifdef SPI_MASTER2
    LOG(I, "Initializing SPI2");
    hal_spi_master_init(HAL_SPI_HOST2, SPI_MASTER2_MISO, SPI_MASTER2_MOSI, SPI_MASTER2_CLK, 0);
#endif
#ifdef SPI_MASTER3
    LOG(I, "Initializing SPI3");
    hal_spi_master_init(HAL_SPI_HOST3, SPI_MASTER3_MISO, SPI_MASTER3_MOSI, SPI_MASTER3_CLK, SPI_MASTER3_FREQ);
#endif

#ifdef HAL_GPIO
    LOG(I, "Initializing GPIO HAL");
    hal_gpio_init();
#endif

#ifdef HAL_LEDC_PWM
    LOG(I, "Initializing LEDC PWM HAL");
    hal_ledc_init();
#endif

    // Fixme: why it does not work before board_initialization!!!!
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
    LOG(I, "Initializing ethernet");
    HAL_ETHNET_Initialize(NULL);
#endif

}

