/**
 * \file	ddm_log.c
 * \brief	DDM Log implementation.
 *
 *
 * \{
 */
#include "configuration.h"

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "ddm_log.h"
#include "ddm2.h"
#include "connector_system.h"
#include "inventory_handler.h"
#include "sorted_list.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "utils.h"
#include "ddm_log_query_parser.h"
#include "ddm_log_spec_comp.h"
#include "ddm_log_spec_parser.h"
#include "ddm_log_event.h"
#include "ddm_log_event_flash.h"
#include "ddm_store.h"
#include "ddm_entry.h"
#include "broker.h"
#include "hal_mem.h"
#include "load_configurations_ddm_log.h"

#if !defined(LOAD_CONFIGURATION_USE_JSON_CJSON) && !defined(LOAD_CONFIGURATION_USE_JSON_JSMN)
// default to CJSON
#define LOAD_CONFIGURATION_USE_JSON_CJSON
#endif
#if defined(LOAD_CONFIGURATION_USE_JSON_CJSON) && defined(LOAD_CONFIGURATION_USE_JSON_JSMN)
#error Cannot have both LOAD_CONFIGURATION_USE_JSON_CJSON and LOAD_CONFIGURATION_USE_JSON_JSMN defined
#endif
#ifdef LOAD_CONFIGURATION_USE_JSON_CJSON
#include "cJSON.h"
#else
#include "jsmn.h"
#endif

#define LOG_EVENT                                           (0x5a)
#define SAMPLE_EVENT                                        (0x5b)

#define INVENTORY_HANDLER_STORAGE_SIZE                      32


/* Private */
#ifdef LOAD_CONFIGURATION_USE_JSON_JSMN
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if ((tok->type == JSMN_STRING) && ((int)strlen(s) == tok->end - tok->start) &&
        (strncmp(json + tok->start, s, tok->end - tok->start) == 0))
    {
        return 0;
    }
    return -1;
}
#endif
static void ddm_log_interval_timer_callback(const TimerHandle_t xTimer);
static void ddm_sample_interval_timer_callback(const TimerHandle_t xTimer);
static void inventory_handler_cb(void *arg, uint32_t device_class_instance, bool is_available);
static bool do_condition_comparison(const ddm_log_specification_data_t * const p_data);
static int check_hysteresis(ddm_log_specification_data_t * const p_log_spec_data);
static void increment_log_read_data(const ddm_log_specification_data_t *p_log_data);
static void publish_log_read_data_update(ddm_log_read_specification_data_t *p_data);
static struct ddm_entry *add_subscription_to_store(const struct ddm_log *ddm_log, const uint32_t parameter);
static void add_subscription(const struct ddm_log * p_ddm_log, const uint32_t parameter);
static int log_specification_add_new(const struct ddm_log *p_ddm_log, const ddm_log_specification_t *p_log_spec);
static int log_specification_set_activate(const struct ddm_log *p_ddm_log, const struct ddm_entry *entry, uint32_t instance);
static int get_stored_data(ddm_log_data_t *p_log_data, int32_t *sample_data);
static int handle_set(struct ddm_log * context, const DDMP2_FRAME * ddmp2_frame);
static int handle_subscribe(const struct ddm_log * context, const DDMP2_FRAME * ddmp2_frame);
static void handle_publish(const struct ddm_log * context, const DDMP2_FRAME *p_ddmp2_frame);
static int handle_generic(const struct ddm_log * context, const DDMP2_FRAME *p_ddmp2_frame);
static void worker_task(void * arg);

//static void extract_entry_to_str(const struct ddm_entry *entry, char* text, int capacity);
//static int log_specification_delete(const struct ddm_log * ddm_log, uint8_t index);


static nvs_handle_t nvs_logcfgs;

static uint32_t max_storage_size;
static size_t log_specification_count = 0;
static size_t log_read_specification_count = 0;

static EXT_RAM_ATTR ddm_log_specification_data_t log_specifications[DDM_LOG__SPEC_MAX];
static EXT_RAM_ATTR ddm_log_read_specification_data_t log_read_specifications[DDM_LOG__SPEC_MAX];

static struct ddm_log *l_my_ddm_log;
static inventory_handler_t l_inventory_handle;		/**< Internal inventory handles, implemented inventory management */

static EXT_RAM_ATTR DDMP2_FRAME l_ddmp_frame;       // Used to send bin data

DECLARE_SORTED_LIST_EXTRAM(inventory_list, INVENTORY_HANDLER_STORAGE_SIZE);

static void ddm_log_interval_timer_callback(const TimerHandle_t xTimer)
{
    // Time to generate a log event
    uint32_t log_spec_index = (uint32_t)pvTimerGetTimerID(xTimer);
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, LOG_EVENT, &log_spec_index, sizeof(log_spec_index), l_my_ddm_log->p__description->connector->connector_id, portMAX_DELAY);
}

static void ddm_sample_interval_timer_callback(const TimerHandle_t xTimer)
{
    // Time to sample data
    uint32_t log_spec_index = (uint32_t)pvTimerGetTimerID(xTimer);
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, SAMPLE_EVENT, &log_spec_index, sizeof(log_spec_index), l_my_ddm_log->p__description->connector->connector_id, portMAX_DELAY);
}

static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available)
{
    //LOG(P, "Inventory handler callback: %08x %s", device_class_instance, is_available ? "available" : "unavailable");

    struct ddm_log *p_ddm_log = (struct ddm_log *)argument;
    if (is_available)
    {
        size_t num_index = ddm_store__occupied(p_ddm_log->p__ddm_store_subscribed);
        for (size_t s = 0; s < num_index; s++)
        {
            struct ddm_entry * entry;

            ddm_store__iterate(p_ddm_log->p__ddm_store_subscribed, (int)s, &entry);

            if (DDMP2_INVENTORY_CLASS_INSTANCE(ddm_entry__parameter_id(entry)) == device_class_instance)
            {
                connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, ddm_entry__parameter_id(entry), NULL, 0, p_ddm_log->p__description->connector->connector_id, portMAX_DELAY);
            }
        }
    }
}

