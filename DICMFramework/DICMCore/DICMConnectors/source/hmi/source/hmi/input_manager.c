/*! \file input_manager.c
 *	\brief Module for tracking input device state and generating input events
 *	that should be processed by ui_engine.
 *
 *	Input device pins are mapped to trigger EXTI interrupts on change. The EXTI
 *	interrupt handler (see stm32l4xx_it.c) maps these to the correct device name
 *	and device type handler function.
 *
 *	Input event definitions (found in event_data.c) map changes in input device
 *	state to the menu events that these changes should trigger. Input events are
 *	monitored and updated by input_event_update() which must be called when input event
 *	is detected. This function also provides the correct input event to the ui_engine
 *	that will be handed over to the menu module which will trigger the correspodning menu events.
 */

#include "configuration.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_freertos_hooks.h"
#include "esp_idf_version.h"
#include "freertos/timers.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "event.h"
#include "hmi_data.h"
#include "screen.h"
#include "varstate.h"

#include "adc_buttons.h"
#include "connector.h"
#include "hal_cpu.h"
#include "hmi_dispatcher.h"
#include "input_manager.h"
#include "ui_engine.h"
#include "ulp_api.h"

/** Defines *******************************************************************/
#define ROTARY_STEPS_TIMEOUT             pdMS_TO_TICKS(1000)  //!< Time since last activity before rotary steps are zeroed. In system ticks.
#define ADC_DEFAULT_REFERENCE_VOLTAGE_MV 1100

#define ABSVAL(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

//! Holds static data for a connected button.
typedef struct BUTTON
{
    gpio_port_t port;
    gpio_num_t pin;
} BUTTON;

//! Holds static data for a connected rotary encoder.
typedef struct ROTARY
{
    gpio_port_t port_a;
    gpio_port_t port_b;
    gpio_num_t pin_a;
    gpio_num_t pin_b;
} ROTARY;

//! Holds runtime state for a connected button.
typedef struct _BUTTON_STATE
{
    bool active;
} BUTTON_STATE;

//! Holds runtime state for connected buttons.
typedef struct _BUTTONS_STATE
{
    BUTTON_STATE button_state[EVENT_NUM_BUTTONS];  // Runtime state for connected buttons
    bool is_any_button_active;                     // True if any button is currently being held down.
} BUTTONS_STATE;

//! Holds runtime state for a connected rotary and buttons.
typedef struct _INPUT_EVENT_DATA
{
    enum NUM_ROTARY_DIRECTION direction;  // Runtime state of the rotary
    BUTTONS_STATE buttons_state;          // Runtime state for connected buttons
} INPUT_EVENT_DATA;

bool two_hold_button = false;

#ifdef CONNECTOR_HMI_ADC_BUTTONS
static esp_adc_cal_characteristics_t adc_chars;
#endif

//! Connected button definitions.
static const BUTTON buttons[EVENT_NUM_BUTTONS] = {
#if defined(CONNECTOR_HMI_BUTTON_LEFT_PIN)
    [EVENT_BUTTON_LEFT] = {
        .port = GPIO_PORT_0,
        .pin = CONNECTOR_HMI_BUTTON_LEFT_PIN,
    },
#endif
#if defined(CONNECTOR_HMI_BUTTON_MIDDLE_PIN)
    [EVENT_BUTTON_MIDDLE] = {
        .port = GPIO_PORT_0,
        .pin = CONNECTOR_HMI_BUTTON_MIDDLE_PIN,
    },
#endif
#if defined(CONNECTOR_HMI_BUTTON_RIGHT_PIN)
    [EVENT_BUTTON_RIGHT] = {
        .port = GPIO_PORT_0,
        .pin = CONNECTOR_HMI_BUTTON_RIGHT_PIN,
    },
#endif

#ifdef CONNECTOR_HMI_GPIO_BUTTONS
    CONNECTOR_HMI_GPIO_BUTTONS
#endif

};

#ifdef CONNECTOR_HMI_ADC_BUTTONS
//! Assigns ADC values (+/- margin) to buttons.
static const ADC_BUTTON adc_buttons[] = {
    CONNECTOR_HMI_ADC_BUTTONS};
#endif  // CONNECTOR_HMI_ADC_BUTTONS

#if defined(CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM)
#if !defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
#define ADC_BUTTON_THRESHOLD (4000u)
#else
#define ADC_BUTTON_THRESHOLD ULP_ADC_BUTTON_THRESHOLD
#endif  // !CONNECTOR_HMI_ULP_BUTTONS_ENABLED
#endif  // CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM

//! Connected button states.
static volatile BUTTONS_STATE button_states;

#ifndef CONNECTOR_HMI_ULP_ROTARY_ENABLED
//! Connected rotary encoder definitions.
#ifndef CONNECTOR_HMI_GPIO_BUTTONS
static const ROTARY rotaries[EVENT_NUM_ROTARIES] = {
#if defined(CONNECTOR_HMI_ROTARY_OP1_PIN) && defined(CONNECTOR_HMI_ROTARY_OP2_PIN)
    [EVENT_ROTARY_ONE] = {
        .port_a = GPIO_PORT_0,
        .port_b = GPIO_PORT_0,
        .pin_a = CONNECTOR_HMI_ROTARY_OP1_PIN,
        .pin_b = CONNECTOR_HMI_ROTARY_OP2_PIN,
    },
#endif
};
#endif  // CONNECTOR_HMI_GPIO_BUTTONS
#endif  /* CONNECTOR_HMI_ULP_ROTARY_ENABLED */

static KEY_PRESS_EVENT_DEF key_press_def;
static KEY_HOLD_EVENT_DEF key_hold_def;
static KEY_RELEASE_EVENT_DEF key_release_def;
static ROTARY_DEF rotary_def;

static bool hold_event_triggered = false;  // True if any hold event has triggered since last time no button was active.
static bool is_input_event_detected = false;

