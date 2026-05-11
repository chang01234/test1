/*! \file ddm.h
	\brief Main DDMP header.

	Link layer enums, structs, macros and functions.
*/

#ifndef DDM_H_
#define DDM_H_

#include <stdint.h>
#include <string.h>

#define DDMPLIB_VERSION			"2.5"						//!< \~ DDMP library version

#ifndef ZERO
#define ZERO(x)					memset(x, 0, sizeof(x))		//!< \~ Zerofill variable
#endif

#ifndef ELEMENTS
#define ELEMENTS(x)				(sizeof(x) / sizeof(x[0]))	//!< \~ Element count of array
#endif

#ifndef MIN
#define MIN(a,b)				((a) < (b) ? (a) : (b))		//!< \~ Minimum value macro
#endif

#ifndef MAX
#define MAX(a,b)				((a) < (b) ? (b) : (a))		//!< \~ Maximum value macro
#endif

#ifdef DDMP_BROKER
#define DDMP_INTERFACE_COUNT		7						//!< \~ Maximum value of interfaces supported, 7 to support Rubicon broker (2xUART, 4xWiFi, 1xBLE)
#define DDMP_UART_INTERFACE_COUNT	2						//!< \~ Amount of UARTS in system (Amount of UART related buffers)
#else
#define DDMP_INTERFACE_COUNT		1						//!< \~ Maximum value of interfaces supported, 1 to support Rubicon end device (1xUART)
#define DDMP_UART_INTERFACE_COUNT	1						//!< \~ Amount of UARTS in system (Amount of UART related buffers)
#endif

#define CQ_QUEUESIZE			64							//!< \~ Item count to fit in each queue (2^x)

#define DDMP_TIMEOUT			2000						//!< \~ Time in milliseconds from a transmission until a frame is considered lost
#define DDMP_RETRY_COUNT		5							//!< \~ Number of times to try to transmit a frame before giving up

#define DDM_MISSING_UINT8		(uint8_t)0xff				//!< \~ Value denoting a missing value for an 8-bit unsigned value (255)
#define DDM_MISSING_UINT16		(uint16_t)0xffff			//!< \~ Value denoting a missing value for an 16-bit unsigned value (65535)
#define DDM_MISSING_UINT32		(uint32_t)0xffffffff		//!< \~ Value denoting a missing value for an 32-bit unsigned value (4294967295)
#define DDM_MISSING_INT8		(int8_t)0x80				//!< \~ Value denoting a missing value for an 8-bit signed value (-128)
#define DDM_MISSING_INT16		(int16_t)0x8000				//!< \~ Value denoting a missing value for an 16-bit signed value (-32768)
#define DDM_MISSING_INT32		(int32_t)0x80000000			//!< \~ Value denoting a missing value for an 32-bit signed value (-2147483648)
#define DDM_MISSING_VOID		0							//!< \~ Value for a non-value (void) parameter

#define MISSING_uint8			DDM_MISSING_UINT8
#define MISSING_uint16			DDM_MISSING_UINT16
#define MISSING_uint32			DDM_MISSING_UINT32
#define MISSING_int8			DDM_MISSING_INT8
#define MISSING_int16			DDM_MISSING_INT16
#define MISSING_int32			DDM_MISSING_INT32
#define MISSING_void			DDM_MISSING_VOID

#define DDMP_SIZE_CONTROL		1							//!< \~ Total size of a frame only consisting of a control byte
#define DDMP_SIZE_PARAMETER		(1+4)						//!< \~ Total size of a frame consisting of a control byte and a parameter ID
#define DDMP_SIZE_UINT8			(1+DDMP_SIZE_PARAMETER)		//!< \~ Total size of a frame containing an uint8_t value
#define DDMP_SIZE_INT8			(1+DDMP_SIZE_PARAMETER)		//!< \~ Total size of a frame containing an int8_t value
#define DDMP_SIZE_UINT16		(2+DDMP_SIZE_PARAMETER)		//!< \~ Total size of a frame containing an uint16_t value
#define DDMP_SIZE_INT16			(2+DDMP_SIZE_PARAMETER)		//!< \~ Total size of a frame containing an int16_t value
#define DDMP_SIZE_UINT32		(4+DDMP_SIZE_PARAMETER)		//!< \~ Total size of a frame containing an uint32_t value
#define DDMP_SIZE_INT32			(4+DDMP_SIZE_PARAMETER)		//!< \~ Total size of a frame containing an int32_t value
#define DDMP_FRAME_SIZE_MAX		20							//!< \~ Maximum size of a DDMP frame

