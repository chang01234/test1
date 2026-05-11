#include "configuration.h"

#include "lin_server.h"
#include "lin_server_definition.h"
#include "lin_server_production.h"
#include "lin_server_scheduler.h"

#include "ddm_store.h"

#define LIN_SERVER_GENERIC_TIMER_EVENT        0x01
#define LIN_SERVER_GENERIC_SWITCH_STATE_EVENT 0x02
#define LIN_SERVER_GO_TO_SLEEP_TIMEOUT_MS     (3 * 1000)  // 3s
#define LIN_SERVER_WAKE_UP_TIMEOUT_MS         (5 * 1000)  // 5s

static void lin_server_switch_timer_mode(void);
static lin_server_state_t lin_server_get_state(void);
static bool lin_server_is_in_current_state(lin_server_state_t state_to_check);
static void lin_server_set_state(lin_server_state_t state);

static int initialize_frames(const lin_server_slave_device_t *slave_device);

static void lin_server_ddm_storage_verbose_logging(const lin_server_slave_device_t *const device);
static void lin_server_ddm_parameter_update_verbose_logging(const lin_server_slave_device_t *device, const ddm_entry_t *ddm_entry);

static void create_slave_entries(const lin_server_slave_device_t *const slave_device);
static bool handle_owned_parameter_write(ddm_entry_t *ddm_entry, const void *data, size_t data_size);
static void handle_owned_parameter_publish(const lin_server_slave_device_t *device, ddm_entry_t *entry);
static void handle_owned_parameter_storage_publish(const lin_server_slave_device_t *device, ddm_store_t *store);
static void handle_owned_parameter_reset_has_changed(ddm_entry_t *ddm_entry);

static const lin_server_device_frames_bundle_def_t *lin_server_get_frame_bundle_definition_by_ctrl_frame(const lin_server_slave_device_t *device, uint8_t frame_id);
static const lin_server_device_frames_bundle_def_t *lin_server_get_frame_bundle_definition_by_info_frame(const lin_server_slave_device_t *device, uint8_t frame_id);

const lin_server_device_frame_def_t *lin_server_get_frame_definition_by_ddm2_parameter(const lin_server_slave_device_t *device, lin_frame_type_t frame_type, uint32_t ddm2_parameter);
static int invoke_ddm2_to_lin_conversion_functions(const lin_server_slave_device_t *device, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_buffer_def);
static void handle_lin_to_ddm_device_signals_conversion(const lin_server_slave_device_t *device, const lin_server_device_frame_def_t *info_frame_def);
static void handle_lin_to_ddm_device_function_specific_signals_conversion(const lin_server_slave_device_t *device, const lin_server_device_frame_def_t *info_frame_def);

static const lin_server_map_ddm_to_lin_t *ddm_to_lin_device_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter);
static const lin_server_map_ddm_to_lin_t *ddm_to_lin_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter);
static const lin_server_map_ddm_to_lin_t *ddm_to_lin_get_function_specific_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_updated_prameter);
static const lin_server_map_lin_to_ddm_t *lin_to_ddm_device_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter);
static const lin_server_map_lin_to_ddm_t *lin_to_ddm_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter);
static const lin_server_map_lin_to_ddm_t *lin_to_ddm_get_function_specific_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_updated_prameter);

static int ddm2_to_lin_ctrl_logic_init(const lin_server_slave_device_t *device);
static void ddm2_to_lin_ctrl_logic_remove(const lin_server_slave_device_t *device);
static void ddm2_to_lin_ctrl_logic_set_table_index(const lin_server_slave_device_t *device, int index);
static int ddm2_to_lin_ctrl_logic_get_table_index(const lin_server_slave_device_t *device);
static void ddm2_to_lin_ctrl_logic_clear_table_index(const lin_server_slave_device_t *device);
static bool ddm2_to_lin_ctrl_logic_is_registered_for_device(const lin_server_slave_device_t *device);
static bool ddm2_to_lin_ctrl_logic_is_pre_send_registered(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table);
static bool ddm2_to_lin_ctrl_logic_is_post_send_registered(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table);
static void ddm2_to_lin_ctrl_logic_prepare_for_conversion(const lin_server_slave_device_t *device, const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table);
static int ddm2_to_lin_ctrl_logic_get_handle_index_by_ddm_parameter(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table, size_t ctrl_logic_table_size, uint32_t parameter);
static const lin_server_ddm2_to_lin_ctrl_logic_t *ddm2_to_lin_ctrl_logic_get_handle_by_index(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table, size_t ctrl_logic_table_size, int index);
static const lin_server_ddm2_to_lin_ctrl_logic_t *ddm2_to_lin_ctrl_logic_find_handle_by_ddm_parameter(const lin_server_slave_device_t *device, uint32_t parameter, int *index);
static void lin_to_ddm2_info_logic_post_received_functions(const lin_server_slave_device_t *device, const lin_server_device_frame_def_t *info_frame_def, const lin_server_device_frame_def_t *ctrl_frame_def);
static bool lin_to_ddm2_info_logic_is_registered_for_device(const lin_server_slave_device_t *device);
static bool lin_to_ddm2_info_logic_is_post_received_registered(const lin_server_lin_to_ddm2_info_logic_t *info_logic_table);

typedef struct lin_server
{
    lin_server_state_t state;
    TimerHandle_t lin_server_sleep_timer_handle;
    StaticTimer_t lin_server_sleep_timer;
} lin_server_t;

static EXT_RAM_ATTR lin_server_t lin_server;

static void lin_server_ddm_storage_verbose_logging(const lin_server_slave_device_t *const device)
{
    const void *owned_data_storage;
    size_t owned_data_storage_size;
    const void *heap_data_storage;
    size_t heap_data_storage_size;
    size_t owned_occupied = ddm_store__occupied(device->ddm_owned_store);
    size_t heap_occupied = ddm_store__occupied(device->data->heap_store);

    if (owned_occupied != heap_occupied)
    {
        LOG(E, "Storages differes in size. Owned:%d vs Heap:%d",
            owned_occupied,
            heap_occupied);
        return;
    }
    for (size_t index = 0; index < owned_occupied; ++index)
    {
        ddm_entry_t *ddm_entry_owned;
        ddm_entry_t *ddm_entry_heap;
        ddm_store__iterate(device->ddm_owned_store, index, &ddm_entry_owned);
        ddm_store__iterate(device->data->heap_store, index, &ddm_entry_heap);

        ddm_entry__read__value(ddm_entry_owned, &owned_data_storage, &owned_data_storage_size);
        ddm_entry__read__value(ddm_entry_heap, &heap_data_storage, &heap_data_storage_size);

        if (memcmp(owned_data_storage, heap_data_storage, MIN(owned_data_storage_size, heap_data_storage_size) != 0))
        {
            LOG(D, "'%s0%s' differs.", ddm_entry__device_class(ddm_entry_owned), ddm_entry__property(ddm_entry_owned));

            if (ddm_entry__is_value_int32(ddm_entry_owned))
            {
                LOG(D, " Owned storage: %d, Heap storage: %d",
                    ddm_entry__value_i32(ddm_entry_owned),
                    ddm_entry__value_i32(ddm_entry_heap));
            }
            else if (ddm_entry__is_value_str(ddm_entry_owned))
            {
                LOG(D, " Owned storage: %s, Heap storage: %s",
                    ddm_entry__value_str(ddm_entry_owned),
                    ddm_entry__value_str(ddm_entry_heap));
            }
            else if (ddm_entry__is_value_struct(ddm_entry_owned) ||
                     ddm_entry__is_value_other(ddm_entry_owned))
            {
                LOG(D, " Owned storage: ");
                for (size_t i = 0; i < owned_data_storage_size; i++)
                {
                    LOG(D, " %02X ", ((uint8_t *)owned_data_storage)[i]);
                }
                LOG(D, " Heap storage: ");
                for (size_t i = 0; i < heap_data_storage_size; i++)
                {
                    LOG(D, " %02X ", ((uint8_t *)heap_data_storage)[i]);
                }
            }
            else
            {
                // Do nothing
            }
        }
    }
}

