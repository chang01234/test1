/*
 * power_management.c
 *
 *  Created on: 22 Sep 2022
 *      Author: Ulf.Landberger
 * 
 * Some background information about the mechanism used for DICM power management
 *  https://www.freertos.org/low-power-tickless-rtos.html
 * 
 */

#include "configuration.h"


#ifndef CONFIG_PM_ENABLE
#error CONFIG_PM_ENABLE Must be defined to run DICM power management
#endif

#include <string.h>
#include "power_management.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
#include "clk.h"
#else
#include "esp_private/rtc_clk.h"
#include "soc/rtc.h"
#endif
#include "rtc.h"
#include "rtc_wdt.h"

#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_pm.h"

#ifdef PM_ENABLE_DEBUG_CLOCK
#include "soc/sens_reg.h"
#endif /* PM_ENABLE_DEBUG_CLOCK */

#define RTC_CLK_WAIT_STABILISATION_US 1000000

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
#ifndef CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ
#endif
#ifndef CONFIG_XTAL_FREQ
#define CONFIG_XTAL_FREQ CONFIG_ESP32_XTAL_FREQ
#endif
#ifndef CONFIG_RTC_CLK_CAL_CYCLES
#define CONFIG_RTC_CLK_CAL_CYCLES CONFIG_ESP32_RTC_CLK_CAL_CYCLES
#endif
#ifndef RTC_SLOW_CLK_FREQ_150K
#define RTC_SLOW_CLK_FREQ_150K RTC_SLOW_FREQ_RTC
#endif
static esp_pm_config_esp32_t pm_config =
#else
static esp_pm_config_t pm_config =
#endif
{
	.max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
	.min_freq_mhz = CONFIG_XTAL_FREQ,
#ifdef CONFIG_FREERTOS_USE_TICKLESS_IDLE    
	.light_sleep_enable = true
#endif
};

#define CALIBRATE_ONE(cali_clk) calibrate_one(cali_clk, #cali_clk)
static uint32_t calibrate_one(rtc_cal_sel_t cal_clk, const char* name)
{
    const uint32_t cal_count = CONFIG_RTC_CLK_CAL_CYCLES;
    const float factor = (1 << 19) * 1000.0f;
    uint32_t cali_val;
    cali_val = rtc_clk_cal(cal_clk, cal_count);
    if (cali_val) {
        LOG(C, "Calibrated %s to %.3f kHz\n", name, factor / (float) cali_val);
    } else {
        LOG(E, "Calibration failed: %s:\n", name);
    }
    return cali_val;
}

#ifdef PM_ENABLE_DEBUG_CLOCK
/* The following two are not unit tests, but are added here to make it easy to
 * check the frequency of 150k/32k oscillators. The following two "tests" will
 * output either 32k or 150k clock to GPIO25.
 */

static void pull_out_clk(int sel)
{
    REG_SET_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_MUX_SEL_M);
    REG_CLR_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_RDE_M | RTC_IO_PDAC1_RUE_M);
    REG_SET_FIELD(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_FUN_SEL, 1);
    REG_SET_FIELD(SENS_SAR_DAC_CTRL1_REG, SENS_DEBUG_BIT_SEL, 0);
    REG_SET_FIELD(RTC_IO_RTC_DEBUG_SEL_REG, RTC_IO_DEBUG_SEL0, sel);
}

