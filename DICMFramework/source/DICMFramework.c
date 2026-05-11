/*
 * DICMFramework.c
 *
 *  Created on: 31 jan. 2022
 *      Author: Andlun
 */

#include "DICMFramework.h"
#include "configuration.h"

#include "DICMVersion.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#ifndef CONFIG_IDF_TARGET_LINUX
#include "esp_flash_encrypt.h"
#endif
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"
#ifdef CONFIG_NVS_ENCRYPTION
#include "nvs_sec_provider.h"
#endif

#include "broker.h"
#ifdef CONFIG_POWER_MANAGEMENT
#include "power_management.h"
#endif
#include "hal_initialize.h"
#include "load_configurations.h"
#include "rtc_watchdog.h"
#ifdef CONFIG_LOG_SERVICE
#include "dicm_log_service.h"
#endif
#ifdef CONFIG_IDF_TARGET_LINUX
#include "linux_dicmframework_setup.h"
#endif

static void flash_encryption_setup(void);

void DICMFramework_start(void)
{
    LOG(C, "Booting " FIRMWARE_STRING);
    LOG(C, "DICM Framework %s", GIT_DICM_VERSION);
    LOG(I, "Heap left: %" PRIu32 ", stack left: %" PRIu32 "", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));
    LOG(I, "Core: %d", xPortGetCoreID());

#ifdef CONFIG_POWER_MANAGEMENT
    /* Important that power management is initialized before wifi & ble is started. */
    initialize_power_management();
#endif

    flash_encryption_setup();

    hal_initialize();
    LED_R(LED_R_DEFAULT_STATE);
    LED_G(LED_G_DEFAULT_STATE);
    LED_B(LED_B_DEFAULT_STATE);

#ifndef CONFIG_IDF_TARGET_LINUX
    LOG(I, "Initializing RTC watchdog...");
    rtc_watchdog_init();
#endif

#ifdef CONFIG_IDF_TARGET_LINUX
    linux_dicmframework_setup();
#endif
#ifdef EOL_TESTING
    if (!readEOLgpio())
    {
        initialize_console();
        initialize_Accelerometer();
        initialize_PMIC();
        intialize_bg95_uart_eol();
        start_console();
        return;
    }
#endif

#ifdef BOARD_INITIALIZATION
    LOG(I, "Initializing board...");
    board_initialization();
#endif

    LOG(I, "Loading configuration from flash");
    gateway_load_configuration();

    install_file_system();

#if defined(CONNECTOR_WIFI) || defined(IOT_MODEM) || defined(IOT_MODEM_OLD) || defined(CONFIG_ETH_USE_ESP32_EMAC)
    LOG(I, "Initializing TCP/IP stack...");
    ZERO_CHECK(esp_event_loop_create_default());
    TRUE_CHECK((network_events = xEventGroupCreate()) != NULL);
    esp_netif_init();
#endif

    TRUE_CHECK((system_events = xEventGroupCreate()) != NULL);

    LOG(I, "Initializing broker");
    initialize_broker();

#ifdef CONFIG_LOG_SERVICE
    dicm_log_service_initialize();
#endif

#ifdef LOAD_CONFIGURATIONS
    LOG(I, "Loading configurations...");
#endif
    load_configurations();

#if !defined(CONFIG_DICM_TARGET_LMC_GW_1)
    // Indicate that system has started
    xEventGroupSetBits(system_events, SYSTEM_START_BIT);
#endif

#ifdef APPL_INITIALIZATION
    extern void appl_initialization(void);
    LOG(I, "Application init...");
    appl_initialization();
#endif

    LOG(I, "System up! Heap left: %" PRIu32 ", stack left: %" PRIu16 "", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));
}

static void flash_encryption_setup(void)
{
#ifndef CONFIG_IDF_TARGET_LINUX
    esp_flash_enc_mode_t mode = esp_get_flash_encryption_mode();

    if (mode != ESP_FLASH_ENC_MODE_DISABLED)
    {
        LOG(I, "Flash Encryption: %s", (mode == ESP_FLASH_ENC_MODE_RELEASE) ? "RELEASE" : "DEVELOPMENT");
#ifdef SECURE_PLATFORM_PRODUCTION
        if (mode != ESP_FLASH_ENC_MODE_RELEASE)
        {
            LOG(W, "Enabling Release flash encryption!");
            esp_flash_encryption_set_release_mode();
        }
#endif
    }
#endif
}
