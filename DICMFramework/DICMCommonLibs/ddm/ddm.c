/*! \file ddm.c
	\brief Main DDMP source.

	Link layer functions.
 */

#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include "ddm.h"
#include "encapsulation.h"
#include "parameters.h"

//#define NO_WIRELESS_ACKS

extern size_t _stuff(void* destination, const void *data, size_t size);

static DDMP_SEND_ERROR_ENUM _ddmp_transmit(int intf, const DDMP_FRAME *frame);

//! \~ Pointer to global DDMP callback functions
const DDMP_CALLBACKS *ddmp_callbacks;

//! \~ Pointer to DDMP interface definitions
static const DDMP_INTERFACE *ddmp_interfaces;

//! \~ Number of interfaces pointed to by ddmp_interfaces
static size_t ddmp_inteface_count;

//! \~ Library is in sniffer mode; do not send anything
static int ddmp_listen_mode;

//! \~ DDMPlib has been initialized
static int initialized;

//! \~ Logical connection status per interface
DDMP_CONNECTION_STATUS ddmp_connection_status[DDMP_INTERFACE_COUNT];

//! \~ Ping frame
const DDMP_FRAME Ddmp_ping_frame =
{
		.control = DDMP_ACTION_PING,
		.size = DDMP_SIZE_CONTROL,
};

//! \~ Hello frame
const DDMP_FRAME Ddmp_hello_frame =
{
		.control = DDMP_ACTION_HELLO,
		.size = DDMP_SIZE_CONTROL,
};

//! \~ ACK frame
const DDMP_FRAME Ddmp_ack_frame =
{
		.control = DDMP_ACTION_ACK,
		.size = DDMP_SIZE_CONTROL,
};

//! \~ NAK frame
const DDMP_FRAME Ddmp_nak_frame =
{
		.control = DDMP_ACTION_NAK,
		.size = DDMP_SIZE_CONTROL,
};

//! \~ NOP frame
const DDMP_FRAME Ddmp_nop_frame =
{
		.control = DDMP_ACTION_NOP,
		.size = DDMP_SIZE_CONTROL,
};

//! \~ Description strings for DDMP errors
//! \sa DDMP_ERROR_ENUM
static const uint8_t *Ddmp_error_strings[DDMP_ERROR_COUNT + 1] =
{
		(uint8_t*)"DDMP_ERROR_S_WITHOUT_E: Sync byte encountered, expected end of previous frame",
		(uint8_t*)"DDMP_ERROR_E_WITHOUT_S: End byte encountered, but no frame started",
		(uint8_t*)"DDMP_ERROR_BUFFER_OVERFLOW: UART buffer size exceeded",
		(uint8_t*)"DDMP_ERROR_CRC: CRC calculation does not agree with supplied signature",
		(uint8_t*)"DDMP_ERROR_SHORT_FRAME: Frame too short to be valid (<4 bytes)",
		(uint8_t*)"DDMP_ERROR_BUFFER_TOO_SMALL: Frame too big for UART buffer",
		(uint8_t*)"DDMP_ERROR_TIMEOUT: Transmitted frame not acknowledged",
		(uint8_t*)"DDMP_ERROR_TIMEOUTS_EXCEEDED: Connection error",
		(uint8_t*)"DDMP_ERROR_FRAME_SIZE: Frame has an invalid size",
		(uint8_t*)"DDMP_ERROR_ARGUMENT: Illegal argument to function",
		(uint8_t*)"ddmp_error_string: Error number out of range",
};

//! \~ Description strings for DDMP send errors
//! \sa DDMP_SEND_ERROR_ENUM
static const uint8_t *Ddmp_send_error_strings[DDMP_SEND_ERROR_COUNT + 1] =
{
		(uint8_t*)"DDMP_SEND_ERROR_OK: Frame queued or sent successfully",
		(uint8_t*)"DDMP_SEND_ERROR_NO_CONNECTION: Underlying physical layer is disconnected",
		(uint8_t*)"DDMP_SEND_ERROR_QUEUE_FULL: Send queue is full",
		(uint8_t*)"DDMP_SEND_ERROR_QUEUE_EMPTY:  Tried to send from empty queue",
		(uint8_t*)"DDMP_SEND_ERROR_INTERFACE_ERROR: (unused)",
		(uint8_t*)"DDMP_SEND_ERROR_NOT_INITIALIZED: DDMP library is not initialized",
		(uint8_t*)"ddmp_send_error_string: Error number out of range",
};

