/*
 * Copyright (C) 2017 Robert Bosch. All Rights Reserved.
 *
 * Disclaimer
 *
 * Common:
 * Bosch Sensortec products are developed for the consumer goods industry. They may only be used
 * within the parameters of the respective valid product data sheet.  Bosch Sensortec products are
 * provided with the express understanding that there is no warranty of fitness for a particular purpose.
 * They are not fit for use in life-sustaining, safety or security sensitive systems or any system or device
 * that may lead to bodily harm or property damage if the system or device malfunctions. In addition,
 * Bosch Sensortec products are not fit for use in products which interact with motor vehicle systems.
 * The resale and/or use of products are at the purchasers own risk and his own responsibility. The
 * examination of fitness for the intended use is the sole responsibility of the Purchaser.
 *
 * The purchaser shall indemnify Bosch Sensortec from all third party claims, including any claims for
 * incidental, or consequential damages, arising from any product use not covered by the parameters of
 * the respective valid product data sheet or not approved by Bosch Sensortec and reimburse Bosch
 * Sensortec for all costs in connection with such claims.
 *
 * The purchaser must monitor the market for the purchased products, particularly with regard to
 * product safety and inform Bosch Sensortec without delay of all security relevant incidents.
 *
 * Engineering Samples are marked with an asterisk (*) or (e). Samples may vary from the valid
 * technical specifications of the product series. They are therefore not intended or fit for resale to third
 * parties or for use in end products. Their sole purpose is internal client testing. The testing of an
 * engineering sample may in no way replace the testing of a product series. Bosch Sensortec
 * assumes no liability for the use of engineering samples. By accepting the engineering samples, the
 * Purchaser agrees to indemnify Bosch Sensortec from all claims arising from the use of engineering
 * samples.
 *
 * Special:
 * This software module (hereinafter called "Software") and any information on application-sheets
 * (hereinafter called "Information") is provided free of charge for the sole purpose to support your
 * application work. The Software and Information is subject to the following terms and conditions:
 *
 * The Software is specifically designed for the exclusive use for Bosch Sensortec products by
 * personnel who have special experience and training. Do not use this Software if you do not have the
 * proper experience or training.
 *
 * This Software package is provided `` as is `` and without any expressed or implied warranties,
 * including without limitation, the implied warranties of merchantability and fitness for a particular
 * purpose.
 *
 * Bosch Sensortec and their representatives and agents deny any liability for the functional impairment
 * of this Software in terms of fitness, performance and safety. Bosch Sensortec and their
 * representatives and agents shall not be liable for any direct or indirect damages or injury, except as
 * otherwise stipulated in mandatory applicable law.
 *
 * The Information provided is believed to be accurate and reliable. Bosch Sensortec assumes no
 * responsibility for the consequences of use of such Information nor for any infringement of patents or
 * other rights of third parties which may result from its use. No license is granted by implication or
 * otherwise under any patent or patent rights of Bosch. Specifications mentioned in the Information are
 * subject to change without notice.
 *
 * It is not allowed to deliver the source code of the Software to any third party without permission of
 * Bosch Sensortec.
 *
 */

/*!
 * @file bsec_integration.c
 *
 * @brief
 * Private part of the example for using of BSEC library.
 */

/*!
 * @addtogroup bsec_examples BSEC Examples
 * @brief BSEC usage examples
 * @{*/

/**********************************************************************************************************************/
/* header files */
/**********************************************************************************************************************/
#include "configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if ( CONFIG_DICM_SUPPORT_INTEGRATED_BSEC_LIB_2_X == 1 )

#include "bme68x.h"
#include "osal.h"
#include "bsec_integration_2_x.h"
#include "bme68x_defs.h"
#include "esp_attr.h"
#include "bsec_serialized_configurations_selectivity.h"
#include "hal_cpu.h"

/**********************************************************************************************************************/
/* local macro definitions */
/**********************************************************************************************************************/

#define NUM_USED_OUTPUTS                      ((uint8_t)    10u)
#define BME68X_FORCED_MODE_SLEEP_TIME_MS      ((uint32_t)    5u)   // 5 msec
#define BME68X_FORCED_MODE_MAX_WAIT_TIME_MS   ((uint32_t) 1000u)   // 1 sec
#define BME68X_FORCED_MODE_ERROR              ((int8_t)       1)
#define BME68X_BSEC_MAX_RETRY_COUNT           ((uint32_t)    5u)
#define BSEC_IDLE_STATE_SLEEP_INTERVAL        ((uint32_t) 1000u)
#define TIME_OFFSET_INTERVAL_MS               ((int64_t)    100)
#define BME_BSEC_WAIT_TIME_BEFORE_SLEEP_MS    ((uint32_t) 2000u)

typedef enum __bsec_process_state
{
    STATE_INIT         = 0,
    STATE_IDLE         = 1,
    STATE_SLEEP        = 2,
    STATE_BSEC_PROCESS = 3,
    STATE_WAIT         = 4,
    STATE_ERROR        = 5
}bsec_process_state;

