/*! \file connector_hmi.c
    \brief HMI connector for HMI platform
 */

#include "configuration.h"

#include "connector_hmi.h"
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "ddm2_parameter_list.h"
#include "ddm_wrapper.h"
#include "hal_mem.h"
#include "ui_engine.h"
#include "st7789v.h"
#include "draw.h"
#include "hmi_dispatcher.h"
#include "hmi_sensor.h"
#include "varstate.h"
#include "ulp_api.h"

#include "input_manager.h"

// Definition of dynamic item resolution used to store if we have resolved a dynamic parameter id or not
typedef struct _dynamic_item_t
{
    uint32_t param_id;      // Orig parameter id from data
    uint32_t resolved_id;   // Resolved parameter id
    bool is_resolved;       // true when fully resolved
    size_t index;           // Index into ddmw_items list
} dynamic_item_t;

typedef struct _hmi_connector_data_t
{
    ddmw_t ddm_wrapper;

    size_t num_ddmw_items;		//!< Number of DDMW items.
    size_t num_dyn_items;       //!< Number of dynamic DDMW items.
    size_t start_ddmw_sub_items_index;  //!< Number of DDMW subscribe items.
    dynamic_item_t *dyn_items;  //!< List of dyn items resolution information
    ddmw_item_t *ddmw_items;	//!< DDMW items.
    size_t *ddmw_varstate_idx;	//!< Varstate index for each DDMW item.
    DDM2_TYPE_ENUM *ddmw_type;  //!< Type for each DDMW item.
} hmi_connector_data_t;

// Dummy functions for callbacks
static void dummy_task_cb(void) {}
static void dummy_ddmw_updated_cb(uint8_t a, int32_t b) {}
static void connector_hmi_ddmp_set_cb(uint32_t parameter_id, uint8_t var_index);
static void connector_hmi_varstate_cb(uint8_t var_index);
static void mark_dyn_item_as_resolved(uint32_t orig_param, uint32_t resolved_param, size_t index);
static bool is_dyn_item_resolved(uint32_t orig_param, uint32_t *resolved_param);
static void monitor_gw0inv(uint32_t parameter);

// My callback functions, defaulted to do nothing
static hmi_update_task_cb product_update_task = dummy_task_cb;
static hmi_ddmw_updated_cb product_ddmw_updated = dummy_ddmw_updated_cb;
static hmi_menu_ddmp_set_cb product_hmi_ddmp_set = NULL;
static hmi_varstate_set_cb product_hmi_varstate_set = NULL;

void set_hmi_update_task_cb(hmi_update_task_cb cb)
{
    if (NULL != cb)
    {
        product_update_task = cb;
    }
}

void set_ddmw_updated_cb(hmi_ddmw_updated_cb cb)
{
    if (NULL != cb)
    {
        product_ddmw_updated = cb;
    }
}

void set_hmi_ddmp_set_cb(hmi_menu_ddmp_set_cb cb)
{
    product_hmi_ddmp_set = cb;
}

void set_hmi_varstate_cb(hmi_varstate_set_cb cb)
{
    product_hmi_varstate_set = cb;
}

void hmi_product_update_task(void)
{
    if (product_update_task)
    {
        product_update_task();
    }
}

static EXT_RAM_ATTR hmi_connector_data_t hmi_connector_data;

static void connector_hmi_ddmp_set_cb(uint32_t parameter_id, uint8_t var_index)
{
    // parameter_id can be a dynamic parameter so we need to check that and resolve if possible
    if (parameter_id & HMI_DATA_DYNAMIC_PARAM_MASK)
    {
        uint32_t resolved_param = 0;
        if (is_dyn_item_resolved(parameter_id, &resolved_param))
        {
            LOG(D, "Resolving param: 0x%08x to 0x%08x", parameter_id, resolved_param);
            parameter_id = resolved_param;
        }
    }
    if (NULL != product_hmi_ddmp_set)
    {
        // Call override function
        product_hmi_ddmp_set(parameter_id, var_index);
    }
    else
    {
        //uint8_t parameter_length = varstate_get_parameter_type(var_index);
        int32_t value = varstate_get(var_index);
        p_ddmp_set_sub_array[var_index] = value;

        // Find item
        ddmw_item_t *p_item = ddmw_find_item(&hmi_connector_data.ddm_wrapper, parameter_id);
        if (p_item)
        {
            ddmw_set_i32(p_item, value);
        }
    }
}

