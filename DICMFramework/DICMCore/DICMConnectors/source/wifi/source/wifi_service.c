/*****************************************************************************
 * \file       wifi_service.c
 * \brief      WIFI UDP and TCP Services for DICM DDM2
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/

#include "configuration.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef CONNECTOR_TLS
#include "connector_tls.h"
#include "esp_tls.h"
#include "esp_tls_crypto.h"
#endif
#include "connector_wifi.h"

#include "broker.h"
#include "cJSON.h"
#include "ddm2.h"
#include "esp_event.h"
#include "freertos/semphr.h"
#include "fsm.h"
#include "mbedtls/base64.h"
#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
#include "network_discovery.h"
#endif
#include "sorted_list.h"
#include "sorted_list64.h"
#include "wifi_network.h"
#include "wifi_service.h"

#ifndef CONFIG_IDF_TARGET_LINUX
#define STATIC_TEST static
#else
#define STATIC_TEST
#endif
#define INVALID_SOCKET            0
#define STOP_SERVICE_SYNC_TIME_MS 250
#define TCP_SERVER_SYNC_BIT       (1 << 0)  // bit 0
#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
#define UDP_SERVER_SYNC_BIT       (1 << 1)  // bit 1
#endif

#define WIFI_SERVICE_STATE_MACHINE_EVENT_GENERIC_PARAMETER 0

typedef struct wifi_service
{
    ws_fsm_t ws_sm;

    CONNECTOR *connector;

    int tcp_port;
    int tcp_sockets[CONNECTOR_WIFI_MAX_CONNECTIONS + 1];
    fd_set tcp_read_socket_set;
    fd_set tcp_write_socket_set;
    fd_set tcp_exception_socket_set;

    int max_socket;
    int socket_count;
    SemaphoreHandle_t socket_mutex;

#ifdef CONNECTOR_TLS
#ifndef CONFIG_IDF_TARGET_LINUX
    esp_tls_t *tls_ctxs[CONNECTOR_WIFI_MAX_CONNECTIONS + 1];
#endif  // CONFIG_IDF_TARGET_LINUX
    char *tls_cert;
    char *tls_key;
    bool tls_valid;
    SemaphoreHandle_t tls_mutex;
#endif  // CONNECTOR_TLS

#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
    int udp_port;
    int udp_socket;
#endif

    bool started;
    EventGroupHandle_t event_group;
} wifi_service_t;

static wifi_service_t *service_instance = NULL;

static int socket_get_error_code(int socket);
static void socket_enable_keepalive(const int socket);
static int tcp_service_add_socket(wifi_service_t *service, const int socket, const int client);
static int tcp_service_disconnect_socket(wifi_service_t *service, int index);
static void tcp_service_task(void *task_param);
static esp_err_t tcp_service_start(wifi_service_t *service);
#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
static void udp_service_task(void *task_param);
static esp_err_t udp_service_start(wifi_service_t *service);
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void connection_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void disconnection_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
#endif  // CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
#endif
#ifndef CONFIG_IDF_TARGET_LINUX
// FSM states
static void sm_wifi_service_startup(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_wifi_service_waiting(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_wifi_service_active(fsm_t *const fsm, fsm_event_t const *const event);
#endif
// FSM state support functions
static bool sm_wifi_service_handle_connection_state_event(fsm_t *const fsm, fsm_event_t const *const event);
static void sm_wifi_service_generate_event(uint32_t id);

static int socket_get_error_code(int socket)
{
    int result;
    socklen_t optlen = sizeof(int);

    if (!socket)
    {
        LOG(I, "Socket closed");
        return -1;
    }

    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen) == -1)
    {
        LOG(W, "Getsockopt failed");
        return -1;
    }

    return result;
}

static void socket_enable_keepalive(const int socket)
{
    const int keepalive = 1;
    const int keepalive_idle = 3;
    const int keepalive_interval = 3;
    const int keepalive_count = 3;

    setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepalive_idle, sizeof(keepalive_idle));
    setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepalive_interval, sizeof(keepalive_interval));
    setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &keepalive_count, sizeof(keepalive_count));

    // Set socket to non-blocking mode for use with select()
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags != -1)
    {
        if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            LOG(E, "Failed to set socket %d to non-blocking mode", socket);
        }
    }
}

#ifdef CONNECTOR_TLS
#ifndef CONFIG_IDF_TARGET_LINUX
static int tcp_tls_recv(wifi_service_t *service, int index, uint8_t *buffer, int size)
{
    if (service->tls_ctxs[index] == NULL)
    {
        LOG(E, "TLS context is NULL for socket %d", service->tcp_sockets[index]);
        return -1;
    }

    return esp_tls_conn_read(service->tls_ctxs[index], buffer, size);
}

static int tcp_tls_send(wifi_service_t *service, int index, const uint8_t *buffer, int size)
{
    if (service->tls_ctxs[index] == NULL)
    {
        LOG(E, "TLS context is NULL for socket %d", service->tcp_sockets[index]);
        return -1;
    }

    return esp_tls_conn_write(service->tls_ctxs[index], buffer, size);
}
#endif  // CONFIG_IDF_TARGET_LINUX
#endif  // CONNECTOR_TLS

static int tcp_socket_recv(wifi_service_t *service, int index, uint8_t *buffer, int size)
{
    return recv(service->tcp_sockets[index], buffer, size, 0);
}

static int tcp_socket_send(wifi_service_t *service, int index, const uint8_t *buffer, int size)
{
    return send(service->tcp_sockets[index], buffer, size, 0);
}

static int tcp_service_receive(wifi_service_t *service, int index, uint8_t *buffer, int size)
{
    int ret = -1;
#ifdef CONNECTOR_TLS
#ifndef CONFIG_IDF_TARGET_LINUX
    if (service->tls_ctxs[index])
    {
        ret = tcp_tls_recv(service, index, buffer, size);
    }
    else
#endif  // CONFIG_IDF_TARGET_LINUX
#endif  // CONNECTOR_TLS
    {
        ret = tcp_socket_recv(service, index, buffer, size);
    }

    return ret;
}

static int tcp_service_send(wifi_service_t *service, int index, const uint8_t *buffer, int size)
{
    int ret = -1;
#ifdef CONNECTOR_TLS
#ifndef CONFIG_IDF_TARGET_LINUX
    if (service->tls_ctxs[index])
    {
        ret = tcp_tls_send(service, index, buffer, size);
    }
    else
#endif  // CONFIG_IDF_TARGET_LINUX
#endif  // CONNECTOR_TLS
    {
        ret = tcp_socket_send(service, index, buffer, size);
    }

    return ret;
}

static int tcp_service_add_socket(wifi_service_t *service, const int socket, const int client)
{
    TRUE_CHECK(xSemaphoreTake(service->socket_mutex, portMAX_DELAY));
#ifdef CONNECTOR_TLS
    bool tls_valid = false;
    TRUE_CHECK(xSemaphoreTake(service->tls_mutex, portMAX_DELAY));
    tls_valid = service->tls_valid;
    TRUE_CHECK(xSemaphoreGive(service->tls_mutex));
#endif  // CONNECTOR_TLS

    for (int pos = 1; pos <= CONNECTOR_WIFI_MAX_CONNECTIONS; pos++)
    {
        if (!service->tcp_sockets[pos])
        {
#ifdef CONNECTOR_TLS
#ifndef CONFIG_IDF_TARGET_LINUX
            service->tls_ctxs[pos] = NULL;
#endif  // CONFIG_IDF_TARGET_LINUX
#endif  // CONNECTOR_TLS

#ifdef CONNECTOR_TLS
            // Create a TLS context for a server socket
            if (!client && tls_valid)
            {
#ifndef CONFIG_IDF_TARGET_LINUX
                esp_tls_cfg_server_t tls_cfg = {
                    .servercert_pem_buf = (const unsigned char *)service->tls_cert,
                    .servercert_pem_bytes = strlen((const char *)service->tls_cert) + 1,
                    .serverkey_pem_buf = (const unsigned char *)service->tls_key,
                    .serverkey_pem_bytes = strlen((const char *)service->tls_key) + 1,
                };

                service->tls_ctxs[pos] = esp_tls_init();
                if (!service->tls_ctxs[pos])
                {
                    LOG(E, "Failed to initialize TLS context");
                    TRUE_CHECK(xSemaphoreGive(service->socket_mutex));
                    return -1;
                }

                if (esp_tls_server_session_create(&tls_cfg, socket, service->tls_ctxs[pos]) != 0)
                {
                    LOG(E, "Failed to create TLS session!");
                    esp_tls_conn_destroy(service->tls_ctxs[pos]);
                    service->tls_ctxs[pos] = NULL;
                    TRUE_CHECK(xSemaphoreGive(service->socket_mutex));
                    return -1;
                }
#endif  // CONFIG_IDF_TARGET_LINUX
                LOG(I, "TLS session created.");
            }
#endif  // CONNECTOR_TLS
            service->tcp_sockets[pos] = socket;
            FD_SET(socket, client ? &service->tcp_write_socket_set : &service->tcp_read_socket_set);
            FD_SET(socket, &service->tcp_exception_socket_set);
            service->socket_count++;
            service->max_socket = MAX(service->max_socket, socket + 1);
            TRUE_CHECK(xSemaphoreGive(service->socket_mutex));

            LOG(I, "Added %s socket %d (%d), %d active connection(s)", client ? "client" : "server", socket, pos, service->socket_count - 1);

            return pos;
        }
    }

    TRUE_CHECK(xSemaphoreGive(service->socket_mutex));

    return -1;
}

static int tcp_service_disconnect_socket(wifi_service_t *service, int index)
{
    CONNECTOR *connector = service->connector;

    BOUNDED(index, CONNECTOR_WIFI_MAX_CONNECTIONS + 1);

    TRUE_CHECK(xSemaphoreTake(service->socket_mutex, portMAX_DELAY));

    int socket = service->tcp_sockets[index];

    if (socket)
    {
#ifdef CONNECTOR_TLS
#ifndef CONFIG_IDF_TARGET_LINUX
        if (service->tls_ctxs[index])
        {
            LOG(C, "Destroying TLS context");
            esp_tls_conn_destroy(service->tls_ctxs[index]);
            service->tls_ctxs[index] = NULL;
        }
#endif  // CONFIG_IDF_TARGET_LINUX
#endif  // CONNECTOR_TLS
        service->tcp_sockets[index] = INVALID_SOCKET;
        ZERO_CHECK(close(socket));
        LOG(I, "Socket %d closed with status '%s'", socket, strerror(errno));
        FD_CLR(socket, &service->tcp_read_socket_set);
        FD_CLR(socket, &service->tcp_write_socket_set);
        FD_CLR(socket, &service->tcp_exception_socket_set);
        service->socket_count--;
        LOG(I, "Disconnected socket %d (%d), %d active connection(s) left", socket, index, service->socket_count - 1);
        connector_send_frame_to_broker(DDMP2_CONTROL_MESSAGE, DDMP2_MESSAGE_RESET, NULL, 0, connector->connector_id + index, portMAX_DELAY);
    }
    else
    {
        LOG(I, "Socket at index %d already disconnected, %d active connection(s) left", index, service->socket_count - 1);
    }

    TRUE_CHECK(xSemaphoreGive(service->socket_mutex));

    return socket;
}

static int tcp_service_find_separator(const uint8_t *const buffer, const int pos)
{
    for (int i = 0; i < pos; i++)
    {
        if (buffer[i] == '\r')
        {
            return i;
        }
    }
    return -1;
}

static int tcp_service_parse_base64(wifi_service_t *service, DDMP2_FRAME *const pframe,
                                    const uint8_t *const base64, const int base64_length,
                                    const int data_line)
{
    TRUE_CHECK_RETURN0(NULL != pframe);
    TRUE_CHECK_RETURN0(NULL != base64);

    size_t binary_length;
    int raw_frame_result, decode_result;
    uint8_t binary_buffer[256];
    CONNECTOR *connector = service->connector;

    if (!base64_length)
    {
        LOG(W, "Empty BASE64 string!");
        return 0;
    }

    decode_result = mbedtls_base64_decode(binary_buffer, sizeof(binary_buffer), &binary_length, base64, base64_length);
    switch (decode_result)
    {
    case MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL:
        LOG(W, "BASE64 data too large to be decoded!");
        return 0;
    case MBEDTLS_ERR_BASE64_INVALID_CHARACTER:
        LOG(W, "BASE64 decode error: %s", base64);
        return 0;
        break;
    default:
        break;
    }

    raw_frame_result = ddmp2_create_raw_frame(pframe, binary_buffer, binary_length, connector->connector_id + data_line);
    if (!raw_frame_result)
    {
        LOG(W, "Broken BASE64 frame received! ");
        ESP_LOG_BUFFER_HEXDUMP("RAW", binary_buffer, binary_length, ESP_LOG_WARN);
    }

    return raw_frame_result;
}

static void tcp_service_handle_frame(const DDMP2_FRAME *pframe, const int socket, const int data_line, const uint8_t connector_id)
{
    TRUE_CHECK_RETURN(NULL != pframe);

    switch (pframe->frame.control)
    {
    default:
        LOG(D, "Forwarding %s from WiFi:%d to broker", ddmp2_control_string(pframe->frame.control), data_line);
        TRUE_CHECK(connector_forward_frame_to_broker(pframe));
        break;

    case DDMP2_CONTROL_MESSAGE:
        switch (pframe->frame.message.id)  // which message id?
        {
        case DDMP2_MESSAGE_RESET:  // reset message, client has just started up
            LOG(I, "Forwarding reset to broker");
            TRUE_CHECK(connector_forward_frame_to_broker(pframe));
            break;

        case DDMP2_MESSAGE_PING:  // reply to ping message
            LOG(D, "Replying to ping message from Wi-Fi connector (%d)", data_line);

            DECLARE_DDMP2_MESSAGE_FRAME(Ping_reply, DDMP2_MESSAGE_PINGREPLY, connector_id);

            TRUE_CHECK(wifi_service_send_frame((DDMP2_FRAME *)&Ping_reply));
            break;

        case DDMP2_MESSAGE_ERROR:
            LOG(W, "Wi-Fi Protocol error");
            break;

        default:
            LOG(W, "Unhandled Wi-Fi message %s (%d)", ddmp2_message_string(pframe->frame.message.id), pframe->frame.message.id);
            break;
        }
        break;

    case DDMP2_CONTROL_NOP:
        break;
    }
}

static void tcp_service_task(void *task_param)
{
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len = sizeof(remote_addr);
    int recv_length;
    int bufferpos[CONNECTOR_WIFI_MAX_CONNECTIONS + 1] = {0};
    int separatorpos;
    int socket_slot;
    int new_socket;
    int activity;
    bool server_stop = false;
    fd_set read_set;
    fd_set exception_set;
    fd_set write_set;
    DDMP2_FRAME frame;
    wifi_service_t *service = (wifi_service_t *)task_param;
    static EXT_RAM_ATTR uint8_t *tcp_buffer[CONNECTOR_WIFI_MAX_CONNECTIONS];

    for (int i = 0; i < CONNECTOR_WIFI_MAX_CONNECTIONS; i++)
    {
        tcp_buffer[i] = hal_mem_malloc_prefer(RECEIVE_BUFFER_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        TRUE_CHECK(NULL != tcp_buffer[i]);
    }

    service->max_socket = service->tcp_sockets[0] + 1;

    while (!server_stop)
    {
        // Copy fd_sets and max_socket with mutex protection to avoid race conditions
        TRUE_CHECK(xSemaphoreTake(service->socket_mutex, portMAX_DELAY));
        read_set = service->tcp_read_socket_set;
        write_set = service->tcp_write_socket_set;
        exception_set = service->tcp_exception_socket_set;
        int current_max_socket = service->max_socket;
        TRUE_CHECK(xSemaphoreGive(service->socket_mutex));
        activity = select(current_max_socket, &read_set, &write_set, &exception_set, NULL);

        if (activity <= 0)
        {
            if (errno == EBADF)
            {
                // Socket closed, exit loop
                server_stop = true;
                LOG(W, "TCP server task: socket closed, exiting loop");
            }
            else if (activity < 0)
            {
                LOG(E, "select() failed: errno=%d (%s)", errno, strerror(errno));
            }
            continue;
        }

        if (FD_ISSET(service->tcp_sockets[0], &read_set))
        {
            NONNEG_CHECK(new_socket = accept(service->tcp_sockets[0], (struct sockaddr *)&remote_addr, &remote_addr_len));

            if ((service->socket_count > CONNECTOR_WIFI_MAX_CONNECTIONS) || (new_socket == -1))
            {
                LOG(W, "Connection rejected, no available connections (%d), errno: %d", new_socket, errno);
                if (new_socket > 0)
                {
                    ZERO_CHECK(close(new_socket));
                }
#ifdef CONFIG_IDF_TARGET_LINUX
                if (errno == EINVAL)
                {
                    // Bad socket descriptor, we are exiting the loop
                    server_stop = true;
                    LOG(W, "TCP server task: socket closed, exiting loop");
                    continue;
                }
#endif
            }
            else
            {
                socket_enable_keepalive(new_socket);

                LOG(I, "Accepting socket %d from %s, %d client(s)", new_socket, inet_ntoa(remote_addr.sin_addr), service->socket_count);
                socket_slot = tcp_service_add_socket(service, new_socket, 0);

                if (socket_slot < 0)
                {
                    LOG(W, "Sockets full!");
                    ZERO_CHECK(close(new_socket));
                }
                else
                {
                    bufferpos[socket_slot] = 0;
                }
            }
        }

        for (int i = 1; i <= CONNECTOR_WIFI_MAX_CONNECTIONS; i++)
        {
            if (FD_ISSET(service->tcp_sockets[i], &read_set))
            {
                do
                {
                    recv_length = tcp_service_receive(service, i, &tcp_buffer[i - 1][bufferpos[i]], RECEIVE_BUFFER_SIZE - bufferpos[i]);

                    if (recv_length <= 0)
                    {
                        break;
                    }

                    bufferpos[i] += recv_length;

                    while (((separatorpos = tcp_service_find_separator(tcp_buffer[i - 1], bufferpos[i]))) != -1)
                    {
                        tcp_buffer[i - 1][separatorpos] = 0;

                        if (tcp_service_parse_base64(service, &frame, tcp_buffer[i - 1], separatorpos, i))
                        {
                            tcp_service_handle_frame(&frame, service->tcp_sockets[i], i, service->connector->connector_id + i);
                        }

                        memmove(tcp_buffer[i - 1], &tcp_buffer[i - 1][separatorpos + 1], RECEIVE_BUFFER_SIZE - separatorpos - 1);
                        bufferpos[i] -= separatorpos + 1;
                    }

                    if (bufferpos[i] >= RECEIVE_BUFFER_SIZE)
                    {
                        LOG(W, "TCP Receive buffer overflow! Flushing buffer! (%d)", i);
                        bufferpos[i] = 0;
                    }
                } while (recv_length > 0);

                if ((recv_length < 0) && (errno != EWOULDBLOCK))
                {
                    LOG(W, "Read error on socket %d (%s)", service->tcp_sockets[i], strerror(errno));
                    tcp_service_disconnect_socket(service, i);
                }
            }

            if (FD_ISSET(service->tcp_sockets[i], &exception_set))
            {
                tcp_service_disconnect_socket(service, i);
            }

            if (FD_ISSET(service->tcp_sockets[i], &write_set))
            {
                FD_CLR(service->tcp_sockets[i], &service->tcp_write_socket_set);

                struct sockaddr_in peer = {0};
                socklen_t peer_size = sizeof(peer);
                int peername = getpeername(service->tcp_sockets[i], (struct sockaddr *)&peer, &peer_size);
                if (!peername)
                {
                    LOG(I, "Connection to %s established", inet_ntoa(peer.sin_addr));
                }
                else
                {
                    LOG(W, "Connection failed! %s (%d)", strerror(errno), errno);
                    tcp_service_disconnect_socket(service, i);
                }
            }
        }
    }

    for (int i = 0; i < CONNECTOR_WIFI_MAX_CONNECTIONS; i++)
    {
        if (tcp_buffer[i])
        {
            hal_mem_free(tcp_buffer[i]);
            tcp_buffer[i] = NULL;
        }
    }
    xEventGroupSetBits(service->event_group, TCP_SERVER_SYNC_BIT);
    vTaskDelete(NULL);
}

static esp_err_t tcp_service_start(wifi_service_t *service)
{
    LOG(W, "TCP port: %d", service->tcp_port);

    NONNEG_CHECK(service->tcp_sockets[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

    if (service->tcp_sockets[0] < 0)
    {
        int socket_error = socket_get_error_code(service->tcp_sockets[0]);
        LOG(E, "TCP socket() failed (%s)", esp_err_to_name(socket_error));
        return ESP_FAIL;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(service->tcp_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    // Bind socket to listening address
    if (bind(service->tcp_sockets[0], (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        int socket_error = socket_get_error_code(service->tcp_sockets[0]);
        LOG(E, "TCP bind() %s", esp_err_to_name(socket_error));
        ZERO_CHECK(close(service->tcp_sockets[0]));
        service->tcp_sockets[0] = INVALID_SOCKET;
        return ESP_FAIL;
    }

    if (listen(service->tcp_sockets[0], 5) < 0)
    {
        int socket_error = socket_get_error_code(service->tcp_sockets[0]);
        LOG(E, "TCP listen() %s", esp_err_to_name(socket_error));
    }

    FD_SET(service->tcp_sockets[0], &service->tcp_read_socket_set);

    // Start the TCP server task
    TRUE_CHECK(xTaskCreate(
        tcp_service_task,
        "tcp_service_task",
        4096,
        service,
        xTASK_PRIORITY_BELOW_NORMAL,
        NULL));

    return ESP_OK;
}

static void tcp_service_stop(wifi_service_t *service)
{
    if (service->tcp_sockets[0] > 0)
    {
        close(service->tcp_sockets[0]);
        service->tcp_sockets[0] = INVALID_SOCKET;
    }

    for (int i = 1; i <= CONNECTOR_WIFI_MAX_CONNECTIONS; i++)
    {
        tcp_service_disconnect_socket(service, i);
    }
    // wait until stop event complete (task deleted)
    EventBits_t bits = xEventGroupWaitBits(service->event_group, TCP_SERVER_SYNC_BIT, pdTRUE, pdTRUE, STOP_SERVICE_SYNC_TIME_MS / portTICK_PERIOD_MS);
    if ((bits & TCP_SERVER_SYNC_BIT) != TCP_SERVER_SYNC_BIT)
    {
        LOG(E, "Failed to sync tcp_service_stop");
    }
    else
    {
        LOG(I, "tcp_service_stop done");
    }
}

#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
static void udp_service_task(void *task_param)
{
    int len;
    bool server_stop = false;
    socklen_t socklen;
    uint8_t udp_buffer[NETWORK_DISCOVERY__MAX_REQUEST_SIZE];
    struct sockaddr_in remote_addr;
    wifi_service_t *service = (wifi_service_t *)task_param;

    while (!server_stop)
    {
        socklen = sizeof(remote_addr);
        len = recvfrom(service->udp_socket, udp_buffer, sizeof(udp_buffer),
                       0, (struct sockaddr *)&remote_addr, &socklen);
        if (len <= 0)
        {
            if (errno == EBADF)
            {
                // Socket closed, exit loop
                server_stop = true;
                LOG(W, "UDP server task: socket closed, exiting loop");
            }
#ifdef CONFIG_IDF_TARGET_LINUX
            else if (errno)
            {
                // Anything is wrong, Bailout
                server_stop = true;
                LOG(W, "UDP server task: socket closed (%d), exiting loop", errno);
            }
#endif
            else if (len < 0)
            {
                int socket_error = socket_get_error_code(service->udp_socket);
                LOG(E, "recvfrom() failed (%s)", esp_err_to_name(socket_error));
            }
            continue;
        }

        int discovery_error = network_discovery__request_validate(udp_buffer, len);
        switch (discovery_error)
        {
        case NETWORK_DISCOVERY__ERROR_INVALID_SIZE:
            LOG(W, "Invalid packet length %d, from %s:%u", len, inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
            ESP_LOG_BUFFER_HEXDUMP("UDP", udp_buffer, len, ESP_LOG_WARN);
            break;
        case NETWORK_DISCOVERY__ERROR_DATA_MISMATCH:
            LOG(W, "Unknown packet data!");
            break;
        case NETWORK_DISCOVERY__ERROR_OK:
        {
            void *data;
            size_t size;

            LOG(D, "Discovery packet from %s:%u", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));

            if (network_discovery__response_generate(&data, &size) == NETWORK_DISCOVERY__ERROR_OK)
            {
                cJSON *discovery_json = cJSON_Parse(data);
                char *discovery_response;

                if (!discovery_json)
                {
                    LOG(E, "Malformed discovery response JSON: %s", (char *)data);
                    network_discovery__response_free(data);
                    continue;
                }

#ifdef CONNECTOR_TLS
                cJSON_AddBoolToObject(discovery_json, "tls", service->tls_valid);
#endif
                discovery_response = cJSON_PrintUnformatted(discovery_json);
                if (!discovery_response)
                {
                    discovery_response = (char *)data;
                }

                NONNEG_CHECK(
                    len = sendto(service->udp_socket, discovery_response,
                                 strlen(discovery_response), 0,
                                 (struct sockaddr *)&remote_addr,
                                 sizeof(remote_addr)));

                if (errno == EBADF)
                {
                    // Socket closed, exit loop
                    server_stop = true;
                    LOG(W, "UDP server task: socket closed, exiting loop");
                }

                if (discovery_response != (char *)data)
                {
                    cJSON_free(discovery_response);
                }

                cJSON_Delete(discovery_json);
                network_discovery__response_free(data);
            }
            else
            {
                LOG(W, "Failed to generate response: no memory available");
            }
            break;
        }
        default:
            LOG(W, "Unhandled application discovery error %u", discovery_error);
            break;
        }
    }
    xEventGroupSetBits(service->event_group, UDP_SERVER_SYNC_BIT);
    vTaskDelete(NULL);
}

static esp_err_t udp_service_start(wifi_service_t *service)
{
    LOG(W, "UDP port: %d", service->udp_port);

    NONNEG_CHECK(service->udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

    if (service->udp_socket < 0)
    {
        int socket_error = socket_get_error_code(service->udp_socket);
        LOG(E, "UDP socket() failed (%s)", esp_err_to_name(socket_error));
        return ESP_FAIL;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(service->udp_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(service->udp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        int socket_error = socket_get_error_code(service->udp_socket);
        LOG(E, "UDP bind() failed (%s)", esp_err_to_name(socket_error));
        ZERO_CHECK(close(service->udp_socket));
        service->udp_socket = INVALID_SOCKET;
        return ESP_FAIL;
    }

    // Start the UDP server task
    TRUE_CHECK(xTaskCreate(
        udp_service_task,
        "udp_service_task",
        4096,
        service,
        xTASK_PRIORITY_BELOW_NORMAL,
        NULL));

    // DTLS

    return ESP_OK;
}

static void udp_service_stop(wifi_service_t *service)
{
    if (service->udp_socket > 0)
    {
        close(service->udp_socket);
        service->udp_socket = INVALID_SOCKET;
    }
    // Wait until stop event complete (task deleted)
    EventBits_t bits = xEventGroupWaitBits(service->event_group, UDP_SERVER_SYNC_BIT, pdTRUE, pdTRUE, STOP_SERVICE_SYNC_TIME_MS / portTICK_PERIOD_MS);
    if ((bits & UDP_SERVER_SYNC_BIT) != UDP_SERVER_SYNC_BIT)
    {
        LOG(E, "Failed to sync udp_service_stop");
    }
    else
    {
        LOG(I, "udp_service_stop done");
    }
}
#endif

static void wifi_service_start_all(wifi_service_t *service)
{
    if (service->started)
    {
        return;
    }

    LOG(I, "Starting Wi-Fi services.");
#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
    udp_service_start(service);
#endif
    tcp_service_start(service);

    service->started = true;
}

static void wifi_service_stop_all(wifi_service_t *service)
{
    if (!service->started)
    {
        return;
    }

    LOG(W, "Stopping Wi-Fi services.");

    tcp_service_stop(service);
#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
    udp_service_stop(service);
#endif

    service->started = false;
}

static void wifi_service_ap_event_handler(WIFI_NETWORK__AP_EVENT_ENUM event)
{
    wifi_service_t *service = service_instance;

    if (!service)
    {
        LOG(E, "WiFi service instance is NULL!");
        return;
    }
    switch (event)
    {
    case WIFI_NETWORK__AP_EVENT_STARTING:
        LOG(D, "WIFI_NETWORK__AP_EVENT_STARTING");
        sm_wifi_service_generate_event(WIFI_SERVICE_AP_STARTED_EVENT);
        break;
    case WIFI_NETWORK__AP_EVENT_STOPPING:
        LOG(D, "WIFI_NETWORK__AP_EVENT_STOPPING");
        sm_wifi_service_generate_event(WIFI_SERVICE_AP_STOPPED_EVENT);
        break;
    default:
        break;
    }
}

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/**
 * @brief esp_event callback for IP_EVENT_STA_GOT_IP event
 *
 */
