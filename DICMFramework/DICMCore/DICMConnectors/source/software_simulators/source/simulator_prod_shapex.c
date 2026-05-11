/**
 * \file
 * \date		2023-10-25
 * \author	  (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief	   Simulator for PROD DDM2 class - variant SHAPEX implementation
 *
 * Implementation of tables to simulate PROD DDM2 class - variant `SHAPEX`
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
#include "simulator_prod_shapex.h"

typedef struct CLIST_T
{
	uint32_t list[1];
} PACKED CLIST_T;

static CLIST_T clist =
{
	.list = {0x01020000}
};

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
{
	{
		.ddm_parameter = PROD0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = PROD0FWVER,
		.value =
		{
			.storage = { .str = "1.0.0" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0HWVER,
		.value =
		{
			.storage = { .str = "1.0.0" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0NAME,
		.value =
		{
			.storage = { .str = "product name" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0SN,
		.value =
		{
			. storage = { .str = "product sn" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0SKU,
		.value =
		{
			.storage = { .str = "product sku" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0PNC,
		.value =
		{
			.storage = { .str = "product pnc" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0MDL,
		.value =
		{
			.storage = { .str = "product mdl" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0EAN,
		.value =
		{
			.storage = { .str = "product ean" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0DESCRIPTION,
		.value =
		{
			.storage = { .str = "product description" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = PROD0CLIST,
		.value =
		{
			.storage = { .structure = &clist },
			.size = sizeof(clist),
			.type = DDM2_TYPE_STRUCT
		}
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_prod_shapex_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(PROD0AVL),
	.ddm_owned_store = &s__ddm_owned_store,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "SHAPEX"
};