/** Private prototypes ********************************************************/
#ifndef CONNECTOR_HMI_ULP_ROTARY_ENABLED
#ifndef CONNECTOR_HMI_GPIO_BUTTONS
static void disable_EXTI_interrupts(void);
#endif  // CONNECTOR_HMI_GPIO_BUTTONS
static void enable_EXTI_interrupts(void);
#else   /* CONNECTOR_HMI_ULP_ROTARY_ENABLED */
static void update_rotary_steps(enum NUM_ROTARY_DIRECTION direction);
#endif  // CONNECTOR_HMI_ULP_ROTARY_ENABLED
#ifdef CONNECTOR_HMI_ADC_BUTTONS
#ifdef CONNECTOR_HMI_ULP_BUTTONS_ENABLED
static uint32_t get_adc_raw_value(void);
static uint32_t filter_adc_raw_value(uint32_t adc_value);
#endif  // CONNECTOR_HMI_ULP_BUTTONS_ENABLED
static void check_adc_buttons(uint32_t adc_value, BUTTONS_STATE *button_states);
#endif  // CONNECTOR_HMI_ADC_BUTTONS

static bool check_buttons(const BUTTONS_STATE *button_state, const EVENT_BUTTON_INDEX *buttons, uint8_t num_buttons);
static bool check_buttons_hold(const BUTTONS_STATE *button_states, const EVENT_BUTTON_INDEX *buttons, uint8_t num_buttons);
static KEY_PRESS_DATA *update_key_press(const INPUT_EVENT_DATA *input_event_data);
static KEY_RELEASE_DATA *update_key_release(const INPUT_EVENT_DATA *input_event_data);
static KEY_HOLD_DATA *key_hold(uint32_t tick_time, const INPUT_EVENT_DATA *input_event_data);
static ROTARY_DATA *update_rotation(const INPUT_EVENT_DATA *input_event_data);

#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
void input_manager_init_ulp();
static bool ulp_idle_hook_cb(void);
static void update_ulp_display_state_var(uint8_t suspended);
#endif  // defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)

#if defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
#if defined(CONNECTOR_HMI_ADC_BUTTONS)
static void update_ulp_sampling_period(uint32_t adc_raw_val);
#endif  // defined(CONNECTOR_HMI_ADC_BUTTONS)
#endif  // defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED)

static void input_manager_send_event_to_ui_engine(uint32_t event_id, const void *const data, size_t data_size)
{
    ui_engine_process_event(event_id, data, data_size);
}

#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
#define ULP_POLLING_TIME_MS                                   10
#define TIME_BETWEEN_LAST_TWO_CALLS(time_now, time_last_call) (((time_now) >= (time_last_call)) ? ((time_now) - (time_last_call)) : ((UINT32_MAX - (time_last_call)) + (time_now)))

/*! \brief ULP's idle hook callback function
 *
 *	Check ULP's status if any buttons/rorary events are detected.
 *	If so, forward the ULP event to the HMI's task(connector_task) and
 *	process it by input_event_update() functions
 */
static bool ulp_idle_hook_cb(void)
{
    uint32_t data = 0;
    uint32_t ulp_event = HMI_TYPE_EVENT_NONE;
    uint32_t time_now_ms = 0;
    static uint32_t time_of_last_call_ms = 0;

    time_now_ms = hal_cpu_get_millis();
    uint32_t time_difference_ms = TIME_BETWEEN_LAST_TWO_CALLS(time_now_ms, time_of_last_call_ms);
    if (time_difference_ms >= ULP_POLLING_TIME_MS)
    {
        time_of_last_call_ms = hal_cpu_get_millis();
#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED)
        if (get_click_cw())
        {
            data = ROTARY_DIRECTION_CW;
            ulp_event = HMI_TYPE_INPUT_EVENTS_ROTARIES;
        }
        else if (get_click_ccw())
        {
            data = ROTARY_DIRECTION_CCW;
            ulp_event = HMI_TYPE_INPUT_EVENTS_ROTARIES;
        }
        else
#endif  // defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED)
        {
#if defined(CONNECTOR_HMI_ADC_BUTTONS)
#if defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
            uint32_t adc_raw_val = 0;
            static bool is_adc_raw_stable_sample_detected = false;

            adc_raw_val = get_adc_raw_value();
            update_ulp_sampling_period(adc_raw_val);

            uint32_t adc_raw_stable_sample = filter_adc_raw_value(adc_raw_val);
            if (adc_raw_stable_sample < ULP_ADC_BUTTON_THRESHOLD || (is_adc_raw_stable_sample_detected == true))
            {
                data = adc_raw_stable_sample;
                ulp_event = HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS;

                is_adc_raw_stable_sample_detected = true;
                // button press is considered only after the button is released.
                // send the last event, so the detection can be evaluated as true
                if (adc_raw_stable_sample >= ULP_ADC_BUTTON_THRESHOLD)
                {
                    is_adc_raw_stable_sample_detected = false;
                }
            }
#endif  // defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
#endif  // defined(CONNECTOR_HMI_ADC_BUTTONS)
        }

        if (ulp_event != HMI_TYPE_EVENT_NONE)
        {
            // Non blocking call
            hmi_dispatch_event_to_connector(HMI_SET_MODULE_INPUT_EVENT(HMI_MODULE_TYPE_INPUT_MANAGER, ulp_event), &data, sizeof(data), 0);
        }
    }

    return true;
}

/*! \brief Update ULP's lcd state variable
 *
 *	Provide LCD state to the ULP, so it can disable detection of rotary events when LDC is
 *	suspended(HMI_SLEEP state).	Runs from HMI task(connector_task).
 *
 *	\param	suspended		State of the LCD: HMI_WAKEUP or HMI_SLEEP
 */
static void update_ulp_display_state_var(uint8_t suspended)
{
    if ((suspended != HMI_WAKEUP) && (suspended != HMI_SLEEP))
    {
        LOG(E, "Invalid LCD state received!");
        return;
    }

    set_ulp_display_state_var(suspended);
}

//! \brief Initialize ULP and registers idle hook callback function
void input_manager_init_ulp()
{
    initialize_ulp();

    esp_register_freertos_idle_hook_for_cpu(ulp_idle_hook_cb, xPortGetCoreID());
}
#endif  // defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_BUTTONS_ENABLED)

#if defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
#if defined(CONNECTOR_HMI_ADC_BUTTONS)
/*! \brief Update's ULP's sampling period
 *
 *	Change ULP sampling period depending on LCD state or button
 *	interaction/noise when LCD is in SLEEP state. Runs from IDLE task.
 *
 *	\param	adc_raw_val		ADC raw value read from ULP. Indication to change the ULP
 *							sampling period when in HMI_SLEEP state.
 */
