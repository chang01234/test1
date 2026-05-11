/**
 * @file wifi_network.h
 * @brief Wifi module
 *
 * WiFi and network management.
 *
 * @defgroup    wifi Wi-Fi management
 * @brief       Wi-Fi management interface
 * @{
 */

#ifndef WIFI_NETWORK_H_
#define WIFI_NETWORK_H_

#include <stdbool.h>
#include <stdint.h>

#include "esp_wifi_types.h"

/**
 * @brief   Length of SSID string
 *
 * This macro should match corresponding macro in wifi_list module. This macro should add +1 to the
 * value of length which is stored by ESP IDF framework since we also want to store NULL terminating
 * character.
 */
#define WIFI_NETWORK__SSID_LENGTH (MAX_SSID_LEN + 1u)

/**
 * @brief   Length of password string
 *
 * As for @ref WIFI_NETWORK__SSID_LENGTH this macro calculates the length of password taking into
 * account null terminating character.
 */
#define WIFI_NETWORK__PASSWORD_LENGTH (MAX_PASSPHRASE_LEN + 1u)

/**
 * @brief   Maximum number of managed networks
 *
 * This macro should match corresponding macro in wifi_list module.
 */
#define WIFI_NETWORK__NO_OF_NETWORKS 8

/**
 * @brief   Number of entries in scanned networks results structure
 */
#define WIFI_NETWORK__NO_OF_SCANNED_NETW 10

/**
 * @brief   Return codes, operation status OK.
 */
#define WIFI_NETWORK__OK 0

/**
 * @brief   Return codes, operation status timeout.
 */
#define WIFI_NETWORK__TIMEOUT -1

/**
 * @brief   Return codes, operation status error.
 */
#define WIFI_NETWORK__ERROR -2

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/**
 * @brief   Callback events for station
 *
 * Each event might receive an event payload which can be an integral int32_t type or might be a
 * pointer to a structure.
 *
 * @note    In case the payload is a pointer to a structure it points to a constant structure. Event
 *          handle should not modify event payload data. If that is needed then event handler first
 *          needs to copy data to its internal temporary storage and modify the data there.
 */
