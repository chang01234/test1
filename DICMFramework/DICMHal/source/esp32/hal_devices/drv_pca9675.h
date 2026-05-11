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
#ifndef DRV_PCA9675_H_
#define DRV_PCA9675_H_

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "hal_types.h"

/*****************************************************************************
 * Public defines
 *****************************************************************************/

/******************************************************************************
 * Public types
 *****************************************************************************/

/*****************************************************************************
 * Public functions
 *****************************************************************************/
int pca9675_changedpins(const int i2c_port,const uint8_t i2c_address,uint16_t *current_levels,uint16_t *changed_pins);
int pca9675_setlevel(const int i2c_port,const uint8_t i2c_address,const int port,const int pin,const int level);
int pca9675_getlevel(const int i2c_port,const uint8_t i2c_address,const int port,const int pin,int *level);
int pca9675_setlevels(const int i2c_port,const uint8_t i2c_address,const uint16_t levels);
int pca9675_getlevels(const int i2c_port,const uint8_t i2c_address,uint16_t *levels);

/*****************************************************************************/
#endif /* DRV_PCA9675_H_ */