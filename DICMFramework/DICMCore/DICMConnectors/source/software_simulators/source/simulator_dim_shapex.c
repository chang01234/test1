/**
 * \file
 * \date		2023-10-25
 * \author	  (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief	   Simulator for DIM DDM2 class - variant SHAPEX implementation
 *
 * Implementation of tables to simulate DIM DDM2 class - variant `SHAPEX`
 *
 * \li		  2022-10-25  (NR) Initial implementation
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

#include "ddm_store.h"
#include "ddm2_parameter_list.h"
#include "esp_attr.h" // EXT_RAM_ATTR
#include "ddm2.h" // ELEMENTS
#include "software_simulator.h"
#include "software_simulator_defaults.h"
#include "simulator_dim_shapex.h"

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
{
	{
		.ddm_parameter = DIM0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = DIM0CMD,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = DIM0DD,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = DIM0LVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = DIM0RGB,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = DIM0OC,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = DIM0LSTAT,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = DIM0NAME,
		.value =
		{
			.storage = { .str = "dim" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = DIM0TYPE,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store_a, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_dim_a_shapex_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(DIM0AVL),
	.ddm_owned_store = &s__ddm_owned_store_a,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "SHAPEX_A"
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store_b, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_dim_b_shapex_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(DIM0AVL),
	.ddm_owned_store = &s__ddm_owned_store_b,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "SHAPEX_B"
};