//! \~ Strings of DDM parameter types
//! \sa DDMP_TYPES_ENUM
static const uint8_t *Ddm_type_format_strings[DDMP_TYPES_COUNT + 1] =
{
		(uint8_t*)"",
		(uint8_t*)"%"PRIu8,
		(uint8_t*)"%"PRIu16,
		(uint8_t*)"%"PRIu32,
		(uint8_t*)"%"PRIi8,
		(uint8_t*)"%"PRIi16,
		(uint8_t*)"%"PRIi32,
		(uint8_t*)"%s",
		(uint8_t*)"ERR",
};

//! \~ Strings of DDM parameter units
//! \sa DDMP_UNITS_ENUM
static const uint8_t *Ddm_unit_strings[DDMP_UNITS_COUNT + 1] =
{
		(uint8_t*)" ",
		(uint8_t*)" ",
		(uint8_t*)"ea",
		(uint8_t*)" ",
		(uint8_t*)" ",
		(uint8_t*)"degC",
		(uint8_t*)"V",
		(uint8_t*)"A",
		(uint8_t*)"ERR",
};

//! \~ Strings of DDMP actions
//! \sa DDMP_ACTIONS_ENUM
static const uint8_t *Ddmp_action_strings[DDMP_ACTION_COUNT + 1] =
{
		[DDMP_ACTION_PUBLISH] = (uint8_t*)"Publish",
		[DDMP_ACTION_SUBSCRIBE] = (uint8_t*)"Subscribe",
		[DDMP_ACTION_PING] = (uint8_t*)"Ping",
		[DDMP_ACTION_HELLO] = (uint8_t*)"Hello",
		[DDMP_ACTION_ACK] = (uint8_t*)"ACK",
		[DDMP_ACTION_NAK] = (uint8_t*)"NAK",
		[DDMP_ACTION_NOP] = (uint8_t*)"NOP",
		[DDMP_ACTION_COUNT] = (uint8_t*)"Unknown action",
};

/*! \brief Accept/copy a DDMP value from incoming frame
	\param dest Destination DDMP frame in parameter list to store value in
	\param src Source/incoming frame to copy value from
	\return true: Value was changed
	\return false: Value did not change
 */
int ddm_write_parameter(DDMP_FRAME *dest, const DDMP_FRAME *src)
{
	int different;

	if ((!src) || (!dest))
	{
		if (ddmp_callbacks->error_cb)
		{
			ddmp_callbacks->error_cb(DDMP_INTERFACE_NONE, DDMP_ERROR_ARGUMENT, ddmp_error_string(DDMP_ERROR_ARGUMENT));
		}

		return 0;
	}

	if ((src->size < DDMP_SIZE_PARAMETER) || (src->size > DDMP_FRAME_SIZE_MAX))	//assert sane frame size
	{
		if (ddmp_callbacks->error_cb)
		{
			ddmp_callbacks->error_cb(DDMP_INTERFACE_NONE, DDMP_ERROR_FRAME_SIZE, ddmp_error_string(DDMP_ERROR_FRAME_SIZE));
		}

		return 0;
	}

	DDMP_GLOBAL_LOCK;
	{
		if (dest->size != src->size)					//if size has changed, value is considered changed by default
		{
			different = 1;
			dest->size = src->size;							//update size from source
		}
		else
		{
			different = ddm_compare_values(dest, src);	//check if data has changed
		}

		if (different)
		{
			memcpy(dest->value, src->value, dest->size - DDMP_SIZE_PARAMETER);	//copy new data
		}

		DDMP_GLOBAL_UNLOCK;
	}

	return different;
}

//! \~  Compare new parameter value with current
int ddm_compare_values(const DDMP_FRAME *master, const DDMP_FRAME *candidate)
{
	return memcmp(master->value, candidate->value, master->size - DDMP_SIZE_PARAMETER);
}

//! \~ Update parameter value and publish it if different, assumes non-void parameter
void ddm_update_and_publish(int intf, DDMP_FRAME *dest, DDMP_FRAME *src)
{
	DDMP_FRAME copy;
	int different;

	if ((!src) || (!dest))
	{
		if (ddmp_callbacks->error_cb)
		{
			ddmp_callbacks->error_cb(intf, DDMP_ERROR_ARGUMENT, ddmp_error_string(DDMP_ERROR_ARGUMENT));
		}

		return;
	}

	if (!src->size)
	{
		src->size = dest->size;
	}

	if ((src->size < DDMP_SIZE_PARAMETER) || (src->size > DDMP_FRAME_SIZE_MAX))	//assert sane frame size
	{
		if (ddmp_callbacks->error_cb)
		{
			ddmp_callbacks->error_cb(intf, DDMP_ERROR_FRAME_SIZE, ddmp_error_string(DDMP_ERROR_FRAME_SIZE));
		}

		return;
	}

	DDMP_GLOBAL_LOCK;
	{
		if (src->size!=dest->size)	//if size is specified in source value frame and sizes are different, value is considered different
		{
			different = 1;
			dest->size = src->size;							//update size from source
		}
		else
		{
			different = ddm_compare_values(dest, src); // else check the actual value data
		}

		if (different)
		{
			memcpy(dest->value, src->value, dest->size - DDMP_SIZE_PARAMETER);
		}

		copy = *dest;	//make a copy of resulting frame
		DDMP_GLOBAL_UNLOCK;
	}

	if (different)
		ddmp_send_publish(intf, &copy);	//publish the copy if changed

}

