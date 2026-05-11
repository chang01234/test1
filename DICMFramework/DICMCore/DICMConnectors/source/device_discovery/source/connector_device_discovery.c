/*! \file connector_device_discovery.c
    \brief Device discovery connector is responsible for the mDNS feature of the system.

    The mDNS is used to detect other DICMs on a local wirelesss network.  This module will when configured
    via 'ssid' parameter, either setup a softAP for the local network and start scanning for devices (this
    is the use case for a 'master'/rotary-controller node in the full climate system), or it will try to
    connect to a AP of a local network.  This module will also be reponsible for detecting if the local network
    is not the currently selected network.

    The module is also reponsible for 'initiate' the broker-to-broker connections (found in wlist parameter)
    that is required for full climate system to get access to connected devices classes.
*/

#include "configuration.h"

#include "connector_device_discovery.h"
#include "network_discovery.h"
#if defined(CONNECTOR_WIFI)
#include "wifi_network.h"
#endif
#include "ddm2_parameter_list.h"
#include "freertos/FreeRTOS.h"
#include <stdint.h>
#include <string.h>

#ifndef CONNECTOR_DEVICE_DISCOVERY_EXTENDED_LOGS
#define CONNECTOR_DEVICE_DISCOVERY_EXTENDED_LOGS 0
#endif

#if CONNECTOR_DEVICE_DISCOVERY_EXTENDED_LOGS
#define DEVICE_LOG(level, format, ...) LOG(level, format, __VA_ARGS__)
#else
#define DEVICE_LOG(level, format, ...) ((void)0)
#endif  // CONNECTOR_DEVICE_DISCOVERY_EXTENDED_LOGS

#ifndef CONNECTOR_DEVICE_DISCOVERY_QUERY_TIMEOUT_MS
#define CONNECTOR_DEVICE_DISCOVERY_QUERY_TIMEOUT_MS 30000  // Poll every 30 s
#endif
#define DEBUG_PRINTF 0

#ifndef CONNECTOR_DEVICE_DISCOVERY_DEBOUNCE_TIMES
#define CONNECTOR_DEVICE_DISCOVERY_DEBOUNCE_TIMES 2
#endif
// Module internal events
#define NOTIFY_EVENT        (DDM2_PARAMETER_CLASS_INSTANCE(SD0) | DDM2_PARAMETER_PROPERTY_FIELD(0xFD))
#define QUERY_TIMEOUT_MS    (CONNECTOR_DEVICE_DISCOVERY_QUERY_TIMEOUT_MS)
#define WIFI_OFF_TIMEOUT_MS (3000)  // OFF timeout

#define MAX_DETECTABLE_DEVICES (8)
#define MAX_DEVICE_NAME        (32)
#define DEVICE_LIST_DEBOUNCE   (CONNECTOR_DEVICE_DISCOVERY_DEBOUNCE_TIMES + 1)  // > 0

#if (CONNECTOR_DEVICE_DISCOVERY_DEBOUNCE_TIMES == 0)
#warning "NOTE: configuration-CONNECTOR_DEVICE_DISCOVERY_DEBOUNCE_TIMES. Running without debounce"
#endif

typedef struct _list_of_devices_t
{
    uint8_t num_devices;
    char device_list[MAX_DETECTABLE_DEVICES][MAX_DEVICE_NAME];
    uint8_t device_list_debounce[MAX_DETECTABLE_DEVICES];
} list_of_devices_t;

typedef struct _device_discovery_t
{
    list_of_devices_t current_list;
    list_of_devices_t temp_list;
    int instance;
    int active;
    int cnw;
    bool setupAP;
    char ssid[MAX_SSID_LEN + 1];
    TimerHandle_t query_timer;
    TimerHandle_t on_off_timer;
} device_discovery_t;

