/**
 * \file
 * \date        2022-07-05
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for HTR DDM2 class - variant `none` implementation
 *
 * Implementation of tables to simulate HTR DDM2 class - variant `none`
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
#include "simulator_htr_none.h"

static const struct ddm_store_ddm s__ddm_initial_values[] =
{
	{
		.ddm_parameter = HTR0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0ESEL,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0AON,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0WTRON,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0EL,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0GAS,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0AMD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0MDL,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 2 },
		}
	},
	{
		.ddm_parameter = HTR0ATEMP,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 18000 },
		}
	},
	{
		.ddm_parameter = HTR0SMAXFAN,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 4 },
		}
	},
	{
		.ddm_parameter = HTR0VMINFAN,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 3 },
		}
	},
	{
		.ddm_parameter = HTR0TEMP,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
		}
	},
	{
		.ddm_parameter = HTR0EL,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
		}
	},
	{
		.ddm_parameter = HTR0WTRTEMP,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0UVTH,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 11400 }, // Range: 10000-12500
		}
	},
	{
		.ddm_parameter = HTR0SYSU,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 0 } // Metric, Celsius
			//.storage = { .i32 = 1 } // Imperial, Fahrenheit
		}
	},
	{
		.ddm_parameter = HTR0CVER,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0CVER,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0BVER,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0BVER,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0PCBA,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0PROT,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0ERRST,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
#if 0
	{
		.ddm_parameter = HTR0ERRCD1,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 0 },
		}
	},
	{
		.ddm_parameter = HTR0ERRCD2,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 0 },
		}
	},
	{
		.ddm_parameter = HTR0ERRCD3,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 0 },
		}
	},
	{
		.ddm_parameter = HTR0ERRCD4,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 0 },
		}
	},
#endif
	{
		.ddm_parameter = HTR0ACST,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0GASWTRHST,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0ACWTRHST,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0RTS,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 25000 },
		}
	},
	{
		.ddm_parameter = HTR0WTRTS,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0DATEY,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0DATEM,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0DATED,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0TIMEH,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0TIMEM,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0TIMES,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0TTZ,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0AHTOFFST,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0AHTOFFH,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0AHTOFFM,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0AHTONST,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0AHTONH,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0AHTONM,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0WTRTST,
		.value =
		{
			.type = DDM2_TYPE_INT32_T,
			.storage = { .i32 = 1 },
		}
	},
	{
		.ddm_parameter = HTR0WTRTONH,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0WTRTONM,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0WTRTKET,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = HTR0WEEKD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_store, ELEMENTS(s__ddm_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_htr_none_descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(HTR0AVL),
	.ddm_owned_store = &s__ddm_store,
	.ddm_owned_initial_values = s__ddm_initial_values,
	.ddm_owned_initial_values_size = ELEMENTS(s__ddm_initial_values),
	.worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
	.worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
	.name = "none"
};
