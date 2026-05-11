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

/* Depends */
#include <string.h>

/* Implements */
#include "event_manager.h"

#include "broker.h"
#include "cJSON.h"
#include "configuration.h"
#include "connector_event_notification_defaults.h"
#include "ddm_converter.h"
#include "ddm_entry.h"
#include "event_record.h"
#include "event_type.h"
#include "make_json.h"
#if defined(CONNECTOR_MQTT)
#include "connector_mqtt.h"
#endif

#if (CONNECTOR_EVENT_NOTIFICATION_VERBOSE_LOG == 1)
#define EVENT_NOTIFICATION_LOG(level, format, ...) LOG(level, format, ##__VA_ARGS__)
#else
#define EVENT_NOTIFICATION_LOG(level, format, ...)
#endif

#define OPS_SUCCESS                   0
#define OPS_FAILURE                   1
#define OPS_ERROR_JSON_INVALID        2
#define OPS_ERROR_JSON_MISSING_FIELDS 3
#define OPS_ERROR_TYPE_NOT_HANDLED    4
#define OPS_ERROR_D_FIELD             5
#define OPS_ERROR_ALREADY_EXISTS      6
#define OPS_ERROR_NO_MEMORY           7

#define NVS_EVENT_ID_KEY               "enevent_id"
#define NVS_ACKNOWLEDGED_KEY           "enack"
#define NVS_EVENT_RECORD_ID_KEY        "ener%02uid"
#define NVS_EVENT_RECORD_TS_KEY        "ener%02uts"
#define NVS_EVENT_RECORD_TYPE_KEY      "ener%02utype"
#define NVS_EVENT_RECORD_FLAGS_KEY     "ener%02uflags"
#define NVS_EVENT_RECORD_SIZE_KEY      "ener%02usize"
#define NVS_EVENT_RECORD_DDM_ID_KEY    "ener%02uddm%02uid"
#define NVS_EVENT_RECORD_DDM_DATA_KEY  "ener%02uddm%02udata"
#define NVS_EVENT_RECORD_TRG_DATA_KEY  "ener%02utrg%02udata"
#define NVS_EVENT_RECORD_DATA_FLAG_KEY "ener%02udat%02uflag"
#define NVS_EVENT_RECORD_DB_SIZE_KEY   "enerdbsize"

#define EVENT_RECORD_DATA_FLAG_HAS_NO_DDM_VALUE     0
#define EVENT_RECORD_DATA_FLAG_HAS_DDM_VALUE        1
#define EVENT_RECORD_DATA_FLAG_HAS_NO_TRIGGER_VALUE 2
#define EVENT_RECORD_DATA_FLAG_HAS_TRIGGER_VALUE    3

#define KEYWORD_PREFIX         "%"
#define KEYWORD_TRIGGER_PREFIX KEYWORD_PREFIX "trigger."

#define CLOUD_JSON_BUFFER_SIZE 500

/* Concrete ennack_t structure */

typedef struct nack_concrete
{
    int32_t nack;
    int32_t ids[CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS];
} nack_concrete_t;

typedef int(parser_inserter_fn_t)(void *p_arg, const event_type_t *p_event_type);

/* -- Misc helpers -- */
static int event_type_db_insert_wrap(void *p_arg, const event_type_t *p_event_type);
static int broker_publish(const event_manager_t *p_evm, const ddm_entry_t *p_entry);
static int evm_broker_publish_changed(event_manager_t *p_evm);
static int broker_subscribe(const event_manager_t *p_evm, const ddm_entry_t *p_entry);
static void nack_update(nack_concrete_t *p_nack, const event_record_db_t *p_evm);
static size_t nack_sizeof(const nack_concrete_t *p_nack);
/* -- JSON parser/unparser -- */
static bool parser_is_keyword_data_element(const char *p_data_element_str);
static bool parser_is_trigger_data_element(const char *p_data_element_str);
static bool parser_is_parameter_data_element(const char *p_data_element_str);
static int parser_parse_trigger_data_element(const char *p_data_element_str, int32_t *p_out_trigger);
static int parser_parse_parameter_data_element(const char *p_data_element_str, uint32_t *p_out_parameter);
static int parser_parse_optional_field(const char *p_data_element_str,
                                       int32_t *p_field_offset,
                                       int32_t *p_field_size);
static bool parser_get_int_field(cJSON *p_json_object, const char *p_field_name, int32_t *p_out_value);
static bool parser_get_string_field(cJSON *p_json_object, const char *p_field_name, const char **p_out_value);
static bool parser_get_array_field(cJSON *p_json_object, const char *p_field_name, cJSON **p_out_array);
static bool parser_is_valid_root(cJSON *p_json_root);
static int parser_parse_event_type_values_element(event_manager_t *p_evm, cJSON *p_data_element, event_type_t *p_event_type);
static int parser_parse_event_type_array_element(event_manager_t *p_evm,
                                                 cJSON *p_type_element,
                                                 parser_inserter_fn_t *p_inserter,
                                                 void *p_inserter_arg);
static int parser_parse_event_type_array(event_manager_t *p_evm,
                                         cJSON *p_type_array,
                                         parser_inserter_fn_t *p_inserter,
                                         void *p_inserter_arg);
static int parser_parse_event_type_format(event_manager_t *p_evm,
                                          cJSON *p_json_root,
                                          parser_inserter_fn_t *p_inserter,
                                          void *p_inserter_arg);
static int parser_parse_event_type(event_manager_t *p_evm,
                                   const char *p_json_string,
                                   parser_inserter_fn_t *p_inserter,
                                   void *p_inserter_arg);

/* -- DDM handling -- */
static void convert_evm0ack(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0ack);
static void convert_evm0cmd(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0cmd);
static void convert_evm0rec(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0trig);
static void convert_evm0trig(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0trig);
static int32_t handle_owned_parameter_register(uint32_t ddm_class, const CONNECTOR *connector);
static void handle_owned_parameter_set(event_manager_t *p_evm,
                                       const DDMP2_FRAME *p_ddmp2_frame);
static void handle_owned_parameter_subscribe(event_manager_t *p_evm,
                                             const DDMP2_FRAME *p_ddmp2_frame);
static void handle_other_parameter_publish(event_manager_t *p_evm,
                                           const DDMP2_FRAME *p_ddmp2_frame);
#if defined(CONNECTOR_MQTT)
static void mqtt_setup(event_manager_t *p_evm);
static bool mqtt_is_connected_ddm_entry(const ddm_entry_t *p_ddm_entry);
static bool mqtt_is_connected_cached(event_manager_t *p_evm);
static int mqtt_send_event_record(event_manager_t *p_evm, const event_record_t *p_event_record);
static uint32_t mqtt_send_all_event_records(event_manager_t *p_evm);
#endif

static void inventory_handler_cb(void *p_arg, uint32_t device_class_instance, bool is_available);

/* -- Event manager functions -- */
static void event_manager_init(event_manager_t *p_evm,
                               CONNECTOR *p_connector,
                               SORTED_CONTAINER *p_event_type_db_storage,
                               SORTED_CONTAINER *p_event_record_db_storage,
                               event_id_t *p_event_record_db_fifo_storage,
                               SORTED_LIST *p_inventory_handler_storage,
                               SORTED_CONTAINER *p_owned_ddm_store_storage,
                               SORTED_CONTAINER *p_other_ddm_store_storage);
static void event_manager_init_owned_ddm(event_manager_t *p_evm,
                                         const ddm_store_ddm_t *p_owned_ddm_values,
                                         size_t owned_ddm_values_size,
                                         int32_t ddm_class_instance);
static int event_manager_load_event_types(event_manager_t *p_evm, const char *json_string);
static void event_manager_subscribe_other(event_manager_t *p_evm);
static int event_manager_start(event_manager_t *p_evm);
static int event_manager_save_record_ddm_value(const char *p_nvs_key, const ddm_entry_t *p_record_ddm);
static int event_manager_load_record_ddm_value(const char *p_nvs_key, ddm_entry_t *p_record_ddm);
static int event_manager_load_record_data(event_record_t *er, uint32_t er_index, size_t er_size, const event_type_db_t *p_event_type_db);
static int event_manager_load_records(event_manager_t *p_evm);
static int event_manager_save_records(const event_manager_t *p_evm);
static int event_manager_load_event_id(event_manager_t *p_evm);
static int event_manager_save_event_id(const event_manager_t *p_evm);
static int event_manager_load_acknowledged(event_manager_t *p_evm);
static int event_manager_save_acknowledged(const event_manager_t *p_evm);
static void event_manager_set_defaults(event_manager_t *p_evm);
static void event_manager_set_defaults_keep_event_id(event_manager_t *p_evm);
static void event_manager_generate_initial_values(event_manager_t *p_evm);

static nack_concrete_t initial_nack_value;

/**
 * @brief       This array defines out_type DDM parameters initial values
 *              which are owned by this connector (EVM class).
 *
 * Since these are owned by us, this array is defined using the DDM parameter
 * OUT types.
 */
static const ddm_store_ddm_t owned_ddm_store_initial_values[] = {
    {
        .ddm_parameter = EVM0AVL,
        .value = {
            .storage = {.i32 = 1},
            .type = DDM2_TYPE_INT32_T,
        },
    },
    {
        .ddm_parameter = EVM0ID,
        .value = {
            .storage = {.i32 = 0},
            .type = DDM2_TYPE_INT32_T,
        },
    },
    {
        .ddm_parameter = EVM0ACK,
        .value = {
            .storage = {.i32 = 0},
            .type = DDM2_TYPE_INT32_T,
        },
    },
    {
        .ddm_parameter = EVM0CMD,
        .value = {
            .storage = {.i32 = EVM0CMD_CLEAR_ALL},
            .type = DDM2_TYPE_INT32_T,
        },
    },
    {
        .ddm_parameter = EVM0NACK,
        .value = {
            .storage = {.structure = &initial_nack_value},
            .type = DDM2_TYPE_STRUCT,
        },
    },
    {
        .ddm_parameter = EVM0REC,
        .value = {
            .storage = {.str = "{}"},
            .type = DDM2_TYPE_STRING,
        },
    },
    {
        .ddm_parameter = EVM0TRIG,
        .value = {
            .storage = {.i32 = 0},
            .type = DDM2_TYPE_INT32_T,
        },
    },
};

/**
 * @brief       Conversion table needed for processing owned DDM parameters.
 */
static const ddm_converter_t event_manager_conversion_table[] = {
    {
        .parameter_id = EVM0ACK,
        .fn = convert_evm0ack,
    },
    {
        .parameter_id = EVM0CMD,
        .fn = convert_evm0cmd,
    },
    {
        .parameter_id = EVM0REC,
        .fn = convert_evm0rec,
    },
    {
        .parameter_id = EVM0TRIG,
        .fn = convert_evm0trig,
    },
};

/**
 * @brief       Stores current instance of event manager.
 *
 * This variable is used to store current instance of event manager. Currently,
 * this is only a pointer, but it could become a list of pointers in the future
 * when we want to have multuple instances of event managers.
 */
static event_manager_t *p_current_evm;

#if defined(CONNECTOR_MQTT)
/**
 * @brief       JSON buffer used for generating JSON strings.
 */
static EXT_RAM_ATTR char json_buffer[CLOUD_JSON_BUFFER_SIZE];
#endif

/* -- Misc helpers -- */

/**
 * @brief       Wrapper around event type database used by JSON parser code.
 *
 * @param p_arg             Pointer to event type database
 * @param p_event_type      Pointer to event type to insert
 * @return Insertion result
 * @retval OPS_SUCCESS               Insertion successful
 * @retval OPS_ERROR_ALREADY_EXISTS  Event type already exists
 * @retval OPS_ERROR_NO_MEMORY       Not enough memory to insert event type
 *
 */
static int event_type_db_insert_wrap(void *p_arg, const event_type_t *p_event_type)
{
    int status = event_type_db_insert(p_arg, p_event_type);
    switch (status)
    {
    case EVENT_TYPE_DB_NO_ERROR:
        return OPS_SUCCESS;
    case EVENT_TYPE_DB_ERROR_ALREADY_EXISTS:
        return OPS_ERROR_ALREADY_EXISTS;
    case EVENT_TYPE_DB_ERROR_NO_MEMORY:
        return OPS_ERROR_NO_MEMORY;
    default:
        return OPS_FAILURE;
    }
}

/**
 * @brief       Helper function to publish a DDM parameter value in specified @a entry.
 *
 * @param p_evm        Pointer to event manager instance.
 * @param p_entry      Pointer to DDM entry to publish.
 * @return Publish result
 * @retval OPS_SUCCESS       Publish successful
 * @retval OPS_FAILURE       Publish failed
 */
static int broker_publish(const event_manager_t *p_evm, const ddm_entry_t *p_entry)
{
    const void *p_data = NULL;
    size_t data_size = 0u;

    ddm_entry__read__value(p_entry, &p_data, &data_size);
    EVENT_NOTIFICATION_LOG(I,
                           "DDM entry %s0%s(0x%x) publish data %p, data_size %u",
                           ddm_entry__device_class(p_entry), ddm_entry__property(p_entry),
                           ddm_entry__parameter_id(p_entry),
                           p_data,
                           data_size);
    TRUE_CHECK_RETURNX(OPS_FAILURE, (p_data != NULL) && (data_size != 0));
    return connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                          ddm_entry__parameter_id(p_entry),
                                          p_data,
                                          data_size,
                                          p_evm->p_connector->connector_id,
                                          portMAX_DELAY) == true
               ? OPS_SUCCESS
               : OPS_FAILURE;
}