//#define BSEC_LIB_INTEG_LOG
#define ENABLE_EXCUTION_TIME_DEBUG  0

//#ifdef CONFIG_DICM_SUPPORT_INTEGRATED_BSEC_LIB_2_X

/**********************************************************************************************************************/
/* global variable declarations */
/**********************************************************************************************************************/
extern const BME6X_BSEC_LIB_INTERFACE g_bsec_lib_intf;
extern uint32_t bsec_critical_error;
// FOR EOL testing
static int8_t bme68x_stat = 0;

/* Global sensor APIs data structure */
static struct bme68x_dev bme68x_g;

/* Global temperature offset to be subtracted */
static float bme68x_temperature_offset_g = 0.0f;

/* Save state variables */
EXT_RAM_ATTR uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
EXT_RAM_ATTR uint8_t bsec_config[BSEC_MAX_PROPERTY_BLOB_SIZE] = {0};
EXT_RAM_ATTR uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};

/* Variable for error count */
static uint32_t bme68x_error_retry_count = 0u;
static uint8_t interval_err = 0;
static uint8_t bme68x_init_flag = 0;

/* Instance to store the read data from BME680 */
EXT_RAM_ATTR bme68x_bsec_output bsec_data_output;

/* Allocate enough memory for up to BSEC_MAX_PHYSICAL_SENSOR physical inputs*/
EXT_RAM_ATTR bsec_input_t bsec_inputs[BSEC_MAX_PHYSICAL_SENSOR];

/* Output buffer set to the maximum virtual sensor outputs supported */
EXT_RAM_ATTR bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];

/* BSEC sensor settings struct */
EXT_RAM_ATTR bsec_bme_settings_t sensor_settings;

static bsec_process_state bsec_proc_st = STATE_INIT;

/* Timestamp variables */
static int64_t time_stamp = 0;
static int64_t time_stamp_interval_ms = 0;
static uint32_t sleep_interval = BSEC_IDLE_STATE_SLEEP_INTERVAL;
static bme68x_bsec_output bsec_out;
static bsec_library_return_t bsec_ret_sens_ctrl;
static bsec_library_return_t bsec_ret_proc_data;
static bsec_library_return_t bsec_ret_state_save;
static int8_t bme68x_status = BME68X_OK;
static uint8_t num_bsec_inputs = 0u;    /* Number of inputs to BSEC */
static uint32_t bsec_state_len = 0u;
EXT_RAM_ATTR struct bme68x_conf conf;
static struct bme68x_heatr_conf heatr_conf;
static struct bme68x_data bme68x_read_data;
static BME68X_ERROR bme68x_curr_err_stat = BME68X_NO_ERROR;
static BME68X_ERROR bme68x_prev_err_stat = BME68X_NO_ERROR;

static BME6X_BSEC_LIB_INTERFACE bsec_lib_intf;

#ifdef CONNECTOR_EOL_SERVICE
static uint32_t op_mode = BME68X_FORCED_MODE;
#else
static uint32_t op_mode = 0xFF;
#endif
#if ENABLE_EXCUTION_TIME_DEBUG
static TickType_t last_tick = 0;
#endif
static TickType_t diff_tick = 0;
static uint32_t bsec_config_len = 0;

extern void bme68x_error_code(const BME68X_ERROR error);

/**********************************************************************************************************************/
/* functions */
/**********************************************************************************************************************/

/*!
 * @brief        Function to init BSEC library interface
 *
 * @return       void
 */
void init_bsec_lib_interface(void)
{
    bsec_lib_intf.bus_read           = g_bsec_lib_intf.bus_read;
    bsec_lib_intf.bus_write          = g_bsec_lib_intf.bus_write;
    bsec_lib_intf.delay_us           = g_bsec_lib_intf.delay_us;
    bsec_lib_intf.config_load        = g_bsec_lib_intf.config_load;
    bsec_lib_intf.state_load         = g_bsec_lib_intf.state_load;
    bsec_lib_intf.state_save         = g_bsec_lib_intf.state_save;
    bsec_lib_intf.temperature_offset = g_bsec_lib_intf.temperature_offset;
    bsec_lib_intf.get_time_stamp_us  = g_bsec_lib_intf.get_time_stamp_us;
    bsec_lib_intf.output_ready       = g_bsec_lib_intf.output_ready;
    bsec_lib_intf.sample_rate        = g_bsec_lib_intf.sample_rate;
}

/*!
 * @brief        Virtual sensor subscription
 *               Please call this function before processing of data using bsec_do_steps function
 *
 * @param[in]    sample_rate         mode to be used (either BSEC_SAMPLE_RATE_ULP or BSEC_SAMPLE_RATE_LP)
 *
 * @return       subscription result, zero when successful
 */
