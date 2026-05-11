/**
 * \file
 * \date        2025-03-25
 * \author      Andreas Lundeen
 * \brief       Simulator for MBAT DDM2 class
 *
 * Implementation of tables to simulate MBAT DDM2 class
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

#include <stdint.h>
#include "configuration.h"
#include "ddm_store.h"
#include "ddm2_parameter_list.h"
#include "esp_attr.h" // EXT_RAM_ATTR
#include "ddm2.h" // ELEMENTS
#include "software_simulator.h"
#include "software_simulator_defaults.h"
#include "simulator_mbat.h"

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
{
    {
        .ddm_parameter = MBAT0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
    },

	{
        .ddm_parameter = MBAT0SOH,
		.value =
		{
			.storage = { .i32 = 100 },
			.type = DDM2_TYPE_INT32_T
		}
	},
    {
        .ddm_parameter = MBAT0SOC,
        .value =
        {
            .storage = { .i32 = 100 },
            .type = DDM2_TYPE_INT32_T
        }
    },
	{
        .ddm_parameter = MBAT0CAPREL,
		.value =
		{
			.storage = { .i32 = 100 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = MBAT0CHGDET,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_mbat_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(MBAT0AVL),
	.ddm_owned_store = &s__ddm_owned_store,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK+1024,
	.name = "MBAT"
};