/*! \brief Extract/copy a DDMP value from parameter list
	\param src Parameter frame to extract
	\return Copied frame
 */
DDMP_FRAME ddm_read_parameter(const DDMP_FRAME *src)
{
	DDMP_FRAME dest;

	DDMP_GLOBAL_LOCK;
	{
		dest = *src;
		DDMP_GLOBAL_UNLOCK;
	}
	
	return dest;
}

/*! \brief Check whether frame is containing the missing value for its data type
	\param index Index into master parameter list
	\param frame DDMP frame to check
	\return true: Frame has a missing value
	\sa Ddmp_parameter_data
 */
int ddm_missing_value(int index, const DDMP_FRAME *frame)
{
	switch (Ddm_parameter_data[index].type)
	{
	case DDMP_TYPE_UINT8:
		if (frame->uint8 == DDM_MISSING_UINT8)
			return 1;
		break;
	case DDMP_TYPE_UINT16:
		if (frame->uint16 == DDM_MISSING_UINT16)
			return 1;
		break;
	case DDMP_TYPE_UINT32:
		if (frame->uint32 == DDM_MISSING_UINT32)
			return 1;
		break;
	case DDMP_TYPE_INT8:
		if (frame->int8 == DDM_MISSING_INT8)
			return 1;
		break;
	case DDMP_TYPE_INT16:
		if (frame->int16 == DDM_MISSING_INT16)
			return 1;
		break;
	case DDMP_TYPE_INT32:
		if (frame->int32 == DDM_MISSING_INT32)
			return 1;
		break;
	default:
		return 0;
	}
	return 0;
}

int ddmp_isconnected(int intf)
{
	return ddmp_connection_status[intf].active_connection;
}

/*! \brief Pack a DDMP frame for transmission (internal)
	\param buffer Raw DDMP frame buffer
	\param frame DDMP frame to pack
	\return true: Success, size of resulting frame
	\return false: Faliure
 */
uint8_t ddmp_pack(DDMP_FRAME_BUFFER buffer, const DDMP_FRAME *frame)
{
	if ((!frame) || (frame->size>DDMP_FRAME_SIZE_MAX))
		return 0;

	switch (frame->size)
	{
	case 0:
	case 2:
	case 3:
	case 4:
		return 0;
	}

	memcpy(buffer, &frame->control, sizeof(frame->control));
	if (frame->size == DDMP_SIZE_CONTROL)
		return DDMP_SIZE_CONTROL;

	memcpy(buffer + DDMP_SIZE_CONTROL, &frame->parameter, sizeof(frame->parameter));
	if (frame->size == DDMP_SIZE_PARAMETER)
		return DDMP_SIZE_PARAMETER;

	memcpy(buffer + DDMP_SIZE_PARAMETER, &frame->value, frame->size - DDMP_SIZE_PARAMETER);

	return frame->size;
}

/*! \brief Unpack a raw DDMP frame (internal)
	\param frame DDMP frame to unpack to
	\param buffer Raw DDMP frame buffer to unpack
	\param size Size of raw DDMP frame
	\return Size of resulting frame
 */
uint8_t ddmp_unpack(DDMP_FRAME *frame, const DDMP_FRAME_BUFFER buffer, uint8_t size)
{
	if (size > DDMP_FRAME_SIZE_MAX)
		return 0;

	switch (size)
	{
	case 0:
	case 2:
	case 3:
	case 4:
		return 0;
	}

	memset(frame, 0, sizeof(*frame));

	memcpy(&frame->control, buffer, sizeof(frame->control));
	if (size == DDMP_SIZE_CONTROL)
		return frame->size = DDMP_SIZE_CONTROL;

	memcpy(&frame->parameter, buffer + DDMP_SIZE_CONTROL, sizeof(frame->parameter));
	if (size == DDMP_SIZE_PARAMETER)
		return frame->size = DDMP_SIZE_PARAMETER;

	memcpy(&frame->value, buffer + DDMP_SIZE_PARAMETER, size - DDMP_SIZE_PARAMETER);

	return frame->size = size;
}