static void handle_incoming_frame(DDMP2_FRAME *pframe);
static void handle_subscribe(DDMP2_FRAME *pframe);
static void connector_process_task(void *Parameter);
static int initialize_connector(void);
static void search_notify_callback(void);
static void query_timer_timeout(TimerHandle_t xTimer);
static void off_timer_timeout(TimerHandle_t xTimer);
static bool update_and_start_softAP(char const *ssid_string);
static void publish_list(void);
static void sort_device_list(list_of_devices_t *p_list);
static void pack_device_list(list_of_devices_t *p_list);
#if DEBUG_PRINTF == 0
#define print_device_list(s)
#else
static void print_device_list(list_of_devices_t *p_list);
#endif

CONNECTOR connector_device_discovery =
    {
        .name = "Device Discovery",          // connector name
        .initialize = initialize_connector,  // connector initialize function
};

EXT_RAM_ATTR static device_discovery_t l_device_discovery;

static void connector_process_task(void *Parameter)
{
    DDMP2_FRAME l_frame;

    DDMP2_FRAME *pframe;
    size_t frame_size;

    // Create and start query timer
    l_device_discovery.query_timer = xTimerCreate(NULL, pdMS_TO_TICKS(QUERY_TIMEOUT_MS), pdTRUE, (void *)0, query_timer_timeout);
    TRUE_CHECK(NULL != l_device_discovery.query_timer);
    l_device_discovery.on_off_timer = xTimerCreate(NULL, pdMS_TO_TICKS(WIFI_OFF_TIMEOUT_MS), pdFALSE, (void *)0, off_timer_timeout);
    TRUE_CHECK(NULL != l_device_discovery.on_off_timer);

    TRUE_CHECK(xTimerReset(l_device_discovery.query_timer, portMAX_DELAY));

#if defined(CONNECTOR_WIFI)
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                              WIFI0CNW,
                                              NULL,
                                              0,
                                              connector_device_discovery.connector_id,
                                              portMAX_DELAY));

    LOG(I, "Starting mDNS service");
    TRUE_CHECK(network_discovery__mdns_initialize());
#endif
    // Load configuration
    char text[64];
    size_t size = sizeof(text);
    if (ESP_OK == (esp_err_t)config_get_str("sd0ssid", text, &size))
    {
        update_and_start_softAP(text);
    }

    while (1)
    {
        pframe = xRingbufferReceive(connector_device_discovery.to_connector, &frame_size, portMAX_DELAY);
        TRUE_CHECK(NULL != pframe);
        if (frame_size != 0)
        {
            memcpy(&l_frame, pframe, frame_size);
        }
        vRingbufferReturnItem(connector_device_discovery.to_connector, pframe);
        pframe = &l_frame;

        switch (pframe->frame.control)
        {
        case DDMP2_CONTROL_REG:
        {
            // Registration done
            l_device_discovery.instance = DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.reg.device_class);
            LOG(D, "Device discovery instance: %d", l_device_discovery.instance);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                           SD0AVL | DDM2_PARAMETER_INSTANCE(l_device_discovery.instance),
                                           &One,
                                           sizeof(One),
                                           connector_device_discovery.connector_id,
                                           portMAX_DELAY);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                           SD0ACT | DDM2_PARAMETER_INSTANCE(l_device_discovery.instance),
                                           &l_device_discovery.active,
                                           sizeof(l_device_discovery.active),
                                           connector_device_discovery.connector_id,
                                           portMAX_DELAY);

            break;
        }
        case DDMP2_CONTROL_SUBSCRIBE:
        {
            handle_subscribe(pframe);
            break;
        }
        case DDMP2_CONTROL_SET:
        case DDMP2_CONTROL_PUBLISH:
        case DDMP2_CONTROL_GENERIC:
            handle_incoming_frame(pframe);  // process received frame
            break;
        default:
            LOG(W, "Device discovery connector received UNHANDLED frame %02x from broker!", pframe->frame.control);
        }
    }
}

