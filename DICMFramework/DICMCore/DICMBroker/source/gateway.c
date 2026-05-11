/*
 * gateway.c
 *
 *  Created on: 3 feb. 2022
 *      Author: Andlun
 */

#include <stdint.h>
#include <string.h>

#include "configuration.h"

#if CONFIG_IDF_TARGET_ESP32C2
#define GW_NOT_SUPPORT_GW0OTA
#define GW_NOT_SUPPORT_GW0ICT
#define GW_NOT_SUPPORT_GW0SKU_MDNS_UPDATE
#define GW_NOT_SUPPORT_JUMBO_FRAME
#endif

#include "broker.h"
#ifndef GW_NOT_SUPPORT_GW0ICT
#include "conf_funcs.h"
#endif
#include "connector.h"
#ifndef GW_NOT_SUPPORT_JUMBO_FRAME
#include "esp_rom_md5.h"
#endif
#include "esp_system.h"
#include "gateway.h"
#include "hal_cpu.h"
#ifndef GW_NOT_SUPPORT_GW0OTA
#include "ota.h"
#endif
#include "utils.h"

#if !defined(CONFIG_IDF_TARGET_LINUX) && !defined(GW_NOT_SUPPORT_GW0SKU_MDNS_UPDATE)
#include "network_discovery.h"
#endif

#define MINUTE_TO_MICROSECONDS (60000000)
#define GATEWAY_UPTIME_FACTOR  (MINUTE_TO_MICROSECONDS / Ddm2_unit_factor_list[DDM2_UNIT_MINUTE])

#ifndef GW_NOT_SUPPORT_JUMBO_FRAME
static EXT_RAM_ATTR uint8_t jumbo_frame[DDMP2_JUMBO_FRAME_MAX_SIZE];
static size_t jumbo_size;
static int gateway_finalize_jumbo_frame(const DDMP2_FRAME *pframe);

static const uint8_t **const certificates[] = {
    (const uint8_t **)&gw0awssc,
    (const uint8_t **)&gw0awscc,
    (const uint8_t **)&gw0awsccpk,
    (const uint8_t **)&gw0awsca,
    (const uint8_t **)&gw0otasc,
    (const uint8_t **)&gw0otacc,
    (const uint8_t **)&gw0otaccpk,
    (const uint8_t **)&gw0otaca,
};

static const char *cert_keys[] = {
    "gw0awssc",
    "gw0awscc",
    "gw0awsccpk",
    "gw0awsca",
    "gw0otasc",
    "gw0otacc",
    "gw0otaccpk",
    "gw0otaca",
};
#endif

static const char *default_ota_path = "file";

#define GATEWAY_FACTORY_RESET_ID         0
#define GATEWAY_RESTART_ID               1
#define GATEWAY_FACTORY_RESET_TIMEOUT_MS 4000
#define GATEWAY_RESTART_TIMEOUT_MS       4000

static EXT_RAM_ATTR TimerHandle_t gateway_restart_timer;
static EXT_RAM_ATTR TimerHandle_t factory_reset_timer;

static void gateway_reset_command_timeout(TimerHandle_t xTimer);

#ifndef GW_NOT_SUPPORT_JUMBO_FRAME
static void gateway_reply_direct(const uint32_t parameter, const uint8_t connector, const void *const value, const size_t value_size);
#endif
static void gateway_accept_and_publish_int32(const DDMP2_FRAME *const pframe, int32_t *const value_store, const char *const key);

#ifndef GW_NOT_SUPPORT_JUMBO_FRAME
static void md5(unsigned char hash[16], const void *const data, const size_t data_size)
{
    ASSERT(hash != NULL)
    ASSERT(data != NULL)

    md5_context_t md5context;

    esp_rom_md5_init(&md5context);
    esp_rom_md5_update(&md5context, data, data_size);
    esp_rom_md5_final(hash, &md5context);
}