/**
 * @brief       Helper function to publish all changed DDM parameters owned by
 *              the event manager.
 *
 * @param p_evm    Pointer to event manager instance.
 * @return Publish result
 * @retval OPS_SUCCESS       Publish successful
 * @retval OPS_FAILURE       Publish failed
 */
static int evm_broker_publish_changed(event_manager_t *p_evm)
{
    int retval = OPS_SUCCESS;
    /* Do publish of changed values */
    for (size_t i = 0; i < ddm_store__occupied(&p_evm->owned_ddm_store); i++)
    {
        ddm_entry_t *p_ddm_entry;
        ddm_store__iterate(&p_evm->owned_ddm_store, i, &p_ddm_entry);
        if (ddm_entry__has_changed(p_ddm_entry) && ddm_entry__is_subscribed(p_ddm_entry))
        {
            retval = broker_publish(p_evm, p_ddm_entry);
            if (retval != OPS_SUCCESS)
            {
                EVENT_NOTIFICATION_LOG(E,
                                       "Failed to publish DDM entry %s0%s(0x%x)",
                                       ddm_entry__device_class(p_ddm_entry),
                                       ddm_entry__property(p_ddm_entry),
                                       ddm_entry__parameter_id(p_ddm_entry));
                break;
            }
            /* Regardless if publish was successful or not we are going to clear
             * `has_changed` flag.
             */
            ddm_entry__set__has_changed(p_ddm_entry, false);
        }
    }
    return retval;
}

/**
 * @brief       Helper function to subscribe to a DDM parameter specified in
 *              @a entry.
 *
 * @param p_evm        Pointer to event manager instance.
 * @param p_entry      Pointer to DDM entry to subscribe to.
 * @return Subscribe result
 * @retval OPS_SUCCESS       Subscribe successful
 * @retval OPS_FAILURE       Subscribe failed
 */
static int broker_subscribe(const event_manager_t *p_evm, const ddm_entry_t *p_entry)
{
    return connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                          ddm_entry__parameter_id(p_entry),
                                          NULL,
                                          0,
                                          p_evm->p_connector->connector_id,
                                          portMAX_DELAY) == true
               ? OPS_SUCCESS
               : OPS_FAILURE;
}

/**
 * @brief       Update NACK structure with IDs of unacknowledged event records.
 *
 * @param p_nack      Pointer to NACK structure to update.
 * @param p_er_db     Pointer to event record database.
 */
static void nack_update(nack_concrete_t *p_nack, const event_record_db_t *p_er_db)
{
    p_nack->nack = 0;

    size_t records = event_record_db_occupied(p_er_db);
    for (size_t i = 0u; i < records; i++)
    {
        const event_record_t *er = event_record_db_iterate(p_er_db, i);

        if (event_record_is_app_acknowledged(er) == false)
        {
            p_nack->ids[p_nack->nack++] = event_record_get_id(er);
        }
    }
}

/**
 * @brief       Calculate size of NACK structure in bytes.
 *
 * @param p_nack      Pointer to NACK structure.
 * @return Size of NACK structure in bytes.
 */
static size_t nack_sizeof(const nack_concrete_t *p_nack)
{
    return (uint8_t *)&p_nack->ids[p_nack->nack] - (uint8_t *)p_nack;
}

/* -- JSON parser/unparser -- */

/**
 * @brief       Check if data element string represents a keyword parameter.
 *
 * @param p_data_element_str   Data element string to check
 * @return If data element is a keyword parameter
 * @retval true if keyword parameter
 * @retval false if not a keyword parameter
 */
static bool parser_is_keyword_data_element(const char *p_data_element_str)
{
    return (p_data_element_str[0] == KEYWORD_PREFIX[0]);
}

/**
 * @brief       Check if data element string represents a trigger data element.
 *
 * @param p_data_element_str   Data element string to check
 * @return If data element is a trigger data element
 * @retval true if trigger data element
 * @retval false if not a trigger data element
 */
static bool parser_is_trigger_data_element(const char *p_data_element_str)
{
    if (strncmp(p_data_element_str, KEYWORD_TRIGGER_PREFIX, sizeof(KEYWORD_TRIGGER_PREFIX) - 1) != 0)
    {
        return false;
    }
    return true;
}

/**
 * @brief       Check if data element string represents a DDM data element.
 *
 * @param p_data_element_str   Data element string to check
 * @return If data element is a DDM data element
 * @retval true if DDM data element
 * @retval false if not a DDM data element
 */
static bool parser_is_parameter_data_element(const char *p_data_element_str)
{
    bool is_ddm = false;

    // Ensure this is not a keyword data element first and then check for digits
    if (parser_is_keyword_data_element(p_data_element_str) == false)
    {
        // All ddm parameters have a digit somewhere in the string, not at beginning
        // and not at end.
        is_ddm = false;
        size_t len = strlen(p_data_element_str);
        for (size_t i = 1; i < len - 1; i++)
        {
            if (p_data_element_str[i] >= '0' && p_data_element_str[i] <= '9')
            {
                is_ddm = true;
                break;
            }
        }
    }
    return is_ddm;
}

/**
 * @brief       Parse trigger data element string.
 *
 * Expected format: "KEYWORD_TRIGGER_PREFIX<trigger_number>"
 *
 * @param p_data_element_str   Data element string to parse
 * @param p_out_trigger        Parsed trigger number
 *
 * @return Parsing result
 * @retval OPS_SUCCESS           Parsing successful
 * @retval OPS_ERROR_D_FIELD     Parsing error due to invalid trigger data element
 */
static int parser_parse_trigger_data_element(const char *p_data_element_str, int32_t *p_out_trigger)
{
    int32_t trigger = -1;
    if (sscanf(&p_data_element_str[sizeof(KEYWORD_TRIGGER_PREFIX) - 1], "%d", &trigger) != 1)
    {
        EVENT_NOTIFICATION_LOG(E, "Invalid trigger data element '%s'", p_data_element_str);
        return OPS_ERROR_D_FIELD;
    }
    *p_out_trigger = trigger;
    return OPS_SUCCESS;
}

/**
 * @brief       Parse DDM data element string.
 *
 * Expected format: "<ddm_parameter_id>"
 *
 * @param p_data_element_str   Data element string to parse
 * @param p_out_parameter      Parsed DDM parameter ID
 *
 * @return Parsing result
 * @retval OPS_SUCCESS           Parsing successful
 * @retval OPS_ERROR_D_FIELD     Parsing error due to invalid DDM data element
 */
static int parser_parse_parameter_data_element(const char *p_data_element_str, uint32_t *p_out_parameter)
{
    uint32_t parameter_id;
    int parameter_index = ddm2_parse_parameter_string(&parameter_id,
                                                      p_data_element_str,
                                                      strnlen(p_data_element_str, DDMP2_MAX_VALUE_SIZE));
    if (parameter_index == -1)
    {
        EVENT_NOTIFICATION_LOG(E, "Unknown DDM parameter name '%s'", p_data_element_str);
        return OPS_ERROR_D_FIELD;
    }
    *p_out_parameter = parameter_id;
    return OPS_SUCCESS;
}

/**
 * @brief       Parse optional field (field_offset / field_size) from data element string.
 *
 * @param p_data_element_str   Data element string to parse
 * @param p_field_offset       Parsed field offset (set to 0 if not present)
 * @param p_field_size         Parsed field size (set to 0 if not present)
 *
 * @return Parsing result
 * @retval OPS_SUCCESS           Parsing successful
 * @retval OPS_ERROR_D_FIELD     Parsing error due to invalid field offset/size
 */
static int parser_parse_optional_field(const char *p_data_element_str,
                                       int32_t *p_field_offset,
                                       int32_t *p_field_size)
{
    char *p_sep1 = strchr(p_data_element_str, ':');
    char *p_sep2 = NULL;
    *p_field_offset = 0;
    *p_field_size = 0;

    // Parse optional field offset and size if present
    if (p_sep1 != NULL)
    {
        // There is at least one ':', parse field_offset
        *p_sep1 = '\0';
        p_sep2 = strchr(p_sep1 + 1, ':');
        int32_t offset = 0;
        int32_t size = 0;

        if (sscanf(p_sep1 + 1, "%d", &offset) != 1)
        {
            EVENT_NOTIFICATION_LOG(E, "Invalid field offset in data element '%s'", p_data_element_str);
            return OPS_ERROR_D_FIELD;
        }
        *p_field_offset = offset;

        if (p_sep2 != NULL)
        {
            // There is a second ':', parse field_size
            if (sscanf(p_sep2 + 1, "%d", &size) != 1)
            {
                EVENT_NOTIFICATION_LOG(E, "Invalid field size in data element '%s'", p_data_element_str);
                return OPS_ERROR_D_FIELD;
            }
            *p_field_size = size;
        }
    }
    return OPS_SUCCESS;
}

/**
 * @brief       Get integer field from JSON object.
 *
 * @param p_json_object     Pointer to JSON object.
 * @param p_field_name      Name of the field to get.
 * @param p_out_value       Pointer to output integer value.
 * @return If field was successfully retrieved and is an integer.
 * @retval true if field was successfully retrieved and is an integer.
 * @retval false otherwise.
 */
static bool parser_get_int_field(cJSON *p_json_object, const char *p_field_name, int32_t *p_out_value)
{
    bool retval = false;
    cJSON *p_field = cJSON_GetObjectItemCaseSensitive(p_json_object, p_field_name);
    if (p_field != NULL)
    {
        if (cJSON_IsNumber(p_field))
        {
            *p_out_value = p_field->valueint;
            retval = true;
        }
        else
        {
            EVENT_NOTIFICATION_LOG(W, "Field '%s' is not a number", p_field_name);
        }
    }
    else
    {
        EVENT_NOTIFICATION_LOG(W, "Field '%s' is missing", p_field_name);
    }
    return retval;
}

/**
 * @brief       Get string field from JSON object.
 *
 * @param p_json_object     Pointer to JSON object.
 * @param p_field_name      Name of the field to get.
 * @param p_out_value       Pointer to output string value.
 * @return If field was successfully retrieved and is a string.
 * @retval true if field was successfully retrieved and is a string.
 * @retval false otherwise.
 */
static bool parser_get_string_field(cJSON *p_json_object, const char *p_field_name, const char **p_out_value)
{
    bool retval = false;
    cJSON *p_field = cJSON_GetObjectItemCaseSensitive(p_json_object, p_field_name);
    if (p_field != NULL)
    {
        if (cJSON_IsString(p_field))
        {
            *p_out_value = p_field->valuestring;
            retval = true;
        }
        else
        {
            EVENT_NOTIFICATION_LOG(W, "Field '%s' is not a string", p_field_name);
        }
    }
    else
    {
        EVENT_NOTIFICATION_LOG(W, "Field '%s' is missing", p_field_name);
    }
    return retval;
}

/**
 * @brief       Get array field from JSON object.
 *
 * @param p_json_object     Pointer to JSON object.
 * @param p_field_name      Name of the field to get.
 * @param p_out_array       Pointer to output array value.
 * @return If field was successfully retrieved and is an array.
 * @retval true if field was successfully retrieved and is an array.
 * @retval false otherwise.
 */
static bool parser_get_array_field(cJSON *p_json_object, const char *p_field_name, cJSON **p_out_array)
{
    bool retval = false;
    cJSON *p_field = cJSON_GetObjectItemCaseSensitive(p_json_object, p_field_name);
    if (p_field != NULL)
    {
        if (cJSON_IsArray(p_field))
        {
            *p_out_array = p_field;
            retval = true;
        }
        else
        {
            EVENT_NOTIFICATION_LOG(W, "Field '%s' is not an array", p_field_name);
        }
    }
    else
    {
        EVENT_NOTIFICATION_LOG(W, "Field '%s' is missing", p_field_name);
    }
    return retval;
}

/**
 * @brief       Validate JSON root object.
 *
 * A valid root object is non-NULL and is of type object.
 *
 * @param p_json_root      Pointer to JSON root object.
 * @return If JSON root is valid.
 * @retval true if JSON root is valid.
 * @retval false otherwise.
 */
static bool parser_is_valid_root(cJSON *p_json_root)
{
    bool retval = false;

    if (p_json_root != NULL)
    {
        // Root must be an object
        if (cJSON_IsObject(p_json_root))
        {
            retval = true;
        }
        else
        {
            EVENT_NOTIFICATION_LOG(E, "Event types JSON root is not an object");
        }
    }
    else
    {
        EVENT_NOTIFICATION_LOG(E, "Event types JSON parsing error");
    }
    return retval;
}

/**
 * @brief       Parse event type data element from JSON.
 *
 * The data element is expected to be a string representing either a trigger
 * data element or a DDM data element, optionally followed by field offset
 * and size. Example data element strings:
 * - "%trigger.1:0:4" (trigger data element with offset 0 and size 4)
 * - "CLASS1234NAME:2:2" (DDM data element with parameter ID CLASS1234NAME, offset 2 and size 2)
 *
 * @param p_evm            Pointer to event manager instance.
 * @param p_data_element   Pointer to JSON data element.
 * @param p_event_type     Pointer to event type to add data element to.
 *
 * @return Parsing result
 * @retval OPS_SUCCESS                    Parsing successful
 * @retval OPS_ERROR_JSON_INVALID         Parsing error due to invalid JSON
 * @retval OPS_ERROR_JSON_MISSING_FIELDS  Parsing error due to missing fields
 */