#define DDMP_INTERFACE_NONE		(-1)						//!< \~ Frame did not originate from an interface, forward to all

#define CQ_EMPTY(cq)	(cq->read_index==cq->write_index)							//!< \~ Is queue empty?
#define CQ_FULL(cq)		(cq->read_index==((cq->write_index+1)&(CQ_QUEUESIZE-1)))	//!< \~ Is queue full?

/*! \brief Macro to simplify definition of a published parameter DDMP frame

	\param p Parameter name
	\param t Type ID
	\param v Value
*/
#define PUB(p,t,v)	\
{\
.size = p ## _SIZE ,\
.parameter = p,\
.t = v,\
.control = DDMP_ACTION_PUBLISH,\
.publish = 1,\
}

/*! \brief Macro to simplify definition of a published array parameter DDMP frame

	\param p Parameter name
	\param s Size
	\param ... Value
*/
#define PUB_ARRAY(p,s,...)	\
{\
.size = s + DDMP_SIZE_PARAMETER ,\
.parameter = p,\
.value = __VA_ARGS__,\
.control = DDMP_ACTION_PUBLISH,\
.publish = 1,\
}

/*! \brief Macro to simplify definition of a subscribed array parameter DDMP frame

	\param p Parameter name
	\param s Size
	\param ... Value
*/
#define SUB_ARRAY(p,s,...)	\
{\
.size = s + DDMP_SIZE_PARAMETER ,\
.parameter = p,\
.value = __VA_ARGS__,\
.control = DDMP_ACTION_PUBLISH,\
.subscribe = 1,\
}

/*! \brief Macro to simplify definition of a published and subscribed array parameter DDMP frame

	\param p Parameter name
	\param s Size
	\param ... Value
*/
#define PUBSUB_ARRAY(p,s,...)	\
{\
.size = s + DDMP_SIZE_PARAMETER ,\
.parameter = p,\
.value = __VA_ARGS__,\
.control = DDMP_ACTION_PUBLISH,\
.publish = 1,\
.subscribe = 1,\
}

/*! \brief Macro to simplify definition of a subscribed parameter DDMP frame

	\param p Parameter name
	\param t Type ID
*/
#define SUB(p,t)	\
{\
.size = p ## _SIZE ,\
.parameter = p,\
.t = MISSING_ ## t,\
.control = DDMP_ACTION_SUBSCRIBE,\
.subscribe = 1,\
}

/*! \brief Macro to simplify definition of a subscribed parameter DDMP frame with initial value

	\param p Parameter name
	\param t Type ID
	\param i Initial value
*/
#define SUB_INIT(p,t,i)	\
{\
.size = p ## _SIZE ,\
.parameter = p,\
.t = i,\
.control = DDMP_ACTION_SUBSCRIBE,\
.subscribe = 1,\
}

/*! \brief Macro to simplify definition of a DDMP frame that is both published and subscribed

	\param p Parameter name
	\param t Type ID
	\param v Value
*/
#define PUBSUB(p,t,v)	\
{\
.size = p ## _SIZE ,\
.parameter = p,\
.t = v,\
.control = DDMP_ACTION_PUBLISH,\
.publish = 1,\
.subscribe = 1,\
}

/*! \brief Macro to simplify definition of a parameter string DDMP frame that is published and subscribed

	\param p Parameter name
	\param v Value
*/
#define STRING(p,v)	\
{\
.size = 15 + DDMP_SIZE_PARAMETER,\
.parameter = p,\
.value = v,\
.control = DDMP_ACTION_PUBLISH,\
.publish = 1,\
.subscribe = 1,\
}

/*! \brief Macro to simplify definition of a static parameter string DDMP frame that is only published

	\param p Parameter name
	\param v Value
*/
#define STATIC_STRING(p,v)	\
{\
.size = sizeof(v) + DDMP_SIZE_PARAMETER,\
.parameter = p,\
.value = v,\
.control = DDMP_ACTION_PUBLISH,\
.publish = 1,\
}

