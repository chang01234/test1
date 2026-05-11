/**
 * \file
 * \date        2021-10-08
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Soft simulator implementation.
 *
 * This connector simulates a connector. For details refer to header file.
 *
 * \li          2021-10-08  (NR) Initial implementation
 * \li          2022-07-08  (NR) Refactor of software simulator, add generators and actions
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

#include "configuration.h"

#include "broker.h"
#include "ddm_entry.h"
#include "ddm_store.h"
#include "freertos/FreeRTOS.h"
#include "hal_cpu.h"
#include "software_simulator.h"

static void generators_init(struct software_simulator__generator_exec *sa)
{
    sa->p__t_prev = 0u;
    sa->p__current_cycle = 0u;
}

static bool generators_should_execute(const struct software_simulator__generator_exec *sa)
{
    uint32_t t_now;
    uint32_t t_elapsed;

    t_now = hal_cpu_get_millis();
    t_elapsed = sa->p__t_prev - t_now;

    if (t_elapsed >= SOFTWARE_SIMULATOR__GENERATORS_EXECUTION_PERIOD_MS)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void generators_mark_cycle(struct software_simulator__generator_exec *ae)
{
    ae->p__t_prev = hal_cpu_get_millis();
    ae->p__current_cycle++;
}

static void handle_generators_exec(SOFTWARE_SIMULATOR *context)
{
    for (size_t i = 0u; i < context->p__description->generators_size; i++)
    {
        ddm_entry_t *ddm_entry;
        const SOFTWARE_SIMULATOR__GENERATOR_ENTRY *entry;

        entry = &context->p__description->generators[i];
        ddm_entry = ddm_store__access(context->p__ddm_owned_store, entry->ddm_parameter);
        if (ddm_entry != NULL)
        {
            entry->handler(context, ddm_entry, entry->parameters, context->p__description->args);
        }
        else
        {
            /* A DDM that does not exist in our owned DDM store or it was deleted from DDM store */
            LOG(W, "action for non-existing DDM 0x%x", entry->ddm_parameter);
        }
    }
}

/*
 * This function will:
 * - Get the DDM from DDM store
 * - Store the value into DDM according to DDM in_type
 */
static void handle_owned_parameter_set(SOFTWARE_SIMULATOR *context,
                                       const DDMP2_FRAME *ddmp2_frame)
{
    ddm_entry_t *ddm_entry;
    bool has_changed;

    ddm_entry = ddm_store__access(context->p__ddm_owned_store, ddmp2_frame->frame.set.parameter);

    if (ddm_entry == NULL)
    {
        /* Somebody tried to set a value to something that we haven't created */
        LOG(W, "%s(%s): set non-existing DDM: 0x%x",
            context->p__connector->name,
            context->p__description->name,
            ddmp2_frame->frame.set.parameter);
        return;
    }
    has_changed = ddm_entry__write__value(
        ddm_entry,
        &ddmp2_frame->frame.set.value,
        ddmp2_value_size(ddmp2_frame));
    ddm_entry__set__has_changed_conditionally(ddm_entry, has_changed);
}

/*
 * This function will:
 * - check if a parameter may be subscribed to
 * - set that it is subscribed
 * - set that is has changed so it can be immediately published
 */
static void handle_owned_parameter_subscribe(SOFTWARE_SIMULATOR *context,
                                             const DDMP2_FRAME *ddmp2_frame)
{
    ddm_entry_t *ddm_entry;

    ddm_entry = ddm_store__access(
        context->p__ddm_owned_store,
        ddmp2_frame->frame.subscribe.parameter);

    if (ddm_entry == NULL)
    {
        /* Somebody tried to subscribe to something that we haven't created */
        LOG(W,
            "%s(%s): subscribe to non-existing DDM: 0x%x",
            context->p__connector->name,
            context->p__description->name,
            ddmp2_frame->frame.set.parameter);
        return;
    }
    /* See if it has some output type? */
    if (ddm_entry__out_type(ddm_entry) == DDM2_TYPE_NONE)
    {
        /* This parameter has no output type. */
        LOG(W,
            "%s(%s): denied subscription to DDM: 0x%x (no output type)",
            context->p__connector->name,
            context->p__description->name,
            ddmp2_frame->frame.set.parameter);
        return;
    }
    ddm_entry__set__is_subscribed(ddm_entry, true);
    ddm_entry__set__has_changed(ddm_entry, true); /* Force publish of the paramter */
}

