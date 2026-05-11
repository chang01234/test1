/**
 * @file ble_peripheral.c
 *
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ble_peripheral.h"
#include "ble_peripheral_dicm.h"
#include "ble_state_machine.h"
#include "bluetooth_le.h"
#include "broker.h"
#include "configuration.h"
#include "connector.h"
#include "connector_ble.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "freertos/semphr.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "iGeneralDefinitions.h"
#include "nimble/ble.h"
#include "nvs.h"
#include "peripheral_role.h"
#include "services/dis/ble_svc_dis.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sorted_list.h"

#define GATT_SERVICE_INPUT  0
#define GATT_SERVICE_OUTPUT 1

#define MAX_NUMBER_STORED_BLE_CLIENTS (5)
#define PERIPHERAL_SERVICE_LIST_SIZE  (6)

extern int ble_reply(const uint32_t parameter, const void *const value, const uint8_t value_size);  // connector_ble.c

typedef struct
{
    int32_t instance;
    int32_t conn;
    uint8_t name_len;
    uint8_t name[128];
} ble_client_dynamic_info_t;

typedef struct
{
    ble_addr_t addr[MAX_NUMBER_STORED_BLE_CLIENTS];
    ble_client_dynamic_info_t info[MAX_NUMBER_STORED_BLE_CLIENTS];
} ble_client_info_t;

typedef struct ble_diconnect_cmd_t
{
    bool remove;
    int index;
} ble_diconnect_cmd_t;

EXT_RAM_ATTR peripheral_connection_state_t peripheral_connection_state[BLE_PERIPHERAL_MAX_CONNECTIONS];

static EXT_RAM_ATTR ble_diconnect_cmd_t ble_diconnect_cmd;
static EXT_RAM_ATTR ble_client_info_t ble_client_info;
static EXT_RAM_ATTR int num_client_info;

static SemaphoreHandle_t peripheral_connections_mutex;
static EXT_RAM_ATTR struct ble_gatt_svc_def ble_peripheral_gatt_services[PERIPHERAL_SERVICE_LIST_SIZE];
static int ble_peripheral_gatt_service_count = 0;

/* Local functions */
static void peripheral_print_conn_desc(struct ble_gap_conn_desc *desc);
static int ble_read_gatt_attr_cb(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);
static int peripheral_gap_event(struct ble_gap_event *event, void *arg);
static void update_ble_client_storage(void);
static void update_ble_client_name_in_storage(int name_index);
static void delete_ble_client_from_storage(int client_index);

static const ble_uuid_t *Uuid_read_device_name = BLE_UUID16_DECLARE(BLE_SVC_GAP_CHR_UUID16_DEVICE_NAME);
static char manufacturer_name[32] = "Dometic";
static bool bBonded = false;

const get_data_buffer_function_t Get_advertisement_data_functions[] =
    {
        ble_peripheral_dicm_get_advertisement_data,
#ifdef BLE_PERIPHERAL_TEMPLATE
        ble_peripheral_template_get_advertisement_data,
#endif
#ifdef BLE_PERIPHERAL_APPLICATION
        ble_peripheral_application_get_advertisement_data,
#endif
#ifdef BLE_PERIPHERAL_PDT
        NULL,
#endif
};

const get_data_buffer_function_t Get_scan_response_data_functions[] =
    {
        ble_peripheral_dicm_get_scan_response_data,
#ifdef BLE_PERIPHERAL_TEMPLATE
        ble_peripheral_template_get_scan_response_data,
#endif
#ifdef BLE_PERIPHERAL_APPLICATION
        ble_peripheral_application_get_scan_response_data,
#endif
#ifdef BLE_PERIPHERAL_PDT
        NULL,
#endif
};
static const ble_peripheral_connect_cb_t Ble_peripheral_connect_cbs[] = {
    NULL,
#ifdef BLE_PERIPHERAL_TEMPLATE
    NULL,
#endif
#ifdef BLE_PERIPHERAL_APPLICATION
    ble_peripheral_application_connect_cb,
#endif
#ifdef BLE_PERIPHERAL_PDT
    ble_peripheral_pdt_on_connection_update_cb,
#endif
};

#ifdef CONNECTOR_BLE_PERIPHERAL_DIS
static int device_information_service_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(ctxt->om, manufacturer_name, strlen((char *)manufacturer_name));

    return 0;
}

const struct ble_gatt_svc_def device_information_service[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_DIS_CHR_UUID16_MANUFACTURER_NAME),
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = device_information_service_cb,
            },
            {
                0,
            },
        },
    },
};
#endif