static bsec_library_return_t bme68x_bsec_update_subscription(float sample_rate)
{
    bsec_sensor_configuration_t requested_virtual_sensors[NUM_USED_OUTPUTS];
    uint8_t n_requested_virtual_sensors = NUM_USED_OUTPUTS;

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    bsec_library_return_t status = BSEC_OK;

    /* note: Virtual sensors as desired to be added here */
    requested_virtual_sensors[0].sensor_id   = BSEC_OUTPUT_IAQ;
    requested_virtual_sensors[0].sample_rate = sample_rate;
    requested_virtual_sensors[1].sensor_id   = BSEC_OUTPUT_CO2_EQUIVALENT;
    requested_virtual_sensors[1].sample_rate = sample_rate;
    requested_virtual_sensors[2].sensor_id   = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[2].sample_rate = sample_rate;
    requested_virtual_sensors[3].sensor_id   = BSEC_OUTPUT_BREATH_VOC_EQUIVALENT;
    requested_virtual_sensors[3].sample_rate = sample_rate;
    requested_virtual_sensors[4].sensor_id   = BSEC_OUTPUT_RAW_GAS;
    requested_virtual_sensors[4].sample_rate = sample_rate;
    requested_virtual_sensors[5].sensor_id   = BSEC_OUTPUT_RAW_TEMPERATURE;
    requested_virtual_sensors[5].sample_rate = sample_rate;
    requested_virtual_sensors[6].sensor_id   = BSEC_OUTPUT_RAW_HUMIDITY;
    requested_virtual_sensors[6].sample_rate = sample_rate;
    requested_virtual_sensors[7].sensor_id   = BSEC_OUTPUT_STATIC_IAQ;
    requested_virtual_sensors[7].sample_rate = sample_rate;
    requested_virtual_sensors[8].sensor_id   = BSEC_OUTPUT_STABILIZATION_STATUS;
    requested_virtual_sensors[8].sample_rate = sample_rate;
    requested_virtual_sensors[9].sensor_id   = BSEC_OUTPUT_RUN_IN_STATUS;
    requested_virtual_sensors[9].sample_rate = sample_rate;

    /* Call bsec_update_subscription() to enable/disable the requested virtual sensors */
    status = bsec_update_subscription(requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings,
        &n_required_sensor_settings);

    return status;
}

/*!
 * @brief       Initialize BSEC and BME688 LIb
 *
 * @return      Return the return_values_init
 */
return_values_init initialize_bsec_lib(void)
{
    return_values_init ret = {BME68X_OK, BSEC_OK};

    /* Fixed I2C configuration */
    bme68x_g.intf     = BME68X_I2C_INTF;
    /* User configurable I2C configuration */
    bme68x_g.write    = bsec_lib_intf.bus_write;
    bme68x_g.read     = bsec_lib_intf.bus_read;
    bme68x_g.delay_us = bsec_lib_intf.delay_us;

    /* Initialize BME68x API */
    ret.bme68x_status = bme68x_init(&bme68x_g);
    if ( ret.bme68x_status != BME68X_OK )
    {
        return ret;
    }

    /* Initialize BSEC library */
    ret.bsec_status = bsec_init();
    if ( ret.bsec_status != BSEC_OK )
    {
        LOG(W, "bsec_init error = %d", ret.bsec_status);
        return ret;
    }

    /* Load library config, if available */
    bsec_config_len = bsec_lib_intf.config_load(bsec_config, sizeof(bsec_config));

    if ( bsec_config_len != 0 )
    {
        ret.bsec_status = bsec_set_configuration(bsec_config, bsec_config_len, work_buffer, sizeof(work_buffer));
        if ( ret.bsec_status != BSEC_OK )
        {
            LOG(E, "bsec_set_configuration error = %d", ret.bsec_status);
            return ret;
        }
    }

    /* Load previous library state, if available */
    bsec_state_len = bsec_lib_intf.state_load(bsec_state, sizeof(bsec_state));

    if ( bsec_state_len != 0u )
    {
        ret.bsec_status = bsec_set_state(bsec_state, bsec_state_len, work_buffer, sizeof(work_buffer));

        if ( ret.bsec_status != BSEC_OK )
        {
            if ( ret.bsec_status == BSEC_E_CONFIG_VERSIONMISMATCH )
            {
                LOG(W, "BSEC Version Mismatch error");

                bsec_ret_state_save = bsec_get_state(0, bsec_state, sizeof(bsec_state), work_buffer, sizeof(work_buffer), &bsec_state_len);

				if ( bsec_ret_state_save == BSEC_OK )
				{
                    LOG(W, "Write to NVS");
					bsec_lib_intf.state_save(bsec_state, bsec_state_len);
				}
            }
            else
            {
                LOG(E, "bsec_set_state error = %d", ret.bsec_status);
                return ret;
            }
        }
    }

    /* Set temperature offset */
    bme68x_temperature_offset_g = bsec_lib_intf.temperature_offset;

    /* Call to the function which sets the library with subscription information */
    ret.bsec_status = bme68x_bsec_update_subscription(bsec_lib_intf.sample_rate);
    if ( ret.bsec_status != BSEC_OK )
    {
        LOG(E, "bme68x_bsec_update_subscription error = %d", ret.bsec_status);
        return ret;
    }

    return ret;
}

/*!
 * @brief       Trigger the measurement based on sensor settings
 *
 * @param[in]   sensor_settings     settings of the BME680 sensor adopted by sensor control function
 *
 * @return      bme680_status       bme680 status
 */
