/**
 * \file        connector_event_notification.c
 * \date        2024-06-27
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Notification connector implementation
 *
 * Implementation of Event Notification connector.
 *
 * \li          2024-06-27  (NR) Initial implementation
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

#include <string.h>

#include "broker.h"
#include "configuration.h"
#include "connector_event_notification.h"
#include "connector_event_notification_defaults.h"
#include "ddm_converter.h"
#include "ddm_store.h"
#include "event_manager.h"
#include "event_record.h"
#include "event_record_db.h"
#include "fifo_fixed.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "fsm.h"
#include "inventory_handler.h"
#include "make_json.h"

#if (CONNECTOR_EVENT_NOTIFICATION_VERBOSE_LOG == 1)
#define EVENT_NOTIFICATION_LOG(level, format, ...) LOG(level, format, ##__VA_ARGS__)
#else
#define EVENT_NOTIFICATION_LOG(level, format, ...)
#endif

#define OPS_SUCCESS 0
#define OPS_FAILURE 1

/**
 * @brief       Cast a member of a structure out to the containing structure.
 * @param       ptr the pointer to the member.
 * @param       type the type of the container struct this is embedded in.
 * @param       member the name of the member within the struct.
 */
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(uintptr_t)ptr - offsetof(type, member)))

/**
 * @brief    This is a context structure for event manager.
 *
 * It contains all things bundled together so they are accessible from a single
 * pointer.
 */
typedef struct event_notification
{
    CONNECTOR *connector;
    ddm_store_t owned_ddm_store;            //!< DDM store handle for owned EVM and EVN DDM parameters
    inventory_handler_t inventory_handler;  //!< Inventory handler for other DDM parameters
    fsm_t fsm;                              //!< FSM for handling timeouts and event sending
    StaticSemaphore_t fsm_mutex_buffer;     //!< Mutex storage
    SemaphoreHandle_t fsm_mutex;            //!< Mutex used to serialize access to FSM
    StaticTimer_t fsm_timer_buffer;         //!< Timer storage
    TimerHandle_t fsm_timer;                //!< Timer used for timeouts.
    uint32_t fsm_timer_event;               //!< ID of the time event that needs to be sent.
    fifo_fixed_t fifo_queue;
} event_notification_t;

enum evn_fsm_events
{
    EVN_FSM_EVENT_NEW = FSM_USER_EVENT,  //!< Event comming from event manager
    EVN_FSM_EVENT_ACK,                   //!< Event comming from event manager
    EVN_FSM_EVENT_DEL_ALL,               //!< Event comming from event manager
    EVN_FSM_EVENT_ACK_ALL,               //!< Event comming from event manager
    EVN_FSM_EVENT_REC_IN,                //!< Event comming from Broker
    EVN_FSM_EVENT_ACK_IN,                //!< Event comming from Broker
    EVN_FSM_EVENT_TIMEOUT,
};

typedef struct evn_fsm_event_new
{
    event_id_t new_event_id;
} evn_fsm_event_new_t;

typedef struct evn_fsm_event_ack
{
    event_id_t ack_event_id;
} evn_fsm_event_ack_t;

typedef struct evn_fsm_event_rec_in
{
    event_id_t rec_in_event_id;
} evn_fsm_event_rec_in_t;

typedef struct evn_fsm_event_ack_in
{
    char ack_in_request[DDMP2_MAX_VALUE_SIZE];
} evn_fsm_event_ack_in_t;

/* -- Converter functions -- */
static void convert_evn0rec(void *arg, const DDMP2_FRAME *in_frame, ddm_entry_t *out_type_ddm);
static void convert_evn0ack(void *arg, const DDMP2_FRAME *in_frame, ddm_entry_t *out_type_ddm);

/* -- Misc helpers -- */
static event_notification_t *evn_from_fsm(fsm_t *fsm);
static int evn_broker_publish(const event_notification_t *evn, const ddm_entry_t *entry);
static int evn_broker_publish_changed(event_notification_t *evn);

/* -- DDM handling -- */
static int32_t handle_owned_parameter_register(uint32_t ddm_class, const CONNECTOR *connector);
static void handle_owned_parameter_set(
    event_notification_t *evn,
    const DDMP2_FRAME *ddmp2_frame);
static void handle_owned_parameter_subscribe(
    event_notification_t *evn,
    const DDMP2_FRAME *ddmp2_frame);

static void inventory_handler_cb(void *arg, uint32_t device_class_instance, bool is_available);

/* -- Timer handling -- */

static void timer_callback(TimerHandle_t fsm_timer);
static void evn_fsm_timer_init(event_notification_t *evn);
static void evn_fsm_timer_start_after(event_notification_t *evn, uint32_t after_ms, uint32_t fsm_event);
static void evn_fsm_timer_cancel(event_notification_t *evn);

