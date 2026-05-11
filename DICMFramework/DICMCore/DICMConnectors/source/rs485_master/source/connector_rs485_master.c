/*! \file connector_rs485_master.c
    \brief DDMP2 RS485 master connector with Modbus RTU support
*/

#include "configuration.h"

// System includes
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
// IDF includes
#include "esp_idf_version.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#include "driver/uart.h"  // UART port and configuration definition
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/myuart.h"  // UART port and configuration definition
#else
#include "driver/uart.h"  // UART port and configuration definition
#endif
#include "driver/gpio.h"
#include "esp_modbus_master.h"
// Framework includes
#include "connector.h"
#include "ddm2.h"
#include "product_database.h"
// Current module includes
#include "connector_rs485_master.h"
#include "rs485_modbus_master.h"
#include "rs485_modbus_master_battery.h"

#define RS485_POLL_INTERVAL_MS          1000
#define RS485_CONNECTOR_TASK_STACK_SIZE 4096

typedef struct
{
    rs485_battery_cid_t cid;
    uint8_t value[RS485_PARAM_VALUE_SIZE_U32];
    uint8_t type;
    uint8_t size;
    bool is_valid;
    bool has_changed;
} rs485_param_cache_t;

// Cache indexed by slave address (ADDR_OFFSET(addr)), where addr is 1-20
// Use RS485_SLAVE_ADDR_MAX (20) to match s_slave_status array size (RS485_SLAVE_ADDR_MAX = 20)
static EXT_RAM_ATTR rs485_param_cache_t s_param_cache[RS485_SLAVE_ADDR_MAX][BATTERY_CID_MAX];

/**
 * @brief Parse DDM2 parameter to Modbus information
 * Extracts instance, converts to CID, gets slave address, and optionally gets descriptor
 * @param ddm2_param DDM2 parameter ID (with instance)
 * @param[out] cid Pointer to store Modbus CID
 * @param[out] addr Pointer to store slave address
 * @param[out] desc_ptr Pointer to store descriptor pointer (can be NULL)
 * @return esp_err_t ESP_OK on success, error code on failure
 */