static bool update_and_start_softAP(char const *ssid_string)
{
    bool ret_val = false;
    char text[MAX_SSID_LEN + MAX_PASSPHRASE_LEN + 2];
    char ssid[MAX_SSID_LEN + 1];
    char pwd[MAX_PASSPHRASE_LEN + 1];
    pwd[0] = '\0';
    strcpy(text, ssid_string);
    char *pwdPtr = strchr(&text[1], '\\');
    if (NULL != pwdPtr)
    {
        // Found a '\'.
        char *token;
        char *rest = (char *)&text[1];
        token = strtok_r(rest, "\\", &rest);
        LOG(I, "Found ssid: %s", token);
        strncpy(ssid, token, sizeof(ssid));
        token = strtok_r(rest, "\\", &rest);
        // LOG(I, "Found pwd: %s", token);
        strncpy(pwd, token, sizeof(pwd));
        // Make sure to null terminate
        pwd[sizeof(pwd) - 1u] = '\0';
        /* Null terminate the string in case `ssid` is bigger than what event can hold */
        ssid[sizeof(ssid) - 1u] = '\0';
        strcpy(l_device_discovery.ssid, ssid);
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  SD0SSID | DDM2_PARAMETER_INSTANCE(l_device_discovery.instance),
                                                  l_device_discovery.ssid,
                                                  strlen(l_device_discovery.ssid) + 1,
                                                  connector_device_discovery.connector_id,
                                                  portMAX_DELAY));

        if (text[0] == 'S')
        {
            // Startup AP
            l_device_discovery.setupAP = true;
            wifi_network__ap_update_settings(ssid, pwd);
            wifi_network__ap_enable();
            LOG(I, "SoftAP started");
            l_device_discovery.active = 1;
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                      SD0ACT | DDM2_PARAMETER_INSTANCE(l_device_discovery.instance),
                                                      &One,
                                                      4,
                                                      connector_device_discovery.connector_id,
                                                      portMAX_DELAY));
            ret_val = true;
        }
        else if (text[0] == 'R')
        {
            // Inform Wifi that we have a preferred network to connect to
            // 1. Check current list of avaiable networks (candidate_list)
            // 2. Check if ssid is found in whitelist already.
            // 2.1 If found -> start procedure to select that network as current, i.e. connect to network
            // 2.2 If not found -> Add to whitelist and start procedure to select that network as current, i.e. connect to network

            l_device_discovery.setupAP = false;
            int index = wifi_network__network_find_ssid(ssid);
            LOG(I, "wifi_network__network_find_ssid returned %d", index);
            if (index != -1)
            {
                // select the network
                TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                                          WIFI0CNW,
                                                          &index,
                                                          4,
                                                          connector_device_discovery.connector_id,
                                                          portMAX_DELAY));
                // Turn off Wifi
                TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                                          WIFI0ON,
                                                          &Zero,
                                                          4,
                                                          connector_device_discovery.connector_id,
                                                          portMAX_DELAY));
                // Let system know we are trying to connect to a local network
                (void)xEventGroupSetBits(network_events, LOCAL_NETWORK_BIT);

                TRUE_CHECK(xTimerReset(l_device_discovery.on_off_timer, portMAX_DELAY));
            }
            ret_val = true;
        }
    }
    else
    {
        // Invalid format
        LOG(W, "Invalid format for ssid");
    }
    return ret_val;
}

