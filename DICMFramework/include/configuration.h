/*! \file configuration.h
    \brief Configuration header

    Configuration parameters and data storage.
*/

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "dicm_framework_config.h"
#include "esp_attr.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "hal_gpio.h"
#include "hal_ledc.h"
#include "iGeneralDefinitions.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "sorted_list.h"
#include "sorted_list64.h"
#include <stdint.h>

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#ifdef EXT_RAM_ATTR
#undef EXT_RAM_ATTR
#define EXT_RAM_ATTR EXT_RAM_BSS_ATTR
#endif
#endif

//! \brief Common send timeout for messages from broker->connectors
#define BROKER_SEND_TIMEOUT 500  //!< \~ Time to wait until failing to send to connector

#define LOG_LEVEL_STRING_SIZE  64
#define ONBOARDING_STRING_SIZE 32
#define OTA_URL_STRING_SIZE    80
#define CONN_URL_STRING_SIZE   100
#define MAX_SKU_LENGTH         17

#ifndef STOCK_KEEPING_UNIT
#define STOCK_KEEPING_UNIT "0000000000"
#endif  // STOCK_KEEPING_UNIT

#ifndef PRODUCT_NUMBER_CODE
#define PRODUCT_NUMBER_CODE "000000000"
#endif

//! \~ Struct collecting device configuration
typedef struct
{
    uint8_t bluetooth_enabled;  //!< \~ Active bluetooth mode
    uint8_t bluetooth_up;       //!< \~ Bluetooth has started
    char default_name[15];      //!< \~ Default name if not set
    char id[6];                 //!< \~ MAC address
    char id_string[13];         //!< \~ Text representation of half of MAC address
    char firmware_version[32];  //!< \~ Firmware version
} DEVICE_INFORMATION;

// Bluetooth mode enum
typedef enum
{
    BLUETOOTH_OFF,
    BLUETOOTH_ON,
} BLUETOOTH_MODE_ENUM;

// Wifi mode enum
typedef enum
{
    WIFI_OFF,
    WIFI_ON,
} WIFI_MODE_ENUM;

typedef struct _FLASH_PARAMETER
{
    char *key;
    DDM2_TYPE_ENUM type;
    void *storage;
    uint16_t size;
} FLASH_PARAMETER;
typedef struct _FLASH_PARAMETER_STRING
{
    char *key;
    DDM2_TYPE_ENUM type;
    void **storage;
    uint16_t size;
} FLASH_PARAMETER_STRING;

// TODO remove global variables
extern uint8_t *gw0awssc;
extern uint8_t *gw0awscc;
extern uint8_t *gw0awsccpk;
extern uint8_t *gw0awsca;
extern uint8_t *gw0otasc;
extern uint8_t *gw0otacc;
extern uint8_t *gw0otaccpk;
extern uint8_t *gw0otaca;
extern uint8_t gw0dsn[ONBOARDING_STRING_SIZE];
extern uint8_t gw0sku[MAX_SKU_LENGTH + 1];
extern uint8_t gw0pnc[ONBOARDING_STRING_SIZE];
extern uint8_t gw0thing[ONBOARDING_STRING_SIZE];
extern int32_t gw0cupdt;
extern int32_t gw0batwin;
extern int32_t gw0tempwin;
extern int32_t gw0remtwin;
extern int32_t cfg0itempth;
extern int32_t cfg0wwtrth;
extern int32_t cfg0fwtrth;
extern int32_t cfg0batth;
extern int32_t cfg0ntwth;
extern int32_t cfg0dport;
extern int32_t gw0dev1st;
extern int32_t gw0ict;
extern int32_t cfg0ostat;
extern int32_t gw0upt;
extern int32_t gw0tst;
extern int32_t gw0ocnt;
extern int32_t gw0rcnt;
extern int32_t gw0thtyid;
extern char cfg0loglvl[LOG_LEVEL_STRING_SIZE];
extern char cfg0opath[OTA_URL_STRING_SIZE];
extern uint8_t gw0connurl[CONN_URL_STRING_SIZE];

uint8_t gateway_advertisement_flag_byte(void);
void gateway_factory_reset(void);
void gateway_load_configuration(void);
const char *gateway_get_firmware_version(void);
const esp_partition_t *gateway_file_system_get_unused_partition(void);
bool gateway_file_system_save_next_partition(void);
const char *gateway_get_file_system_partition_version(void);
void gateway_ota_publish_status(const int32_t new_value);
void gateway_ota_increase_count(void);

