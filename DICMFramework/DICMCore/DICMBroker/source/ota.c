/*
 * ota.c
 *
 *  Created on: 3 feb. 2022
 *      Author: Andlun
 */

#include "ota.h"
#include "broker.h"
#include "configuration.h"
#include "esp_idf_version.h"
#include "gateway.h"
#include <stdint.h>
#include <strings.h>
#ifndef CONFIG_IDF_TARGET_LINUX
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#endif
#include "hal_cpu.h"
#ifdef CONNECTOR_WIFI
#include "wifi_network.h"
#endif

#ifndef CONFIG_IDF_TARGET_LINUX
static esp_err_t gateway_ota_handler(esp_http_client_event_t *evt)
{
    static int received = 0;
    static int content_length = 0;
    static int percent = 0;
    char percent_string[32];
    int percent_string_len;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        LOG(E, "ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        LOG(D, "ON_CONNECTED");
        received = 0;
        content_length = 0;
        percent = 0;
        break;
    case HTTP_EVENT_HEADER_SENT:
        LOG(D, "HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        LOG(D, "ON_HEADER, %s=%s", evt->header_key, evt->header_value);
        if (!strcasecmp("Content-Length", evt->header_key))
        {
            content_length = atoi(evt->header_value);
            LOG(I, "Content-Length: %d", content_length);
        }
        break;
    case HTTP_EVENT_ON_DATA:
        received += evt->data_len;
        if (content_length > 0)
        {
            if (((received * 100) / content_length) > percent)
            {
                percent++;
                percent_string_len = snprintf(percent_string, sizeof(percent_string), "%.3fkB (%d%%)", received / 1024.0, percent);
            }
            else
            {
                percent_string_len = 0;
            }
        }
        else
        {
            percent_string_len = snprintf(percent_string, sizeof(percent_string), "%.3f kB", received / 1024.0);
        }
        if (percent_string_len)
        {
            LOG(D, "%s", percent_string);
            gateway_publish(GW0OTA, &percent_string, percent_string_len);
        }
        gateway_ota_publish_status(CFG0OSTAT_ONGOING);
        break;
    case HTTP_EVENT_ON_FINISH:
        LOG(D, "ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        LOG(D, "DISCONNECTED");
        break;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    case HTTP_EVENT_REDIRECT:
        LOG(D, "HTTP_EVENT_REDIRECT");
        break;
#endif
    }
    return ESP_OK;
}
#endif

//! \~ Perform Over The Air update of firmware from URL
int gateway_ota(const char *const url)
{
#ifdef CONFIG_IDF_TARGET_LINUX
    esp_err_t ret = ESP_FAIL;
    char failstring[32];
    int failstring_len;
    failstring_len = snprintf(failstring, sizeof(failstring), "OTA failed! (%d)", ret);
    gateway_publish(GW0OTA, failstring, failstring_len);
    gateway_ota_publish_status(CFG0OSTAT_FAILED);
    LOG(E, "OTA failed!");
    return (int)ret;
#else
    TRUE_CHECK_RETURN0(url != NULL);

    esp_err_t ret;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    const esp_http_client_config_t http_client = {
        .url = url,
        .event_handler = gateway_ota_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_https_ota_config_t client = {
        .http_config = &http_client,
        .partial_http_download = false,
    };
#else
    esp_http_client_config_t client = {
        .url = url,
        .event_handler = gateway_ota_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
#endif
    char failstring[32];
    int failstring_len;

#ifdef CONNECTOR_DCP_UART
    connector_dcp_fw_upgrade(1);
#endif  // CONNECTOR_DCP_UART
#ifdef CONNECTOR_WIFI
    wifi_network__disable_background_scanner(); /* Disable background scanner so it will not interfere with download process */
#endif
    LOG(I, "Downloading new firmware image from %s", url);
    gateway_publish(GW0OTA, SS("Starting OTA..."));
    LED_G(1);

    ZERO_CHECK(ret = esp_https_ota(&client));

    if (ret == ESP_OK)
    {
        LOG(I, "Restarting due to OTA update!");
        gateway_ota_publish_status(CFG0OSTAT_OK);
        gateway_publish(GW0OTA, SS("Restarting..."));

        for (int i = 0; i <= 15; i++)  // blink LED a few times to indicate restart is imminent
        {
            LED_G(i & 1);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        hal_cpu_reset(HALCPU_RESET_FLAG_NONE);
    }
#ifdef CONNECTOR_WIFI
    wifi_network__enable_background_scanner(); /* Re-enable the background scanner if we failed to do OTA */
#endif
    LED_G(0);

    failstring_len = snprintf(failstring, sizeof(failstring), "OTA failed! (%d)", ret);
    gateway_publish(GW0OTA, failstring, failstring_len);

#ifdef CONNECTOR_DCP_UART
    connector_dcp_fw_upgrade(0);
#endif  // CONNECTOR_DCP_UART

    gateway_ota_publish_status(CFG0OSTAT_FAILED);
    LOG(E, "OTA failed!");
    return ret;
#endif
}
