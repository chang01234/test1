/*! \file connector_lin_server.c
	\brief Connector for CIBus master

	\author Stefan.Henningsohn
	\author Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
*/

#include "configuration.h"
#include "broker.h"

#include "connector_lin_server.h"
#include "lin_server_device_definition.h"
#include "lin_server_scheduler.h"
#include "lin_server_uart.h"
#include "lin_server.h"
#include "lin_server_production.h"

#include "ddm_store.h"

/* Private macros */
#define WORKER_STACK_SIZE               3094
#define WORKER_PRIORITY                 xTASK_PRIORITY_NORMAL

/* Private types */

/**
 * @brief		LIN server descriptor
 * 
 * This is used to store lin server descriptor data.
 * 
 * @note		All members of this structure are private to LIN server descriptor.
 * @note		Type definition is for internal LIN server module usage.
 */
typedef struct lin_server_descriptor
{
	size_t worker_stack_size;
	uint32_t worker_priority;
	const lin_server_slave_devices_t * lin_server_slave_devices;
	const size_t lin_server_slave_devices_size;
} lin_server_descriptor_t;

/* Private functions */
static int connector_lin_server_init(void);
static void connector_lin_server_task(void * context);
static const lin_server_slave_device_t * connector_lin_server_find_device_by_production_class(uint32_t parameter_class_instance);
static const lin_server_slave_device_t * connector_lin_server_find_device_by_device_class(uint32_t parameter_class_instance);
static const lin_server_slave_device_t * connector_lin_server_find_device_on_ddm_request(DDMP2_FRAME * ddmp2_frame);

static void handle_generic_event(const DDMP2_FRAME * ddmp2_frame);
static void broker_handle_set(const lin_server_slave_device_t * device, const DDMP2_FRAME * ddmp2_frame);
static void broker_handle_subscribe(const lin_server_slave_device_t * device, const DDMP2_FRAME * ddmp2_frame);

/******* LIN server configuration start *******/
static const lin_server_scheduler_table_item_t lin_server_scheduler_table_items[] =
{
#ifdef LIN_SERVER_DEVICE_FRESHJET_2200_3000
	{ .frame_id = DOMETIC_FRESHJET_CTRL_FRAME_ID,      .frame_type = LIN_CONTROL_FRAME,    .slave_device_type = LIN_SERVER_DEVICE_TYPE_FRESHJET_2200_3000, },
	{ .frame_id = DOMETIC_FRESHJET_INFO_FRAME_ID,      .frame_type = LIN_INFO_FRAME,       .slave_device_type = LIN_SERVER_DEVICE_TYPE_FRESHJET_2200_3000, },
#endif
#ifdef LIN_SERVER_DEVICE_APAC_AC
	{ .frame_id = DOMETIC_APAC_AC_CTRL_FRAME_ID,       .frame_type = LIN_CONTROL_FRAME,    .slave_device_type = LIN_SERVER_DEVICE_TYPE_APAC_AC, },
	{ .frame_id = DOMETIC_APAC_AC_INFO_FRAME_ID,       .frame_type = LIN_INFO_FRAME,       .slave_device_type = LIN_SERVER_DEVICE_TYPE_APAC_AC, },
#endif
#ifdef LIN_SERVER_DEVICE_BRIDGE_NRX
	{ .frame_id = DOMETIC_BRIDGE_NRX_CTRL_FRAME_ID,    .frame_type = LIN_CONTROL_FRAME,    .slave_device_type = LIN_SERVER_DEVICE_TYPE_BRIDGE_NRX, },
	{ .frame_id = DOMETIC_BRIDGE_NRX_INFO_FRAME_ID,    .frame_type = LIN_INFO_FRAME,       .slave_device_type = LIN_SERVER_DEVICE_TYPE_BRIDGE_NRX, },
#endif
#ifdef LIN_SERVER_DEVICE_TEMPRA_BATTERY
	{ .frame_id = TEMPRA_INFO_FRAME_IBS_FRM2_ID,       .frame_type = LIN_INFO_FRAME,       .slave_device_type = LIN_SERVER_DEVICE_TYPE_TEMPRA, },
	{ .frame_id = TEMPRA_INFO_FRAME_IBS_FRM6_ID,       .frame_type = LIN_INFO_FRAME,       .slave_device_type = LIN_SERVER_DEVICE_TYPE_TEMPRA, },
	{ .frame_id = TEMPRA_INFO_FRAME_IBS_FRM5_ID,       .frame_type = LIN_INFO_FRAME,       .slave_device_type = LIN_SERVER_DEVICE_TYPE_TEMPRA, },
#endif
};

static lin_server_scheduler_table_item_def_t lin_server_scheduler_table_items_def[ELEMENTS(lin_server_scheduler_table_items)];