static void update_ulp_sampling_period(uint32_t adc_raw_val)
{
    uint32_t ulp_wakeup_period = get_ulp_wakeup_period();

    if (get_ulp_display_state_var() == HMI_SLEEP)
    {
        if (adc_raw_val < ULP_ADC_BUTTON_THRESHOLD)
        {
            /* Start sampling faster, so we can determine if it was a noise or a button press */
            if (ulp_wakeup_period == ULP_WAKEUP_TIME_MAX_US)
            {
                set_ulp_wakeup_period(0, ULP_WAKEUP_TIME_MIN_US);
            }
        }
        else
        {
            if (ulp_wakeup_period == ULP_WAKEUP_TIME_MIN_US)
            {
                set_ulp_wakeup_period(0, ULP_WAKEUP_TIME_MAX_US);
            }
        }
    }
    else
    {
        if (ulp_wakeup_period == ULP_WAKEUP_TIME_MAX_US)
        {
            set_ulp_wakeup_period(0, ULP_WAKEUP_TIME_MIN_US);
        }
    }
}
#endif  // defined(CONNECTOR_HMI_ADC_BUTTONS)
#endif  // defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)

/*! \brief Handler for updating button state during EXTI interrupt.
 *
 * 	\param	button	Index of button that triggered interrupt.
 */
static void IRAM_ATTR input_manager_EXTI_button(void *args)
{
    EVENT_BUTTON_INDEX button = (EVENT_BUTTON_INDEX)args;

    if (!gpio_get_level(buttons[button].pin))
    {
        button_states.button_state[button].active = true;
        button_states.is_any_button_active = true;
    }
    else
    {
        button_states.button_state[button].active = false;

        for (size_t n = 0; n < EVENT_NUM_BUTTONS; ++n)
        {
            if (button_states.button_state[button].active == true)
            {
                return;
            }
            button_states.is_any_button_active = false;
        }
    }
}

#ifdef CONNECTOR_HMI_ULP_ROTARY_ENABLED
/*! \brief Updates steps counter by checking ULP output detected in ulp_idle_hook_cb().
 *
 * 	Step size is given by event_data.c, ROTATION_CW and ROTATION_CCW
 * */
static void update_rotary_steps(enum NUM_ROTARY_DIRECTION direction)
{
    ROTARY_STATE *rotary_state = &rotary_def.rotary[0].rotary_state;
    if (direction == ROTARY_DIRECTION_CW)
    {
        rotary_state->steps += 4u;
    }
    if (direction == ROTARY_DIRECTION_CCW)
    {
        rotary_state->steps -= 4u;
    }
}
#endif  // CONNECTOR_HMI_ULP_ROTARY_ENABLED

#ifndef CONNECTOR_HMI_ULP_ROTARY_ENABLED
#if defined(CONNECTOR_HMI_ROTARY_OP1_PIN) && defined(CONNECTOR_HMI_ROTARY_OP2_PIN)
/*! \brief Handler for updating rotary state during EXTI interrupt.
 *
 * 	\param	rotary	Index of rotary that triggered interrupt.
 */
static void input_manager_EXTI_rotary(void *args)
{
    EVENT_ROTARY_INDEX rotary = (EVENT_ROTARY_INDEX)args;
    ROTARY_STATE *rotary_state = &rotary_def.rotary[rotary].rotary_state;

    static const int8_t step_lookup[4][4] = {
        {0, 1, -1, 0},
        {-1, 0, 0, 1},
        {1, 0, 0, -1},
        {0, -1, 1, 0},
    };

    uint32_t a = gpio_get_level(rotaries[rotary].pin_a);
    uint32_t b = gpio_get_level(rotaries[rotary].pin_b);

    int8_t position = ((!!b) << 1) | (!!a);
    int8_t previous = rotary_state->prev_position;

    // Look up direction from old vs new position.
    // 0 -> 1 -> 3 -> 2 -> 0 gives CW rotation.
    // 0 -> 2 -> 3 -> 1 -> 0 gives CCW rotation.
    rotary_state->steps += step_lookup[previous][position];
    // ESP_DRAM_LOGI(DRAM_STR("rotISR"), "steps: %d", rotary_states[rotary].steps);
    rotary_state->prev_position = position;

    rotary_state->last_change = pdTICKS_TO_MS(xTaskGetTickCountFromISR());
}
#endif

//! \brief Enable EXTI interrupts for all input devices.
static void enable_EXTI_interrupts(void)
{
    for (size_t i = 0; i < EVENT_NUM_BUTTONS; ++i)
    {
        if (buttons[i].pin != 0)
        {
            gpio_intr_enable(buttons[i].pin);
        }
    }
#ifndef CONNECTOR_HMI_GPIO_BUTTONS
    for (size_t i = 0; i < EVENT_NUM_ROTARIES; ++i)
    {
        if ((rotaries[i].pin_a != 0) && (rotaries[i].pin_b != 0))
        {
            gpio_intr_enable(rotaries[i].pin_a);
            gpio_intr_enable(rotaries[i].pin_b);
        }
    }
#endif  // CONNECTOR_HMI_GPIO_BUTTONS
}

#ifndef CONNECTOR_HMI_GPIO_BUTTONS
//! \brief Disable EXTI interrupts for all input devices.
static void disable_EXTI_interrupts(void)
{
    for (size_t i = 0; i < EVENT_NUM_BUTTONS; ++i)
    {
        if (buttons[i].pin != 0)
        {
            gpio_intr_disable(buttons[i].pin);
        }
    }

    for (size_t i = 0; i < EVENT_NUM_ROTARIES; ++i)
    {
        if ((rotaries[i].pin_a != 0) && (rotaries[i].pin_b != 0))
        {
            gpio_intr_disable(rotaries[i].pin_a);
            gpio_intr_disable(rotaries[i].pin_b);
        }
    }
}
#endif  // CONNECTOR_HMI_GPIO_BUTTONS
#endif  // CONNECTOR_HMI_ULP_ROTARY_ENABLED