/*! \brief Send ACK/NAK if one is buffered (internal)
	\param intf Interface
 */
static void _ddmp_send_ack(int intf)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];
	if (connection->ack_waiting)
	{
		_ddmp_transmit(intf, &connection->ack_buffer);

		connection->ack_waiting = 0;
	}
	
}

/*! \brief Process send queues and select a frame to be sent (internal)
	\param intf Interface
 */
static DDMP_SEND_ERROR_ENUM _process_queue(int intf)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];
	int send_result = DDMP_SEND_ERROR_OK;

	while (!connection->awaiting_ack)
	{
		if (!connection->connected)
		{
			return DDMP_SEND_ERROR_NO_CONNECTION;
		}

		_ddmp_send_ack(intf);

		if (!(connection->active_connection))	//send pings until connection is up
		{
			connection->awaiting_ack = 1;
			if (!ddmp_listen_mode)
				DDMP_TIMER(DDMP_TIMER_START);
			send_result = _ddmp_transmit(intf, &Ddmp_ping_frame);
		}
		else if (connection->outbuffer_waiting && connection->keep_frame)	//resend current frame
		{
			connection->keep_frame = 0;
			connection->awaiting_ack = 1;
			if (!ddmp_listen_mode)
				DDMP_TIMER(DDMP_TIMER_START);
			send_result = _ddmp_transmit(intf, &connection->outbuffer);
		}
		else if (!cqueue_empty(&connection->queue[DDMP_REPLY_QUEUE]))		//send from reply queue
		{
			cqueue_pop(intf, DDMP_REPLY_QUEUE, &connection->outbuffer);
			connection->outbuffer_waiting = 1;
			connection->keep_frame = 0;
#ifdef NO_WIRELESS_ACKS
			if (ddmp_interfaces[intf].type == DDMP_INTF_UART)	//do not ACK to wireless interfaces unless a PING
#endif
			if (!ddmp_listen_mode)
			{
				connection->awaiting_ack = 1;
				DDMP_TIMER(DDMP_TIMER_START);
			}
			send_result = _ddmp_transmit(intf, &connection->outbuffer);
		}
		else if (!cqueue_empty(&connection->queue[DDMP_SEND_QUEUE]))
		{
			cqueue_pop(intf, DDMP_SEND_QUEUE, &connection->outbuffer);
			connection->outbuffer_waiting = 1;
			connection->keep_frame = 0;
#ifdef NO_WIRELESS_ACKS
			if (ddmp_interfaces[intf].type == DDMP_INTF_UART)	//do not ACK to wireless interfaces unless a PING
#endif
			if (!ddmp_listen_mode)
			{
				connection->awaiting_ack = 1;
				DDMP_TIMER(DDMP_TIMER_START);
			}
			send_result = _ddmp_transmit(intf, &connection->outbuffer);
		}
		else
		{
			return DDMP_SEND_ERROR_OK;
		}
	}

	return send_result;
}

DDMP_SEND_ERROR_ENUM ddmp_connect(int intf)
{
	return _process_queue(intf);
}

/*! \brief Transmit a DDMP_PARAMETER over an interface (internal)
	\param intf Interface
	\param frame Frame to transmit over interface
	\return DDMP_SEND_ERROR_ENUM error code
 */
static DDMP_SEND_ERROR_ENUM _ddmp_transmit(int intf, const DDMP_FRAME *frame)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];
	int send_result;

	if (ddmp_listen_mode)
		return DDMP_SEND_ERROR_OK;

	if (!connection->connected)
	{
		return DDMP_SEND_ERROR_NO_CONNECTION;
	}

	if (!frame->size)
	{
		return DDMP_SEND_ERROR_OK;
	}

	DDMP_FRAME_BUFFER frame_buffer;
	size_t buffer_len = ddmp_pack(frame_buffer, frame);
	
	if (!buffer_len)
		return DDMP_SEND_ERROR_INTERFACE_ERROR;

	if (ddmp_interfaces[intf].type == DDMP_INTF_UART)
	{
		DDMP_ENCFRAME_BUFFER encframe_buffer;
		size_t stuff_len;
		stuff_len = _stuff(encframe_buffer, frame_buffer, buffer_len);
		send_result = ddmp_interfaces[intf].send_cb(intf, encframe_buffer, stuff_len);
		DDMP_ASSERT(send_result);
	}
	else
	{
		send_result = ddmp_interfaces[intf].send_cb(intf, frame_buffer, buffer_len);
		DDMP_ASSERT(send_result);
	}

	return DDMP_SEND_ERROR_OK;
}

