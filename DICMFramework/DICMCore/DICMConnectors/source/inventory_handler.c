/**
 * \brief Inventory handler helper, parses the GW0INV and call the callbacks when a change is
 *        detected.
 */

#include "configuration.h"
#include "inventory_handler.h"
#include "ddm2_parameter_list.h"
#include "iGeneralDefinitions.h"

#define INVENTORY_HANDLER_ALL_INSTANCES     DDM2_PARAMETER_INSTANCE(0xFF)

int inventory_handler_init(
	inventory_handler_t * const ih,
	SORTED_LIST * const sorted_list,
	inventory_handler_available_t * const cb_available,
	void * const cb_argument)
{
	TRUE_CHECK_RETURN1(ih != NULL);
	TRUE_CHECK_RETURN1(cb_available != NULL);
	TRUE_CHECK_RETURN1(sorted_list != NULL);

	ih->sorted_list = sorted_list;
	ih->cb_available = cb_available;
	ih->cb_argument = cb_argument;

	return 0;
}

int inventory_handler_add(inventory_handler_t * ih, uint32_t device_class_instance)
{
    SORTED_LIST_RETURN_VALUE retval;
    retval = sorted_list_unique_add(ih->sorted_list, DDMP2_INVENTORY_CLASS_INSTANCE(device_class_instance), 0);
    if (retval == SORTED_LIST_FAIL)
    {
        return -1;
    }
    return 0;
}

int inventory_handler_add_any(inventory_handler_t * ih, uint32_t device_class_base_instance)
{
    SORTED_LIST_RETURN_VALUE retval;
    // We mask out any possible class intances and add special one 0xFF that means all class instances
    uint32_t device_class_instance = DDMP2_INVENTORY_CLASS(device_class_base_instance) | INVENTORY_HANDLER_ALL_INSTANCES;
    retval = sorted_list_unique_add(ih->sorted_list, DDMP2_INVENTORY_CLASS_INSTANCE(device_class_instance), 0);
    if (retval == SORTED_LIST_FAIL)
    {
        return -1;
    }
    return 0;
}

int inventory_handler_remove(inventory_handler_t * const ih, const uint32_t device_class_instance)
{
	TRUE_CHECK_RETURN1(ih != NULL);

	return SORTED_LIST_FAIL == sorted_list_unique_remove(ih->sorted_list, DDMP2_INVENTORY_CLASS_INSTANCE(device_class_instance));
}

int inventory_handler_start(MAYBE_UNUSED inventory_handler_t * const ih, const CONNECTOR * const connector)
{
	int Inventory_subscribe_status = connector_send_frame_to_broker(
		/* control */		DDMP2_CONTROL_SUBSCRIBE,
		/* parameter */		GW0INV,
		/* value */			NULL,
		/* value_size */	0,
		/* connector */		connector->connector_id,
		/* timeout */		portMAX_DELAY);

	if (!Inventory_subscribe_status)
	{
		return 1;
	}

	return 0;
}

int inventory_handler_update(inventory_handler_t * const ih, const DDMP2_FRAME * const pframe)
{
	TRUE_CHECK_RETURN1(ih != NULL);
	TRUE_CHECK_RETURN1(pframe != NULL);

	switch (pframe->frame.control)
	{
	case DDMP2_CONTROL_PUBLISH:
		if (pframe->frame.publish.parameter != GW0INV)
		{
			return 0;	// Not an inventory update
		}

		const size_t Frame_size = ddmp2_value_size(pframe);

		for (size_t s = 0; s < Frame_size; s += sizeof(uint32_t))
		{
			const uint32_t Inventory_token = *(const uint32_t *)&pframe->frame.publish.value.raw[s];
			const uint32_t Device_class_instance = DDMP2_INVENTORY_CLASS_INSTANCE(Inventory_token);
			const uint32_t New_available_status = DDMP2_INVENTORY_AVL(Inventory_token);

			int entry_position;
			const int Entry_found = sorted_list_key_search(&entry_position, ih->sorted_list, Device_class_instance);	// Search for the device class instance in the sorted list

			if (!Entry_found)	// Device class instance not among the managed ones
			{
                // check wildcard entry
                if (!sorted_list_key_search(&entry_position, ih->sorted_list, DDMP2_INVENTORY_CLASS(Device_class_instance) | INVENTORY_HANDLER_ALL_INSTANCES))
                {
                    continue;
                }
			}

			ih->cb_available(ih->cb_argument, Device_class_instance, New_available_status);	// Notify the user of the inventory update
		}
		return 1;
	default:	// Not a publish frame
		return 0;
	}
}
