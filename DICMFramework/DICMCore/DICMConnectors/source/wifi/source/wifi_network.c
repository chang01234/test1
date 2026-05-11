/**
 * @file wifi_network.c
 * @brief Wifi module
 *
 * WiFi and network management.
 */

#include <stdint.h>
#include <string.h>

#include "configuration.h"

#include "esp_event.h"
#include "esp_netif_types.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_wifi_types_generic.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/ip4_addr.h"
#include "wifi_network.h"

// Need to define them here as we use mocked versions of libraries
#ifdef CONFIG_IDF_TARGET_LINUX
ESP_EVENT_DEFINE_BASE(IP_EVENT);
ESP_EVENT_DEFINE_BASE(WIFI_EVENT);
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
#define IDF_HAS_BEACON_TIMEOUT
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#define IDF_HAS_WIFI_EVENT_HOME_CHANNEL_CHANGE
#endif

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
#define PASSIVE_SCAN_PERIOD_MAX_MS 30000u
#define ACTIVE_SCAN_PERIOD_MAX_MS  60000u
#define IDLE_TIME_WAIT_MS          1000u

#define CONNECTION_RSSI_BAD_THRESHOLD (-80)

#define WHITELIST_KEY_LENGTH        32u
#define WHITELIST_SSID_UNUSED_ENTRY ""
#define WIFI_WIFI0CNW               "wifi0cnw"
#endif

#define WIFI_STACK_ERR_OK               0
#define WIFI_STACK_ERR_INTERNAL         -1
#define WIFI_STACK_ERR_BAD_ARGS         -2
#define WIFI_STACK_ERR_ALREADY_ENABLED  -3
#define WIFI_STACK_ERR_ALREADY_DISABLED -4

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
#define WIFI_STA_EVENT_HANDLER_MAX_COUNT 2
#endif
#define WIFI_AP_EVENT_HANDLER_MAX_COUNT  2

/*
 * Enable this to see candidate list printed in logs
 */
#define ENABLE_VERBOSE_CANDIDATE 0

/*
 * Structure containing configurations for station and AP
 */
typedef struct stack_context
{
    wifi_mode_t mode;             //<! Current Wi-Fi stack mode: AP, APSTA, STA or NULL
    SemaphoreHandle_t mode_lock;  //<! Synchronization for accessing `mode` member
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    wifi_config_t sta;            //<! Station configuration
#endif
    wifi_config_t ap;             //<! AP configuration
} STACK_CONTEXT;

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
struct stack_connection_info
{
    uint32_t channel;
    int32_t rssi;
};

/* -- Time lists -- */

/*
 * Time list
 */
typedef struct time_list
{
    const uint32_t *periods;  //!< Pointer to array containing different timeouts
    uint32_t no_periods;      //!< Number of elements in pointed array
} TIME_LIST;

/*
 * Time list iterator
 */
typedef struct time_list_iter
{
    const struct time_list *tl;  //!< Time list associated with this iterator
    uint32_t index;              //!< Current index of the iterator
} TIME_LIST_ITER;

/* -- Wi-Fi Station Adapter state machine types -- */

/**
 * @brief   Wi-Fi station adapter SM events
 */
typedef enum sta_adapter_evt
{
    /* External Input events */
    STA_ADAPTER_EVT_SCAN = 100,
    STA_ADAPTER_EVT_SCAN_ABORT,
    STA_ADAPTER_EVT_CONNECT,
    STA_ADAPTER_EVT_DISCONNECT,
    STA_ADAPTER_EVT_LOCK_MODE_CHANGE,
    STA_ADAPTER_EVT_UNLOCK_MODE_CHANGE,
    /* External Output events */
    STA_ADAPTER_EVT_DISCONNECTED = 200,
    STA_ADAPTER_EVT_DISCONNECTED_RETRYING,
    STA_ADAPTER_EVT_DISCONN_BAD_PASSWORD,
    STA_ADAPTER_EVT_DISCONN_NO_AP_FOUND,
    STA_ADAPTER_EVT_CONNECTED,
    STA_ADAPTER_EVT_STARTED,
    STA_ADAPTER_EVT_STOPPED,
    STA_ADAPTER_EVT_SCAN_RESULTS,
    STA_ADAPTER_EVT_SCAN_DENIED,
    /* Internal Input events: these events are comming from ESP-IDF Wi-Fi stack */
    STA_ADAPTER_EVT_STACK_STARTED = 500,
    STA_ADAPTER_EVT_STACK_STOPPED,
    STA_ADAPTER_EVT_STACK_DISCONNECTED,
    STA_ADAPTER_EVT_STACK_CONNECTED,
    STA_ADAPTER_EVT_STACK_SCAN_DONE,
    /* Internal time events */
    STA_ADAPTER_EVT_TIMEOUT = 10000
} STA_ADAPTER_EVT_ENUM;

typedef enum sta_adapter_evt_stack_disconn_reason
{
    STA_ADAPTER_EVT_STACK_DISCONN_REASON_BAD_PASSWORD,
    STA_ADAPTER_EVT_STACK_DISCONN_REASON_NO_AP_FOUND,
    STA_ADAPTER_EVT_STACK_DISCONN_REASON_UNKNOWN
} STA_ADAPTER_EVT_STACK_DISCONN_REASON_ENUM;

typedef enum sta_adapter_evt_disconn_reason
{
    STA_ADAPTER_EVT_DISCONN_REASON_BAD_PASSWORD,
    STA_ADAPTER_EVT_DISCONN_REASON_NO_AP_FOUND,
    STA_ADAPTER_EVT_DISCONN_REASON_UNKNOWN
} STA_ADAPTER_EVT_DISCONN_REASON_ENUM;

/**
 * @brief   A connect request event structure
 *
 * Each connect request event contains a Wi-Fi list entry and a time list for connection retry
 * sequence.
 */
struct sta_adapter_evt_connect
{
    WIFI_LIST__ENTRY network;
    TIME_LIST retry_sequence;
};

enum sta_adapter_evt_scan_type
{
    STA_ADAPTER_EVT_SCAN_TYPE_ACTIVE,
    STA_ADAPTER_EVT_SCAN_TYPE_PASSIVE
};

enum sta_adapter_evt_stack_scan_type
{
    STA_ADAPTER_EVT_STACK_SCAN_TYPE_ACTIVE = STA_ADAPTER_EVT_SCAN_TYPE_ACTIVE,
    STA_ADAPTER_EVT_STACK_SCAN_TYPE_PASSIVE = STA_ADAPTER_EVT_SCAN_TYPE_PASSIVE
};

/**
 * @brief   Scan request with a specific channel
 *
 * If channel is set to zero, then all channels will be scanned, otherwise a specific channel is
 * scanned.
 */
struct sta_adapter_evt_scan
{
    uint32_t channel;
    enum sta_adapter_evt_scan_type type;
};

/**
 * @brief   Once a connection is established `connected` event is generated.
 *
 * It contains the original data contained in `connect` event.
 */
struct sta_adapter_evt_connected
{
    WIFI_LIST__ENTRY network;
};

/**
 * @brief   Once a SM decides that a network cannot be connected to this event is generated.
 *
 * It contains the reason for disconnection.
 */
struct sta_adapter_evt_disconnected
{
    STA_ADAPTER_EVT_DISCONN_REASON_ENUM reason;
};

/**
 * @brief   Before the SM finally decides that it can not connect to a network it will generate
 *          this event.
 *
 * This event is generated whenever Wi-Fi stack responds with a disconnect event.
 */
struct sta_adapter_evt_disconnected_retrying
{
    STA_ADAPTER_EVT_DISCONN_REASON_ENUM reason;
};

/**
 * @brief   Disconnected event generated by Wi-Fi stack.
 */
struct sta_adapter_evt_stack_disconnected
{
    STA_ADAPTER_EVT_STACK_DISCONN_REASON_ENUM reason;
};

/**
 * @brief   Event generated at end of scan cycle.
 */
struct sta_adapter_evt_scan_results
{
    struct wifi_network__scan_results info;
};

/**
 * @brief   Workspace structure of Wi-Fi station adapter SM
 */
struct sta_adapter_workspace
{
    eds__etimer *timeout;  //!< A timer handle used for time events
    TIME_LIST_ITER retry;  //!< Connection retry time list

    /**
     * @brief   Is Wi-Fi mode change allowed?
     *
     * When this boolean is set, the state machine may change the Wi-Fi mode in order to optimize
     * the power consumption, otherwise, the Wi-Fi mode will remain unchanged. This might impact
     * the connectivity as well in some cases when lock was done while station mode was disabled.
     */
    bool is_mode_change_allowed;

    /**
     * @brief   Connect events
     *
     * Contains references to current and pending connection events. The current one is kept while
     * connected, once the network is disconnected, the event is thrown away and the pending event
     * takes the place of current one.
     */
    struct sta_adapter_mode_connect
    {
        const eds__event *pending;
        const eds__event *current;
    } connect;

    struct sta_adapter_evt_scan scan;                               //!< Scan event data which temporary contains scan request data
    STA_ADAPTER_EVT_STACK_DISCONN_REASON_ENUM last_disconn_reason;  //!< Last disconnected reason
};

/* -- Wi-Fi Station Manager state machine types -- */

/**
 * @brief   Wi-Fi station manager events
 *
 */
enum sta_manager_evt
{
    /* External input events */
    STA_MANAGER_EVT_NETWORK_SELECT = 1000,
    STA_MANAGER_EVT_NETWORK_ADD,
    STA_MANAGER_EVT_NETWORK_INITIALIZE,
    STA_MANAGER_EVT_NETWORK_SSID_UPDATE,
    STA_MANAGER_EVT_NETWORK_PWD_UPDATE,
    STA_MANAGER_EVT_NETWORK_CHN_UPDATE,
    STA_MANAGER_EVT_NETWORK_REMOVE,
    STA_MANAGER_EVT_DISABLE,
    STA_MANAGER_EVT_ENABLE,
    STA_MANAGER_EVT_NETWORK_SYNC,
    STA_MANAGER_EVT_SCAN_START,
    STA_MANAGER_EVT_SCAN_STOP,
    STA_MANAGER_EVT_RECONNECT,
    STA_MANAGER_EVT_ENABLE_BACKGROUND_SCANNER,
    STA_MANAGER_EVT_DISABLE_BACKGROUND_SCANNER,
    /* Timeout events */
    STA_MANAGER_EVT_TIMEOUT = 20000,
};

/**
 * @brief   Workspace structure of Wi-Fi station manager SM
 */
struct sta_manager_workspace
{
    eds__etimer *timeout;  //!< A timer handle used for time events

    /**
     * @brief   Is this a first connection try?
     *
     * When this boolean is set, the state machine will enforce setting the network which is
     * selected by CNW.
     */
    bool is_first_connect;

    /**
     * @brief   Passive scanner sub-structure
     *
     * This structure is used only by passive background scanner.
     */
    struct sta_passive_scan
    {
        uint32_t current_index;     //<! Index used to iterate over channel list
        TIME_LIST_ITER scan_retry;  //<! Timeout list iterator
        bool is_enabled;            //<! Flag used to temporary enable/disable scanner
    } passive_scan;

    /**
     * @brief   Search scanner sub-structure
     *
     * This structure is used only by search foreground scanner.
     */
    struct sta_search_scan
    {
        uint32_t current_index;       //<! Index used to iterate over channel list
        TIME_LIST_ITER scan_retry;    //<! Timeout list iterator
        const uint32_t *scan_list;    //<! Currently used channel scan list
        uint32_t scan_list_elements;  //<! Number of entries in current channel scan list
    } search_scan;

    /**
     * @brief   Scanned list
     *
     * This list is formed by active search/passive background scanning requests. This list is
     * slowly being filled with available AP for all enabled channels. Once all channels are scanned
     * then this list is transferred to `candidate_list`.
     */
    WIFI_LIST__REF scanned_list;
    /**
     * @brief   Candidate list
     *
     * This list contains sorted entries of networks which are detected in the active search/passive
     * background scan. Scanned list contains Wi-Fi network references and scanned channel.
     */
    WIFI_LIST__REF candidate_list;
};

/*
 * Attached data for STA_MANAGER_EVT_NETWORK_SELECT event
 */
struct sta_manager_evt_network_select
{
    int32_t index;
};

/*
 * Attached data for STA_MANAGER_EVT_NETWORK_ADD event
 */
struct sta_manager_evt_network_add
{
    char ssid[WIFI_LIST__SSID_LENGTH];
    char pwd[WIFI_LIST__PWD_LENGTH];
};

/*
 * Attached data for STA_MANAGER_EVT_NETWORK_REMOVE event
 */
struct sta_manager_evt_network_remove
{
    int32_t index;
};

/*
 * Attached data for STA_MANAGER_EVT_NETWORK_SSID event
 */
struct sta_manager_evt_network_ssid
{
    int32_t index;
    char ssid[WIFI_LIST__SSID_LENGTH];
};

/*
 * Attached data for STA_MANAGER_EVT_NETWORK_PWD event
 */
struct sta_manager_evt_network_pwd
{
    int32_t index;
    char pwd[WIFI_LIST__PWD_LENGTH];
};

/*
 * Attached data for STA_MANAGER_EVT_NETWORK_CHN event
 */
struct sta_manager_evt_network_chn
{
    int32_t index;
    uint32_t chn;
};

/*
 * Attached data for STA_MANAGER_EVT_NETWORK_INITIALIZE event
 */
struct sta_manager_evt_network_initialize
{
    char ssid[WIFI_LIST__SSID_LENGTH];
    char pwd[WIFI_LIST__PWD_LENGTH];
};
#endif

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void time_list_iter_init(TIME_LIST_ITER *iter, const TIME_LIST *tl);
static void time_list_iter_next(TIME_LIST_ITER *iter);
static int32_t time_list_iter_current(const TIME_LIST_ITER *iter);
static void time_list_iter_reset(TIME_LIST_ITER *iter);

static int32_t cnw_load(void);
static void cnw_save(int32_t cnw);

static void whitelist_init(void);
static int32_t whitelist_synchronize(void);
static void whitelist_save(void);
static int32_t whitelist_cnw(void);
static bool whitelist_entry_is_used(uint32_t index);
static void whitelist_update(const WIFI_LIST__ENTRY *entry, uint32_t index);
static void whitelist_update_ssid(uint32_t index, const char *ssid);
static void whitelist_update_pwd(uint32_t index, const char *pwd);
static void whitelist_update_channel(uint32_t index, int32_t channel);
static void whitelist_delete_at(uint32_t index);
static WIFI_LIST__ENTRY *whitelist_entry(uint32_t index);
static int32_t whitelist_find(const char *ssid);
static bool whitelist_is_empty(void);
#endif

/*
 * Helper functions which are used to generate station and AP events to external code using station
 * and AP event callbacks
 */
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void sta_event_generate_i32(enum wifi_network__sta_event sta_event, int32_t payload);
static void sta_event_generate(enum wifi_network__sta_event sta_event,
                               union wifi_network__event_payload payload);
#endif
static void ap_event_generate(WIFI_NETWORK__AP_EVENT_ENUM ap_event);

/*
 * Common stack helper functions
 */
static void stack_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void stack_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void stack_init(void);

/*
 * Stack functions for managing AP
 */
static void stack_ap_init(void);
static int stack_ap_enable(void);
static int stack_ap_disable(void);

/*
 * Stack functions for managing station
 */
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static void stack_sta_init(void);
static int stack_sta_enable(void);
static int stack_sta_disable(void);
static int stack_sta_connect(const char *ssid, const char *pwd, uint32_t channel);
static int stack_sta_disconnect(void);
static void stack_sta_get_connection_info(struct stack_connection_info *info);
static void stack_sta_scan_start(uint32_t channel, enum sta_adapter_evt_stack_scan_type scan_type);
static void stack_sta_scan_stop(void);
static uint32_t stack_sta_scan_get_results(struct wifi_network__scan_result *results,
                                           uint32_t no_entries);