/*! \brief Macro to simplify definition of a parameter string DDMP frame that only subscribed (write only)

	\param p Parameter name
	\param v Value
	\param s Size
*/
#define SUB_STRING(p,v,s)	\
{\
.size = s + DDMP_SIZE_PARAMETER,\
.parameter = p,\
.value = v,\
.control = DDMP_ACTION_SUBSCRIBE,\
.subscribe = 1,\
}

/*! \brief Macro to simplify definition of a parameter without a value (event)

	\param p Parameter name
*/
#define SUB_VOID(p)	\
{\
.size = DDMP_SIZE_PARAMETER,\
.parameter = p,\
.control = DDMP_ACTION_SUBSCRIBE,\
.subscribe = 1,\
}

//! \~ DDMP queue errors
//! \sa DDMP_ERROR
typedef enum _CQUEUE_ERROR_ENUM
{
	CQUEUE_ERROR_ARGUMENT = -3,								//!< \~ Invalid argument, probably NULL
	CQUEUE_ERROR_QUEUE_EMPTY,								//!< \~ Frame queue is empty, can't pop
	CQUEUE_ERROR_QUEUE_FULL,								//!< \~ Frame queue is full, can't push
} CQUEUE_ERROR_ENUM;

//! \~ DDMP connection state
typedef enum _DDMP_CONNECTION_STATE
{
	DDMP_DISCONNECTED,										//!< \~ Logical link disconnected
	DDMP_CONNECTED,											//!< \~ Logical link connected
} DDMP_CONNECTION_STATE;

//! \~ DDMP stack errors
//! \sa DDMP_ERROR
typedef enum _DDMP_ERROR_ENUM
{
	DDMP_ERROR_S_WITHOUT_E,									//!< \~ Sync byte encountered, expected end of previous frame
	DDMP_ERROR_E_WITHOUT_S,									//!< \~ End byte encountered, but no frame started
	DDMP_ERROR_BUFFER_OVERFLOW,								//!< \~ UART buffer size exceeded
	DDMP_ERROR_CRC,											//!< \~ CRC calculation does not agree with supplied signature
	DDMP_ERROR_SHORT_FRAME,									//!< \~ Enc Frame too short to be valid (<4 bytes)
	DDMP_ERROR_BUFFER_TOO_SMALL,							//!< \~ Frame too big for UART buffer
	DDMP_ERROR_TIMEOUT,										//!< \~ Timeout exceeded, frame considered lost, retrying
	DDMP_ERROR_TIMEOUTS_EXCEEDED,							//!< \~ The maximum amount of tries exceeded, backing off
	DDMP_ERROR_FRAME_SIZE,									//!< \~ DDMP_FRAME has an invalid size
	DDMP_ERROR_ARGUMENT,									//!< \~ Illegal argument to function
	DDMP_ERROR_COUNT,										//!< \~ Number of errors
} DDMP_ERROR_ENUM;

//! \~ ddmp_send() function return status
typedef enum _DDMP_SEND_ERROR_ENUM
{
	DDMP_SEND_ERROR_OK,										//!< \~ Frame queued sucessfully
	DDMP_SEND_ERROR_NO_CONNECTION,							//!< \~ Underlying physical layer is disconnected
	DDMP_SEND_ERROR_QUEUE_FULL,								//!< \~ Send queue is full
	DDMP_SEND_ERROR_QUEUE_EMPTY,							//!< \~ Tried to send from empty queue
	DDMP_SEND_ERROR_INTERFACE_ERROR,						//!< \~ Interface failed to send frame
	DDMP_SEND_ERROR_NOT_INITIALIZED,						//!< \~ DDMP library is not initialized
	DDMP_SEND_ERROR_COUNT,									//!< \~ Number of send errors
} DDMP_SEND_ERROR_ENUM;

//! \~ Timer callback actions
//! \sa DDMP_TIMER
//! \sa DDMP_TIMER_CB
typedef enum _DDMP_TIMER_ACTION_ENUM
{
	DDMP_TIMER_START,										//!< \~ Start timeout timer
	DDMP_TIMER_STOP,										//!< \~ Deactivate timeout timer
	DDMP_TIMER_ACTION_COUNT,
} DDMP_TIMER_ACTION_ENUM;