static void connector_hmi_varstate_cb(uint8_t var_index)
{
    if (NULL != product_hmi_varstate_set)
    {
        product_hmi_varstate_set(var_index);
    }
}

static void ddmw_update_check(void)
{
    for (size_t i = 0; i < hmi_connector_data.num_ddmw_items; i++)
    {
        if (ddmw_is_updated(&hmi_connector_data.ddmw_items[i]))
        {
            // Check dependencies from dyn map table
            uint8_t var_index = hmi_connector_data.ddmw_varstate_idx[i];
            const VARSTATE_DYNENTRY_DEF *p_entry = varstate_find_any_dyn_table_entry_from_var_index(var_index);
            if (p_entry)
            {
                LOG(D, "Parameter (0x%08x) referenced in dynamic table", hmi_connector_data.ddmw_items[i].parameter);
                // One or more dynamic parameter specification dependency is updated
                // 1. Find out which other ddmw items are mapped to updated item
                // 2. For each found item, check the ddm2 out type. It has to be uint32_t (out unit: DDM2_UNIT_PARAMETER,DDM2_UNIT_INSTANCE)
                //    or struct (out unit: DDM2_UNIT_PARAMETER,DDM2_UNIT_INSTANCE
                int index = ddm2_parameter_list_lookup(hmi_connector_data.ddmw_items[i].parameter);
                if (index == -1)
                {
                    LOG(E, "Parameter not found (0x%08x)", hmi_connector_data.ddmw_items[i].parameter);
                    continue;
                }
                DDM2_TYPE_ENUM out_type = (DDM2_TYPE_ENUM)Ddm2_parameter_list_data[index].out_type;
                DDM2_UNIT_ENUM out_unit = (DDM2_UNIT_ENUM)Ddm2_parameter_list_data[index].out_unit;
                bool valid_parameter = false;
                if ((out_type == DDM2_TYPE_UINT32_T) && ((out_unit == DDM2_UNIT_PARAMETER) || (out_unit == DDM2_UNIT_INSTANCE)))
                {
                    valid_parameter = true;
                    LOG(D, "Parameter is of valid type: DDM2_TYPE_UINT32_T (DDM2_UNIT_PARAMETER|DDM2_UNIT_INSTANCE)");
                }
                else if ((out_type == DDM2_TYPE_STRUCT) && ((out_unit == DDM2_UNIT_PARAMETER) || (out_unit == DDM2_UNIT_INSTANCE)))
                {
                    valid_parameter = true;
                    LOG(D, "Parameter is of valid type: DDM2_TYPE_STRUCT (DDM2_UNIT_PARAMETER|DDM2_UNIT_INSTANCE)");
                }
                else
                {
                    valid_parameter = false;
                }
                for (size_t j = 0; (valid_parameter == true) && (j < hmi_connector_data.num_ddmw_items); j++)
                {
                    if (i == j)
                    {
                        continue;   // Do not check ourself
                    }

                    // Is item a dynamic item
                    if (hmi_connector_data.ddmw_items[j].parameter & HMI_DATA_DYNAMIC_PARAM_MASK)
                    {
                        // This parameter has not been subscribed to yet. Can we fully resolve via dynamic map table now?
                        uint32_t orig_param = hmi_connector_data.ddmw_items[j].parameter;
                        // Instance value is index into the table
                        uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(orig_param);
                        const VARSTATE_DYNENTRY_DEF *p_entry = varstate_get_dyn_table_entry(instance);
                        if (var_index == p_entry->var_index)
                        {
                            LOG(D, "Parameter (0x%08x) affects var_index %d", hmi_connector_data.ddmw_items[i].parameter, var_index);
                            // Found a match; "i" item affects "j" item
                            // Get the data from "i" item
                            int paramdata_inst = -1;
                            if (out_type == DDM2_TYPE_UINT32_T)
                            {
                                uint32_t paramdata = ddmw_get_u32(&hmi_connector_data.ddmw_items[i]);
                                paramdata_inst = DDM2_PARAMETER_INSTANCE_FIELD(paramdata);
                            }
                            else
                            {
                                // DDM2_TYPE_STRUCT
                                int num_params = (ddmw_sizeof(&hmi_connector_data.ddmw_items[i]) / sizeof(uint32_t));
                                uint32_t paramdata[num_params];
                                LOG(D, "Parameter (0x%08x) data size %d", hmi_connector_data.ddmw_items[i].parameter, num_params*sizeof(uint32_t));
                                if (ddmw_get_data(&hmi_connector_data.ddmw_items[i], paramdata, num_params * sizeof(uint32_t)) == (num_params * (int)sizeof(uint32_t)))
                                {
                                    // data retrieved
                                    // Now we loop the data to find same base class as the dynamic parameter (mask off dynamic mask)
                                    uint8_t class_num = 0;
                                    for (int loop = 0; loop < num_params; ++loop)
                                    {
                                        LOG(D, "Parameter (0x%08x) data %08x", hmi_connector_data.ddmw_items[i].parameter, paramdata[loop]);
                                        if ((DDM2_PARAMETER_CLASS(hmi_connector_data.ddmw_items[j].parameter) & ~HMI_DATA_DYNAMIC_PARAM_MASK) ==
                                                DDM2_PARAMETER_CLASS(paramdata[loop]))
                                        {
                                            if (class_num == p_entry->data)
                                            {
                                                paramdata_inst = DDM2_PARAMETER_INSTANCE_FIELD(paramdata[loop]);
                                                break;
                                            }
                                            else
                                            {
                                                // Check for next match
                                                class_num++;
                                            }
                                        }
                                    }
                                }
                            }
                            // Have we found a match?
                            if (paramdata_inst != -1)
                            {
                                // update parameter id and subscribe
                                uint32_t parameter_id = DDM2_PARAMETER_BASE_INSTANCE(orig_param & ~HMI_DATA_DYNAMIC_PARAM_MASK);
                                ddmw_add(&hmi_connector_data.ddm_wrapper, &hmi_connector_data.ddmw_items[j], parameter_id, paramdata_inst);
                                ddmw_set_type(&hmi_connector_data.ddmw_items[j], DDMW_ACTION_SET);
                                ddmw_subscribe(&hmi_connector_data.ddmw_items[j]);
                                // Mark as resolved
                                mark_dyn_item_as_resolved(orig_param, hmi_connector_data.ddmw_items[j].parameter, j);
                            }
                        }
                    }
                }
            }
            // Check hmi0-class items specifically here
            if (hmi_connector_data.ddmw_items[i].parameter == HMI0EVENT)
            {
                // Received data in format of {uint32_t id;uint8_t data[0] }
                // Example: Send an input event (button pressed) to UI_engine: 2100301000 where last byte is which button (0-middle, 1-left, 2-right)
                uint8_t data[20];
                int data_size = ddmw_get_data(&hmi_connector_data.ddmw_items[i], data, 20);
                LOG(D, "Generate HMI event id: %x", *(uint32_t*)data);
                send_generic_frame_to_connector(*(uint32_t*)data, &data[4], data_size-4, pdMS_TO_TICKS(200));
            }
            else if (hmi_connector_data.ddmw_items[i].parameter == HMI0VARDATA)
            {
                int32_t var_idx = ddmw_get_i32(&hmi_connector_data.ddmw_items[i]);
                int32_t var_data = varstate_get(var_idx);
                ddmw_set_i32(&hmi_connector_data.ddmw_items[i], var_data);
            }
            else if (hmi_connector_data.ddmw_items[i].parameter == HMI0BACKLIGHT)
            {
                // Handle backlight settings directly
                int32_t backlight = ddmw_get_i32(&hmi_connector_data.ddmw_items[i]);
                LOG(D, "HMI0BACKLIGHT: %d", backlight);
                if ((backlight >= 0) && (backlight <= 100))
                {
                    screen_display_brightness_set(backlight);
                    uint8_t var_index = hmi_connector_data.ddmw_varstate_idx[i];
                    if (backlight != varstate_get(var_index))
                    {
                        varstate_set_changed(var_index, true);
                    }
                    varstate_set(var_index, backlight);
                }
                else
                {
                    ddmw_set_i32(&hmi_connector_data.ddmw_items[i], screen_display_brightness_get());
                }
            }
            // Handle struct type variables
            else if (hmi_connector_data.ddmw_type[i] == DDM2_TYPE_STRUCT)
            {
                // We use these only as triggers
                uint8_t data[DDMW_ITEM_DATA_CAPACITY+1];
                uint8_t var_index = hmi_connector_data.ddmw_varstate_idx[i];
                int32_t size = ddmw_get_data(&hmi_connector_data.ddmw_items[i], &data[1], sizeof(data)-1);
                data[0] = (uint8_t)size;
                // Simply pass data to local function to handle
                product_ddmw_updated(var_index, (int32_t)data);
                uint8_t *pcdata;
                pcdata = (uint8_t *)varstate_get(var_index);
                if ((int32_t)pcdata == INT32_MAX || (pcdata[0] != data[0]) || memcmp(pcdata, data, data[0]+1))
                {
                    varstate_set(var_index, (int32_t)data);
                    varstate_set_changed(var_index, true);
                }
            }
            // Handle int32_t type variables
            else if ((hmi_connector_data.ddmw_type[i] == DDM2_TYPE_INT32_T) || (hmi_connector_data.ddmw_type[i] == DDM2_TYPE_UINT32_T))
            {
                uint8_t var_index = hmi_connector_data.ddmw_varstate_idx[i];
                int32_t value = ddmw_get_i32(&hmi_connector_data.ddmw_items[i]);
                if (hmi_connector_data.ddmw_type[i] == DDM2_TYPE_UINT32_T)
                {
                    value = ddmw_get_u32(&hmi_connector_data.ddmw_items[i]);
                }

                product_ddmw_updated(var_index, value);

                //LOG(D, "DDMP set HMI var #%d to %d.", var_index, value);
#ifdef CONNECTOR_HMI_DDMW_UPDATE_FORCE_VAR_CHANGE_ALWAYS
                // Skip check for real changed variable. Generate event for any update
#else
                if (value != varstate_get(var_index))
#endif
                {
                    varstate_set_changed(var_index, true);
                }
                varstate_set(var_index, value);
                //Set the ddmp subscribe value into ddmp set and subscribe array
                p_ddmp_set_sub_array[var_index] = value;
            }
        }
    }
}

