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
 * @file bsec_integration.h
 *
 * @brief
 * Contains BSEC integration API
 */

/*!
 * @addtogroup bsec_examples BSEC Examples
 * @brief BSEC usage examples
 * @{*/

#ifndef __BSEC_INTEGRATION_2_X_H__
#define __BSEC_INTEGRATION_2_X_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**********************************************************************************************************************/
/* header files */
/**********************************************************************************************************************/
#include "osal.h"

/* Use the following bme68x driver: https://github.com/BoschSensortec/BME680_driver/releases/tag/bme680_v3.5.1 */
#include "bme68x.h"
#include "bme68x_defs.h"
/* BSEC header files are available in the inc/ folder of the release package */
#include "bsec_interface.h"
#include "bsec_datatypes.h"

#ifndef BME68X_USE_FPU
typedef uint32_t bme68x_op_data_type;
#else
typedef float    bme68x_op_data_type;
#endif

/**********************************************************************************************************************/
/* type definitions */
/**********************************************************************************************************************/

typedef enum __bme68x_err
{
    BME68X_NO_ERROR             = 0,
    BME68X_COMM_ERROR           = 1,
    BME68X_DATA_PLAUSIBLE_ERROR = 2
}BME68X_ERROR;

typedef enum _bme6x_accuracy
{
	BME6X_STABILIZATION_ONGOING = 0,
	BME6X_LOW_ACCURACY          = 1,
	BME6X_MEDIUM_ACCURACY       = 2,
	BME6X_HIGH_ACCURACY         = 3,
} BME6X_ACCURACY;

typedef struct _bme68x_param_op_data
{
    uint8_t              accuracy;
    bme68x_op_data_type      data;
}bme68x_param_op_data;

typedef struct _bme68x_bsec_output
{
    bme68x_param_op_data iaq;
    bme68x_param_op_data static_iaq;
    bme68x_param_op_data co2_equivalent;
	bme68x_param_op_data raw_temperature;
	bme68x_param_op_data raw_pressure;
	bme68x_param_op_data raw_humidity;
	bme68x_param_op_data raw_gas; 
	bme68x_param_op_data stabilization_status;
	bme68x_param_op_data run_in_status;
	bme68x_param_op_data sensor_heat_comp_temp;
	bme68x_param_op_data sensor_heat_comp_humidity;
	bme68x_param_op_data compensated_gas;
	bme68x_param_op_data gas_percentage;
    bme68x_param_op_data breath_voc_eq;
	int64_t              time_stamp;
}bme68x_bsec_output;

/* function pointer to the system specific timestamp derivation function */
typedef int64_t (*get_timestamp_us_fct)();

/* function pointer to the function processing obtained BSEC outputs */
typedef void (*output_ready_fct)(bme68x_bsec_output *bsec_out);

/* function pointer to the function loading a previous BSEC state from NVM */
typedef uint32_t (*state_load_fct)(uint8_t *state_buffer, uint32_t n_buffer);
 
/* function pointer to the function saving BSEC state to NVM */
typedef void (*state_save_fct)(const uint8_t *state_buffer, uint32_t length);

/* function pointer to the function loading the BSEC configuration string from NVM */
typedef uint32_t (*config_load_fct)(uint8_t *state_buffer, uint32_t n_buffer);

/* Structure for interfacing BSEC lib */
typedef struct __bme6x_bsec_lib_interface
{
    float                  sample_rate;          // sample rate ( BSEC_SAMPLE_RATE_LP / BSEC_SAMPLE_RATE_ULP )
    float                  temperature_offset;   // temperature offset vaue
    bme68x_write_fptr_t    bus_write;            // pointer to the bus writing function
    bme68x_read_fptr_t     bus_read;             // pointer to the bus reading function
    bme68x_delay_us_fptr_t delay_us;             // pointer to the system specific sleep function
    state_load_fct         state_load;           // pointer to the system-specific state load function
    config_load_fct        config_load;          // pointer to the system-specific config load function
    state_save_fct         state_save;           // pointer to the system-specific state save function
    get_timestamp_us_fct   get_time_stamp_us;    // pointer to get time stamp in microseconds
    output_ready_fct       output_ready;         // pointer to output ready callback function
}BME6X_BSEC_LIB_INTERFACE;
    
/* structure definitions */

/* Structure with the return value from bsec_iot_init() */
typedef struct{
	/*! Result of API execution status */
	int8_t bme68x_status;
	/*! Result of BSEC library */
	bsec_library_return_t bsec_status;
}return_values_init;
/**********************************************************************************************************************/
/* function declarations */
/**********************************************************************************************************************/

/*!
 * @brief       Initialize the BME688 sensor and the BSEC library
 *
 * @return      return_values_init
 */
return_values_init initialize_bsec_lib(void);

/*!
 * @brief       Runs the main (endless) loop that queries sensor settings, applies them, and processes the measured data
 *
 * @param[in]   void
 *
 * @return      void
 */ 
void bsec_processing_loop(osal_queue_handle_t queue);

/*!
 * @brief       Init the BSCE linrary interface
 *
 * @return      void
 */
void init_bsec_lib_interface(void);

int8_t get_bmestatus(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSEC_INTEGRATION_2_X_H__ */

/*! @}*/

