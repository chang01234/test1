/*! \file modem.c
	\brief Modem source

	Modem handler.
 */

#include "configuration.h"

#if defined(IOT_MODEM) || defined(IOT_MODEM_OLD)

#include "connector_iot_modem.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "connector.h"
#include "ddm_wrapper.h"

#include "esp_modem.h"
#include "esp_log.h"
#include "sim800.h"
#include "hal_i2c_master.h"

#define MODEM_MIN_RSSI (5)				//!< \~ Minimum RSSI level for connection attempt
#define MODEM_MAX_RSSI (98)				//!< \~ Maximum RSSI level for connection attempt
#define MODEM_RECONNECT_STATES (7)		//!< \~ Number of items in modem_reconnect_intervals

static const char *TAG = "iot_modem";
static EventGroupHandle_t event_group = NULL;

static const int CONNECT_BIT = BIT0;
static const int STOP_BIT = BIT1;
static const int DISCONNECT_BIT = BIT2;

static TimerHandle_t timeout_timer;								//!< \~ Reset timer
static modem_dte_t *dte = NULL;
static modem_dce_t *dce = NULL;


typedef struct
{
	// DDM wrapper
	ddmw_t ddm;
	int instance;

	struct
	{
		ddmw_item_t imsi;
		ddmw_item_t imei;
		ddmw_item_t rssi;
	} iot_class;
} iot_modem_t;
static EXT_RAM_ATTR iot_modem_t l_iot_modem;

static int init_fail_cnt = 0;

//! \~ Time to wait after each failed PPP connection attempt.
static const TickType_t modem_reconnect_intervals[MODEM_RECONNECT_STATES] =
{
	60,				// 1:	60 seconds
	60*2,			// 2:
	60*2*2,			// 3:
	60*2*2*2,		// 4:
	60*2*2*2*2,		// 5:
	60*2*2*2*2*2,	// 6:
	60*2*2*2*2*2*2,	// 7:
};

static void modem_sw_reset(void)
{
	init_fail_cnt++;
	if (init_fail_cnt >= MODEM_RECONNECT_STATES)
	{
		init_fail_cnt = 0;
	}

	if (dce != NULL)
	{
		ESP_LOGI(TAG, "SW Reset");

		// CFUN=0
		esp_modem_dce_sw_reset(dce, 0);
		// CFUN=1
		esp_modem_dce_sw_reset(dce, 1);
	}
}

static void reset_timeout(TimerHandle_t xTimer)
{
	(void)xTimer;

	init_fail_cnt++;
	if (init_fail_cnt >= MODEM_RECONNECT_STATES)
	{
		init_fail_cnt = 0;
	}

	if (dte)
	{
		esp_modem_exit_ppp(dte);
	}
}

static void modem_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	switch (event_id) {
	case MODEM_EVENT_PPP_START:
		ESP_LOGI(TAG, "Modem PPP Started");
		break;
	case MODEM_EVENT_PPP_CONNECT:
		ESP_LOGI(TAG, "Modem Connect to PPP Server");
		ppp_client_ip_info_t *ipinfo = (ppp_client_ip_info_t *)(event_data);
		ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
		ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&ipinfo->ip));
		ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&ipinfo->netmask));
		ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&ipinfo->gw));
		ESP_LOGI(TAG, "Name Server1: " IPSTR, IP2STR(&ipinfo->ns1));
		ESP_LOGI(TAG, "Name Server2: " IPSTR, IP2STR(&ipinfo->ns2));
		ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
		xTimerStop(timeout_timer,  portMAX_DELAY);
		xEventGroupSetBits(event_group, CONNECT_BIT);
		xEventGroupSetBits(network_events, MODEM_IP_BIT);
		init_fail_cnt = 0; // At every successful connection, start fom zero again
		break;
	case MODEM_EVENT_PPP_DISCONNECT:
		ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
		xEventGroupSetBits(event_group, DISCONNECT_BIT);
		xEventGroupClearBits(network_events, MODEM_IP_BIT);
		break;
	case MODEM_EVENT_PPP_STOP:
		ESP_LOGI(TAG, "Modem PPP Stopped");
		xEventGroupSetBits(event_group, STOP_BIT);
		xEventGroupClearBits(network_events, MODEM_IP_BIT);
		break;
	case MODEM_EVENT_UNKNOWN:
		ESP_LOGW(TAG, "Unknow line received: %s", (char *)event_data);

		// "NO CARRIER" means that we got disconnected for some reason.
		if (strncmp(event_data, "NO CARRIER", 10) == 0)
		{
			xEventGroupSetBits(event_group, DISCONNECT_BIT);
			xEventGroupClearBits(network_events, MODEM_IP_BIT);
		}
		break;
	default:
		break;
	}
}

