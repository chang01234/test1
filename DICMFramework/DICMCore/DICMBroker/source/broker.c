/*! \file broker.c
    \brief Broker source

    Broker and DDMP handling.
 */
#include "broker.h"
#include "configuration.h"
#include "gateway.h"
#include "production.h"

#include "connector.h"
#include "ddm2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "sorted_list.h"
#include <string.h>

static RingbufHandle_t broker_ringbuffer_handle;
static EXT_RAM_ATTR uint8_t *broker_ringbuffer_storage;
static EXT_RAM_ATTR SORTED_LIST_KEY_TYPE *keys;
static StaticRingbuffer_t broker_ringbuffer;

DECLARE_SORTED_LIST_EXTRAM_PTR(subscription_table, SUBSCRIPTION_DEPTH);            // single	parameter -> connector ID
DECLARE_SORTED_LIST_EXTRAM_PTR(routing_table, ROUTE_DEPTH);                        // unique   instance -> connector ID
DECLARE_SORTED_LIST_EXTRAM_PTR(instance_table, INSTANCE_DEPTH);                    // unique   instance -> first free instance #
DECLARE_SORTED_LIST_EXTRAM_PTR(unused_instance_table, INSTANCE_DEPTH);             // single   class -> free instance
DECLARE_SORTED_LIST_EXTRAM_PTR(inventory_table, INVENTORY_DEPTH);                  // unique   instance -> AVL
#if (MULTIBROKER == 1)
DECLARE_SORTED_LIST64_EXTRAM(multibroker_client_table, MULTIBROKER_MAX_BROKERS);   // unique	device ID -> client ID
DECLARE_SORTED_LIST_EXTRAM(multibroker_routing_table, MULTIBROKER_MAX_BROKERS);    // unique	connector ID -> client ID
DECLARE_SORTED_LIST_EXTRAM(multibroker_client_id_store, MULTIBROKER_MAX_BROKERS);  // unique	0 -> free client ID
#endif

static SemaphoreHandle_t instance_table_mutex;

#if (MULTIBROKER == 1)
static int broker_id;
#endif
// static int broker_bits;

#if (MULTIBROKER == 1)
//! \~ Adjusts instances of outgoig intentory update
void broker_adjust_inventory(DDMP2_FRAME *const new_frame)
{
    size_t entries = ddmp2_value_size(new_frame) >> 2;
    uint32_t *pentry = (uint32_t *)new_frame->frame.publish.value.raw;
    uint8_t *pinstance;

    for (size_t n = 0; n < entries; n++, pentry++)
    {
        pinstance = ((uint8_t *)pentry) + 1;
        if (!(*pinstance & MULTIBROKER_BROKER_ID_MASK))  // local parameter
        {
            *pinstance |= broker_id << MULTIBROKER_INSTANCE_BITS;
        }
    }
}
#endif

//! \~ Inspects frame and possibly modifies it before going out
static int broker_forward_frame_to_connector(const DDMP2_FRAME *const pframe, const int destination_connector)
{
#if MULTIBROKER == 1
    SORTED_LIST_VALUE_TYPE broker_id;
    SORTED_LIST_RETURN_VALUE get_result;

    get_result = sorted_list_unique_get(&broker_id, &multibroker_routing_table, destination_connector, 0);

    if (get_result == SORTED_LIST_OK)
    {
        DDMP2_FRAME new_frame = *pframe;

        switch (pframe->frame.control)
        {
        case DDMP2_CONTROL_PUBLISH:
            switch (pframe->frame.publish.parameter)
            {
            case GW0INV:
                broker_adjust_inventory(&new_frame);
                return connector_forward_frame_to_connector(&new_frame, destination_connector);
            }
            break;
        }
    }

#endif  // MULTIBROKER

    return connector_forward_frame_to_connector(pframe, destination_connector);
}

//! \~ Add device to inventory if it does not already exists
static SORTED_LIST_RETURN_VALUE broker_add_to_inventory(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL)

    return sorted_list_unique_add(&inventory_table, pframe->frame.publish.parameter, pframe->frame.publish.value.int32);
}