void install_file_system(void);
void get_file_system_info(size_t *total_size, size_t *used_size);

const esp_partition_t *get_log_partition(void);
const char *get_log_partition_label(void);

// Non-volatile storage (todo: should we use hal_??... hal_nvmc exists but the API is not compatible, see key type)
void set_eolpass(int32_t value);
int32_t get_eolpass(void);
int config_erase(const char *key);
int config_get_i32(const char *key, int32_t *value);
int32_t config_get_i32_def(const char *const key, const int32_t value_def);
int config_get_blob(const char *key, void *data, size_t *size);
int config_get_str(const char *key, char *str, size_t *length);
int config_set_i32(const char *key, int32_t value);
int config_set_blob(const char *key, const void *data, size_t size);
int config_set_str(const char *key, const char *str);
esp_err_t config_get_instance(const nvs_handle_t handle_nvs, const int parameter_index, const int instance, void *const storage, size_t storage_size);
esp_err_t config_set_instance(const nvs_handle_t handle_nvs, const int parameter_index, const int instance, const void *const storage, size_t storage_size);
esp_err_t config_get_id(const nvs_handle_t handle_nvs, const int parameter_index, const char *const tag, const void *const id, size_t const id_size, void *const storage, size_t *const storage_size);
esp_err_t config_set_id(const nvs_handle_t handle_nvs, const int parameter_index, const char *const tag, const void *const id, size_t id_size, void *const storage, const size_t storage_size);

#ifdef BOARD_INITIALIZATION
void board_initialization_test(void);
void board_initialization(void);
#endif  // BOARD_INITIALIZATION

#ifndef LED_R_DEFAULT_STATE
#define LED_R_DEFAULT_STATE 0
#endif
#ifndef LED_G_DEFAULT_STATE
#define LED_G_DEFAULT_STATE 0
#endif
#ifndef LED_B_DEFAULT_STATE
#define LED_B_DEFAULT_STATE 0
#endif

HAL_GPIO_RESULT_ENUM LED_R(int) __attribute__((weak));
HAL_GPIO_RESULT_ENUM LED_G(int) __attribute__((weak));
HAL_GPIO_RESULT_ENUM LED_B(int) __attribute__((weak));
HAL_GPIO_RESULT_ENUM READ_LED_R(int *) __attribute__((weak));
HAL_GPIO_RESULT_ENUM READ_LED_G(int *) __attribute__((weak));
HAL_GPIO_RESULT_ENUM READ_LED_B(int *) __attribute__((weak));

#ifdef HAL_GPIO

//! \~ Pin name definitions
#define GPIO_PIN(name, device, port, pin, pinmode, pinlevel, intrmode, cb) name##_PIN = pin,
typedef enum _HAL_GPIO_PIN_ENUM
{
    GPIO_PINS
} HAL_GPIO_PIN_ENUM;
#undef GPIO_PIN

//! \~ Read GPIO function prototypes
#define GPIO_PIN(name, device, port, pin, pinmode, pinlevel, intrmode, cb) \
    HAL_GPIO_RESULT_ENUM name(int);
GPIO_PINS
#undef GPIO_PIN

//! \~ Write GPIO function prototypes
#define GPIO_PIN(name, device, port, pin, pinmode, pinlevel, intrmode, cb) \
    HAL_GPIO_RESULT_ENUM READ_##name(int *);
GPIO_PINS
#undef GPIO_PIN

#endif  // HAL_GPIO

#ifdef HAL_LEDC_PWM

//! \~ Define Set duty cycle functions, invoke as name_set_duty(duty_cycle);
#define LEDC_PWM(name, gpio_num, duty_resolution, freq_hz, speed_mode, timer_num, clk_cfg, channel, duty, hpoint) \
    extern int name##_set_duty(uint32_t duty_cycle);
LEDC_CONFIGURATION
#undef LEDC_PWM

#endif /* HAL_LEDC_PWM */

#ifndef MULTIBROKER_BROKER_BITS
#define MULTIBROKER_BROKER_BITS 4
#endif

#ifndef MULTIBROKER
#define MULTIBROKER 0
#endif

extern DEVICE_INFORMATION device_information;
extern nvs_handle_t nvs;
extern EventGroupHandle_t network_events;
extern EventGroupHandle_t system_events;
extern const int32_t Minone;
extern const int32_t Zero;
extern const int32_t One;

#endif  // CONFIGURATION_H_
