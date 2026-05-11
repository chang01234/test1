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

#if ( CONFIG_DICM_SUPPORT_INTEGRATED_BSEC_LIB_1_X == 1 )

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bsec_integration.h"
#include "esp_attr.h"
#include "bsec_serialized_configurations_iaq.h"
#include "hal_system.h"

/**********************************************************************************************************************/
/* local macro definitions */
/**********************************************************************************************************************/
#define VOC_SENSOR_STATUS_INIT_FAILED     ((uint8_t)  1u)
#define VOC_SENSOR_STATUS_INIT_SUCCESS    ((uint8_t)  0u)

#define NUM_USED_OUTPUTS                      ((uint8_t)    10u)
#define BME680_FORCED_MODE_SLEEP_TIME_MS      ((uint32_t)    5u)   // 5 msec
#define BME680_FORCED_MODE_MAX_WAIT_TIME_MS   ((uint32_t) 1000u)   // 1 sec
#define BME680_FORCED_MODE_ERROR              ((int8_t)       1)
#define BME680_BSEC_MAX_RETRY_COUNT           ((uint32_t)    5u)
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

//#define BSEC_LIB_INTEG_LOG                //Uncomment to Enable BME68x LOG
#define ENABLE_EXCUTION_TIME_DEBUG  1
extern void bme68x_error_code(const BME68X_ERROR error);
/**********************************************************************************************************************/
/* global variable declarations */
/**********************************************************************************************************************/
extern const BME680_BSEC_LIB_INTERFACE g_bsec_lib_intf;

// FOR EOL testing
static int8_t bme68x_stat = 0;

static BME680_BSEC_LIB_INTERFACE bsec_lib_intf;

/* Global sensor APIs data structure */
static struct bme680_dev bme680_g;

/* Global temperature offset to be subtracted */
static float bme680_temperature_offset_g = 0.0f;

/* Save state variables */
EXT_RAM_ATTR uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
EXT_RAM_ATTR uint8_t bsec_config[BSEC_MAX_PROPERTY_BLOB_SIZE] = {0};
EXT_RAM_ATTR uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0}; 

/* Variable for error count */
static uint32_t bme680_error_retry_count = 0u;

/* Instance to store the read data from BME680 */
EXT_RAM_ATTR bme680_bsec_output bsec_data_output;

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
static bme680_bsec_output bsec_out;
static bsec_library_return_t bsec_ret_sens_ctrl;
static bsec_library_return_t bsec_ret_proc_data;
static bsec_library_return_t bsec_ret_state_save;
static int8_t bme680_status = BME680_OK;
static uint8_t num_bsec_inputs = 0u;    /* Number of inputs to BSEC */
static uint32_t bsec_state_len = 0u;

#ifdef CONNECTOR_EOL_SERVICE
static uint32_t op_mode = BME680_FORCED_MODE;
#else
static uint32_t op_mode = 0xFF;
#endif

static uint8_t err = 0;
static TickType_t last_tick = 0;
static TickType_t diff_tick = 0;
static uint32_t bsec_config_len = 0;
static BME68X_ERROR bme68x_curr_err_stat = BME68X_NO_ERROR;
static BME68X_ERROR bme68x_prev_err_stat = BME68X_NO_ERROR;

/**********************************************************************************************************************/
/* functions */
/**********************************************************************************************************************/

/*!
 * @brief        Virtual sensor subscription
 *               Please call this function before processing of data using bsec_do_steps function
 *
 * @param[in]    sample_rate         mode to be used (either BSEC_SAMPLE_RATE_ULP or BSEC_SAMPLE_RATE_LP)
 *  
 * @return       subscription result, zero when successful
 */
static bsec_library_return_t bme680_bsec_update_subscription(float sample_rate)
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
 * @brief       Initialize the BME680 sensor and the BSEC library
 *
 * @return      zero if successful, negative otherwise
 */