static void connection_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    LOG(D, "STA connected");
    sm_wifi_service_generate_event(WIFI_SERVICE_STA_CONNECTED_EVENT);
}

/**
 * @brief esp_event callback for WIFI_EVENT_STA_DISCONNECTED event
 *
 */
static void disconnection_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    LOG(D, "STA disconnected");
    sm_wifi_service_generate_event(WIFI_SERVICE_STA_DISCONNECTED_EVENT);
}
#endif  // CONNECTOR_WIFI_SERVICE_ON_AP_ONLY

/**
 * @brief FSM state function for startup state
 *
 * Waiting in this state until TLS is valid. Handle all connection state events while waiting for TLS.
 *
 * @param[in] fsm Pointer to the fsm_t structure
 * @param[in] event Pointer to the fsm_event_t structure, holding event id
 */
STATIC_TEST void sm_wifi_service_startup(fsm_t *const fsm, fsm_event_t const *const event)
{
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        // If configured to not use TLS then change state to 'waiting'
#ifndef CONNECTOR_TLS
        fsm_state_change(fsm, FSM_STATE_HANDLER(sm_wifi_service_waiting));
#endif
        break;
#ifdef CONNECTOR_TLS
    case WIFI_SERVICE_TLS_VALID_EVENT:
        LOG(D, "WIFI_SERVICE_TLS_VALID_EVENT");
        fsm_state_change(fsm, FSM_STATE_HANDLER(sm_wifi_service_waiting));
        break;
#endif
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_SERVICE_STA_CONNECTED_EVENT:
        // fallthrough
    case WIFI_SERVICE_STA_DISCONNECTED_EVENT:
        // fallthrough
#endif  // CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_SERVICE_AP_STARTED_EVENT:
        // fallthrough
    case WIFI_SERVICE_AP_STOPPED_EVENT:
        sm_wifi_service_handle_connection_state_event(fsm, event);
        break;
    default:
        break;
    }
}