static const lin_server_slave_devices_t lin_server_slave_device[] =
{
#ifdef LIN_SERVER_DEVICE_FRESHJET_2200_3000
	{ .slave_device = &lin_server_freshjet_2200_3000_device },
#endif
#ifdef LIN_SERVER_DEVICE_APAC_AC
	{ .slave_device = &lin_server_apac_ac_device },
#endif
#ifdef LIN_SERVER_DEVICE_BRIDGE_NRX
	{ .slave_device = &lin_server_bridge_nrx_device },
#endif
#ifdef LIN_SERVER_DEVICE_TEMPRA_BATTERY
	{ .slave_device = &lin_server_tempra_device },
#endif
};

static const lin_server_descriptor_t lin_server_descriptor =
{
	.worker_priority = WORKER_PRIORITY,
	.worker_stack_size = WORKER_STACK_SIZE,
	.lin_server_slave_devices = lin_server_slave_device,
	.lin_server_slave_devices_size = ELEMENTS(lin_server_slave_device),
};

/******* LIN server configuration end *******/

/* Implementation */
size_t lin_server_get_number_of_slave_devices(void)
{
	return lin_server_descriptor.lin_server_slave_devices_size;
}

const lin_server_slave_device_t * lin_server_get_slave_device_by_index(uint32_t slave_device_index)
{
	return (slave_device_index < lin_server_descriptor.lin_server_slave_devices_size ? lin_server_descriptor.lin_server_slave_devices[slave_device_index].slave_device : NULL);
}

const lin_server_slave_device_t * lin_server_get_slave_device(lin_server_device_type_t slave_device_type)
{
	for (size_t numb_devices = 0; numb_devices < lin_server_descriptor.lin_server_slave_devices_size; numb_devices++)
	{
		if (lin_server_descriptor.lin_server_slave_devices[numb_devices].slave_device->device_type == slave_device_type)
		{
			return lin_server_descriptor.lin_server_slave_devices[numb_devices].slave_device;
		}
	}

	return NULL;
}

int lin_server_register_ddm_class_instance(uint32_t device_class)
{
	int instance = -1;

	instance = broker_register_instance(&device_class, connector_lin_server.connector_id);
	if (instance == -1)
	{
		LOG(E, "Registration of class [0x%X] not successful!", device_class);
		return -1;
	}

	return instance;
}

void lin_server_delete_ddm_class_instance(uint32_t device_class, uint32_t instance)
{
	TRUE_CHECK(connector_send_frame_to_broker(
		DDMP2_CONTROL_PUBLISH,
		device_class | DDM2_PARAMETER_INSTANCE(instance),
		&Zero,
		sizeof(Zero),
		connector_lin_server.connector_id,
		portMAX_DELAY));
}

int lin_server_publish(uint32_t parameter, uint32_t instance, const void * const value, size_t value_size)
{
	int status;

	TRUE_CHECK(status = connector_send_frame_to_broker(
		DDMP2_CONTROL_PUBLISH,
		parameter | DDM2_PARAMETER_INSTANCE(instance),
		value,
		value_size,
		connector_lin_server.connector_id,
		portMAX_DELAY));

	return status;
}

void lin_server_generic_event(uint32_t event_id, const void * const value, size_t value_size)
{
	TRUE_CHECK(connector_send_frame_to_connector(
		DDMP2_CONTROL_GENERIC,
		event_id,
		value,
		value_size,
		connector_lin_server.connector_id,
		portMAX_DELAY));
}

static void broker_handle_set(const lin_server_slave_device_t * device, const DDMP2_FRAME * ddmp2_frame)
{
	uint32_t parameter = ddmp2_frame->frame.set.parameter;

	if (IS_PRODUCTION_PARAMETER_REQUEST(parameter))
	{
		lin_server_production_handle_set(
			device,
			DDM2_PARAMETER_BASE_INSTANCE(parameter),
			&ddmp2_frame->frame.set.value,
			ddmp2_value_size(ddmp2_frame));
	}
	else
	{
		lin_server_handle_set(
			device,
			DDM2_PARAMETER_BASE_INSTANCE(parameter),
			&ddmp2_frame->frame.set.value,
			ddmp2_value_size(ddmp2_frame));
	}
}

static void broker_handle_subscribe(const lin_server_slave_device_t * device, const DDMP2_FRAME * ddmp2_frame)
{
	uint32_t parameter = ddmp2_frame->frame.subscribe.parameter;

	if (IS_PRODUCTION_PARAMETER_REQUEST(parameter))
	{
		lin_server_production_handle_subscribe(device, DDM2_PARAMETER_BASE_INSTANCE(parameter));
	}
	else
	{
		lin_server_handle_subscribe(device, DDM2_PARAMETER_BASE_INSTANCE(parameter));
	}
}

static void handle_generic_event(const DDMP2_FRAME * ddmp2_frame)
{
	lin_server_handle_generic(
		ddmp2_frame->frame.generic.id,
		ddmp2_frame->frame.generic.data,
		ddmp2_value_size(ddmp2_frame));
}

