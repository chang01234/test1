/*! \file rs485_master_battery.h
    \brief Mobile power bank device parameter definitions for Modbus RTU
*/

#ifndef RS485_MASTER_BATTERY_H_
#define RS485_MASTER_BATTERY_H_

#include "configuration.h"

// System includes
#include <stdbool.h>
#include <stdint.h>
// IDF includes
#include "esp_err.h"

#define RS485_SLAVE_ADDR_MIN       1   // Minimum slave address
#define RS485_SLAVE_ADDR_MAX       20  // Maximum slave address
#define BATTERY_SLAVE_ADDR_DEFAULT 1
#define ADDR_OFFSET(addr)          (addr - 1)
#define RS485_PARAM_VALUE_SIZE_U16 2
#define RS485_PARAM_VALUE_SIZE_U32 4
#define RS485_PARAM_VALUE_SIZE_STR 20
#define BIT_SIZE_U16               16

/**
 * @brief Characteristic IDs for mobile power bank parameters
 * based on mobile power bank Modbus protocol specification.
 */
typedef enum
{
    BATTERY_CID_VOLTAGE = 0,    // DC voltage
    BATTERY_CID_CURRENT,        // DC current
    BATTERY_CID_SOC,            // State of charge
    BATTERY_CID_SOH,            // State of health
    BATTERY_CID_FULL_CAPACITY,  // Full charge capacity
    BATTERY_CID_CAPREM,         // Remaining capacity
    BATTERY_CID_TEMP,           // Ambient temperature of acquisition board
    BATTERY_CID_CHGDET,         // Charge detection status
    BATTERY_CID_HTR,            // Heating switch status
    BATTERY_CID_DVOLT,          // Charging voltage value
    BATTERY_CID_DCURR,          // Charging current limit value
    // Battery fault/error status
    BATTY_CID_ERROR_1,             // Battery fault/error status part 1
    BATTY_CID_ERROR_2,             // Battery fault/error status part 2
    BATTERY_CID_CYCLE_NUM,         // The number of charge cycles
    BATTERY_CID_DESIGN_CAPACITY,   // Design capacity
    BATTERY_CID_PRODUCT_NUM_CODE,  // Product numeric code
    BATTERY_CID_SERIAL_NUM,        // Serial number
    BATTERY_CID_FIRMWARE_VER,      // Firmware version
    BATTERY_CID_HARDWARE_VER,      // Hardware version
    BATTERY_CID_MAX                // Maximum CID value
} rs485_battery_cid_t;

// Input and Holding register parameters structure
typedef struct
{
    uint32_t battery_voltage;     // 30001-30002: Battery voltage
    int32_t total_current;        // 30003-30004: Total current
    uint16_t soc;                 // 30005: SOC
    uint16_t soh;                 // 30006: SOH
    uint32_t full_capacity;       // 30041-30042: Full charge capacity
    uint32_t remain_capacity;     // 30043-30044: Remaining capacity
    int16_t ambient_temp;         // 30045: Ambient temperature
    uint16_t charg_mos_status;    // 30047: Charging MOS status
    uint16_t d_charg_mos_status;  // 30048: Discharging MOS status
    uint16_t work_status;         // 30050: Working status of BMS
    uint16_t h_switch_status;     // 30051: Heating switch status
    uint32_t charg_voltage;       // 30292: Charging voltage value
    uint32_t charg_current;       // 30293: Charging current
    // Battery fault/error status
    uint16_t battery_error_status_1;                       // 10001: Battery fault/error status part 1
    uint16_t battery_error_status_2;                       // 10002: Battery fault/error status
    uint16_t cycle_times;                                  // 30007: Cycle times
    uint32_t design_capacity;                              // 30206-30207: Design capacity
    uint8_t product_number[RS485_PARAM_VALUE_SIZE_STR];    // 40211: Product number
    uint8_t production_date[RS485_PARAM_VALUE_SIZE_STR];   // 40251: Production date
    uint8_t serial_number[RS485_PARAM_VALUE_SIZE_STR];     // 40261: Serial number
    uint8_t firmware_version[RS485_PARAM_VALUE_SIZE_STR];  // 40281: Firmware version
    uint8_t hardware_version[RS485_PARAM_VALUE_SIZE_STR];  // 40291: Hardware version
} reg_params_t;

/**
 * @brief Get parameter descriptor table for mobile power bank
 * @param[out] descriptor_table Pointer to store descriptor table pointer
 * @param[out] num_elements Pointer to store number of elements
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_battery_get_descriptor_table(const void **descriptor_table,
                                             uint16_t *num_elements);

/**
 * @brief Get number of parameters in descriptor table
 * @return uint16_t Number of parameters
 */
uint16_t rs485_battery_get_parameter_count(void);

/**
 * @brief Check if a CID should be polled regularly
 * Returns true for parameters that need regular polling (VOLTAGE, CURRENT, SOC, SOH, CYCLE_NUM)
 * Returns false for one-time read parameters (product info parameters)
 * @param cid Modbus CID
 * @return bool true if parameter should be polled regularly, false otherwise
 */