//! \~ create a reply to a subscription to the gateway
static void gateway_reply_direct(const uint32_t parameter, const uint8_t connector, const void *const value, const size_t value_size)
{
    if (value_size)
    {
        TRUE_CHECK_RETURN(value != NULL)
    }
    connector_send_frame_to_connector(DDMP2_CONTROL_PUBLISH, parameter, value, value_size, connector, BROKER_SEND_TIMEOUT);
}
#endif

static void gateway_accept_and_publish_int32(const DDMP2_FRAME *const pframe, int32_t *const value_store, const char *const key)
{
    TRUE_CHECK_RETURN(pframe != NULL);
    TRUE_CHECK_RETURN(value_store != NULL);

    int changed = pframe->frame.set.value.int32 != *value_store;

    if (changed)
    {
        *value_store = pframe->frame.set.value.int32;
        gateway_publish(pframe->frame.set.parameter, value_store, sizeof(int32_t));
        config_set_i32(key, *value_store);
    }
}

static void gateway_loglevel(const DDMP2_FRAME *const pframe)
{
    char log_level_config[LOG_LEVEL_STRING_SIZE] = {0};
    char log_level_tag[LOG_LEVEL_STRING_SIZE] = {0};
    bool is_valid_log_level = true;
    int level = 0;

    ddmp2_extract_string_from_frame(pframe, log_level_config, sizeof(log_level_config));
    if (sscanf(log_level_config, "%[^:]:%d", log_level_tag, &level) != 2)
    {
        LOG(E, "Failed to parse name and number from string");
        is_valid_log_level = false;
    }

    if ((level < ESP_LOG_NONE) || (level > ESP_LOG_VERBOSE))
    {
        LOG(E, "Invalid log level: %d", level);
        is_valid_log_level = false;
    }

    if (is_valid_log_level &&
        memcmp(cfg0loglvl, log_level_config, strlen(log_level_config)))
    {
        LOG(D, "Current log level: %s", cfg0loglvl);
        LOG(D, "Log level config: %s", log_level_config);
        LOG(D, "Log level: tag=%s level=%d", log_level_tag, level);
        esp_log_level_set(log_level_tag, level);
        snprintf(cfg0loglvl, sizeof(cfg0loglvl), "%s", log_level_config);
    }

    gateway_publish(pframe->frame.set.parameter, cfg0loglvl, strlen(cfg0loglvl));
}

void gateway_accept_and_publish_string(const DDMP2_FRAME *const pframe, uint8_t *const value_store, const char *const key, const size_t store_capacity)
{
    ASSERT(pframe != NULL);
    ASSERT(value_store != NULL);

    int value_size = (int)MIN(ddmp2_value_size(pframe), store_capacity - 1);
    int store_size = strlen((char *)value_store);
    int changed = (value_size != store_size) || memcmp((char *)pframe->frame.set.value.raw, (char *)value_store, value_size);

    if (changed)
    {
        memcpy(value_store, pframe->frame.set.value.raw, value_size);
        value_store[value_size] = '\0';
        gateway_publish(pframe->frame.set.parameter, value_store, value_size);
        config_set_str(key, (char *)value_store);
    }
}

#ifndef GW_NOT_SUPPORT_JUMBO_FRAME
static int gateway_finalize_jumbo_frame(const DDMP2_FRAME *pframe)
{
    DDMP2_JUMBO_FRAME *pjumboframe = (DDMP2_JUMBO_FRAME *)jumbo_frame;

    jumbo_frame[jumbo_size] = '\0';

    ESP_LOG_BUFFER_HEXDUMP("jumbo", jumbo_frame, jumbo_size, ESP_LOG_DEBUG);

    if (jumbo_size < (DDMP2_PARAMETER_SIZE))
    {
        LOG(E, "Jumbo frame too small! (%u)", jumbo_size);
        return 0;
    }

    // For optimization reasons, we disregard all GW0OTAXX parameters
#if 0
	if ((pjumboframe->parameter < GW0AWSSC) || (pjumboframe->parameter > GW0OTACA))
#else
    if ((pjumboframe->parameter < GW0AWSSC) || (pjumboframe->parameter > GW0AWSCA))
#endif
    {
        LOG(E, "Jumbo frame not for a valid parameter! (%08x)", pjumboframe->parameter);
        return 0;
    }

    LOG(I, "Accepting %u byte jumbo frame for parameter %08x", jumbo_size, pjumboframe->parameter);

    config_set_str(cert_keys[pjumboframe->parameter - GW0AWSSC], (char *)pjumboframe->value);

    memcpy((void *)(*(certificates[pjumboframe->parameter - GW0AWSSC])), pjumboframe->value, jumbo_size - DDMP2_PARAMETER_SIZE + 1);
    unsigned char hash[16];

    md5(hash, pjumboframe->value, jumbo_size - DDMP2_PARAMETER_SIZE);
    gateway_reply_direct(pjumboframe->parameter, pframe->source_connector, hash, sizeof(hash) - 1);  // make fit in 20 bytes

    return 1;
}
#endif