/*! \brief Set the manufacturer name
    \param name Pointer to the manufacturer name string, or NULL to set to default "Dometic"
*/
void ble_peripheral_set_manufacturer_name(const char *const name)
{
    const char *const pname = (name == NULL) ? "Dometic" : name;
    const size_t Length = MIN(strlen((const char *)pname), sizeof(manufacturer_name) - 1);

    strncpy(manufacturer_name, pname, Length);
}

/*! \brief Get the bonded flag status
    \return 1 if bonded, 0 otherwise
*/
bool ble_peripheral_get_bonded_flag(void)
{
    return bBonded;
}

/*! \brief Find a connection slot by connection handle; Looks up a connection in the peripheral connection list)
    \param connection_handle Connection handle
    \warning This function requires the peripheral_connections_mutex to be held before calling it
    \return Connection slot, or -1 if not found
*/
static int peripheral_find_connection(const uint16_t connection_handle)
{
    for (int slot = 0; slot < BLE_PERIPHERAL_MAX_CONNECTIONS; slot++)  // Loop through BLE connection list
    {
        if (peripheral_connection_state[slot].connection_handle == connection_handle)
        {
            return slot;
        }
    }

    return -1;
}

/*! \brief Look up connection slot by connection handle
    \param connection_handle Connection handle
    \param remove Remove entry from lookup table if found
    \return Connection slot
 */
int ble_peripheral_connection_slot_lookup(const uint16_t connection_handle, const int remove)
{
    xSemaphoreTake(peripheral_connections_mutex, portMAX_DELAY);

    const int Slot = peripheral_find_connection(connection_handle);

    if ((Slot != -1) && remove)
    {
        peripheral_connection_state[Slot].connection_handle = INVALID_CONNECTION_HANDLE;
        bt0clients--;
    }

    xSemaphoreGive(peripheral_connections_mutex);

    return Slot;
}

/*! \brief Disconnect all peripheral connections
 */
