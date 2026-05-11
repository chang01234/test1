/**
 * \file
 * \date        2024-12-24
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for SPIR DDM2 class - variant `None` implementation
 *
 * Implementation of tables to simulate SPIR DDM2 class - variant `None`
 *
 * \li          2024-12-24  (NR) Initial implementation
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
#include "simulator_spir_none.h"

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
{
    {
        .ddm_parameter = SPIR0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
    },

	{
        .ddm_parameter = SPIR0STATUS,
		.value =
		{
			. storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = SPIR0FR,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = SPIR0EVT,
		.value =
		{
			.storage = { .i32 = SPIR0EVT_MOTION_OUT_DETECT },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = SPIR0SENDEVT,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_spir_none_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(SPIR0AVL),
	.ddm_owned_store = &s__ddm_owned_store,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "None"
};