static void gateway_reset_command_timeout(TimerHandle_t xTimer)
{
    int32_t timer_id = (int32_t)pvTimerGetTimerID(xTimer);
    if (timer_id == GATEWAY_FACTORY_RESET_ID)
    {
        gateway_factory_reset();
    }
    else if (timer_id == GATEWAY_RESTART_ID)
    {
        hal_cpu_reset(HALCPU_RESET_FLAG_NONE);
    }
}

int gateway_handle_fragment_frame(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL);

#ifndef GW_NOT_SUPPORT_JUMBO_FRAME
    if (pframe->frame.fragment.offset == 0xffff)
    {
        return gateway_finalize_jumbo_frame(pframe);
    }

    size_t value_size = ddmp2_value_size(pframe);

    jumbo_size = value_size + pframe->frame.fragment.offset;

    if (jumbo_size >= DDMP2_JUMBO_FRAME_MAX_SIZE)
    {
        LOG(E, "Jumbo frame too big!");
        return 0;
    }

    LOG(I, "Accepting %u byte fragment at offset %hu, %u bytes transferred", value_size, pframe->frame.fragment.offset, jumbo_size);

    memcpy(&jumbo_frame[pframe->frame.fragment.offset], pframe->frame.fragment.value, value_size);
#endif

    return 1;
}

