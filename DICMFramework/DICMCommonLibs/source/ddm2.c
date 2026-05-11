/*! \file ddm2.c
    \brief Main DDMP2 source

    Link layer functions

    2020-10-23 - Moved to shared library directory (JB)
    2021-12-20 - Added forward to broker/connector functions (JB)
    2022-08-07 - Stripped "DDMP2 " from control strings (JB)
    2024-05-29 - Added value size bound checking
 */

#include "ddm2.h"
#include <stddef.h>
#include <string.h>

#ifdef __IAR_SYSTEMS_ICC__
#include "stm8s.h"
#else
#include <stdint.h>
#endif  //__IAR_SYSTEMS_ICC__

const uint16_t Endian = 1;

//! \~ Strings of DDMP control bytes
//! \sa DDMP_CONTROL_ENUM
static const uint8_t *Ddmp2_control_strings[] =
    {
        (uint8_t *)"Publish",
        (uint8_t *)"Set",
        (uint8_t *)"Subscribe",
        (uint8_t *)"NOP",
        (uint8_t *)"Fragment",
        (uint8_t *)"Message",
        (uint8_t *)"Register",
        (uint8_t *)"Multibroker",
        (uint8_t *)"UNUSED (0x18)",
        (uint8_t *)"Generic",
        (uint8_t *)"Unsubscribe",
};

static const uint8_t *Ddmp2_message_strings[] =
    {
        (uint8_t *)"RESET",
        (uint8_t *)"ERROR",
        (uint8_t *)"PING",
        (uint8_t *)"PING REPLY",
        (uint8_t *)"OK",
        (uint8_t *)"AGGREGATION",
};

/*! \brief Converts a DDMP2 control byte to a string
    \param control Control byte
    \return String of control byte
 */
const uint8_t *ddmp2_control_string(const DDMP2_CONTROL_ENUM control)
{
    const int String_index = (int)control - DDMP2_CONTROL_PUBLISH;  // Assert signedness of control byte

    if ((String_index < 0) || (String_index > (int)(ELEMENTS(Ddmp2_control_strings) - 1)))
    {
        return (uint8_t *)"???";
    }
    else
    {
        return Ddmp2_control_strings[control - DDMP2_CONTROL_PUBLISH];
    }
}

/*! \brief Converts a DDMP2 message ID to a string
    \param id Message ID
    \return String of message ID
 */
const uint8_t *ddmp2_message_string(const uint8_t id)
{
    const int String_index = (int)id - DDMP2_MESSAGE_RESET;  // Assert signedness of id

    if ((String_index < 0) || (String_index > (int)(ELEMENTS(Ddmp2_message_strings) - 1)))
    {
        return (uint8_t *)"???";
    }
    else
    {
        return Ddmp2_message_strings[id - DDMP2_MESSAGE_RESET];
    }
}

/*! \brief Determines whether the DDMP2 frame is of a type that can contain a value
    \param pframe Pointer to DDMP2 frame
    \return TRUE if can contain a value
 */
int ddmp2_can_have_value(const DDMP2_FRAME *const pframe)
{
    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_PUBLISH:
    case DDMP2_CONTROL_SET:
    case DDMP2_CONTROL_FRAGMENT:
    case DDMP2_CONTROL_GENERIC:
        return 1;
    default:
        return 0;
    }
}

/*! \brief Determines the amount of payload data contained in a DDMP2 frame
    \param pframe Pointer to DDMP2 frame
    \return Size of value in frame
 */
size_t ddmp2_value_size(const DDMP2_FRAME *const pframe)
{
    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_PUBLISH:
        return pframe->frame_size - DDMP2_CONTROL_SIZE - (sizeof(((DDMP2_FRAME *)0)->frame.publish) - sizeof(((DDMP2_FRAME *)0)->frame.publish.value));
    case DDMP2_CONTROL_SET:
        return pframe->frame_size - DDMP2_CONTROL_SIZE - (sizeof(((DDMP2_FRAME *)0)->frame.set) - sizeof(((DDMP2_FRAME *)0)->frame.set.value));
    case DDMP2_CONTROL_FRAGMENT:
        return pframe->frame_size - DDMP2_CONTROL_SIZE - (sizeof(((DDMP2_FRAME *)0)->frame.fragment) - sizeof(((DDMP2_FRAME *)0)->frame.fragment.value));
    case DDMP2_CONTROL_GENERIC:
        return pframe->frame_size - DDMP2_CONTROL_SIZE - (sizeof(((DDMP2_FRAME *)0)->frame.generic) - sizeof(((DDMP2_FRAME *)0)->frame.generic.data));
    default:
        return 0;
    }
}

