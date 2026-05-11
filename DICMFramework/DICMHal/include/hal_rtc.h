/*!	\file hal_rtc.h
	\brief RTC Hardware Abstraction
	\author	Sebastien Fortin
	\author	Jens Björnhager
*/

#ifndef HAL_RTC_H_
#define HAL_RTC_H_

#include "hal_types.h"
#include <time.h>

//! \~ Custom time type with added millisecond field
typedef struct _HAL_RTC_DATE_TIME
{
	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  weekday;
	uint8_t  timezone;
	uint8_t  hour;
	uint8_t  minute;
	uint8_t  second;
	uint16_t millisecond;
} HAL_RTC_DATE_TIME;

hal_err_t hal_rtc_init(void);
hal_err_t hal_rtc_status(void);
hal_err_t hal_rtc_read(HAL_RTC_DATE_TIME * const date_time);
hal_err_t hal_rtc_write(HAL_RTC_DATE_TIME * const date_time);
hal_err_t hal_rtc_set_date(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t weekday);
hal_err_t hal_rtc_set_time(const uint8_t hour, const uint8_t minute, const uint8_t second);
hal_err_t hal_rtc_set_datetime(const struct tm * const sys_time);

void hal_rtc_format_date(const HAL_RTC_DATE_TIME * const date_time, char * const text);
void hal_rtc_format_time(const HAL_RTC_DATE_TIME * const date_time, char * const text);

#endif // HAL_RTC_H_
