/**
 * @file connector_log_service.c
 *
 * @brief The purpose of the log service is to have a deterministic performance penalty for logging entries in the DICM system.
 *
 * This is done by using our own vprintf function instead of IDF stdlib implementation. This will function will only copy the format string,
 * data arguments into a log message structure and post that to a queue(Ringbuffer). The actual log formatting and printing to stdout,
 * is later done in either IDLE task or the connector task.
 *
 * The parsing of format string and arguments is based on free software nanoprintf (see https://github.com/charlesnicholson/nanoprintf).
 *
 *  Created on: 25 apr. 2023
 *      Author: Andlun
 * @note Logging from ISR is not supported.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "configuration.h"
#include "dicm_log_service.h"
#include "esp_err.h"
#include "esp_freertos_hooks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "hal_cpu.h"
#include "hal_mem.h"
#include "utils.h"
#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#endif
// Nanoprintf configurations
#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS   1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS       1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS       1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS      1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS   0

#define NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW

// Defines the maximum number of arguments printf arguments supported
#ifndef CONFIG_LOG_SERVICE_MAX_ARGUMENTS
#define CONFIG_LOG_SERVICE_MAX_ARGUMENTS (20)
#endif
COMPILE_TIME_ASSERT(CONFIG_LOG_SERVICE_MAX_ARGUMENTS <= 32);

#include "nanoprintf.h"

/**
 * @brief Defines if time profiling should be built or not. Defaults to not include in build.
 *
 */
#ifndef CONFIG_LOG_SERVICE_TIME_PROFILING
#define CONFIG_LOG_SERVICE_TIME_PROFILING 0
#endif

/**
 * @brief Set this to 1 if profiling the original log code
 *
 */
#ifndef CONFIG_LOG_SERVICE_TIME_PROFILING_ORIGINAL
#define CONFIG_LOG_SERVICE_TIME_PROFILING_ORIGINAL 0
#endif

/**
 * @brief Set this to define amount of ticks to wait until log is discarded
 *
 */
#ifndef CONFIG_LOG_SERVICE_TICKS_TO_WAIT
#define CONFIG_LOG_SERVICE_TICKS_TO_WAIT 1
#endif

#define LOG_SERVICE_IN_IDLE_TASK      (0)
#define LOG_SERVICE_IN_CONNECTOR_TASK (1)

/**
 * @brief Defines the type of logging mechanism to be used. Defaults to using freeRTOS IDLE task.
 *
 */
#ifndef CONFIG_LOG_SERVICE_EXECUTOR
#define CONFIG_LOG_SERVICE_EXECUTOR LOG_SERVICE_IN_CONNECTOR_TASK
#endif

#ifndef CONNECTOR_LOG_SERVICE_RINGBUFFER_SIZE
#define CONNECTOR_LOG_SERVICE_RINGBUFFER_SIZE (10 * 1024)
#endif
#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
#define WEB_SERVER_LOG_BUFFER_SIZE 1024
/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
typedef struct async_resp_arg
{
    httpd_handle_t hd;
    int fd;
    char data[WEB_SERVER_LOG_BUFFER_SIZE];
} async_resp_arg_t;
#endif

/**
 * @brief Log message structure used in ringbuffer
 *
 */
typedef struct
{
    const char *format;
    int32_t num;
    uint32_t alloc;
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
    int64_t delta_time;
#endif
    char values[0];
} log_service_message_t;

typedef struct
{
    RingbufHandle_t rbufHandle;
    void *storage;
    StaticRingbuffer_t ringbuffer;
#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
    httpd_handle_t server;
#endif
} dicm_log_service_t;

#if !defined(CONFIG_IDF_TARGET_LINUX)
#if CONFIG_LOG_SERVICE_EXECUTOR == LOG_SERVICE_IN_CONNECTOR_TASK
static void log_service_task(void *parameter);
#endif
#endif
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
static void log_service_mean(uint16_t elapsed);
#endif
static int internal_logger(const char *format, ...);

#if defined(CONFIG_IDF_TARGET_LINUX)
int log_service_vprintf(const char *format, va_list varlist);
bool log_service_idle_cb(char *log_buffer, size_t log_buffer_size);
#else
static int log_service_vprintf(const char *format, va_list varlist);
static bool log_service_idle_cb(void);
#endif

#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
static void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static httpd_handle_t start_webserver(void);
static esp_err_t stop_webserver(httpd_handle_t server);
static esp_err_t index_html_get_handler(httpd_req_t *req);
static esp_err_t websocket_handler(httpd_req_t *req);
static esp_err_t log_server_handler(httpd_req_t *req);
static esp_err_t favicon_get_handler(httpd_req_t *req);
static void ws_async_send(void *arg);
static esp_err_t trigger_async_send(const char *output);
#endif
// Service information.
static dicm_log_service_t l_dicm_log_service;

