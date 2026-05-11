/*! \file
    \brief RTC watchdog

    This file contains implementation of RTC watchdog. The responsibility of RTC watchdog is to
    measure RTC XTAL time constants and then deduce if components are valid or not. If the RTC
    XTAL components are valid, then the RTC XTAL oscillator will be enabled and used by the RTC
    peripheral.

    The implementation uses several peripherals to achive its functionality. Some peripherals are
    initialized by using HAL API (preferred way) but some peripheral functions are accessed by using
    low level layers directly in order to achieve fast execution speed during critical measurement
    code sections.

    The firmware is configured to initially run from internal oscillator (8MHz/256 or 150kHz). In
    ESP32 chip datasheet text it is stated that 8MHz is more stable then 150kHz oscillator. From
    this it is decided that 8MHz oscillator should be used. The drawback of using 8MHz compared to
    150kHz is higher sleep current by 1mA. During the runtime if it is evaluated that 32kHz XTAL is
    not made with correct resistor, the RTC will continue using internal oscillator.

    Note: there are no electrical specifications stating how stable are 8MHz and 150kHz oscillators.

    \author Nenad Radulovic (nenad.radulovic@seavus.com)
*/
#include "rtc_watchdog.h"

#include <stdint.h>

#include "configuration.h"
#include "esp_idf_version.h"
#include "soc/rtc.h"                    // Manipulate RTC clock settings
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_private/esp_clk.h"
#else
#if defined(CONFIG_IDF_TARGET_ESP32)
#include "esp32/clk.h"                  // Setting calibration values
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
#include "esp32s3/clk.h"                // Setting calibration values
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#include "esp32c3/clk.h"                // Setting calibration values
#else
#error Not supported config target
#endif
#endif
#include "driver/gpio.h"                // Used to initialize RTC XTAL GPIO pins
#include "sys/time.h"                   // Used to get current time marker
#include "esp_rom_sys.h"                // Used to get delays without using FreeRTOS Delay functions
#include "hal/gpio_ll.h"                // Used to manipulate RTC XTAL GPIO pins in efficient manner


#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
#if defined(CONFIG_IDF_TARGET_ESP32)
#define CONFIG_RTC_CLK_CAL_CYCLES CONFIG_ESP32_RTC_CLK_CAL_CYCLES
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
#define CONFIG_RTC_CLK_CAL_CYCLES CONFIG_ESP32S3_RTC_CLK_CAL_CYCLES
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#define CONFIG_RTC_CLK_CAL_CYCLES CONFIG_ESP32C3_RTC_CLK_CAL_CYCLES
#endif
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
#define GPIO_XTAL_OUTPUT    (GPIO_NUM_33)
#define GPIO_XTAL_INPUT     (GPIO_NUM_32)
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
#define GPIO_XTAL_OUTPUT    (GPIO_NUM_16)
#define GPIO_XTAL_INPUT     (GPIO_NUM_15)
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
// TODO CHECK
#define GPIO_XTAL_OUTPUT    (GPIO_NUM_16)
#define GPIO_XTAL_INPUT     (GPIO_NUM_15)
#endif

/**
 * @brief       Defines if to execute procedure to try to switch to external XTAL. 
 *              Set to 0 in configuration.h to everride. Default is to perform switch.
 */
#ifndef CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL
#define CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL 1   
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
#if (CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL==1) && (defined(CONFIG_ESP32_RTC_CLK_SRC_EXT_CRYS) || defined(CONFIG_RTC_CLK_SRC_EXT_CRYS))
#warning "Dangerous configuration settings: CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL=1 AND (CONFIG_ESP32_RTC_CLK_SRC_EXT_CRYS OR CONFIG_RTC_CLK_SRC_EXT_CRYS)"
#endif
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
#if (CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL==1) && defined(CONFIG_ESP32S3_RTC_CLK_SRC_EXT_CRYS)
#warning "Dangerous configuration settings: CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL=1 AND CONFIG_ESP32_RTC_CLK_SRC_EXT_CRYS"
#endif
#endif
/**
 * @brief       Define wait stabilization time after enabling RTC oscillator in microseconds
 */
