/*****************************************************************************
 * \file       drv_bq25895.h
 * \brief      PMIC Driver Header File
 *
 *****************************************************************************/
#ifdef DEVICE_BQ25895
#include <stdint.h>

//Initialization values 
#define BATTRY_REGULATION_VOLTAGE         4200000
#define CHARGE_CURRENT                    300000
#define TERMINATION_CURRENT               50000
#define PRECHARGE_CURRENT                 128000
#define MINIMUM_SYS_VOLTAGE               3600000
#define BOOST_VOLTAGE                     5000000
#define BOOST_MAX_CURRENT                 1000000
#define THERMAL_REGULATION_VOLTAGE        120

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum bq25895_fields
{
		F_EN_HIZ, F_EN_ILIM, F_IILIM,				     /* Reg00 */
		F_BHOT, F_BCOLD, F_VINDPM_OFS,				     /* Reg01 */
		F_CONV_START, F_CONV_RATE, F_BOOSTF, F_ICO_EN,
		F_HVDCP_EN, F_MAXC_EN, F_FORCE_DPM, F_AUTO_DPDM_EN,	     /* Reg02 */
		F_BAT_LOAD_EN, F_WD_RST, F_OTG_CFG, F_CHG_CFG, F_SYSVMIN,
		F_MIN_VBAT_SEL,						     /* Reg03 */
		F_PUMPX_EN, F_ICHG,					     /* Reg04 */
		F_IPRECHG, F_ITERM,					     /* Reg05 */
		F_VREG, F_BATLOWV, F_VRECHG,				     /* Reg06 */
		F_TERM_EN, F_STAT_DIS, F_WD, F_TMR_EN, F_CHG_TMR,
		F_JEITA_ISET,						     /* Reg07 */
		F_BATCMP, F_VCLAMP, F_TREG,				     /* Reg08 */
		F_FORCE_ICO, F_TMR2X_EN, F_BATFET_DIS, F_JEITA_VSET,
		F_BATFET_DLY, F_BATFET_RST_EN, F_PUMPX_UP, F_PUMPX_DN,	     /* Reg09 */
		F_BOOSTV, F_PFM_OTG_DIS, F_BOOSTI,			     /* Reg0A */
		F_VBUS_STAT, F_CHG_STAT, F_PG_STAT, F_SDP_STAT,
		F_VSYS_STAT,						     /* Reg0B */
		F_WD_FAULT, F_BOOST_FAULT, F_CHG_FAULT, F_BAT_FAULT,
		F_NTC_FAULT,						     /* Reg0C */
		F_FORCE_VINDPM, F_VINDPM,				     /* Reg0D */
		F_THERM_STAT, F_BATV,					     /* Reg0E */
		F_SYSV,							     /* Reg0F */
		F_TSPCT,						     /* Reg10 */
		F_VBUS_GD, F_VBUSV,					     /* Reg11 */
		F_ICHGR,						     /* Reg12 */
		F_VDPM_STAT, F_IDPM_STAT, F_IDPM_LIM,			     /* Reg13 */
		F_REG_RST, F_ICO_OPTIMIZED, F_PN, F_TS_PROFILE, F_DEV_REV,   /* Reg14 */
		F_MAX_FIELDS
};

typedef struct bq_field_s
{
		uint32_t  reg_id;
		uint8_t reg_addr;
		uint8_t start_fld;
		uint8_t stop_fld;
}bq_field_t;

struct bq25895_state
{
		uint8_t online;
		uint8_t chrg_status;
		uint8_t chrg_fault;
		uint8_t vsys_status;
		uint8_t boost_fault;
		uint8_t bat_fault;
};

enum bq25895_chrg_fault {
	CHRG_FAULT_NORMAL,
	CHRG_FAULT_INPUT,
	CHRG_FAULT_THERMAL_SHUTDOWN,
	CHRG_FAULT_TIMER_EXPIRED,
};

enum bq25895_status {
	STATUS_NOT_CHARGING,
	STATUS_PRE_CHARGING,
	STATUS_FAST_CHARGING,
	STATUS_TERMINATION_DONE
};

enum PMIC_status {
	PMIC_DISCHARGING,
	PMIC_NOT_CHARGING,
	PMIC_CHARGING,
	PMIC_FULL
};
enum PMIC_charge_type {
	PMIC_CHARGE_TYPE_NONE,
	PMIC_CHARGE_TYPE_STANDARD,
	PMIC_CHARGE_TYPE_FAST,
	PMIC_CHARGE_TYPE_UNKNOWN
};

enum PMIC_health {
	PMIC_HEALTH_GOOD,
	PMIC_HEALTH_OVERVOLTAGE,
	PMIC_HEALTH_SAFETY_TIMER_EXPIRE,
	PMIC_HEALTH_OVERHEAT,
	PMIC_HEALTH_UNSPEC_FAILURE
};

struct bq25895_range
{
		uint32_t min;
		uint32_t max;
		uint32_t step;
};

struct bq25895_lookup 
{
	const uint32_t *tbl;
	uint32_t size;
};

typedef enum pmic_property
{
		PMIC_MANUFACTURER,
		PMIC_MODEL_NAME,
		PMIC_STATUS,
		PMIC_CHARGE_TYPE,
		PMIC_ONLINE,
		PMIC_HEALTH,
		PMIC_CHARGE_CURRENT,
		PMIC_REGULATION_VOLTAGE,
		PMIC_TERMINATION_VOLTAGE,
		PMIC_TERMINATION_CURRENT,
		PMIC_PRECHARGE_CURRENT,
		PMIC_MIN_SYSTEM_VOLTAGE,
		PMIC_BOOST_MODE_VOLTAGE_REGULATION,
		PMIC_THERMAL_REGULATION_THRESHOLD
} pmic_property_t;


/*
 * Most of the val -> idx conversions can be computed, given the minimum,
 * maximum and the step between values. For the rest of conversions, we use
 * lookup tables.
 */
typedef enum bq25895_table_ids {
	/* range tables */
	TBL_ICHG,
	//TBL_IILIM,
	TBL_VREG,
	TBL_ITERM,
	TBL_IPRECHG,
	TBL_SYSVMIN,
	TBL_BOOSTV,
	//TBL_VBATCOMP,
	//TBL_RBATCOMP,

	/* lookup tables */
	TBL_BOOSTI,
	TBL_TREG,
}bq25895_table_ids_t;

int pmic_hw_get_params(pmic_property_t parm_id, void *data_ptr);

int pmic_hw_set_params(pmic_property_t parm_id, void * value);

int pmic_hw_init();

#endif
