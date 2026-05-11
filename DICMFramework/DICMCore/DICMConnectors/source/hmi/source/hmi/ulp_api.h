#ifndef ULP_H_
#define ULP_H_

#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)

#include <stdint.h>

/* value below which ESP32 shall wake up */
#define ULP_ADC_BUTTON_THRESHOLD (4000u)

/* Default min and max ulp wake-up intervals */
#ifndef ULP_WAKEUP_TIME_MIN_MS
#define ULP_WAKEUP_TIME_MIN_MS    (10u)
#endif

#ifndef ULP_WAKEUP_TIME_MAX_MS
#define ULP_WAKEUP_TIME_MAX_MS    (150u)
#endif

#define ULP_WAKEUP_TIME_MIN_US    (ULP_WAKEUP_TIME_MIN_MS * 1000u)
#define ULP_WAKEUP_TIME_MAX_US    (ULP_WAKEUP_TIME_MAX_MS * 1000u)

#if (ULP_WAKEUP_TIME_MIN_US >= ULP_WAKEUP_TIME_MAX_US)
#error "Invalid configuration, ULP_WAKEUP_TIME_MIN_US >= ULP_WAKEUP_TIME_MAX_US"
#endif

void initialize_ulp(void);
uint16_t get_adc_button(void);
uint16_t get_ulp_adc_ntc(void);
int16_t get_click_cw(void);
int16_t get_click_ccw(void);

void set_ulp_wakeup_period(size_t period_index, uint32_t period_us);

static inline uint32_t get_ulp_wakeup_period(void)
{
    extern uint32_t wakeup_time_us;
    return wakeup_time_us;
};

/*! \brief Set ULP's display variable
 *
 * 'lcd_suspended' global symbol defined in ULP FSM program,
 *  is declared as 'ulp_lcd_suspended' in ${ULP_APP_NAME}.h
 */
static inline void set_ulp_display_state_var(uint8_t suspended)
{
    extern uint32_t ulp_lcd_suspended;
    ulp_lcd_suspended = (uint32_t)suspended;
};

/*! \brief Get ULP's display variable
 *
 * 'lcd_suspended' global symbol defined in ULP FSM program,
 *  is declared as 'ulp_lcd_suspended' in ${ULP_APP_NAME}.h
 */
static inline uint32_t get_ulp_display_state_var(void)
{
    extern uint32_t ulp_lcd_suspended;
    return ulp_lcd_suspended;
};

#endif //defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
#endif /* ULP_H_ */