#include "configuration.h"

#ifdef CONNECTOR_GNSS

#include <string.h>
#include <stdint.h>

#include "gnss_l86.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/uart.h"

//#define L86_DEBUG_PRINT_UNUSED_MESSAGES // Prints messages not sent to a parsing function.
//#define L86_DEBUG_PRINT_MESSAGES // Prints all transmitted and correctly received messages.

#define L86_RESPONSE_QUEUE_ITEMS (16u)
#define L86_MESSAGE_LEN          (64u)
#define L86_TX_BUFFER_LEN        (256u)
#define L86_RX_BUFFER_LEN        (1024u)
#define L86_RETRY_COUNT          (5u)

#ifndef GNSS_L86_FIX_INTERVAL_MS
#define GNSS_L86_FIX_INTERVAL_MS (1000u)
#endif

typedef struct {
	const char *id;
	const char id_offset;
	void (*cbk)(char *);
} nmea_lookup_t;

typedef struct {
	int gll;
	int rmc;
	int vtg;
	int gga;
	int gsa;
	int gsv;
	int grs;
	int gst;
	int zda;
	int mchn;
} nmea_interval_t;

static int get_nmea_output(nmea_interval_t *in);
static int set_nmea_output(const nmea_interval_t *in);
static void uart_task(void *parameter);
static void uart_read_data(size_t len);
static uint8_t *nmea_check_message(uint8_t *message, size_t message_len);
static void nmea_send_message(const char *message);
static void nmea_parse_message(char *message);
static void nmea_parse_rmc(char *message);
static void nmea_parse_gga(char *message);
static void nmea_parse_gsa(char *message);
#ifdef L86_DEBUG_PRINT_UNUSED_MESSAGES
static void nmea_debug_print(char *message);
#endif
static void nmea_queue_response(char *message);
static int nmea_wait_response(const char *match, char *buffer, size_t buffer_size);
static void nmea_empty_responses(void);
static int nmea_send_message_wait_response(const char *tx_buf, const char *match, char *rx_buf, size_t rx_buf_size);
static int parse_ack_status(const char *buf);

// Specifies output frequencies of each NMEA sentence relative to the fix interval.
// Valid values are 0 to 5: 0=disabled, 1=Every fix, 2=Every other fix, ..., 5=Every fifth fix.
static const nmea_interval_t default_nmea_interval = {
	.gll  = 0, // (default=1) GLL interval - Geographic position, latitude and longitude
	.rmc  = 1, // (default=1) RMC interval - Recommended minimum specific GNSS sentence
	.vtg  = 0, // (default=1) VTG interval - Course over ground and ground speed
	.gga  = 1, // (default=1) GGA interval - GPS fix data
	.gsa  = 1, // (default=1) GSA interval - GNSS DOPS and active satellites
	.gsv  = 0, // (default=1) GSV interval - GNSS satellites in view
	.grs  = 0, // (default=0) GRS interval - GNSS range residuals
	.gst  = 0, // (default=0) GST interval - GNSS pseudorange error statistics
	.zda  = 0, // (default=0) ZDA interval - Time and date
	.mchn = 0, // (default=0) PMTKCHN interval - GNSS channel status
};

// Parsing routines for incoming data. Make sure that all messages used are enabled in the default_nmea_interval table above.
static const nmea_lookup_t nmea_lookup_table[] = {
	{ "RMC",   2, nmea_parse_rmc      }, // --RMC Recommended Minimum Position Data
	{ "GGA",   2, nmea_parse_gga      }, // --GGA Global Positioning System Fix Data
	{ "GSA",   2, nmea_parse_gsa      }, // --GSA GNSS DOP and Active Satellites
	{ "GPTXT", 0, NULL                }, // GPTXT Status messages
	{ "PMTK",  0, nmea_queue_response }, // PMTK  MTK protocol
#ifdef L86_DEBUG_PRINT_UNUSED_MESSAGES
	{ "",      0, nmea_debug_print    }, // Print unhandled messages, for debug only
#endif
};

static QueueHandle_t nmea_resp_queue_handle;
static StaticQueue_t nmea_resp_queue_buffer;
static uint8_t nmea_resp_queue_storage_buffer[L86_RESPONSE_QUEUE_ITEMS * sizeof(void *)];

static gnss_l86_message_callback_t message_callback = NULL;