static void handle_incoming_frame(DDMP2_FRAME *pframe)
{
    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_SET:
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.set.parameter))
        {
        case SD0SSID:
        {
            char text[64];
            ddmp2_extract_string_from_frame(pframe, text, sizeof(text));
            LOG(I, "SD0SSID recived: %s", text);
            if (true == update_and_start_softAP(text))
            {
                // Update AP settings
                config_set_str("sd0ssid", text);
            }
            break;
        }
        default:
            break;
        }
        break;
    }
    case DDMP2_CONTROL_GENERIC:
    {
        if (pframe->frame.generic.id == (uint32_t)NOTIFY_EVENT)
        {
            //
            // Time to fetch query results
            DEVICE_LOG(I, "NOTIFY_EVENT received");
            network_discovery_result_t my_results;
            memset(&my_results, 0, sizeof(my_results));
            network_discovery__query_fetch_results(&my_results);
            int i = 0;
            int ii = 0;
            // Clear all for now
            memset(&l_device_discovery.temp_list, 0, sizeof(l_device_discovery.temp_list));
            while ((i < my_results.num_items) && my_results.items)
            {
                DEVICE_LOG(I, "Detected device %d: %s", i, my_results.items[i].name ? my_results.items[i].name : "No name");
                if (my_results.items[i].hostname)
                {
                    DEVICE_LOG(I, "Server: %s.local:%u", my_results.items[i].hostname, my_results.items[i].port);
                    strcat(l_device_discovery.temp_list.device_list[ii++], my_results.items[i].hostname);
                }
                if (my_results.items[i].fwversion)
                {
                    DEVICE_LOG(I, "FW version: %s", my_results.items[i].fwversion);
                }
                if (my_results.items[i].sku)
                {
                    DEVICE_LOG(I, "SKU: %s", my_results.items[i].sku);
                }
                else
                {
                    DEVICE_LOG(W, "No SKU");
                }
                if (my_results.items[i].board)
                {
                    DEVICE_LOG(I, "Board: %s", my_results.items[i].board);
                }
                else
                {
                    DEVICE_LOG(W, "No board");
                }

                DEVICE_LOG(I, "IP: " IPSTR, IP2STR(&(my_results.items[i].ip_addr.u_addr.ip4)));
                ++i;
            }
            network_discovery__query_free_results(&my_results);
            l_device_discovery.temp_list.num_devices = ii;
            pack_device_list(&l_device_discovery.temp_list);
            sort_device_list(&l_device_discovery.temp_list);
            // Compare and update list
            // 1. Debounce and remove existing devices first
            i = 0;
            int rem_dev = 0;
            for (i = 0; i < l_device_discovery.current_list.num_devices; ++i)
            {
                bool b_match = false;
                for (int j = 0; j < l_device_discovery.temp_list.num_devices; ++j)
                {
                    if (strcmp(l_device_discovery.current_list.device_list[i], l_device_discovery.temp_list.device_list[j]) == 0)
                    {
                        // Match found, no need to continue
                        b_match = true;
                        l_device_discovery.current_list.device_list_debounce[i] = DEVICE_LIST_DEBOUNCE;
                        break;
                    }
                }
                if (false == b_match)
                {
                    // Debounce entry
                    l_device_discovery.current_list.device_list_debounce[i]--;
                    if (l_device_discovery.current_list.device_list_debounce[i] == 0u)
                    {
                        // Remove from list
                        l_device_discovery.current_list.device_list[i][0] = '\0';
                        rem_dev++;
                    }
                }
            }
            l_device_discovery.current_list.num_devices -= rem_dev;
            pack_device_list(&l_device_discovery.current_list);

            // 2. Add new devices to list
            int add_dev = 0;
            for (i = 0; i < l_device_discovery.temp_list.num_devices; ++i)
            {
                bool b_match = false;
                for (int j = 0; j < l_device_discovery.current_list.num_devices; ++j)
                {
                    if (strcmp(l_device_discovery.temp_list.device_list[i], l_device_discovery.current_list.device_list[j]) == 0)
                    {
                        // Match found, no need to continue
                        b_match = true;
                    }
                }
                if (false == b_match)
                {
                    if (MAX_DETECTABLE_DEVICES == l_device_discovery.current_list.num_devices)
                    {
                        LOG(E, "Too many devices found. Ignoring %s", l_device_discovery.temp_list.device_list[i]);
                    }
                    else
                    {
                        // Add new entry
                        strcpy(l_device_discovery.current_list.device_list[l_device_discovery.current_list.num_devices + add_dev],
                               l_device_discovery.temp_list.device_list[i]);
                        l_device_discovery.current_list.device_list_debounce[l_device_discovery.current_list.num_devices + add_dev] = DEVICE_LIST_DEBOUNCE;
                        ++add_dev;
                    }
                }
            }
            l_device_discovery.current_list.num_devices += add_dev;
            pack_device_list(&l_device_discovery.current_list);
            sort_device_list(&l_device_discovery.current_list);
            print_device_list(&l_device_discovery.current_list);
            // Publish list if changed
            if ((add_dev > 0) || (rem_dev > 0))
            {
                publish_list();
            }
        }
        else
        {
            LOG(E, "Bad generic event id received: %x", pframe->frame.generic.id);
        }
        break;
    }
    case DDMP2_CONTROL_PUBLISH:
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.publish.parameter))
        {
        case WIFI0CNW:
        {
            l_device_discovery.cnw = pframe->frame.publish.value.int32;
            LOG(D, "WIFI0CNW: %d", pframe->frame.publish.value.int32);
            int index = wifi_network__network_find_ssid(l_device_discovery.ssid);
            if (index != -1)
            {
                if (l_device_discovery.cnw != index)
                {
                    // Let system know we are not connected to a local network
                    (void)xEventGroupClearBits(network_events, LOCAL_NETWORK_BIT);
                }
                else
                {
                    // Let system know we are trying to connect/connected to a local network
                    (void)xEventGroupSetBits(network_events, LOCAL_NETWORK_BIT);
                }
            }
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void handle_subscribe(DDMP2_FRAME *pframe)
{
    DEVICE_LOG(D, "Sub: %x", pframe->frame.set.parameter);
    switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.set.parameter))
    {
    case SD0ACT:
    {
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  SD0ACT | DDM2_PARAMETER_INSTANCE(l_device_discovery.instance),
                                                  &l_device_discovery.active,
                                                  4,
                                                  connector_device_discovery.connector_id,
                                                  portMAX_DELAY));

        break;
    }
    case SD0LIST:
    {
        publish_list();
        if (true == l_device_discovery.setupAP)
        {
            // Request new query
            network_discovery__query_async_service(search_notify_callback);
        }
        break;
    }
    case SD0WLIST:
    {
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  SD0WLIST | DDM2_PARAMETER_INSTANCE(l_device_discovery.instance),
                                                  0,
                                                  0,
                                                  connector_device_discovery.connector_id,
                                                  portMAX_DELAY));

        break;
    }
    case SD0SSID:
    {
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  SD0SSID | DDM2_PARAMETER_INSTANCE(l_device_discovery.instance),
                                                  l_device_discovery.ssid,
                                                  strlen(l_device_discovery.ssid) + 1,
                                                  connector_device_discovery.connector_id,
                                                  portMAX_DELAY));

        break;
    }
    }
}