static esp_err_t parse_ddm2_param_to_modbus_info(uint32_t ddm2_param, rs485_battery_cid_t *cid, uint8_t *addr, const void **desc_ptr)
{
    if (cid == NULL || addr == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert DDM2 parameter to Modbus CID
    esp_err_t err = rs485_battery_ddm2_param_to_cid(ddm2_param, cid);
    if (err != ESP_OK)
    {
        return ESP_ERR_NOT_FOUND;
    }

    err = rs485_battery_ddm2_param_to_addr(ddm2_param, addr);
    if (err != ESP_OK)
    {
        return err;
    }

    err = rs485_modbus_master_get_cid_info(*cid, desc_ptr);
    if (err != ESP_OK)
    {
        return err;
    }
    // Validate final slave address
    if (*addr == 0 || *addr > RS485_SLAVE_ADDR_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/**
 * @brief Calculate and publish MBAT0CAPREL parameter
 * based on the cached SOC and SOH values for the slave device.
 * @param ddm2_param DDM2 parameter ID (with instance)
 * @param control    DDM2 Control enum
 */
static void calculate_mbatxcaprel_param(uint32_t ddm2_param, const DDMP2_CONTROL_ENUM control)
{
    uint32_t base_param = DDM2_PARAMETER_BASE_INSTANCE(ddm2_param);
    uint8_t instance = DDM2_PARAMETER_INSTANCE_PART(ddm2_param) >> 8;

    if (base_param == MBAT0CAPREL && control == DDMP2_CONTROL_SUBSCRIBE)
    {
        LOG(D, "Subscribe Parameter: MBATxCAPREL");
    }
    else if ((base_param == MBAT0SOC && control == DDMP2_CONTROL_PUBLISH) || (base_param == MBAT0SOH && control == DDMP2_CONTROL_PUBLISH))
    {
        LOG(D, "Publish Parameter: MBATxCAPREL");
    }
    else
    {
        LOG(D, "Parameter 0x%08x is not match, cannot calculate MBATxCAPREL", ddm2_param);
        return;
    }

    uint8_t addr;
    uint8_t param_size = 0;
    rs485_battery_cid_t soc_cid;
    rs485_battery_cid_t soh_cid;

    esp_err_t err = rs485_battery_ddm2_param_to_addr(ddm2_param, &addr);
    if (err != ESP_OK)
    {
        return;
    }
    err = rs485_battery_ddm2_param_to_cid(MBAT0SOC, &soc_cid);
    if (err != ESP_OK)
    {
        return;
    }
    err = rs485_battery_ddm2_param_to_cid(MBAT0SOH, &soh_cid);
    if (err != ESP_OK)
    {
        return;
    }
    rs485_param_cache_t mbatxsoc = s_param_cache[ADDR_OFFSET(addr)][soc_cid];
    rs485_param_cache_t mbatxsoh = s_param_cache[ADDR_OFFSET(addr)][soh_cid];

    // Calculate MBATxCAPREL value
    // Convert rs485_battery value to DDM2 value (in-place conversion on temp buffer)
    err = rs485_battery_value_to_ddm2_value(soc_cid, mbatxsoc.value, &param_size);
    if (err != ESP_OK)
    {
        return;
    }
    err = rs485_battery_value_to_ddm2_value(soh_cid, mbatxsoh.value, &param_size);
    if (err != ESP_OK)
    {
        return;
    }
    int32_t mbatxcaprel_value = *(int32_t *)mbatxsoc.value * *(int32_t *)mbatxsoh.value / 100;
    // Publish MBATxCAPREL value
    bool result = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                 MBAT0CAPREL | DDM2_PARAMETER_INSTANCE(instance),
                                                 &mbatxcaprel_value,
                                                 param_size,
                                                 connector_rs485_master.connector_id,
                                                 portMAX_DELAY);
    if (result)
    {
        LOG(D, "Published DDM2: 0x%08x, value: %d", MBAT0CAPREL | DDM2_PARAMETER_INSTANCE(instance), mbatxcaprel_value);
    }
    else
    {
        LOG(W, "Failed to publish DDM2: 0x%08x, value: %d", MBAT0CAPREL | DDM2_PARAMETER_INSTANCE(instance), mbatxcaprel_value);
    }
}

/**
 * @brief MBATxCHGST and MBATxDISCHGST don't match the parameters of the RS485 protocol.
 * Their values need to be assigned to them based on the values of MBATxCHGDET.
 * @param ddm2_param DDM2 parameter
 * @param control    DDM2 Control enum
 */
static void calculate_chgst_dischgst_param(uint32_t ddm2_param, const DDMP2_CONTROL_ENUM control)
{
    uint32_t base_param = DDM2_PARAMETER_BASE_INSTANCE(ddm2_param);
    if ((base_param == MBAT0CHGST && control == DDMP2_CONTROL_SUBSCRIBE) || (base_param == MBAT0DISCHGST && control == DDMP2_CONTROL_SUBSCRIBE))
    {
        LOG(D, "Receive Subscribe Parameter: MBATxCHGST or MBATxDISCHGST, will Publish:0x%08x", ddm2_param);
    }
    else if (base_param == MBAT0CHGDET && control == DDMP2_CONTROL_PUBLISH)
    {
        LOG(D, "Publish Parameter: MBATxCHGST and MBATxDISCHGST");
    }
    else
    {
        LOG(D, "Parameter 0x%08x is not match, cannot calculate MBATxCHGST or MBATxDISCHGST", ddm2_param);
        return;
    }

    uint8_t addr;
    rs485_battery_cid_t cid;

    esp_err_t err = rs485_battery_ddm2_param_to_addr(ddm2_param, &addr);
    if (err != ESP_OK)
    {
        return;
    }
    err = rs485_battery_ddm2_param_to_cid(MBAT0CHGDET, &cid);
    if (err != ESP_OK)
    {
        return;
    }

    rs485_param_cache_t mbatxchgdet = s_param_cache[ADDR_OFFSET(addr)][cid];

    uint16_t battery_work_status = *(uint16_t *)mbatxchgdet.value;
    uint8_t instance = DDM2_PARAMETER_INSTANCE_PART(ddm2_param) >> 8;

    if (battery_work_status > 2)
    {
        LOG(D, "battery_work_status value error:%d", battery_work_status);
        return;
    }

    int32_t charging = false;
    int32_t discharging = false;
    switch (battery_work_status)
    {
    case 0:  // Idle mode
        charging = false;
        discharging = false;
        break;
    case 1:  // Charging mode
        charging = true;
        discharging = false;
        break;
    case 2:  // Discharging mode
        charging = false;
        discharging = true;
        break;
    }

    // Publish to Broker
    if ((base_param == MBAT0CHGST && control == DDMP2_CONTROL_SUBSCRIBE) || (control == DDMP2_CONTROL_PUBLISH))
    {
        bool result = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                     MBAT0CHGST | DDM2_PARAMETER_INSTANCE(instance),
                                                     &charging,
                                                     sizeof(int32_t),
                                                     connector_rs485_master.connector_id,
                                                     portMAX_DELAY);
        if (result)
        {
            LOG(D, "Published DDM2: 0x%08x", MBAT0CHGST | DDM2_PARAMETER_INSTANCE(instance));
        }
        else
        {
            LOG(W, "Failed to publish DDM2: 0x%08x to broker", MBAT0CHGST | DDM2_PARAMETER_INSTANCE(instance));
        }
    }
    // Publish to Broker
    if ((base_param == MBAT0DISCHGST && control == DDMP2_CONTROL_SUBSCRIBE) || (control == DDMP2_CONTROL_PUBLISH))
    {
        bool result = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                     MBAT0DISCHGST | DDM2_PARAMETER_INSTANCE(instance),
                                                     &discharging,
                                                     sizeof(int32_t),
                                                     connector_rs485_master.connector_id,
                                                     portMAX_DELAY);
        if (result)
        {
            LOG(D, "Published DDM2: 0x%08x", MBAT0DISCHGST | DDM2_PARAMETER_INSTANCE(instance));
        }
        else
        {
            LOG(W, "Failed to publish DDM2: 0x%08x to broker", MBAT0DISCHGST | DDM2_PARAMETER_INSTANCE(instance));
        }
    }
}

