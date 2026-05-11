/*! \file bluetooth_le.c
    \brief Main BLE source.

    Main Bluetooth low energy/GAP module.
    \author Jens Björnhager
    \author Andreas Lundeen
 */

#include "configuration.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "ble_state_machine.h"
#include "bluetooth_le.h"
#include "broker.h"
#include "connector_ble.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "peripheral_role.h"

#ifndef BLUETOOTH_BOND_INTERVAL
#define BLUETOOTH_BOND_INTERVAL 60000  //!< \~ Bluetooth bond timer, 60 seconds
#endif

#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
#ifndef BLINK_TIMER_INTERVAL_MS
#define BLINK_TIMER_INTERVAL_MS 200  //!< 200 ms
#endif
#ifndef BLINK_COUNTER_MAX
#define BLINK_COUNTER_MAX 20
#endif
uint32_t bond_mode;                      //!< \~ Bond mode is active
static TimerHandle_t bond_timer = NULL;  //!< \~ Timer keeping track of BLE bond interval
#ifndef CONNECTOR_BLE_DISABLE_LED
static TimerHandle_t blink_timer = NULL;  //!< \~ Timer keeping track of BLE bond interval
static int blink_counter = 0;
#endif /* CONNECTOR_BLE_DISABLE_LED */
static int bonded_during_bond_interval;
//! \~ Timer keeping track of BLE bond interval
static void bond_timer_timeout(TimerHandle_t xTimer);
#ifndef CONNECTOR_BLE_DISABLE_LED
static void blink_timer_timeout(TimerHandle_t xTimer);
#endif  /* CONNECTOR_BLE_DISABLE_LED */
#endif  // CONNECTOR_BLE_PERIPHERAL_GATT

/*! \brief Print a BLE address as a string
    \param addr Pointer to the BLE address
    \return A string representation of the BLE address
    \warning This function is not reentrant
*/
char *addr_str(const void *const addr)
{
    static char buf[6 * 2 + 5 + 1];
    const uint8_t *const u8p = addr;

    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);

    return buf;
}

/*! \brief Print a BLE connection description
    \param desc Pointer to the BLE connection description
*/
void print_conn_desc(const struct ble_gap_conn_desc *desc)
{
    LOG(D, "handle=%d peer_id_addr_type=%d peer_id_addr=%s", desc->conn_handle, desc->peer_id_addr.type, addr_str(desc->peer_id_addr.val));
    LOG(D, "conn_itvl=%d conn_latency=%d supervision_timeout=%d encrypted=%d authenticated=%d bonded=%d",
        desc->conn_itvl, desc->conn_latency, desc->supervision_timeout, desc->sec_state.encrypted,
        desc->sec_state.authenticated, desc->sec_state.bonded);
}

/*! \brief Activate bonding mode and start bond timer
 */
void bond_timer_start(void)
{
#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
    LOG(I, "Entering bond mode");
    TRUE_CHECK(xTimerStart(bond_timer, portMAX_DELAY));
    bonded_during_bond_interval = 0;
    bond_mode = BT0PAIR_OUT_PAIRING_MODE_ACTIVE;
    ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
#ifndef CONNECTOR_BLE_DISABLE_LED
    blink_counter = 0;
    TRUE_CHECK(xTimerStart(blink_timer, portMAX_DELAY));
#endif /* CONNECTOR_BLE_DISABLE_LED */
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, BT0PAIR, &bond_mode, sizeof(bond_mode), connector_ble.connector_id, portMAX_DELAY);
#endif  // CONNECTOR_BLE_PERIPHERAL_GATT
}

/*! \brief Blink the blue LED
    \note This function is called to indicate that a device has bonded.
*/
#ifndef CONNECTOR_BLE_DISABLE_LED
void blink_blue(void)
{
    if (LED_B(LED_B_DEFAULT_STATE) != HAL_GPIO_ERROR_PIN_UNAVAILABLE)
    {
        TRUE_CHECK(xTimerStop(blink_timer, portMAX_DELAY));
        blink_counter = 0;
        TRUE_CHECK(xTimerStart(blink_timer, portMAX_DELAY));
    }
}
#endif /* CONNECTOR_BLE_DISABLE_LED */

/*! \brief Deactivate bonding mode and stop bond timer
    \param reason The reason for stopping the bond timer
*/
void bond_timer_stop(const BT0PAIR_OUT_ENUM reason)
{
#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
    bool keep_open = false;

    switch (reason)
    {
    case BT0PAIR_OUT_DEVICE_PAIRED:  // paired with device
#ifdef CONNECTOR_BLE_PERIPHERAL_KEEP_BOND_OPEN
        keep_open = true;
#ifndef CONNECTOR_BLE_DISABLE_LED
        blink_blue();
#endif /* CONNECTOR_BLE_DISABLE_LED */
#endif /* CONNECTOR_BLE_PERIPHERAL_KEEP_BOND_OPEN */
        bonded_during_bond_interval = 1;
        break;
    case BT0PAIR_OUT_PAIRING_MODE_INACTIVE:  // bond mode was manually turned off
    case BT0PAIR_OUT_PAIRING_MODE_TIMEOUT:   // bond timer ran out
#ifndef CONNECTOR_BLE_DISABLE_LED
        TRUE_CHECK(xTimerStop(blink_timer, portMAX_DELAY));
        LED_B(LED_B_DEFAULT_STATE);
#endif /* CONNECTOR_BLE_DISABLE_LED */
        break;
    case BT0PAIR_OUT_DEVICE_PAIR_FAILED:
    default:
        break;
    }

    if (!keep_open)
    {
        TRUE_CHECK(xTimerStop(bond_timer, portMAX_DELAY));
        LOG(I, "Exiting bond mode");
        bond_mode = BT0PAIR_OUT_PAIRING_MODE_INACTIVE;
        if ((bonded_during_bond_interval) && (!bt0advintvl))  // begin advertising normally if previously disabled
        {
            set_advertisement_interval(BT0ADVINTVL_DEFAULT);
        }
        else
        {
            ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
        }
        bonded_during_bond_interval = 0;
    }

    const int32_t bt0pair_out = reason;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, BT0PAIR, &bt0pair_out, sizeof(bt0pair_out), connector_ble.connector_id, portMAX_DELAY);

