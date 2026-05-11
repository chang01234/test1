/*****************************************************************************
 * \file       connector_wifi.c
 * \brief      BASE64 WiFi connector
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "connector_wifi.h"
#ifdef CONNECTOR_TLS
#include "connector_tls.h"
#endif
#include "broker.h"
#include "ddm2.h"
#include "wifi_network.h"
#include "wifi_service.h"

typedef struct _WIFI_APLIST
{
    char ssid[WIFI_NETWORK__SSID_LENGTH];
    char pwd[WIFI_NETWORK__PASSWORD_LENGTH];
} WIFI_APLIST;

/*****************************************************************************
 * Private functions
 ****************************************************************************/
static void handle_wifi_parameter(const DDMP2_FRAME *const pframe);
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void handle_wifi_event(enum wifi_network__sta_event sta_event,
                              union wifi_network__event_payload payload);
#endif
static void connector_wifi_process_task(void *parameter);
static void connector_wifi_load_aplist(void);
static int initialize_connector_wifi(void);
static void generate_random_chars(char *const buf, const int len);

/*****************************************************************************
 * Private variables
 ****************************************************************************/
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/**
 * @brief	Wi-Fi state variables used for Wi-Fi DDM handling
 *
 * This structure contains data which is accessed through DDM parameters.
 */
typedef struct wifi_state
{
    int32_t is_on;              //!< Is Wi-Fi stack enabled or not
    int32_t cnw;                //!< Wi-Fi current network
    WIFI0STS_ENUM last_status;  //!< Last saved status
    struct wifi_network
    {
        bool is_available;
        int32_t chn;
        char ssid[WIFI_NETWORK__SSID_LENGTH];
    } networks[WIFI_NETWORK__NO_OF_NETWORKS];  //!< Information about networks (SSID, channel,...)
} wifi_state_t;
#endif

#ifdef CONNECTOR_TLS
static bool tls_cert_valid = false;
static bool tls_cacert_valid = false;
#endif

static EXT_RAM_ATTR WIFI_APLIST wifi_aplist[WIFI_APLIST_SIZE];
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static EXT_RAM_ATTR wifi_state_t wifi_state;
#endif