/* -- FSM states -- */
static void evn_fsm_state_idle(fsm_t *fsm, fsm_event_t const *event);
static void evn_fsm_state_pending(fsm_t *fsm, fsm_event_t const *event);
static void evn_fsm_state_cooldown(fsm_t *fsm, fsm_event_t const *event);

/* -- FSM handling -- */
static void evn_fsm_init(event_notification_t *evn);
static void evn_fsm_dispatch(event_notification_t *evn, const fsm_event_t *fsm_event);

/* -- Event notification functions -- */
static void event_notification_init(
    event_notification_t *evn,
    CONNECTOR *connector,
    SORTED_LIST *p_inventory_handler_storage,
    SORTED_CONTAINER *p_owned_ddm_store_storage,
    event_id_t *fifo_storage,
    size_t fifo_storage_size);
static void event_notification_init_owned_ddm(
    event_notification_t *evn,
    const ddm_store_ddm_t *owned_ddm_values,
    size_t owned_ddm_values_size,
    int32_t ddm_class_instance);

/**
 * @brief       The event manager context structure instance
 *
 * The connector needs one instance of event_notification structure to hold all
 * relevant data needed for operation.
 */
static event_notification_t event_notification;

/**
 * @brief       This array defines out_type DDM parameters initial values
 *              which are owned by this connector (EVN class).
 *
 * Since these are owned by us, this array is defined using the DDM parameter
 * OUT types.
 */
static const ddm_store_ddm_t owned_evn_ddm_values[] = {
    {
        .ddm_parameter = EVN0AVL,
        .value = {.storage = {.i32 = 1}, .type = DDM2_TYPE_INT32_T},
    },
    {
        .ddm_parameter = EVN0ID,
        .value = {.storage = {.i32 = 0}, .type = DDM2_TYPE_INT32_T},
    },
    {
        .ddm_parameter = EVN0ACK,
        .value = {.storage = {.str = "new"}, .type = DDM2_TYPE_STRING},
    },
    {
        .ddm_parameter = EVN0REC,
        .value = {.storage = {.str = "{}"}, .type = DDM2_TYPE_STRING},
    },
};

static const ddm_converter_t event_notification_conversion_table[] = {
    {
        .parameter_id = EVN0ACK,
        .fn = convert_evn0ack,
    },
    {
        .parameter_id = EVN0REC,
        .fn = convert_evn0rec,
    },
};

/* Sorted container that stores our owned EVM and EVN DDM parameters */
SORTED_CONTAINER__DECLARE_EXTRAM(
    event_notification_owned_ddm_store_storage,
    ELEMENTS(owned_evn_ddm_values),
    ddm_entry_t);

/* Inventory used when subscribing to other DDM parameters */
DECLARE_SORTED_LIST_EXTRAM(event_notification_inventory_handler_storage, 1);

/* FIFO Storage */
static event_id_t event_notification_fifo_storage[CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS];

/* -- Converter functions -- */
static void convert_evn0rec(void *arg, const DDMP2_FRAME *in_frame, ddm_entry_t *out_type_ddm)
{
    /* Expected input type is int32_t */
    if (ddmp2_value_size(in_frame) != sizeof(int32_t))
    {
        /* We got an incomplete SET, ignore the value, do not generate publish? */
        LOG(W, "Expected frame with int32 value");
    }
    else
    {
        event_notification_t *evn = arg;

        /* It seems we got a complete frame, process it. */
        int32_t event_id = in_frame->frame.set.value.int32;

        evn_fsm_event_rec_in_t rec_in_event_data = {
            .rec_in_event_id = event_id,
        };
        fsm_event_t rec_in_event = {
            .id = EVN_FSM_EVENT_REC_IN,
            .p_data = &rec_in_event_data,
        };
        evn_fsm_dispatch(evn, &rec_in_event);
    }
}

static void convert_evn0ack(void *arg, const DDMP2_FRAME *in_frame, ddm_entry_t *out_type_ddm)
{
    /* Expected input type is a string */
    if (ddmp2_value_size(in_frame) == 0)
    {
        /* We got an incomplete SET, ignore the value, do not generate publish? */
        LOG(W, "Expected frame with string value");
    }
    else
    {
        event_notification_t *evn = arg;

        /* It seems we got a complete frame, process it. */
        evn_fsm_event_ack_in_t ack_in_event_data;
        ddmp2_extract_string_from_frame(
            in_frame,
            &ack_in_event_data.ack_in_request[0],
            sizeof(ack_in_event_data.ack_in_request));
        fsm_event_t ack_in_event = {
            .id = EVN_FSM_EVENT_ACK_IN,
            .p_data = &ack_in_event_data,
        };
        evn_fsm_dispatch(evn, &ack_in_event);
    }
}

/* -- Misc helpers -- */

static event_notification_t *evn_from_fsm(fsm_t *fsm)
{
    return CONTAINER_OF(fsm, event_notification_t, fsm);
}

