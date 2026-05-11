/*! \file hal_adc.h
 *	\brief Hardware Abstraction Layer for ADC.
 *
 *  HAL used for measuring analog voltages.
 */

#ifndef HAL_ADC_H_
#define HAL_ADC_H_

/** Includes ******************************************************************/
#include <stdint.h>

#ifdef HAL_ADC
    #include "driver/gpio.h"
    #include "driver/adc.h"
    #include "esp_adc_cal.h"
#endif
/** Defines *******************************************************************/

/*! Used to pass application specific configuration for nRF52 implementation.
 *
 *  gain should be a value from \p nrf_saadc_gain_t.
 *  acq_time should be a value from \p nrf_saadc_acqtime_t.
 */
typedef struct HAL_ADC_NRF52_CONF
{
     int gain;
     int acq_time;
} HAL_ADC_NRF52_CONF;

/*! ESP32 ADC configuration structure
*   channel     : adc channel
*   width       : adc conversion width
*   attenuation : adc input range select
*   unit        : adc internal unit selection
*   *adc_chars  : to store adc characteristic value
 */ 
typedef struct _hal_adc_esp32_conf_
{
#ifdef HAL_ADC
    adc_channel_t channel;
    adc_bits_width_t width;
    adc_atten_t attenuation;
    adc_unit_t unit;
    esp_adc_cal_characteristics_t *adc_chars;
#endif
} HAL_ADC_ESP32_CONF;
typedef struct HAL_ADC_INIT_CONF
{
    union
    {
        HAL_ADC_NRF52_CONF nrf52;
        HAL_ADC_ESP32_CONF esp32;
    };
} HAL_ADC_INIT_CONF;

/** Function prototypes *******************************************************/

/*! \brief HAL init function.
 *
 *  Must be called before calling any other hal_adc functions. Should only be
 *  called once.
 *
 *  conf is used to provide application specific configuration. Exact contents
 *  and their data types used will depend on which platform implementation is 
 *  being used.
 *
 *  \param conf Application specific configuration data.
 *
 *  \returns    0 if successful. Any other value otherwise.
 */
int hal_adc_init(const HAL_ADC_INIT_CONF *conf);

/*! \brief Measure ADC channel samples times and return average voltage in millivolts.
 *
 *  This will do samples number of measurements on the specified ADC channel
 *  then calculate an average voltage of these measurements in millivolts 
 *  (relative GND) and write this voltage to the value pointer.
 *
 *  \param channel  Analog channel to measure.
 *  \param value    Pointer to which result will be written.
 *  \param samples  Number of samples to measure.
 *
 *  \returns    0 if successful. Any other value otherwise.
 */
int hal_adc_measure(int channel, uint16_t *value, uint16_t samples);

#ifdef HAL_ADC
/**
 * @brief convert adc raw value to Analog voltage value 
 * 
 * @param adc_raw_value :	adc raw value input
 * @param adc_chararct 	:	adc characteristics
 * @return uint32_t 	: 	return converted adc value 
 */
uint32_t hal_adc_to_mv_conversion(uint16_t adc_raw_value, esp_adc_cal_characteristics_t adc_chararct);
#endif

#endif //HAL_ADC_H_