/**
 * @brief FSM state function for waiting state
 *
 * All configured requirements must be met until wifi services can start
 *
 * @param[in] fsm Pointer to the fsm_t structure
 * @param[in] event Pointer to the fsm_event_t structure, holding event id
 */
STATIC_TEST void sm_wifi_service_waiting(fsm_t *const fsm, fsm_event_t const *const event)
{
    ws_fsm_t *sm = (ws_fsm_t *)fsm;
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        if (sm->sm_bit)
        {
            // Already in active
            fsm_state_change(fsm, FSM_STATE_HANDLER(sm_wifi_service_active));
            break;
        }
        break;
#ifdef CONNECTOR_TLS
    case WIFI_SERVICE_TLS_INVALID_EVENT:
        // Change state to startup
        fsm_state_change(fsm, FSM_STATE_HANDLER(sm_wifi_service_startup));
        break;
#endif
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_SERVICE_STA_CONNECTED_EVENT:
        // fallthrough
#endif  // CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_SERVICE_AP_STARTED_EVENT:
        if (sm_wifi_service_handle_connection_state_event(fsm, event))
        {
            fsm_state_change(fsm, FSM_STATE_HANDLER(sm_wifi_service_active));
        }
        break;
    }
}

/**
 * @brief FSM state function for active state
 *
 * Responsible for starting and stopping the wifi services.
 *
 * @param[in] fsm Pointer to the fsm_t structure
 * @param[in] event Pointer to the fsm_event_t structure, holding event id
 */