void gnss_l86_init(gnss_l86_message_callback_t callback)
{
	message_callback = NULL;

	memset(nmea_resp_queue_storage_buffer, 0, sizeof(nmea_resp_queue_storage_buffer));
	TRUE_CHECK(nmea_resp_queue_handle = xQueueCreateStatic(L86_RESPONSE_QUEUE_ITEMS, sizeof(void *), nmea_resp_queue_storage_buffer, &nmea_resp_queue_buffer));
	TRUE_CHECK(xTaskCreate(uart_task, "L86", 2048, NULL, xTASK_PRIORITY_NORMAL, NULL));

#ifdef GNSS_L86_FORCE_ON
	GNSS_L86_FORCE_ON(1);
#endif

	// It takes a moment for the L86 to start after a reset.
	vTaskDelay(pdMS_TO_TICKS(5000));

	// Make sure that L86 is fully on.
	gnss_l86_power_on();

	// Update fix interval if required.
	int fix = 0;
	if (gnss_l86_get_fix_interval(&fix) == 0)
	{
		if (fix != GNSS_L86_FIX_INTERVAL_MS)
		{
			if (gnss_l86_set_fix_interval(GNSS_L86_FIX_INTERVAL_MS) != 0)
			{
				LOG(W, "Could not set fix interval.");
			}
		}
	}
	else
	{
		LOG(W, "Could not get fix interval.");
	}

	// Update nmea output intervals if required.
	nmea_interval_t interval;
	if (get_nmea_output(&interval) == 0)
	{
		if (memcmp(&default_nmea_interval, &interval, sizeof(nmea_interval_t)) != 0)
		{
			if (set_nmea_output(&default_nmea_interval) != 0)
			{
				LOG(W, "Could not set NMEA update interval.");
			}
		}
	}
	else
	{
		LOG(W, "Could not get NMEA update interval.");
	}

	// Enable message callback function.
	message_callback = callback;
}