static int parser_parse_event_type_values_element(event_manager_t *p_evm, cJSON *p_data_element, event_type_t *p_event_type)
{
    int retval = OPS_FAILURE;

    if (cJSON_IsString(p_data_element))
    {
        const char *p_data_element_str = p_data_element->valuestring;
        int32_t field_offset = 0;
        int32_t field_size = 0;

        // Parse optional field offset and size if present
        if (parser_parse_optional_field(p_data_element_str, &field_offset, &field_size) == OPS_SUCCESS)
        {
            // Successfully parsed optional field, see if we have trigger or DDM data element
            if (parser_is_trigger_data_element(p_data_element_str))
            {
                int32_t trigger;
                if (parser_parse_trigger_data_element(p_data_element_str, &trigger) == OPS_SUCCESS)
                {
                    event_type_add_trigger_data(p_event_type, trigger, field_offset, field_size);
                    retval = OPS_SUCCESS;
                }
                else
                {
                    EVENT_NOTIFICATION_LOG(E, "Failed to parse trigger data element '%s'", p_data_element_str);
                    retval = OPS_ERROR_JSON_INVALID;
                }
            }
            else if (parser_is_parameter_data_element(p_data_element_str))
            {
                uint32_t parameter;
                if (parser_parse_parameter_data_element(p_data_element_str, &parameter) == OPS_SUCCESS)
                {
                    event_type_add_parameter_data(p_event_type, parameter, field_offset, field_size);
                    retval = OPS_SUCCESS;
                }
                else
                {
                    EVENT_NOTIFICATION_LOG(E, "Failed to parse parameter data element '%s'", p_data_element_str);
                    retval = OPS_ERROR_JSON_INVALID;
                }
            }
            else
            {
                EVENT_NOTIFICATION_LOG(E, "Unknown data element type '%s'", p_data_element_str);
                retval = OPS_ERROR_JSON_MISSING_FIELDS;
            }
        }
        else
        {
            EVENT_NOTIFICATION_LOG(E, "Failed to parse optional field in data element '%s'", p_data_element_str);
            retval = OPS_ERROR_JSON_INVALID;
        }
    }
    else
    {
        EVENT_NOTIFICATION_LOG(E, "Data element in event type id %d is not a string", event_type_get_type(p_event_type));
        retval = OPS_ERROR_JSON_INVALID;
    }

    return retval;
}

/**
 * @brief       Parse event type array element from JSON.
 *
 * @param p_evm            Pointer to event manager instance.
 * @param p_type_element   Pointer to JSON type element.
 * @param p_inserter       Pointer to inserter function to insert parsed event type.
 * @param p_inserter_arg   Pointer to argument for inserter function.
 *
 * @return Parsing result
 * @retval OPS_SUCCESS                    Parsing successful
 * @retval OPS_ERROR_JSON_INVALID         Parsing error due to invalid JSON
 * @retval OPS_ERROR_JSON_MISSING_FIELDS  Parsing error due to missing fields
 */
static int parser_parse_event_type_array_element(event_manager_t *p_evm,
                                                 cJSON *p_type_element,
                                                 parser_inserter_fn_t *p_inserter,
                                                 void *p_inserter_arg)
{
    int retval = OPS_SUCCESS;

    if (cJSON_IsObject(p_type_element))
    {
        event_type_t event_type;
        event_type_init(&event_type);

        int32_t type_id;
        int32_t config;
        cJSON *p_values_array;

        // Parse mandatory fields: id, config, data
        bool has_valid_id = parser_get_int_field(p_type_element, "id", &type_id);
        bool has_valid_config = parser_get_int_field(p_type_element, "config", &config);
        bool has_valid_values_array = parser_get_array_field(p_type_element, "values", &p_values_array);

        if (has_valid_id && has_valid_config && has_valid_values_array)
        {
            event_type_set_type(&event_type, type_id);
            event_type_set_config(&event_type, config);

            // Parse data elements
            retval = true;  // Assume success unless a data element fails
            int data_array_size = cJSON_GetArraySize(p_values_array);
            if (data_array_size != 0)
            {
                for (int j = 0; j < data_array_size; j++)
                {
                    cJSON *p_values_element = cJSON_GetArrayItem(p_values_array, j);

                    retval = parser_parse_event_type_values_element(p_evm, p_values_element, &event_type);
                    if (retval != OPS_SUCCESS)
                    {
                        EVENT_NOTIFICATION_LOG(E, "Failed to parse data element at index %d in event type id %d", j, type_id);
                        break;
                    }
                }
                if (retval == OPS_SUCCESS)
                {
                    // Insert parsed event type using provided inserter function
                    if (p_inserter(p_inserter_arg, &event_type) != OPS_SUCCESS)
                    {
                        EVENT_NOTIFICATION_LOG(E, "Failed to insert event type id %d", type_id);
                        retval = OPS_ERROR_JSON_INVALID;
                    }
                }
            }
            else
            {
                EVENT_NOTIFICATION_LOG(E, "Data array in event type id %d is empty", type_id);
                retval = OPS_ERROR_JSON_MISSING_FIELDS;
            }
        }
        else
        {
            EVENT_NOTIFICATION_LOG(E, "Missing or invalid mandatory fields in event type element");
            retval = OPS_ERROR_JSON_MISSING_FIELDS;
        }
        event_type_terminate(&event_type);
    }
    else
    {
        EVENT_NOTIFICATION_LOG(E, "Type array element is not an object");
        retval = OPS_ERROR_JSON_INVALID;
    }
    return retval;
}

/**
 * @brief       Parse event type array from JSON.
 *
 * Expected JSON format:
 * [
 *     "PROD0NAME",
 *     "AC0TTEMP:4:12",
 *     "AC0ITEMP:8",
 *     "AC0FAN:0:16"
 * ]
 *
 *
 * @param p_evm            Pointer to event manager instance.
 * @param p_type_array     Pointer to JSON type array.
 * @param p_inserter       Pointer to inserter function to insert parsed event types.
 * @param p_inserter_arg   Pointer to argument for inserter function.
 *
 * @return Parsing result
 * @retval OPS_SUCCESS                    Parsing successful
 * @retval OPS_ERROR_JSON_INVALID         Parsing error due to invalid JSON
 * @retval OPS_ERROR_JSON_MISSING_FIELDS  Parsing error due to missing fields
 */
static int parser_parse_event_type_array(event_manager_t *p_evm,
                                         cJSON *p_type_array,
                                         parser_inserter_fn_t *p_inserter,
                                         void *p_inserter_arg)
{
    int retval = OPS_SUCCESS;

    // Parse each event type in the type array
    int type_array_size = cJSON_GetArraySize(p_type_array);
    if (type_array_size != 0)
    {
        for (int i = 0; i < type_array_size; i++)
        {
            cJSON *type_element = cJSON_GetArrayItem(p_type_array, i);

            retval = parser_parse_event_type_array_element(p_evm, type_element, p_inserter, p_inserter_arg);
            if (retval != OPS_SUCCESS)
            {
                EVENT_NOTIFICATION_LOG(E, "Failed to parse event type array element at index %d", i);
                break;
            }
        }
    }
    else
    {
        EVENT_NOTIFICATION_LOG(E, "Event type array is empty");
        retval = OPS_ERROR_JSON_MISSING_FIELDS;
    }
    return retval;
}

/**
 * @brief       Parse event types in format version 2 from JSON.
 *
 * Expected JSON format:
 * {
 *     "version": 100,
 *     "key": "climate",
 *     "type":
 *     [
 *         {
 *             "id": 1234,
 *             "config": 3,
 *             "data":
 *             [
 *                 "PROD0NAME",
 *                 "AC0TTEMP:4:12",
 *                 "AC0ITEMP:8",
 *                 "AC0FAN:0:16"
 *             ]
 *         },
 *         {
 *             "id": 1235,
 *             "config": 3,
 *             "data":
 *             [
 *                 "PROD0NAME",
 *                 "%trigger.0",
 *                 "%trigger.1"
 *             ]
 *         }
 *     ]
 * }
 */
static int parser_parse_event_type_format(event_manager_t *p_evm,
                                          cJSON *p_json_root,
                                          parser_inserter_fn_t *p_inserter,
                                          void *p_inserter_arg)
{
    int retval = OPS_SUCCESS;
    const char *p_version;
    const char *p_key;
    cJSON *p_type_array;

    // Check mandatory fields for format version 2
    bool has_valid_version = parser_get_string_field(p_json_root, "version", &p_version);
    bool has_valid_key = parser_get_string_field(p_json_root, "key", &p_key);
    bool has_valid_type_array = parser_get_array_field(p_json_root, "type", &p_type_array);

    if (has_valid_version && has_valid_key && has_valid_type_array)
    {
        // Copy version and key to event manager
        EVENT_NOTIFICATION_LOG(I, "Event types format version 2 detected: version='%s', key='%s'", p_version, p_key);
        snprintf(p_evm->version, sizeof(p_evm->version) - 1, "%s", p_version);
        p_evm->version[sizeof(p_evm->version) - 1] = '\0';
        snprintf(p_evm->product_name, sizeof(p_evm->product_name) - 1, "%s", p_key);
        p_evm->product_name[sizeof(p_evm->product_name) - 1] = '\0';

        // Parse event type array
        retval = parser_parse_event_type_array(p_evm, p_type_array, p_inserter, p_inserter_arg);
    }
    else
    {
        EVENT_NOTIFICATION_LOG(E, "Missing or invalid mandatory fields for event type format version 2");
        retval = OPS_ERROR_JSON_MISSING_FIELDS;
    }
    return retval;
}

/**
 * @brief       Parse the input JSON string into event types (new format)
 *
 * @param p_evm            Pointer to event manager instance.
 * @param p_json_string    Pointer to input JSON string.
 * @param p_inserter       Pointer to inserter function to insert parsed event types.
 * @param p_inserter_arg   Pointer to argument for inserter function.
 *
 * @return Parsing result
 * @retval OPS_SUCCESS                    Parsing successful
 * @retval OPS_ERROR_JSON_INVALID         Parsing error due to invalid JSON
 * @retval OPS_ERROR_JSON_MISSING_FIELDS  Parsing error due to missing fields
 */
static int parser_parse_event_type(event_manager_t *p_evm,
                                   const char *p_json_string,
                                   parser_inserter_fn_t *p_inserter,
                                   void *p_inserter_arg)
{
    cJSON *p_json_root;
    int retval = OPS_SUCCESS;

    p_json_root = cJSON_Parse(p_json_string);

    if (parser_is_valid_root(p_json_root))
    {
        int32_t format_version = 0;
        // Check format version
        if (parser_get_int_field(p_json_root, "format", &format_version))
        {
            if (format_version == 2)
            {
                // New format parsing
                retval = parser_parse_event_type_format(p_evm, p_json_root, p_inserter, p_inserter_arg);
            }
            else
            {
                EVENT_NOTIFICATION_LOG(E, "Invalid JSON format version");
                retval = OPS_ERROR_JSON_INVALID;
            }
        }
        else
        {
            EVENT_NOTIFICATION_LOG(E, "Invalid or missing JSON format field");
            retval = OPS_ERROR_JSON_INVALID;
        }
    }
    cJSON_Delete(p_json_root);
    return retval;
}

/* -- DDM handling -- */

/**
 * @brief       Convert evm0ack input data (int32) to appropriate response.
 *
 * This function affects the following DDM parameters: EVM0ACK, EVM0NACK.
 *
 * @param p_arg             Pointer to event manager instance.
 * @param p_in_frame        Pointer to input DDM frame.
 * @param p_evm0ack         Pointer to DDM entry for EVM0ACK parameter
 */
static void convert_evm0ack(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0ack)
{
    /* Expected input type is int32_t */
    if (ddmp2_value_size(p_in_frame) != sizeof(int32_t))
    {
        /* We got an incomplete SET, ignore the value, do not generate publish? */
        LOG(W, "Expected frame with int32 value");
    }
    else
    {
        event_manager_t *p_evm = p_arg;

        /* It seems we got a complete frame, process it. */
        int32_t acknowledge_id = p_in_frame->frame.set.value.int32;

        EVENT_NOTIFICATION_LOG(I, "Acknowledging Event Record with ID: %d", acknowledge_id);
        event_record_t *p_event_record = event_record_db_find_by_id(&p_evm->event_record_db,
                                                                    (uint32_t)acknowledge_id);
        if (p_event_record != NULL)
        {
            if (event_record_is_app_acknowledged(p_event_record) == false)
            {
                nack_concrete_t nack;

                /* This event record was not acknowledged, so we acknowledge it. */
                p_evm->acknowledged = acknowledge_id;
                event_manager_save_acknowledged(p_evm);
                /* Update the event record. */
                event_record_set_app_acknowledged(p_event_record, true);
                event_manager_save_records(p_evm);
                /* Update and publish EVM0NACK. */
                nack_update(&nack, &p_evm->event_record_db);
                ddm_entry_t *p_evm0nack = ddm_store__access(&p_evm->owned_ddm_store, EVM0NACK);
                bool has_changed = ddm_entry__set__value_struct(p_evm0nack, &nack, nack_sizeof(&nack));
                ddm_entry__set__has_changed(p_evm0nack, has_changed);
                /* Notify event listener */
                if (p_evm->p_event_listener != NULL)
                {
                    p_evm->p_event_listener(p_evm->p_event_listener_arg,
                                            EVENT_MANAGER_LISTENER_EVENT_ACK,
                                            acknowledge_id);
                }
            }
            else
            {
                /* This event record was already acknowledged, do nothing. */
                EVENT_NOTIFICATION_LOG(I, "Event record %d is already acknowledged", acknowledge_id);
            }
        }
        else
        {
            LOG(W, "Acknowledging non-existing Event Record with ID: %d", acknowledge_id);
        }
        /* Always generate publish of EVM0ACK */
        ddm_entry__set__value_i32(p_evm0ack, p_evm->acknowledged);
        ddm_entry__set__has_changed(p_evm0ack, true);
    }
}

