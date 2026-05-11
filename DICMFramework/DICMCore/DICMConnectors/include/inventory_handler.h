/**
 * \brief Inventory handler helper, parses the GW0INV and call the callbacks when a change is
 * detected.
 */

#ifndef INVENTORY_HANDLER_H_
#define INVENTORY_HANDLER_H_

#include <stdbool.h>
#include <stdint.h>
#include "sorted_list.h"
#include "connector.h"

/**
 * \brief Inventory handler available callback function
 *
 * This function is called by inventory handler when a handled device class instance becomes
 * available or unavailable.
 *
 * The first argument passed to this function is just a `void` pointer that was given during the
 * inventory handler initialization to \ref inventory_handler_init function.
 *
 * The second argument is the device_class_instance that has become available or unavailable.
 *
 * The third argument is boolean stating if the provided device_class_instance has become available
 * or not.
 */
typedef void (inventory_handler_available_t)(void * argument, uint32_t device_class_instance, bool is_available);

/**
 * \brief Inventory handler structure
 *
 * All members of this structure are private to inventory handler component.
 */
typedef struct inventory_handler
{
	SORTED_LIST * sorted_list;
	inventory_handler_available_t * cb_available;
	void * cb_argument;
} inventory_handler_t;

/**
 * \brief Initialize inventory handler
 *
 * Call this function before all other functions to initialize the inventory handler instance. In
 * order to initialize the inventory handler instance you need to allocate a sorted list instance
 * of sufficient size to hold needed device class instance records.
 *
 * \param	ih Pointer to inventory handler structure instance that needs to be initialized
 * \param	sorted_list Pointer to declared sorted_list. See macros in "sorted_list.h" to
 *			create an instance of sorted list. Provide a sorted list of sufficient size to
 *			store necessary number of device class instances. Adding of device class instances
 *			is done by \ref inventory_handler_add function.
 * \param	cb_available Pointer to function which will be called when the handled
 *			device_class_instance becomes available or unavailable.
 * \param	cb_argument User defined pointer to callback arguments. This argument pointer is just
 *			passed to the callback when it is called.
 * \retval	0 - Operation successful
 * \retval	1 - Operation was not successful
 */
int inventory_handler_init(
	inventory_handler_t * const ih,
	SORTED_LIST * const sorted_list,
	inventory_handler_available_t * const cb_available,
	void * const cb_argument);

/**
 * \brief	Add device_class_instance to be handled by inventory handler
 *
 * Use this function to add a device class instance to sorted_list which will be managed by
 * inventory_handler. Call this function multiple times with desired device_class_instances before
 * starting the inventory handler with \ref inventory_handler_start function.
 *
 * @note        If the entry already exists for given device_class_instance then this function will
 *              just reuse the existing entry.
 *
 * \param       ih Pointer to initialized inventory handler instance
 * \param       device_class_instance Desired device class
 * \return      Operation status
 * \retval      0 - Operation successful
 * \retval      -1 - Operation was not successful
 */
int inventory_handler_add(inventory_handler_t * ih, uint32_t device_class_instance);

/**
 * \brief   Add any device_class_instance to be handled by inventory handler
 *
 * Use this function to add any instances if a device class to sorted_list which will be managed by
 * inventory_handler. Call this function multiple times with desired device_class_instances before
 * starting the inventory handler with \ref inventory_handler_start function.
 *
 * @note        If the entry already exists for given device_class_base_instance then this function will
 *              just reuse the existing entry.
 *
 * \param       ih Pointer to initialized inventory handler instance
 * \param       device_class_base_instance Desired device class
 * \return      Operation status
 * \retval      0 - Operation successful
 * \retval      -1 - Operation was not successful
 */
int inventory_handler_add_any(inventory_handler_t * ih, uint32_t device_class_base_instance);

/**
 * \brief		Remove device_class_instance to be handled by inventory handler
 *
 * Use this function to remove device_class_instances to sorted_list which will be managed by
 * inventory_handler.
 *
 * \param	ih Pointer to initialized inventory handler instance
 * \param	device_class_instance Removed device class
 * \retval	0 - Operation successful
 * \retval	1 - Operation was not successful
 */
int inventory_handler_remove(inventory_handler_t * const ih, const uint32_t device_class_instance);

/**
 * \brief Start the inventory handler
 *
 * This function will inventory to Inventory. Call this function just before DDM Event Loop and
 * after setting a desired device_class_instance.
 *
 * \param	ih Pointer to initialized inventory handler instance
 * \param	connector Pointer to connector instance
 * \retval	0 - Operation successful
 * \retval	1 - Operation was not successful
 */
int inventory_handler_start(MAYBE_UNUSED inventory_handler_t * const ih, const CONNECTOR * const connector);

/**
 * \brief Update subscriptions
 *
 * Call this function inside the DDM Event Loop when a DDM gets published. If the published DDM is
 * GW0INV this function will process it, otherwise, it just exits so it is safe to be called even
 * when DDM parameter is not GW0INV. If the Inventory gets updated, a transition is detected, a
 * callback function cb_available will be called.
 *
 * \param	ih Pointer to initialized inventory handler instance
 * \param	pframe Pointer to DDMP frame.
 * \retval	1 - Frame was an inventory update
 * \retval	0 - Frame was not an inventory update
 */
int inventory_handler_update(inventory_handler_t * const ih, const DDMP2_FRAME * const pframe);

#endif /* INVENTORY_HANDLER_H_ */
