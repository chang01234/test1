#ifndef HAL_I2CDEV_H_
#define HAL_I2CDEV_H_

#include "driver/i2c.h"

#define I2C_FREQ_HZ 400000
#define I2CDEV_TIMEOUT 1000

typedef struct {
    i2c_port_t port;            // I2C port number
    uint8_t addr;               // I2C address
    gpio_num_t sda_io_num;      // GPIO number for I2C sda signal
    gpio_num_t scl_io_num;      // GPIO number for I2C scl signal
        uint32_t clk_speed;             // I2C clock frequency for master mode
} i2c_dev_t;

// dev->port = I2C_MASTER0_PORT;
// dev->addr = PCF8563_ADDR;
// dev->sda_io_num = sda;
// dev->scl_io_num = scl;
// dev->clk_speed = freq;
esp_err_t i2c_master_init(i2c_port_t port, int sda, int scl);
esp_err_t i2c_dev_read(const i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size);
esp_err_t i2c_dev_write(const i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size);
static inline esp_err_t i2c_dev_read_reg(const i2c_dev_t *dev, uint8_t reg,
        void *in_data, size_t in_size)
{
    return i2c_dev_read(dev, &reg, 1, in_data, in_size);
}

static inline esp_err_t i2c_dev_write_reg(const i2c_dev_t *dev, uint8_t reg,
        const void *out_data, size_t out_size)
{
    return i2c_dev_write(dev, &reg, 1, out_data, out_size);
}
#endif /* HAL_I2CDEV_H_ */