//! \~ DDM parameter units
//! \sa Ddm_parameter_data
//! \sa Ddm_unit_strings
typedef enum _DDMP_UNITS_ENUM
{
	DDMP_UNIT_NONE,											//!< \~ No unit
	DDMP_UNIT_FLAG,											//!< \~ On or off
	DDMP_UNIT_COUNT,										//!< \~ Number of items
	DDMP_UNIT_ENUM,											//!< \~ One out of a set
	DDMP_UNIT_TUPLE,										//!< \~ Compound data
	DDMP_UNIT_CELSIUS,										//!< \~ Temperature
	DDMP_UNIT_VOLTS,										//!< \~ Voltage
	DDMP_UNIT_AMPERES,										//!< \~ Current
	DDMP_UNIT_CODE,											//!< \~ Access code
	DDMP_UNITS_COUNT,										//!< \~ Number of units
} DDMP_UNITS_ENUM;

//! \~ DDM parameter data types
//! \sa Ddm_parameter_data
//! \sa Ddm_type_format_strings
typedef enum _DDMP_TYPES_ENUM
{
	DDMP_TYPE_VOID,											//!< \~ No data
	DDMP_TYPE_UINT8,										//!< \~ 8-bit unsigned integer
	DDMP_TYPE_UINT16,										//!< \~ 16-bit unsigned integer
	DDMP_TYPE_UINT32,										//!< \~ 32-bit unsigned integer
	DDMP_TYPE_INT8,											//!< \~ 8-bit signed integer
	DDMP_TYPE_INT16,										//!< \~ 16-bit signed integer
	DDMP_TYPE_INT32,										//!< \~ 32-bit signed integer
	DDMP_TYPE_ARRAY,										//!< \~ List of same type elements
	DDMP_TYPE_STRING,										//!< \~ Text character string (UTF-8)
	DDMP_TYPES_COUNT,										//!< \~ Number of types
} DDMP_TYPES_ENUM;

//! \~ DDMP actions
//! \sa DDMP_PARAMETER
typedef enum _DDMP_ACTIONS_ENUM
{
	DDMP_ACTION_PUBLISH = 0x00,								//!< \~ Publish action
	DDMP_ACTION_SUBSCRIBE = 0x01,							//!< \~ Subscribe action
	DDMP_ACTION_PING = 0x02,								//!< \~ Connect action
	DDMP_ACTION_HELLO = 0x03,								//!< \~ Connection acknowledgement
	DDMP_ACTION_ACK = 0x04,									//!< \~ Acknowledgement to action
	DDMP_ACTION_NAK = 0x05,									//!< \~ Negative acknowledgement to action
	DDMP_ACTION_NOP = 0x06,									//!< \~ NO-OP, is to be ignored by receiver and not acked
	DDMP_ACTION_COUNT,										//!< \~ Number of actions
} DDMP_ACTIONS_ENUM;

//! \~ DDMP supported interface types
//! \sa DDMP_INTERFACE
//! \sa ddmp_initialize()
typedef enum _DDMP_INTF_TYPE_ENUM
{
	DDMP_INTF_UART,											//!< \~ RS232 Frame Serial link, COM port
	DDMP_INTF_BLE,											//!< \~ BLE GATT characteristic
	DDMP_INTF_WIFI,											//!< \~ JSON over TCP over WiFi
} DDMP_INTF_TYPE_ENUM;

//! \~ Active send queue
//! \sa ddmp_send()
//! \sa ddmp_send_reply()
//! \sa ddmp_send_multiple()
//! \sa ddmp_send_reply_multiple()
typedef enum _DDMP_QUEUE_ENUM
{
	DDMP_SEND_QUEUE,										//!< \~ Main send queue
	DDMP_REPLY_QUEUE,										//!< \~ Reply queue
	DDMP_ACK_QUEUE,											//!< \~ ACK/NAK queue
} DDMP_QUEUE_ENUM;

