/*!	\file hal_rtc.c
	\brief RTC Hardware Abstraction
	\author	Sebastien Fortin
	\author	Jens Björnhager
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dicm_framework_config.h"
#include "hal_rtc.h"
#ifdef RTC_PCF85263A
#include "drv_pcf85263a.h"
#endif
#ifdef RTC_PCF85063A
#include "drv_pcf85063a.h"
#endif
#ifdef RTC_PCF8563
#include "drv_pcf8563.h"
#endif

/*! \brief	Initialize RTC devices
	\retval	HAL_E_OK
 */
hal_err_t hal_rtc_init(void)
{
	return HAL_E_OK;
}

/*! \brief	Check whether time in the RTC is set
	\return	Error code (success = HAL_E_OK)
 */
hal_err_t hal_rtc_status(void)
{
	// Read current date-time
	HAL_RTC_DATE_TIME datetime;
	hal_err_t result = hal_rtc_read(&datetime);

	if (result == HAL_E_OK)
	{
		// Is set?
		if (datetime.year < 2022)
		{
			result = HAL_E_NOT_SET;
		}
	}
	return result;
}

/*! \brief	Read from RTC
	\param[out]	datetime Pointer to date_time structure that will receive the current time
	\return	Error code (success = HAL_E_OK)
 */
hal_err_t hal_rtc_read(HAL_RTC_DATE_TIME * const datetime)
{
	memset(datetime, 0, sizeof(HAL_RTC_DATE_TIME));

	hal_err_t result = HAL_E_DEVICE;

	#ifdef RTC_PCF85263A
		result = pcf85263a_read(RTC_PCF85263A_I2C_PORT, datetime);
	#endif

	#ifdef RTC_PCF85063A
		result = pcf85063a_read(RTC_PCF85063A_I2C_PORT, datetime);
	#endif

	#ifdef RTC_PCF8563
		result = pcf8563_read(RTC_PCF8563_I2C_PORT, datetime);
	#endif	

	return result;
}

/*! \brief	Write to RTC
	\param[in]	datetime Pointer to date_time structure that contains the time to set
	\return	Error code (success = HAL_E_OK)
 */
hal_err_t hal_rtc_write(HAL_RTC_DATE_TIME * const datetime)
{
	// Find device
	hal_err_t result = HAL_E_DEVICE;
	
	#ifdef RTC_PCF85263A
		result = pcf85263a_write(RTC_PCF85263A_I2C_PORT, datetime);
	#endif

	#ifdef RTC_PCF85063A
		result = pcf85063a_write(RTC_PCF85063A_I2C_PORT, datetime);
	#endif

	#ifdef RTC_PCF8563
		result = pcf8563_init(RTC_PCF8563_I2C_PORT);	
		result = pcf8563_write(RTC_PCF8563_I2C_PORT, datetime);
	#endif	

	return result;
}

/*!	\brief	Set RTC date
	\param[in]	year	Year to set 2020-2099
	\param[in]	month	Month to set (1-12)
	\param[in]	day		Day of the month (1-31)
	\param[in]	weekday	Day of the week (1-7). Day 1 is Sunday.
	\return	Error code (success = E_OK)
 */
hal_err_t hal_rtc_set_date(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t weekday)
{
	// Read current datetime
	HAL_RTC_DATE_TIME datetime;
	hal_err_t result = hal_rtc_read(&datetime);
	if (result == HAL_E_OK)
	{
		// Modify date
		datetime.year		= year;
		datetime.month		= month;
		datetime.day		= day;
		datetime.weekday	= weekday;

		// Write date/time
		result = hal_rtc_write(&datetime);
	}
	return result;
}

/*!	\brief	Set RTC time
 	\param[in]	hour	hours to set (24h format, 0-23)
	\param[in]	minute	minutes to set (0-59)
	\param[in]	second	seconds to set (0-59)
	\return	Error code (success = E_OK)
 */
hal_err_t hal_rtc_set_time(const uint8_t hour, const uint8_t minute, const uint8_t second)
{
	// Read current date-time
	HAL_RTC_DATE_TIME date_time;
	hal_err_t result = hal_rtc_read(&date_time);
	if (result == HAL_E_OK)
	{
		// Modify time
		date_time.hour   = hour;
		date_time.minute = minute;
		date_time.second = second;
		date_time.millisecond  = 0;

		// Write date/time
		result = hal_rtc_write(&date_time);
	}
	return result;
}

/*!	\brief	Set RTC date and time from struct tm
 	\param[in]	sys_time	Pointer to system time and date in struct tm format
	\return	Error code (success = E_OK)
 */
hal_err_t hal_rtc_set_datetime(const struct tm * const sys_time)
{
	hal_err_t date_err = hal_rtc_set_date(sys_time->tm_year + 1900, sys_time->tm_mon + 1, sys_time->tm_mday, sys_time->tm_wday + 1);
	if (date_err == HAL_E_OK)
	{
		return hal_rtc_set_time(sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec);
	}

	return date_err;
};

/*!	\brief	Print formatted integer and return size
	\param[in]	value  Integer value
	\param[out]	output Pointer to output text buffer
	\param[in]	digits Minimum text length (will do left padding using '0')
	\return text length
 */
static int hal_rtc_itoa(const int value, char * const output, const int digits)
{
	// Print locally
	char buffer[12];
	int  length = 0;
	//itoa(value, buffer, 10);  //non-standard function
	sprintf(buffer, "%d", value);
	while ((buffer[length] != 0) && (length < (int)sizeof(buffer)))
    {
		length++;
    }
	// Padding (if required)
	int index = 0;
	for (int i=length; i<digits; i++)
    {
		output[index++] = '0';
	}
	// Copy
	for (int i=0; i<length; i++)
    {
		output[index++] = buffer[i];
    }
	return index;
}

/*!	\brief	Format date to string
	\param[in]	datetime Pointer to date/time information
	\param[out]	text     Pointer to output text buffer
 */
void  hal_rtc_format_date(const HAL_RTC_DATE_TIME * const datetime, char * const text)
{
	int index = 0;
	index += hal_rtc_itoa(datetime->year,  &text[index], 4);
	text[index++] = '-';
	index += hal_rtc_itoa(datetime->month, &text[index], 2);
	text[index++] = '-';
	index += hal_rtc_itoa(datetime->day,   &text[index], 2);
	text[index] = 0;
}

/*!	\brief  Format time to string
	\param[in]	datetime Pointer to date/time information
	\param[out]	text     Pointer to output text buffer
 */
void hal_rtc_format_time(const HAL_RTC_DATE_TIME * const datetime, char * const text)
{
	int index = 0;
	index += hal_rtc_itoa(datetime->hour,   &text[index], 2);
	text[index++] = ':';
	index += hal_rtc_itoa(datetime->minute, &text[index], 2);
	text[index++] = ':';
	index += hal_rtc_itoa(datetime->second, &text[index], 2);
	text[index] = 0;
}
