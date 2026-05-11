/*! \file ble_peripheral_dicm.c
 *  \brief Implementation of the DICM peripheral service.
 *  \author Jens Björnhager
 *
 *  This file contains the implementation of the DICM peripheral service.
 */

#include "ble_peripheral_dicm.h"
#include "ble_state_machine.h"
#include "bluetooth_le.h"
#include "configuration.h"
#include "connector.h"
#include "connector_ble.h"
#include "host/ble_hs.h"
#include "peripheral_role.h"
#include "services/dis/ble_svc_dis.h"
#include "services/gap/ble_svc_gap.h"

#define NOTIFICATION_OVERFLOW_MAX_DRAINS_PER_CALLBACK 10

//! \~ Create UUID derived from base UUID 537axxxx-0995-481f-926c-1604e23fd515
//clang-format off
#define DICM_UUID(x) 0x15, 0xd5, 0x3f, 0xe2, 0x04, 0x16, 0x6c, 0x92, 0x1f, 0x48, 0x95, 0x09, (((unsigned int)(x)) & 0xff), (((unsigned int)(x)) >> 8), 0x7a, 0x53,
//clang-format on

const ble_uuid128_t Dicm_service_uuid = BLE_UUID128_INIT(DICM_UUID(UUID_SERVICE));
const ble_uuid128_t Dicm_input_uuid = BLE_UUID128_INIT(DICM_UUID(UUID_INPUT));
const ble_uuid128_t Dicm_output_uuid = BLE_UUID128_INIT(DICM_UUID(UUID_OUTPUT));

uint16_t ble_peripheral_dicm_input_handle;
uint16_t ble_peripheral_dicm_output_handle;

static char local_adv_name[15];  //! \~ Local string of GAP advertise name
static int local_adv_name_set;   //! \~ Indicates whether a local name is set for the advertising name

static int ble_peripheral_dicm_gatt_access_cb(uint16_t connection_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

//! \~ Raw GAP advertisement data (max 31 bytes)
static DICM_ADVERTISEMENT_DATA dicm_advertisement_data = {
    .flags = {
        .length = sizeof(GAP_FLAGS) - 1,
        .type = GAP_FIELD_FLAGS,
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
    },
    .manufacturer_data = {
        .length = 0,
        .type = GAP_FIELD_MANUFACTURER_DATA,
        .company_id = BLE_DOMETIC_ID,
        .product_type = DEFAULT_PRODUCT_ID,
        .mac = {
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
        },
        .sku = {
            0,
        },
    },
};

//! \~ Raw GAP scan response data (max 31 bytes)
static DICM_SCAN_RESPONSE_DATA dicm_scan_response_data = {
    .manufacturer_data = {
        .length = sizeof(DICM_SCAN_RESPONSE_MANUFACTURER_DATA) - 1,
        .type = GAP_FIELD_MANUFACTURER_DATA,
        .company_id = BLE_DOMETIC_ID,
        .status_flag = 0x00,  // filled in dynamically
    },
    .local_name = {
        .length = 0,  // filled in dynamically
        .type = GAP_FIELD_LOCAL_NAME,
        .local_name = {
            0,
        },  // filled in dynamically
    },
};

const struct ble_gatt_svc_def ble_peripheral_dicm_service = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &Dicm_service_uuid.u,
    .characteristics = (struct ble_gatt_chr_def[]){
        {
            .uuid = &Dicm_input_uuid.u,
            .access_cb = ble_peripheral_dicm_gatt_access_cb,
            .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
            .val_handle = &ble_peripheral_dicm_input_handle,
        },
        {
            .uuid = &Dicm_output_uuid.u,
            .access_cb = ble_peripheral_dicm_gatt_access_cb,
            .flags = BLE_GATT_CHR_F_NOTIFY,
            .val_handle = &ble_peripheral_dicm_output_handle,
        },
        {
            0,
        },
    },
};

/*! \brief Handle incoming GATT frame; forward to broker
    \param pframe Pointer to the DDM frame to process
    \param connection_handle The connection handle for the incoming frame
*/
void handle_incoming_gatt_frame(const DDMP2_FRAME *const pframe, const uint16_t connection_handle)
{
    TRUE_CHECK_RETURN(pframe);

    DECLARE_DDMP2_MESSAGE_FRAME(Ping_reply, DDMP2_MESSAGE_PINGREPLY, pframe->source_connector);
    const int Connection_slot = connector_ble_peripheral_connection_slot(pframe->source_connector);

    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_MESSAGE:
        switch (pframe->frame.message.id)  // which message id?
        {
        case DDMP2_MESSAGE_RESET:  // reset message, DICM has just started up
            LOG(I, "Forwarding reset to broker from BLE connector:%d", Connection_slot);
            connector_forward_frame_to_broker(pframe);
            break;

        case DDMP2_MESSAGE_PING:  // reply to ping message
            LOG(D, "Replying to ping message from BLE connector:%d", Connection_slot);
            send_gatt((DDMP2_FRAME *)&Ping_reply, Connection_slot);
            break;

        case DDMP2_MESSAGE_ERROR:
            LOG(W, "BLE Protocol error, BLE connector:%d", Connection_slot);
            break;
        }
        break;

    case DDMP2_CONTROL_NOP:
        break;

    default:
        LOG(D, "Forwarding %s from BLE:%d to broker", ddmp2_control_string(pframe->frame.control), connector_ble_connector_slot(pframe->source_connector));
        TRUE_CHECK(connector_forward_frame_to_broker(pframe));
        break;
    }
}