static void lin_server_ddm_parameter_update_verbose_logging(const lin_server_slave_device_t *device, const ddm_entry_t *ddm_entry)
{
    const void *data;
    size_t data_size;

    if (ddm_entry__has_changed(ddm_entry))
    {
        if (ddm_entry__is_value_int32(ddm_entry))
        {
            LOG(D, "%s: DDM \"%s0%s\" has been updated to: %d",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                ddm_entry__value_i32(ddm_entry));
        }
        else if (ddm_entry__is_value_str(ddm_entry))
        {
            LOG(D, "%s: DDM \"%s0%s\" has been updated to: %s",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                ddm_entry__value_str(ddm_entry));
        }
        else if (ddm_entry__is_value_struct(ddm_entry))
        {
            ddm_entry__read__value(ddm_entry, &data, &data_size);

            LOG(D, "%s: DDM \"%s0%s\" has been updated to: ",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));

            for (size_t i = 0; i < data_size; i++)
            {
                LOG(D, "%02X ", ((uint8_t *)data)[i]);
            }
        }
        else if (ddm_entry__is_value_other(ddm_entry))
        {
            ddm_entry__read__value(ddm_entry, &data, &data_size);

            LOG(D, "%s: DDM \"%s0%s\" has been updated to: ",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));

            for (size_t i = 0; i < data_size; i++)
            {
                LOG(D, "%02X ", ((uint8_t *)data)[i]);
            }
        }
        else
        {
            LOG(D, "%s: DDM \"%s0%s\" has been updated",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
        }
    }
    else
    {
        if (ddm_entry__is_value_int32(ddm_entry))
        {
            LOG(D, "%s: DDM \"%s0%s\" has retained its old value: %d",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                ddm_entry__value_i32(ddm_entry));
        }
        else if (ddm_entry__is_value_str(ddm_entry))
        {
            LOG(D, "%s: DDM \"%s0%s\" has retained its old value: %s",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
                ddm_entry__value_str(ddm_entry));
        }
        else if (ddm_entry__is_value_struct(ddm_entry))
        {
            ddm_entry__read__value(ddm_entry, &data, &data_size);

            LOG(D, "%s: DDM \"%s0%s\" has retained its old value:",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));

            for (size_t i = 0; i < data_size; i++)
            {
                LOG(D, "%02X ", ((uint8_t *)data)[i]);
            }
        }
        else if (ddm_entry__is_value_other(ddm_entry))
        {
            ddm_entry__read__value(ddm_entry, &data, &data_size);

            LOG(D, "%s: DDM \"%s0%s\" has retained its old value:",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));

            for (size_t i = 0; i < data_size; i++)
            {
                LOG(D, "%02X ", ((uint8_t *)data)[i]);
            }
        }
        else
        {
            LOG(D, "%s: DDM \"%s0%s\" has retained its old value",
                device->name,
                ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
        }
    }
}

static void create_slave_entries(const lin_server_slave_device_t *const slave_device)
{
    ddm_store__load_entries(
        slave_device->ddm_owned_store,
        slave_device->ddm_owned_initial_values,
        slave_device->ddm_owned_initial_values_size,
        0);
}

static void delete_slave_entries(const lin_server_slave_device_t *const slave_device)
{
    ddm_store__delete_all(slave_device->ddm_owned_store);
}

static void handle_owned_parameter_storage_publish(const lin_server_slave_device_t *device, ddm_store_t *store)
{
    for (size_t index = 0; index < ddm_store__occupied(store); ++index)
    {
        ddm_entry_t *ddm_entry;
        ddm_store__iterate(store, index, &ddm_entry);

        if (ddm_entry__has_changed(ddm_entry))
        {
            handle_owned_parameter_publish(device, ddm_entry);
        }
    }
}

static void handle_owned_parameter_publish(const lin_server_slave_device_t *device, ddm_entry_t *entry)
{
    const void *ddm_entry_value;
    size_t ddm_entry_value_size;

    ddm_entry__read__value(entry, &ddm_entry_value, &ddm_entry_value_size);
    lin_server_publish(
        ddm_entry__parameter_id(entry),
        device->data->class_instance,
        ddm_entry_value,
        ddm_entry_value_size);

    lin_server_ddm_parameter_update_verbose_logging(device, entry);

    handle_owned_parameter_reset_has_changed(entry);
}

static bool handle_owned_parameter_write(ddm_entry_t *ddm_entry, const void *data, size_t data_size)
{
    bool has_changed;

    has_changed = ddm_entry__set__value(
        ddm_entry,
        data,
        data_size);
    ddm_entry__set__has_changed_conditionally(ddm_entry, has_changed);

    return has_changed;
}

static void handle_owned_parameter_reset_has_changed(ddm_entry_t *ddm_entry)
{
    ddm_entry__set__has_changed(ddm_entry, false);
}

