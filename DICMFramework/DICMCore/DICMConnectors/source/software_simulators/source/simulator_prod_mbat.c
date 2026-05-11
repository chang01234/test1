/**
 * \file
 * \date	   2025-03-25
 * \author	   Andreas Lundeen
 * \brief	   Simulator for PROD DDM2 class - variant MBAT implementation
 *
 * Implementation of tables to simulate PROD DDM2 class - variant `mbat`
 *
 * \copyright   Dometic Group
 *			  This source file and the information contained in it are
 *			  confidential and proprietary to Dometic Group
 *			  The reproduction or disclosure, in whole or in part,
 *			  to anyone outside of Dometic Group without the written
 *			  approval of a Dometic Group officer under a Non-Disclosure
 *			  Agreement is expressly prohibited.
 *
 *			  All rights reserved
 */
#include <stdint.h>
#include <string.h>

#include "configuration.h"

#include "broker.h"
#include "ddm2.h"  // ELEMENTS
#include "ddm2_parameter_list.h"
#include "ddm_entry.h"
#include "ddm_store.h"
#include "esp_attr.h"  // EXT_RAM_ATTR
#include "esp_log.h"

#define SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK 2048 + 1024
#include "product_database.h"
#include "simulator_prod_mbat.h"
#include "software_simulator.h"
#include "software_simulator_defaults.h"
#include "uid_generator.h"

#ifndef SOFTWARE_SIMULATOR_PRODMBAT_MANUF
#define SOFTWARE_SIMULATOR_PRODMBAT_MANUF "GOPOWER"
#endif

typedef struct CLIST_T
{
    uint32_t list[5];
} PACKED CLIST_T;

static CLIST_T clist = {
    .list = {
        0x01020000,
    },
};

