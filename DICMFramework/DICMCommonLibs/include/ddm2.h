/*! \file ddm2.h
    \brief Main DDMP2 header

    Link layer enums, structs, macros and functions
*/

#ifndef DDM2_H_
#define DDM2_H_

#ifdef __IAR_SYSTEMS_ICC__
#include "stm8s.h"
#else
#include <stdint.h>
#endif

#include <stddef.h>

#define DDMP2LIB_VERSION "4.6"  //!< \~ DDMP2 library version (X.Y)

#define DDMP2_LISTEN_PORT 13143  //!< DDMP2 application server listen port
#define IS_SMALL_ENDIAN   (*(const uint8_t *)&Endian)
#define IS_BIG_ENDIAN     (*(((const uint8_t *)&Endian) + 1))
#define SWAP2(x)          ((((uint16_t)(x) >> 8) & 0xff) | (((uint16_t)(x) & 0xff) << 8))
#define SWAP4(x)          ((((uint32_t)(x) & 0xff000000) >> 24) | (((uint32_t)(x) & 0x00ff0000) >> 8) | (((uint32_t)(x) & 0x0000ff00) << 8) | (((uint32_t)(x) & 0x000000ff) << 24))

#ifndef ZERO
#define ZERO(x) memset((x), 0, sizeof(x))  //!< \~ Zerofill variable
#endif

#ifndef ELEMENTS
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))  //!< \~ Element count of array
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))  //!< \~ Minimum value macro
#endif

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))  //!< \~ Maximum value macro
#endif

#define CHARACTERS(x)                     ((sizeof(x) / sizeof(x[0])) - 1)  //!< \~ Character count of string literal
#define SS(s)                             s, CHARACTERS(s)                  //!< \~ String literal with size
#define ABS(n)                            (((n) < 0) ? (-(n)) : (n))        //!< \~ Absolute value macro
#define DDM2_PARAMETER_PROPERTY_FIELD(x)  ((x) & 0x000000ff)
#define DDM2_PARAMETER_INSTANCE_FIELD(x)  (((x) >> 8) & 0xff)
#define DDM2_PARAMETER_CLASS_FIELD(x)     (((x) >> 16) & 0xffff)
#define DDM2_PARAMETER_CLASS(x)           ((x) & 0xffff0000)
#define DDM2_PARAMETER_GROUP(x)           ((x) & 0xff000000)
#define DDM2_PARAMETER_CLASS_INSTANCE(x)  ((x) & 0xffffff00)
#define DDM2_PARAMETER_INSTANCE_PART(x)   ((x) & 0x0000ff00)
#define DDM2_PARAMETER_BASE_INSTANCE(x)   ((x) & 0xffff00ff)
#define DDM2_PARAMETER_INSTANCE(x)        ((uint32_t)(((uint8_t)(x)) << 8))
#define DDM2_INSTANCEOF_PARAMETER(p,i)    (DDM2_PARAMETER_BASE_INSTANCE(p) | DDM2_PARAMETER_INSTANCE(i))
#define DDM2_INSTANCEOF_CLASS(c,i)        (DDM2_PARAMETER_CLASS(c) | DDM2_PARAMETER_INSTANCE(i))
#define DDM2_CLASS_NUMBER(x)              ((uint32_t)(((uint8_t)(x)) << 16))
#define DDM2_IS_DDM2_CLASS(x)             (!((x) & 0x0000ffff))
#define DDM2_IS_GATEWAY_CLASS(x)          (!((x) & 0xff000000))
#define DDM2_IS_AVAIL_PROPERTY(x)         (!((x) & 0x000000ff))
#define DDMP2_CONTROL_SIZE                (sizeof(uint8_t))
#define DDMP2_OFFSET_SIZE                 (sizeof(((DDMP2_RAW_FRAME *)0)->fragment.offset))
#define DDMP2_PARAMETER_SIZE              (sizeof(uint32_t))
#define DDMP2_MAX_ATT_MTU_SIZE            158                                                  //!< \~ Old iOS limit
#define DDMP2_JUMBO_FRAME_MAX_SIZE        4000                                                 //!< \~ NVS strings are limited to 4000 B
#define DDMP2_MAX_FRAME_SIZE              (DDMP2_MAX_ATT_MTU_SIZE - 3)                         //!< \~ ATT frame header
#define DDMP2_MAX_VALUE_SIZE              (DDMP2_MAX_FRAME_SIZE - 5)                           //!< \~ Control byte + Parameter
#define DDMP2_MAX_FRAGMENT_VALUE_SIZE     (DDMP2_MAX_FRAME_SIZE - 3)                           //!< \~ Control byte + Offset
#define DDMP2_MAX_JUMBO_VALUE_SIZE        (DDMP2_JUMBO_FRAME_MAX_SIZE - DDMP2_PARAMETER_SIZE)  //!< \~ Parameter
#define DDMP2_INVENTORY_AVL(x)            ((x) & 0x000000ff)
#define DDMP2_INVENTORY_CLASS_INSTANCE(x) ((x) & 0xffffff00)
#define DDMP2_INVENTORY_INSTANCE(x)       ((x) & 0x0000ff00)
#define DDMP2_INVENTORY_CLASS(x)          ((x) & 0xffff0000)
#define DDMP2_INVALID_CLASS               0xffffffff
#define DDMP2_INVALID_INSTANCE            0xffffffff
#define DDMP2_INVALID_PARAMETER           0xffffffff

