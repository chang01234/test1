/*
 * linux_dicmframework_setup.c
 *
 *  Created on: 30 sep. 2024
 *      Author: Andlun
 */


#include "esp_netif.h"
//#include "esp_netif_mock.h"
#include "esp_netif_types.h"
#include "lwip/tcpip.h"
#include "esp_netif_lwip_internal.h"
#include "Mockesp_netif.h"
#include "Mockesp_wifi.h"
#include "Mockesp_wifi_netif.h"
#include "Mockesp_wifi_default.h"

static esp_netif_t l_ap_netif;
static esp_netif_t l_sta_netif;

static esp_err_t CMOCK_esp_netif_init_CALLBACK_fn(int cmock_num_calls);
static esp_err_t CMOCK_esp_netif_init_CALLBACK_fn(int cmock_num_calls)
{
    tcpip_init(NULL, NULL);
    return 0;
}

void linux_dicmframework_setup(void)
{
    Mockesp_netif_Init();
    Mockesp_wifi_Init();
    Mockesp_wifi_netif_Init();
    Mockesp_wifi_default_Init();
    
    //esp_netif_init_IgnoreAndReturn(0);
    esp_netif_init_StubWithCallback(CMOCK_esp_netif_init_CALLBACK_fn);
    esp_netif_create_default_wifi_ap_IgnoreAndReturn(&l_ap_netif);
    esp_netif_create_default_wifi_sta_IgnoreAndReturn(&l_sta_netif);
    esp_netif_get_ip_info_IgnoreAndReturn(0);
    esp_netif_is_netif_up_IgnoreAndReturn(1);
    esp_netif_get_handle_from_ifkey_IgnoreAndReturn(&l_sta_netif);
    esp_wifi_init_IgnoreAndReturn(0);
}


void* esp_netif_get_netif_impl(esp_netif_t *esp_netif)
{
    if (esp_netif) {
        return esp_netif->lwip_netif;
    }
    return NULL;
}
