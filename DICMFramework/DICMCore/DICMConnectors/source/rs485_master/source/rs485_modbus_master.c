/*! \file rs485_modbus_master.c
    \brief Modbus Master wrapper implementation for RS485 connector
*/

#include "configuration.h"
// System includes
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
// IDF includes
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// Component includes
#include "esp_modbus_common.h"
#include "esp_modbus_master.h"
// Framework includes
#include "connector_rs485_master.h"
#include "hal_mem.h"
#include "rs485_modbus_master.h"
#include "rs485_modbus_master_battery.h"

// Macro definitions
#define RS485_BAUDRATE_DEFAULT            19200
#define RS485_SLAVE_DEFAULT_ADDRESS       1
#define RS485_MASTER_POLL_INTERVAL_MS     100   // Poll interval: 100ms
#define RS485_MASTER_MIN_SEND_INTERVAL_MS 100   // Minimum interval between Modbus requests: 100ms
#define RS485_SLAVE_RESPONSE_TIMEOUT_MS   1000  // Slave response timeout: 1 second
#define MAX_PARAMETER_SIZE                8     // Max parameter size in bytes
#define RS485_ONLINE_SLAVE_QUERY_COUNT    5     // Number of queries for online slaves per cycle
#define RS485_SLAVE_OFFLINE_THRESHOLD     3     // Consecutive failure threshold to mark offline
#define RS485_POLL_TASK_STACK_SIZE        4096

// Module static variables
static EXT_RAM_ATTR void *s_mb_master_ctx;
static EXT_RAM_ATTR rs485_modbus_master_param_update_cb_t s_update_callback;  // parameter update callback
static EXT_RAM_ATTR TaskHandle_t s_poll_task_handle;
static EXT_RAM_ATTR uint16_t s_poll_param_count;
static EXT_RAM_ATTR uint16_t (*s_get_param_count_func)(void);
EXT_RAM_ATTR rs485_slave_status_t s_slave_status[RS485_SLAVE_ADDR_MAX];
static EXT_RAM_ATTR uint8_t s_online_slave_list[RS485_SLAVE_ADDR_MAX];
static EXT_RAM_ATTR uint8_t s_online_slave_count;
static EXT_RAM_ATTR uint8_t s_offline_slave_list[RS485_SLAVE_ADDR_MAX];
static EXT_RAM_ATTR uint8_t s_offline_slave_count;
static EXT_RAM_ATTR bool s_initial_scan_done;

// Public API functions
esp_err_t rs485_modbus_master_init(void)
{
    // Initialize the (local file) global variables
    s_mb_master_ctx = NULL;
    s_update_callback = NULL;
    s_poll_task_handle = NULL;
    s_poll_param_count = 0;
    s_get_param_count_func = NULL;
    memset(s_slave_status, 0, RS485_SLAVE_ADDR_MAX);
    memset(s_online_slave_list, 0, RS485_SLAVE_ADDR_MAX);
    s_online_slave_count = 0;
    memset(s_offline_slave_list, 0, RS485_SLAVE_ADDR_MAX);
    s_offline_slave_count = 0;
    s_initial_scan_done = false;

    mb_communication_info_t s_mb_comm_info = {0};

    // Configure Modbus communication parameters
    s_mb_comm_info.mode = MB_RTU;
    s_mb_comm_info.ser_opts.port = CONNECTOR_RS485_MASTER_NUM;
    s_mb_comm_info.ser_opts.mode = MB_RTU;
    s_mb_comm_info.ser_opts.baudrate = RS485_BAUDRATE_DEFAULT;
    s_mb_comm_info.ser_opts.parity = MB_PARITY_NONE;
    s_mb_comm_info.ser_opts.data_bits = UART_DATA_8_BITS;
    s_mb_comm_info.ser_opts.stop_bits = UART_STOP_BITS_1;
    s_mb_comm_info.ser_opts.uid = RS485_SLAVE_DEFAULT_ADDRESS;
    s_mb_comm_info.ser_opts.response_tout_ms = RS485_SLAVE_RESPONSE_TIMEOUT_MS;

    // Create Modbus Master controller
    // This will install UART driver internally
    esp_err_t err = mbc_master_create_serial(&s_mb_comm_info, &s_mb_master_ctx);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to create Modbus Master: %s", esp_err_to_name(err));
        return err;
    }

    // Set UART pins AFTER driver installation (esp_modbus installs the driver)
    err = uart_set_pin(CONNECTOR_RS485_MASTER_NUM, CONNECTOR_RS485_MASTER_TX, CONNECTOR_RS485_MASTER_RX, CONNECTOR_RS485_MASTER_RTS, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to set UART pins: %s", esp_err_to_name(err));
        mbc_master_delete(s_mb_master_ctx);
        s_mb_master_ctx = NULL;
        return err;
    }

    // Set UART mode to RS485 half-duplex (CRITICAL for RS485 communication)
    err = uart_set_mode(CONNECTOR_RS485_MASTER_NUM, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to set RS485 half-duplex mode: %s", esp_err_to_name(err));
        mbc_master_delete(s_mb_master_ctx);
        s_mb_master_ctx = NULL;
        return err;
    }
    LOG(D, "Modbus Master initialized: UART%d, TX:GPIO%d, RX:GPIO%d, RTS:GPIO%d, Baud:%lu, SlaveAddr:%d, Timeout:%dms",
        CONNECTOR_RS485_MASTER_NUM, CONNECTOR_RS485_MASTER_TX, CONNECTOR_RS485_MASTER_RX, CONNECTOR_RS485_MASTER_RTS, RS485_BAUDRATE_DEFAULT, RS485_SLAVE_DEFAULT_ADDRESS, s_mb_comm_info.ser_opts.response_tout_ms);

    return ESP_OK;
}