static int initialize_frames(const lin_server_slave_device_t *slave_device)
{
    bool ctrl_frame_exists = false;
    bool info_frame_exists = false;
    ddm_entry_t *ddm_entries[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
    const uint8_t *ddm_data[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
    size_t ddm_data_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
    lin_server_device_frame_t *slave_device_ctrl_frame = NULL;
    lin_server_device_frame_t *slave_device_info_frame = NULL;

    TRUE_CHECK_RETURN0(slave_device->frames_bundle_defs);
    TRUE_CHECK_RETURN0(slave_device->frames_bundle_defs_size > 0);

    /* Initialize frames memory */
    for (size_t i = 0; i < slave_device->frames_bundle_defs_size; ++i)
    {
        if (slave_device->frames_bundle_defs[i].ctrl_frame_def)
        {
            ctrl_frame_exists = true;
            slave_device_ctrl_frame = slave_device->frames_bundle_defs[i].ctrl_frame_def->frame;
            memset(slave_device_ctrl_frame, 0, sizeof(lin_server_device_frame_t));

            lin_server_reset_ctrl_frame_verification(slave_device_ctrl_frame);
        }
        if (slave_device->frames_bundle_defs[i].info_frame_def)
        {
            info_frame_exists = true;
            slave_device_info_frame = slave_device->frames_bundle_defs[i].info_frame_def->frame;
            memset(slave_device_info_frame, 0, sizeof(lin_server_device_frame_t));
        }
    }

    /* Error only if ctrl_frame exists, but map_ddm_to_lin and map_ddm_to_lin_size are not set */
    TRUE_CHECK_RETURN0(!ctrl_frame_exists || (slave_device->map_ddm_to_lin && slave_device->map_ddm_to_lin_size > 0));
    /* Error only if info_frame exists, but map_lin_to_ddm and map_lin_to_ddm_size are not set */
    TRUE_CHECK_RETURN0(!info_frame_exists || (slave_device->map_lin_to_ddm && slave_device->map_lin_to_ddm_size > 0));

    for (size_t j = 0; j < slave_device->map_ddm_to_lin_size; j++)
    {
        const lin_server_map_ddm_to_lin_t *link_entry = &slave_device->map_ddm_to_lin[j];

        /* Error if mapped member hasn't specified ddm to lin conversion cb */
        TRUE_CHECK_RETURN0(link_entry->convert_ddm_to_lin);
        /* Error if mapped member hasn't specified ctrl frame definition */
        TRUE_CHECK_RETURN0(link_entry->ctrl_frame_def);

        slave_device_ctrl_frame = link_entry->ctrl_frame_def->frame;
        for (size_t i = 0u; i < LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY; i++)
        {
            if (link_entry->ddm[i] == 0)
            {
                break; /*We reached at end of listed DDM parameters */
            }

            /* Error if there is non-handled DDM listed in ctrl link map */
            TRUE_CHECK_RETURN0(ddm_entries[i] = ddm_store__access(slave_device->ddm_owned_store, link_entry->ddm[i]));

            LOG(D, "initializing DDM %s0%s(%x) value", ddm_entry__device_class(ddm_entries[i]), ddm_entry__property(ddm_entries[i]), ddm_entry__parameter_id(ddm_entries[i]));
            ddm_entry__read__value(ddm_entries[i], (const void **)&ddm_data[i], &ddm_data_size[i]);
        }

        // Execute specific DDM to LIN converstion function and prepare CTRL frame
        link_entry->convert_ddm_to_lin(
            ddm_data,
            ddm_data_size,
            &slave_device_ctrl_frame->frame_signals);
    }

    for (size_t j = 0; j < slave_device->map_ddm_to_lin_size; j++)
    {
        const lin_server_map_lin_to_ddm_t *link_entry = &slave_device->map_lin_to_ddm[j];

        /* Error if mapped member hasn't specified lin to ddm conversion cb */
        TRUE_CHECK_RETURN0(link_entry->convert_lin_to_ddm);
        /* Error if mapped member hasn't specified info frame definition */
        TRUE_CHECK_RETURN0(link_entry->info_frame_def)
    }

    return 1;
}

int lin_server_discriminate_model(const lin_server_slave_device_t *slave_device)
{
    const lin_server_generic_profile_t *generic_profile = slave_device->generic_profile;

    if (generic_profile != NULL)
    {
        bool is_model_set = false;
        uint32_t device_model = 0;
        uint8_t active_variant_id = slave_device->data->active_variant_id;
        uint16_t active_function_id = slave_device->data->active_config_data->function_id;

        // Generic profile entires not set
        TRUE_CHECK_RETURN0(generic_profile->profile_entry != NULL);
        TRUE_CHECK_RETURN0(generic_profile->profile_entry_size != 0);

        for (size_t i = 0; i < generic_profile->profile_entry_size; ++i)
        {
            if (active_function_id == generic_profile->profile_entry[i].function_id)
            {
                if (active_variant_id == generic_profile->profile_entry[i].variant_id)
                {
                    device_model = generic_profile->profile_entry[i].ddm_model;

                    LOG(D, "found device_model[%d]: function_id[0x%X], variant_id[0x%X]",
                        device_model,
                        active_function_id,
                        active_variant_id);

                    is_model_set = true;
                    break;
                }
            }
        }
        // Function id and variant id received do not match with any of the listed function-variant id pairs
        TRUE_CHECK_RETURN0(is_model_set == true);

        if (generic_profile->discriminator_ddm != DISCRIMINATOR_DDM_PARAMETER_MODEL_NONE)
        {
            ddm_entry_t *discriminator_ddm = ddm_store__access(slave_device->ddm_owned_store, slave_device->generic_profile->discriminator_ddm);
            // Discriminator parameter not allocated
            TRUE_CHECK_RETURN0(discriminator_ddm != NULL);

            /* Write DDM model in the discriminator parameter entry */
            ddm_entry__set__value_i32(discriminator_ddm, device_model);
            LOG(D, "Discriminator model[0x%X] found", ddm_entry__parameter_id(discriminator_ddm));
        }
        else
        {
            /* It is possible for certain DDM classes to not have discriminator parameter.
             * Use runtime mutable device storage to store the model instead. */
            slave_device->data->active_device_model = device_model;
            LOG(D, "Discriminator model not found!");
        }
    }
    else
    {
        LOG(W, "[%s]: Generic profile not set.", slave_device->name);
    }

    return 1;
}

static int lin_server_init_device(const lin_server_slave_device_t *slave_device)
{
    int status = 0;

    /* Create sync object per device */
    slave_device->data->lock = xSemaphoreCreateMutex();
    if (slave_device->data->active_config_data == NULL)
    {
        LOG(E, "[%s]: No active device configuration!", slave_device->name);
        return status;
    }

    status = lin_server_discriminate_model(slave_device);
    if (status == 0)
    {
        LOG(E, "[%s]: Model discrimination unsuccessful!", slave_device->name);
        return status;
    }

    // Init ddm2 to lin ctrl logic
    status = ddm2_to_lin_ctrl_logic_init(slave_device);
    if (status == 0)
    {
        LOG(E, "[%s]: Storage not allocated!", slave_device->name);
        return status;
    }

    // Frames initialization
    status = initialize_frames(slave_device);
    if (status == 0)
    {
        LOG(E, "[%s] Invalid frames initialization!", slave_device->name);
        return status;
    }

    if (slave_device->init_function)
    {
        // Custom init of private members per device
        status = slave_device->init_function(slave_device);
        if (status == 0)
        {
            LOG(E, "Custom init function for '%s' device failed!", slave_device->name);
            return status;
        }
    }

    status = lin_server_production_reqister_production_class_instance(slave_device);
    if (status == 0)
    {
        LOG(E, "[%s]: Production class not registered!", slave_device->name);
    }

    return status;
}

static const lin_server_device_frames_bundle_def_t *lin_server_get_frame_bundle_definition_by_ctrl_frame(const lin_server_slave_device_t *device, uint8_t frame_id)
{
    const lin_server_device_frames_bundle_def_t *frames_bundle_def = NULL;
    for (size_t i = 0; i < device->frames_bundle_defs_size; ++i)
    {
        if (device->frames_bundle_defs[i].ctrl_frame_def == NULL)
        {
            continue;
        }
        if (device->frames_bundle_defs[i].ctrl_frame_def->frame_id == frame_id)
        {
            frames_bundle_def = &device->frames_bundle_defs[i];
            break;
        }
    }
    return frames_bundle_def;
}

static const lin_server_device_frames_bundle_def_t *lin_server_get_frame_bundle_definition_by_info_frame(const lin_server_slave_device_t *device, uint8_t frame_id)
{
    const lin_server_device_frames_bundle_def_t *frames_bundle_def = NULL;
    for (size_t i = 0; i < device->frames_bundle_defs_size; ++i)
    {
        if (device->frames_bundle_defs[i].info_frame_def == NULL)
        {
            continue;
        }
        if (device->frames_bundle_defs[i].info_frame_def->frame_id == frame_id)
        {
            frames_bundle_def = &device->frames_bundle_defs[i];
            break;
        }
    }
    return frames_bundle_def;
}

const lin_server_device_frames_bundle_def_t *lin_server_get_frame_bundle_definition(const lin_server_slave_device_t *device, lin_frame_type_t frame_type, uint8_t frame_id)
{
    const lin_server_device_frames_bundle_def_t *frames_bundle_def = NULL;
    switch (frame_type)
    {
    case LIN_CONTROL_FRAME:
        frames_bundle_def = lin_server_get_frame_bundle_definition_by_ctrl_frame(device, frame_id);
        break;
    case LIN_INFO_FRAME:
        frames_bundle_def = lin_server_get_frame_bundle_definition_by_info_frame(device, frame_id);
        break;
    default:
        LOG(E, "Frame type not supported: %d", frame_type);
        break;
    }
    return frames_bundle_def;
}

const lin_server_device_frame_def_t *lin_server_get_frame_definition_by_ddm2_parameter(const lin_server_slave_device_t *device, lin_frame_type_t frame_type, uint32_t ddm2_parameter)
{
    const lin_server_device_frame_def_t *frame_def = NULL;

    switch (frame_type)
    {
    case LIN_CONTROL_FRAME:
    {
        const lin_server_map_ddm_to_lin_t *link_entry = ddm_to_lin_device_get_link_entry(device, ddm2_parameter);
        if (link_entry)
        {
            frame_def = link_entry->ctrl_frame_def;
        }
        break;
    }
    case LIN_INFO_FRAME:
    {
        const lin_server_map_lin_to_ddm_t *link_entry = lin_to_ddm_device_get_link_entry(device, ddm2_parameter);
        if (link_entry)
        {
            frame_def = link_entry->info_frame_def;
        }
        break;
    }
    default:
        LOG(E, "Frame type not supported: %d", frame_type);
        break;
    }

    return frame_def;
}

bool lin_server_sync_protocol_local_change_is_local_change_bit_set(const lin_server_device_frame_def_t *frame_def, const lin_server_device_frame_t *buffer)
{
    uint8_t local_change_byte_pos = frame_def->protocol_definition.local_change->local_change.local_change_byte_pos;
    uint8_t local_change_bit_pos = frame_def->protocol_definition.local_change->local_change.local_change_bit_pos;

    // extract local change bit value and covert it boolean
    return ((buffer->frame_signals.raw_frame[local_change_byte_pos] >> local_change_bit_pos) & 0x01) == 0 ? false : true;
}

bool lin_server_sync_protocol_local_change_is_sync_frame_bit_set(const lin_server_device_frame_def_t *frame_def, const lin_server_device_frame_t *buffer)
{
    uint8_t sync_frame_byte_pos = frame_def->protocol_definition.local_change->sync_frame.sync_frame_byte_pos;
    uint8_t sync_frame_bit_pos = frame_def->protocol_definition.local_change->sync_frame.sync_frame_bit_pos;

    return ((buffer->frame_signals.raw_frame[sync_frame_byte_pos] >> sync_frame_bit_pos) & 0x01) == 0 ? false : true;
}

void lin_server_sync_protocol_local_change_set_sync_frame_bit(const lin_server_device_frame_def_t *frame_def, lin_server_device_frame_t *buffer)
{
    uint8_t sync_frame_byte_pos = frame_def->protocol_definition.local_change->sync_frame.sync_frame_byte_pos;
    uint8_t sync_frame_bit_pos = frame_def->protocol_definition.local_change->sync_frame.sync_frame_bit_pos;

    buffer->frame_signals.raw_frame[sync_frame_byte_pos] |= (1 << sync_frame_bit_pos);
}

void lin_server_sync_protocol_local_change_clear_sync_frame_bit(const lin_server_device_frame_def_t *frame_def, lin_server_device_frame_t *buffer)
{
    uint8_t sync_frame_byte_pos = frame_def->protocol_definition.local_change->sync_frame.sync_frame_byte_pos;
    uint8_t sync_frame_bit_pos = frame_def->protocol_definition.local_change->sync_frame.sync_frame_bit_pos;

    buffer->frame_signals.raw_frame[sync_frame_byte_pos] &= ~(1 << sync_frame_bit_pos);
}

lin_server_sync_protocol_type_t lin_server_get_sync_protocol(const lin_server_slave_device_t *device)
{
    return device->protocol;
}

int lin_server_get_ctrl_frame_verification_type(lin_server_device_frame_t *ctrl_frame)
{
    return ctrl_frame->frame_data.ctrl_frame_type;
}

bool lin_server_has_ctrl_frame_verification_attempts_left(lin_server_device_frame_t *ctrl_frame)
{
    return (--ctrl_frame->frame_data.ctrl_frame_verification_counter > 0 ? true : false);
}

bool lin_server_has_ctrl_frame_verification_local_change_ack_attempts_left(lin_server_device_frame_t *ctrl_frame)
{
    return (--ctrl_frame->frame_data.ctrl_frame_verification_local_change_ack_counter > 0 ? true : false);
}

bool lin_server_has_ctrl_frame_been_sent(lin_server_device_frame_t *ctrl_frame)
{
    return ctrl_frame->frame_data.has_ctrl_frame_been_sent;
}

bool lin_server_has_any_ctrl_frame_been_sent(const lin_server_slave_device_t *device)
{
    bool has_any_ctrl_been_sent = false;
    for (size_t i = 0; i < device->frames_bundle_defs_size; ++i)
    {
        if (device->frames_bundle_defs[i].ctrl_frame_def)
        {
            if (lin_server_has_ctrl_frame_been_sent(device->frames_bundle_defs[i].ctrl_frame_def->frame) == true)
            {
                has_any_ctrl_been_sent = true;
                break;
            }
        }
    }
    return has_any_ctrl_been_sent;
}

void lin_server_set_ctrl_frame_verification(lin_server_device_frame_t *ctrl_frame, int ctrl_frame_type)
{
    ctrl_frame->frame_data.has_ctrl_frame_been_sent = true;
    ctrl_frame->frame_data.ctrl_frame_type = ctrl_frame_type;
}

void lin_server_reset_ctrl_frame_verification_counter(lin_server_device_frame_t *ctrl_frame, int counter)
{
    ctrl_frame->frame_data.ctrl_frame_verification_counter = counter;
}

void lin_server_reset_ctrl_frame_verification_local_change_ack_counter(lin_server_device_frame_t *ctrl_frame)
{
    ctrl_frame->frame_data.ctrl_frame_verification_local_change_ack_counter = LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_COUNTER;
}

void lin_server_reset_ctrl_frame_verification(lin_server_device_frame_t *ctrl_frame)
{
    ctrl_frame->frame_data.has_ctrl_frame_been_sent = false;
    ctrl_frame->frame_data.ctrl_frame_type = SCHEDULE_FRAME_TYPE_NONE;
    ctrl_frame->frame_data.ctrl_frame_verification_counter = LIN_SERVER_CTRL_FRAME_VERIFICATION_COUNTER;
    ctrl_frame->frame_data.ctrl_frame_verification_local_change_ack_counter = LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_COUNTER;
}

static void lin_server_handle_lin_to_ddm_signal_conversion(const lin_server_slave_device_t *device, const lin_server_device_frame_t *info_frame, const lin_server_map_lin_to_ddm_t *link_entry)
{
    ddm_entry_t *ddm_entries[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
    size_t ddm_data_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
    /* Increase flexibility of the callback functions to store the maximum data values in bytes,
     * without worring whether the memory size allocated from ddm_store/ddm_entry will be efficient
     * at the time the conversion function is called. This is important for the ddm2 parameters with
     * variable data lenght(storage, other, strings etc..) */
    uint8_t ddm_data_buffer[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY];

    ZERO(ddm_entries);

    for (size_t i = 0u; i < LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY; i++)
    {
        const void *ddm_data_temp;
        if (link_entry->ddm[i] == 0)
        {
            /*
             * We reached at end of listed DDM parameters.
             */
            break;
        }
        /* Get DDM value */
        ddm_entry_t *ddm_entry = ddm_store__access(device->ddm_owned_store, link_entry->ddm[i]);

        if (ddm_entry == NULL)
        {
            LOG(E, "non-owned DDM 0x%x was listed in info link map at index %u", link_entry->ddm[i], i);
            break;
        }
        ddm_entries[i] = ddm_entry;
        ddm_entry__read__value(ddm_entry, &ddm_data_temp, &ddm_data_size[i]);
        /* Copy value data so we can send a copy to conversion function to not modify
         * the info frame bits directly in convert_lin_to_ddm callback by using the
         * original memory buffer allocated by the ddm_store/ddm_entry library. */
        memcpy(&ddm_data_buffer[i][0], ddm_data_temp, ddm_data_size[i]);
    }

    bool is_converted = link_entry->convert_lin_to_ddm(
        &info_frame->frame_signals,
        ddm_data_buffer,
        ddm_data_size);

    for (size_t i = 0u; i < LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY; i++)
    {
        // end of listed DDM parameters
        if (ddm_entries[i] == NULL)
        {
            /*
             * We reached at end of listed DDM parameters.
             */
            break;
        }

        if (is_converted == true)
        {
            // Store DDM values and publish them
            bool has_changed = ddm_entry__set__value(ddm_entries[i], (const void *)ddm_data_buffer[i], ddm_data_size[i]);
            ddm_entry__set__has_changed(ddm_entries[i], has_changed);
        }
        else
        {
            /* Converter function didn't write anything to these DDM values, skip setting them */
            LOG(W, "Invalid conversion to DDM %s0%s(%x)", ddm_entry__device_class(ddm_entries[i]), ddm_entry__property(ddm_entries[i]), ddm_entry__parameter_id(ddm_entries[i]));
        }
    }
}

static void handle_lin_to_ddm_device_signals_conversion(const lin_server_slave_device_t *device, const lin_server_device_frame_def_t *info_frame_def)
{
    for (size_t i = 0; i < device->map_lin_to_ddm_size; i++)
    {
        const lin_server_map_lin_to_ddm_t *link_entry = &device->map_lin_to_ddm[i];
        if (link_entry->info_frame_def->frame_id == info_frame_def->frame_id)
        {
            lin_server_handle_lin_to_ddm_signal_conversion(device, info_frame_def->frame, link_entry);
        }
    }
}

static void handle_lin_to_ddm_device_function_specific_signals_conversion(const lin_server_slave_device_t *device, const lin_server_device_frame_def_t *info_frame_def)
{
    if (device->data->active_config_data != NULL)
    {
        const lin_server_function_specific_config_data_t *function_specific_config_data = device->data->active_config_data;
        if (function_specific_config_data->map_function_specific_lin_to_ddm != NULL)
        {
            for (size_t i = 0; i < function_specific_config_data->map_function_specific_lin_to_ddm_size; i++)
            {
                const lin_server_map_lin_to_ddm_t *link_entry = &function_specific_config_data->map_function_specific_lin_to_ddm[i];
                if (link_entry->info_frame_def->frame_id == info_frame_def->frame_id)
                {
                    lin_server_handle_lin_to_ddm_signal_conversion(device, info_frame_def->frame, link_entry);
                }
            }
        }
        else
        {
            LOG(D, "Device's function-specific[0x%04X] variant does not support additional conversion functions.", device->data->active_config_data->function_id);
        }
    }
}

/* Executed in UART task */
void lin_server_handle_lin_info_frame_received(const lin_server_slave_device_t *device, const lin_server_device_frames_bundle_def_t *bundle_frame_def)
{
    LOG(D, "DDM parameters update from LIN Slave");

    const lin_server_device_frame_def_t *info_frame_def = bundle_frame_def->info_frame_def;
    const lin_server_device_frame_def_t *ctrl_frame_def = bundle_frame_def->ctrl_frame_def;

    lin_server_lock_device(device);
    {
        /* LIN to DDM2 device's conversion */
        handle_lin_to_ddm_device_signals_conversion(device, info_frame_def);
        /* LIN to DDM2 device's function-specific variant conversion */
        handle_lin_to_ddm_device_function_specific_signals_conversion(device, info_frame_def);
        /* LIN to DDM2 device's logical functions */
        lin_to_ddm2_info_logic_post_received_functions(device, info_frame_def, ctrl_frame_def);
        /* Apply changes in the ctrl frame */
        invoke_ddm2_to_lin_conversion_functions(device, device->ddm_owned_store, ctrl_frame_def);
        /* Now publish updated parameters and reset has_updated flag */
        handle_owned_parameter_storage_publish(device, device->ddm_owned_store);

        device->data->has_been_updated = true;
    }
    lin_server_unlock_device(device);
}

static const lin_server_map_lin_to_ddm_t *lin_to_ddm_get_function_specific_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_updated_prameter)
{
    const lin_server_map_lin_to_ddm_t *link_entry = NULL;

    // Check if there is DDM2 device's function-specific linked ddm2 to lin conversion function
    const lin_server_function_specific_config_data_t *function_specific_config_data = device->data->active_config_data;
    if (function_specific_config_data->map_function_specific_lin_to_ddm != NULL)
    {
        for (size_t i = 0; i < function_specific_config_data->map_function_specific_lin_to_ddm_size; i++)
        {
            if (ddm2_updated_prameter == function_specific_config_data->map_function_specific_lin_to_ddm[i].ddm[0])
            {
                link_entry = &function_specific_config_data->map_function_specific_lin_to_ddm[i];
                break;
            }
        }
    }

    return link_entry;
}

static const lin_server_map_lin_to_ddm_t *lin_to_ddm_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter)
{
    const lin_server_map_lin_to_ddm_t *link_entry = NULL;

    for (size_t i = 0; i < device->map_lin_to_ddm_size; i++)
    {
        if (ddm2_prameter == device->map_lin_to_ddm[i].ddm[0])
        {
            link_entry = &device->map_lin_to_ddm[i];
            break;
        }
    }

    return link_entry;
}

static const lin_server_map_lin_to_ddm_t *lin_to_ddm_device_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter)
{
    const lin_server_map_lin_to_ddm_t *link_entry;

    link_entry = lin_to_ddm_get_link_entry(device, ddm2_prameter);
    if (link_entry == NULL)
    {
        link_entry = lin_to_ddm_get_function_specific_link_entry(device, ddm2_prameter);
    }

    // Execute specific LIN to DDM conversion function
    if ((link_entry != NULL) && (link_entry->convert_lin_to_ddm == NULL))
    {
        LOG(E, "No conversion function linked for the parameter 0x%08x!", ddm2_prameter);
    }

    return link_entry;
}