//! \brief Initializes input devices.
void input_manager_init()
{
    memset(&key_press_def, 0, sizeof(key_press_def));
    memset(&key_hold_def, 0, sizeof(key_hold_def));
    memset(&key_release_def, 0, sizeof(key_release_def));
    memset(&rotary_def, 0, sizeof(rotary_def));

    ui_engine_get_event_definition_data(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS), &key_press_def, sizeof(key_press_def));
    ui_engine_get_event_definition_data(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD), &key_hold_def, sizeof(key_hold_def));
    ui_engine_get_event_definition_data(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_BUTTON_RELEASE), &key_release_def, sizeof(key_release_def));
    ui_engine_get_event_definition_data(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_ROTARIES), &rotary_def, sizeof(rotary_def));

    // gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
#ifdef CONNECTOR_HMI_ADC_BUTTONS
#ifdef CONNECTOR_HMI_ULP_BUTTONS_ENABLED
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
#else
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
    adc_power_acquire();
#endif
#endif /* CONNECTOR_HMI_ULP_ROTARY_ENABLED */
    memset(&adc_chars, 0, sizeof(adc_chars));
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, ADC_DEFAULT_REFERENCE_VOLTAGE_MV, &adc_chars);
#else
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_12Bit, ADC_DEFAULT_REFERENCE_VOLTAGE_MV, &adc_chars);
#endif
#endif /* CONNECTOR_HMI_ADC_BUTTONS */

    for (size_t i = 0; i < EVENT_NUM_BUTTONS; ++i)
    {
        if (buttons[i].pin != 0)
        {
            gpio_set_direction(buttons[i].pin, GPIO_MODE_INPUT);
            gpio_isr_handler_add(buttons[i].pin, input_manager_EXTI_button, (void *)i);
            gpio_set_intr_type(buttons[i].pin, GPIO_INTR_ANYEDGE);

            button_states.button_state[i].active = !gpio_get_level(buttons[i].pin);
        }
    }

#ifdef CONNECTOR_HMI_GPIO_BUTTONS
    enable_EXTI_interrupts();
#endif

#ifndef CONNECTOR_HMI_ULP_ROTARY_ENABLED
#ifndef CONNECTOR_HMI_GPIO_BUTTONS

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = HAL_GPIO_PINMODE_READWRITE;
    io_conf.pin_bit_mask = 1 << ROTARY_ON_PIN;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_set_level(ROTARY_ON_PIN, 1);

    for (size_t i = 0; i < EVENT_NUM_ROTARIES; ++i)
    {
        if ((rotaries[i].pin_a != 0) && (rotaries[i].pin_b != 0))
        {
            gpio_set_direction(rotaries[i].pin_a, GPIO_MODE_INPUT);
            gpio_set_direction(rotaries[i].pin_b, GPIO_MODE_INPUT);
            gpio_isr_handler_add(rotaries[i].pin_a, input_manager_EXTI_rotary, (void *)i);
            gpio_isr_handler_add(rotaries[i].pin_b, input_manager_EXTI_rotary, (void *)i);
            gpio_set_intr_type(rotaries[i].pin_a, GPIO_INTR_ANYEDGE);
            gpio_set_intr_type(rotaries[i].pin_b, GPIO_INTR_ANYEDGE);

            ROTARY_STATE *rotary_state = &rotary_def.rotary[i].rotary_state;
            rotary_state->steps = 0;
            rotary_state->prev_position = 0;
        }
    }
#endif  // CONNECTOR_HMI_GPIO_BUTTONS
#endif  /* CONNECTOR_HMI_ULP_ROTARY_ENABLED */
#if defined(CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM)
    adc_buttons_initialize_ranges(adc_buttons, ELEMENTS(adc_buttons));
#endif  // CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM
}

#ifdef CONNECTOR_HMI_ADC_BUTTONS
#ifdef CONNECTOR_HMI_ULP_BUTTONS_ENABLED
static uint32_t filter_adc_raw_value(uint32_t adc_value)
{
    uint32_t adc_val = ULP_ADC_BUTTON_THRESHOLD;

    // Calculate rolling average of samples, if enabled.
#if defined(CONNECTOR_HMI_ADC_AVG_SAMPLES) && (CONNECTOR_HMI_ADC_AVG_SAMPLES > 0)
    static int avg_samples[CONNECTOR_HMI_ADC_AVG_SAMPLES] = {0};
    static int avg_sum = 0;
    static int avg_sample_index = 0;

    avg_sum -= avg_samples[avg_sample_index];
    avg_sum += adc_value;
    avg_samples[avg_sample_index] = adc_value;

    avg_sample_index++;
    if (avg_sample_index >= CONNECTOR_HMI_ADC_AVG_SAMPLES)
    {
        avg_sample_index = 0;
    }
    adc_value = avg_sum / CONNECTOR_HMI_ADC_AVG_SAMPLES;
#endif  //(CONNECTOR_HMI_ADC_AVG_SAMPLES) && (CONNECTOR_HMI_ADC_AVG_SAMPLES > 0)

    // Log ADC data to console.
#ifdef CONNECTOR_HMI_ADC_LOG
    // ESP_DRAM_LOGI(DRAM_STR("filter_adc_raw_value"), "adc_value: %d", adc_value);
#endif

    // Wait for a stable reading before evaluating button presses.
#if defined(CONNECTOR_HMI_ADC_STABLE_SAMPLES) && (CONNECTOR_HMI_ADC_STABLE_SAMPLES > 0)
    static uint32_t old_adcval = 0;
    static int stable_count = 0;

    int adc_diff = ABSVAL(adc_value, old_adcval);
    old_adcval = adc_value;

    if (adc_diff > CONNECTOR_HMI_ADC_STABLE_MARGIN)
    {
        stable_count = 0;
    }
    else if (stable_count < CONNECTOR_HMI_ADC_STABLE_SAMPLES)
    {
        stable_count++;
    }

    if (stable_count >= CONNECTOR_HMI_ADC_STABLE_SAMPLES)
#endif  //(CONNECTOR_HMI_ADC_STABLE_SAMPLES) && (CONNECTOR_HMI_ADC_STABLE_SAMPLES > 0)
    {
        adc_val = adc_value;
    }

    return adc_val;
}