/**
 * @brief       Helper function to publish a DDM parameter value in specified
 *              @a entry.
 */
static int evn_broker_publish(const event_notification_t *evn, const ddm_entry_t *entry)
{
    const void *data = NULL;
    size_t data_size = 0u;

    ddm_entry__read__value(entry, &data, &data_size);
    EVENT_NOTIFICATION_LOG(
        I,
        "DDM entry %s0%s(0x%x) publish data %p, data_size %u",
        ddm_entry__device_class(entry), ddm_entry__property(entry),
        ddm_entry__parameter_id(entry),
        data,
        data_size);
    TRUE_CHECK_RETURNX(OPS_FAILURE, (data != NULL) && (data_size != 0));
    return connector_send_frame_to_broker(
               DDMP2_CONTROL_PUBLISH,
               ddm_entry__parameter_id(entry),
               data,
               data_size,
               evn->connector->connector_id,
               portMAX_DELAY) == true
               ? OPS_SUCCESS
               : OPS_FAILURE;
}

static int evn_broker_publish_changed(event_notification_t *evn)
{
    int retval = OPS_SUCCESS;
    /* Do publish of changed values */
    for (size_t i = 0; i < ddm_store__occupied(&evn->owned_ddm_store); i++)
    {
        ddm_entry_t *ddm_entry;

        ddm_store__iterate(&evn->owned_ddm_store, i, &ddm_entry);
        if (ddm_entry__has_changed(ddm_entry) && ddm_entry__is_subscribed(ddm_entry))
        {
            retval |= evn_broker_publish(evn, ddm_entry);
            /* Regardless if publish was successful or not we are going to clear
             * `has_changed` flag.
             */
            ddm_entry__set__has_changed(ddm_entry, false);
        }
    }
    return retval;
}

/* -- DDM handling -- */

/**
 * @brief       Register the DDMP2 class instance with broker
 *
 * @param       ddm_class DDM class which needs to be registerd.
 * @param       connector connector instance
 * @return      Operation error if the operation failed, ddm_class instance
 *              otherwise.
 */
static int32_t handle_owned_parameter_register(uint32_t ddm_class, const CONNECTOR *connector)
{
    int instance;

    /* We want to be the owner of the parameters. Request an instance then
     * publish the available DDM parameter.
     */
    instance = broker_register_instance(&ddm_class, connector->connector_id);

    return (int32_t)instance;
}

/**
 * @brief       Handle set frames for our own DDM parameters.
 *
 * This function will:
 * - Get the DDM from DDM store
 * - Store the value into DDM according to DDM in_type
 */
static void handle_owned_parameter_set(event_notification_t *evn, const DDMP2_FRAME *ddmp2_frame)
{
    ddm_entry_t *ddm_entry;

    ddm_entry = ddm_store__access(&evn->owned_ddm_store, ddmp2_frame->frame.set.parameter);

    if (ddm_entry == NULL)
    {
        /* Somebody tried to set a value to something that we haven't created */
        LOG(W, "%s: set non-existing DDM: 0x%x",
            evn->connector->name,
            ddmp2_frame->frame.set.parameter);
    }
    else
    {
        /* We need to do proper conversion from DDM frame (IN type) to DDM value
         * (OUT type) in DDM store/DDM entry.
         */
        bool is_converted = ddm_converter_set_and_store(
                                event_notification_conversion_table,
                                ELEMENTS(event_notification_conversion_table),
                                evn,
                                ddmp2_frame,
                                ddm_entry) == DDM_CONVERTER_ERR_NONE
                                ? true
                                : false;

        if (!is_converted)
        {
            EVENT_NOTIFICATION_LOG(W, "No converter function for DDM Parameter 0x%x", ddmp2_frame->frame.set.parameter);
        }
    }
}

/**
 * @brief       Handle subscribe frames of our own parameters.
 *
 * This function will handle subscribe frames that we receive for subscription
 * to EVM and EVN DDM parameters.
 */
static void handle_owned_parameter_subscribe(
    event_notification_t *evn,
    const DDMP2_FRAME *ddmp2_frame)
{
    ddm_entry_t *ddm_entry;

    ddm_entry = ddm_store__access(&evn->owned_ddm_store, ddmp2_frame->frame.set.parameter);

    if (ddm_entry == NULL)
    {
        /* Somebody has subscribed to something that we haven't created */
        LOG(W, "%s: subscribe to non-existing DDM: 0x%x",
            evn->connector->name,
            ddmp2_frame->frame.set.parameter);
    }
    else
    {
        ddm_entry__set__is_subscribed(ddm_entry, true);
        TRUE_CHECK(evn_broker_publish(evn, ddm_entry) == OPS_SUCCESS);
        /* Regardless if publish was successful or not we are going to clear
         * `has_changed` flag.
         */
        ddm_entry__set__has_changed(ddm_entry, false);
    }
}