/*
 * 1. Go through all stored DDM paramters
 * 2. For each DDM parameter that has changed and is subscribed:
 *    a) reset the publish flag
 *    b) publish its value
 */
static void handle_owned_parameter_publish(SOFTWARE_SIMULATOR *context)
{
    size_t occupied;

    occupied = ddm_store__occupied(context->p__ddm_owned_store);

    for (size_t i = 0; i < occupied; i++)
    {
        ddm_entry_t *ddm_entry;

        ddm_store__iterate(context->p__ddm_owned_store, i, &ddm_entry);

        if (ddm_entry__has_changed(ddm_entry) && ddm_entry__is_subscribed(ddm_entry))
        {
            ddm_entry__set__has_changed(ddm_entry, false); /* Reset the flag */
            software_simulator__broker_publish(context, ddm_entry);
        }
    }
}

static void handle_other_parameter_set(SOFTWARE_SIMULATOR *context)
{
    size_t occupied;

    occupied = ddm_store__occupied(context->p__ddm_other_store);

    for (size_t i = 0; i < occupied; i++)
    {
        ddm_entry_t *ddm_entry;

        ddm_store__iterate(context->p__ddm_other_store, i, &ddm_entry);

        if (ddm_entry__flags(ddm_entry) & SOFTWARE_SIMULATOR__BROKER_SET)
        {
            ddm_entry__set__flags(ddm_entry, 0); /* Reset the flag */
            software_simulator__broker_set(context, ddm_entry);
        }
    }
}

static void handle_other_parameter_publish(SOFTWARE_SIMULATOR *context,
                                           const DDMP2_FRAME *ddmp2_frame)
{
    ddm_entry_t *ddm_entry;
    bool has_changed;

    ddm_entry = ddm_store__access(
        context->p__ddm_other_store,
        ddmp2_frame->frame.publish.parameter);

    if (ddm_entry == NULL)
    {
        /* Somebody tried to publish to us something that we haven't been subscribed to */
        LOG(W,
            "%s(%s): publish of non-existing DDM: 0x%x",
            context->p__connector->name,
            context->p__description->name,
            ddmp2_frame->frame.set.parameter);
        return;
    }
    has_changed = ddm_entry__set__value(
        ddm_entry,
        &ddmp2_frame->frame.publish.value,
        ddmp2_value_size(ddmp2_frame));
    ddm_entry__set__has_changed_conditionally(ddm_entry, has_changed);
}

static void handle_other_parameter_reset_has_changed(SOFTWARE_SIMULATOR *context)
{
    size_t occupied;

    occupied = ddm_store__occupied(context->p__ddm_other_store);

    for (size_t i = 0; i < occupied; i++)
    {
        ddm_entry_t *ddm_entry;

        ddm_store__iterate(context->p__ddm_other_store, i, &ddm_entry);
        ddm_entry__set__has_changed(ddm_entry, false);
    }
}

static void handle_other_parameter_subscribe(SOFTWARE_SIMULATOR *context)
{
    size_t occupied;

    occupied = ddm_store__occupied(context->p__ddm_other_store);

    for (size_t i = 0; i < occupied; i++)
    {
        DDMP2_FRAME *frame;
        ddm_entry_t *ddm_entry;
        size_t frame_size = 0;
        ddm_store__iterate(context->p__ddm_other_store, i, &ddm_entry);

        LOG(I,
            "%s(%s): subscribing to %s0%s(0x%x)",
            context->p__connector->name,
            context->p__description->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
            ddm_entry__parameter_id(ddm_entry));
        /* subscribe to parameters */
        software_simulator__broker_subscribe(context, ddm_entry);
        /* wait up until 1s before continuing */
        frame = xRingbufferReceive(
            context->p__connector->to_connector,
            &frame_size,
            pdMS_TO_TICKS(1000));
        if (frame != NULL)
        {
            switch (frame->frame.control)
            {
            case DDMP2_CONTROL_PUBLISH:
                handle_other_parameter_publish(context, frame);
                break;
            case DDMP2_CONTROL_SET:
            case DDMP2_CONTROL_SUBSCRIBE:
            case DDMP2_CONTROL_NOP:
            case DDMP2_CONTROL_FRAGMENT:
            case DDMP2_CONTROL_MESSAGE:
            case DDMP2_CONTROL_REG:
            case DDMP2_CONTROL_COUNT:
                LOG(W, "%s(%s): unhandled frame %u during subcription",
                    context->p__connector->name,
                    context->p__description->name,
                    frame->frame.control);
                break;
            }
            vRingbufferReturnItem(context->p__connector->to_connector, frame);
        }
        else
        {
            LOG(W,
                "%s(%s): timeout during subscribe of %s0%s(0x%x)",
                context->p__connector->name,
                context->p__description->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                ddm_entry__parameter_id(ddm_entry));
        }
    }
}