typedef enum wifi_network__sta_event
{
    /**
     * @brief       Index of selected network
     *
     * Payload is int32_t index of currently selected Wi-Fi network with range [0 - n].
     */
    WIFI_NETWORK__STA_EVENT_SELECTED,

    /**
     * @brief       Index of network which was not selected
     *
     * This event happens when user tries to input a number of a network which is not managed by
     * whitelist.
     *
     * Payload is int32_t argument value that was given in request.
     */
    WIFI_NETWORK__STA_EVENT_NOT_SELECTED,

    /**
     * @brief       A new network was added or updated
     *
     * Payload is int32_t index of added Wi-Fi network with range [0 - n].
     */
    WIFI_NETWORK__STA_EVENT_ADDED,

    /**
     * @brief       A new network was not added
     *
     * This event may happen when whitelist is full.
     *
     * Payload is int32_t with value -1 since the whitelist is full.
     */
    WIFI_NETWORK__STA_EVENT_NOT_ADDED,

    /**
     * @brief   Network password was updated
     *
     * Payload is int32_t index of updated Wi-Fi network with range [0 - n].
     */
    WIFI_NETWORK__STA_EVENT_PWD_UPDATED,

    /**
     * @brief   Network password was not updated
     *
     * Payload is int32_t argument value given in request.
     */
    WIFI_NETWORK__STA_EVENT_PWD_NOT_UPDATED,

    /**
     * @brief   Network SSID was updated
     *
     * Payload is a static `const struct wifi_network__network_ssid` containing:
     *  - index of updated network and
     *  - SSID string
     */
    WIFI_NETWORK__STA_EVENT_SSID_UPDATED,

    /**
     * @brief   Network SSID was not updated
     *
     * Payload is the same as for WIFI_NETWORK__STA_EVENT_SSID_UPDATED event:
     *  - index of network given in request and
     *  - SSID string given in request
     */
    WIFI_NETWORK__STA_EVENT_SSID_NOT_UPDATED,

    /**
     * @brief   Network channel was updated
     *
     * Payload is a static const `struct wifi_network__network_chn` containing:
     *  - index of updated network and
     *  - channel
     */
    WIFI_NETWORK__STA_EVENT_CHN_UPDATED,

    /**
     * @brief   Network channel was updated
     *
     * Payload is a static const `struct wifi_network__network_chn` containing:
     *  - index of network to be updated given in request and
     *  - channel given in request
     */
    WIFI_NETWORK__STA_EVENT_CHN_NOT_UPDATED,

    /**
     * @brief   Network was removed from whitelist
     *
     * This event is generated when a network entry was removed from whitelist
     *
     * Payload is int32_t index of removed Wi-Fi network with range [0 - n].
     */
    WIFI_NETWORK__STA_EVENT_REMOVED,

    /**
     * @brief   Network was not removed from whitelist
     *
     * This event is generated when a network entry was not removed from
     * whitelist because of invalid index or whitelist entry was not used.
     *
     * Payload is int32_t argument value given in request.
     */
    WIFI_NETWORK__STA_EVENT_NOT_REMOVED,

    /**
     * @brief   Network whitelist entry became available
     *
     * Payload is int32_t index of Wi-Fi network with range [0 - n] that became
     * available.
     */
    WIFI_NETWORK__STA_EVENT_AVAILABLE,

    /**
     * @brief   Network whitelist entry became not-available
     *
     * Payload is int32_t index of Wi-Fi network with range [0 - n] that became
     * not-available.
     */
    WIFI_NETWORK__STA_EVENT_NOT_AVAILABLE,

    /**
     * @brief   Wi-Fi networking was enabled
     *
     * This event has no payload.
     */
    WIFI_NETWORK__STA_EVENT_ENABLED,

    /**
     * @brief   Wi-Fi networking was disabled
     *
     * This event has no payload.
     */
    WIFI_NETWORK__STA_EVENT_DISABLED,

    /**
     * @brief   Connecting to a network
     *
     * Payload is int32_t index of Wi-Fi network with range [0 - n] which is
     * being connected to.
     */
    WIFI_NETWORK__STA_EVENT_CONNECTING,

    /**
     * @brief   Connected to a network
     *
     * Payload is int32_t index of Wi-Fi network with range [0 - n].
     * Payload may be negative if the connected network was deleted in meantime.
     */
    WIFI_NETWORK__STA_EVENT_CONNECTED,

    /**
     * @brief   Disconnected from a network
     *
     * Payload is enumerated reason of disconnection:
     *  - 0 - other (WIFI_NETWORK__DISCONNECTED_OTHER)
     *  - 1 - bad password (WIFI_NETWORK__DISCONNECTED_REASON_BAD_PWD)
     *  - 2 - no such AP (WIFI_NETWORK__DISCONNECTED_REASON_NO_AP)
     */
    WIFI_NETWORK__STA_EVENT_DISCONNECTED,

    /**
     * @brief   Disconnected from a network and retrying
     *
     * Payload is enumerated reason of disconnection:
     *  - 0 - other (WIFI_NETWORK__DISCONNECTED_OTHER)
     *  - 1 - bad password (WIFI_NETWORK__DISCONNECTED_REASON_BAD_PWD)
     *  - 2 - no such AP (WIFI_NETWORK__DISCONNECTED_REASON_NO_AP)
     */
    WIFI_NETWORK__STA_EVENT_DISCONNECTED_RETRYING,

    /**
     * @brief   Scan procedure started
     *
     * This event has no payload.
     */
    WIFI_NETWORK__STA_EVENT_SCAN_STARTED,

    /**
     * @brief   Scan procedure stopped (finished or aborted)
     *
     * This event has no payload.
     */
    WIFI_NETWORK__STA_EVENT_SCAN_STOPPED,

    /**
     * @brief   Scan procedure results
     *
     * Payload is a static const struct wifi_network__scan_results.
     *
     * The scan results are not ordered in any particular order. Use function
     * wifi_network__scan_results_sort_by_rssi to sort the results by RSSI value.
     */
    WIFI_NETWORK__STA_EVENT_SCAN_RESULTS,
} WIFI_NETWORK__STA_EVENT_ENUM;
#endif

/**
 * @brief   Callback events for access point
 *
 * Each event might receive an event payload which can be an integral int32_t type or might be a
 * pointer to a structure.
 *
 * @note    In case the payload is a pointer to a structure it points to a constant structure. Event
 *          handle should not modify event payload data. If that is needed then event handler first
 *          needs to copy data to its internal temporary storage and modify the data there.
 * @note    Access point functionality is still in progress.
 */