/**
 * @brief       Event manager callback function
 */
static void event_manager_event_listener_cb(
    void *arg,
    event_manager_listener_event_t event,
    event_id_t event_id)
{
    event_notification_t *evn = arg;

    EVENT_NOTIFICATION_LOG(I, "Received callback: event: %d, event_id: %d", event, event_id);
    switch (event)
    {
    case EVENT_MANAGER_LISTENER_EVENT_NEW:
    {
        evn_fsm_event_new_t new_event_data = {
            .new_event_id = event_id,
        };
        // Should we check if this event type is on cloud?
        // Insert event_record event_id into our FIFO
        // Check should we update EVN0ID as well
        // We should publish EVN0ACK with "new"
        fsm_event_t new_event = {
            .id = EVN_FSM_EVENT_NEW,
            .p_data = &new_event_data,
        };
        evn_fsm_dispatch(evn, &new_event);
        break;
    }
    case EVENT_MANAGER_LISTENER_EVENT_ACK:
    {
        // Find this event_id in our FIFO, if it is there, remove it,
        // Check should we update EVN0ID as well
        evn_fsm_event_ack_t ack_event_data = {
            .ack_event_id = event_id,
        };
        fsm_event_t new_event = {
            .id = EVN_FSM_EVENT_NEW,
            .p_data = &ack_event_data,
        };
        evn_fsm_dispatch(evn, &new_event);
        break;
    }
    case EVENT_MANAGER_LISTENER_EVENT_DELETE_ALL:
    {
        fsm_event_t delete_all_event = {
            .id = EVN_FSM_EVENT_DEL_ALL,
        };
        evn_fsm_dispatch(evn, &delete_all_event);
        break;
    }
    case EVENT_MANAGER_LISTENER_EVENT_ACK_ALL:
        // Clear our FIFO
        // Set EVN0ID to zero
        // Set EVN0REC to "{}"
        {
            fsm_event_t ack_all_event = {
                .id = EVN_FSM_EVENT_ACK_ALL,
            };
            evn_fsm_dispatch(evn, &ack_all_event);
            break;
        }
    default:
        LOG(W, "Unknown event received from Event Manager: %u", event);
        break;
    }
}

/**
 * @brief       Inventory handler callback function.
 *
 * @param       arg During initialization inventory handled was setup to pass
 *              the pointer to event_notification_t structure instance.
 */
static void inventory_handler_cb(void *arg, uint32_t device_class_instance, bool is_available)
{
    if (DDM2_PARAMETER_CLASS_INSTANCE(EVM0AVL) == device_class_instance)
    {
        if (is_available)
        {
            int reg_status = event_manager_register_event_listener_cb(
                event_manager_event_listener_cb,
                arg);  // Pass the arg, which pointer to event_notification_t
            TRUE_CHECK(reg_status == EVENT_MANAGER_NO_ERROR);
        }
    }
}

/* -- Timer handling -- */

static void timer_callback(TimerHandle_t fsm_timer)
{
    event_notification_t *evn;
    fsm_event_t time_event;

    evn = pvTimerGetTimerID(fsm_timer);
    time_event.id = evn->fsm_timer_event;
    time_event.p_data = NULL;
    evn_fsm_dispatch(evn, &time_event);
}

static void evn_fsm_timer_init(event_notification_t *evn)
{
    memset(&evn->fsm_timer_buffer, 0, sizeof(evn->fsm_timer_buffer));
    memset(&evn->fsm_timer, 0, sizeof(evn->fsm_timer));
    evn->fsm_timer_event = FSM_UNDEFINED_EVENT;
    evn->fsm_timer = xTimerCreateStatic(
        "EVN after",
        pdMS_TO_TICKS(100),
        pdFALSE,
        evn,
        timer_callback,
        &evn->fsm_timer_buffer);
    TRUE_CHECK(evn->fsm_timer != NULL);
}

static void evn_fsm_timer_start_after(event_notification_t *evn, uint32_t after_ms, uint32_t fsm_event)
{
    evn->fsm_timer_event = fsm_event;

    BaseType_t status = xTimerChangePeriod(evn->fsm_timer, pdMS_TO_TICKS(after_ms), portMAX_DELAY);
    TRUE_CHECK(status == pdTRUE);
}

static void evn_fsm_timer_cancel(event_notification_t *evn)
{
    evn->fsm_timer_event = FSM_UNDEFINED_EVENT;
    BaseType_t status = xTimerStop(evn->fsm_timer, portMAX_DELAY);
    TRUE_CHECK(status == pdTRUE);
}

/* -- FSM states -- */

