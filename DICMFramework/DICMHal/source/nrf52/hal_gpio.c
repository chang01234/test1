/*! \file hal_gpio.c
	\brief GPIO Hardware Abstraction Layer
*/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nrfx_gpiote.h"

#include "hal_gpio.h"

#define NUM_ISR_PINS NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS

typedef struct ISR_PIN
{
    nrfx_gpiote_pin_t pin_id;
    HAL_GPIO_ISR_CB cb;
    HAL_GPIO_INTRMODE_ENUM intrmode;
    uint8_t port;
    uint8_t pin;
    bool configured;
} ISR_PIN;

static ISR_PIN isr_pins[NUM_ISR_PINS] = { 0 };

static void nrf52_pinsetup(int port, int pin, HAL_GPIO_PINMODE_ENUM pinmode, HAL_GPIO_INTRMODE_ENUM intrmode, HAL_GPIO_ISR_CB cb);
static void nrf52_setlevel(int port, int pin, int level);
static void nrf52_getlevel(int port, int pin, int *level);
static bool nrf52_pin_is_free(nrfx_gpiote_pin_t pin_id);
static void nrf52_gpio_isr(nrfx_gpiote_pin_t pin_id, nrf_gpiote_polarity_t action);

HAL_GPIO_RESULT_ENUM hal_gpio_init(void)
{
    if (!nrfx_gpiote_is_init())
    {
        nrfx_gpiote_init();
    }
    
    //Note: Errors on NRF52 are handled trough asserts. Function only returns if no error.
    return HAL_GPIO_OK;
}

HAL_GPIO_RESULT_ENUM hal_gpio_pinsetup(const HAL_GPIO_DEVICE_ENUM device, const int port, const int pin, const HAL_GPIO_PINMODE_ENUM pinmode, const HAL_GPIO_INTRMODE_ENUM intrmode,const HAL_GPIO_ISR_CB cb)
{   
    switch (device)
    {
        case HAL_GPIO_DEVICE_SELF:
            nrf52_pinsetup(port, pin, pinmode, intrmode, cb);
            break;
        default:
            APP_ERROR_HANDLER(0);   //Unsupported device.
    }
    
    //Note: Errors on NRF52 are handled trough asserts. Function only returns if no error.
    return HAL_GPIO_OK;
}

HAL_GPIO_RESULT_ENUM hal_gpio_setlevel(const HAL_GPIO_DEVICE_ENUM device, const int port, const int pin, const int level)
{
    switch (device)
    {
        case HAL_GPIO_DEVICE_SELF:
            nrf52_setlevel(port, pin, level);
            break;
        default:
            APP_ERROR_HANDLER(0);   //Unsupported device.
    }
    
    //Note: Errors on NRF52 are handled trough asserts. Function only returns if no error.
    return HAL_GPIO_OK;
}

HAL_GPIO_RESULT_ENUM hal_gpio_getlevel(const HAL_GPIO_DEVICE_ENUM device, const int port, const int pin, int *level)
{  
    nrfx_gpiote_pin_t pin_id = NRF_GPIO_PIN_MAP(port, pin);
    
    switch (device)
    {
        case HAL_GPIO_DEVICE_SELF:
            nrf52_getlevel(port, pin, level);
            break;
        default:
            APP_ERROR_HANDLER(0);   //Unsupported device.
    }
    
    //Note: Errors on NRF52 are handled trough asserts. Function only returns if no error.
    return HAL_GPIO_OK;
}

/*! \brief Configure specified nRF52 pin.
 *
 *  Configures an nRF52 pin based on provided parameters. Uses SDK GPIO macros
 *  to handle pins unless pin if configured for input with callback on pin level
 *  change in which case GPIOTE module will be used.
 *
 *  Trying to configure an enabled pin to any state except
 *  HAL_GPIO_PINMODE_DISABLE is an error. Trying to disable an already disabled
 *  pin is not.
 *
 *  \param port     Port of pin to configure.
 *  \param pin      Number of pin to configure.
 *  \param pinmode  New pin mode (input/output).
 *  \param intmode  New input pin interrupt mode. Ignored for output pins.
 *  \param cb       Callback function for interrupt pins. Ignored if intrmode is HAL_GPIO_INTRMODE_DISABLE.
 */