typedef enum wifi_network__ap_event
{
    WIFI_NETWORK__AP_EVENT_DISCONNECTED,
    WIFI_NETWORK__AP_EVENT_CONNECTED,
    WIFI_NETWORK__AP_EVENT_STARTING,
    WIFI_NETWORK__AP_EVENT_STARTED,
    WIFI_NETWORK__AP_EVENT_STOPPING,
    WIFI_NETWORK__AP_EVENT_STOPPED
} WIFI_NETWORK__AP_EVENT_ENUM;

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/**
 * @brief   Event payload for network SSID
 */
struct wifi_network__network_ssid
{
    uint32_t index;                        //!< Index of network in whitelist
    char ssid[WIFI_NETWORK__SSID_LENGTH];  //!< Null terminated string of network SSID
};

/**
 * @brief   Event payload for network channel
 */
struct wifi_network__network_chn
{
    uint32_t index;  //!< Index of network in whitelist
    int32_t chn;     //!< Channel information
};

/**
 * @brief   Reason when a network gets disconnectected
 */
typedef enum wifi_network__disconnected_reason
{
    WIFI_NETWORK__DISCONNECTED_OTHER,           //!< Other reason
    WIFI_NETWORK__DISCONNECTED_REASON_BAD_PWD,  //!< Bad password
    WIFI_NETWORK__DISCONNECTED_REASON_NO_AP     //!< No access point was found
} WIFI_NETWORK__DISCONNECTED_REASON_ENUM;

/**
 * @brief   Station authorization mode
 */
typedef enum wifi_network__auth_mode
{
    WIFI_NETWORK__AUTH_OPEN = 0,         //!< authenticate mode : open
    WIFI_NETWORK__AUTH_WEP,              //!< authenticate mode : WEP
    WIFI_NETWORK__AUTH_WPA_PSK,          //!< authenticate mode : WPA_PSK
    WIFI_NETWORK__AUTH_WPA2_PSK,         //!< authenticate mode : WPA2_PSK
    WIFI_NETWORK__AUTH_WPA_WPA2_PSK,     //!< authenticate mode : WPA_WPA2_PSK
    WIFI_NETWORK__AUTH_WPA2_ENTERPRISE,  //!< authenticate mode : WPA2_ENTERPRISE
    WIFI_NETWORK__AUTH_WPA3_PSK,         //!< authenticate mode : WPA3_PSK
    WIFI_NETWORK__AUTH_WPA2_WPA3_PSK,    //!< authenticate mode : WPA2_WPA3_PSK
} WIFI_NETWORK__AUTH_MODE;

/**
 * @brief   One scan entry for scanned results
 */
struct wifi_network__scan_result
{
    int32_t rssi;
    uint32_t channel;
    enum wifi_network__auth_mode auth_mode;
    char ssid[WIFI_NETWORK__SSID_LENGTH];
};

/**
 * @brief   All scanned entries from scan procedure
 */
struct wifi_network__scan_results
{
    uint32_t no_entries;  //!< Number of scanned entries in result array
    struct wifi_network__scan_result result[WIFI_NETWORK__NO_OF_SCANNED_NETW];
};

/**
 * @brief   Union that holds event payload data.
 *
 * Depending on the event ID event handler should access proper member of this union.
 */
union wifi_network__event_payload
{
    int32_t i32;
    const struct wifi_network__scan_results *scan_results;
    const struct wifi_network__network_ssid *network_ssid;
    const struct wifi_network__network_chn *network_chn;
};

/**
 * @brief   Event handler type for station
 */
typedef void(wifi_network__event_handler)(WIFI_NETWORK__STA_EVENT_ENUM, union wifi_network__event_payload payload);
#endif

/**
 * @brief   Event handler type for access point
 */
typedef void(wifi_network__ap_event_handler)(WIFI_NETWORK__AP_EVENT_ENUM);

/**
 * @name        Module functions
 * @{
 */

/**
 * @brief       Initialize wifi_network module
 *
 * This function needs to be called before any other function of this module. The only exception to
 * this rule is @ref wifi_network__register_handler and @ref wifi_network__ap_register_handler which
 * can be used before this function to setup
 * an event handler.
 */
void wifi_network__init(void);