/**
 * @brief       Convert evm0cmd input data (int32) to appropriate response.
 *
 * This function affects the following DDM parameters: EVM0CMD, EVM0ACK, EVM0NACK.
 *
 * @param p_arg        Pointer to event manager instance.
 * @param p_in_frame   Pointer to input DDM frame.
 * @param p_evm0cmd    Pointer to DDM entry for EVM0CMD parameter
 *
 * @note        Clearing all events will NOT reset the event ID counter to zero.
 */
static void convert_evm0cmd(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0cmd)
{
    /* Expected input type is int32_t */
    if (ddmp2_value_size(p_in_frame) != sizeof(int32_t))
    {
        /* We got an incomplete SET, ignore the value, do not generate publish? */
        LOG(W, "Expected frame with int32 value");
    }
    else
    {
        event_manager_t *p_evm = p_arg;

        /* It seems we got a complete frame, process it. */
        switch (p_in_frame->frame.set.value.int32)
        {
        case EVM0CMD_ACKNOWLEDGE_ALL:
        {
            nack_concrete_t nack;

            EVENT_NOTIFICATION_LOG(I, "Acknowledging all events");

            /* Always publish EVM0CMD */
            ddm_entry__set__value_i32(p_evm0cmd, EVM0CMD_ACKNOWLEDGE_ALL);
            ddm_entry__set__has_changed(p_evm0cmd, true);
            /* Iterate over record events and set acknowledge */
            for (size_t i = 0u; i < event_record_db_occupied(&p_evm->event_record_db); i++)
            {
                event_record_t *p_event_record;
                p_event_record = event_record_db_iterate(&p_evm->event_record_db, i);
                if (event_record_is_app_acknowledged(p_event_record) == false)
                {
                    event_record_set_app_acknowledged(p_event_record, true);
                    p_evm->acknowledged = p_event_record->id;
                }
            }
            /* If at least one record has changed */
            event_manager_save_records(p_evm);
            /* We also need to generate publish of evm0nack parameter */
            nack_update(&nack, &p_evm->event_record_db);
            ddm_entry_t *p_evm0nack = ddm_store__access(&p_evm->owned_ddm_store, EVM0NACK);
            bool has_changed = ddm_entry__set__value_struct(p_evm0nack, &nack, nack_sizeof(&nack));
            ddm_entry__set__has_changed(p_evm0nack, has_changed);
            /* We also need to generate publish of evm0ack parameter */
            ddm_entry_t *p_evm0ack = ddm_store__access(&p_evm->owned_ddm_store, EVM0ACK);
            has_changed = ddm_entry__set__value_i32(p_evm0ack, p_evm->acknowledged);
            ddm_entry__set__has_changed(p_evm0ack, has_changed);
            /* After setting and publishing save the state. */
            event_manager_save_acknowledged(p_evm);
            if (p_evm->p_event_listener != NULL)
            {
                p_evm->p_event_listener(p_evm->p_event_listener_arg,
                                        EVENT_MANAGER_LISTENER_EVENT_ACK_ALL,
                                        0);
            }
            break;
        }
        case EVM0CMD_CLEAR_ALL:
        {
            nack_concrete_t nack;

            EVENT_NOTIFICATION_LOG(I, "Clearing all events");

            /* Always publish EVM0CMD */
            ddm_entry__set__value_i32(p_evm0cmd, EVM0CMD_CLEAR_ALL);
            ddm_entry__set__has_changed(p_evm0cmd, true);
            /* The following will:
             * - Delete all event records.
             * - Clear not_acknowleded counter
             * - Clear acknowleded counter
             *  -Clear last event ID, TODO: Should we reset the event_id counter?
             */
            event_manager_set_defaults_keep_event_id(p_evm);
            event_manager_save_records(p_evm);
            /* We also need to generate publish of evm0id parameter */
            ddm_entry_t *p_evm0id = ddm_store__access(&p_evm->owned_ddm_store, EVM0ID);
            bool has_changed = ddm_entry__set__value_i32(p_evm0id, p_evm->event_id);
            ddm_entry__set__has_changed(p_evm0id, has_changed);
            /* After setting and publishing save the state. */
            event_manager_save_event_id(p_evm);
            /* We also need to generate publish of evm0nack parameter */
            nack_update(&nack, &p_evm->event_record_db);
            ddm_entry_t *p_evm0nack = ddm_store__access(&p_evm->owned_ddm_store, EVM0NACK);
            has_changed = ddm_entry__set__value_struct(p_evm0nack, &nack, nack_sizeof(&nack));
            ddm_entry__set__has_changed(p_evm0nack, has_changed);
            /* We also need to generate publish of evm0ack parameter */
            ddm_entry_t *p_evm0ack = ddm_store__access(&p_evm->owned_ddm_store, EVM0ACK);
            has_changed = ddm_entry__set__value_i32(p_evm0ack, p_evm->acknowledged);
            ddm_entry__set__has_changed(p_evm0ack, has_changed);
            /* After setting and publishing save the state. */
            event_manager_save_acknowledged(p_evm);
            /* Notify listener */
            if (p_evm->p_event_listener != NULL)
            {
                p_evm->p_event_listener(p_evm->p_event_listener_arg,
                                        EVENT_MANAGER_LISTENER_EVENT_DELETE_ALL,
                                        0);
            }
            break;
        }
        default:
            LOG(W, "Unrecognized command %d", p_in_frame->frame.set.value.int32);
            /* In case of invalid input just publish the current value */
            ddm_entry__set__has_changed(p_evm0cmd, true);
            break;
        }
    }
}

/**
 * @brief       Convert evm0rec input data (int32) to appropriate response.
 *
 * This function affects the following DDM parameters: EVM0REC.
 *
 * @param p_arg        Pointer to event manager instance.
 * @param p_in_frame   Pointer to input DDM frame.
 * @param p_evm0rec    Pointer to DDM entry for EVM0REC parameter
 */
static void convert_evm0rec(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0rec)
{
    /* Expected input type is uint32_t */
    if (ddmp2_value_size(p_in_frame) != sizeof(uint32_t))
    {
        /* We got an incomplete SET, ignore the value, do not generate publish? */
        LOG(W, "Expected frame with uint32 value");
    }
    else
    {
        /* It seems we got a complete frame, process it. */
        event_manager_t *p_evm = p_arg;
        char buffer[DDMP2_MAX_VALUE_SIZE];
        const event_record_t *p_event_record = event_record_db_find_by_id(&p_evm->event_record_db,
                                                                          p_in_frame->frame.set.value.uint32);
        if (p_event_record != NULL)
        {
#if defined(CONNECTOR_MQTT)
            const char *p_thing_name = connector_mqtt_get_thing();
#else
            const char *p_thing_name = "null";
#endif
            if (p_thing_name != NULL)
            {
                int status = make_json_event_record(p_event_record, p_evm->product_name, p_thing_name, buffer, sizeof(buffer));
                if (status != MAKE_JSON_NO_ERROR)
                {
                    /* Generate empty JSON as an error response */
                    strncpy(buffer, "{}", sizeof(buffer));
                }
            }
            else
            {
                /* We don't have thing name, generate empty JSON as an error response */
                strncpy(buffer, "{}", sizeof(buffer));
                LOG(W, "MQTT thing name is NULL, cannot generate JSON for event record %u", p_evm->event_id);
            }
        }
        else
        {
            /* We got a event_id that we don't have in event_type database then
             * generate empty JSON as an error response.
             */
            LOG(W, "Unknown event record ID %u in EVM0REC", p_in_frame->frame.set.value.uint32);
            strncpy(buffer, "{}", sizeof(buffer));
        }
        /* Store the value into OUT type ddmp storage, evm0rec */
        ddm_entry__set__value(p_evm0rec, buffer, strnlen(buffer, sizeof(buffer)));
        ddm_entry__set__has_changed(p_evm0rec, true);
    }
}

/**
 * @brief       Convert evm0trig input data (int32) to appropriate response.
 *
 * This function affects the following DDM parameters:
 * EVM0TRIG - Number of event record ID,
 * EVM0ID - Number of event record ID,
 * EVM0NACK - Increased number of not acknowledged events,
 * EVN0ID - Number of event record.
 *
 * This function may also create new event records in event record database.
 *
 * @param p_arg        Pointer to event manager instance.
 * @param p_in_frame   Pointer to input DDM frame.
 * @param p_evm0trig   Pointer to DDM entry for EVM0TRIG parameter
 */
static void convert_evm0trig(void *p_arg, const DDMP2_FRAME *p_in_frame, ddm_entry_t *p_evm0trig)
{
    size_t evm0trig_input_size = ddmp2_value_size(p_in_frame);
    /* Expected input type is uint32_t*/
    if (evm0trig_input_size < sizeof(EVM0TRIG_T))
    {
        /* We got an incomplete SET, ignore the value, do not generate publish? */
        LOG(W, "Expected frame with structure");
    }
    else
    {
        /* It seems we got a complete frame, process it. */
        event_manager_t *p_evm = p_arg;
        event_record_t event_record;

        EVENT_NOTIFICATION_LOG(I, "Processing EVM0TRIG input frame with size %u", (unsigned)evm0trig_input_size);
        /* Extract event_type ID from input frame */
        const EVM0TRIG_T *p_evm0trig_input = (const EVM0TRIG_T *)p_in_frame->frame.set.value.raw;
        /* Find event_type structure by event_type id */
        const event_type_t *p_event_type = event_type_db_find_by_type(&p_evm->event_type_db,
                                                                      p_evm0trig_input->type);
        if (p_event_type == NULL)
        {
            LOG(W, "Unknown event type 0x%x in EVM0TRIG", p_evm0trig_input->type);
            /* We got a event_type that we don't have in event_type database. Just
             * ignore the value, but generate publish, or not?
             */
            ddm_entry__set__has_changed(p_evm0trig, true);
        }
        else
        {
            nack_concrete_t nack;
            bool is_record_finalized = true;  // Assume we will finalize the record (fill in all data)
            event_id_t next_event_id = p_evm->event_id + 1;

            EVENT_NOTIFICATION_LOG(I, "Creating event record of type %d(0x%x) with next event ID %d with data size %u",
                                   event_type_get_type(p_event_type),
                                   event_type_get_type(p_event_type),
                                   next_event_id,
                                   event_type_get_data_size(p_event_type));
            /* Create event record with current DDM values and event ID + 1 */
            event_record_init(&event_record, next_event_id, event_type_get_type(p_event_type));
            for (size_t i = 0u; i < event_type_get_data_size(p_event_type); i++)
            {
                if (event_type_is_data_trigger(p_event_type, i))
                {
                    /* Fetch trigger data from input frame */
                    uint32_t trigger_id = event_type_get_trigger_id(p_event_type, i);
                    EVENT_NOTIFICATION_LOG(I, "Getting trigger ID %u", trigger_id);
                    uint32_t max_index = (evm0trig_input_size - sizeof(EVM0TRIG_T)) / sizeof(((EVM0TRIG_T *)0)->data[0]);
                    if (trigger_id >= max_index)
                    {
                        EVENT_NOTIFICATION_LOG(W, "Inconsistent internal structures, missing trigger data, requested ID %u, max index %u",
                                               trigger_id,
                                               max_index);
                        is_record_finalized = false;
                        break;
                    }
                    int32_t trigger_data = p_evm0trig_input->data[trigger_id];
                    size_t field_offset = event_type_get_trigger_field_offset(p_event_type, i);
                    size_t field_size = event_type_get_trigger_field_size(p_event_type, i);
                    event_record_add_trigger_data(&event_record, trigger_data, field_offset, field_size);
                }
                else if (event_type_is_data_parameter(p_event_type, i))
                {
                    /* Fetch ddm_entry from other_ddm_store */
                    uint32_t ddm_parameter = event_type_get_parameter_id(p_event_type, i);
                    EVENT_NOTIFICATION_LOG(I, "Getting ddm parameter 0x%x", ddm_parameter);
                    ddm_entry_t *p_ddm_entry = ddm_store__access(&p_evm->other_ddm_store, ddm_parameter);
                    if (p_ddm_entry == NULL)
                    {
                        /* This should not happen, we already created all entries specified
                         * in event_types.
                         */
                        EVENT_NOTIFICATION_LOG(W, "Inconsistent internal structures, missing ddm entry!");
                        is_record_finalized = false;
                        break;
                    }
                    EVENT_NOTIFICATION_LOG(I, "Found ddm parameter 0x%x with size %u bytes", ddm_parameter, ddm_entry__value_size(p_ddm_entry));
                    size_t field_offset = event_type_get_parameter_field_offset(p_event_type, i);
                    size_t field_size = event_type_get_parameter_field_size(p_event_type, i);
                    event_record_add_parameter_data(&event_record, p_ddm_entry, field_offset, field_size);
                }
            }
            if (is_record_finalized)
            {
                event_record_db_insert(&p_evm->event_record_db, &event_record);
                /* Generate event ID */
                p_evm->event_id = next_event_id;
#if defined(CONNECTOR_MQTT)
                /* Send all pending event record to MQTT connector, even the stale ones. */
                if (mqtt_is_connected_cached(p_evm))
                {
                    // At this point we have added the new event record to database, so send all (including the new one)
                    // We are not interested in the return value here since we always save the records below.
                    mqtt_send_all_event_records(p_evm);
                }
                else
                {
                    LOG(I, "MQTT not connected, not sending event record ID %d", p_evm->event_id);
                }
#endif
                event_manager_save_records(p_evm);
                /* Store the value into OUT type ddmp storage, EVM0TRIG */
                ddm_entry__set__value_i32(p_evm0trig, p_evm->event_id);
                ddm_entry__set__has_changed(p_evm0trig, true);
                /* We also need to generate publish of EVM0ID parameter */
                ddm_entry_t *p_evm0id = ddm_store__access(&p_evm->owned_ddm_store, EVM0ID);
                bool has_changed = ddm_entry__set__value_i32(p_evm0id, p_evm->event_id);
                ddm_entry__set__has_changed(p_evm0id, has_changed);
                event_manager_save_event_id(p_evm);
                /* We also need to generate publish of EVM0NACK parameter */
                nack_update(&nack, &p_evm->event_record_db);
                ddm_entry_t *p_evm0nack = ddm_store__access(&p_evm->owned_ddm_store, EVM0NACK);
                has_changed = ddm_entry__set__value_struct(p_evm0nack, &nack, nack_sizeof(&nack));
                ddm_entry__set__has_changed(p_evm0nack, has_changed);
                /* Notify listener */
                if (p_evm->p_event_listener != NULL)
                {
                    p_evm->p_event_listener(p_evm->p_event_listener_arg,
                                            EVENT_MANAGER_LISTENER_EVENT_NEW,
                                            p_evm->event_id);
                }
            }
            /* Terminate working copy of event record. */
            event_record_terminate(&event_record);
        }
    }
}