bool rs485_battery_is_polling_parameter(rs485_battery_cid_t cid);

/**
 * @brief Convert Modbus CID to DDM2 parameter ID
 * @param cid Modbus CID
 * @param instance Instance number (0-based)
 * @return uint32_t DDM2 parameter ID with instance, or 0 if not found
 */
uint32_t rs485_battery_cid_to_ddm2_param(rs485_battery_cid_t cid, uint8_t instance);

/**
 * @brief Convert DDM2 parameter ID to Modbus CID
 * @param ddm2_param DDM2 parameter ID (with or without instance)
 * @param[out] cid Pointer to store CID
 * @return esp_err_t ESP_OK if found, ESP_ERR_NOT_FOUND otherwise
 */
esp_err_t rs485_battery_ddm2_param_to_cid(uint32_t ddm2_param, rs485_battery_cid_t *cid);

/**
 * @brief Get Modbus slave address from DDM2 parameter ID
 * @param ddm2_param DDM2 parameter ID
 * @param[out] addr Pointer to store Modbus slave address
 * @return esp_err_t ESP_OK if found, ESP_ERR_NOT_FOUND otherwise
 */
esp_err_t rs485_battery_ddm2_param_to_addr(uint32_t ddm2_param, uint8_t *addr);

/**
 * @brief Convert rs485_battery value to DDM2 value
 * Converts a value from rs485_battery format (Modbus register format) to DDM2 format.
 * Handles byte order conversion (U32_CDAB) for voltage and current, and precision
 * conversion for SOC and SOH.
 * The value parameter is used for both input and output (in-place conversion).
 * @param cid Modbus CID identifying the parameter type
 * @param value Pointer to the value (input type depends on CID, output is int32_t)
 * @param size Pointer to store size of converted value in bytes
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid parameters, ESP_ERR_NOT_SUPPORTED for unsupported CID
 */
esp_err_t rs485_battery_value_to_ddm2_value(rs485_battery_cid_t cid, uint8_t *value, uint8_t *size);

/**
 * @brief Process product information parameters and update product database
 * Handles parsing and updating product database for one-time read parameters.
 * @param cid Modbus CID identifying the parameter type
 * @param value Pointer to the value buffer
 * @param value_size Size of the value in bytes
 * @param instance DDM2 instance number for product database update
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid parameters, ESP_ERR_NOT_SUPPORTED for unsupported CID
 */
esp_err_t rs485_battery_process_product_info(rs485_battery_cid_t cid, void *value, uint8_t value_size, int instance);

/**
 * @brief Check if battery parameter value has changed
 * Compares a new value with a cached value to determine if it has changed.
 * If the cache is invalid (first read), it marks it as valid and returns true.
 * For VOLTAGE and CURRENT, changes must be >= 100mV/100mA to be considered changed.
 * @param cid Modbus CID identifying the parameter type
 * @param cached_value Pointer to cached value buffer
 * @param new_value Pointer to new value buffer
 * @param size Size of the value in bytes
 * @param[in,out] is_valid Pointer to validity flag (input: current state, output: updated state)
 * @return bool true if value has changed or cache was invalid, false otherwise
 */
bool rs485_battery_check_value_changed(rs485_battery_cid_t cid,
                                       const uint8_t *cached_value,
                                       const uint8_t *new_value,
                                       uint8_t size,
                                       bool *is_valid);

/**
 * @brief Publish battery status to Broker
 * Converts rs485_battery format value to DDM2 format and publishes it to Broker.
 * This function does NOT modify the original value or cache - it works on a copy.
 * @param cid Modbus CID identifying the parameter type
 * @param ddm2_param DDM2 parameter ID (with instance)
 * @param rs485_value Pointer to rs485_battery format value (will not be modified)
 * @param value_size Size of the value in bytes
 * @param connector_id Connector ID for Broker communication
 * @return bool true if published successfully, false otherwise
 */
bool rs485_battery_publish_status(rs485_battery_cid_t cid,
                                  uint32_t ddm2_param,
                                  const void *rs485_value,
                                  uint8_t value_size,
                                  uint8_t connector_id);

/**
 * @brief Handle slave battery device login
 * Registers MBAT0, MBATHIST0, and product class instances for the slave device.
 * @param addr Slave device address
 */
void slave_battery_device_login(uint8_t addr);

/**
 * @brief Handle slave battery device logout
 * Unregisters MBAT0 and product class instances for the slave device.
 * @param addr Slave device address
 */
void slave_battery_device_logout(uint8_t addr);

/**
 * @brief Calculate and publish MBAT0LINKED and MBAT0TYPE parameters
 * based on the registered instances for the slave device.
 * @param ddm2_param DDM2 parameter ID (with instance)
 */
void calculate_mbatxlinked_mbatxtype_param(uint32_t ddm2_param);

#endif /* RS485_MASTER_BATTERY_H_ */