/**
 * @brief Modbus parameter update callback
 * Called by rs485_modbus_master when parameters are polled.
 * This function:
 * 1. Updates parameter cache (indexed by addr and cid)
 * 2. Detects value changes
 * 3. Publishes changed values to Broker with correct instance
 * @param cid Characteristic ID
 * @param addr Modbus slave address (1-247)
 * @param value Pointer to parameter value buffer
 * @param size Size of parameter value in bytes
 * @param type Parameter type
 */
static void modbus_update_cb(uint16_t cid, uint8_t addr, const uint8_t *value, uint8_t size, uint8_t type)
{
    if (cid >= BATTERY_CID_MAX || addr == 0 || addr > RS485_SLAVE_ADDR_MAX)
    {
        return;
    }

    rs485_battery_cid_t battery_cid = (rs485_battery_cid_t)cid;
    rs485_param_cache_t *cache = &s_param_cache[ADDR_OFFSET(addr)][cid];

    // Check if value has changed using encapsulated function
    bool value_changed = rs485_battery_check_value_changed(battery_cid, cache->value, value, size, &cache->is_valid);

    if (value_changed)
    {
        // Update cache
        memcpy(cache->value, value, size);
        cache->type = type;
        cache->size = size;
        cache->cid = battery_cid;
        cache->has_changed = true;

        // Convert CID to DDM2 parameter ID with instance
        uint32_t ddm2_param = rs485_battery_cid_to_ddm2_param(battery_cid, s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance);
        if (ddm2_param != 0)
        {
            // Publish to Broker using encapsulated function
            if (rs485_battery_publish_status(battery_cid, ddm2_param, value, size, connector_rs485_master.connector_id))
            {
                LOG(D, "Published CID %d from slave %d (DDM2: 0x%08x, instance %d) value changed",
                    battery_cid, addr, ddm2_param, s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance);
                cache->has_changed = false;  // Clear change flag after publish
            }
            else
            {
                LOG(W, "Failed to publish CID %d from slave %d to broker (queue full?)", battery_cid, addr);
            }
            // Special handling for MBATxCAPREL calculation
            calculate_mbatxcaprel_param(ddm2_param, DDMP2_CONTROL_PUBLISH);
            //  Special handling for MBATxCHGST & MBATxDISCHGST
            calculate_chgst_dischgst_param(ddm2_param, DDMP2_CONTROL_PUBLISH);
        }
        else
        {
            LOG(W, "No DDM2 mapping for CID %d", battery_cid);
        }
    }
}

/**
 * @brief Handle SUBSCRIBE command from Broker
 * When a parameter is subscribed, we should immediately publish its current value
 * Supports multi-slave by extracting instance from DDM2 parameter
 */
