/**
 * @file lin_server_uart.c
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server production class implementation
 * @date 2024-05-14
 */
#include "configuration.h"

#include "lin_server.h"
#include "lin_server_production.h"

#include "ddm_store.h"

#define MODEL_UNKNOWN     -1
#define MODEL_UNKNOWN_STR "unknown"

static void lin_server_production_create_slave_entries(const lin_server_slave_device_t *const slave_device)
{
    ddm_store__load_entries(
        slave_device->ddm_production_store,
        slave_device->ddm_production_initial_values,
        slave_device->ddm_production_initial_values_size,
        0);
}

static void lin_server_production_delete_slave_entries(const lin_server_slave_device_t *const slave_device)
{
    ddm_store__delete_all(slave_device->ddm_production_store);
}

static const char *lin_server_production_get_device_model(uint32_t device_class, uint32_t model)
{
    const char *device_model = MODEL_UNKNOWN_STR;
    switch (device_class)
    {
    case AC0:
    {
        switch (model)
        {
        case AC0MDL_NONE:
            device_model = "none";
            break;
        case AC0MDL_DOMETIC_FRESHJET:
            device_model = "DOMETIC FreshJet";
            break;
        case AC0MDL_TRUMA_AVENTA_COMFORT:
            device_model = "TRUMA Aventa Comfort";
            break;
        case AC0MDL_TRUMA_AVENTA_COMFORT_CP_PLUS:
            device_model = "TRUMA Aventa Comfort CP+";
            break;
        case AC0MDL_TRUMA_SAPHIR_COMPACT:
            device_model = "TRUMA Saphir Compact";
            break;
        case AC0MDL_TRUMA_SAPHIR_COMPACT_CP_PLUS:
            device_model = "TRUMA Saphir Compact CP+";
            break;
        case AC0MDL_TRUMA_AVENTA_ECO:
            device_model = "TRUMA Aventa Eco";
            break;
        case AC0MDL_TRUMA_AVENTA_ECO_CP_PLUS:
            device_model = "TRUMA Aventa Eco CP+";
            break;
        case AC0MDL_TRUMA_SAPHIR_COMFORT_RC:
            device_model = "TRUMA Saphir Comfort-RC";
            break;
        case AC0MDL_TRUMA_SAPHIR_COMFORT_RC_CP_PLUS:
            device_model = "TRUMA Saphir Comfort-RC CP+";
            break;
        case AC0MDL_DOMETIC_FRESHWELL:
            device_model = "DOMETIC FreshWell";
            break;
        case AC0MDL_UNKNOWN:
            device_model = "unknown";
            break;
        case AC0MDL_DOMETIC_FJX4000_SERIES:
            device_model = "Dometic FJX4000 series";
            break;
        case AC0MDL_DOMETIC_FJX7000_SERIES:
            device_model = "Dometic FJX7000 series";
            break;
        case AC0MDL_DOMETIC_APAC_HARRIER:
            device_model = "Dometic APAC Harrier";
            break;
        case AC0MDL_DOMETIC_APAC_IBIS4:
            device_model = "Dometic APAC IBIS4";
            break;
        case AC0MDL_DOMETIC_APAC_CK_LITE:
            device_model = "Dometic APAC CK-Lite";
            break;
        default:
            LOG(E, "Device class[0x%X] model not supported", device_class);
            break;
        }
        break;
    }
    case MBAT0:
    {
        switch (model)
        {
        case TLB150:
            device_model = "NDS TLB150";
            break;
        case TLB120:
            device_model = "NDS TLB120";
            break;
        case TLB100:
            device_model = "NDS TLB100";
            break;
        case TLB150F:
            device_model = "NDS TLB150F";
            break;
        case TLB120F:
            device_model = "NDS TLB120F";
            break;
        case TLB100F:
            device_model = "NDS TLB100F";
            break;
        default:
            LOG(E, "Device class[0x%X] model not supported", device_class);
            break;
        }
        break;
    }
    default:
        LOG(W, "Device class[0x%X] not yet supported", device_class);
        break;
    }

    return device_model;
}

