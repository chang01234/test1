#include "configuration.h"

#if defined(CONNECTOR_HMI_ULP_NTC_ENABLED)

#include "ddm2_parameter_list.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hmi_dispatcher.h"
#include "hmi_sensor.h"
#include "ulp_api.h"

#define DEBUG_SENSOR_LOG_ENABLE 0

// Constants for calculating temperature from NTC voltage.
#define ADC_MAX 4095         //!< Highest possible ADC output value.
#define R_CONST 10000        //!< NTC voltage divider constant resistance in ohms.
#define BETA    3500.0f      //!< NTC thermistor beta value.
#define R_INF   0.07976128f  //!< For NTC with beta = 3500 and R0 = 10kohm @ 25C.

#define NTC_MAX_TEMP_C (150 * Ddm2_unit_factor_list[DDM2_UNIT_DEGC])
#define NTC_MIN_TEMP_C (-50 * Ddm2_unit_factor_list[DDM2_UNIT_DEGC])
#define NTC_MAX_TEMP_F (320 * Ddm2_unit_factor_list[DDM2_UNIT_DEG]) /* 320 F = 150 C */
#define NTC_MIN_TEMP_F (-58 * Ddm2_unit_factor_list[DDM2_UNIT_DEG]) /* -58 F = -50 C */

static void read_temperature_cb(TimerHandle_t xTimer);
static TimerHandle_t read_temperature_timer; /* Read temperature timer */

/*! \brief Initialize HMI sensors.
 *
 * \param ddm_wrapper DDM wrapper used to trigger temperature value update
 */
void hmi_sensor_init(void)
{
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_12);
    adc1_config_width(ADC_WIDTH_BIT_12);

    TRUE_CHECK(read_temperature_timer = xTimerCreate(NULL, pdMS_TO_TICKS(1000), pdTRUE, (void *)0, read_temperature_cb));
    xTimerStart(read_temperature_timer, 0);
}

/*! \brief Calculate NTC temperature in Celsius from measured ADC value.
 *
 * Calculates temperature in degrees Celsius based on the provided ADC value
 * and defined constants.
 *
 * \returns  Temperature in degrees Celsius converted to DDM parameter.
 */
bool get_hmi_temperature_celsius(int32_t *ddm_temperature)
{
    bool temperature_value_ok;
    uint16_t adc_val;
    float c, r;
    adc_val = get_ulp_adc_ntc();
    r = R_CONST / (ADC_MAX / (float)adc_val - 1);  // NTC resistance in ohms.
    c = BETA / logf(r / R_INF) - 273.15f;          // NTC temperature in deg C.

    c -= 1.9f;                                       // Apply offset for better accuracy.
    c = (c * Ddm2_unit_factor_list[DDM2_UNIT_DEGC]); /* convert to DDM value */

    *ddm_temperature = (int32_t)c;

    if ((c > NTC_MAX_TEMP_C) || (c < NTC_MIN_TEMP_C))
    {
        temperature_value_ok = false;
    }
    else
    {
        temperature_value_ok = true;
    }

    return temperature_value_ok;
}

/*! \brief Timer that reads and updates room temperature value
 *
 */
static void read_temperature_cb(TimerHandle_t xTimer)
{
    int32_t temp_val;
    bool temp_status_ok;
    uint32_t event_id;

    temp_status_ok = get_hmi_temperature_celsius(&temp_val);
    event_id = HMI_TYPE_SET_MODULE_FIELD(HMI_MODULE_TYPE_INPUT_MANAGER) | HMI_TYPE_SET_GROUP_FIELD(HMI_TYPE_INPUT_EVENTS) | HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_NTC);
    hmi_dispatch_event_to_connector(event_id, &temp_val, sizeof(temp_val), portMAX_DELAY);
    if (!temp_status_ok)
    {
        /* LOG warning message if needed here */
    }
#if DEBUG_SENSOR_LOG_ENABLE
    LOG(I, "HMI room temperature: %d, status: %d", temp_val, temp_status);
#endif /* DEBUG_SENSOR_LOG_ENABLE */
}
#endif
