/*! \file hal_adc.c
	\brief ADC Hardware Abstraction Layer nRF52 implementation
*/

/** Includes ******************************************************************/
#include "hal_adc.h"

#include <stdint.h>
#include <stdbool.h>

#include "hal_scheduler.h"

#include "nrfx_saadc.h"
#include "nrf_pwr_mgmt.h"
#include "app_error.h"

/** Defines *******************************************************************/
#define ADC_VREF            600         // ADC voltage reference voltage in millivolts.
#define ADC_RECAL_PERIOD    1000*60*60  // Period between ADC recalibrations in milliseconds.

/** Variables *****************************************************************/
static nrf_saadc_gain_t gain;
static nrf_saadc_acqtime_t acq_time;
static uint16_t vmax;

static bool init_complete = false;
static volatile bool adc_done;

/** Private functions *********************************************************/

/*! \brief ADC callback function.
 *
 *  Used by HAL to determine when an ADC measurement or calibration is done.
 *
 *  \param event    Event that triggered callback.
 */
static void adc_cb(const nrfx_saadc_evt_t *event)
{
    adc_done = true;
}

/*! \brief Scheduler callback used for regular calibration.
 *
 *  This function is scheduled by the HAL to periodically recalibrate the ADC.
 * 
 *  \param data Unused. Required by scheduler.
 *  \paran size Unused. Required by scheduler.
 */
static void calibration_task(void *data, size_t size)
{
    APP_ERROR_CHECK_BOOL(init_complete);
    
    nrfx_saadc_config_t adc_conf = { 
        .resolution = NRF_SAADC_RESOLUTION_12BIT,
        .oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
        .low_power_mode = true,
        .interrupt_priority = NRFX_SAADC_CONFIG_IRQ_PRIORITY,
    };
    
    APP_ERROR_CHECK(nrfx_saadc_init(&adc_conf, adc_cb));
    
    adc_done = false;
    APP_ERROR_CHECK(nrfx_saadc_calibrate_offset());
    
    while (!adc_done)
    {
        nrf_pwr_mgmt_run();
    }
    
    nrfx_saadc_uninit();
}

/*! \brief Check if periodic calibration is running. If not start it.
 *
 *  Used to calibrate the nRF52 ADC the first time it is used after boot. Also
 *  schedules a recalibration every \p ADC_RECAL_PERIOD milliseconds.
 */
static void calibration_init(void)
{
    static bool calibration_running = false;
    
    if (!calibration_running)
    {
        // Initial calibration
        calibration_task(NULL, 0);
        
        // Recalibrate every hour.
        hal_scheduler_add_periodic(calibration_task, NULL, 0, ADC_RECAL_PERIOD);
        
        calibration_running = true;
    }
}

/** Public functions **********************************************************/

int hal_adc_init(const HAL_ADC_INIT_CONF *conf)
{
    if (!init_complete)
    {
        gain = conf->nrf52.gain;
        acq_time = conf->nrf52.acq_time;

        switch(gain)
        {
            case NRF_SAADC_GAIN1_6:
                vmax = 6*ADC_VREF;
                break;
            case NRF_SAADC_GAIN1_5:
                vmax = 5*ADC_VREF;
                break;
            case NRF_SAADC_GAIN1_4:
                vmax = 4*ADC_VREF;
                break;
            case NRF_SAADC_GAIN1_3:
                vmax = 3*ADC_VREF;
                break;
            case NRF_SAADC_GAIN1_2:
                vmax = 2*ADC_VREF;
                break;
            case NRF_SAADC_GAIN1:
                vmax  = ADC_VREF;
                break;
            case NRF_SAADC_GAIN2:
                vmax  = ADC_VREF/2;
                break;
            case NRF_SAADC_GAIN4:
                vmax  = ADC_VREF/4;
                break;
            default:
                APP_ERROR_CHECK_BOOL(false);    // Invalid gain specified.
        }

        init_complete = true;
    }

    return 0;   //If we reach this point no error occurred.
}

int hal_adc_measure(int channel, uint16_t *value, uint16_t samples)
{
    APP_ERROR_CHECK_BOOL(init_complete);
    APP_ERROR_CHECK_BOOL(samples > 0);
    
    uint16_t samples_left = samples;
    uint32_t value_sum = 0;
    
    calibration_init();
    
    while (samples_left > 0)
    {
        uint16_t n;
        bool burst;
        nrf_saadc_oversample_t oversample;
        if (samples_left == 1)
        {
            n = 1;
            burst = false;
            oversample = NRF_SAADC_OVERSAMPLE_DISABLED;
        }
        else if (samples_left < 4)
        {
            n = 2;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_2X;
        }
        else if (samples_left < 8)
        {
            n = 4;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_4X;
        }
        else if (samples_left < 16)
        {
            n = 8;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_8X;
        }
        else if (samples_left < 32)
        {
            n = 16;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_16X;
        }
        else if (samples_left < 64)
        {
            n = 32;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_32X;
        }
        else if (samples_left < 128)
        {
            n = 64;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_64X;
        }
        else if (samples_left < 256)
        {
            n = 128;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_128X;
        }
        else
        {
            n = 256;
            burst = true;
            oversample = NRF_SAADC_OVERSAMPLE_256X;
        }
        
        nrfx_saadc_config_t adc_conf = { 
            .resolution = NRF_SAADC_RESOLUTION_14BIT,
            .oversample = oversample,
            .low_power_mode = true,
            .interrupt_priority = NRFX_SAADC_CONFIG_IRQ_PRIORITY,
        };
        
        APP_ERROR_CHECK(nrfx_saadc_init(&adc_conf, adc_cb));

        nrf_saadc_channel_config_t channel_conf = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(channel);
        channel_conf.gain = gain;
        channel_conf.acq_time = acq_time;
        channel_conf.burst = burst;
    
        APP_ERROR_CHECK(nrfx_saadc_channel_init(0, &channel_conf));

        adc_done = false;
        nrf_saadc_value_t adc_buf = 0;
        APP_ERROR_CHECK(nrfx_saadc_buffer_convert(&adc_buf, 1));
        APP_ERROR_CHECK(nrfx_saadc_sample());

        while (!adc_done)
        {
            nrf_pwr_mgmt_run();
        }

        if (adc_buf < 0) { adc_buf = 0; }
        value_sum += (uint32_t)adc_buf*n;

        nrfx_saadc_uninit();
        
        samples_left -= n;
    }
    
    *value = (vmax*(value_sum/samples))/16383U; // 16383 = 2^14 - 1.
}