#ifndef _MSC_VER
#define PACKED __attribute__((packed))
#else  //_MSC_VER
#define PACKED
#endif  //_MSC_VER

//! \~ DDMP2 Message enum
//! \sa DDMP2_CONTROL_ENUM
typedef enum DDMP2_MESSAGE_ENUM
{
    DDMP2_MESSAGE_RESET = 0x00,        //!< \~ Reset and restart connection
    DDMP2_MESSAGE_ERROR = 0x01,        //!< \~ An error has occured
    DDMP2_MESSAGE_PING = 0x02,         //!< \~ Request for connection verification
    DDMP2_MESSAGE_PINGREPLY = 0x03,    //!< \~ Connection verification acknowledgement
    DDMP2_MESSAGE_OK = 0x04,           //!< \~ Generic success status indication
    DDMP2_MESSAGE_AGGREGATION = 0x05,  //!< \~ Enable frame aggregation
} DDMP2_MESSAGE_ENUM;

//! \~ DDMP2 Multibroker enum
//! \sa DDMP2_CONTROL_ENUM
typedef enum DDMP2_MULTIBROKER_ENUM
{
    DDMP2_MULTIBROKER_CONNECT = 0x00,  //!< \~ Connect as a multibroker client
    DDMP2_MULTIBROKER_ACK = 0x01,      //!< \~ Replied to a multibroker client connect
} DDMP2_MULTIBROKER_ENUM;

//! \~ DDMP2 control bytes
//! \sa DDMP2_FRAME
typedef enum DDMP2_CONTROL_ENUM
{
    DDMP2_CONTROL_PUBLISH = 0x10,      //!< \~ Publish a new value from a bus
    DDMP2_CONTROL_SET = 0x11,          //!< \~ Value change request frame
    DDMP2_CONTROL_SUBSCRIBE = 0x12,    //!< \~ Subscribe action
    DDMP2_CONTROL_NOP = 0x13,          //!< \~ NOP, do nothing
    DDMP2_CONTROL_FRAGMENT = 0x14,     //!< \~ Fragmented jumbo frame transfer
    DDMP2_CONTROL_MESSAGE = 0x15,      //!< \~ Control message
    DDMP2_CONTROL_REG = 0x16,          //!< \~ Register device instance
    DDMP2_CONTROL_MULTIBROKER = 0x17,  //!< \~ Connect as multibroker client
    // UNUSED					0x18,	//!< \~ Unused
    DDMP2_CONTROL_GENERIC = 0x19,      //!< \~ Generic events frame
    DDMP2_CONTROL_UNSUBSCRIBE = 0x1a,  //!< \~ Unsubscribe action
    DDMP2_CONTROL_COUNT,               //!< \~ Number of action slots
} DDMP2_CONTROL_ENUM;

#ifdef _MSC_VER
#pragma pack(push)
#pragma pack(1)
#endif  //_MSC_VER

//! \~ DDMP2 frame value storage
typedef union DDMP2_PAYLOAD
{
    uint32_t uint32;                    //!< \~ Access value as an uint32_t
    int32_t int32;                      //!< \~ Access value as an int32_t
    uint8_t raw[DDMP2_MAX_VALUE_SIZE];  //!< \~ Raw value data access
} PACKED DDMP2_PAYLOAD;

//! \~ DDMP2 publish frame
typedef struct DDMP2_PUBLISH_FRAME
{
    uint32_t parameter;   //!< \~ Unique parameter ID
    DDMP2_PAYLOAD value;  //!< \~ Value
} PACKED DDMP2_PUBLISH_FRAME;