STATIC_TEST void sm_wifi_service_active(fsm_t *const fsm, fsm_event_t const *const event)
{
    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        wifi_service_start_all((wifi_service_t *)fsm);
        break;
    case FSM_EXIT_EVENT:
        wifi_service_stop_all((wifi_service_t *)fsm);
        break;
#ifdef CONNECTOR_TLS
    case WIFI_SERVICE_TLS_INVALID_EVENT:
        // Change state to startup
        fsm_state_change(fsm, FSM_STATE_HANDLER(sm_wifi_service_startup));
        break;
#endif
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_SERVICE_STA_CONNECTED_EVENT:
        // fallthruogh
    case WIFI_SERVICE_STA_DISCONNECTED_EVENT:
        // fallthruogh
#endif  // CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_SERVICE_AP_STARTED_EVENT:
        // fallthruogh
    case WIFI_SERVICE_AP_STOPPED_EVENT:
        if (!sm_wifi_service_handle_connection_state_event(fsm, event))
        {
            fsm_state_change(fsm, FSM_STATE_HANDLER(sm_wifi_service_waiting));
        }
        break;
    }
}

/**
 * @brief Handles connection state update events in fsm states
 *
 * @param[in] fsm Pointer to the fsm_t structure
 * @param[in] event Pointer to the fsm_event_t structure, holding event id
 * @return bool true if state information indicates active services
 */