//return_values_init initialize_bsec_lib(BME680_BSEC_LIB_INTERFACE* bsec_intf)
return_values_init bsec_iot_init(void)
{
    return_values_init ret = {BME680_OK, BSEC_OK};

    /* Fixed I2C configuration */
    bme680_g.dev_id = 0;
    bme680_g.intf = BME680_I2C_INTF;
    /* User configurable I2C configuration */
    bme680_g.write = bsec_lib_intf.bus_write;
    bme680_g.read = bsec_lib_intf.bus_read;
    bme680_g.delay_ms = bsec_lib_intf.delay_ms;
    
    /* Initialize BME680 API */
    ret.bme680_status = bme680_init(&bme680_g);
    if ( ret.bme680_status != BME680_OK )
    {
        return ret;
    }
    
    /* Initialize BSEC library */
    ret.bsec_status = bsec_init();
    if ( ret.bsec_status != BSEC_OK )
    {
        return ret;
    }
    
    /* Load library config, if available */
    bsec_config_len = bsec_lib_intf.config_load(bsec_config, sizeof(bsec_config));
    if ( bsec_config_len != 0 )
    {       
        ret.bsec_status = bsec_set_configuration(bsec_config, bsec_config_len, work_buffer, sizeof(work_buffer));     
        if ( ret.bsec_status != BSEC_OK )
        {
            return ret;
        }
    }
    
    /* Load previous library state, if available */
    bsec_state_len = bsec_lib_intf.state_load(bsec_state, sizeof(bsec_state));
    if ( bsec_state_len != 0 )
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
    bme680_temperature_offset_g = bsec_lib_intf.temperature_offset;
    
    /* Call to the function which sets the library with subscription information */
    ret.bsec_status = bme680_bsec_update_subscription(bsec_lib_intf.sample_rate);
    if ( ret.bsec_status != BSEC_OK )
    {
        return ret;
    }
     
    return ret;
}

/*!
 * @brief       Trigger the measurement based on sensor settings
 *
 * @param[in]   sensor_settings     settings of the BME680 sensor adopted by sensor control function
 * @param[in]   sleep               pointer to the system specific sleep function
 *
 * @return      bme680_status       bme680 status
 */