static bool do_condition_comparison(const ddm_log_specification_data_t * const p_data)
{
    bool compare_value = true;
    // Loop all conditions. Implicit AND between separate conditions
    for (int i = 0; i < p_data->num_conditions; i++)
    {
        if (p_data->conditions[i].comparison == COMPARISON_NONE)
        {
            compare_value &= true;  // Unconditional
            continue;
        }
        int32_t lhs;

        if (p_data->conditions[i].lhs.parameter == DDMP2_INVALID_PARAMETER) // Get value from literal
        {
            lhs = p_data->conditions[i].lhs.literal;
        }
        else                                                            // Get value from store
        {
            lhs = 0;
            if (get_stored_data((ddm_log_data_t *)&(p_data->conditions[i].lhs), &lhs))
            {
                LOG(W, "No available data for lhs 0x%08x", p_data->conditions[i].lhs.parameter);
                return 0;
            }
        }
        int32_t rhs;

        if (p_data->conditions[i].rhs.parameter == DDMP2_INVALID_PARAMETER)
        {
            rhs = p_data->conditions[i].rhs.literal;
        }
        else
        {
            rhs = 0;
            if (get_stored_data((ddm_log_data_t *)&(p_data->conditions[i].rhs), &rhs))
            {
                LOG(W, "No available data for rhs 0x%08x", p_data->conditions[i].rhs.parameter);
                return 0;
            }
        }

        bool part_result = false;
        switch (p_data->conditions[i].comparison)
        {
        case COMPARISON_EQ:
            part_result = (lhs == rhs);
            LOG(D, "COMPARISON_EQ: %d", part_result);
            break;
        case COMPARISON_NEQ:
            part_result = (lhs != rhs);
            LOG(D, "COMPARISON_NEQ: %d", part_result);
            break;
        case COMPARISON_LT:
            part_result = (lhs < rhs);
            LOG(D, "COMPARISON_LT: %d", part_result);
            break;
        case COMPARISON_LTEQ:
            part_result = (lhs <= rhs);
            LOG(D, "COMPARISON_LTEQ: %d", part_result);
            break;
        case COMPARISON_GT:
            part_result = (lhs > rhs);
            LOG(D, "COMPARISON_GT: %d", part_result);
            break;
        case COMPARISON_GTEQ:
            part_result = (lhs >= rhs);
            LOG(D, "COMPARISON_GTEQ: %d", part_result);
            break;
        default:
            LOG(W, "Unknown comparison");
            break;
        }
        compare_value &= part_result;
    }

    return compare_value;
}

static int check_hysteresis(ddm_log_specification_data_t * const p_log_spec_data)
{
    int ret_val = 1;
    int64_t new_data = p_log_spec_data->accumulator / p_log_spec_data->sample_counter;
    if (((new_data - p_log_spec_data->prev_data) >= p_log_spec_data->hysteresis.upper) ||
       ((new_data - p_log_spec_data->prev_data) <= -p_log_spec_data->hysteresis.lower))
    {
        ret_val = 1;
    }
    else
    {
        ret_val = 0;
    }
    return ret_val;
}


static void increment_log_read_data(const ddm_log_specification_data_t *p_log_data)
{
    ddm_log_read_specification_data_t *p_data = NULL;
    // Check all log read configs
    for (size_t i = 0; i < log_read_specification_count; i++)
    {
        if ((intptr_t)p_log_data == (intptr_t)log_read_specifications[i].p_logconfig)
        {
            p_data = &log_read_specifications[i];

            // Valid data?
            if (p_log_data->sample_counter)
            {
                p_data->sample_counter++;
                p_data->accumulator += (p_log_data->accumulator / p_log_data->sample_counter);
            }
            else
            {
                // No data. What do use?
            }
            // Increase time counters. Data is processed every log interval
            p_data->log_read_time.counter += p_log_data->log_interval;
            LOG(D, "Time counters: %d == %d", p_data->log_read_time.counter, p_data->log_read_time.interval);
            if (p_data->log_read_time.counter == p_data->log_read_time.interval)
            {
                // Store filtered data in new slot and publish updates in DATA
                int32_t new_data = INT32_MIN;
                if (p_data->sample_counter > 0)
                {
                    new_data = (int32_t)(p_data->accumulator / p_data->sample_counter);
                }
                // Move data
                for (int j = p_data->data_buffer_length - 1; j > 0; --j)
                {
                    p_data->data_buffer[j] = p_data->data_buffer[j - 1];
                }
                p_data->data_buffer[0] = new_data;

                // Publish new json structure in DATA
                publish_log_read_data_update(p_data);

                // Restart filter
                p_data->log_read_time.counter = 0;
                p_data->sample_counter = 0;
                p_data->accumulator = 0;
            }
        }
    }
}