static void nrf52_pinsetup(int port, int pin, HAL_GPIO_PINMODE_ENUM pinmode, HAL_GPIO_INTRMODE_ENUM intrmode, HAL_GPIO_ISR_CB cb)
{
    nrfx_gpiote_pin_t pin_id = NRF_GPIO_PIN_MAP(port, pin);
    
    switch (pinmode)
    {
        case HAL_GPIO_PINMODE_DISABLE:
            if (!nrf52_pin_is_free(pin_id)) // Check if pin is in use. do nothing if pin not in use.
            {
                // Search for pin_id in callback list.
                uint8_t i;
                for (i = 0; i < NUM_ISR_PINS; ++i)
                {
                    if (isr_pins[i].configured && (isr_pins[i].pin_id == pin_id)) { break; }
                }
                
                /* If pin not connected to callback disable trough GPIO else
                 * else disable trough GPIOTE and remove callback.
                 */
                if (i == NUM_ISR_PINS)
                {
                    nrf_gpio_cfg_default(pin_id);
                }
                else
                {
                    nrfx_gpiote_in_event_disable(pin_id);
                    nrfx_gpiote_in_uninit(pin_id);
                    isr_pins[i].configured = false;
                }
            }
            break;
        case HAL_GPIO_PINMODE_READ:
            APP_ERROR_CHECK_BOOL(nrf52_pin_is_free(pin_id));   //Specified pin already in use by GPIO.
            if (intrmode == HAL_GPIO_INTRMODE_DISABLE)
            {
                nrf_gpio_cfg_input(pin_id, NRF_GPIO_PIN_NOPULL);
            }
            else
            {
                uint8_t i = 0;
                while (isr_pins[i].configured)
                {
                    i += 1;
                    APP_ERROR_CHECK_BOOL(i < NUM_ISR_PINS); //No free ISR handlers left.
                }
                
                nrfx_gpiote_in_config_t conf = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
                APP_ERROR_CHECK(nrfx_gpiote_in_init(pin_id, &conf, nrf52_gpio_isr));
                nrfx_gpiote_in_event_enable(pin_id, true);
                isr_pins[i] = (ISR_PIN){pin_id, cb, intrmode, port, pin, true};
            }
            break;
        case HAL_GPIO_PINMODE_WRITE:
        case HAL_GPIO_PINMODE_READWRITE:
            APP_ERROR_CHECK_BOOL(nrf52_pin_is_free(pin_id));   //Specified pin already in use by GPIO.
            nrf_gpio_cfg_output(pin_id);    
            break;
        case HAL_GPIO_PINMODE_OPENDRAIN:
            APP_ERROR_CHECK_BOOL(nrf52_pin_is_free(pin_id));   //Specified pin already in use by GPIO.
            nrf_gpio_cfg(pin_id, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT,
                            NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);    
            break;
        default:
            APP_ERROR_HANDLER(0);   //Unsupported pin mode.
    }
}

/*! \brief Set specified nRF52 pin to level (0/1).
 *
 *  \param port     Port of pin to set.
 *  \param pin      Number of pin to set.
 *  \param level    New pin output level.
 */
static void nrf52_setlevel(int port, int pin, int level)
{
    nrfx_gpiote_pin_t pin_id = NRF_GPIO_PIN_MAP(port, pin);
    
    if (nrf_gpio_pin_dir_get(pin_id) == NRF_GPIO_PIN_DIR_OUTPUT)
    {
        nrf_gpio_pin_write(pin_id, level);
    }
    else
    {
        APP_ERROR_HANDLER(0);   //Tried to set pin not configured as output.
    }
}