void modem_task(void *parameter)
{
	esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_CONFIG();

	uint32_t netw_status;
	uint32_t rcode;
	uint32_t netw_fail_cnt;
	uint8_t mac[6];
	uint32_t mac_low = 1;
	TickType_t  delay_ms;

	uint32_t rssi = 0, ber = 0;
	int reconnect_cnt = 0;
	static uint32_t rssi_cnt = 0;
	uint8_t Periperalbit = 0x03;

	//tcpip_adapter_init();

	xTimerStop(timeout_timer, portMAX_DELAY);


	vTaskDelay(pdMS_TO_TICKS(4000));
	ESP_LOGW(TAG, "Send I2C command");
	//
	ZERO_CHECK(hal_i2c_master_write(0, I2C_MASTER0_SLAVE_ADDR_I2CB, &Periperalbit, sizeof(Periperalbit)));//make bit as low
	vTaskDelay(pdMS_TO_TICKS(3000));
	Periperalbit = 0x04;
	ZERO_CHECK(hal_i2c_master_write(0, I2C_MASTER0_SLAVE_ADDR_I2CB, &Periperalbit, sizeof(Periperalbit)));//make bit as low

	while (dte == NULL)
	{
		// Give the modem a moment to start before attempting to connect.
		vTaskDelay(pdMS_TO_TICKS(1000));

		dte = esp_modem_dte_init(&config);
		if (dte == NULL)
		{
			ESP_LOGE(TAG, "esp_modem_dte_init failed");
			vTaskDelay(pdMS_TO_TICKS(10000));
		}
	}

	// If the processor was reset, the modem might be left in data mode.
	// Sending PPP LCP terminate request (see RFC1331), as terminating the
	// connection with "+++" could make the modem unable to reconnect for a
	// very long time.

	vTaskDelay(pdMS_TO_TICKS(500));
	dte->send_data(dte, "\x7e\xff\x7d\x23\xc0\x21\x7d\x25\x7d\x23\x7d\x20\x7d\x24\x85\x72\x7e", 17);
	vTaskDelay(pdMS_TO_TICKS(500));
	dte->send_data(dte, "\r\n", 2);
	vTaskDelay(pdMS_TO_TICKS(1000));

	if (esp_efuse_mac_get_default(mac) == ESP_OK)
	{
		mac_low = ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];
	}

	ESP_ERROR_CHECK(esp_modem_add_event_handler(dte, modem_event_handler, NULL));

	while (1)
	{
		// Try to establish communication with the modem.

		if ((dce == NULL) || (init_fail_cnt == 0))
		{
			delay_ms = 1 + (mac_low % modem_reconnect_intervals[init_fail_cnt]);
			delay_ms *= 1000;
			ESP_LOGI(TAG, "Before init[%d], waiting %ds.", init_fail_cnt, delay_ms/1000);

			/* Wait, to be Network friendly */
			vTaskDelay(pdMS_TO_TICKS(delay_ms));

			if (dce == NULL)
			{
				dce = sim800_init(dte);

				if (dce == NULL)
				{
					ESP_LOGE(TAG, "sim800_init failed");
					goto modem_reset;
				}

				// Configuration.
				ESP_ERROR_CHECK(dce->set_flow_ctrl(dce, MODEM_FLOW_CONTROL_NONE));
				ESP_ERROR_CHECK(dce->store_profile(dce));

				ESP_LOGI(TAG, "Module: %s", dce->name);
				ESP_LOGI(TAG, "Operator: %s", dce->oper);
				ESP_LOGI(TAG, "IMEI: %s", dce->imei);
				ddmw_set_str(&l_iot_modem.iot_class.imei, dce->imei);
				ESP_LOGI(TAG, "IMSI: %s", dce->imsi);
				ddmw_set_str(&l_iot_modem.iot_class.imsi, dce->imsi);
				ESP_LOGI(TAG, "DCE init done.");
				ddmw_send_generic_event_data(&l_iot_modem.ddm, 0, NULL, 0);
			}
			else
			{
				/* Make sure the modem is in CFUN=1 */
				esp_modem_dce_sw_reset(dce, 1);
			}
		}
		else
		{
			delay_ms = modem_reconnect_intervals[init_fail_cnt] + (mac_low % modem_reconnect_intervals[init_fail_cnt]);
			delay_ms *= 1000;

			ESP_LOGI(TAG, "Before init[%d], waiting %ds.", init_fail_cnt, delay_ms/1000);

			/* Here we might end up after a SW reset. In that case CFUN=1 is the last
			 * executed command. To be "Network friendly" we need to be unregistered
			 * to the network while waiting which means CFUN=4.
			*/
			esp_modem_dce_deregister(dce);

			/* Wait, to be Network friendly */
			vTaskDelay(pdMS_TO_TICKS(delay_ms));

			/* Put modem back to CFUN=1 */
			esp_modem_dce_sw_reset(dce, 1);
		}

		// Wait until reception is good enough.
		rssi = 0;
		while ((rssi < MODEM_MIN_RSSI) || (rssi > MODEM_MAX_RSSI))
		{
			vTaskDelay(pdMS_TO_TICKS(1000));
			rssi_cnt++;
			if (dce->get_signal_quality(dce, &rssi, &ber) == ESP_OK)
			{
				ESP_LOGI(TAG, "rssi: %d, ber: %d", rssi, ber);
				ddmw_set_i32(&l_iot_modem.iot_class.rssi, rssi);
                ddmw_send_generic_event_data(&l_iot_modem.ddm, 0, NULL, 0);
			}
			if (rssi_cnt > 10)
			{
				rssi_cnt = 0;
				goto modem_reset;
			}
		}
		rssi_cnt = 0;
		ESP_LOGI(TAG, "RSSI is OK.");

		netw_status = 0;
		netw_fail_cnt = 0;
		while ((netw_status != 1) && (netw_status != 5) && (netw_fail_cnt < 20))
		{
			if (dce->get_netw_status(dce, &rcode, &netw_status) == ESP_OK)
			{
				ESP_LOGI(TAG, "rcode: %d, status: %d", rcode, netw_status);
			}
			else
			{
				ESP_LOGI(TAG, "Failed to run +CREG?");
				netw_status = 0;
			}

			if ((netw_status != 1) && (netw_status != 5))
			{
				ESP_LOGI(TAG, "Wait 4s. Not correct network status, cnt=%d", netw_fail_cnt);
				vTaskDelay(pdMS_TO_TICKS(4000));
				netw_fail_cnt++;
			}
		}

		// Only continue if correct status
		if ((netw_status == 1) || (netw_status == 5))
		{
			ESP_LOGI(TAG, "Correct status, netw_status=%d", netw_status);

			// Dial PPP connection.
			reconnect_cnt = 0;
			while (esp_modem_setup_ppp(dte) != ESP_OK)
			{
				ESP_LOGI(TAG, "PPP failed. Wait cnt (%d).", reconnect_cnt);
				vTaskDelay(1000);
				reconnect_cnt++;
				if (reconnect_cnt >= MODEM_RECONNECT_STATES)
				{
					reconnect_cnt = 0;
					init_fail_cnt++;
					goto ppp_reset;
				}
			}
			TRUE_CHECK(xTimerStart(timeout_timer, portMAX_DELAY));
			xEventGroupClearBits(event_group, CONNECT_BIT | STOP_BIT | DISCONNECT_BIT);

			// Wait for disconnection.
			ESP_LOGI(TAG, "PPP is up.");
			TRUE_CHECK(xTimerReset(timeout_timer, portMAX_DELAY));
			xEventGroupWaitBits(event_group, DISCONNECT_BIT | STOP_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
			ESP_LOGI(TAG, "Lost PPP connection.");
ppp_reset:
			// Change to command mode.
			esp_modem_change_to_cmd_mode(dte);
			// Deregister from the network
			esp_modem_dce_deregister(dce);
		}
		else
		{
			ESP_LOGI(TAG, "Failed, netw_status=%d", netw_status);
modem_reset:
			ESP_LOGI(TAG, "Failed, SW Reset of modem");
			modem_sw_reset();
		}
	}
}
static void connector_iot_modem_process_task(const DDMP2_FRAME * const pframe)
{
	ddmw_process(&l_iot_modem.ddm, pframe);
	ddmw_process_publish(&l_iot_modem.ddm);
}

