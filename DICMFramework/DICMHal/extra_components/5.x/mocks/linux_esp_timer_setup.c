/*
 * linux_driver_setup.c
 *
 *  Created on: 2 okt. 2023
 *      Author: Andlun
 */

#include "iGeneralDefinitions.h"
#include "esp_timer.h"
#include "Mockesp_timer.h"
#include "linux_esp_timer_setup.h"
#include <time.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

static struct timespec init_time;

//static CMOCK_esp_timer_get_time_CALLBACK my_esp_timer_get_time;
static int64_t my_esp_timer_get_time(int num_calls);
// CMOCK_esp_timer_create_CALLBACK
static esp_err_t my_esp_timer_create(const esp_timer_create_args_t* create_args, esp_timer_handle_t* out_handle, int cmock_num_calls);
// typedef esp_err_t (* CMOCK_esp_timer_start_periodic_CALLBACK)(esp_timer_handle_t timer, uint64_t period, int cmock_num_calls);
static esp_err_t my_esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period, int cmock_num_calls);

void linux_esp_timer_setup(void)
{
    int result = clock_gettime(CLOCK_MONOTONIC, &init_time);
    assert(result == 0);
    Mockesp_timer_Init();

    esp_timer_get_time_StubWithCallback(my_esp_timer_get_time);
    esp_timer_create_StubWithCallback(my_esp_timer_create);
    esp_timer_start_periodic_Stub(my_esp_timer_start_periodic);
}

static int64_t my_esp_timer_get_time(int num_calls)
{
    struct timespec current_time;
    int result = clock_gettime(CLOCK_MONOTONIC, &current_time);
    assert(result == 0);
    int64_t microseconds = (current_time.tv_sec * 1000000 + current_time.tv_nsec / 1000) -
            (init_time.tv_sec * 1000000 + init_time.tv_nsec / 1000);

    return microseconds;
}

/*
 *     esp_timer_create_args_t timer_conf = {
        .callback = _mdns_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "mdns_timer"
 *
 */
static esp_timer_create_args_t my_timer_conf[10];
static int my_timer_conf_num = 0;

static void my_tick_handler(TimerHandle_t timer)
{
    esp_timer_create_args_t* p_timer_args = (esp_timer_create_args_t*)pvTimerGetTimerID(timer);
    p_timer_args->callback(p_timer_args->arg);
    LOG(I, "Timer callback %s", p_timer_args->name);
}

static esp_err_t my_esp_timer_create(const esp_timer_create_args_t* create_args, esp_timer_handle_t* out_handle, int cmock_num_calls)
{
    esp_err_t err = ESP_OK;
    my_timer_conf[my_timer_conf_num] = *create_args;
    TimerHandle_t timer = xTimerCreate(create_args->name,
                                    100,
                                    pdTRUE,
                                    &my_timer_conf[my_timer_conf_num],
                                    my_tick_handler);
    if (timer != NULL)
    {
        my_timer_conf_num++;
        *out_handle = (esp_timer_handle_t)timer;
    }
    else
    {
        err = ESP_FAIL;
    }
    return err;
}

static esp_err_t my_esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period, int cmock_num_calls)
{
    esp_err_t err = ESP_OK;

    if (pdPASS != xTimerChangePeriod((TimerHandle_t)timer, period/1000/portTICK_PERIOD_MS, 200))
    {
        err = ESP_FAIL;
    }
    if ((err == ESP_OK) && (pdPASS != xTimerReset((TimerHandle_t)timer, 200)))
    {
        err = ESP_FAIL;
    }
    return err;
}