/*! \brief Add frame to queue (internal)
	\param intf Interface
	\param q Queue to add frame to
	\param frame Frame to add to queue
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM _ddmp_enqueue(int intf, DDMP_QUEUE_ENUM q, const DDMP_FRAME *frame)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];
	DDMP_QUEUE_ENUM putqueue = q;
	DDMP_SEND_ERROR_ENUM transmit_result;

	switch (frame->control)
	{
	case DDMP_ACTION_ACK:
	case DDMP_ACTION_NAK:
		if (!connection->ack_waiting)
		{
			connection->ack_buffer = *frame;
			connection->ack_waiting = 1;
			_ddmp_send_ack(intf);
		}

		return DDMP_SEND_ERROR_OK;
		break;
	}

	if (cqueue_push(intf, putqueue, frame)==CQUEUE_ERROR_QUEUE_FULL)
		return DDMP_SEND_ERROR_QUEUE_FULL;

	if (connection->awaiting_ack)	//don't try to process queue if already waiting for an ACK
		return DDMP_SEND_ERROR_OK;

	transmit_result = _process_queue(intf);

	return transmit_result;
}

/*! \defgroup Interface DDMP interface functions
@{ */

/*! \brief Initialize DDMP library
	\param callbacks Pointer to DDMP_CALLBACKS structure filled with callback function pointers
	\param interfaces Pointer to array of interface definitions
	\param interface_count Number of interface definitions
	\param listen_mode Don't send anything, just monitor (bool)
	\return Zero if successful
 */
int ddmp_initialize(DDMP_CALLBACKS *callbacks, const DDMP_INTERFACE *interfaces, size_t interface_count, int listen_mode)
{
	if ((!callbacks->frame_cb) || (!callbacks->timer_cb) || (!callbacks->connect_cb) || (!interfaces) || (!callbacks->global_lock_cb) || (!callbacks->global_unlock_cb) || (!interface_count))
		return 1;

	if ((!callbacks->lock_cb) || (!callbacks->unlock_cb))
	{
		callbacks->lock_cb = callbacks->global_lock_cb;
		callbacks->unlock_cb = callbacks->global_unlock_cb;
	}

	callbacks->global_lock_cb(-1);

	ZERO(ddmp_connection_status);	//clear connection status

	for (size_t i = 0; i < interface_count; i++)	//check whether all interfaces have a send function
	{
		if (!interfaces[i].send_cb)
			if (!listen_mode)
				return 1;
		ddmp_connection_status[i].connected = interfaces[i].type == DDMP_INTF_UART;	//UART physical layer is considered always connected
	}

	ddmp_callbacks = callbacks;
	ddmp_interfaces = interfaces;
	ddmp_inteface_count = interface_count;
	ddmp_listen_mode = listen_mode;

	initialized = 1;

	callbacks->global_unlock_cb(-1);

	return 0;
}

/*! \brief Retransmit a previously transmitted frame

	Called when transmission times out.
	\param intf Interface
	\return Zero
 */
int ddmp_retransmit(int intf)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];

	if (connection->tries == DDMP_RETRY_COUNT - 1)
	{
		connection->active_connection = DDMP_DISCONNECTED;
		DDMP_ERROR(DDMP_ERROR_TIMEOUTS_EXCEEDED);
		connection->tries++;
	}
	else
	{
		DDMP_ERROR(DDMP_ERROR_TIMEOUT);
		if (connection->tries <= DDMP_RETRY_COUNT)
			connection->tries++;
	}

	if (!connection->connected)
	{
		//DDMP_ASSERT_TRIGGER(intf,"No connection!","");
		return 0;
	}

	DDMP_FRAME frame;

	if ((!connection->active_connection) || (!connection->outbuffer_waiting))
	{
		frame = Ddmp_ping_frame;
		connection->keep_frame = 1;
	}
	else
		frame = connection->outbuffer;

	_ddmp_transmit(intf, &frame);

	switch (frame.control)
	{
	case DDMP_ACTION_ACK:
	case DDMP_ACTION_NAK:
		return DDMP_SEND_ERROR_OK;
	default:
		connection->awaiting_ack = 1;
		if (!ddmp_listen_mode)
			DDMP_TIMER(DDMP_TIMER_START);
		break;
	}

	return 0;
}

/*! \brief Converts an error code to a string
	\param error Error code
	\return Error string
 */
const uint8_t *ddmp_error_string(DDMP_ERROR_ENUM error)
{
	if (error < DDMP_ERROR_COUNT)
		return Ddmp_error_strings[error];
	else
		return Ddmp_error_strings[DDMP_ERROR_COUNT];
}

