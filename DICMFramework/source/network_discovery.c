/**
 * @file
 * @brief Application network discovery implementation
 *
 * Application network discovery over Wi-Fi or LAN connections
 */

#include "configuration.h"
//#ifndef CONFIG_IDF_TARGET_LINUX

#include "network_discovery.h"
#include "esp_idf_version.h"
#include <stdint.h>
#include <string.h>
#include "hal_mem.h"

#include "mdns.h"
#include "cJSON.h"
#include "ddm2.h"

#define DISCOVERY_REQUEST_DATA              0x444d4444      //!< \~ Dometic Coolbox WiFi Discovery, 'DMDD' "DDMD"
#define DDMP_WIFI_PROTOCOL_VERSION          0x2

static network_discovery_query_callback_t l_notify_callback = NULL;
static mdns_search_once_t *l_search = NULL;

#ifdef MULTICAST_DNS_SERVICE_NAME
const char * const mdns_service_name = MULTICAST_DNS_SERVICE_NAME;
#else
const char * const mdns_service_name = "Dometic DICM";
#endif //MULTICAST_DNS_SERVICE_NAME

/**
 * @brief       UDP discovery packet sent from app
 */
typedef struct _DISCOVERY_REQUEST
{
    uint32_t packet_type;               //!< \~ Packet type identifier
} DISCOVERY_REQUEST;

/* NOTE:
 * We must check if NETWORK_DISCOVERY__MAX_REQUEST_SIZE is of sufficient size for keeping
 * DISCOVERY_REQUEST.
 */
COMPILE_TIME_ASSERT(sizeof(DISCOVERY_REQUEST) <= NETWORK_DISCOVERY__MAX_REQUEST_SIZE);

/**
 * @brief       Generates an announcement JSON object
 */
static char * generate_announcement_json(
    const char * const serial_number,
    const char * const device_name,
    const uint8_t pid,
    const char * const sku)
{
    cJSON *root = cJSON_CreateObject();

    TRUE_CHECK_RETURN0(root != NULL);

    cJSON_AddNumberToObject(root, "version", DDMP_WIFI_PROTOCOL_VERSION);
    cJSON_AddNumberToObject(root, "pid", pid);
    cJSON_AddStringToObject(root, "id", serial_number);
    cJSON_AddStringToObject(root, "name", device_name);
    cJSON_AddNumberToObject(root, "f", gateway_advertisement_flag_byte());
    cJSON_AddStringToObject(root, "sku", sku);

    char * string = cJSON_Print(root);
    cJSON_Minify(string);
    cJSON_Delete(root);

    return string;
}

int network_discovery__mdns_initialize(void)
{
	const char *version = gateway_get_firmware_version();
	char fwid_list[40] = FIRMWARE_BUILD_ID "_";
	strcat(fwid_list, version);
    mdns_txt_item_t service_txt_data[] =
    {
        {"board", HARDWARE_STRING},
        {"sku", (char*)gw0sku},
        {"fwversion", (char*)fwid_list},
    };

    ZERO_CHECK_RETURN0(mdns_init());
    ZERO_CHECK_RETURN0(mdns_hostname_set(device_information.default_name));

    LOG(I, "mDNS hostname set to [%s]", device_information.default_name);
    
    ZERO_CHECK_RETURN0(mdns_instance_name_set("instance"));
    ZERO_CHECK_RETURN0(mdns_service_add(mdns_service_name, "_ddmp", "_tcp", DDMP2_LISTEN_PORT, service_txt_data, ELEMENTS(service_txt_data)));

    return 1;
}

esp_err_t network_discovery__mdns_update(const char * const key, const char * const value)
{
    return mdns_service_txt_item_set("_ddmp", "_tcp", key, value);
}

