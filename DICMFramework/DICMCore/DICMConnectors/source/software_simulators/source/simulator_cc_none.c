/**
 * \file
 * \date        2022-07-05
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for CC DDM2 class - variant `none` implementation
 *
 * Implementation of tables to simulate CC DDM2 class - variant `none`
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

#include "ddm_store.h"
#include "ddm2_parameter_list.h"
#include "esp_attr.h" // EXT_RAM_ATTR
#include "ddm2.h" // ELEMENTS
#include "software_simulator.h"
#include "software_simulator_defaults.h"
#include "simulator_cc_none.h"

static const struct ddm_store_ddm s__ddm_initial_values[] =
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

DDM_STORE__DECLARE_EXTRAM(s__ddm_store, ELEMENTS(s__ddm_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_cc_none_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(CC0AVL),
	.ddm_owned_store = &s__ddm_store,
	.ddm_owned_initial_values = s__ddm_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "none"
};