static ddm_entry_t *find_ddm_in_any_store(SOFTWARE_SIMULATOR *context, uint32_t ddm_parameter)
{
    ddm_entry_t *ddm_entry;

    // Check if this parameter is part of base class I am simulating. Then consider the instance
    if (DDM2_PARAMETER_CLASS(ddm_parameter) == context->p__description->ddm_class)
    {
        ddm_parameter = DDM2_PARAMETER_BASE_INSTANCE(ddm_parameter) | DDM2_PARAMETER_INSTANCE(context->instance);
    }
    ddm_entry = context->p__ddm_owned_store != NULL ? ddm_store__access(context->p__ddm_owned_store, ddm_parameter) : NULL;
    if (ddm_entry == NULL)
    {
        ddm_entry = context->p__ddm_other_store != NULL ? ddm_store__access(context->p__ddm_other_store, ddm_parameter) : NULL;
    }
    return ddm_entry;
}

static int evaluate_any_trigger(SOFTWARE_SIMULATOR *context, uint32_t entry_i, bool *has_changed)
{
    const SOFTWARE_SIMULATOR__ACTION_ENTRY *entry;

    *has_changed = false;
    entry = &context->p__description->actions[entry_i];

    for (uint32_t trigger_i = 0u; trigger_i < SOFTWARE_SIMULATOR__ACTION_MAX_TRIGGERS; trigger_i++)
    {
        uint32_t ddm_parameter;
        ddm_entry_t *ddm_entry;

        ddm_parameter = entry->trigger.any[trigger_i];

        if (ddm_parameter == 0u)
        {
            /* We found unpopulated entry which tells us that we are at end of list, exit */
            break;
        }
        ddm_entry = find_ddm_in_any_store(context, ddm_parameter);
        if (ddm_entry == NULL)
        {
            return SOFTWARE_SIMULATOR__OPS_FAILURE;
        }
        if (ddm_entry__has_changed(ddm_entry))
        {
            *has_changed = true;
            break;
        }
    }
    return SOFTWARE_SIMULATOR__OPS_SUCCESS;
}

static void handle_actions_exec(SOFTWARE_SIMULATOR *context)
{
    for (size_t entry_i = 0u; entry_i < context->p__description->actions_size; entry_i++)
    {
        bool any_has_changed;
        int retval;
        const SOFTWARE_SIMULATOR__ACTION_ENTRY *entry;

        entry = &context->p__description->actions[entry_i];

        if (entry->trigger.any[0] == 0)
        {
            LOG(W,
                "%s(%s): action %u, trigger is empty",
                context->p__connector->name,
                context->p__description->name,
                entry_i);
            break;
        }
        /* Check for `any` trigger */
        retval = evaluate_any_trigger(context, entry_i, &any_has_changed);
        if (retval == SOFTWARE_SIMULATOR__OPS_FAILURE)
        {
            LOG(W,
                "%s(%s): action %u, trigger `any` contains non-owned and non-subscribed DDM",
                context->p__connector->name,
                context->p__description->name,
                entry_i);
            break;
        }
        if (any_has_changed)
        {
            if (entry->ddm_parameter != 0u)
            {
                bool has_changed;
                ddm_entry_t *ddm_entry;

                ddm_entry = NULL;
                if (context->p__ddm_owned_store != NULL)
                {
                    uint32_t ddm_parameter = entry->ddm_parameter;
                    // Check if this parameter is part of base class I am simulating. Then consider the instance
                    if (DDM2_PARAMETER_CLASS(ddm_parameter) == context->p__description->ddm_class)
                    {
                        ddm_parameter = DDM2_PARAMETER_BASE_INSTANCE(ddm_parameter) | DDM2_PARAMETER_INSTANCE(context->instance);
                    }

                    ddm_entry = ddm_store__access(context->p__ddm_owned_store, ddm_parameter);
                }
                if ((ddm_entry == NULL) && (context->p__ddm_other_store != NULL))
                {
                    ddm_entry = ddm_store__access(context->p__ddm_other_store, entry->ddm_parameter);
                }
                if (ddm_entry == NULL)
                {
                    LOG(W,
                        "%s(%s): action %u, destination contains non mapped DDM",
                        context->p__connector->name,
                        context->p__description->name,
                        entry_i);
                    break;
                }
                has_changed = entry->handler(context, ddm_entry, context->p__description->args);
                ddm_entry__set__has_changed_conditionally(ddm_entry, has_changed);
            }
            else
            {
                entry->handler(context, NULL, context->p__description->args);
            }
        }
    }
}