static uint32_t get_adc_raw_value(void)
{
#ifdef CONNECTOR_HMI_ULP_BUTTONS_ENABLED
    return (uint32_t)get_adc_button();
#else  /* CONNECTOR_HMI_ULP_BUTTONS_ENABLED */
    return adc1_get_raw(ADC1_CHANNEL_0);
#endif /* CONNECTOR_HMI_ULP_BUTTONS_ENABLED */
}
#endif

//! \brief Updates button states for buttons attached to the ADC.
static void check_adc_buttons(uint32_t adc_value, BUTTONS_STATE *buttons_state)
{
#ifdef CONNECTOR_HMI_ADC_BUTTON_USE_CALIBRATION
    int adc_val = esp_adc_cal_raw_to_voltage(adc_value, &adc_chars);
#else  /* CONNECTOR_HMI_ULP_BUTTONS_ENABLED */
    int adc_val = adc_value;
#endif /* CONNECTOR_HMI_ULP_BUTTONS_ENABLED */
       // Trigger buttons in the list with matching adc value.
    buttons_state->is_any_button_active = false;

    for (int i = 0; i < EVENT_NUM_BUTTONS; i++)
    {
        buttons_state->button_state[i].active = false;
    }
#if !defined(CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM)
    // Calculate the closest matching adc value in the adc_buttons list.
    int best_match = 0;
    int best_dist = abs(adc_buttons[0].adcval - adc_val);

    for (int i = 1; i < (int)ELEMENTS(adc_buttons); i++)
    {
        int dist = abs(adc_buttons[i].adcval - adc_val);
        if (dist < best_dist)
        {
            best_dist = dist;
            best_match = i;
        }
    }

    // Adjust adc value to closest matching, if within margin.
    if (best_dist <= CONNECTOR_HMI_ADC_BUTTON_MARGIN)
    {
        adc_val = adc_buttons[best_match].adcval;
    }

    for (int i = 0; i < (int)ELEMENTS(adc_buttons); i++)
    {
        if (adc_val == adc_buttons[i].adcval)
        {
            buttons_state->button_state[adc_buttons[i].index].active = true;
            buttons_state->is_any_button_active = true;
#ifdef CONNECTOR_HMI_ADC_LOG
            LOG(I, "Button (%d) pressed, adcval=%d", adc_buttons[i].index, adc_val);
#endif  // CONNECTOR_HMI_ADC_LOG
        }
    }
#else  // !CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM
    BUTTON_RANGE *range = adc_buttons_find_range_by_value(adc_val);
    if (range)
    {
#ifdef CONNECTOR_HMI_ADC_LOG
        adc_buttons_dump_range_and_value(range, adc_val);
#endif  // CONNECTOR_HMI_ADC_LOG

        for (uint32_t i = 0; i < range->index_num; ++i)
        {
            buttons_state->button_state[range->index[i]].active = true;
            buttons_state->is_any_button_active = true;
        }
    }
    else
    {
        if (adc_value < ADC_BUTTON_THRESHOLD)
        {
            LOG(W, "ADC value out of range: %u [%umV]", adc_value, adc_val);
        }
    }
#endif  // CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM
}
#endif  // CONNECTOR_HMI_ADC_BUTTONS

/*! \brief Detects correct input event provided by ULP's idle hook callback function
 *
 *	Updates the state for all input events and sends the corresponding event ID to the UI engine
 *	together with the associated state index for each event that has been detected as active. This
 *	function assumes that the state index/input event ID pair sent to the UI engine will be acted upon
 *	and will trigger execution of the correct menu event.
 *
 *	Also updates hold_event_triggered flag when required.
 *
 *	EXTI interrupts associated with input device pins are disabled during update
 *	to ensure input device state does not change mid update. Any interrupts that
 *	trigger while disabled will still set their respective interrupt pending
 *	flags and will be serviced (once) when interrupts are re-enabled.
 */
static void input_event_update(const INPUT_EVENT_DATA *input_event_data)
{
    uint32_t tick_time = hal_cpu_get_millis();  // Current tick time.

#ifndef CONNECTOR_HMI_ULP_ROTARY_ENABLED
#ifndef CONNECTOR_HMI_GPIO_BUTTONS
    uint32_t rotary_timeout = tick_time - ROTARY_STEPS_TIMEOUT;  // Timeout time for rotary encoders.

    disable_EXTI_interrupts();

    // Check state of rotaries and zero any leftover steps after a while.
    for (size_t i = 0; i < EVENT_NUM_ROTARIES; ++i)
    {
        volatile ROTARY_STATE *state = &rotary_def.rotary[i].rotary_state;

        if (state->steps != 0)
        {
            if (rotary_timeout > state->last_change)
            {
                // LOG(I, "steps=%d", state->steps);
                state->steps = 0;
            }
        }
    }

    enable_EXTI_interrupts();
#endif  // CONNECTOR_HMI_GPIO_BUTTONS
#endif  /* CONNECTOR_HMI_ULP_ROTARY_ENABLED */

    ROTARY_DATA *rotary_data = update_rotation(input_event_data);
    if (rotary_data)
    {
        input_manager_send_event_to_ui_engine(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_ROTARIES), &rotary_data->state_index, sizeof(rotary_data->state_index));
    }

    KEY_HOLD_DATA *key_hold_data = key_hold(tick_time, input_event_data);
    if (key_hold_data)
    {
        input_manager_send_event_to_ui_engine(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_BUTTON_HOLD), &key_hold_data->state_index, sizeof(key_hold_data->state_index));
    }

    KEY_PRESS_DATA *key_press_data = update_key_press(input_event_data);
    if (key_press_data)
    {
        input_manager_send_event_to_ui_engine(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS), &key_press_data->state_index, sizeof(key_press_data->state_index));
    }

    KEY_RELEASE_DATA *key_release_data = update_key_release(input_event_data);
    if (key_release_data)
    {
        input_manager_send_event_to_ui_engine(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_BUTTON_RELEASE), &key_release_data->state_index, sizeof(key_release_data->state_index));
    }

    /* Reset hold_event_triggered flag only after user releases buttons and all
     * key press events have been evaluated.
     */
    if (!input_event_data->buttons_state.is_any_button_active)
    {
        hold_event_triggered = false;
    }

    // Reset idle_ticks on user input
    if (is_input_event_detected == true)
    {
        input_manager_send_event_to_ui_engine(HMI_TYPE_SET_INPUT_EVENT(HMI_TYPE_INPUT_EVENTS_RESET_IDLE_TIMERS), NULL, 0);
        is_input_event_detected = false;
    }
}