//! \~ Reply to a subscription to the inventory
void broker_serve_inventory(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL)

    const int countperpage = DDMP2_MAX_VALUE_SIZE / 4;
    const int fullpages = inventory_table.entry_count / countperpage;
    int restcount = inventory_table.entry_count - countperpage * fullpages;
    int page, row;

    DDMP2_FRAME inventory_reply =
        {
            .source_connector = pframe->source_connector,
            .destination_connector = pframe->source_connector,
            .frame.control = DDMP2_CONTROL_PUBLISH,
            .frame.publish.parameter = GW0INV,
            .frame_size = DDMP2_CONTROL_SIZE + sizeof(pframe->frame.publish) - sizeof(pframe->frame.publish.value) + countperpage * 4,
        };

    for (page = 0; page < fullpages; page++)
    {
        for (row = 0; row < countperpage; row++)
        {
            memcpy(&inventory_reply.frame.publish.value.raw[4 * row], &inventory_table.pdata[page * countperpage + row].key, 4);
            memcpy(&inventory_reply.frame.publish.value.raw[4 * row], &inventory_table.pdata[page * countperpage + row].value, 1);
        }
        TRUE_CHECK(broker_forward_frame_to_connector(&inventory_reply, inventory_reply.source_connector));
    }

    if (restcount)
    {
        for (row = 0; row < restcount; row++)
        {
            memcpy(&inventory_reply.frame.publish.value.raw[4 * row], &inventory_table.pdata[page * countperpage + row].key, 4);
            memcpy(&inventory_reply.frame.publish.value.raw[4 * row], &inventory_table.pdata[page * countperpage + row].value, 1);
        }
        inventory_reply.frame_size = 1 + sizeof(pframe->frame.publish) - sizeof(pframe->frame.publish.value) + restcount * 4;
        TRUE_CHECK(broker_forward_frame_to_connector(&inventory_reply, inventory_reply.source_connector));
    }
}

//! \~ Forward publish frame to subscribers
int broker_forward_publish(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL)

    SORTED_LIST_VALUE_TYPE connector_list[32];
    int connector_count = ELEMENTS(connector_list);
    MAYBE_UNUSED uint8_t name_buffer[32];
    MAYBE_UNUSED size_t name_length;

    sorted_list_single_get(connector_list, &connector_count, &subscription_table, pframe->frame.publish.parameter, 0);

    while (connector_count-- > 0)  // no split horizon at broker level
    {
        const CONNECTOR_SLOT *const Pconnector = connectors[connector_list[connector_count]];

        name_length = sizeof(name_buffer);
        BROKER_LOG(I, "Forwarding publish for %s (%08x) to %s:%d", ddm2_parameter_name(pframe->frame.publish.parameter, name_buffer, &name_length), pframe->frame.publish.parameter, Pconnector->name, Pconnector->sub_connector_id);

        if (!broker_forward_frame_to_connector(pframe, connector_list[connector_count]))
        {
            LOG(E, "Failed to forward publish to %s:%d (%d bytes)", Pconnector->name, Pconnector->sub_connector_id, pframe->frame_size + DDMP2_METADATA_SIZE);
        }
    }

    return 1;
}

//! \~ Forward fragment frame to subscribers
int broker_forward_fragment(const DDMP2_FRAME *const pframe, const uint32_t parameter)
{
    ASSERT(pframe != NULL)

    SORTED_LIST_VALUE_TYPE connector_list[32];
    int connector_count = ELEMENTS(connector_list);
    MAYBE_UNUSED uint8_t name_buffer[32];
    MAYBE_UNUSED size_t name_length = sizeof(name_buffer);

    sorted_list_single_get(connector_list, &connector_count, &subscription_table, parameter, 0);

    while (connector_count-- > 0)  // no split horizon at broker level
    {
        const CONNECTOR_SLOT *const Pconnector = connectors[connector_list[connector_count]];
        BROKER_LOG(I, "Forwarding fragment for %s (%08x) to %s:%d", ddm2_parameter_name(parameter, name_buffer, &name_length), parameter, Pconnector->name, Pconnector->sub_connector_id);

        if (!broker_forward_frame_to_connector(pframe, connector_list[connector_count]))
        {
            LOG(E, "Failed to forward fragment to %s:%d (%d bytes)", Pconnector->name, Pconnector->sub_connector_id, pframe->frame_size + DDMP2_METADATA_SIZE);
        }
    }

    return 1;
}

