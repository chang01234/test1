/**
 * @file lindev_generic.c
 *
 */

#include <stdint.h>
#include <string.h>

#include "configuration.h"

#include "lindev_generic.h"

#include "broker.h"
#include "dometic.h"

#if (LINDEV_GENERIC_VERBOSE_LOG == 1)
#define LINDEV_GENERIC_LOG(level, format, ...) LOG(level, format, ##__VA_ARGS__)
#else
#define LINDEV_GENERIC_LOG(level, format, ...)
#endif

#if !defined(LINDEV_GENERIC_SAVE_PID)
#define LINDEV_GENERIC_SAVE_PID 1
#endif

#define LIN_NAD_CONFIG_STR "lin_nad"
#define LIN_PID_CONFIG_STR "lin_pid%u"
#define FRAME_PID_WILDCARD 0xffu

/*
 * A customer has requested that we need to reply only when Supplier ID and
 * Function ID match in Read-by-Identifier diagnostic message. To me, this is
 * against the LIN 2.2A specification. Define this macro to 0 (zero) if you want
 * to have LIN connector behave as stated in LIN 2.2A specification.
 */
#ifndef LINDEV_GENERIC_DIAG_SHY_RESPONSE
#define LINDEV_GENERIC_DIAG_SHY_RESPONSE 1
#endif

TaskHandle_t worker_task_handle;

/**
 * @brief   A structure used to represent diagnostic request data
 *
 * This structure ised as an overlay for raw frame data.
 */
typedef struct LIN_PACKED lin_frame_diag_req_data
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t d1;
    uint8_t d2;
    uint8_t d3;
    uint8_t d4;
    uint8_t d5;
} lin_frame_diag_req_data_t;

static void struct_init(lindev_generic_t *connector_lindev);
static ddm_entry_t *handle_other_parameter_publish(lindev_generic_t *lindev_generic, const DDMP2_FRAME *ddmp2_frame);
static void handle_other_parameter_change(lindev_generic_t *lindev_generic, const ddm_entry_t *ddm_entry);
static void handle_other_parameter_reset_has_changed(lindev_generic_t *lindev_generic, ddm_entry_t *ddm_entry);
static size_t get_ddm_cb(void *arg, uint32_t ddm_parameter, void *ddm_data, size_t ddm_data_size);
static void set_ddm_cb(void *arg, uint32_t ddm_parameter, const void *ddm_data, size_t ddm_data_size);
static void process_ctrl_frame(lindev_generic_t *lindev_generic, uint_fast8_t initial_frame_id);
static void process_info_frame(lindev_generic_t *lindev_generic, uint_fast8_t initial_frame_id);
static bool frame_pid_is_valid(lindev_generic_t *lindev_generic, uint8_t index, uint_fast8_t new_frame_pid);
static void set_runtime_pid(lindev_generic_t *lindev_generic, uint8_t frame_index, uint_fast8_t new_frame_pid);
static bool save_nad(lindev_generic_t *lindev_generic, uint8_t new_nad);
static void restore_nad(lindev_generic_t *lindev_generic);
#if (LINDEV_GENERIC_SAVE_PID == 1)
static bool save_pid(lindev_generic_t *lindev_generic);
static void restore_pid(lindev_generic_t *lindev_generic);
#endif
static bool diag_req_resolve_function_and_variant(lindev_generic_t *lindev_generic, uint16_t req_function_id, uint16_t *selected_function_id, uint8_t *selected_variant_id);
static void process_diag_req_frame(lindev_generic_t *lindev_generic);
static void broker_set(const lindev_generic_t *lindev_generic, const ddm_entry_t *ddm_entry);
static void broker_subscribe(const lindev_generic_t *lindev_generic, const ddm_entry_t *ddm_entry);
static void broker_subscribe_parameter(const lindev_generic_t *lindev_generic, uint32_t parameter_id);
static bool is_model_discriminated_custom(lindev_generic_t *lindev_generic, const DDMP2_FRAME *frame);
static bool is_model_discriminated_generic(lindev_generic_t *lindev_generic, const DDMP2_FRAME *frame);
static void inventory_handler_cb_subscribe_class(lindev_generic_t *lindev_generic, uint32_t ddm_class_instance);
static void inventory_handler_cb_subscribe_discriminators_only(lindev_generic_t *lindev_generic, uint32_t ddm_class_instance);
static void inventory_handler_cb(void *arg, uint32_t ddm_class_instance, bool is_available);
static void worker_loop(lindev_generic_t *lindev_generic);
static void profile_worker(void *arg);
static void no_profile_worker(void *arg);

/**
 * @brief   Default UART configuration
 * Configure UART. Note that REF_TICK is used so that the baud rate remains
 * correct while APB frequency is changing in light sleep mode.
 */
static const uart_config_t default_uart_config = {
    .baud_rate = 19200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 0,
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    .source_clk = UART_SCLK_REF_TICK,
#else
    .source_clk = UART_SCLK_XTAL,
#endif
};

static void struct_init(lindev_generic_t *lindev_generic)
{
    lindev_table_status_t lindev_table_status;

    LINDEV_GENERIC_LOG(I, "initializing with configuration: %s", lindev_generic->descriptor->name);
    LINDEV_GENERIC_LOG(I, "DDM store with %u DDM parameters", lindev_generic->descriptor->ddm_other_initial_values_size);
    lindev_generic->device_config_function_variant_id_index = UINT8_MAX;
    if (lindev_generic->descriptor->device_config != NULL)
    {
        lindev_generic->device_config = *lindev_generic->descriptor->device_config;
        lindev_generic->is_device_config_available = true;
        TRUE_CHECK((lindev_generic->device_config.function_variant_ids_size == 0) || ((lindev_generic->device_config.function_variant_ids_size != 0) && (lindev_generic->device_config.function_ids != NULL) && (lindev_generic->device_config.variant_ids != NULL)))
    }
    else
    {
        memset(&lindev_generic->device_config, 0, sizeof(lindev_generic->device_config));
        lindev_generic->is_device_config_available = false;
    }
    lindev_generic->ddm_other_store = lindev_generic->descriptor->ddm_other_store;
    lindev_table_status = lindev_table_initialize_table_state(
        lindev_generic->descriptor->lindev_table_config,
        &lindev_generic->table_state);
    TRUE_CHECK(lindev_table_status == LINDEV_TABLE_STATUS_OK);
    lindev_generic->ddm_other_store_lock = xSemaphoreCreateMutexStatic(&lindev_generic->ddm_other_store_lock_buffer);
    TRUE_CHECK(lindev_generic->ddm_other_store_lock != NULL);
    lindev_generic->state = LINDEV_GENERIC_STATE_OPERATIONAL;
}

static ddm_entry_t *handle_other_parameter_publish(lindev_generic_t *lindev_generic,
                                                   const DDMP2_FRAME *ddmp2_frame)
{
    ddm_entry_t *ddm_entry;
    bool has_changed;

    // LOCK
    xSemaphoreTake(lindev_generic->ddm_other_store_lock, portMAX_DELAY);
    ddm_entry = ddm_store__access(lindev_generic->ddm_other_store, ddmp2_frame->frame.publish.parameter);

    if (ddm_entry == NULL)
    {
        xSemaphoreGive(lindev_generic->ddm_other_store_lock);
        return NULL;
    }
    has_changed = ddm_entry__set__value(
        ddm_entry,
        &ddmp2_frame->frame.publish.value,
        ddmp2_value_size(ddmp2_frame));
    // UNLOCK
    xSemaphoreGive(lindev_generic->ddm_other_store_lock);
    ddm_entry__set__has_changed_conditionally(ddm_entry, has_changed);
    return ddm_entry;
}

