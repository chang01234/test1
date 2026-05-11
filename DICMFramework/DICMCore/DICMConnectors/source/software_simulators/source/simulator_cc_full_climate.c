/**
 * \file
 * \date        2022-07-05
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for CC DDM2 class - variant `full climate` implementation
 *
 * Implementation of tables to simulate CC DDM2 class - variant `full climate`
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
#include "simulator_cc_full_climate.h"
#include "full_climate_state.h"

#include <string.h>
#include <stdlib.h>

#define AMBIENT_TEMPERATURE_RANDOM_RANGE		100
#define AMBIENT_TEMPERATURE_VENTILATE_RANGE		500
#define AMBIENT_TEMPERATURE_LEAK_STEP			50

struct internal_variables
{
	int32_t set_temp;
    int32_t internal_temp;
    bool is_htr_available;
    bool is_ac_available;
    bool is_iv_available;
};

static struct internal_variables s__internal_variables;

static bool action__cc_temp(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	void * g_args)
{
	int32_t itemp;

	if (s__internal_variables.is_ac_available && s__internal_variables.is_htr_available)
	{
		ddm_entry_t * ddm_htr_temp;
		ddm_entry_t * ddm_ac_itemp;
		int32_t ac_itemp;
		int32_t htr_temp;

		ddm_htr_temp = ddm_store__access(software_simulator__other_store(ss), HTR0TEMP);
		ddm_ac_itemp = ddm_store__access(software_simulator__other_store(ss), AC0ITEMP);
		ac_itemp = ddm_entry__value_i32(ddm_ac_itemp);
		htr_temp = ddm_entry__value_i32(ddm_htr_temp);
		itemp = (ac_itemp + htr_temp) / 2;

	}
	else if (s__internal_variables.is_ac_available)
	{
		ddm_entry_t * ddm_ac_itemp;

		ddm_ac_itemp = ddm_store__access(software_simulator__other_store(ss), AC0ITEMP);
		itemp = ddm_entry__value_i32(ddm_ac_itemp);
	}
	else if (s__internal_variables.is_htr_available)
	{
		ddm_entry_t * ddm_htr_temp;

		ddm_htr_temp = ddm_store__access(software_simulator__other_store(ss), HTR0TEMP);
		itemp = ddm_entry__value_i32(ddm_htr_temp);
	}
	else
	{
		itemp = 0;
	}
	s__internal_variables.internal_temp = itemp;
	return ddm_entry__set__value_i32(entry, s__internal_variables.internal_temp);
}

static bool action__cc_devices(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	void * g_args)
{
	/* HTR/AC/IV */
	static const char * devices[] =
	{
		[0x0] = "NONE",
		[0x1] = "IV",
		[0x2] = "AC",
		[0x3] = "AC/IV",
		[0x4] = "HTR",
		[0x5] = "HTR/IV",
		[0x6] = "HTR/AC",
		[0x7] = "HTR/AC/IV"
 	};
	ddm_entry_t * ddm_htr_avl;
	ddm_entry_t * ddm_ac_avl;
	ddm_entry_t * ddm_iv_avl;
	uint32_t index;

	ddm_htr_avl = ddm_store__access(software_simulator__other_store(ss), HTR0AVL);
	ddm_ac_avl = ddm_store__access(software_simulator__other_store(ss), AC0AVL);
	ddm_iv_avl = ddm_store__access(software_simulator__other_store(ss), IV0AVL);

	s__internal_variables.is_htr_available = ddm_entry__value_i32(ddm_htr_avl) == 1 ? true : false;
	s__internal_variables.is_ac_available = ddm_entry__value_i32(ddm_ac_avl) == 1 ? true : false;
	s__internal_variables.is_iv_available = ddm_entry__value_i32(ddm_iv_avl) == 1 ? true : false;

	index = (s__internal_variables.is_htr_available ? 4 : 0) + (s__internal_variables.is_ac_available ? 2 : 0) + (s__internal_variables.is_iv_available ? 1 : 0);
	return ddm_entry__set__value_str(entry, devices[index], strlen(devices[index]));
}