/**
 * @brief Store and mark a dynamic parameter as resolved
 *
 * @param[in] orig_param Dynamic paramter id
 * @param[in] resolved_param Resolved parameter id
 * @param[in] index Index into the ddmw_item list
 *
 */
static void mark_dyn_item_as_resolved(uint32_t orig_param, uint32_t resolved_param, size_t index)
{
    for (size_t i = 0; i < hmi_connector_data.num_dyn_items; ++i)
    {
        if (hmi_connector_data.dyn_items[i].index == index)
        {
            if (hmi_connector_data.dyn_items[i].is_resolved)
            {
                LOG(W, "Trying to resolve already resolved param: 0x%08x:0x%08x, index: %d", orig_param, resolved_param, index);
                break;
            }

            if (hmi_connector_data.dyn_items[i].param_id == orig_param)
            {
                // update to resolved
                hmi_connector_data.dyn_items[i].resolved_id = resolved_param;
                hmi_connector_data.dyn_items[i].is_resolved = true;
                LOG(D, "Resolved 0x%08x to 0x%08x", orig_param, resolved_param);
                break;
            }
        }
    }
}

/**
 * @brief Check and return if a given dynamic parameter id is resolved or not
 *
 * @param[in] orig_param Dynamic paramter id
 * @param[out] resolved_param Pointer to store the resolved parameter id
 *
 * @return true if parameter is resolved, false if not
 */