static vprintf_like_t old_vprintf_func = NULL;
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
// Local variables used for logging
#define MAX_SAMPLES 20
static EXT_RAM_ATTR int64_t mean_delta_time;
static EXT_RAM_ATTR int64_t max_delta_time;
static EXT_RAM_ATTR int64_t delta_time_samples[MAX_SAMPLES];
static EXT_RAM_ATTR int32_t sample_index;
static bool first_iter = false;
#endif

/**
 * @brief Initializes the DICM log service
 *
 * @return 1 if successful
 */
int dicm_log_service_initialize(void)
{
    memset(&l_dicm_log_service, 0, sizeof(l_dicm_log_service));
    // Allocate buffer
    l_dicm_log_service.storage = heap_caps_malloc_prefer(CONNECTOR_LOG_SERVICE_RINGBUFFER_SIZE, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
    TRUE_CHECK_RETURN0(l_dicm_log_service.storage != NULL);
    l_dicm_log_service.rbufHandle = xRingbufferCreateStatic(CONNECTOR_LOG_SERVICE_RINGBUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, l_dicm_log_service.storage, &l_dicm_log_service.ringbuffer);
    TRUE_CHECK_RETURN0(l_dicm_log_service.rbufHandle != NULL);
#if !defined(CONFIG_IDF_TARGET_LINUX)
#if CONFIG_LOG_SERVICE_EXECUTOR == LOG_SERVICE_IN_IDLE_TASK
    // Register our idle task callback function
    esp_register_freertos_idle_hook(log_service_idle_cb);
#endif

#if CONFIG_LOG_SERVICE_EXECUTOR == LOG_SERVICE_IN_CONNECTOR_TASK
    TRUE_CHECK_RETURN0(xTaskCreate(log_service_task, "log_service_task", 3584, 0, xTASK_PRIORITY_IDLE, NULL));
#endif
    // Initialize to use our own vprintf, old will be used in "internal logging"
    old_vprintf_func = esp_log_set_vprintf(log_service_vprintf);
#endif
#if CONFIG_LOG_SERVICE_EXECUTOR == LOG_SERVICE_IN_IDLE_TASK
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
    // Todo create task for this configuration
#error Not implemented yet!
#endif
#endif
#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &l_dicm_log_service.server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &l_dicm_log_service.server));

#endif
    LOG(I, "DICM logging service installed");
    return 1;
}

#if !defined(CONFIG_IDF_TARGET_LINUX)
#if CONFIG_LOG_SERVICE_EXECUTOR == LOG_SERVICE_IN_CONNECTOR_TASK
static void log_service_task(void *parameter)
{
    while (1)
    {
        log_service_idle_cb();
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
        log_service_mean(elapsed);
#endif
    }
}
#endif
#endif
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
/**
 * @brief Connector task function used when measuring log execution times.
 *
 * @param elapsed Elapsed time in ms since last call
 */
static void log_service_mean(uint16_t elapsed)
{
    static uint16_t timer_1s = 0;  // Every 1 seconds
    if (timer16_is_due(&timer_1s, elapsed, 500))
    {
        int64_t sum = 0;
        if (first_iter)
        {
            for (int i = 0; i < MAX_SAMPLES; i++)
            {
                sum += delta_time_samples[i];
            }
            mean_delta_time = sum / MAX_SAMPLES;
        }
        else
        {
            int32_t numsamples = sample_index;
            for (int i = 0; i < numsamples; i++)
            {
                sum += delta_time_samples[i];
            }
            mean_delta_time = sum / numsamples;
        }
        LOG(W, "Mean/max: %ld/%ld", (long int)mean_delta_time, (long int)max_delta_time);
    }
}
#endif

/**
 * @brief Internal log function that will use orig IDF vprintf callback
 *
 * @param[in] format Print format string
 * @return Number of "printed" characters
 */
static int internal_logger(const char *format, ...)
{
    va_list varlist;
    int n = 0;
    va_start(varlist, format);
    if (old_vprintf_func != NULL)
    {
        n = old_vprintf_func(format, varlist);
    }
    va_end(varlist);
    return n;
}

/**
 * @brief Log service vprintf function callback
 *
 * @param[in] format Format string
 * @param[in] varlist Variable list containing all arguments
 * @return Number for bytes to be printed
 */

#if defined(CONFIG_IDF_TARGET_LINUX)
int log_service_vprintf(const char *format, va_list varlist)
#else
static int log_service_vprintf(const char *format, va_list varlist)
#endif
{
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
    volatile int64_t starttime = hal_cpu_get_micros();
#endif
#if CONFIG_LOG_SERVICE_TIME_PROFILING_ORIGINAL == 0
    static uint32_t num_skipped_logs = 0;
    arg_list arglist = {0};
    arglist.num = 0;
    if (pdTRUE == xPortInIsrContext())
    {
        // Do not accept logs from interrupts
        return 0;
    }
    // Calculates the final length to print
    int num = npf_vsnprintf(&arglist, NULL, 0, format, varlist);
    log_service_message_t *log_msg;
    if (pdFALSE == xRingbufferSendAcquire(l_dicm_log_service.rbufHandle, (void **)&log_msg, sizeof(log_service_message_t) + arglist.num * sizeof(arglist.args[0]), CONFIG_LOG_SERVICE_TICKS_TO_WAIT))
    {
        // Increase counter and free any allocated memory
        ++num_skipped_logs;
        for (int i = 0; i < arglist.num; i++)
        {
            if (arglist.allocs[i])
            {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
                free((void *)(intptr_t)arglist.args[i].vm);
#else
                free((void *)(int32_t)arglist.args[i].vi);
#endif
            }
        }
    }
    else
    {
        if (num_skipped_logs > 0)
        {
            internal_logger("Skipped %" PRIu32 " logs", num_skipped_logs);
            num_skipped_logs = 0;
        }
        // Prepare log message
        log_msg->alloc = (uint32_t)0;
        memcpy((char *)log_msg + offsetof(log_service_message_t, values), (char *)&arglist.args[0], arglist.num * sizeof(arglist.args[0]));
        log_msg->format = format;
        log_msg->num = arglist.num;
        // Find allocated strings and pack information
        for (int i = 0; i < arglist.num; i++)
        {
            if (arglist.allocs[i])
            {
                log_msg->alloc |= (1 << i);
            }
        }
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
        volatile int64_t endtime = hal_cpu_get_micros();
        log_msg->delta_time = endtime - starttime;
#endif
        xRingbufferSendComplete(l_dicm_log_service.rbufHandle, log_msg);
    }
#else
    int num = old_vprintf_func(format, varlist);
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
    volatile int64_t endtime = hal_cpu_get_micros();
    if (max_delta_time < (endtime - starttime))
    {
        max_delta_time = endtime - starttime;
    }
    delta_time_samples[sample_index++] = endtime - starttime;
    if (sample_index >= MAX_SAMPLES)
    {
        sample_index = 0;
        first_iter = true;
    }
#endif
#endif
    return num;
}

/**
 * @brief Idle callback function to be called by IDLE task to perform the actual string formatting and printing functionality.
 *
 * @return false if we want to be called again, more data to be processed, true if we can go to PM-sleep
 */
#if defined(CONFIG_IDF_TARGET_LINUX)
bool log_service_idle_cb(char *log_buffer, size_t log_buffer_size)
#else
static bool log_service_idle_cb(void)
#endif
{
    static arg_list l_arglist;
#if !defined(CONFIG_IDF_TARGET_LINUX)
#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
    static char log_buffer[WEB_SERVER_LOG_BUFFER_SIZE];  // log buffer
#else
    static char log_buffer[1];  // Dummy buffer
#endif
    const size_t log_buffer_size = sizeof(log_buffer);
#endif
    va_list va_list_dummy;
    size_t msg_size = 0;
    size_t values_size = 0;
    log_service_message_t *log_msg = NULL;

#if CONFIG_LOG_SERVICE_EXECUTOR == LOG_SERVICE_IN_CONNECTOR_TASK
    log_msg = (log_service_message_t *)xRingbufferReceive(l_dicm_log_service.rbufHandle, &msg_size, portMAX_DELAY);
#else
    // NOTE: not allowed to block here!
    log_msg = (log_service_message_t *)xRingbufferReceive(l_dicm_log_service.rbufHandle, &msg_size, 0);
#endif
    if (log_msg && (msg_size > 0))
    {
        values_size = msg_size - offsetof(log_service_message_t, values);
        l_arglist.num = values_size / sizeof(l_arglist.args[0]);
        assert((l_arglist.num < CONFIG_LOG_SERVICE_MAX_ARGUMENTS) && "To much data received");
        memcpy((char *)&l_arglist.args[0], log_msg->values, values_size);
        // Un-pack allocation information
        for (int i = 0; i < l_arglist.num; i++)
        {
            if ((1 << i) & log_msg->alloc)
            {
                l_arglist.allocs[i] = 1;
            }
            else
            {
                l_arglist.allocs[i] = 0;
            }
        }
        l_arglist.num = 0;  // Reset needed for parsing in correct order
        log_buffer[0] = '\0';
        npf_vsnprintf(&l_arglist, log_buffer, log_buffer_size, log_msg->format, va_list_dummy);
        fflush(stdout);
        for (int i = 0; i < log_msg->num; i++)
        {
            if (l_arglist.allocs[i])
            {
                // Time to release allocated memory
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
                free((void *)(intptr_t)l_arglist.args[i].vm);
#else
                free((void *)(int32_t)l_arglist.args[i].vi);
#endif
            }
        }
#if CONFIG_LOG_SERVICE_TIME_PROFILING == 1
        if (max_delta_time < (log_msg->delta_time))
        {
            max_delta_time = log_msg->delta_time;
            internal_logger("MAX:%d\n", (int)max_delta_time);
        }
        delta_time_samples[sample_index++] = log_msg->delta_time;
        if (sample_index >= MAX_SAMPLES)
        {
            sample_index = 0;
            first_iter = true;
        }

#endif
        // Return buffer
        vRingbufferReturnItem(l_dicm_log_service.rbufHandle, log_msg);
#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
        // log_buffer will always be null terminated
        if (trigger_async_send(log_buffer))
        {
            printf("error in trigger_async_send\n");
        }
#endif

        // We could have more to process
        return false;
    }
    else
    {
        // Nothing to be processed. We are ready for sleep
        return true;
    }
}

#ifdef CONFIG_LOG_SERVICE_WEB_SERVER
static void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        LOG(I, "Starting webserver");
        *server = start_webserver();
    }
}
static void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        LOG(I, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)
        {
            *server = NULL;
        }
        else
        {
            LOG(E, "Failed to stop http server");
        }
    }
}

