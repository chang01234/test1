/*
 * connector_ddm_log.c
 */
/**
 * \brief    DDM Event Log Service
 *
 * Time stamped logging of any DDM parameter.
 *
 * \{
 */

#include "configuration.h"

#include "freertos/task.h"
#include "connector_ddm_log.h"
#include "ddm_store.h"
#include "ddm_entry.h"
#include "ddm_log.h"

/**
 * \brief    Worker thread stack size
 */
#define DDM_LOG_WORKER_STACK_SIZE   4096

/**
 * \brief    Worker thread priority
 */
#define DDM_LOG_WORKER_PRIORITY     xTASK_PRIORITY_NORMAL

/**
 * \brief    Maximum number of parameters in local DDM database
 */
#ifdef CONNECTOR_DDM_LOG_NO_OWNED_PARAMETERS
#define NBR_OF_OWNED_PARAMETERS     CONNECTOR_DDM_LOG_NO_OWNED_PARAMETERS
#else
#define NBR_OF_OWNED_PARAMETERS		50
#endif

static int initialize_connector(void);

CONNECTOR connector_ddm_log =
{
	.name = "DDM Log connector",
	.initialize = initialize_connector
};

static struct ddm_log s__ddm_log;

DDM_STORE__DECLARE_EXTRAM(s__ddm_log_subscribed__ddm_store, DDM_LOG__SPEC_MAX);
DDM_STORE__DECLARE_EXTRAM(s__ddm_log_owned__ddm_store, NBR_OF_OWNED_PARAMETERS);

/* Flash allocations */
static const struct ddm_store_ddm s__ddm_log__initial_owned_values[] =
{
	{
		.ddm_parameter = LS0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = LS0ADD,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = LS0READ,
		.value =
		{
			.storage = { .str = "" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = LS0LSCFG,
		.value =
		{
			.storage = { .str = "" },//{ .u32 = ~0 },
			.type = DDM2_TYPE_STRING//DDM2_TYPE_UINT32_T
		}
	},
	{
		.ddm_parameter = LS0RAMSIZE,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	}
#if 0
	,
	{
		.ddm_parameter = LSCFG0AVL,
		.value =
		{
			.storage = { .i32 = 1 },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = LSCFG0DDM,
		.value =
		{
			.storage = { .str = "" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = LSCFG0STOR,
		.value =
		{
			.storage = { .str = "" },
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = LSCFG0RET,
		.value =
		{
			.storage = { .str = "" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = LSCFG0INT,
		.value =
		{
			.storage = { .str = "" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = LSCFG0RULE,
		.value =
		{
			.storage = { .str = "" },
			.type = DDM2_TYPE_STRING
		}
	},
	{
		.ddm_parameter = LSCFG0STS,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = LSCFG0ACT,
		.value =
		{
			.type = DDM2_TYPE_INT32_T
		}
	},
	{
		.ddm_parameter = LSCFG0DEL,
		.value =
		{
			.type = DDM2_TYPE_NONE
		}
	}
#endif
};

static const struct ddm_log__descriptor s__ddm_log__descriptor =
{
	.ddm_class = DDM2_PARAMETER_CLASS(LS0),
	.connector = &connector_ddm_log,
	.ddm_store_owned = &s__ddm_log_owned__ddm_store,
	.ddm_initial_owned_values = s__ddm_log__initial_owned_values,
	.ddm_initial_owned_values_size = ELEMENTS(s__ddm_log__initial_owned_values),
	.ddm_store_subscribed = &s__ddm_log_subscribed__ddm_store,
	.worker_priority = DDM_LOG_WORKER_PRIORITY,
	.worker_stack_size = DDM_LOG_WORKER_STACK_SIZE,
};

static int initialize_connector(void)
{
	return ddm_log__init(&s__ddm_log, &s__ddm_log__descriptor);
}

/** \} */
