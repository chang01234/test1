/*!
 * \file hmi_sensor.h
 * \brief API for managing and initializing which HMI onboard sensors
 *
 *  Created on: 18 june 2023
 *      Author: Anatolie Pocropivnii
 */

#ifndef HMI_SENSOR_H_
#define HMI_SENSOR_H_

#include <stdint.h>
#include <math.h>

#define CELSIUS_FAHRENHEIT_OFFSET                                  (32.0f)
#define CELSIUS_FAHRENHEIT_RATIO                                   (1.8f)

#define CONVERT_CELSIUS_TO_FAHRENHEIT(value, scale_factor)         ((int32_t)round(((value) * (scale_factor) * CELSIUS_FAHRENHEIT_RATIO) + CELSIUS_FAHRENHEIT_OFFSET))
#define CONVERT_FAHRENHEIT_TO_CELSIUS(value, scale_factor)         ((int32_t)round((((value) - CELSIUS_FAHRENHEIT_OFFSET) / CELSIUS_FAHRENHEIT_RATIO) / (scale_factor)))

void hmi_sensor_init(void);
bool get_hmi_temperature_celsius(int32_t * ddm_temperature);

#endif /* HMI_SENSOR_H_ */