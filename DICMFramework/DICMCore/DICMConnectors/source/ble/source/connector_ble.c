/*! \file connector_ble.c
    \brief BLE GAP/GATT connector, NimBLE edition
    \author Jens Björnhager
    \author Andreas Lundeen
*/

// System includes
#include <stdint.h>

// IDF includes
#include "esp_idf_version.h"
#include "sdkconfig.h"
#if !CONFIG_IDF_TARGET_ESP32C2
#include "esp_nimble_hci.h"
#endif
#include "host/ble_att.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/ble_hs_pvcy.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
// Framework includes
#include "configuration.h"

#include "ble_peripheral.h"
#include "ble_state_machine.h"
#include "bluetooth_le.h"
#include "broker.h"
#include "connector_ble.h"
#include "ddm2_parameter_list.h"
#include "hal_cpu.h"
#include "hal_mem.h"
#include "peripheral_role.h"
#include "product_database.h"
#include "sorted_list.h"
#include "utils.h"

COMPILE_TIME_ASSERT(sizeof(ble_addr_t) == sizeof(BT0ADDWL_T));

/* Broker slot configuration:
    service     1                               (0)
    peripheral  BLE_PERIPHERAL_MAX_CONNECTIONS  (1..BLE_PERIPHERAL_MAX_CONNECTIONS)
    central     BLE_CENTRAL_MAX_NODE_INSTANCES  (BLE_PERIPHERAL_MAX_CONNECTIONS + 1 .. BLE_PERIPHERAL_MAX_CONNECTIONS + BLE_CENTRAL_MAX_NODE_INSTANCES)
*/

#define IS_DESTINATION_SERVICE(slot)    ((slot) == 0)                                                                                                                //!< \~ Check if the destination slot is the service slot
#define IS_DESTINATION_PERIPHERAL(slot) (((slot) > 0) && ((slot) <= BLE_PERIPHERAL_MAX_CONNECTIONS)) 

nvs_handle_t nvs_ble;  //!< \~ Handle to ble flash storage

int32_t bt0advintvl = BT0ADVINTVL_INITIAL;  //!< \~ BLE advertising interval
int32_t bt0clients;                         //!< \~ Active BLE peripheral client count

static int32_t bt0on = 1;

#ifdef CONNECTOR_BLE_PERIPHERAL_GATT_NOTIFY_ENCRYPTION
extern void ble_gatts_set_clt_cfg_perm_flags(uint8_t flags);  // from "../host/src/ble_gatt_priv.h"
#endif

extern void ble_store_config_init(void);

/*! \brief Get the BLE connector slot from connector ID
    \param connector_id The connector ID
    \return The connector slot
*/
int connector_ble_connector_slot(const int connector_id)
{
    return connector_id - connector_ble.connector_id;
}

/*! \brief Get the connector ID for a peripheral connection slot
    \param peripheral_connection_slot The peripheral connection slot
    \return The connector ID
*/
int connector_ble_peripheral_connector_id(const int peripheral_connection_slot)
{
    return connector_ble.connector_id + 1 + peripheral_connection_slot;
}

/*! \brief Get the peripheral connection slot for a BLE connector slot
    \param connector_slot The connector slot
    \return The peripheral connection slot
*/
int connector_ble_peripheral_connection_slot(const int connector_slot)
{
    const int Peripheral_start = 1;

    if ((connector_slot < Peripheral_start) || (connector_slot >= (Peripheral_start + BLE_PERIPHERAL_MAX_CONNECTIONS)))
    {
        return -1;  // invalid connector slot
    }

    return connector_slot - Peripheral_start;
}

/*! \brief Send a reply frame to the broker from the BLE connector service
    \param parameter The parameter to reply to
    \param value Pointer to the value to send
    \param value_size Size of the value in bytes
    \return TRUE on success
*/
int ble_reply(const uint32_t parameter, const void *const value, const uint8_t value_size)
{
    return connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, parameter, value, value_size, connector_ble.connector_id, portMAX_DELAY);
}

/*! \brief Set the BLE advertising interval (BT0ADVINTVL)
    \param interval The advertising interval in milliseconds
*/
void set_advertisement_interval(int32_t interval)
{
    if (interval)
    {
        constrain(&interval, (BLE_HCI_ADV_ITVL_MIN * BLE_HCI_ADV_ITVL) / 1000, ((BLE_HCI_ADV_ITVL_MAX - 100) * BLE_HCI_ADV_ITVL) / 1000);
    }

    bt0advintvl = interval;

#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
    ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
#endif

    config_set_i32("bt0advintvl", bt0advintvl);
    ble_reply(BT0ADVINTVL, &bt0advintvl, sizeof(bt0advintvl));
}