static bool is_dyn_item_resolved(uint32_t orig_param, uint32_t *resolved_param)
{
    bool ret_val = false;

    for (size_t i = 0; i < hmi_connector_data.num_dyn_items; ++i)
    {
        if (hmi_connector_data.dyn_items[i].param_id == orig_param)
        {
            ret_val = hmi_connector_data.dyn_items[i].is_resolved;
            if (ret_val)
            {
                if (resolved_param)
                {
                    *resolved_param = hmi_connector_data.dyn_items[i].resolved_id;
                }
            }
        }
    }
    return ret_val;
}
/**
 * Generic handling of the re-subscribing to parameters / variables specified in hmi data
 */
static void monitor_gw0inv(uint32_t parameter)
{
    // Loop all items to check if any needs to be re-subscribed
    for (size_t par_index = 0; par_index < hmi_connector_data.num_ddmw_items; ++par_index)
    {
        ddmw_item_t *p_item = &hmi_connector_data.ddmw_items[par_index];
        if (ddmw_get_type(p_item) == DDMW_ACTION_SET)
        {
            // We are only interested in sub variables
            if (DDMP2_INVENTORY_CLASS_INSTANCE(p_item->parameter) == DDMP2_INVENTORY_CLASS_INSTANCE(parameter))
            {
                if (DDMP2_INVENTORY_AVL(parameter))
                {
                    // New class added to inventory
                    if (!ddmw_is_subscribed(p_item))
                    {
                        // (Re-)subscribe
                        ddmw_subscribe(p_item);
                    }
                }
                else
                {
                    // Class removed from inventory
                    // Re-add with default setting
                    ddmw_add(&hmi_connector_data.ddm_wrapper, p_item, p_item->parameter, DDM2_PARAMETER_INSTANCE_FIELD(p_item->parameter));
                    ddmw_set_type(p_item, DDMW_ACTION_SET);
                }
            }
        }
    }
}