/*! \brief Converts a send error code to a string
	\param error Error code
	\return Error string
 */
const uint8_t *ddmp_send_error_string(DDMP_SEND_ERROR_ENUM error)
{
	if (error < DDMP_SEND_ERROR_COUNT)
		return Ddmp_send_error_strings[error];
	else
		return Ddmp_send_error_strings[DDMP_SEND_ERROR_COUNT];
}

/*! \brief Converts a DDM type to a string
	\param type Type ID
	\return Type string
 */
const uint8_t *ddm_type_format_string(DDMP_TYPES_ENUM type)
{
	if (type < DDMP_TYPES_COUNT)
		return Ddm_type_format_strings[type];
	else
		return Ddm_type_format_strings[DDMP_TYPES_COUNT];
}

/*! \brief Converts a DDM unit to a string
	\param unit Unit ID
	\return Unit string
 */
const uint8_t *ddm_unit_string(DDMP_UNITS_ENUM unit)
{
	if (unit < DDMP_UNITS_COUNT)
		return Ddm_unit_strings[unit];
	else
		return Ddm_unit_strings[DDMP_UNITS_COUNT];
}

/*! \brief Converts a DDMP action to a string
	\param action Action ID
	\return Action string
 */
const uint8_t *ddmp_action_string(DDMP_ACTIONS_ENUM action)
{
	if (action < DDMP_ACTION_COUNT)
		return Ddmp_action_strings[action];
	else
		return Ddmp_action_strings[DDMP_ACTION_COUNT];
}

/*! \brief Converts a DDM parameter to a string
	\param parameter Parameter ID
	\return Parameter string
 */
const uint8_t *ddm_parameter_string(uint32_t parameter)
{
	for (int i = 0; i < DDM_PARAMETER_COUNT; i++)
	{
		if (parameter == Ddm_parameter_data[i].parameter)
		{
			return Ddm_parameter_data[i].parameter_string;
		}
	}
	return (uint8_t*)"Unknown parameter";
}

/*! \brief Fill a DDMP_PARAMETER structure with a DDMP frame
	\param dst Destination DDMP_PARAMETER
	\param action Action of this frame
	\param parameter Parameter ID (optional)
	\param value Pointer to value data (optional)
	\param value_size Size of value data (optional)
 */
void create_frame(DDMP_FRAME *dst, uint8_t action, uint32_t parameter, void *value, uint8_t value_size)
{
	dst->control = action;
	if ((action == DDMP_ACTION_PING) || (action == DDMP_ACTION_HELLO) || (action == DDMP_ACTION_ACK) || (action == DDMP_ACTION_NAK))
	{
		dst->size = DDMP_SIZE_CONTROL;
		return;
	}
	dst->parameter = parameter;
	if ((action == DDMP_ACTION_SUBSCRIBE))
	{
		dst->size = DDMP_SIZE_PARAMETER;
		return;
	}
	memcpy(&dst->value, value, MAX(value_size, 15));
	dst->size = DDMP_SIZE_PARAMETER + value_size;

	
	return;
}