/**
 * @brief       Terminate wifi_network module
 *
 * The function will stop all running state machines, delete all allocated events and de-initialize
 * the Wi-Fi stack.
 *
 * @note        Before calling this function first disable Wi-Fi station and AP by calling
 *              @ref wifi_network__disable and @ref wifi_network__ap_disable functions.
 */
void wifi_network__terminate(void);

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/** @} */
/**
 * @name        Station functions
 * @{
 */

/**
 * @brief       Register an handler for Wi-Fi events
 *
 * @param       handler Pointer to event handler function
 *
 * Handler will receive enumerated events `WIFI_NETWORK__STA_EVENT_ENUM` with an optional payload
 * which is of type union.
 * 
 * @return      Operation status.
 * @retval      WIFI_NETWORK__OK on success.
 * @retval      WIFI_NETWORK__ERROR on failure, e.g. handler is NULL or no more space for handlers.
 */
int wifi_network__register_handler(wifi_network__event_handler *handler);

/**
 * @brief       Select a network from whitelist
 *
 * @param       index Possible values [0 - size(whitelist)] for manual mode and
 *              `-1` for automatic mode.
 *
 * Generates events:
 *  - WIFI_NETWORK__STA_EVENT_SELECTED
 *  - WIFI_NETWORK__STA_EVENT_NOT_SELECTED
 */
void wifi_network__network_select(int32_t index);

/**
 * @brief       Add new network to whitelist
 *
 * @param       ssid string which is to be added
 *
 * Generates events:
 *  - WIFI_NETWORK__STA_EVENT_ADDED
 *  - WIFI_NETWORK__STA_EVENT_NOT_ADDED
 */
void wifi_network__network_add(const char *ssid);

/**
 * @brief       Remove a network
 *
 * @param       index index of the network to be removed. Possible values [0 - size(whitelist)].
 *
 * Generates events:
 *  - WIFI_NETWORK__STA_EVENT_REMOVED
 *  - WIFI_NETWORK__STA_EVENT_NOT_REMOVED
 */
void wifi_network__network_remove(int32_t index);

/**
 * @brief       Initialize a network in whitelist
 *
 * If there is a free slot in whitelist the network will be but into the free slot. Otherwise, the
 * function will just ignore the network.
 *
 * @param       ssid Name of the network
 * @param       pwd Password of the network
 */
void wifi_network__network_initialize(const char *ssid, const char *pwd);

/**
 * @brief       Update an existing whitelist entry SSID
 *
 * @param       index index of the network to be updated. Possible values [0 - size(whitelist)].
 * @param       ssid New name of a network
 *
 * Generates events:
 *  - WIFI_NETWORK__STA_EVENT_SSID_UPDATED
 *  - WIFI_NETWORK__STA_EVENT_SSID_NOT_UPDATED
 */
void wifi_network__network_ssid(int32_t index, const char *ssid);

/**
 * @brief       Update an existing whitelist entry PWD
 *
 * @param       index index of the network to be updated. Possible values [0 - size(whitelist)].
 * @param       pwd New password of a network
 *
 * Generates events:
 *  - WIFI_NETWORK__STA_EVENT_PWD_UPDATED
 *  - WIFI_NETWORK__STA_EVENT_PWD_NOT_UPDATED
 */
void wifi_network__network_pwd(int32_t index, const char *pwd);

/**
 * @brief       Update an existing whitelist entry channel
 *
 * @param index index of the network to be updated. Possible values [0 - size(whitelist)].
 * @param chn   new channel of a network
 *
 * Generates events:
 *  - WIFI_NETWORK__STA_EVENT_CHN_UPDATED
 *  - WIFI_NETWORK__STA_EVENT_CHN_NOT_UPDATED
 */
void wifi_network__network_chn(int32_t index, uint32_t chn);

/**
 * @brief       Finds an existing whitelist entry
 *
 * @param       ssid New name of a network
 * @return      index of the found network to be updated. Possible values [0 - size(whitelist)].
 *              -1 if not found
 *
 * Generates events:
 */
int wifi_network__network_find_ssid(const char *ssid);

/**
 * @brief       Enable Wi-Fi connection
 *
 * Generates event:
 *  - WIFI_NETWORK__STA_EVENT_ENABLED
 */
void wifi_network__enable(void);