static bool action__cc_settemp(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	void * g_args)
{
	if (s__internal_variables.is_ac_available)
	{
		ddm_entry_t ac_ttemp;

		ddm_entry__init(&ac_ttemp, AC0TTEMP);
		ddm_entry__copy__value(&ac_ttemp, entry);
		software_simulator__broker_set(ss, &ac_ttemp);
		ddm_entry__terminate(&ac_ttemp);
	}
	if (s__internal_variables.is_htr_available)
	{
		ddm_entry_t htr_atemp;

		ddm_entry__init(&htr_atemp, HTR0ATEMP);
		ddm_entry__copy__value(&htr_atemp, entry);
		software_simulator__broker_set(ss, &htr_atemp);
		ddm_entry__terminate(&htr_atemp);
	}
	s__internal_variables.set_temp = ddm_entry__value_i32(entry);
	return false;
}

static bool action__cc_sts(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	void * g_args)
{
	if (ddm_entry__value_i32(ddm_store__access(software_simulator__owned_store(ss), CC0ACT)) == 1)
	{
		if (s__internal_variables.set_temp > (s__internal_variables.internal_temp + AMBIENT_TEMPERATURE_VENTILATE_RANGE))
		{
			return ddm_entry__set__value_i32(entry, CC0STS_HEAT);
		}
		else if (s__internal_variables.set_temp < (s__internal_variables.internal_temp - AMBIENT_TEMPERATURE_VENTILATE_RANGE))
		{
			return ddm_entry__set__value_i32(entry, CC0STS_COOL);
		}
		else
		{
			return ddm_entry__set__value_i32(entry, CC0STS_VENT);
		}
	}
	else
	{
		return ddm_entry__set__value_i32(entry, CC0STS_IDLE);
	}
}

static bool handler__set_device_mode(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	void * g_args)
{
	ddm_entry_t ac_on;
	ddm_entry_t ac_md;
	ddm_entry_t iv_mode;
	ddm_entry_t htr_amd;
	ddm_entry_t htr_aon;

	ddm_entry__init(&ac_on, AC0ON);
	ddm_entry__init(&ac_md, AC0MD);
	ddm_entry__init(&iv_mode, IV0MODE);
	ddm_entry__init(&htr_amd, HTR0AMD);
	ddm_entry__init(&htr_aon, HTR0AON);

	switch (ddm_entry__value_i32(ddm_store__access(software_simulator__owned_store(ss), CC0STS)))
	{
	case CC0STS_HEAT:
		ddm_entry__set__value_i32(&ac_md, AC0MD_HEAT);
		ddm_entry__set__value_i32(&ac_on, 1);
		ddm_entry__set__value_i32(&iv_mode, IV0MODE_AUTO);
		ddm_entry__set__value_i32(&htr_amd, HTR0AMD_AUTO);
		ddm_entry__set__value_i32(&htr_aon, 1);
		break;
	case CC0STS_COOL:
		ddm_entry__set__value_i32(&ac_md, AC0MD_COOL);
		ddm_entry__set__value_i32(&ac_on, 1);
		ddm_entry__set__value_i32(&iv_mode, IV0MODE_AUTO);
		ddm_entry__set__value_i32(&htr_aon, 0);
		break;
	case CC0STS_VENT:
		ddm_entry__set__value_i32(&ac_md, AC0MD_FAN);
		ddm_entry__set__value_i32(&ac_on, 1);
		ddm_entry__set__value_i32(&iv_mode, IV0MODE_AUTO);
		ddm_entry__set__value_i32(&htr_amd, HTR0AMD_VENTILATION);
		ddm_entry__set__value_i32(&htr_aon, 1);
		break;
	default:
		ddm_entry__set__value_i32(&ac_on, 0);
		ddm_entry__set__value_i32(&iv_mode, IV0MODE_OFF);
		ddm_entry__set__value_i32(&htr_aon, 0);
		break;
	}
	software_simulator__broker_set(ss, &ac_md);
	software_simulator__broker_set(ss, &ac_on);
	software_simulator__broker_set(ss, &iv_mode);
	software_simulator__broker_set(ss, &htr_amd);
	software_simulator__broker_set(ss, &htr_aon);

	ddm_entry__terminate(&htr_aon);
	ddm_entry__terminate(&htr_amd);
	ddm_entry__terminate(&iv_mode);
	ddm_entry__terminate(&ac_md);
	ddm_entry__terminate(&ac_on);

	return false;
}