static void evn_fsm_state_idle(fsm_t *fsm, fsm_event_t const *event)
{
    event_notification_t *evn = evn_from_fsm(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        EVENT_NOTIFICATION_LOG(I, "Waiting for command...");
        // We need to publish EVN0ID set to zero
        ddm_entry_t *evn0id = ddm_store__access(&evn->owned_ddm_store, EVN0ID);
        bool has_changed = ddm_entry__set__value_i32(evn0id, 0);
        ddm_entry__set__has_changed(evn0id, has_changed);
        // Publish empty JSON in EVN0REC
        ddm_entry_t *evn0rec = ddm_store__access(&evn->owned_ddm_store, EVN0REC);
        has_changed = ddm_entry__set__value_str(evn0rec, "{}", sizeof("{}"));
        ddm_entry__set__has_changed(evn0rec, has_changed);
        // Publish "new" in EVN0ACK
        ddm_entry_t *evn0ack = ddm_store__access(&evn->owned_ddm_store, EVN0ACK);
        has_changed = ddm_entry__set__value_str(evn0ack, "", sizeof(""));
        ddm_entry__set__has_changed(evn0ack, has_changed);
        break;
    }
    case EVN_FSM_EVENT_NEW:
    {
        evn_fsm_event_new_t *new_data = event->p_data;
        event_type_t event_type;
        event_record_t event_record;
        bool is_pending = false;

        EVENT_NOTIFICATION_LOG(I, "New record: %d", new_data->new_event_id);
        int status = event_manager_find_record_by_id(new_data->new_event_id, &event_record);
        TRUE_CHECK_RETURN(status == EVENT_MANAGER_NO_ERROR);
        status = event_manager_find_type_by_id(event_record_get_type(&event_record), &event_type);
        if (status == EVENT_MANAGER_NO_ERROR)
        {
            if (event_type_is_cloud(&event_type))
            {
                char record[DDMP2_MAX_VALUE_SIZE];
                // Insert to FIFO
                fifo_fixed_enqueue_back(&evn->fifo_queue, &new_data->new_event_id);
                const char *device_id = event_manager_get_device_id();
                const char *product_name = event_manager_get_product_name();
                // Since this is the first event we got we can generate JSON.
                status = make_json_event_record(&event_record,
                                                product_name,
                                                device_id,
                                                record,
                                                sizeof(record));
                if (status == MAKE_JSON_NO_ERROR)
                {
                    // We need to publish EVN0ID
                    ddm_entry_t *evn0id = ddm_store__access(&evn->owned_ddm_store, EVN0ID);
                    bool has_changed = ddm_entry__set__value_i32(evn0id, new_data->new_event_id);
                    ddm_entry__set__has_changed(evn0id, has_changed);
                    // Publish JSON in EVN0REC
                    ddm_entry_t *evn0rec = ddm_store__access(&evn->owned_ddm_store, EVN0REC);
                    has_changed = ddm_entry__set__value_str(
                        evn0rec,
                        record,
                        strnlen(record, DDMP2_MAX_VALUE_SIZE));
                    ddm_entry__set__has_changed(evn0rec, has_changed);
                    // Publish "new" in EVN0ACK
                    ddm_entry_t *evn0ack = ddm_store__access(&evn->owned_ddm_store, EVN0ACK);
                    has_changed = ddm_entry__set__value_str(evn0ack, "new", strlen("new"));
                    ddm_entry__set__has_changed(evn0ack, has_changed);
                    is_pending = true;
                }
                else
                {
                    LOG(W, "Error in making JSON: %d", status);
                }
            }
            else
            {
                EVENT_NOTIFICATION_LOG(I, "Record %d is not cloud record", new_data->new_event_id);
            }
            event_type_terminate(&event_type);
        }
        event_record_terminate(&event_record);
        if (is_pending)
        {
            fsm_state_change(fsm, evn_fsm_state_pending);
        }
        break;
    }
    case EVN_FSM_EVENT_DEL_ALL:
    case EVN_FSM_EVENT_ACK_ALL:
    {
        fifo_fixed_delete_all(&evn->fifo_queue);
        fsm_state_change(fsm, evn_fsm_state_idle);
        break;
    }
    case FSM_EXIT_EVENT:
        break;
    default:
        break;
    }
}