//! \~ Send fragment frame to owner connector, use device class + instance as lookup
static int broker_route_fragment(const DDMP2_FRAME *const pframe, const uint32_t parameter)
{
    int route_found;
    SORTED_LIST_VALUE_TYPE destination_connector;
    uint32_t parameter_class_instance = DDM2_PARAMETER_CLASS_INSTANCE(parameter);
    uint8_t name_buffer[32];
    size_t name_length = sizeof(name_buffer);

    ASSERT(pframe != NULL)

    route_found = sorted_list_unique_get(&destination_connector, &routing_table, parameter_class_instance, 0);

    if (route_found)
    {
        if (pframe->source_connector != destination_connector)  // split horizon; if owner sent this frame, don't send it back
        {
            if (broker_forward_frame_to_connector(pframe, destination_connector))
            {
                BROKER_LOG(I, "Routed %s for %s (%08x) to %s:%d", ddmp2_control_string(pframe->frame.control), ddm2_instance_name(parameter_class_instance, name_buffer, &name_length), parameter_class_instance, connectors[destination_connector]->name, connectors[destination_connector]->sub_connector_id);
            }
            else
            {
                LOG(E, "Failed to route %s for %s (%08x) to %s:%d", ddmp2_control_string(pframe->frame.control), ddm2_instance_name(parameter_class_instance, name_buffer, &name_length), parameter_class_instance, connectors[destination_connector]->name, connectors[destination_connector]->sub_connector_id);
            }
        }
    }

    return 1;
}

//! \~ Create and publish an inventory update
static void broker_publish_inventory_update(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL)

    DDMP2_FRAME reply;

    reply = *pframe;
    reply.frame.publish.parameter = GW0INV;
    reply.frame.publish.value.uint32 = pframe->frame.publish.parameter | (pframe->frame.publish.value.uint32 & 0xff);
    reply.frame_size = DDMP2_CONTROL_SIZE + sizeof(pframe->frame.publish) - sizeof(pframe->frame.publish.value) + sizeof(uint32_t);

    broker_forward_publish(&reply);
}

//! \~ Add source connector to routing table for this instance
static SORTED_LIST_RETURN_VALUE broker_add_to_routing_table(const SORTED_LIST_KEY_TYPE class_instance, const SORTED_LIST_VALUE_TYPE connector)
{
    uint8_t name_buffer[32];
    size_t name_length = sizeof(name_buffer);

    SORTED_LIST_RETURN_VALUE result = sorted_list_unique_add(&routing_table, class_instance, connector);

    switch (result)
    {
    case SORTED_LIST_ENTRY_INSERTED:
        LOG(I, "Added instance %s (%08x) to routing table for %s:%d", ddm2_instance_name(class_instance, name_buffer, &name_length), class_instance, connectors[connector]->name, connectors[connector]->sub_connector_id);
        break;
    case SORTED_LIST_ENTRY_UPDATED:
        LOG(E, "Rerouted %s (%08x) to %s:%d!", ddm2_instance_name(class_instance, name_buffer, &name_length), class_instance, connectors[connector]->name, connectors[connector]->sub_connector_id);
        break;
    case SORTED_LIST_NO_CHANGE:
        break;
    case SORTED_LIST_FAIL:
        LOG(E, "Routing table full!");
        break;
    default:
        LOG(W, "Unhandled return value %d!", result);
        break;
    }

    return result;
}

//! \~ Send set frame to owner connector, use device class + instance as lookup
static int broker_route_frame(const DDMP2_FRAME *const pframe)
{
    int route_found;
    SORTED_LIST_VALUE_TYPE destination_connector;
    uint32_t parameter_class_instance;
    uint8_t name_buffer[32];
    size_t name_length = sizeof(name_buffer);

    ASSERT(pframe != NULL)

    switch (pframe->frame.control)
    {
    case DDMP2_CONTROL_SET:
        parameter_class_instance = DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.set.parameter);
        break;

    case DDMP2_CONTROL_SUBSCRIBE:
        parameter_class_instance = DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.subscribe.parameter);
        break;

    default:
        return 0;
    }

    route_found = sorted_list_unique_get(&destination_connector, &routing_table, parameter_class_instance, 0);

    if (route_found)
    {
        if (pframe->source_connector != destination_connector)  // split horizon; if owner sent this frame, don't send it back
        {
            if (broker_forward_frame_to_connector(pframe, destination_connector))
            {
                BROKER_LOG(I, "Routed %s for %s (%08x) to %s:%d", ddmp2_control_string(pframe->frame.control), ddm2_instance_name(parameter_class_instance, name_buffer, &name_length), parameter_class_instance, connectors[destination_connector]->name, connectors[destination_connector]->sub_connector_id);
            }
            else
            {
                LOG(E, "Failed to route %s for %s (%08x) to %s:%d", ddmp2_control_string(pframe->frame.control), ddm2_instance_name(parameter_class_instance, name_buffer, &name_length), parameter_class_instance, connectors[destination_connector]->name, connectors[destination_connector]->sub_connector_id);
            }
        }
    }

    return 1;
}