static int8_t bme680_bsec_trigger_measurement(bsec_bme_settings_t *sensor_settings, sleep_fct sleep)
{
    uint16_t meas_period;
    uint8_t set_required_settings;
    volatile int8_t bme680_status = BME680_OK;
    volatile uint32_t sleep_time = 0u;
        
    /* Check if a forced-mode measurement should be triggered now */
    if ( sensor_settings->trigger_measurement )
    {
        /* Set sensor configuration */
#ifdef BSEC_LIB_INTEG_LOG		
		LOG(I, "heater_temperature = %d ", sensor_settings->heater_temperature);
		LOG(I, "heating_duration = %d ", sensor_settings->heating_duration);
		LOG(I, "humidity_oversampling = %d ", sensor_settings->humidity_oversampling);
		LOG(I, "next_call = %llx ns ", sensor_settings->next_call);
		LOG(I, "pressure_oversampling = %d ", sensor_settings->pressure_oversampling);
		LOG(I, "process_data = %d ", sensor_settings->process_data);
		LOG(I, "run_gas = %d ", sensor_settings->run_gas);
		LOG(I, "temperature_oversampling = %d ", sensor_settings->temperature_oversampling);
		LOG(I, "trigger_measurement = %d ", sensor_settings->trigger_measurement);
#endif 

        bme680_g.tph_sett.os_hum     = sensor_settings->humidity_oversampling;
        bme680_g.tph_sett.os_pres    = sensor_settings->pressure_oversampling;
        bme680_g.tph_sett.os_temp    = sensor_settings->temperature_oversampling;
        bme680_g.gas_sett.run_gas    = sensor_settings->run_gas;
        bme680_g.gas_sett.heatr_temp = sensor_settings->heater_temperature; /* degree Celsius */
        bme680_g.gas_sett.heatr_dur  = sensor_settings->heating_duration; /* milliseconds */
        
        /* Select the power mode */
        /* Must be set before writing the sensor configuration */
        bme680_g.power_mode = BME680_FORCED_MODE;
        /* Set the required sensor settings needed */
        set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_GAS_SENSOR_SEL;
        
        /* Set the desired sensor configuration */
        bme680_status = bme680_set_sensor_settings(set_required_settings, &bme680_g);

        if ( BME680_OK == bme680_status )
        {
            /* Set power mode as forced mode and trigger forced mode measurement */
            bme680_status = bme680_set_sensor_mode(&bme680_g);

            if ( BME680_OK == bme680_status )
            {
                /* Get the total measurement duration so as to sleep or wait till the measurement is complete */
                bme680_get_profile_dur(&meas_period, &bme680_g);

                /* Delay till the measurement is ready. Timestamp resolution in ms */
                sleep((uint32_t)meas_period);
            }
        }
    }

#ifdef BSEC_LIB_INTEG_LOG
    LOG(I, "Waiting for BME680 transistion to Sleep Mode | current power_mode = %d", bme680_g.power_mode);
#endif

    if ( BME680_OK == bme680_status )
    {
        /* Call the API to get current operation mode of the sensor */
        bme680_status = bme680_get_sensor_mode(&bme680_g);  
        /* When the measurement is completed and data is ready for reading, the sensor must be in BME680_SLEEP_MODE.
        * Read operation mode to check whether measurement is completely done and wait until the sensor is no more
        * in BME680_FORCED_MODE. */
        while ( ( bme680_g.power_mode == BME680_FORCED_MODE ) && ( BME680_OK == bme680_status ) )
        {
            /* sleep for 5 ms */
            sleep(BME680_FORCED_MODE_SLEEP_TIME_MS);
            bme680_status = bme680_get_sensor_mode(&bme680_g);
            sleep_time += BME680_FORCED_MODE_SLEEP_TIME_MS;

            // Added by LTTS..To avoid any dead lock scenario
            if ( sleep_time >= BME680_FORCED_MODE_MAX_WAIT_TIME_MS )
            {
                bme680_status = BME680_FORCED_MODE_ERROR;
                break;
            }
        }        
    }

#ifdef BSEC_LIB_INTEG_LOG
    LOG(I, "Exit from forced mode | current power_mode = %d", bme680_g.power_mode);
#endif

    return bme680_status;
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
static int8_t bme680_bsec_read_data(int64_t time_stamp_trigger, bsec_input_t *inputs, uint8_t *num_bsec_inputs,
    int32_t bsec_process_data)
{
    static struct bme680_field_data data;
    int8_t bme680_status = BME680_OK;
    
    /* We only have to read data if the previous call the bsec_sensor_control() actually asked for it */
    if ( bsec_process_data )
    {
        bme680_status = bme680_get_sensor_data(&data, &bme680_g);
        if(bme680_status == BME68X_E_COM_FAIL)
        {
            LOG(E,"BME68x Communication failed");
        }

        if ( BME680_OK == bme680_status )
        {
            if (data.status & BME680_NEW_DATA_MSK)
            {
                /* Pressure to be processed by BSEC */
                if (bsec_process_data & BSEC_PROCESS_PRESSURE)
                {
                    /* Place presssure sample into input struct */
                    inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_PRESSURE;
                    inputs[*num_bsec_inputs].signal = data.pressure;
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                    #ifdef BSEC_LIB_INTEG_LOG
                        LOG(I, "Raw pressure data = %d", data.pressure);
                    #endif
                }
                /* Temperature to be processed by BSEC */
                if (bsec_process_data & BSEC_PROCESS_TEMPERATURE)
                {
                    /* Place temperature sample into input struct */
                    inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
                    #ifdef BME680_FLOAT_POINT_COMPENSATION
                        inputs[*num_bsec_inputs].signal = data.temperature;
                    #else
                        inputs[*num_bsec_inputs].signal = data.temperature / 100.0f;
                    #endif
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                    
                    /* Also add optional heatsource input which will be subtracted from the temperature reading to 
                    * compensate for device-specific self-heating (supported in BSEC IAQ solution)*/
                    inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_HEATSOURCE;
                    inputs[*num_bsec_inputs].signal = bme680_temperature_offset_g;
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                    #ifdef BSEC_LIB_INTEG_LOG
                        LOG(I, "Raw temperature data = %f", (data.temperature / 100));
                    #endif
                }
                /* Humidity to be processed by BSEC */
                if (bsec_process_data & BSEC_PROCESS_HUMIDITY)
                {
                    /* Place humidity sample into input struct */
                    inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
                    #ifdef BME680_FLOAT_POINT_COMPENSATION
                        inputs[*num_bsec_inputs].signal = data.humidity;
                    #else
                        inputs[*num_bsec_inputs].signal = data.humidity / 1000.0f;
                    #endif  
                    inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                    (*num_bsec_inputs)++;
                    #ifdef BSEC_LIB_INTEG_LOG
                        LOG(I, "Raw humdity data = %f", data.humidity / 1000);
                    #endif
                }
                /* Gas to be processed by BSEC */
                if (bsec_process_data & BSEC_PROCESS_GAS)
                {
                    /* Check whether gas_valid flag is set */
                    if(data.status & BME680_GASM_VALID_MSK)
                    {
                        /* Place sample into input struct */
                        inputs[*num_bsec_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
                        inputs[*num_bsec_inputs].signal = data.gas_resistance;
                        inputs[*num_bsec_inputs].time_stamp = time_stamp_trigger;
                        (*num_bsec_inputs)++;
                        #ifdef BSEC_LIB_INTEG_LOG
                            LOG(I, "Raw Gas Resistance data = %f", data.gas_resistance);
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
static bsec_library_return_t bme680_bsec_process_data(bsec_input_t *bsec_inputs, uint8_t num_bsec_inputs, output_ready_fct output_ready)
{
    uint8_t num_bsec_outputs = 0;
    uint8_t index = 0;
    bsec_library_return_t bsec_status = BSEC_OK;

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
            for (index = 0; index < num_bsec_outputs; index++)
            {
                switch (bsec_outputs[index].sensor_id)
                {
                    case BSEC_OUTPUT_IAQ:
                        bsec_data_output.iaq.data                           = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.iaq.accuracy                       = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_STATIC_IAQ:
                        bsec_data_output.static_iaq.data                    = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.static_iaq.accuracy                = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_CO2_EQUIVALENT:
                        bsec_data_output.co2_equivalent.data                = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.co2_equivalent.accuracy            = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                        bsec_data_output.breath_voc_eq.data                 = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.breath_voc_eq.accuracy             = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                        bsec_data_output.sensor_heat_comp_temp.data         = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.sensor_heat_comp_temp.accuracy     = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_PRESSURE:
                        bsec_data_output.raw_pressure.data                  = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_pressure.accuracy              = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                        bsec_data_output.sensor_heat_comp_humidity.data     = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.sensor_heat_comp_humidity.accuracy = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_GAS:
                        bsec_data_output.raw_gas.data                       = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_gas.accuracy                   = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_TEMPERATURE:
                        bsec_data_output.raw_temperature.data               = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_temperature.accuracy           = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RAW_HUMIDITY:
                        bsec_data_output.raw_humidity.data                  = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.raw_humidity.accuracy              = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_COMPENSATED_GAS:
                        bsec_data_output.compensated_gas.data               = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.compensated_gas.accuracy           = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_GAS_PERCENTAGE:
                        bsec_data_output.gas_percentage.data                = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.gas_percentage.accuracy            = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_STABILIZATION_STATUS:
                        bsec_data_output.stabilization_status.data          = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.stabilization_status.accuracy      = bsec_outputs[index].accuracy;
                        break;
                    case BSEC_OUTPUT_RUN_IN_STATUS:
                        bsec_data_output.run_in_status.data                 = (bme680_op_data_type)bsec_outputs[index].signal;
                        bsec_data_output.run_in_status.accuracy             = bsec_outputs[index].accuracy;
                        break;
                    default:
                        continue;
                }
                
                /* Assume that all the returned timestamps are the same */
                bsec_data_output.time_stamp = bsec_outputs[index].time_stamp;
            }

            /* Pass the extracted outputs to the user provided output_ready() function. */
            output_ready(&bsec_data_output);
        }
        else
        {
            LOG(E,"bsec_do_steps error %d", bsec_status);
        }
    }

    return bsec_status;
}

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
    bme680_temperature_offset_g = BME680_TEMPERATURE_OFFSET;
    
    /* Call to the function which sets the library with subscription information */
    bsec_ret_sens_ctrl |= bme680_bsec_update_subscription(BSEC_SAMPLE_RATE_LP);

    return bsec_ret_sens_ctrl;
}

/*!
 * @brief       Runs the main (endless) loop that queries sensor settings, applies them, and processes the measured data
 *
 * @param[in]   queue               queue to the system specific
 *
 * @return      none
 */
void bsec_processing_loop(osal_queue_handle_t queue)
{
    while (1)
    {
        if ( pdPASS == xQueueReceive(queue, (void *)&op_mode, 0) )
        {
            LOG(I, "BME680 Operating Mode = %d", op_mode);

            if ( BME680_SLEEP_MODE == op_mode )
            {
                bsec_proc_st = STATE_WAIT;
            }
        }

        switch (bsec_proc_st)
        {
            case STATE_INIT:
                if ( BME680_FORCED_MODE == op_mode )
                {
                    /* Initialize the BME680 sensor */
                    return_values_init ret_val = bsec_iot_init();
                    bme68x_stat = ret_val.bme680_status;
                    if ( ( BSEC_OK == ret_val.bsec_status ) && ( BME680_OK == ret_val.bme680_status ) )
                    {
                        LOG(I, "BME680 Init success");
                        err = VOC_SENSOR_STATUS_INIT_SUCCESS;
                    }
                    else
                    {
                        LOG(E, "BME680 Init failed : bme680_status =  %d, bsec_status = %d ", ret_val.bme680_status, ret_val.bsec_status);
                        err = VOC_SENSOR_STATUS_INIT_FAILED;
                    }
                    if ( VOC_SENSOR_STATUS_INIT_SUCCESS != err )
                    {
                        bme68x_curr_err_stat = BME68X_COMM_ERROR;
                        bsec_proc_st = STATE_ERROR;
                    }
                    else
                    {
                        bme68x_curr_err_stat = BME68X_NO_ERROR;
                        bsec_proc_st = STATE_BSEC_PROCESS;
                    }
                }
                break;

            case STATE_BSEC_PROCESS:
#if ENABLE_EXCUTION_TIME_DEBUG
                diff_tick = xTaskGetTickCount() - last_tick;
                LOG(I,"%" PRId64 "", time_stamp_interval_ms);
                LOG(I, "BSEC execution interval %d ms | configured %d ms", pdTICKS_TO_MS(diff_tick), sleep_interval);
#endif
                /* get the timestamp in nanoseconds before calling bsec_sensor_control() */
				time_stamp = bsec_lib_intf.get_time_stamp_us() * 1000;
        
				/* Retrieve sensor settings to be used in this time instant by calling bsec_sensor_control */
				bsec_ret_sens_ctrl = bsec_sensor_control(time_stamp, &sensor_settings);
				
				/* Trigger a measurement if necessary */
				bme680_status = bme680_bsec_trigger_measurement(&sensor_settings, bsec_lib_intf.delay_ms);
					
				if ( ( BME680_OK == bme680_status ) && ( BSEC_OK == bsec_ret_sens_ctrl ) )
				{
					/* Read data from last measurement */
					num_bsec_inputs = 0u;
					bme680_status = bme680_bsec_read_data(time_stamp, bsec_inputs, &num_bsec_inputs, sensor_settings.process_data);
		
					if ( BME680_OK == bme680_status )
					{
						/* Time to invoke BSEC to perform the actual processing */
						bsec_ret_proc_data = bme680_bsec_process_data(bsec_inputs, num_bsec_inputs, bsec_lib_intf.output_ready);
		
						/* Compute how long we can sleep until we need to call bsec_sensor_control() next */
						/* Time_stamp is converted from microseconds to nanoseconds first and then the difference to milliseconds */
						time_stamp_interval_ms = (sensor_settings.next_call - bsec_lib_intf.get_time_stamp_us() * 1000) / 1000000;
						
                        if ( time_stamp_interval_ms > TIME_OFFSET_INTERVAL_MS )
                        {
                            // To avoid the timing viloation due schedueler time constraints
						    sleep_interval = (uint32_t)time_stamp_interval_ms - TIME_OFFSET_INTERVAL_MS; 
                        }

#if ENABLE_EXCUTION_TIME_DEBUG
                        last_tick = xTaskGetTickCount();
#endif
					}
				}
		
				if ( ( BME680_OK != bme680_status ) || ( BSEC_OK != bsec_ret_sens_ctrl ) )
				{
					LOG(E, "BSEC error = %d bme680_status = %d ", bsec_ret_sens_ctrl, bme680_status);
					/* Increment error retry counter */
					bme680_error_retry_count++;
		
					if ( bme680_error_retry_count >= BME680_BSEC_MAX_RETRY_COUNT )
					{
                        bme68x_curr_err_stat = BME68X_DATA_PLAUSIBLE_ERROR;
						bme680_error_retry_count = 0u;
						/* Max Retry reached ..Report error*/
                        bsec_proc_st = STATE_ERROR;
					}
				}
                else
                {
                    bme68x_curr_err_stat = BME68X_NO_ERROR;
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
                bme680_g.power_mode = BME680_SLEEP_MODE;
                /* Set power mode as forced mode and trigger forced mode measurement */
                bme680_status = bme680_set_sensor_mode(&bme680_g);

                if ( BME680_OK != bme680_status )
                {
                    LOG(I, "bme680_status error = %d", bme680_status);
                    bme68x_curr_err_stat = BME68X_DATA_PLAUSIBLE_ERROR;
                    bsec_proc_st = STATE_ERROR;
                }
                else
                {
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
                /* Update the reset value to broker */
                //update_voc_sens_to_broker(&bsec_out);
                bsec_lib_intf.output_ready(&bsec_out);/* Update the reset value to broker */
                /* Set the sleep interval for IDLE state */
                sleep_interval = BSEC_IDLE_STATE_SLEEP_INTERVAL;
                break;

            case STATE_IDLE:
                if ( BME680_FORCED_MODE == op_mode )
                {
                    /* Initialize BSEC library */
                    bsec_ret_sens_ctrl = bsec_reinit();

                    if ( BSEC_OK != bsec_ret_sens_ctrl )
                    {
                        bme68x_curr_err_stat = BME68X_COMM_ERROR;
                        bsec_proc_st = STATE_ERROR;
                    }
                    else
                    {
                        bsec_proc_st = STATE_BSEC_PROCESS;
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
                    LOG(I, "bsec bme680 error");
                    bme68x_prev_err_stat = bme68x_curr_err_stat;
                    bme68x_error_code(bme68x_curr_err_stat);
                }
                op_mode        = BME680_FORCED_MODE;
                sleep_interval = BSEC_IDLE_STATE_SLEEP_INTERVAL;
                bsec_proc_st   = STATE_IDLE;
                break;
			
            default:
				break;
        };
        
        bsec_lib_intf.delay_ms(sleep_interval);
    }
}
/*!
 * @brief        Function to init BSEC library interface
 *
 * @param[in]    bsec_intf pointer to the BME6X_BSEC_LIB_INTERFACE
 *  
 * @return       void
 */
void init_bsec_lib_interface(void)
{
    bsec_lib_intf.bus_read           = g_bsec_lib_intf.bus_read;
    bsec_lib_intf.bus_write          = g_bsec_lib_intf.bus_write;
    bsec_lib_intf.delay_ms           = g_bsec_lib_intf.delay_ms;
    bsec_lib_intf.config_load        = g_bsec_lib_intf.config_load;
    bsec_lib_intf.state_load         = g_bsec_lib_intf.state_load;
    bsec_lib_intf.state_save         = g_bsec_lib_intf.state_save;
    bsec_lib_intf.temperature_offset = g_bsec_lib_intf.temperature_offset;
    bsec_lib_intf.get_time_stamp_us  = g_bsec_lib_intf.get_time_stamp_us;
    bsec_lib_intf.output_ready       = g_bsec_lib_intf.output_ready;
    bsec_lib_intf.sample_rate        = g_bsec_lib_intf.sample_rate;
}

/**
  * @brief  Function to get BME68x initialization status 
  * @param  void.
  * @retval BME68x init status
  */
int8_t get_bmestatus(void)
{
    return bme68x_stat;
}

#endif  // CONFIG_DICM_SUPPORT_INTEGRATED_BSEC_LIB_1_X

/*! @}*/

