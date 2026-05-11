/**
 * @file lindev_generic.h
 *
 */

#ifndef LINDEV_GENERIC_H_
#define LINDEV_GENERIC_H_

#include <stddef.h>
#include <stdint.h>

#include "connector.h"
#include "ddm_entry.h"
#include "ddm_store.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "inventory_handler.h"
#include "lin_common.h"
#include "lindev_table.h"
#include "lindev_uart.h"
#include "sorted_list.h"

/**
 * @brief 		LINDEV generic verbose logging
 *
 * By default, LINDEV generic will not print any message. Define this macro in local project
 * configuration file to enable verbose logging.
 *
 * @note		This will generate a lot of logs.
 * @note		Do not leave this option enabled in firmware production releases.
 */
#ifndef LINDEV_GENERIC_VERBOSE_LOG
#define LINDEV_GENERIC_VERBOSE_LOG 0
#endif

/* Forward declaration */
typedef struct lindev_generic lindev_generic_t;

typedef enum lindev_generic_profile_type
{
    LINDEV_GENERIC_PROFILE_TYPE_GENERIC,
    LINDEV_GENERIC_PROFILE_TYPE_CUSTOM,
} lindev_generic_profile_type_t;

typedef struct lindev_generic_descriptor
{
    const lin_device_config_data_t *device_config;
    ddm_store_t *ddm_other_store;
    const ddm_store_ddm_t *ddm_other_initial_values;
    size_t ddm_other_initial_values_size;
    const char *name;
    uart_port_t lindev_uart;
    lindev_frame_t *lindev_frames;
    const lindev_frame_def_t *lindev_frame_defs;
    size_t lindev_frame_defs_size;
    const lindev_table_config_t *lindev_table_config;
    void (*cb_ddm_loop_has_started)(void);
    void (*cb_ddm_entry_has_changed)(const ddm_entry_t *ddm_entry);
} lindev_generic_descriptor_t;

typedef struct lindev_generic_profile_entry
{
    uint32_t ddm_value;
    const lindev_generic_descriptor_t *descriptor;
} lindev_generic_profile_entry_t;

typedef struct lindev_generic_profile
{
    lindev_generic_profile_type_t profile_type;
    const uint32_t *discriminator_list;  //!< Pointer to array of DDM parameter classes to use as discriminators
    size_t discriminator_list_size;      //!< Number of elements in inventory_discriminator_list array
    union
    {
        struct generic_profile
        {
            const lindev_generic_profile_entry_t *entries;
            size_t entries_size;
        } generic_profile;
        struct custom_profile
        {
            const lindev_generic_descriptor_t *(*cb_discriminate_model)(const lindev_generic_t *lindev_generic, const uint32_t parameter, const int32_t parameter_value);
        } custom_profile;
    };
} lindev_generic_profile_t;

typedef struct lindev_generic_config
{
    size_t worker_stack_size;           //!< Stack size of worker task (in bytes)
    uint32_t worker_priority;           //!< Priority of worker task
    const uint32_t *inventory_list;     //!< Pointer to array of DDM parameter classes to subscribe to
    size_t inventory_size;              //!< Number of elements in inventory_list array
    SORTED_LIST *inventory_sortedlist;  //!< Pointer to sorted list to hold the states of DDM classes we want to subscribe to
} lindev_generic_config_t;

typedef enum lindev_generic_state
{
    LINDEV_GENERIC_STATE_DISCRIMINATING,  //!< Currently we are waiting for discriminator parameters to arrive
    LINDEV_GENERIC_STATE_OPERATIONAL,     //!< We are operational, LIN device is configured and we are communicating
} lindev_generic_state_t;

typedef struct lindev_generic
{
    const lindev_generic_profile_t *profile;
    const lindev_generic_descriptor_t *descriptor;
    const lindev_generic_config_t *config;
    lin_device_config_data_t device_config;
    uint8_t device_config_function_variant_id_index;  // At init 0xFF, becomes 0, 1, ...
    bool is_device_config_available;
    lindev_table_state_t table_state;
    inventory_handler_t inventory_handler;  //!< Inventory handler instance
    SemaphoreHandle_t ddm_other_store_lock;
    ddm_store_t *ddm_other_store;
    CONNECTOR *connector;
    uint32_t connector_instance;
    lindev_t lindev;
    StaticSemaphore_t ddm_other_store_lock_buffer;
    lindev_generic_state_t state;  //!< Current state of LINDEV generic
} lindev_generic_t;