static bool sm_wifi_service_handle_connection_state_event(fsm_t *const fsm, fsm_event_t const *const event)
{
    ws_fsm_t *sm = (ws_fsm_t *)fsm;
    switch (event->id)
    {
    case WIFI_SERVICE_AP_STARTED_EVENT:
        LOG(D, "WIFI_SERVICE_AP_STARTED_EVENT");
        sm->sm_bit |= (1 << WIFI_SERVICE_AP_STARTED_EVENT);
        break;
    case WIFI_SERVICE_AP_STOPPED_EVENT:
        LOG(D, "WIFI_SERVICE_AP_STOPPED_EVENT");
        sm->sm_bit &= ~(1 << WIFI_SERVICE_AP_STARTED_EVENT);
        break;
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_SERVICE_STA_CONNECTED_EVENT:
        LOG(D, "WIFI_SERVICE_STA_CONNECTED_EVENT");
        sm->sm_bit |= (1 << WIFI_SERVICE_STA_CONNECTED_EVENT);
        break;
    case WIFI_SERVICE_STA_DISCONNECTED_EVENT:
        LOG(D, "WIFI_SERVICE_STA_DISCONNECTED_EVENT");
        sm->sm_bit &= ~(1 << WIFI_SERVICE_STA_CONNECTED_EVENT);
        break;
#endif  // CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    default:
        break;
    }
    return (sm->sm_bit > 0u);
}