static void handle_other_parameter_change(lindev_generic_t *lindev_generic, const ddm_entry_t *ddm_entry)
{
    const lindev_table_config_t *lindev_table = lindev_generic->descriptor->lindev_table_config;
    uint32_t ddm_parameter = ddm_entry__parameter_id(ddm_entry);
    bool is_ddm_entry_mapped = false;

    /* Process DDM parameter listed in info link map */
    for (size_t i = 0u; i < lindev_table_get_info_link_map_length(lindev_table); i++)
    {
        const lindev_table_info_link_map_entry_t *link_entry;
        const lindev_table_info_bundle_t *info_bundle;
        const void *ddm_data[LINDEV_TABLE_MAX_DDM_PER_INFO_LINK_MAP_ENTRY];
        size_t ddm_data_size[LINDEV_TABLE_MAX_DDM_PER_INFO_LINK_MAP_ENTRY];
        lindev_frame_data_t frame_data;
        uint_fast8_t initial_frame_id;
        lindev_table_status_t lindev_table_status = LINDEV_TABLE_STATUS_OK;

        link_entry = lindev_table_match_ddm_with_link_map_entry(lindev_table, ddm_parameter, i);
        if (link_entry == NULL)
        {
            continue;
        }
        is_ddm_entry_mapped = true;
        if (ddm_entry__has_changed(ddm_entry))
        {
            /* Mark pending update in info bundle */
            lindev_table_status = lindev_table_info_set_pending_update(
                lindev_table,
                &lindev_generic->table_state,
                link_entry);
        }
        TRUE_CHECK_RETURN(lindev_table_status != LINDEV_TABLE_STATUS_ERROR);
        if (lindev_table_status == LINDEV_TABLE_STATUS_BUNDLE_NOT_AVAILABLE)
        {
            LINDEV_GENERIC_LOG(I, "Bundle map entry for for parameter %s0%s(%x) in frame 0x%x does not exits.",
                               ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                               ddm_entry__parameter_id(ddm_entry),
                               link_entry->bundle->frame_id);
            break;
        }
        for (size_t i = 0u; i < LINDEV_TABLE_MAX_DDM_PER_INFO_LINK_MAP_ENTRY; i++)
        {
            if (link_entry->ddm[i] == 0)
            {
                /*
                 * We reached at end of listed DDM parameters.
                 */
                break;
            }
            /* Get DDM value */
            ddm_entry = ddm_store__access(lindev_generic->ddm_other_store, link_entry->ddm[i]);
            if (ddm_entry == NULL)
            {
                LOG(W, "non-subscribed DDM 0x%x was listed in info link map at index %u", link_entry->ddm[i], i);
                return;
            }
            LINDEV_GENERIC_LOG(I, "processing DDM %s0%s(%x) value", ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry), ddm_entry__parameter_id(ddm_entry));
            ddm_entry__read__value(ddm_entry, &ddm_data[i], &ddm_data_size[i]);
        }
        info_bundle = lindev_table_info_conv_to_dometic(lindev_table, link_entry, ddm_data, ddm_data_size);
        lindev_table_info_bundle_stuff_to_frame(lindev_table, info_bundle, frame_data.bytes, &initial_frame_id);
        /* Set LIN frame data */
        lindev_set_frame_data(&lindev_generic->lindev, initial_frame_id, &frame_data);
    }
    /* Process other DDM parameters that are not listed in info link map */
    if (is_ddm_entry_mapped == false)
    {
        /* We are subscribed to some DDM, but it is not mapped as frame payload. For this situation
         * we just call ddm_entry device handler callback function. Complain if the callback is not
         * defined in product LINDEV.
         */
        if (lindev_generic->descriptor->cb_ddm_entry_has_changed == NULL)
        {
            /* We can use logging from DDM loop task */
            LOG(W,
                "tried to update DDM 0x%x(%s0%s) but no handler was defined",
                ddm_entry__parameter_id(ddm_entry),
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
        }
        else
        {
            LINDEV_GENERIC_LOG(I, "alternate DDM entry update 0x%x(%s0%s)",
                               ddm_entry__parameter_id(ddm_entry),
                               ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
            lindev_generic->descriptor->cb_ddm_entry_has_changed(ddm_entry);
        }
    }
}

static void handle_other_parameter_reset_has_changed(lindev_generic_t *lindev_generic, ddm_entry_t *ddm_entry)
{
    (void)lindev_generic;

    ddm_entry__set__has_changed(ddm_entry, false);
}

static size_t get_ddm_cb(void *arg, uint32_t ddm_parameter, void *ddm_data, size_t ddm_data_size)
{
    lindev_generic_t *lindev_generic = arg;
    ddm_entry_t *ddm_entry;
    const void *ddm_data_entry_buffer;
    size_t ddm_data_entry_buffer_size;

    ddm_entry = ddm_store__access(lindev_generic->ddm_other_store, ddm_parameter);
    if (ddm_entry == NULL)
    {
        LOG(W, "non-subscribed DDM 0x%x was listed in ctrl link map", ddm_parameter);
        return 0;
    }
    ddm_entry__read__value(ddm_entry, &ddm_data_entry_buffer, &ddm_data_entry_buffer_size);
    if ((ddm_data_entry_buffer == NULL) || (ddm_data_entry_buffer_size == 0u))
    {
        /* No value was found for this DDM entry. Just return. */
        return 0;
    }
    memcpy(ddm_data, ddm_data_entry_buffer, MIN(ddm_data_size, ddm_data_entry_buffer_size));
    return MIN(ddm_data_size, ddm_data_entry_buffer_size);
}

static void set_ddm_cb(void *arg, uint32_t ddm_parameter, const void *ddm_data, size_t ddm_data_size)
{
    lindev_generic_t *lindev_generic = arg;
    ddm_entry_t *ddm_entry;
    bool has_changed;

    // LOCK the DDM store
    xSemaphoreTake(lindev_generic->ddm_other_store_lock, portMAX_DELAY);
    ddm_entry = ddm_store__access(lindev_generic->ddm_other_store, ddm_parameter);
    if (ddm_entry == NULL)
    {
        LOG(W, "non-subscribed DDM 0x%x was listed in ctrl link map", ddm_parameter);
        xSemaphoreGive(lindev_generic->ddm_other_store_lock);
        return;
    }
    has_changed = ddm_entry__set__value(ddm_entry, ddm_data, ddm_data_size);

    // UNLOCK the DDM store
    xSemaphoreGive(lindev_generic->ddm_other_store_lock);
    if (has_changed)
    {
        if (ddm_entry__is_value_int32(ddm_entry))
        {
            LINDEV_GENERIC_LOG(I,
                               "DDM %s0%s(%x) has changed to %d",
                               ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                               ddm_entry__parameter_id(ddm_entry),
                               ddm_entry__value_i32(ddm_entry));
        }
        else
        {
            LINDEV_GENERIC_LOG(I,
                               "DDM %s0%s(%x) has changed",
                               ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                               ddm_entry__parameter_id(ddm_entry));
        }
        broker_set(lindev_generic, ddm_entry);
    }
}

static void process_ctrl_frame(lindev_generic_t *lindev_generic, uint_fast8_t initial_frame_id)
{
    const lindev_table_config_t *lindev_table = lindev_generic->descriptor->lindev_table_config;
    const lindev_table_ctrl_bundle_t *ctrl_bundle;
    const lindev_table_info_bundle_t *info_bundle;
    lindev_table_status_t lindev_table_status;
    lindev_frame_data_t ctrl_frame_data;
    lindev_frame_data_t info_frame_data;
    uint8_t ddm_workspace[DDMP2_MAX_VALUE_SIZE];
    uint_fast8_t info_initial_frame_id;

    lindev_get_frame_data(&lindev_generic->lindev, initial_frame_id, &ctrl_frame_data);
    ctrl_bundle = lindev_table_ctrl_extract_to_dometic(lindev_table, ctrl_frame_data.bytes, initial_frame_id);
    if (ctrl_bundle == NULL)
    {
        LINDEV_GENERIC_LOG(E, "No bundle was found for frame with ID: 0x%x", initial_frame_id);
        return;
    }
    lindev_table_status = lindev_table_ctrl_synchronize_with_info_bundle(
        /* lindev_table */ lindev_table,
        /* lindev_table_state */ &lindev_generic->table_state,
        /* ctrl_bundle */ ctrl_bundle,
        /* info_bundle [out] */ &info_bundle);
    TRUE_CHECK_RETURN(lindev_table_status != LINDEV_TABLE_STATUS_ERROR);
    switch (lindev_table_status)
    {
    case LINDEV_TABLE_STATUS_SYNC_NOT_AVAILABLE:
        /* NOTE:
         * This CTRL frame does not have sync bits defined in the protocol structure. In this case
         * we always apply data from CTRL frame. No INFO frame is being updated here.
         */
        LINDEV_GENERIC_LOG(I, "This CTRL bundle does not have synchronization protocol defined");
        /* Convert Dometic structure to DDM */
        lindev_table_ctrl_conv_to_ddm(lindev_table, ctrl_bundle, ddm_workspace, sizeof(ddm_workspace), get_ddm_cb, set_ddm_cb, lindev_generic);
        break;
    case LINDEV_TABLE_STATUS_SYNC_COMPLETED:
    case LINDEV_TABLE_STATUS_SYNC_PENDING:
        /* NOTE:
         * This CTRL frame does have sync bits, synchronizaton was pending or is still in process,
         * so now we just need to update INFO frame to reflect synchronization state changes and
         * ignore CTRL data.
         */
        LINDEV_GENERIC_LOG(I, "This CTRL bundle synchronization is ongoing or just completed");
        /*
         * We need to update associated INFO frame data to reflect local chage bit modifications by sync
         * protocol. Relevant sync protocol data was set by lindev_table_ctrl_synchronize_with_info_bundle
         * function, we just need to convert Dometic data structure to frame data.
         */
        /* Stuff existing data (including the sync protocol data) into frame data */
        lindev_table_info_bundle_stuff_to_frame(lindev_table, info_bundle, info_frame_data.bytes, &info_initial_frame_id);
        /* Set LIN frame data */
        lindev_set_frame_data(&lindev_generic->lindev, info_initial_frame_id, &info_frame_data);
        break;
    case LINDEV_TABLE_STATUS_SYNC_NOT_PENDING:
        LINDEV_GENERIC_LOG(I, "This CTRL bundle synchronization is done");
        /* NOTE:
         * We need to update associated INFO frame data to reflect local chage bit modifications by sync
         * protocol. Relevant sync protocol data was set by lindev_table_ctrl_synchronize_with_info_bundle
         * function, we just need to convert Dometic data structure to frame data.
         */
        /* Stuff existing data (including the sync protocol data) into frame data */
        lindev_table_info_bundle_stuff_to_frame(lindev_table, info_bundle, info_frame_data.bytes, &info_initial_frame_id);
        /* Set LIN frame data */
        lindev_set_frame_data(&lindev_generic->lindev, info_initial_frame_id, &info_frame_data);
        /* Convert Dometic structure to DDM */
        lindev_table_ctrl_conv_to_ddm(lindev_table, ctrl_bundle, ddm_workspace, sizeof(ddm_workspace), get_ddm_cb, set_ddm_cb, lindev_generic);
        break;
    default:
        break;
    }
}

static void process_info_frame(lindev_generic_t *lindev_generic, uint_fast8_t initial_frame_id)
{
    const lindev_table_config_t *lindev_table = lindev_generic->descriptor->lindev_table_config;
    lindev_frame_data_t info_frame_data;
    lindev_table_status_t status;

    status = lindev_table_info_stuff_next_page(lindev_table, initial_frame_id, info_frame_data.bytes);

    if (status == LINDEV_TABLE_STATUS_OK)
    {
        /* Set LIN frame data */
        lindev_set_frame_data(&lindev_generic->lindev, initial_frame_id, &info_frame_data);
    }
    else if (status == LINDEV_TABLE_STATUS_ERROR)
    {
        LOG(E, "Invalid bundle_map, could not find INFO frame with initial Frame ID: %u", initial_frame_id);
    }
}

static bool frame_pid_is_valid(lindev_generic_t *lindev_generic, uint8_t index, uint_fast8_t new_frame_pid)
{
    // Ignore "do not care" requests.
    if (new_frame_pid == FRAME_PID_WILDCARD)
    {
        return true;
    }

    // Unassign requests are not supported.
    if (new_frame_pid == 0x00u)
    {
        return false;
    }

    // Make sure that index is in range.
    if (index >= lindev_get_num_frame_defs(&lindev_generic->lindev))
    {
        return false;
    }

    // Make sure that the new ID is valid.
    if (LIN_IS_FRAME_PID_DIAG_REQUEST(new_frame_pid))
    {
        return false;
    }

    if (LIN_IS_FRAME_PID_DIAG_RESPONSE(new_frame_pid))
    {
        return false;
    }

    return true;
}

static void set_runtime_pid(lindev_generic_t *lindev_generic, uint8_t frame_index, uint_fast8_t new_frame_pid)
{
    if (frame_pid_is_valid(lindev_generic, frame_index, new_frame_pid) == false)
    {
        // Ignore setting invalid PIDs
        return;
    }
    // Ignore wildcards
    if (new_frame_pid == FRAME_PID_WILDCARD)
    {
        return;
    }
    lindev_set_frame_pid(&lindev_generic->lindev, frame_index, new_frame_pid);
}

static bool save_nad(lindev_generic_t *lindev_generic, uint8_t new_nad)
{
    esp_err_t esp_err;
    bool is_nad_saved;
    /* Save new NAD */
    esp_err = config_set_i32(LIN_NAD_CONFIG_STR, new_nad);
    if (esp_err == ESP_OK)
    {
        is_nad_saved = true;
        lindev_generic->device_config.nad = new_nad;
        LINDEV_GENERIC_LOG(I, "Assigned new NAD: %u", new_nad);
    }
    else
    {
        is_nad_saved = false;
        LINDEV_GENERIC_LOG(E, "Failed to save new NAD, got NVS error: %d", esp_err);
    }
    return is_nad_saved;
}

static void restore_nad(lindev_generic_t *lindev_generic)
{
    esp_err_t esp_err;
    int32_t configured_nad;
    /* Load NAD configuration if it exists. */
    esp_err = config_get_i32(LIN_NAD_CONFIG_STR, &configured_nad);
    if (esp_err == ESP_OK)
    {
        lindev_generic->device_config.nad = configured_nad;
        LOG(I, "%s(%s): using configured NAD: %u",
            lindev_generic->connector->name,
            lindev_generic->descriptor->name,
            lindev_generic->device_config.nad);
    }
    else
    {
        LOG(I, "%s(%s): using default NAD: %u",
            lindev_generic->connector->name,
            lindev_generic->descriptor->name,
            lindev_generic->device_config.nad);
    }
}

#if (LINDEV_GENERIC_SAVE_PID == 1)
static bool save_pid(lindev_generic_t *lindev_generic)
{
    bool is_pid_saved = true;
    for (size_t i = 0u; i < lindev_get_num_frame_defs(&lindev_generic->lindev); i++)
    {
        // Sizeof configuration string + 2 for max expected digits + 1 for NULL.
        char nvs_frame_name[sizeof(LIN_PID_CONFIG_STR) + 3u];
        uint32_t new_frame_pid;

        esp_err_t esp_err;
        /* Iterate over all frames and reassign PIDs if they were saved */
        int cx = snprintf(nvs_frame_name, sizeof(nvs_frame_name), LIN_PID_CONFIG_STR, i);
        if ((cx < 0) || (cx > (int)sizeof(nvs_frame_name)))
        {
            is_pid_saved = false;
            break;
        }
        new_frame_pid = lindev_get_frame_pid(&lindev_generic->lindev, i);
        esp_err = config_set_i32(nvs_frame_name, new_frame_pid);
        if (esp_err != ESP_OK)
        {
            is_pid_saved = false;
            break;
        }
        LINDEV_GENERIC_LOG(I, "saved %s := %u", nvs_frame_name, new_frame_pid);
    }
    /* If restoring of a frame PID was NOT successful then reset all PIDs to the initial ones. */
    if (is_pid_saved == false)
    {
        LOG(W, "failed to save frame PIDs, restoring");
        for (size_t i = 0u; i < lindev_get_num_frame_defs(&lindev_generic->lindev); i++)
        {
            uint32_t initial_frame_id;
            initial_frame_id = lindev_get_initial_frame_id(&lindev_generic->lindev, i);
            lindev_set_frame_id(&lindev_generic->lindev, i, initial_frame_id);
            LOG(W, "restored frame %u PID to 0x%x", i, initial_frame_id);
        }
    }
    return is_pid_saved;
}

static void restore_pid(lindev_generic_t *lindev_generic)
{
    bool is_pid_restored = true;
    /* Load Frame PIDs if they exist */
    for (size_t i = 0u; i < lindev_get_num_frame_defs(&lindev_generic->lindev); i++)
    {
        // Sizeof configuration string + 2 for max expected digits + 1 for NULL.
        char nvs_frame_name[sizeof(LIN_PID_CONFIG_STR) + 3u];
        int32_t new_frame_pid;
        esp_err_t esp_err;

        /* Iterate over all frames and reassign PIDs if they were saved */
        int cx = snprintf(nvs_frame_name, sizeof(nvs_frame_name), LIN_PID_CONFIG_STR, i);
        if ((cx < 0) || (cx > (int)sizeof(nvs_frame_name)))
        {
            is_pid_restored = false;
            break;
        }
        esp_err = config_get_i32(nvs_frame_name, &new_frame_pid);
        if (esp_err != ESP_OK)
        {
            is_pid_restored = false;
            break;
        }
        lindev_set_frame_pid(&lindev_generic->lindev, i, new_frame_pid);
        LOG(I, "loaded %s := 0x%x", nvs_frame_name, new_frame_pid);
    }
    /* If setting of a frame PID was NOT successful then reset all PIDs to the initial ones. */
    if (is_pid_restored == false)
    {
        LOG(I, "failed to load frame PIDs, using initial frame PIDs");
        for (size_t i = 0u; i < lindev_get_num_frame_defs(&lindev_generic->lindev); i++)
        {
            uint32_t initial_frame_id;
            initial_frame_id = lindev_get_initial_frame_id(&lindev_generic->lindev, i);
            lindev_set_frame_id(&lindev_generic->lindev, i, initial_frame_id);
            LOG(I, "using initial frame %u PID to 0x%x", i, initial_frame_id);
        }
    }
}
#endif /*  (LINDEV_GENERIC_SAVE_PID == 1) */

static bool diag_req_resolve_function_and_variant(lindev_generic_t *lindev_generic, uint16_t req_function_id, uint16_t *selected_function_id, uint8_t *selected_variant_id)
{
    bool is_function_id_valid;
    // Are we using a single Function ID?
    if (lindev_generic->device_config.function_variant_ids_size == 0)
    {
        /* Match against stored function ID or wildcard */
        is_function_id_valid = ((req_function_id == lindev_generic->device_config.function_id) || (req_function_id == LIN_FUNCTION_ID_WILDCARD));
        *selected_function_id = lindev_generic->device_config.function_id;
        *selected_variant_id = lindev_generic->device_config.variant_id;
    }
    else
    {
        // We are using multiple Function IDs, check do we have a selected Function ID?
        if (lindev_generic->device_config_function_variant_id_index == UINT8_MAX)
        {
            if (req_function_id == LIN_FUNCTION_ID_WILDCARD)
            {
                // By default select the first member (default one) when we receive wildcard
                lindev_generic->device_config_function_variant_id_index = 0u;
            }
            else
            {
                // The function ID is still not selected. See if it matches any given Function ID.
                for (uint8_t i = 0; i < lindev_generic->device_config.function_variant_ids_size; i++)
                {
                    if (lindev_generic->device_config.function_ids[i] == req_function_id)
                    {
                        lindev_generic->device_config_function_variant_id_index = i;
                        LOG(I, "Selected Function ID & Varian ID index: %u", lindev_generic->device_config_function_variant_id_index);
                        break;
                    }
                }
            }
        }
        // When we arrive at this point, we still might not have a selected Function ID
        if (lindev_generic->device_config_function_variant_id_index == UINT8_MAX)
        {
            is_function_id_valid = false;
            *selected_function_id = 0x0u;
            *selected_variant_id = 0x0u;
        }
        else
        {
            // At this point we have a selected Function ID & Variant ID, but we still need to check if provided Function ID matches ours
            is_function_id_valid = ((req_function_id == lindev_generic->device_config.function_ids[lindev_generic->device_config_function_variant_id_index]) || (req_function_id == LIN_FUNCTION_ID_WILDCARD));
            *selected_function_id = lindev_generic->device_config.function_ids[lindev_generic->device_config_function_variant_id_index];
            *selected_variant_id = lindev_generic->device_config.variant_ids[lindev_generic->device_config_function_variant_id_index];
        }
    }
    return is_function_id_valid;
}

static void process_diag_req_frame(lindev_generic_t *lindev_generic)
{
    lin_device_config_data_t *device_config = &lindev_generic->device_config;
    lindev_frame_data_t diag_req_frame_data;
    lindev_frame_data_t diag_resp_frame_data;
    const lin_frame_diag_req_data_t *request;
    static const lin_frame_diag_req_data_t sleep_request = {
        .nad = 0x00u,
        .pci = 0xffu,
        .sid = 0xffu,
        .d1 = 0xffu,
        .d2 = 0xffu,
        .d3 = 0xffu,
        .d4 = 0xffu,
        .d5 = 0xffu,
    };
    lindev_get_diag_req_frame_data(&lindev_generic->lindev, &diag_req_frame_data);
    /* Reinterpret frame data as diagnostic data */
    request = (const lin_frame_diag_req_data_t *)&diag_req_frame_data.bytes;

    if (memcmp(request, &sleep_request, sizeof(*request)) == 0)
    {
        lindev_sleep(&lindev_generic->lindev);
        return;
    }
    if (lindev_generic->is_device_config_available == false)
    {
        /* Since we don't have device config we will not reply to any diagnostic request message */
        return;
    }
    /* NAD match or wildcard. */
    if ((request->nad != device_config->nad) && (request->nad != 0x7fu))
    {
        /* Not for us. */
        LINDEV_GENERIC_LOG(I, "request NAD does not match ours: %u != %u", request->nad, device_config->nad);
        return;
    }
    /* Read by identifier */
    if ((request->pci == LIN_PCI_6) && (request->sid == LIN_SID_READ_BY_IDENTIFIER))
    {
        bool is_lpi_requested;  // Is LIN PRODUCT IDENTIFICATION requested
        bool is_sn_requested;   // Is SERIAL NUMBER requested
        bool is_supplier_id_valid;
        bool is_function_id_valid = false;
        uint16_t our_function_id = 0x0u;
        uint8_t our_variant_id = 0x0u;
        uint8_t identifier = request->d1;
        uint16_t supplier_id = request->d2 | ((uint16_t)request->d3 << 8);
        uint16_t function_id = request->d4 | ((uint16_t)request->d5 << 8);

        LINDEV_GENERIC_LOG(I, "identifier: %u, supplier_id: 0x%x, function_id: 0x%x", identifier, supplier_id, function_id);
        is_lpi_requested = (identifier == LIN_SID_READ_BY_IDENTIFIER_PRODUCT_IDENTIFICATION);
        is_sn_requested = (identifier == LIN_SID_READ_BY_IDENTIFIER_SERIAL_NUMBER);
        /* Match against stored supplier or wildcard */
        is_supplier_id_valid = ((supplier_id == device_config->supplier_id) || (supplier_id == 0x7fffu));
        /* Match against stored function ID or wildcard */
        /* Evaluate Function ID only if supplier is valid, because we now have a side effect when evaluating Function ID */
        if (is_supplier_id_valid)
        {
            is_function_id_valid = diag_req_resolve_function_and_variant(lindev_generic, function_id, &our_function_id, &our_variant_id);
        }

        LINDEV_GENERIC_LOG(I, "is_lpi_requested: %s, is_sn_requested: %s, is_supplier_id_valid: %s, is_function_id_valid: %s",
                           is_lpi_requested ? "true" : "false",
                           is_sn_requested ? "true" : "false",
                           is_supplier_id_valid ? "true" : "false",
                           is_function_id_valid ? "true" : "false");
        if (is_lpi_requested && is_supplier_id_valid && is_function_id_valid)
        {
            diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
            diag_resp_frame_data.bytes[1] = 0x06u;  // PCI
            diag_resp_frame_data.bytes[2] = 0xf2u;  // RSID
            diag_resp_frame_data.bytes[3] = (uint8_t)device_config->supplier_id;
            diag_resp_frame_data.bytes[4] = (uint8_t)(device_config->supplier_id >> 8u);
            diag_resp_frame_data.bytes[5] = (uint8_t)our_function_id;          // function_id from descriptor
            diag_resp_frame_data.bytes[6] = (uint8_t)(our_function_id >> 8u);  // function_id
            diag_resp_frame_data.bytes[7] = our_variant_id;
            lindev_set_diag_resp_frame_data(&lindev_generic->lindev, &diag_resp_frame_data);
        }
        else if (is_sn_requested && is_supplier_id_valid && is_function_id_valid)
        {
            diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
            diag_resp_frame_data.bytes[1] = 0x05u;  // PCI
            diag_resp_frame_data.bytes[2] = 0xf2u;  // RSID
            diag_resp_frame_data.bytes[3] = (uint8_t)(device_config->serial_number >> 0u);
            diag_resp_frame_data.bytes[4] = (uint8_t)(device_config->serial_number >> 8u);
            diag_resp_frame_data.bytes[5] = (uint8_t)(device_config->serial_number >> 16u);
            diag_resp_frame_data.bytes[6] = (uint8_t)(device_config->serial_number >> 24u);
            diag_resp_frame_data.bytes[7] = 0xffu;  // Unused
            lindev_set_diag_resp_frame_data(&lindev_generic->lindev, &diag_resp_frame_data);
        }
#if (LINDEV_GENERIC_DIAG_SHY_RESPONSE == 1)
        else if (is_supplier_id_valid && is_function_id_valid)
        /* Only respond negative when we have supplier_id/function_id match */
#else
        else
#endif
        {
            /* We did not get the supplier_id/function_id match or we don't support the provided
             * identifier.
             */
            LINDEV_GENERIC_LOG(I,
                               "We did not get the supplier_id(%x)/function_id(%x) match or invalid ID: %u",
                               supplier_id,
                               function_id,
                               identifier);
            diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
            diag_resp_frame_data.bytes[1] = 0x03u;                       // PCI
            diag_resp_frame_data.bytes[2] = 0x7fu;                       // RSID
            diag_resp_frame_data.bytes[3] = LIN_SID_READ_BY_IDENTIFIER;  // Requested SID
            diag_resp_frame_data.bytes[4] = 0x12u;                       // Error code
            diag_resp_frame_data.bytes[5] = 0xffu;                       // Unused
            diag_resp_frame_data.bytes[6] = 0xffu;                       // Unused
            diag_resp_frame_data.bytes[7] = 0xffu;                       // Unused
            lindev_set_diag_resp_frame_data(&lindev_generic->lindev, &diag_resp_frame_data);
        }
    }
    /* Assign Frame ID range */
    else if ((request->pci == LIN_PCI_6) && (request->sid == LIN_SID_ASSIGN_FRAME_ID_RANGE))
    {
        uint8_t start_index;
        uint8_t pid0;
        uint8_t pid1;
        uint8_t pid2;
        uint8_t pid3;
        bool is_every_pid_valid;

        start_index = request->d1;
        pid0 = request->d2;
        pid1 = request->d3;
        pid2 = request->d4;
        pid3 = request->d5;

        is_every_pid_valid =
            frame_pid_is_valid(lindev_generic, start_index + 0u, pid0) &&
            frame_pid_is_valid(lindev_generic, start_index + 1u, pid1) &&
            frame_pid_is_valid(lindev_generic, start_index + 2u, pid2) &&
            frame_pid_is_valid(lindev_generic, start_index + 3u, pid3);
        if (is_every_pid_valid)
        {
            bool is_pid_saved;

            LINDEV_GENERIC_LOG(I, "Renamed frames: %u, %u, %u, %u", pid0, pid1, pid2, pid3);
            set_runtime_pid(lindev_generic, start_index + 0u, pid0);
            set_runtime_pid(lindev_generic, start_index + 1u, pid1);
            set_runtime_pid(lindev_generic, start_index + 2u, pid2);
            set_runtime_pid(lindev_generic, start_index + 3u, pid3);
#if (LINDEV_GENERIC_SAVE_PID == 1)
            is_pid_saved = save_pid(lindev_generic);
#else
            is_pid_saved = true;
#endif
            if (is_pid_saved)
            {
                /* Positive response */
                diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
                diag_resp_frame_data.bytes[1] = 0x01u;  // PCI
                diag_resp_frame_data.bytes[2] = 0xf7u;  // RSID
                diag_resp_frame_data.bytes[3] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[4] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[5] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[6] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[7] = 0xffu;  // Unused
            }
            else
            {
                /* Error response */
                diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
                diag_resp_frame_data.bytes[1] = 0x03u;                          // PCI
                diag_resp_frame_data.bytes[2] = 0x7fu;                          // RSID
                diag_resp_frame_data.bytes[3] = LIN_SID_ASSIGN_FRAME_ID_RANGE;  // Requested SID
                diag_resp_frame_data.bytes[4] = 0xffu;                          // Unused
                diag_resp_frame_data.bytes[5] = 0xffu;                          // Unused
                diag_resp_frame_data.bytes[6] = 0xffu;                          // Unused
                diag_resp_frame_data.bytes[7] = 0xffu;                          // Unused
            }
        }
        else
        {
            /* Error response */
            diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
            diag_resp_frame_data.bytes[1] = 0x03u;                          // PCI
            diag_resp_frame_data.bytes[2] = 0x7fu;                          // RSID
            diag_resp_frame_data.bytes[3] = LIN_SID_ASSIGN_FRAME_ID_RANGE;  // Requested SID
            diag_resp_frame_data.bytes[4] = 0xffu;                          // Unused
            diag_resp_frame_data.bytes[5] = 0xffu;                          // Unused
            diag_resp_frame_data.bytes[6] = 0xffu;                          // Unused
            diag_resp_frame_data.bytes[7] = 0xffu;                          // Unused
        }
        /* Send positive or negative (error) response to master */
        lindev_set_diag_resp_frame_data(&lindev_generic->lindev, &diag_resp_frame_data);
    }
    /* Assign NAD diagnostic request */
    else if ((request->pci == LIN_PCI_6) && (request->sid == LIN_SID_ASSIGN_NAD))
    {
        uint16_t supplier_id = request->d1 | ((uint16_t)request->d2 << 8);
        uint16_t function_id = request->d3 | ((uint16_t)request->d4 << 8);
        uint8_t new_nad = request->d5;
        bool is_supplier_id_valid;
        bool is_function_id_valid = false;
        uint16_t our_function_id = 0x0;
        uint8_t our_variant_id = 0x0u;

        LINDEV_GENERIC_LOG(I, "Assign NAD: supplier_id: 0x%x, function_id: 0x%x, new NAD: %u", supplier_id, function_id, new_nad);
        /* Match against stored supplier ID  */
        is_supplier_id_valid = (supplier_id == device_config->supplier_id);
        // Match against stored function ID.
        // Evaluate Function ID only if Supplier ID is valid and Function ID is not a wildcard,
        // because we now have a side effect when evaluating Function ID.
        if ((is_supplier_id_valid) && (function_id != LIN_FUNCTION_ID_WILDCARD))
        {
            is_function_id_valid = diag_req_resolve_function_and_variant(lindev_generic, function_id, &our_function_id, &our_variant_id);
        }

        if (is_supplier_id_valid && is_function_id_valid)
        {
            bool is_nad_saved;

            is_nad_saved = save_nad(lindev_generic, new_nad);

            if (is_nad_saved)
            {
                diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
                diag_resp_frame_data.bytes[1] = 0x01u;  // PCI
                diag_resp_frame_data.bytes[2] = 0xf0u;  // RSID
                diag_resp_frame_data.bytes[3] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[4] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[5] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[6] = 0xffu;  // Unused
                diag_resp_frame_data.bytes[7] = 0xffu;  // Unused
                /* Send positive response */
                lindev_set_diag_resp_frame_data(&lindev_generic->lindev, &diag_resp_frame_data);
            }
            else
            {
                /* No negative (error) response, and no generic logging as well */
            }
        }
        else
        {
            /* No negative (error) response from us in this case */
        }
    }
    /* Send standard error reply for unsupported diagnostic frames */
    else
    {
        diag_resp_frame_data.bytes[0] = lindev_generic->device_config.nad;
        diag_resp_frame_data.bytes[1] = 0x03u;         // PCI
        diag_resp_frame_data.bytes[2] = 0x7fu;         // RSID
        diag_resp_frame_data.bytes[3] = request->sid;  // Requested SID
        diag_resp_frame_data.bytes[4] = 0xffu;         // Unused
        diag_resp_frame_data.bytes[5] = 0xffu;         // Unused
        diag_resp_frame_data.bytes[6] = 0xffu;         // Unused
        diag_resp_frame_data.bytes[7] = 0xffu;         // Unused
        /* Send negative (error) response */
        lindev_set_diag_resp_frame_data(&lindev_generic->lindev, &diag_resp_frame_data);
    }
}

static void broker_set(const lindev_generic_t *lindev_generic, const ddm_entry_t *ddm_entry)
{
    const void *ddm_data;
    size_t ddm_data_size;
    int32_t retval;

    ddm_entry__read__value(ddm_entry, &ddm_data, &ddm_data_size);

    retval = connector_send_frame_to_broker(
        /* control */ DDMP2_CONTROL_SET,
        /* parameter */ ddm_entry__parameter_id(ddm_entry),
        /* value */ ddm_data,
        /* value_size */ ddm_data_size,
        /* connector */ lindev_generic->connector->connector_id,
        /* timeout */ portMAX_DELAY);
    if (retval != 1)
    {
        LOG(W, "%s(%s): failed to set value to %s0%s(0x%x)",
            lindev_generic->connector->name,
            lindev_generic->descriptor->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
            ddm_entry__parameter_id(ddm_entry));
    };
}

static void broker_subscribe(const lindev_generic_t *lindev_generic, const ddm_entry_t *ddm_entry)
{
    broker_subscribe_parameter(lindev_generic, ddm_entry__parameter_id(ddm_entry));
}

static void broker_subscribe_parameter(const lindev_generic_t *lindev_generic, uint32_t parameter_id)
{
    int retval = connector_send_frame_to_broker(
        /* control */ DDMP2_CONTROL_SUBSCRIBE,
        /* parameter */ parameter_id,
        /* value */ NULL,
        /* value_size */ 0u,
        /* connector */ lindev_generic->connector->connector_id,
        /* timeout */ portMAX_DELAY);

    if (retval != 1)
    {
        LOG(W, "%s: failed to subscribe to parameter 0x%x", lindev_generic->connector->name, parameter_id);
    }
}

static bool is_model_discriminated_custom(lindev_generic_t *lindev_generic, const DDMP2_FRAME *frame)
{
    bool is_model_detected = false;

    switch (frame->frame.control)
    {
    case DDMP2_CONTROL_PUBLISH:
    {
        uint32_t parameter = frame->frame.publish.parameter;
        int32_t parameter_value = frame->frame.publish.value.int32;
        const lindev_generic_descriptor_t *descriptor = lindev_generic->profile->custom_profile.cb_discriminate_model(lindev_generic, parameter, parameter_value);
        if (descriptor)
        {
            lindev_generic->descriptor = descriptor;
            struct_init(lindev_generic);
            is_model_detected = true;
        }
        break;
    }
    default:
        LOG(W, "%s: unhandled frame %u", lindev_generic->connector->name, frame->frame.control);
        break;
    }

    return is_model_detected;
}

static bool is_model_discriminated_generic(lindev_generic_t *lindev_generic, const DDMP2_FRAME *frame)
{
    bool is_model_detected = false;

    switch (frame->frame.control)
    {
    case DDMP2_CONTROL_PUBLISH:
    {
        LINDEV_GENERIC_LOG(I, "Got discriminator DDM parameter (%x) value", frame->frame.publish.parameter);
        int32_t ddm_value = frame->frame.publish.value.int32;
        for (size_t i = 0u; i < lindev_generic->profile->generic_profile.entries_size; i++)
        {
            if ((int32_t)lindev_generic->profile->generic_profile.entries[i].ddm_value == ddm_value)
            {
                lindev_generic->descriptor = lindev_generic->profile->generic_profile.entries[i].descriptor;
                /* Reinitialize the rest of the struct */
                struct_init(lindev_generic);
                is_model_detected = true;
                break;
            }
        }
        break;
    }
    default:
        LOG(W, "%s: unhandled frame %u", lindev_generic->connector->name, frame->frame.control);
        break;
    }

    return is_model_detected;
}

/**
 * @brief Subscribe to all parameters in the "other" DDM store that belong to a given parameter class.
 *
 * @param lindev_generic Pointer to the lindev_generic context.
 * @param ddm_class_instance The parameter class to subscribe to.
 */
static void inventory_handler_cb_subscribe_class(lindev_generic_t *lindev_generic, uint32_t ddm_class_instance)
{
    size_t occupied;

    occupied = ddm_store__occupied(lindev_generic->ddm_other_store);

    for (size_t i = 0; i < occupied; i++)
    {
        ddm_entry_t *ddm_entry;

        ddm_store__iterate(lindev_generic->ddm_other_store, i, &ddm_entry);
        // Subscribe only to parameters of the given class
        if (DDM2_PARAMETER_CLASS_INSTANCE(ddm_entry__parameter_id(ddm_entry)) == ddm_class_instance)
        {
            /* subscribe to parameters */
            broker_subscribe(lindev_generic, ddm_entry);
        }
    }
}

/**
 * @brief Subscribe only to discriminator parameters that belong to a given parameter class.
 *
 * @param lindev_generic Pointer to the lindev_generic context.
 * @param ddm_class_instance The parameter class to subscribe to.
 */
static void inventory_handler_cb_subscribe_discriminators_only(lindev_generic_t *lindev_generic, uint32_t ddm_class_instance)
{
    for (size_t i = 0u; i < lindev_generic->profile->discriminator_list_size; i++)
    {
        uint32_t parameter = lindev_generic->profile->discriminator_list[i];
        // Subscribe only to parameters of the given class
        if (DDM2_PARAMETER_CLASS_INSTANCE(parameter) == ddm_class_instance)
        {
            // subscribe only to discriminator parameters
            broker_subscribe_parameter(lindev_generic, parameter);
        }
    }
}

/**
 * @brief Callback function for inventory handler updates.
 *
 * This function is called whenever a parameter class becomes available or unavailable.
 * If a parameter class becomes available, it subscribes to the relevant parameters. The subscription
 * depends on the current state of the lindev_generic instance (discriminating or normal operation).
 *
 * @param arg Pointer to the lindev_generic context.
 * @param ddm_class_instance The parameter class that has been updated.
 * @param is_available True if the parameter class is now available, false if it is no longer available.
 */
static void inventory_handler_cb(void *arg, uint32_t ddm_class_instance, bool is_available)
{
    lindev_generic_t *lindev_generic = arg;

    if (is_available)
    {
        if (lindev_generic->state == LINDEV_GENERIC_STATE_DISCRIMINATING)
        {
            inventory_handler_cb_subscribe_discriminators_only(lindev_generic, ddm_class_instance);
        }
        else
        {
            inventory_handler_cb_subscribe_class(lindev_generic, ddm_class_instance);
        }
    }
}

static void worker_loop(lindev_generic_t *lindev_generic)
{
    const lindev_generic_descriptor_t *descriptor = lindev_generic->descriptor;

    restore_nad(lindev_generic);
    ddm_store__load_entries(
        lindev_generic->ddm_other_store,
        descriptor->ddm_other_initial_values,
        descriptor->ddm_other_initial_values_size,
        0);
    lindev_init(
        &lindev_generic->lindev,
        descriptor->lindev_uart,
        &default_uart_config,
        descriptor->lindev_frame_defs,
        descriptor->lindev_frames,
        descriptor->lindev_frame_defs_size);
#if (LINDEV_GENERIC_SAVE_PID == 1)
    restore_pid(lindev_generic);
#endif
    lindev_start(&lindev_generic->lindev);
    if (descriptor->cb_ddm_loop_has_started != NULL)
    {
        descriptor->cb_ddm_loop_has_started();
    }
    // Restart inventory handler by adding all inventory classes again. This will also trigger
    // subscriptions to DDM store parameters via the inventory handler callback, resulting in
    // the lindev_generic instance being populated with the required DDM parameters.
    for (size_t i = 0u; i < lindev_generic->config->inventory_size; i++)
    {
        inventory_handler_add(&lindev_generic->inventory_handler, lindev_generic->config->inventory_list[i]);
    }
    inventory_handler_start(&lindev_generic->inventory_handler, lindev_generic->connector);
    for (;;)
    {
        DDMP2_FRAME *frame;
        size_t item_size = 0;
        frame = xRingbufferReceive(lindev_generic->connector->to_connector, &item_size, portMAX_DELAY);
        if (frame == NULL)
        {
            LOG(W, "%s(%s): got NULL frame", lindev_generic->connector->name, descriptor->name);
            continue;
        }
        /* Update inventory handler if needed */
        if (!inventory_handler_update(&lindev_generic->inventory_handler, frame))
        {
            switch (frame->frame.control)
            {
            case DDMP2_CONTROL_PUBLISH:
                if (lindev_generic->ddm_other_store != NULL)
                {
                    ddm_entry_t *ddm_entry;
                    ddm_entry = handle_other_parameter_publish(lindev_generic, frame);
                    if (ddm_entry != NULL)
                    {
                        if (ddm_entry__has_changed(ddm_entry))
                        {
                            if (ddm_entry__is_value_int32(ddm_entry))
                            {
                                LINDEV_GENERIC_LOG(I, "%s(%s): DDM \"%s0%s\" has been updated to: %d",
                                                   lindev_generic->connector->name,
                                                   lindev_generic->descriptor->name,
                                                   ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                                                   ddm_entry__value_i32(ddm_entry));
                            }
                            else if (ddm_entry__is_value_str(ddm_entry))
                            {
                                LINDEV_GENERIC_LOG(I, "%s(%s): DDM \"%s0%s\" has been updated to: %s",
                                                   lindev_generic->connector->name,
                                                   lindev_generic->descriptor->name,
                                                   ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                                                   ddm_entry__value_str(ddm_entry));
                            }
                            else
                            {
                                LINDEV_GENERIC_LOG(I, "%s(%s): DDM \"%s0%s\" has been updated",
                                                   lindev_generic->connector->name,
                                                   lindev_generic->descriptor->name,
                                                   ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
                            }
                        }
                        else
                        {
                            if (ddm_entry__is_value_int32(ddm_entry))
                            {
                                LINDEV_GENERIC_LOG(I, "%s(%s): DDM \"%s0%s\" has retained its old value: %d",
                                                   lindev_generic->connector->name,
                                                   lindev_generic->descriptor->name,
                                                   ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                                                   ddm_entry__value_i32(ddm_entry));
                            }
                            else if (ddm_entry__is_value_str(ddm_entry))
                            {
                                LINDEV_GENERIC_LOG(I, "%s(%s): DDM \"%s0%s\" has retained its old value: %s",
                                                   lindev_generic->connector->name,
                                                   lindev_generic->descriptor->name,
                                                   ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                                                   ddm_entry__value_str(ddm_entry));
                            }
                            else
                            {
                                LINDEV_GENERIC_LOG(I, "%s(%s): DDM \"%s0%s\" has retained its old value",
                                                   lindev_generic->connector->name,
                                                   lindev_generic->descriptor->name,
                                                   ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
                            }
                        }
                        handle_other_parameter_change(lindev_generic, ddm_entry);
                        handle_other_parameter_reset_has_changed(lindev_generic, ddm_entry);
                    }
                    else
                    {
                        /* Somebody tried to publish to us something that we haven't been subscribed to */
                        LOG(W,
                            "%s(%s): publish of non-subscribed DDM: 0x%x",
                            lindev_generic->connector->name,
                            lindev_generic->descriptor->name,
                            frame->frame.set.parameter);
                    }
                }
                break;
            case DDMP2_CONTROL_NOP:
            case DDMP2_CONTROL_FRAGMENT:
            case DDMP2_CONTROL_MESSAGE:
            case DDMP2_CONTROL_REG:
            case DDMP2_CONTROL_COUNT:
            default:
                LOG(W, "%s(%s): unhandled frame %u",
                    lindev_generic->connector->name,
                    descriptor->name,
                    frame->frame.control);
                break;
            }
        }
        vRingbufferReturnItem(lindev_generic->connector->to_connector, frame);
    }
}

static void profile_worker(void *arg)
{
    lindev_generic_t *lindev_generic = arg;
    bool (*selector_fn)(lindev_generic_t *, const DDMP2_FRAME *) = NULL;
    bool is_model_detected = false;

    // Check for config->inventory_list classes vs profile->discriminator_list classes
    // Iterate over the discriminator list and check that each class is present in the inventory list
    for (size_t i = 0u; i < lindev_generic->profile->discriminator_list_size; i++)
    {
        bool is_class_found = false;
        uint32_t discriminator_class = DDM2_PARAMETER_CLASS_INSTANCE(lindev_generic->profile->discriminator_list[i]);
        for (size_t j = 0u; j < lindev_generic->config->inventory_size; j++)
        {
            uint32_t inventory_class = lindev_generic->config->inventory_list[j];
            if (discriminator_class == inventory_class)
            {
                is_class_found = true;
                break;
            }
        }
        if (!is_class_found)
        {
            LOG(E, "%s: discriminator class 0x%x not found in inventory list",
                lindev_generic->connector->name,
                discriminator_class);
            vTaskDelete(worker_task_handle);
            return;
        }
    }
    // Setup inventory based on lindev generic configuration
    inventory_handler_init(
        &lindev_generic->inventory_handler,
        lindev_generic->config->inventory_sortedlist,
        inventory_handler_cb,
        lindev_generic);
    LINDEV_GENERIC_LOG(I, "%s: DDM inventory classes: %u",
                       lindev_generic->connector->name,
                       lindev_generic->config->inventory_size);
    // Add all inventory items from the lindev generic configuration to the inventory handler
    for (size_t i = 0u; i < lindev_generic->config->inventory_size; i++)
    {
        inventory_handler_add(&lindev_generic->inventory_handler, lindev_generic->config->inventory_list[i]);
    }

    switch (lindev_generic->profile->profile_type)
    {
    case LINDEV_GENERIC_PROFILE_TYPE_GENERIC:
        selector_fn = is_model_discriminated_generic;
        break;
    case LINDEV_GENERIC_PROFILE_TYPE_CUSTOM:
        selector_fn = is_model_discriminated_custom;
        break;
    default:
        LOG(E, "%s: invalid profile type %d", lindev_generic->connector->name, lindev_generic->profile->profile_type);
        vTaskDelete(worker_task_handle);
        return;
    }

    // Ring buffer for looping until we have discriminated the model
    inventory_handler_start(&lindev_generic->inventory_handler, lindev_generic->connector);
    for (;;)
    {
        DDMP2_FRAME *frame;
        size_t item_size = 0;

        frame = xRingbufferReceive(lindev_generic->connector->to_connector, &item_size, portMAX_DELAY);
        if (frame == NULL)
        {
            LOG(W, "%s: got NULL frame", lindev_generic->connector->name);
            continue;
        }

        if (!inventory_handler_update(&lindev_generic->inventory_handler, frame))
        {
            is_model_detected = selector_fn(lindev_generic, frame);
        }

        // return frame to ring buffer
        vRingbufferReturnItem(lindev_generic->connector->to_connector, frame);
        if (is_model_detected)
        {
            break;  // exit for loop
        }
    }

    worker_loop(lindev_generic);
}

static void no_profile_worker(void *arg)
{
    lindev_generic_t *lindev_generic = arg;

    struct_init(lindev_generic);

    // Setup inventory based on lindev_generic_config_t
    inventory_handler_init(
        &lindev_generic->inventory_handler,
        lindev_generic->config->inventory_sortedlist,
        inventory_handler_cb,
        lindev_generic);
    LINDEV_GENERIC_LOG(I, "%s(%s): DDM inventory classes: %u",
                       lindev_generic->connector->name,
                       lindev_generic->descriptor->name,
                       lindev_generic->config->inventory_size);

    worker_loop(lindev_generic);
}

int lindev_generic_init_with_profile(
    lindev_generic_t *connector_lindev,
    CONNECTOR *connector,
    const lindev_uart_config_t *uart_config,
    const lindev_generic_config_t *config,
    const lindev_generic_profile_t *profile)
{
    BaseType_t error = pdFALSE;

    TRUE_CHECK_RETURNX(CONNECTOR_INIT_FAILURE, profile->discriminator_list != NULL);
    TRUE_CHECK_RETURNX(CONNECTOR_INIT_FAILURE, profile->discriminator_list_size > 0u);
    if (profile->profile_type == LINDEV_GENERIC_PROFILE_TYPE_GENERIC)
    {
        TRUE_CHECK_RETURNX(CONNECTOR_INIT_FAILURE, profile->generic_profile.entries_size > 0u);
        for (size_t i = 0u; i < profile->generic_profile.entries_size; i++)
        {
            LOG(I, "initializing %s with profile %u: %s",
                connector->name,
                i,
                profile->generic_profile.entries[i].descriptor->name);
        }
    }
    else
    {
        TRUE_CHECK_RETURNX(CONNECTOR_INIT_FAILURE, profile->custom_profile.cb_discriminate_model);
    }
    LOG(I, "Task stack size %u and priority %u", config->worker_stack_size, config->worker_priority);
    LOG(I, "Context RAM usage: %u bytes", sizeof(lindev_generic_t));
    connector_lindev->profile = profile;
    connector_lindev->descriptor = NULL;
    connector_lindev->config = config;
    connector_lindev->connector = connector;
    connector_lindev->state = LINDEV_GENERIC_STATE_DISCRIMINATING;
    lindev_preinit(&connector_lindev->lindev, uart_config);
    error = xTaskCreate(
        /* code       */ profile_worker,
        /* name       */ connector_lindev->connector->name,
        /* stack size */ config->worker_stack_size,
        /* parameters */ connector_lindev,
        /* priority   */ config->worker_priority,
        /* handle     */ NULL);

    return error == pdPASS ? CONNECTOR_INIT_SUCCESS : CONNECTOR_INIT_FAILURE;
}

int lindev_generic_init(
    lindev_generic_t *connector_lindev,
    CONNECTOR *connector,
    const lindev_uart_config_t *uart_config,
    const lindev_generic_config_t *config,
    const lindev_generic_descriptor_t *descriptor)
{
    BaseType_t error;

    LOG(I, "Task stack size %u and priority %u", config->worker_stack_size, config->worker_priority);
    LOG(I, "Context RAM usage: %u bytes", sizeof(lindev_generic_t));
    connector_lindev->profile = NULL;
    connector_lindev->descriptor = descriptor;
    connector_lindev->config = config;
    connector_lindev->connector = connector;
    // NOTE: No discrimination needed: connector_lindev->state = LINDEV_GENERIC_STATE_DISCRIMINATING;
    lindev_preinit(&connector_lindev->lindev, uart_config);
    error = xTaskCreate(
        /* code       */ no_profile_worker,
        /* name       */ connector_lindev->connector->name,
        /* stack size */ config->worker_stack_size,
        /* parameters */ connector_lindev,
        /* priority   */ config->worker_priority,
        /* handle     */ NULL);

    return error == pdPASS ? CONNECTOR_INIT_SUCCESS : CONNECTOR_INIT_FAILURE;
}

void lindev_generic_process_event(lindev_generic_t *context, const lindev_event_id_t event_id, size_t frame_index)
{
    switch (event_id)
    {
    case LINDEV_EVENT_ID_CONTROL_FRAME:  //!< Control frame was successfully received from master
        process_ctrl_frame(context, lindev_get_initial_frame_id(&context->lindev, frame_index));
        break;
    case LINDEV_EVENT_ID_DIAG_REQ_FRAME:
        process_diag_req_frame(context);  //!< Diagnostic request frame was received from master
        break;
    case LINDEV_EVENT_ID_INFO_FRAME:  //!< Info frame was successfully sent to master
        process_info_frame(context, lindev_get_initial_frame_id(&context->lindev, frame_index));
        break;
    case LINDEV_EVENT_ID_DIAG_RESP_FRAME:  //!< Diagnostic response frame was successfully sent to master
        /* We are currently not using these events. */
        break;
    case LINDEV_EVENT_ID_NOT_FOR_US:
        /* We don't need to do anything if a frame is not for us */
        break;
    case LINDEV_EVENT_ID_ERROR_RX_CHECKSUM:         //!< Received frame checksum is invalid
    case LINDEV_EVENT_ID_ERROR_RX_HEADER:           //!< Received frame header is invalid
    case LINDEV_EVENT_ID_ERROR_RX_FIFO_FULL:        //!< While receiving, detected that the internal FIFO is full
    case LINDEV_EVENT_ID_ERROR_RX_FIFO_OVF:         //!< While receiving, detected that the internal FIFO overflowed
    case LINDEV_EVENT_ID_ERROR_TX_CHECKSUM:         //!< While transmitting, detected that checksum is invalid
    case LINDEV_EVENT_ID_ERROR_TX_LENGTH:           //!< While transmitting, detected that all bytes were not sent
    case LINDEV_EVENT_ID_ERROR_TX_FIFO_FULL:        //!< While transmitting, detected that the internal FIFO is full
    case LINDEV_EVENT_ID_ERROR_TX_FIFO_OVF:         //!< While transmitting, detected that the internal FIFO overflowed
    case LINDEV_EVENT_ID_ERROR_TX_BRK:              //!< While transmitting, detected a BREAK
    case LINDEV_EVENT_ID_ERROR_BRK_FIFO_FULL:       //!< While waiting for BREAK, detected that the internal FIFO is full
    case LINDEV_EVENT_ID_ERROR_BRK_FIFO_OVF:        //!< While waiting for BREAK, detected that the internal FIFO overflowed
    case LINDEV_EVENT_ID_ERROR_UNKNOWN_IRQ:         //!< Unknown UART IRQ occured
    case LINDEV_EVENT_ID_ERROR_INTERNAL_OVF:        //!< Internal buffer overflow
    case LINDEV_EVENT_ID_ERROR_INVALID_FRAME_TYPE:  //!< Found a frame with invalid frame type
        /* NOTE: Do not log here, as that will impact LIN code execution time */
        break;
    default:
        break;
    }
}

void lindev_generic_disable(lindev_generic_t *lindev_generic)
{
    lindev_disable(&lindev_generic->lindev);
}

void lindev_generic_enable(lindev_generic_t *lindev_generic)
{
    lindev_enable(&lindev_generic->lindev);
}

void lindev_generic_set_serial_number(lindev_generic_t *lindev_generic, uint32_t serial_number)
{
    lindev_generic->device_config.serial_number = serial_number;
}

void lindev_generic_reset_config(lindev_generic_t *lindev_generic)
{
    lindev_generic->device_config.nad = lindev_generic->descriptor->device_config->nad;
}
