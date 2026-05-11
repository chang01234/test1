/**
 * \file
 * \date	 	2024-01-21
 * \author	   	(NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief	   	Simulator for AC DDM2 class - variant `Event notification` implementation
 *
 * Implementation of tables to simulate AC DDM2 class - variant `Event notification`
 *
 * \li		  	2022-07-05  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *			  	This source file and the information contained in it are
 *			  	confidential and proprietary to Dometic Group
 *			  	The reproduction or disclosure, in whole or in part,
 *			  	to anyone outside of Dometic Group without the written
 *			 	approval of a Dometic Group officer under a Non-Disclosure
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
#include "simulator_ac_event_notification.h"

typedef struct AC_ERROR_T
{
	uint32_t error[2];
} PACKED AC_ERROR_T;

typedef struct event_notification_context
{
	bool is_itemp_raising;
} event_notification_context_t;

static void generator__itemp(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS l_args,
	void * g_args);

	static void generator__etemp(
		SOFTWARE_SIMULATOR * ss,
		ddm_entry_t * entry,
		SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS l_args,
		void * g_args);

static AC_ERROR_T error =
{
	.error = {0}
};

static const struct ddm_store_ddm s__ddm_initial_values[] =
{
	{
		.ddm_parameter = AC0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0MD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0FS,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0FSPD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0PWR,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0FMD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0LGT,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0DMR,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0MDL,
		.value =
		{
			.storage = { .i32 = AC0MDL_DOMETIC_FJX4000_SERIES },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0LGTBS,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0ELGT,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0PUR,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0ERRC,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
#ifdef AC0ALTC
	{
		.ddm_parameter = AC0ALTC,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
#endif
	{
		.ddm_parameter = AC0ON,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0TTEMP,
		.value =
		{
			.storage = { .i32 = 20000 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0ITEMP,
		.value =
		{
			.storage = { .i32 = 20000 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0FMD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0TONA,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0TOFFA,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0TONH,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0TOFFH,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0TONM,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0TOFFM,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0SYSU,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0SLEEP,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0HFAVL,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0LFAVL,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = AC0STATUS,
		.value =
		{
			.storage = { .structure = &error },
			.size = sizeof(error),
			.type = DDM2_TYPE_STRUCT
		}
	},
	{
		.ddm_parameter = AC0VER,
		.value =
		{
			.storage = { .str = "1.0.0" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = AC0OFFSET,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},	
	{
		.ddm_parameter = AC0ECO,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
#ifdef AC0FEATURE
    {
        .ddm_parameter = AC0FEATURE,
        .value =
        {
            .storage = { .u32 = 0 },
            .type = DDM2_TYPE_UINT32_T
        }
    },
#endif
    {
        .ddm_parameter = AC0ACTEXT,
        .value =
        {
            .storage = { .u32 = 0 },
            .type = DDM2_TYPE_UINT32_T
        }
    },
#ifdef AC0ETEMP
	{
		.ddm_parameter = AC0ETEMP,
		.value =
		{
			.storage = { .i32 = 20000 },
			.type = DDM2_TYPE_INT32_T
		}
	},
#endif
#ifdef AC0HTTEMP
    {
        .ddm_parameter = AC0HTTEMP,
        .value =
        {
            .storage = { .i32 = 20000 },
            .type = DDM2_TYPE_INT32_T
        }
    },
#endif
#ifdef AC0FILTER
    {
        .ddm_parameter = AC0FILTER,
        .value =
        {
            .storage = { .i32 = 3 },
            .type = DDM2_TYPE_INT32_T
        }
    },
#endif
};

static const SOFTWARE_SIMULATOR__GENERATOR_ENTRY s__generators[] =
{
	{
		.ddm_parameter = AC0ITEMP,
		.handler = generator__itemp
	},
	{
		.ddm_parameter = AC0ETEMP,
		.handler = generator__etemp
	}
};

static event_notification_context_t event_notification_context;

DDM_STORE__DECLARE_EXTRAM(s__ddm_store, ELEMENTS(s__ddm_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_ac_event_notification_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(AC0AVL),
	.ddm_owned_store = &s__ddm_store,
	.ddm_owned_initial_values = s__ddm_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_initial_values),
	.generators = s__generators,
	.generators_size = ELEMENTS(s__generators),
	.args = &event_notification_context,
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "event notification"
};

static void generator__itemp(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * itemp,
	SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS l_args,
	void * g_args)
{
	static bool is_initialized;
	event_notification_context_t * state = g_args;

	if (is_initialized == false)
	{
		is_initialized = true;
		state->is_itemp_raising = true;
	}
	/* Update ambient temperature every second */
	if (software_simulator__current_cycle_match(ss, 2000))
	{
		int32_t itemp_val;

		itemp_val = ddm_entry__value_i32(itemp);
		if (state->is_itemp_raising)
		{
			itemp_val += 1000;
		}
		else
		{
			itemp_val -= 1000;
		}
		if (itemp_val > 30000)
		{
			state->is_itemp_raising = false;
		}
		else if (itemp_val < 10000)
		{
			state->is_itemp_raising = true;
		}
		bool has_changed = ddm_entry__set__value_i32(itemp, itemp_val);
		ddm_entry__set__has_changed(itemp, has_changed);
	}
}

static void generator__etemp(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * etemp,
	SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS l_args,
	void * g_args)
{
	/* Update external temperature every from internal temperature every 3 seconds */
	if (software_simulator__current_cycle_match(ss, 5000))
	{
		ddm_entry_t * itemp = ddm_store__access(software_simulator__owned_store(ss), AC0ITEMP);
		int32_t itemp_val = ddm_entry__value_i32(itemp);
		
		bool has_changed = ddm_entry__set__value_i32(etemp, itemp_val);
		ddm_entry__set__has_changed(etemp, has_changed);
	}
}
