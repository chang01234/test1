/*! \file hal_analog.h
 *	\brief Hardware Abstraction Layer for analog inputs (ADC/Comp)
 *
 *  HAL used for measuring analog voltages.
 */

#ifndef HAL_ANALOG_H_
#define HAL_ANALOG_H_

/** Includes ******************************************************************/
#include <stdint.h>

/** Defines *******************************************************************/

/*! Used to pass application specific configuration for nRF52 implementation.
 *
 *  gain should be a value from \p nrf_saadc_gain_t.
 *  acq_time should be a value from \p nrf_saadc_acqtime_t.
 */
typedef struct HAL_ANALOG_NRF52_CONF
{
     int gain;
     int acq_time;
} HAL_ANALOG_NRF52_CONF;

typedef struct HAL_ANALOG_INIT_CONF
{
    union
    {
        HAL_ANALOG_NRF52_CONF nrf52;
    };
} HAL_ANALOG_INIT_CONF;

/** Function prototypes *******************************************************/

/*! \brief HAL init function.
 *
 *  Must be called before calling any other hal_analog functions. Should only be
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
int hal_analog_init(const HAL_ANALOG_INIT_CONF *conf);

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
int hal_analog_measure(int channel, uint16_t *value, uint16_t samples);

#endif //HAL_ANALOG_H_