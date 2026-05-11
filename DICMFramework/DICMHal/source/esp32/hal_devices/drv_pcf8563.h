#ifndef DRV_PCF8563_H_
#define DRV_PCF8563_H_

#include "hal_rtc.h"

hal_err_t pcf8563_read(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime);
hal_err_t pcf8563_write(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime);
hal_err_t pcf8563_init(const uint8_t i2c_port);
#endif /* DRV_PCF8563_H_ */