typedef DDMP2_PUBLISH_FRAME DDMP2_SET_FRAME;

//! \~ DDMP2 subscribe frame
typedef struct DDMP2_SUBSCRIBE_FRAME
{
    uint32_t parameter;  //!< \~ Unique parameter ID
} PACKED DDMP2_SUBSCRIBE_FRAME;

//! \~ DDMP2 fragment frame
typedef struct DDMP2_FRAGMENT_FRAME
{
    uint16_t offset;                               //!< \~ Unique parameter ID
    uint8_t value[DDMP2_MAX_FRAGMENT_VALUE_SIZE];  //!< \~ Value
} PACKED DDMP2_FRAGMENT_FRAME;

//! \~ DDMP2 jumbo frame; can only be sent through fragment frames
typedef struct DDMP2_JUMBO_FRAME
{
    uint32_t parameter;
    uint8_t value[DDMP2_MAX_JUMBO_VALUE_SIZE];
} PACKED DDMP2_JUMBO_FRAME;

//! \~ DDMP2 publish frame
typedef struct DDMP2_MESSAGE_FRAME
{
    uint8_t id;  //!< \~ Message type
} PACKED DDMP2_MESSAGE_FRAME;

//! \~ DDMP2 register frame
typedef struct DDMP2_REG_FRAME
{
    uint32_t device_class;  //!< \~ REQUEST: networked_thing | device_class | XX | XX	REPLY: networked_thing | device_class | instance | 00
} PACKED DDMP2_REG_FRAME;

//! \~ DDMP2 multibroker frame
typedef struct DDMP2_MULTIBROKER_FRAME
{
    uint8_t control;  //!< \~ Control byte \sa DDMP2_MULTIBROKER_ENUM
    uint64_t data;    //!< \~ Device ID
} PACKED DDMP2_MULTIBROKER_FRAME;

/*! \~ DDMP2 Generic connector's internal event
    DDMP2 Generic frame supports sending any kind of event. DDMP2 frames is one type of event,
    that is supported by all the modules. Generic frames are events that could have different
    sources(e.g. timer) which should be defined and process by the modules themselves.

    Please refer to the "connector_system.c" implementation, as an example of how the generic
    frames can be used for sharing events between/within a module.
*/
typedef struct DDMP2_GENERIC_FRAME
{
    uint32_t id;                         //!< \~ Event id
    uint8_t data[DDMP2_MAX_VALUE_SIZE];  //!< \~ Payload
} PACKED DDMP2_GENERIC_FRAME;

typedef DDMP2_SUBSCRIBE_FRAME DDMP2_UNSUBSCRIBE_FRAME;

//-----------------------------------------------

//! \~ DDMP2 frame data
typedef struct DDMP2_RAW_FRAME
{
    uint8_t control;  //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    union
    {
        DDMP2_PUBLISH_FRAME publish;
        DDMP2_SUBSCRIBE_FRAME subscribe;
        DDMP2_SET_FRAME set;
        DDMP2_FRAGMENT_FRAME fragment;
        DDMP2_MESSAGE_FRAME message;
        DDMP2_REG_FRAME reg;
        DDMP2_MULTIBROKER_FRAME multibroker;
        DDMP2_GENERIC_FRAME generic;
        DDMP2_UNSUBSCRIBE_FRAME unsubscribe;
    };
} PACKED DDMP2_RAW_FRAME;

//! \~ DDMP2 frame with metadata
typedef struct DDMP2_FRAME
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    DDMP2_RAW_FRAME frame;
} PACKED DDMP2_FRAME;

#define DDMP2_METADATA_SIZE (sizeof(DDMP2_FRAME) - sizeof(DDMP2_RAW_FRAME))

typedef struct DDMP2_PUBLISH_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_PUBLISH_FRAME publish_frame;
} PACKED DDMP2_PUBLISH_FRAME_BUFFER;

typedef struct DDMP2_SET_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_SET_FRAME set_frame;
} PACKED DDMP2_SET_FRAME_BUFFER;

typedef struct DDMP2_SUBSCRIBE_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_SUBSCRIBE_FRAME subscribe_frame;
} PACKED DDMP2_SUBSCRIBE_FRAME_BUFFER;

typedef struct DDMP2_CONTROL_NOP_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
} PACKED DDMP2_CONTROL_NOP_FRAME_BUFFER;

typedef struct DDMP2_FRAGMENT_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_FRAGMENT_FRAME fragment_frame;
} PACKED DDMP2_FRAGMENT_FRAME_BUFFER;

