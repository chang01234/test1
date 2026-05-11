/*! \file hal_gpio.c
    \brief GPIO Hardware Abstraction Layer
*/

#include "hal_gpio.h"

#include "dicm_framework_config.h"
#include "driver/gpio.h"
#include "iGeneralDefinitions.h"

#ifdef HAL_GPIO

#include "esp_idf_version.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#ifdef EXT_RAM_ATTR
#undef EXT_RAM_ATTR
#define EXT_RAM_ATTR EXT_RAM_BSS_ATTR
#endif  // EXT_RAM_ATTR
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

#if !defined(CONFIG_IDF_TARGET_LINUX)
#if defined(DEVICE_PCA9675) || defined(DEVICE_PCA9675_B)
#include "drv_pca9675.h"
#endif
#ifdef DEVICE_TCA6424A
#include "drv_tca6424a.h"
#endif
#ifdef DEVICE_TCA9554A
#include "drv_tca9554a.h"
#endif
#endif  // !defined(CONFIG_IDF_TARGET_LINUX)

extern const HAL_GPIO_PINLIST Gpio_pinlist[];  //!< \~ Master HAL GPIO pin list
extern const int Gpio_pinlist_size;            //!< \~ Need separate size as it is exported from configuration c source
static QueueHandle_t hal_gpio_isr_queue;       //!< \~ Queue of pins between isr and callback task

typedef struct
{
    uint32_t key;
    HAL_GPIO_ISR_CB cb;
} gpio_cb_entry_t;

typedef struct
{
    gpio_cb_entry_t table[CB_TABLE_SIZE];
    uint32_t num;
} gpio_cb_table_t;

static EXT_RAM_ATTR gpio_cb_table_t gpio_cb_table;

//! \~ Define write GPIO functions, invoke as PIN_NAME(level);
#define GPIO_PIN(name, device, port, pin, pinmode, pinlevel, intrmode, cb) \
    HAL_GPIO_RESULT_ENUM name(int level)                                   \
    {                                                                      \
        return hal_gpio_setlevel(device, port, pin, level);                \
    };
GPIO_PINS
#undef GPIO_PIN

//! \~ Define read GPIO functions, invoke as READ_PIN_NAME(level);
#define GPIO_PIN(name, device, port, pin, pinmode, pinlevel, intrmode, cb) \
    HAL_GPIO_RESULT_ENUM READ_##name(int *level)                           \
    {                                                                      \
        return hal_gpio_getlevel(device, port, pin, level);                \
    };
GPIO_PINS
#undef GPIO_PIN

//! \~ Associate a GPIO pin to an interrupt callback function
static void add_gpio_cb(HAL_GPIO_DEVICE_ENUM device, int port, int pin, HAL_GPIO_ISR_CB gpio_cb)
{
    // return sorted_list_unique_add(&cb_table, (device << 16) + (port << 8) + pin, (uint32_t)gpio_cb);
    uint32_t key = (uint32_t)((device << 16) + (port << 8) + pin);
    for (uint32_t i = 0; i < CB_TABLE_SIZE; i++)
    {
        // First first empty slot
        if (gpio_cb_table.table[i].key == 0u)
        {
            gpio_cb_table.table[i].key = key;
            gpio_cb_table.table[i].cb = gpio_cb;
            gpio_cb_table.num++;
            break;
        }
    }
}

//! \~ Dispatch a callback to routine associated to pin
static void dispatch_gpio_cb(const int device, const int port, const int pin)
{

    uint32_t gpio_key = (device << 16) + (port << 8) + pin;
    for (uint32_t i = 0; i < gpio_cb_table.num; i++)
    {
        if (gpio_cb_table.table[i].key == gpio_key)
        {
            // Found match
            if (gpio_cb_table.table[i].cb != NULL)
            {
                gpio_cb_table.table[i].cb(device, port, pin);
            }
            break;
        }
    }
    return;
}

