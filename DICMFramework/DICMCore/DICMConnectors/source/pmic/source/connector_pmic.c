/*****************************************************************************
 * \file       connector_pmic.c
 * \brief      Connector for battery services PMIC BQ25895
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/
#include <string.h>
#include <time.h>
#include "configuration.h"
#ifdef CONNECTOR_PMIC
#include "connector_pmic.h"
#include "ddm_wrapper.h"
#include "utils.h"
#include "hal_rtc.h"
#include "hal_pmic.h"

/**********************************************************
 * Private defines
 *********************************************************/
#define BATTERY_CLASS_NAME          "pmic"

/**********************************************************
 * Private types
 *********************************************************/
typedef struct
{
	// DDM
	ddmw_item_t avl;   		// Available
	ddmw_item_t batrv;    	// Regulation voltage
	ddmw_item_t chrgc;     	// Charge current
	ddmw_item_t tc;   		// Termination current
	ddmw_item_t pchrgc;   	// Precharge current
	ddmw_item_t msysv;   	// Min system volt limit
	ddmw_item_t bstrv; 		// Boost reg volt
	ddmw_item_t thrgth; 	// Thermal regulation thres
	ddmw_item_t bstat;  	// Battery status
	ddmw_item_t chty;  		// Charge type
	ddmw_item_t mid;  		// Manufacturer Id
	ddmw_item_t bmdl;  		// Battery model
	ddmw_item_t online; 	// Online
	ddmw_item_t bhlt;  		// Battery health
} pmic_par_t;

typedef struct
{
	// DDM wrapper
	ddmw_t ddm;
	pmic_par_t items;
} connector_pmic_t;

static EXT_RAM_ATTR connector_pmic_t pmic_management;

/**********************************************************
 * Private functions
 *********************************************************/
static int		connector_pmic_init(void);
static void     connector_pmic_task(uint16_t elapsed);
static void     connector_pmic_register(pmic_par_t* battery);
static void 	connector_pmic_pval_1s(void);
static void 	connector_pmic_pset_subtask(void);

/**********************************************************
 * Private variables
 *********************************************************/

static char manufacturer_id [18] = "UNKNOWN";
static char bat_model [8] = "UNKNOWN";

/**********************************************************
 * Public variables
 *********************************************************/
CONNECTOR connector_pmic =
{
	.name       = "PMIC connector",
	.initialize = connector_pmic_init,
	.task_run   = connector_pmic_task
};

/**********************************************************
 * Implementation
 *********************************************************/

/**********************************************************
 * Function:    connector_pmic_init
 * Description: Initialize the connector
 *********************************************************/
static int connector_pmic_init(void)
{
	// Memory
	memset(&pmic_management, 0, sizeof(pmic_management));

	// DDM
	ddmw_init(&pmic_management.ddm, &connector_pmic);

	// Register to ddm
	connector_pmic_register(&pmic_management.items);

	//Initialize pmic parameters
	ddmw_set_i32(&pmic_management.items.avl, 1);

	//Initialize pmic driver
	hal_pmic_init();

	return 1;
}

/**********************************************************
 * Function:    connector_pmic_task
 * Description: Run connector task
 *********************************************************/
static void connector_pmic_task(uint16_t elapsed)
{
	// Sub-tasks
	static uint16_t timer_1s = 0;
	if (timer16_is_due(&timer_1s, elapsed, 1000))
	{
		connector_pmic_pval_1s();
		connector_pmic_pset_subtask();
	}

	// DDM
	ddmw_process(&pmic_management.ddm);
}

/**********************************************************
 * Function:    connector_pmic_register
 * Description: Register battery instance
 *********************************************************/
static void connector_pmic_register(pmic_par_t* pmic)
{
	// Register new instance to DDM
	int instance = ddmw_register(&pmic_management.ddm, PMIC0AVL);
	
	ddmw_add(&pmic_management.ddm, &pmic->avl,		PMIC0AVL,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->batrv,	PMIC0BATRV,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->chrgc,	PMIC0CHRGC,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->tc,		PMIC0TC,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->pchrgc,	PMIC0PCHRGC,instance);
	ddmw_add(&pmic_management.ddm, &pmic->msysv,	PMIC0MSYSV,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->bstrv, 	PMIC0BSTRV, instance);
	ddmw_add(&pmic_management.ddm, &pmic->thrgth, 	PMIC0THRGTH,instance);
	ddmw_add(&pmic_management.ddm, &pmic->bstat,  	PMIC0BSTAT,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->chty,  	PMIC0CHTY,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->mid,		PMIC0MID,	instance);
	ddmw_add(&pmic_management.ddm, &pmic->bmdl,  	PMIC0BMDL,  instance);
	ddmw_add(&pmic_management.ddm, &pmic->online,	PMIC0ONLINE,instance);
	ddmw_add(&pmic_management.ddm, &pmic->bhlt,  	PMIC0BHLT,  instance);

	// Init
	ddmw_set_i32(&pmic->avl, 0);
}

/**********************************************************
 * Function:    connector_pmic_write_subtask
 * Description: Handle Set and Publish task
 *********************************************************/
