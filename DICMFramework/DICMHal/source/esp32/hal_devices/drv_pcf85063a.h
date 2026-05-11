/******************************************************************************
 * \file       drv_pcf85063a
 * \brief      Driver for I2C RTC device PCF85063A
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
#ifndef DRV_PCF85063A_H_
#define DRV_PCF85063A_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "hal_rtc.h"

/******************************************************************************
 * Public Functions
 *****************************************************************************/
hal_err_t pcf85063a_read(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime);
hal_err_t pcf85063a_write(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime);

/*****************************************************************************/
#endif // DRV_PCF85063_H_