// Needed for the actions
static const struct ddm_store_ddm s__ddm_owned_initial_values[] = {
    {
        .ddm_parameter = PROD0CLIST,
        .value = {
            .storage = {.structure = NULL},
            .size = 0,
            .type = DDM2_TYPE_STRUCT,
        },
    },
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

static const struct ddm_store_ddm s__ddm_other_initial_values[] = {
    {
        .ddm_parameter = MBAT0AVL,
        .value = {
            .storage = {.i32 = 0},
            .type = DDM2_TYPE_INT32_T,
        },
    },
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_other_store, ELEMENTS(s__ddm_other_initial_values));

static bool my_mbat0avl_handler(struct software_simulator *p_sim, ddm_entry_t *p_entry, void *p_arg)
{
    bool has_changed = false;
    static bool is_initialized = false;
    ddm_entry_t *p_mbat0avl, *p_entry_clist;
    (void)p_entry;  // entry is set to NULL by software simulator since the action is not associated with an owned DDM parameter

    LOG(D, "action handler MBAT0AVL");
    p_mbat0avl = ddm_store__access(software_simulator__other_store(p_sim), MBAT0AVL);

    if ((ddm_entry__value_i32(p_mbat0avl) == 1) && (is_initialized == false))
    {
        is_initialized = true;
        static prod_database_t prod_data = {0};
        strcpy(prod_data.name, "Battery name");
        strcpy(prod_data.mdl, "Battery product mdl");
        strcpy(prod_data.sn, "Battery product sn");

        // Register instance to broker
        int prod_inst = ProdDBProdClassNodeCreate(&prod_data, sizeof(prod_data), p_sim->p__connector->connector_id);
        if (prod_inst < 0)
        {
            // invalid creation
            LOG(E, "Could not create prod node %d", prod_inst);
            return true;
        }
        prodxprop_type_t type = {0};
        type.type.cls = PRODXPROP_TYPE_CLASS_POWER;
        type.type.intf = PRODXPROP_TYPE_INTERFACE_UNKNOWN;

        ProdDBUpdateCache((const void *)&type, sizeof(prodxprop_type_t), FIELD_PROP_TYPE, prod_inst);
        ProdDBUpdateCache(SOFTWARE_SIMULATOR_PRODMBAT_MANUF, 0, FIELD_MANUF, prod_inst);
        ProdDBUpdateCache("1.0.0", 0, FIELD_FWVER, prod_inst);
        ProdDBUpdateCache("1.0.0", 0, FIELD_HWVER, prod_inst);
        ProdDBUpdateCache("Battery product pnc", 0, FIELD_PNC, prod_inst);
        ProdDBUpdateCache("Battery product sku", 0, FIELD_SKU, prod_inst);
        ProdDBUpdateCache("Simulated battery product", 0, FIELD_DESC, prod_inst);
        ProdDBUpdateCache("Battery product ean", 0, FIELD_EAN, prod_inst);

        // Add DDM store to software simulator. NOTE: Using private structure members since this is not
        // supported by existing software simulator API.
        p_sim->p__ddm_owned_store = &s__ddm_owned_store;
        p_sim->instance = prod_inst;
        int32_t instance = prod_inst;

        // Add entries and load initial values from predefined array
        ddm_store__load_entries(p_sim->p__ddm_owned_store, s__ddm_owned_initial_values, ELEMENTS(s__ddm_owned_initial_values), instance);
        p_entry_clist = ddm_store__access(software_simulator__owned_store(p_sim), PROD0CLIST | DDM2_PARAMETER_INSTANCE(instance));
        if (p_entry_clist)
        {
            // Set initial value
            uint32_t param = ddm_entry__parameter_id(p_mbat0avl);
            clist.list[0] = param;
            LOG(D, "Added 0x%08x to PROD%uCLIST", param, instance);
            has_changed = ddm_entry__set__value(p_entry_clist, &clist, sizeof(uint32_t));
            ddm_entry__set__has_changed(p_entry_clist, false);

            // update cache
            ProdDBUpdateCache((const void *)&clist, sizeof(uint32_t), FIELD_CLIST, prod_inst);
        }
    }
    return has_changed;
}

static bool my_prod0clist_handler(struct software_simulator *p_sim, ddm_entry_t *p_entry, void *p_arg)
{
    // we already have handled this in ProdDBFrameHandler() so just make a copy to our database
    ddm_entry_t *p_entry_clist;
    (void)p_entry;  // entry is set to NULL by software simulator since the action is not associated with an owned DDM parameter

    p_entry_clist = ddm_store__access(software_simulator__owned_store(p_sim), PROD0CLIST | DDM2_PARAMETER_INSTANCE(p_sim->instance));
    CLIST_T l_data = {0};
    size_t l_data_size = sizeof(l_data);
    ProdDBReadCache(FIELD_CLIST, p_sim->instance, &l_data, &l_data_size);
    ESP_LOG_BUFFER_HEXDUMP("my_prod0clist_handler", &l_data, l_data_size, ESP_LOG_DEBUG);
    ddm_entry__set__value(p_entry_clist, &l_data, l_data_size);
    ddm_entry__set__has_changed(p_entry_clist, false);

    return false;
}

static const struct software_simulator__action_entry s__ddm__action_entries[] = {
    {
        .ddm_parameter = 0,
        .trigger = {.any = {MBAT0AVL}},
        .handler = my_mbat0avl_handler,
    },
    {
        .ddm_parameter = 0,
        .trigger = {.any = {PROD0CLIST}},
        .handler = my_prod0clist_handler,
    },
};

static bool hook_function(DDMP2_FRAME *p_frame)
{
    // Handle product database here
    ProdDBFrameHandler(p_frame);
    // Continue execution in simulation worker task
    return true;
}

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_prod_mbat_descriptor = {
    .ddm_class = DDM2_PARAMETER_CLASS(PROD0AVL),
    // Do not set here as we want to control and use ProdDB instead
    //	.ddm_owned_store = &s__ddm_owned_store,
    //	.ddm_owned_initial_values = s__ddm_owned_initial_values,
    //	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
    .ddm_other_store = &s__ddm_other_store,
    .ddm_other_initial_values = s__ddm_other_initial_values,
    .ddm_other_initial_values_size = ELEMENTS(s__ddm_other_initial_values),
    .actions = s__ddm__action_entries,
    .actions_size = ELEMENTS(s__ddm__action_entries),
    .worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
    .worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
    .name = "PROD-MBAT",
    .hook_function = hook_function,
};