/**
 * @brief       Register the DDMP2 class instance with broker
 *
 * @param       ddm_class     DDM class which needs to be registerd.
 * @param       p_connector   connector instance
 * @return      Operation error if the operation failed, ddm_class instance
 *              otherwise.
 */
static int32_t handle_owned_parameter_register(uint32_t ddm_class, const CONNECTOR *p_connector)
{
    int instance;

    /* We want to be the owner of the parameters. Request an instance then
     * publish the available DDM parameter.
     */
    instance = broker_register_instance(&ddm_class, p_connector->connector_id);

    return (int32_t)instance;
}

/**
 * @brief       Handle set frames for our own DDM parameters.
 *
 * This function will:
 * - Get the DDM from DDM store
 * - Store the value into DDM according to DDM in_type
 *
 * @param       p_evm           Pointer to event_manager_t context structure.
 * @param       p_ddmp2_frame   Pointer to received DDMP2 frame.
 */
static void handle_owned_parameter_set(event_manager_t *p_evm,
                                       const DDMP2_FRAME *p_ddmp2_frame)
{
    ddm_entry_t *p_ddm_entry;

    p_ddm_entry = ddm_store__access(&p_evm->owned_ddm_store, p_ddmp2_frame->frame.set.parameter);

    if (p_ddm_entry == NULL)
    {
        /* Somebody tried to set a value to something that we haven't created */
        LOG(W, "%s: set non-existing DDM: 0x%x",
            p_evm->p_connector->name,
            p_ddmp2_frame->frame.set.parameter);
    }
    else
    {
        /* We need to do proper conversion from DDM frame (IN type) to DDM value
         * (OUT type) in DDM store/DDM entry.
         */
        bool is_converted = ddm_converter_set_and_store(event_manager_conversion_table,
                                                        ELEMENTS(event_manager_conversion_table),
                                                        p_evm,
                                                        p_ddmp2_frame,
                                                        p_ddm_entry) == DDM_CONVERTER_ERR_NONE
                                ? true
                                : false;

        if (is_converted)
        {
            /* Do publish of changed values */
            for (size_t i = 0; i < ddm_store__occupied(&p_evm->owned_ddm_store); i++)
            {
                ddm_store__iterate(&p_evm->owned_ddm_store, i, &p_ddm_entry);
                if (ddm_entry__has_changed(p_ddm_entry) && ddm_entry__is_subscribed(p_ddm_entry))
                {
                    TRUE_CHECK(broker_publish(p_evm, p_ddm_entry) == OPS_SUCCESS);
                    /* Regardless if publish was successful or not we are going to clear
                     * `has_changed` flag.
                     */
                    ddm_entry__set__has_changed(p_ddm_entry, false);
                }
            }
        }
        else
        {
            EVENT_NOTIFICATION_LOG(W, "No converter function for DDM Parameter 0x%x", p_ddmp2_frame->frame.set.parameter);
        }
    }
}

/**
 * @brief       Handle subscribe frames of our own parameters.
 *
 * This function will handle subscribe frames that we receive for subscription
 * to EVM DDM parameters.
 *
 * @param       p_evm           Pointer to event_manager_t context structure.
 * @param       p_ddmp2_frame   Pointer to received DDMP2 frame.
 */
static void handle_owned_parameter_subscribe(event_manager_t *p_evm,
                                             const DDMP2_FRAME *p_ddmp2_frame)
{
    ddm_entry_t *p_ddm_entry;

    p_ddm_entry = ddm_store__access(&p_evm->owned_ddm_store, p_ddmp2_frame->frame.set.parameter);

    if (p_ddm_entry == NULL)
    {
        /* Somebody has subscribed to something that we haven't created */
        LOG(W, "%s: subscribe to non-existing DDM: 0x%x",
            p_evm->p_connector->name,
            p_ddmp2_frame->frame.set.parameter);
    }
    else
    {
        ddm_entry__set__is_subscribed(p_ddm_entry, true);
        TRUE_CHECK(broker_publish(p_evm, p_ddm_entry) == OPS_SUCCESS);
        /* Regardless if publish was successful or not we are going to clear
         * `has_changed` flag.
         */
        ddm_entry__set__has_changed(p_ddm_entry, false);
    }
}

/**
 * @brief       Handle publish of other DDM parameters
 *
 * This function will store the new values of subscribed-to DDM parameters into
 * DDM store so when event needs to be generated we have the latest values of
 * DDM parameters.
 *
 * @param       p_evm           Pointer to event_manager_t context structure.
 * @param       p_ddmp2_frame   Pointer to received DDMP2 frame.
 */
static void handle_other_parameter_publish(event_manager_t *p_evm,
                                           const DDMP2_FRAME *p_ddmp2_frame)
{
    ddm_entry_t *p_published;

    p_published = ddm_store__access(&p_evm->other_ddm_store,
                                    p_ddmp2_frame->frame.publish.parameter);

    if (p_published == NULL)
    {
        /* Somebody tried to publish a value to something that we haven't
         * created.
         */
        LOG(W, "%s: publishing of non-existing DDM: 0x%x",
            p_evm->p_connector->name,
            p_ddmp2_frame->frame.publish.parameter);
    }
    else
    {
        bool has_changed;

        EVENT_NOTIFICATION_LOG(I,
                               "Updating other DDM parameter 0x%x size %u bytes",
                               p_ddmp2_frame->frame.publish.parameter,
                               ddmp2_value_size(p_ddmp2_frame));
        /* Use DDM OUT type when setting `other` DDM parameters.
         * We don't care about has_changed return value for other DDM
         * parameters. We need them to only store values.
         */
        has_changed = ddm_entry__set__value(p_published,
                                            &p_ddmp2_frame->frame.set.value,
                                            ddmp2_value_size(p_ddmp2_frame));

#if defined(CONNECTOR_MQTT)
        /* If MQTT is connected, we need to act on it so we can send updates.
         *
         * We need to check if the MQTT client is connected before sending updates.
         *
         * We only send updates if the value of MQTTxSTAT has changed.
         *
         * If MQTT is not connected we just wait for next time.
         *
         * If the value has changed we try to send all pending event records
         * to MQTT connector. After sending we save the event records to persistent
         * storage, so we don't try to send them again.
         */
        if (has_changed && (ddm_entry__parameter_id(p_published) == MQTT0STAT))
        {
            // At this point the MQTT0STAT parameter has changed, check if we are connected
            if (mqtt_is_connected_ddm_entry(p_published))
            {
                uint32_t sent_count = mqtt_send_all_event_records(p_evm);
                if (sent_count > 0u)
                {
                    event_manager_save_records(p_evm);
                }
            }
            else
            {
                LOG(I, "MQTT not connected, not sending event records");
            }
        }
#else
        (void)has_changed;  // To avoid compiler warning when MQTT is not used
#endif
    }
}

#if defined(CONNECTOR_MQTT)
/**
 * @brief       Setup MQTT DDM entries and inventory handler.
 *
 * This function will create the MQTT0STAT DDM entry in other_ddm_store
 * and register MQTT0 to inventory handler. The MQTT0STAT entry will be used
 * to track the MQTT connection status.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 */
static void mqtt_setup(event_manager_t *p_evm)
{
    ddm_entry_t *p_mqtt0stat;
    /* Register to MQTT connection status changes */
    p_mqtt0stat = ddm_store__new_entry(&p_evm->other_ddm_store, MQTT0STAT);
    if (p_mqtt0stat != NULL)
    {
        /* Initialize the DDM entry to NOT CONNECTED state */
        ddm_entry__set__value_i32(p_mqtt0stat, MQTT0STAT_NOT_CONNECTED);
    }
    else
    {
        EVENT_NOTIFICATION_LOG(E, "Not enough space in other DDM storage to set MQTT0STAT");
    }
    /* Register the MQTT status entry to inventory handler */
    EVENT_NOTIFICATION_LOG(I, "Registering MQTT0 to inventory handler");
    int status = inventory_handler_add(&p_evm->inventory_handler, MQTT0);
    if (status != 0)
    {
        LOG(E, "Failed to register MQTT0 to inventory handler");
    }
}

/**
 * @brief       Check if MQTT is connected using DDM entry.
 *
 * This function checks the value of the provided MQTT0STAT DDM entry
 * to determine if MQTT is connected.
 *
 * @param       p_mqtt0stat Pointer to MQTT0STAT DDM entry.
 * @return      true if MQTT is connected, false otherwise.
 */
static bool mqtt_is_connected_ddm_entry(const ddm_entry_t *p_mqtt0stat)
{
    bool is_connected = false;

    /* Check if the value is an integer, it might not be integer if MQTT class gets deleted */
    if (ddm_entry__is_value_int32(p_mqtt0stat))
    {
        int32_t value = ddm_entry__value_i32(p_mqtt0stat);
        /* NOTE: Which connected state is good for us? */
        if ((value == MQTT0STAT_CONNECTED) || (value == MQTT0STAT_CONNECTED_PUB_ROOT_OK))
        {
            is_connected = true;
        }
    }
    else
    {
        EVENT_NOTIFICATION_LOG(W, "MQTT0STAT is not int32");
    }

    return is_connected;
}

/**
 * @brief       Check if MQTT is connected using cached DDM entry.
 *
 * This function accesses the cached MQTT0STAT DDM entry in other_ddm_store
 * to determine if MQTT is connected.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      true if MQTT is connected, false otherwise.
 */
static bool mqtt_is_connected_cached(event_manager_t *p_evm)
{
    ddm_entry_t *p_mqtt0stat;
    bool is_connected = false;

    /* Access MQTTxSTAT parameter in DDM other store */
    p_mqtt0stat = ddm_store__access(&p_evm->other_ddm_store, MQTT0STAT);
    if (p_mqtt0stat != NULL)
    {
        is_connected = mqtt_is_connected_ddm_entry(p_mqtt0stat);
    }
    else
    {
        EVENT_NOTIFICATION_LOG(W, "MQTT0STAT is not available");
    }

    return is_connected;
}

/**
 * @brief       Send a single event record to MQTT connector.
 *
 * This function will prepare JSON string for the event record and queue it to
 * MQTT connector.
 *
 * @param       p_evm            Pointer to event_manager_t context structure.
 * @param       p_event_record   Pointer to event_record_t structure to send.
 * @return      OPS_SUCCESS if the event record was queued to MQTT connector,
 *              OPS_FAILURE otherwise.
 */