static int8_t bme68x_bsec_trigger_measurement(bsec_bme_settings_t *sensor_settings)
{
    uint32_t meas_period;
    uint32_t sleep_time = 0u;
    uint8_t  mode;
    int8_t   bme68x_status = BME68X_E_SELF_TEST;

    /* Check if a forced-mode measurement should be triggered now */
    if ( sensor_settings->trigger_measurement )
    {
#ifdef BSEC_LIB_INTEG_LOG
        /* Set sensor configuration */
		LOG(I, "heater_temperature = %d ", sensor_settings->heater_temperature);
		LOG(I, "heating_duration = %d ", sensor_settings->heater_duration);
		LOG(I, "humidity_oversampling = %d ", sensor_settings->humidity_oversampling);
		LOG(I, "next_call = %llx ns ", sensor_settings->next_call);
		LOG(I, "pressure_oversampling = %d ", sensor_settings->pressure_oversampling);
		LOG(I, "process_data = %d ", sensor_settings->process_data);
		LOG(I, "run_gas = %d ", sensor_settings->run_gas);
		LOG(I, "temperature_oversampling = %d ", sensor_settings->temperature_oversampling);
		LOG(I, "trigger_measurement = %d ", sensor_settings->trigger_measurement);
        LOG(I, "op_mode = %d ", sensor_settings->op_mode);
        LOG(I, "run_gas = %d ", sensor_settings->run_gas);
#endif

        conf.filter  = BME68X_FILTER_OFF;
        conf.odr     = BME68X_ODR_NONE;
        conf.os_hum  = sensor_settings->humidity_oversampling;
        conf.os_pres = sensor_settings->pressure_oversampling;
        conf.os_temp = sensor_settings->temperature_oversampling;

        /* Set sensor sampling configuration */
        bme68x_status = bme68x_set_conf(&conf, &bme68x_g);

        heatr_conf.enable          = sensor_settings->run_gas;
        heatr_conf.heatr_temp      = sensor_settings->heater_temperature;
        heatr_conf.heatr_dur       = sensor_settings->heater_duration;
        heatr_conf.heatr_dur_prof  = sensor_settings->heater_duration_profile;
        heatr_conf.heatr_temp_prof = sensor_settings->heater_temperature_profile;
        heatr_conf.profile_len     = sensor_settings->heater_profile_len;

        /* Set sensor heater configuration */
        bme68x_status = bme68x_set_heatr_conf(sensor_settings->op_mode, &heatr_conf, &bme68x_g);
        /* Set the operating mode */
        bme68x_status = bme68x_set_op_mode(sensor_settings->op_mode, &bme68x_g);
        /* Get the gas measurement duration */
        meas_period = bme68x_get_meas_dur(sensor_settings->op_mode, &conf, &bme68x_g);
        meas_period += ( heatr_conf.heatr_dur * 1000 );

        bsec_lib_intf.delay_us(meas_period, NULL);
    }

    if ( BME68X_OK == bme68x_status )
    {
#ifdef BSEC_LIB_INTEG_LOG
        LOG(I, "Wait bme68x_status = %d ", bme68x_status);
#endif
        /* Call the API to get current operation mode of the sensor */
        bme68x_status = bme68x_get_op_mode(&mode, &bme68x_g);
        /* When the measurement is completed and data is ready for reading, the sensor must be in BME68X_SLEEP_MODE.
           Read operation mode to check whether measurement is completely done and
           wait until the sensor is no more in BME68X_FORCED_MODE. */

        while ( ( mode == BME68X_FORCED_MODE ) && ( BME68X_OK == bme68x_status ) )
        {
            /* sleep for 5 ms */
            bsec_lib_intf.delay_us(BME68X_FORCED_MODE_SLEEP_TIME_MS * 1000, NULL);
            bme68x_status = bme68x_get_op_mode(&mode, &bme68x_g);

            sleep_time += BME68X_FORCED_MODE_SLEEP_TIME_MS;

            if ( sleep_time >= BME68X_FORCED_MODE_MAX_WAIT_TIME_MS )
            {
                bme68x_status = BME68X_FORCED_MODE_ERROR;
                break;
            }
        }
    }

    return bme68x_status;
}

/*!
 * @brief       Read the data from registers and populate the inputs structure to be passed to do_steps function
 *
 * @param[in]   time_stamp_trigger      settings of the sensor returned from sensor control function
 * @param[in]   inputs                  input structure containing the information on sensors to be passed to do_steps
 * @param[in]   num_bsec_inputs         number of inputs to be passed to do_steps
 * @param[in]   bsec_process_data       process data variable returned from sensor_control
 *
 * @return      bme680_status
 */