static void evn_fsm_state_pending(fsm_t *fsm, fsm_event_t const *event)
{
    event_notification_t *evn = evn_from_fsm(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
        evn_fsm_timer_start_after(
            evn,
            CONNECTOR_EVENT_NOTIFICATION_CLOUD_WAIT_ACK_MS,
            EVN_FSM_EVENT_TIMEOUT);
        EVENT_NOTIFICATION_LOG(I, "Waiting for ACK");
        break;
    case FSM_EXIT_EVENT:
        evn_fsm_timer_cancel(evn);
        break;
    case EVN_FSM_EVENT_ACK_IN:
    {
        evn_fsm_event_ack_in_t *ack_in_data = event->p_data;
        if (strcmp(ack_in_data->ack_in_request, "acked") == 0)
        {
            event_id_t acked_event_id;

            fifo_fixed_dequeue(&evn->fifo_queue, &acked_event_id);
            EVENT_NOTIFICATION_LOG(I, "Got ACK for %d", acked_event_id);
            int status = event_manager_ack_record_by_id(acked_event_id);
            TRUE_CHECK(status == EVENT_MANAGER_NO_ERROR);
            fsm_state_change(fsm, evn_fsm_state_cooldown);
        }
        else
        {
            LOG(W, "Unexpected input in EVN0ACK: %s", ack_in_data->ack_in_request);
        }
        break;
    }
    case EVN_FSM_EVENT_NEW:
    {
        evn_fsm_event_new_t *new_data = event->p_data;
        event_type_t event_type;
        event_record_t event_record;

        EVENT_NOTIFICATION_LOG(I, "New record: %d", new_data->new_event_id);
        int status = event_manager_find_record_by_id(new_data->new_event_id, &event_record);
        TRUE_CHECK_RETURN(status == EVENT_MANAGER_NO_ERROR);
        status = event_manager_find_type_by_id(event_record_get_type(&event_record), &event_type);
        if (status == EVENT_MANAGER_NO_ERROR)
        {
            if (event_type_is_cloud(&event_type))
            {
                // Insert to FIFO
                fifo_fixed_enqueue_back(&evn->fifo_queue, &new_data->new_event_id);
            }
            else
            {
                EVENT_NOTIFICATION_LOG(I, "Record %d is not cloud record", new_data->new_event_id);
            }
            event_type_terminate(&event_type);
        }
        event_record_terminate(&event_record);
        break;
    }
    case EVN_FSM_EVENT_TIMEOUT:
    {
        event_id_t acked_event_id;
        EVENT_NOTIFICATION_LOG(I, "Acknowledge period elapsed.");
        fifo_fixed_dequeue(&evn->fifo_queue, &acked_event_id);
        fsm_state_change(fsm, evn_fsm_state_cooldown);
        break;
    }
    case EVN_FSM_EVENT_DEL_ALL:
    case EVN_FSM_EVENT_ACK_ALL:
    {
        fifo_fixed_delete_all(&evn->fifo_queue);
        fsm_state_change(fsm, evn_fsm_state_idle);
        break;
    }
    default:
        break;
    }
}

