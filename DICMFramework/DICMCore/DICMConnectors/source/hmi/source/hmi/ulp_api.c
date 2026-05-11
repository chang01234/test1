/* ulp.c
 *
 * This file handles data from ULP and ULP itself
 *
*/

#include "configuration.h"

#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)

#include "ulp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_idf_version.h"
#include <stdio.h>
#include <string.h>
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "driver/dac.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_private/esp_clk.h"
#include "ulp.h"
#include "ulp_common.h"
#else
#if defined(CONFIG_IDF_TARGET_ESP32)
#include "esp32/ulp.h"
#include "esp32/clk.h"
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#include "esp32s3/ulp.h"
#include "esp32s3/clk.h"
#endif
#endif
#include "ulp_hmi.h"
#include "event.h"


extern const uint8_t ulp_hmi_bin_start[] asm("_binary_ulp_hmi_bin_start");
extern const uint8_t ulp_hmi_bin_end[]   asm("_binary_ulp_hmi_bin_end");

static void init_ulp(void);
static void start_ulp_program(void);

uint16_t adc_val_button;
uint16_t adc_val_ntc;
uint32_t timeout;

uint32_t wakeup_time_us = 0;

/*! \brief Get last button ADC value read by ULP.
 *
 * \returns ADC raw value
*/
uint16_t get_adc_button(void)
{
#ifdef BUTTON_MIDDLE_PRESSED
    /* If middle button is pressed substitute adc value */
    if (!(ulp_button_middle & 0x01))
    {
        adc_val_button = BUTTON_MIDDLE_PRESSED;
    }
    else
#endif
    {
        adc_val_button = ulp_adc_last_result & UINT16_MAX;
    }
    return adc_val_button;
}

/*! \brief Get last NTC ADC value read by ULP.
 *
 * \returns ADC raw value
*/
uint16_t get_ulp_adc_ntc(void)
{
    adc_val_ntc = ulp_adc_ntc & UINT16_MAX;
    return adc_val_ntc;
}

/*! \brief Get valid clock wise rotation status.
 * If you call this function it will reset click event until new click will occur
 *
 * \returns Clock wise click status flag
*/
int16_t get_click_cw(void)
{
    uint16_t ret_val;
    if (ulp_click_cw & UINT16_MAX)
    {
        ret_val = 1;
        ulp_click_cw = 0;
    }
    else
    {
        ret_val = 0;
    }

    return ret_val;
}

/*! \brief Get valid counter clock wise rotation status.
 * If you call this function it will reset click event until new click will occur
 *
 * \returns Counter clock wise click status flag
*/
int16_t get_click_ccw(void)
{
    uint16_t ret_val;
    if (ulp_click_ccw & UINT16_MAX)
    {
        ret_val = 1;
        ulp_click_ccw = 0;
    }
    else
    {
        ret_val = 0;
    }

    return ret_val;
}

/*! \brief Initialize ULP, needed peripherals and start ULP. */
static void init_ulp(void)
{
    esp_err_t err = ulp_load_binary(0, ulp_hmi_bin_start,
            (ulp_hmi_bin_end - ulp_hmi_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    /* Configure ADC channel */
    /* Note: when changing channel here, also change 'adc_channel' constant
       in adc.S */
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
#if CONFIG_IDF_TARGET_ESP32
    adc1_config_width(ADC_WIDTH_BIT_12);
#elif CONFIG_IDF_TARGET_ESP32S2
    adc1_config_width(ADC_WIDTH_BIT_13);
#endif
    adc1_ulp_enable();

    ulp_adc_button_threshold = ULP_ADC_BUTTON_THRESHOLD;

#ifdef BUTTON_MIDDLE_PRESSED
    rtc_gpio_init(GPIO_NUM_4);
    rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_INPUT_ONLY);
#endif

    /* Set ULP wake up period to 10ms */
    set_ulp_wakeup_period(0, ULP_WAKEUP_TIME_MIN_US);
#ifdef CONNECTOR_HMI_ULP_ROTARY_ENABLED
    rtc_gpio_init(GPIO_NUM_12);
    rtc_gpio_set_direction(GPIO_NUM_12, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_NUM_12, 0);
#endif /* CONNECTOR_HMI_ULP_ROTARY_ENABLED */
    start_ulp_program();
}

/*! \brief Initialize ULP in main init funtion */
void initialize_ulp(void)
{
    init_ulp();
}

/*! \brief Start ULP program. */
static void start_ulp_program(void)
{
    /* Reset sample counter */
    ulp_sample_counter = 0;

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}

void set_ulp_wakeup_period(size_t period_index, uint32_t period_us)
{
#if 1
    wakeup_time_us = period_us;

// Overriding original ulp's helper function 'ulp_set_wakeup_period', since it uses
// ESP_LOGW macro, which causes stack smashing when is called from the idle hook cb function.
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3) && (ESP_IDF_VERSION <= ESP_IDF_VERSION_VAL(4, 4, 8)) )
#if CONFIG_IDF_TARGET_ESP32
    if (period_index > 4) {
        return;
    }
    uint64_t period_us_64 = period_us;
    uint64_t period_cycles = (period_us_64 << RTC_CLK_CAL_FRACT) / esp_clk_slowclk_cal_get();
    uint64_t min_sleep_period_cycles = ULP_FSM_PREPARE_SLEEP_CYCLES
                                    + ULP_FSM_WAKEUP_SLEEP_CYCLES
                                    + REG_GET_FIELD(RTC_CNTL_TIMER2_REG, RTC_CNTL_ULPCP_TOUCH_START_WAIT);
    if (period_cycles < min_sleep_period_cycles) {
        period_cycles = 0;
        ESP_DRAM_LOGW(DRAM_STR("set_ulp_wakeup_period"), "Sleep period clipped to minimum of %d cycles", (uint32_t) min_sleep_period_cycles);
    } else {
        period_cycles -= min_sleep_period_cycles;
    }
    REG_SET_FIELD(SENS_ULP_CP_SLEEP_CYC0_REG + period_index * sizeof(uint32_t),
            SENS_SLEEP_CYCLES_S0, (uint32_t) period_cycles);
#else // CONFIG_IDF_TARGET_ESP32
#error "Please update code with support for IDF target different than 'CONFIG_IDF_TARGET_ESP32'"
#endif // CONFIG_IDF_TARGET_ESP32
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

#else // ESP_IDF_VERSION
#error "Please update code with support for IDF version different than '4.4.3-6"
#endif // ESP_IDF_VERSION
#else
    ulp_set_wakeup_period(period_index, period_us);
#endif
}

#endif //defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