/*! \brief Set the BLE connection state (BT0ON)
    \param on TRUE to enable BLE connections, FALSE to disable
*/
static void set_bt_connection_state(int32_t on)
{
    bt0on = on;

    if (bt0on)
    {
        LOG(D, "BLE connections: enabled");
        ble_state_machine_generate_event(BLE_SM_ENABLE_CONNECTIONS_EVENT, 0);
    }
    else
    {
        LOG(D, "BLE connections: disabled");
        ble_state_machine_generate_event(BLE_SM_DISABLE_CONNECTIONS_EVENT, 0);
    }

    ble_reply(BT0ON, &bt0on, sizeof(bt0on));
}

/*! \brief Handle BLE service set frames (BT0, BTC0, BTWL0)
 *
 *  \param pframe Pointer to the incoming DDMP2 frame
 *  \return TRUE if handled
 */
static int handle_ble_set(const DDMP2_FRAME *const pframe)
{
    switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.set.parameter))
    {
    case BTC0DEL:
        ble_peripheral_handle_clients(pframe);
        return 1;
    }

    switch (pframe->frame.set.parameter)
    {
#if defined(CONNECTOR_BLE_CENTRAL_GATT)
    case BT0PAIR:
        switch (pframe->frame.set.value.int32)
        {
        case BT0PAIR_IN_PAIRING_MODE_OFF:
            bond_timer_stop(BT0PAIR_OUT_PAIRING_MODE_INACTIVE);
            return 1;

        case BT0PAIR_IN_PAIRING_MODE_ON:
            bond_timer_start();
            return 1;

        case BT0PAIR_IN_CLEAR_BONDS:
            clear_bonds();
            return 1;

        default:
            return 1;
        }
        break;

    case BT0ADVINTVL:
        set_advertisement_interval(pframe->frame.set.value.int32);
        return 1;
#endif
    case BT0ON:
        set_bt_connection_state(pframe->frame.set.value.int32);
        return 1;

    case BT0ADDGAP:
        LOG(W, "Not supported command - BT0ADDGAP");
        break;

        return 1;
    }

    return 0;
}

/*! \brief Handle BLE subscribe frames (BTC0, BT0, BTWL0)
 *  \param pframe Pointer to the incoming DDMP2 frame
 *  \return TRUE if handled
 */
static int handle_ble_subscribe(const DDMP2_FRAME *pframe)
{
    switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter))
    {
    case BTC0CONN:
    case BTC0NAME:
        ble_peripheral_handle_clients(pframe);
        return 1;
    }

    switch (pframe->frame.subscribe.parameter)
    {
    case BT0SCAN:
    case BT0ADDWL:
    case BT0ADDGAP:
        return 1;

    case BT0PAIR:
        ble_reply(pframe->frame.subscribe.parameter, &bond_mode, sizeof(bond_mode));
        return 1;

    case BT0ADVINTVL:
        ble_reply(pframe->frame.subscribe.parameter, &bt0advintvl, sizeof(bt0advintvl));
        return 1;

    case BT0CLIENTS:
        ble_reply(pframe->frame.subscribe.parameter, &bt0clients, sizeof(bt0clients));
        return 1;

    case BT0ON:
        ble_reply(pframe->frame.subscribe.parameter, &bt0on, sizeof(bt0on));
        return 1;
    }

    return 0;
}

/*! \brief Handle BLE publish frames (GW0SKU, SVC0SYSTUPD)
 *  \param pframe Pointer to the incoming DDMP2 frame
 *  \return TRUE if handled
 */
int handle_ble_publish(const DDMP2_FRAME *pframe)
{
    const int Value_size = (int)ddmp2_value_size(pframe);
    char value_buffer[32] = {0};

    switch (pframe->frame.publish.parameter)
    {
    case GW0SKU:
        memcpy(value_buffer, pframe->frame.publish.value.raw, MIN(Value_size, (int)sizeof(value_buffer) - 1));
        LOG(I, "Received PUBLISH frame for parameter %08x, size:%d, %s", pframe->frame.publish.parameter, Value_size, value_buffer);
#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
        ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
#endif
        return 1;

    }

    return 0;
}