void gateway_handle_set(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL);

    switch (pframe->frame.set.parameter)
    {
    case GW0OTA:
    #ifndef GW_NOT_SUPPORT_GW0OTA
        char file_name[DDMP2_MAX_VALUE_SIZE + 1] = {0};
        char url[DDMP2_MAX_VALUE_SIZE + 1] = {0};

        gateway_ota_increase_count();

        memcpy(file_name, pframe->frame.set.value.raw, pframe->frame_size - 5);
        if (strstr(file_name, "http") == NULL)
        {
            // Only filename provided
            strcpy(url, cfg0opath);
            strcat(url, file_name);
        }
        else
        {
            // Full path provided
            memcpy(url, pframe->frame.set.value.raw, pframe->frame_size - 5);
        }
        gateway_ota(url);
    #endif
        break;
    case GW0TST:
        gw0tst = pframe->frame.set.value.int32;
        gateway_publish(pframe->frame.set.parameter, &gw0tst, sizeof(gw0tst));
        break;
    case GW0DSN:
        gateway_accept_and_publish_string(pframe, gw0dsn, "gw0dsn", sizeof(gw0dsn));
        break;
    case GW0SKU:
        gateway_accept_and_publish_string(pframe, gw0sku, "gw0sku", sizeof(gw0sku));
#if !defined(CONFIG_IDF_TARGET_LINUX) && !defined(GW_NOT_SUPPORT_GW0SKU_MDNS_UPDATE)
        network_discovery__mdns_update("sku", (char *)gw0sku);
#endif
        break;
    case GW0PNC:
        gateway_accept_and_publish_string(pframe, gw0pnc, "gw0pnc", sizeof(gw0pnc));
        break;
    case GW0THING:
        gateway_accept_and_publish_string(pframe, gw0thing, "gw0thing", sizeof(gw0thing));
        break;
    case GW0THTYID:
        gateway_accept_and_publish_int32(pframe, &gw0thtyid, "gw0thtyid");
        break;
    case GW0CUPDT:
        gateway_accept_and_publish_int32(pframe, &gw0cupdt, "gw0cupdt");
        break;
    case GW0BATWIN:
        gateway_accept_and_publish_int32(pframe, &gw0batwin, "gw0batwin");
        break;
    case GW0TEMPWIN:
        gateway_accept_and_publish_int32(pframe, &gw0tempwin, "gw0tempwin");
        break;
    case GW0REMTWIN:
        gateway_accept_and_publish_int32(pframe, &gw0remtwin, "gw0remtwin");
        break;
    case GW0CUPD:
    {
        switch (pframe->frame.set.value.int32)
        {
        case GW0CUPD_CLOUD_UPDATE:
#ifdef CONNECTOR_MQTT
            LOG(W, "Forcing cloud update!");
            connector_send_frame_to_broker(DDMP2_CONTROL_SET, MQTT0UPDATE, &One, sizeof(One), 0, BROKER_SEND_TIMEOUT);
            gateway_publish_int32(pframe->frame.set.parameter, GW0CUPD_ACKNOWLEDGED);
#endif  // CONNECTOR_MQTT
            break;
        case GW0CUPD_RESTART:
            LOG(W, "Restarting due to request!");
            gateway_publish_int32(pframe->frame.set.parameter, GW0CUPD_ACKNOWLEDGED);
#ifdef CONNECTOR_MQTT
            connector_send_frame_to_broker(DDMP2_CONTROL_SET, MQTT0UPDATE, &One, sizeof(One), 0, BROKER_SEND_TIMEOUT);
#endif  // CONNECTOR_MQTT
            if (gateway_restart_timer == 0)
            {
                const int32_t timer_id = GATEWAY_RESTART_ID;
                TRUE_CHECK((gateway_restart_timer = xTimerCreate(NULL, pdMS_TO_TICKS(GATEWAY_RESTART_TIMEOUT_MS), pdFALSE, (void *)timer_id, gateway_reset_command_timeout)) != NULL);
            }
            TRUE_CHECK(xTimerReset(gateway_restart_timer, portMAX_DELAY));
            break;
        case GW0CUPD_FACTORY_RESET:
        {
            LOG(W, "Factory reset received, restarting in %d s!", GATEWAY_FACTORY_RESET_TIMEOUT_MS / 1000);
            gateway_publish_int32(pframe->frame.set.parameter, GW0CUPD_FACTORY_RESET);
#ifdef CONNECTOR_MQTT
            connector_send_frame_to_broker(DDMP2_CONTROL_SET, MQTT0UPDATE, &One, sizeof(One), 0, BROKER_SEND_TIMEOUT);
#endif  // CONNECTOR_MQTT

            //  Create a start factory reset timer
            if (factory_reset_timer == 0)
            {
                const int32_t timer_id = GATEWAY_FACTORY_RESET_ID;
                TRUE_CHECK((factory_reset_timer = xTimerCreate(NULL, pdMS_TO_TICKS(GATEWAY_FACTORY_RESET_TIMEOUT_MS), pdFALSE, (void *)timer_id, gateway_reset_command_timeout)) != NULL);
            }
            TRUE_CHECK(xTimerReset(factory_reset_timer, portMAX_DELAY));
            break;
        }
        case GW0CUPD_TEST_TRIGGER:
            gateway_publish_int32(pframe->frame.set.parameter, GW0CUPD_ACKNOWLEDGED);
            break;
        }
        break;
    }
    case GW0DEV1ST:
        gateway_update_and_publish_int32(GW0DEV1ST, pframe->frame.set.value.int32, &gw0dev1st);
        break;
    case GW0ICT:
        // Check again
        xEventGroupClearBits(network_events, INTERNET_BIT);
    #ifndef GW_NOT_SUPPORT_GW0ICT
        gw0ict = (int32_t)check_internet_connection();
    #else
        gw0ict = Zero;
    #endif
        gw0ict = !!gw0ict;  // Normalize
        if (gw0ict)
        {
            xEventGroupSetBits(network_events, INTERNET_BIT);
        }
        gateway_publish(GW0ICT, &gw0ict, sizeof(int32_t));
        break;
    case GW0CONNURL:
        gateway_accept_and_publish_string(pframe, gw0connurl, "gw0connurl", sizeof(gw0connurl));
        break;
    case CFG0ITEMPTH:
        gateway_accept_and_publish_int32(pframe, &cfg0itempth, "cfg0itempth");
        break;
    case CFG0WWTRTH:
        gateway_accept_and_publish_int32(pframe, &cfg0wwtrth, "cfg0wwtrth");
        break;
    case CFG0FWTRTH:
        gateway_accept_and_publish_int32(pframe, &cfg0fwtrth, "cfg0fwtrth");
        break;
    case CFG0BATTH:
        gateway_accept_and_publish_int32(pframe, &cfg0batth, "cfg0batth");
        break;
    case CFG0OPATH:
        gateway_accept_and_publish_string(pframe, (uint8_t *)cfg0opath, "cfg0opath", sizeof(cfg0opath));
        break;
    case CFG0NTWTH:
        gateway_accept_and_publish_int32(pframe, &cfg0ntwth, "cfg0ntwth");
        break;
    case CFG0DPORT:
        gateway_accept_and_publish_int32(pframe, &cfg0dport, "cfg0dport");
        break;
    case CFG0LOGLVL:
        gateway_loglevel(pframe);
        break;
    default:
        break;
    }
}

