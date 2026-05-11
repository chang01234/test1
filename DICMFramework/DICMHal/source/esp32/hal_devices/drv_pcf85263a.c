/*****************************************************************************
 * \file       drv_pcf85263a
 * \brief      Driver for I2C RTC device PCF85263A
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
#include "drv_pcf85263a.h"
#include "hal_i2c_master.h"

/*****************************************************************************
 * Local defines
 *****************************************************************************/
#define PCF85263A_ADDR					0x51   // r=0xa2 w=0xa3
#define PCF85263A_REGITER_DATE_TIME		0x00
#define PCF85263A_REGITER_OSC			0x25
#define PCF85263A_BCD_ENCODE(x)			((((x)/10)*16) + ((x)%10))
#define PCF85263A_BCD_DECODE(x)			((((x)/16)*10) + ((x)%16))

/*****************************************************************************
 * \name  pcf85263a_check
 * \brief Check date/time validity and try to fix
 *****************************************************************************/
static hal_err_t pcf85263a_check(HAL_RTC_DATE_TIME * const datetime)
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
 * \name  pcf85263a_decode
 * \brief Decode date/time from registers values
 *****************************************************************************/
static hal_err_t pcf85263a_decode(const uint8_t * const registers, HAL_RTC_DATE_TIME * const datetime)
{
	datetime->millisecond	= PCF85263A_BCD_DECODE(registers[0]);
	datetime->millisecond	*= 10; // 1/100s to 1/1000s
	datetime->second		= PCF85263A_BCD_DECODE(registers[1]&0x7F);
	datetime->minute		= PCF85263A_BCD_DECODE(registers[2]);
	datetime->hour			= PCF85263A_BCD_DECODE(registers[3]);
	datetime->day			= PCF85263A_BCD_DECODE(registers[4]);  
	datetime->weekday		= PCF85263A_BCD_DECODE(registers[5]);  
	datetime->month			= PCF85263A_BCD_DECODE(registers[6]);  
	datetime->year			= PCF85263A_BCD_DECODE(registers[7]); 
	datetime->year			+= 2000;

	return pcf85263a_check(datetime);
}

/*****************************************************************************
 * \name  pcf85263a_encode
 * \brief Encode date/time to registers values
 *****************************************************************************/
static hal_err_t pcf85263a_encode(HAL_RTC_DATE_TIME * const datetime, uint8_t * const registers)
{
	registers[0] = PCF85263A_BCD_ENCODE(datetime->millisecond / 10);
	registers[1] = PCF85263A_BCD_ENCODE(datetime->second);
	registers[2] = PCF85263A_BCD_ENCODE(datetime->minute);
	registers[3] = PCF85263A_BCD_ENCODE(datetime->hour);
	registers[4] = PCF85263A_BCD_ENCODE(datetime->day);
	registers[5] = PCF85263A_BCD_ENCODE(datetime->weekday);
	registers[6] = PCF85263A_BCD_ENCODE(datetime->month);
	registers[7] = PCF85263A_BCD_ENCODE(datetime->year % 100);

	return pcf85263a_check(datetime);
}

/*****************************************************************************
 * \name  pcf85263a_read
 * \brief Read date/time registers on selected I2C port
 * \param i2c_port: I2C port index 
 * \param datetime: Pointer to read Date/Time
 *****************************************************************************/
hal_err_t pcf85263a_read(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime)
{
	hal_err_t result = HAL_E_FAIL;
	uint8_t address = PCF85263A_REGITER_DATE_TIME;
	uint8_t registers[8];

	// Write address
	result = hal_i2c_master_write(i2c_port, PCF85263A_ADDR, &address, 1);
	if (result == HAL_E_OK)
	{
		// Read data
		result = hal_i2c_master_read(i2c_port, PCF85263A_ADDR, &registers[0], sizeof(registers));
		if (result == HAL_E_OK)
		{        
			// Decode (ignore errors)
			pcf85263a_decode(registers, datetime);
		}
	}
	return result;
}

/****************************************************************************
 * \name  pcf85263a_write
 * \brief Write date/time registers on selected I2C port
 * \param i2c_port: I2C port index 
 * \param datetime: Pointer to Date/Time to write
 *****************************************************************************/
hal_err_t pcf85263a_write(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime)
{
	// Write oscillator config (24h mode)
	uint8_t Buffer[9];
	Buffer[0] = PCF85263A_REGITER_OSC;
	Buffer[1] = 0x00u;
	hal_err_t result = hal_i2c_master_write(i2c_port, PCF85263A_ADDR, Buffer, 2);
	if (result == HAL_E_OK)
	{
		// Write new date/time
		Buffer[0] = PCF85263A_REGITER_DATE_TIME;
		pcf85263a_encode(datetime, &Buffer[1]);
		result = hal_i2c_master_write(i2c_port, PCF85263A_ADDR, Buffer, sizeof(Buffer));
	}

	return result;
}