/*! \brief Handle BLE frames from broker (PUBLISH, SET, SUBSCRIBE) and GENERIC frames from the BLE state machine
 *  \param pframe Pointer to the incoming DDMP2 frame
 *  \return TRUE if handled
 */
static int handle_ble_parameter(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe);

    int handle_result;

    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_GENERIC:
        return ble_state_machine_dispatch_event((void *)pframe);  // Forward to ble_state_machine

    case DDMP2_CONTROL_PUBLISH:
        handle_result = handle_ble_publish(pframe);

        if (!handle_result)
        {
            LOG(W, "Unhandled PUBLISH frame for parameter %08x!", pframe->frame.publish.parameter);
        }
        return handle_result;

    case DDMP2_CONTROL_SET:
        handle_result = handle_ble_set(pframe);

        if (!handle_result)
        {
            LOG(W, "Unhandled SET frame for parameter %08x!", pframe->frame.set.parameter);
        }
        return handle_result;

    case DDMP2_CONTROL_SUBSCRIBE:
        handle_result = handle_ble_subscribe(pframe);

        if (!handle_result)
        {
            LOG(W, "Unhandled SUBSCRIBE frame for parameter %08x!", pframe->frame.subscribe.parameter);
        }
        return handle_result;
    }

    return 0;
}

/*! \brief Process BLE frames from the broker; forward frames to the appropriate handler based on the destination slot.
    \param parameter Unused parameter, can be NULL
*/
static void connector_ble_process_task(void *parameter)
{
    DDMP2_FRAME *pframe;
    size_t frame_size;

    // Subscribe to common parameters
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, GW0SKU, NULL, 0, connector_ble.connector_id, portMAX_DELAY));

    vTaskDelay(pdMS_TO_TICKS(1000));

#ifdef CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE
    // Activate pairing procedure by default if no bonding records are available in memory
    if (do_bonding_records_exist() == false)
    {
        bond_timer_start();
    }
#endif  // CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE

    while (1)
    {
        while ((pframe = xRingbufferReceive(connector_ble.to_connector, &frame_size, portMAX_DELAY)))
        {
            const int Ble_connector_slot = pframe->destination_connector - connector_ble.connector_id;
            if (ProdDBFrameHandler(pframe))
            {
                // handled in product database
            }
            else if (IS_DESTINATION_SERVICE(Ble_connector_slot))  // service slot
            {
                ESP_LOG_BUFFER_HEXDUMP("cb_service", pframe, frame_size, ESP_LOG_DEBUG);

                if (!handle_ble_parameter(pframe))
                {
                    LOG(W, "Unhandled frame for service slot, parameter %08x", pframe->frame.set.parameter);
                }
            }
            else if (IS_DESTINATION_PERIPHERAL(Ble_connector_slot))  // peripheral slot
            {
                const int Peripheral_connection_slot = connector_ble_peripheral_connection_slot(Ble_connector_slot);

                ESP_LOG_BUFFER_HEXDUMP("cb_peripheral", pframe, frame_size, ESP_LOG_DEBUG);

                if (!ble_peripheral_handle_frame(pframe, Peripheral_connection_slot))
                {
                    LOG(W, "Unhandled frame for peripheral slot %d, parameter %08x", Peripheral_connection_slot, pframe->frame.set.parameter);
                }
            }
            else
            {
                LOG(E, "Invalid destination slot %d for frame with parameter %08x", Ble_connector_slot, pframe->frame.set.parameter);
            }

            vRingbufferReturnItem(connector_ble.to_connector, pframe);
        }
    }
}

/*! \brief Callback for NimBLE reset event (from ble_hs_cfg)
    \param reason The reason for the reset
*/
static void on_reset(int reason)
{
    LOG(E, "Resetting NimBLE state; reason=%d", reason);
}

/*! \brief Callback for NimBLE sync event (from ble_hs_cfg)
    \note This is called when the NimBLE stack is ready to use, after initialization.
*/
static void on_sync(void)
{
    int rc;
    uint8_t addr_val[6] = {0};
    uint8_t own_addr_type = 0;
    LOG(I, "on_sync() callback");
#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
    assert(ble_hs_util_ensure_addr(0) == 0); /* Make sure we have proper identity address set (public preferred) */

    rc = ble_hs_id_infer_auto(0, &own_addr_type);

    if (rc)
    {
        LOG(E, "ble_hs_id_infer_auto returned %d", rc);
    }

    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    if (rc)
    {
        LOG(E, "ble_hs_id_copy_addr returned %d", rc);
    }
#ifdef CONFIG_IDF_TARGET_ESP32
    ble_hs_pvcy_rpa_config(0);
#endif
    ble_state_machine_generate_event(BLE_SM_STACK_SYNC_DONE_EVENT, 0);
#endif
}

