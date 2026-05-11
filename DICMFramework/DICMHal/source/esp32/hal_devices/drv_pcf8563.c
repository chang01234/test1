#include <string.h>
#include <time.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "freertos/FreeRTOS.h"
#pragma GCC diagnostic pop
#include "freertos/task.h"
#include "esp_log.h"
#include "dicm_framework_config.h"

#ifdef RTC_PCF8563

#include "drv_pcf8563.h"
#include "hal_rtc.h"
#include "hal_i2c_master.h"

#define PCF8563_BCD_ENCODE(x)			((((x)/10)*16) + ((x)%10))
#define PCF8563_BCD_DECODE(x)			((((x)/16)*10) + ((x)%16))

#define PCF8563_ADDR					    0x51   // r=0xa2 w=0xa3
#define PCF8563_REGITER_DATE_TIME		    0x02
#define PCF8563_REGITER_CONTROL_STATUS_1	0x00
#define PCF8563_REGITER_CONTROL_STATUS_2	0x01

static hal_err_t pcf8563_check(HAL_RTC_DATE_TIME * const datetime);
static hal_err_t pcf8563_encode(HAL_RTC_DATE_TIME * const datetime, uint8_t * const registers);
static hal_err_t pcf8563_decode(uint8_t *registers, HAL_RTC_DATE_TIME * const datetime);

/*****************************************************************************
 * \name  pcf8563_check
 * \param datetime: Pointer to read Date/Time 
 *****************************************************************************/
static hal_err_t pcf8563_check(HAL_RTC_DATE_TIME * const datetime)
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
 * \name  pcf8563_encode
 * \brief Encode date/time to registers values
 * \param datetime: Pointer to read Date/Time 
 * \param registers: Pointer to register values to enocde 
 *****************************************************************************/
static hal_err_t pcf8563_encode(HAL_RTC_DATE_TIME * const datetime, uint8_t * const registers)
{
	registers[0] = PCF8563_BCD_ENCODE(datetime->second);
	registers[1] = PCF8563_BCD_ENCODE(datetime->minute);
	registers[2] = PCF8563_BCD_ENCODE(datetime->hour);
	registers[3] = PCF8563_BCD_ENCODE(datetime->day);
	registers[4] = PCF8563_BCD_ENCODE(datetime->weekday);
	registers[5] = PCF8563_BCD_ENCODE(datetime->month);
	registers[6] = PCF8563_BCD_ENCODE(datetime->year % 100);

	return pcf8563_check(datetime);
}


/*****************************************************************************
 * \name  pcf8563_decode
 * \brief Decode date/time from registers values
 * \param registers: Pointer to register values to deocde 
 * \param datetime: Pointer to read Date/Time
 *****************************************************************************/
static hal_err_t pcf8563_decode(uint8_t *registers, HAL_RTC_DATE_TIME * const datetime)
{
    registers[0] = registers[0] & 0x7F;
    registers[1] = registers[1] & 0x7F;
    registers[2] = registers[2] & 0x3F;
    registers[3] = registers[3] & 0x3F;
    registers[4] = registers[4] & 0x07;
    registers[5] = registers[5] & 0x1F;
    registers[6] = registers[6] & 0xFF;
  
    datetime->millisecond   = 0;
    datetime->second		= PCF8563_BCD_DECODE(registers[0]);
    datetime->minute		= PCF8563_BCD_DECODE(registers[1]);
    datetime->hour			= PCF8563_BCD_DECODE(registers[2]);
    datetime->day			= PCF8563_BCD_DECODE(registers[3]);  
    datetime->weekday		= PCF8563_BCD_DECODE(registers[4]);
    datetime->weekday       = datetime->weekday + 1;  
    datetime->month			= PCF8563_BCD_DECODE(registers[5]);       
    datetime->year          = PCF8563_BCD_DECODE(registers[6]); 
    datetime->year			+= 2000; 

    return pcf8563_check(datetime);
}


/****************************************************************************
 * \name  pcf8563_init
 * \brief Initialize PCF8563
 * \param i2c_port: I2C port index  
 *****************************************************************************/
hal_err_t pcf8563_init(const uint8_t i2c_port)
{
    hal_err_t result = HAL_E_FAIL;
	uint8_t buffer[3];

	buffer[0] = PCF8563_REGITER_CONTROL_STATUS_1;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
	result = hal_i2c_master_write(i2c_port, PCF8563_ADDR, buffer, sizeof(buffer));
	return result;
}


/*****************************************************************************
 * \name  pcf8563_read
 * \brief Read date/time registers on selected I2C port
 * \param i2c_port: I2C port index 
 * \param datetime: Pointer to read Date/Time
 *****************************************************************************/
hal_err_t pcf8563_read(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime)
{
	hal_err_t result = HAL_E_FAIL;
	uint8_t address = PCF8563_REGITER_DATE_TIME;
	uint8_t registers[7];

	// Write address
	result = hal_i2c_master_write(i2c_port, PCF8563_ADDR, &address, 1);
	if (result == HAL_E_OK)
	{
		// Read data
		result = hal_i2c_master_read(i2c_port, PCF8563_ADDR, &registers[0], sizeof(registers));
		if (result == HAL_E_OK)
		{        
			// Decode (ignore errors)
			pcf8563_decode(registers, datetime);
		}
	}
	return result;
}


/****************************************************************************
 * \name  pcf8563_write
 * \brief Write date/time registers on selected I2C port
 * \param i2c_port: I2C port index 
 * \param datetime: Pointer to Date/Time to write
 *****************************************************************************/
hal_err_t pcf8563_write(const uint8_t i2c_port, HAL_RTC_DATE_TIME * const datetime)
{
    hal_err_t result = HAL_E_FAIL;
	uint8_t Buffer[8];

    // Write new date/time
    datetime->weekday = datetime->weekday - 1;
    Buffer[0] = PCF8563_REGITER_DATE_TIME;
    pcf8563_encode(datetime, &Buffer[1]);
    result = hal_i2c_master_write(i2c_port, PCF8563_ADDR, Buffer, sizeof(Buffer));

	return result;
}
#endif

