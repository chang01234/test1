/*! \file hal_ledc.c
	\brief LED Control Hardware Abstraction Layer ESP32 implementation
*/
#include "dicm_framework_config.h"

#ifdef HAL_LEDC_PWM
#include "iGeneralDefinitions.h"
#include <string.h>
#include "hal_ledc.h"

extern const HAL_LEDC_PWM_CONFIG ledc_pwmconfig[];
extern const int ledc_pwm_config_size;

//! \~ Define Set duty cycle functions, invoke as name_set_duty(duty_cycle);
#define LEDC_PWM(name, gpio_num, duty_resolution, freq_hz, speed_mode, timer_num, clk_cfg, channel, duty, hpoint) int name##_set_duty (uint32_t duty_cycle) \
{\
    return hal_ledc_set_duty(speed_mode, channel, duty_cycle); \
}
LEDC_CONFIGURATION
#undef LEDC_PWM

/*! \brief  Init LEDC 
	\param  Void
	\return ESP_OK on Success / ESP_FAIL on Fail
 */
int hal_ledc_init(void)
{
    esp_err_t result = ESP_OK;
    uint8_t index;
    ledc_timer_config_t ledc_timer;
    ledc_channel_config_t ledc_channel;

    memset(&ledc_timer, 0, sizeof(ledc_timer_config_t));
    memset(&ledc_channel, 0, sizeof(ledc_channel_config_t));
    for (index = 0; index < ledc_pwm_config_size; index++)
    {
       ledc_timer.duty_resolution = ledc_pwmconfig[index].duty_resolution;
       ledc_timer.freq_hz         = ledc_pwmconfig[index].freq_hz;
       ledc_timer.speed_mode      = ledc_pwmconfig[index].speed_mode;
       ledc_timer.timer_num       = ledc_pwmconfig[index].timer_num;
       ledc_timer.clk_cfg         = ledc_pwmconfig[index].clk_cfg;

       LOG(W, " duty_resolution = %d, freq_hz = %d,  speed_mode = %d , timer_num = %d, clk_cfg = %d", ledc_timer.duty_resolution, ledc_timer.freq_hz
                , ledc_timer.speed_mode, ledc_timer.timer_num, ledc_timer.clk_cfg );

       // ledc timer configuration
       ZERO_CHECK(ledc_timer_config(&ledc_timer));

       ledc_channel.channel     = ledc_pwmconfig[index].channel;
       ledc_channel.duty        = ledc_pwmconfig[index].duty;
       ledc_channel.gpio_num    = ledc_pwmconfig[index].gpio_num;
       ledc_channel.speed_mode  = ledc_pwmconfig[index].speed_mode;
       ledc_channel.hpoint      = ledc_pwmconfig[index].hpoint;
       ledc_channel.timer_sel   = ledc_pwmconfig[index].timer_num;
	   ledc_channel.intr_type	= LEDC_INTR_DISABLE;

       LOG(W, " channel = %d, duty = %d,  gpio_num = %d , speed_mode = %d, hpoint = %d timer_sel = %d ", ledc_channel.channel, ledc_channel.duty
                , ledc_channel.gpio_num, ledc_channel.speed_mode, ledc_channel.hpoint,  ledc_channel.timer_sel);

       // ledc channel configuration
       ZERO_CHECK(ledc_channel_config(&ledc_channel));
    }

    return result;
}

/*! \brief  Function to Set duty cycle
	\param  DutyCycle
	\return ESP_OK on Success / ESP_FAIL on Fail
 */
int hal_ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty_cycle) 
{
    esp_err_t result = ESP_OK;

    ZERO_CHECK(ledc_set_duty(speed_mode, channel, duty_cycle));
    ZERO_CHECK(ledc_update_duty(speed_mode, channel));

    return result;
}

#endif




