/**
 * \file
 * \date        2024-12-24
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for RFAN DDM2 class - variant `None` implementation
 *
 * Implementation of tables to simulate RFAN DDM2 class - variant `None`
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
#include "simulator_rfan_none.h"

static const struct ddm_store_ddm s__ddm_owned_initial_values[] =
{
    {
        .ddm_parameter = RFAN0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
    },

	{
        .ddm_parameter = RFAN0SYST,
		.value =
		{
			.storage = { .i32 = RFAN0SYST_OFF },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0FM,
		.value =
		{
			.storage = { .i32 = RFAN0FM_AUTO },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SPDMD,
		.value =
		{
			.storage = { .i32 = RFAN0SPDMD_MANUAL },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0LIGHT,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0FSPDSET,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0WINDDRSW,
		.value =
		{
			.storage = { .i32 = RFAN0WINDDRSW_AIR_OUT },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0DOMEPOS,
		.value =
		{
			.storage = { .i32 = RFAN0DOMEPOS_CLOSED },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0RAINSNS,
		.value =
		{
			.storage = { .i32 = RFAN0RAINSNSSTS_NO_RAIN_DETECTED },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AMBTEMP,
		.value =
		{
			.storage = { .i32 = 20000 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SETPT,
		.value =
		{
			.storage = { .i32 = 22000 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0DMMODE,
		.value =
		{
			.storage = { .i32 = RFAN0DMMODE_STOP },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0DSRDDMPOS,
		.value =
		{
			.storage = { .i32 = 69 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SETPCTLDM,
		.value =
		{
			.storage = { .i32 = RFAN0SETPCTLDM_NO_AUTOMATIC_DOME_CONTROL },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0DMCLSFOFF,
		.value =
		{
			.storage = { .i32 = RFAN0DMCLSFOFF_AUTO_DOME_CLOSE_ON_FAN_OFF },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0FOFFDMCLS,
		.value =
		{
			.storage = { .i32 = RFAN0FOFFDMCLS_AUTO_FAN_OFF_ON_DOME_CLOSE },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0FSPDINCDEC,
		.value =
		{
			.storage = { .i32 = RFAN0FSPDINCDEC_DECREMENT_FAN_SPEED },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0FSPDIDSTP,
		.value =
		{
			.storage = { .i32 = 33 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0FSPDSTPSUP,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0RAINSNSSTS,
		.value =
		{
			.storage = { .i32 = RFAN0RAINSNSSTS_NO_RAIN_DETECTED },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0MTNSNSEN,
		.value =
		{
			.storage = { .i32 = 0 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0MTNSNSTIM,
		.value =
		{
			.storage = { .i32 = 10 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0LGTCMD,
		.value =
		{
			.storage = { .i32 = DIM0CMD_OFF },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0LGTDD,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0LGTLVL,
		.value =
		{
			.storage = { .i32 = 11 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0LGTRGB,
		.value =
		{
			.storage = { .i32 = 5 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0LGTOC,
		.value =
		{
			.storage = { .i32 = CURRENTSTATUS_NORMAL },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0LGTLSTAT,
		.value =
		{
			.storage = { .i32 = DIM0LSTAT_ON },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0CSCH,
		.value =
		{
			.storage = { .i32 = 32 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0NSCH,
		.value =
		{
			.storage = { .i32 = 31 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SLEEPTIMEH,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SLEEPTIMEM,
		.value =
		{
			.storage = { .i32 = 2 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SLEEPFMODE,
		.value =
		{
			.storage = { .i32 = RFAN0SLEEPFMODE_FAN_MANUAL },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SLEEPFDIR,
		.value =
		{
			.storage = { .i32 = RFAN0WINDDRSW_AIR_IN },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0SLEEPFSETP,
		.value =
		{
			.storage = { .i32 = 2 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAKETIMEH,
		.value =
		{
			.storage = { .i32 = 4 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAKETIMEM,
		.value =
		{
			.storage = { .i32 = 5 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAKEFMODE,
		.value =
		{
			.storage = { .i32 = RFAN0SLEEPFMODE_FAN_OFF },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAKEFDIR,
		.value =
		{
			.storage = { .i32 = RFAN0WINDDRSW_AIR_OUT },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAKEFSETP,
		.value =
		{
			.storage = { .i32 = 7 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAYTIMEH,
		.value =
		{
			.storage = { .i32 = 8 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAYTIMEM,
		.value =
		{
			.storage = { .i32 = 9 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAYFMODE,
		.value =
		{
			.storage = { .i32 = RFAN0SLEEPFMODE_FAN_AUTO },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAYFDIR,
		.value =
		{
			.storage = { .i32 = RFAN0WINDDRSW_AIR_IN },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0AWAYFSETP,
		.value =
		{
			.storage = { .i32 = 22 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0ERR,
		.value =
		{
			.storage = { .i32 = 11 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
        .ddm_parameter = RFAN0ALARM,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_rfan_none_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(RFAN0AVL),
	.ddm_owned_store = &s__ddm_owned_store,
	.ddm_owned_initial_values = s__ddm_owned_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "None"
};