static const lin_server_map_ddm_to_lin_t *ddm_to_lin_get_function_specific_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_updated_prameter)
{
    const lin_server_map_ddm_to_lin_t *link_entry = NULL;

    // Check if there is DDM2 device's function-specific linked ddm2 to lin conversion function
    const lin_server_function_specific_config_data_t *function_specific_config_data = device->data->active_config_data;
    if (function_specific_config_data->map_function_specific_ddm_to_lin != NULL)
    {
        for (size_t i = 0; i < function_specific_config_data->map_function_specific_ddm_to_lin_size; i++)
        {
            if (ddm2_updated_prameter == function_specific_config_data->map_function_specific_ddm_to_lin[i].ddm[0])
            {
                link_entry = &function_specific_config_data->map_function_specific_ddm_to_lin[i];
                break;
            }
        }
    }

    return link_entry;
}

static const lin_server_map_ddm_to_lin_t *ddm_to_lin_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter)
{
    const lin_server_map_ddm_to_lin_t *link_entry = NULL;

    for (size_t i = 0; i < device->map_ddm_to_lin_size; i++)
    {
        if (ddm2_prameter == device->map_ddm_to_lin[i].ddm[0])
        {
            link_entry = &device->map_ddm_to_lin[i];
            break;
        }
    }

    return link_entry;
}