static int8_t bme68x_bsec_read_data(int64_t time_stamp_trigger, bsec_input_t *inputs, uint8_t *num_bsec_inputs,
    int32_t bsec_process_data)
{
    int8_t bme680_status = -1; //BME68X_OK;
    uint8_t num_data = 0;

    /* We only have to read data if the previous call the bsec_sensor_control() actually asked for it */
    if ( bsec_process_data )
    {
        bme680_status = bme68x_get_data(BME68X_FORCED_MODE, &bme68x_read_data, &num_data, &bme68x_g);
#ifdef BSEC_LIB_INTEG_LOG
        LOG(W, "num_data = %d", num_data);
#endif
        if ( BME68X_OK == bme680_status )
        {
            if ( bme68x_read_data.status & BME68X_NEW_DATA_MSK )
            {
                /* Pressure to be processed by BSEC */
                if ( bsec_process_data & BSEC_PROCESS_PRESSURE )
                {
                    /* Place presssure sample into input struct */
                    inputs[*num_bsec_inputs].sensor_id  = BSEC_INPUT_PRESSURE;
                    inputs[*num_bsec_inputs].signal     = bme68x_read_data.pressure;
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                    #ifdef BSEC_LIB_INTEG_LOG
                        #ifdef BME68X_USE_FPU
                            LOG(I, "Raw pressure data = %f", bme68x_read_data.pressure);
                        #else
                            LOG(I, "Raw pressure data = %d", bme68x_read_data.pressure);
                        #endif
                    #endif
                }
                /* Temperature to be processed by BSEC */
                if ( bsec_process_data & BSEC_PROCESS_TEMPERATURE )
                {
                    /* Place temperature sample into input struct */
                    inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
                    #ifdef BME68X_USE_FPU
                        inputs[*num_bsec_inputs].signal = bme68x_read_data.temperature;
                    #else
                        inputs[*num_bsec_inputs].signal = bme68x_read_data.temperature / 100.0f;
                    #endif
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;

                    /* Also add optional heatsource input which will be subtracted from the temperature reading to
                    * compensate for device-specific self-heating (supported in BSEC IAQ solution) */
                    inputs[*num_bsec_inputs].sensor_id  = BSEC_INPUT_HEATSOURCE;
                    inputs[*num_bsec_inputs].signal     = bme68x_temperature_offset_g;
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                    #ifdef BSEC_LIB_INTEG_LOG
                        #ifdef BME68X_USE_FPU
                            LOG(I, "Raw temperature data = %f", (bme68x_read_data.temperature / 100));
                        #else
                            LOG(I, "Raw temperature data = %d", bme68x_read_data.temperature);
                        #endif
                    #endif
                }
                /* Humidity to be processed by BSEC */
                if ( bsec_process_data & BSEC_PROCESS_HUMIDITY )
                {
                    /* Place humidity sample into input struct */
                    inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
                    #ifdef BME68X_USE_FPU
                        inputs[*num_bsec_inputs].signal = bme68x_read_data.humidity;
                    #else
                        inputs[*num_bsec_inputs].signal = bme68x_read_data.humidity / 1000.0f;
                    #endif
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                    #ifdef BSEC_LIB_INTEG_LOG
                        #ifdef BME68X_USE_FPU
                            LOG(I, "Raw humdity data = %f", (bme68x_read_data.humidity / 1000.0f));
                        #else
                            LOG(I, "Raw humdity data = %d", bme68x_read_data.humidity);
                        #endif
                    #endif
                }
                /* Gas to be processed by BSEC */
                if ( bsec_process_data & BSEC_PROCESS_GAS )
                {
                    /* Check whether gas_valid flag is set */
                    if( bme68x_read_data.status & BME68X_GASM_VALID_MSK )
                    {
                        /* Place sample into input struct */
                        inputs[*num_bsec_inputs].sensor_id  = BSEC_INPUT_GASRESISTOR;
                        inputs[*num_bsec_inputs].signal     = bme68x_read_data.gas_resistance;
                        inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                        (*num_bsec_inputs)++;
                        #ifdef BSEC_LIB_INTEG_LOG
                            #ifdef BME68X_USE_FPU
                                LOG(I, "Raw gas_resistance data = %f", bme68x_read_data.gas_resistance);
                            #else
                                LOG(I, "Raw gas_resistance data = %d", bme68x_read_data.gas_resistance);
                            #endif
                        #endif
                    }
                }
            }
        }
    }

    return bme680_status;
}

/*!
 * @brief       This function is written to process the sensor data for the requested virtual sensors
 *
 * @param[in]   bsec_inputs         input structure containing the information on sensors to be passed to do_steps
 * @param[in]   num_bsec_inputs     number of inputs to be passed to do_steps
 * @param[in]   output_ready        pointer to the function processing obtained BSEC outputs
 *
 * @return      bsec return code
 */
