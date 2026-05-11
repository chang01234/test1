/*! \file connector_hmi.h
	\brief HMI connector for HMI platform
*/

#ifndef CONNECTOR_HMI_H_
#define CONNECTOR_HMI_H_

#include "configuration.h"

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "connector.h"
#include "ddm_wrapper.h"

extern CONNECTOR connector_hmi;

typedef void (*hmi_update_task_cb)(void);
typedef void (*hmi_ddmw_updated_cb)(uint8_t, int32_t);
typedef void(*hmi_menu_ddmp_set_cb)(uint32_t, uint8_t);
typedef void(*hmi_varstate_set_cb)(uint8_t);

/**
 * @brief Sets a new callback function
 *
 * This is called last in \a connector_hmi_task
 *
 * @param cb  new callback function
 *
 */
void set_hmi_update_task_cb(hmi_update_task_cb cb);

/**
 * @brief Sets a new callback function
 *
 * This is called when an updated ddmw parameters is found
 *
 */
void set_ddmw_updated_cb(hmi_ddmw_updated_cb cb);

/**
 * @brief Sets a new callback function
 *
 * This is called after executing a MENU_ACTION_DDMP_SET action
 *
 */
void set_hmi_ddmp_set_cb(hmi_menu_ddmp_set_cb cb);

/**
 * @brief Sets a new callback function
 *
 * This is called while executing a varstate_set call
 *
 */
void set_hmi_varstate_cb(hmi_varstate_set_cb cb);


//! \brief Call to the hmi_update_task_cb
void hmi_product_update_task(void);

ddmw_item_t *connector_hmi_get_item_from_var_index(bool is_pub_item, uint8_t var_index);

//! \brief Sends generic data frame to HMI connector
void send_generic_frame_to_connector(uint32_t id, const void * data, const size_t data_size, uint32_t timeout);
#endif /* CONNECTOR_HMI_H */