#define RTC_CLK_WAIT_STABILISATION_US       1000000 // 1 s
#define RTC_CLK_WAIT_BOOTSTRAP_US           20000   // 20 ms

/**
 * @brief       Defines the number of bootstrap cycles to perform. 
 */
#ifndef CONFIG_RTC_CLK_BOOTSTRAP_CYCLES
#define CONFIG_RTC_CLK_BOOTSTRAP_CYCLES            (10)
#endif

/**
 * @brief       Defines the number of calibration reries before bailing out
 */
#ifndef CONFIG_RTC_CLK_CALIB_RETRIES
#define CONFIG_RTC_CLK_CALIB_RETRIES                (10)
#endif

/**
 * @brief       RC time measurement stabilization time
 *
 * This time is added to before the RC time measurement because we want to avoid any oscillations
 * by XTAL.
 */
#define RC_TIME_STABILISATION_TIME_US       10000

/**
 * @brief       Lower threshold of RC time (in microseconds) which is considered as valid RTC XTAL
 *
 * This value was determined experimentally on hardware which has resistor with bad value. Maximum
 * value measured was then increased by 10% as a safety margin.
 */
#define VALID_OSC_MIN_RC_TIME_US            48

/**
 * @brief       Higher threshold for a reasonably-looking calibration value for a 32k XTAL.
 *
 * Assuming we are using 5% crystal gives us: MAX: 16 800 000 (cal_value)
 */
#define RTC_WATCHDOG_MAX_PERIOD_VALUE       16800000ul

/**
 * @brief       Lower threshold for a reasonably-looking calibration value for a 32k XTAL.
 *
 * Assuming we are using 5% crystal gives us: MIN: 15 200 000 (cal_value)
 */
#define RTC_WATCHDOG_MIN_PERIOD_VALUE       15200000ul

/**
 * @brief       Enumerate possible RTC clocks
 *
 * This enumerator takes into account only 32kHz XTAL oscillator and a variant of internal
 * oscillator. Which variant of internal oscillator is determined by CONFIG_x macros (from project
 * configuration).
 */
typedef enum rtc_clk_source
{
    RTC_CLK_SOURCE_32K_XTAL,            //!< Using external 32kHz XTAL oscillator
    RTC_CLK_SOURCE_INTERNAL,            //!< Using an internal RTC oscillator
} rtc_clk_source_t;

/**
 * @brief       Currently used RTC clock source
 *
 * This variable is initialized to a value depending on CONFIG_x macros (from project configuration).
 */
#if (CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL==1)

static enum rtc_clk_source s__rtc_clk_source =
#if defined(CONFIG_ESP32_RTC_CLK_SRC_INT_RC)     || \
    defined(CONFIG_ESP32_RTC_CLK_SRC_INT_8MD256) || \
    defined(CONFIG_RTC_CLK_SRC_INT_8MD256)       || \
    defined(CONFIG_RTC_CLK_SRC_EXT_CRYS)         || \
    defined(CONFIG_ESP32S3_RTC_CLK_SRC_INT_RC)   || \
    defined(CONFIG_ESP32S3_RTC_CLK_SRC_INT_8MD256)
    RTC_CLK_SOURCE_INTERNAL;
#else
    RTC_CLK_SOURCE_32K_XTAL;
#endif

/**
 * @brief       Convert period counter value to actual frequency
 *
 * In RTC peripheral there is a period counter register which is counting number of oscillations.
 * This counter is then matched to a predefined counter value and a period count is calculated. For
 * ideal 32768kHz the period counter value has 16 000 000 ((524 288 * 1 000 000) / 16 000 000 = 32768).
 *
 * A value of period counter equal to zero is a special value, meaning that oscillator is not
 * working at all.
 *
 * @param       value Period counter value (no unit is defined)
 * @return      Frequency in Hz unit
 */
