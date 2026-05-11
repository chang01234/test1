/**
 * \file
 * \date        2023-10-25
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for MCCC DDM2 class - variant CFX2` implementation
 *
 * Implementation of tables to simulate MCCC DDM2 class - variant `CFX2`
 *
 * \li          2022-10-25  (NR) Initial implementation
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

#include "ddm_store.h"
#include "ddm2_parameter_list.h"
#include "esp_attr.h" // EXT_RAM_ATTR
#include "ddm2.h" // ELEMENTS
#include "software_simulator.h"
#include "software_simulator_defaults.h"
#include "simulator_mccc_cfxx.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define AMBIENT_TEMPERATURE_RANDOM_RANGE 100

/**
 * @brief 		Make a concrete TEMPARR structure with one element inside.
 *
 * In order to be able to statically allocate and initialize TEMPARR structure,
 * we created this structure which has the definition the same as TEMPARR_T but
 * instead of flexible array member we have a real array with 1 member.
 */
typedef struct TEMPARR_CONCRETE_T
{
	int32_t temp[1];
} PACKED TEMPARR_CONCRETE_T;

/**
 * @brief 		Make a concrete TEMPRANGEARR structure with one element inside.
 *
 * In order to be able to statically allocate and initialize TEMPRANGEARR
 * structure, we created this structure which has the definition the same as
 * TEMPRANGEARR_T, but instead of flexible array member we have a real array
 * with 1 member.
 */
typedef struct TEMPRANGEARR_CONCRETE_T
{
	struct TEMP_RANGE temp_range[1];
} PACKED TEMPRANGEARR_CONCRETE_T;

/**
 * @brief 		Make a concrete ERROR structure with one element inside.
 *
 * In order to be able to statically allocate and initialize ERROR_T structure,
 * we created this structure which has the definition the same as ERROR_T, but
 * instead of flexible array member we have a real array with 1 member.
 */
typedef struct ERROR_CONCRETE_T
{
	uint16_t error[1];
} PACKED ERROR_CONCRETE_T;

struct mccc0_global_device_state
{
	int dummy;
};

struct mccc0cpow
{
	bool state;
};

static struct mccc0cpow initial_cpow =
{
	.state = true
};

static TEMPARR_CONCRETE_T initial_ctemp =
{
	.temp = {24000}
};

static TEMPARR_CONCRETE_T initial_csettemp =
{
	.temp = {20000}
};

static TEMPARR_CONCRETE_T initial_ctempofs =
{
	.temp = {1100}
};

struct mccc0cdoor
{
	bool state;
};

static struct mccc0cdoor initial_mccc0cdoor =
{
	.state = true
};

static ERROR_CONCRETE_T initial_error =
{
	.error = {0}
};

static TEMPRANGEARR_CONCRETE_T initial_mccc0ctemprng =
{
	.temp_range =
	{
		{.mintemp = 10000, .maxtemp = 70000}
	}
};

static TEMPRANGEARR_CONCRETE_T initial_mccc0crecdrng =
{
	.temp_range =
	{
		{.mintemp = 20000, .maxtemp = 50000}
	}
};

static void generator__ctemp(
	SOFTWARE_SIMULATOR * ss,
	ddm_entry_t * entry,
	SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS l_args,
	void * args)
{
	/* The AC class device gets its temperature from ambient every second */
	if (software_simulator__current_cycle_match(ss, 1000))
	{
		bool has_changed;
		TEMPARR_CONCRETE_T value;

		value = *(TEMPARR_CONCRETE_T *)ddm_entry__value_struct(entry);
		/* Add some randomness */
		value.temp[0] += (rand() % AMBIENT_TEMPERATURE_RANDOM_RANGE) - (AMBIENT_TEMPERATURE_RANDOM_RANGE / 2);
		has_changed = ddm_entry__set__value_struct(entry, &value, sizeof(value));
		ddm_entry__set__has_changed_conditionally(entry, has_changed);
	}
}

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
{
    {
        .ddm_parameter = MCCC0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
    },
    {
        .ddm_parameter = MCCC0PTYPE,
		.value =
		{
			.storage = { .i32 = CC0PTYPE_UNCONFIGURED_RUBICON_CFX3 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0NOCPT,
		.value =
		{
			. storage = { .i32 = 1	},
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0CPOW,
		.value =
		{
			.storage = { .other = &initial_cpow },
			.size = sizeof(initial_cpow),
			.type = DDM2_TYPE_OTHER
		}
	},
	{
        .ddm_parameter = MCCC0CTEMP,
		.value =
		{
			.storage = { .structure = &initial_ctemp },
			.size = sizeof(initial_ctemp),
			.type = DDM2_TYPE_STRUCT
		}
	},
	{
        .ddm_parameter = MCCC0CSETTEMP,
		.value =
		{
			.storage = { .structure = &initial_csettemp },
			.size = sizeof(initial_csettemp),
			.type = DDM2_TYPE_STRUCT
		}
	},
	{
        .ddm_parameter = MCCC0ACPT,
		.value =
		{
			.storage = { .i32 = CC0ACPT_INACTIVE },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0CDOOR,
		.value =
		{
			.storage = { .other = &initial_mccc0cdoor },
			.size = sizeof(initial_mccc0cdoor),
			.type = DDM2_TYPE_OTHER
		}
	},
	{
    	.ddm_parameter = MCCC0CTEMPRNG,
		.value =
		{
			.storage = { .structure = &initial_mccc0ctemprng },
			.size = sizeof(initial_mccc0ctemprng),
			.type = DDM2_TYPE_STRUCT
		}
	},
	{
        .ddm_parameter = MCCC0CRECDRNG,
		.value =
		{
			.storage = { .structure = &initial_mccc0crecdrng },
			.size = sizeof(initial_mccc0crecdrng),
			.type = DDM2_TYPE_STRUCT
		}
	},
	{
        .ddm_parameter = MCCC0CTEMPOFS,
		.value =
		{
			.storage = { .structure = &initial_ctempofs },
			.size = sizeof(initial_ctempofs),
			.type = DDM2_TYPE_STRUCT
		}
	},
	{
        .ddm_parameter = MCCC0COOLERPOW,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0V,
		.value =
		{
			.storage = { .i32 = 12000 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0BATPROTLVL,
		.value =
		{
			.storage = { .i32 = CC0BATPROTLVL_LOW },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0COMPPOW,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0I,
		.value =
		{
			.storage = { .i32 = 1200 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0POWSRC,
		.value =
		{
			.storage = { .i32 = CC0POWSRC_DC },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0ICEPOW,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MCCC0ERRST,
		.value =
		{
			.storage = { .structure = &initial_error },
			.size = sizeof(initial_error),
			.type = DDM2_TYPE_STRUCT
		}
	},
	{
        .ddm_parameter = MCCC0SN,
		.value =
		{
			.storage = { .str = "demosn" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
        .ddm_parameter = MCCC0SKU,
		.value =
		{
			.storage = { .str = "demosku" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
        .ddm_parameter = MCCC0FWVER,
		.value =
		{
			.storage = { .str = "1.0.0" },
			.type = DDM2_TYPE_STRING
		}
	},
};

static const SOFTWARE_SIMULATOR__GENERATOR_ENTRY s__generators[] =
{
	{
		.ddm_parameter = MCCC0CTEMP,
		.handler = generator__ctemp
	}
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_mccc_cfxx_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(MCCC0AVL),
	.ddm_owned_store = &s__ddm_owned_store,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.generators = s__generators,
	.generators_size = ELEMENTS(s__generators),
	.name = "CFX2"
};