//! \~ Task receiving interrupt request pins from GPIO ISR and dispatching the respective callback functions
static void hal_gpio_isr_task(void *arg)
{
    uint8_t isr_pin;
    int device;

#if defined(DEVICE_PCA9675) || defined(DEVICE_PCA9675_B)
    int port, pin;
    uint16_t changed_pins;
#endif

#ifdef DEVICE_PCA9675
    uint16_t current_levels;
    // Read current values so that we can know whether any pin has changed
    pca9675_getlevels(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS, &current_levels);
#endif

#ifdef DEVICE_PCA9675_B
    uint16_t current_levels_B;
    // Read current values so that we can know whether any pin has changed
    pca9675_getlevels(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS_B, &current_levels_B);
#endif

#ifdef DEVICE_TCA6424A
    int port, pin;
    int pin_level;
    uint32_t changed_pin;
    uint32_t current_level;
    // Read current values so that we can know whether any pin has changed
    tca6424_getlevels(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, &current_level);
#endif

    while (1)
    {
        TRUE_CHECK(xQueueReceive(hal_gpio_isr_queue, &isr_pin, portMAX_DELAY));
        switch (isr_pin)
        {
#if defined(DEVICE_PCA9675) || defined(DEVICE_PCA9675_B)
        case DEVICE_PCA9675_INT_PIN:
#ifdef DEVICE_PCA9675
            device = HAL_GPIO_DEVICE_PCA9675;
            ZERO_CHECK(pca9675_changedpins(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS, &current_levels, &changed_pins));
            for (int bit = 0; changed_pins; changed_pins >>= 1, bit++)
            {
                if (changed_pins & 1)  // pin level change detected
                {
                    port = bit >> 3;
                    pin = bit & 0x07;
                    dispatch_gpio_cb(device, port, pin);
                }
            }
#endif
#ifdef DEVICE_PCA9675_B
            device = HAL_GPIO_DEVICE_PCA9675_B;
            ZERO_CHECK(pca9675_changedpins(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS_B, &current_levels_B, &changed_pins));
            for (int bit = 0; changed_pins; changed_pins >>= 1, bit++)
            {
                if (changed_pins & 1)  // pin level change detected
                {
                    port = bit >> 3;
                    pin = bit & 0x07;
                    dispatch_gpio_cb(device, port, pin);
                }
            }
#endif
            break;
#endif

#ifdef DEVICE_TCA6424A
        case DEVICE_TCA6424A_INT_PIN:
            // printf("Interrupt Get\n");
            device = HAL_GPIO_DEVICE_TCA6424A;
            ZERO_CHECK(tca6424_changedpins(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, &current_level, &changed_pin));
            for (int bit = 0; changed_pin; changed_pin >>= 1, bit++)
            {
                if (changed_pin & 1)  // pin level change detected
                {
                    port = bit >> 3;
                    pin = bit & 0x07;
                    dispatch_gpio_cb(device, port, pin);
                }
            }
            port = 1;

            tca6424_getlevel(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, port, 6, &pin_level);
            if (pin_level)
            {
                pin = 6;
                dispatch_gpio_cb(device, port, pin);
            }

            tca6424_getlevel(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, port, 7, &pin_level);
            if (pin_level)
            {
                pin = 7;
                dispatch_gpio_cb(device, port, pin);
            }

            tca6424_getlevel(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, port, 0, &pin_level);
            if (pin_level)
            {
                pin = 0;
                dispatch_gpio_cb(device, port, pin);
            }
            break;
#endif

        default:
            device = HAL_GPIO_DEVICE_ESP32;
            dispatch_gpio_cb(device, 0, isr_pin);
        }
    }
}

//! \~ GPIO ISR, forwards pin to hal_gpio_isr_task()
static void IRAM_ATTR hal_gpio_isr(void *arg)
{
    xQueueSendFromISR(hal_gpio_isr_queue, &arg, NULL);
}

//! \~ Automatically initialize configured pins
static void initialize_gpio_pins(void)
{
    for (int i = 0; i < Gpio_pinlist_size; i++)
    {
        ZERO_CHECK(hal_gpio_pinsetup(Gpio_pinlist[i].device, Gpio_pinlist[i].port, Gpio_pinlist[i].pin, Gpio_pinlist[i].pinmode, Gpio_pinlist[i].intrmode, Gpio_pinlist[i].cb));
        ZERO_CHECK(hal_gpio_setlevel(Gpio_pinlist[i].device, Gpio_pinlist[i].port, Gpio_pinlist[i].pin, Gpio_pinlist[i].level));
    }
}

//! \~ Initialize GPIO HAL
HAL_GPIO_RESULT_ENUM hal_gpio_init(void)
{
    ZERO_CHECK(gpio_install_isr_service(0));
    TRUE_CHECK((hal_gpio_isr_queue = xQueueCreate(16, sizeof(uint8_t))) != NULL);
    initialize_gpio_pins();
    TRUE_CHECK(xTaskCreate(hal_gpio_isr_task, "gpio", 2048, NULL, 10, NULL));
#if defined(DEVICE_PCA9675) || defined(DEVICE_PCA9675_B)
    ZERO_CHECK(gpio_isr_handler_add(DEVICE_PCA9675_INT_PIN, hal_gpio_isr, (void *)DEVICE_PCA9675_INT_PIN));
#endif
#ifdef DEVICE_TCA6424A
    // ZERO_CHECK(gpio_isr_handler_add(DEVICE_TCA6424A_INT_PIN, hal_gpio_isr, (void*)DEVICE_TCA6424A_INT_PIN));
#endif
    return HAL_GPIO_OK;
}