/**
 * @brief Sends an event to the wifi service FSM
 *
 * @param[in] if event id to send
 */
static void sm_wifi_service_generate_event(uint32_t id)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, WIFI_SERVICE_STATE_MACHINE_EVENT_GENERIC_PARAMETER, &id, sizeof(id), service_instance->connector->connector_id, portMAX_DELAY);
}

void wifi_services_init(CONNECTOR *connector)
{
    wifi_service_t *service = NULL;

    TRUE_CHECK_RETURN(
        (service = hal_mem_malloc_prefer(sizeof(*service),
                                         HAL_MEM_SPIRAM,
                                         HAL_MEM_INTERNAL_RAM)) != NULL);

    memset(service, 0, sizeof(*service));
    service_instance = service;

    // Init state machine
    fsm_initialize(&service->ws_sm.sm, FSM_STATE_HANDLER(sm_wifi_service_startup));
    service->tcp_port = DDMP2_LISTEN_PORT;
#ifndef CONNECTOR_WIFI_NETWORK_DIS_NOT_SUPPORT
    service->udp_port = NETWORK_DISCOVERY__LISTEN_PORT;
    service->udp_socket = INVALID_SOCKET;
#endif
    service->max_socket = 0;
    service->socket_count = 1;
    service->started = false;
    service->connector = connector;

    service->event_group = xEventGroupCreate();
    service->socket_mutex = xSemaphoreCreateMutex();
    TRUE_CHECK_RETURN(NULL != service->socket_mutex);
#ifdef CONNECTOR_TLS
    service->tls_mutex = xSemaphoreCreateMutex();
    TRUE_CHECK_RETURN(NULL != service->tls_mutex);
    // Allocate memory for TLS key and certificate
    TRUE_CHECK_RETURN(
        (service->tls_key = hal_mem_malloc_prefer(TLS_CONTEXT_KEY_SIZE,
                                                  HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);

    TRUE_CHECK_RETURN(
        (service->tls_cert = hal_mem_malloc_prefer(TLS_CONTEXT_KEY_SIZE,
                                                   HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
#endif  // CONNECTOR_TLS

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    // Get events when we can startup services
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, connection_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, disconnection_handler, NULL));
#endif
    wifi_network__ap_register_handler(wifi_service_ap_event_handler);
}

#ifdef CONNECTOR_TLS
void wifi_services_tls_valid(bool tls_valid)
{
    wifi_service_t *service = service_instance;
    uint32_t event_id;
    if (!service)
    {
        LOG(E, "WiFi service instance is NULL!");
        return;
    }

    TRUE_CHECK(xSemaphoreTake(service->tls_mutex, portMAX_DELAY));
    service->tls_valid = tls_valid;
    if (tls_valid)
    {
        tls_context_get_key(service->tls_key);
        tls_context_get_certificate(service->tls_cert);

        // Generate TLS_VALID event
        event_id = WIFI_SERVICE_TLS_VALID_EVENT;
    }
    else
    {
        // Generate TLS_INVALID event
        event_id = WIFI_SERVICE_TLS_INVALID_EVENT;

        memset(service->tls_key, 0, TLS_CONTEXT_KEY_SIZE);
        memset(service->tls_cert, 0, TLS_CONTEXT_KEY_SIZE);
    }
    xSemaphoreGive(service->tls_mutex);
    sm_wifi_service_generate_event(event_id);
}
#endif  // CONNECTOR_TLS

static int wifi_service_send_base64_frame(wifi_service_t *service, const DDMP2_FRAME *const frame,
                                          const SORTED_LIST_VALUE_TYPE *const socket_slot_list,
                                          const size_t socket_count)
{
    uint8_t base64_buffer[256];
    size_t base64_length;
    int send_count;
    int encode_result;
    int socket;

    TRUE_CHECK_RETURN0(NULL != frame);
    TRUE_CHECK_RETURN0(frame->frame_size);
    TRUE_CHECK_RETURN0(NULL != socket_slot_list);

    encode_result = mbedtls_base64_encode(base64_buffer, sizeof(base64_buffer), &base64_length, (unsigned char *)&frame->frame, frame->frame_size);

    if (encode_result == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
    {
        LOG(W, "BASE64 data too large to be encoded!");
        return 0;
    }

    base64_buffer[base64_length++] = '\r';

    for (int s = 0; s < (int)socket_count; s++)
    {
        socket = service->tcp_sockets[socket_slot_list[s]];

        if (!socket)
        {
            continue;
        }

        send_count = tcp_service_send(service, socket_slot_list[s], base64_buffer, base64_length);

        if (send_count == -1)
        {
            int socket_error = socket_get_error_code(socket);
            LOG(I, "Socket %d error %s (%d)", socket, strerror(socket_error), socket_error);
            tcp_service_disconnect_socket(service, socket_slot_list[s]);
        }
    }

    return 1;
}

int wifi_service_send_frame(const DDMP2_FRAME *pframe)
{
    wifi_service_t *service = service_instance;

    if (service)
    {
        CONNECTOR *connector = service->connector;

        TRUE_CHECK_RETURN0(NULL != pframe);
        BOUNDED(pframe->destination_connector, connector_count);

        SORTED_LIST_VALUE_TYPE socket = pframe->destination_connector - connector->connector_id;

        return wifi_service_send_base64_frame(service, pframe, &socket, 1);
    }

    return 0;
}

void wifi_service_handle_generic_parameter(const DDMP2_FRAME *pframe)
{
    wifi_service_t *service = service_instance;

    if (service)
    {
        TRUE_CHECK_RETURN(NULL != pframe);

        if (pframe->frame.generic.id == WIFI_SERVICE_STATE_MACHINE_EVENT_GENERIC_PARAMETER)
        {
            uint32_t *id = (uint32_t *)pframe->frame.generic.data;
            fsm_event_t event = {0};
            event.id = *id;
            fsm_state_dispatch(&service->ws_sm.sm, &event);
        }
    }
}

#ifdef CONFIG_IDF_TARGET_LINUX
ws_fsm_t *get_ws_fsm(void)
{
    if (service_instance != NULL)
    {
        return &service_instance->ws_sm;
    }
    return NULL;
}
#endif
