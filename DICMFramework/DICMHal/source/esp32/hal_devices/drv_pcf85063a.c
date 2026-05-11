/*****************************************************************************
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

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "drv_pcf85063a.h"
#include "hal_i2c_master.h"

/*****************************************************************************
 * Local defines
 *****************************************************************************/
#define PCF85063A_ADDR					0x51   // r=0xa2 w=0xa3
#define PCF85063A_REGITER_DATE_TIME		0x04
#define PCF85063A_REGISTER_CTRL1		0x00
#define PCF85063A_BCD_ENCODE(x)			((((x)/10)*16) + ((x)%10))
#define PCF85063A_BCD_DECODE(x)			((((x)/16)*10) + ((x)%16))

/*****************************************************************************
 * \name  pcf85063a_check
 * \brief Check date/time validity and try to fix
 *****************************************************************************/
static hal_err_t pcf85063a_check(HAL_RTC_DATE_TIME * const datetime)
{
	hal_err_t result = HAL_E_OK;
	
	if (datetime->year < 2020 || datetime->year > 2199)
	{
		datetime->year = 2000;
		result = HAL_E_PARAM;
	}
	
	if (datetime->month < 1 || datetime->month > 12)
	{
		datetime->month = 1;
		result = HAL_E_PARAM;
	}
	
	if (datetime->day < 1 || datetime->day > 31)
	{
		datetime->day = 1;
		result = HAL_E_PARAM;
	}	
	
	if (datetime->weekday < 1 || datetime->weekday > 7)
	{
		datetime->weekday = 1;
		result = HAL_E_PARAM;
	}		
	
	if (datetime->hour > 23)
	{
		datetime->hour = 0;
		result = HAL_E_PARAM;
	}		
	
	if (datetime->minute > 59)
	{
		datetime->minute = 0;
		result = HAL_E_PARAM;
	}		
	
	if (datetime->second > 59)
	{
		datetime->second = 0;
		result = HAL_E_PARAM;
	}			
	
	if (datetime->millisecond > 999)
	{
		datetime->millisecond = 0;
		result = HAL_E_PARAM;
	}			
	
	return result;
}

/*****************************************************************************
 * \name  pcf85063a_decode
 * \brief Decode date/time from registers values
 *****************************************************************************/
static hal_err_t pcf85063a_decode(const uint8_t * const registers, HAL_RTC_DATE_TIME * const datetime)
{
	datetime->millisecond	= 0;
	datetime->second		= PCF85063A_BCD_DECODE(registers[0]&0x7F);
	datetime->minute		= PCF85063A_BCD_DECODE(registers[1]&0x7F);
	datetime->hour			= PCF85063A_BCD_DECODE(registers[2]&0x3F);
	datetime->day			= PCF85063A_BCD_DECODE(registers[3]&0x3F);  
	datetime->weekday		= PCF85063A_BCD_DECODE(registers[4]&0x07);  
	datetime->month			= PCF85063A_BCD_DECODE(registers[5]&0x1F);  
	datetime->year			= PCF85063A_BCD_DECODE(registers[6]); 
	datetime->year			+= 2000;

	return pcf85063a_check(datetime);
}

/*****************************************************************************
 * \name  pcf85063a_encode
 * \brief Encode date/time to registers values
 *****************************************************************************/
static hal_err_t pcf85063a_encode(HAL_RTC_DATE_TIME * const datetime, uint8_t * const registers)
{
	registers[0] = PCF85063A_BCD_ENCODE(datetime->second);
	registers[1] = PCF85063A_BCD_ENCODE(datetime->minute);
	registers[2] = PCF85063A_BCD_ENCODE(datetime->hour);
	registers[3] = PCF85063A_BCD_ENCODE(datetime->day);
	registers[4] = PCF85063A_BCD_ENCODE(datetime->weekday);
	registers[5] = PCF85063A_BCD_ENCODE(datetime->month);
	registers[6] = PCF85063A_BCD_ENCODE(datetime->year % 100);

	return pcf85063a_check(datetime);
}

/*****************************************************************************
 * \name  pcf85063a_read
 * \brief Read date/time registers on selected I2C port
 * \param i2c_port: I2C port index 
 * \param datetime: Pointer to read Date/Time
 *****************************************************************************/
hal_err_t pcf85063a_read(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime)
{
	hal_err_t result = HAL_E_FAIL;
	uint8_t address = PCF85063A_REGITER_DATE_TIME;
	uint8_t registers[7];

	// Write address
	result = hal_i2c_master_write(i2c_port, PCF85063A_ADDR, &address, 1);
	if (result == HAL_E_OK)
	{
		// Read data
		result = hal_i2c_master_read(i2c_port, PCF85063A_ADDR, &registers[0], sizeof(registers));
		if (result == HAL_E_OK)
		{        
			// Decode (ignore errors)
			pcf85063a_decode(registers, datetime);
		}
	}
	
	return result;
}

/****************************************************************************
 * \name  pcf85063a_write
 * \brief Write date/time registers on selected I2C port
 * \param i2c_port: I2C port index 
 * \param datetime: Pointer to Date/Time to write
 *****************************************************************************/
hal_err_t pcf85063a_write(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime)
{
	// Write Control 1 config (24h mode)
	uint8_t buffer[8];

	buffer[0] = PCF85063A_REGISTER_CTRL1;
	buffer[1] = 0x00u;
	hal_err_t result = hal_i2c_master_write(i2c_port, PCF85063A_ADDR, buffer, 2);
	if (result == HAL_E_OK)
	{
		// Write new date/time
		buffer[0] = PCF85063A_REGITER_DATE_TIME;
		pcf85063a_encode(datetime, &buffer[1]);
		result = hal_i2c_master_write(i2c_port, PCF85063A_ADDR, buffer, sizeof(buffer));
	}

	return result;
}