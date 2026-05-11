/*! \file hal_cpu.c
 *  \brief CPU Hardware Abstraction Layer nRF52 implementation
 *  
 *  Note: Only some functions have been implemented. Remaining functions to be
 *  added as necessary.
 */

/** Includes ******************************************************************/
#include "hal_cpu.h"

#include <stdint.h>
#include <stdbool.h>

#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_nvic.h"
#include "nrfx_rtc.h"

/** Public functions **********************************************************/

/*! \brief Delay for duration microseconds. Then return.
 *
 *  \param durataion Number of microseconds to wait.
 */
void hal_cpu_wait_us(uint32_t duration)
{
    nrf_delay_us(duration);
}

/*! \brief Do a software reset of the system.
 *
 *  \param reason Unused.
 */
void hal_cpu_reset(const uint8_t reason)
{
    sd_nvic_SystemReset();
}

/** Millisecond counter *******************************************************/

/* Note: The millisecond counter only works on boards which have at least 3 RTC
 * peripherals. The code will not compile when targeting a board without it and
 * is therefore conditionally compiled only if RTC2 is present.
 *
 * (RTC0 used by SoftDevice. RTC1 by app_timer)
 */
#if NRFX_CHECK(NRFX_RTC2_ENABLED)

static const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(2);
static uint64_t timestamp = 0;
static bool rtc_init_done = false;


/*! \brief Function for handling RTC interrupts.
 *
 *  Increases upper part of counter value on RTC overflow.
 *
 *  \param int_type Type of the event that triggered the handler.
 */
static void rtc_cb(nrfx_rtc_int_type_t int_type)
{
    if (int_type == NRFX_RTC_INT_OVERFLOW)
    {
          timestamp += (1 << 24);
    }
}

//! \brief Millisecond counter LFCLOCK and RTC initialization.
static void rtc_init(void)
{
    nrf_drv_clock_init();   // Only possible error is drv_clock already initialized so we ignore errors.
    nrf_drv_clock_lfclk_request(NULL);
    
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = 32;  // RTC frequency is 32768 / (prescaler + 1) or 993Hz.
    APP_ERROR_CHECK(nrfx_rtc_init(&rtc, &config, rtc_cb));
    nrfx_rtc_overflow_enable(&rtc,true);
    nrfx_rtc_counter_clear(&rtc);
    nrfx_rtc_enable(&rtc);
}

/*! \brief Returns milliseconds elapsed since first call.
 *
 *  On first call after boot starts the counter and returns 0. On subsequent
 *  calls returns number of milliseconds since that call.
 *
 *  Runs during sleep.
 *
 *  \returns Milliseconds elapsed since first call.
 */
static uint64_t get_millis(void)
{
    if (!rtc_init_done)
    {
        rtc_init();
        rtc_init_done = true;
    }
    
    // Actual frequency is only 993Hz so compensate output
    return ((nrfx_rtc_counter_get(&rtc) + timestamp)*1007)/1000;
}

/*! \brief Returns milliseconds elapsed since first call.
 *
 *  On first call after boot to this function or \p hal_cpu_get_millis_u64 
 *  starts the counter and returns 0. On subsequent calls returns number of 
 *  milliseconds elapsed since first call.
 *
 *  Runs during sleep.
 *
 *  \returns Milliseconds elapsed since first call. (Lower 4 bytes only)
 */
uint32_t hal_cpu_get_millis(void)
{
    return get_millis();
}

/*! \brief Returns milliseconds elapsed since first call.
 *
 *  On first call after boot to this function or \p hal_cpu_get_millis starts
 *  the counter and returns 0. On subsequent calls returns number of
 *  milliseconds elapsed since first call.
 *
 *  Runs during sleep.
 *
 *  \returns Milliseconds elapsed since first call.
 */
uint64_t hal_cpu_get_millis_u64(void)
{
    return get_millis();
}

#endif //NRFX_CHECK(NRFX_RTC2_ENABLED)