#endif  // CONNECTOR_BLE_PERIPHERAL_GATT
}

#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
/*! \brief BLE bond interval timer callback
    \param xTimer The timer that expired
    \note This function is called when the bond timer expires, indicating that bonding mode should be stopped.
*/
static void bond_timer_timeout(TimerHandle_t xTimer)
{
#ifdef CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE
    if (do_bonding_records_exist() == false)
    {
        bond_timer_start();
    }
    else
#endif  // CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE
    {
        bond_timer_stop(BT0PAIR_OUT_PAIRING_MODE_TIMEOUT);
    }
}

#ifndef CONNECTOR_BLE_DISABLE_LED
/*! \brief Blink the blue LED using a timer
 */
static void blink_timer_timeout(TimerHandle_t xTimer)
{
    blink_counter++;
    LED_B(blink_counter & 1);
    if (blink_counter < BLINK_COUNTER_MAX)
    {
        // Restart to get 10 blinks
        TRUE_CHECK(xTimerStart(blink_timer, portMAX_DELAY));
    }
    else
    {
        blink_counter = 0;
    }
}
#endif /* CONNECTOR_BLE_DISABLE_LED */

/*! \brief Create BLE bond and blink timers
 */
void bond_timer_create(void)
{
    TRUE_CHECK((bond_timer = xTimerCreate(NULL, pdMS_TO_TICKS(BLUETOOTH_BOND_INTERVAL), pdFALSE, NULL, bond_timer_timeout)) != NULL);
#ifndef CONNECTOR_BLE_DISABLE_LED
    TRUE_CHECK((blink_timer = xTimerCreate(NULL, pdMS_TO_TICKS(BLINK_TIMER_INTERVAL_MS), pdFALSE, NULL, blink_timer_timeout)) != NULL);
#endif /* CONNECTOR_BLE_DISABLE_LED */
}

#ifdef CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE
/*! \brief Check whether BLE device bonds have been recorded
 */
bool do_bonding_records_exist(void)
{
    int count = 0;
    int rc = ble_store_util_count(BLE_STORE_OBJ_TYPE_PEER_SEC, &count);

    if (rc)
    {
        LOG(E, "ble_store_util_count returned: %d", rc);
    }

    return count == 0 ? false : true;
}
#endif  // CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE

/*! \brief Clear all BLE device bonds
 */
void clear_bonds(void)
{
    LOG(I, "Clearing BLE bonds");
    ble_addr_t addr[CONFIG_BT_NIMBLE_MAX_BONDS + 1];
    union ble_store_key key = {0};
    int num_peers = 0;
    if (!ble_store_util_bonded_peers(addr, &num_peers, CONFIG_BT_NIMBLE_MAX_BONDS + 1))
    {
        LOG(I, "Deleting %d peers", num_peers);
        key.sec.peer_addr = addr[0];
        ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_PEER_SEC, &key);
        memset(&key, 0, sizeof(key));
        key.sec.peer_addr = addr[0];
        ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_OUR_SEC, &key);
        memset(&key, 0, sizeof(key));
        key.cccd.peer_addr = addr[0];
        ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_CCCD, &key);
        memset(&key, 0, sizeof(key));
        key.rpa_rec.peer_rpa_addr = addr[0];
        ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_PEER_ADDR, &key);
        memset(&key, 0, sizeof(key));
        key.local_irk.addr = addr[0];
        ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_LOCAL_IRK, &key);
    }
    ble_store_clear();
    nvs_handle_t nimble_handle;
    nvs_open("nimble_bond", NVS_READWRITE, &nimble_handle);
    nvs_erase_all(nimble_handle);
    nvs_commit(nimble_handle);
    nvs_close(nimble_handle);

    set_advertisement_interval(BT0ADVINTVL_INITIAL);  // return advertising interval to default, possibly disabled
}
#endif  // CONNECTOR_BLE_PERIPHERAL_GATT

/*! \brief Convert mbuf from notification to flat memory buffer
    \param om The mbuf to convert
    \param min_len The minimum length of the buffer
    \param max_len The maximum length of the buffer
    \param dst The destination buffer
    \param len The length of the data written to the destination buffer
    \return 0 on success, or a BLE error code on failure
*/
int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len, void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len)
    {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}