/*! \brief Returns true if all buttons mapped to event are active.
 *
 *	Checks all buttons mapped to provided event and returns true if they are
 *	all active. Also returns true if event has 0 buttons. Returns false if at
 *	least one mapped button is not active.
 *
 *	\param	buttons			Array of indexes for buttons used by event.
 *	\param	num_buttons		Number of elements in \a buttons array
 *
 *	\returns	True if all buttons are active else false.
 */
static bool check_buttons(const BUTTONS_STATE *buttons_state, const EVENT_BUTTON_INDEX *buttons, uint8_t num_buttons)
{
    if (buttons_state)
    {
        for (size_t i = 0; i < num_buttons; ++i)
        {
            if (!buttons_state->button_state[buttons[i]].active)
            {
                // LOG(D, "No press")
                return false;
            }
        }
    }

    // LOG(D, "Press")
    return true;
}

/*! \brief Updates key press event state based on user's input. Returns key press
 *	data reference(state_index) for the active event.
 *
 *	Key press events are actually key release events that are triggered when user
 *	releases all HMI buttons.
 *
 *	A key press event is triggered when all HMI buttons are released after one
 *	or more buttons specified in a key press event definition have been pushed
 *	by user and only if no key hold event has been generated for this user
 *	interaction.
 *
 *	Only one key press event can be triggered for each interaction. If multiple
 *	events exist for the combination of triggered buttons the highest priority
 *	event(determined by the order of key_press_data array) will be triggered
 *	and the others will be ignored.
 *
 *	Note that a key press event only requires at least the specified buttons to
 *	be pushed and then released. Pushing additional buttons will not prevent the
 *	event from triggering unless this also triggers a higher priority key press
 *	event or a key hold event.
 *
 *	\return	KEY_PRESS_DATA reference for the active event.
 */
static KEY_PRESS_DATA *update_key_press(const INPUT_EVENT_DATA *input_event_data)
{
    KEY_PRESS_DATA *key_press_data = NULL;

    for (int i = 0; i < key_press_def.num_key_press_events; ++i)
    {
        KEY_PRESS_DATA *data = &key_press_def.key_press_event[i].key_press_data;
        KEY_PRESS_STATE *state = &key_press_def.key_press_event[i].key_press_state;

        if (input_event_data->buttons_state.is_any_button_active && !state->pushed)
        {
            // Update event pushed state if all buttons are active.
            state->pushed = check_buttons(&input_event_data->buttons_state, data->key_press->buttons, data->key_press->num_buttons);
#ifdef HMI_EVENT_DEBUG_NAMES
            LOG(D, "Key pushed?: %d, state_index: (%d)", state->pushed, data->state_index);
#endif
        }
        else if (!input_event_data->buttons_state.is_any_button_active && state->pushed)
        {
            /* At most one key press should trigger when user releases all
             * buttons. So set pushed to false for all key press events when
             * this happens but only return the associated KEY_PRESS_DATA reference
             * if there's not already an event pending.
             *
             * if any hold event has been triggered in response to the current set
             * of button presses assume that user did not mean to trigger a key
             * press event in addition to the hold event.
             */
            state->pushed = false;
            if (!hold_event_triggered && (is_input_event_detected == false))
            {
                is_input_event_detected = true;
                key_press_data = data;
#ifdef HMI_EVENT_DEBUG_NAMES
                LOG(D, "KEY_PRESS: %d, state_index: (%d)", data->key_press->buttons[0], data->state_index);
#endif
            }
        }
    }

    return key_press_data;
}

/*! \brief Updates key release event state based on user's input. Returns key release
 *	data reference(state_index) for the active event.
 *
 *	Key release events are actually key release events that are triggered when user
 *	releases all HMI buttons.
 *
 *	A key release event is triggered when all buttons are released after one
 *	or more buttons specified in a key press event definition have been pushed
 *	by user and only if no key hold event has been generated for this user
 *	interaction.
 *
 *	Only one key release event can be triggered for each interaction. If multiple
 *	events exist for the combination of triggered buttons the highest priority
 *	event will be triggered and the others ignored.
 *
 *	Note that a key release event only requires at least the specified buttons to
 *	be pushed and then released. Pushing additional buttons will not prevent the
 *	event from triggering unless this also triggers a higher priority key press
 *	event or a key hold event.
 *
 * 	\returns	KEY_RELEASE_DATA reference for the active event.
 */
static KEY_RELEASE_DATA *update_key_release(const INPUT_EVENT_DATA *input_event_data)
{
    KEY_RELEASE_DATA *key_release_data = NULL;

    for (int i = 0; i < key_release_def.num_key_release_events; ++i)
    {
        KEY_RELEASE_DATA *data = &key_release_def.key_release_event[i].key_release_data;
        KEY_RELEASE_STATE *state = &key_release_def.key_release_event[i].key_release_state;

        // Update event pushed state.
        state->pushed = check_buttons(&input_event_data->buttons_state, data->key_release->buttons, data->key_release->num_buttons);

        if (state->pushed)
        {
#ifdef HMI_EVENT_DEBUG_NAMES
            LOG(D, "KEY_RELEASE: pushed: %d, state_index (%d)", state->pushed, data->state_index);
#endif

            state->pushed_once = true;
        }

        if (!input_event_data->buttons_state.is_any_button_active && state->pushed_once && !state->pushed)
        {
            is_input_event_detected = true;
            key_release_data = data;

            state->pushed = false;
            state->pushed_once = false;
#ifdef HMI_EVENT_DEBUG_NAMES
            LOG(D, "KEY_RELEASE: %d, state_index: (%d)", data->key_release->buttons[0], data->state_index);
#endif
        }
    }

    return key_release_data;
}