uint8_t *ble_peripheral_dicm_get_advertisement_data(size_t *const data_size)
{
    const size_t Sku_size = strlen((char *)gw0sku);
    dicm_advertisement_data.manufacturer_data.length = sizeof(dicm_advertisement_data.manufacturer_data) - sizeof(dicm_advertisement_data.manufacturer_data.sku) + Sku_size - 1;
    memcpy(&dicm_advertisement_data.manufacturer_data.sku, gw0sku, Sku_size);
    memcpy(&dicm_advertisement_data.manufacturer_data.mac, device_information.id, sizeof(dicm_advertisement_data.manufacturer_data.mac));

    *data_size = sizeof(dicm_advertisement_data) - sizeof(dicm_advertisement_data.manufacturer_data.sku) + Sku_size;
    return (uint8_t *)&dicm_advertisement_data;
}

uint8_t *ble_peripheral_dicm_get_scan_response_data(size_t *const data_size)
{
    if (!local_adv_name_set)  // is local advertising name set?
    {                         // no, use default
        dicm_scan_response_data.local_name.length = MIN(strlen(device_information.default_name) + 1, sizeof(dicm_scan_response_data.local_name.local_name));
        memcpy(&dicm_scan_response_data.local_name.local_name, device_information.default_name, dicm_scan_response_data.local_name.length - 1);
    }
    else
    {  // yes, use local name
        dicm_scan_response_data.local_name.length = MIN(strlen(local_adv_name) + 1, sizeof(dicm_scan_response_data.local_name.local_name));
        memcpy(&dicm_scan_response_data.local_name.local_name, local_adv_name, dicm_scan_response_data.local_name.length - 1);
    }

    dicm_scan_response_data.local_name.local_name[dicm_scan_response_data.local_name.length - 1] = '\0';
    ble_svc_gap_device_name_set((char *)dicm_scan_response_data.local_name.local_name);  // Set the GAP service device name to the advertising name

    dicm_scan_response_data.manufacturer_data.status_flag = gateway_advertisement_flag_byte();

    *data_size = sizeof(dicm_scan_response_data) - sizeof(dicm_scan_response_data.local_name.local_name) + dicm_scan_response_data.local_name.length - 1;

    return (uint8_t *)&dicm_scan_response_data;
}

struct ble_gatt_svc_def *ble_peripheral_dicm_get_service(void)
{
    ble_svc_dis_manufacturer_name_set("Dometic");

    return (struct ble_gatt_svc_def *)&ble_peripheral_dicm_service;
}

/*! \brief Try to send a notification to a connected client from the overflow buffer
    \param peripheral_slot The peripheral connection slot to send the notification to
    \param data Pointer to the data to send
    \param data_size The size of the data to send
    \return SEND_NOTIFICATION_RESULT_ENUM
*/
static send_notification_result_enum_t send_notification_direct(const int peripheral_slot, const void *const data, const size_t data_size)
{
    TRUE_CHECK_RETURNX(SEND_NOTIFICATION_RESULT_INVALID_PARAMETER, data != NULL);
    TRUE_CHECK_RETURNX(SEND_NOTIFICATION_RESULT_INVALID_PARAMETER, data_size);

    const peripheral_connection_state_t *const Pcs = &peripheral_connection_state[peripheral_slot];

    struct os_mbuf *mbuf;
    mbuf = ble_hs_mbuf_from_flat(data, data_size);  // create memory buffer from data

    if (mbuf == NULL)
    {
        return SEND_NOTIFICATION_RESULT_MBUF_ALLOCATION_FAILED;
    }

    LOG(D, "Sending notification to peripheral %d handle %d, data size %zu", peripheral_slot, Pcs->connection_handle, data_size);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    const int Notify_result = ble_gatts_notify_custom(Pcs->connection_handle, ble_peripheral_dicm_output_handle, mbuf);  // Notify the client
#else
    const int Notify_result = ble_gattc_notify_custom(Pcs->connection_handle, ble_peripheral_dicm_output_handle, mbuf);  // Notify the client (deprecated)
#endif

    return Notify_result == 0 ? SEND_NOTIFICATION_RESULT_SUCCESS : SEND_NOTIFICATION_RESULT_NOTIFICATION_FAILED;
}