//! \~ DDM parameter metadata for application layer
//! \sa Ddm_parameter_data
typedef struct _DDM_PARAMETER_DATA
{
	uint32_t parameter;										//!< \~ Parameter ID
	uint8_t *parameter_string;								//!< \~ Parameter name
	DDMP_TYPES_ENUM type;									//!< \~ Parameter value type
	DDMP_UNITS_ENUM unit;									//!< \~ Parameter value unit
	int multiplier;											//!< \~ Parameter value scale factor (positive: *, negative: /)
	int16_t range_min;										//!< \~ Min value of parameter
	int16_t range_max;										//!< \~ Max value of parameter
	int8_t size;											//!< \~ Size of value data
} DDM_PARAMETER_DATA;

//! \~ DDMP frame data/metadata struct
typedef struct _DDMP_FRAME
{
	uint8_t		size;										//!< \~ Size of DDMP frame
	uint8_t		control;									//!< \~ Control byte
	uint32_t	parameter;									//!< \~ Parameter ID
	union
	{
		uint8_t		uint8;									//!< \~ Access value as an uint8_t
		uint16_t	uint16;									//!< \~ Access value as an uint16_t
		uint32_t	uint32;									//!< \~ Access value as an uint32_t
		int8_t		int8;									//!< \~ Access value as an int8_t
		int16_t		int16;									//!< \~ Access value as an int16_t
		int32_t		int32;									//!< \~ Access value as an int32_t
		uint8_t		value[15];								//!< \~ Raw value data access
	};														//!< \~ Value
	uint8_t publish;										//!< \~ This parameter is published
	uint8_t subscribe;										//!< \~ This parameter is subscribed
} DDMP_FRAME;

//! \~ Locked circular queue struct
typedef struct _CQUEUE
{
	uint_fast8_t read_index;			//!< \~ Read index; consumer
	uint_fast8_t write_index;			//!< \~ Write index; producer
	DDMP_FRAME frames[CQ_QUEUESIZE];	//!< \~ Queue frame data
} CQUEUE;

//! \~ DDMP connection status data
typedef struct _DDMP_CONNECTION_STATUS
{
	volatile int active_connection;							//!< \~ Logical connection is up
	volatile int awaiting_ack;								//!< \~ Sent a frame, waiting for ack before sending the next one
	volatile int ack_waiting;								//!< \~ There is an acknowledgement waiting to be sent
	volatile int connected;									//!< \~ Physical layer is disconnected, don't process interface
	volatile int keep_frame;								//!< \~ Keep frame in outbuffer and try to send it again
	volatile int outbuffer_waiting;							//!< \~ There is a frame in the outbuffer waiting to be sent
	DDMP_FRAME outbuffer;									//!< \~ Frame that is in the process of sending
	DDMP_FRAME ack_buffer;									//!< \~ Outgoing buffer for ACKs
	CQUEUE queue[2];										//!< \~ Outgoing frame queues \sa DDMP_QUEUE_ENUM
	int tries;												//!< \~ Numer of times tried to send frame
} DDMP_CONNECTION_STATUS;

/*! \defgroup Callbacks DDMP callbacks
@{ */

/*! \brief Called when a frame is received

	Handle incoming traffic here; filter parameters you are interested in.
	\param intf Interface
	\param parameter Received frame
*/
typedef void(*DDMP_FRAME_CB)(int intf, DDMP_FRAME *frame);

/*! \brief Called whenever an error occurs (optional)
	\param intf Interface
	\param error Error code
	\param error_string Error description
*/
typedef void(*DDMP_ERROR_CB)(int intf, DDMP_ERROR_ENUM error, const uint8_t* error_string);

/*! \brief Called to start/stop transaction timeout timer

	Write timer handling code here. If timer reaches DDMP_TIMEOUT ms, ddmp_retransmit should be called.
	\param intf Interface
	\param timer_action Action to perform on timer
*/
typedef void(*DDMP_TIMER_CB)(int intf, DDMP_TIMER_ACTION_ENUM timer_action);

/*! \brief Called when the library wants to send data out an interface \sa DDMP_INTERFACE

	Write platform and interface specific data transfer code here.
	\param intf Interface
	\param data Pointer to data to send
	\param size Size of data
	\return true: transmission successful
*/
typedef int(*DDMP_SEND_CB)(int intf, void* data, size_t size);