static void publish_log_read_data_update(ddm_log_read_specification_data_t *p_data)
{
    uint32_t paramdata = LSRCFG0DATA | DDM2_PARAMETER_INSTANCE(p_data->instance);
    uint32_t parambindata = LSRCFG0BINDATA | DDM2_PARAMETER_INSTANCE(p_data->instance);
    LSRCFG0BINDATA_T *p_bindata = (LSRCFG0BINDATA_T *)l_ddmp_frame.frame.publish.value.raw;

    cJSON *p_base_item = cJSON_CreateObject();
    cJSON *p_item = cJSON_CreateNumber(0);  // Time is 0
    cJSON_AddItemToObject(p_base_item, "t", p_item);
    p_bindata->time = 0;
    p_item = cJSON_CreateNumber(p_data->log_read_time.interval);
    cJSON_AddItemToObject(p_base_item, "i", p_item);
    p_bindata->interval = p_data->log_read_time.interval;
    p_item = cJSON_CreateArray();
    for (int i = 0; i < p_data->data_buffer_length; i++)
    {
        cJSON *p_arr_item = NULL;
        if (p_data->data_buffer[i] != INT32_MIN)
        {
            p_arr_item = cJSON_CreateNumber(p_data->data_buffer[i]);
        }
        else
        {
            p_arr_item = cJSON_CreateNull();
        }
        cJSON_AddItemToArray(p_item, p_arr_item);
        p_bindata->data[i] = p_data->data_buffer[i];
    }

    cJSON_AddItemToObject(p_base_item, "d", p_item);
    char *temp = cJSON_PrintUnformatted(p_base_item);
    LOG(D, "%s", temp);

    struct ddm_entry *p_entry;
    size_t bindata_size = sizeof(LSRCFG0BINDATA_T) + p_data->data_buffer_length * sizeof(p_bindata->data[0]);

    p_entry = ddm_store__access(l_my_ddm_log->p__ddm_store_owned, paramdata);
    if (p_entry)
    {
        ddm_entry__set__value_str(p_entry, temp, strlen(temp));
    }
    p_entry = ddm_store__access(l_my_ddm_log->p__ddm_store_owned, parambindata);
    if (p_entry)
    {
        ddm_entry__set__value_struct(p_entry, p_bindata, bindata_size);
    }
    size_t data_size = strlen(temp);
    size_t sent_size = 0;
    // Loop to send max frames
    while (data_size > DDMP2_MAX_VALUE_SIZE)
    {
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, paramdata, temp + sent_size, DDMP2_MAX_VALUE_SIZE, l_my_ddm_log->p__description->connector->connector_id, portMAX_DELAY);
        sent_size += DDMP2_MAX_VALUE_SIZE;
        data_size -= DDMP2_MAX_VALUE_SIZE;
    }
    // Send the rest
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, paramdata, temp + sent_size, data_size, l_my_ddm_log->p__description->connector->connector_id, portMAX_DELAY);
    cJSON_free(temp);
    cJSON_Delete(p_base_item);
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, parambindata, p_bindata, bindata_size, l_my_ddm_log->p__description->connector->connector_id, portMAX_DELAY);
}

static struct ddm_entry *add_subscription_to_store(const struct ddm_log *ddm_log, const uint32_t parameter)
{
    struct ddm_entry * entry = NULL;

	// already exists?
	entry = ddm_store__access(ddm_log->p__ddm_store_subscribed, parameter);
	if (!entry)
	{
		entry = ddm_store__new_entry(ddm_log->p__ddm_store_subscribed, parameter);
	}

    return entry;
}

static void add_subscription(const struct ddm_log *p_ddm_log, const uint32_t parameter)
{
    struct ddm_entry *p_entry = add_subscription_to_store(p_ddm_log, parameter);                // Add to store and set subscription flag
    ASSERT(p_entry != NULL);
    int ret = inventory_handler_add(&l_inventory_handle, parameter); // Add to inventory handler
    ASSERT(ret == 0);
}

static int log_specification_add_new(const struct ddm_log *p_ddm_log, const ddm_log_specification_t *p_log_spec)
{
    if (log_specification_count < DDM_LOG__SPEC_MAX)
    {
        ddm_log_specification_data_t * const p_new_log_spec = &log_specifications[log_specification_count];
        uint32_t class_instance = LSCFG0;
        int instance = broker_register_instance(&class_instance, p_ddm_log->p__description->connector->connector_id);

        if (instance == -1)	// No more instances available
        {
            return 1;
        }

        const int Parse_failure = ddm_log_spec_parser__parse(p_new_log_spec, p_log_spec->config); // Parse the log specification string
        p_new_log_spec->instance = instance;
        if (Parse_failure)
        {
            return 1;
        }
        if (p_log_spec->num_read_configs > 0)
        {
            if ((log_read_specification_count + p_log_spec->num_read_configs) > DDM_LOG__SPEC_MAX)
            {
                LOG(E, "No space for storing log read configs from %s", p_log_spec->name);
                return 1;
            }
        }
        strncpy(p_new_log_spec->name, p_log_spec->name, sizeof(p_new_log_spec->name) - 1);

        add_subscription(p_ddm_log, p_new_log_spec->data.parameter);

        // Check all possible conditions
        for (int i = 0; i < p_new_log_spec->num_conditions; i++)
        {
            if (p_new_log_spec->conditions[i].comparison != COMPARISON_NONE)
            {
                if (p_new_log_spec->conditions[i].lhs.parameter != DDMP2_INVALID_PARAMETER)
                {
                    add_subscription(p_ddm_log, p_new_log_spec->conditions[i].lhs.parameter);
                }

                if (p_new_log_spec->conditions[i].rhs.parameter != DDMP2_INVALID_PARAMETER)
                {
                    add_subscription(p_ddm_log, p_new_log_spec->conditions[i].rhs.parameter);
                }
            }
        }

        if (p_new_log_spec->log_interval)	//todo: check order of timer events, use counter for samples?
        {
            if (!p_new_log_spec->sample_interval)
            {
                p_new_log_spec->sample_interval = p_new_log_spec->log_interval;
            }

            TRUE_CHECK_RETURN1(p_new_log_spec->sample_timer = xTimerCreateStatic(NULL, pdMS_TO_TICKS(p_new_log_spec->sample_interval * 1000), pdTRUE, (void*)log_specification_count, ddm_sample_interval_timer_callback, &p_new_log_spec->sample_timer_buffer));
            TRUE_CHECK_RETURN1(p_new_log_spec->log_timer = xTimerCreateStatic(NULL, pdMS_TO_TICKS(p_new_log_spec->log_interval * 1000), pdTRUE, (void*)log_specification_count, ddm_log_interval_timer_callback, &p_new_log_spec->log_timer_buffer));
        }
        else
        {
            // log all received frames separately?
            if (p_new_log_spec->sample_interval)
            {
                TRUE_CHECK_RETURN1(p_new_log_spec->sample_timer = xTimerCreateStatic(NULL, pdMS_TO_TICKS(p_new_log_spec->sample_interval * 1000), pdTRUE, (void*)log_specification_count, ddm_sample_interval_timer_callback, &p_new_log_spec->sample_timer_buffer));
            }
        }

        log_specification_count++;

		struct ddm_entry *p_entry = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSCFG0CFG | DDM2_PARAMETER_INSTANCE(instance));
		ddm_entry__set__value_str(p_entry, p_log_spec->config, strlen(p_log_spec->config));

		p_entry = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSCFG0NAME | DDM2_PARAMETER_INSTANCE(instance));
		strcpy(p_new_log_spec->name, p_log_spec->name);
		ddm_entry__set__value_str(p_entry, p_new_log_spec->name, strlen(p_log_spec->name));


        // Create LSCFG0ACT, default 1
        p_entry = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSCFG0ACT | DDM2_PARAMETER_INSTANCE(instance));
        ddm_entry__set__value_i32(p_entry, 1);
        log_specification_set_activate(p_ddm_log, p_entry, instance);

		// Create LSCFG0READCFG parameter
		struct ddm_entry *p_entry2 = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSCFG0READCFG | DDM2_PARAMETER_INSTANCE(instance));
		ddm_entry__set__value_str(p_entry2, NULL, 0);
		char temp_string[150];
		memset(temp_string, '\0', sizeof(temp_string));

        if (p_log_spec->num_read_configs > 0)
        {
            int num_read_configs = 0;
            while (num_read_configs < p_log_spec->num_read_configs)
            {
                ddm_log_read_specification_data_t * const p_new_log_read_spec = &log_read_specifications[log_read_specification_count];
                memset(p_new_log_read_spec, 0, sizeof(ddm_log_read_specification_data_t)); // clear the specification data
                p_new_log_read_spec->p_logconfig = p_new_log_spec;
                if (ddm_log_read_spec_parser__parse(p_new_log_read_spec, p_log_spec->p_read_configs[num_read_configs].config))
                {
                    // Failure to parse
                    return 1;
                }

                class_instance = LSRCFG0;
                instance = broker_register_instance(&class_instance, p_ddm_log->p__description->connector->connector_id);
                if (instance != -1)
                {
                    p_new_log_read_spec->instance = instance;
                    strncpy(p_new_log_read_spec->name, p_log_spec->p_read_configs[num_read_configs].name, sizeof(p_new_log_read_spec->name) - 1);

                    if (strlen(temp_string) > 0)
                    {
                        strcat(temp_string, ",");
                    }
                    strcat(temp_string, p_log_spec->p_read_configs[num_read_configs].name);
                    strcat(temp_string, ":");

                    size_t buffer_size = sizeof(temp_string) - strlen(temp_string);
                    ddm2_instance_name(class_instance, &temp_string[strlen(temp_string)], &buffer_size);

	                p_entry = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSRCFG0NAME | DDM2_PARAMETER_INSTANCE(instance));
	                ddm_entry__set__value_str(p_entry, p_log_spec->p_read_configs[num_read_configs].name, strlen(p_log_spec->p_read_configs[num_read_configs].name));
	                p_entry = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSRCFG0CFG | DDM2_PARAMETER_INSTANCE(instance));
	                ddm_entry__set__value_str(p_entry, p_log_spec->p_read_configs[num_read_configs].config, strlen(p_log_spec->p_read_configs[num_read_configs].config));
	                p_entry = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSRCFG0DATA | DDM2_PARAMETER_INSTANCE(instance));
                    p_entry = ddm_store__new_entry(p_ddm_log->p__ddm_store_owned, LSRCFG0BINDATA | DDM2_PARAMETER_INSTANCE(instance));

                    publish_log_read_data_update(p_new_log_read_spec);
                }
                else
                {
                    return 1;
                }
                num_read_configs++;
                log_read_specification_count++;
            }
        }
        ddm_entry__set__value_str(p_entry2, temp_string, strlen(temp_string));
    }

    return 0;
}