/* sel = 4 : 32k XTAL; sel = 5 : internal 150k RC */
static void set_up_clk(int sel)
{
    SET_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_X32N_MUX_SEL | RTC_IO_X32P_MUX_SEL);
    CLEAR_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_X32P_RDE | RTC_IO_X32P_RUE | RTC_IO_X32N_RUE | RTC_IO_X32N_RDE);
    CLEAR_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_X32N_MUX_SEL | RTC_IO_X32P_MUX_SEL);
    SET_PERI_REG_BITS(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_DAC_XTAL_32K, 1, RTC_IO_DAC_XTAL_32K_S);
    SET_PERI_REG_BITS(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_DRES_XTAL_32K, 3, RTC_IO_DRES_XTAL_32K_S);
    SET_PERI_REG_BITS(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_DBIAS_XTAL_32K, 0, RTC_IO_DBIAS_XTAL_32K_S);
    SET_PERI_REG_MASK(RTC_IO_XTAL_32K_PAD_REG, RTC_IO_XPD_XTAL_32K);
    REG_SET_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_MUX_SEL_M);
    REG_CLR_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_RDE_M | RTC_IO_PDAC1_RUE_M);
    REG_SET_FIELD(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_FUN_SEL, 1);
    REG_SET_FIELD(SENS_SAR_DAC_CTRL1_REG, SENS_DEBUG_BIT_SEL, 0);
    REG_SET_FIELD(RTC_IO_RTC_DEBUG_SEL_REG, RTC_IO_DEBUG_SEL0, sel);
}
#endif /* PM_ENABLE_DEBUG_CLOCK */

void calibrate_slow_clk(void) {
    uint32_t slow_clock_freq;
#ifdef PM_ENABLE_DEBUG_CLOCK
	rtc_clk_32k_bootstrap(3000);
	rtc_clk_32k_bootstrap(3000);
#endif /* PM_ENABLE_DEBUG_CLOCK */
	rtc_clk_32k_enable(true);
#ifdef PM_ENABLE_DEBUG_CLOCK
	set_up_clk(4);
#endif /* PM_ENABLE_DEBUG_CLOCK */
    rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
    esp_rom_delay_us(RTC_CLK_WAIT_STABILISATION_US); // Wait for RTC to stabilize
    slow_clock_freq = CALIBRATE_ONE(RTC_CAL_32K_XTAL);
 
    if (slow_clock_freq == 0) {
        LOG(W, "Switching to RTC_SLOW_CLK_FREQ_150K");
        rtc_clk_slow_freq_set(RTC_SLOW_CLK_FREQ_150K);
       	CALIBRATE_ONE(RTC_CAL_RTC_MUX);
    } else {
        rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
    }
#ifdef PM_ENABLE_DEBUG_CLOCK
    pull_out_clk(RTC_IO_DEBUG_SEL0_32K_XTAL);
#endif /* PM_ENABLE_DEBUG_CLOCK */
}


void  print_wakeup_reason(esp_sleep_wakeup_cause_t wakeup_reason)
{
    switch(wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0 : LOG(W, "Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : LOG(W, "Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : LOG(W, "Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : LOG(W, "Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_GPIO : LOG(W, "Wakeup caused by GPIO"); break;
        case ESP_SLEEP_WAKEUP_ULP : LOG(W, "Wakeup caused by ULP program"); break;
        default : LOG(W, "Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
    }
}

#ifdef ENABLE_PM_SUPERVISION
static void pm_supervision_task(void* Parameter)
{
	LOG(W, "Supervising PM...");

	for (;;)
	{
        esp_pm_dump_locks(stdout);
        vTaskDelay(10000);
	}

}
#endif

void initialize_power_management(void) 
{
	LOG(I, "Enabling System Power Management...");

	pm_config.min_freq_mhz = (int) rtc_clk_xtal_freq_get();

    ZERO_CHECK(esp_pm_configure(&pm_config));

#ifdef ENABLE_PM_SUPERVISION
    /* Dump power management and sleep statistics */
	TRUE_CHECK(xTaskCreate(pm_supervision_task, "pm_supervision", 4096, NULL, xTASK_PRIORITY_IDLE, NULL));
#endif

    /* No wake-up sources as default. */
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

#ifdef ENABLE_TWAI_PM
	// Enable wakeup on RX pin
	ZERO_CHECK(gpio_set_pull_mode(DEVICE_TWAI_RX, GPIO_PULLUP_ONLY));
	ZERO_CHECK(gpio_wakeup_enable(DEVICE_TWAI_RX, GPIO_INTR_LOW_LEVEL));
#ifdef CONNECTOR_LINDEV
    ZERO_CHECK(gpio_wakeup_enable(CONNECTOR_LINDEV_UART_RX, GPIO_INTR_LOW_LEVEL));
#endif
	ZERO_CHECK(esp_sleep_enable_gpio_wakeup());    
#endif

#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED) 
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
#endif
}