static const char random_char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static void handle_wifi_parameter(const DDMP2_FRAME *const pframe)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    char text[64];
#endif

    ASSERT(NULL != pframe);

    // Set command?
    if (pframe->frame.control == DDMP2_CONTROL_SET)
    {
    #ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
        // Wifi
        switch (pframe->frame.set.parameter)
        {
        case WIFI0SCAN:
        {
            wifi_network__scan_start();
            return;
        }
        case WIFI0ADD:
            ddmp2_extract_string_from_frame(pframe, text, sizeof(text));
            wifi_network__network_add(text);
            return;
        case WIFI0CNW:
            wifi_network__network_select(pframe->frame.set.value.int32);
            return;
        case WIFI0ON:
            if (pframe->frame.set.value.int32 != 0)
            {
                if (wifi_state.is_on == 0)
                {
                    wifi_network__enable();
                }
                else
                {
                    // Already on
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                   pframe->frame.subscribe.parameter,
                                                   &wifi_state.is_on,
                                                   sizeof(wifi_state.is_on),
                                                   connector_wifi.connector_id,
                                                   portMAX_DELAY);
                }
            }
            else
            {
                if (wifi_state.is_on == 1)
                {
                    wifi_network__disable();
                }
                else
                {
                    // Already off
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                   pframe->frame.subscribe.parameter,
                                                   &wifi_state.is_on,
                                                   sizeof(wifi_state.is_on),
                                                   connector_wifi.connector_id,
                                                   portMAX_DELAY);
                }
            }
            return;
        default:
            break;
        }

        // Whitelist
        uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.set.parameter);
        switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.set.parameter))
        {
        case WFWL0DEL:
            wifi_network__network_remove(instance);
            return;
        case WFWL0SSID:
        {
            char *delimited_string;
            int32_t chn = 0;

            ddmp2_extract_string_from_frame(pframe, text, sizeof(text));
            /* Separate SSID and channel information (if provided) */
            delimited_string = strchr(text, '\\');

            if (delimited_string != NULL)
            {
                chn = MAX(atoi(&delimited_string[1]), 0);
                /* Erase rest of string */
                memset(delimited_string, 0, strlen(delimited_string));
            }
            wifi_network__network_ssid(instance, text);
            wifi_network__network_chn(instance, chn);
            return;
        }
        case WFWL0PW:
            ddmp2_extract_string_from_frame(pframe, text, sizeof(text));
            wifi_network__network_pwd(instance, text);
            return;
        };
    #endif
    }
    // Suscribe command?
    else if (pframe->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
    #ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
        // Wifi
        switch (pframe->frame.set.parameter)
        {
        case WIFI0STS:
        {
            int32_t last_status = wifi_state.last_status;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                           pframe->frame.subscribe.parameter,
                                           &last_status,
                                           sizeof(last_status),
                                           connector_wifi.connector_id,
                                           portMAX_DELAY);
            return;
        }
        case WIFI0CNW:
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                           pframe->frame.subscribe.parameter,
                                           &wifi_state.cnw,
                                           sizeof(wifi_state.cnw),
                                           connector_wifi.connector_id,
                                           portMAX_DELAY);
            return;
        case WIFI0EV:
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                           pframe->frame.subscribe.parameter,
                                           &Zero,
                                           sizeof(Zero),
                                           connector_wifi.connector_id,
                                           portMAX_DELAY);
            return;
        case WIFI0ON:
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                           pframe->frame.subscribe.parameter,
                                           &wifi_state.is_on,
                                           sizeof(wifi_state.is_on),
                                           connector_wifi.connector_id,
                                           portMAX_DELAY);
            return;
        case WIFI0MD:
        {
            // TODO: Implement correct mode status
            int32_t mode = (int32_t)WIFI0MD_NONE;
            connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                pframe->frame.subscribe.parameter,
                &mode,
                sizeof(mode),
                connector_wifi.connector_id,
                portMAX_DELAY);
            return;
        }
        case WIFI0SCAN:
        case WIFI0ADD:
            // Do nothing, report ok
            return;
        default:
            break;
        }

        // Whitelist
        uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.subscribe.parameter);
        switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter))
        {
        case WFWL0SSID:
            // Valid instance?
            if (instance < ELEMENTS(wifi_state.networks))
            {
                // Publish
                connector_send_frame_to_broker(
                    DDMP2_CONTROL_PUBLISH,
                    pframe->frame.subscribe.parameter,
                    wifi_state.networks[instance].ssid,
                    strlen(wifi_state.networks[instance].ssid),
                    connector_wifi.connector_id,
                    portMAX_DELAY);
            }
            return;
        default:
            break;
        }
    #endif
    }
#ifdef CONNECTOR_TLS
    else if (pframe->frame.control == DDMP2_CONTROL_PUBLISH)
    {
        switch (pframe->frame.publish.parameter)
        {
        case TLS0CACERT:
            tls_cacert_valid = pframe->frame.publish.value.int32 != 0;
            break;
        case TLS0CERT:
            tls_cert_valid = pframe->frame.publish.value.int32 != 0;
            break;
        default:
            break;
        }

        wifi_services_tls_valid(tls_cert_valid && tls_cacert_valid);
    }
