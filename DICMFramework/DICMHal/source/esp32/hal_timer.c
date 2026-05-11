#include "dicm_framework_config.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "freertos/FreeRTOS.h"
#pragma GCC diagnostic pop

#include "hal_timer.h"

static app_timer_cb_t app_cb_function;

static bool hal_timer_isr(void* arg);

/**
 * @brief hal timer init function to initialize Timer and its interrupt call back function
 *        Separate calls has to be done from different TIMER init with the respective TIMER id structure
 * @param p_timer_config configuration parameter for timer
 */
void hal_init_timer(hal_timer_config_t *p_timer_config)
{ 
    timer_config_t config =
    {
		.alarm_en       = p_timer_config->timer_configuration.alarm_en,
		.counter_en     = p_timer_config->timer_configuration.counter_en,
		.intr_type      = p_timer_config->timer_configuration.intr_type,
		.counter_dir    = p_timer_config->timer_configuration.counter_dir,
		.auto_reload    = p_timer_config->timer_configuration.auto_reload,
		.divider        = p_timer_config->timer_configuration.divider,   /* 1 us per tick */
    };
    
    timer_init(p_timer_config->timer_group, p_timer_config->timer_group_num, &config);
    timer_set_counter_value(p_timer_config->timer_group, p_timer_config->timer_group_num, 0);
    timer_set_alarm_value(p_timer_config->timer_group, p_timer_config->timer_group_num, p_timer_config->alarm_interval);
    timer_enable_intr(p_timer_config->timer_group, p_timer_config->timer_group_num);
    app_cb_function = p_timer_config->timer_cb;
    timer_isr_callback_add(p_timer_config->timer_group, p_timer_config->timer_group_num, hal_timer_isr, NULL, 0);
    timer_start(p_timer_config->timer_group, p_timer_config->timer_group_num);
}

/**
 * @brief Hal Timer isr function called when timer interrupt occurs
 * 
 * @param arg 
 */
static IRAM_ATTR bool hal_timer_isr(void* arg)
{
    BaseType_t high_task_awoken = pdFALSE;

    if (app_cb_function != NULL)
    {
        (*app_cb_function)(&high_task_awoken);
    }
    return high_task_awoken;
}