/*! \brief Send multiple parameters over an interface as a reply to a subscription

	This function bypasses interface logical link state and transmits directly.
	This does not wait for achnowledgement for individual parameters.
	\param intf Interface
	\param frame DDMP_PARAMETER to transmit
	\param frame_count Number of frames to send
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_send_reply_multiple(int intf, DDMP_FRAME *frame, int frame_count)
{
	DDMP_SEND_ERROR_ENUM result = DDMP_SEND_ERROR_OK;
	for (; frame_count > 0; frame_count--, frame++)
	{
		result = ddmp_send_reply(intf, frame);
		if (result)
			return result;
	}

	return result;
}

/*! \brief Send a frame as a publish
	\param intf Interface
	\param frame Frame to add to reply queue
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_send_publish(int intf, const DDMP_FRAME *frame)
{
	DDMP_SEND_ERROR_ENUM result;
	DDMP_FRAME publish_frame = *frame;

	publish_frame.control = DDMP_ACTION_PUBLISH;
	result = ddmp_send(intf, &publish_frame);

	return result;
}

/*! \brief Send a frame as a publish as a reply
	\param intf Interface
	\param frame Frame to add to reply queue
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_publish_reply(int intf, const DDMP_FRAME *frame)
{
	DDMP_SEND_ERROR_ENUM result;
	DDMP_FRAME publish_frame = *frame;

	publish_frame.control = DDMP_ACTION_PUBLISH;
	result = _ddmp_enqueue(intf, DDMP_REPLY_QUEUE, &publish_frame);

	return result;
}

/*! \brief Send a frame as a subscribe as a reply
	\param intf Interface
	\param frame Frame to add to reply queue
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_subscribe_reply(int intf, const DDMP_FRAME *frame)
{
	DDMP_SEND_ERROR_ENUM result;
	DDMP_FRAME subscribe_frame = *frame;

	subscribe_frame.control = DDMP_ACTION_SUBSCRIBE;
	subscribe_frame.size = DDMP_SIZE_PARAMETER;
	result = _ddmp_enqueue(intf, DDMP_REPLY_QUEUE, &subscribe_frame);

	return result;
}

/*! \brief Send a frame as a reply
	\param intf Interface
	\param frame Frame to add to reply queue
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_send_reply(int intf, const DDMP_FRAME *frame)
{
	DDMP_SEND_ERROR_ENUM result;

	result = _ddmp_enqueue(intf, DDMP_REPLY_QUEUE, frame);
	if (result == DDMP_SEND_ERROR_QUEUE_FULL)
		result = _ddmp_enqueue(intf, DDMP_SEND_QUEUE, frame);

	return result;
}

/*! \brief Send a parameter over an interface
	\param intf Interface
	\param frame Frame to send
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_send(int intf, const DDMP_FRAME *frame)
{
	DDMP_SEND_ERROR_ENUM result;

	if (!initialized)
		return DDMP_SEND_ERROR_NOT_INITIALIZED;

	result = _ddmp_enqueue(intf, DDMP_SEND_QUEUE, frame);

	if (result == DDMP_SEND_ERROR_QUEUE_FULL)
	{
		result = _ddmp_enqueue(intf, DDMP_REPLY_QUEUE, frame);
	}

	return result;
}

/*! \brief Send one or more parameters over an interface

	CANNOT BE USED TO SEND REPLIES
	\param intf Interface
	\param frame Pointer to frame array
	\param frame_count Number of parameters to send
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_send_multiple(int intf, const DDMP_FRAME *frame, int frame_count)
{
	DDMP_SEND_ERROR_ENUM send_result = DDMP_SEND_ERROR_OK;

	for (int i = 0; i < frame_count; i++)
	{
		send_result = ddmp_send(intf, &frame[i]);
		if (send_result != DDMP_SEND_ERROR_OK)
			return send_result;
	}
	
	return send_result;
}

/*! \brief Send a subscribe frame
	\param intf Interface
	\param parameter Parameter ID to subscribe to
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_subscribe(int intf, uint32_t parameter)
{
	DDMP_SEND_ERROR_ENUM result;

	DDMP_FRAME subscribe_frame =
	{
			.control = DDMP_ACTION_SUBSCRIBE,
			.parameter = parameter,
			.size = DDMP_SIZE_PARAMETER,
	};

	result = ddmp_send(intf, &subscribe_frame);

	return result;
}

/*! \brief Send a publish frame
	\param intf Interface
	\param parameter Parameter ID to subscribe to
	\param value Value data
	\param value_size Size of value data
	\return DDMP_SEND_ERROR_ENUM error code
 */
DDMP_SEND_ERROR_ENUM ddmp_publish(int intf, uint32_t parameter, void *value, uint8_t value_size)
{
	DDMP_SEND_ERROR_ENUM result;

	DDMP_FRAME publish_frame =
	{
			.control = DDMP_ACTION_PUBLISH,
			.parameter = parameter,
			.size = DDMP_SIZE_PARAMETER + value_size,
	};

	memcpy(&publish_frame.value, value, value_size);

	result = ddmp_send(intf, &publish_frame);

	return result;
}

/*! \brief Make a string of parameter data
	\param outstring Buffer to built string
	\param value Pointer to parameter value data to present
	\param index Parameter index in DDM parameter list
 */