//! \~ Wipe a class instance from broker state
static void broker_remove_class_instance(const uint32_t class_instance)
{
    sorted_list_unique_remove(&routing_table, class_instance);                                                                            // clear routes to this instance
    sorted_list_single_add(&unused_instance_table, DDM2_PARAMETER_CLASS(class_instance), DDM2_PARAMETER_INSTANCE_FIELD(class_instance));  // recycle the instance

    for (int position = (int)subscription_table.entry_count - 1; position >= 0; position--)  // clear subscriptions to this instance
    {
        if (DDM2_PARAMETER_CLASS_INSTANCE(subscription_table_data[position].key) == class_instance)
        {
            sorted_list_remove_entry(&subscription_table, position);
        }
    }
}

//! \~ Force an instance to be unavailable and remove associated broker state
static void broker_make_class_instance_unavailable(const uint32_t class_instance)
{
    DDMP2_FRAME avl_frame;
    SORTED_LIST_VALUE_TYPE avl;
    SORTED_LIST_RETURN_VALUE get_result = sorted_list_unique_get(&avl, &inventory_table, class_instance, 0);

    if ((get_result == SORTED_LIST_OK) && avl)                                      // already in inventory and available?
    {                                                                               // then force it unavailable
        sorted_list_unique_add(&inventory_table, class_instance, 0);                // set AVL 0
        ddmp2_create_publish(&avl_frame, class_instance, &Zero, sizeof(Zero), -1);  // create a fake AVL 0 frame
        broker_publish_inventory_update(&avl_frame);                                // produce inventory update from fake AVL 0 frame
        broker_forward_publish(&avl_frame);                                         // transmit fake AVL 0 frame
    }

    broker_remove_class_instance(class_instance);  // remove instance broker state
}

//! \~ Wipe a connector's state from broker
static void broker_remove_connector_instances(const int connector)
{
    int key_count = ROUTE_DEPTH;
    sorted_list_get_keys(keys, &key_count, &routing_table, connector, 1);  // get and remove all class instances associated with this connector

    for (int key = 0; key < key_count; key++)
    {
        broker_make_class_instance_unavailable(keys[key]);  // force each class instance unavailable and remove its broker state
    }

    key_count = ROUTE_DEPTH;  // remove subscriptions from this connector
    sorted_list_get_keys(keys, &key_count, &subscription_table, connector, 1);

#if (MULTIBROKER == 1)
    sorted_list_unique_remove(&multibroker_routing_table, connector);  // remove multibroker state
#endif
}

//! \~ Request device instance for device class from broker. Return instance number, -1 if fail.
int broker_request_instance_n(const uint32_t device_class)
{
    SORTED_LIST_VALUE_TYPE instance;
    uint32_t parameter_class = DDM2_PARAMETER_CLASS(device_class);

    TRUE_CHECK(xSemaphoreTake(instance_table_mutex, portMAX_DELAY));
    {
        int get_result = sorted_list_unique_get(&instance, &unused_instance_table, parameter_class, 1);  // get first match in single list

        if (get_result == SORTED_LIST_FAIL)
        {
            get_result = sorted_list_unique_get(&instance, &instance_table, parameter_class, 0);
            instance = (get_result == SORTED_LIST_OK) ? instance + 1 : 0;

            if (instance > 254)  // TODO: add new maximum
            {
                LOG(E, "Out of instances for class %s (%08x)!", ddm2_class_name(parameter_class), parameter_class);
                TRUE_CHECK(xSemaphoreGive(instance_table_mutex));
                return -1;
            }

            TRUE_CHECK(sorted_list_unique_add(&instance_table, parameter_class, instance));
            BROKER_LOG(I, "Allocated new instance %02x for class %s (%08x)", instance, ddm2_class_name(parameter_class), parameter_class);
        }
        else
        {
            BROKER_LOG(I, "Reused instance %02x for class %s (%08x)", instance, ddm2_class_name(parameter_class), parameter_class);
        }

        TRUE_CHECK(xSemaphoreGive(instance_table_mutex));
    }

    return instance;
}