/*static void log_specification_clear_spec(uint32_t index)
{
    memset(&specifications[index], 0, sizeof(specifications[index]));
    //TODO: delete ddm_entry
    // preset pointer
    specifications[index].data = &specification_data[index];
}*/

/*static int log_specification_delete(const struct ddm_log * ddm_log, uint8_t index)
{
    //Reset and store data
    memset(&specification_data[index], 0, sizeof(specification_data[0]));

    // Reset metadata
    log_specification_clear_spec(index);

    ZERO_CHECK(nvs_set_blob(nvs_logcfgs, "log_cfgs", specification_data, log_specification_count * sizeof(specification_data[0])));
    ZERO_CHECK(nvs_commit(nvs_logcfgs));

    // Unsubscribe to parameter
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, LSCFG0AVL|DDM2_PARAMETER_INSTANCE(index), &Zero, sizeof(Zero), ddm_log->p__description->connector->connector_id, portMAX_DELAY);

    return 1;
}
*/
static int log_specification_set_activate(const struct ddm_log *p_ddm_log, const struct ddm_entry *entry, uint32_t instance)
{
    int32_t active = ddm_entry__value_i32(entry);

    ddm_log_specification_data_t *p_log_spec = NULL;
    // Find log spec
    for (size_t i = 0; i < log_specification_count; i++)
    {
        if ((uint32_t)log_specifications[i].instance == instance)
        {
            p_log_spec = &log_specifications[i];
            break;
        }
    }
    if (NULL == p_log_spec)
    {
        return 0;
    }
    if (active)
    {
        // Re-start timers if not already running
        if ((p_log_spec->sample_interval > 0) && !xTimerIsTimerActive(p_log_spec->sample_timer))
        {
            LOG(D, "Starting sample_timer for %s", p_log_spec->name);
            TRUE_CHECK_RETURN0(xTimerStart(p_log_spec->sample_timer, portMAX_DELAY));
        }
        if ((p_log_spec->log_interval > 0) && !xTimerIsTimerActive(p_log_spec->log_timer))
        {
            LOG(D, "Starting log_timer for %s", p_log_spec->name);
            TRUE_CHECK_RETURN0(xTimerStart(p_log_spec->log_timer, portMAX_DELAY));
        }
    }
    else
    {
        // Stop timers if running
        if ((p_log_spec->sample_interval > 0) && xTimerIsTimerActive(p_log_spec->sample_timer))
        {
            LOG(D, "Stopping sample_timer for %s", p_log_spec->name);
            xTimerStop(p_log_spec->sample_timer, portMAX_DELAY);
        }
        if ((p_log_spec->log_interval > 0) && xTimerIsTimerActive(p_log_spec->log_timer))
        {
            LOG(D, "Stopping log_timer for %s", p_log_spec->name);
            xTimerStop(p_log_spec->log_timer, portMAX_DELAY);
        }
    }
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, LSCFG0ACT | DDM2_PARAMETER_INSTANCE(instance), &active, sizeof(active), p_ddm_log->p__description->connector->connector_id, portMAX_DELAY);

    return 1;
}

