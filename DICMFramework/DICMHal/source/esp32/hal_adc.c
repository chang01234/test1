/*****************************************************************************
 * \file       hal_adc.c
 * \brief      ESP32 ADC Hardware Abstraction
 *****************************************************************************/

#include "dicm_framework_config.h"

#ifdef HAL_ADC

//!< **** Includes *************************************** */
#include <string.h>
#include "hal_adc.h"
#include "esp_log.h"


//!< **** Defines **************************************** */
#define LOG_ADC_EN	0
/** Defines *******************************************************************/
#define ESP32_DEFAULT_VREF    1100        // Use adc2_vref_to_gpio() to obtain a better estimate

//!< **** Variables ************************************** */



//!< **** Prototypes ************************************* */
//Drivers
static esp_adc_cal_characteristics_t adc_chars;


//!< **** Functionality ********************************** */

/**
 * @brief initialize adc to read analog values
 * 
 * @param conf - adc configuration details
 * @return int adc initialize success / failed
 */
int hal_adc_init(const HAL_ADC_INIT_CONF *conf)
{
	int adc_ret = HAL_GPIO_OK; 
	esp_adc_cal_characteristics_t	*adc_chars;
	esp_adc_cal_value_t val_type;

	adc_chars = conf->esp32.adc_chars;

	adc_ret = adc1_config_width(conf->esp32.width);

	if ( adc_ret == ESP_ERR_INVALID_ARG )
	{
		LOG(I,"adc config width failed");
		return adc_ret;
	}

    adc_ret = adc1_config_channel_atten(conf->esp32.channel, conf->esp32.attenuation);

	if ( adc_ret == ESP_ERR_INVALID_ARG )
	{
		LOG(I,"adc config attenuation failed");
		return adc_ret;
	}

	val_type = esp_adc_cal_characterize(conf->esp32.unit, conf->esp32.attenuation, conf->esp32.width, ESP32_DEFAULT_VREF, adc_chars);

    return val_type;
}

/**
 * @brief read adc value
 * 
 * @param channel 	: channel number
 * @param value 	: pointer to store adc value
 * @param samples 	: number of samples
 * @return int 
 */
int hal_adc_measure(int channel, uint16_t *value, uint16_t samples)
{
	uint32_t adc_reading = 0;
    int sample_count = 0;
    
    for ( sample_count = 0; sample_count < samples; sample_count++ ) 
    {
        //adc_reading += adc1_get_raw((adc1_channel_t)adc_configuration.channel);
        adc_reading += adc1_get_raw(channel);
    }

    adc_reading /= samples;

	*value = adc_reading;

	return 0;
}

/**
 * @brief convert adc raw value to Analog voltage value 
 * 
 * @param adc_raw_value :	adc raw value input
 * @param adc_chararct 	:	adc characteristics
 * @return uint32_t 	: 	return converted adc value 
 */
uint32_t hal_adc_to_mv_conversion(uint16_t adc_raw_value, esp_adc_cal_characteristics_t adc_chararct)
{
	uint32_t adc_voltage;

	adc_chars	 = adc_chararct;

	adc_voltage = esp_adc_cal_raw_to_voltage(adc_raw_value, &adc_chars);
	
	#if LOG_ADC_EN
    	LOG(I,"[con_ADC RAW %d V = %d ]", adc_raw_value, adc_voltage);
	#endif
	return adc_voltage;
}

#endif