int gnss_l86_get_fix_interval(int *interval_ms)
{
	char buf[L86_MESSAGE_LEN];

	// Query fix interval (PMTK_API_Q_FIX_CTL -> PMTK_DT_FIX_CTL)
	if (nmea_send_message_wait_response("PMTK400", "PMTK500", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	// PMTK500,Interval,Reserved,Reserved,Reserved,Reserved
	if (sscanf(buf, "PMTK500,%d", interval_ms) != 1)
	{
		LOG(I, "Could not parse response: %s", buf);
		return 1;
	}

	return 0;
}

int gnss_l86_set_fix_interval(int interval_ms)
{
	char buf[L86_MESSAGE_LEN];

	// Make sure interval is within supported range.
	if ((interval_ms < 100) || (interval_ms > 10000))
		return 1;

	// Set fix interval (PMTK_SET_POS_FIX)
	snprintf(buf, sizeof(buf), "PMTK220,%d", interval_ms);

	if (nmea_send_message_wait_response(buf, "PMTK001,220", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}

	return 0;
}

int gnss_l86_power_on(void)
{
	char buf[L86_MESSAGE_LEN];

	LOG(I, "Switching to full on mode.");

#ifdef GNSS_L86_FORCE_ON
	GNSS_L86_FORCE_ON(1);
	vTaskDelay(pdMS_TO_TICKS(500));

	if (nmea_send_message_wait_response("PMTK225,0", "PMTK001,225", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}
#else
	if (nmea_send_message_wait_response("PMTK225,0", "PMTK001,225", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}

#endif

	return 0;
}

int gnss_l86_power_alwayslocate(void)
{
	char buf[L86_MESSAGE_LEN];

#ifdef GNSS_L86_FORCE_ON
	LOG(I, "Switching to alwayslocate using backup mode.");

	GNSS_L86_FORCE_ON(0);
	vTaskDelay(pdMS_TO_TICKS(500));

	// AlwaysLocate using backup mode.
	if (nmea_send_message_wait_response("PMTK225,9", "PMTK001,225", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}
#else
	LOG(I, "Switching to alwayslocate using standby mode.");

	// AlwaysLocate using standby mode.
	if (nmea_send_message_wait_response("PMTK225,8", "PMTK001,225", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}
#endif

	return 0;
}

int gnss_l86_power_save(void)
{
	char buf[L86_MESSAGE_LEN];

#ifdef GNSS_L86_FORCE_ON
	LOG(I, "Switching to backup mode.");

	GNSS_L86_FORCE_ON(0);
	vTaskDelay(pdMS_TO_TICKS(100));

	// Backup mode.
	if (nmea_send_message_wait_response("PMTK225,4", "PMTK001,225", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}
#else
	LOG(I, "Switching to standby mode.");

	// Standby mode.
	if (nmea_send_message_wait_response("PMTK161,0", "PMTK001,161", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}
#endif

	return 0;
}

static int get_nmea_output(nmea_interval_t *in)
{
	char buf[L86_MESSAGE_LEN];

	memset(in, 0, sizeof(nmea_interval_t));

	// PMTK_API_Q_NMEA_OUTPUT
	if (nmea_send_message_wait_response("PMTK414", "PMTK514", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	// PMTK514,GLL,RMC,VTG,GGA,GSA,GSV,GRS,GST,0,0,0,0,0,0,0,0,0,ZDA,MCHN PMTK_DT_NMEA_OUTPUT
	if (sscanf(buf, "PMTK514,%d,%d,%d,%d,%d,%d,%d,%d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%d,%d",
		&in->gll, &in->rmc, &in->vtg, &in->gga, &in->gsa, &in->gsv, &in->grs, &in->gst, &in->zda, &in->mchn) != 10)
	{
		return 1;
	}

	return 0;
}

static int set_nmea_output(const nmea_interval_t *in)
{
	char buf[L86_MESSAGE_LEN];

	//PMTK314,GLL,RMC,VTG,GGA,GSA,GSV,GRS,GST,0,0,0,0,0,0,0,0,0,ZDA,MCHN PMTK_API_SET_NMEA_OUTPUT
	snprintf(buf, sizeof(buf), "PMTK314,%d,%d,%d,%d,%d,%d,%d,%d,0,0,0,0,0,0,0,0,0,%d,%d",
		in->gll, in->rmc, in->vtg, in->gga, in->gsa, in->gsv, in->grs, in->gst, in->zda, in->mchn);

	if (nmea_send_message_wait_response(buf, "PMTK001,314", buf, sizeof(buf)) != 0)
	{
		return 1;
	}

	if (parse_ack_status(buf) != 3)
	{
		return 1;
	}

	return 0;
}

static void uart_task(void *parameter)
{

	/**
	 * @brief   UART initialization configuration
	 * Configure UART. Note that REF_TICK is used so that the baud rate remains
	 * correct while APB frequency is changing in light sleep mode.
	 */
	static const uart_config_t uart_config =
	{
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
	#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
			.source_clk = UART_SCLK_REF_TICK,
	#else
			.source_clk = UART_SCLK_XTAL,
	#endif
#endif
	};

	static QueueHandle_t uart_queue;
	uart_event_t event;

	ZERO_CHECK(uart_param_config(GNSS_L86_UART, &uart_config));
	ZERO_CHECK(uart_set_pin(GNSS_L86_UART, GNSS_L86_TX_GPIO, GNSS_L86_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ZERO_CHECK(uart_driver_install(GNSS_L86_UART, L86_RX_BUFFER_LEN, L86_TX_BUFFER_LEN, 8, &uart_queue, 0));

	while (1)
	{
		TRUE_CHECK(xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY));
		switch (event.type)
		{
			case UART_DATA:
				if (event.size > 0)
				{
					uart_read_data(event.size);
				}
				break;
			case UART_FIFO_OVF:
				LOG(W, "HW FIFO overflow, flushing");
				uart_flush_input(GNSS_L86_UART);
				break;
			case UART_BUFFER_FULL:
				LOG(W, "Ring buffer full, flushing");
				uart_flush_input(GNSS_L86_UART);
				break;
			case UART_BREAK:
				//LOG(I, "UART RX break");
				break;
			case UART_PARITY_ERR:
				LOG(I, "UART parity error");
				break;
			case UART_FRAME_ERR:
				LOG(I, "UART frame error");
				break;
			case UART_PATTERN_DET:
				LOG(I, "UART pattern detect");
				break;
			default:
				LOG(I, "UART event type: %d", event.type);
				break;
		}
	}
}

static void uart_read_data(size_t len)
{
	static uint8_t rx_buffer[L86_RX_BUFFER_LEN];
	static size_t rx_buffer_bytes = 0;
	int bytes_read;
	uint8_t *lf;
	uint8_t *message;

	// Buffer and process bytes from the UART until FIFO is empty.
	while ((bytes_read = uart_read_bytes(GNSS_L86_UART, rx_buffer + rx_buffer_bytes, MIN(len, sizeof(rx_buffer) - rx_buffer_bytes), 0)) > 0)
	{
		rx_buffer_bytes += bytes_read;

		// Split and remove buffered data one line at a time.
		while ((lf = (uint8_t *)memchr(rx_buffer, '\n', rx_buffer_bytes)) != NULL)
		{
			size_t matched = lf - rx_buffer + 1; // +1 = Include linefeed in match.

			// Verify format and checksum.
			message = nmea_check_message(rx_buffer, matched);

			// Parse message if valid.
			if (message != NULL)
			{
				nmea_parse_message((char *)message);
			}

			// Remove message contents from buffer.
			memmove(rx_buffer, rx_buffer + matched, rx_buffer_bytes - matched);
			rx_buffer_bytes -= matched;
		}

		// Empty the buffer if it is still full after scanning for lines.
		if (rx_buffer_bytes >= sizeof(rx_buffer))
		{
			rx_buffer_bytes = 0;
			LOG(W, "RX buffer overflow!");
		}
	}
}

static uint8_t *nmea_check_message(uint8_t *message, size_t message_len)
{
	uint8_t *start;
	uint8_t *end;

	// Locate start and end of message.
	start = (uint8_t *)memchr(message, '$', message_len);
	end = (uint8_t *)memrchr(message, '*', message_len);

	if ((start == NULL) || (end == NULL))
	{
		LOG(D, "Incomplete NMEA message.");
		return NULL;
	}

	// Sanity checks.
	size_t start_pos = start - message;
	size_t end_pos = end - message;

	if (start_pos > end_pos)
	{
		LOG(D, "Malformed NMEA message.");
		return NULL;
	}
	if ((end_pos + 2) >= message_len)
	{
		LOG(D, "NMEA message is missing checksum.");
		return NULL;
	}

	int checksum = 0;
	for (size_t i = start_pos + 1; i < end_pos; i++)
	{
		checksum ^= message[i];
	}

	int msg_checksum = 0;
	sscanf((char *)&message[end_pos + 1], "%02X", &msg_checksum);

	if ((uint8_t)checksum != (uint8_t)msg_checksum)
	{
		LOG(D, "Bad NMEA checksum.");
	}

	// Zero-terminate buffer after message and return start of contents.
	message[end_pos] = '\0';
	return &message[start_pos + 1];
}

static void nmea_send_message(const char *message)
{
	char tx_buffer[L86_MESSAGE_LEN];

	uint8_t checksum = 0;
	for (size_t i = 0; i < strlen(message); i++)
	{
		checksum ^= message[i];
	}

	snprintf(tx_buffer, sizeof(tx_buffer), "$%s*%02X\r\n", message, checksum);

	int tx_buffer_bytes = strlen(tx_buffer);

#ifdef L86_DEBUG_PRINT_MESSAGES
	LOG(D, "L86 TX: %s", tx_buffer);
#endif

	if (uart_write_bytes(GNSS_L86_UART, tx_buffer, tx_buffer_bytes) != tx_buffer_bytes)
	{
		LOG(E, "TX failed.");
	}
}

static void nmea_parse_message(char *message)
{
#ifdef L86_DEBUG_PRINT_MESSAGES
	LOG(D, "L86 RX: %s", message);
#endif

	for (size_t i = 0; i < (sizeof(nmea_lookup_table) / sizeof(nmea_lookup_table[0])); i++)
	{
		if (strncmp(message + nmea_lookup_table[i].id_offset, nmea_lookup_table[i].id, strlen(nmea_lookup_table[i].id)) == 0)
		{
			if (nmea_lookup_table[i].cbk != NULL)
			{
				nmea_lookup_table[i].cbk(message);
			}
			break;
		}
	}
}

static void nmea_parse_rmc(char *message)
{
	// --RMC,UTC Time,Data Valid,Latitude,N/S,Longitude,E/W,Speed,COG,Date,Magnetic Variation,E/W,Positioning Mode,Navigational status

	gnss_l86_message_t m;
	memset(&m, 0, sizeof(m));
	m.type = GNSS_RMC;

	// Extract all fields (this modifies message string and pointer).
	                         strsep(&message, ","); // --RMC
	m.rmc.utc_time         = strsep(&message, ","); // hhmmss.ssss
	m.rmc.data_valid       = strsep(&message, ","); // 'V'=Invalid 'A'=Valid
	m.rmc.latitude         = strsep(&message, ","); // ddmm.mmmm
	m.rmc.latitude_ns      = strsep(&message, ","); // 'N'=North 'S'=South
	m.rmc.longitude        = strsep(&message, ","); // dddmm.mmmm
	m.rmc.longitude_ew     = strsep(&message, ","); // 'E'=East 'W'=West
	m.rmc.speed            = strsep(&message, ","); // Speed over ground (knots)
	m.rmc.cog              = strsep(&message, ","); // Course over ground (degrees)
	m.rmc.date             = strsep(&message, ","); // ddmmyy
	                         strsep(&message, ","); // Magnetic variation (degrees)
	                         strsep(&message, ","); // 'E'=East 'W'=West
	m.rmc.positioning_mode = strsep(&message, ","); // 'N'=No fix 'A'=Autonomous GNSS fix 'D'=Differential GNSS fix
	m.rmc.nav_status       = strsep(&message, ","); // 'V'=Navigational status not valid

	// Report message data to connector.
	if (message_callback != NULL)
	{
		message_callback(&m);
	}
}

static void nmea_parse_gga(char *message)
{
	// --GGA,UTC Time,Latitude,N/S,Longitude,E/W,Fix Status,Number of SV,HDOP,Altitude,M,Geoid Separation,M,DGPS Age,DGPS Station ID

	gnss_l86_message_t m;
	memset(&m, 0, sizeof(m));
	m.type = GNSS_GGA;

	// Extract all fields (this modifies message string and pointer).
	                       strsep(&message, ","); // --GGA
	m.gga.utc_time        = strsep(&message, ","); // hhmmss.ssss
	m.gga.latitude        = strsep(&message, ","); // ddmm.mmmm
	m.gga.latitude_ns     = strsep(&message, ","); // 'N'=North 'S'=South
	m.gga.longitude       = strsep(&message, ","); // dddmm.mmmm
	m.gga.longitude_ew    = strsep(&message, ","); // 'E'=East 'W'=West
	m.gga.fix_status      = strsep(&message, ","); // '0'=Invalid '1'=GNSS '2'=DGPS '6'=Estimated mode
	m.gga.number_of_sv    = strsep(&message, ","); // Number of satellites used
	m.gga.hdop            = strsep(&message, ","); // x.xx
	m.gga.altitude        = strsep(&message, ","); // Altitude according to WGS84 ellipsoid xx.x (meters)
	                        strsep(&message, ","); // 'M'
	m.gga.geoid_height    = strsep(&message, ","); // Height of geoid (sea level) above WGS84 ellipsoid xx.x (meters)
	                        strsep(&message, ","); // 'M'
	m.gga.dgps_age        = strsep(&message, ","); // Age of DGPS data
	m.gga.dgps_station_id = strsep(&message, ","); // DGPS station ID

	// Report message data to connector.
	if (message_callback != NULL)
	{
		message_callback(&m);
	}
}

static void nmea_parse_gsa(char *message)
{
	// --GSA,Mode,Fix Status,Satellite used 1,...,Satellite used 12,PDOP,HDOP,VDOP,GNSS System ID

	gnss_l86_message_t m;
	memset(&m, 0, sizeof(m));
	m.type = GNSS_GSA;

	// Extract all fields (this modifies message string and pointer).
	                       strsep(&message, ","); // --GSA
	m.gsa.mode           = strsep(&message, ","); // 'M'=Manual mode 'A'=Automatic 2D/3D mode
	m.gsa.fix_status     = strsep(&message, ","); // '1'=No fix '2'=2D fix '3'=3D fix
	m.gsa.satellite[0]   = strsep(&message, ","); // Satellite used 1
	m.gsa.satellite[1]   = strsep(&message, ","); // Satellite used 2
	m.gsa.satellite[2]   = strsep(&message, ","); // Satellite used 3
	m.gsa.satellite[3]   = strsep(&message, ","); // Satellite used 4
	m.gsa.satellite[4]   = strsep(&message, ","); // Satellite used 5
	m.gsa.satellite[5]   = strsep(&message, ","); // Satellite used 6
	m.gsa.satellite[6]   = strsep(&message, ","); // Satellite used 7
	m.gsa.satellite[7]   = strsep(&message, ","); // Satellite used 8
	m.gsa.satellite[8]   = strsep(&message, ","); // Satellite used 9
	m.gsa.satellite[9]   = strsep(&message, ","); // Satellite used 10
	m.gsa.satellite[10]  = strsep(&message, ","); // Satellite used 11
	m.gsa.satellite[11]  = strsep(&message, ","); // Satellite used 12
	m.gsa.pdop           = strsep(&message, ","); // Position dilution of precision
	m.gsa.hdop           = strsep(&message, ","); // Horizontal dilution of precision
	m.gsa.vdop           = strsep(&message, ","); // Vertical dilution of precision
	m.gsa.gnss_system_id = strsep(&message, ","); // '1'=GPS '2'=GLONASS '3'=Galileo '4'=BeiDou

	// Report message data to connector.
	if (message_callback != NULL)
	{
		message_callback(&m);
	}
}

#ifdef L86_DEBUG_PRINT_UNUSED_MESSAGES
static void nmea_debug_print(char *message)
{
	LOG(W, "Unhandled message: \"%s\"", message);
}
#endif

static void nmea_queue_response(char *message)
{
	if (uxQueueSpacesAvailable(nmea_resp_queue_handle) > 0)
	{
		// Allocates space for the message and queues it for later.

		size_t msg_len = strlen(message) + 1; // Include null character.
		void *msg = heap_caps_malloc(msg_len, MALLOC_CAP_SPIRAM);
		if (msg != NULL)
		{
			memcpy(msg, message, msg_len);
			if (xQueueSend(nmea_resp_queue_handle, &msg, 0) != pdTRUE)
			{
				heap_caps_free(msg);
				LOG(E, "Failed to queue message, dropping response: %s", message);
			}
		}
		else
		{
			LOG(E, "Failed to alloc memory, dropping response: %s", message);
		}
	}
	else
	{
#ifdef L86_DEBUG_PRINT_UNUSED_MESSAGES
		LOG(D, "Queue is full, dropping response: %s", message);
#endif
	}
}

static int nmea_wait_response(const char *match, char *buffer, size_t buffer_size)
{
	char *msg = NULL;

	// Remove and free queued messages until one matches.
	while (xQueueReceive(nmea_resp_queue_handle, &msg, pdMS_TO_TICKS(1000)) == pdTRUE)
	{
		if (memcmp(match, msg, strlen(match)) == 0)
		{
			strncpy(buffer, msg, buffer_size);
			heap_caps_free(msg);
			return 0;
		}
		else
		{
			heap_caps_free(msg);
		}
	}

	// Timed out.
	return 1;
}

static void nmea_empty_responses(void)
{
	char *msg = NULL;

	// Remove and free all messages in the queue.
	while (xQueueReceive(nmea_resp_queue_handle, &msg, 0) == pdTRUE)
	{
		heap_caps_free(msg);
	}
}

static int nmea_send_message_wait_response(const char *tx_buf, const char *match, char *rx_buf, size_t rx_buf_size)
{
	unsigned int retry = L86_RETRY_COUNT;

	nmea_empty_responses();

	// Transmits message and waits for a matching response.
	while (retry > 0)
	{
		nmea_send_message(tx_buf);

		if (nmea_wait_response(match, rx_buf, rx_buf_size) == 0)
			break;

		retry--;
	}

	if (retry == 0)
	{
		LOG(I, "Response timeout.");
		return 1;
	}

	return 0;
}

static int parse_ack_status(const char *buf)
{
	int flag = 0;

	if (sscanf(buf, "PMTK001,%*d,%d", &flag) != 1)
	{
		LOG(D, "Not a PMTK_ACK message: %s", buf);
		return 0;
	}

	switch (flag)
	{
		case 0:
			LOG(D, "ACK: Invalid packet");
			break;
		case 1:
			LOG(D, "ACK: Unsupported packet type");
			break;
		case 2:
			LOG(D, "ACK: Valid packet, action failed");
			break;
		case 3:
			//LOG(D, "ACK: Valid packet, action succeeded");
			break;
	}

	return flag;
}

#endif