static uint32_t calc_value_to_hz(const uint32_t value)
{
    if (value == 0)
    {
        return 0;
    }
    return ((uint64_t)0x1ul << 19) * 1000000ul / (uint64_t)value;
}

/**
 * @brief       Switch to pre-selected internal oscillator
 *
 * The pre-selected internal oscillator is determined by CONFIG_x macros (from project configuration).
 *
 * After enabling the internal oscillator a calibration is executed to fine tune the frequency.
 */
static void switch_to_internal_osc_rtc(void)
{
    if (s__rtc_clk_source != RTC_CLK_SOURCE_INTERNAL)
    {
        s__rtc_clk_source = RTC_CLK_SOURCE_INTERNAL;
#if defined(CONFIG_ESP32_RTC_CLK_SRC_INT_RC) || defined(CONFIG_ESP32S3_RTC_CLK_SRC_INT_RC)
        uint32_t period;

        LOG(I, "Switching to internal 150kHz oscillator...");
        rtc_clk_slow_freq_set(RTC_SLOW_FREQ_RTC);  // Set the clock source to 150kHz
        period = rtc_clk_cal(RTC_CAL_RTC_MUX, CONFIG_RTC_CLK_CAL_CYCLES); // Measure current frequency
#if (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1)
        LOG(I, "internal oscillator calibration period: %u", period);
#endif /* (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1) */
        esp_clk_slowclk_cal_set(period); // Set the calibration, after this point we should have the calibrated clock
#endif /* defined(CONFIG_ESP32_RTC_CLK_SRC_INT_RC) */
#if defined(CONFIG_ESP32_RTC_CLK_SRC_INT_8MD256) || defined(CONFIG_RTC_CLK_SRC_INT_8MD256) || defined(CONFIG_ESP32S3_RTC_CLK_SRC_INT_8MD256)
        uint32_t period;

        LOG(I, "Switching to internal 8MHz/256 oscillator...");
        rtc_clk_8m_enable(true, true); // Enable the 8MHz oscillator
        rtc_clk_slow_freq_set(RTC_SLOW_FREQ_8MD256); // Set the clock source to 8MHz divided by 256
        period = rtc_clk_cal(RTC_CAL_RTC_MUX, CONFIG_RTC_CLK_CAL_CYCLES); // Measure current frequency
#if (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1)
        LOG(I, "internal oscillator calibration period: %u", period);
#endif /* (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1) */
        esp_clk_slowclk_cal_set(period); // Set the calibration, after this point we should have the calibrated clock
#endif /* defined(CONFIG_ESP32_RTC_CLK_SRC_INT_8MD256) || defined(CONFIG_RTC_CLK_SRC_INT_8MD256) || defined(CONFIG_ESP32S3_RTC_CLK_SRC_INT_8MD256) */
    }
    else
    {
        LOG(I, "already using internal oscillator.");
    }
}

/**
 * @brief       Switch to 32kHz XTAL oscillator
 *
 * @return      Returns calibrated frequency of the oscillator in Hz
 *
 * The function will try to switch to 32kHz oscillator and do the calibration. If calibration value
 * is out of +/-5% range another switch and calibration is retried. If the oscillator is completely
 * offline (not oscillating) then a 32kHz bootstrap procedure is executed. The 32kHz bootstrap
 * procedure is to toggle RTC XTAL GPIO pins by CPU a few times, then the pins are reconfigured and
 * routed to RTC peripheral, RTC oscillator is enabled and hopefully we should have self occurring
 * oscillations by that moment.
 *
 * The execution time of this function is approximately between 20 - 120ms depending on the state of
 * RTC XTAL oscillator.
 */