//! \~ Configure a pin
HAL_GPIO_RESULT_ENUM hal_gpio_pinsetup(HAL_GPIO_DEVICE_ENUM device, int port, int pin, const uint8_t pinmode, HAL_GPIO_INTRMODE_ENUM intrmode, HAL_GPIO_ISR_CB cb)
{
    gpio_config_t io_conf;
    if (cb)
    {
        add_gpio_cb(device, port, pin, cb);
    }

    switch (device)
    {
    case HAL_GPIO_DEVICE_ESP32:
        ASSERT(!port);
        io_conf.intr_type = (gpio_int_type_t)intrmode;
        io_conf.pin_bit_mask = 1ULL << pin;
        io_conf.mode = (pinmode & HAL_GPIO_PIN_MODE_MASK);
        io_conf.pull_down_en = (pinmode & HAL_GPIO_PIN_PULLUP_DOWN_MASK) == HAL_GPIO_PULLDOWN_ENABLE ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = (pinmode & HAL_GPIO_PIN_PULLUP_DOWN_MASK) == HAL_GPIO_PULLUP_ENABLE ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
        ZERO_CHECK(gpio_config(&io_conf));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
        // Ignore warning
        if (intrmode)
        {
            ZERO_CHECK(gpio_isr_handler_add(pin, (gpio_isr_t)hal_gpio_isr, (void *)pin));
        }
#pragma GCC diagnostic pop
        break;
#ifdef DEVICE_PCA9675
    case HAL_GPIO_DEVICE_PCA9675:
        break;
#endif
#ifdef DEVICE_PCA9675_B
    case HAL_GPIO_DEVICE_PCA9675_B:
        break;
#endif
#ifdef DEVICE_TCA6424A
    case HAL_GPIO_DEVICE_TCA6424A:
        tca6424_configure_pin(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, port, pin, pinmode);
        break;
#endif
#ifdef DEVICE_TCA9554A
    case HAL_GPIO_DEVICE_TCA9554A:
        tca9554a_configure_pin(DEVICE_TCA9554A_I2C_PORT, DEVICE_TCA9554A_ADDRESS, pin, pinmode);
        break;
#endif
    default:
        return HAL_GPIO_ERROR_DEVICE_UNAVAILABLE;
    }

    return HAL_GPIO_OK;
}

#endif  // HAL_GPIO

//! \~ Set output level of pin
HAL_GPIO_RESULT_ENUM hal_gpio_setlevel(HAL_GPIO_DEVICE_ENUM device, MAYBE_UNUSED int port, int pin, int level)
{
    switch (device)
    {
    case HAL_GPIO_DEVICE_ESP32:
        if (GPIO_IS_VALID_OUTPUT_GPIO(pin))
        {
            ZERO_CHECK(gpio_set_level(pin, level));
        }
        break;
#ifdef DEVICE_PCA9675
    case HAL_GPIO_DEVICE_PCA9675:
        ZERO_CHECK(pca9675_setlevel(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS, port, pin, level));
        break;
#endif
#ifdef DEVICE_PCA9675_B
    case HAL_GPIO_DEVICE_PCA9675_B:
        ZERO_CHECK(pca9675_setlevel(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS_B, port, pin, level));
        break;
#endif
#ifdef DEVICE_TCA6424A
    case HAL_GPIO_DEVICE_TCA6424A:
        tca6424_setlevel(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, port, pin, level);
        break;
#endif
#ifdef DEVICE_TCA9554A
    case HAL_GPIO_DEVICE_TCA9554A:
        tca9554a_setlevel(DEVICE_TCA9554A_I2C_PORT, DEVICE_TCA9554A_ADDRESS, pin, level);
        break;
#endif
    default:
        return HAL_GPIO_ERROR_DEVICE_UNAVAILABLE;
    }

    return HAL_GPIO_OK;
}

//! \~ Get level of pin
HAL_GPIO_RESULT_ENUM hal_gpio_getlevel(HAL_GPIO_DEVICE_ENUM device, int port, int pin, int *const level)
{
    switch (device)
    {
    case HAL_GPIO_DEVICE_ESP32:
        *level = gpio_get_level(pin);
        break;
#ifdef DEVICE_PCA9675
    case HAL_GPIO_DEVICE_PCA9675:
        ZERO_CHECK(pca9675_getlevel(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS, port, pin, level));
        break;
#endif
#ifdef DEVICE_PCA9675_B
    case HAL_GPIO_DEVICE_PCA9675_B:
        ZERO_CHECK(pca9675_getlevel(DEVICE_PCA9675_I2C_PORT, DEVICE_PCA9675_ADDRESS_B, port, pin, level));
        break;
#endif
#ifdef DEVICE_TCA6424A
    case HAL_GPIO_DEVICE_TCA6424A:
        tca6424_getlevel(DEVICE_TCA6424A_I2C_PORT, DEVICE_TCA6424A_ADDRESS, port, pin, level);
        break;
#endif
#ifdef DEVICE_TCA9554A
    case HAL_GPIO_DEVICE_TCA9554A:
        tca9554a_getlevel(DEVICE_TCA9554A_I2C_PORT, DEVICE_TCA9554A_ADDRESS, pin, (uint8_t *)level);
        break;
#endif
    default:
        return HAL_GPIO_ERROR_DEVICE_UNAVAILABLE;
    }

    return HAL_GPIO_OK;
}