static void lin_server_production_initialize_parameters(const lin_server_slave_device_t *slave_device)
{
    const char *model_name_str = MODEL_UNKNOWN_STR;
    ddm_entry_t *prod_name_entry = ddm_store__access(slave_device->ddm_production_store, PROD0NAME);
    ddm_entry_t *prod_mdl_entry = ddm_store__access(slave_device->ddm_production_store, PROD0MDL);
    ddm_entry_t *prod_clist_entry = ddm_store__access(slave_device->ddm_production_store, PROD0CLIST);

    if (slave_device->generic_profile)
    {
        ddm_entry_t *model_id = ddm_store__access(slave_device->ddm_owned_store, slave_device->generic_profile->discriminator_ddm);
        if (model_id)
        {
            model_name_str = lin_server_production_get_device_model(slave_device->device_class, ddm_entry__value_i32(model_id));
        }
        else
        {
            /* It is possible for certain DDM classes to not have discriminator parameter.
             * Use runtime mutable device storage to get the model instead. */
            model_name_str = lin_server_production_get_device_model(slave_device->device_class, slave_device->data->active_device_model);
        }
    }
    else
    {
        model_name_str = lin_server_production_get_device_model(slave_device->device_class, MODEL_UNKNOWN);
    }

    ddm_entry__set__value_str(prod_name_entry, slave_device->name, strlen(slave_device->name));
    ddm_entry__set__value_struct(prod_clist_entry, &slave_device->device_class, sizeof(slave_device->device_class));
    ddm_entry__set__value_str(prod_mdl_entry, model_name_str, strlen(model_name_str));
}

int lin_server_production_reqister_production_class_instance(const lin_server_slave_device_t *slave_device)
{
    TRUE_CHECK_RETURN0(slave_device->ddm_production_store);
    TRUE_CHECK_RETURN0(slave_device->ddm_production_initial_values);
    TRUE_CHECK_RETURN0(slave_device->ddm_production_initial_values_size != 0);

    lin_server_production_create_slave_entries(slave_device);

    slave_device->data->prod_instance = lin_server_register_ddm_class_instance(PROD0);
    TRUE_CHECK_RETURN0(slave_device->data->prod_instance != -1);

    LOG(I, "Production class instance[%d] has been registered for device[%s]",
        slave_device->data->prod_instance,
        slave_device->name);

    lin_server_production_initialize_parameters(slave_device);

    return 1;
}

void lin_server_production_remove_production_class_instance(const lin_server_slave_device_t *slave_device)
{
    lin_server_delete_ddm_class_instance(PROD0, slave_device->data->prod_instance);

    LOG(I, "Production class instance[%d] removed for device[%s]",
        slave_device->data->prod_instance,
        slave_device->name);

    slave_device->data->prod_instance = -1;

    lin_server_production_delete_slave_entries(slave_device);
}

void lin_server_production_handle_set(const lin_server_slave_device_t *const device, uint32_t parameter, const void *const value, size_t value_size)
{
    ddm_entry_t *ddm_entry;
    const void *ddm_data;
    size_t ddm_data_size;

    ddm_entry = ddm_store__access(device->ddm_production_store, parameter);
    if (ddm_entry == NULL)
    {
        /* Somebody tried to SET something that we are not owner to */
        LOG(W,
            "%s: set of non-owned production DDM: 0x%X",
            device->name,
            parameter | DDM2_PARAMETER_INSTANCE(device->data->prod_instance));

        return;
    }

    // Store parameter value
    ddm_entry__set__value(ddm_entry, value, value_size);

    if (ddm_entry__has_changed(ddm_entry))
    {
        // Publish current state
        ddm_entry__read__value(ddm_entry, &ddm_data, &ddm_data_size);
        TRUE_CHECK_RETURN(lin_server_publish(
            ddm_entry__parameter_id(ddm_entry),
            device->data->prod_instance,
            ddm_data,
            ddm_data_size));
    }

    // reset changed flag
    ddm_entry__set__has_changed(ddm_entry, false);
}

void lin_server_production_handle_subscribe(const lin_server_slave_device_t *const device, uint32_t parameter)
{
    ddm_entry_t *ddm_entry;
    const void *ddm_data;
    size_t ddm_data_size;

    ddm_entry = ddm_store__access(device->ddm_production_store, parameter);
    if (ddm_entry == NULL)
    {
        /* Somebody tried to SUBSCRIBE to something that we do not handle */
        LOG(W, "%s: subscription of non-handled production DDM: 0x%X",
            device->name,
            parameter | DDM2_PARAMETER_INSTANCE(device->data->prod_instance));
        return;
    }

    if (ddm_entry__is_value_int32(ddm_entry))
    {
        LOG(D, "%s: Production DDM \"%s0%s\" subscription value: %d",
            device->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
            ddm_entry__value_i32(ddm_entry));
    }
    else if (ddm_entry__is_value_str(ddm_entry))
    {
        LOG(D, "%s: Production DDM \"%s0%s\" subscription value: %s",
            device->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry),
            ddm_entry__value_size(ddm_entry) ? ddm_entry__value_str(ddm_entry) : "");
    }
    else
    {
        LOG(D, "%s: Production DDM \"%s0%s\" subscription",
            device->name,
            ddm_entry__device_class(ddm_entry), ddm_entry__property(ddm_entry));
    }

    ddm_entry__read__value(ddm_entry, &ddm_data, &ddm_data_size);
    TRUE_CHECK(lin_server_publish(
        ddm_entry__parameter_id(ddm_entry),
        device->data->prod_instance,
        ddm_data,
        ddm_data_size));
}