static int get_stored_data(ddm_log_data_t *p_log_data, int32_t *sample_data)
{
    struct ddm_entry *p_entry = ddm_store__access(l_my_ddm_log->p__ddm_store_subscribed, p_log_data->parameter);
    TRUE_CHECK_RETURN1(p_entry != NULL);
    const void *data;
    size_t data_size;
    //Get pointer to data
    ddm_entry__read__value(p_entry, &data, &data_size);
    // Extract data
    if ((data == NULL) || (data_size == 0))
    {
        // Have not received any values yet
        return 1;
    }
    TRUE_CHECK_RETURN1(data_size >= (size_t)(p_log_data->offset + p_log_data->size));
    switch (p_log_data->size)
    {
    case 1:
        *sample_data = *((int8_t*)(data + p_log_data->offset));
        break;
    case 2:
        *sample_data = *((int16_t*)(data + p_log_data->offset));
        break;
    case 4:
        *sample_data = *((int32_t*)(data + p_log_data->offset));
        break;
    default:
        break;
    }

    return 0;
}

static int handle_set(struct ddm_log * ddm_log, const DDMP2_FRAME * ddmp2_frame)
{
    int is_valid = 0;
    struct ddm_entry * ddm_entry;

    ddm_entry = ddm_store__access(ddm_log->p__ddm_store_owned, ddmp2_frame->frame.set.parameter);
    if (ddm_entry == NULL)
    {
        /* Somebody tried to set a value to something that we haven't created */
        LOG(W, "%s: set non-existing DDM: 0x%08x",
            ddm_log->p__description->connector->name,
            ddmp2_frame->frame.set.parameter);
        return 0;
    }

    ddm_entry__write__value(ddm_entry, &ddmp2_frame->frame.set.value, ddmp2_value_size(ddmp2_frame));

    // Log Service
    switch (ddmp2_frame->frame.set.parameter)
    {
    case LS0LSCFG:
    {
        const char *str = ddm_entry__value_str(ddm_entry);
        // loop all LSCFG0
        uint32_t ret_class = DDMP2_INVALID_PARAMETER;
        for (size_t i = 0; i < log_specification_count; i++)
        {
            if (i >= DDM_LOG__SPEC_MAX)
            {
                break;
            }
            // Get ddm_entry
            if (strncmp(log_specifications[i].name, str, strlen(log_specifications[i].name)) == 0)
            {
                // Match
                ret_class = LSCFG0 | DDM2_PARAMETER_INSTANCE(log_specifications[i].instance);
                break;
            }
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, ddmp2_frame->frame.set.parameter, &ret_class, sizeof(ret_class), ddm_log->p__description->connector->connector_id, portMAX_DELAY);
        break;
    }
    }
    // Log Specification
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(ddmp2_frame->frame.set.parameter);
    switch (DDM2_PARAMETER_BASE_INSTANCE(ddmp2_frame->frame.set.parameter))
    {
        case LSCFG0ACT:
        {
            is_valid = log_specification_set_activate(ddm_log, ddm_entry, instance);
            break;
        }
        default:
            break;
    }
    return is_valid;
}

static int handle_subscribe(const struct ddm_log * ddm_log, const DDMP2_FRAME * ddmp2_frame)
{
    struct ddm_entry * ddm_entry;

    //ddm_entry = ddm_store__access(ddm_log->p__ddm_store_owned, DDM2_PARAMETER_BASE_INSTANCE(ddmp2_frame->frame.subscribe.parameter));
    ddm_entry = ddm_store__access(ddm_log->p__ddm_store_owned, ddmp2_frame->frame.subscribe.parameter);
    if (ddm_entry == NULL)
    {
        /* Somebody tried to subscribe to something that we haven't created */
        LOG(W, "%s: subscribe to non-existing DDM: 0x%08x",
            ddm_log->p__description->connector->name,
            ddmp2_frame->frame.subscribe.parameter);
        return 0;
    }
    /* See if it has some output type? */
    if (ddm_entry__out_type(ddm_entry) == DDM2_TYPE_NONE)
    {
        /* This parameter has no output type. */
        LOG(W, "%s: denied subscription to DDM: %u (no output type)",
            ddm_log->p__description->connector->name,
            ddmp2_frame->frame.subscribe.parameter);
        return 0;
    }

    switch (ddmp2_frame->frame.subscribe.parameter)
    {
        case LS0RAMSIZE:
        {
            uint32_t storage_size = max_storage_size;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, LS0RAMSIZE, &storage_size, sizeof(storage_size), ddm_log->p__description->connector->connector_id, portMAX_DELAY);
            return 1;
        }
        default:
            break;
    }

    //uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(ddmp2_frame->frame.subscribe.parameter);
    switch (DDM2_PARAMETER_BASE_INSTANCE(ddmp2_frame->frame.subscribe.parameter))
    {
        case LSRCFG0DATA:
        case LSCFG0CFG:
        case LSCFG0NAME:
        case LSRCFG0NAME:
        case LSRCFG0CFG:
        case LSCFG0READCFG:
        {
            const char *str = ddm_entry__value_str(ddm_entry);
            // Need to handle case if strings are too long
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, ddmp2_frame->frame.subscribe.parameter, str, ddm_entry__value_size(ddm_entry), ddm_log->p__description->connector->connector_id, portMAX_DELAY);
            break;
        }
        case LSCFG0ACT:
        {
            int32_t active = ddm_entry__value_i32(ddm_entry);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, ddmp2_frame->frame.subscribe.parameter, &active, sizeof(active), ddm_log->p__description->connector->connector_id, portMAX_DELAY);
            break;
        }
        case LSRCFG0BINDATA:
        {
            const void *data = ddm_entry__value_struct(ddm_entry);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, ddmp2_frame->frame.subscribe.parameter, data, ddm_entry__value_size(ddm_entry), ddm_log->p__description->connector->connector_id, portMAX_DELAY);
            break;
        }
        default:
            break;
    }

    return 1;
}

