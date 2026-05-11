/*! \file hal_ledc.h
	\brief LED Control Hardware Abstraction Layer
*/

#ifndef HAL_LEDC_H_
#define HAL_LEDC_H_

#include <stddef.h>
#include <stdint.h>
#include "dicm_framework_config.h"
#ifdef HAL_LEDC_PWM
#include "driver/ledc.h"

typedef struct _HAL_LEDC_PWM_CONFIG
{
  int32_t                 gpio_num;
  uint32_t         duty_resolution;
  uint32_t                 freq_hz;
  ledc_mode_t           speed_mode;
  ledc_timer_t           timer_num;
  ledc_clk_cfg_t           clk_cfg;
  ledc_channel_t           channel;
  uint32_t                    duty;
  int32_t                   hpoint;
}HAL_LEDC_PWM_CONFIG;

/*! \brief HAL Function to initilialize the LEDC PWM Control
	\param void
	\return ESP_OK on Success / ESP_FAIL on Fail
 */
int hal_ledc_init(void);

/*! \brief HAL Function to set duty cycle
	\param speed_mode
	\param channel
	\param duty_cycle
	\return ESP_OK on Success / ESP_FAIL on Fail
 */
int hal_ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty_cycle);
#endif
#endif /* HAL_LEDC_H_ */