/*! \brief Determines the amount memory a RAW DDMP2 frame would require
    \param control Control byte
    \param value_size Amount of payload in frame
    \return Size of raw frame
 */
size_t ddmp2_raw_frame_size(const DDMP2_CONTROL_ENUM control, const uint8_t value_size)
{
    switch (control)
    {
    case DDMP2_CONTROL_PUBLISH:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.publish) - sizeof(((DDMP2_FRAME *)0)->frame.publish.value) + value_size;
    case DDMP2_CONTROL_SET:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.set) - sizeof(((DDMP2_FRAME *)0)->frame.set.value) + value_size;
    case DDMP2_CONTROL_SUBSCRIBE:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.subscribe);
    case DDMP2_CONTROL_NOP:
        return DDMP2_CONTROL_SIZE;
    case DDMP2_CONTROL_FRAGMENT:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.fragment) - sizeof(((DDMP2_FRAME *)0)->frame.fragment.value) + value_size;
    case DDMP2_CONTROL_MESSAGE:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.message);
    case DDMP2_CONTROL_REG:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.reg);
    case DDMP2_CONTROL_MULTIBROKER:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.multibroker);
    case DDMP2_CONTROL_GENERIC:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.generic) - sizeof(((DDMP2_FRAME *)0)->frame.generic.data) + value_size;
    case DDMP2_CONTROL_UNSUBSCRIBE:
        return DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.unsubscribe);
    default:
        return 0;
    }
}

/*! \brief Determines the amount memory a DDMP2 frame would require
    \param control Control byte
    \param value_size Amount of payload in frame
    \return Size of frame
 */
size_t ddmp2_full_frame_size(const DDMP2_CONTROL_ENUM control, const uint8_t value_size)
{
    switch (control)
    {
    case DDMP2_CONTROL_PUBLISH:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.publish) - sizeof(((DDMP2_FRAME *)0)->frame.publish.value) + value_size;
    case DDMP2_CONTROL_SET:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.set) - sizeof(((DDMP2_FRAME *)0)->frame.set.value) + value_size;
    case DDMP2_CONTROL_SUBSCRIBE:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.subscribe);
    case DDMP2_CONTROL_NOP:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE;
    case DDMP2_CONTROL_FRAGMENT:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.fragment) - sizeof(((DDMP2_FRAME *)0)->frame.fragment.value) + value_size;
    case DDMP2_CONTROL_MESSAGE:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.message);
    case DDMP2_CONTROL_REG:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.reg);
    case DDMP2_CONTROL_MULTIBROKER:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.multibroker);
    case DDMP2_CONTROL_GENERIC:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.generic) - sizeof(((DDMP2_FRAME *)0)->frame.generic.data) + value_size;
    case DDMP2_CONTROL_UNSUBSCRIBE:
        return DDMP2_METADATA_SIZE + DDMP2_CONTROL_SIZE + sizeof(((DDMP2_FRAME *)0)->frame.unsubscribe);
    default:
        return 0;
    }
}