static void worker(void *arg)
{
    SOFTWARE_SIMULATOR *software_simulator = arg;

    generators_init(&software_simulator->p__generator_exec);

    if (software_simulator->p__ddm_owned_store != NULL)
    {
        uint32_t ddm_class = software_simulator->p__description->ddm_class;

        int32_t instance = broker_register_instance(&ddm_class, software_simulator->p__connector->connector_id);
        if (instance == -1)
        {
            LOG(E,
                "%s(%s): failed to register ownership",
                software_simulator->p__connector->name,
                software_simulator->p__description->name);
            vTaskSuspend(NULL);
        }
        software_simulator->instance = instance;
        ddm_store__load_entries(
            software_simulator->p__ddm_owned_store,
            software_simulator->p__description->ddm_owned_initial_values,
            software_simulator->p__description->ddm_owned_initial_values_size,
            instance);
    }

    if (software_simulator->p__ddm_other_store != NULL)
    {
        ddm_store__load_entries(
            software_simulator->p__ddm_other_store,
            software_simulator->p__description->ddm_other_initial_values,
            software_simulator->p__description->ddm_other_initial_values_size,
            0);
        handle_other_parameter_subscribe(software_simulator);
    }
    handle_actions_exec(software_simulator); /* Initial execution of actions */

    for (;;)
    {
        DDMP2_FRAME *frame;
        size_t frame_size = 0;

        frame = xRingbufferReceive(
            software_simulator->p__connector->to_connector,
            &frame_size,
            pdMS_TO_TICKS(SOFTWARE_SIMULATOR__GENERATORS_EXECUTION_PERIOD_MS));
        if (frame != NULL)
        {
            bool continue_loop = true;
            if (software_simulator->hook_function != NULL)
            {
                continue_loop = software_simulator->hook_function(frame);
            }
            if (continue_loop)
            {
                switch (frame->frame.control)
                {
                case DDMP2_CONTROL_SET:
                    if (software_simulator->p__ddm_owned_store != NULL)
                    {
                        handle_owned_parameter_set(software_simulator, frame);
                        handle_actions_exec(software_simulator);
                    }
                    break;
                case DDMP2_CONTROL_SUBSCRIBE:
                    if (software_simulator->p__ddm_owned_store != NULL)
                    {
                        handle_owned_parameter_subscribe(software_simulator, frame);
                    }
                    break;
                case DDMP2_CONTROL_PUBLISH:
                    if (software_simulator->p__ddm_other_store != NULL)
                    {
                        handle_other_parameter_publish(software_simulator, frame);
                        handle_actions_exec(software_simulator);
                    }
                    break;
                case DDMP2_CONTROL_NOP:
                case DDMP2_CONTROL_FRAGMENT:
                case DDMP2_CONTROL_MESSAGE:
                case DDMP2_CONTROL_REG:
                case DDMP2_CONTROL_COUNT:
                default:
                    LOG(W, "%s(%s): unhandled frame %u",
                        software_simulator->p__connector->name,
                        software_simulator->p__description->name,
                        frame->frame.control);
                    break;
                }
            }
            vRingbufferReturnItem(software_simulator->p__connector->to_connector, frame);
        }
        /* NOTE:
         * Execute actions before publishing is a must for actions to operate correctly since
         * actions depend on `has_changed` property
         */
        if (generators_should_execute(&software_simulator->p__generator_exec))
        {
            generators_mark_cycle(&software_simulator->p__generator_exec);
            handle_generators_exec(software_simulator);
        }
        if (software_simulator->p__ddm_owned_store != NULL)
        {
            handle_owned_parameter_publish(software_simulator);
        }
        if (software_simulator->p__ddm_other_store != NULL)
        {
            /* Set parameters modifed by generators, reset flags after setting */
            handle_other_parameter_set(software_simulator);
            handle_other_parameter_reset_has_changed(software_simulator);
        }
    }
}