esp_err_t rs485_modbus_master_deinit(void)
{
    if (s_mb_master_ctx == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = mbc_master_stop(s_mb_master_ctx);
    if (err != ESP_OK)
    {
        LOG(W, "Failed to stop Modbus Master: %s", esp_err_to_name(err));
    }

    err = mbc_master_delete(s_mb_master_ctx);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to delete Modbus Master: %s", esp_err_to_name(err));
        return err;
    }

    s_mb_master_ctx = NULL;
    LOG(D, "Modbus Master deinitialized");

    return ESP_OK;
}

esp_err_t rs485_modbus_master_start(void)
{
    if (s_mb_master_ctx == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = mbc_master_start(s_mb_master_ctx);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to start Modbus Master: %s", esp_err_to_name(err));
        return err;
    }

    LOG(D, "Modbus Master started (library scheduler active)");
    return ESP_OK;
}

esp_err_t rs485_modbus_master_set_descriptor(const void *descriptor,
                                             uint16_t num_elements)
{
    if (s_mb_master_ctx == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (descriptor == NULL || num_elements == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const mb_parameter_descriptor_t *desc = (const mb_parameter_descriptor_t *)descriptor;
    esp_err_t err = mbc_master_set_descriptor(s_mb_master_ctx, desc, num_elements);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to set Modbus descriptor: %s", esp_err_to_name(err));
        return err;
    }

    LOG(D, "Modbus descriptor table set: %d elements", num_elements);
    return ESP_OK;
}

/**
 * @brief Helper function to get slave address from CID
 * @param cid Characteristic ID
 * @param[out] addr Pointer to store slave address
 * @return esp_err_t ESP_OK on success, ESP_ERR_NOT_FOUND if descriptor not found
 */
static esp_err_t get_slave_addr_from_cid(uint16_t cid, uint8_t *addr)
{
    if (addr == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const mb_parameter_descriptor_t *desc = NULL;
    esp_err_t err = mbc_master_get_cid_info(s_mb_master_ctx, cid, &desc);
    if (err != ESP_OK || desc == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }

    *addr = desc->mb_slave_addr;
    return ESP_OK;
}

esp_err_t rs485_modbus_master_get_parameter_with(uint16_t cid, uint8_t addr, uint8_t *value, uint8_t *type)
{
    if (s_mb_master_ctx == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (value == NULL || type == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Get addr from descriptor if 0 is passed
    uint8_t target_slave_addr = addr;
    if (target_slave_addr == 0)
    {
        esp_err_t err = get_slave_addr_from_cid(cid, &target_slave_addr);
        if (err != ESP_OK)
        {
            return err;
        }
    }

    esp_err_t err = mbc_master_get_parameter_with(s_mb_master_ctx, cid, target_slave_addr, value, type);
    if (err != ESP_OK)
    {
        LOG(D, "Failed to get parameter CID %d from slave %d: %s", cid, target_slave_addr, esp_err_to_name(err));
        return err;
    }

    vTaskDelay(RS485_MASTER_POLL_INTERVAL_MS / portTICK_PERIOD_MS);

    return ESP_OK;
}

esp_err_t rs485_modbus_master_get_cid_info(uint16_t cid, const void **descriptor)
{
    if (s_mb_master_ctx == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (descriptor == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const mb_parameter_descriptor_t *desc = NULL;
    esp_err_t err = mbc_master_get_cid_info(s_mb_master_ctx, cid, &desc);
    if (err != ESP_OK)
    {
        return err;
    }

    *descriptor = (const void *)desc;
    return ESP_OK;
}

/**
 * @brief Scan a slave address to check if it's online
 * Attempts to read the first parameter (voltage) from the slave to detect if it's online.
 * @param addr Slave address to scan (1-20)
 * @return bool true if slave is online, false otherwise
 */
static bool scan_slave_address(uint8_t addr)
{
    if (s_mb_master_ctx == NULL || addr < RS485_SLAVE_ADDR_MIN || addr > RS485_SLAVE_ADDR_MAX)
    {
        return false;
    }

    // Try to read the first parameter (voltage) to check if slave is online
    uint8_t value_buffer[MAX_PARAMETER_SIZE];
    uint8_t type;
    esp_err_t err = rs485_modbus_master_get_parameter_with(BATTERY_CID_VOLTAGE, addr, value_buffer, &type);

    return (err == ESP_OK);
}

/**
 * @brief Initial scan of all slave addresses to determine online status
 * Scans all slave addresses (1-20) and builds online/offline lists.
 * This should be called once at startup.
 */
static void initial_slave_scan(void)
{
    if (s_mb_master_ctx == NULL)
    {
        return;
    }

    // Initialize slave status array
    for (uint8_t i = 0; i < RS485_SLAVE_ADDR_MAX; i++)
    {
        s_slave_status[i].addr = i + 1;
        s_slave_status[i].is_online = false;
        s_slave_status[i].consecutive_failures = 0;
        s_slave_status[i].registered_prod_instance = -1;
        s_slave_status[i].registered_mbat_instance = -1;
        s_slave_status[i].product_info_read = false;
    }

    s_online_slave_count = 0;
    s_offline_slave_count = 0;

    LOG(D, "Starting initial slave scan (addresses %d-%d)...", RS485_SLAVE_ADDR_MIN, RS485_SLAVE_ADDR_MAX);

    // Scan all slave addresses
    for (uint8_t addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        if (scan_slave_address(addr))
        {
            // Slave is online
            s_slave_status[ADDR_OFFSET(addr)].is_online = true;
            s_slave_status[ADDR_OFFSET(addr)].consecutive_failures = 0;
            s_online_slave_list[s_online_slave_count++] = addr;
            LOG(D, "Slave address %d is ONLINE", addr);
            slave_battery_device_login(addr);
        }
        else
        {
            // Slave is offline
            s_slave_status[ADDR_OFFSET(addr)].is_online = false;
            s_slave_status[ADDR_OFFSET(addr)].consecutive_failures++;
            s_offline_slave_list[s_offline_slave_count++] = addr;
            LOG(D, "Slave address %d is OFFLINE", addr);
        }
    }

    LOG(D, "Initial scan complete: %d online, %d offline", s_online_slave_count, s_offline_slave_count);
    s_initial_scan_done = true;
}

/**
 * @brief Update descriptor table based on currently online slaves
 * Generates a dynamic descriptor table for all online slaves.
 * Each slave gets a complete set of parameter descriptors with its slave address.
 * @param[out] descriptor_table Pointer to store descriptor table pointer
 * @param[out] num_elements Pointer to store number of elements
 * @return esp_err_t ESP_OK on success
 */
static esp_err_t update_descriptor_table(mb_parameter_descriptor_t *descriptor_table, uint16_t *num_elements)
{
    // Get base descriptor table from battery module
    const void *base_descriptor = NULL;
    uint16_t base_num_elements = 0;
    esp_err_t err = rs485_battery_get_descriptor_table(&base_descriptor, &base_num_elements);
    if (err != ESP_OK || base_descriptor == NULL || base_num_elements == 0)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const mb_parameter_descriptor_t *base_desc = (const mb_parameter_descriptor_t *)base_descriptor;

    // Generate descriptors for each online slave
    uint16_t desc_idx = 0;
    for (uint8_t i = 0; i < s_online_slave_count; i++)
    {
        uint8_t addr = s_online_slave_list[i];
        uint16_t cid_offset = i * base_num_elements;

        // Copy base descriptors and update slave address
        for (uint16_t j = 0; j < base_num_elements; j++)
        {
            descriptor_table[desc_idx] = base_desc[j];
            descriptor_table[desc_idx].mb_slave_addr = addr;
            descriptor_table[desc_idx].cid = (uint16_t)(cid_offset + j);
            desc_idx++;
        }
    }

    // Calculate total descriptor count: online_slave_count * base_num_elements
    *num_elements = s_online_slave_count * base_num_elements;

    LOG(D, "Updated descriptor table: %d slaves, %d total descriptors", s_online_slave_count, *num_elements);
    return ESP_OK;
}

/**
 * @brief Check and update slave online/offline status
 * @param addr Slave address to check
 * @return bool true if status changed, false otherwise
 */
static bool update_slave_status(uint8_t addr)
{
    if (addr < RS485_SLAVE_ADDR_MIN || addr > RS485_SLAVE_ADDR_MAX)
    {
        return false;
    }

    rs485_slave_status_t *status = &s_slave_status[ADDR_OFFSET(addr)];
    bool was_online = status->is_online;
    bool is_online_now = scan_slave_address(addr);

    if (is_online_now)
    {
        // Slave responded successfully
        status->consecutive_failures = 0;
        if (!was_online)
        {
            // Slave came online
            status->is_online = true;
            slave_battery_device_login(addr);
            return true;
        }
    }
    else
    {
        // Slave did not respond
        status->consecutive_failures++;
        if (status->consecutive_failures >= RS485_SLAVE_OFFLINE_THRESHOLD && was_online)
        {
            // Slave went offline
            status->is_online = false;
            slave_battery_device_logout(addr);
            return true;
        }
    }

    return false;
}

/**
 * @brief Rebuild online/offline lists from slave status array
 */
static void rebuild_slave_lists(void)
{
    s_online_slave_count = 0;
    s_offline_slave_count = 0;

    for (uint8_t i = 0; i < RS485_SLAVE_ADDR_MAX; i++)
    {
        uint8_t addr = i + 1;
        if (s_slave_status[i].is_online)
        {
            s_online_slave_list[s_online_slave_count++] = addr;
        }
        else
        {
            s_offline_slave_list[s_offline_slave_count++] = addr;
        }
    }
}

/**
 * @brief Read the prod class information from Slave device,
 * when the Slave device coming online.
 * @param addr Modbus slave address (1-20)
 */
static void slave_product_info_read(uint8_t addr)
{
    if (addr < RS485_SLAVE_ADDR_MIN || addr > RS485_SLAVE_ADDR_MAX)
    {
        return;
    }

    // Use s_slave_status[ADDR_OFFSET(addr)] as the single source of truth
    rs485_slave_status_t *status = &s_slave_status[ADDR_OFFSET(addr)];

    // Read product info parameters once when device comes online
    if (!status->product_info_read)
    {
        if (s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance >= 0)
        {
            // List of product info CIDs to read
            rs485_battery_cid_t product_info_cids[] = {
                BATTERY_CID_DESIGN_CAPACITY,
                BATTERY_CID_PRODUCT_NUM_CODE,
                BATTERY_CID_SERIAL_NUM,
                BATTERY_CID_FIRMWARE_VER,
                BATTERY_CID_HARDWARE_VER,
            };
            uint8_t num_product_params = sizeof(product_info_cids) / sizeof(product_info_cids[0]);

            bool all_read_success = true;
            for (uint8_t i = 0; i < num_product_params; i++)
            {
                rs485_battery_cid_t cid = product_info_cids[i];
                uint8_t value_buffer[RS485_PARAM_VALUE_SIZE_STR + 1];  // Max size for product info (20 bytes for strings, 4 for uint32)
                uint8_t type;
                esp_err_t err = rs485_modbus_master_get_parameter_with(cid, addr, value_buffer, &type);

                if (err == ESP_OK)
                {
                    // Get parameter size from descriptor
                    const void *desc_ptr = NULL;
                    uint8_t param_size = RS485_PARAM_VALUE_SIZE_STR;  // Default for string parameters
                    if (rs485_modbus_master_get_cid_info(cid, &desc_ptr) == ESP_OK)
                    {
                        const mb_parameter_descriptor_t *desc = (const mb_parameter_descriptor_t *)desc_ptr;
                        param_size = desc->param_size;
                    }

                    // Process and update product database
                    err = rs485_battery_process_product_info(cid, value_buffer, param_size, s_slave_status[ADDR_OFFSET(addr)].registered_prod_instance);
                    if (err != ESP_OK)
                    {
                        LOG(W, "Failed to process product info CID %d for slave %d: %s", cid, addr, esp_err_to_name(err));
                        all_read_success = false;
                    }
                    else
                    {
                        LOG(D, "Read product info CID %d for slave %d (instance %d)", cid, addr, s_slave_status[ADDR_OFFSET(addr)].registered_prod_instance);
                    }
                }
                else
                {
                    LOG(D, "Failed to read product info CID %d from slave %d: %s", cid, addr, esp_err_to_name(err));
                    all_read_success = false;
                }
            }

            if (all_read_success)
            {
                status->product_info_read = true;
                LOG(D, "All product info parameters read for slave %d", addr);
            }
        }
    }
}

/**
 * @brief Internal poll task: Periodically read parameters from all slaves and call callback
 * This task implements dynamic slave scanning and polling:
 * - Initial scan: Scans all slave addresses (1-20) to determine online status
 * - Online slaves: Queried 5 times per cycle
 * - Offline slaves: Scanned once per cycle to detect when they come online
 */
mb_parameter_descriptor_t dynamic_descriptor_table[BATTERY_CID_MAX * RS485_SLAVE_ADDR_MAX] = {0};
static void rs485_master_poll_task(void *pvParameters)
{
    uint16_t dynamic_num_elements = 0;
    uint8_t value_buffer[MAX_PARAMETER_SIZE];  // Max parameter size
    uint8_t type;
    esp_err_t err;

    LOG(D, "Modbus poll task started with dynamic slave scanning");

    while (1)
    {
        if (s_mb_master_ctx == NULL || s_update_callback == NULL)
        {
            vTaskDelay(RS485_MASTER_POLL_INTERVAL_MS / portTICK_PERIOD_MS);
            continue;
        }

        // Perform initial scan if not done yet
        if (!s_initial_scan_done)
        {
            initial_slave_scan();

            for (uint8_t i = 0; i < s_online_slave_count; i++)
            {
                uint8_t addr = s_online_slave_list[i];
                slave_product_info_read(addr);
            }

            // Update descriptor table
            if (update_descriptor_table(dynamic_descriptor_table, &dynamic_num_elements) == ESP_OK)
            {
                rs485_modbus_master_set_descriptor(dynamic_descriptor_table, dynamic_num_elements);
            }
        }

        // Get base parameter count (per slave)
        uint16_t base_param_count = s_poll_param_count;
        if (s_get_param_count_func != NULL)
        {
            base_param_count = s_get_param_count_func();
        }

        bool descriptor_updated = false;

        // Query online slaves (5 times per cycle)
        for (uint8_t s_online_query_counter = 0; s_online_slave_count > 0 && s_online_query_counter < RS485_ONLINE_SLAVE_QUERY_COUNT; s_online_query_counter++)
        {
            // Poll all parameters for all online slaves
            for (uint8_t slave_idx = 0; slave_idx < s_online_slave_count; slave_idx++)
            {
                uint8_t addr = s_online_slave_list[slave_idx];
                uint16_t cid_base = slave_idx * base_param_count;

                // Poll all parameters for this slave
                for (uint16_t param_idx = 0; param_idx < base_param_count; param_idx++)
                {
                    uint16_t cid = cid_base + param_idx;
                    uint16_t original_cid = param_idx;

                    // Skip one-time read parameters (only poll regular parameters)
                    if (!rs485_battery_is_polling_parameter((rs485_battery_cid_t)original_cid))
                    {
                        continue;
                    }

                    // Get parameter descriptor
                    const mb_parameter_descriptor_t *desc = NULL;
                    if (mbc_master_get_cid_info(s_mb_master_ctx, cid, &desc) != ESP_OK || desc == NULL)
                    {
                        continue;
                    }

                    // Read parameter
                    err = rs485_modbus_master_get_parameter_with(cid, addr, value_buffer, &type);

                    if (err == ESP_OK)
                    {
                        // Get parameter size
                        uint8_t param_size = desc->param_size;
                        if (param_size == 0 || param_size > MAX_PARAMETER_SIZE)
                        {
                            // Fallback: Determine size based on type
                            switch (type)
                            {
                            case PARAM_TYPE_U8:
                            case PARAM_TYPE_I8_A:
                            case PARAM_TYPE_I8_B:
                            case PARAM_TYPE_U8_A:
                            case PARAM_TYPE_U8_B:
                                param_size = 1;
                                break;
                            case PARAM_TYPE_U16:
                            case PARAM_TYPE_I16_AB:
                            case PARAM_TYPE_I16_BA:
                                param_size = 2;
                                break;
                            case PARAM_TYPE_U32:
                            case PARAM_TYPE_I32_ABCD:
                            case PARAM_TYPE_FLOAT:
                                param_size = 4;
                                break;
                            default:
                                param_size = MAX_PARAMETER_SIZE;
                                break;
                            }
                        }

                        // Call callback with original CID (0-based, not offset)
                        s_update_callback(original_cid, addr, value_buffer, param_size, type);
                    }
                    else
                    {
                        LOG(W, "Read failed, cid %d, addr: %d", cid, addr);
                        if (update_slave_status(addr))
                        {
                            descriptor_updated = true;
                        }
                    }
                    vTaskDelay(RS485_MASTER_POLL_INTERVAL_MS / portTICK_PERIOD_MS);
                }
            }
        }

        // Scan offline slaves (once per cycle)
        if (s_offline_slave_count > 0)
        {
            for (uint8_t i = 0; i < s_offline_slave_count; i++)
            {
                uint8_t addr = s_offline_slave_list[i];
                if (update_slave_status(addr))
                {
                    descriptor_updated = true;
                }
            }
        }

        // Update descriptor table if slave status changed
        if (descriptor_updated)
        {
            rebuild_slave_lists();

            if (update_descriptor_table(dynamic_descriptor_table, &dynamic_num_elements) == ESP_OK)
            {
                rs485_modbus_master_set_descriptor(dynamic_descriptor_table, dynamic_num_elements);
            }

            for (uint8_t i = 0; i < s_online_slave_count; i++)
            {
                uint8_t addr = s_online_slave_list[i];
                // The newly onlined device reads the prod class params
                slave_product_info_read(addr);
            }
        }

        // Wait before next poll cycle
        vTaskDelay(RS485_MASTER_POLL_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

esp_err_t rs485_modbus_master_set_update_callback(rs485_modbus_master_param_update_cb_t callback)
{
    s_update_callback = callback;

    LOG(D, "Parameter update callback %s", callback ? "registered" : "unregistered");
    return ESP_OK;
}

esp_err_t rs485_modbus_master_set_poll_config(uint16_t param_count, uint16_t (*get_param_count_func)(void))
{
    s_poll_param_count = param_count;
    s_get_param_count_func = get_param_count_func;

    // Create poll task if callback is registered and task doesn't exist
    if (s_update_callback != NULL && s_poll_task_handle == NULL)
    {
        BaseType_t result = xTaskCreate(rs485_master_poll_task,
                                        "rs485_master_poll",
                                        RS485_POLL_TASK_STACK_SIZE,
                                        NULL,
                                        xTASK_PRIORITY_BELOW_NORMAL,  // Lower priority than connector task
                                        &s_poll_task_handle);
        if (result != pdPASS)
        {
            LOG(E, "Failed to create Modbus poll task");
            return ESP_ERR_NO_MEM;
        }
        LOG(D, "Modbus poll task created");
    }

    return ESP_OK;
}