void initialize_ddm_wrapper(void)
{
    const VARSTATE_DDMP_PARAMETER *pub_list;
    const VARSTATE_DDMP_PARAMETER *sub_list;

    pub_list = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.pub_list != NULL ? HMI_DATA_ADDRESS(def_data_header.pub_list) : (uint32_t)def_data_header.pub_list);
    sub_list = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.sub_list != NULL ? HMI_DATA_ADDRESS(def_data_header.sub_list) : (uint32_t)def_data_header.sub_list);

    hmi_connector_data.num_ddmw_items = 0;
    hmi_connector_data.num_dyn_items = 0;
    hmi_connector_data.start_ddmw_sub_items_index = 0;
    hmi_connector_data.ddmw_items = NULL;
    hmi_connector_data.dyn_items = NULL;
    hmi_connector_data.ddmw_varstate_idx = NULL;
    hmi_connector_data.ddmw_type = NULL;

    // Count total number of items in PUB and SUB lists.
    size_t num_items = 0;
    for (const VARSTATE_DDMP_PARAMETER *par = pub_list; par != NULL; par = (par->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(par->next_entry) : NULL))
    {
        num_items++;
    }
    size_t num_dyn_params = 0;
    for (const VARSTATE_DDMP_PARAMETER *par = sub_list; par != NULL; par = (par->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(par->next_entry) : NULL))
    {
        // If parameter is of dynamic type, we cannot subscribe here
        if (par->parameter_id & HMI_DATA_DYNAMIC_PARAM_MASK)
        {
            // Dynamic parameter
            num_dyn_params++;
        }

        num_items++;
    }

    LOG(D, "Allocate item list (%d:%d)", num_items, num_dyn_params);
    // Allocate item list.
    hmi_connector_data.num_dyn_items = num_dyn_params;
    hmi_connector_data.num_ddmw_items = num_items;
    if (num_items > 0)
    {
        hmi_connector_data.ddmw_items = hal_mem_malloc_prefer(sizeof(ddmw_item_t) * num_items, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        hmi_connector_data.ddmw_varstate_idx = hal_mem_malloc_prefer(sizeof(size_t) * num_items, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        hmi_connector_data.ddmw_type = hal_mem_malloc_prefer(sizeof(DDM2_TYPE_ENUM) * num_items, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        if (num_dyn_params > 0)
        {
            hmi_connector_data.dyn_items = hal_mem_malloc_prefer(sizeof(dynamic_item_t) * num_dyn_params, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            ASSERT(hmi_connector_data.dyn_items != NULL);
            assert(hmi_connector_data.dyn_items != NULL);
        }
        ASSERT(hmi_connector_data.ddmw_items != NULL);
        assert(hmi_connector_data.ddmw_items != NULL);
        ASSERT(hmi_connector_data.ddmw_varstate_idx != NULL);
        assert(hmi_connector_data.ddmw_varstate_idx != NULL);
        ASSERT(hmi_connector_data.ddmw_type != NULL);
        assert(hmi_connector_data.ddmw_type != NULL);
        memset(hmi_connector_data.ddmw_items, 0, sizeof(ddmw_item_t) * num_items);
        memset(hmi_connector_data.ddmw_varstate_idx, 0, sizeof(size_t) * num_items);
        memset(hmi_connector_data.ddmw_type, DDM2_TYPE_INT32_T, sizeof(DDM2_TYPE_ENUM) * num_items);
    }

    ddmw_init(&hmi_connector_data.ddm_wrapper, &connector_hmi);

    // Add all parameters.
    size_t par_index = 0;
    for (const VARSTATE_DDMP_PARAMETER *par = pub_list; par != NULL; par = (par->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(par->next_entry) : NULL))
    {
        if (DDM2_PARAMETER_PROPERTY_FIELD(par->parameter_id) == 0)
        {
#ifdef HMI_VARSTATE_DEBUG_NAMES		
            LOG(D, "REG %08x (%d:%s)", par->parameter_id, par->var_index, par->name);
#else
            LOG(D, "REG %08x (%d)", par->parameter_id, par->var_index);
#endif
            ddmw_register(&hmi_connector_data.ddm_wrapper, DDM2_PARAMETER_CLASS(par->parameter_id));
        }
    }
    for (const VARSTATE_DDMP_PARAMETER *par = pub_list; par != NULL; par = (par->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(par->next_entry) : NULL))
    {
#ifdef HMI_VARSTATE_DEBUG_NAMES
        LOG(D, "PUB %08x (%d:%s)", par->parameter_id, par->var_index, par->name);
#else
        LOG(D, "PUB %08x (%d)", par->parameter_id, par->var_index);
#endif
        ddmw_add(&hmi_connector_data.ddm_wrapper, &hmi_connector_data.ddmw_items[par_index], par->parameter_id, DDM2_PARAMETER_INSTANCE_FIELD(par->parameter_id));
        ddmw_set_type(&hmi_connector_data.ddmw_items[par_index], DDMW_ACTION_PUBLISH);
        hmi_connector_data.ddmw_type[par_index] = par->type;
        if (par->type == DDM2_TYPE_INT32_T)
        {
            ddmw_set_i32(&hmi_connector_data.ddmw_items[par_index], varstate_get(par->var_index));
        }
        else if (par->type == DDM2_TYPE_UINT32_T)
        {
            ddmw_set_u32(&hmi_connector_data.ddmw_items[par_index], varstate_get(par->var_index));
        }
        hmi_connector_data.ddmw_varstate_idx[par_index] = par->var_index;
        par_index++;
    }
    hmi_connector_data.start_ddmw_sub_items_index = par_index;
    num_dyn_params = 0;
    for (const VARSTATE_DDMP_PARAMETER *par = sub_list; par != NULL; par = (par->next_entry ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(par->next_entry) : NULL))
    {
#ifdef HMI_VARSTATE_DEBUG_NAMES
        LOG(D, "SUB %08x (%d:%s)", par->parameter_id, par->var_index, par->name);
#else
        LOG(I, "SUB %08x (%d)", par->parameter_id, par->var_index);
#endif
        ddmw_add(&hmi_connector_data.ddm_wrapper, &hmi_connector_data.ddmw_items[par_index], par->parameter_id, DDM2_PARAMETER_INSTANCE_FIELD(par->parameter_id));
        ddmw_set_type(&hmi_connector_data.ddmw_items[par_index], DDMW_ACTION_SET);
        //ddmw_set_i32(&hmi_connector_data.ddmw_items[par_index], varstate_get(par->var_index));
        //
        // If parameter is of dynamic type, we cannot subscribe here
        if (par->parameter_id & HMI_DATA_DYNAMIC_PARAM_MASK)
        {
            // Wait with subscription until we can fully resolve via dynamic map table
            // Instance value is index into the table
            int instance = DDM2_PARAMETER_INSTANCE_FIELD(par->parameter_id);
            const VARSTATE_DYNENTRY_DEF *p_entry = varstate_get_dyn_table_entry(instance);
            assert(p_entry);
            hmi_connector_data.dyn_items[num_dyn_params].is_resolved = false;
            hmi_connector_data.dyn_items[num_dyn_params].param_id = par->parameter_id;
            hmi_connector_data.dyn_items[num_dyn_params].resolved_id = 0;
            hmi_connector_data.dyn_items[num_dyn_params].index = par_index;
            num_dyn_params++;
        }
        else
        {
            ddmw_subscribe(&hmi_connector_data.ddmw_items[par_index]);
        }
        hmi_connector_data.ddmw_varstate_idx[par_index] = par->var_index;
        hmi_connector_data.ddmw_type[par_index] = par->type;
        par_index++;
    }
    ddmw_get_inventory(&hmi_connector_data.ddm_wrapper, monitor_gw0inv);
    // Add hmi data version
    ddmw_item_t *p_item = ddmw_find_item(&hmi_connector_data.ddm_wrapper, HMI0VER);
    if (p_item != NULL)
    {
        ddmw_set_str(p_item, hmi_data_get_version());
    }
}

void send_generic_frame_to_connector(uint32_t id, const void * data, const size_t data_size, uint32_t timeout)
{
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, id, data, data_size, hmi_connector_data.ddm_wrapper.connector->connector_id, timeout);
}

static int initialize_connector_hmi(void)
{
    // Initialize HMI.
    memset(&hmi_connector_data, 0, sizeof(hmi_connector_data));
    if (!hmi_data_is_compatible())
    {
        LOG(W, "HMI init failed due to HMI data compatibility reason");
        return 0;
    }
    varstate_init((const VARSTATE_CONF *)HMI_DATA_ADDRESS(def_data_header.varstate_conf), connector_hmi_varstate_cb);
    screen_init();
    initialize_ddm_wrapper();

    ui_engine_init((const EVENT_DEF *)HMI_DATA_ADDRESS(def_data_header.events), def_data_header.num_events, (const MENU_STATE *)HMI_DATA_ADDRESS(def_data_header.menu_boot_state), connector_hmi_ddmp_set_cb);
    input_manager_init();
#if defined(CONNECTOR_HMI_ULP_ROTARY_ENABLED) || defined(CONNECTOR_HMI_ULP_BUTTONS_ENABLED)
    input_manager_init_ulp();
#if defined(CONNECTOR_HMI_ULP_NTC_ENABLED)
    hmi_sensor_init();
#endif /* CONNECTOR_HMI_ULP_NTC_ENABLED */
#endif
    LOG(D, "HMI initialized ready");
    return 1;
}

static void connector_hmi_task(const DDMP2_FRAME * frame)
{
    ddmw_process(&hmi_connector_data.ddm_wrapper, frame);

    if (ddmw_is_generic_event_updated(&hmi_connector_data.ddm_wrapper))
    {
        ddmw_event_t event;

        event.event_id = ddmw_get_generic_event_id(&hmi_connector_data.ddm_wrapper);
        event.data_size = ddmw_get_generic_event_data(&hmi_connector_data.ddm_wrapper, event.data, sizeof(event.data));
        hmi_dispatch_event_to_module(event.event_id, event.data, event.data_size);
    }
    else // DDMP2
    {
        ddmw_update_check();
        hmi_dispatch_event_to_module(HMI_TYPE_SET_MODULE_FIELD(HMI_MODULE_TYPE_UI_ENGINE) | HMI_TYPE_SET_GROUP_FIELD(HMI_TYPE_DDMP2), NULL, 0);
    }

    ddmw_process_publish(&hmi_connector_data.ddm_wrapper);
}

CONNECTOR connector_hmi =				    // Broker's interface to connector
{
        .name = "HMI connector",			    // Connector name
        .initialize = initialize_connector_hmi,	// Connector initialize function
        .process_event = connector_hmi_task,	// Connector process event function
};

/** Imported from ddmp_uart ***************************************************/

int ddmp_uart_set(uint32_t parameter_id, int32_t value, size_t length)
{
    (void)length;

    uint8_t var_index = varstate_get_var_index_both(parameter_id);
    if (VARSTATE_INVALID_INDEX != var_index)
    {
        //Update HMI set value into ddmp set and subscribe array
        p_ddmp_set_sub_array[var_index] = value;

        ddmw_item_t *p_item = ddmw_find_item(&hmi_connector_data.ddm_wrapper, parameter_id);
        if (p_item)
        {
            ddmw_set_i32(p_item, value);
        }
    }
    else
    {
        LOG(W, "Parameter not in varstate: %08x", parameter_id);
    }
    return 0;
}

ddmw_item_t *connector_hmi_get_item_from_var_index(bool is_pub_item, uint8_t var_index)
{
    const VARSTATE_DDMP_PARAMETER *pub = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.pub_list ? HMI_DATA_ADDRESS(def_data_header.pub_list) : (uint32_t)def_data_header.pub_list);
    if (is_pub_item)
    {
        // Find parameter entry in publish list.
        while (pub != NULL)
        {
            if (pub->var_index == var_index)
            {
                break;
            }
            pub = (pub->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(pub->next_entry) : NULL);
        }
    }
    else
    {
        pub = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.sub_list ? HMI_DATA_ADDRESS(def_data_header.sub_list) : (uint32_t)def_data_header.sub_list);
        // Find parameter entry in subscribe list.
        while (pub != NULL)
        {
            if (pub->var_index == var_index)
            {
                break;
            }
            pub = (pub->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(pub->next_entry) : NULL);
        }
    }
    // Pub will be NULL only if no publish or subsbribe info found
    if (pub != NULL)
    {
        for (size_t i = 0; i < hmi_connector_data.num_ddmw_items; i++)
        {
            if (hmi_connector_data.ddmw_varstate_idx[i] == var_index)
            {
                return &hmi_connector_data.ddmw_items[i];
            }
        }
    }
    return NULL;
}
