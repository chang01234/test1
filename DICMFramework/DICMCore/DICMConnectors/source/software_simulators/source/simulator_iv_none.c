/**
 * \file
 * \date        2022-07-06
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for IV DDM2 class - variant `none` implementation
 *
 * Implementation of tables to simulate IV DDM2 class - variant `none`
 *
 * \li          2022-07-06  (NR) Initial implementation
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

#if defined(CONNECTOR_SIMULATOR_INVENTILATE)

#include "simulator_iv_none.h"

static const struct ddm_store_ddm s__ddm_initial_values[] =
{
    {
        .ddm_parameter = IV0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
    },
	{
		.ddm_parameter = IV0MODE,
		.value =
		{
			.storage = { .i32 = IV0MODE_AUTO },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = IV0PWRON,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = IV0FILST,
		.value =
		{
			.storage = { .i32 = IV0FILST_FILTER_CHANGE_NOT_REQ },
			.type = DDM2_TYPE_INT32_T
		}
	},
//	{
//		.ddm_parameter = IV0VEHMD,
//		.value =
//		{
//			.storage = { .i32 = IV0VEHMD_ACTIVE_MODE },
//			.type = DDM2_TYPE_INT32_T
//		}
//	},
	{
		.ddm_parameter = IV0ERRST,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = IV0WARN,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
//	{
//		.ddm_parameter = IV0ERRCD1,
//		.value =
//		{
//			.type = DDM2_TYPE_INT32_T
//		}
//	},
//	{
//		.ddm_parameter = IV0ERRCD2,
//		.value =
//		{
//			.type = DDM2_TYPE_INT32_T
//		}
//	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_store, ELEMENTS(s__ddm_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_iv_none_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(IV0AVL),
	.ddm_owned_store = &s__ddm_store,
	.ddm_owned_initial_values = s__ddm_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
    .name = "none"
};

#endif /*CONNECTOR_SIMULATOR_INVENTILATE*/