/*! \brief Called when connection is (re)established

	Use this function to (re)send subscriptions
	\param intf Interface
	\param size Size of data
*/
typedef void(*DDMP_CONNECT_CB)(int intf);

/*! \brief Called when the stack needs to lock, either interface locally or globally

	Use this function to start a critical section that cannot be interrupted.
	\param intf Interface
*/
typedef void(*DDMP_LOCK_CB)(int intf);

/*! \brief Called when the stack needs to unlock prevous lock, either interface locally or globally

	Use this function to leave a critical section.
	\param intf Interface
*/
typedef void(*DDMP_UNLOCK_CB)(int intf);

/*! \brief Called whenever an error occurs (optional)
	\param intf Interface
	\param error Error code
	\param error_string Error description
*/

typedef void(*DDMP_ASSERT_CB)(int line, const char *func, char* assertion, int value);
//! @}

/*! \defgroup Macros to invoke DDMP callbacks
@{ */
#define DDMP_CALLBACK(p) ddmp_callbacks->frame_cb(intf,p)		//!< \~ Invoke frame callback with the received frame \sa _ddmp_incoming_frame()
#define DDMP_ERROR(err) if (ddmp_callbacks->error_cb) ddmp_callbacks->error_cb(intf,err,ddmp_error_string(err))	//!< \~ Invoke error callback with the error and a description of the error if error callback is defined
#define DDMP_TIMER(action) ddmp_callbacks->timer_cb(intf,action)	//!< \~ Invoke timer callback with supplied action \sa _ddmp_transmit() \sa _ddmp_incoming_frame()
#define DDMP_CONNECT ddmp_callbacks->connect_cb(intf);			//!< \~ Invoke connect callback
#define DDMP_LOCK ddmp_callbacks->lock_cb(intf);				//!< \~ Enter critical section (local)
#define DDMP_UNLOCK ddmp_callbacks->unlock_cb(intf);			//!< \~ Leave critical section (local)
#define DDMP_GLOBAL_LOCK ddmp_callbacks->global_lock_cb(-1);	//!< \~ Enter critical section (global)
#define DDMP_GLOBAL_UNLOCK ddmp_callbacks->global_unlock_cb(-1);	//!< \~ Leave critical section (global)
#define DDMP_ASSERT_TRIGGER(l,f,a,v) if (ddmp_callbacks->assert_cb) ddmp_callbacks->assert_cb(l,f,a,v);	//!< \~ Trigger assertion
//! @}