static int initialize_modem(void)
{
	LOG(I, "Initializing modem connector");

	event_group = xEventGroupCreate();

	ddmw_init(&l_iot_modem.ddm, &connector_iot_modem);
	l_iot_modem.instance = ddmw_register(&l_iot_modem.ddm, MDM0AVL);
	ddmw_add(&l_iot_modem.ddm, &l_iot_modem.iot_class.imsi, MDM0IMSI, l_iot_modem.instance);
	ddmw_add(&l_iot_modem.ddm, &l_iot_modem.iot_class.imei, MDM0IMEI, l_iot_modem.instance);
	ddmw_add(&l_iot_modem.ddm, &l_iot_modem.iot_class.rssi, MDM0RSSI, l_iot_modem.instance);

	LOG(I, "Starting modem");
	timeout_timer = xTimerCreate(NULL, pdMS_TO_TICKS(10000), pdFALSE, (void*)0, reset_timeout);
	TRUE_CHECK(timeout_timer);
	BaseType_t ret = xTaskCreate(modem_task, "iot_modem", 2248, NULL, xTASK_PRIORITY_NORMAL, NULL);
	TRUE_CHECK(ret);
	return 1;
}

CONNECTOR connector_iot_modem =
{
	.name="IOT modem connector",
	.initialize = initialize_modem,
	.process_event = connector_iot_modem_process_task
};


#endif