typedef struct DDMP2_MESSAGE_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_MESSAGE_FRAME message_frame;
} PACKED DDMP2_MESSAGE_FRAME_BUFFER;

typedef struct DDMP2_REG_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_REG_FRAME reg_frame;
} PACKED DDMP2_REG_FRAME_BUFFER;

typedef struct DDMP2_MULTIBROKER_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_MULTIBROKER_FRAME multibroker_frame;
} PACKED DDMP2_MULTIBROKER_FRAME_BUFFER;

typedef struct DDMP2_GENERIC_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_GENERIC_FRAME generic_frame;
} PACKED DDMP2_GENERIC_FRAME_BUFFER;

typedef struct DDMP2_UNSUBSCRIBE_FRAME_BUFFER
{
    uint8_t frame_size;             //!< \~ Size of raw frame
    uint8_t source_connector;       //!< \~ Source connector
    uint8_t destination_connector;  //!< \~ Destination connector
    uint8_t control;                //!< \~ Control byte \sa DDMP2_CONTROL_ENUM
    DDMP2_UNSUBSCRIBE_FRAME unsubscribe_frame;
} PACKED DDMP2_UNSUBSCRIBE_FRAME_BUFFER;

#define DECLARE_DDMP2_PUBLISH_FRAME(name, parameter, value, value_size, connector)              \
    DDMP2_PUBLISH_FRAME_BUFFER name =                                                           \
        {                                                                                       \
            .frame_size = sizeof(DDMP2_PUBLISH_FRAME) - sizeof(DDMP2_PAYLOAD) + value_size + 1, \
            .source_connector = connector,                                                      \
            .destination_connector = connector,                                                 \
            .control = DDMP2_CONTROL_PUBLISH,                                                   \
            .publish_frame.parameter = parameter,                                               \
    };                                                                                          \
    memmove(&name.publish_frame.value, value, value_size);

#define DECLARE_DDMP2_SET_FRAME(name, parameter, value, value_size, connector)              \
    DDMP2_SET_FRAME_BUFFER name =                                                           \
        {                                                                                   \
            .frame_size = sizeof(DDMP2_SET_FRAME) - sizeof(DDMP2_PAYLOAD) + value_size + 1, \
            .source_connector = connector,                                                  \
            .destination_connector = connector,                                             \
            .control = DDMP2_CONTROL_SET,                                                   \
            .set_frame.parameter = parameter,                                               \
    };                                                                                      \
    memmove(&name.set_frame.value, value, value_size);

#define DECLARE_DDMP2_SUBSCRIBE_FRAME(name, parameter, connector) \
    DDMP2_SUBSCRIBE_FRAME_BUFFER name =                           \
        {                                                         \
            .frame_size = sizeof(DDMP2_SUBSCRIBE_FRAME) + 1,      \
            .source_connector = connector,                        \
            .destination_connector = connector,                   \
            .control = DDMP2_CONTROL_SUBSCRIBE,                   \
            .subscribe_frame.parameter = parameter,               \
    };

#define DECLARE_DDMP2_CONTROL_NOP_FRAME(name, connector) \
    DDMP2_CONTROL_NOP_FRAME_BUFFER name =                \
        {                                                \
            .frame_size = 1,                             \
            .source_connector = connector,               \
            .destination_connector = connector,          \
            .control = DDMP2_CONTROL_NOP,                \
    }

#define DECLARE_DDMP2_FRAGMENT_FRAME(name, offset, value, value_size, connector)                         \
    DDMP2_FRAGMENT_FRAME_BUFFER name =                                                                   \
        {                                                                                                \
            .frame_size = sizeof(DDMP2_FRAGMENT_FRAME) - DDMP2_MAX_FRAGMENT_VALUE_SIZE + value_size + 1, \
            .source_connector = connector,                                                               \
            .destination_connector = connector,                                                          \
            .control = DDMP2_CONTROL_FRAGMENT,                                                           \
            .fragment_frame.offset = offset,                                                             \
    };                                                                                                   \
    memmove(&name.fragment_frame.value, value, value_size);

#define DECLARE_DDMP2_MESSAGE_FRAME(name, message_id, connector) \
    DDMP2_MESSAGE_FRAME_BUFFER name =                            \
        {                                                        \
            .frame_size = sizeof(DDMP2_MESSAGE_FRAME) + 1,       \
            .source_connector = connector,                       \
            .destination_connector = connector,                  \
            .control = DDMP2_CONTROL_MESSAGE,                    \
            .message_frame.id = message_id,                      \
    }

