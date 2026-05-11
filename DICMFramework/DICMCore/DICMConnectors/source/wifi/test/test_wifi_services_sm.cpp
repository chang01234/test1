/*
 * test_wifi_services_sm.cpp
 *
 *  Created on: 21 may 2025
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
#include "linux_dicmframework_setup.h"
#include "lwip/sockets.h"
#include "wifi_service.h"
}

#include "DICMFrameworkTestFixture.hpp"
CONNECTOR test;

class WifiServiceSMTestFixture : public DICMFrameworkTestFixture
{
  protected:
    void SetUp() override
    {
        DICMFrameworkTestFixture::SetUp();
        ZERO_CHECK(esp_event_loop_create_default());

        wifi_services_init(&test);
        linux_dicmframework_setup();
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};

TEST_F(WifiServiceSMTestFixture, initialize)
{
    ws_fsm_t *ws_fsm = get_ws_fsm();

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should start in sm_wifi_service_startup" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit == 0) << "No bits should be set" << std::endl;
}

TEST_F(WifiServiceSMTestFixture, startup)
{
    ws_fsm_t *ws_fsm = get_ws_fsm();

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should start in sm_wifi_service_startup" << std::endl;
    fsm_event_t event;
    event.id = WIFI_SERVICE_TLS_INVALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should stay in sm_wifi_service_startup" << std::endl;

    event.id = WIFI_SERVICE_TLS_VALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should move to sm_wifi_service_waiting" << std::endl;

    event.id = WIFI_SERVICE_TLS_INVALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should move to sm_wifi_service_startup" << std::endl;

    event.id = WIFI_SERVICE_STA_CONNECTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should stay in sm_wifi_service_startup" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should set STA bit in sm_bit" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should set not AP bit in sm_bit" << std::endl;

    event.id = WIFI_SERVICE_AP_STARTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should stay in sm_wifi_service_startup" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should set STA bit in sm_bit" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should set set AP bit in sm_bit" << std::endl;

    event.id = WIFI_SERVICE_STA_DISCONNECTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should stay in sm_wifi_service_startup" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should set set AP bit in sm_bit" << std::endl;

    event.id = WIFI_SERVICE_AP_STOPPED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should stay in sm_wifi_service_startup" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should not set set AP bit in sm_bit" << std::endl;
}

TEST_F(WifiServiceSMTestFixture, waiting)
{
    ws_fsm_t *ws_fsm = get_ws_fsm();

    fsm_event_t event;

    esp_netif_init();

    event.id = WIFI_SERVICE_TLS_VALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should move to sm_wifi_service_waiting" << std::endl;

    event.id = WIFI_SERVICE_AP_STOPPED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should stay in sm_wifi_service_waiting" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should not set set AP bit in sm_bit" << std::endl;

    event.id = WIFI_SERVICE_STA_DISCONNECTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should stay in sm_wifi_service_waiting" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should not set set AP bit in sm_bit" << std::endl;

    event.id = WIFI_SERVICE_STA_DISCONNECTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    event.id = WIFI_SERVICE_TLS_INVALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);
    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should move to sm_wifi_service_startup" << std::endl;

    event.id = WIFI_SERVICE_AP_STARTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should stay in sm_wifi_service_startup" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should set set AP bit in sm_bit" << std::endl;

    event.id = WIFI_SERVICE_TLS_VALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_active)) << "Should move to sm_wifi_service_active" << std::endl;
}

TEST_F(WifiServiceSMTestFixture, active)
{
    ws_fsm_t *ws_fsm = get_ws_fsm();

    fsm_event_t event;

    esp_netif_init();

    event.id = WIFI_SERVICE_TLS_VALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should move to sm_wifi_service_waiting" << std::endl;

    event.id = WIFI_SERVICE_AP_STARTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_active)) << "Should move to sm_wifi_service_active" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should set set AP bit in sm_bit" << std::endl;

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_active)) << "Should still be in sm_wifi_service_active" << std::endl;
    event.id = WIFI_SERVICE_AP_STOPPED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);
    printf("Stopping services\n");
    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_waiting)) << "Should move to sm_wifi_service_waiting" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should not set STA bit in sm_bit" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should not set set AP bit in sm_bit" << std::endl;

    event.id = WIFI_SERVICE_STA_CONNECTED_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_active)) << "Should move to sm_wifi_service_active" << std::endl;
    EXPECT_TRUE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_STA_CONNECTED_EVENT)) << "Should set STA bit in sm_bit" << std::endl;
    EXPECT_FALSE(ws_fsm->sm_bit & (1 << WIFI_SERVICE_AP_STARTED_EVENT)) << "Should not set set AP bit in sm_bit" << std::endl;

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_active)) << "Should still be in sm_wifi_service_active" << std::endl;

    event.id = WIFI_SERVICE_TLS_INVALID_EVENT;
    fsm_state_dispatch(&ws_fsm->sm, &event);

    EXPECT_TRUE(ws_fsm->sm.m_state == FSM_STATE_HANDLER(sm_wifi_service_startup)) << "Should move to sm_wifi_service_startup" << std::endl;
}