/*! \brief Task to run NimBLE host in FreeRTOS
    \param param Unused parameter
*/
static void host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/*! \brief Callback for BLE store status events (from ble_hs_cfg)
    \note This function handles BLE store overflow event with a round robin unpairing of the oldest peer.
    \param event Pointer to the BLE store status event
    \param arg Unused argument
    \return 0 on success, or an error code
*/
static int ble_store_util_status_rr_(struct ble_store_status_event *event, void *arg)
{
    LOG(I, "Code: %d", event->event_code);

    switch (event->event_code)
    {
    case BLE_STORE_EVENT_OVERFLOW:
        switch (event->overflow.obj_type)
        {
        case BLE_STORE_OBJ_TYPE_OUR_SEC:
        case BLE_STORE_OBJ_TYPE_PEER_SEC:
            return ble_gap_unpair_oldest_peer();

        case BLE_STORE_OBJ_TYPE_CCCD:
            /* Try unpairing oldest peer except current peer */
            return ble_gap_unpair_oldest_except(&event->overflow.value->cccd.peer_addr);

        default:
            return BLE_HS_EUNKNOWN;
        }

    case BLE_STORE_EVENT_FULL:
        /* Just proceed with the operation.  If it results in an overflow,
         * we'll delete a record when the overflow occurs.
         */
        return 0;

    default:
        return BLE_HS_EUNKNOWN;
    }
}

/**
 * @brief Initilaize the nimnle stack
 * @return None
 */
void ble_nimble_stack_start_init(void)
{
    LOG(D, "BLE stack start init!");
#ifdef CONNECTOR_BLE_PERIPHERAL_GATT_NOTIFY_ENCRYPTION
    ble_gatts_set_clt_cfg_perm_flags(BLE_ATT_F_READ | BLE_ATT_F_WRITE | BLE_ATT_F_WRITE_ENC);
#endif
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
#endif
    nimble_port_init();

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr_;

#ifdef CONNECTOR_BLE_PERIPHERAL_GATT
    ble_hs_cfg.gatts_register_cb = gatt_server_register_cb;
    ble_hs_cfg.sm_io_cap = 3;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
#endif

    ble_svc_gap_device_name_set(device_information.default_name);
    assert(gatt_server_initialize() == 0);
    ble_store_config_init();

    /*
    * Create task where NimBLE host will run. It is not strictly necessary to
    * have separate task for NimBLE host, but since something needs to handle
    * default queue it is just easier to make separate task which does this.
    */
    nimble_port_freertos_init(host_task);

    return;
}

/**
 * @brief Deint the nimnle stack
 * @return None
 */
void ble_nimble_stack_stop_deinit(void)
{
    LOG(I, "BLE stack stop deinit!");

    btc_class_unregister();

    int rc = nimble_port_stop();
    if (rc == 0) 
    {
        nimble_port_deinit();
    } 
    else 
    {
        LOG(E, "nimble_port_stop rc=%d", rc);
    }

    return;
}

/*! \brief Initializes and starts the bluetooth subsystem, if enabled
    \return TRUE on success
*/
static int initialize_connector_ble(void)
{
#if defined(CONNECTOR_BLE_PERIPHERAL_GATT)
    bond_timer_create();
#endif

    ZERO_CHECK(nvs_open("ble", NVS_READWRITE, &nvs_ble));
    config_get_i32("bt0advintvl", &bt0advintvl);

    init_ble_state_machine(connector_ble.connector_id);

    ZERO_CHECK(ProdDBInit());

    ble_peripheral_initialize();

    ble_nimble_stack_start_init();
   
    uint32_t bt0 = BT0;  // request BT0 instance
    int bt_instance = broker_register_instance(&bt0, connector_ble.connector_id);
    ASSERT(bt_instance == 0);  // BT0 should not be requested by any other

    // initialized; start task
    TRUE_CHECK(xTaskCreate(connector_ble_process_task, connector_ble.name, 3072, NULL, xTASK_PRIORITY_ABOVE_NORMAL, NULL));

    return 1;
}

CONNECTOR connector_ble = {
    .name = "BLE connector",
    .initialize = initialize_connector_ble,
    .data_lines = BLE_PERIPHERAL_MAX_CONNECTIONS,  //!< \~ Number of extra broker slots (peripheral)
};