/**
 * @brief       Disable Wi-Fi connection
 *
 * This function will stop Wi-Fi station operation. If the AP component is also disabled then the
 * whole Wi-Fi peripheral will be put into sleep mode.
 *
 * Generates event:
 *  - WIFI_NETWORK__STA_EVENT_DISABLED
 */
void wifi_network__disable(void);

/**
 * @brief       Instruct the SM that it should generate a series of events in order to completelly
 *              update whitelist information
 *
 */
void wifi_network__network_sync(void);
#endif

/**
 * @brief       Enable background passive scanner
 *
 * Wi-Fi network module incorporates background passive scanner. This scanner is used to create a
 * list of best Wi-Fi network candidates to which we can connect to. This scanner is started
 * periodically by the Wi-Fi network module. During the operation of background passive scanner,
 * Wi-Fi throughput will be impacted. In some situations it is desired to sacrifice creation of
 * candidate list in order to get better data throughput.
 *
 * Passive background scanner is engaged when currently connected Wi-Fi network has bad signal
 * quality grade. The signal quality grade is calculated by currently used data rate and RSSI
 * information. The goal of passive background scanner is to find best possible candidates in case
 * when the currently selected network is dropped.
 *
 * By default, background passive scanner is enabled so you don't need to explicitly enable it
 * after startup.
 */
void wifi_network__enable_background_scanner(void);

/**
 * @brief       Disable background passive scanner
 */
void wifi_network__disable_background_scanner(void);

/**
 * @brief       Lock Wi-Fi stack mode changes
 *
 * ESP IDF supports the following Wi-Fi stack modes:
 * - AP: only Wi-Fi AP is enabled
 * - STA: only Wi-Fi station is enabled
 * - APSTA: both, AP and station are enabled
 * - NULL: None of the above are enabled
 *
 * Mode changes are requered to allow ESP IDF to manage radio power consumption in optimal way.
 *
 * In some situations it was seen that mode changes during the operation of AP will reset all the
 * connected clients, therefore we offer manual mechanism to override Wi-Fi mode changes.
 *
 * Once this function is called, mode changes are forbidden. To re-enable mode changes call to
 * @ref wifi_network__unlock_mode is needed.
 */
void wifi_network__lock_mode(void);

/**
 * @brief       Unlock Wi-Fi stack mode changes
 */
void wifi_network__unlock_mode(void);

#ifndef CONNECTOR_WIFI_SERVICE_ON_AP_ONLY
/**
 * @brief       Start active scan
 *
 * Generates event:
 *  - WIFI_NETWORK__STA_EVENT_SCAN_STARTED
 *  - WIFI_NETWORK__STA_EVENT_SCAN_STOPPED
 *  - WIFI_NETWORK__STA_EVENT_SCAN_RESULTS
 */
void wifi_network__scan_start(void);
#endif

/** @} */
/**
 * @name        Access Point functions
 * @{
 */

/**
 * @brief       Register event handler for AP
 *
 * @param       handler Pointer to event handler function.
 *
 * Handler will receive enumerated events `WIFI_NETWORK__AP_EVENT_ENUM`.
 * 
 * @return      Operation status.
 * @retval      WIFI_NETWORK__OK on success.
 * @retval      WIFI_NETWORK__ERROR on failure, e.g. handler is NULL or no more space for handlers.
 */
int wifi_network__ap_register_handler(wifi_network__ap_event_handler *handler);

/**
 * @brief       Enable access point
 *
 * @note        Before calling this function wifi_network__ap_update_settings() should be called.
 */
void wifi_network__ap_enable(void);

/**
 * @brief       Disable access point
 *
 * This function will stop Wi-Fi AP operation. If the station component is also disabled then the
 * whole Wi-Fi peripheral will be put into sleep mode.
 *
 * @note        After AP is disabled, the station background scanner is enabled back.
 */
void wifi_network__ap_disable(void);

/**
 * @brief       Set new SSID and password for AP
 *
 * @param       ssid NULL terminated C string with max length of 32 bytes.
 * @param       pwd NULL terminated C string with max length of 64 bytes.
 */
void wifi_network__ap_update_settings(const char *ssid, const char *pwd);

/**
 * @brief       Get configured SSID string
 *
 * @param[out]  ssid pointer to char array of size @ref WIFI_NETWORK__SSID_LENGTH.
 */
void wifi_network__ap_get_ssid(char *ssid);

/** @} */
#endif /* WIFI_NETWORK_H_ */
       /** @} */