static uint32_t switch_to_32khz_freq_rtc(void)
{
    if (s__rtc_clk_source != RTC_CLK_SOURCE_32K_XTAL)
    {
        uint32_t period;
        uint32_t retries;

        s__rtc_clk_source = RTC_CLK_SOURCE_32K_XTAL;

        retries = CONFIG_RTC_CLK_CALIB_RETRIES;
        do
        {
            rtc_clk_32k_enable(true);
            rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
            esp_rom_delay_us(RTC_CLK_WAIT_STABILISATION_US); // Wait for RTC to stabilize
            period = rtc_clk_cal(RTC_CAL_32K_XTAL, CONFIG_RTC_CLK_CAL_CYCLES);
#if (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1)
            LOG(I, "RTC XTAL frequency during calibration: %u", calc_value_to_hz(period));
#endif /* (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1) */
            if ((period > RTC_WATCHDOG_MIN_PERIOD_VALUE) && (period < RTC_WATCHDOG_MAX_PERIOD_VALUE))
            {
                esp_clk_slowclk_cal_set(period);
                period = rtc_clk_cal(RTC_CAL_32K_XTAL, CONFIG_RTC_CLK_CAL_CYCLES);
#if (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1)
                LOG(I, "RTC XTAL frequency after calibration: %u", calc_value_to_hz(period));
#endif /* (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1) */
                return calc_value_to_hz(period);
            }
            if (period == 0)
            {
                LOG(W, "Detected no oscillations. Bootstrapping RTC XTAL...");
                esp_rom_delay_us(RTC_CLK_WAIT_BOOTSTRAP_US);
                rtc_clk_32k_bootstrap(CONFIG_RTC_CLK_BOOTSTRAP_CYCLES);
            }
            else
            {
                LOG(W, "Invalid RTC XTAL frequency %u, retrying...", calc_value_to_hz(period));
            }
        }
        while (retries--);
        LOG(W, "failed to start RTC XTAL oscillator");
        return 0;
    }
    else
    {
        // We are already using this oscillator, just return the current frequency.
        return calc_value_to_hz(rtc_clk_cal(RTC_CAL_32K_XTAL, CONFIG_RTC_CLK_CAL_CYCLES));
    }
}

/**
 * @brief       Measure and evaluate if RTC XTAL resistor has a valid value.
 *
 * This function will measure the RC charge/discharge times in order to deduce if the resistor value
 * is correct or not.
 *
 * The RC charge/discharge times are measured by setting one pin of RTC XTAL oscillator as output
 * and other pin as input. The output pin is then toggled and change is observed on the input pin.
 * The time is measured between pin toggling and change being observed. This time measurement is
 * done multiple times and then the average value is calculated.
 *
 * The evaluation is done by comparing measured time with a constant. The value of constant was
 * defined by experiment. If duration time is small, then it means that the resistor value is also
 * small. Bigger duration values indicate a resistor with bigger resistance (which is a valid value
 * for this resistor).
 *
 * The execution time of this function is approximately 1 second.
 *
 * @note        This function must to be allocated in IRAM_ATTR section since we want to avoid
 *              FLASH cache wait times during the execution of this function.
 *
 * @return      The state of RTC XTAL resistor
 * @retval      true - The RTC XTAL resistor is valid. The oscillator can be used.
 * @retval      false - The RTC XTAL resistor is not valid. The oscillator should not be used.
 */
