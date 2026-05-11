/*! \file connector_bz_service.c
	\brief Buzzer Service
*/

#include "configuration.h"  //configuration.h must be included first

#include "connector_bz_service.h"
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "ddm2_parameter_list.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "osal.h"
#include "hal_ledc.h"
#include "broker.h"
#include "ddm_wrapper.h"
#include "connector_system.h"
#include "utils.h"

#define BUZZER_TIMER_EVENT      0

typedef struct
{
    // DDM wrapper
    ddmw_t ddm;

    // Time
    struct
    {
        ddmw_item_t available;
        ddmw_item_t Event;
    } Bz_service;

} Bz_t;
static EXT_RAM_ATTR Bz_t Bz;

/* Static functions declarations */
static int connector_bz_init(void);
static void connector_bz_service_process_task(const DDMP2_FRAME * frame);

CONNECTOR connector_bz_service =
{
    .name            = "bz service connector",
    .initialize      = connector_bz_init,
    .process_event   = connector_bz_service_process_task
};

static TimerHandle_t buzzer_timer;
static StaticTimer_t buzzer_timer_buffer;

static void connector_bz_service_timer_callback(TimerHandle_t xTimer)
{
    ddmw_send_generic_event_data(&Bz.ddm, BUZZER_TIMER_EVENT, NULL, 0);
}

static int connector_bz_init(void)
{
    int instance;

    memset(&Bz, 0, sizeof(Bz));
    ddmw_init(&Bz.ddm, &connector_bz_service);
    instance = ddmw_register(&Bz.ddm, BZ0AVL);
    ddmw_add(&Bz.ddm, &Bz.Bz_service.available, BZ0AVL, instance);
    ddmw_add(&Bz.ddm, &Bz.Bz_service.Event, BZ0EVT, instance);
    ddmw_set_i32(&Bz.Bz_service.available, 1);

    // In order to create the timer, timer period has to be > 0
    TRUE_CHECK_RETURN0(buzzer_timer = xTimerCreateStatic(NULL, UINT32_MAX, pdFALSE, NULL, connector_bz_service_timer_callback, &buzzer_timer_buffer));

    return 1;
}

static void connector_bz_service_process_task(const DDMP2_FRAME * frame)
{
    ddmw_process(&Bz.ddm, frame);

    if (ddmw_is_generic_event_updated(&Bz.ddm))
    {
        uint32_t event_id = ddmw_get_generic_event_id(&Bz.ddm);

        if (event_id == BUZZER_TIMER_EVENT)
        {
            // Turn off buzzer
            buzzer_set_duty(0);
        }
    }
    else
    {
        if (ddmw_is_updated(&Bz.Bz_service.Event))
        {
            int32_t buzz_time_ticks = 0;
            int32_t buzz_time_ms = ddmw_get_i32(&Bz.Bz_service.Event);

            buzz_time_ticks = pdMS_TO_TICKS(buzz_time_ms);
            if (buzz_time_ticks > 0)
            {
                // xTimerChangePeriod() has equivalent functionality to the
                // xTimerStart() API function even if the timer was in the dormant state.
                if (xTimerChangePeriod(buzzer_timer, buzz_time_ticks, portMAX_DELAY))
                {
                    // Turn on buzzer
                    buzzer_set_duty(1);
                }
            }
            else
            {
                LOG(E, "Invalid buzzer time(%dms) set! Mininum valid time is : (%dms)", buzz_time_ms, pdTICKS_TO_MS(1))
            }
        }
    }

    ddmw_process_publish(&Bz.ddm);
}
