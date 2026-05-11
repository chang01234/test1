#include <stddef.h>
#include <stdint.h>
#include "driver/timer.h"
#include "driver/periph_ctrl.h"

typedef void (*h_timer_cb_t)(BaseType_t *cb_status);

/* Function pointer declaration */
typedef void (*app_timer_cb_t)(BaseType_t *handle_status);

typedef struct 
{
    timer_config_t  timer_configuration;
    h_timer_cb_t timer_cb;
    uint16_t   alarm_interval;
    uint8_t timer_group;
    uint8_t timer_group_num;
} hal_timer_config_t;

//ESP32_timer
//<<! structure for timer
typedef struct 
{
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint16_t timer_counter_value;
} hal_timer_event_t;

void hal_init_timer(hal_timer_config_t *p_timer_config);