static bool IRAM_ATTR is_rtc_xtal_resistor_valid(void)
{
    gpio_config_t out_config = {0};
    gpio_config_t in_config = {0};
    esp_err_t esp_err;
    struct timeval tv_now_begin;
    struct timeval tv_now_end;
    uint32_t sample_number;
    uint64_t sample_acc;

    out_config.intr_type = GPIO_INTR_DISABLE;
    out_config.mode = GPIO_MODE_OUTPUT;
    out_config.pin_bit_mask = (uint64_t)0x1ul << GPIO_XTAL_OUTPUT;
    out_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    out_config.pull_up_en = GPIO_PULLUP_DISABLE;
    esp_err = gpio_config(&out_config);
    TRUE_CHECK(esp_err == ESP_OK);
    in_config.intr_type = GPIO_INTR_DISABLE;
    in_config.mode = GPIO_MODE_INPUT;
    in_config.pin_bit_mask = (uint64_t)0x1ul << GPIO_XTAL_INPUT;
    in_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    in_config.pull_up_en = GPIO_PULLUP_DISABLE;
    esp_err = gpio_config(&in_config);
    TRUE_CHECK(esp_err == ESP_OK);
    sample_number = 100;
    sample_acc = 0;
#if (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1)
    LOG(I, "Measuring RTC XTAL RC time duration...");
#endif /* (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1) */
    esp_err = gpio_set_level(GPIO_XTAL_OUTPUT, 0);
    TRUE_CHECK(esp_err == ESP_OK);
    for (uint32_t i = 0; i < sample_number; i++)
    {
        /* Set the output to zero and wait for it to stabilise */
        gpio_ll_set_level(GPIO_LL_GET_HW(0), GPIO_XTAL_OUTPUT, 0);
        esp_rom_delay_us(RC_TIME_STABILISATION_TIME_US); // Stabilization time and to avoid oscillations from XTAL
        while (gpio_ll_get_level(GPIO_LL_GET_HW(0), GPIO_XTAL_INPUT) != 0);
        gettimeofday(&tv_now_begin, NULL); // Start measurement
        gpio_ll_set_level(GPIO_LL_GET_HW(0), GPIO_XTAL_OUTPUT, 1); // Set output to 1 and wait for it to become 1
        while (gpio_ll_get_level(GPIO_LL_GET_HW(0), GPIO_XTAL_INPUT) != 1);
        gettimeofday(&tv_now_end, NULL); // End measurement after it was detected that pin has become 1
        gpio_ll_set_level(GPIO_LL_GET_HW(0), GPIO_XTAL_OUTPUT, 0);
        int64_t time_us_begin = (int64_t)tv_now_begin.tv_sec * 1000000L + (int64_t)tv_now_begin.tv_usec;
        int64_t time_us_end = (int64_t)tv_now_end.tv_sec * 1000000L + (int64_t)tv_now_end.tv_usec;
        int64_t period_us = time_us_end - time_us_begin;
        sample_acc += period_us;
#if (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1)
        LOG(I, "Sample %u period [us]: %llu", i, period_us);
#endif /* (RTC_WATCHDOG_LOG_MEASURES_ENABLED == 1) */
    }
    uint32_t duration = sample_acc / sample_number;
    LOG(C, "Average RTC XTAL RC time duration [us]: %u", duration);
    if (duration > VALID_OSC_MIN_RC_TIME_US)
    {
        return true;
    }
    return false;
}
#endif

void rtc_watchdog_init(void)
{
#if (CONFIG_RTC_CLK_SWITCH_TO_EXT_XTAL==1)
    uint32_t clock_hz;

#if defined(CONFIG_ESP32_RTC_CLK_SRC_EXT_CRYS) || defined(CONFIG_RTC_CLK_SRC_EXT_CRYS) || defined(CONFIG_ESP32S3_RTC_CLK_SRC_EXT_CRYS)
    LOG(E, "External crystal has already been setup. Cannot perform resistor check. This is a dangerous configuration");
#else

    LOG(I, "Using internal oscillator by default. Evaluating whether the RTC XTAL oscillator is operational...");
    if (is_rtc_xtal_resistor_valid())
    {
        LOG(I, "RTC XTAL oscillator is valid.");
        clock_hz = switch_to_32khz_freq_rtc();
        if (clock_hz == 0)
        {
            LOG(E, "The XTAL 32kHz oscillator is invalid.");
            switch_to_internal_osc_rtc();
        }
    }
    else
    {
        LOG(E, "RTC resistor is not valid. Using internal oscillator.");
    }
#endif
#else
    LOG(I, "Keep current clock configuration." );
#endif
}