static void generator__ambient_temperature(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS l_args,
	void * g_args)
{
	static bool is_initialized;

	if (is_initialized == false)
	{
		is_initialized = true;
		srand(10);
	}
	/* Update ambient temperature every 3 seconds */
	if (software_simulator__current_cycle_match(ss, 3000))
	{
		FULL_CLIMATE_STATE * state = g_args;

		state->ambient_temperature += state->ac_temp_increment +
			state->htr_temp_increment +
			state->iv_temp_increment;

		/* Add leaking of temperature to/from outside temperature */
		if (state->ambient_temperature > FULL_CLIMATE_STATE__OUTSIDE_TEMPERATURE)
		{
			state->ambient_temperature -= AMBIENT_TEMPERATURE_LEAK_STEP;
		}
		else if (state->ambient_temperature < FULL_CLIMATE_STATE__OUTSIDE_TEMPERATURE)
		{
			state->ambient_temperature += AMBIENT_TEMPERATURE_LEAK_STEP;
		}
		/* Add some randomness */
		state->ambient_temperature += (rand() % AMBIENT_TEMPERATURE_RANDOM_RANGE) - (AMBIENT_TEMPERATURE_RANDOM_RANGE / 2);
	}
}

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
{
	{
		.ddm_parameter = CC0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0NAME,
		.value =
		{
			.storage = { .str = "NONE" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = CC0ADD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0DEL,
		.value =
		{
			.type = DDM2_TYPE_VOID
		}
	},
	{
		.ddm_parameter = CC0ACT,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0DEVICES,
		.value =
		{
			.storage = { .str = "NONE" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = CC0SETTEMP,
		.value =
		{
			.storage = { .i32 = 21 * 1000 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0SETHUMID,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0STS,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0PCY,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0TEMP,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0VOC,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0HUMID,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = CC0DELTAP,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
};

static const struct ddm_store_ddm s__ddm_other[] =
{
	{
		.ddm_parameter = HTR0AVL,
	},
	{
		.ddm_parameter = HTR0TEMP,
	},
	{
		.ddm_parameter = AC0AVL,
	},
	{
		.ddm_parameter = AC0ITEMP,
	},
	{
		.ddm_parameter = IV0AVL,
	}
};

static const SOFTWARE_SIMULATOR__ACTION_ENTRY s__actions[] =
{
	{
		.ddm_parameter = CC0DEVICES,
		.trigger = { .any = {HTR0AVL, AC0AVL, IV0AVL}},
		.handler = action__cc_devices
	},
	{
		.ddm_parameter = CC0TEMP,
		.trigger = { .any = {HTR0TEMP, AC0ITEMP}},
		.handler = action__cc_temp
	},
	{
		.ddm_parameter = CC0SETTEMP,
		.trigger = { .any = {CC0SETTEMP}},
		.handler = action__cc_settemp,
	},
	{
		.ddm_parameter = CC0STS,
		.trigger = { .any = {CC0SETTEMP, HTR0TEMP, AC0ITEMP, CC0ACT}},
		.handler = action__cc_sts,
	},
	{
		.trigger = { .any = {CC0STS}},
		.handler = handler__set_device_mode
	},
};

static const SOFTWARE_SIMULATOR__GENERATOR_ENTRY s__generators[] =
{
	{
		.ddm_parameter = CC0AVL,
		.handler = generator__ambient_temperature,
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));
DDM_STORE__DECLARE_EXTRAM(s__ddm_other_store, ELEMENTS(s__ddm_other));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_cc_full_climate_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(CC0AVL),
	.ddm_owned_store = &s__ddm_owned_store,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.ddm_other_store = &s__ddm_other_store,
	.ddm_other_initial_values = s__ddm_other,
	.ddm_other_initial_values_size = ELEMENTS(s__ddm_other),
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