static void handle_publish(const struct ddm_log * const ddm_log, const DDMP2_FRAME * const p_ddmp2_frame)
{
	ddm_entry_t * ddm_entry = ddm_store__access(ddm_log->p__ddm_store_subscribed, p_ddmp2_frame->frame.publish.parameter);
	if (ddm_entry != NULL)
	{
	    size_t value_size = ddmp2_value_size(p_ddmp2_frame);
		LOG(P, "%i %08x", ddm_entry__set__value(ddm_entry, p_ddmp2_frame->frame.publish.value.raw, value_size), p_ddmp2_frame->frame.publish.parameter);

		// Now, do I need to also create a log entry at this point?
		// Check the log specifications for filter and interval
		ddm_log_specification_data_t *p_log_spec_data = NULL;
	    // Find log spec
	    for (size_t i = 0; i < log_specification_count; i++)
	    {
	        p_log_spec_data = &log_specifications[i];
	        if (p_log_spec_data->data.parameter == p_ddmp2_frame->frame.publish.parameter)
	        {
	            if ((p_log_spec_data->log_interval > 0) || (p_log_spec_data->sample_interval > 0))
	            {
	                // If there is log interval or sample interval, logging is handled later (SAMPLE_EVENT or LOG_EVENT)
	                continue;
	            }
	            else
	            {
	                // Both are zero -> means all events are to be logged. Generate internal SAMPLE_EVENT and LOG_EVENT
	                uint32_t log_spec_index = (uint32_t)i;
	                connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, SAMPLE_EVENT, &log_spec_index, sizeof(log_spec_index), ddm_log->p__description->connector->connector_id, portMAX_DELAY);
	            }
	        }
	    }
	}
}

static int handle_generic(const struct ddm_log *context, const DDMP2_FRAME * const p_ddmp2_frame)
{
    const uint32_t Log_specification_index = *(const uint32_t*)p_ddmp2_frame->frame.generic.data;
    ddm_log_specification_data_t *p_log_spec_data = &log_specifications[Log_specification_index];

    switch (p_ddmp2_frame->frame.generic.id)
    {
    case LOG_EVENT:     // Create a log entry from the data in the accumulator
    {
        //LOG(P, "LOG_EVENT for log configuration %d", Log_specification_index);

        if (!do_condition_comparison(p_log_spec_data))
        {
            LOG(D, "Condition not met for logging configuration %d", Log_specification_index);
            return 0;
        }

        int64_t new_data = 0;
        // Check hysteresis against previously logged value
        if (p_log_spec_data->sample_counter > 0)
        {
            if (check_hysteresis(p_log_spec_data))
            {
                new_data = p_log_spec_data->accumulator / p_log_spec_data->sample_counter;
                LOG(D, "Create new log entry: %lld, outside %lld:%lld:%lld", new_data, p_log_spec_data->prev_data - p_log_spec_data->hysteresis.lower, p_log_spec_data->prev_data, p_log_spec_data->prev_data + p_log_spec_data->hysteresis.upper);

                ddm_log_event__data_t *p_log_event = ddm_log_event__create(p_log_spec_data->data.size);
                p_log_event->val.header.ddm_id = p_log_spec_data->data.parameter;
                p_log_event->val.header.offset = p_log_spec_data->data.offset;
                p_log_event->val.header.repeat = p_log_spec_data->repeat;
                memcpy(p_log_event->val.value, &new_data, p_log_spec_data->data.size);

                ddm_log_event__write(p_log_spec_data->destination, p_log_event);
                ddm_log_event__free(p_log_event);
                p_log_spec_data->repeat = 0;
            }
            else
            {
                // Skip to create a log event here as new value not outside hysteresis values
                p_log_spec_data->repeat++;
                LOG(D, "Skip to create log event (%d). Value (%lld) not outside %lld:%lld:%lld", p_log_spec_data->repeat, new_data, p_log_spec_data->prev_data - p_log_spec_data->hysteresis.lower, p_log_spec_data->prev_data, p_log_spec_data->prev_data + p_log_spec_data->hysteresis.upper);
                // Keep old reference to avoid drifting
                new_data = p_log_spec_data->prev_data;
            }
        }
        // Pass data to log read configs if any
        increment_log_read_data(p_log_spec_data);
        // Resetting data
        p_log_spec_data->sample_counter = 0;
        p_log_spec_data->prev_data = new_data;
        p_log_spec_data->accumulator = 0;
        break;
    }
    case SAMPLE_EVENT:  // Read and accumulate data from store
    {
        //LOG(P, "SAMPLE_EVENT for log configuration %d (param: 0x%08x)", Log_specification_index, p_log_spec_data->data.parameter);
        int32_t sample_data = 0;
        // Get stored data
        if (get_stored_data(&(p_log_spec_data->data), &sample_data))
        {
            return 1;
        }
        p_log_spec_data->accumulator += (sample_data);
//        LOG(D, "Acc data: %d", (int)p_data->accumulator);
        p_log_spec_data->sample_counter++;
        if (p_log_spec_data->log_interval == 0)
        {
            // Trig to log event
            connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, LOG_EVENT, &Log_specification_index, sizeof(Log_specification_index), context->p__description->connector->connector_id, portMAX_DELAY);
        }
        break;
    }
    default:
        return 1;
    };

    return 0;
}

static void worker_task(void * arg)
{
    struct ddm_log *ddm_log = (struct ddm_log *)arg;
    l_my_ddm_log = ddm_log;

    ddm_store__load_entries(ddm_log->p__ddm_store_owned,
        ddm_log->p__description->ddm_initial_owned_values,
        ddm_log->p__description->ddm_initial_owned_values_size,
        0);

    /*  We want to be the owner of the parameters. Request an instance then
     *  publish the available DDM parameter.
     */
    uint32_t connector_instance = ddm_log->p__description->ddm_class;
    int ret_val = broker_register_instance(&connector_instance, ddm_log->p__description->connector->connector_id);
    if (ret_val == -1)
    {
        LOG(E, "%s: failed to register ownership", ddm_log->p__description->connector->name);
        vTaskSuspend(NULL);
    }

    for (;;)
    {
        DDMP2_FRAME *p_frame;
        size_t frame_size;

        frame_size = 0u;
        p_frame = (DDMP2_FRAME *)xRingbufferReceive(ddm_log->p__description->connector->to_connector, &frame_size, portMAX_DELAY);
        if (p_frame != NULL)
        {
            if (!inventory_handler_update(&l_inventory_handle, p_frame))
            {
                switch (p_frame->frame.control)
                {
                case DDMP2_CONTROL_SET:
                    handle_set(ddm_log, p_frame);
                    break;
                case DDMP2_CONTROL_SUBSCRIBE:
                    handle_subscribe(ddm_log, p_frame);
                    break;
                case DDMP2_CONTROL_PUBLISH:
                    handle_publish(ddm_log, p_frame);
                    break;
                case DDMP2_CONTROL_GENERIC:
                    handle_generic(ddm_log, p_frame);
                    break;
                default:
                    break;
                }
            }
            vRingbufferReturnItem(ddm_log->p__description->connector->to_connector, p_frame);
        }
    }
}