/**
 * \brief       Initialize LINDEV connector with multiple possible profiles.
 *
 * \param       lindev_generic Pointer to \ref lindev_generic structure instance.
 * \param       connector Pointer to connector
 * \param       uart_config Is a pointer to constant configuration structure containing LINDEV UART callbacks.
 * \param       config Pointer to \ref lindev_generic_config_t structure instance containing common configuration parameters.
 * \param       profile Pointer to \ref lindev_generic_profile_t structure containing definition of profiles.
 *
 * \return      Status of operation.
 * \retval      0 - in case of an error.
 * \retval      1 - in case when operation was successful.
 *
 * \pre         Parameter \p lindev_generic must be a non-NULL pointer.
 * \pre         Parameter \p connector must be a non-NULL pointer.
 * \pre         Parameter \p uart_config must be a non-NULL pointer.
 * \pre         Parameter \p config must be a non-NULL pointer.
 * \pre         Parameter \p profile must be a non-NULL pointer.
 */
int lindev_generic_init_with_profile(
    lindev_generic_t *lindev_generic,
    CONNECTOR *connector,
    const lindev_uart_config_t *uart_config,
    const lindev_generic_config_t *config,
    const lindev_generic_profile_t *profile);

/**
 * \brief       Initialize and start the LINDEV connector
 *
 * \param       lindev_generic Pointer to \ref lindev_generic structure instance.
 * \param       connector Pointer to connector
 * \param       uart_config Is a pointer to constant configuration structure containing LINDEV UART callbacks.
 * \param       config Pointer to \ref lindev_generic_config_t structure instance containing common configuration parameters.
 * \param       description Pointer to \ref lindev_generic_descriptor_t structure instance.
 *
 * \return      Status of operation.
 * \retval      0 - in case of an error.
 * \retval      1 - in case when operation was successful.
 *
 * \pre         Parameter \p lindev_generic must be a non-NULL pointer.
 * \pre         Parameter \p connector must be a non-NULL pointer.
 * \pre         Parameter \p uart_config must be a non-NULL pointer.
 * \pre         Parameter \p config must be a non-NULL pointer.
 * \pre         Parameter \p description must be a non-NULL pointer.
 */
int lindev_generic_init(
    lindev_generic_t *lindev_generic,
    CONNECTOR *connector,
    const lindev_uart_config_t *uart_config,
    const lindev_generic_config_t *config,
    const lindev_generic_descriptor_t *description);

void lindev_generic_process_event(
    lindev_generic_t *context,
    const lindev_event_id_t event_id,
    size_t frame_index);

/**
 * @brief       Disable LIN communication of the LIN connector
 *
 * This function will temporarily disable communication of the LIN connector. The connector will
 * still receive and process DDMP2 events, but the communication to LIN bus is not enabled.
 *
 * Typically this function is to be called before the special firmware state, like OTA state to
 * minimize the impact of LIN execution on OTA stability.
 */
void lindev_generic_disable(lindev_generic_t *lindev_generic);

/**
 * @brief       Enable LIN communication of the LIN connector
 *
 * This function will re-enable the previosly disabled LIN communication. It is forbidden to call
 * this function without first calling the connector_lindev_disable function.
 */
void lindev_generic_enable(lindev_generic_t *lindev_generic);

/**
 * @brief       Set a new serial number value to device configuration
 *
 * @param       lindev_generic Pointer to \ref lindev_generic structure instance.
 * @param       serial_number New serial number to be stored in connector_lindev. Once the
 *              diagnostic message Read-By-Identifier Serial Number is received this number will be
 *              part of reply message.
 */
void lindev_generic_set_serial_number(lindev_generic_t *lindev_generic, uint32_t serial_number);

/**
 * @brief       Reset the LIN slave configuration to default one
 *
 * @param       lindev_generic Pointer to \ref lindev_generic structure instance.
 *
 * It is possible that LIN master configure LIN slave configuration. This configuration is preserved
 * between system reboots at it will take over the default configuration once it is set. Use this
 * function to disregard any LIN master set configuration and return the configuration to the
 * default set by connector.
 *
 * This function will reset:
 * - NAD
 */
void lindev_generic_reset_config(lindev_generic_t *lindev_generic);

#endif /* LINDEV_GENERIC_H_ */