/*! \brief Write current input (0/1) of specified nRF52 pin to level.
 *
 *  \param port     Port of pin to check.
 *  \param pin      Number of pin to check.
 *  \param level    Pointer to which pin input level will be written.
 */
static void nrf52_getlevel(int port, int pin, int *level)
{
    nrfx_gpiote_pin_t pin_id = NRF_GPIO_PIN_MAP(port, pin);
    
    // Search for pin_id in callback list.
    uint8_t i;
    for (i = 0; i < NUM_ISR_PINS; ++i)
    {
        if (isr_pins[i].configured && (isr_pins[i].pin_id == pin_id)) { break; }
    }
        
    if (i == NUM_ISR_PINS)
    {
        /* If pin_id not in callback list check if pin configured as input. If
         * pin_id configured as input read pin level using GPIO else raise error.
         */
        if ((nrf_gpio_pin_dir_get(pin_id) == NRF_GPIO_PIN_DIR_INPUT)
                && (nrf_gpio_pin_input_get(pin_id) == NRF_GPIO_PIN_INPUT_CONNECT))
        {
            *level = nrf_gpio_pin_read(pin_id);
        }
        else
        {
            APP_ERROR_HANDLER(0);   //Tried to read pin not configured as input.
        }
    }
    else
    {
        // If pin in callback list read pin level using GPIOTE.
        *level = nrfx_gpiote_in_is_set(pin_id);
    }

}

/*! \brief Returns true if NRF52 pin with provided ID is not in use.
 *
 *  Checks if identified pin is configured as disconnected input which is the
 *  default/unconfigured state used by the NRF5 SDK for unused pins.
 *
 *  \param pin_id   ID of pin to check.
 *
 *  \returns True if pin is configured as input and disconnected. False otherwise.
 */
static bool nrf52_pin_is_free(nrfx_gpiote_pin_t pin_id)
{
    return ((nrf_gpio_pin_dir_get(pin_id) == NRF_GPIO_PIN_DIR_INPUT)
                && (nrf_gpio_pin_input_get(pin_id) == NRF_GPIO_PIN_INPUT_DISCONNECT));
}

/*! \brief GPIO hal interrupt callback.
 *
 *  Callback function called on level change for pins with a registered callback
 *  function.
 *
 *  This function is called on both positive and negative edges regardless of
 *  what was requested by the user due to the way low power GPIO interrupts
 *  function on the NRF52. This function will then find pin in callback list,
 *  check if edge is that requested by user, and if so call user supplied cb.
 *  
 *  Note that action parameter will always be NRF_GPIOTE_POLARITY_TOGGLE due to
 *  the above workaround.
 *
 *  \param pin_id   nRF5 SDK ID of pin that triggered interrupt.
 *  \param action   Always NRF_GPIOTE_POLARITY_TOGGLE.
 */
static void nrf52_gpio_isr(nrfx_gpiote_pin_t pin_id, nrf_gpiote_polarity_t action)
{
    // Search for pin_id in callback list.
    uint8_t i;
    for (i = 0; i < NUM_ISR_PINS; ++i)
    {
        if (isr_pins[i].configured && (isr_pins[i].pin_id == pin_id)) { break; }
    }
    
    // If pin_id has a callback check that triggering edge matches interrupt mode and if so call callback.
    if (i < NUM_ISR_PINS)
    {
        bool pin_set = nrfx_gpiote_in_is_set(pin_id);
        if ((isr_pins[i].intrmode == HAL_GPIO_INTRMODE_ANYEDGE)
            || (isr_pins[i].intrmode == HAL_GPIO_INTRMODE_POSEDGE && pin_set)
            || (isr_pins[i].intrmode == HAL_GPIO_INTRMODE_NEGEDGE && !pin_set))
        {
            isr_pins[i].cb(HAL_GPIO_DEVICE_SELF, isr_pins[i].port, isr_pins[i].pin);
        }
    }
    
}