bool ddm_log__get_localtime(struct tm * const tm_time)
{
#ifdef CONNECTOR_SYSTEM
    if (connector_system_get_local_time(tm_time))
    {
        return 1;
    }
#endif //CONNECTOR_SYSTEM
    return 0;
}

int ddm_log__init(struct ddm_log * ddm_log, const struct ddm_log__descriptor * description)
{
    BaseType_t error = 0;

    ddm_log->p__description = description;
    ddm_log->p__ddm_store_owned = description->ddm_store_owned;
    ddm_log->p__ddm_store_subscribed = description->ddm_store_subscribed;

    // Instantiate, initialize and start inventory handler object
    inventory_handler_init(&l_inventory_handle, &inventory_list, inventory_handler_cb, (void*)ddm_log);

    error = xTaskCreate(
        /* code */          worker_task,
        /* name */          description->connector->name,
        /* stack size */    description->worker_stack_size,
        /* parameters */    ddm_log,
        /* priority */      description->worker_priority,
        /* handle */        NULL);
    TRUE_CHECK(error);

    ZERO(log_specifications);
    ZERO(log_read_specifications);
    ZERO_CHECK(nvs_open("logcfgs", NVS_READWRITE, &nvs_logcfgs));

    nvs_get_u32(nvs_logcfgs, "log_size", &max_storage_size);
    max_storage_size = ddm_log_event__init(DDM_LOG_SPEC_PARSER__RAM, max_storage_size);
    ddm_log_event__init(DDM_LOG_SPEC_PARSER__FLASH, 0);

    ZERO_CHECK(ddm_log_spec_parser__prepare());

    return error == pdPASS ? CONNECTOR_INIT_SUCCESS : CONNECTOR_INIT_FAILURE;
}