/*! \brief Creates a publish DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param parameter DDMP2 parameter ID
    \param value Payload to add to frame
    \param value_size Size of payload
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_publish(DDMP2_FRAME *const pframe, const uint32_t parameter, const void *const value, const uint8_t value_size, const uint8_t connector)
{
    const uint8_t *value_data = value;

    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    if (value_size && !value)  // if value size is not zero, we need the associated data
    {
        return 0;
    }

    if (value_size > sizeof(pframe->frame.publish.value))  // payload does not fit
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_PUBLISH, value_size);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_PUBLISH;
    pframe->frame.publish.parameter = parameter;
    memmove(&pframe->frame.publish.value, value_data, value_size);

    return 1;
}

/*! \brief Creates a set DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param parameter DDMP2 parameter ID
    \param value Payload to add to frame
    \param value_size Size of payload
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_set(DDMP2_FRAME *const pframe, const uint32_t parameter, const void *const value, const uint8_t value_size, const uint8_t connector)
{
    if (!ddmp2_create_publish(pframe, parameter, value, value_size, connector))
    {
        return 0;
    }

    pframe->frame.control = DDMP2_CONTROL_SET;  // change control byte to SET

    return 1;
}

/*! \brief Creates a subscribe DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param parameter DDM2 parameter ID
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_subscribe(DDMP2_FRAME *const pframe, const uint32_t parameter, const uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_SUBSCRIBE, 0);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_SUBSCRIBE;
    pframe->frame.subscribe.parameter = parameter;

    return 1;
}

/*! \brief Creates a NOP DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_nop(DDMP2_FRAME *const pframe, uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_NOP, 0);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_NOP;

    return 1;
}

/*! \brief Creates a fragment DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param offset Offset of fragment in bytes
    \param value Payload to add to frame
    \param value_size Size of payload
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_fragment(DDMP2_FRAME *const pframe, const int16_t offset, const void *const value, const uint8_t value_size, const uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    if (value_size && !value)  // if value size is not zero, we need the associated data
    {
        return 0;
    }

    if ((offset < -1) || ((offset + value_size) > 4096))  // offset range needs to be in [-1..4095]
    {
        return 0;
    }

    if ((offset == -1) && value_size)  // if offset is -1 (finish), we can't have a payload
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_FRAGMENT, value_size);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_FRAGMENT;
    pframe->frame.fragment.offset = offset;
    memmove(&pframe->frame.fragment.value, value, value_size);

    return 1;
}

/*! \brief Creates a message DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param id DDMP2 message id
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_message(DDMP2_FRAME *const pframe, const uint8_t id, const uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_MESSAGE, 0);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_MESSAGE;
    pframe->frame.message.id = id;

    return 1;
}

/*! \brief Creates a reg DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param device_class DDMP2 parameter class to request instance from
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_reg(DDMP2_FRAME *const pframe, const uint32_t device_class, const uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_REG, 0);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_REG;
    pframe->frame.reg.device_class = device_class;

    return 1;
}

/*! \brief Creates a multibroker DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param control Multibroker control byte
    \param data Multibroker ID data
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_multibroker(DDMP2_FRAME *const pframe, uint8_t control, const uint64_t data, const uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_MULTIBROKER, 0);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_MULTIBROKER;
    pframe->frame.multibroker.control = control;
    pframe->frame.multibroker.data = data;

    return 1;
}

/*! \brief Creates a generic DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param event_id Event id defined by each connector
    \param data Payload to add to frame
    \param data_size Size of payload
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_generic(DDMP2_FRAME *const pframe, const uint32_t event_id, const void *const data, const uint8_t data_size, const uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    if ((data_size != 0) && (data == NULL))  // if data size is not zero, we need the associated data
    {
        return 0;
    }

    if (data_size > sizeof(pframe->frame.generic.data))  // payload does not fit
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_GENERIC, data_size);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_GENERIC;
    pframe->frame.generic.id = event_id;
    memmove(&pframe->frame.generic.data, data, data_size);

    return 1;
}

/*! \brief Creates an unsubscribe DDMP2 frame from parameters
    \param pframe Destination DDMP2 frame
    \param parameter DDM2 parameter ID
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_unsubscribe(DDMP2_FRAME *const pframe, const uint32_t parameter, const uint8_t connector)
{
    if (pframe == NULL)  // destination frame can't be null
    {
        return 0;
    }

    pframe->frame_size = (uint8_t)ddmp2_raw_frame_size(DDMP2_CONTROL_UNSUBSCRIBE, 0);
    pframe->source_connector = pframe->destination_connector = connector;
    pframe->frame.control = DDMP2_CONTROL_UNSUBSCRIBE;
    pframe->frame.subscribe.parameter = parameter;

    return 1;
}

/**
 * @brief 		Extract string from a DDMP2 frame and copy it to @a text buffer
 *
 * @param 		pframe Pointer to DDMP2 frame which contains a string.
 * @param 		text Pointer to a buffer where to extract and copy the string
 * @param 		capacity Capacity of used buffer
 * @return 		Operation status
 * @retval		1 - Operation completed successfully.
 * @note		The string in text buffer might be truncated when @a capacity is smaller than the
 * 				length of string in @a pframe.
 * @pre			The pointer @a pframe must point to a frame which is of type @ref DDM2_TYPE_INT32_T.
 * @post		The string in @a text buffer will always be a NULL terminated string.
 */