void gateway_handle_subscribe(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL);

    switch (pframe->frame.subscribe.parameter)
    {
    case GW0INV:
        broker_serve_inventory(pframe);
        return;
    case GW0VER:
        gateway_reply(pframe, device_information.firmware_version, strlen(device_information.firmware_version));
        break;
    case GW0UPT:
        gateway_publish_int32(GW0UPT, (int32_t)(hal_cpu_get_micros() / GATEWAY_UPTIME_FACTOR));
        break;
    case GW0OTA:
        gateway_reply(pframe, default_ota_path, sizeof(default_ota_path));
        break;
    case GW0TST:
        gateway_reply_int32(pframe, gw0tst);
        break;
    case GW0DSN:
        gateway_reply(pframe, gw0dsn, strlen((char *)gw0dsn));
        break;
    case GW0SKU:
        gateway_reply(pframe, gw0sku, strlen((char *)gw0sku));
        break;
    case GW0PNC:
        gateway_reply(pframe, gw0pnc, strlen((char *)gw0pnc));
        break;
    case GW0MAC:
        gateway_reply(pframe, device_information.id, sizeof(device_information.id));
        break;
    case GW0THING:
        gateway_reply(pframe, gw0thing, strlen((char *)gw0thing));
        break;
    case GW0THTYID:
        gateway_reply_int32(pframe, gw0thtyid);
        break;
    case GW0PTYPE:
        gateway_reply_int32(pframe, DEFAULT_PRODUCT_ID);
        break;
    case GW0CUPDT:
        gateway_reply_int32(pframe, gw0cupdt);
        break;
    case GW0DEV1ST:
        gateway_reply_int32(pframe, gw0dev1st);
        break;
    case GW0ICT:
        gateway_reply_int32(pframe, gw0ict);  // Latest Internet Connection Test result
        break;
    case GW0OCNT:
        gateway_reply_int32(pframe, gw0ocnt);
        break;
    case GW0RCNT:
        gateway_reply_int32(pframe, gw0rcnt);
        break;
    case GW0RRSN:
        gateway_reply_int32(pframe, esp_reset_reason());
        break;
    case GW0BATWIN:
        gateway_reply_int32(pframe, gw0batwin);
        break;
    case GW0TEMPWIN:
        gateway_reply_int32(pframe, gw0tempwin);
        break;
    case GW0REMTWIN:
        gateway_reply_int32(pframe, gw0remtwin);
        break;
    case GW0CONNURL:
        gateway_reply(pframe, gw0connurl, strlen((char *)gw0connurl));
        break;
    case CFG0ITEMPTH:
        gateway_reply_int32(pframe, cfg0itempth);
        break;
    case CFG0WWTRTH:
        gateway_reply_int32(pframe, cfg0wwtrth);
        break;
    case CFG0FWTRTH:
        gateway_reply_int32(pframe, cfg0fwtrth);
        break;
    case CFG0BATTH:
        gateway_reply_int32(pframe, cfg0batth);
        break;
    case CFG0OPATH:
        gateway_reply(pframe, cfg0opath, strlen((char *)cfg0opath));
        break;
    case CFG0OSTAT:
        gateway_reply_int32(pframe, cfg0ostat);
        break;
    case CFG0NTWTH:
        gateway_reply_int32(pframe, cfg0ntwth);
        break;
#ifdef FIRMWARE_BUILD_ID
    case CFG0FWID:
        gateway_reply(pframe, FIRMWARE_BUILD_ID, strlen(FIRMWARE_BUILD_ID));
        break;
#endif  // FIRMWARE_BUILD_ID
    case CFG0DPORT:
        gateway_reply_int32(pframe, cfg0dport);
        break;
    case CFG0LOGLVL:
        gateway_reply(pframe, cfg0loglvl, strlen(cfg0loglvl));
    default:
        break;
    }
}