#define DECLARE_DDMP2_REG_FRAME(name, device_class, connector) \
    DDMP2_REG_FRAME_BUFFER name =                              \
        {                                                      \
            .frame_size = sizeof(DDMP2_REG_FRAME) + 1,         \
            .source_connector = connector,                     \
            .destination_connector = connector,                \
            .control = DDMP2_CONTROL_REG,                      \
            .reg_frame.device_class = device_class,            \
    }

#define DECLARE_DDMP2_MULTIBROKER_FRAME(name, control, data, connector) \
    DDMP2_MULTIBROKER_FRAME_BUFFER name =                               \
        {                                                               \
            .frame_size = sizeof(DDMP2_MULTIBROKER_FRAME) + 1,          \
            .source_connector = connector,                              \
            .destination_connector = connector,                         \
            .control = DDMP2_CONTROL_MULTIBROKER,                       \
            .multibroker_frame.control = control,                       \
            .multibroker_frame.data = data,                             \
    }

#define DECLARE_DDMP2_GENERIC_FRAME(name, event_id, data, data_size, connector)               \
    DDMP2_GENERIC_FRAME_BUFFER name =                                                         \
        {                                                                                     \
            .frame_size = sizeof(DDMP2_GENERIC_FRAME) - DDMP2_MAX_VALUE_SIZE + data_size + 1, \
            .source_connector = connector,                                                    \
            .destination_connector = connector,                                               \
            .control = DDMP2_CONTROL_GENERIC,                                                 \
            .generic_frame.id = event_id,                                                     \
    };                                                                                        \
    memmove(&name.generic_frame.data, data, data_size);

#define DECLARE_DDMP2_UNSUBSCRIBE_FRAME(name, parameter, connector) \
    DDMP2_UNSUBSCRIBE_FRAME_BUFFER name =                           \
        {                                                           \
            .frame_size = sizeof(DDMP2_UNSUBSCRIBE_FRAME) + 1,      \
            .source_connector = connector,                          \
            .destination_connector = connector,                     \
            .control = DDMP2_CONTROL_UNSUBSCRIBE,                   \
            .unsubscribe_frame.parameter = parameter,               \
    }

#ifdef _MSC_VER
#pragma pack(pop)
#endif  //_MSC_VER

extern const uint16_t Endian;

const uint8_t *ddmp2_control_string(const DDMP2_CONTROL_ENUM control);
const uint8_t *ddmp2_message_string(const uint8_t id);
int ddmp2_can_have_value(const DDMP2_FRAME *const pframe);
size_t ddmp2_value_size(const DDMP2_FRAME *const pframe);
size_t ddmp2_raw_frame_size(const DDMP2_CONTROL_ENUM control, const uint8_t value_size);
size_t ddmp2_full_frame_size(const DDMP2_CONTROL_ENUM control, const uint8_t value_size);
int ddmp2_create_publish(DDMP2_FRAME *const pframe, const uint32_t parameter, const void *const value, const uint8_t value_size, const uint8_t connector);
int ddmp2_create_set(DDMP2_FRAME *const pframe, const uint32_t parameter, const void *const value, const uint8_t value_size, const uint8_t connector);
int ddmp2_create_subscribe(DDMP2_FRAME *const pframe, const uint32_t parameter, const uint8_t connector);
int ddmp2_create_nop(DDMP2_FRAME *const pframe, const uint8_t connector);
int ddmp2_create_raw_frame(DDMP2_FRAME *const pframe, const void *const data, const size_t data_size, const uint8_t connector);
int ddmp2_create_fragment(DDMP2_FRAME *const pframe, const int16_t offset, const void *const value, const uint8_t value_size, const uint8_t connector);
int ddmp2_create_message(DDMP2_FRAME *const pframe, const uint8_t id, const uint8_t connector);
int ddmp2_create_reg(DDMP2_FRAME *const pframe, const uint32_t device_class, const uint8_t connector);
int ddmp2_create_multibroker(DDMP2_FRAME *const pframe, uint8_t control, const uint64_t data, const uint8_t connector);
int ddmp2_create_generic(DDMP2_FRAME *const pframe, const uint32_t event_id, const void *const data, const uint8_t data_size, const uint8_t connector);
int ddmp2_create_unsubscribe(DDMP2_FRAME *const pframe, const uint32_t parameter, const uint8_t connector);
int ddmp2_extract_string_from_frame(const DDMP2_FRAME *const pframe, char *const text, const size_t capacity);

#endif /* DDM2_H_ */