static const char *stack_sta_err_to_string(wifi_err_reason_t wifi_err_reason);
#endif
static void stack_init(void);
static void stack_terminate(void);

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/*
 * Wi-Fi station adapter state machine states
 *
 * The state machine diagram can be found in `documentation/wifi_network.md` document.
 */
static eds__sm_action sta_adapter_init(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_service(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_idle(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_idle_waiting(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_idle_dormant(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_wait_start(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_active(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_disconnecting(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_active_connect(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_connecting(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_connected(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_connecting_start(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_connecting_retry(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_changing_network(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_active_scan(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_scan_wait_start(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_adapter_scan_start(eds__sm *sm, void *arg, const eds__event *event);

/*
 * Wi-Fi station manager state machine states
 *
 * The state machine diagram can be found in `documentation/wifi_network.md` document.
 */
static eds__sm_action sta_manager_init(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_service(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_idle(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_disable_wait_adapter(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_active(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_active_connecting(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_connecting_recent(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_recent_loop(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_recent_connecting(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_search_scan(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_search_scan_start(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_search_scan_do(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_search_scan_cooldown(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_search_scan_wait(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_search_active_scan(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_recent_connected(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_passive_scan_start(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_passive_scan_do(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_passive_scan_cooldown(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_passive_scan_wait(eds__sm *sm, void *arg, const eds__event *event);
static eds__sm_action sta_manager_active_scan(eds__sm *sm, void *arg, const eds__event *event);

static bool sta_adapter_evt_connect_ssid_is_equal(
    const struct sta_adapter_evt_connect *self,
    const struct sta_adapter_evt_connect *other);

static eds__agent *s__sta_adapter_agent;
static eds__agent *s__sta_manager_agent;
static SemaphoreHandle_t l_wifi_whitelist_mutex;
static EXT_RAM_ATTR WIFI_LIST s__wifi_whitelist;

/*
 * This contains wifi_network interface scan results
 *
 * It was allocated statically since the structure would provide quite a load on stack.
 */
static EXT_RAM_ATTR struct wifi_network__scan_results if_scan_results;

/*
 * Callback function for handling station events
 */
static wifi_network__event_handler *s__event_handlers[WIFI_STA_EVENT_HANDLER_MAX_COUNT];
#endif

/*
 * Callback function for handling AP events
 */
static wifi_network__ap_event_handler *s__ap_event_handlers[WIFI_AP_EVENT_HANDLER_MAX_COUNT];

/*
 * Country setting defaults
 */
static const wifi_country_t s__country_defaults = {
    .cc = "CN",
    .schan = 1,
    .nchan = 13,
    .policy = WIFI_COUNTRY_POLICY_AUTO,
};

/*
 * Stack context used by ESP IDF framework.
 *
 * NOTE: It seems that ESP IDF framework does not like EXT_RAM_ATTR for this
 * structure as it will not properly start from time to time. This is based on
 * feedback from testers and is not confirmed with Espressif.
 */
static STACK_CONTEXT stack_context = {
    .mode = WIFI_MODE_NULL,
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    .sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
#endif
    .ap.ap.max_connection = 1,
    .ap.ap.channel = 0,
    .ap.ap.authmode = WIFI_AUTH_WPA2_PSK,
    .ap.ap.ssid = DEFAULT_DEVICE_NAME_PREFIX,
    .ap.ap.password = DEFAULT_DEVICE_PASSWORD,
};

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
static const uint32_t KNOWN_CHANNEL_PERIODS[] = {
    1000,
    1000,
    2000,
};

static const TIME_LIST KNOWN_CHANNEL_TIME_LIST = {
    .no_periods = ELEMENTS(KNOWN_CHANNEL_PERIODS),
    .periods = KNOWN_CHANNEL_PERIODS};

static const uint32_t UNKNOWN_CHANNEL_PERIODS[] = {
    1000,
    1000,
    2000,
    2000,
    5000,
};

static const TIME_LIST UNKNOWN_CHANNEL_TIME_LIST = {
    .no_periods = ELEMENTS(UNKNOWN_CHANNEL_PERIODS),
    .periods = UNKNOWN_CHANNEL_PERIODS};

static const uint32_t PASSIVE_SCAN_PERIODS[] = {
    1000,
    1000,
    2000,
    2000,
    3000,
    4000,
    5000,
    5000,
};

static const TIME_LIST PASSIVE_SCAN_TIME_LIST = {
    .no_periods = ELEMENTS(PASSIVE_SCAN_PERIODS),
    .periods = PASSIVE_SCAN_PERIODS};

static const uint32_t ACTIVE_SCAN_PERIODS[] = {
    1000,
    1000,
    2000,
    2000,
    3000,
    4000,
    5000,
    5000,
};

static const TIME_LIST ACTIVE_SCAN_TIME_LIST = {
    .no_periods = ELEMENTS(ACTIVE_SCAN_PERIODS),
    .periods = ACTIVE_SCAN_PERIODS};

static const uint32_t PASSIVE_SCAN_CHANNEL_LIST[] = {
    1,
    2,
    4,
    6,
    8,
    11,
    13};

/*
 * Primary list of channels which are to be scanned in first round.
 *
 * Typical AP resideds on these channels.
 */
static const uint32_t ACTIVE_SCAN_CHANNEL_LIST[] = {
    1,
    6,
    11,
};

/*
 * Secondary list of channels which are to be scanned in second round
 */
static const uint32_t ACTIVE_SCAN_CHANNEL_LIST_ALTERNATE[] = {
    2,
    3,
    4,
    5,
    7,
    8,
    9,
    10,
    12,
    13};

static void time_list_iter_init(TIME_LIST_ITER *iter, const TIME_LIST *tl)
{
    iter->tl = tl;
    iter->index = 0u;
}

static void time_list_iter_next(TIME_LIST_ITER *iter)
{
    /* This function will increment index, which can go from 0 to time list number of periods.
     * We want to keep index limited so reccurring calls to this function won't overflow the
     * index member.
     * When index is equal to time list number of periods then it means all periods have been
     * used.
     */
    if (iter->index <= iter->tl->no_periods)
    {
        iter->index++;
    }
}

static int32_t time_list_iter_current(const TIME_LIST_ITER *iter)
{
    if (iter->index < iter->tl->no_periods)
    {
        return (int32_t)iter->tl->periods[iter->index];
    }
    else
    {
        return -1;
    }
}

static void time_list_iter_reset(TIME_LIST_ITER *iter)
{
    iter->index = 0u;
}

static int32_t cnw_load(void)
{
    esp_err_t esp_err;
    int32_t cnw = -1;

    esp_err = config_get_i32(WIFI_WIFI0CNW, &cnw);

    if (esp_err != ESP_OK)
    {
        if (esp_err == ESP_ERR_NVS_NOT_FOUND)
        {
            LOG(I, "No data stored for " WIFI_WIFI0CNW);
        }
        else
        {
            LOG(E, "Could not restore " WIFI_WIFI0CNW " from flash, error = 0x%x", esp_err);
        }
    }
    return cnw;
}

static void cnw_save(int32_t cnw)
{
    esp_err_t esp_err;

    esp_err = config_set_i32(WIFI_WIFI0CNW, cnw);

    if (esp_err)
    {
        LOG(W, "failed to save the value: %d", esp_err);
    }
}

static void whitelist_init(void)
{
    l_wifi_whitelist_mutex = xSemaphoreCreateMutex();
    wifi_list__init(&s__wifi_whitelist);
}

static int32_t whitelist_synchronize(void)
{
    esp_err_t esp_err;
    int32_t cnw;
    int32_t new_cnw = 0;
    char cnw_ssid[WIFI_LIST__SSID_LENGTH];

    memset(cnw_ssid, 0, sizeof(cnw_ssid));
    TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
    wifi_list__delete_all(&s__wifi_whitelist);

    for (uint_fast8_t i = 0u; i < WIFI_LIST__SIZE; i++)
    {
        char key[WHITELIST_KEY_LENGTH];
        struct wifi_list__entry *entry;
        size_t data_length;
        int32_t channel;
        char new_ssid[WIFI_LIST__SSID_LENGTH];

        /* Read SSID */
        snprintf(key, sizeof(key), "wfwl%dssid", i);
        data_length = sizeof(new_ssid);
        esp_err = config_get_str(key, new_ssid, &data_length);
        if (esp_err)
        {
            wifi_list__delete_at(&s__wifi_whitelist, i);
            continue;
        }
        if (strlen(new_ssid) == 0)
        {
            wifi_list__delete_at(&s__wifi_whitelist, i);
            continue;
        }
        /* We don't want to add duplicated entries, so just skip to next one. Ignore empty strings
         * first.
         */
        if (whitelist_find(new_ssid) != -1)
        {
            wifi_list__delete_at(&s__wifi_whitelist, i);
            continue;
        }
        entry = wifi_list__entry(&s__wifi_whitelist, i);
        strncpy(entry->ssid, new_ssid, sizeof(entry->ssid));

        /* Read password */
        snprintf(key, sizeof(key), "wfwl%dpw", i);
        data_length = WIFI_LIST__PWD_LENGTH;
        esp_err = config_get_str(key, entry->pwd, &data_length);
        if (esp_err)
        {
            LOG(W, "whitelist: missing password for entry %d", i);
            wifi_list__delete_at(&s__wifi_whitelist, i);
            continue;
        }
        /* Read channel */
        snprintf(key, sizeof(key), "wfwl%dchn", i);
        esp_err = config_get_i32(key, &channel);
        if (esp_err)
        {
            LOG(I, "whitelist: missing channel for entry %d, assuming no channel", i);
            channel = 0u;
        }
        entry->channel = (int8_t)channel;
    }
    /* Get the currently selected network ID */
    cnw = cnw_load();

    if ((cnw == -1) || (cnw >= WIFI_LIST__SIZE))
    {
        LOG(I, "invalid CNW, assuming 0");
        cnw_save(0);
        cnw = 0;
    }
    /* Convert current network (CNW) ID into SSID */
    if (wifi_list__is_used(&s__wifi_whitelist, cnw))
    {
        WIFI_LIST__ENTRY *entry;

        entry = wifi_list__entry(&s__wifi_whitelist, cnw);
        /* Copy SSID to temporary storage */
        strncpy(cnw_ssid, entry->ssid, WIFI_LIST__SSID_LENGTH);
    }
    if (wifi_list__compact(&s__wifi_whitelist))
    {
        whitelist_save();
    }
    /* We still don't have CNW SSID? Choose the first from list. */
    if (cnw_ssid[0] == '\0')
    {
        WIFI_LIST__ENTRY *entry;

        entry = wifi_list__entry(&s__wifi_whitelist, 0);
        strncpy(cnw_ssid, entry->ssid, WIFI_LIST__SSID_LENGTH);
        LOG(D, "invalid CNW, using first whitelist entry: %s@%u", entry->ssid, entry->channel);
    }
    /* If CNW SSID is valid convert it back to CNW ID */
    if (cnw_ssid[0] != '\0')
    {
        new_cnw = wifi_list__find(&s__wifi_whitelist, cnw_ssid);
        if (new_cnw != cnw)
        {
            cnw_save(new_cnw);
        }
    }
    else
    {
        /* We failed to set the current network.
         *
         * This could only happen when whitelist is empty.
         */
        new_cnw = 0;
    }
    /* Print stuff for information */
    LOG(I, "CNW ID %u -> %u", cnw, new_cnw);
    for (uint32_t i = 0; i < WIFI_LIST__SIZE; i++)
    {
        WIFI_LIST__ENTRY *entry;

        entry = wifi_list__entry(&s__wifi_whitelist, i);
        if ((int32_t)i == new_cnw)
        {
            LOG(I, "entry %u: \"%s\"@%u <= current", i, entry->ssid, entry->channel);
        }
        else
        {
            LOG(I, "entry %u: \"%s\"@%u", i, entry->ssid, entry->channel);
        }
    }
    TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
    return new_cnw;
}

static void whitelist_save(void)
{
    for (uint32_t i = 0u; i < WIFI_LIST__SIZE; i++)
    {
        const WIFI_LIST__ENTRY *entry = wifi_list__entry(&s__wifi_whitelist, i);
        whitelist_update(entry, i);
    }
}

static int32_t whitelist_cnw(void)
{
    int32_t cnw;

    /* Get the currently selected network ID */
    cnw = cnw_load();

    if ((cnw == -1) || (cnw >= WIFI_LIST__SIZE))
    {
        return -1;
    }
    if (wifi_list__is_used(&s__wifi_whitelist, cnw) == false)
    {
        return -1;
    }
    return cnw;
}

static bool whitelist_entry_is_used(uint32_t index)
{
    return wifi_list__is_used(&s__wifi_whitelist, index);
}

static void whitelist_update(const WIFI_LIST__ENTRY *entry, uint32_t index)
{
    whitelist_update_ssid(index, entry->ssid);
    whitelist_update_pwd(index, entry->pwd);
    whitelist_update_channel(index, entry->channel);
}

static void whitelist_update_ssid(uint32_t index, const char *ssid)
{
    WIFI_LIST__ENTRY *entry;
    esp_err_t esp_err;
    char key[WHITELIST_KEY_LENGTH];

    LOG(I, "whitelist: saving ssid (%s) for entry %u", ssid, index);
    entry = whitelist_entry(index);
    strncpy(entry->ssid, ssid, sizeof(entry->ssid));
    entry->ssid[sizeof(entry->ssid) - 1u] = '\0'; /* NULL terminate the string */
    /* Save to NVM this only */
    snprintf(key, sizeof(key), "wfwl%dssid", index);
    esp_err = config_set_str(key, entry->ssid);
    if (esp_err)
    {
        LOG(W, "whitelist: failed to save network SSID %s: %d", entry->ssid, esp_err);
    }
}

static void whitelist_update_pwd(uint32_t index, const char *pwd)
{
    WIFI_LIST__ENTRY *entry;
    esp_err_t esp_err;
    char key[WHITELIST_KEY_LENGTH];

    LOG(I, "whitelist: saving pwd for entry %u", index);
    entry = whitelist_entry(index);
    strncpy(entry->pwd, pwd, WIFI_LIST__PWD_LENGTH);
    entry->pwd[sizeof(entry->pwd) - 1u] = '\0'; /* NULL terminate the string */
    /* Save to NVM this only */
    snprintf(key, sizeof(key), "wfwl%dpw", index);
    esp_err = config_set_str(key, entry->pwd);
    if (esp_err)
    {
        LOG(W, "whitelist: failed to save network pwd for %s: %d", entry->ssid, esp_err);
    }
}

static void whitelist_update_channel(uint32_t index, int32_t channel)
{
    WIFI_LIST__ENTRY *entry;
    esp_err_t esp_err;
    char key[WHITELIST_KEY_LENGTH];

    entry = whitelist_entry(index);
    /* Don't save to NVS if information is the same */
    if (entry->channel != channel)
    {
        LOG(I, "whitelist: saving channel (%d) for entry %u", channel, index);
        entry->channel = channel;
        /* Save to NVM this only */
        snprintf(key, sizeof(key), "wfwl%dchn", index);
        esp_err = config_set_i32(key, entry->channel);
        if (esp_err)
        {
            LOG(W, "whitelist: failed to save network channel for %s: %d", entry->ssid, esp_err);
        }
    }
}

static void whitelist_delete_at(uint32_t index)
{
    WIFI_LIST__ENTRY *entry;
    esp_err_t esp_err;
    char key[WHITELIST_KEY_LENGTH];

    entry = whitelist_entry(index);
    LOG(I, "whitelist: deleting \"%s\"@%u for entry %u", entry->ssid, entry->channel, index);
    wifi_list__delete_at(&s__wifi_whitelist, index);
    /* Save to NVM SSID */
    sprintf(key, "wfwl%dssid", index);
    esp_err = config_set_str(key, entry->ssid);
    if (esp_err)
    {
        LOG(W, "whitelist: failed to save network SSID %s: %d", entry->ssid, esp_err);
    }
    /* Save to NVM PWD */
    sprintf(key, "wfwl%dpw", index);
    esp_err = config_set_str(key, entry->pwd);
    if (esp_err)
    {
        LOG(W, "whitelist: failed to save network pwd for %s: %d", entry->ssid, esp_err);
    }
    /* Save to NVM channel */
    sprintf(key, "wfwl%dchn", index);
    esp_err = config_set_i32(key, entry->channel);
    if (esp_err)
    {
        LOG(W, "whitelist: failed to save network channel for %s: %d", entry->ssid, esp_err);
    }
}

static WIFI_LIST__ENTRY *whitelist_entry(uint32_t index)
{
    return wifi_list__entry(&s__wifi_whitelist, index);
}

static int32_t whitelist_find(const char *ssid)
{
    return wifi_list__find(&s__wifi_whitelist, ssid);
}

static bool whitelist_is_empty(void)
{
    /* Lets see if at least one entry is existing in the whitelist */
    for (uint32_t i = 0u; i < WIFI_LIST__SIZE; i++)
    {
        if (whitelist_entry_is_used(i))
        {
            return false;
        }
    }
    return true;
}

static void sta_event_generate_i32(enum wifi_network__sta_event sta_event, int32_t i32)
{
    union wifi_network__event_payload payload;

    payload.i32 = i32;
    sta_event_generate(sta_event, payload);
}

static void sta_event_generate(enum wifi_network__sta_event sta_event,
                               union wifi_network__event_payload payload)
{
    for (size_t i = 0; i < ELEMENTS(s__event_handlers); i++)
    {
        if (s__event_handlers[i] != NULL)
        {
            s__event_handlers[i](sta_event, payload);
        }
    }
}
#endif

static void ap_event_generate(WIFI_NETWORK__AP_EVENT_ENUM ap_event)
{
    for (size_t i = 0; i < ELEMENTS(s__ap_event_handlers); i++)
    {
        if (s__ap_event_handlers[i] != NULL)
        {
            s__ap_event_handlers[i](ap_event);
        }
    }
}

static void stack_wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    eds__error error = EDS__ERROR_NONE;
    eds__event *event = NULL;
#endif

    switch (event_id)
    {
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_EVENT_STA_START:
    {
        error = eds__event_create(STA_ADAPTER_EVT_STACK_STARTED, 0, &event);
        ZERO_CHECK_RETURN(error);
        break;
    }
    case WIFI_EVENT_STA_STOP:
        error = eds__event_create(STA_ADAPTER_EVT_STACK_STOPPED, 0, &event);
        ZERO_CHECK_RETURN(error);
        break;
    case WIFI_EVENT_STA_CONNECTED:
        error = eds__event_create(STA_ADAPTER_EVT_STACK_CONNECTED, 0, &event);
        ZERO_CHECK_RETURN(error);
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
    {
        struct sta_adapter_evt_stack_disconnected *stack_disconnected;

        LOG(W, "Wi-Fi stack disconnect reason: %u (%s)",
            ((wifi_event_sta_disconnected_t *)event_data)->reason,
            stack_sta_err_to_string(((wifi_event_sta_disconnected_t *)event_data)->reason));
        error = eds__event_create(
            STA_ADAPTER_EVT_STACK_DISCONNECTED,
            sizeof(*stack_disconnected),
            &event);
        ZERO_CHECK_RETURN(error);
        stack_disconnected = eds__event_put_data(event);
        switch (((wifi_event_sta_disconnected_t *)event_data)->reason)
        {
        case WIFI_REASON_AUTH_EXPIRE:
            // fallthrough
        case WIFI_REASON_AUTH_FAIL:
            // fallthrough
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            // fallthrough
        case WIFI_REASON_ASSOC_LEAVE:
            stack_disconnected->reason = STA_ADAPTER_EVT_STACK_DISCONN_REASON_BAD_PASSWORD;
            break;
        case WIFI_REASON_CONNECTION_FAIL:
            // fallthrough
        case WIFI_REASON_BEACON_TIMEOUT:
            // fallthrough
        case WIFI_REASON_NO_AP_FOUND:
            stack_disconnected->reason = STA_ADAPTER_EVT_STACK_DISCONN_REASON_NO_AP_FOUND;
            break;
        default:
            stack_disconnected->reason = STA_ADAPTER_EVT_STACK_DISCONN_REASON_UNKNOWN;
            break;
        }
        break;
    }
    case WIFI_EVENT_SCAN_DONE:
    {
        error = eds__event_create(STA_ADAPTER_EVT_STACK_SCAN_DONE, 0, &event);
        ZERO_CHECK_RETURN(error);
        break;
    }
#if defined(IDF_HAS_BEACON_TIMEOUT)
    case WIFI_EVENT_STA_BEACON_TIMEOUT:
        /* This happens when reception is poor, maybe we can use this as triger to do scan? */
        break;

#endif
#endif
    case WIFI_EVENT_AP_START:
    {
        LOG(I, "our AP has been started");
        ap_event_generate(WIFI_NETWORK__AP_EVENT_STARTED);
        break;
    }
    case WIFI_EVENT_AP_STOP:
    {
        LOG(I, "our AP has been stopped");
        ap_event_generate(WIFI_NETWORK__AP_EVENT_STOPPED);
        break;
    }
    case WIFI_EVENT_AP_STACONNECTED:
    {
        LOG(I, "a station was connected to our AP");
        ap_event_generate(WIFI_NETWORK__AP_EVENT_CONNECTED);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        LOG(I, "a station has been disconnected from our AP");
        ap_event_generate(WIFI_NETWORK__AP_EVENT_DISCONNECTED);
        break;
    }
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
#if defined(IDF_HAS_WIFI_EVENT_HOME_CHANNEL_CHANGE)
    case WIFI_EVENT_HOME_CHANNEL_CHANGE:
    {
        const wifi_event_home_channel_change_t *chan_info = event_data;
        LOG(I, "home channel changed from %u:%u to %u:%u", chan_info->old_chan, chan_info->old_snd, chan_info->new_chan, chan_info->new_snd);
        break;
    }
#endif
#endif
    default:
        LOG(W, "unhandled Wi-Fi event %d", event_id);
        break;
    }
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    if (event != NULL)
    {
        error = eds__agent_send(s__sta_adapter_agent, event);
        if (error != EDS__ERROR_NONE)
        {
            /* At this point we don't care if another error happens so ignore return value check */
            (void)eds__event_cancel(event);
        }
    }
#endif
}

static void stack_ip_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    switch (event_id)
    {
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case IP_EVENT_STA_GOT_IP:
        LOG(C, "Got IP from access point: %s", ip4addr_ntoa((ip4_addr_t *)&((ip_event_got_ip_t *)event_data)->ip_info.ip));
        xEventGroupSetBits(network_events, WIFI_IP_BIT);
        break;
    case IP_EVENT_STA_LOST_IP:
        LOG(I, "IP lost!");
        xEventGroupClearBits(network_events, WIFI_IP_BIT);
        break;
#endif
    case IP_EVENT_AP_STAIPASSIGNED:
        LOG(I, "IP was assigned to connected station");
        break;
    default:
        LOG(W, "unhandled IP event %d", event_id);
        break;
    }
}

static void stack_ap_init(void)
{
    esp_netif_t *netif;

    LOG(I, "initializing Wi-Fi AP mode...");
    netif = esp_netif_create_default_wifi_ap();
    ASSERT(netif != NULL);
    (void)netif; /* Remove compiler warning when assert is disabled */
}

/**
 * @brief       Switch on AP mode
 * @return      Operation status
 * @retval      WIFI_STACK_ERR_OK - Wi-Fi was enabled
 * @retval      WIFI_STACK_ERR_ALREADY_ENABLED - Wi-Fi was already enabled
 * @retval      WIFI_STACK_ERR_INTERNAL - Other errors
 */
static int stack_ap_enable(void)
{
    esp_err_t esp_err;
    int retval;

    xSemaphoreTake(stack_context.mode_lock, portMAX_DELAY);
    /* Switch to appropriate WiFi mode */
    switch (stack_context.mode)
    {
    case WIFI_MODE_NULL:
        esp_err = esp_wifi_set_country(&s__country_defaults);
        if (esp_err == ESP_OK)
        {
            esp_err = esp_wifi_set_mode(WIFI_MODE_AP);
            if (esp_err == ESP_OK)
            {
                esp_err = esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_AP, &stack_context.ap);
                if (esp_err == ESP_OK)
                {
                    esp_err = esp_wifi_start();
                    if (esp_err == ESP_OK)
                    {
                        stack_context.mode = WIFI_MODE_AP;
                        retval = WIFI_STACK_ERR_OK;
                    }
                    else
                    {
                        LOG(E, "internal error: esp_wifi_start: %d", esp_err);
                        retval = WIFI_STACK_ERR_INTERNAL;
                    }
                }
                else
                {
                    LOG(E, "internal error: esp_wifi_set_config: %d", esp_err);
                    retval = WIFI_STACK_ERR_INTERNAL;
                }
            }
            else
            {
                LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
                retval = WIFI_STACK_ERR_INTERNAL;
            }
        }
        else
        {
            LOG(E, "internal error: esp_wifi_set_country: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
        }
        break;
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_MODE_STA:
        esp_err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (esp_err == ESP_OK)
        {
            esp_err = esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_AP, &stack_context.ap);
            if (esp_err == ESP_OK)
            {
                stack_context.mode = WIFI_MODE_APSTA;
                retval = WIFI_STACK_ERR_OK;
            }
            else
            {
                LOG(E, "internal error: esp_wifi_set_config: %d", esp_err);
                retval = WIFI_STACK_ERR_INTERNAL;
            }
        }
        else
        {
            LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
        }
        break;
#endif
    case WIFI_MODE_AP:
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
        // fallthrough
    case WIFI_MODE_APSTA:
#endif
        retval = WIFI_STACK_ERR_ALREADY_ENABLED;
        break;
    default:
        retval = WIFI_STACK_ERR_INTERNAL;
        break;
    }
    xSemaphoreGive(stack_context.mode_lock);
    return retval;
}

/**
 * @brief       Switch off AP mode
 * @return      Operation status
 * @retval      WIFI_STACK_ERR_OK - Wi-Fi was disabled
 * @retval      WIFI_STACK_ERR_ALREADY_DISABLED - Wi-Fi was already disabled
 * @retval      WIFI_STACK_ERR_INTERNAL - Other errors
 */
static int stack_ap_disable(void)
{
    esp_err_t esp_err;
    int retval;

    xSemaphoreTake(stack_context.mode_lock, portMAX_DELAY);
    /* Switch to appropriate WiFi mode */
    switch (stack_context.mode)
    {
    case WIFI_MODE_NULL:
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
        // fallthrough
    case WIFI_MODE_STA:
#endif
        retval = WIFI_STACK_ERR_ALREADY_DISABLED;
        break;
    case WIFI_MODE_AP:
        esp_err = esp_wifi_set_mode(WIFI_MODE_NULL);
        if (esp_err == ESP_OK)
        {
            stack_context.mode = WIFI_MODE_NULL;
            retval = WIFI_STACK_ERR_OK;
        }
        else
        {
            LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
        }
        break;
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    case WIFI_MODE_APSTA:
        esp_err = esp_wifi_set_mode(WIFI_MODE_STA);
        if (esp_err != ESP_OK)
        {
            stack_context.mode = WIFI_MODE_STA;
            retval = WIFI_STACK_ERR_OK;
        }
        else
        {
            LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
        }
        break;
#endif
    default:
        retval = WIFI_STACK_ERR_INTERNAL;
        break;
    }
    xSemaphoreGive(stack_context.mode_lock);
    return retval;
}

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/**
 * @brief       Initialize station
 *
 * This function assumes:
 * - wifi is initialized and not started
 */
static void stack_sta_init(void)
{
    esp_netif_t *netif;

    LOG(I, "initializing Wi-Fi station mode...");
    netif = esp_netif_create_default_wifi_sta();
    ASSERT(netif != NULL);
    (void)netif; /* Remove compiler warning when assert is disabled */
}

/**
 * @brief       Switch to Station mode
 * @return      Operation status
 * @retval      WIFI_STACK_ERR_OK - Wi-Fi was enabled
 * @retval      WIFI_STACK_ERR_ALREADY_ENABLED - Wi-Fi was already enabled
 * @retval      WIFI_STACK_ERR_INTERNAL - Other errors
 */
static int stack_sta_enable(void)
{
    esp_err_t esp_err;
    int retval;

    xSemaphoreTake(stack_context.mode_lock, portMAX_DELAY);

    /* Switch to appropriate WiFi mode */
    switch (stack_context.mode)
    {
    case WIFI_MODE_NULL:
        esp_err = esp_wifi_set_country(&s__country_defaults);
        if (esp_err != ESP_OK)
        {
            LOG(E, "internal error: esp_wifi_set_country: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
            break;
        }
        esp_err = esp_wifi_set_mode(WIFI_MODE_STA);
        if (esp_err != ESP_OK)
        {
            LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
            break;
        }
        esp_err = esp_wifi_start();
        if (esp_err != ESP_OK)
        {
            LOG(E, "internal error: esp_wifi_start: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
            break;
        }
        stack_context.mode = WIFI_MODE_STA;
        retval = WIFI_STACK_ERR_OK;
        break;
    case WIFI_MODE_STA:
        retval = WIFI_STACK_ERR_ALREADY_ENABLED;
        break;
    case WIFI_MODE_AP:
        esp_err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (esp_err != ESP_OK)
        {
            LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
            break;
        }
        stack_context.mode = WIFI_MODE_APSTA;
        retval = WIFI_STACK_ERR_OK;
        break;
    case WIFI_MODE_APSTA:
        retval = WIFI_STACK_ERR_ALREADY_ENABLED;
        break;
    default:
        retval = WIFI_STACK_ERR_INTERNAL;
        break;
    }
    xSemaphoreGive(stack_context.mode_lock);

    return retval;
}

/**
 * @brief       Switch off Station mode
 * @return      Operation status
 * @retval      WIFI_STACK_ERR_OK - Wi-Fi was disabled
 * @retval      WIFI_STACK_ERR_ALREADY_DISABLED - Wi-Fi was already disabled
 * @retval      WIFI_STACK_ERR_INTERNAL - Other errors
 */
static int stack_sta_disable(void)
{
    esp_err_t esp_err;
    int retval;

    xSemaphoreTake(stack_context.mode_lock, portMAX_DELAY);
    /* Switch to appropriate WiFi mode */
    switch (stack_context.mode)
    {
    case WIFI_MODE_NULL:
        // fallthrough
    case WIFI_MODE_AP:
        retval = WIFI_STACK_ERR_ALREADY_DISABLED;
        break;
    case WIFI_MODE_STA:
        esp_err = esp_wifi_set_mode(WIFI_MODE_NULL);
        if (esp_err != ESP_OK)
        {
            LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
            break;
        }
        esp_err = esp_wifi_stop();
        if (esp_err != ESP_OK)
        {
            LOG(E, "internal error: esp_wifi_stop: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
            break;
        }
        stack_context.mode = WIFI_MODE_NULL;
        retval = WIFI_STACK_ERR_OK;
        break;
    case WIFI_MODE_APSTA:
        esp_err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (esp_err != ESP_OK)
        {
            LOG(E, "internal error: esp_wifi_set_mode: %d", esp_err);
            retval = WIFI_STACK_ERR_INTERNAL;
            break;
        }
        stack_context.mode = WIFI_MODE_AP;
        retval = WIFI_STACK_ERR_OK;
        break;
    default:
        retval = WIFI_STACK_ERR_INTERNAL;
        break;
    }
    xSemaphoreGive(stack_context.mode_lock);

    return retval;
}

/**
 * @brief       Connect to a network
 *
 * This function assumes:
 * - we have default_wifi_sta
 * - we are in correct wifi_mode
 * - wifi is started
 * - we are not connected to an AP (disconnected)
 *
 * @retval      WIFI_STACK_ERR_OK operation completed successfully
 * @retval      WIFI_STACK_ERR_BAD_ARGS invalid password or SSID
 * @retval      WIFI_STACK_ERR_INTERNAL other internal errors
 */
static int stack_sta_connect(const char *ssid, const char *pwd, uint32_t channel)
{
    esp_err_t esp_err;

    LOG(I, "connecting to %s on channel %u", ssid, channel);
    strncpy((char *)stack_context.sta.sta.ssid, ssid, sizeof(stack_context.sta.sta.ssid));
    strncpy((char *)stack_context.sta.sta.password, pwd, sizeof(stack_context.sta.sta.password));

    // UL 220426 (Setup listen interval)
    stack_context.sta.sta.listen_interval = 10;

    if (channel != 0u)
    {
        stack_context.sta.sta.scan_method = WIFI_FAST_SCAN;
    }
    else
    {
        stack_context.sta.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    }
    stack_context.sta.sta.channel = channel;
    esp_err = esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &stack_context.sta);
    TRUE_CHECK_RETURNX(WIFI_STACK_ERR_INTERNAL, (esp_err == ESP_OK) || (esp_err == ESP_ERR_WIFI_PASSWORD));
    if (esp_err == ESP_ERR_WIFI_PASSWORD)
    {
        LOG(W, "invalid password");
        return WIFI_STACK_ERR_BAD_ARGS;
    }
    esp_err = esp_wifi_connect();
    TRUE_CHECK_RETURNX(WIFI_STACK_ERR_INTERNAL, (esp_err == ESP_OK) || (esp_err == ESP_ERR_WIFI_SSID));
    if (esp_err == ESP_ERR_WIFI_SSID)
    {
        LOG(W, "invalid SSID");
        return WIFI_STACK_ERR_BAD_ARGS;
    }
    return WIFI_STACK_ERR_OK;
}

static int stack_sta_disconnect(void)
{
    esp_err_t esp_err;

    LOG(I, "disconnecting...");
    esp_err = esp_wifi_disconnect();
    TRUE_CHECK((esp_err != ESP_ERR_WIFI_NOT_INIT) && (esp_err != ESP_ERR_WIFI_NOT_STARTED));
    if (esp_err != ESP_OK)
    {
        LOG(W, "internal WiFi error: %d", esp_err);
    }
    return esp_err == ESP_OK ? 0 : WIFI_STACK_ERR_INTERNAL;
}

static void stack_sta_get_connection_info(struct stack_connection_info *info)
{
    esp_err_t esp_err;
    wifi_ap_record_t ap_info;

    esp_err = esp_wifi_sta_get_ap_info(&ap_info);
    if ((esp_err != ESP_OK) && (esp_err != ESP_ERR_WIFI_NOT_CONNECT))
    {
        LOG(I, "disconnected while scanning");
        info->channel = 0;
        info->rssi = WIFI_LIST__RSSI_OUT_OF_RANGE;
    }
    else
    {
        info->channel = ap_info.primary;
        info->rssi = ap_info.rssi;
    }
}

static void stack_sta_scan_start(uint32_t channel, enum sta_adapter_evt_stack_scan_type scan_type)
{
    wifi_scan_config_t scan_config;
    esp_err_t esp_err;

    memset(&scan_config, 0, sizeof(scan_config));
    switch (scan_type)
    {
    case STA_ADAPTER_EVT_STACK_SCAN_TYPE_ACTIVE:
        scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        break;
    case STA_ADAPTER_EVT_STACK_SCAN_TYPE_PASSIVE:
        scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        break;
    default:
        ASSERT(false); /* force failing assert */
    }
    scan_config.channel = channel;
    esp_err = esp_wifi_scan_start(&scan_config, false);
    ZERO_CHECK(esp_err);
}

static void stack_sta_scan_stop(void)
{
    esp_err_t esp_err;

    esp_err = esp_wifi_scan_stop();
    TRUE_CHECK((esp_err == ESP_OK) || (esp_err == ESP_ERR_WIFI_NOT_STARTED))
}

#define STACK_STA_MAX_SCANNED_NETWORKS 32u
/* We don't want ap_records array on stack since it is big */
static EXT_RAM_ATTR wifi_ap_record_t ap_records[STACK_STA_MAX_SCANNED_NETWORKS];

static uint32_t stack_sta_scan_get_results(struct wifi_network__scan_result *results,
                                           uint32_t no_entries)
{
    uint16_t no_records;
    uint16_t no_copies;
    esp_err_t esp_err;

    /* Initialize all entries */
    for (uint_fast8_t i = 0u; i < no_entries; i++)
    {
        memset(results[i].ssid, 0, sizeof(results[i].ssid));
        results[i].channel = 0u;
        results[i].rssi = WIFI_LIST__RSSI_OUT_OF_RANGE;
        results[i].auth_mode = WIFI_NETWORK__AUTH_OPEN;
    }
    /* Get entries into static ap_records array */
    no_records = ELEMENTS(ap_records);
    esp_err = esp_wifi_scan_get_ap_records(&no_records, ap_records);
    if (esp_err != ESP_OK)
    {
        no_records = 0u;
    }
    /* Sort entries */
    if (no_records >= 2u)
    {
        for (uint_fast8_t i = 0u; i < no_records; i++)
        {
            for (uint_fast8_t v = 1u; v <= no_records - 1u; v++)
            {
                if (ap_records[v - 1u].rssi < ap_records[v].rssi)
                {
                    wifi_ap_record_t smaller_value;

                    /* swap entries */
                    smaller_value = ap_records[v - 1u];
                    ap_records[v - 1u] = ap_records[v];
                    ap_records[v] = smaller_value;
                }
            }
        }
    }
    /* Copy sorted entries to output results array */
    no_copies = MIN(no_entries, no_records);
    for (uint_fast8_t i = 0u; i < no_copies; i++)
    {
        strncpy(results[i].ssid, (char *)ap_records[i].ssid, sizeof(results[i].ssid));
        results[i].ssid[sizeof(results[i].ssid) - 1u] = '\0';
        results[i].channel = ap_records[i].primary;
        results[i].rssi = ap_records[i].rssi;

        /* Convert from ESP-IDF enums to station adapter enums */
        switch (ap_records[i].authmode)
        {
        case WIFI_AUTH_OPEN:
            results[i].auth_mode = WIFI_NETWORK__AUTH_OPEN;
            break;
        case WIFI_AUTH_WEP:
            results[i].auth_mode = WIFI_NETWORK__AUTH_WEP;
            break;
        case WIFI_AUTH_WPA_PSK:
            results[i].auth_mode = WIFI_NETWORK__AUTH_WPA_PSK;
            break;
        case WIFI_AUTH_WPA2_PSK:
            results[i].auth_mode = WIFI_NETWORK__AUTH_WPA2_PSK;
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            results[i].auth_mode = WIFI_NETWORK__AUTH_WPA_WPA2_PSK;
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            results[i].auth_mode = WIFI_NETWORK__AUTH_WPA2_ENTERPRISE;
            break;
        case WIFI_AUTH_WPA3_PSK:
            results[i].auth_mode = WIFI_NETWORK__AUTH_WPA3_PSK;
            break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            results[i].auth_mode = WIFI_NETWORK__AUTH_WPA2_WPA3_PSK;
            break;
        default:
            /* Unknown mode */
            results[i].auth_mode = WIFI_NETWORK__AUTH_OPEN;
            break;
        }
    }
    return no_copies;
}

static const char *__attribute__((unused)) stack_sta_err_to_string(wifi_err_reason_t wifi_err_reason)
{
    const char *hr_reason;

    switch (wifi_err_reason)
    {
    case WIFI_REASON_UNSPECIFIED:
        hr_reason = "unspecified";
        break;
    case WIFI_REASON_AUTH_EXPIRE:
        hr_reason = "auth expire";
        break;
    case WIFI_REASON_AUTH_LEAVE:
        hr_reason = "auth leave";
        break;
    case WIFI_REASON_ASSOC_EXPIRE:
        hr_reason = "assoc expire";
        break;
    case WIFI_REASON_ASSOC_TOOMANY:
        hr_reason = "assoc too many";
        break;
    case WIFI_REASON_NOT_AUTHED:
        hr_reason = "not authed";
        break;
    case WIFI_REASON_NOT_ASSOCED:
        hr_reason = "not assoced";
        break;
    case WIFI_REASON_ASSOC_LEAVE:
        hr_reason = "assoc leave";
        break;
    case WIFI_REASON_ASSOC_NOT_AUTHED:
        hr_reason = "assoc not authed";
        break;
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:
        hr_reason = "disassoc pwrcap bad";
        break;
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
        hr_reason = "disassoc supchan bad";
        break;
    case WIFI_REASON_IE_INVALID:
        hr_reason = "io invalid";
        break;
    case WIFI_REASON_MIC_FAILURE:
        hr_reason = "mic failure";
        break;
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        hr_reason = "4way handshake timeout";
        break;
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
        hr_reason = "group key update timeout";
        break;
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        hr_reason = "ie in 4way differs";
        break;
    case WIFI_REASON_GROUP_CIPHER_INVALID:
        hr_reason = "group cipher invalid";
        break;
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
        hr_reason = "pairwise cipher invalid";
        break;
    case WIFI_REASON_AKMP_INVALID:
        hr_reason = "akmp invalid";
        break;
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
        hr_reason = "unsupp rsn ie version";
        break;
    case WIFI_REASON_INVALID_RSN_IE_CAP:
        hr_reason = "invalid rsn ie cap";
        break;
    case WIFI_REASON_802_1X_AUTH_FAILED:
        hr_reason = "802 1x auth failed";
        break;
    case WIFI_REASON_CIPHER_SUITE_REJECTED:
        hr_reason = "cipher suite rejected";
        break;
    case WIFI_REASON_INVALID_PMKID:
        hr_reason = "invalid pmkid";
        break;
    case WIFI_REASON_BEACON_TIMEOUT:
        hr_reason = "beacon timeout";
        break;
    case WIFI_REASON_NO_AP_FOUND:
        hr_reason = "no AP found";
        break;
    case WIFI_REASON_AUTH_FAIL:
        hr_reason = "auth fail";
        break;
    case WIFI_REASON_ASSOC_FAIL:
        hr_reason = "assoc fail";
        break;
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
        hr_reason = "handshake timeout";
        break;
    case WIFI_REASON_CONNECTION_FAIL:
        hr_reason = "connection fail";
        break;
    case WIFI_REASON_AP_TSF_RESET:
        hr_reason = "AP TSF reset";
        break;
    default:
        hr_reason = "unknown";
        break;
    }
    return hr_reason;
}
#endif

static void stack_init(void)
{
    esp_err_t esp_err;
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();

    /*
     * NOTE:
     * esp_netif_init() and esp_event_loop_create_default() are already being
     * called in application main entry function.
     */
    esp_err = esp_event_handler_instance_register(
        /* event_base*/ WIFI_EVENT,
        /* event_id */ ESP_EVENT_ANY_ID,
        /* event_handler */ stack_wifi_event_handler,
        /* event_handler_arg */ NULL,
        /* instance */ NULL);
    ZERO_CHECK_RETURN(esp_err);
    esp_err = esp_event_handler_instance_register(
        /* event_base*/ IP_EVENT,
        /* event_id */ ESP_EVENT_ANY_ID,
        /* event_handler */ stack_ip_event_handler,
        /* event_handler_arg */ NULL,
        /* instance */ NULL);
    ZERO_CHECK_RETURN(esp_err);
    stack_context.mode_lock = xSemaphoreCreateMutex();
    ASSERT(stack_context.mode_lock != NULL);
    stack_ap_init();
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    stack_sta_init();
#endif
    LOG(I, "initializing stack...");
    esp_err = esp_wifi_init(&wifi_config);
    ZERO_CHECK_RETURN(esp_err);
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
#if CONFIG_PM_ENABLE
    // UL 220426 (Setup wifi ps)
    esp_err = esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    ZERO_CHECK_RETURN(esp_err);
#endif
#endif
}

static void stack_terminate(void)
{
    esp_err_t esp_err;

    esp_err = esp_wifi_deinit();

    if (esp_err != ESP_OK)
    {
        LOG(W, "failed to deinit Wi-Fi (%d)", esp_err);
    }
    /* According to API in esp_wifi.h and esp_wifi_default.h no need to undo stack_ap_init() work */
    /* According to API in esp_wifi.h and esp_wifi_default.h no need to undo stack_sta_init() work */
    esp_err = esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, stack_ip_event_handler);
    if (esp_err != ESP_OK)
    {
        LOG(W, "failed to deregister Wi-Fi handler (%d)", esp_err);
    }
    esp_err = esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, stack_wifi_event_handler);
    if (esp_err != ESP_OK)
    {
        LOG(W, "failed to deregister Wi-Fi handler (%d)", esp_err);
    }
}

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/* --  Wi-Fi Station Adapter State Machine -- */

/*
 * Initialize state machine data
 */
static eds__sm_action sta_adapter_init(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;
    eds__error error;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__INIT:
        /* Initialize workspace */
        ws->is_mode_change_allowed = true;
        error = eds__etimer_create(NULL, &ws->timeout);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        return eds__sm_transit_to(sm, sta_adapter_idle);
    default:
        return eds__sm_super_state(sm, &eds__sm_top_state);
    }
}

static eds__sm_action sta_adapter_service(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case STA_ADAPTER_EVT_LOCK_MODE_CHANGE:
    {
        LOG(I, "Wi-Fi mode change is locked");
        ws->is_mode_change_allowed = false;
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_UNLOCK_MODE_CHANGE:
        LOG(I, "Wi-Fi mode change is unlocked");
        ws->is_mode_change_allowed = true;
        return eds__sm_event_handled(sm);
    default:
        return eds__sm_super_state(sm, &eds__sm_top_state);
    }
}

static eds__sm_action sta_adapter_idle(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__INIT:
    {
        return eds__sm_transit_to(sm, sta_adapter_idle_waiting);
    }
    case STA_ADAPTER_EVT_CONNECT:
    {
        eds__error error;
        int stack_retval;

        error = eds__event_keep(event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        ws->connect.current = event;
        stack_retval = stack_sta_enable();
        TRUE_CHECK_RETURNX(eds__sm_event_handled(sm), stack_retval != WIFI_STACK_ERR_INTERNAL);
        if (stack_retval == WIFI_STACK_ERR_ALREADY_ENABLED)
        {
            return eds__sm_transit_to(sm, sta_adapter_connecting);
        }
        else
        {
            return eds__sm_transit_to(sm, sta_adapter_wait_start);
        }
    }
    case STA_ADAPTER_EVT_SCAN:
    {
        const struct sta_adapter_evt_scan *scan;
        int stack_retval;

        scan = eds__event_data(event);
        ws->scan = *scan;
        stack_retval = stack_sta_enable();
        TRUE_CHECK_RETURNX(eds__sm_event_handled(sm), stack_retval != WIFI_STACK_ERR_INTERNAL);
        if (stack_retval == WIFI_STACK_ERR_ALREADY_ENABLED)
        {
            return eds__sm_transit_to(sm, sta_adapter_scan_start);
        }
        else
        {
            return eds__sm_transit_to(sm, sta_adapter_scan_wait_start);
        }
    }
    case STA_ADAPTER_EVT_DISCONNECT:
    {
        /* Respond to manager that we are already disconnected and stopped. */
        eds__error error;
        eds__event *disconnected;
        eds__event *stopped;

        error = eds__event_create(STA_ADAPTER_EVT_DISCONNECTED, 0, &disconnected);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__agent_send(s__sta_manager_agent, disconnected);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__event_create(STA_ADAPTER_EVT_STOPPED, 0, &stopped);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__agent_send(s__sta_manager_agent, stopped);
        ZERO_CHECK(error);

        return eds__sm_event_handled(sm);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_service);
    }
}

static eds__sm_action sta_adapter_idle_waiting(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;

        error = eds__event_create(STA_ADAPTER_EVT_TIMEOUT, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__etimer_send_after(
            ws->timeout,
            eds__agent_from_sm(sm),
            o_event,
            IDLE_TIME_WAIT_MS);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__EXIT:
    {
        /* We are not interested in any error here */
        (void)eds__etimer_cancel(ws->timeout);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_TIMEOUT:
    {
        return eds__sm_transit_to(sm, sta_adapter_idle_dormant);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_idle);
    }
}

static eds__sm_action sta_adapter_idle_dormant(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        if (ws->is_mode_change_allowed)
        {
            /* NOTE: we don't care about error that may happen during switch off */
            (void)stack_sta_disable();
        }
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_STACK_STOPPED:
    {
        eds__error error;
        eds__event *stopped;

        error = eds__event_create(STA_ADAPTER_EVT_STOPPED, 0, &stopped);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__agent_send(s__sta_manager_agent, stopped);
        /* NOTE:
         * This SM will start before the manager SM. When that happens when we try to send the
         * message to manager SM we would get EDS__ERROR_NO_PERMISSION, which is fine in this case.
         */
        TRUE_CHECK((error == EDS__ERROR_NONE) || (error == EDS__ERROR_NO_PERMISSION));
        return eds__sm_event_handled(sm);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_idle);
    }
}

static eds__sm_action sta_adapter_wait_start(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
        LOG(I, "starting Wi-Fi adapter");
        return eds__sm_event_handled(sm);
    case STA_ADAPTER_EVT_STACK_STARTED:
    {
        eds__error error;
        eds__event *o_event;

        error = eds__event_create(STA_ADAPTER_EVT_STARTED, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        LOG(I, "Wi-Fi started adapter");

        return eds__sm_transit_to(sm, sta_adapter_connecting);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_active_connect);
    }
}

static eds__sm_action sta_adapter_active(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    return eds__sm_super_state(sm, sta_adapter_service);
}

static eds__sm_action sta_adapter_disconnecting(eds__sm *sm, void *arg, const eds__event *event)
{
    eds__error error;
    eds__event *o_event;
    (void)arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
        /* At this point we are not interested in return error, if any. */
        (void)stack_sta_disconnect();
        return eds__sm_event_handled(sm);
    case STA_ADAPTER_EVT_STACK_DISCONNECTED:
    {
        struct sta_adapter_evt_disconnected *disconnected;

        error = eds__event_create(STA_ADAPTER_EVT_DISCONNECTED, sizeof(*disconnected), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        disconnected = eds__event_put_data(o_event);
        disconnected->reason = STA_ADAPTER_EVT_DISCONN_REASON_UNKNOWN; /* TODO: Remove reason, not used anywhere */
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_transit_to(sm, sta_adapter_idle);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_active);
    }
}

static eds__sm_action sta_adapter_active_connect(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__EXIT:
    {
        eds__error error;

        error = eds__event_toss(ws->connect.current);
        ZERO_CHECK(error);
        stack_sta_scan_stop(); /* In case a scan was started we need to abort it */
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__INIT:
        return eds__sm_transit_to(sm, sta_adapter_connecting);
    case STA_ADAPTER_EVT_CONNECT:
    {
        const struct sta_adapter_evt_connect *evt_connect;
        const struct sta_adapter_evt_connect *i_current;
        bool is_equal;

        evt_connect = eds__event_data(event);
        i_current = eds__event_data(ws->connect.current);
        LOG(W, "reconnecting from %s to %s", i_current->network.ssid, evt_connect->network.ssid);
        is_equal = sta_adapter_evt_connect_ssid_is_equal(i_current, evt_connect);
        if (!is_equal)
        {
            int stack_error;
            eds__error error;

            error = eds__event_keep(event);
            /* Since we cannot keep the event it is best if we don't do anything with it
             * afterwards. Hence the return value macro call.
             */
            ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
            ws->connect.pending = event;
            stack_error = stack_sta_disconnect();
            if (stack_error)
            {
                eds__error error;

                error = eds__event_toss(ws->connect.current);
                ZERO_CHECK(error);
                ws->connect.current = ws->connect.pending;
                ws->connect.pending = NULL;
            }
            else
            {
                return eds__sm_transit_to(sm, sta_adapter_changing_network);
            }
        }
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_SCAN:
    {
        const struct sta_adapter_evt_scan *scan;

        scan = eds__event_data(event);
        stack_sta_scan_stop();
        stack_sta_scan_start(scan->channel, (enum sta_adapter_evt_stack_scan_type)scan->type);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_STACK_SCAN_DONE:
    {
        eds__error error;
        eds__event *o_event;
        struct sta_adapter_evt_scan_results *scan_results;
        uint32_t no_entries;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN_RESULTS, sizeof(*scan_results), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        scan_results = eds__event_put_data(o_event);
        no_entries = stack_sta_scan_get_results(scan_results->info.result,
                                                ELEMENTS(scan_results->info.result));
        scan_results->info.no_entries = no_entries;
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_DISCONNECT:
    {
        int stack_error = stack_sta_disconnect();
        if (stack_error == 0)
        {
            struct sta_adapter_evt_disconnected *disconnected;
            eds__error error;
            eds__event *o_event;

            error = eds__event_create(STA_ADAPTER_EVT_DISCONNECTED,
                                      sizeof(*disconnected),
                                      &o_event);
            ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
            disconnected = eds__event_put_data(o_event);
            disconnected->reason = STA_ADAPTER_EVT_DISCONN_REASON_UNKNOWN;
            error = eds__agent_send(s__sta_manager_agent, o_event);
            ZERO_CHECK(error);
            return eds__sm_transit_to(sm, sta_adapter_idle);
        }
        else
        {
            return eds__sm_transit_to(sm, sta_adapter_disconnecting);
        }
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_active);
    }
}

static eds__sm_action sta_adapter_connecting(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        const struct sta_adapter_evt_connect *connect;

        connect = eds__event_data(ws->connect.current);
        time_list_iter_init(&ws->retry, &connect->retry_sequence);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__INIT:
        return eds__sm_transit_to(sm, sta_adapter_connecting_start);
    case STA_ADAPTER_EVT_STACK_CONNECTED:
    {
        eds__error error;
        eds__event *o_event;
        const struct sta_adapter_evt_connect *connect;
        struct sta_adapter_evt_connected *evt_connected;
        struct stack_connection_info info;

        connect = eds__event_data(ws->connect.current);
        error = eds__event_create(STA_ADAPTER_EVT_CONNECTED, sizeof(*evt_connected), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        stack_sta_get_connection_info(&info);
        evt_connected = eds__event_put_data(o_event);
        evt_connected->network = connect->network;     /* Copy SSID and PWD */
        evt_connected->network.channel = info.channel; /* Update the channel info */
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        return eds__sm_transit_to(sm, sta_adapter_connected);
    }
    case STA_ADAPTER_EVT_SCAN:
    {
        /* During connecting to a network we must deny any scan requests because ESP IDF will
         * fail to initiate scan during connecting period. As a result station adapter will
         * just respond with denied event so the requester may try again later.
         */
        eds__error error;
        eds__event *o_event;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN_DENIED, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_event_handled(sm);
    }
    default:
        return eds__sm_super_state(sm, &sta_adapter_active_connect);
    }
}

static eds__sm_action sta_adapter_connected(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
        return eds__sm_event_handled(sm);
    case STA_ADAPTER_EVT_STACK_DISCONNECTED:
    {
        eds__error error;
        eds__event *o_event;
        const struct sta_adapter_evt_stack_disconnected *stack_disconnected;
        struct sta_adapter_evt_disconnected_retrying *disconnected_retrying;

        stack_disconnected = eds__event_data(event);
        ws->last_disconn_reason = stack_disconnected->reason; /* Save for later usage in other state */
        error = eds__event_create(STA_ADAPTER_EVT_DISCONNECTED_RETRYING,
                                  sizeof(*disconnected_retrying),
                                  &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        disconnected_retrying = eds__event_put_data(o_event);
        switch (stack_disconnected->reason)
        {
        case STA_ADAPTER_EVT_STACK_DISCONN_REASON_BAD_PASSWORD:
            disconnected_retrying->reason = STA_ADAPTER_EVT_DISCONN_REASON_BAD_PASSWORD;
            break;
        case STA_ADAPTER_EVT_STACK_DISCONN_REASON_NO_AP_FOUND:
            disconnected_retrying->reason = STA_ADAPTER_EVT_DISCONN_REASON_NO_AP_FOUND;
            break;
        default:
            disconnected_retrying->reason = STA_ADAPTER_EVT_DISCONN_REASON_UNKNOWN;
            break;
        }
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_transit_to(sm, sta_adapter_connecting_retry);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_active_connect);
    }
}

static eds__sm_action sta_adapter_connecting_start(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        const struct sta_adapter_evt_connect *i_event;

        i_event = eds__event_data(ws->connect.current);
        /* NOTE: We are not interested in errors at this point */
        (void)stack_sta_connect(i_event->network.ssid,
                                i_event->network.pwd,
                                i_event->network.channel);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_STACK_DISCONNECTED:
    {
        eds__error error;
        eds__event *o_event;
        const struct sta_adapter_evt_stack_disconnected *stack_disconnected;
        struct sta_adapter_evt_disconnected_retrying *disconnected_retrying;

        stack_disconnected = eds__event_data(event);
        ws->last_disconn_reason = stack_disconnected->reason; /* Save for later usage in other state */
        error = eds__event_create(STA_ADAPTER_EVT_DISCONNECTED_RETRYING,
                                  sizeof(*disconnected_retrying),
                                  &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        disconnected_retrying = eds__event_put_data(o_event);
        switch (stack_disconnected->reason)
        {
        case STA_ADAPTER_EVT_STACK_DISCONN_REASON_BAD_PASSWORD:
            disconnected_retrying->reason = STA_ADAPTER_EVT_DISCONN_REASON_BAD_PASSWORD;
            break;
        case STA_ADAPTER_EVT_STACK_DISCONN_REASON_NO_AP_FOUND:
            disconnected_retrying->reason = STA_ADAPTER_EVT_DISCONN_REASON_NO_AP_FOUND;
            break;
        default:
            disconnected_retrying->reason = STA_ADAPTER_EVT_DISCONN_REASON_UNKNOWN;
            break;
        }
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_transit_to(sm, sta_adapter_connecting_retry);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_connecting);
    }
}

static eds__sm_action sta_adapter_connecting_retry(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;
        int32_t timeout_ms;

        timeout_ms = time_list_iter_current(&ws->retry);
        if (timeout_ms < 1)
        {
            timeout_ms = 1;
        }
        error = eds__event_create(STA_ADAPTER_EVT_TIMEOUT, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__etimer_send_after(
            ws->timeout,
            eds__agent_from_sm(sm),
            o_event,
            (uint32_t)timeout_ms);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        LOG(I, "retrying in %u ms", timeout_ms);

        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__EXIT:
        /* We are not interested in any error here */
        (void)eds__etimer_cancel(ws->timeout);
        return eds__sm_event_handled(sm);
    case STA_ADAPTER_EVT_TIMEOUT:
    {
        time_list_iter_next(&ws->retry);
        if (time_list_iter_current(&ws->retry) == -1)
        {
            int stack_error = stack_sta_disconnect();
            if (stack_error == 0)
            {
                struct sta_adapter_evt_disconnected *disconnected;
                eds__error error;
                eds__event *o_event;

                error = eds__event_create(STA_ADAPTER_EVT_DISCONNECTED,
                                          sizeof(*disconnected),
                                          &o_event);
                ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
                disconnected = eds__event_put_data(o_event);
                switch (ws->last_disconn_reason)
                {
                case STA_ADAPTER_EVT_STACK_DISCONN_REASON_BAD_PASSWORD:
                    disconnected->reason = STA_ADAPTER_EVT_DISCONN_REASON_BAD_PASSWORD;
                    break;
                case STA_ADAPTER_EVT_STACK_DISCONN_REASON_NO_AP_FOUND:
                    disconnected->reason = STA_ADAPTER_EVT_DISCONN_REASON_NO_AP_FOUND;
                    break;
                default:
                    disconnected->reason = STA_ADAPTER_EVT_DISCONN_REASON_UNKNOWN;
                    break;
                }
                error = eds__agent_send(s__sta_manager_agent, o_event);
                ZERO_CHECK(error);
                return eds__sm_transit_to(sm, sta_adapter_idle);
            }
            else
            {
                return eds__sm_transit_to(sm, sta_adapter_disconnecting);
            }
        }
        else
        {
            return eds__sm_transit_to(sm, sta_adapter_connecting_start);
        }
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_connecting);
    }
}

static eds__sm_action sta_adapter_changing_network(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__EXIT:
        ws->connect.current = ws->connect.pending;
        ws->connect.pending = NULL;
        return eds__sm_event_handled(sm);
    case STA_ADAPTER_EVT_STACK_DISCONNECTED:
    {
        struct sta_adapter_evt_disconnected *disconnected;
        eds__error error;
        eds__event *o_event;

        /* TODO: What to fill in for disconnected information since I don't have current anymore */
        error = eds__event_create(STA_ADAPTER_EVT_DISCONNECTED, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        disconnected = eds__event_put_data(o_event);
        disconnected->reason = STA_ADAPTER_EVT_DISCONN_REASON_UNKNOWN;
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_transit_to(sm, sta_adapter_active_connect);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_active);
    }
}

static eds__sm_action sta_adapter_active_scan(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__INIT:
        return eds__sm_transit_to(sm, sta_adapter_scan_start);
    default:
        return eds__sm_super_state(sm, sta_adapter_active);
    }
}

static eds__sm_action sta_adapter_scan_wait_start(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;
    ;

    switch (eds__event_id(event))
    {
    case STA_ADAPTER_EVT_STACK_STARTED:
    {
        eds__error error;
        eds__event *o_event;

        error = eds__event_create(STA_ADAPTER_EVT_STARTED, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_transit_to(sm, sta_adapter_scan_start);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_active_scan);
    }
}

static eds__sm_action sta_adapter_scan_start(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_adapter_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
        stack_sta_scan_start(ws->scan.channel, (enum sta_adapter_evt_stack_scan_type)ws->scan.type);
        return eds__sm_event_handled(sm);
    case EDS__SM_EVENT__EXIT:
        stack_sta_scan_stop();
        return eds__sm_event_handled(sm);
    case STA_ADAPTER_EVT_STACK_SCAN_DONE:
    {
        eds__error error;
        eds__event *o_event;
        struct sta_adapter_evt_scan_results *scan_results;
        uint32_t no_entries;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN_RESULTS, sizeof(*scan_results), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        ASSERT(!error);
        scan_results = eds__event_put_data(o_event);
        no_entries = stack_sta_scan_get_results(scan_results->info.result,
                                                ELEMENTS(scan_results->info.result));
        scan_results->info.no_entries = no_entries;
        error = eds__agent_send(s__sta_manager_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_transit_to(sm, sta_adapter_idle);
    }
    default:
        return eds__sm_super_state(sm, sta_adapter_active_scan);
    }
}

/* -- Wi-Fi Station Manager state machine states ---- */

static eds__sm_action sta_manager_init(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__INIT:
    {
        eds__error error;

        ws->passive_scan.is_enabled = true;
        ws->is_first_connect = true;
        whitelist_init();
        time_list_iter_init(&ws->passive_scan.scan_retry, &PASSIVE_SCAN_TIME_LIST);
        time_list_iter_init(&ws->search_scan.scan_retry, &ACTIVE_SCAN_TIME_LIST);
        wifi_list__ref_init(&ws->scanned_list, &s__wifi_whitelist);
        wifi_list__ref_init(&ws->candidate_list, &s__wifi_whitelist);
        /* We are not interested in CNW return value at this moment */
        (void)whitelist_synchronize();
        error = eds__etimer_create(NULL, &ws->timeout); /* This timer is used for various timeouts */
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        return eds__sm_transit_to(sm, sta_manager_idle);
    }
    default:
        return eds__sm_super_state(sm, &eds__sm_top_state);
    }
}

static eds__sm_action sta_manager_service(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case STA_MANAGER_EVT_NETWORK_SELECT:
    {
        const struct sta_manager_evt_network_select *network_select;

        network_select = eds__event_data(event);

        if (whitelist_entry_is_used(network_select->index))
        {
            cnw_save(network_select->index);
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_SELECTED,
                                   network_select->index);
        }
        else
        {
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_NOT_SELECTED,
                                   network_select->index);
        }

        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_NETWORK_ADD:
    {
        const struct sta_manager_evt_network_add *network_add;
        struct wifi_network__network_ssid network_ssid;
        struct wifi_network__network_chn network_chn;
        int existing_index;

        network_add = eds__event_data(event);

        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        existing_index = whitelist_find(network_add->ssid);

        if (existing_index == -1)
        {
            int32_t new_index;

            new_index = whitelist_find(WHITELIST_SSID_UNUSED_ENTRY);

            if (new_index == -1)
            {
                sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_NOT_ADDED, -1);
            }
            else
            {
                union wifi_network__event_payload event_payload;

                whitelist_update_ssid(new_index, network_add->ssid);
                whitelist_update_channel(new_index, 0); /* Default channel is zero */
                sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_ADDED, new_index);
                strncpy(network_ssid.ssid, network_add->ssid, sizeof(network_ssid.ssid));
                network_ssid.index = new_index;
                event_payload.network_ssid = &network_ssid;
                sta_event_generate(WIFI_NETWORK__STA_EVENT_SSID_UPDATED, event_payload);
                network_chn.chn = 0;
                network_chn.index = new_index;
                event_payload.network_chn = &network_chn;
                sta_event_generate(WIFI_NETWORK__STA_EVENT_CHN_UPDATED, event_payload);
                sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_AVAILABLE, new_index);
                // PWD included?
                if (strlen(network_add->pwd) > 0)
                {
                    whitelist_update_pwd(new_index, network_add->pwd);
                }
            }
        }
        else
        {
            union wifi_network__event_payload event_payload;

            whitelist_update_ssid(existing_index, network_add->ssid);
            whitelist_update_channel(existing_index, 0); /* Default channel is zero */
            network_chn.chn = 0;
            network_chn.index = existing_index;
            event_payload.network_chn = &network_chn;
            sta_event_generate(WIFI_NETWORK__STA_EVENT_CHN_UPDATED, event_payload);
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_ADDED, existing_index);
        }
        TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_NETWORK_REMOVE:
    {
        const struct sta_manager_evt_network_remove *network_remove;

        network_remove = eds__event_data(event);
        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        if (whitelist_entry_is_used(network_remove->index))
        {
            whitelist_delete_at((uint32_t)network_remove->index);
            wifi_list__ref_sync(&ws->candidate_list);
            wifi_list__ref_sync(&ws->scanned_list);
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_NOT_AVAILABLE,
                                   network_remove->index);
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_REMOVED,
                                   network_remove->index);
        }
        else
        {
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_NOT_REMOVED,
                                   network_remove->index);
        }
        TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_NETWORK_SSID_UPDATE:
    {
        const struct sta_manager_evt_network_ssid *network_ssid;
        union wifi_network__event_payload event_payload;
        struct wifi_network__network_ssid o__network_ssid;

        network_ssid = eds__event_data(event);
        LOG(D, "update %u SSID %s", network_ssid->index, network_ssid->ssid);
        /* Fill in static output data */
        o__network_ssid.index = network_ssid->index;
        strncpy(o__network_ssid.ssid,
                network_ssid->ssid,
                sizeof(o__network_ssid.ssid));
        /* NULL terminate the string */
        o__network_ssid.ssid[sizeof(o__network_ssid.ssid) - 1u] = '\0';
        event_payload.network_ssid = &o__network_ssid;
        if (network_ssid->index < WIFI_LIST__SIZE)
        {
            TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
            whitelist_update_ssid(network_ssid->index, network_ssid->ssid);
            TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
            sta_event_generate(WIFI_NETWORK__STA_EVENT_SSID_UPDATED, event_payload);
        }
        else
        {
            sta_event_generate(WIFI_NETWORK__STA_EVENT_SSID_NOT_UPDATED, event_payload);
        }
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_NETWORK_PWD_UPDATE:
    {
        const struct sta_manager_evt_network_pwd *network_pwd;

        network_pwd = eds__event_data(event);
        LOG(D, "update %u PWD", network_pwd->index);
        if (network_pwd->index < WIFI_LIST__SIZE)
        {
            TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
            whitelist_update_pwd(network_pwd->index, network_pwd->pwd);
            TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_PWD_UPDATED,
                                   network_pwd->index);
        }
        else
        {
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_PWD_NOT_UPDATED,
                                   network_pwd->index);
        }
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_NETWORK_CHN_UPDATE:
    {
        const struct sta_manager_evt_network_chn *network_chn;
        struct wifi_network__network_chn network_chn_event;
        union wifi_network__event_payload event_payload;

        network_chn = eds__event_data(event);
        LOG(D, "update %u channel %u", network_chn->index, network_chn->chn);
        network_chn_event.index = network_chn->index;
        network_chn_event.chn = network_chn->chn;
        event_payload.network_chn = &network_chn_event;
        if ((network_chn->index < WIFI_LIST__SIZE) && (network_chn->chn < 16))
        {
            TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
            whitelist_update_channel(network_chn->index, network_chn->chn);
            TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
            sta_event_generate(WIFI_NETWORK__STA_EVENT_CHN_UPDATED, event_payload);
        }
        else
        {
            sta_event_generate(WIFI_NETWORK__STA_EVENT_CHN_NOT_UPDATED, event_payload);
        }
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_NETWORK_INITIALIZE:
    {
        const struct sta_manager_evt_network_initialize *network_initialize;
        int32_t existing_index;

        network_initialize = eds__event_data(event);
        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        existing_index = whitelist_find(network_initialize->ssid);

        if (existing_index >= 0)
        {
            LOG(I, "updating existing entry %d", existing_index);
            whitelist_update_ssid(existing_index, network_initialize->ssid);
            whitelist_update_pwd(existing_index, network_initialize->pwd);
            whitelist_update_channel(existing_index, 0);
        }
        else
        {
            int32_t new_index;

            new_index = whitelist_find(WHITELIST_SSID_UNUSED_ENTRY);
            if (new_index >= 0)
            {
                LOG(I, "creating a new entry %d", new_index);
                whitelist_update_ssid(new_index, network_initialize->ssid);
                whitelist_update_pwd(new_index, network_initialize->pwd);
                whitelist_update_channel(new_index, 0);
            }
            else
            {
                LOG(W, "failed to add build/config network (no space)");
            }
        }
        TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_NETWORK_SYNC:
    {
        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        for (uint_fast8_t i = 0u; i < WIFI_LIST__SIZE; i++)
        {
            if (whitelist_entry_is_used(i))
            {
                WIFI_LIST__ENTRY *entry;
                struct wifi_network__network_chn network_chn;
                struct wifi_network__network_ssid network_ssid;
                union wifi_network__event_payload event_payload;

                entry = whitelist_entry(i);
                network_ssid.index = i;
                strncpy(network_ssid.ssid,
                        entry->ssid,
                        sizeof(network_ssid.ssid));
                network_chn.index = i;
                network_chn.chn = entry->channel;
                sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_AVAILABLE, i);
                event_payload.network_ssid = &network_ssid;
                sta_event_generate(WIFI_NETWORK__STA_EVENT_SSID_UPDATED, event_payload);
                event_payload.network_chn = &network_chn;
                sta_event_generate(WIFI_NETWORK__STA_EVENT_CHN_UPDATED, event_payload);
            }
            else
            {
                sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_NOT_AVAILABLE, i);
            }
        }
        TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_SCAN_START:
    {
        eds__error error;
        eds__event *o_event;
        struct sta_adapter_evt_scan *scan;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN, sizeof(*scan), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        scan = eds__event_put_data(o_event);
        scan->channel = 0u;                            /* All channel scan */
        scan->type = STA_ADAPTER_EVT_SCAN_TYPE_ACTIVE; /* Active scan */
        error = eds__agent_send(s__sta_adapter_agent, o_event);
        ZERO_CHECK(error);
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_SCAN_STARTED, 0);
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_SCAN_STOP:
    {
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_SCAN_STOPPED, 0);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_SCAN_RESULTS:
    {
        const struct sta_adapter_evt_scan_results *scan_results;
        union wifi_network__event_payload event_payload;
        uint32_t no_copies;

        scan_results = eds__event_data(event);
        /* Copy internal structure to interface structure */
        no_copies = MIN(scan_results->info.no_entries, ELEMENTS(if_scan_results.result));
        for (uint_fast8_t i = 0u; i < no_copies; i++)
        {
            if_scan_results.result[i] = scan_results->info.result[i];
        }
        if_scan_results.no_entries = no_copies;
        event_payload.scan_results = &if_scan_results;
        sta_event_generate(WIFI_NETWORK__STA_EVENT_SCAN_RESULTS, event_payload);
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_ENABLE_BACKGROUND_SCANNER:
    {
        LOG(I, "Background passive scanner is enabled");
        ws->passive_scan.is_enabled = true;
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_DISABLE_BACKGROUND_SCANNER:
    {
        LOG(I, "Background passive scanner is disabled");
        ws->passive_scan.is_enabled = false;
        return eds__sm_event_handled(sm);
    }
    default:
        return eds__sm_super_state(sm, &eds__sm_top_state);
    }
}

static eds__sm_action sta_manager_idle(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        ws->is_first_connect = true;
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_ENABLE:
    {
        int32_t cnw;

        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        /* Get CNW ID */
        cnw = whitelist_cnw();
        if (cnw != -1)
        {
            /* Map CNW ID to an Wi-Fi network */
            WIFI_LIST__ENTRY *entry = wifi_list__entry(&s__wifi_whitelist, cnw);
            /* If candidate list is full, delete the entry with smalles RSSI first to make place
             * for CNW network.
             */
            if (wifi_list__ref_is_full(&ws->candidate_list) == true)
            {
                /* Delete last entry in the list */
                wifi_list__ref_delete_at(&ws->candidate_list, wifi_list__ref_entries(&ws->candidate_list) - 1u);
            }
            if (entry->channel != 0)
            {
                wifi_list__ref_insert_at(
                    /* list_ref */ &ws->candidate_list,
                    /* ssid */ entry->ssid,
                    /* channel */ entry->channel,
                    /* rssi */ 0, /* use 0 to force it at the top of candidate list during sorting */
                    /* index */ 0 /* insert as the first entry */);
            }
            else
            {
                TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
                return eds__sm_transit_to(sm, sta_manager_search_scan);
            }
        }
        TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
        if (wifi_list__ref_is_empty(&ws->candidate_list))
        {
            /* We don't have a Wi-Fi network canditate, we won't enable. No need to inform
             * anyone that Wi-Fi was not enabled
             */
            return eds__sm_event_handled(sm);
        }
        else
        {
            return eds__sm_transit_to(sm, sta_manager_active);
        }
    }
    case STA_MANAGER_EVT_DISABLE:
    {
        // At this point STA is already disabled, just finish the operation
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISABLED, 0);
        return eds__sm_event_handled(sm);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_service);
    }
}

static eds__sm_action sta_manager_disable_wait_adapter(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case STA_ADAPTER_EVT_DISCONNECTED:
    {
        LOG(I, "Waiting for adapter to become idle...");
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_STOPPED:
    {
        LOG(I, "Adapter stopped...");
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISABLED, 0);
        return eds__sm_transit_to(sm, sta_manager_idle);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_service);
    }
}

static eds__sm_action sta_manager_active(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_ENABLED, 0);
        return eds__sm_event_handled(sm);
    case EDS__SM_EVENT__EXIT:
    {
        eds__error error;
        eds__event *o_event;

        error = eds__event_create(STA_ADAPTER_EVT_DISCONNECT, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__agent_send(s__sta_adapter_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__INIT:
        return eds__sm_transit_to(sm, sta_manager_active_connecting);
    case STA_MANAGER_EVT_ENABLE:
        // STA is already enabled, just finish the operation
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_ENABLED, 0);
        return eds__sm_event_handled(sm);
    case STA_MANAGER_EVT_DISABLE:
        return eds__sm_transit_to(sm, sta_manager_disable_wait_adapter);
    default:
        return eds__sm_super_state(sm, sta_manager_service);
    }
}

static eds__sm_action sta_manager_active_connecting(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__INIT:
        return eds__sm_transit_to(sm, sta_manager_connecting_recent);
    default:
        return eds__sm_super_state(sm, sta_manager_active);
    }
}

static eds__sm_action sta_manager_connecting_recent(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__INIT:
    {
        return eds__sm_transit_to(sm, sta_manager_recent_loop);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_active_connecting);
    }
}

static eds__sm_action sta_manager_recent_loop(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__INIT:
    {
        return eds__sm_transit_to(sm, sta_manager_recent_connecting);
    }
    case STA_ADAPTER_EVT_DISCONNECTED_RETRYING:
    {
        const struct sta_adapter_evt_disconnected_retrying *disconnected_retrying;

        disconnected_retrying = eds__event_data(event);
        switch (disconnected_retrying->reason)
        {
        case STA_ADAPTER_EVT_DISCONN_REASON_BAD_PASSWORD:
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISCONNECTED_RETRYING,
                                   WIFI_NETWORK__DISCONNECTED_REASON_BAD_PWD);
            break;
        case STA_ADAPTER_EVT_DISCONN_REASON_NO_AP_FOUND:
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISCONNECTED_RETRYING,
                                   WIFI_NETWORK__DISCONNECTED_REASON_NO_AP);
            break;
        default:
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISCONNECTED_RETRYING,
                                   WIFI_NETWORK__DISCONNECTED_OTHER);
        }
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_DISCONNECTED:
    {
        const struct sta_adapter_evt_disconnected *disconnected;

        /* Generate external event */
        disconnected = eds__event_data(event);
        switch (disconnected->reason)
        {
        case STA_ADAPTER_EVT_DISCONN_REASON_BAD_PASSWORD:
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISCONNECTED,
                                   WIFI_NETWORK__DISCONNECTED_REASON_BAD_PWD);
            break;
        case STA_ADAPTER_EVT_DISCONN_REASON_NO_AP_FOUND:
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISCONNECTED,
                                   WIFI_NETWORK__DISCONNECTED_REASON_NO_AP);
            break;
        default:
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_DISCONNECTED,
                                   WIFI_NETWORK__DISCONNECTED_OTHER);
            break;
        }
        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        /* Delete this entry from candidate list */
        wifi_list__ref_delete_at(&ws->candidate_list, 0);
        TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
        /* Do we have a next candidate? */
        if (wifi_list__ref_is_empty(&ws->candidate_list) == false)
        {
            int32_t whitelist_index;
            const char *new_ssid;
            uint32_t new_channel;
            int32_t new_rssi;

            /* Get the next candidate data so we can generate external event */
            wifi_list__ref_peek(&ws->candidate_list, 0, &new_ssid, &new_channel, &new_rssi);
            whitelist_index = whitelist_find(new_ssid);
            cnw_save(whitelist_index);
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_SELECTED, whitelist_index);
            return eds__sm_transit_to(sm, sta_manager_recent_loop);
        }
        else
        {
            /* We don't have any candidates, do we have any entries in the whitelist? We will
             * try them all. Also check are we allowed to search for next candidate?
             */
            if (whitelist_is_empty() == false)
            {
                return eds__sm_transit_to(sm, sta_manager_search_scan);
            }
            else
            {
                /* No candidates, no whitelist entries or we are not allowed to do search scan then going to sleep */
                return eds__sm_transit_to(sm, sta_manager_disable_wait_adapter);
            }
        }
    }
    default:
        return eds__sm_super_state(sm, sta_manager_connecting_recent);
    }
}

static eds__sm_action sta_manager_recent_connecting(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        const char *new_ssid;
        uint32_t new_channel;
        int32_t new_rssi;
        WIFI_LIST__ENTRY *entry = NULL;

#if (ENABLE_VERBOSE_CANDIDATE == 1)
        {
            for (uint32_t i = 0u; i < wifi_list__ref_entries(&ws->candidate_list); i++)
            {
                wifi_list__ref_peek(&ws->candidate_list, i, &new_ssid, &new_channel, &new_rssi);
                LOG(I, "candidate %u: %s@%u with RSSI %d", i, new_ssid, new_channel, new_rssi);
            }
        }
#endif
        new_ssid = NULL; /* Set pointer to NULL so we can see if candidate list is empty or not */
        wifi_list__ref_peek(&ws->candidate_list, 0, &new_ssid, &new_channel, &new_rssi);
        if (new_ssid != NULL)
        {
            int32_t entry_index;

            entry_index = whitelist_find(new_ssid);
            /* Do not check entry_index for -1 since the code above already did checked it. */
            entry = whitelist_entry(entry_index);
        }
        if (entry != NULL)
        {
            eds__error error;
            eds__event *o_event;
            struct sta_adapter_evt_connect *evt_connect;

            error = eds__event_create(STA_ADAPTER_EVT_CONNECT, sizeof(*evt_connect), &o_event);
            ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
            evt_connect = eds__event_put_data(o_event);
            strncpy(evt_connect->network.ssid, new_ssid, sizeof(evt_connect->network.ssid));
            strncpy(evt_connect->network.pwd, entry->pwd, sizeof(evt_connect->network.pwd));
            evt_connect->network.channel = new_channel;

            if (new_channel == 0)
            {
                evt_connect->retry_sequence = UNKNOWN_CHANNEL_TIME_LIST;
            }
            else
            {
                evt_connect->retry_sequence = KNOWN_CHANNEL_TIME_LIST;
            }
            error = eds__agent_send(s__sta_adapter_agent, o_event);
            ZERO_CHECK(error);
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_CONNECTING,
                                   whitelist_find(entry->ssid));
        }
        else
        {
            /* We didn't get a candidate, since it was deleted from whitelist during our effort
             * to connect.
             */
            eds__error error;
            eds__event *o_event;

            LOG(W, "tried to connect to a deleted network");
            /* Setup a timeout of 100ms so we can get out of this state */
            error = eds__event_create(STA_MANAGER_EVT_TIMEOUT, 0, &o_event);
            ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
            error = eds__etimer_send_after(ws->timeout,
                                           eds__agent_from_sm(sm),
                                           o_event,
                                           100);
            ZERO_CHECK(error);
        }

        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__EXIT:
    {
        /* We are not interested in any error here */
        (void)eds__etimer_cancel(ws->timeout);
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_TIMEOUT:
    {
        /* This is a kludge, we use timeout to force us to exit from this state in case somebody
         * has deleted whitelist entry while we were connecting.
         */
        if (whitelist_is_empty())
        {
            return eds__sm_transit_to(sm, sta_manager_disable_wait_adapter);
        }
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_CONNECTED:
    {
        const struct sta_adapter_evt_connected *evt_connected;
        int whitelist_index;

        evt_connected = eds__event_data(event);
        whitelist_index = whitelist_find(evt_connected->network.ssid);
        /* Maybe someone deleted the network in the mean time? */
        if (whitelist_index >= 0)
        {
            if (cnw_load() != whitelist_index)
            {
                cnw_save(whitelist_index);
            }
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_CONNECTED, whitelist_index);
        }
        else
        {
            sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_CONNECTED, -1);
        }
        return eds__sm_transit_to(sm, sta_manager_recent_connected);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_recent_loop);
    }
}

static eds__sm_action sta_manager_search_scan(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        // Note: The following code (esp_wifi_...) is not in the proper layer. This code belongs to adapter state machine,
        // not to manager state machine.
        wifi_country_t country = {
            .cc = "CN",
            .schan = 1,
            .nchan = 13,
            .policy = WIFI_COUNTRY_POLICY_MANUAL,
        };
        esp_err_t esp_err = esp_wifi_set_country(&country);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), esp_err);
        time_list_iter_reset(&ws->search_scan.scan_retry);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__EXIT:
    {
        // Note: The following code (esp_wifi_...) is not in the proper layer. This code belongs to adapter state machine,
        // not to manager state machine.
        wifi_country_t country = {
            .cc = "CN",
            .schan = 1,
            .nchan = 13,
            .policy = WIFI_COUNTRY_POLICY_AUTO,
        };
        esp_err_t esp_err = esp_wifi_set_country(&country);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), esp_err);

        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__INIT:
    {
        return eds__sm_transit_to(sm, sta_manager_search_scan_start);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_connecting_recent);
    }
}

static eds__sm_action sta_manager_search_scan_start(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        wifi_list__ref_clear_all(&ws->scanned_list);
        ws->search_scan.current_index = 0u;
        ws->search_scan.scan_list = ACTIVE_SCAN_CHANNEL_LIST;
        ws->search_scan.scan_list_elements = ELEMENTS(ACTIVE_SCAN_CHANNEL_LIST);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__INIT:
    {
        return eds__sm_transit_to(sm, sta_manager_search_scan_do);
    }
    case STA_MANAGER_EVT_SCAN_START:
    {
        /* This event was generated by external code when someone wants to start an active scan.
         */
        return eds__sm_transit_to(sm, sta_manager_search_scan_wait);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_search_scan);
    }
}

static eds__sm_action sta_manager_search_scan_do(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;
        struct sta_adapter_evt_scan *scan;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN, sizeof(*scan), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        scan = eds__event_put_data(o_event);
        scan->channel = ws->search_scan.scan_list[ws->search_scan.current_index];
        scan->type = STA_ADAPTER_EVT_SCAN_TYPE_ACTIVE;
        error = eds__agent_send(s__sta_adapter_agent, o_event);
        LOG(I, "scanning channel %u", scan->channel);
        ZERO_CHECK(error);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_SCAN_RESULTS:
    {
        /* This case processes scan results from active scan request made in entry section of
         * this state.
         */
        const struct sta_adapter_evt_scan_results *scan_results;
        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        scan_results = eds__event_data(event);
        for (uint_fast8_t i = 0u; i < scan_results->info.no_entries; i++)
        {
            int32_t entry_index;

            entry_index = wifi_list__ref_insert_sorted_rssi(
                &ws->scanned_list,
                scan_results->info.result[i].ssid,
                scan_results->info.result[i].channel,
                scan_results->info.result[i].rssi);

#if (ENABLE_VERBOSE_CANDIDATE == 1)
            if (entry_index != -1)
            {
                LOG(D, "inserted %s@%u with RSSI %d at position %d",
                    scan_results->info.result[i].ssid,
                    scan_results->info.result[i].channel,
                    scan_results->info.result[i].rssi,
                    entry_index);
            }
#else
            (void)entry_index; /* Remove compiler warnings */
#endif /* (ENABLE_VERBOSE_CANDIDATE == 1) */
        }
        ws->search_scan.current_index++;
        if (ws->search_scan.current_index == ws->search_scan.scan_list_elements)
        {
            ws->search_scan.current_index = 0u;
            if (ws->is_first_connect)
            {
                /* Remove networks in which we are not interested in */
                int32_t cnw;
                WIFI_LIST__ENTRY *cnw_entry;

                cnw = cnw_load(); /* CNW is always valid at this point */
                cnw_entry = whitelist_entry(cnw);
#if (ENABLE_VERBOSE_CANDIDATE == 1)
                LOG(D, "searching for %s network", cnw_entry->ssid);
#endif /* (ENABLE_VERBOSE_CANDIDATE == 1) */
                for (uint32_t i = 0u; i < wifi_list__ref_entries(&ws->scanned_list); i++)
                {
                    const char *new_ssid;
                    uint32_t new_channel;
                    int32_t new_rssi;
                    wifi_list__ref_peek(&ws->scanned_list, i, &new_ssid, &new_channel, &new_rssi);

                    if (strncmp(cnw_entry->ssid, new_ssid, sizeof(cnw_entry->ssid)) != 0)
                    {
#if (ENABLE_VERBOSE_CANDIDATE == 1)
                        LOG(D, "not interested in %s@%u", new_ssid, new_channel);
#endif /* (ENABLE_VERBOSE_CANDIDATE == 1) */
                        /* NOTE:
                         * 1. Unsigned integer underflow is well defined so we are fine about it
                         *    using it here.
                         * 2. Once an entry is deleted in the list, it will take the same index as
                         *    the current one, hence we need to go one step backwards.
                         */
                        wifi_list__ref_delete_at(&ws->scanned_list, i--);
                    }
#if (ENABLE_VERBOSE_CANDIDATE == 1)
                    else
                    {
                        LOG(D, "interested in %s@%u", new_ssid, new_channel);
                    }
#endif /* (ENABLE_VERBOSE_CANDIDATE == 1) */
                }
            }
            /* Save scanned list to candidate list */
            wifi_list__ref_copy(&ws->scanned_list, &ws->candidate_list);
            TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
            if (wifi_list__ref_is_empty(&ws->candidate_list))
            {
                if (ws->search_scan.scan_list == ACTIVE_SCAN_CHANNEL_LIST)
                {
                    LOG(I, "using alternate scan list");
                    ws->search_scan.scan_list = ACTIVE_SCAN_CHANNEL_LIST_ALTERNATE;
                    ws->search_scan.scan_list_elements = ELEMENTS(ACTIVE_SCAN_CHANNEL_LIST_ALTERNATE);
                    return eds__sm_transit_to(sm, sta_manager_search_scan_do);
                }
                else
                {
                    ws->search_scan.scan_list = ACTIVE_SCAN_CHANNEL_LIST;
                    ws->search_scan.scan_list_elements = ELEMENTS(ACTIVE_SCAN_CHANNEL_LIST);
                    if (ws->is_first_connect)
                    {
                        /* Since this was first connect attempt and we failed to connect to CNW
                         * try searching again for any network
                         */
                        ws->is_first_connect = false;
                        return eds__sm_transit_to(sm, sta_manager_search_scan_do);
                    }
                    else
                    {
                        /* We still don't have candidates, cooldown a bit and retry again. */
                        return eds__sm_transit_to(sm, sta_manager_search_scan_cooldown);
                    }
                }
            }
            else
            {
                /* We have some candidates, go to connecting. */
                return eds__sm_transit_to(sm, sta_manager_active_connecting);
            }
        }
        else
        {
            TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
            /* We still haven't completed scan channels, go to next channel scan */
            return eds__sm_transit_to(sm, sta_manager_search_scan_do);
        }
    }
    default:
        return eds__sm_super_state(sm, sta_manager_search_scan_start);
    }
}

static eds__sm_action sta_manager_search_scan_cooldown(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;
        int32_t timeout_ms;

        /* Calculate next scan period */
        time_list_iter_next(&ws->search_scan.scan_retry);
        timeout_ms = time_list_iter_current(&ws->search_scan.scan_retry);

        if (timeout_ms == -1)
        {
            /* We went through the whole time list, now switch over to constant period scanning */
            timeout_ms = ACTIVE_SCAN_PERIOD_MAX_MS;
        }
        error = eds__event_create(STA_MANAGER_EVT_TIMEOUT, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__etimer_send_after(ws->timeout,
                                       eds__agent_from_sm(sm),
                                       o_event,
                                       timeout_ms);
        ZERO_CHECK(error);
        LOG(W, "No network candidates found, retrying in %" PRIi32 " ms", timeout_ms);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__EXIT:
    {
        /* We are not interested in any error here */
        (void)eds__etimer_cancel(ws->timeout);
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_TIMEOUT:
    {
        return eds__sm_transit_to(sm, sta_manager_search_scan_start);
    }
    case STA_MANAGER_EVT_SCAN_START:
    {
        return eds__sm_transit_to(sm, sta_manager_search_active_scan);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_search_scan);
    }
}

static eds__sm_action sta_manager_search_scan_wait(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case STA_ADAPTER_EVT_SCAN_RESULTS:
    {
        /* Throw away this event data, this scan contains data from a previous active scan
         * request. We want to proceed to sta_manager_active_scan to start an actual active
         * scan
         */
        return eds__sm_transit_to(sm, sta_manager_search_active_scan);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_search_scan);
    }
}

static eds__sm_action sta_manager_search_active_scan(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;
        struct sta_adapter_evt_scan *scan;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN, sizeof(*scan), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        scan = eds__event_put_data(o_event);
        scan->channel = 0u;                            /* All channel scan */
        scan->type = STA_ADAPTER_EVT_SCAN_TYPE_ACTIVE; /* Active scan */
        error = eds__agent_send(s__sta_adapter_agent, o_event);
        ZERO_CHECK(error);
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_SCAN_STARTED, 0);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_SCAN_RESULTS:
    {
        const struct sta_adapter_evt_scan_results *scan_results;
        union wifi_network__event_payload event_payload;
        uint32_t no_copies;

        scan_results = eds__event_data(event);
        /* Copy internal structure to interface structure */
        no_copies = MIN(scan_results->info.no_entries, ELEMENTS(if_scan_results.result));
        for (uint_fast8_t i = 0u; i < no_copies; i++)
        {
            if_scan_results.result[i] = scan_results->info.result[i];
        }
        if_scan_results.no_entries = no_copies;
        event_payload.scan_results = &if_scan_results;
        sta_event_generate(WIFI_NETWORK__STA_EVENT_SCAN_RESULTS, event_payload);
        time_list_iter_reset(&ws->search_scan.scan_retry);
        return eds__sm_transit_to(sm, sta_manager_search_scan_start);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_search_scan);
    }
}

static eds__sm_action sta_manager_recent_connected(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        time_list_iter_reset(&ws->passive_scan.scan_retry);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__INIT:
    {
        return eds__sm_transit_to(sm, sta_manager_passive_scan_start);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_recent_loop);
    }
}

static eds__sm_action sta_manager_passive_scan_start(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        wifi_list__ref_clear_all(&ws->scanned_list);
        ws->passive_scan.current_index = 0u;
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__INIT:
    {
        struct stack_connection_info connection_info;
        stack_sta_get_connection_info(&connection_info);
        if ((ws->passive_scan.is_enabled == false) || (connection_info.rssi == WIFI_LIST__RSSI_OUT_OF_RANGE))
        {
            /* The connection dropped or we are disabled */
            return eds__sm_transit_to(sm, sta_manager_passive_scan_cooldown);
        }
        if (connection_info.rssi < CONNECTION_RSSI_BAD_THRESHOLD)
        {
            return eds__sm_transit_to(sm, sta_manager_passive_scan_do);
        }
        else
        {
            return eds__sm_transit_to(sm, sta_manager_passive_scan_cooldown);
        }
    }
    case STA_MANAGER_EVT_SCAN_START:
    {
        /* This event was generated by external code when someone wants to start an active scan.
         */
        return eds__sm_transit_to(sm, sta_manager_passive_scan_wait);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_recent_connected);
    }
}

/*
 * Automatically start passive scanning of a single channel. After scanning of a channel switch
 * over to next channel in PASSIVE_SCAN_CHANNEL_LIST list. Once all channels are scanned go to
 * sta_manager_passive_scan_cooldown where the SM will wait for predefined time (see scan_retry in
 * workspace structure).
 */
static eds__sm_action sta_manager_passive_scan_do(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;
        struct sta_adapter_evt_scan *scan;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN, sizeof(*scan), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        scan = eds__event_put_data(o_event);
        scan->channel = PASSIVE_SCAN_CHANNEL_LIST[ws->passive_scan.current_index];
        scan->type = STA_ADAPTER_EVT_SCAN_TYPE_PASSIVE;
        error = eds__agent_send(s__sta_adapter_agent, o_event);
        ZERO_CHECK(error);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_SCAN_RESULTS:
    {
        /* This case processes scan results from passive scan request made in entry section of
         * this state.
         */
        const struct sta_adapter_evt_scan_results *scan_results;

        scan_results = eds__event_data(event);
        TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
        for (uint_fast8_t i = 0u; i < scan_results->info.no_entries; i++)
        {
            (void)wifi_list__ref_insert_sorted_rssi(
                &ws->scanned_list,
                scan_results->info.result[i].ssid,
                scan_results->info.result[i].channel,
                scan_results->info.result[i].rssi);
        }
        ws->passive_scan.current_index++;
        if (ws->passive_scan.current_index == ELEMENTS(PASSIVE_SCAN_CHANNEL_LIST))
        {
            /* Save scanned list to candidate list */
            wifi_list__ref_copy(&ws->scanned_list, &ws->candidate_list);
#if (ENABLE_VERBOSE_CANDIDATE == 1)
            {
                const char *new_ssid;
                uint32_t new_channel;
                int32_t new_rssi;
                for (uint32_t i = 0u; i < wifi_list__ref_entries(&ws->candidate_list); i++)
                {
                    wifi_list__ref_peek(&ws->candidate_list, i, &new_ssid, &new_channel, &new_rssi);
                    LOG(I, "candidate %u: %s@%u with RSSI %d", i, new_ssid, new_channel, new_rssi);
                }
            }
#endif
            TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
            return eds__sm_transit_to(sm, sta_manager_passive_scan_cooldown);
        }
        else
        {
            TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
            return eds__sm_transit_to(sm, sta_manager_passive_scan_do);
        }
    }
    default:
        return eds__sm_super_state(sm, sta_manager_passive_scan_start);
    }
}

/*
 * Wait a bit between next scan cycle. This wait is defined by time list. Once the whole time list
 * is iterated, the SM will go into constant passive scan period.
 */
static eds__sm_action sta_manager_passive_scan_cooldown(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;
        int32_t timeout_ms;

        /* Calculate next scan period */
        time_list_iter_next(&ws->passive_scan.scan_retry);
        timeout_ms = time_list_iter_current(&ws->passive_scan.scan_retry);

        if (timeout_ms == -1)
        {
            /* We went through the whole time list, now switch over to constant period scanning */
            timeout_ms = PASSIVE_SCAN_PERIOD_MAX_MS;
        }
        error = eds__event_create(STA_MANAGER_EVT_TIMEOUT, 0, &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        error = eds__etimer_send_after(ws->timeout,
                                       eds__agent_from_sm(sm),
                                       o_event,
                                       timeout_ms);
        ZERO_CHECK(error);
        return eds__sm_event_handled(sm);
    }
    case EDS__SM_EVENT__EXIT:
    {
        /* We are not interested in any error here */
        (void)eds__etimer_cancel(ws->timeout);
        return eds__sm_event_handled(sm);
    }
    case STA_MANAGER_EVT_TIMEOUT:
    {
        return eds__sm_transit_to(sm, sta_manager_passive_scan_start);
    }
    case STA_MANAGER_EVT_SCAN_START:
    {
        return eds__sm_transit_to(sm, sta_manager_active_scan);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_recent_connected);
    }
}

/*
 * Wait for passive scan results and throw them away; proceed to start active scanning.
 */
static eds__sm_action sta_manager_passive_scan_wait(eds__sm *sm, void *arg, const eds__event *event)
{
    (void)arg;

    switch (eds__event_id(event))
    {
    case STA_ADAPTER_EVT_SCAN_RESULTS:
    {
        /* Throw away this event data, this scan contains data from a previous passive scan
         * request. We want to proceed to sta_manager_active_scan to start an actual active
         * scan
         */
        return eds__sm_transit_to(sm, sta_manager_active_scan);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_recent_connected);
    }
}

/*
 * Start active scan, after scan results have been processed proceed to passive scanning
 */
static eds__sm_action sta_manager_active_scan(eds__sm *sm, void *arg, const eds__event *event)
{
    struct sta_manager_workspace *ws = arg;

    switch (eds__event_id(event))
    {
    case EDS__SM_EVENT__ENTER:
    {
        eds__error error;
        eds__event *o_event;
        struct sta_adapter_evt_scan *scan;

        error = eds__event_create(STA_ADAPTER_EVT_SCAN, sizeof(*scan), &o_event);
        ZERO_CHECK_RETURNX(eds__sm_event_handled(sm), error);
        scan = eds__event_put_data(o_event);
        scan->channel = 0u;                            /* All channel scan */
        scan->type = STA_ADAPTER_EVT_SCAN_TYPE_ACTIVE; /* Active scan */
        error = eds__agent_send(s__sta_adapter_agent, o_event);
        ZERO_CHECK(error);
        sta_event_generate_i32(WIFI_NETWORK__STA_EVENT_SCAN_STARTED, 0);
        return eds__sm_event_handled(sm);
    }
    case STA_ADAPTER_EVT_SCAN_RESULTS:
    {
        const struct sta_adapter_evt_scan_results *scan_results;
        union wifi_network__event_payload event_payload;
        uint32_t no_copies;

        scan_results = eds__event_data(event);
        /* Copy internal structure to interface structure */
        no_copies = MIN(scan_results->info.no_entries, ELEMENTS(if_scan_results.result));
        for (uint_fast8_t i = 0u; i < no_copies; i++)
        {
            if_scan_results.result[i] = scan_results->info.result[i];
        }
        if_scan_results.no_entries = no_copies;
        event_payload.scan_results = &if_scan_results;
        sta_event_generate(WIFI_NETWORK__STA_EVENT_SCAN_RESULTS, event_payload);
        time_list_iter_reset(&ws->passive_scan.scan_retry);
        return eds__sm_transit_to(sm, sta_manager_passive_scan_start);
    }
    default:
        return eds__sm_super_state(sm, sta_manager_recent_connected);
    }
}

/* End of sta_manager state machine */

static bool sta_adapter_evt_connect_ssid_is_equal(
    const struct sta_adapter_evt_connect *self,
    const struct sta_adapter_evt_connect *other)
{
    if (strncmp(self->network.ssid, other->network.ssid, WIFI_LIST__SSID_LENGTH) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif

void wifi_network__init(void)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    LOG(I, "Initializing EDS");
    main_eds_init_sys_network();
    LOG(I, "Initializing WiFi");

#if defined(WIFI_NETWORK_STA) || defined(WIFI_NETWORK_AP)
    static EXT_RAM_ATTR struct sta_adapter_workspace sta_adapter_workspace;
    static EXT_RAM_ATTR struct sta_manager_workspace sta_manager_workspace;
    eds__error error;
    esp_err_t esp_err;

    stack_init();
    error = eds__agent_create(sta_adapter_init,
                              &sta_adapter_workspace,
                              NULL,
                              &s__sta_adapter_agent);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_create(sta_manager_init,
                              &sta_manager_workspace,
                              NULL,
                              &s__sta_manager_agent);
    ZERO_CHECK_RETURN(error);
    error = eds__network_add_agent(g__main_sys_network, s__sta_adapter_agent);
    ZERO_CHECK(error);
    error = eds__network_add_agent(g__main_sys_network, s__sta_manager_agent);
    ZERO_CHECK(error);
    esp_err = esp_register_shutdown_handler(wifi_network__terminate);
    ZERO_CHECK(esp_err);
#endif /* defined(WIFI_NETWORK_STA) || defined(WIFI_NETWORK_AP) */
#else
    LOG(I, "Initializing WiFi");

    stack_init();
    ZERO_CHECK(esp_register_shutdown_handler(wifi_network__terminate));
#endif
}

void wifi_network__terminate(void)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    eds__error error;
    eds__agent *sta_manager_agent = s__sta_manager_agent;
    eds__agent *sta_adapter_agent = s__sta_adapter_agent;

    error = eds__network_remove_agent(g__main_sys_network, sta_manager_agent);
    ZERO_CHECK(error);
    error = eds__network_remove_agent(g__main_sys_network, sta_adapter_agent);
    ZERO_CHECK(error);
    s__sta_manager_agent = NULL; /* From this point, nobody can access the manager state machine */
    s__sta_adapter_agent = NULL; /* From this point, nobody can access the adapter state machine */
    error = eds__agent_delete(sta_manager_agent);
    ZERO_CHECK(error);
    error = eds__agent_delete(sta_adapter_agent);
    ZERO_CHECK(error);
    (void)stack_sta_disconnect(); /* Unconditionally disconnect from station. We don't care for errors. */
    (void)stack_sta_disable();    /* Unconditionally disable station mode. We don't care for errors. */
#endif
    (void)stack_ap_disable();     /* Unconditionally disable AP mode. We don't care for errors. */
    stack_terminate();
}

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/* Wi-Fi network interface functions */

int wifi_network__register_handler(wifi_network__event_handler *handler)
{
    int retval = WIFI_NETWORK__ERROR;
    if (handler != NULL)
    {
        for (uint32_t i = 0u; i < ELEMENTS(s__event_handlers); i++)
        {
            if (s__event_handlers[i] == NULL)
            {
                s__event_handlers[i] = handler;
                retval = WIFI_NETWORK__OK;
                break;
            }
        }
    }
    return retval;
}

void wifi_network__network_select(int32_t index)
{
    eds__error error;
    eds__event *event;
    struct sta_manager_evt_network_select *network_select;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_SELECT, sizeof(*network_select), &event);
    ZERO_CHECK_RETURN(error);
    network_select = eds__event_put_data(event);
    network_select->index = index;
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
    /* In case we are connected we need to reconnect */
    error = eds__event_create(STA_MANAGER_EVT_RECONNECT, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__network_add(const char *ssid)
{
    eds__error error;
    eds__event *event;
    struct sta_manager_evt_network_add *network_add;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_ADD, sizeof(*network_add), &event);
    ZERO_CHECK_RETURN(error);
    network_add = eds__event_put_data(event);

    // Check if this is an extended command with pwd included, <ssid>\<pwd>
    network_add->pwd[0] = '\0';
    char *pwdPtr = strchr(ssid, '\\');
    if (NULL != pwdPtr)
    {
        // Found a '\'.
        char *token;
        char *rest = (char *)ssid;
        token = strtok_r(rest, "\\", &rest);
        LOG(I, "Found ssid: %s", token);
        strncpy(network_add->ssid, token, sizeof(network_add->ssid));
        token = strtok_r(rest, "\\", &rest);
        LOG(I, "Found pwd");
        strncpy(network_add->pwd, token, sizeof(network_add->pwd));
        // Make sure to null terminate
        network_add->pwd[sizeof(network_add->pwd) - 1u] = '\0';
    }
    else
    {
        strncpy(network_add->ssid, ssid, sizeof(network_add->ssid));
    }
    /* Null terminate the string in case `ssid` is bigger than what event can hold */
    network_add->ssid[sizeof(network_add->ssid) - 1u] = '\0';
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__network_remove(int32_t index)
{
    eds__error error;
    eds__event *event;
    struct sta_manager_evt_network_remove *network_remove;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_REMOVE, sizeof(*network_remove), &event);
    ZERO_CHECK_RETURN(error);
    network_remove = eds__event_put_data(event);
    network_remove->index = index;
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__network_ssid(int32_t index, const char *ssid)
{
    eds__error error;
    eds__event *event;
    struct sta_manager_evt_network_ssid *network_ssid;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_SSID_UPDATE, sizeof(*network_ssid), &event);
    ZERO_CHECK_RETURN(error);
    network_ssid = eds__event_put_data(event);
    network_ssid->index = index;
    strncpy(network_ssid->ssid, ssid, sizeof(network_ssid->ssid));
    /* Null terminate the string in case `ssid` is bigger than what event can hold */
    network_ssid->ssid[sizeof(network_ssid->ssid) - 1u] = '\0';
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__network_initialize(const char *ssid, const char *pwd)
{
    eds__error error;
    eds__event *event;
    struct sta_manager_evt_network_initialize *network_init;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_INITIALIZE, sizeof(*network_init), &event);
    ZERO_CHECK_RETURN(error);
    network_init = eds__event_put_data(event);
    strncpy(network_init->ssid, ssid, sizeof(network_init->ssid));
    strncpy(network_init->pwd, pwd, sizeof(network_init->pwd));
    /* Null terminate the string in case `ssid` is bigger than what event can hold */
    network_init->ssid[sizeof(network_init->ssid) - 1u] = '\0';
    network_init->pwd[sizeof(network_init->pwd) - 1u] = '\0';
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__network_pwd(int32_t index, const char *pwd)
{
    eds__error error;
    eds__event *event;
    struct sta_manager_evt_network_pwd *network_pwd;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_PWD_UPDATE, sizeof(*network_pwd), &event);
    ZERO_CHECK_RETURN(error);
    network_pwd = eds__event_put_data(event);
    network_pwd->index = index;
    strncpy(network_pwd->pwd, pwd, sizeof(network_pwd->pwd));
    /* Null terminate the string in case `pwd` is bigger than what event can hold */
    network_pwd->pwd[sizeof(network_pwd->pwd) - 1u] = '\0';
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__network_chn(int32_t index, uint32_t chn)
{
    eds__error error;
    eds__event *event;
    struct sta_manager_evt_network_chn *network_chn;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_CHN_UPDATE, sizeof(*network_chn), &event);
    ZERO_CHECK_RETURN(error);
    network_chn = eds__event_put_data(event);
    network_chn->index = index;
    network_chn->chn = chn;
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

int wifi_network__network_find_ssid(const char *ssid)
{
    TRUE_CHECK(xSemaphoreTake(l_wifi_whitelist_mutex, portMAX_DELAY));
    int existing_index = whitelist_find(ssid);
    TRUE_CHECK(xSemaphoreGive(l_wifi_whitelist_mutex));
    return existing_index;
}

void wifi_network__enable(void)
{
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_MANAGER_EVT_ENABLE, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__disable(void)
{
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_MANAGER_EVT_DISABLE, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}

void wifi_network__network_sync(void)
{
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_MANAGER_EVT_NETWORK_SYNC, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}
#endif

void wifi_network__enable_background_scanner(void)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_MANAGER_EVT_ENABLE_BACKGROUND_SCANNER, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
#endif
}

void wifi_network__disable_background_scanner(void)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_MANAGER_EVT_DISABLE_BACKGROUND_SCANNER, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
#endif
}

void wifi_network__lock_mode(void)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_ADAPTER_EVT_LOCK_MODE_CHANGE, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_adapter_agent, event);
    ZERO_CHECK(error);
#endif
}

void wifi_network__unlock_mode(void)
{
#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_ADAPTER_EVT_UNLOCK_MODE_CHANGE, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_adapter_agent, event);
    ZERO_CHECK(error);
#endif
}

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
void wifi_network__scan_start(void)
{
    eds__error error;
    eds__event *event;

    error = eds__event_create(STA_MANAGER_EVT_SCAN_START, 0, &event);
    ZERO_CHECK_RETURN(error);
    error = eds__agent_send(s__sta_manager_agent, event);
    ZERO_CHECK(error);
}
#endif

int wifi_network__ap_register_handler(wifi_network__ap_event_handler *handler)
{
    int retval = WIFI_NETWORK__ERROR;

    if (handler != NULL)
    {
        for (size_t i = 0; i < ELEMENTS(s__ap_event_handlers); i++)
        {
            if (s__ap_event_handlers[i] == NULL)
            {
                s__ap_event_handlers[i] = handler;
                retval = WIFI_NETWORK__OK;
                break;
            }
        }
    }

    return retval;
}

void wifi_network__ap_enable(void)
{
    int stack_ret = stack_ap_enable();
    if (WIFI_STACK_ERR_OK == stack_ret)
    {
        ap_event_generate(WIFI_NETWORK__AP_EVENT_STARTING);
    }
    else
    {
        LOG(W, "failed to enable AP: %d", stack_ret);
    }
}

void wifi_network__ap_disable(void)
{
    ap_event_generate(WIFI_NETWORK__AP_EVENT_STOPPING);
    stack_ap_disable();
}

void wifi_network__ap_update_settings(const char *ssid, const char *pwd)
{
    strncpy((char *)stack_context.ap.ap.ssid, ssid, sizeof(stack_context.ap.ap.ssid));
    strncpy((char *)stack_context.ap.ap.password, pwd, sizeof(stack_context.ap.ap.password));
}

void wifi_network__ap_get_ssid(char *ssid)
{
    strncpy(ssid, (char *)stack_context.ap.ap.ssid, WIFI_NETWORK__SSID_LENGTH);
    /* NULL terminate the string if it was 32 bytes long */
    ssid[WIFI_NETWORK__SSID_LENGTH] = '\0';
}