int network_discovery__request_validate(const void * const packet_data, const size_t length)
{
    const DISCOVERY_REQUEST * packet = packet_data;

    if (length != sizeof(DISCOVERY_REQUEST))
    {
        return NETWORK_DISCOVERY__ERROR_INVALID_SIZE;
    }
    if (packet->packet_type != DISCOVERY_REQUEST_DATA)
    {
        return NETWORK_DISCOVERY__ERROR_DATA_MISMATCH;
    }
    return NETWORK_DISCOVERY__ERROR_OK;
}

int network_discovery__response_generate(void ** response_data, size_t * response_data_size)
{
    char * response;

    response = generate_announcement_json(
        /* serial_number */     device_information.id_string,
        /* device_name */       device_information.default_name,
        /* pid */               DEFAULT_PRODUCT_ID,
        /* sku */               (char *)gw0sku);
    if (response == NULL)
    {
        return NETWORK_DISCOVERY__ERROR_NO_MEMORY;
    }
    *response_data = response;
    *response_data_size = strlen(response);
    return NETWORK_DISCOVERY__ERROR_OK;
}

void network_discovery__response_free(void * response_data_size)
{
    cJSON_free(response_data_size);
}
static inline bool is_netif_ap(mdns_result_t *r)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	return (esp_netif_get_handle_from_ifkey("WIFI_AP_DEF") == r->esp_netif);
#else
	return (r->tcpip_if == MDNS_IF_AP);