/*! \brief Updates key hold event state based on user's input. Returns key hold
 *	data reference(state_index) for the active event.
 *
 *	Updates runtime state for the provided key hold event based on the current
 *	time tick.
 *
 *	Key hold events trigger if user holds down the specified set of buttons for
 *	at least the specified duration. Multiple key hold events can trigger in
 *	order and the same key hold event can trigger multiple times if the event
 *	repeat flag is set to true.
 *
 *	Note that key hold events only require at least the specified buttons to be
 *	held. Holding additional buttons will not prevent the event from triggering.
 *
 *	If multiple key hold events would trigger, only	the event with highest priority
 *	will be triggered. The lower priority events will trigger on subsequent call to
 *	input_event_update() if they are still valid.
 *
 * 	\param	tick_time	System time during event update.
 *
 * 	\returns			KEY_HOLD_DATA reference for the active event.
 */
static KEY_HOLD_DATA *key_hold(uint32_t tick_time, const INPUT_EVENT_DATA *input_event_data)
{
    KEY_HOLD_DATA *key_hold_data = NULL;

    for (int i = 0; i < key_hold_def.num_key_hold_events; ++i)
    {
        KEY_HOLD_DATA *data = &key_hold_def.key_hold_event[i].key_hold_data;
        KEY_HOLD_STATE *state = &key_hold_def.key_hold_event[i].key_hold_state;

        if (input_event_data->buttons_state.is_any_button_active && check_buttons_hold(&input_event_data->buttons_state, data->key_hold->buttons, data->key_hold->num_buttons))
        {
            if (state->repeat_count == 0)
            {
                /* If repeat_count is 0 then event just became active
                 * so set repeat_count and trigger_time.
                 */
                state->repeat_count = 1;
                state->trigger_time = tick_time + data->key_hold->delay;
            }
            else if (two_hold_button == true)  //! taking the two hold buttons as highest priority
            {                                  /* This scenario is for 2 keys on HOLD.
                                                   As of now the state_index is fixed for RIGHT & LEFT.
                                                   General solution to consider other buttons and 3 button hold to be updated
                                               */
                if ((tick_time > state->trigger_time) && (is_input_event_detected == false) /*&& (event->state_index == 3)*/)
                {
                    hold_event_triggered = true;
                    is_input_event_detected = true;
                    key_hold_data = data;

                    if (data->key_hold->repeat)
                    {
                        state->repeat_count++;
                        state->trigger_time += data->key_hold->delay;
                    }
                    else
                    {
                        state->trigger_time = UINT32_MAX;
                    }
                    two_hold_button = false;
#ifdef HMI_EVENT_DEBUG_NAMES
                    LOG(D, "KEY_HOLD, Two Buttons Hold; menu event state index : (%d)", data->state_index);
#endif
                }
            }
            else if ((tick_time > state->trigger_time) && (is_input_event_detected == false))
            {
                /* Key hold events should trigger in order of priority so if
                 * multiple events are pending only update repeat_count and
                 * trigger time if this is the highest priority event. That is if
                 * is_input_event_detected is still false.
                 */
                hold_event_triggered = true;
                is_input_event_detected = true;
                key_hold_data = data;

                if (data->key_hold->repeat)
                {
                    state->repeat_count++;
                    state->trigger_time += data->key_hold->delay;
                }
                else
                {
                    state->trigger_time = portMAX_DELAY;
                }
#ifdef HMI_EVENT_DEBUG_NAMES
                LOG(D, "KEY_HOLD: state_index: %d", data->state_index);
#endif
            }
            else if ((tick_time > state->trigger_time) && (is_input_event_detected == true))
            {
#ifdef HMI_EVENT_DEBUG_NAMES
                LOG(D, "Want to trigger KEY_HOLD: state_index: %d (%d)", data->key_hold->buttons[0], data->state_index);
#endif
            }
        }
        else
        {
            // If any event buttons not active unset event.
            state->repeat_count = 0;
            state->trigger_time = portMAX_DELAY;
        }
    }

    return key_hold_data;
}

/*! \brief  Updates rotation event state based on user's input. Returns rotary
 *	data reference(state_index) for the active event.
 *
 *	Rotation events trigger if user turns a rotary encoder. The event module
 *	counts the number and direction of steps for each rotary encoder. A
 *	rotation event will trigger if the step count is higher than the number of
 *	steps specified for the event in the correct direction and at least the
 *	buttons specified in the event definition are being held down.
 *
 *	When triggered a rotation event will subtract its specified step count from
 *	the number of rotary steps. At most one rotation event can trigger during
 *	each input_event_update().
 *
 *	If multiple rotation event are valid during a call to input_event_update() only
 *	the highest priority event will trigger.
 *
 * 	\returns			ROTARY_DATA reference for the active event.
 */
static ROTARY_DATA *update_rotation(const INPUT_EVENT_DATA *input_event_data)
{
    ROTARY_DATA *rotary_data = NULL;

    for (int i = 0; i < EVENT_NUM_ROTARIES; ++i)
    {
        volatile ROTARY_STATE *state = &rotary_def.rotary[i].rotary_state;
        /* Rotations have no event state so only update if we still haven't
         * found a valid event.
         *
         * We refrain form checking any_buttons_active because rotation
         * events can have 0 buttons mapped. In which case check_buttons()
         * will still return true.
         */
        if ((is_input_event_detected == false) && (state->steps != 0))
        {
            if (state->steps > 0)  // CW
            {
                ROTARY_DATA *data_cw = &rotary_def.rotary[i].rotary_data[ROTARY_DIRECTION_CW];
                if (check_buttons(&input_event_data->buttons_state, data_cw->rotation->buttons, data_cw->rotation->num_buttons))
                {
                    if (state->steps >= data_cw->rotation->steps)
                    {
                        state->steps -= data_cw->rotation->steps;
                        rotary_data = data_cw;
                        is_input_event_detected = true;
#ifdef HMI_EVENT_DEBUG_NAMES
                        LOG(D, "ROTATION: direction: CW");
#endif
                    }
                }
            }
            else if (state->steps < 0)  // CCW
            {
                ROTARY_DATA *data_ccw = &rotary_def.rotary[i].rotary_data[ROTARY_DIRECTION_CCW];
                if (check_buttons(&input_event_data->buttons_state, data_ccw->rotation->buttons, data_ccw->rotation->num_buttons))
                {
                    if (state->steps <= data_ccw->rotation->steps)
                    {
                        state->steps -= data_ccw->rotation->steps;
                        rotary_data = data_ccw;
                        is_input_event_detected = true;
#ifdef HMI_EVENT_DEBUG_NAMES
                        LOG(D, "ROTATION: direction: CCW");
#endif
                    }
                }
            }
        }
    }

    return rotary_data;
}