static void evn_fsm_state_cooldown(fsm_t *fsm, fsm_event_t const *event)
{
    event_notification_t *evn = evn_from_fsm(fsm);

    switch (event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        EVENT_NOTIFICATION_LOG(I, "Cooldown period...");
        evn_fsm_timer_start_after(
            evn,
            CONNECTOR_EVENT_NOTIFICATION_CLOUD_WAIT_SEND_MS,
            EVN_FSM_EVENT_TIMEOUT);
        // We need to publish EVN0ID set to zero
        ddm_entry_t *evn0id = ddm_store__access(&evn->owned_ddm_store, EVN0ID);
        bool has_changed = ddm_entry__set__value_i32(evn0id, 0);
        ddm_entry__set__has_changed(evn0id, has_changed);
        // Publish empty JSON in EVN0REC
        ddm_entry_t *evn0rec = ddm_store__access(&evn->owned_ddm_store, EVN0REC);
        has_changed = ddm_entry__set__value_str(evn0rec, "{}", sizeof("{}"));
        ddm_entry__set__has_changed(evn0rec, has_changed);
        // Publish "new" in EVN0ACK
        ddm_entry_t *evn0ack = ddm_store__access(&evn->owned_ddm_store, EVN0ACK);
        has_changed = ddm_entry__set__value_str(evn0ack, "", sizeof(""));
        ddm_entry__set__has_changed(evn0ack, has_changed);
        break;
    }
    case FSM_EXIT_EVENT:
        evn_fsm_timer_cancel(evn);
        break;
    case EVN_FSM_EVENT_NEW:
    {
        evn_fsm_event_new_t *new_data = event->p_data;
        event_type_t event_type;
        event_record_t event_record;

        EVENT_NOTIFICATION_LOG(I, "New record: %d", new_data->new_event_id);
        int status = event_manager_find_record_by_id(new_data->new_event_id, &event_record);
        TRUE_CHECK_RETURN(status == EVENT_MANAGER_NO_ERROR);
        status = event_manager_find_type_by_id(event_record_get_type(&event_record), &event_type);
        if (status == EVENT_MANAGER_NO_ERROR)
        {
            if (event_type_is_cloud(&event_type))
            {
                // Insert to FIFO
                fifo_fixed_enqueue_back(&evn->fifo_queue, &new_data->new_event_id);
            }
            else
            {
                EVENT_NOTIFICATION_LOG(I, "Record %d is not cloud record", new_data->new_event_id);
            }
            event_type_terminate(&event_type);
        }
        event_record_terminate(&event_record);
        break;
    }
    case EVN_FSM_EVENT_TIMEOUT:
    {
        if (fifo_fixed_is_empty(&evn->fifo_queue))
        {
            fsm_state_change(fsm, evn_fsm_state_idle);
        }
        else
        {
            event_record_t event_record;
            bool is_pending = false;

            event_id_t new_event_id = *(event_id_t *)fifo_fixed_peek(&evn->fifo_queue, 0);

            EVENT_NOTIFICATION_LOG(I, "Preparing new event record: %d", new_event_id);
            int status = event_manager_find_record_by_id(new_event_id, &event_record);
            if (status == EVENT_MANAGER_NO_ERROR)
            {
                char record[DDMP2_MAX_VALUE_SIZE];
                const char *device_id = event_manager_get_device_id();
                const char *product_name = event_manager_get_product_name();
                // Generate the JSON for this record
                status = make_json_event_record(&event_record,
                                                product_name,
                                                device_id,
                                                record,
                                                sizeof(record));
                if (status == MAKE_JSON_NO_ERROR)
                {
                    // We need to publish EVN0ID
                    ddm_entry_t *evn0id = ddm_store__access(&evn->owned_ddm_store, EVN0ID);
                    bool has_changed = ddm_entry__set__value_i32(evn0id, new_event_id);
                    ddm_entry__set__has_changed(evn0id, has_changed);
                    // Publish JSON in EVN0REC
                    ddm_entry_t *evn0rec = ddm_store__access(&evn->owned_ddm_store, EVN0REC);
                    has_changed = ddm_entry__set__value_str(
                        evn0rec,
                        record,
                        strnlen(record, DDMP2_MAX_VALUE_SIZE));
                    ddm_entry__set__has_changed(evn0rec, has_changed);
                    // Publish "new" in EVN0ACK
                    ddm_entry_t *evn0ack = ddm_store__access(&evn->owned_ddm_store, EVN0ACK);
                    has_changed = ddm_entry__set__value_str(evn0ack, "new", strlen("new"));
                    ddm_entry__set__has_changed(evn0ack, has_changed);
                    is_pending = true;
                }
                else
                {
                    LOG(W, "Error in making JSON: %d", status);
                }
                event_record_terminate(&event_record);
            }
            if (is_pending)
            {
                fsm_state_change(fsm, evn_fsm_state_pending);
            }
            else
            {
                fsm_state_change(fsm, evn_fsm_state_idle);
            }
        }
        break;
    }
    case EVN_FSM_EVENT_DEL_ALL:
    case EVN_FSM_EVENT_ACK_ALL:
    {
        fifo_fixed_delete_all(&evn->fifo_queue);
        fsm_state_change(fsm, evn_fsm_state_idle);
        break;
    }
    default:
        break;
    }
}

/* -- FSM handling -- */

static void evn_fsm_init(event_notification_t *evn)
{
    evn->fsm_mutex = xSemaphoreCreateMutexStatic(&evn->fsm_mutex_buffer);
    TRUE_CHECK(evn->fsm_mutex != NULL);
}

static void evn_fsm_dispatch(event_notification_t *evn, const fsm_event_t *fsm_event)
{
    BaseType_t status = xSemaphoreTake(evn->fsm_mutex, portMAX_DELAY);
    TRUE_CHECK_RETURN(status == pdTRUE);
    fsm_state_dispatch(&evn->fsm, fsm_event);
    status = xSemaphoreGive(evn->fsm_mutex);
    TRUE_CHECK(status == pdTRUE);
    /* After dispatching, the state machine most probably have made some
     * modifications to owned DDM parameters.
     */
    evn_broker_publish_changed(evn);
}

/* -- Event notification functions -- */

/**
 * @brief       Initialize event_notification connector context structure.
 *
 * @param       evn is a pointer to event_notification context structure that
 *              needs to be initialized.
 * @param       connector is a pointer to connector handle structure.
 * @param       p_inventory_handler_storage is a pointer to sorted list needed for
 *              inventory operation.
 * @param       p_owned_ddm_store_storage Pointer to SORTED_CONTAINER which will
 *              be used to store instaces if ddm_entry structures for DDM
 *              parameters which are owned by this connector.
 * @param       p_other_ddm_store_storage Pointer to SORTED_CONTAINER which will
 *              be used to store instaces if ddm_entry structures for DDM
 *              parameters which are __NOT__ owned by this connector.
 * @param       fifo_storage is a pointer to an array containing event_id_t
 *              eleements.
 * @param       fifo_storage_size is number of elements in allocated
 *              @a fifo_starage array.
 */