static void handle_subscribe_command(const DDMP2_FRAME *pframe)
{
    uint32_t ddm2_param = pframe->frame.subscribe.parameter;
    rs485_battery_cid_t cid;
    uint8_t addr;
    const void *desc_ptr = NULL;
    esp_err_t err;

    // Parse DDM2 parameter to get CID, slave address, and descriptor
    err = parse_ddm2_param_to_modbus_info(ddm2_param, &cid, &addr, &desc_ptr);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NOT_FOUND)  // The DDM parameter doesn't directly match the 485 parameter.
        {
            LOG(D, "No Modbus CID mapping for DDM2 parameter 0x%08x", DDM2_PARAMETER_BASE_INSTANCE(ddm2_param));
            // Special handling for MBATxLINKED
            calculate_mbatxlinked_mbatxtype_param(ddm2_param);
            // Special handling for MBATxCAPREL calculation
            calculate_mbatxcaprel_param(ddm2_param, DDMP2_CONTROL_SUBSCRIBE);
            // Special handling for MBATxCHGST & MBATxDISCHGST
            calculate_chgst_dischgst_param(ddm2_param, DDMP2_CONTROL_SUBSCRIBE);
        }
        else
        {
            LOG(W, "Cannot determine slave address for CID %d (DDM2: 0x%08x)", cid, ddm2_param);
        }
        return;
    }

    if (cid < BATTERY_CID_MAX && addr >= RS485_SLAVE_ADDR_MIN && addr <= RS485_SLAVE_ADDR_MAX)
    {
        rs485_param_cache_t *cache = &s_param_cache[ADDR_OFFSET(addr)][cid];
        if (cache->is_valid)
        {
            // Publish cached value using encapsulated function
            if (rs485_battery_publish_status(cid, ddm2_param, cache->value, cache->size, connector_rs485_master.connector_id))
            {
                LOG(D, "Published cached value for CID %d from slave %d (DDM2: 0x%08x, instance %d) on subscribe",
                    cid, addr, ddm2_param, s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance);
            }
        }
        else
        {
            return;  // No valid cached value to publish
        }
    }
}

/**
 * @brief connector rs485 master process task
 * Handles incoming frames from Broker.
 * @param Parameter not used
 * @return void
 */
static void connector_rs485_master_process_task(void *Parameter)
{
    DDMP2_FRAME *pframe;
    size_t frame_size;

    while (1)
    {
        TRUE_CHECK(pframe = xRingbufferReceive(connector_rs485_master.to_connector, &frame_size, portMAX_DELAY));

        if (ProdDBFrameHandler(pframe))
        {
            // Do nothing
        }
        else
        {
            switch (pframe->frame.control)
            {
            case DDMP2_CONTROL_SET:
                // There is no control requirement.
                break;

            case DDMP2_CONTROL_SUBSCRIBE:
                handle_subscribe_command(pframe);
                break;

            case DDMP2_CONTROL_PUBLISH:
                break;

            default:
                LOG(W, "RS485 master connector received UNHANDLED frame %02x from broker!", pframe->frame.control);
                break;
            }
        }
        vRingbufferReturnItem(connector_rs485_master.to_connector, pframe);
    }
}

/**
 * @brief init connector rs485 master device
 * @param void
 * @return 0 on failure, 1 on success
 */
static int initialize_connector_rs485_master(void)
{
    esp_err_t err;
    const void *descriptor_table = NULL;
    uint16_t num_elements = 0;
    err = rs485_modbus_master_init();
    if (err != ESP_OK)
    {
        LOG(E, "Failed to initialize Modbus Master: %s", esp_err_to_name(err));
        return 0;
    }
    esp_log_level_set("MB_CONTROLLER_MASTER", ESP_LOG_NONE);

    // Get battery parameter descriptor table
    err = rs485_battery_get_descriptor_table(&descriptor_table, &num_elements);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to get battery descriptor table: %s", esp_err_to_name(err));
        rs485_modbus_master_deinit();
        return 0;
    }

    // Set Modbus parameter descriptor table
    err = rs485_modbus_master_set_descriptor(descriptor_table, num_elements);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to set Modbus descriptor: %s", esp_err_to_name(err));
        rs485_modbus_master_deinit();
        return 0;
    }
    // Start Modbus Master
    err = rs485_modbus_master_start();
    if (err != ESP_OK)
    {
        LOG(E, "Failed to start Modbus Master: %s", esp_err_to_name(err));
        rs485_modbus_master_deinit();
        return 0;
    }

    // Initialize parameter cache
    memset(s_param_cache, 0, sizeof(s_param_cache));

    // Register parameter update callback
    // The callback will be called by the Modbus library's poll task
    err = rs485_modbus_master_set_update_callback(modbus_update_cb);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to register Modbus update callback: %s", esp_err_to_name(err));
        rs485_modbus_master_deinit();
        return 0;
    }

    // Configure polling (this will create internal poll task)
    uint16_t param_count = rs485_battery_get_parameter_count();
    err = rs485_modbus_master_set_poll_config(param_count, rs485_battery_get_parameter_count);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to configure Modbus polling: %s", esp_err_to_name(err));
        rs485_modbus_master_deinit();
        return 0;
    }
    // Create task to process commands from Broker (SET, SUBSCRIBE, etc.)
    TRUE_CHECK(xTaskCreate(connector_rs485_master_process_task, (char *)connector_rs485_master.name, RS485_CONNECTOR_TASK_STACK_SIZE, NULL, xTASK_PRIORITY_NORMAL, NULL));

    LOG(D, "RS485 Master connector initialized with Modbus RTU support");

    return 1;
}

CONNECTOR connector_rs485_master = {
    .name = "RS485 master connector",
    .initialize = initialize_connector_rs485_master,
};