/*! \brief Returns true if all buttons mapped to event are active.
 *
 *	Checks all buttons mapped to provided event and returns true if they are
 *	all active.  Returns false if at least one mapped button is not active.
 *
 *	\param	buttons			Array of indexes for buttons used by event.
 *	\param	num_buttons		Number of elements in \a buttons array
 *
 *	\returns	True if all buttons are active else false.
 */
static bool check_buttons_hold(const BUTTONS_STATE *buttons_state, const EVENT_BUTTON_INDEX *buttons, uint8_t num_buttons)
{
    bool hold_buttons_status = false;

    switch (num_buttons)
    {
    case ONE_BUTTON_HOLD:
        // In case of multibuttons pressed, we have to ignore single hold buttons
        if (!two_hold_button && buttons_state->button_state[buttons[0]].active == true)
        {
#ifdef HMI_EVENT_DEBUG_NAMES
            // LOG(D, "Hold button %s (%d)", event->name, buttons[0]);
#endif
            hold_buttons_status = true;
        }
        break;
    case TWO_BUTTON_HOLD:
        if ((buttons_state->button_state[buttons[0]].active == true) && (buttons_state->button_state[buttons[1]].active == true))
        {
#ifdef HMI_EVENT_DEBUG_NAMES
            LOG(D, "Hold two buttons (%d %d)", buttons[0], buttons[1]);
#endif
            two_hold_button = true;
            hold_buttons_status = true;
        }
        else
        {
            two_hold_button = false;
        }
        break;

    default:
        break;
    }
    return hold_buttons_status;
}

#ifdef CONNECTOR_HMI_GPIO_BUTTONS
static void check_intr_buttons(BUTTONS_STATE *buttons_state)
{
    for (size_t i = 0; i < EVENT_NUM_BUTTONS; i++)
    {
        buttons_state->button_state[i].active = button_states.button_state[i].active;
        buttons_state->is_any_button_active = button_states.is_any_button_active;
    }
}
#endif  // CONNECTOR_HMI_GPIO_BUTTONS

//!< \brief Process input events received by Dispatcher that are supported by Input Manager
static void input_manager_process_input_events(uint32_t event_id, const void *const data, size_t data_size)
{
    INPUT_EVENT_DATA input_event_data = {0};
    TRUE_CHECK_RETURN(HMI_TYPE_IS_INPUT_EVENT(event_id));

    DEBUG_EVENT_NAME(event_id);
    switch (HMI_TYPE_EVENT_FIELD(event_id))
    {
    case HMI_TYPE_INPUT_EVENTS_BUTTON_PRESS:
    {
#if defined(CONNECTOR_HMI_ADC_BUTTONS)
        TRUE_CHECK_RETURN(data);
        TRUE_CHECK_RETURN(data_size == sizeof(uint32_t));
        uint32_t adc_raw_val = *(uint32_t *)data;

        check_adc_buttons(adc_raw_val, &input_event_data.buttons_state);
#endif  // defined(CONNECTOR_HMI_ADC_BUTTONS)

#ifdef CONNECTOR_HMI_GPIO_BUTTONS
        check_intr_buttons(&input_event_data.buttons_state);
#endif  // CONNECTOR_HMI_GPIO_BUTTONS
        input_event_update(&input_event_data);
        break;
    }
    case HMI_TYPE_INPUT_EVENTS_ROTARIES:
    {
#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED)
        TRUE_CHECK_RETURN(data);
        TRUE_CHECK_RETURN(data_size == sizeof(enum NUM_ROTARY_DIRECTION));
        enum NUM_ROTARY_DIRECTION direction = *(enum NUM_ROTARY_DIRECTION *)data;

        input_event_data.direction = direction;
        update_rotary_steps(input_event_data.direction);
#endif  // defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED)
        input_event_update(&input_event_data);
        break;
    }
    case HMI_TYPE_INPUT_EVENTS_NTC:
    {
#if defined(CONNECTOR_HMI_ULP_NTC_ENABLED)
        varstate_set_no_screen_upd(VARSTATE_ROOM_TEMP_INDEX, *(int32_t *)data);
#endif  // defined(CONNECTOR_HMI_ULP_NTC_ENABLED)
        break;
    }
    default:
        LOG(E, "Input Manager input event type(%02X) not supported!", HMI_TYPE_EVENT_FIELD(event_id));
        return;
    }
}

//!< \brief Process internal events received by Dispatcher that are supported by Input Manager
static void input_manager_process_internal_events(uint32_t event_id, const void *const data, size_t data_size)
{
    DEBUG_EVENT_NAME(event_id);
    switch (HMI_TYPE_EVENT_FIELD(event_id))
    {
    case HMI_TYPE_INTERNAL_EVENTS_SCREEN_STATE_UPDATED:
    {
#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
        TRUE_CHECK_RETURN(data);
        TRUE_CHECK_RETURN(data_size == sizeof(uint8_t));
        uint8_t suspended = *(uint8_t *)data;

        update_ulp_display_state_var(suspended);
#endif  // defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
        break;
    }
    default:
        LOG(E, "Input Manager internal event type(%02X) not supported!", HMI_TYPE_EVENT_FIELD(event_id));
        break;
    }
}

//!< \brief Process events received from Dispatcher that are supported by Input Manager
void input_manager_process_event(uint32_t event_id, const uint8_t *const data, size_t data_size)
{
    TRUE_CHECK_RETURN(HMI_TYPE_IS_GROUP_FIELD(event_id));

    DEBUG_GROUP_EVENT_NAME(event_id);
    switch (HMI_TYPE_GROUP_FIELD(event_id))
    {
    case HMI_TYPE_INPUT_EVENTS:
        input_manager_process_input_events(event_id, data, data_size);
        break;
    case HMI_TYPE_INTERNAL_EVENTS:
        input_manager_process_internal_events(event_id, data, data_size);
        break;
    default:
        LOG(E, "UI Engine group event type(%02X) not supported!", HMI_TYPE_GROUP_FIELD(event_id));
        break;
    }
}