/*! \brief Send a notification to a connected client, buffer if BLE stack memory is exhausted
    \param peripheral_slot The peripheral connection slot to send the notification to
    \param data Pointer to the data to send
    \param data_size The size of the data to send
    \return send_notification_result_enum_t
    \warning LOCKING: Caller must hold peripheral_connection_state[connection_slot].notification_mutex
*/
static send_notification_result_enum_t send_notification(const int peripheral_slot, const void *const data, const size_t data_size)
{
    const send_notification_result_enum_t Notification_result = send_notification_direct(peripheral_slot, data, data_size);  // Try to send notification

    return Notification_result;
}

/*! \brief Send a notification frame to a connected client
    \param connection_handle The connection handle to send the notification to
    \param pframe Pointer to the DDM frame to send
    \warning LOCKING: Caller must hold peripheral_connection_state[connection_slot].notification_mutex
*/
static void send_notification_frame(const int peripheral_slot, const DDMP2_FRAME *const pframe)
{
    send_notification(peripheral_slot, &pframe->frame, pframe->frame_size);
}

/*! \brief Send GATT data directly to client, or aggregate if activated on connection
    \param pframe Pointer to the DDM frame to send
    \param connection_slot The connection slot to send the data on
    \return 1 on success, 0 on failure
*/
int send_gatt(const DDMP2_FRAME *const pframe, const int connection_slot)
{
    TRUE_CHECK_RETURN0(pframe);

    peripheral_connection_state_t *const Pcs = &peripheral_connection_state[connection_slot];

    if (Pcs->connection_handle == INVALID_CONNECTION_HANDLE)
    {
        LOG(W, "Connection handle invalid");

        return 0;
    }

    send_notification_frame(connection_slot, pframe);

    return 1;
}

/*! \brief Update the advertising name
    \param advertising_name Pointer to the new advertising name
 */
void ble_peripheral_dicm_update_advertising_name(const char *advertising_name)
{
    strncpy(local_adv_name, advertising_name, 15);
    local_adv_name[14] = '\0';
    local_adv_name_set = 1;

    ble_state_machine_generate_event(BLE_SM_ADV_RESTART_EVENT, 1);
}

/*! \brief GATT access callback for peripheral
    \param connection_handle The connection handle
    \param attr_handle The attribute handle
    \param ctxt Pointer to the GATT access context
    \param arg Argument passed to the callback
    \return 0 on success, or a BLE error code on failure
*/
static int ble_peripheral_dicm_gatt_access_cb(uint16_t connection_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid = ctxt->chr->uuid;
    const uint8_t *uuid128 = BLE_UUID128(uuid)->value;
    uint16_t data_length;
    int rc;
    DDMP2_FRAME frame;
    static EXT_RAM_ATTR uint8_t input[PERIPHERAL_AGGREGATION_BUFFER_SIZE];

    switch (uuid128[12])
    {
    case 0x01:  // Input characteristic
        switch (ctxt->op)
        {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            rc = gatt_svr_chr_write(ctxt->om, 1, sizeof(input), input, &data_length);

            if (rc != 0)
            {
                return rc;
            }

            const int Connection_slot = ble_peripheral_connection_slot_lookup(connection_handle, 0);
            TRUE_CHECK_RETURNX(BLE_ATT_ERR_UNLIKELY, Connection_slot != -1);

            if (Connection_slot == -1)
            {
                LOG(E, "Connection slot not found for connection %u", connection_handle);
            }

            ESP_LOG_BUFFER_HEXDUMP("BLE", input, data_length, ESP_LOG_DEBUG);

            {
                const int Peripheral_connector_id = connector_ble_peripheral_connector_id(Connection_slot);

                if (!ddmp2_create_raw_frame(&frame, input, data_length, Peripheral_connector_id))
                {
                    LOG(W, "Failed to create raw frame from GATT data!");
                    ESP_LOG_BUFFER_HEXDUMP("BLE", input, data_length, ESP_LOG_WARN);
                    return
#ifdef BLE_ATT_ERR_VALUE_NOT_ALLOWED
                        BLE_ATT_ERR_VALUE_NOT_ALLOWED;
#else
                        BLE_ATT_ERR_UNLIKELY;
#endif
                };

                handle_incoming_gatt_frame(&frame, connection_handle);
            }

            return 0;

        default:
            ASSERT(0);
            return BLE_ATT_ERR_UNLIKELY;
        }
        break;

    case 0x02:  // Output characteristic
        LOG(E, "Output: %02x", ctxt->op);
        break;

    default:
        LOG(E, "Unknown %02x: %02x", uuid128[12], ctxt->op);
    }

    return 0;
}
