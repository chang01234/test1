/*! \file rs485_modbus_master.h
    \brief Modbus Master wrapper for RS485 connector
*/

#ifndef RS485_MODBUS_MASTER_H_
#define RS485_MODBUS_MASTER_H_

#include "configuration.h"
// System includes
#include <stdbool.h>
#include <stdint.h>
// IDF includes
#include "esp_err.h"

/**
 * @brief Initialize Modbus Master for RS485
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_init(void);

/**
 * @brief Deinitialize Modbus Master
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_deinit(void);

/**
 * @brief Start Modbus Master communication
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_start(void);

/**
 * @brief Set parameter descriptor table
 * @param descriptor Pointer to parameter descriptor table
 * @param num_elements Number of elements in the table
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_set_descriptor(const void *descriptor,
                                             uint16_t num_elements);

/**
 * @brief Read parameter from specific Modbus slave
 * @param cid Characteristic ID
 * @param addr Modbus slave address (1-247), 0 to use descriptor's addr
 * @param value Pointer to buffer to store read value
 * @param type Pointer to store parameter type
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_get_parameter_with(uint16_t cid, uint8_t addr, uint8_t *value, uint8_t *type);

/**
 * @brief Get parameter descriptor information for a CID
 * @param cid Characteristic ID
 * @param[out] descriptor Pointer to store descriptor pointer
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_get_cid_info(uint16_t cid, const void **descriptor);

/**
 * @brief Parameter update callback function type
 * Called when a parameter value is read and potentially changed.
 * @param cid Characteristic ID
 * @param addr Modbus slave address (1-247)
 * @param value Pointer to parameter value buffer
 * @param size Size of parameter value in bytes
 * @param type Parameter type
 */
typedef void (*rs485_modbus_master_param_update_cb_t)(uint16_t cid, uint8_t addr, const uint8_t *value, uint8_t size, uint8_t type);

/**
 * @brief Slave status change callback function type
 * Called when a slave comes online or goes offline.
 * @param addr Modbus slave address (1-20)
 * @param is_online true if slave came online, false if went offline
 */
typedef void (*rs485_modbus_master_slave_status_cb_t)(uint8_t addr, bool is_online);

/**
 * @brief Register parameter update callback
 * The callback will be called when parameters are polled.
 * @param callback Callback function (NULL to unregister)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_set_update_callback(rs485_modbus_master_param_update_cb_t callback);

/**
 * @brief Configure polling parameters
 * @param param_count Number of parameters to poll
 * @param get_param_count_func Optional function to get parameter count dynamically (can be NULL)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rs485_modbus_master_set_poll_config(uint16_t param_count, uint16_t (*get_param_count_func)(void));

/**
 * @brief Slave status structure
 */
typedef struct
{
    int registered_prod_instance;      // DDM prod class instance registered for this slave
    int registered_mbathist_instance;  // DDM mbathist class instance registered for this slave
    int registered_mbat_instance;      // DDM mbat class instance registered for this slave
    uint8_t addr;                      // Slave address (1-20)
    bool is_online;                    // Whether slave is online
    uint8_t consecutive_failures;      // Consecutive failure count
    bool product_info_read;            // Whether product info parameters have been read
} rs485_slave_status_t;

// External declaration for slave status array (defined in rs485_modbus_master.c)
extern rs485_slave_status_t s_slave_status[];

#endif /* RS485_MODBUS_MASTER_H_ */