static int mqtt_send_event_record(event_manager_t *p_evm, const event_record_t *p_event_record)
{
    int retval = OPS_FAILURE;
    /* Prepare JSON string buffer for cloud and queue it to MQTT connector */
    const char *p_thing_name = connector_mqtt_get_thing();
    if (p_thing_name != NULL)
    {
        int status = make_json_event_record(p_event_record,
                                            p_evm->product_name,
                                            p_thing_name,
                                            json_buffer,
                                            sizeof(json_buffer));
        if (status == MAKE_JSON_NO_ERROR)
        {
            EVENT_NOTIFICATION_LOG(I, "Generated MQTT JSON: %s", json_buffer);
            // queue it to the MQTT connector.
            status = connector_mqtt_notification_publish(json_buffer, strnlen(json_buffer, sizeof(json_buffer)));
            switch (status)
            {
            case CONNECTOR_MQTT_NOTIFICATION_ERR_NONE:
                retval = OPS_SUCCESS;
                break;
            case CONNECTOR_MQTT_NOTIFICATION_ERR_QUEUE_FULL:
                EVENT_NOTIFICATION_LOG(W, "MQTT notification queue is full");
                break;
            case CONNECTOR_MQTT_NOTIFICATION_ERR_MEM:
                EVENT_NOTIFICATION_LOG(W, "MQTT notification memory error");
                break;
            default:
                EVENT_NOTIFICATION_LOG(W, "Failed to queue event record %u to MQTT connector", p_evm->event_id);
                break;
            }
        }
        else
        {
            LOG(W, "Failed to generate JSON for event record %u, error %d", p_evm->event_id, status);
        }
    }
    else
    {
        LOG(W, "MQTT thing name is NULL, cannot generate JSON for event record %u", p_evm->event_id);
    }
    return retval;
}

/**
 * @brief       Send all pending event records to MQTT connector.
 *
 * This function will iterate over all event records and send those which are
 * not sent to cloud yet.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      Number of event records sent to MQTT connector.
 */
static uint32_t mqtt_send_all_event_records(event_manager_t *p_evm)
{
    uint32_t sent_count = 0;
    size_t db_size = event_record_db_occupied(&p_evm->event_record_db);
    EVENT_NOTIFICATION_LOG(I, "Sending all pending event records to cloud, total %u records", (unsigned int)db_size);

    // Iterate over all event records and send those which are not sent to cloud yet
    for (size_t i = 0u; i < db_size; i++)
    {
        event_record_t *p_event_record;
        p_event_record = event_record_db_iterate(&p_evm->event_record_db, i);
        if (event_record_is_cloud_sent(p_event_record) == false)
        {
            EVENT_NOTIFICATION_LOG(I, "Sending event record ID %d to cloud", p_event_record->id);
            int status = mqtt_send_event_record(p_evm, p_event_record);
            if (status == OPS_SUCCESS)
            {
                /* Mark the event record as sent to cloud */
                event_record_set_cloud_sent(p_event_record, true);
                sent_count++;
            }
            else
            {
                /* Failed to send the event record, stop trying to send more */
                break;
            }
        }
        else
        {
            EVENT_NOTIFICATION_LOG(I, "Event record ID %d already sent to cloud", p_event_record->id);
        }
    }
    return sent_count;
}
#endif  // defined(CONNECTOR_MQTT)

/**
 * @brief       Inventory handler callback function.
 *
 * When an instance class becomes available this callback will just subscribe to
 * all parameters mentioned in event type definitions.
 *
 * @param       p_arg During initialization inventory handled was setup to pass
 *              the pointer to event_manager_t structure instance.
 */
static void inventory_handler_cb(void *p_arg, uint32_t device_class_instance, bool is_available)
{
    event_manager_t *p_evm = p_arg;

    /* Filter the device_class_instance for DDM parameters stored in
     * other_ddm_store.
     */
    for (size_t i = 0u; i < ddm_store__occupied(&p_evm->other_ddm_store); i++)
    {
        ddm_entry_t *p_available_entry;
        ddm_store__iterate(&p_evm->other_ddm_store, i, &p_available_entry);

        if (DDM2_PARAMETER_CLASS_INSTANCE(ddm_entry__parameter_id(p_available_entry)) == device_class_instance)
        {
            if (is_available)
            {
                int status = broker_subscribe(p_evm, p_available_entry);
                if (status != OPS_SUCCESS)
                {
                    LOG(W, "Failed to subscribe to DDM parameter 0x%x",
                        ddm_entry__parameter_id(p_available_entry));
                }
            }
            else
            {
                /* This DDM class become unavailable, create a temporary DDM
                 * entry with NONE value (empty) and use it to set the managed
                 * `ddm_entry`.
                 */
                ddm_entry_t empty_ddm_entry;

                ddm_entry__init(&empty_ddm_entry, ddm_entry__parameter_id(p_available_entry));
                /* We don't care about has_changed return value for other DDM
                 * parameters. We need them to only store values.
                 */
                (void)ddm_entry__copy__value(p_available_entry, &empty_ddm_entry);
                ddm_entry__terminate(&empty_ddm_entry);
            }
        }
    }
}

/**
 * @brief       Initialize event_manager connector context structure.
 *
 * @param       p_evm is a pointer to event_manager_t context structure that needs
 *              to be initialized.
 * @param       p_connector is a pointer to connector handle structure.
 * @param       p_event_type_db_storage Pointer to SORTED_CONTAINER which will be
 *              used to store instances of event type structures.
 * @param       p_event_record_db_storage Pointer to SORTED_CONTAINER which will
 *              be used to store instances of event record structures.
 * @param       p_event_record_db_fifo_storage Pointer to event_id array that
 *              will be used to store event ids which are associated with
 *              instances in @a p_event_record_db_storage.
 * @param       p_inventory_handler_storage is a pointer to sorted list needed for
 *              inventory operation.
 * @param       p_owned_ddm_store_storage Pointer to SORTED_CONTAINER which will
 *              be used to store instaces if ddm_entry structures for DDM
 *              parameters which are owned by this connector.
 * @param       p_other_ddm_store_storage Pointer to SORTED_CONTAINER which will
 *              be used to store instaces if ddm_entry structures for DDM
 *              parameters which are __NOT__ owned by this connector.
 */
static void event_manager_init(event_manager_t *p_evm,
                               CONNECTOR *p_connector,
                               SORTED_CONTAINER *p_event_type_db_storage,
                               SORTED_CONTAINER *p_event_record_db_storage,
                               event_id_t *p_event_record_db_fifo_storage,
                               SORTED_LIST *p_inventory_handler_storage,
                               SORTED_CONTAINER *p_owned_ddm_store_storage,
                               SORTED_CONTAINER *p_other_ddm_store_storage)
{
    /* Initialize members */
    p_evm->p_connector = p_connector;
    p_evm->event_id = 0u;
    p_evm->acknowledged = 0u;
    p_evm->p_event_listener = NULL;
    p_evm->p_event_listener_arg = NULL;
    p_evm->state = EVENT_MANAGER_STATE_UNCONFIGURED;
    /* Construct Event Type DB */
    event_type_db_init(&p_evm->event_type_db, p_event_type_db_storage);
    /* Construct Event Record DB */
    event_record_db_init(&p_evm->event_record_db,
                         p_event_record_db_storage,
                         p_event_record_db_fifo_storage);
    /* Construct inventory handler */
    inventory_handler_init(&p_evm->inventory_handler,
                           p_inventory_handler_storage,
                           inventory_handler_cb,
                           p_evm);
    /* Construct owned DDM store */
    ddm_store__init(&p_evm->owned_ddm_store, p_owned_ddm_store_storage);
    /* Construct other DDM store */
    ddm_store__init(&p_evm->other_ddm_store, p_other_ddm_store_storage);

#if defined(CONNECTOR_MQTT)
    mqtt_setup(p_evm);
#endif
}

/**
 * @brief       Initialized owned DDM parameters.
 *
 * Create memory for owned DDM parameters and initialize with values.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @param       p_owned_ddm_values Pointer to array containing initial DDM
 *              parameter values.
 * @param       owned_ddm_values_size Size of array pointer by
 *              @a p_owned_ddm_values in number of elements.
 * @param       ddm_class_instance that was returned during registration with
 *              Broker.
 */
static void event_manager_init_owned_ddm(event_manager_t *p_evm,
                                         const ddm_store_ddm_t *p_owned_ddm_values,
                                         size_t owned_ddm_values_size,
                                         int32_t ddm_class_instance)
{
    ddm_store__load_entries(&p_evm->owned_ddm_store,
                            p_owned_ddm_values,
                            owned_ddm_values_size,
                            (uint32_t)ddm_class_instance);
    for (size_t i = 0u; i < ddm_store__occupied(&p_evm->owned_ddm_store); i++)
    {
        ddm_entry_t *p_owned;

        ddm_store__iterate(&p_evm->owned_ddm_store, i, &p_owned);
        /* When DDM entries are created by DDM store it will automatically set
         * has_changed flag as well and we don't want that.
         */
        ddm_entry__set__has_changed(p_owned, false);
    }
}

/**
 * @brief       Load event types definitions from JSON string.
 *
 * This function will parse the JSON string and insert event type definitions
 * into event type database.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @param       p_json_string Pointer to JSON string containing event type
 *              definitions.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_ERROR_JSON_INVALID Invalid JSON was specified (wrong
 *              syntax).
 * @retval      OPS_ERROR_JSON_MISSING_FIELDS Missing required fields in the
 *              JSON string.
 * @retval      OPS_ERROR_D_FIELD Missing or invalid D (DDM) fields in JSON.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_load_event_types(event_manager_t *p_evm, const char *p_json_string)
{
    int status;

    status = parser_parse_event_type(p_evm, p_json_string, event_type_db_insert_wrap, &p_evm->event_type_db);
    if (status != OPS_SUCCESS)
    {
        EVENT_NOTIFICATION_LOG(W, "Failed to load event types from JSON: %d", status);
    }
    return status;
}

/**
 * @brief       Subscribe to parameters defined by event types.
 *
 * This function will iterate over all event types in event type database
 * and for each DDM parameter defined in event type it will create an entry
 * in other_ddm_store and subscribe to it using inventory handler.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 */
static void event_manager_subscribe_other(event_manager_t *p_evm)
{
    for (uint32_t i = 0u; i < event_type_db_occupied(&p_evm->event_type_db); i++)
    {
        const event_type_t *p_event_type;

        p_event_type = event_type_db_iterate(&p_evm->event_type_db, i);
        for (uint32_t j = 0u; j < event_type_get_data_size(p_event_type); j++)
        {
            /* We are only interested in DDM parameters */
            if (event_type_is_data_parameter(p_event_type, j))
            {
                ddm_entry_t *p_event_type_ddm;
                /* Create entry in other_ddm_store */
                uint32_t param_id = event_type_get_parameter_id(p_event_type, j);
                p_event_type_ddm = ddm_store__new_entry(&p_evm->other_ddm_store, param_id);
                if (p_event_type_ddm != NULL)
                {
                    EVENT_NOTIFICATION_LOG(I,
                                           "Adding inventory handler for: %s0%s(0x%x)",
                                           ddm_entry__device_class(p_event_type_ddm),
                                           ddm_entry__property(p_event_type_ddm),
                                           ddm_entry__parameter_id(p_event_type_ddm));
                    /* Add inventory handler. We ignore return value here since the
                     * inventory handler is as big as other_ddm_store and we already did
                     * check if other_ddm_store has sufficient space for new entry.
                     */
                    (void)inventory_handler_add(&p_evm->inventory_handler, param_id);
                }
                else
                {
                    /* Not enough space in other_ddm_store to set new DDM parameter */
                    LOG(W, "Not enough space in other DDM storage to set new DDM");
                    break;
                }
            }
        }
    }
}