static bsec_library_return_t bme68x_bsec_process_data(bsec_input_t *bsec_inputs, uint8_t num_bsec_inputs)
{
    uint8_t num_bsec_outputs = 0;
    uint8_t index = 0;
    bsec_library_return_t bsec_status = BSEC_OK;

#ifdef BSEC_LIB_INTEG_LOG
    LOG(I, "num_bsec_inputs = %d", num_bsec_inputs);
#endif

    /* Check if something should be processed by BSEC */
    if ( num_bsec_inputs > 0 )
    {
        /* Set number of outputs to the size of the allocated buffer */
        /* BSEC_NUMBER_OUTPUTS to be defined */
        num_bsec_outputs = BSEC_NUMBER_OUTPUTS;

        /* Perform processing of the data by BSEC
           Note:
           * The number of outputs you get depends on what you asked for during bsec_update_subscription(). This is
             handled under bme680_bsec_update_subscription() function in this example file.
           * The number of actual outputs that are returned is written to num_bsec_outputs. */
        bsec_status = bsec_do_steps(bsec_inputs, num_bsec_inputs, bsec_outputs, &num_bsec_outputs);

        if ( BSEC_OK == bsec_status )
        {
            /* Iterate through the outputs and extract the relevant ones. */
            for ( index = 0; index < num_bsec_outputs; index++ )
            {
                switch ( bsec_outputs[index].sensor_id )
                {
                    case BSEC_OUTPUT_IAQ:
                        bsec_data_output.iaq.data                           = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.iaq.accuracy                       = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_STATIC_IAQ:
                        bsec_data_output.static_iaq.data                    = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.static_iaq.accuracy                = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_CO2_EQUIVALENT:
                        bsec_data_output.co2_equivalent.data                = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.co2_equivalent.accuracy            = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                        bsec_data_output.breath_voc_eq.data                 = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.breath_voc_eq.accuracy             = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                        bsec_data_output.sensor_heat_comp_temp.data         = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.sensor_heat_comp_temp.accuracy     = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_PRESSURE:
                        bsec_data_output.raw_pressure.data                  = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_pressure.accuracy              = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                        bsec_data_output.sensor_heat_comp_humidity.data     = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.sensor_heat_comp_humidity.accuracy = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_GAS:
                        bsec_data_output.raw_gas.data                       = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_gas.accuracy                   = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_TEMPERATURE:
                        bsec_data_output.raw_temperature.data               = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_temperature.accuracy           = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_HUMIDITY:
                        bsec_data_output.raw_humidity.data                  = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_humidity.accuracy              = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_GAS_PERCENTAGE:
                        bsec_data_output.gas_percentage.data                = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.gas_percentage.accuracy            = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_STABILIZATION_STATUS:
                        bsec_data_output.stabilization_status.data          = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.stabilization_status.accuracy      = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RUN_IN_STATUS:
                        bsec_data_output.run_in_status.data                 = (bme68x_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.run_in_status.accuracy             = bsec_outputs[index].accuracy;
                        break;
                    default:
                        continue;
                }

                /* Assume that all the returned timestamps are the same */
                bsec_data_output.time_stamp = bsec_outputs[index].time_stamp;
            }

            /* Pass the extracted outputs to the user provided output_ready() function. */
            bsec_lib_intf.output_ready(&bsec_data_output);
        }
        else
        {
            LOG(E, "bsec_do_steps error %d", bsec_status);
        }
    }

    return bsec_status;
}
#if 0
static bsec_library_return_t bsec_reinit(void)
{
    bsec_library_return_t bsec_ret_sens_ctrl;

    /* Initialize BSEC library */
    bsec_ret_sens_ctrl = bsec_init();

    /* Load library config, if available */
    if ( bsec_config_len != 0 )
    {
        bsec_ret_sens_ctrl |= bsec_set_configuration(bsec_config, bsec_config_len, work_buffer, sizeof(work_buffer));
    }

    /* Load previous library state, if available */
    if ( bsec_state_len != 0 )
    {
        bsec_ret_sens_ctrl |= bsec_set_state(bsec_state, bsec_state_len, work_buffer, sizeof(work_buffer));
    }

    /* Set temperature offset */
    bme68x_temperature_offset_g = bsec_lib_intf.temperature_offset;

    /* Call to the function which sets the library with subscription information */
    bsec_ret_sens_ctrl |= bme68x_bsec_update_subscription(bsec_lib_intf.sample_rate);

    return bsec_ret_sens_ctrl;
}
#endif
/*!
 * @brief       Runs the main (endless) loop that queries sensor settings, applies them, and processes the measured data
 *
 * @param[in]   void
 *
 * @return      none
 */
void bsec_processing_loop(osal_queue_handle_t queue)
{
    while (1)
    {
        if ( pdPASS == xQueueReceive(queue, (void *)&op_mode, 0) )
        {
#ifdef BSEC_LIB_INTEG_LOG
            LOG(I, "BME68X Operating Mode = %d", op_mode);
#endif
            if ( BME68X_SLEEP_MODE == op_mode )
            {
                bsec_proc_st = STATE_WAIT;
            }
        }

        switch (bsec_proc_st)
        {
            case STATE_INIT:
                if ( BME68X_FORCED_MODE == op_mode )
                {
                    /* Initialize the BME680 sensor */
                    return_values_init ret_val = initialize_bsec_lib();
                    bme68x_stat = ret_val.bme68x_status;
                    if ( ( BSEC_OK == ret_val.bsec_status ) && ( BME68X_OK == ret_val.bme68x_status ) )
                    {
                        LOG(I, "BME68X Init success");
                        bsec_proc_st = STATE_BSEC_PROCESS;
                        bme68x_curr_err_stat = BME68X_NO_ERROR;
                    }
                    else
                    {
                        LOG(E, "BME68X Init failed : bme680_status =  %d, bsec_status = %d ", ret_val.bme68x_status, ret_val.bsec_status);
                        bsec_proc_st = STATE_ERROR;
                        bme68x_curr_err_stat = BME68X_COMM_ERROR;
                    }
                }
                break;

            case STATE_BSEC_PROCESS:
#if ENABLE_EXCUTION_TIME_DEBUG
                diff_tick = xTaskGetTickCount() - last_tick;
                LOG(I,"%" PRId64 "", time_stamp_interval_ms);
                LOG(I, "BSEC execution interval %d ms | configured %d ms", pdTICKS_TO_MS(diff_tick), sleep_interval);
#endif


                if(bme68x_init_flag == 0)
                {
                    bme68x_init_flag = 1;
                    LOG(I,"init_flag_init_process  %d ", bme68x_init_flag);
                }
                else
                {
                    if( pdTICKS_TO_MS(diff_tick) > (sleep_interval + 30) )
                    {
                            LOG(I,"[Bsec time Xceed, Need init/restart]");
                            interval_err = 1;
                    }

                }

                if(interval_err == 0)
                {
                    /* get the timestamp in nanoseconds before calling bsec_sensor_control() */
                    time_stamp = bsec_lib_intf.get_time_stamp_us() * 1000;

                    /* Retrieve sensor settings to be used in this time instant by calling bsec_sensor_control */
                    bsec_ret_sens_ctrl = bsec_sensor_control(time_stamp, &sensor_settings);

                    /* Trigger a measurement if necessary */
                    bme68x_status = bme68x_bsec_trigger_measurement(&sensor_settings);
#ifdef BSEC_LIB_INTEG_LOG
                    LOG(I, "trig_measu bme68x_status = %d sens_ctrl_status %d ", bme68x_status, bsec_ret_sens_ctrl);
#endif
                    if ( BSEC_OK == bsec_ret_sens_ctrl )
                    {
                        /* Read data from last measurement */
                        num_bsec_inputs = 0u;
                        bme68x_status = bme68x_bsec_read_data(time_stamp, bsec_inputs, &num_bsec_inputs, sensor_settings.process_data);

                        if ( BME68X_OK == bme68x_status )
                        {
                            /* Time to invoke BSEC to perform the actual processing */
                            bsec_ret_proc_data = bme68x_bsec_process_data(bsec_inputs, num_bsec_inputs);

                            /* Compute how long we can sleep until we need to call bsec_sensor_control() next */
                            /* Time_stamp is converted from microseconds to nanoseconds first and then the difference to milliseconds */
                            time_stamp_interval_ms = (sensor_settings.next_call - bsec_lib_intf.get_time_stamp_us() * 1000) / 1000000;

                            if ( time_stamp_interval_ms > TIME_OFFSET_INTERVAL_MS )
                            {
                                // To avoid the timing violation due schedueler time constraints
                                sleep_interval = (uint32_t)time_stamp_interval_ms - TIME_OFFSET_INTERVAL_MS;
                            }

#if ENABLE_EXCUTION_TIME_DEBUG
                            last_tick = xTaskGetTickCount();
#endif
                            bme68x_curr_err_stat = BME68X_NO_ERROR;
                        }
                        else
                        {
                            LOG(I,"[BME68x read failed]");
                        }
                    }
                }

				if ( ( BME68X_OK != bme68x_status ) || ( BSEC_OK != bsec_ret_sens_ctrl ) || (1 == interval_err) )
				{
					if(1 == interval_err)				/* Increment error retry counter */
                    {
                        bme68x_error_retry_count = BME68X_BSEC_MAX_RETRY_COUNT;
                        interval_err = 0;
                    }
                    else
                    {
					    bme68x_error_retry_count++;
                    }
		            LOG(E, "BSEC error = %d bme68x_status = %d error_count %d", bsec_ret_sens_ctrl, bme68x_status, bme68x_error_retry_count);
					bsec_critical_error =1;
                    if ( bme68x_error_retry_count >= BME68X_BSEC_MAX_RETRY_COUNT )
					{
						bme68x_error_retry_count = 0u;
						/* Max Retry reached ..Report error*/
                        bme68x_curr_err_stat = BME68X_DATA_PLAUSIBLE_ERROR;
                        bsec_proc_st = STATE_ERROR;
					}
				}
                break;

            case STATE_SLEEP:
                bsec_ret_state_save = bsec_get_state(0, bsec_state, sizeof(bsec_state), work_buffer, sizeof(work_buffer), &bsec_state_len);

				if ( bsec_ret_state_save == BSEC_OK )
				{
                    LOG(W, "Write to NVS");
					bsec_lib_intf.state_save(bsec_state, bsec_state_len);
				}

                /* Change the power mode to sleep */
                bme68x_status = bme68x_set_op_mode(BME68X_SLEEP_MODE, &bme68x_g);

                if ( BME68X_OK != bme68x_status )
                {
                    LOG(E, "bme68x_status error = %d", bme68x_status);
                    bme68x_curr_err_stat = BME68X_DATA_PLAUSIBLE_ERROR;
                    bsec_proc_st = STATE_ERROR;
                }
                else
                {
                    bme68x_curr_err_stat = BME68X_NO_ERROR;
                    bsec_proc_st = STATE_IDLE;
                }

				/* Reset the data */
				bsec_out.iaq.data             	   = 0;
				bsec_out.static_iaq.data           = 0;
				bsec_out.co2_equivalent.data  	   = 0;
				bsec_out.raw_temperature.data 	   = 0;
				bsec_out.raw_pressure.data    	   = 0;
				bsec_out.raw_humidity.data    	   = 0;
				bsec_out.raw_gas.data              = 0;
				bsec_out.stabilization_status.data = 0;
				bsec_out.run_in_status.data        = 0;
				bsec_out.breath_voc_eq.data        = 0;
                bsec_out.iaq.accuracy              = 0;

                bsec_lib_intf.output_ready(&bsec_out);/* Update the reset value to broker */
                //update_voc_sens_to_broker(&bsec_out);

                /* Set the sleep interval for IDLE state */
                sleep_interval = BSEC_IDLE_STATE_SLEEP_INTERVAL;
                break;

            case STATE_IDLE:
                if ( BME68X_FORCED_MODE == op_mode )
                {
                    /* Initialize BSEC library */
                    //bsec_ret_sens_ctrl = bsec_reinit();

                    return_values_init ret_val_sens_ctrl  = initialize_bsec_lib();
                    bsec_ret_sens_ctrl = ret_val_sens_ctrl.bme68x_status;
                    LOG(I,"Bsec reinit %d ", bsec_ret_sens_ctrl);
                    LOG(I,"init_flag_idle_before  %d ", bme68x_init_flag);
                    if ( BSEC_OK != bsec_ret_sens_ctrl )
                    {
                        bme68x_curr_err_stat = BME68X_COMM_ERROR;
                        bsec_proc_st = STATE_ERROR;
                    }
                    else
                    {
                        bme68x_curr_err_stat = BME68X_NO_ERROR;
                        bsec_proc_st = STATE_BSEC_PROCESS;
                        bme68x_init_flag = 0;
                        LOG(I,"init_flag_idle %d ", bme68x_init_flag);
                    }
                }
                else
                {
                    LOG(I," State idle BM68x error mode %d ", op_mode);
                    return_values_init ret_val_sens_ctrl  = initialize_bsec_lib();
                    bsec_ret_sens_ctrl = ret_val_sens_ctrl.bme68x_status;
                    LOG(I,"Idle_err_Bsec reinit %d ", bsec_ret_sens_ctrl);
                    LOG(I,"Idle_err_init_flag_idle_before  %d ", bme68x_init_flag);
                    if ( BSEC_OK != bsec_ret_sens_ctrl )
                    {
                        bme68x_curr_err_stat = BME68X_COMM_ERROR;
                        bsec_proc_st = STATE_ERROR;
                    }
                    else
                    {
                        bme68x_curr_err_stat = BME68X_NO_ERROR;
                        bsec_proc_st = STATE_BSEC_PROCESS;
                        bme68x_init_flag = 0;
                    }
                }
                break;

            case STATE_WAIT:
                /* This wait time is given to avoid NVS access during other components active and excecute from IRAM */
                sleep_interval = BME_BSEC_WAIT_TIME_BEFORE_SLEEP_MS;
                bsec_proc_st   = STATE_SLEEP;
                break;

            case STATE_ERROR:
                if ( bme68x_curr_err_stat != bme68x_prev_err_stat )
                {
#ifdef BSEC_LIB_INTEG_LOG
                    LOG(E, "bsec bme68x error");
#endif
                    bme68x_prev_err_stat = bme68x_curr_err_stat;
                    bme68x_error_code(bme68x_curr_err_stat);
                }
                op_mode        = BME68X_FORCED_MODE;
                sleep_interval = BSEC_IDLE_STATE_SLEEP_INTERVAL;
                bsec_proc_st   = STATE_IDLE;
                break;

            default:
				break;
        };

        if ((bme68x_curr_err_stat == BME68X_NO_ERROR) && (bme68x_prev_err_stat != BME68X_NO_ERROR) 
        && (bsec_proc_st != STATE_ERROR) && (bsec_proc_st != STATE_WAIT))
        {
            bme68x_prev_err_stat = bme68x_curr_err_stat;
            bme68x_error_code(bme68x_curr_err_stat);
        }

        osal_task_delay(sleep_interval);
    }
}

//#endif // CONFIG_DICM_SUPPORT_INTEGRATED_BSEC_LIB_2_X
/**
  * @brief  Function to get BME68x initialization status
  * @param  void.
  * @retval BME68x init status
  */
int8_t get_bmestatus(void)
{
    return bme68x_stat;
}

#endif // BSEC_VERSION_1_X

/*! @}*/