uint8_t *ddmp_present(uint8_t *outstring, void *value, int index)
{
	int multiplier = Ddm_parameter_data[index].multiplier;
	unsigned int intpart;
	unsigned int decpart;
	int32_t ivalue = 0;
	uint32_t uvalue = 0;
	uint8_t buffer[16];

	*outstring = '\0';

	switch (Ddm_parameter_data[index].type)
	{
	case DDMP_TYPE_INT8:
		ivalue = *(int8_t*)value;
		break;
	case DDMP_TYPE_INT16:
		ivalue = *(int16_t*)value;
		break;
	case DDMP_TYPE_INT32:
		ivalue = *(int32_t*)value;
		break;
	case DDMP_TYPE_UINT8:
		uvalue = *(uint8_t*)value;
		break;
	case DDMP_TYPE_UINT16:
		uvalue = *(uint16_t*)value;
		break;
	case DDMP_TYPE_UINT32:
		uvalue = *(uint32_t*)value;
		break;
	default:
		return (uint8_t*)" ";
		break;
	}

	switch (Ddm_parameter_data[index].type)
	{
	case DDMP_TYPE_INT8:
	case DDMP_TYPE_INT16:
	case DDMP_TYPE_INT32:
		if (ivalue < 0)
		{
			ivalue = -ivalue;
			strcat((char*)outstring, "-");
		}
		uvalue = (uint32_t)ivalue;
#if defined(__GNUC__) && __GNUC__ >= 7
		__attribute__ ((fallthrough));
#endif
        /* fallthrough */
	case DDMP_TYPE_UINT8:
	case DDMP_TYPE_UINT16:
	case DDMP_TYPE_UINT32:
		if (multiplier > 0)
		{
			intpart = uvalue / multiplier;
			decpart = uvalue % multiplier;
			sprintf((char*)buffer, "%u.%u ", intpart, decpart);
			strcat((char*)outstring, (char*)buffer);
		}
		else
		{
			intpart = uvalue * -multiplier;
			sprintf((char*)buffer, "%u ", intpart);
			strcat((char*)outstring, (char*)buffer);
		}
		break;
	default:
		break;
	}

	switch (Ddm_parameter_data[index].unit)
	{
	case DDMP_UNIT_VOLTS:
	case DDMP_UNIT_AMPERES:
	case DDMP_UNIT_CELSIUS:
		sprintf((char*)buffer, "%s", (char*)ddm_unit_string(Ddm_parameter_data[index].unit));
		strcat((char*)outstring, (char*)buffer);
		break;
	default:
		break;
	}

	return outstring;
}

/*! \brief Declare physical layer of interface disconnected, suspend stack
	\param intf Interface
 */
void ddmp_physical_disconnected(int intf)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];

	connection->connected = 0;
	connection->awaiting_ack = 0;

	if (initialized)
	{
		DDMP_TIMER(DDMP_TIMER_STOP);
		cqueue_init(intf, &connection->queue[DDMP_SEND_QUEUE]);
		cqueue_init(intf, &connection->queue[DDMP_REPLY_QUEUE]);
	}
	
}

/*! \brief Declare physical layer of interface connected, resume stack operation
	\param intf Interface
 */
void ddmp_physical_connected(int intf)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];
	
	connection->connected = 1;
}

/*! \brief Search for a parameter ID in DDM parameter list
	\param parameter Parameter ID to look up
	\return
	- Index of parameter
	- -1 if not found
 */
int get_parameter_index(uint32_t parameter)
{
	for (int i = 0; i < DDM_PARAMETER_COUNT; i++)
	{
		if (parameter == Ddm_parameter_data[i].parameter)
		{
			return i;
		}
	}
	return -1;
}

// @}

/*! \brief Inspect incoming frame and change state accordingly
	\param intf Interface
	\param frame Incoming frame
 */
void ddmp_incoming_frame(int intf, DDMP_FRAME *frame)
{
	DDMP_CONNECTION_STATUS *connection = &ddmp_connection_status[intf];
	int active = connection->active_connection;

	switch (frame->control)
	{
	case DDMP_ACTION_ACK:	//accept transmission of previous frame
		DDMP_TIMER(DDMP_TIMER_STOP);
		connection->tries = 0;
		if (connection->awaiting_ack)
		{
			connection->awaiting_ack = 0;
			connection->active_connection = 1;
			_process_queue(intf);
		}

		break;
	case DDMP_ACTION_NAK:	//retransmit previous frame
		if (connection->awaiting_ack)
		{
			connection->awaiting_ack = 0;
			ddmp_retransmit(intf);
		}
		break;
	case DDMP_ACTION_PING:	//acknowledge, regardless of interface type
		_ddmp_enqueue(intf, DDMP_REPLY_QUEUE, &Ddmp_ack_frame);
		break;
	case DDMP_ACTION_NOP:	//ignore, do not acknowledge
		break;
	default:
#ifdef NO_WIRELESS_ACKS
		if (ddmp_interfaces[intf].type == DDMP_INTF_UART)	//do not ACK to wireless interfaces unless a PING
#endif
			_ddmp_enqueue(intf, DDMP_REPLY_QUEUE, &Ddmp_ack_frame);
		break;
	}

	DDMP_CALLBACK(frame);	//forward frame to application

	if ((!active) && (connection->active_connection)) //did we get an active connection?
		DDMP_CONNECT;
}