int ddmp2_extract_string_from_frame(const DDMP2_FRAME *const pframe, char *const text, const size_t capacity)
{
    size_t length = MIN(ddmp2_value_size(pframe), capacity - 1u);
    memmove(text, pframe->frame.set.value.raw, length);
    memset(&text[length], 0, capacity - length);

    return 1;
}

//-------------------------------------------------------------------

/*! \brief Creates a DDMP2 frame from raw data
    \param pframe Destination DDMP2 frame
    \param data Raw data input
    \param data_size Size of raw data
    \param connector Connector ID to tag frame with
    \return TRUE if successful
 */
int ddmp2_create_raw_frame(DDMP2_FRAME *pframe, const void *const data, const size_t data_size, const uint8_t connector)
{
    if ((data == NULL) || (data_size == 0))  // pframe may be null for a dry run
    {
        return 0;
    }

    const uint8_t *const Frame_data = data;

    if ((Frame_data[0] < DDMP2_CONTROL_PUBLISH) || (Frame_data[0] >= DDMP2_CONTROL_COUNT))  // control byte out of range
    {
        return 0;
    }

    size_t min_size = -1;
    size_t max_size = 0;

    switch (Frame_data[0])  // determine valid frame size
    {
    case DDMP2_CONTROL_PUBLISH:
        min_size = DDMP2_CONTROL_SIZE + DDMP2_PARAMETER_SIZE;
        max_size = DDMP2_CONTROL_SIZE + sizeof(pframe->frame.publish);
        break;
    case DDMP2_CONTROL_SET:
        min_size = DDMP2_CONTROL_SIZE + DDMP2_PARAMETER_SIZE;
        max_size = DDMP2_CONTROL_SIZE + sizeof(pframe->frame.set);
        break;
    case DDMP2_CONTROL_SUBSCRIBE:
        min_size = DDMP2_CONTROL_SIZE + DDMP2_PARAMETER_SIZE;
        max_size = min_size;
        break;
    case DDMP2_CONTROL_NOP:
        max_size = min_size = DDMP2_CONTROL_SIZE;
        break;
    case DDMP2_CONTROL_FRAGMENT:
        min_size = DDMP2_CONTROL_SIZE + DDMP2_OFFSET_SIZE;
        max_size = DDMP2_MAX_FRAME_SIZE;
        break;
    case DDMP2_CONTROL_MESSAGE:
        max_size = min_size = DDMP2_CONTROL_SIZE + sizeof(uint8_t);
        break;
    case DDMP2_CONTROL_REG:
        max_size = min_size = DDMP2_CONTROL_SIZE + DDMP2_PARAMETER_SIZE;
        break;
    case DDMP2_CONTROL_MULTIBROKER:
        max_size = min_size = DDMP2_CONTROL_SIZE + sizeof(uint8_t) + sizeof(uint64_t);
        break;
    case DDMP2_CONTROL_GENERIC:
        min_size = DDMP2_CONTROL_SIZE + sizeof(pframe->frame.generic.id);
        max_size = DDMP2_MAX_FRAME_SIZE;
        break;
    case DDMP2_CONTROL_UNSUBSCRIBE:
        max_size = min_size = DDMP2_CONTROL_SIZE + DDMP2_PARAMETER_SIZE;
        break;
    }

    if ((data_size > max_size) || (data_size < min_size))
    {
        return 0;
    }

    if (pframe)  // check approved, copy over frame data
    {
        memmove(&pframe->frame, data, data_size);
        pframe->frame_size = (uint8_t)data_size;
        pframe->source_connector = pframe->destination_connector = connector;
    }

    return 1;
}