//! \~ replies (publishes to originating connector) parameter value from a subscription
void gateway_reply(const DDMP2_FRAME *const pframe, const void *const value, const size_t value_size)
{
    TRUE_CHECK_RETURN(pframe != NULL);

    if (value_size)
    {
        TRUE_CHECK_RETURN(value != NULL);
    }

    connector_send_frame_to_connector(DDMP2_CONTROL_PUBLISH, pframe->frame.subscribe.parameter, value, value_size, pframe->source_connector, BROKER_SEND_TIMEOUT);
}

//! \~ replies (publishes to originating connector) a standard int32 parameter value from a subscription
void gateway_reply_int32(const DDMP2_FRAME *const pframe, const int32_t value)
{
    gateway_reply(pframe, &value, sizeof(value));
}

void gateway_update_and_publish_int32(const uint32_t parameter, const int32_t new_value, int32_t *const value_store)
{
    TRUE_CHECK_RETURN(value_store != NULL);

    int changed = new_value != *value_store;

    if (changed)
    {
        *value_store = new_value;
        gateway_publish_int32(parameter, *value_store);
    }
}

//! \~ Create an int32 publish frame and forward it
void gateway_publish_int32(const uint32_t parameter, const int32_t value)
{
    gateway_publish(parameter, &value, sizeof(int32_t));
}

//! \~ Create a publish frame and forward it
void gateway_publish(const uint32_t parameter, const void *const value_store, const size_t value_size)
{
    DDMP2_FRAME publish;

    ddmp2_create_publish(&publish, parameter, value_store, value_size, -1);
    broker_forward_publish(&publish);
}

void gateway_parameter_update_task(const DDMP2_FRAME *frame)
{
    (void)frame;
    gateway_publish_int32(GW0UPT, (int32_t)(hal_cpu_get_micros() / GATEWAY_UPTIME_FACTOR));
}

void gateway_ota_publish_status(const int32_t new_value)
{
    gateway_update_and_publish_int32(CFG0OSTAT, new_value, &cfg0ostat);
}

void gateway_ota_increase_count(void)
{
    int32_t dummy = gw0ocnt;
    dummy++;
    gateway_update_and_publish_int32(GW0OCNT, (const int32_t)dummy, &gw0ocnt);
    config_set_i32("gw0ocnt", (const int32_t)gw0ocnt);
}

void gateway_init(void)
{
    factory_reset_timer = 0;
    gateway_restart_timer = 0;
}
