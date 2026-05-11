/*****************************************************************************
 * \file       hal_pmic.c
 * \brief      PMIC Hardware Abstraction Layer
 *
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "dicm_framework_config.h"
#ifdef HAL_PMIC

#include "drv_bq25895.h"
#include "hal_pmic.h"

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

//! \~ Initialize PMIC device
void hal_pmic_init (void)
{
	pmic_hw_init();
}

//! \~ Read PMIC Parameter
int hal_pmic_get ( pmic_parameter_t parm_id, void *data_ptr)
{
	switch (parm_id)
	{
		case P_BAT_REG_V:
			pmic_hw_get_params(PMIC_REGULATION_VOLTAGE, data_ptr);
			break;

		case P_CHARGE_CURRENT:
			pmic_hw_get_params(PMIC_CHARGE_CURRENT, data_ptr);
			break;

		case P_TERMINATION_CURRENT:
			pmic_hw_get_params(PMIC_TERMINATION_CURRENT, data_ptr);
			break;

		case P_PRECHARGE_CURRENT:
			pmic_hw_get_params(PMIC_PRECHARGE_CURRENT, data_ptr);
			break;

		case P_MIN_SYS_V:
			pmic_hw_get_params(PMIC_MIN_SYSTEM_VOLTAGE, data_ptr);
			break;

		case P_BOOST_REG_V:
			pmic_hw_get_params(PMIC_BOOST_MODE_VOLTAGE_REGULATION, data_ptr);
			break;

		case P_THERMAL_REG:
			pmic_hw_get_params(PMIC_THERMAL_REGULATION_THRESHOLD, data_ptr);
			break;

		case P_BATTERY_STATUS:
			pmic_hw_get_params(PMIC_STATUS, data_ptr);
			break;

		case P_CHARGE_TYPE:
			pmic_hw_get_params(PMIC_CHARGE_TYPE, data_ptr);
			break;

		case P_MANUFACTURE_ID:
			pmic_hw_get_params(PMIC_MANUFACTURER, data_ptr);
			break;

		case P_BAT_MODEL:
			pmic_hw_get_params(PMIC_MODEL_NAME, data_ptr);
			break;

		case P_ONLINE:
			pmic_hw_get_params(PMIC_ONLINE, data_ptr);
			break;

		case P_BAT_HEALTH:
			pmic_hw_get_params(PMIC_HEALTH, data_ptr);
			break;

		default:
			break;
	}
	return 0;
}

//! \~ Write PMIC Parameter
int hal_pmic_set ( pmic_parameter_t parm_id, void * val)
{
	switch (parm_id)
	{
		case P_BAT_REG_V:
			pmic_hw_set_params(PMIC_REGULATION_VOLTAGE, val);
			break;

		case P_CHARGE_CURRENT:
			pmic_hw_set_params(PMIC_CHARGE_CURRENT, val);
			break;

		case P_TERMINATION_CURRENT:
			pmic_hw_set_params(PMIC_TERMINATION_CURRENT, val);
			break;

		case P_PRECHARGE_CURRENT:
			pmic_hw_set_params(PMIC_PRECHARGE_CURRENT, val);
			break;

		case P_MIN_SYS_V:
			pmic_hw_set_params(PMIC_MIN_SYSTEM_VOLTAGE, val);
			break;

		case P_BOOST_REG_V:
			pmic_hw_set_params(PMIC_BOOST_MODE_VOLTAGE_REGULATION, val);
			break;
		case P_THERMAL_REG:
			pmic_hw_set_params(PMIC_THERMAL_REGULATION_THRESHOLD, val);
			break;

		default:
			break;
	}
	return 0;
}
#endif