static const lin_server_map_ddm_to_lin_t *ddm_to_lin_device_get_link_entry(const lin_server_slave_device_t *device, uint32_t ddm2_prameter)
{
    const lin_server_map_ddm_to_lin_t *link_entry;

    link_entry = ddm_to_lin_get_link_entry(device, ddm2_prameter);
    if (link_entry == NULL)
    {
        link_entry = ddm_to_lin_get_function_specific_link_entry(device, ddm2_prameter);
    }

    // Execute specific DDM to LIN converstion function
    if ((link_entry != NULL) && (link_entry->convert_ddm_to_lin == NULL))
    {
        LOG(E, "No conversion function linked for the parameter 0x%08x!", ddm2_prameter);
    }

    return link_entry;
}

static void get_parameters_entries_value(const ddm_entry_t **ddm_entry, size_t ddm_entries_to_read, const void **ddm2_data, size_t *ddm2_data_size)
{
    for (size_t i = 0; i < ddm_entries_to_read; i++)
    {
        if (ddm_entry[i] == NULL)
        {
            break;
        }
        /* Get DDM value */
        ddm_entry__read__value(ddm_entry[i], &ddm2_data[i], &ddm2_data_size[i]);
    }
}

static int get_parameters_entries_in_link_entry(ddm_store_t *ddm_store, const lin_server_map_ddm_to_lin_t *link_entry, const ddm_entry_t **ddm2_entries, size_t *ddm2_entries_size)
{
    int number_of_ddm2_entries_read = 0;

    for (size_t i = 0; i < LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY; i++)
    {
        if (link_entry->ddm[i] == 0)
        {
            /*
             * We reached at end of listed DDM parameters.
             */
            *ddm2_entries_size = i;
            break;
        }
        /* Get DDM entry */
        ddm2_entries[i] = ddm_store__access(ddm_store, link_entry->ddm[i]);
        if (ddm2_entries[i] == NULL)
        {
            *ddm2_entries_size = 0;
            LOG(E, "non-handled DDM 0x%x was listed in ctrl link map at index %u", link_entry->ddm[i], i);
            break;
        }
    }

    number_of_ddm2_entries_read = *ddm2_entries_size;
    return number_of_ddm2_entries_read;
}