// URI handler structures
static const httpd_uri_t index_html = {
    .uri = "/index.html",
    .method = HTTP_GET,
    .handler = index_html_get_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t favicon = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = favicon_get_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t websocket = {
    .uri = "/logconsole",
    .method = HTTP_GET,
    .handler = websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true,
};

static const httpd_uri_t logsocket = {
    .uri = "/log",
    .method = HTTP_GET,
    .handler = log_server_handler,
    .user_ctx = NULL,
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.keep_alive_enable = true;
    config.max_open_sockets = 10;
    // Start the httpd server
    LOG(I, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Registering the handlers
        LOG(I, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_html);
        httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &websocket);
        httpd_register_uri_handler(server, &logsocket);
        return server;
    }

    LOG(E, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    LOG(D, "Stopping http server");
    return httpd_stop(server);
}

static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/log");
    httpd_resp_send(req, NULL, 0);  // Response body can be empty

    return ESP_OK;
}

static esp_err_t log_server_handler(httpd_req_t *req)
{
    // Serve HTTP_GET
    extern const unsigned char weblog_html_start[] asm("_binary_weblog_html_start");
    extern const unsigned char weblog_html_end[] asm("_binary_weblog_html_end");
    const size_t weblog_html_size = (weblog_html_end - weblog_html_start);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)weblog_html_start, weblog_html_size);
    LOG(D, "log_server_handler");
    return ESP_OK;
}

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[] asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

static esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        LOG(D, "Handshake done, the new connection was opened");
    }
    return ESP_OK;
}

/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg)
{
    async_resp_arg_t *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)resp_arg->data;
    ws_pkt.len = strlen(resp_arg->data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.final = true;
    ws_pkt.fragmented = false;
    if (httpd_ws_send_frame_async(hd, fd, &ws_pkt) != ESP_OK)
    {
        LOG(E, "Failed to send");
    }
    free(resp_arg);
}

static esp_err_t trigger_async_send(const char *output)
{
    esp_err_t ret = ESP_OK;
    size_t clients = 20;
    int client_fds[clients];
    int printed = strlen(output);

    if ((printed > 0) && (httpd_get_client_list(l_dicm_log_service.server, &clients, client_fds) == ESP_OK))
    {
        for (size_t i = 0; i < clients; ++i)
        {
            int sock = client_fds[i];
            if (httpd_ws_get_fd_info(l_dicm_log_service.server, sock) == HTTPD_WS_CLIENT_WEBSOCKET)
            {
                async_resp_arg_t *resp_arg = calloc(1, sizeof(async_resp_arg_t));
                if (resp_arg == NULL)
                {
                    return ESP_ERR_NO_MEM;
                }
                resp_arg->hd = l_dicm_log_service.server;
                resp_arg->fd = sock;
                strcpy(resp_arg->data, output);
                ret = httpd_queue_work(l_dicm_log_service.server, ws_async_send, resp_arg);
                if (ret != ESP_OK)
                {
                    free(resp_arg);
                    break;
                }
            }
        }
    }
    return ret;
}
#endif
