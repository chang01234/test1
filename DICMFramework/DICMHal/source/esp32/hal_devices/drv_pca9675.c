/*****************************************************************************
 * \file       drv_pca9675
 * \brief      NXP PCA9675 IO driver
 *             Remote 16-bit I/O expander for Fm+ I2C-bus with interrupt
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
#include "drv_pca9675.h"

#include "hal_i2c_master.h"

/*****************************************************************************
 * Local defines
 *****************************************************************************/

/*****************************************************************************
 * Local variables
 *****************************************************************************/
static uint16_t pca9675_levels_out[2] = {0xFFFF, 0xFFFF}; // fixme: default outputs always 1?

/*****************************************************************************
 * \brief Set all 16 GPIO levels in one go
 *****************************************************************************/
int pca9675_setlevels(const int i2c_port, const uint8_t i2c_address, const uint16_t levels)
{
    return hal_i2c_master_write(i2c_port, i2c_address, (uint8_t*)&levels, sizeof(levels));
}

/*****************************************************************************
 * \brief Get all 16 levels in one go
 *****************************************************************************/
int pca9675_getlevels(const int i2c_port, const uint8_t i2c_address, uint16_t *levels)
{
    return hal_i2c_master_read(i2c_port, i2c_address, (uint8_t*)levels, sizeof(*levels));
}

/*****************************************************************************
 * \brief Response to an interrupt; read levels and compare to previous levels 
 *        to determine changed pin(s)
 *****************************************************************************/
int pca9675_changedpins(const int i2c_port, const uint8_t i2c_address, uint16_t *current_levels, uint16_t *changed_pins)
{
    uint16_t new_levels;

    // Read all pins
    hal_err_t result = pca9675_getlevels(i2c_port, i2c_address, &new_levels);
    if (result == HAL_E_OK)
    {
        // Compare them
        *changed_pins   = new_levels ^ *current_levels;
        *current_levels = new_levels;
    }
    return result;
}

/*****************************************************************************
 * \brief Set one of the levels
 *****************************************************************************/
int pca9675_setlevel(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, const int level)
{    
    const int      dev_idx  = (port>>1);
    const int      dev_port = (port&1);        
    const int      pin_bit  = (dev_port<<3) + pin;
    const uint16_t pin_mask = ~(1<<pin_bit);
    const uint16_t level_mask = level? (1<<pin_bit) : 0;
    volatile uint16_t new_levels;
    
    // Get i2c access
    hal_err_t result = hal_i2c_master_acquire(i2c_port);
    if (result == HAL_E_OK)
    {
        // Set levels (protected by mutex)
        pca9675_levels_out[dev_idx] &= pin_mask;   // Clear bit
        pca9675_levels_out[dev_idx] |= level_mask; // Insert new pin level
        new_levels = pca9675_levels_out[dev_idx];  // Make copy

        // Write
        result = hal_i2c_master_write(i2c_port, i2c_address, (uint8_t*)&new_levels, sizeof(new_levels));

        // Release
        hal_i2c_master_release(i2c_port);
    }
    return result;    
}

/*****************************************************************************
 * \brief Get one of the levels; get all levels and mask out bit
 * ***************************************************************************/
int pca9675_getlevel(const int i2c_port, const uint8_t i2c_address, const int port, const int pin, int *level)
{
	// Find bit + mask
    const int      dev_port = (port&1); 
    const int      pin_bit  = (dev_port<<3) + pin;
    const uint16_t pin_mask = 1<<pin_bit;
	
	// Read from device
    uint16_t pins_levels;
	int      result;
    result = pca9675_getlevels(i2c_port, i2c_address, &pins_levels);
    if (result == HAL_E_OK)
	{
        *level = (pins_levels & pin_mask)?1:0;
	}
    return result;
}
