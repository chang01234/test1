/**
 * \file        event_manager.h
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event manager interface
 *
 * \li          2024-07-05  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef EVENT_MANAGER_H_
#define EVENT_MANAGER_H_

#include "connector.h"
#include "ddm_store.h"
#include "event_record.h"
#include "event_record_db.h"
#include "event_type_db.h"
#include "inventory_handler.h"
#include "sorted_container.h"
#include "sorted_list.h"
#include <stddef.h>
#include <stdint.h>

#define EVENT_MANAGER_NO_ERROR           0
#define EVENT_MANAGER_ERROR_NO_INIT      1
#define EVENT_MANAGER_ERROR_NO_FREE_SLOT 2
#define EVENT_MANAGER_ERROR_NULL_ARG     3
#define EVENT_MANAGER_ERROR_FAILURE      4

#define EVENT_MANAGER_OWNED_DDM_COUNT 7

/**
 * @brief       Listener events
 */
typedef enum event_manager_listener_event
{
    EVENT_MANAGER_LISTENER_EVENT_NEW,         //!< New event was generated
    EVENT_MANAGER_LISTENER_EVENT_ACK,         //!< Event was acknoweldged
    EVENT_MANAGER_LISTENER_EVENT_DELETE_ALL,  //!< Command to delete all was requested
    EVENT_MANAGER_LISTENER_EVENT_ACK_ALL,     //!< Command to acknowledge all events
} event_manager_listener_event_t;

/**
 * @brief       Listener callback function prototype
 *
 * This callback is called when a listen event occurs:
 * - new event was generated,
 * - command to delete all records was requested.
 *
 * Callback receives the following arguments:
 * 1. void * argument which was passed to @ref event_manager_register_event_listener_cb
 * 2. enumeration of event which happened, see @ref event_manager_listener_event_t
 * 3. event_id in case a new event occured.
 */
typedef void(event_manager_listener_cb)(
    void *,
    event_manager_listener_event_t,
    event_id_t);

/**
 * @brief       This is a context structure for event manager.
 *
 * It contains all things bundled together so they are accessible from a single
 * pointer.
 */
typedef struct event_manager_context
{
    CONNECTOR *p_connector;                                               //!< Pointer to Broker connector handle
    ddm_store_t owned_ddm_store;                                          //!< DDM store handle for owned EVM and EVN DDM parameters
    ddm_store_t other_ddm_store;                                          //!< DDM store handle for other DDM parameters
    event_type_db_t event_type_db;                                        //!< Database with all known event types
    event_record_db_t event_record_db;                                    //!< Database with event records
    inventory_handler_t inventory_handler;                                //!< Inventory handler for other DDM parameters
    char version[CONNECTOR_EVENT_NOTIFICATION_VERSION_LENGTH];            //!< Version of event manager connector
    char product_name[CONNECTOR_EVENT_NOTIFICATION_PRODUCT_NAME_LENGTH];  //!< Product name string
    event_id_t event_id;                                                  //!< Latest event ID, saved/restored from NVS
    uint32_t acknowledged;                                                //!< Number of acknowledged records, saved/restored from NVS
    event_manager_listener_cb *p_event_listener;                          //!< Pointer to registered event listener function
    void *p_event_listener_arg;                                           //!< Optional argument for event listener callback function.
    enum event_menager_state
    {
        EVENT_MANAGER_STATE_UNCONFIGURED,  //!< Event manager has not been configured yet.
        EVENT_MANAGER_STATE_CONFIGURED,    //!< Event manager has been successfully configured and is ready.
    } state;                               //!< Internal state of the event manager
} event_manager_t;

/**
 * @brief       Description structure of event manager instance
 *
 * All members of this structure need to be defined (non-NULL pointers). The
 * structure is used only during the initialization.
 */
typedef struct event_manager_description
{
    CONNECTOR *p_connector;                       //!< Connector that uses this instance.
    SORTED_CONTAINER *p_event_type_db_storage;    //!< Sorted container storage for event types
    SORTED_CONTAINER *p_event_record_db_storage;  //!< Event record database requires SORTED_CONTAINER and event_id_t array
    event_id_t *p_event_record_db_fifo_storage;   //!< The length of this array must be equal to p_event_record_db_storage capacity.
    SORTED_LIST *p_inventory_handler_storage;     //!< Inventory handler storage
    SORTED_CONTAINER *p_owned_ddm_store_storage;  //!< Capacity must be equal to EVENT_MANAGER_OWNED_DDM_COUNT
    SORTED_CONTAINER *p_other_ddm_store_storage;  //!< Other DDM store storage
} event_manager_description_t;

/**
 * @brief       Initilize the event manager connector.
 *
 * @param       evm is a pointer to event manager instance that needs to be
 *              initialized.
 * @param       desc is a pointer to descriptor instance containing all needed
 *              information to setup event manager connector.
 * @return      Operation status.
 * @retval      EVENT_MANAGER_NO_ERROR is returned when operation completed
 *              successfully.
 * @retval      EVENT_MANAGER_ERROR_NULL_ARG is returned when an argument is
 *              NULL pointer or if a member in @ref event_manager_description_t
 *              is a NULL pointer.
 * @retval      EVENT_MANAGER_ERROR_FAILURE is returned when an initialization
 *              operation has failed.
 *
 * @pre         Parameter @a evm must be non-NULL pointer.
 * @pre         Parameter @a desc must be non-NULL pointer.
 */