/**
 * @brief       Start the operation of connector after initialization.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_start(event_manager_t *p_evm)
{
    int status = inventory_handler_start(&p_evm->inventory_handler, p_evm->p_connector);

    return status == 0 ? OPS_SUCCESS : OPS_FAILURE;
}

/**
 * @brief       Save DDM value of single event record.
 *
 * @param       p_nvs_key      NVS key under which the value will be stored.
 * @param       p_record_ddm   Pointer to DDM entry containing the value to
 *              be stored.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_save_record_ddm_value(const char *p_nvs_key, const ddm_entry_t *p_record_ddm)
{
    int status;

    switch (ddm_entry__out_type(p_record_ddm))
    {
    case DDM2_TYPE_INT32_T:
        status = config_set_i32(p_nvs_key, ddm_entry__value_i32(p_record_ddm)) == ESP_OK ? OPS_SUCCESS : OPS_FAILURE;
        break;
    case DDM2_TYPE_UINT32_T:
        status = config_set_i32(p_nvs_key, ddm_entry__value_u32(p_record_ddm)) == ESP_OK ? OPS_SUCCESS : OPS_FAILURE;
        break;
    case DDM2_TYPE_STRING:
        status = config_set_str(p_nvs_key, ddm_entry__value_str(p_record_ddm)) == ESP_OK ? OPS_SUCCESS : OPS_FAILURE;
        break;
    default:
    {
        const void *p_ddm_data;
        size_t ddm_data_size;

        ddm_entry__read__value(p_record_ddm, &p_ddm_data, &ddm_data_size);
        status = config_set_blob(p_nvs_key, p_ddm_data, ddm_data_size) == ESP_OK ? OPS_SUCCESS : OPS_FAILURE;
        break;
    }
    }

    return status;
}

/**
 * @brief       Load DDM value of single event record.
 *
 * @param       p_nvs_key      NVS key under which the value is stored.
 * @param       p_record_ddm   Pointer to DDM entry where the loaded value
 *                             will be stored.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_load_record_ddm_value(const char *p_nvs_key, ddm_entry_t *p_record_ddm)
{
    int status = OPS_FAILURE;
    switch (ddm_entry__out_type(p_record_ddm))
    {
    case DDM2_TYPE_INT32_T:
    {
        int32_t value;
        if (config_get_i32(p_nvs_key, &value) == ESP_OK)
        {
            status = OPS_SUCCESS;
            ddm_entry__set__value_i32(p_record_ddm, value);
        }
        break;
    }
    case DDM2_TYPE_UINT32_T:
    {
        int32_t value;
        if (config_get_i32(p_nvs_key, &value) == ESP_OK)
        {
            status = OPS_SUCCESS;
            ddm_entry__set__value_u32(p_record_ddm, (uint32_t)value);
        }
        break;
    }
    case DDM2_TYPE_STRING:
    {
        char string[DDMP2_MAX_VALUE_SIZE];
        size_t string_len = ELEMENTS(string);
        if (config_get_str(p_nvs_key, string, &string_len) == ESP_OK)
        {
            status = OPS_SUCCESS;
            ddm_entry__set__value_str(p_record_ddm, string, string_len);
        }
        break;
    }
    default:
    {
        char buffer[DDMP2_MAX_VALUE_SIZE];
        size_t buffer_len = ELEMENTS(buffer);
        if (config_get_blob(p_nvs_key, buffer, &buffer_len) == ESP_OK)
        {
            status = OPS_SUCCESS;
            if (buffer_len > 0)
            {
                ddm_entry__set__value(p_record_ddm, buffer, buffer_len);
            }
        }
        break;
    }
    }

    return status;
}

/**
 * @brief       Load stored DDM data for single event record.
 *
 * @param       p_er              Pointer to event_record_t structure where the
 *                                loaded data will be stored.
 * @param       er_index          Index of event record in NVS.
 * @param       er_size           Number of data fields in event record.
 * @param       p_event_type_db   Pointer to event type database used to
 *                                validate event types.
 *
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_load_record_data(event_record_t *p_er, uint32_t er_index, size_t er_size, const event_type_db_t *p_event_type_db)
{
    int retval = OPS_SUCCESS;

    /* Find event_type structure by event_type id, to validate we still have it. */
    const event_type_t *p_event_type = event_type_db_find_by_type(p_event_type_db, p_er->type);
    if (p_event_type != NULL)
    {
        for (size_t er_data_index = 0u; er_data_index < er_size; er_data_index++)
        {
            char nvs_key[NVS_KEY_NAME_MAX_SIZE];
            uint32_t data_flag;

            /* Load event record data flag */
            int status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_DATA_FLAG_KEY, er_index, er_data_index);
            TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
            status = config_get_i32(nvs_key, (int32_t *)&data_flag);
            TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);

            if ((data_flag == EVENT_RECORD_DATA_FLAG_HAS_DDM_VALUE) || (data_flag == EVENT_RECORD_DATA_FLAG_HAS_NO_DDM_VALUE))
            {
                ddm_entry_t er_ddm;
                uint32_t er_ddm_id;

                /* Load event record DDM parameter ID */
                status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_DDM_ID_KEY, er_index, er_data_index);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
                status = config_get_i32(nvs_key, (int32_t *)&er_ddm_id);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);

                status = ddm_entry__init(&er_ddm, er_ddm_id);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status == 0);

                if (data_flag == EVENT_RECORD_DATA_FLAG_HAS_DDM_VALUE)
                {
                    /* Load event record DDM data */
                    status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_DDM_DATA_KEY, er_index, er_data_index);
                    if (status < 0)
                    {
                        EVENT_NOTIFICATION_LOG(W, "While loading record (type %u), failed to generate blob key: %d", p_er->type, status);
                        ddm_entry__terminate(&er_ddm);
                        retval = OPS_FAILURE;
                        break;
                    }
                    status = event_manager_load_record_ddm_value(nvs_key, &er_ddm);
                    if (status != OPS_SUCCESS)
                    {
                        EVENT_NOTIFICATION_LOG(W, "While loading record (type %u), failed to retreive blob: %d", p_er->type, status);
                        ddm_entry__terminate(&er_ddm);
                        retval = OPS_FAILURE;
                        break;
                    }
                }
                else
                {
                    EVENT_NOTIFICATION_LOG(I, "In event record (type %u) DDM 0x%x (with index %zu) does not have a value", p_er->type, er_ddm_id, er_data_index);
                }

                /* Since the data in the NVS is already set (parsed or not), the offset and the size should be 0
                   meaning that the whole stored value be loaded back to event record */
                size_t field_offset = 0;
                size_t field_size = 0;
                status = event_record_add_parameter_data(p_er, &er_ddm, field_offset, field_size);
                if (status != EVENT_RECORD_OP_SUCCESS)
                {
                    EVENT_NOTIFICATION_LOG(W, "While loading record, failed to retreive DDM values: %d", status);
                    ddm_entry__terminate(&er_ddm);
                    retval = OPS_FAILURE;
                    break;
                }
                ddm_entry__terminate(&er_ddm);
            }
            else if ((data_flag == EVENT_RECORD_DATA_FLAG_HAS_TRIGGER_VALUE) || (data_flag == EVENT_RECORD_DATA_FLAG_HAS_NO_TRIGGER_VALUE))
            {
                int32_t er_trigger_value;

                if (data_flag == EVENT_RECORD_DATA_FLAG_HAS_TRIGGER_VALUE)
                {

                    /* Load event record trigger data */
                    status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_TRG_DATA_KEY, er_index, er_data_index);
                    TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
                    status = config_get_i32(nvs_key, &er_trigger_value);
                    TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
                }
                else
                {
                    er_trigger_value = 0u;
                }
                size_t field_offset = 0;
                size_t field_size = 0;
                status = event_record_add_trigger_data(p_er, er_trigger_value, field_offset, field_size);
                if (status != EVENT_RECORD_OP_SUCCESS)
                {
                    EVENT_NOTIFICATION_LOG(W, "While loading record, failed to retreive trigger values: %d", status);
                    retval = OPS_FAILURE;
                    break;
                }
            }
            else
            {
                EVENT_NOTIFICATION_LOG(W, "While loading record, invalid data flag %u", data_flag);
                retval = OPS_FAILURE;
                break;
            }
        }
    }
    else
    {
        /* We got a event_type that we don't have in event_type database. */
        EVENT_NOTIFICATION_LOG(W, "Inconsistent type definitions! Missing event type %u in event_type database", p_er->type);
        retval = OPS_SUCCESS;  // Treat as success to continue loading other records
    }

    return retval;
}

/**
 * @brief       Load event records from NVS
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_load_records(event_manager_t *p_evm)
{
    size_t event_record_db_size;
    int retval = OPS_SUCCESS;

    event_record_db_purge(&p_evm->event_record_db);
    /* First retreive number of saved event records */
    int status = config_get_i32(NVS_EVENT_RECORD_DB_SIZE_KEY, (int32_t *)&event_record_db_size);
    if (status == ESP_ERR_NVS_NOT_FOUND)
    {
        /* No saved event records, this is not an error that we need to indicate, but internally it is
         * treated as failure to load any records.
         */
        return OPS_FAILURE;
    }
    EVENT_NOTIFICATION_LOG(I,
                           "Restoring event record database of size %u entries from " NVS_EVENT_RECORD_DB_SIZE_KEY,
                           event_record_db_size);
    if (event_record_db_size > CONNECTOR_EVENT_NOTIFICATION_MAX_EVENT_RECORDS)
    {
        return OPS_FAILURE;
    }
    for (size_t er_index = 0u; er_index < event_record_db_size; er_index++)
    {
        char nvs_key[NVS_KEY_NAME_MAX_SIZE];
        event_record_t er;
        event_id_t event_id;
        event_timestamp_t ts;
        int32_t type;
        uint32_t flags;
        uint32_t size;

        /* Load event record ID */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_ID_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_get_i32(nvs_key, (int32_t *)&event_id);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Load event record ts */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_TS_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_get_i32(nvs_key, (int32_t *)&ts);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Load event record type */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_TYPE_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_get_i32(nvs_key, &type);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Load event record ack */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_FLAGS_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_get_i32(nvs_key, (int32_t *)&flags);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Load event record size */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_SIZE_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_get_i32(nvs_key, (int32_t *)&size);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);

        EVENT_NOTIFICATION_LOG(I,
                               "Restoring event record %u: id=%u, ts=%u, type=%d, flags=0x%08x, size=%u",
                               (unsigned int)er_index,
                               event_id,
                               ts,
                               type,
                               flags,
                               (unsigned int)size);
        /* Initialize working copy of event record. */
        event_record_init(&er, event_id, type);
        event_record_set_ts(&er, ts);
        event_record_set_flags(&er, flags);

        status = event_manager_load_record_data(&er, er_index, size, &p_evm->event_type_db);

        if (status == OPS_SUCCESS)
        {
            event_record_db_insert(&p_evm->event_record_db, &er);
        }
        else
        {
            LOG(W, "Failed to retreive event_record %u", event_id);
            event_record_terminate(&er);
            retval = OPS_FAILURE;
            break;
        }
        /* Terminate working copy of event record. */
        event_record_terminate(&er);
    }
    return retval;
}

/**
 * @brief       Save event records into NVS
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_save_records(const event_manager_t *p_evm)
{
    size_t event_record_db_size = event_record_db_occupied(&p_evm->event_record_db);

    EVENT_NOTIFICATION_LOG(I, "Storing %u event record(s) to NVS.", event_record_db_size);
    /* Go through stored event records, start saving from FIFO tail side towards
     * FIFO head.
     */
    for (size_t er_index = 0u; er_index < event_record_db_size; er_index++)
    {
        char nvs_key[NVS_KEY_NAME_MAX_SIZE];

        const event_record_t *p_er = event_record_db_iterate(&p_evm->event_record_db, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, p_er != NULL);
        /* Save event record ID */
        int status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_ID_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_set_i32(nvs_key, event_record_get_id(p_er));
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Save event record ts / timestamp */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_TS_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_set_i32(nvs_key, event_record_get_ts(p_er));
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Save event record type */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_TYPE_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_set_i32(nvs_key, event_record_get_type(p_er));
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Save event record ack */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_FLAGS_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_set_i32(nvs_key, event_record_get_flags(p_er));
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        /* Save event record size */
        status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_SIZE_KEY, er_index);
        TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
        status = config_set_i32(nvs_key, event_record_get_data_size(p_er));
        TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
        for (size_t j = 0u; j < event_record_get_data_size(p_er); j++)
        {
            if (event_record_is_data_parameter(p_er, j))
            {
                const ddm_entry_t *ddm_entry = event_record_get_parameter_data(p_er, j);
                const void *p_er_ddm;
                size_t er_ddm_data_size;

                /* Save event record DDM id */
                status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_DDM_ID_KEY, er_index, j);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
                status = config_set_i32(nvs_key, ddm_entry__parameter_id(ddm_entry));
                TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
                /* Save event record DDM flag */
                status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_DATA_FLAG_KEY, er_index, j);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
                if (ddm_entry__is_value_none(ddm_entry))
                {
                    status = config_set_i32(nvs_key, EVENT_RECORD_DATA_FLAG_HAS_NO_DDM_VALUE);
                }
                else
                {
                    status = config_set_i32(nvs_key, EVENT_RECORD_DATA_FLAG_HAS_DDM_VALUE);
                }
                TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
                /* Save event record DDM data if there is something to store */
                ddm_entry__read__value(ddm_entry, &p_er_ddm, &er_ddm_data_size);
                if (er_ddm_data_size > 0)
                {
                    status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_DDM_DATA_KEY, er_index, j);
                    TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0)
                    status = event_manager_save_record_ddm_value(nvs_key, ddm_entry);
                    TRUE_CHECK_RETURNX(OPS_FAILURE, status == 0);
                }
            }
            else if (event_record_is_data_trigger(p_er, j))
            {
                int32_t trigger_value = event_record_get_trigger_data(p_er, j);

                /* Save event record trigger flag */
                status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_DATA_FLAG_KEY, er_index, j);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0);
                status = config_set_i32(nvs_key, EVENT_RECORD_DATA_FLAG_HAS_TRIGGER_VALUE);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
                /* Save event record trigger data if there is something to store */
                status = snprintf(nvs_key, sizeof(nvs_key), NVS_EVENT_RECORD_TRG_DATA_KEY, er_index, j);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status >= 0)
                status = config_set_i32(nvs_key, trigger_value);
                TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
            }
            else
            {
                LOG(E, "Invalid event record data type!");
                return OPS_FAILURE;
            }
        }
    }
    int status = config_set_i32(NVS_EVENT_RECORD_DB_SIZE_KEY, (int32_t)event_record_db_size);
    TRUE_CHECK_RETURNX(OPS_FAILURE, status == ESP_OK);
    return OPS_SUCCESS;
}

