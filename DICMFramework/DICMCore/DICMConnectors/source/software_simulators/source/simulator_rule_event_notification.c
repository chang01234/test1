/**
 * \file
 * \date		2025-02-03
 * \author	    (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief	    Simulator for RULE DDM2 class - variant `event notification` implementation
 *
 * Implementation of tables to simulate RULE DDM2 class - variant `event notification`
 *
 * \li		  	2022-07-05  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *			  	This source file and the information contained in it are
 *			  	confidential and proprietary to Dometic Group
 *			  	The reproduction or disclosure, in whole or in part,
 *			  	to anyone outside of Dometic Group without the written
 *			  	approval of a Dometic Group officer under a Non-Disclosure
 *			  	Agreement is expressly prohibited.
 *
 *			  	All rights reserved
 */

#include "ddm_entry.h"
#include "ddm_store.h"
#include "ddm2_parameter_list.h"
#include "esp_attr.h" // EXT_RAM_ATTR
#include "ddm2.h" // ELEMENTS
#include "software_simulator.h"
#define SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK 4096
#include "software_simulator_defaults.h"
#include "simulator_rule_event_notification.h"

struct itemp_action_context
{
	enum item_action_states
	{
		ITEMP_ACTION_LOW_NOTIFY,
		ITEMP_ACTION_HIGH_NOTIFY
	} state;
};

static bool action_internal_temp(
    struct software_simulator * ss,
    ddm_entry_t * ddm_entry,
    void * args);

static const struct ddm_store_ddm s__ddm_initial_values[] =
{
	{
		.ddm_parameter = RULE0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = RULE0NAME,
		.value =
		{
			.storage = { .str = "1" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = RULE0NUM,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
};

static const struct ddm_store_ddm s__ddm_other_initial_values[] =
{
	{
		.ddm_parameter = AC0AVL,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0ITEMP,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = EVM0TRIG,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
};

static const SOFTWARE_SIMULATOR__ACTION_ENTRY s__actions[] =
{
	{
		.ddm_parameter = EVM0TRIG,
		.trigger = {.any = {AC0ITEMP}},
		.handler = action_internal_temp,
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_store, ELEMENTS(s__ddm_initial_values));
DDM_STORE__DECLARE_EXTRAM(s__ddm_other_store, ELEMENTS(s__ddm_other_initial_values));

static struct itemp_action_context itemp_action_context;

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_rule_event_notification_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(RULE0AVL),
	.ddm_owned_store = &s__ddm_store,
	.ddm_owned_initial_values = s__ddm_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_initial_values),
	.ddm_other_store = &s__ddm_other_store,
	.ddm_other_initial_values = s__ddm_other_initial_values,
	.ddm_other_initial_values_size = ELEMENTS(s__ddm_other_initial_values),
	.actions = s__actions,
	.actions_size = ELEMENTS(s__actions),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.args = &itemp_action_context,
	.name = "Rule Event notification"
};

static bool action_internal_temp(
    struct software_simulator * ss,
    ddm_entry_t * evm_trig,
    void * args)
{
	struct itemp_action_context * context = args;
	uint32_t itemp_val;
	ddm_entry_t * itemp;
	bool has_changed = false;

	itemp = ddm_store__access(software_simulator__other_store(ss), AC0ITEMP);
	itemp_val = ddm_entry__value_i32(itemp);

	switch (context->state)
	{
	case ITEMP_ACTION_LOW_NOTIFY:
		if (itemp_val > 25000)
		{
			context->state = ITEMP_ACTION_HIGH_NOTIFY;
			LOG(I, "Triggering event 4444");
			has_changed = ddm_entry__set__value_i32(evm_trig, 4444);
			ddm_entry__set__flags(evm_trig, SOFTWARE_SIMULATOR__BROKER_SET);
		}
		break;
	case ITEMP_ACTION_HIGH_NOTIFY:
		if (itemp_val < 15000)
		{
			context->state = ITEMP_ACTION_LOW_NOTIFY;
			LOG(I, "Triggering event 5555");
			has_changed = ddm_entry__set__value_i32(evm_trig, 5555);
			ddm_entry__set__flags(evm_trig, SOFTWARE_SIMULATOR__BROKER_SET);
		}
		break;
	default:
		break;
	}
	return has_changed;
}