//! \~ Request device instance for device class from broker, register it, make it available and return the instance number
int broker_register_instance(uint32_t *const device_class, const int connector_id)
{
    const int Instance = broker_request_instance_n(*device_class);

    if (Instance == -1)
    {
        return -1;
    }

    *device_class = DDM2_PARAMETER_CLASS(*device_class) | DDM2_PARAMETER_INSTANCE(Instance);                                           // Add instance number to class
    broker_add_to_routing_table(*device_class, connector_id);                                                                          // Add instance to routing table
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, *device_class, &One, sizeof(One), connector_id, portMAX_DELAY));  // Make instance available

    return Instance;
};

//! \~ Handle REG frame from DDM2; register device instance and reply with the new instance number
static void broker_register_instance_ddm2(const DDMP2_FRAME *const pframe)
{
    ASSERT(pframe != NULL)

    uint32_t parameter_class = DDM2_PARAMETER_CLASS(pframe->frame.subscribe.parameter);

    if (broker_register_instance(&parameter_class, pframe->source_connector) != -1)
    {
        connector_send_frame_to_connector(DDMP2_CONTROL_REG, parameter_class, NULL, 0, pframe->source_connector, BROKER_SEND_TIMEOUT);  // Send REG reply with new instance number
    }
    else
    {
        LOG(E, "Failed to register instance for REG from %s:%d", connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
    }
}

//! \~ Add parameter to subscription table if it does not already exists
static SORTED_LIST_RETURN_VALUE broker_add_subscription(const DDMP2_FRAME *const psub)
{
    TRUE_CHECK_RETURNX(SORTED_LIST_INVALID_ARGUMENT, psub != NULL);

    uint8_t name_buffer[32];
    size_t name_length = sizeof(name_buffer);

    SORTED_LIST_RETURN_VALUE result = sorted_list_single_add(&subscription_table, psub->frame.subscribe.parameter, psub->source_connector);

    switch (result)
    {
    case SORTED_LIST_ENTRY_INSERTED:
        LOG(I, "Added subscription for %s (%08x) for %s:%d", ddm2_parameter_name(psub->frame.subscribe.parameter, name_buffer, &name_length), psub->frame.subscribe.parameter, connectors[psub->source_connector]->name, connectors[psub->source_connector]->sub_connector_id);
        break;

    case SORTED_LIST_NO_CHANGE:
        BROKER_LOG(I, "Parameter %s (%08x) already subscribed to by %s:%d!", ddm2_parameter_name(psub->frame.subscribe.parameter, name_buffer, &name_length), psub->frame.subscribe.parameter, connectors[psub->source_connector]->name, connectors[psub->source_connector]->sub_connector_id);
        break;

    case SORTED_LIST_FAIL:
        LOG(E, "Subscription table full!");
        break;

    default:
        LOG(W, "Unhandled return value!");
        break;
    }

    return result;
}

//! \~ Remove parameter from subscription table if it exists
static SORTED_LIST_RETURN_VALUE broker_remove_subscription(const DDMP2_FRAME *const pframe)
{
    TRUE_CHECK_RETURNX(SORTED_LIST_INVALID_ARGUMENT, pframe != NULL);

    uint8_t name_buffer[32];
    size_t name_length = sizeof(name_buffer);

    SORTED_LIST_RETURN_VALUE result = sorted_list_remove_entries(&subscription_table, pframe->frame.unsubscribe.parameter, pframe->source_connector);

    switch (result)
    {
    case SORTED_LIST_OK:
        LOG(I, "Removed subscription for %s (%08x) for %s:%d", ddm2_parameter_name(pframe->frame.unsubscribe.parameter, name_buffer, &name_length), pframe->frame.unsubscribe.parameter, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
        break;

    case SORTED_LIST_NO_CHANGE:
        BROKER_LOG(I, "Parameter %s (%08x) was not subscribed to by %s:%d!", ddm2_parameter_name(pframe->frame.unsubscribe.parameter, name_buffer, &name_length), pframe->frame.unsubscribe.parameter, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
        break;

    default:
        LOG(W, "Unhandled return value %d!", result);
        break;
    }

    return result;
}

//! \~ Lookup AVL in inventory and reply with its status
static void broker_handle_avl_subscribe(const DDMP2_FRAME *const pframe)
{
    uint32_t avl;

    int get_result = sorted_list_unique_get(&avl, &inventory_table, pframe->frame.subscribe.parameter, 0);
    avl = get_result ? avl : 0;
    gateway_reply_int32(pframe, avl);
}

static void broker_handle_subscribe(const DDMP2_FRAME *const pframe)
{
    broker_add_subscription(pframe);  // record subscription in subscription table

    if (DDM2_IS_AVAIL_PROPERTY(pframe->frame.subscribe.parameter))
    {
        broker_handle_avl_subscribe(pframe);
    }
    else if (DDM2_IS_GATEWAY_CLASS(pframe->frame.subscribe.parameter))
    {
        gateway_handle_subscribe(pframe);
    }
    else if (DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.subscribe.parameter) == DDM2_PARAMETER_CLASS_INSTANCE(PROD0))
    {
        production_handle_subscribe(pframe);  // Here we only handle DICM production class instance 0
    }
    else
    {
        broker_route_frame(pframe);  // send frame to owner connector from the routing table
    }
}

static void broker_handle_unsubscribe(const DDMP2_FRAME *const pframe)
{
    broker_remove_subscription(pframe);
}

#if (MULTIBROKER == 1)
static int broker_add_multibroker_client(const DDMP2_FRAME *const pframe)
{
    SORTED_LIST64_VALUE_TYPE broker_id;
    SORTED_LIST64_RETURN_VALUE get_result64, set_result64;
    SORTED_LIST_RETURN_VALUE get_result;
    SORTED_LIST_VALUE_TYPE new_broker_id;
    const uint64_t broker_bits = MULTIBROKER_BROKER_BITS;

    get_result64 = sorted_list64_unique_get(&broker_id, &multibroker_client_table, pframe->frame.multibroker.data, 0);

    switch (get_result64)
    {
    case SORTED_LIST64_OK:  // a previously connected client
        LOG(I, "Added known multibroker client %" PRIx64 " as %" PRIu64, pframe->frame.multibroker.data, broker_id);
        break;
    default:  // not a known client, get a new number for it
        get_result = sorted_list_unique_get(&new_broker_id, &multibroker_client_id_store, 0, 1);

        if (get_result == SORTED_LIST_FAIL)
        {
            LOG(W, "Failed to get new multibroker client ID for %" PRIx64, pframe->frame.multibroker.data);
            TRUE_CHECK(connector_send_frame_to_connector(DDMP2_CONTROL_MULTIBROKER, DDMP2_MULTIBROKER_ACK, &broker_bits, sizeof(broker_bits), pframe->source_connector, BROKER_SEND_TIMEOUT));
            return 0;
        }

        set_result64 = sorted_list64_unique_add(&multibroker_client_table, pframe->frame.multibroker.data, new_broker_id);

        if (set_result64 == SORTED_LIST64_FAIL)
        {
            LOG(W, "Failed to add new multibroker client %" PRIx64, pframe->frame.multibroker.data);
            TRUE_CHECK(connector_send_frame_to_connector(DDMP2_CONTROL_MULTIBROKER, DDMP2_MULTIBROKER_ACK, &broker_bits, sizeof(broker_bits), pframe->source_connector, BROKER_SEND_TIMEOUT));
            return 0;
        }

        LOG(I, "Added new multibroker client %" PRIx64 " as %" PRIu32, pframe->frame.multibroker.data, new_broker_id);
    }

    TRUE_CHECK(sorted_list_unique_add(&multibroker_routing_table, pframe->source_connector, broker_id));
    TRUE_CHECK(connector_send_frame_to_connector(DDMP2_CONTROL_MULTIBROKER, DDMP2_MULTIBROKER_ACK, &broker_bits, sizeof(broker_bits), pframe->source_connector, BROKER_SEND_TIMEOUT));

    return 1;
};
#endif

static TaskHandle_t broker_process_task_handle;
//! \~ Main broker task, receives and handles frames from connectors
static void broker_process_task(void *parameter)
{
    DDMP2_FRAME l_frame;
    DDMP2_FRAME *pframe;
    size_t frame_size;
    uint32_t fragment_parameter;
    MAYBE_UNUSED uint8_t name_buffer[32];
    MAYBE_UNUSED size_t name_length;

    while (1)
    {
        TRUE_CHECK((pframe = xRingbufferReceive(broker_ringbuffer_handle, &frame_size, portMAX_DELAY)) != NULL);

        if (pframe)
        {
            memcpy(&l_frame, pframe, frame_size);
        }

        vRingbufferReturnItem(broker_ringbuffer_handle, pframe);
        pframe = &l_frame;

        if (frame_size != 0)
        {
            if (pframe->source_connector >= connector_count)
            {
                LOG(E, "Spurious connector %02x in frame to broker! Ignoring frame!", pframe->source_connector);
                continue;
            }

            name_length = sizeof(name_buffer);

            switch (pframe->frame.control)
            {
            case DDMP2_CONTROL_MESSAGE:
                switch (pframe->frame.message.id)
                {
                case DDMP2_MESSAGE_RESET:  // reset message, peer has just started up or disconnected
                    LOG(I, "Deleting broker state for %s:%d", connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
                    broker_remove_connector_instances(pframe->source_connector);
                    break;
                case DDMP2_MESSAGE_PING:  // ping message
                    LOG(E, "Received spurious ping");
                    break;
                }
                break;

            case DDMP2_CONTROL_PUBLISH:
                BROKER_LOG(I, "Received PUBLISH %s (%08x) from %s:%d", ddm2_parameter_name(pframe->frame.publish.parameter, name_buffer, &name_length), pframe->frame.publish.parameter, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);

                if (DDM2_IS_AVAIL_PROPERTY(pframe->frame.publish.parameter))
                {
                    broker_add_to_inventory(pframe);
                    broker_publish_inventory_update(pframe);

                    if (pframe->frame.publish.value.int32)  // instance is now available?
                    {
                        SORTED_LIST_RETURN_VALUE add_result = broker_add_to_routing_table(DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.publish.parameter), pframe->source_connector);  // add device instance to routing table

                        if (add_result == SORTED_LIST_ENTRY_INSERTED)  // this instance was not previously registered?
                        {
                            LOG(E, "%s:%d stole instance %s (%08x) from broker", connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id, ddm2_instance_name(pframe->frame.publish.parameter, name_buffer, &name_length), pframe->frame.publish.parameter);
                        }

                        broker_forward_publish(pframe);  // forward frame to subscribers AFTER adding routing information
                    }
                    else
                    {
                        broker_forward_publish(pframe);  // forward frame to subscribers BEFORE removing subscriptions
                        BROKER_LOG(I, "Deleting broker state for instance %s (%08x)", ddm2_instance_name(pframe->frame.publish.parameter, name_buffer, &name_length), pframe->frame.publish.parameter);
                        broker_remove_class_instance(pframe->frame.publish.parameter);  // remove subscriptions and routing for this instance
                    }
                }
                else
                {
                    broker_forward_publish(pframe);  // forward frame to subscribers
                }
                break;

            case DDMP2_CONTROL_SET:
                BROKER_LOG(I, "Received SET %s (%08x) from %s:%d", ddm2_parameter_name(pframe->frame.set.parameter, name_buffer, &name_length), pframe->frame.set.parameter, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);

                if (DDM2_IS_GATEWAY_CLASS(pframe->frame.set.parameter))
                {
                    gateway_handle_set(pframe);
                }
                else if (DDM2_PARAMETER_CLASS_INSTANCE(pframe->frame.subscribe.parameter) == DDM2_PARAMETER_CLASS_INSTANCE(PROD0))
                {
                    // Here we only handle DICM production class instance, 0
                    production_handle_set(pframe);
                }
                else
                {
                    broker_route_frame(pframe);  // send frame to owner connector from the routing table
                }
                break;

            case DDMP2_CONTROL_SUBSCRIBE:
                BROKER_LOG(I, "Received SUBSCRIBE %s (%08x) from %s:%d", ddm2_parameter_name(pframe->frame.subscribe.parameter, name_buffer, &name_length), pframe->frame.subscribe.parameter, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
                broker_handle_subscribe(pframe);
                break;

            case DDMP2_CONTROL_UNSUBSCRIBE:
                BROKER_LOG(I, "Received UNSUBSCRIBE %s (%08x) from %s:%d", ddm2_parameter_name(pframe->frame.subscribe.parameter, name_buffer, &name_length), pframe->frame.subscribe.parameter, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
                broker_handle_unsubscribe(pframe);
                break;

            case DDMP2_CONTROL_NOP:
                break;

            case DDMP2_CONTROL_FRAGMENT:
            {
                // Identify the DDM2 parameter for this fragment. The parameter can be
                // found on the first 4 bytes of the first fragment frame only.
                if (pframe->frame.fragment.offset == 0)
                {
                    memcpy(&fragment_parameter, pframe->frame.fragment.value, 4);
                    BROKER_LOG(I, "Received FRAGMENT %08x from %s:%d",
                               fragment_parameter,
                               connectors[pframe->source_connector]->name,
                               connectors[pframe->source_connector]->sub_connector_id);
                }

                if (DDM2_IS_GATEWAY_CLASS(fragment_parameter))
                {
                    gateway_handle_fragment_frame(pframe);
                }
                else
                {
                    // Send to subscribers of the parameter
                    if (pframe->source_connector != pframe->destination_connector)
                    {
                        broker_forward_fragment(pframe, fragment_parameter);
                    }
                    else
                    {
                        broker_route_fragment(pframe, fragment_parameter);
                    }
                }
                break;
            }

            case DDMP2_CONTROL_REG:
                BROKER_LOG(I, "Received REG %08x from %s:%d", pframe->frame.subscribe.parameter, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
                broker_register_instance_ddm2(pframe);
                break;
        #if (MULTIBROKER == 1)
            case DDMP2_CONTROL_MULTIBROKER:
            {
                switch (pframe->frame.multibroker.control)
                {
                case DDMP2_MULTIBROKER_CONNECT:
                    if (broker_add_multibroker_client(pframe))
                    {
                        broker_id = MULTIBROKER_SERVER_ID;
                        broker_serve_inventory(pframe);
                    }
                    break;

                case DDMP2_MULTIBROKER_ACK:
                    break;
                }

                break;
            }
        #endif
            case DDMP2_CONTROL_GENERIC:
                LOG(E, "GENERIC frame received from %s:%d is not supported!", connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
                break;

            default:
                LOG(E, "Received UNHANDLED frame %02x from %s:%d!", pframe->frame.control, connectors[pframe->source_connector]->name, connectors[pframe->source_connector]->sub_connector_id);
                break;
            }
        }
    }
}

//! \~ Initialize connectors and broker
void initialize_broker(void)
{
    LOG(I, "Initializing broker");

    if (!connectors_enabled())
    {
        return;
    }

    gateway_init();
    // Explicit allocaton in external memory
    broker_ringbuffer_storage = heap_caps_malloc_prefer(TO_BROKER_RINGBUFFER_SIZE, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
    keys = heap_caps_malloc_prefer(ROUTE_DEPTH * sizeof(SORTED_LIST_KEY_TYPE), 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
    INIT_SORTED_LIST_EXTRAM_PTR(subscription_table);
    INIT_SORTED_LIST_EXTRAM_PTR(routing_table);
    INIT_SORTED_LIST_EXTRAM_PTR(instance_table);
    INIT_SORTED_LIST_EXTRAM_PTR(unused_instance_table);
    INIT_SORTED_LIST_EXTRAM_PTR(inventory_table);

    sorted_list_unique_add(&inventory_table, GW0AVL, 1);
    sorted_list_unique_add(&inventory_table, CFG0AVL, 1);
    sorted_list_unique_add(&inventory_table, PROD0AVL, 1);

#if (MULTIBROKER == 1)
    for (unsigned int id = 1; id <= MULTIBROKER_MAX_CLIENTS; id++)  // populate broker id store
    {
        multibroker_client_id_store_data[id - 1].value = id;
    }

    multibroker_client_id_store.entry_count = multibroker_client_id_store.capacity = MULTIBROKER_MAX_CLIENTS;
#endif
    TRUE_CHECK((instance_table_mutex = xSemaphoreCreateMutex()) != NULL);
    TRUE_CHECK((broker_ringbuffer_handle = xRingbufferCreateStatic(TO_BROKER_RINGBUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, broker_ringbuffer_storage, &broker_ringbuffer)) != NULL);

    production_init();

    TRUE_CHECK(xTaskCreate(broker_process_task, "broker", 6144, NULL, xTASK_PRIORITY_HIGH, &broker_process_task_handle));

    LOG(I, "Initializing connectors");
    install_connectors(broker_ringbuffer_handle, gateway_parameter_update_task);
}