static int initialize_connector(void)  // initialize connector
{
    memset(&l_device_discovery, 0, sizeof(l_device_discovery));
    l_device_discovery.cnw = -1;
    // Register Device discovery class
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_REG,
                                              DDM2_PARAMETER_CLASS(SD0AVL),
                                              NULL,
                                              0,
                                              connector_device_discovery.connector_id,
                                              portMAX_DELAY));

    TRUE_CHECK(xTaskCreate(connector_process_task, connector_device_discovery.name, 3584, 0, xTASK_PRIORITY_NORMAL, NULL));

    return 1;
}

static void search_notify_callback(void)
{
    // Create event
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, NOTIFY_EVENT, NULL, 0, connector_device_discovery.connector_id, portMAX_DELAY);
}

static void query_timer_timeout(TimerHandle_t xTimer)
{
    if (true == l_device_discovery.setupAP)
    {
        // Request a new search
        network_discovery__query_async_service(search_notify_callback);
    }
    else if (strlen(l_device_discovery.ssid) == 0)
    {
        // Not configured yet.
        // Do nothing
    }
    else
    {

        // Check that we are connected to correct WiFi AP.
        int index = wifi_network__network_find_ssid(l_device_discovery.ssid);
        // Try to change network
        if ((index != -1) && (l_device_discovery.cnw != index))
        {
            // select the network
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                                      WIFI0CNW,
                                                      &index,
                                                      4,
                                                      connector_device_discovery.connector_id,
                                                      portMAX_DELAY));
            // Turn off Wifi
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                                      WIFI0ON,
                                                      &Zero,
                                                      sizeof(Zero),
                                                      connector_device_discovery.connector_id,
                                                      portMAX_DELAY));
            // Wait before we turn on Wifi again
            TRUE_CHECK(xTimerReset(l_device_discovery.on_off_timer, portMAX_DELAY));
        }
    }
}

