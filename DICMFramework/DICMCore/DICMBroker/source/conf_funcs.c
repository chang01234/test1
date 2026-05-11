/*! \file conf_funcs.c
	\brief Configuration helper functions

	HTTP functions
*/
#include "configuration.h"
#include "conf_funcs.h"
#include <string.h>
#include "esp_log.h"
#include "esp_idf_version.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"

#ifndef CONFIG_IDF_TARGET_LINUX
#include "esp_http_client.h"


static const char *TAG = "HTTP_CLIENT";

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len;       // Stores number of bytes read
	switch(evt->event_id)
	{
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			/*
			 *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
			 *  However, event handler can also be used in case chunked encoding is used.
			 */
			if (!esp_http_client_is_chunked_response(evt->client)) {
				// If user_data buffer is configured, copy the response into the buffer
				if (evt->user_data) {
					memcpy(evt->user_data + output_len, evt->data, evt->data_len);
				} else {
					if (output_buffer == NULL) {
						output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
						output_len = 0;
						if (output_buffer == NULL) {
							ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
							return ESP_FAIL;
						}
					}
					memcpy(output_buffer + output_len, evt->data, evt->data_len);
				}
				output_len += evt->data_len;
			}

			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
			break;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		case HTTP_EVENT_REDIRECT:
			ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
			break;
#endif
	}
	return ESP_OK;
}

#ifndef CONNECTOR_MQTT_USES_MODEM
/* This function uses Microsoft way of checking the internet connection.*/
static int http_internet_check(void)
{
	int result = 0;
	esp_http_client_config_t config = {
		.url = "http://www.msftncsi.com/ncsi.txt",
		.event_handler = http_event_handler
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err = esp_http_client_perform(client);

	if (err == ESP_OK) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		ESP_LOGI(TAG, "HTTP Status = %d, content_length = %"PRId64"",
#else
        ESP_LOGI(TAG, "HTTP Status = %d, content_length = %"PRId32"",
#endif
		esp_http_client_get_status_code(client),
		esp_http_client_get_content_length(client));

		// Error code 200 means ok
		// Length of expected file content is 14
		if ((esp_http_client_get_status_code(client) == 200) &&
			(esp_http_client_get_content_length(client) == 14))
		{
			result = 1;
		}
	} else {
		ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);

	return result;
}
#endif


int check_internet_connection(void)
{
	EventBits_t event;
	int wifi_result = 0;
	int modem_result = 0;

#ifndef CONNECTOR_MQTT_USES_MODEM
	event = xEventGroupGetBits(network_events);
	if (event & WIFI_IP_BIT)
	{
		if (event & LOCAL_NETWORK_BIT)
		{
			// Do nothing. Internet check not possible
		}
		else
		{
			wifi_result = http_internet_check();
		}
	}
#else
	// We use a simple approach for the Modem where IP address
	// means Internet access
	event = xEventGroupGetBits(network_events);
	if (event & MODEM_IP_BIT)
	{
		modem_result = 1;
	}
#endif

	return (wifi_result || modem_result ? 1 : 0);
}
#else
int check_internet_connection(void)
{
    EventBits_t event;
    int wifi_result = 0;
    event = xEventGroupGetBits(network_events);
    if (event & WIFI_IP_BIT)
    {
        if (event & LOCAL_NETWORK_BIT)
        {
            // Do nothing. Internet check not possible
        }
        else
        {
            wifi_result = 1;
        }
    }
    return wifi_result;
}
#endif