#endif
}
static void mdns_parse_results(mdns_result_t *results, network_discovery_result_t *p_result)
{
    mdns_result_t *r = results;
    mdns_ip_addr_t *a = NULL;
    int i = 0;
    size_t t;
    while (r)
    {
    	if (is_netif_ap(r))
        {
            // only count if detected on our AP
            ++i;
        }
        r = r->next;
    }
    if (p_result)
    {
        p_result->num_items = 0;
        p_result->items = NULL;
    }
    if (p_result && i)
    {
        p_result->items =  (network_discovery_item_result_t*)hal_mem_malloc_prefer(sizeof(network_discovery_item_result_t) * i, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (p_result->items)
        {
            p_result->num_items = i;
        }
    }
    i = 1;
    int ii = 0;
    r = results;
    while (p_result && (p_result->num_items > 0) && r)
    {
        p_result->items[ii].name = NULL;
        if (r->instance_name)
        {
        	if (is_netif_ap(r))
            {

                p_result->items[ii].name = (char*)hal_mem_malloc_prefer(strlen(r->instance_name)+1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (p_result->items[ii].name)
                {
                    strcpy(p_result->items[ii].name, r->instance_name);
                }
            }
        }

        p_result->items[ii].hostname = NULL;
        if (r->hostname)
        {
        	if (is_netif_ap(r))
            {
                p_result->items[ii].hostname = (char*)hal_mem_malloc_prefer(strlen(r->hostname)+1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                if (p_result->items[ii].hostname)
                {
                    strcpy(p_result->items[ii].hostname, r->hostname);
                    p_result->items[ii].port = r->port;
                }
            }
        }
        if (r->txt_count)
        {
            p_result->items[ii].sku = NULL;
            p_result->items[ii].fwversion = NULL;
            p_result->items[ii].board = NULL;
            for (t = 0; t < r->txt_count; t++)
            {
            	if (is_netif_ap(r))
                {
                    if (0 == strcmp(r->txt[t].key, "fwversion") && (r->txt_value_len[t] > 0))
                    {
                        p_result->items[ii].fwversion = (char*)hal_mem_malloc_prefer(r->txt_value_len[t]+1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                        if (p_result->items[ii].fwversion)
                        {
                            strcpy(p_result->items[ii].fwversion, r->txt[t].value);
                        }
                    }
                    else if (0 == strcmp(r->txt[t].key, "sku") && (r->txt_value_len[t] > 0))
                    {
                        p_result->items[ii].sku = (char*)hal_mem_malloc_prefer(r->txt_value_len[t]+1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                        if (p_result->items[ii].sku)
                        {
                            strcpy(p_result->items[ii].sku, r->txt[t].value);
                        }
                    }
                    else if (0 == strcmp(r->txt[t].key, "board") && (r->txt_value_len[t] > 0))
                    {
                        p_result->items[ii].board = (char*)hal_mem_malloc_prefer(r->txt_value_len[t]+1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                        if (p_result->items[ii].board)
                        {
                            strcpy(p_result->items[ii].board, r->txt[t].value);
                        }
                    }
                }
            }
        }
        a = r->addr;
        while (a)
        {
            if (a->addr.type == ESP_IPADDR_TYPE_V6)
            {
                // Do nothing
            }
            else
            {
            	if (is_netif_ap(r))
                {
                    p_result->items[ii].ip_addr = a->addr;
                }
            }
            a = a->next;
        }
    	if (is_netif_ap(r))
        {
            ++ii;
        }
        r = r->next;
    }
}

void mdns_query_notify(mdns_search_once_t *search)
{
    // Result are ready to be fetched
    if (l_notify_callback)
    {
        l_notify_callback();
    }
    l_notify_callback = NULL;
}

static void query_mdns_service(network_discovery_result_t *p_result)
{
    LOG(I, "Query PTR: %s.%s.local", "_ddmp", "_tcp");

    mdns_result_t * results = NULL;
    esp_err_t err = mdns_query_ptr("_ddmp", "_tcp", 2000, 10,  &results);
    if (err)
    {
        LOG(E, "Query Failed: %s", esp_err_to_name(err));
        return;
    }
    if (!results)
    {
        LOG(W, "No results found!");
    }
    mdns_parse_results(results, NULL);
    mdns_query_results_free(results);
}

void network_discovery__query_service(network_discovery_result_t *p_result)
{
    query_mdns_service(p_result);
}

bool network_discovery__query_async_service(network_discovery_query_callback_t notify_callback)
{
    if (l_search)
    {
        LOG(W, "Another search is ongoing. Failed to start a new.");
        return false;
    }
    mdns_search_once_t *search = mdns_query_async_new(NULL, "_ddmp", "_tcp", MDNS_TYPE_PTR, 3000, 10, mdns_query_notify);

    if (!search)
    {
        LOG(E, "Async Query Failed");
        return false;
    }
    l_search = search;  // Save search pointer
    l_notify_callback = notify_callback;
    return true;
}

bool network_discovery__query_fetch_results(network_discovery_result_t *p_result)
{
    bool ret_val = false;
    mdns_result_t * results = NULL;
    if (l_search)
    {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    	uint8_t num_results = 0;
        if (true == mdns_query_async_get_results(l_search, 5000, &results, &num_results))
#else
        if (true == mdns_query_async_get_results(l_search, 5000, &results))
#endif
        {
            // Add return of data in new structure
            mdns_parse_results(results, p_result);
            mdns_query_results_free(results);
            ret_val = true;
        }
        else
        {
            LOG(W, "results failed");
        }
        if (ESP_OK == mdns_query_async_delete(l_search))
        {
            l_search = NULL;
        }
    }
    return ret_val;
}

void network_discovery__query_free_results(network_discovery_result_t *p_result)
{
    int i = 0;

    if (p_result && p_result->items)
    {
        while (i < p_result->num_items)
        {
            if (p_result->items[i].fwversion)
            {
                free((void*)p_result->items[i].fwversion);
            }
            if (p_result->items[i].sku)
            {
                free((void*)p_result->items[i].sku);
            }
            if (p_result->items[i].hostname)
            {
                free((void*)p_result->items[i].hostname);
            }
            if (p_result->items[i].name)
            {
                free((void*)p_result->items[i].name);
            }
            if (p_result->items[i].board)
            {
                free((void*)p_result->items[i].board);
            }
            i++;
        }
        free((void*)p_result->items);
    }
}
//#endif