static const lin_server_slave_device_t * connector_lin_server_find_device_by_device_class(uint32_t parameter_class_instance)
{
	const lin_server_slave_device_t * slave_device = NULL;

	for (size_t index = 0; index < lin_server_descriptor.lin_server_slave_devices_size; ++index)
	{
		const lin_server_slave_device_t * next_slave_device = lin_server_descriptor.lin_server_slave_devices[index].slave_device;
		
		uint32_t device_class = next_slave_device->device_class;
		int instance = next_slave_device->data->class_instance;
		uint32_t class_instance = device_class | DDM2_PARAMETER_INSTANCE(instance);

		if (class_instance == parameter_class_instance)
		{
			slave_device = next_slave_device;
			break;
		}
	}

	return slave_device;
}

static const lin_server_slave_device_t * connector_lin_server_find_device_by_production_class(uint32_t parameter_class_instance)
{
	const lin_server_slave_device_t * slave_device = NULL;

	for (size_t index = 0; index < lin_server_descriptor.lin_server_slave_devices_size; ++index)
	{
		const lin_server_slave_device_t * next_slave_device = lin_server_descriptor.lin_server_slave_devices[index].slave_device;

		int instance = next_slave_device->data->prod_instance;
		uint32_t prod_class_instance = PROD0 | DDM2_PARAMETER_INSTANCE(instance);

		if (prod_class_instance == parameter_class_instance)
		{
			slave_device = next_slave_device;
			break;
		}
	}

	return slave_device;
}

static const lin_server_slave_device_t * connector_lin_server_find_device_on_ddm_request(DDMP2_FRAME * ddmp2_frame)
{
	uint32_t parameter = UINT32_MAX;
	const lin_server_slave_device_t * slave_device = NULL;

	switch (ddmp2_frame->frame.control)
	{
	case DDMP2_CONTROL_SUBSCRIBE:
		parameter = ddmp2_frame->frame.subscribe.parameter;
		break;
	case DDMP2_CONTROL_SET:
		parameter = ddmp2_frame->frame.set.parameter;
		break;
	default:
		break;
	}

	uint32_t parameter_class_instance = DDM2_PARAMETER_CLASS_INSTANCE(parameter);
	if (IS_PRODUCTION_PARAMETER_REQUEST(parameter))
	{
		slave_device = connector_lin_server_find_device_by_production_class(parameter_class_instance);
	}
	else
	{
		slave_device = connector_lin_server_find_device_by_device_class(parameter_class_instance);
	}

	if (slave_device == NULL)
	{
		LOG(E, "Device not found for parameter[0x%X]!", parameter);
	}

	return slave_device;
}

//! \~ Main lin server task
static void connector_lin_server_task(void * args)
{
	(void)args;

	// Initialize LIN Server UART peripheral
	lin_server_uart_init();

	lin_server_scheduler_init(
		lin_server_scheduler_table_items,
		lin_server_scheduler_table_items_def,
		ELEMENTS(lin_server_scheduler_table_items));

	lin_server_init();

	// Create LIN server UART task and create/start Scheduler timer
	lin_server_uart_start();

	for (;;)
	{
		DDMP2_FRAME * ddmp2_frame;
		size_t item_size = 0;

		ddmp2_frame = xRingbufferReceive(connector_lin_server.to_connector, &item_size, portMAX_DELAY);
		if (ddmp2_frame == NULL)
		{
			LOG(W, "%s: got NULL frame", connector_lin_server.name);
			continue;
		}

		switch (ddmp2_frame->frame.control)
		{
		case DDMP2_CONTROL_SUBSCRIBE:
		{
			const lin_server_slave_device_t * slave_device = connector_lin_server_find_device_on_ddm_request(ddmp2_frame);
			if (slave_device)
			{
				broker_handle_subscribe(slave_device, ddmp2_frame);
			}
			break;
		}
		case DDMP2_CONTROL_SET:
		{
			const lin_server_slave_device_t * slave_device = connector_lin_server_find_device_on_ddm_request(ddmp2_frame);
			if (slave_device)
			{
				broker_handle_set(slave_device, ddmp2_frame);
			}
			break;
		}
		case DDMP2_CONTROL_GENERIC:
		{
			handle_generic_event(ddmp2_frame);
			break;
		}
		case DDMP2_CONTROL_NOP:
		case DDMP2_CONTROL_FRAGMENT:
		case DDMP2_CONTROL_MESSAGE:
		case DDMP2_CONTROL_REG:
		case DDMP2_CONTROL_COUNT:
		default:
			LOG(W, "%s: unhandled frame %u",
				connector_lin_server.name,
				ddmp2_frame->frame.control);
			break;
		}

		vRingbufferReturnItem(connector_lin_server.to_connector, ddmp2_frame);
	}
}

static int connector_lin_server_init(void)
{
	BaseType_t error;

	error = xTaskCreate(
		connector_lin_server_task,
		connector_lin_server.name,
		lin_server_descriptor.worker_stack_size,
		NULL,
		lin_server_descriptor.worker_priority,
		NULL);

	return error == pdPASS ? CONNECTOR_INIT_SUCCESS : CONNECTOR_INIT_FAILURE;
}

CONNECTOR connector_lin_server =
{
	.name = "LIN server",
	.initialize = connector_lin_server_init,
};