static void connector_pmic_pset_subtask (void)
{
	uint32_t data;

	if(ddmw_is_updated(&pmic_management.items.batrv))
	{
		data = ddmw_get_i32(&pmic_management.items.batrv);
		hal_pmic_set(P_BAT_REG_V, &data);
		hal_pmic_get(P_BAT_REG_V, &data);
		ddmw_set_i32(&pmic_management.items.batrv, data);
	}

	if(ddmw_is_updated(&pmic_management.items.chrgc))
	{
		data = ddmw_get_i32(&pmic_management.items.chrgc);
		hal_pmic_set(P_CHARGE_CURRENT, &data);
		hal_pmic_get(P_CHARGE_CURRENT, &data);
		ddmw_set_i32(&pmic_management.items.chrgc, data);
	}

	if(ddmw_is_updated(&pmic_management.items.tc))
	
	{
		data = ddmw_get_i32(&pmic_management.items.tc);
		hal_pmic_set(P_TERMINATION_CURRENT, &data);
		hal_pmic_get(P_TERMINATION_CURRENT, &data);
		ddmw_set_i32(&pmic_management.items.tc, data);
	}

	if(ddmw_is_updated(&pmic_management.items.pchrgc))
	{
		data = ddmw_get_i32(&pmic_management.items.pchrgc);
		hal_pmic_set(P_PRECHARGE_CURRENT, &data);
		hal_pmic_get(P_PRECHARGE_CURRENT, &data);
		ddmw_set_i32(&pmic_management.items.pchrgc, data);
	}

	if(ddmw_is_updated(&pmic_management.items.batrv))
	{
		data = ddmw_get_i32(&pmic_management.items.batrv);
		hal_pmic_set(P_BAT_REG_V, &data);
		hal_pmic_get(P_MIN_SYS_V, &data);
		ddmw_set_i32(&pmic_management.items.msysv, data);
	}

	if(ddmw_is_updated(&pmic_management.items.bstrv))
	{
		data = ddmw_get_i32(&pmic_management.items.bstrv);
		hal_pmic_set(P_BOOST_REG_V, &data);
		hal_pmic_get(P_BOOST_REG_V, &data);
		ddmw_set_i32(&pmic_management.items.bstrv, data);
	}

	if(ddmw_is_updated(&pmic_management.items.thrgth))
	{
		data = ddmw_get_i32(&pmic_management.items.thrgth);
		hal_pmic_set(P_THERMAL_REG, &data);
		hal_pmic_get(P_THERMAL_REG, &data);
		ddmw_set_i32(&pmic_management.items.thrgth, data);
	}

}

/**********************************************************
 * Function:	connector_pmic_pval_1s
 * Description: Check Parameter values
 *********************************************************/
static void connector_pmic_pval_1s(void)
{
	int32_t data = 999999999;
	battery_status_t bat_status = 9;
	charge_type_t chargetype = 9;
	battery_health_t bat_health = 9;
	bool online = FALSE;
	static uint8_t state = 1;

	if (state)
	{
		hal_pmic_get(P_MANUFACTURE_ID, manufacturer_id);
		ddmw_set_str(&pmic_management.items.mid, manufacturer_id);

		hal_pmic_get(P_BAT_MODEL, bat_model);
		ddmw_set_str(&pmic_management.items.bmdl, bat_model);

		state = 0;
	}

	//Battery Voltage and Current
	hal_pmic_get(P_BAT_REG_V, &data);
	if (ddmw_get_i32(&pmic_management.items.batrv) != data)
		ddmw_set_i32(&pmic_management.items.batrv, data);

	hal_pmic_get(P_CHARGE_CURRENT, &data);
	if (ddmw_get_i32(&pmic_management.items.chrgc) != data)
		ddmw_set_i32(&pmic_management.items.chrgc, data);

	hal_pmic_get(P_TERMINATION_CURRENT, &data);
	if (ddmw_get_i32(&pmic_management.items.tc) != data)
		ddmw_set_i32(&pmic_management.items.tc, data);

	hal_pmic_get(P_PRECHARGE_CURRENT, &data);
	if (ddmw_get_i32(&pmic_management.items.pchrgc) != data)
		ddmw_set_i32(&pmic_management.items.pchrgc, data);

	hal_pmic_get(P_MIN_SYS_V, &data);
	if (ddmw_get_i32(&pmic_management.items.msysv) != data)
		ddmw_set_i32(&pmic_management.items.msysv, data);

	hal_pmic_get(P_BOOST_REG_V, &data);
	if (ddmw_get_i32(&pmic_management.items.bstrv) != data)
			ddmw_set_i32(&pmic_management.items.bstrv, data);

	hal_pmic_get(P_THERMAL_REG, &data);
	if (ddmw_get_i32(&pmic_management.items.thrgth) != data)
		ddmw_set_i32(&pmic_management.items.thrgth, data);

	//Battery Status
	hal_pmic_get(P_BATTERY_STATUS, &bat_status);
	if (ddmw_get_i32(&pmic_management.items.bstat) != (int32_t)bat_status)
		ddmw_set_i32(&pmic_management.items.bstat, bat_status);

	hal_pmic_get(P_CHARGE_TYPE, &chargetype);
	if (ddmw_get_i32(&pmic_management.items.chty) != (int32_t)chargetype)
		ddmw_set_i32(&pmic_management.items.chty, chargetype);

	hal_pmic_get(P_BAT_HEALTH, &bat_health);
	if (ddmw_get_i32(&pmic_management.items.bhlt) != (int32_t)bat_health)
		ddmw_set_i32(&pmic_management.items.bhlt, bat_health);

	//Power Connected - Online
	hal_pmic_get(P_ONLINE, &online);
	if (ddmw_get_i32(&pmic_management.items.online) != online)
		ddmw_set_i32(&pmic_management.items.online, online);
}

#endif  // CONNECTOR_PMIC
