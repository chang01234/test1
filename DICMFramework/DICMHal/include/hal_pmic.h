/*****************************************************************************
 * \file       hal_pmic.h
 * \brief      PMIC Hardware Abstraction Layer
 *
 *****************************************************************************/


typedef enum pmic_parameter
{	
	P_BAT_REG_V,			/* regulation voltage Volt */
	P_CHARGE_CURRENT,		/* charge current	  mA */
	P_TERMINATION_CURRENT,	/* termination current mA		*/
	P_PRECHARGE_CURRENT,	/* precharge current	mA	*/
	P_MIN_SYS_V,			/* minimum system voltage limit Volt */
	P_BOOST_REG_V,			/* boost regulation voltage	Volt */
	P_THERMAL_REG,			/* thermal regulation threshold Degree Celsius */
	P_BATTERY_STATUS,		/* Battery_status(charging,discharging,not chrging,full) – enum */
	P_CHARGE_TYPE,			/* Charge_type(None,Standard,fast,unkown)-enum */
	P_MANUFACTURE_ID,		/* Manufacturer – string */
	P_BAT_MODEL,			/* Model – String */
	P_ONLINE,				/* Bool */
	P_BAT_HEALTH			/*Health – (Good,Overvoltage,safety timer expired, overheat, Unspecified failure)-enum */

} pmic_parameter_t;

typedef enum battery_health
{
	GOOD,
	OVERVOLTAGE,
	SAFETY_TIME_EXPIRE,
	OVERHEAT,
	UNSPEC_FAILURE
} battery_health_t;

typedef enum battery_status
{
	DISCHARGING,
	NOT_CHARGING,
	CHARGING,
	FULL
} battery_status_t;

typedef enum charge_type
{
	NONE,
	STANDARD,
	FAST,
	UNKNOWN
} charge_type_t;

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

//! \~ Initialize PMIC device
void hal_pmic_init(void);	

//! \~ Read PMIC Parameter
int hal_pmic_get ( pmic_parameter_t parm_id, void *data_ptr);

//! \~ Write PMIC Parameter
int hal_pmic_set ( pmic_parameter_t parm_id, void * val);