/**
 * @brief       Load event_id from NVS
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_load_event_id(event_manager_t *p_evm)
{
    int32_t event_id;
    int retval;

    EVENT_NOTIFICATION_LOG(I, "Loading event_id...");
    int status = config_get_i32(NVS_EVENT_ID_KEY, &event_id);
    if (status == ESP_OK)
    {
        EVENT_NOTIFICATION_LOG(I, "Restored event_id: %u", (uint32_t)event_id);
        p_evm->event_id = (uint32_t)event_id;
        retval = OPS_SUCCESS;
    }
    else
    {
        EVENT_NOTIFICATION_LOG(I, "Using event_id default value.");
        retval = OPS_FAILURE;
    }
    return retval;
}

/**
 * @brief       Save event_id into NVS.
 *
 * This function will save the state of event_id member into the NVS.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_save_event_id(const event_manager_t *p_evm)
{
    int retval;

    EVENT_NOTIFICATION_LOG(I, "Saving event_id = %u", p_evm->event_id);
    int status = config_set_i32(NVS_EVENT_ID_KEY, (int32_t)p_evm->event_id);
    if (status == ESP_OK)
    {
        retval = OPS_SUCCESS;
    }
    else
    {
        LOG(W, "Failed to save event_id, error: %d", status);
        retval = OPS_FAILURE;
    }
    return retval;
}

/**
 * @brief       Load acknowledged from NVS.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_load_acknowledged(event_manager_t *p_evm)
{
    int32_t acknowledged;
    int retval;

    EVENT_NOTIFICATION_LOG(I, "Loading acknowledged...");
    int status = config_get_i32(NVS_ACKNOWLEDGED_KEY, &acknowledged);
    if (status == ESP_OK)
    {
        EVENT_NOTIFICATION_LOG(I, "Restored acknowledged: %u", (uint32_t)acknowledged);
        p_evm->acknowledged = (uint32_t)acknowledged;
        retval = OPS_SUCCESS;
    }
    else
    {
        EVENT_NOTIFICATION_LOG(I, "Using acknowledged default value.");
        p_evm->acknowledged = 0u;
        retval = OPS_SUCCESS;
    }
    return retval;
}

/**
 * @brief       Save acknowledged into NVS
 *
 * This function will save the state of acknowledged member into the NVS.
 *
 * @param       en Pointer to event_manager_t context structure.
 * @return      Operation status
 * @retval      OPS_SUCCESS when operation completed successfully.
 * @retval      OPS_FAILURE when operation has encountered an error.
 */
static int event_manager_save_acknowledged(const event_manager_t *p_evm)
{
    int retval;

    EVENT_NOTIFICATION_LOG(I, "Saving acknowledged = %u", p_evm->acknowledged);
    int status = config_set_i32(NVS_ACKNOWLEDGED_KEY, (int32_t)p_evm->acknowledged);
    if (status == ESP_OK)
    {
        retval = OPS_SUCCESS;
    }
    else
    {
        LOG(W, "Failed to save acknowledged, error: %d", status);
        retval = OPS_FAILURE;
    }
    return retval;
}

/**
 * @brief       Set default values in event manager context.
 *
 * This function will set default values in event manager context structure.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 */
static void event_manager_set_defaults(event_manager_t *p_evm)
{
    event_record_db_purge(&p_evm->event_record_db);
    p_evm->event_id = 0u;
    p_evm->acknowledged = 0u;
}

/**
 * @brief       Set default values in event manager context, keeping event_id.
 *
 * This function will set default values in event manager context structure,
 * but it will keep the current value of event_id member.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 */
static void event_manager_set_defaults_keep_event_id(event_manager_t *p_evm)
{
    event_record_db_purge(&p_evm->event_record_db);
    p_evm->acknowledged = 0u;
}

/**
 * @brief       Generate initial values for owned DDM parameters.
 *
 * This function will set initial values for owned DDM parameters in owned
 * DDM store.
 *
 * @param       p_evm Pointer to event_manager_t context structure.
 */
static void event_manager_generate_initial_values(event_manager_t *p_evm)
{
    ddm_entry_t *p_ddm;
    nack_concrete_t nack;

    nack_update(&nack, &p_evm->event_record_db);
    p_ddm = ddm_store__access(&p_evm->owned_ddm_store, EVM0ID);
    ddm_entry__set__value_i32(p_ddm, p_evm->event_id);
    p_ddm = ddm_store__access(&p_evm->owned_ddm_store, EVM0ACK);
    ddm_entry__set__value_i32(p_ddm, p_evm->acknowledged);
    p_ddm = ddm_store__access(&p_evm->owned_ddm_store, EVM0NACK);
    ddm_entry__set__value_struct(p_ddm, &nack, nack_sizeof(&nack));
    p_ddm = ddm_store__access(&p_evm->owned_ddm_store, EVM0TRIG);
    ddm_entry__set__value_i32(p_ddm, p_evm->event_id);
    p_ddm = ddm_store__access(&p_evm->owned_ddm_store, EVM0REC);
    ddm_entry__set__value_str(p_ddm, "{}", sizeof("{}"));  // Empty JSON object
}

/* -- Connector functions -- */

int event_manager_connector_init(event_manager_t *p_evm,
                                 const event_manager_description_t *p_desc)
{
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_evm != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc->p_connector != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc->p_event_record_db_fifo_storage != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc->p_event_record_db_storage != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc->p_event_type_db_storage != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc->p_inventory_handler_storage != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc->p_other_ddm_store_storage != NULL);
    TRUE_CHECK_RETURNX(EVENT_MANAGER_ERROR_NULL_ARG, p_desc->p_owned_ddm_store_storage != 0);

    /* In future we might need to handle multiple instances of event managers.
     * Currently, only store to this single instance.
     */
    p_current_evm = p_evm;

    event_manager_init(p_evm,
                       p_desc->p_connector,
                       p_desc->p_event_type_db_storage,
                       p_desc->p_event_record_db_storage,
                       p_desc->p_event_record_db_fifo_storage,
                       p_desc->p_inventory_handler_storage,
                       p_desc->p_owned_ddm_store_storage,
                       p_desc->p_other_ddm_store_storage);
    int32_t evm_ddm_class_instance = handle_owned_parameter_register(EVM0, p_evm->p_connector);
    if (evm_ddm_class_instance == -1)
    {
        LOG(E, "%s: failed to register EVM0 class, terminating!", p_evm->p_connector->name);
        return EVENT_MANAGER_ERROR_FAILURE;
    }
    /* Initialize EVM DDM class */
    event_manager_init_owned_ddm(p_evm,
                                 &owned_ddm_store_initial_values[0],
                                 ELEMENTS(owned_ddm_store_initial_values),
                                 evm_ddm_class_instance);

    return EVENT_MANAGER_NO_ERROR;
}

void event_manager_conector_process_task(event_manager_t *p_evm, const DDMP2_FRAME *p_frame)
{
    /* If we get a DDMP2 frame, but we are still not configured, just exit the task */
    if (p_evm->state == EVENT_MANAGER_STATE_UNCONFIGURED)
    {
        return;
    }
    /* Process only if frame is inventory update frame about the classes that we
     * are interested in.
     */
    int inventory_status = inventory_handler_update(&p_evm->inventory_handler, p_frame);
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
        switch (p_frame->frame.control)
        {
        case DDMP2_CONTROL_SET:
            handle_owned_parameter_set(p_evm, p_frame);
            break;
        case DDMP2_CONTROL_SUBSCRIBE:
            handle_owned_parameter_subscribe(p_evm, p_frame);
            break;
        case DDMP2_CONTROL_PUBLISH:
            handle_other_parameter_publish(p_evm, p_frame);
            break;
        case DDMP2_CONTROL_NOP:
        case DDMP2_CONTROL_FRAGMENT:
        case DDMP2_CONTROL_MESSAGE:
        case DDMP2_CONTROL_REG:
        case DDMP2_CONTROL_COUNT:
        default:
            LOG(W, "%s: unhandled frame %u", p_evm->p_connector->name, p_frame->frame.control);
            break;
        }
    }
}

int event_manager_load_configuration(event_manager_t *p_evm, const char *p_configuration_json)
{
    /* If we were already configured, but were called again, exit with failure. */
    if (p_evm->state == EVENT_MANAGER_STATE_CONFIGURED)
    {
        return EVENT_MANAGER_ERROR_FAILURE;
    }
    /* Parse Event type and load Event Types into Event Type DB */
    int load_status = event_manager_load_event_types(p_evm, p_configuration_json);
    if (load_status != OPS_SUCCESS)
    {
        /* We failed to load event types. When this happens we don't know how to
         * create/handle event records, therefore, its best to just give up?
         */
        EVENT_NOTIFICATION_LOG(I, "JSON: %s", p_configuration_json);
        return EVENT_MANAGER_ERROR_FAILURE;
    }
    /* Restore previously stored records and state from NVS */
    load_status = event_manager_load_records(p_evm);
    /* Restore last event ID */
    load_status |= event_manager_load_event_id(p_evm);
    /* Restore number of acknowledged event records */
    load_status |= event_manager_load_acknowledged(p_evm);
    if (load_status != OPS_SUCCESS)
    {
        /* If any of the loading operations failed, we reset all to defaults */
        LOG(I, "Restoring NVS data failed (empty or corrupt), using defaults...");
        /* Set default value(s) for all variables if any loading fails. */
        event_manager_set_defaults(p_evm);
    }
    /* Generate initial OUT DDM parameters based on what was loaded previosly */
    event_manager_generate_initial_values(p_evm);
    /* Subscribe to other parameters. */
    event_manager_subscribe_other(p_evm);
    /* We are now configured */
    p_evm->state = EVENT_MANAGER_STATE_CONFIGURED;
    /* Start processing */
    int start_status = event_manager_start(p_evm);
    if (start_status != OPS_SUCCESS)
    {
        return EVENT_MANAGER_ERROR_FAILURE;
    }
    return EVENT_MANAGER_NO_ERROR;
}

int event_manager_register_event_listener_cb(event_manager_listener_cb *p_listener, void *p_listener_arg)
{
    int retval;
    if (p_current_evm != NULL)
    {
        if (p_current_evm->p_event_listener == NULL)
        {
            p_current_evm->p_event_listener = p_listener;
            p_current_evm->p_event_listener_arg = p_listener_arg;
            retval = EVENT_MANAGER_NO_ERROR;
        }
        else
        {
            retval = EVENT_MANAGER_ERROR_NO_FREE_SLOT;
        }
    }
    else
    {
        retval = EVENT_MANAGER_ERROR_NO_INIT;
    }
    return retval;
}

int event_manager_find_record_by_id(event_id_t event_id, event_record_t *p_er)
{
    int retval;

    if (p_current_evm != NULL)
    {
        event_record_t *p_er_local = event_record_db_find_by_id(&p_current_evm->event_record_db,
                                                                event_id);

        if (p_er_local != NULL)
        {
            event_record_copy(p_er_local, p_er);
            retval = EVENT_MANAGER_NO_ERROR;
        }
        else
        {
            retval = EVENT_MANAGER_ERROR_FAILURE;
        }
    }
    else
    {
        retval = EVENT_MANAGER_ERROR_NO_INIT;
    }
    return retval;
}

int event_manager_find_type_by_id(uint32_t type_id, event_type_t *p_et)
{
    int retval;

    if (p_current_evm != NULL)
    {
        const event_type_t *p_et_local = event_type_db_find_by_type(&p_current_evm->event_type_db,
                                                                    type_id);

        if (p_et_local != NULL)
        {
            event_type_copy(p_et_local, p_et);
            retval = EVENT_MANAGER_NO_ERROR;
        }
        else
        {
            retval = EVENT_MANAGER_ERROR_FAILURE;
        }
    }
    else
    {
        retval = EVENT_MANAGER_ERROR_NO_INIT;
    }
    return retval;
}

int event_manager_ack_record_by_id(event_id_t event_id)
{
    int retval;

    if (p_current_evm != NULL)
    {
        event_record_t *p_er = event_record_db_find_by_id(&p_current_evm->event_record_db,
                                                          event_id);

        if (p_er != NULL)
        {
            if (event_record_is_app_acknowledged(p_er) == false)
            {
                nack_concrete_t nack;

                /* This event record was not acknowledged. Save the acknowledged event ID. */
                p_current_evm->acknowledged = event_id;
                event_manager_save_acknowledged(p_current_evm);
                /* Update the event record. Save the acknowledged event record. */
                event_record_set_app_acknowledged(p_er, true);
                event_manager_save_records(p_current_evm);
                /* Update and publish EVM0NACK. */
                nack_update(&nack, &p_current_evm->event_record_db);
                ddm_entry_t *p_evm0nack = ddm_store__access(&p_current_evm->owned_ddm_store, EVM0NACK);
                bool has_changed = ddm_entry__set__value_struct(p_evm0nack, &nack, nack_sizeof(&nack));
                ddm_entry__set__has_changed(p_evm0nack, has_changed);
                evm_broker_publish_changed(p_current_evm);
            }
            else
            {
                /* This event record was already acknowledged, do nothing. */
                EVENT_NOTIFICATION_LOG(I, "Event record %d is already acknowledged", event_id);
            }
            retval = EVENT_MANAGER_NO_ERROR;
        }
        else
        {
            retval = EVENT_MANAGER_ERROR_FAILURE;
        }
    }
    else
    {
        retval = EVENT_MANAGER_ERROR_NO_INIT;
    }
    return retval;
}

const char *event_manager_get_device_id(void)
{
#if defined(CONNECTOR_MQTT)
    return connector_mqtt_get_thing();
#else
    return "null";
#endif
}

const char *event_manager_get_product_name(void)
{
    return p_current_evm->product_name;
}

/*
 * Make sure we have a proper number of DDM instances in interface. When the
 * number of elements in `owned_ddm_store_initial_values` is changed, update
 * the macro in interface.
 */
COMPILE_TIME_ASSERT(ELEMENTS(owned_ddm_store_initial_values) == EVENT_MANAGER_OWNED_DDM_COUNT);
