/**
 * \file
 * \date        2022-07-05
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for AC DDM2 class - variant `full class` implementation
 *
 * Implementation of tables to simulate AC DDM2 class - variant `full climate`
 *
 * \li          2022-07-05  (NR) Initial implementation
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
#if defined(SOFTWARE_SIMULATOR_FULL_CLIMATE)

#include "ddm_store.h"
#include "ddm2_parameter_list.h"
#include "esp_attr.h" // EXT_RAM_ATTR
#include "ddm2.h" // ELEMENTS
#include "software_simulator.h"
#include "software_simulator_defaults.h"
#include "simulator_ac_full_climate.h"
#include "full_climate_state.h"

#define AMBIENT_TEMPERATURE_TURBO_STEP		-400
#define AMBIENT_TEMPERATURE_COOL_STEP		-300
#define AMBIENT_TEMPERATURE_DRY_STEP		-50
#define AMBIENT_TEMPERATURE_FAN_STEP		10
#define AMBIENT_TEMPERATURE_HEAT_STEP		200

static void generator__htr_temp(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS l_args,
	void * args)
{
	/* The AC class device gets its temperature from ambient every second */
	if (software_simulator__current_cycle_match(ss, 1000))
	{
		FULL_CLIMATE_STATE * state = args;
		bool has_changed;

		has_changed = ddm_entry__set__value_i32(entry, state->ambient_temperature - 100);
		ddm_entry__set__has_changed_conditionally(entry, has_changed);
	}
}

static bool action__ambient_temperature(SOFTWARE_SIMULATOR * ss, ddm_entry_t * entry, void * args)
{
	FULL_CLIMATE_STATE * state = args;
	int32_t ac_on;
	int32_t ac_md;
	int32_t ac_itemp;
	int32_t ac_ttemp;

	ac_on = ddm_entry__value_i32(ddm_store__access(software_simulator__owned_store(ss), AC0ON));
	ac_md = ddm_entry__value_i32(ddm_store__access(software_simulator__owned_store(ss), AC0MD));
	ac_itemp = ddm_entry__value_i32(ddm_store__access(software_simulator__owned_store(ss), AC0ITEMP));
	ac_ttemp = ddm_entry__value_i32(ddm_store__access(software_simulator__owned_store(ss), AC0TTEMP));

	if (ac_on == 0)
	{
		state->ac_temp_increment = 0;
	}
	else
	{
		state->ac_temp_increment = 0;

		switch (ac_md)
		{
		case AC0MD_TURBO:
			if (ac_itemp > ac_ttemp)
			{
				state->ac_temp_increment = AMBIENT_TEMPERATURE_TURBO_STEP;
			}
			break;
		case AC0MD_COOL:
			if (ac_itemp > ac_ttemp)
			{
				state->ac_temp_increment = AMBIENT_TEMPERATURE_COOL_STEP;
			}
			break;
		case AC0MD_DRY:
			state->ac_temp_increment = AMBIENT_TEMPERATURE_DRY_STEP;
			break;
		case AC0MD_FAN:
			state->ac_temp_increment = AMBIENT_TEMPERATURE_FAN_STEP;
			break;
		case AC0MD_HEAT:
			if (ac_itemp < ac_ttemp)
			{
				state->ac_temp_increment = AMBIENT_TEMPERATURE_HEAT_STEP;
			}
			break;
		case AC0MD_AUTO:
			if (ac_itemp > ac_ttemp)
			{
				state->ac_temp_increment = AMBIENT_TEMPERATURE_COOL_STEP;
			}
			else if (ac_itemp < ac_ttemp)
			{
				state->ac_temp_increment = AMBIENT_TEMPERATURE_HEAT_STEP;
			}
			break;
		default:
			break;
		}
	}
	return false;
}

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
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
			.storage = { .i32 = AC0MD_COOL },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = AC0FS,
		.value =
		{
			. storage = { .i32 = 20	},
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = AC0FSPD,
		.value =
		{
			.storage = { .i32 = AC0FSPD_MED	},
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = AC0PWR,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = AC0FMD,
		.value =
		{
			.storage = { .i32 = AC0FMD_ON },
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
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = AC0TTEMP,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = AC0ITEMP,
		.value =
		{
			.storage = { .i32 = 254 * 100 },
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
};

static const SOFTWARE_SIMULATOR__ACTION_ENTRY s__actions[] =
{
	{
		.trigger = { .any = { AC0ON, AC0MD, AC0ITEMP, AC0TTEMP }},
		.handler = action__ambient_temperature,
	},
};

static const SOFTWARE_SIMULATOR__GENERATOR_ENTRY s__generators[] =
{
	{
		.ddm_parameter = AC0ITEMP,
		.handler = generator__htr_temp
	}
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_ac_full_climate_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(AC0AVL),
	.ddm_owned_store = &s__ddm_owned_store,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.actions = s__actions,
	.actions_size = ELEMENTS(s__actions),
	.generators = s__generators,
	.generators_size = ELEMENTS(s__generators),
	.args = &g__full_climate_state,
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "Full Climate"
};
#endif