static void event_notification_init(
    event_notification_t *evn,
    CONNECTOR *connector,
    SORTED_LIST *p_inventory_handler_storage,
    SORTED_CONTAINER *p_owned_ddm_store_storage,
    event_id_t *fifo_storage,
    size_t fifo_storage_size)
{
    /* Initialize members */
    evn->connector = connector;
    /* Construct inventory handler */
    inventory_handler_init(
        &evn->inventory_handler,
        p_inventory_handler_storage,
        inventory_handler_cb,
        evn);
    /* Construct owned DDM store */
    ddm_store__init(&evn->owned_ddm_store, p_owned_ddm_store_storage);
    /* Initialize fsm_timer */
    evn_fsm_timer_init(evn);
    /* Initialize fsm */
    evn_fsm_init(evn);
    /* Initialize FIFO queue */
    fifo_fixed_init(&evn->fifo_queue, fifo_storage, sizeof(*fifo_storage), fifo_storage_size);
}

/**
 * @brief       Initialized owned DDM parameters.
 *
 * Create memory for owned DDM parameters and initialize with values.
 *
 * @param       evn Pointer to event_notification_t context structure.
 * @param       owned_ddm_values Pointer to array containing initial DDM
 *              parameter values.
 * @param       owned_ddm_values_size Size of array pointer by
 *              @a owned_ddm_value in number of elements.
 * @param       ddm_class_instance that was returned during registration with
 *              Broker.
 */
static void event_notification_init_owned_ddm(
    event_notification_t *evn,
    const ddm_store_ddm_t *owned_ddm_values,
    size_t owned_ddm_values_size,
    int32_t ddm_class_instance)
{
    ddm_store__load_entries(
        &evn->owned_ddm_store,
        owned_ddm_values,
        owned_ddm_values_size,
        (uint32_t)ddm_class_instance);
    for (size_t i = 0u; i < ddm_store__occupied(&evn->owned_ddm_store); i++)
    {
        ddm_entry_t *entry;

        ddm_store__iterate(&evn->owned_ddm_store, i, &entry);
        /* When DDM entries are created by DDM store it will automatically set
         * has_changed flag as well and we don't want that.
         */
        ddm_entry__set__has_changed(entry, false);
    }
}

/**
 * @brief       Start the operation of connector after initialization.
 *
 * @param       evn Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_notification_start(event_notification_t *evn)
{
    int status = inventory_handler_add(&evn->inventory_handler, EVM0AVL);
    status |= inventory_handler_start(&evn->inventory_handler, evn->connector);
    fsm_initialize(&evn->fsm, evn_fsm_state_idle);

    return status == 0 ? OPS_SUCCESS : OPS_FAILURE;
}

/* -- Connector functions -- */

static int connector_event_notification_init(void)
{
    event_notification_init(
        &event_notification,
        &connector_event_notification,
        &event_notification_inventory_handler_storage,
        &event_notification_owned_ddm_store_storage,
        &event_notification_fifo_storage[0],
        ELEMENTS(event_notification_fifo_storage));
    int32_t evn_ddm_class_instance = handle_owned_parameter_register(EVN0, &connector_event_notification);
    if (evn_ddm_class_instance == -1)
    {
        LOG(E, "%s: failed to register EVN0 class, terminating!", connector_event_notification.name);
        return CONNECTOR_INIT_FAILURE;
    }
    event_notification_init_owned_ddm(
        &event_notification,
        &owned_evn_ddm_values[0],
        ELEMENTS(owned_evn_ddm_values),
        evn_ddm_class_instance);
    int start_status = event_notification_start(&event_notification);
    if (start_status != OPS_SUCCESS)
    {
        return CONNECTOR_INIT_FAILURE;
    }
    return CONNECTOR_INIT_SUCCESS;
}

static void connector_event_notification_process_task(const DDMP2_FRAME *frame)
{
    /* Process only if frame is inventory update frame about the classes that we
     * are interested in.
     */
    int inventory_status = inventory_handler_update(&event_notification.inventory_handler, frame);
    if (inventory_status == 0)
    {
        /* Process data frames.
         *
         * This frame was __NOT__ inventory update frame and it was not
         * processed by inventory handler.
         *
         * The SET and SUBSCRIBE frames are associated with owned parameters, while
         * the PUBLISH frames are associated with other parameters.
         */
        switch (frame->frame.control)
        {
        case DDMP2_CONTROL_SET:
            handle_owned_parameter_set(&event_notification, frame);
            break;
        case DDMP2_CONTROL_SUBSCRIBE:
            handle_owned_parameter_subscribe(&event_notification, frame);
            break;
        case DDMP2_CONTROL_NOP:
        case DDMP2_CONTROL_FRAGMENT:
        case DDMP2_CONTROL_MESSAGE:
        case DDMP2_CONTROL_REG:
        case DDMP2_CONTROL_COUNT:
        default:
            LOG(W, "%s: unhandled frame %u",
                event_notification.connector->name,
                frame->frame.control);
            break;
        }
    }
}

CONNECTOR connector_event_notification = {
    .name = "Event Notification connector",
    .initialize = connector_event_notification_init,
    .process_event = connector_event_notification_process_task,
};