#endif  // CONNECTOR_TLS
    else if (pframe->frame.control == DDMP2_CONTROL_GENERIC)
    {
        wifi_service_handle_generic_parameter(pframe);
    }
}

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void handle_wifi_event(enum wifi_network__sta_event sta_event, union wifi_network__event_payload payload)
{
    int ops;

    switch (sta_event)
    {
    case WIFI_NETWORK__STA_EVENT_SELECTED:
    {
        wifi_state.cnw = payload.i32;

        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0CNW,
                                             &wifi_state.cnw,
                                             sizeof(wifi_state.cnw),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_NOT_SELECTED:
    {
        wifi_state.cnw = -1;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0CNW,
                                             &Minone,
                                             sizeof(Minone),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    /* In both cases added/not-added we want the index of network in whitelist
     * When whitelist is full SM will send -1 which is also what we want here.
     */
    case WIFI_NETWORK__STA_EVENT_NOT_ADDED:
    case WIFI_NETWORK__STA_EVENT_ADDED:
    {
        int32_t instance;

        instance = payload.i32;

        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0ADD,
                                             &instance,
                                             sizeof(instance),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_PWD_UPDATED:
    case WIFI_NETWORK__STA_EVENT_PWD_NOT_UPDATED:
    case WIFI_NETWORK__STA_EVENT_NOT_REMOVED:
        break;
    case WIFI_NETWORK__STA_EVENT_NOT_AVAILABLE:
    case WIFI_NETWORK__STA_EVENT_REMOVED:
    {
        ASSERT((payload.i32 >= (int32_t)0) && (payload.i32 < (int32_t)ELEMENTS(wifi_state.networks)));
        if (wifi_state.networks[payload.i32].is_available)
        {
            wifi_state.networks[payload.i32].is_available = false;
            ops = connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                WFWL0AVL | DDM2_PARAMETER_INSTANCE((uint32_t)payload.i32),
                &Zero,
                sizeof(Zero),
                connector_wifi.connector_id,
                portMAX_DELAY);
            TRUE_CHECK(ops);
        }
        break;
    }
    case WIFI_NETWORK__STA_EVENT_SSID_UPDATED:
    {
        const struct wifi_network__network_ssid *network_ssid;

        network_ssid = payload.network_ssid;

        ASSERT(network_ssid->index < ELEMENTS(wifi_state.networks));

        if (wifi_state.networks[network_ssid->index].is_available)
        {
            /* Take into account that we want to merge SSID and channel information in single
             * string
             */
            char ssid_chn_tuple[WIFI_NETWORK__SSID_LENGTH + 3u];

            snprintf(ssid_chn_tuple,
                     sizeof(ssid_chn_tuple),
                     "%s\\%d",
                     network_ssid->ssid,
                     wifi_state.networks[network_ssid->index].chn);
            ops = connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                WFWL0SSID | DDM2_PARAMETER_INSTANCE(network_ssid->index),
                ssid_chn_tuple,
                strnlen(ssid_chn_tuple, sizeof(ssid_chn_tuple)),
                connector_wifi.connector_id,
                portMAX_DELAY);
            TRUE_CHECK(ops);
        }
        strncpy(wifi_state.networks[network_ssid->index].ssid,
                network_ssid->ssid,
                sizeof(wifi_state.networks[network_ssid->index].ssid));
        break;
    }
    case WIFI_NETWORK__STA_EVENT_CHN_UPDATED:
    {
        const struct wifi_network__network_chn *network_chn;

        network_chn = payload.network_chn;

        ASSERT(network_chn->index < ELEMENTS(wifi_state.networks));

        if (wifi_state.networks[network_chn->index].is_available)
        {
            /* Take into account that we want to merge SSID and channel information in single
             * string
             */
            char ssid_chn_tuple[WIFI_NETWORK__SSID_LENGTH + 3u];

            snprintf(ssid_chn_tuple,
                     sizeof(ssid_chn_tuple),
                     "%s\\%d",
                     wifi_state.networks[network_chn->index].ssid,
                     network_chn->chn);
            ops = connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                WFWL0SSID | DDM2_PARAMETER_INSTANCE(network_chn->index),
                ssid_chn_tuple,
                strnlen(ssid_chn_tuple, sizeof(ssid_chn_tuple)),
                connector_wifi.connector_id,
                portMAX_DELAY);
            TRUE_CHECK(ops);
        }
        wifi_state.networks[network_chn->index].chn = network_chn->chn;
        break;
    }
    case WIFI_NETWORK__STA_EVENT_ENABLED:
    {
        int32_t last_status;
        int32_t last_event;

        wifi_state.is_on = true;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0ON,
                                             &One,
                                             sizeof(One),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        wifi_state.last_status = WIFI0STS_UNKNOWN;
        last_status = (int32_t)wifi_state.last_status;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0STS,
                                             &last_status,
                                             sizeof(last_status),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        last_event = WIFI0EV_STA_STARTED;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0EV,
                                             &last_event,
                                             sizeof(last_event),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_DISABLED:
    {
        int32_t last_status;

        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0ON,
                                             &Zero,
                                             sizeof(Zero),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        wifi_state.is_on = false;
        wifi_state.last_status = WIFI0STS_UNKNOWN;
        last_status = (int32_t)wifi_state.last_status;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0STS,
                                             &last_status,
                                             sizeof(last_status),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_CONNECTING:
    {
        int32_t last_status;

        wifi_state.last_status = WIFI0STS_CONNECTING;
        last_status = (int32_t)wifi_state.last_status;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0STS,
                                             &last_status,
                                             sizeof(last_status),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_CONNECTED:
    {
        int32_t last_status;
        int32_t last_event;

        wifi_state.cnw = payload.i32;

        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0CNW,
                                             &wifi_state.cnw,
                                             sizeof(wifi_state.cnw),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        last_event = WIFI0EV_STA_CONNECTED;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0EV,
                                             &last_event,
                                             sizeof(last_event),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        wifi_state.last_status = WIFI0STS_CONNECTED;
        last_status = (int32_t)wifi_state.last_status;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0STS,
                                             &last_status,
                                             sizeof(last_status),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_DISCONNECTED:
    {
        int32_t last_event;
        int32_t last_status;

        switch (payload.i32)
        {
        case WIFI_NETWORK__DISCONNECTED_REASON_BAD_PWD:
            last_event = WIFI0EV_STA_DISCONN_WRONG_PASSWORD;
            break;
        case WIFI_NETWORK__DISCONNECTED_REASON_NO_AP:
            last_event = WIFI0EV_STA_DISCONN_NO_AP_FOUND;
            break;
        default:
            last_event = WIFI0EV_STA_DISCONNECTED;
            break;
        }
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0EV,
                                             &last_event,
                                             sizeof(last_event),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        wifi_state.last_status = WIFI0STS_DISCONNECTED;
        last_status = (int32_t)wifi_state.last_status;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0STS,
                                             &last_status,
                                             sizeof(last_status),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_DISCONNECTED_RETRYING:
    {
        int32_t last_status;

        wifi_state.last_status = WIFI0STS_DISCONNECTED_RETRYING;
        last_status = (int32_t)wifi_state.last_status;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0STS,
                                             &last_status,
                                             sizeof(last_status),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_SCAN_STARTED:
    {
        int32_t last_event;

        last_event = WIFI0EV_UNKNOWN;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0EV,
                                             &last_event,
                                             sizeof(last_event),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_SCAN_STOPPED:
    {
        int32_t last_event;

        last_event = WIFI0EV_SCAN_COMPLETED;
        ops = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                             WIFI0EV,
                                             &last_event,
                                             sizeof(last_event),
                                             connector_wifi.connector_id,
                                             portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_SCAN_RESULTS:
    {
        const struct wifi_network__scan_results *sorted_scan_results;

        sorted_scan_results = payload.scan_results;
        for (uint32_t i = 0u; i < sorted_scan_results->no_entries; i++)
        {
            /* Each scan entry string contains:
             *  - 4 bytes RSSI
             *  - 4 bytes auth mode
             *    * `0` - open
             *    * `1` - other
             *  - 32 bytes SSID
             *  - 1 byte NULL terminator
             */
            char scan_entry_string[4u + 4u + 32u + 1u];
            int retval;

            retval = snprintf(scan_entry_string, sizeof(scan_entry_string), "%04d%04u%s",
                              sorted_scan_results->result[i].rssi,
                              sorted_scan_results->result[i].auth_mode == WIFI_NETWORK__AUTH_OPEN ? 0u : 1u,
                              sorted_scan_results->result[i].ssid);
            /* NULL terminate the string in case snprintf filled in all characters */
            scan_entry_string[sizeof(scan_entry_string) - 1u] = '\0';
            if ((retval < 0) || (retval > (int)sizeof(scan_entry_string)))
            {
                LOG(W, "failed to parse scan entry %u", i);
                continue;
            }
            LOG(I, "scan entry [%u] %s", i, scan_entry_string);
            ops = connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                WIFI0SCAN,
                scan_entry_string,
                strlen(scan_entry_string),
                connector_wifi.connector_id,
                portMAX_DELAY);
            TRUE_CHECK(ops);
            /* Give some opportunity to UART connector (if used) to send data */
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        /* Terminating publish */
        ops = connector_send_frame_to_broker(
            DDMP2_CONTROL_PUBLISH,
            WIFI0SCAN,
            NULL,
            0,
            connector_wifi.connector_id,
            portMAX_DELAY);
        TRUE_CHECK(ops);
        break;
    }
    case WIFI_NETWORK__STA_EVENT_AVAILABLE:
    {
        ASSERT((payload.i32 >= 0) && (payload.i32 < (int32_t)ELEMENTS(wifi_state.networks)));
        if (!wifi_state.networks[payload.i32].is_available)
        {
            wifi_state.networks[payload.i32].is_available = true;
            ops = connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                WFWL0AVL | DDM2_PARAMETER_INSTANCE((uint32_t)payload.i32),
                &One,
                sizeof(One),
                connector_wifi.connector_id,
                portMAX_DELAY);
            TRUE_CHECK(ops);
        }
        break;
    }
    default:
        LOG(W, "Wi-Fi event %u(%x) was ignored", sta_event, payload.i32);
        break;
    }
}
#endif

//! \~ Send publish from broker to all subscribers, return 1=OK
static void connector_wifi_process_task(void *parameter)
{
    DDMP2_FRAME *pframe;
    DDMP2_FRAME l_frame;
    size_t frame_size;

#ifdef CONNECTOR_WIFI_DELAYED_STARTUP
    vTaskDelay(pdMS_TO_TICKS(CONNECTOR_WIFI_DELAYED_STARTUP));
#endif
    wifi_network__init();

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    int status = wifi_network__register_handler(handle_wifi_event);
    TRUE_CHECK(status == WIFI_NETWORK__OK);
#endif

    connector_wifi_load_aplist();
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    /* Add preconfigured network into whitelist if not already added */
#if defined(CONFIG_WIFI_SSID)
    wifi_network__network_initialize(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
#endif
    /* NOTE:
     * This build network configuration is a hack: it is added from project configuration header
     * file to reduce build times.
     */
#if defined(BUILD_STATION_SSID)
    wifi_network__network_initialize(BUILD_STATION_SSID, BUILD_STATION_PASSWORD);
#endif
#endif
    wifi_services_init(&connector_wifi);

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    wifi_network__enable();  // Autostart Wi-Fi network
    wifi_network__network_sync(); /* Generate events to update whitelist entries */
#endif
    LOG(I, "Wifi, stack left: %d", uxTaskGetStackHighWaterMark(NULL));

#ifdef CONNECTOR_TLS
    // Initialize TLS
    tls_cert_valid = false;
    tls_cacert_valid = false;

    TRUE_CHECK(connector_send_frame_to_broker(
        DDMP2_CONTROL_SUBSCRIBE,
        TLS0CERT,
        NULL,
        0,
        connector_wifi.connector_id,
        portMAX_DELAY));

    TRUE_CHECK(connector_send_frame_to_broker(
        DDMP2_CONTROL_SUBSCRIBE,
        TLS0CACERT,
        NULL,
        0,
        connector_wifi.connector_id,
        portMAX_DELAY));
#endif  // CONNECTOR_TLS
    while (1)
    {
        pframe = xRingbufferReceive(connector_wifi.to_connector, &frame_size, portMAX_DELAY);

        if (NULL == pframe)
        {
            continue;
        }

        memcpy(&l_frame, pframe, frame_size);
        vRingbufferReturnItem(connector_wifi.to_connector, pframe);
        pframe = &l_frame;

        if (connectors[pframe->destination_connector]->sub_connector_id)
        {
            switch (pframe->frame.control)
            {
            default:
                LOG(D, "Forwarding %s to WiFi:%d", ddmp2_control_string(pframe->frame.control), pframe->destination_connector - connector_wifi.connector_id);
                TRUE_CHECK(wifi_service_send_frame(pframe));
                break;

            case DDMP2_CONTROL_NOP:
            case DDMP2_CONTROL_FRAGMENT:
                break;
            }
        }
        else
        {
            handle_wifi_parameter(pframe);
        }
    }
}

//! \~ Load wifi AP stored SSID
static void connector_wifi_load_aplist(void)
{
    // Scan configuration
    for (int i = 0; i < WIFI_APLIST_SIZE; i++)
    {
        // Build key
        char key[32];
        snprintf(key, sizeof(key), "wfal%dssid", i);

        // Load item
        size_t length;
        length = MAX_SSID_LEN;
        if (config_get_str(key, (char *)wifi_aplist[i].ssid, &length) == ESP_OK)
        {
            // Generate temporary password
            generate_random_chars(wifi_aplist[i].pwd, 8);
        }
        else
        {
            // No stored SSID, generate a new one
            memset(&wifi_aplist[i], 0, sizeof(wifi_aplist[i]));
            // Generate SSID
            generate_random_chars(wifi_aplist[i].ssid, 8);
            // Store SSID
            // config_set_str(key, wifi_aplist[i].ssid);
            // Generate temporary password
            generate_random_chars(wifi_aplist[i].pwd, 8);
        }
    }
}

//! \~ Initializes WiFi and turns on configured modes
static int initialize_connector_wifi(void)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    // Request and register instance
    uint32_t device_class = WIFI0;
    if (broker_register_instance(&device_class, connector_wifi.connector_id) == -1)
    {
        LOG(E, "Failed to register WIFI0 instance. Disabling WIFI!");
        return 0;
    }
#endif

#if CONFIG_IDF_TARGET_ESP32S3
    TRUE_CHECK(xTaskCreate(connector_wifi_process_task, "connector_wifi", 3772, NULL, xTASK_PRIORITY_BELOW_NORMAL, NULL));
#else
    TRUE_CHECK(xTaskCreate(connector_wifi_process_task, "connector_wifi", 3260, NULL, xTASK_PRIORITY_BELOW_NORMAL, NULL));
#endif

    return 1;
}

static void generate_random_chars(char *const buf, const int len)
{
    for (int i = 0; i < len; i++)
    {
        buf[i] = random_char[esp_random() % 62];
    }
}

int connector_wifi_get_ap_info(char *const ssid, char *const pwd)
{
    strncpy(ssid, wifi_aplist[0].ssid, MAX_SSID_LEN);
    strncpy(pwd, wifi_aplist[0].pwd, MAX_PASSPHRASE_LEN);

    return 1;
}

CONNECTOR connector_wifi = {
    .name = "BASE64 WiFi connector",
    .initialize = initialize_connector_wifi,
    .data_lines = CONNECTOR_WIFI_MAX_CONNECTIONS,
};