int connector_ddm_log_load_configurations(const struct load_configurations__configuration *config)
{
    int ret_value = 0;

    switch(config->configuration_location_type)
    {
    case LOAD_CONFIGURATION__LOCATION_ROM:
    {
        LOG(E, "Not supported LOAD_CONFIGURATION__LOCATION_ROM");
        ret_value = 1;
        break;
    }
    case LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM:
    {
        // Load json file from spiffs. Filename is the config
        struct stat st;
        if (stat(config->static_config->configuration, &st) != 0)
        {
            LOG(W, "Could not find configuration file: %s", (char*)config->static_config->configuration);
        }
        else
        {
            LOG(I, "Trying to load configuration file: %s (%ld bytes)", (char*)config->static_config->configuration, st.st_size);
            FILE *f = fopen((char*)config->static_config->configuration, "r");
            if (f == NULL)
            {
                LOG(W, "Could not open configuration file: %s", (char*)config->static_config->configuration);
            }
            else
            {
#ifdef LOAD_CONFIGURATION_USE_JSON_CJSON
                cJSON *root = NULL;
                char *buffer = hal_mem_malloc_prefer(st.st_size + 4, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                assert(buffer != NULL);
                size_t read_bytes = fread(buffer, 1, st.st_size, f);
                LOG(D, "Read %d bytes", read_bytes);
                fclose(f);
                root = cJSON_ParseWithLength(buffer, read_bytes);
                //LOG(I,"After cJSON_Parse Heap left: %d, stack left: %d", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));
                if (root != NULL)
                {
                    cJSON *ruleArrayObj = cJSON_GetObjectItemCaseSensitive(root, "configs");
                    if (ruleArrayObj != NULL)
                    {
                        // "rule" item must be an array of "objects"
                        if (cJSON_IsArray(ruleArrayObj))
                        {
                            int num_childs = cJSON_GetArraySize(ruleArrayObj);
                            while (num_childs > 0)
                            {
                                cJSON *childObj = cJSON_GetArrayItem(ruleArrayObj, 0);
#if defined(LOAD_CONFIGURATION_PRINT_JSON)
                                {
                                    char *temp = cJSON_Print(childObj);
                                    LOG(D, "%s", temp);
                                    cJSON_free(temp);
                                    vTaskDelay(pdMS_TO_TICKS(100));
                                }
#endif
                                ddm_log_specification_t config;
                                config.config = NULL;
                                config.num_read_configs = 0;
                                config.p_read_configs = NULL;
                                // Get "name"
                                cJSON *nameObj = cJSON_GetObjectItemCaseSensitive(childObj, "name");
                                memset(config.name, '\0', sizeof(config.name));
                                strncpy(config.name, nameObj->valuestring, sizeof(config.name) - 1);
                                // Get "config"
                                cJSON *configObj = cJSON_GetObjectItemCaseSensitive(childObj, "config");
                                config.config = configObj->valuestring;

                                // Check for read configs
                                cJSON *readConfigsArray = cJSON_GetObjectItemCaseSensitive(childObj, "readconfigs");
                                if ((readConfigsArray != NULL) && (cJSON_IsArray(readConfigsArray)))
                                {
                                    int num_readconfigs = cJSON_GetArraySize(readConfigsArray);
                                    int read_config_idx = 0;
                                    config.num_read_configs = num_readconfigs;
                                    ddm_log_read_specification_t *p_read_spec;
                                    config.p_read_configs = (ddm_log_read_specification_t *)hal_mem_malloc_prefer(num_readconfigs * sizeof(ddm_log_read_specification_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                                    p_read_spec = config.p_read_configs;
                                    while (num_readconfigs > 0)
                                    {
                                        cJSON *readConfigObj = cJSON_GetArrayItem(readConfigsArray, 0);
                                        // Get "name"
                                        cJSON *nameObj = cJSON_GetObjectItemCaseSensitive(readConfigObj, "name");
                                        memset(p_read_spec[read_config_idx].name, '\0', sizeof(p_read_spec[read_config_idx].name));
                                        strncpy(p_read_spec[read_config_idx].name, nameObj->valuestring, sizeof(p_read_spec[read_config_idx].name)-1);
                                        // Get "config"
                                        cJSON *configObj = cJSON_GetObjectItemCaseSensitive(readConfigObj, "config");
                                        p_read_spec[read_config_idx].config = hal_mem_malloc_prefer(strlen(configObj->valuestring) + 1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                                        strcpy(p_read_spec[read_config_idx].config, configObj->valuestring);
                                        read_config_idx++;
                                        cJSON_DeleteItemFromArray(readConfigsArray, 0);
                                        num_readconfigs = cJSON_GetArraySize(readConfigsArray);
                                    }
                                }
                                log_specification_add_new(l_my_ddm_log, &config);
                                hal_mem_free(config.p_read_configs);
                                cJSON_DeleteItemFromArray(ruleArrayObj, 0);
                                num_childs = cJSON_GetArraySize(ruleArrayObj);
                            }
                        }
                        else
                        {
                            LOG(D, "Error json format: \"configs\" is not an cJSON_IsArray");
                        }
                    }
                    else
                    {
                        LOG(D, "Error json format: Could not find \"configs\" item");
                    }
                    cJSON_Delete(root);
                }
                else
                {
                    LOG(W, "Error parsing json: %p (%p)", cJSON_GetErrorPtr(), buffer);
                }
                hal_mem_free(buffer);
                // Start after parsing all configs
                inventory_handler_start(&l_inventory_handle, l_my_ddm_log->p__description->connector);
#else
                int i;
                int r;
                int n;

                jsmn_parser p;
                jsmntok_t *t;

                char *buffer = heap_caps_malloc_prefer(st.st_size+4, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
                assert(buffer != NULL);
                size_t read_bytes = fread(buffer, 1, st.st_size, f);

                LOG(D, "Read %d bytes", read_bytes);
                fclose(f);
                //LOG(I,"After fclose() Heap left: %d, stack left: %d", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));

                jsmn_init(&p);
                n = jsmn_parse(&p, buffer, read_bytes, NULL, 0);
                if (n <= 0)
                {
                    LOG(E, "Failed to parse JSON: %d", n);
                    heap_caps_free(buffer);
                    return -1;
                }
                LOG(D, "Need to allocate %d tokens", n);
                jsmn_init(&p);
                t = heap_caps_malloc_prefer(sizeof(jsmntok_t)*n, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
                r = jsmn_parse(&p, buffer, read_bytes, t, n);
                if (r < 0)
                {
                    LOG(E, "Failed to parse JSON: %d", r);
                    heap_caps_free(buffer);
                    heap_caps_free(t);
                    return -1;
                }

                /* Assume the top-level element is an object */
                if (r < 1 || t[0].type != JSMN_OBJECT)
                {
                    LOG(E, "Object expected");
                    heap_caps_free(buffer);
                    heap_caps_free(t);
                    return -1;
                }
                LOG(D, "Num parsed json tokens: %d", r);

                /* Root object must be "rules" */
                if ((jsoneq(buffer, &t[1], "rules") == 0) && (t[1].type == JSMN_STRING))
                {
                    int j;
                    if (t[2].type != JSMN_ARRAY)
                    {
                        heap_caps_free(buffer);
                        heap_caps_free(t);
                        return -1; /* We expect "rules" to be an array of objects */
                    }
                    i = 3;
                    // For each rule expect objects
                    for (j = 0; j < t[2].size; j++)
                    {
                        if (i >= r)
                        {
                            LOG(E, "Parsing failed!");
                            ret_value = -1;
                            break;
                        }
                        if (t[i].type == JSMN_OBJECT)
                        {
                            struct rule_engine__rule rule;
                            memset(rule.name, '\0', sizeof(rule.name));
                            rule.size = 0;
#if defined(LOAD_CONFIGURATION_PRINT_JSON)
                            LOG I, ("%d [%d (%d) - %d %d] %.*s",i, t[i].type, t[i].size, t[i].start, t[i].end, t[i].end-t[i].start, buffer+t[i].start);
#endif
                            // size in a object means number of tokens
                            int object_size = t[i].size;
                            for (int k = 0; k < object_size; k++)
                            {
                                if ((jsoneq(buffer, &t[i+1], "name") == 0) && (t[i+1].type == JSMN_STRING))
                                {
                                    // Get "name"
                                    strncpy(rule.name, buffer+t[i+2].start, MIN(RULE_ENGINE__RULE_NAME_LEN-1, t[i+2].end-t[i+2].start));
                                    i += 2;
                                }
                                else if ((jsoneq(buffer, &t[i+1], "rule") == 0) && (t[i+1].type == JSMN_STRING))
                                {
                                    // Get "rule"
                                    rule.rule = buffer+t[i+2].start;
                                    rule.size = t[i+2].end-t[i+2].start;
                                    i += 2;
                                }
                                else if ((jsoneq(buffer, &t[i+1], "comment") == 0) && (t[i+1].type == JSMN_STRING))
                                {
                                    i += 2;
                                }
                                else
                                {
                                    LOG(E, "Unexpected string %.*s", t[i+1].end-t[i+1].start, buffer+t[i+1].start);
                                    ret_value = -1;
                                    break;;
                                }
                            }
                            i++;
                            rule_engine__add_specification(p_rule_engine_inst, &rule);
                            LOG(I ,"After add spec Heap left: %d, stack left: %d", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));
                        }
                    }

                }
                else
                {
                    ret_value = -1;
                }
                heap_caps_free(buffer);
                heap_caps_free(t);
#endif
            }
        }
        break;
    }
    default:
        ret_value = -1;
        break;
    }

    return ret_value;
}


/** \} */