#define DDMP_ASSERT(x)\
{\
    int errchk_result=(int)(x);\
	if (!errchk_result) DDMP_ASSERT_TRIGGER(__LINE__, __func__, #x, errchk_result);\
}\

//! \~ DDMP interface definition for ddmp_initialize()
//! \sa ddmp_initialize()
typedef struct _DDMP_INTERFACE
{
	DDMP_INTF_TYPE_ENUM type;								//!< \~ Interface type
	DDMP_SEND_CB send_cb;									//!< \~ Callback function that can send data out of this interface
	uint64_t handle;										//!< \~ Handle to interface/subinterface (optional)
} DDMP_INTERFACE;

//! \~ DDMP global callback functions for ddmp_initialize()
typedef struct _DDMP_CALLBACKS
{
	DDMP_FRAME_CB frame_cb;
	DDMP_ERROR_CB error_cb;
	DDMP_TIMER_CB timer_cb;
	DDMP_CONNECT_CB connect_cb;
	DDMP_LOCK_CB global_lock_cb;
	DDMP_UNLOCK_CB global_unlock_cb;
	DDMP_LOCK_CB lock_cb;
	DDMP_UNLOCK_CB unlock_cb;
	DDMP_ASSERT_CB assert_cb;
} DDMP_CALLBACKS;

//! \~ Buffer guaranteed to being able to hold a complete encapsulation frame (sync byte, 2xframe data, CRC, end byte)
typedef uint8_t DDMP_ENCFRAME_BUFFER[1 + 40 + 1 + 1];

//! \~ Buffer guaranteed to being able to hold a complete raw DDMP frame (20 bytes)
typedef uint8_t DDMP_FRAME_BUFFER[20];

int ddm_compare_values(const DDMP_FRAME *master, const DDMP_FRAME *candidate);
void ddm_update_and_publish(int intf, DDMP_FRAME *dest, DDMP_FRAME *src);
int ddmp_isconnected(int intf);
DDMP_SEND_ERROR_ENUM ddmp_connect(int intf);
int ddm_missing_value(int index, const DDMP_FRAME *frame);
int ddm_write_parameter(DDMP_FRAME *dest, const DDMP_FRAME *src);
DDMP_FRAME ddm_read_parameter(const DDMP_FRAME *src);
int ddmp_initialize(DDMP_CALLBACKS *callbacks, const DDMP_INTERFACE *interfaces, size_t interface_count, int listen_mode);
int ddmp_retransmit(int intf);
const uint8_t *ddmp_error_string(DDMP_ERROR_ENUM error);
const uint8_t *ddmp_send_error_string(DDMP_SEND_ERROR_ENUM error);
const uint8_t *ddm_type_format_string(DDMP_TYPES_ENUM type);
const uint8_t *ddm_unit_string(DDMP_UNITS_ENUM unit);
const uint8_t *ddm_parameter_string(uint32_t parameter);
const uint8_t *ddmp_action_string(DDMP_ACTIONS_ENUM action);
uint8_t ddmp_pack(DDMP_FRAME_BUFFER buffer, const DDMP_FRAME *frame);
uint8_t ddmp_unpack(DDMP_FRAME *frame, const DDMP_FRAME_BUFFER buffer, uint8_t size);
void create_frame(DDMP_FRAME *dst, uint8_t action, uint32_t parameter, void *value, uint8_t value_size);
DDMP_SEND_ERROR_ENUM ddmp_send(int intf, const DDMP_FRAME *frame);
DDMP_SEND_ERROR_ENUM ddmp_send_publish(int intf, const DDMP_FRAME *frame);
DDMP_SEND_ERROR_ENUM ddmp_publish_reply(int intf, const DDMP_FRAME *frame);
DDMP_SEND_ERROR_ENUM ddmp_subscribe_reply(int intf, const DDMP_FRAME *frame);
DDMP_SEND_ERROR_ENUM ddmp_send_reply(int intf, const DDMP_FRAME *frame);
DDMP_SEND_ERROR_ENUM ddmp_send_multiple(int intf, const DDMP_FRAME *frame, int parameter_count);
DDMP_SEND_ERROR_ENUM ddmp_send_reply_multiple(int intf, DDMP_FRAME *frame, int parameter_count);
DDMP_SEND_ERROR_ENUM ddmp_subscribe(int intf, uint32_t parameter);
DDMP_SEND_ERROR_ENUM ddmp_publish(int intf, uint32_t parameter, void *value, uint8_t value_size);
uint8_t *ddmp_present(uint8_t *outstring, void *value, int index);
int get_parameter_index(uint32_t parameter);
void ddmp_physical_disconnected(int intf);
void ddmp_physical_connected(int intf);
void ddmp_incoming_frame(int intf, DDMP_FRAME *frame);

void cqueue_init(int intf, CQUEUE *cq);
int cqueue_write(CQUEUE *cq);
int cqueue_push(int intf, DDMP_QUEUE_ENUM putqueue, const DDMP_FRAME* data);
int cqueue_read(CQUEUE *cq);
int cqueue_pop(int intf, DDMP_QUEUE_ENUM putqueue, DDMP_FRAME* data);
int cqueue_full(CQUEUE *cq);
int cqueue_empty(CQUEUE *cq);
void cqueue_print(CQUEUE *cq);
int cqueue_available(int intf, DDMP_QUEUE_ENUM putqueue);

extern const DDMP_FRAME Ddmp_nak_frame;
extern const DDMP_FRAME Ddmp_ack_frame;
extern const DDMP_FRAME Ddmp_ping_frame;
extern const DDMP_FRAME Ddmp_hello_frame;
extern const DDMP_FRAME Ddmp_nop_frame;
extern const DDMP_CALLBACKS *ddmp_callbacks;

extern DDMP_CONNECTION_STATUS ddmp_connection_status[DDMP_INTERFACE_COUNT];

#endif /* DDM_H_ */