int event_manager_connector_init(
    event_manager_t *evm,
    const event_manager_description_t *desc);

/**
 * @brief       Process task for event manager connector.
 *
 * @param       evm is a pointer to event manager instance initialized previosly
 *              by @ref event_manager_connector_init.
 * @param       frame is a pointer to DDMP2 frame.
 *
 * @pre         Parameter @a evm must be non-NULL pointer.
 * @pre         Parameter @a frame must be non-NULL pointer.
 */
void event_manager_conector_process_task(event_manager_t *evm, const DDMP2_FRAME *frame);

/**
 * @brief       Configure the event manager connector.
 *
 *              After configuring the event manager connector, it will start processing events and frames.
 *
 * @param       evm is a pointer to event manager instance initialized previosly
 *              by @ref event_manager_connector_init.
 * @param       configuration_json is a pointer to a string buffer containing configuration JSON.
 *
 * @pre         Parameter @a evm must be non-NULL pointer.
 * @pre         Parameter @a configuration_json must be non-NULL pointer.
 */
int event_manager_load_configuration(event_manager_t *evm, const char *configuration_json);

/**
 * @brief       Register an event listener callback with event menagers
 *
 * This function will register a provided callback which is called whenever a
 * new event is generated. In case there are multiple instances of event
 * managers events from all events managers are passed to same listener.
 *
 * @param       listener is a pointer to callback function.
 * @param       listener_arg is a optional pointer which is passed to listener
 *              callback function.
 * @return      Operation status.
 * @retval      EVENT_MANAGER_NO_ERROR is returned when the operation is
 *              successful.
 * @retval      EVENT_MANAGER_ERROR_NO_FREE_SLOT is returned when a callback is
 *              already registered. It is possible to register only one
 *              callback.
 * @retval      EVENT_MANAGER_ERROR_NO_INIT is returned when this function is
 *              called before @ref event_manager_connector_init function.
 *
 * @pre         Parameter @a listener must be non-NULL pointer.
 */
int event_manager_register_event_listener_cb(event_manager_listener_cb *listener, void *listener_arg);

/**
 * @brief       Find and retreive a record by its ID.
 *
 * @param       event_id is event identifier of a record.
 * @param       er is a pointer to allocated event_record_t instance that will
 *              contain event record data. This instance then must be terminated
 *              with @ref event_record_terminate function.
 * @return      Operation status.
 * @retval      EVENT_MANAGER_NO_ERROR is returned when the operation is
 *              successful.
 * @retval      EVENT_MANAGER_ERROR_FAILURE is returned when this function is
 *              unable to find event_record with given @a event_id.
 * @retval      EVENT_MANAGER_ERROR_NO_INIT is returned when this function is
 *              called before @ref event_manager_connector_init function.
 *
 * @pre         Parameter @a er must be non-NULL pointer.
 */
int event_manager_find_record_by_id(event_id_t event_id, event_record_t *er);

/**
 * @brief       Find and retreive a event type by its ID.
 *
 * @param       type_id is event type identifier.
 * @param       et is a pointer to allocated event_type_t instance that will
 *              contain event type data. This instance then must be terminated
 *              with @ref event_type_terminate function.
 * @return      Operation status.
 * @retval      EVENT_MANAGER_NO_ERROR is returned when the operation is
 *              successful.
 * @retval      EVENT_MANAGER_ERROR_FAILURE is returned when this function is
 *              unable to find event_type with given @a type_id.
 * @retval      EVENT_MANAGER_ERROR_NO_INIT is returned when this function is
 *              called before @ref event_manager_connector_init function.
 *
 * @pre         Parameter @a er must be non-NULL pointer.
 */
int event_manager_find_type_by_id(uint32_t type_id, event_type_t *et);

/**
 * @brief       Find and acknowledge an event record by its ID.
 *
 * @param       event_id is event identifier of a record.
 * @return      Operation status.
 * @retval      EVENT_MANAGER_NO_ERROR is returned when the operation is
 *              successful.
 * @retval      EVENT_MANAGER_ERROR_FAILURE is returned when this function is
 *              unable to find event_type with given @a type_id.
 * @retval      EVENT_MANAGER_ERROR_NO_INIT is returned when this function is
 *              called before @ref event_manager_connector_init function.
 *
 * @pre         Parameter @a er must be non-NULL pointer.
 */
int event_manager_ack_record_by_id(event_id_t event_id);

/**
 * @brief       Get device ID string.
 *
 * @return      Pointer to device ID string.
 *
 * @pre         Parameter @a evm must be non-NULL pointer.
 */
const char *event_manager_get_device_id(void);

/**
 * @brief       Get product name string.
 *
 * @param       evm is a pointer to event manager instance.
 *
 * @return      Pointer to product name string.
 *
 * @pre         Parameter @a evm must be non-NULL pointer.
 */
const char *event_manager_get_product_name(void);

#endif /* EVENT_MANAGER_H_ */
