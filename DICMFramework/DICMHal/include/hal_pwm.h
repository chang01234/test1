/*! \file hal_pwm.h
	\brief PWM module Hardware Abstraction Layer
*/

#ifndef HAL_PWM_H_
#define HAL_PWM_H_

#include <stddef.h>
#include <stdint.h>

#define HAL_PWM_BIT(n)	(1UL << (n))

/**
 * @brief Capture type in HAL.
 */
typedef enum hal_pwm_capture_edge_type
{
	HAL_PWM_CAP_NEG_EDGE = HAL_PWM_BIT(0),
	HAL_PWM_CAP_POS_EDGE = HAL_PWM_BIT(1),
	HAL_PWM_CAP_BOTH_EDGE = (HAL_PWM_CAP_NEG_EDGE | HAL_PWM_CAP_POS_EDGE),
} hal_pwm_capture_edge_type_t;

typedef bool (*hal_pwm_cap_isr_cb_t)(uint8_t unit, uint8_t capture_signal, uint32_t value);

#define DEVICE_PWM_FREQUENCY 	(25000U)				            //!< PWM Freq 25KHz.

void hal_pwm_init_gpio(uint8_t unit);

void hal_pwm_init(uint8_t unit);

void hal_pwm_capture_enable(uint8_t unit, uint8_t capture_signal, hal_pwm_capture_edge_type_t capture_edge, hal_pwm_cap_isr_cb_t cb);

void hal_pwm_set_duty_cycle(uint8_t unit, uint8_t pwm_channel, uint32_t duty_cycle);

#endif /* HAL_PWM_H_ */