int software_simulator__init(
    SOFTWARE_SIMULATOR *software_simulator,
    CONNECTOR *connector,
    const SOFTWARE_SIMULATOR__DESCRIPTOR *description)
{
    BaseType_t error;

    LOG(I, "initializing %s with configuration: %s", connector->name, description->name);
    software_simulator->p__description = description;
    software_simulator->p__ddm_owned_store = description->ddm_owned_store;
    software_simulator->p__ddm_other_store = description->ddm_other_store;
    software_simulator->p__connector = connector;
    software_simulator->instance = 0;
    software_simulator->hook_function = description->hook_function;
    LOG(I,
        "owning %u and subscribing to %u DDM parameters, %u generator(s) , %u action(s)",
        software_simulator->p__description->ddm_owned_initial_values_size,
        software_simulator->p__description->ddm_other_initial_values_size,
        software_simulator->p__description->generators_size,
        software_simulator->p__description->actions_size);
    error = xTaskCreate(
        /* code */ worker,
        /* name */ software_simulator->p__connector->name,
        /* stack size */ description->worker_stack_size,
        /* parameters */ software_simulator,
        /* priority */ description->worker_priority,
        /* handle */ NULL);

    return error == pdPASS ? CONNECTOR_INIT_SUCCESS : CONNECTOR_INIT_FAILURE;
}

int software_simulator__broker_publish(const SOFTWARE_SIMULATOR *cw, const ddm_entry_t *entry)
{
    const void *data;
    size_t data_size;

    ddm_entry__read__value(entry, &data, &data_size);

    return connector_send_frame_to_broker(
               /* control */ DDMP2_CONTROL_PUBLISH,
               /* parameter */ ddm_entry__parameter_id(entry),
               /* value */ data,
               /* value_size */ data_size,
               /* connector */ cw->p__connector->connector_id,
               /* timeout */ portMAX_DELAY) == true
               ? SOFTWARE_SIMULATOR__OPS_SUCCESS
               : SOFTWARE_SIMULATOR__OPS_FAILURE;
}

int software_simulator__broker_set(const SOFTWARE_SIMULATOR *cw, const ddm_entry_t *entry)
{
    const void *data;
    size_t data_size;

    ddm_entry__read__value(entry, &data, &data_size);

    return connector_send_frame_to_broker(
               /* control */ DDMP2_CONTROL_SET,
               /* parameter */ ddm_entry__parameter_id(entry),
               /* value */ data,
               /* value_size */ data_size,
               /* connector */ cw->p__connector->connector_id,
               /* timeout */ portMAX_DELAY) == true
               ? SOFTWARE_SIMULATOR__OPS_SUCCESS
               : SOFTWARE_SIMULATOR__OPS_FAILURE;
}

int software_simulator__broker_subscribe(const SOFTWARE_SIMULATOR *cw, const ddm_entry_t *entry)
{
    return connector_send_frame_to_broker(
               /* control */ DDMP2_CONTROL_SUBSCRIBE,
               /* parameter */ ddm_entry__parameter_id(entry),
               /* value */ NULL,
               /* value_size */ 0,
               /* connector */ cw->p__connector->connector_id,
               /* timeout */ portMAX_DELAY) == true
               ? SOFTWARE_SIMULATOR__OPS_SUCCESS
               : SOFTWARE_SIMULATOR__OPS_FAILURE;
}