static int invoke_ddm2_to_lin_conversion_functions(const lin_server_slave_device_t *device, ddm_store_t *ddm_store, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    int is_conversion_successful = 0;
    const ddm_entry_t *ddm2_entries[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
    size_t ddm2_entries_size = ELEMENTS(ddm2_entries);
    const uint8_t *ddm2_data[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
    size_t ddm2_data_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];

    if (ctrl_frame_def != NULL)
    {
        for (size_t index = 0; index < ddm_store__occupied(ddm_store); ++index)
        {
            ddm_entry_t *entry;
            ddm_store__iterate(ddm_store, index, &entry);

            if (ddm_entry__has_changed(entry))
            {
                // Find corresponding ddm2 to lin conversion function(if any)
                const lin_server_map_ddm_to_lin_t *ddm_to_lin_link_entry = ddm_to_lin_device_get_link_entry(device, ddm_entry__parameter_id(entry));
                if (ddm_to_lin_link_entry && ddm_to_lin_link_entry->convert_ddm_to_lin)
                {
                    // Find ddm2 parameters linked to the conversion function
                    int parameters_read = get_parameters_entries_in_link_entry(ddm_store, ddm_to_lin_link_entry, ddm2_entries, &ddm2_entries_size);
                    if (parameters_read != 0)
                    {
                        // Read ddm2 parameters value and do the conversion
                        get_parameters_entries_value(ddm2_entries, ddm2_entries_size, (const void **)ddm2_data, ddm2_data_size);
                        is_conversion_successful = ddm_to_lin_link_entry->convert_ddm_to_lin(
                            ddm2_data,
                            ddm2_data_size,
                            &ctrl_frame_def->frame->frame_signals);
                    }

                    if ((is_conversion_successful == 0) || (parameters_read == 0))
                    {
                        LOG(E, "Invalid conversion for parameter '%s0%s:[0x%X]'", ddm_entry__device_class(entry), ddm_entry__property(entry), ddm_entry__parameter_id(entry));
                        break;
                    }
                }
            }
        }
    }

    return is_conversion_successful;
}

static int ddm2_to_lin_ctrl_logic_init(const lin_server_slave_device_t *device)
{
    device->data->heap_store = ddm_store__create(device->ddm_owned_initial_values_size);
    TRUE_CHECK_RETURN0(device->data->heap_store);

    int status = ddm_store__copy_entries(device->data->heap_store, device->ddm_owned_store);
    TRUE_CHECK_RETURN0(status == 0);

    ddm2_to_lin_ctrl_logic_clear_table_index(device);

    return 1;
}

static void ddm2_to_lin_ctrl_logic_remove(const lin_server_slave_device_t *device)
{
    if (device->data->heap_store != NULL)
    {
        ddm_store__destroy(device->data->heap_store);
    }

    ddm2_to_lin_ctrl_logic_clear_table_index(device);
}

static void ddm2_to_lin_ctrl_logic_set_table_index(const lin_server_slave_device_t *device, int index)
{
    device->data->table_index = index;
}

static int ddm2_to_lin_ctrl_logic_get_table_index(const lin_server_slave_device_t *device)
{
    return device->data->table_index;
}

static void ddm2_to_lin_ctrl_logic_clear_table_index(const lin_server_slave_device_t *device)
{
    device->data->table_index = -1;
}

static bool ddm2_to_lin_ctrl_logic_is_registered_for_device(const lin_server_slave_device_t *device)
{
    return (device->ddm2_to_lin_ctrl_logic == NULL ? false : true);
}

static bool ddm2_to_lin_ctrl_logic_is_pre_send_registered(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table)
{
    return (ctrl_logic_table->ddm2_to_lin_handle_logic_pre_send == NULL ? false : true);
}

static bool ddm2_to_lin_ctrl_logic_is_post_send_registered(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table)
{
    return (ctrl_logic_table && (ctrl_logic_table->ddm2_to_lin_handle_logic_post_send != NULL) ? true : false);
}

static bool lin_to_ddm2_info_logic_is_registered_for_device(const lin_server_slave_device_t *device)
{
    return (device->lin_to_ddm2_info_logic == NULL ? false : true);
}

static bool lin_to_ddm2_info_logic_is_post_received_registered(const lin_server_lin_to_ddm2_info_logic_t *info_logic_table)
{
    return (info_logic_table && (info_logic_table->lin_to_ddm2_handle_logic_post_receive != NULL) ? true : false);
}

static const lin_server_ddm2_to_lin_ctrl_logic_t *ddm2_to_lin_ctrl_logic_get_handle_by_index(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table, size_t ctrl_logic_table_size, int index)
{
    TRUE_CHECK_RETURN0((int)ctrl_logic_table_size > index);
    return &ctrl_logic_table[index];
}

static void ddm2_to_lin_ctrl_logic_prepare_for_conversion(const lin_server_slave_device_t *device, const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table)
{
    for (size_t i = 0; i < ctrl_logic_table->ddm2_parameter_list_count; i++)
    {
        ddm_entry_t *entry = ddm_store__access(device->data->heap_store, ctrl_logic_table->ddm2_parameter_list[i]);
        ddm_entry__set__has_changed(entry, true);
    }
}

static void lin_to_ddm2_info_logic_post_received_functions(const lin_server_slave_device_t *device, const lin_server_device_frame_def_t *info_frame_def, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    if (lin_to_ddm2_info_logic_is_registered_for_device(device))
    {
        for (size_t i = 0; i < device->lin_to_ddm2_info_logic_size; i++)
        {
            const lin_server_lin_to_ddm2_info_logic_t *link_entry = &device->lin_to_ddm2_info_logic[i];
            if (lin_to_ddm2_info_logic_is_post_received_registered(link_entry))
            {
                if (link_entry->info_frame_def == info_frame_def)
                {
                    lin_server_logic_func_response_t status = link_entry->lin_to_ddm2_handle_logic_post_receive(
                        device,
                        info_frame_def,
                        device->ddm_owned_store,
                        ctrl_frame_def);

                    switch (status)
                    {
                    case LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE:
                        TRUE_CHECK_RETURN(ctrl_frame_def);  // You cannot schedule, if there is no frame to schedule.
                        // Schedule CTRL frame back, so we keep slave and master in sync
                        lin_server_scheduler_schedule_frame_by_device_type_and_id(
                            LIN_CONTROL_FRAME,
                            ctrl_frame_def->frame_id,
                            device->device_type,
                            SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);
                        break;
                    case LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH:
                        // It will store and publish by default
                        break;
                    case LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION:
                        LOG(E, "Invalid post received conversion!");
                        break;
                    case LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE:
                        break;
                    default:
                        LOG(E, "Invalid status %d.", status);
                        break;
                    }
                }
            }
        }
    }
}

static int ddm2_to_lin_ctrl_logic_get_handle_index_by_ddm_parameter(const lin_server_ddm2_to_lin_ctrl_logic_t *ctrl_logic_table, size_t ctrl_logic_table_size, uint32_t parameter)
{
    int ctrl_logic_handle_index = -1;

    for (size_t i = 0; i < ctrl_logic_table_size; i++)
    {
        for (size_t j = 0; j < ctrl_logic_table[i].ddm2_parameter_list_count; j++)
        {
            if (parameter == ctrl_logic_table[i].ddm2_parameter_list[j])
            {
                ctrl_logic_handle_index = i;
                break;
            }
        }
    }

    return ctrl_logic_handle_index;
}

static const lin_server_ddm2_to_lin_ctrl_logic_t *ddm2_to_lin_ctrl_logic_find_handle_by_ddm_parameter(const lin_server_slave_device_t *device, uint32_t parameter, int *table_index)
{
    const lin_server_ddm2_to_lin_ctrl_logic_t *ddm_ctrl_logic = NULL;

    // Do we have logic handles registered for this device?
    if (ddm2_to_lin_ctrl_logic_is_registered_for_device(device) == true)
    {
        // Do we have logic handle for this parameter? If so, return the table index.
        int ctrl_logic_handle_index = ddm2_to_lin_ctrl_logic_get_handle_index_by_ddm_parameter(
            device->ddm2_to_lin_ctrl_logic,
            device->ddm2_to_lin_ctrl_logic_size,
            parameter);

        if (ctrl_logic_handle_index != -1)
        {
            // Find the parameter handle
            ddm_ctrl_logic = ddm2_to_lin_ctrl_logic_get_handle_by_index(
                device->ddm2_to_lin_ctrl_logic,
                device->ddm2_to_lin_ctrl_logic_size,
                ctrl_logic_handle_index);

            LOG(D, "%s: Logic handler for parameter 0x%X found at index: %d",
                device->name,
                parameter,
                ctrl_logic_handle_index);
            // Store table index of lin_server_ddm2_to_lin_ctrl_logic_t for the trigger parameter so we can execute pre_send/post_send functions
            *table_index = ctrl_logic_handle_index;
        }
    }

    return ddm_ctrl_logic;
}

bool lin_server_state_evaluate(void)
{
    bool is_signal_frame_pending = true;

    size_t numb_of_slave_devices = lin_server_get_number_of_slave_devices();
    for (size_t device_index = 0; device_index < numb_of_slave_devices; device_index++)
    {
        const lin_server_slave_device_t *device = lin_server_get_slave_device_by_index(device_index);

        if (device->data->is_on_bus_detected == true)
        {
            if (lin_server_has_any_ctrl_frame_been_sent(device) == true)
            {
                LOG(D, "CTRL frame verification detected.");
                is_signal_frame_pending = true;
                break;
            }
            else if (lin_server_scheduler_is_ctrl_frame_scheduled() == true)
            {
                LOG(D, "CTRL frame pending detected.");
                is_signal_frame_pending = true;
                break;
            }
            else if (device->data->has_been_updated == true)
            {
                LOG(D, "INFO frame update detected.");
                is_signal_frame_pending = true;
                device->data->has_been_updated = false;
                break;
            }
            else
            {
                is_signal_frame_pending = false;
            }
        }
        else
        {
            is_signal_frame_pending = false;
        }
    }
    return is_signal_frame_pending;
}

void lin_server_handle_generic(uint32_t event_id, const void *const value, size_t value_size)
{
    switch (event_id)
    {
    case LIN_SERVER_GENERIC_SWITCH_STATE_EVENT:
    {
        TRUE_CHECK_RETURN(value);
        TRUE_CHECK_RETURN(value_size == sizeof(lin_server_state_t));

        lin_server_state_t state = *(lin_server_state_t *)value;
        lin_server_set_state(state);
        lin_server_switch_timer_mode();

        break;
    }
    case LIN_SERVER_GENERIC_TIMER_EVENT:
    {
        lin_server_state_t state = lin_server_get_state();
        switch (state)
        {
        case LIN_SERVER_STATE_OPERATIONAL:
            LOG(D, "Timer go-to-sleep request triggered.");
            lin_server_trigger_go_to_sleep_event(LIN_SERVER_STATE_SLEEP_REQUEST_TIMER);
            break;
        case LIN_SERVER_STATE_SLEEPING:
            LOG(D, "Timer wake-up request triggered.");
            lin_server_trigger_wakeup_event(LIN_SERVER_STATE_WAKEUP_REQUEST_TIMER);
            break;
        case LIN_SERVER_STATE_WAKEUP_PENDING:
        case LIN_SERVER_STATE_INVALID:
        // fall through
        default:
            LOG(E, "Invalid state: %d", state);
            break;
        }
        break;
    }
    default:
        LOG(E, "Unsupported generic event id: %d", event_id);
        break;
    }
}

/* Executed in broker task */
void lin_server_handle_set(const lin_server_slave_device_t *const device, uint32_t parameter, const void *const value, size_t value_size)
{
    bool should_update_store = false;
    bool should_schedule_store = false;
    const lin_server_device_frame_def_t *ctrl_frame_def;
    lin_server_device_frame_def_t ctrl_frame_def_current_state;

    LOG(D, "DDM parameter update from Broker");

    ddm_entry_t *ddm_entry_trigger = ddm_store__access(device->ddm_owned_store, parameter);
    if (ddm_entry_trigger == NULL)
    {
        /* Somebody tried to SET parameter that is not listed in the device's ddm_store_ddm */
        LOG(W, "%s: set of non-handled DDM: 0x%X",
            device->name,
            parameter | DDM2_PARAMETER_INSTANCE(device->data->class_instance));
        return;
    }

    ctrl_frame_def = lin_server_get_frame_definition_by_ddm2_parameter(device, LIN_CONTROL_FRAME, parameter);
    if (ctrl_frame_def == NULL)
    {
        /* Somebody tried to SET parameter that is not defined in the device's map_ddm_to_lin_t.
         * This can happen if the device does not have any CTRL frame in it's definition.
         */
        LOG(W, "%s: No CTRL frame was assosiated with the DDM2 parameter[0x%X] being SET",
            device->name,
            parameter | DDM2_PARAMETER_INSTANCE(device->data->class_instance));
        return;
    }

    // Take snapshot of the device's current state
    lin_server_lock_device(device);

    memcpy(&ctrl_frame_def_current_state, ctrl_frame_def, sizeof(lin_server_device_frame_def_t));
    ddm_store__copy_entries(device->data->heap_store, device->ddm_owned_store);  // Copy storage to temporary one

    // Take current state from temporary storage and update with the new value
    ddm_entry_t *ddm_entry_trigger_new_state = ddm_store__access(device->data->heap_store, parameter);
    handle_owned_parameter_write(ddm_entry_trigger_new_state, value, value_size);

    // Check if there is existing logic handle for the trigger parameter
    int table_index = -1;
    const lin_server_ddm2_to_lin_ctrl_logic_t *ddm_ctrl_logic_handle = ddm2_to_lin_ctrl_logic_find_handle_by_ddm_parameter(device, parameter, &table_index);
    if (ddm_ctrl_logic_handle && ddm2_to_lin_ctrl_logic_is_pre_send_registered(ddm_ctrl_logic_handle))
    {
        LOG(D, "Handle CTRL pre_send logic for parameter \"%s0%s\" (0x%X) found.",
            ddm_entry__device_class(ddm_entry_trigger_new_state), ddm_entry__property(ddm_entry_trigger_new_state),
            ddm_entry__parameter_id(ddm_entry_trigger_new_state));

        // Call logic function for the trigger ddm parameter
        lin_server_logic_func_response_t status = ddm_ctrl_logic_handle->ddm2_to_lin_handle_logic_pre_send(
            device,
            ddm_entry_trigger_new_state,
            device->data->heap_store,
            &ctrl_frame_def_current_state);

        LOG(D, "CTRL pre_send logic status: %d", status);
        switch (status)
        {
        case LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH:
            should_update_store = true;
            should_schedule_store = false;
            break;
        case LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE:
            ddm2_to_lin_ctrl_logic_prepare_for_conversion(device, ddm_ctrl_logic_handle);
            // Update it only if we are expecting post send event i.e. sheduled ctrl frame
            ddm2_to_lin_ctrl_logic_set_table_index(device, table_index);

            should_update_store = true;
            should_schedule_store = true;
            break;
        case LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION:
            should_update_store = false;
            should_schedule_store = false;
            break;
        case LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE:
            break;
        default:
            break;
        }
    }
    else
    {
        should_schedule_store = true;
    }

    if (should_schedule_store)
    {
        int status = invoke_ddm2_to_lin_conversion_functions(device, device->data->heap_store, &ctrl_frame_def_current_state);
        if (status == 0)
        {
            // Conversion function(s) failed. Do not update store or publish the new value(s).
            should_update_store = false;
        }
        else
        {
            // Conversion function(s) successed. Update store and publish the new value(s).
            should_update_store = true;
        }
    }

    if (should_update_store)
    {
        if (should_schedule_store)
        {
            if (lin_server_is_in_current_state(LIN_SERVER_STATE_SLEEPING) == true)
            {
                lin_server_trigger_wakeup_event(LIN_SERVER_STATE_WAKEUP_REQUEST_BROKER);
            }
            // Prepare ctrl frame
            memcpy(ctrl_frame_def->frame->frame_signals.raw_frame, ctrl_frame_def_current_state.frame->frame_signals.raw_frame, ctrl_frame_def->frame_len);
            lin_server_scheduler_schedule_frame_by_device_type_and_id(
                LIN_CONTROL_FRAME,
                ctrl_frame_def->frame_id,
                device->device_type,
                SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);
        }

        // Copy back values and flags from temporary storage
        ddm_store__copy_entries(device->ddm_owned_store, device->data->heap_store);
        // Publish back new state of the DDM parameter(s) that has been updated
        for (size_t index = 0; index < ddm_store__occupied(device->ddm_owned_store); ++index)
        {
            ddm_entry_t *ddm_entry;
            ddm_store__iterate(device->ddm_owned_store, index, &ddm_entry);

            if (ddm_entry__has_changed(ddm_entry))
            {
                handle_owned_parameter_publish(device, ddm_entry);
            }
        }
    }
    else
    {
        LOG(D, "Revert requested change for parameter 0x%X", ddm_entry__parameter_id(ddm_entry_trigger));

        // Publish back old state of the DDM trigger parameter as we have not accepted the new change
        handle_owned_parameter_publish(device, ddm_entry_trigger);
        // Reset data table index as the logical function or ddm to lin conversion failed
        ddm2_to_lin_ctrl_logic_clear_table_index(device);
    }

    lin_server_ddm_storage_verbose_logging(device);
    lin_server_unlock_device(device);
}

void lin_server_ddm2_to_lin_handle_logic_post_send(const lin_server_slave_device_t *device, const lin_server_device_frame_def_t *ctrl_frame_def)
{
    if (ddm2_to_lin_ctrl_logic_is_registered_for_device(device))
    {
        // If table index is set, then check if there is post_send cb function registered
        int table_index = ddm2_to_lin_ctrl_logic_get_table_index(device);
        if (table_index != -1)
        {
            const lin_server_ddm2_to_lin_ctrl_logic_t *ddm_ctrl_logic_handle = ddm2_to_lin_ctrl_logic_get_handle_by_index(
                device->ddm2_to_lin_ctrl_logic,
                device->ddm2_to_lin_ctrl_logic_size,
                table_index);

            if (ddm_ctrl_logic_handle && ddm2_to_lin_ctrl_logic_is_post_send_registered(ddm_ctrl_logic_handle))
            {
                LOG(D, "Handle CTRL post_send logic found at index %d", table_index);
                lin_server_logic_func_response_t status = ddm_ctrl_logic_handle->ddm2_to_lin_handle_logic_post_send(device, ctrl_frame_def);
                switch (status)
                {
                case LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE:
                    // Schedule CTRL frame back, additional update is necessary
                    lin_server_scheduler_schedule_frame_by_device_type_and_id(
                        LIN_CONTROL_FRAME,
                        ctrl_frame_def->frame_id,
                        device->device_type,
                        SCHEDULE_FRAME_TYPE_CTRL_BROKER_REQUEST);
                    break;
                case LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION:
                    LOG(E, "Invalid post received conversion!");
                    break;
                case LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH:  // fall through
                case LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE:               // fall through
                default:
                    break;
                }
            }

            ddm2_to_lin_ctrl_logic_clear_table_index(device);
        }
    }
}

/* Executed in broker task */
void lin_server_handle_subscribe(const lin_server_slave_device_t *const device, uint32_t parameter)
{
    ddm_entry_t *ddm_entry;
    const void *ddm_data;
    size_t ddm_data_size;

    ddm_entry = ddm_store__access(device->ddm_owned_store, parameter);
    if (ddm_entry == NULL)
    {
        /* Somebody tried to SUBSCRIBE to something that we do not handle */
        LOG(W, "%s: subscription of non-handled DDM: 0x%X",
            device->name,
            parameter | DDM2_PARAMETER_INSTANCE(device->data->class_instance));
        return;
    }

    if (ddm_entry__is_value_int32(ddm_entry))
    {
        LOG(D, "%s: DDM \"%s0%s\" subscription value: %d",
            device->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
            ddm_entry__value_i32(ddm_entry));
    }
    else if (ddm_entry__is_value_str(ddm_entry))
    {
        LOG(D, "%s: DDM \"%s0%s\" subscription value: %s",
            device->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
            ddm_entry__value_str(ddm_entry));
    }
    else
    {
        LOG(D, "%s: DDM \"%s0%s\" subscription",
            device->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
    }

    ddm_entry__read__value(ddm_entry, &ddm_data, &ddm_data_size);
    TRUE_CHECK(lin_server_publish(
        ddm_entry__parameter_id(ddm_entry),
        device->data->class_instance,
        ddm_data,
        ddm_data_size));
}

int lin_server_register_device(const lin_server_slave_device_t *slave_device)
{
    int status = 0;

    create_slave_entries(slave_device);

    slave_device->data->class_instance = lin_server_register_ddm_class_instance(slave_device->device_class);
    if (slave_device->data->class_instance == -1)
    {
        LOG(E, "DDM class[0x%X] not registered for device[%s]", slave_device->device_class, slave_device->name);
        return status;
    }
    else
    {
        LOG(I, "DDM class[0x%X] instance[%d] has been registered for device[%s]",
            slave_device->device_class,
            slave_device->data->class_instance,
            slave_device->name);
    }

    status = lin_server_init_device(slave_device);
    if (status == 0)
    {
        LOG(E, "Initialization of device '%s failed!", slave_device->name);
        lin_server_remove_device(slave_device);
    }

    return status;
}

static void lin_server_remove_class_instance(const lin_server_slave_device_t *slave_device)
{
    lin_server_delete_ddm_class_instance(slave_device->device_class, slave_device->data->class_instance);

    LOG(I, "DDM class[0x%X] instance[%d] removed for device[%s]",
        slave_device->device_class,
        slave_device->data->class_instance,
        slave_device->name);

    slave_device->data->class_instance = -1;

    delete_slave_entries(slave_device);
}

void lin_server_remove_device(const lin_server_slave_device_t *slave_device)
{
    lin_server_slave_device_mutable_data_t *mutable_data = slave_device->data;
    mutable_data->is_on_bus_detected = false;
    mutable_data->is_initialized = false;
    mutable_data->active_config_data = NULL;
    mutable_data->bus_detection_counter = LIN_SERVER_BUS_DETECTION_COUNTER_INIT_VALUE;
    vSemaphoreDelete(mutable_data->lock);

    lin_server_remove_class_instance(slave_device);
    lin_server_production_remove_production_class_instance(slave_device);

    ddm2_to_lin_ctrl_logic_remove(slave_device);
}

void lin_server_lock_device(const lin_server_slave_device_t *device)
{
    xSemaphoreTake(device->data->lock, portMAX_DELAY);
}

void lin_server_unlock_device(const lin_server_slave_device_t *device)
{
    xSemaphoreGive(device->data->lock);
}

static void lin_server_switch_timer_mode(void)
{
    TimerHandle_t timer_handle = lin_server.lin_server_sleep_timer_handle;
    TRUE_CHECK_RETURN(timer_handle);

    lin_server_state_t state = lin_server_get_state();
    switch (state)
    {
    case LIN_SERVER_STATE_OPERATIONAL:
        TRUE_CHECK(xTimerChangePeriod(timer_handle, pdMS_TO_TICKS(LIN_SERVER_GO_TO_SLEEP_TIMEOUT_MS), portMAX_DELAY));
        TRUE_CHECK(xTimerReset(timer_handle, portMAX_DELAY));
        break;
    case LIN_SERVER_STATE_SLEEPING:
        TRUE_CHECK(xTimerChangePeriod(timer_handle, pdMS_TO_TICKS(LIN_SERVER_WAKE_UP_TIMEOUT_MS), portMAX_DELAY));
        TRUE_CHECK(xTimerReset(timer_handle, portMAX_DELAY));
        break;
    case LIN_SERVER_STATE_WAKEUP_PENDING:
        TRUE_CHECK(xTimerStop(timer_handle, portMAX_DELAY));
        return;
    default:
        LOG(E, "Lin Master timer not set. Unsupported state:%d", state);
        break;
    }
}

static lin_server_state_t lin_server_get_state(void)
{
    return lin_server.state;
}

static void lin_server_set_state(lin_server_state_t state)
{
    lin_server.state = state;
}

static bool lin_server_is_in_current_state(lin_server_state_t state_to_check)
{
    return lin_server_get_state() == state_to_check;
}

void lin_server_switch_state(lin_server_state_t state)
{
    lin_server_generic_event(LIN_SERVER_GENERIC_SWITCH_STATE_EVENT, &state, sizeof(state));
}

static void lin_server_timer_cb(TimerHandle_t xTimer)
{
    lin_server_generic_event(LIN_SERVER_GENERIC_TIMER_EVENT, NULL, 0);
}

void lin_server_init(void)
{
    lin_server_set_state(LIN_SERVER_STATE_WAKEUP_PENDING);
    TRUE_CHECK_RETURN(lin_server.lin_server_sleep_timer_handle = xTimerCreateStatic(
                          "LIN Server sleep timer",
                          pdMS_TO_TICKS(LIN_SERVER_GO_TO_SLEEP_TIMEOUT_MS),
                          pdTRUE,
                          NULL,
                          lin_server_timer_cb,
                          &lin_server.lin_server_sleep_timer));
    lin_server_switch_timer_mode();
}

/**
 * @brief Compile time validation if the device frame layout definition is 8 bytes
 */
COMPILE_TIME_ASSERT(LIN_FRAME_DATA_LEN >= sizeof(lin_server_device_frame_signals_t));