static void off_timer_timeout(TimerHandle_t xTimer)
{
    // Turn on Wifi
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                              WIFI0ON,
                                              &One,
                                              sizeof(One),
                                              connector_device_discovery.connector_id,
                                              portMAX_DELAY));
}

static void publish_list(void)
{
    int i = 0;
    DDMP2_PAYLOAD p_data;
    memset(&p_data, 0, sizeof(p_data));
    while (l_device_discovery.current_list.num_devices > i)
    {
        strcat((char *)p_data.raw, l_device_discovery.current_list.device_list[i]);
        strcat((char *)p_data.raw, ";");
        i++;
    }
    if (strlen((char *)p_data.raw) > 0)
    {
        connector_send_frame_to_broker(
            DDMP2_CONTROL_PUBLISH,
            SD0LIST,
            (char *)p_data.raw,
            strlen((char *)p_data.raw) + 1,  // including null
            connector_device_discovery.connector_id,
            portMAX_DELAY);
    }
    else
    {
        // Just an empty string
        connector_send_frame_to_broker(
            DDMP2_CONTROL_PUBLISH,
            SD0LIST,
            "",
            sizeof(""),
            connector_device_discovery.connector_id,
            portMAX_DELAY);
    }
}

// list should already be packed
static void sort_device_list(list_of_devices_t *p_list)
{
    int i, j;
    int num = p_list->num_devices;
    for (i = 0; i < num; i++)
    {
        for (j = 0; j < (num - i - 1); j++)
        {
            if (strcmp(p_list->device_list[j], p_list->device_list[j + 1]) > 0)
            {
                uint8_t tmp;
                char temp[MAX_DEVICE_NAME];
                strcpy(temp, p_list->device_list[j]);
                tmp = p_list->device_list_debounce[j];
                strcpy(p_list->device_list[j], p_list->device_list[j + 1]);
                p_list->device_list_debounce[j] = p_list->device_list_debounce[j + 1];
                strcpy(p_list->device_list[j + 1], temp);
                p_list->device_list_debounce[j + 1] = tmp;
            }
        }
    }
}

static void pack_device_list(list_of_devices_t *p_list)
{
    int i, j;
    for (i = MAX_DETECTABLE_DEVICES - 2; i >= 0; --i)
    {
        if (strlen(p_list->device_list[i]) == 0)
        {
            for (j = i; j < MAX_DETECTABLE_DEVICES - 1; j++)
            {
                // Copy entries to the left
                strcpy(p_list->device_list[j], p_list->device_list[j + 1]);
                p_list->device_list_debounce[j] = p_list->device_list_debounce[j + 1];
            }
            // Reset last
            p_list->device_list[MAX_DETECTABLE_DEVICES - 1][0] = '\0';
            p_list->device_list_debounce[MAX_DETECTABLE_DEVICES - 1] = 0;
        }
    }
}

#if DEBUG_PRINTF == 1
static void print_device_list(list_of_devices_t *p_list)
{
    int i;
    LOG(D, "DEVICE LIST, number of devices: %u", p_list->num_devices);
    for (i = 0; i < MAX_DETECTABLE_DEVICES; ++i)
    {
        LOG(D, "Device %d, %s:%u", i, p_list->device_list[i], p_list->device_list_debounce[i]);
    }
}
#endif
