/*
 * test_wifi_services.cpp
 *
 *  Created on: 22 may 2025
 *      Author: Andlun
 */

extern "C" {
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "broker.h"
#include "configuration.h"
#include "connector.h"
#include "connector_wifi.h"
#include "ddm2.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "mbedtls/base64.h"
#include "wifi_service.h"
}

#include "DICMFrameworkTestFixture.hpp"

class WifiServicesTestFixture : public DICMFrameworkTestFixture
{
  protected:
    void SetUp() override
    {
        DICMFrameworkTestFixture::SetUp();
        setConnectorId(&connector_wifi.connector_id);
        DICMFrameworkTestFixture::SetupFramework();
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};

TEST_F(WifiServicesTestFixture, udp_service)
{
    ws_fsm_t *ws_fsm = get_ws_fsm();

    fsm_event_t event;

    event.id = WIFI_SERVICE_TLS_VALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should move to sm_wifi_service_waiting" << std::endl;

    event.id = WIFI_SERVICE_AP_STARTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_active)) << "Should move to sm_wifi_service_active" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should set set AP bit in sm_bit" << std::endl;

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    EXPECT_TRUE(s >= 0) << "Socket must be larger than 0" << std::endl;
    LOG(W, "socket: %d", s);

    char buffer[5] = "DDMD";
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(13143);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ss = sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    EXPECT_TRUE(ss == 4) << "Should send 4 bytes but sent " << ss << " bytes" << std::endl;
    uint8_t readbuffer[100];
    int n = recvfrom(s, readbuffer, sizeof(readbuffer) - 1, 0, NULL, NULL);
    EXPECT_TRUE(n > 0) << "Should receive bytes in response" << std::endl;
    ESP_LOG_BUFFER_HEXDUMP("readbuffer", readbuffer, n, ESP_LOG_INFO);
}

TEST_F(WifiServicesTestFixture, tcp_service)
{
    ws_fsm_t *ws_fsm = get_ws_fsm();

    fsm_event_t event;

    event.id = WIFI_SERVICE_TLS_VALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should move to sm_wifi_service_waiting" << std::endl;

    event.id = WIFI_SERVICE_AP_STARTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_active)) << "Should move to sm_wifi_service_active" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should set set AP bit in sm_bit" << std::endl;

    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    EXPECT_TRUE(s >= 0) << "Socket must be larger than 0" << std::endl;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(13143);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int conn = connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    EXPECT_TRUE(conn == 0);

    DDMP2_RAW_FRAME rframe;
    rframe.control = DDMP2_CONTROL_SUBSCRIBE;
    rframe.subscribe.parameter = GW0INV;
    size_t raw_size = ddmp2_raw_frame_size(DDMP2_CONTROL_SUBSCRIBE, 0);

    uint8_t base64_buffer[256];
    size_t base64_length;

    int encode_result = mbedtls_base64_encode(base64_buffer, sizeof(base64_buffer), &base64_length,
                                              (unsigned char *)&rframe, raw_size);
    if (encode_result == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
    {
        EXPECT_TRUE(false) << "BASE64 data too large to be encoded!";
        return;
    }

    base64_buffer[base64_length++] = '\r';

    int ss = send(s, base64_buffer, base64_length, 0);
    EXPECT_TRUE((size_t)ss == base64_length) << "Should send " << base64_length << " bytes but sent " << ss << " bytes" << std::endl;
    uint8_t readbuffer[100];
    int n = recv(s, readbuffer, sizeof(readbuffer) - 1, 0);
    EXPECT_TRUE(n > 0) << "Should receive bytes in response" << std::endl;
    ESP_LOG_BUFFER_HEXDUMP("readbuffer", readbuffer, n, ESP_LOG_INFO);

    size_t binary_length;
    int decode_result;
    uint8_t binary_buffer[256];

    // remove separator
    EXPECT_TRUE(readbuffer[n - 1] == '\r') << "Could not find separator" << std::endl;
    decode_result = mbedtls_base64_decode(binary_buffer, sizeof(binary_buffer), &binary_length, readbuffer, n - 1);
    EXPECT_FALSE(decode_result == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) << "BASE64 data too large to be decoded!" << std::endl;
    EXPECT_FALSE(decode_result == MBEDTLS_ERR_BASE64_INVALID_CHARACTER) << "BASE64 decode error" << std::endl;
    DDMP2_FRAME frame;
    ddmp2_create_raw_frame(&frame, binary_buffer, binary_length, 0);
    EXPECT_TRUE(frame.frame.control == DDMP2_CONTROL_PUBLISH) << "Should receive a publish" << std::endl;
    EXPECT_TRUE(frame.frame.publish.parameter == GW0INV) << "Should receive a publish GW0INV" << std::endl;
    ESP_LOG_BUFFER_HEXDUMP("frame", binary_buffer, binary_length, ESP_LOG_INFO);
}