void ble_peripheral_disconnect_all(void)
{
    xSemaphoreTake(peripheral_connections_mutex, portMAX_DELAY);
    {
        for (int slot = 0; slot < BLE_PERIPHERAL_MAX_CONNECTIONS; slot++)
        {
            if (peripheral_connection_state[slot].connection_handle != INVALID_CONNECTION_HANDLE)
            {
                ble_gap_terminate(peripheral_connection_state[slot].connection_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
        }

        xSemaphoreGive(peripheral_connections_mutex);
    }
}

/*! \brief Add a new connection slot for a peripheral connection
    \param connection_handle The connection handle to add
    \return The new connection slot, or -1 if no slot was available
 */
static int peripheral_connection_slot_add(const uint16_t connection_handle)
{
    xSemaphoreTake(peripheral_connections_mutex, portMAX_DELAY);

    const int Slot = peripheral_find_connection(INVALID_CONNECTION_HANDLE);  // find empty slot

    if (Slot != -1)
    {
        peripheral_connection_state[Slot].connection_handle = connection_handle;
        bt0clients++;
    }

    xSemaphoreGive(peripheral_connections_mutex);

    return Slot;
}

/*! \brief Print connection description
    \param desc Pointer to the connection description structure
*/
static void peripheral_print_conn_desc(struct ble_gap_conn_desc *desc)
{
    LOG(D, "handle=%d peer_id_addr_type=%d peer_id_addr=%s", desc->conn_handle, desc->peer_id_addr.type, addr_str(desc->peer_id_addr.val));
    LOG(D, "handle=%d peer_ota_addr_type=%d peer_ota_addr=%s", desc->conn_handle, desc->peer_ota_addr.type, addr_str(desc->peer_ota_addr.val));
    LOG(D, "conn_itvl=%d conn_latency=%d supervision_timeout=%d encrypted=%d authenticated=%d bonded=%d",
        desc->conn_itvl, desc->conn_latency,
        desc->supervision_timeout,
        desc->sec_state.encrypted,
        desc->sec_state.authenticated,
        desc->sec_state.bonded);
}

/*! \brief GATT attribute access callback
    \param conn_handle Connection handle
    \param attr_handle Attribute handle
    \param ctxt Pointer to the GATT access context
    \param arg Argument passed to the callback
    \return 0 on success, non-zero on failure
*/
static int ble_read_gatt_attr_cb(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    static EXT_RAM_ATTR char name[128];

    switch (error->status)
    {
    case 0:  // success
    {
        int rc;
        struct ble_gap_conn_desc desc;
        rc = ble_gap_conn_find(conn_handle, &desc);
        assert(rc == 0);

        // Read out device name
        uint16_t out_copy_len = 0;
        rc = ble_hs_mbuf_to_flat(attr->om, name, sizeof(name) - 1, &out_copy_len);
        ZERO_CHECK(rc);
        name[out_copy_len] = '\0';
        LOG(D, "Device name: %s for device(%d): %s", name, conn_handle, addr_str(desc.peer_id_addr.val));

        // Save cache data
        for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
        {
            if (ble_addr_cmp(&ble_client_info.addr[i], &desc.peer_id_addr) == 0)
            {
                // Found match
                memcpy(ble_client_info.info[i].name, name, out_copy_len);
                ble_client_info.info[i].name_len = out_copy_len;
                update_ble_client_storage();
                update_ble_client_name_in_storage(i);
                break;
            }
        }
        break;
    }

    case BLE_HS_EDONE:
        // Operation completed
        break;

    case 0x10a:  // BLE_ATT_ERR_ATTR_NOT_FOUND
        LOG(W, "No such attribute %u", error->att_handle);
        break;

    default:
        LOG(W, "Unexpected status %d", error->status);
        break;
    }
    return 0;
}

/*! \brief Peripheral GAP event handler
    \param event Pointer to the GAP event structure
    \param arg Argument passed to the handler
    \return 0
*/
static int peripheral_gap_event(struct ble_gap_event *event, void *arg)
{
    unsigned int active_advertisement = (unsigned int)arg;
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        LOG(I, "connection(%d) %s; status=%d ", active_advertisement, event->connect.status == 0 ? "established" : "failed", event->connect.status);

        const int New_connection_slot = peripheral_connection_slot_add(event->connect.conn_handle);

        if (New_connection_slot != -1)
        {
            ble_reply(BT0CLIENTS, &bt0clients, sizeof(bt0clients));
        }
        else
        {
            LOG(W, "No more available BLE peripheral connections, disconnecting!");
            // Only report as pairing failure if in active pairing mode
            if (bond_mode == BT0PAIR_OUT_PAIRING_MODE_ACTIVE)
            {
                bond_timer_stop(BT0PAIR_OUT_DEVICE_PAIR_FAILED);
            }

            ZERO_CHECK(ble_gap_terminate(event->connect.conn_handle, BLE_ERR_REM_USER_CONN_TERM));
        }

        if (event->connect.status != 0)  // connection failed; restart advertising
        {
            ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
            return 0;
        }

        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        assert(rc == 0);
        peripheral_print_conn_desc(&desc);

        struct ble_store_key_sec key_sec = {0};
        struct ble_store_value_sec value_sec = {0};
        key_sec.peer_addr = desc.peer_id_addr;

        rc = ble_store_read_peer_sec(&key_sec, &value_sec);

        if (!rc)
        {
            bBonded = true;
            LOG(I, "Device recognized!");
        }
        else
        {
#ifdef CONNECTOR_BLE_PERIPHERAL_NO_BOND_REQUIRED
            if (1)
#else
            if (bond_mode)
#endif
            {
                LOG(I, "Accepting BLE connection! (%s)", bond_mode ? "bond mode active" : "unrestricted access");
                bond_timer_stop(BT0PAIR_OUT_DEVICE_PAIRED);
                // DICM service always required security
                ble_gap_security_initiate(event->connect.conn_handle);
            }
            else
            {
                LOG(W, "Device not recognized");
                ZERO_CHECK(ble_gap_terminate(event->connect.conn_handle, BLE_ERR_REM_USER_CONN_TERM));
            }
        }

        for (unsigned int i = 0; i < ELEMENTS(Ble_peripheral_connect_cbs); ++i)
        {
            if (Ble_peripheral_connect_cbs[i] != NULL)
            {
                Ble_peripheral_connect_cbs[i](true, &desc, New_connection_slot);
            }
        }

        ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        LOG(I, "disconnect; reason=%d", event->disconnect.reason);

        // Only report as pairing failure if in active pairing mode
        if (bond_mode == BT0PAIR_OUT_PAIRING_MODE_ACTIVE)
        {
            bond_timer_stop(BT0PAIR_OUT_DEVICE_PAIR_FAILED);
        }

        const int Peripheral_connection_slot = ble_peripheral_connection_slot_lookup(event->disconnect.conn.conn_handle, 1);

        if (Peripheral_connection_slot != -1)
        {
            const int Peripheral_connector_id = connector_ble_peripheral_connector_id(Peripheral_connection_slot);
            peripheral_connection_state_t *const Pcs = &peripheral_connection_state[Peripheral_connection_slot];

            connector_send_frame_to_broker(DDMP2_CONTROL_MESSAGE, DDMP2_MESSAGE_RESET, NULL, 0, Peripheral_connector_id, portMAX_DELAY);  // Reset broker slot
       
            Pcs->connection_handle = INVALID_CONNECTION_HANDLE;

            ble_reply(BT0CLIENTS, &bt0clients, sizeof(bt0clients));
        }
        else
        {
            LOG(E, "Connection slot not found for connection %u", event->disconnect.conn.conn_handle);
        }

        xSemaphoreTake(peripheral_connections_mutex, portMAX_DELAY);
        {
            for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
            {
                if (ble_addr_cmp(&event->disconnect.conn.peer_id_addr, &ble_client_info.addr[i]) == 0)
                {
                    // Found match
                    // Check instance
                    if (ble_diconnect_cmd.remove && (ble_diconnect_cmd.index == i))
                    {
                        ble_reply(BTC0AVL | DDM2_PARAMETER_INSTANCE(ble_client_info.info[i].instance), &Zero, sizeof(Zero));
                        // Delete key from nvs
                        delete_ble_client_from_storage(i);
                        // We are to remove from cache list
                        ble_client_info.info[i].instance = -1;
                        ble_client_info.info[i].name_len = 0;
                        memset(&ble_client_info.addr[i], 0, sizeof(ble_client_info.addr[i]));
                        ble_diconnect_cmd.remove = false;
                        ble_diconnect_cmd.index = -1;
                        update_ble_client_storage();
                    }
                    else
                    {
                        const uint32_t Btc0conn_instance = BTC0CONN | DDM2_PARAMETER_INSTANCE(ble_client_info.info[i].instance);
                        ble_client_info.info[i].conn = Zero;
                        ble_reply(Btc0conn_instance, &Zero, sizeof(Zero));
                    }
                    break;
                }
            }

            xSemaphoreGive(peripheral_connections_mutex);
        }

        for (unsigned int i = 0; i < ELEMENTS(Ble_peripheral_connect_cbs); ++i)
        {
            if (Ble_peripheral_connect_cbs[i] != NULL)
            {
                Ble_peripheral_connect_cbs[i](false, NULL, Peripheral_connection_slot);
            }
        }

        ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        LOG(I, "connection updated; status=%d ", event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        peripheral_print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        if (event->adv_complete.reason == BLE_HS_ETIMEOUT)
        {
            LOG(D, "advertise complete; status=%d", event->adv_complete.reason);
        }
        else
        {
            LOG(I, "advertise complete; status=%d", event->adv_complete.reason);
        }

        ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 0);  // Restart/continue advertising
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
    {
        LOG(I, "encryption change event; status=%d ", event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        peripheral_print_conn_desc(&desc);

        if (event->enc_change.status == 0)
        {
            // Check if we have an client class for this connection. If not, we register one.
            bool found_client = false;

            for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
            {
                if ((ble_client_info.info[i].instance != -1) && (ble_addr_cmp(&desc.peer_id_addr, &ble_client_info.addr[i]) == 0))
                {
                    // Found match
                    uint32_t param = DDM2_PARAMETER_BASE_INSTANCE(BTC0CONN) | DDM2_PARAMETER_INSTANCE(ble_client_info.info[i].instance);
                    ble_client_info.info[i].conn = One;
                    ble_reply(param, &One, sizeof(One));
                    found_client = true;
                    break;
                }
            }

            if (!found_client)
            {
                // Find first available slot
                for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
                {
                    if (ble_client_info.info[i].instance == -1)
                    {
                        uint32_t param = BTC0;
                        int instance = broker_register_instance(&param, connector_ble.connector_id);
                        ble_client_info.info[i].instance = instance;
                        ble_client_info.info[i].conn = One;
                        memcpy(&ble_client_info.addr[i], &desc.peer_id_addr, sizeof(ble_addr_t));
                        param = BTC0CONN | DDM2_PARAMETER_INSTANCE(instance);
                        ble_reply(param, &One, sizeof(One));
                        num_client_info++;
                        break;
                    }
                }
            }

            ZERO_CHECK(ble_gattc_read_by_uuid(event->enc_change.conn_handle, 1, ~0, Uuid_read_device_name, ble_read_gatt_attr_cb, NULL));

            // Request updated connection parameters.
            const struct ble_gap_upd_params Gap_upd_params = {
                .itvl_max = 24,
                .itvl_min = 24,
                .latency = 0,
                .max_ce_len = 0,
                .min_ce_len = 0,
                .supervision_timeout = 400,
            };

            rc = ble_gap_update_params(event->enc_change.conn_handle, &Gap_upd_params);

            if (rc)
            {
                LOG(E, "ble_gap_update_params returned %d", rc);
            }
        }
        return 0;
    }
    case BLE_GAP_EVENT_SUBSCRIBE:
        LOG(I, "subscribe event; conn_handle=%d attr_handle=%d reason=%d prevn=%d curn=%d previ=%d curi=%d",
            event->subscribe.conn_handle,
            event->subscribe.attr_handle,
            event->subscribe.reason,
            event->subscribe.prev_notify,
            event->subscribe.cur_notify,
            event->subscribe.prev_indicate,
            event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_MTU:
        LOG(I, "mtu update event; conn_handle=%d cid=%d mtu=%d", event->mtu.conn_handle, event->mtu.channel_id, event->mtu.value);
#ifdef BLE_PERIPHERAL_PDT
        ble_peripheral_pdt_on_mtu_update_cb(event->mtu.conn_handle, event->mtu.value);
#endif
        return 0;

    case BLE_GAP_EVENT_DATA_LEN_CHG:
        LOG(D, "data len change: conn_handle=%d max_tx_oct=%d max_rx_oct=%d max_tx_time=%d max_rx_time=%d",
            event->data_len_chg.conn_handle, event->data_len_chg.max_tx_octets, event->data_len_chg.max_rx_octets,
            event->data_len_chg.max_tx_time, event->data_len_chg.max_rx_time);
        rc = ble_gap_conn_find(event->data_len_chg.conn_handle, &desc);
        peripheral_print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        LOG(I, "repeat pairing; conn_handle=%d", event->repeat_pairing.conn_handle);
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_IDENTITY_RESOLVED:
        LOG(I, "identity resolved; conn_handle=%d", event->identity_resolved.conn_handle);
        rc = ble_gap_conn_find(event->identity_resolved.conn_handle, &desc);
        assert(rc == 0);
        peripheral_print_conn_desc(&desc);
        break;

    case BLE_GAP_EVENT_NOTIFY_TX:
        LOG(D, "notify tx; conn_handle=%d indication=%d", event->notify_tx.conn_handle, event->notify_tx.indication);
        break;

#ifdef BLE_GAP_EVENT_PARING_COMPLETE
    case BLE_GAP_EVENT_PARING_COMPLETE:
        if (!event->pairing_complete.status)
        {
            LOG(I, "Pairing complete for connection %04x", event->pairing_complete.conn_handle);
        }
        else
        {
            LOG(I, "Pairing failed for connection %04x (%d)", event->pairing_complete.conn_handle, event->pairing_complete.status);
        }
        break;
#endif
#ifdef BLE_GAP_EVENT_LINK_ESTAB
    case BLE_GAP_EVENT_LINK_ESTAB:
        if (!event->link_estab.status)
        {
            LOG(I, "link established; conn_handle=%04x", event->link_estab.conn_handle);
        }
        else
        {
            LOG(W, "link establishment failed; status=%d conn_handle=%04x", event->link_estab.status, event->link_estab.conn_handle);
        }
        return 0;
#endif
    default:
        LOG(W, "Unhandled event %u", event->type);
    }

    return 0;
}

/*! \brief Handle a frame received from the broker; forward to BLE client
    \param pframe Pointer to frame
    \param connection_slot Connection slot
    \return 1 if it is handled, 0 otherwise
 */
int ble_peripheral_handle_frame(const DDMP2_FRAME *const pframe, const int connection_slot)
{
    int rc = 0;
    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_NOP:
        rc = 0;
        break;
    case DDMP2_CONTROL_GENERIC:
#ifdef BLE_PERIPHERAL_PDT
        if (ble_peripheral_pdt_is_pdt_generic_frame(pframe->frame.generic.id))
        {
            rc = ble_peripheral_pdt_handle_frame(pframe, connection_slot);
        }
#endif
        break;
    default:
        LOG(D, "Forwarding %s to BLE client %d", ddmp2_control_string(pframe->frame.control), connection_slot);
        rc = send_gatt(pframe, connection_slot);
        break;
    }
    return rc;
}

/* Get the bond mode flag
   \return The current bond mode status
 */
uint32_t ble_peripheral_get_bond_mode(void)
{
    return bond_mode;
}

/*! \brief Stop advertising
 */
void peripheral_advertise_stop(void)
{
    int rc = ble_gap_adv_stop();
    LOG(D, "Stopped advertising, %d", rc);
}

static int get_next_advertisement_set(unsigned int current_advertisement_set)
{
    current_advertisement_set++;  // Next to try
    for (; current_advertisement_set < ELEMENTS(Get_advertisement_data_functions); current_advertisement_set++)
    {
        if (Get_advertisement_data_functions[current_advertisement_set] == NULL)
        {
            continue;  // skip empty advertisement sets
        }

        return current_advertisement_set;  // return next advertisement set
    }

    return 0;  // reset to first advertisement set
}

/*! \brief Start advertising as peripheral
    \note This function will not start advertising if the advertising interval is set to 0 and bond mode is disabled.
 */
void peripheral_advertise(const int start)
{
    static unsigned int active_advertisement_set = 0;

    LOG(D, "Starting advertising set %d/%d", active_advertisement_set + 1, ELEMENTS(Get_advertisement_data_functions));

    ASSERT(Get_advertisement_data_functions[0] != NULL);
    ASSERT(Get_scan_response_data_functions[0] != NULL);

    if ((bt0advintvl == 0) && (!bond_mode))  // don't advertise if advertising interval is set to 0 and bond mode is disabled
    {
        return;
    }

    const uint32_t Advertising_interval = bond_mode ? 200 : bt0advintvl;                         // do quicker advertisements during bond mode
    const uint16_t Hci_advertising_interval = (Advertising_interval * 1000) / BLE_HCI_ADV_ITVL;  // put interval into HCI units (1.6ms)
    const struct ble_gap_adv_params Adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,  // undirected-Connectable mode
        .disc_mode = BLE_GAP_DISC_MODE_GEN,  // general discoverable mode
        .itvl_min = Hci_advertising_interval,
        .itvl_max = Hci_advertising_interval + 100,
    };

    uint8_t own_addr_type = BLE_OWN_ADDR_PUBLIC;

    ZERO_CHECK_RETURN(ble_hs_id_infer_auto(0, &own_addr_type));

    size_t advertisement_data_size = 0;
    uint8_t *advertisement_data = Get_advertisement_data_functions[active_advertisement_set](&advertisement_data_size);
    if ((advertisement_data == NULL) || (advertisement_data_size == 0))
    {
        LOG(D, "Reverting to default advertisement data");
        advertisement_data = Get_advertisement_data_functions[0](&advertisement_data_size);
    }
    ZERO_CHECK_RETURN(ble_gap_adv_set_data(advertisement_data, advertisement_data_size));
    ESP_LOG_BUFFER_HEXDUMP("bp_adv", advertisement_data, advertisement_data_size, ESP_LOG_DEBUG);

    size_t scan_response_size = 0;
    uint8_t *scan_response_data = Get_scan_response_data_functions[active_advertisement_set](&scan_response_size);
    if ((scan_response_data == NULL) || (scan_response_size == 0))
    {
        LOG(D, "Reverting to default scan response data");
        scan_response_data = Get_scan_response_data_functions[0](&scan_response_size);
    }
    ZERO_CHECK_RETURN(ble_gap_adv_rsp_set_data(scan_response_data, scan_response_size));
    ESP_LOG_BUFFER_HEXDUMP("bp_scan", scan_response_data, scan_response_size, ESP_LOG_DEBUG);

    const unsigned int Next_advertisement_set = get_next_advertisement_set(active_advertisement_set);  // get next advertisement set

    const int Advertise_duration = (Next_advertisement_set == active_advertisement_set) ? BLE_HS_FOREVER : PERIPHERAL_ADVERTISE_DURATION_MS;  // advertise forever if no advertisement data function is set, otherwise use defined duration

    const int Advertise_result = ble_gap_adv_start(own_addr_type, NULL, Advertise_duration, &Adv_params, peripheral_gap_event, (void *)active_advertisement_set);

    if (start)
    {
        if ((Advertise_result != 0) && (Advertise_result != BLE_HS_EALREADY))
        {
            LOG(E, "Error enabling advertisement; rc=%d interval=%u HCI=%u", Advertise_result, Advertising_interval, Hci_advertising_interval);
        }
        else
        {
            LOG(I, "Started advertising, interval=%u HCI=%u count=%d duration=%d", Advertising_interval, Hci_advertising_interval, ELEMENTS(Get_advertisement_data_functions), Advertise_duration);
        }
    }

    active_advertisement_set = Next_advertisement_set;
}

/*! \brief Update the BLE client storage
    \note This function compresses the stored BLE client information and saves it to NVS.
 */
static void update_ble_client_storage(void)
{
    static EXT_RAM_ATTR ble_addr_t l_ble_addr_store[MAX_NUMBER_STORED_BLE_CLIENTS];
    int num_clients = 0;
    // Update name information and ble client store
    // Compress when storing
    for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
    {
        if (ble_addr_cmp(&ble_client_info.addr[i], BLE_ADDR_ANY) != 0)
        {
            // Found match
            memcpy(&l_ble_addr_store[num_clients], &ble_client_info.addr[i], sizeof(ble_addr_t));
            num_clients++;
        }
    }

    ZERO_CHECK(nvs_set_blob(nvs_ble, "clients", l_ble_addr_store, num_clients * sizeof(ble_addr_t)));
    ZERO_CHECK(nvs_commit(nvs_ble));
}

/*! \brief Update the BLE client name in storage
    \param name_index The index of the BLE client name to update
    \note This function saves the BLE client name to NVS.
 */
static void update_ble_client_name_in_storage(int name_index)
{
    char namekey[14];
    uint8_t *p_val = ble_client_info.addr[name_index].val;
    snprintf(namekey, sizeof(namekey), "%02x%02x%02x%02x%02x%02x", p_val[0], p_val[1], p_val[2], p_val[3], p_val[4], p_val[5]);
    LOG(D, "saving %s", namekey);
    ZERO_CHECK(nvs_set_blob(nvs_ble, namekey, ble_client_info.info[name_index].name, ble_client_info.info[name_index].name_len));
    ZERO_CHECK(nvs_commit(nvs_ble));
}

/*! \brief Delete a BLE client from storage
    \param client_index The index of the BLE client to delete
    \note This function removes the BLE client information from NVS.
 */
static void delete_ble_client_from_storage(int client_index)
{
    char namekey[14];
    uint8_t *p_val = ble_client_info.addr[client_index].val;
    snprintf(namekey, sizeof(namekey), "%02x%02x%02x%02x%02x%02x", p_val[0], p_val[1], p_val[2], p_val[3], p_val[4], p_val[5]);
    LOG(D, "erasing %s", namekey);
    ZERO_CHECK(nvs_erase_key(nvs_ble, namekey));
    ZERO_CHECK(nvs_commit(nvs_ble));
}

/*! \brief GATT server registration callback
    \param ctxt Pointer to the GATT registration context
    \param arg Argument passed to the callback
*/
void gatt_server_register_cb(struct ble_gatt_register_ctxt *const ctxt, void *const arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op)
    {
    case BLE_GATT_REGISTER_OP_SVC:
        LOG(D, "Registered service %s handle=%d", ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        LOG(D, "Registering characteristic %s def_handle=%d val_handle=%d", ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf), ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        LOG(D, "Registering descriptor %s handle=%d", ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf), ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

static int install_service(const struct ble_gatt_svc_def *const service)
{
    if (service == NULL)
    {
        return -1;
    }

    if (ble_peripheral_gatt_service_count < (int)(ELEMENTS(ble_peripheral_gatt_services)))
    {
        ble_peripheral_gatt_services[ble_peripheral_gatt_service_count++] = *service;
        return 0;
    }
    else
    {
        LOG(E, "Failed to install service, max number of services reached");
        return -2;
    }
}

static void install_services(void)
{
    ble_peripheral_gatt_service_count = 0;
    int rc;

    memset(ble_peripheral_gatt_services, 0, sizeof(ble_peripheral_gatt_services));

    rc = install_service(ble_peripheral_dicm_get_service());
    LOG(I, "DICM service %sinstalled", rc ? "NOT " : "");
#ifdef BLE_PERIPHERAL_TEMPLATE
    rc = install_service(ble_peripheral_template_get_service());
    LOG(I, "Template service %sinstalled", rc ? "NOT " : "");
#endif
#ifdef BLE_PERIPHERAL_APPLICATION
    rc = install_service(ble_peripheral_application_get_service());
    LOG(I, "Application service %sinstalled", rc ? "NOT " : "");
#endif
#ifdef BLE_PERIPHERAL_PDT
    rc = install_service(ble_peripheral_pdt_get_service());
    LOG(I, "Partition Data Transfer service %sinstalled", rc ? "NOT " : "");
#endif
#ifdef CONNECTOR_BLE_PERIPHERAL_DIS
    rc = install_service(device_information_service);
    LOG(I, "Device Information service %sinstalled", rc ? "NOT " : "");

#endif
    LOG(I, "Total number of services installed: %d", ble_peripheral_gatt_service_count);
};

/*! \brief Initialize the GATT server; Add GAP and GATT services, read stored BLE client information from NVS
    \return 0 on success, or a non-zero error code on failure
 */
int gatt_server_initialize(void)
{
    // ble_svc_dis_init();

    // ble_svc_dis_model_number_set("1.0");
    // ble_peripheral_set_manufacturer_name("Tässätuote Oy");

    install_services();

    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc;

    rc = ble_gatts_count_cfg(ble_peripheral_gatt_services);

    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(ble_peripheral_gatt_services);

    if (rc != 0)
    {
        return rc;
    }

    // Init storage
    memset(&ble_client_info, 0, sizeof(ble_client_info));

    for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
    {
        ble_client_info.info[i].instance = -1;
    }

    num_client_info = 0;
    ble_diconnect_cmd.remove = false;
    // Read from nvs if we have stored information
    size_t blob_size = sizeof(ble_client_info.addr);
    rc = nvs_get_blob(nvs_ble, "clients", ble_client_info.addr, &blob_size);

    if (rc != ESP_ERR_NVS_NOT_FOUND)
    {
        for (int i = 0; (i < MAX_NUMBER_STORED_BLE_CLIENTS) && (i < (int)(blob_size / sizeof(ble_addr_t))); ++i)
        {
            // Can we also find a corresponding name
            size_t key_size = 0;
            char namekey[14];
            uint8_t *p_val = ble_client_info.addr[i].val;
            snprintf(namekey, sizeof(namekey), "%02x%02x%02x%02x%02x%02x", p_val[0], p_val[1], p_val[2], p_val[3], p_val[4], p_val[5]);
            rc = nvs_get_blob(nvs_ble, namekey, NULL, &key_size);

            ZERO_CHECK(rc);

            if (rc)
            {
                LOG(E, "namekey - %s data - read: %d", namekey, key_size);
            }

            if (key_size > sizeof(ble_client_info.info[i].name))
            {
                LOG(W, "Too short local storage");
            }

            ZERO_CHECK(nvs_get_blob(nvs_ble, namekey, ble_client_info.info[i].name, &key_size));

            if (rc)
            {
                LOG(E, "namekey - %s data - read: %d", namekey, key_size);
            }

            ble_client_info.info[i].name_len = key_size;
            // Create classes
            uint32_t param = BTC0;
            int instance = broker_register_instance(&param, connector_ble.connector_id);
            ble_client_info.info[i].instance = instance;
            ble_client_info.info[i].conn = Zero;
            param = DDM2_PARAMETER_BASE_INSTANCE(BTC0CONN) | DDM2_PARAMETER_INSTANCE(instance);
            ble_reply(param, &Zero, sizeof(Zero));
            num_client_info++;
        }
    }

    return 0;
}

void ble_peripheral_initialize(void)
{
    peripheral_connections_mutex = xSemaphoreCreateMutex();

    for (int i = 0; i < BLE_PERIPHERAL_MAX_CONNECTIONS; i++)
    {
        peripheral_connection_state_t *const Pcs = &peripheral_connection_state[i];

        memset(Pcs, 0, sizeof(peripheral_connection_state_t));

        Pcs->connection_handle = INVALID_CONNECTION_HANDLE;
    }
#ifdef BLE_PERIPHERAL_PDT
    ble_peripheral_pdt_init();
#endif
}

void ble_peripheral_handle_clients(const DDMP2_FRAME *const pframe)
{
    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_SUBSCRIBE:
        for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
        {
            if (ble_client_info.info[i].instance == (int)DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.subscribe.parameter))
            {
                switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter))
                {
                case BTC0NAME:
                    ble_reply(pframe->frame.subscribe.parameter, ble_client_info.info[i].name, ble_client_info.info[i].name_len);
                    break;

                case BTC0CONN:
                    ble_reply(pframe->frame.subscribe.parameter, &ble_client_info.info[i].conn, sizeof(int32_t));
                    break;

                default:
                    break;
                }
                break;
            }
        }
        break;

    case DDMP2_CONTROL_SET:
        for (int i = 0; i < MAX_NUMBER_STORED_BLE_CLIENTS; ++i)
        {
            if (ble_client_info.info[i].instance == (int)DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.set.parameter))
            {
                ble_addr_t l_addr = ble_client_info.addr[i];

                int rc = ble_gap_conn_find_by_addr(&ble_client_info.addr[i], NULL);

                if (rc != 0)
                {
                    // No connection with this device, so it is safe to remove this from structure here
                    // Remove class and entry in store
                    delete_ble_client_from_storage(i);
                    ble_reply(BTC0AVL | DDM2_PARAMETER_INSTANCE(ble_client_info.info[i].instance), &Zero, sizeof(Zero));
                    ble_client_info.info[i].instance = -1;
                    memset(&ble_client_info.addr[i], 0, sizeof(ble_addr_t));
                    ble_client_info.info[i].name_len = 0;
                    update_ble_client_storage();
                }
                else
                {
                    ble_diconnect_cmd.remove = true;
                    ble_diconnect_cmd.index = i;
                }
                // Delete bonding information and disconnect
                rc = ble_gap_unpair(&l_addr);
                ZERO_CHECK(rc);
                break;
            }
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Unregister the btc class.
 */
void btc_class_unregister(void)
{
    uint32_t param;
    int instance;

    for (int i = 0; i < num_client_info; i++)
    {
        instance = i;

        param = DDM2_PARAMETER_BASE_INSTANCE(BTC0CONN) | DDM2_PARAMETER_INSTANCE(instance);
        ble_reply(param, &Zero, sizeof(Zero));

        param = DDM2_PARAMETER_BASE_INSTANCE(BTC0) | DDM2_PARAMETER_INSTANCE(instance);
        connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, param,  NULL, 0, connector_ble.connector_id, (TickType_t)portMAX_DELAY);
    }
}
