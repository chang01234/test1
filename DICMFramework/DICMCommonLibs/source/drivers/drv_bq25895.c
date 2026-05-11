/*****************************************************************************
 * \file       drv_bq25895.c
 * \brief      PMIC Driver File
 *
 *****************************************************************************/

#include "iGeneralDefinitions.h"

#ifdef DEVICE_BQ25895
#include "hal_i2c_master.h"
#include "drv_bq25895.h"
#include <string.h>

#define BQ25895_MANUFACTURER		"Texas Instruments"
#define BQ25895_CHIP_NAME			"BQ25895"
static void set_fields(uint32_t idx, const struct bq_field_s *tbl, uint8_t val );
static uint8_t get_fields(uint32_t idx, const struct bq_field_s *tbl);

static int bq25895_chip_reset();

static uint8_t bq25890_find_idx(uint32_t value, bq25895_table_ids_t id);
static uint32_t bq25895_find_val(uint8_t idx, bq25895_table_ids_t id);

static int bq25895_get_chip_state(const struct bq_field_s *bq, struct bq25895_state *state);


const bq_field_t bq25895_reg_fields[]=
{
	/* REG00 */
	{F_EN_HIZ, 0x00, 7, 7},
	{F_EN_ILIM, 0x00, 6, 6},
	{F_IILIM,0x00, 0, 5},
	/* REG01 */
	{F_BHOT,0x01, 6, 7},
	{F_BCOLD,0x01, 5, 5},
	{F_VINDPM_OFS,0x01, 0, 4},
	/* REG02 */
	{F_CONV_START,0x02, 7, 7},
	{F_CONV_RATE,0x02, 6, 6},
	{F_BOOSTF,0x02, 5, 5},
	{F_ICO_EN,0x02, 4, 4},
	{F_HVDCP_EN,0x02, 3, 3},  // reserved on BQ25896
	{F_MAXC_EN,0x02, 2, 2},  // reserved on BQ25896
	{F_FORCE_DPM,0x02, 1, 1},
	{F_AUTO_DPDM_EN,0x02, 0, 0},
	/* REG03 */
	{F_BAT_LOAD_EN,0x03, 7, 7},
	{F_WD_RST,0x03, 6, 6},
	{F_OTG_CFG,0x03, 5, 5},
	{F_CHG_CFG,0x03, 4, 4},
	{F_SYSVMIN,0x03, 1, 3},
	{F_MIN_VBAT_SEL,0x03, 0, 0}, // BQ25896 only
	/* REG04 */
	{F_PUMPX_EN,0x04, 7, 7},
	{F_ICHG,0x04, 0, 6},
	/* REG05 */
	{F_IPRECHG,0x05, 4, 7},
	{F_ITERM,0x05, 0, 3},
	/* REG06 */
	{F_VREG,0x06, 2, 7},
	{F_BATLOWV,0x06, 1, 1},
	{F_VRECHG,0x06, 0, 0},
	/* REG07 */
	{F_TERM_EN,0x07, 7, 7},
	{F_STAT_DIS,0x07, 6, 6},
	{F_WD,0x07, 4, 5},
	{F_TMR_EN,0x07, 3, 3},
	{F_CHG_TMR,0x07, 1, 2},
	{F_JEITA_ISET,0x07, 0, 0}, // reserved on BQ25895
	/* REG08 */
	{F_BATCMP,0x08, 5, 7},
	{F_VCLAMP,0x08, 2, 4},
	{F_TREG,0x08, 0, 1},
	/* REG09 */
	{F_FORCE_ICO,0x09, 7, 7},
	{F_TMR2X_EN,0x09, 6, 6},
	{F_BATFET_DIS,0x09, 5, 5},
	{F_JEITA_VSET,0x09, 4, 4}, // reserved on BQ25895
	{F_BATFET_DLY,0x09, 3, 3},
	{F_BATFET_RST_EN,0x09, 2, 2},
	{F_PUMPX_UP,0x09, 1, 1},
	{F_PUMPX_DN,0x09, 0, 0},
	/* REG0A */
	{F_BOOSTV,0x0A, 4, 7},
	{F_BOOSTI,0x0A, 0, 2}, // reserved on BQ25895
	{F_PFM_OTG_DIS,0x0A, 3, 3}, // BQ25896 only
	/* REG0B */
	{F_VBUS_STAT,0x0B, 5, 7},
	{F_CHG_STAT,0x0B, 3, 4},
	{F_PG_STAT,0x0B, 2, 2},
	{F_SDP_STAT,0x0B, 1, 1}, // reserved on BQ25896
	{F_VSYS_STAT,0x0B, 0, 0},
	/* REG0C */
	{F_WD_FAULT,0x0C, 7, 7},
	{F_BOOST_FAULT,0x0C, 6, 6},
	{F_CHG_FAULT,0x0C, 4, 5},
	{F_BAT_FAULT,0x0C, 3, 3},
	{F_NTC_FAULT,0x0C, 0, 2},
	/* REG0D */
	{F_FORCE_VINDPM,0x0D, 7, 7},
	{F_VINDPM,0x0D, 0, 6},
	/* REG0E */
	{F_THERM_STAT,0x0E, 7, 7},
	{F_BATV,0x0E, 0, 6},
	/* REG0F */
	{F_SYSV,0x0F, 0, 6},
	/* REG10 */
	{F_TSPCT,0x10, 0, 6},
	/* REG11 */
	{F_VBUS_GD,0x11, 7, 7},
	{F_VBUSV,0x11, 0, 6},
	/* REG12 */
	{F_ICHGR,0x12, 0, 6},
	/* REG13 */
	{F_VDPM_STAT,0x13, 7, 7},
	{F_IDPM_STAT,0x13, 6, 6},
	{F_IDPM_LIM,0x13, 0, 5},
	/* REG14 */
	{F_REG_RST,0x14, 7, 7},
	{F_ICO_OPTIMIZED,0x14, 6, 6},
	{F_PN,0x14, 3, 5},
	{F_TS_PROFILE,0x14, 2, 2},
	{F_DEV_REV,0x14, 0, 1}
};


/* Thermal Regulation Threshold lookup table, in degrees Celsius */
static const uint32_t bq25895_treg_tbl[] = { 60, 80, 100, 120 };

#define BQ25895_TREG_TBL_SIZE		ARRAY_SIZE(bq25895_treg_tbl)

/* Boost mode current limit lookup table, in uA */
static const uint32_t bq25895_boosti_tbl[] = {
	500000, 700000, 1100000, 1300000, 1600000, 1800000, 2100000, 2400000
};

#define BQ25895_BOOSTI_TBL_SIZE		ARRAY_SIZE(bq25895_boosti_tbl)

static const union {
	struct bq25895_range  rt;
	struct bq25895_lookup lt;
} bq25895_tables[] = {
	/* range tables */
	/* TODO: BQ25896 has max ICHG 3008 mA */
	/* range tables */
	/* TODO: BQ25896 has max ICHG 3008 mA */
	//[TBL_ICHG] =	{ .rt = {0,	  5056000, 64000} },	 /* uA */
	[TBL_ICHG] =	{ .rt = {0,	  5056000, 64000} },	 /* uA */
	[TBL_VREG] =	{ .rt = {3840000, 4608000, 16000} },	 /* uV */
	[TBL_ITERM] =	{ .rt = {64000,   1024000, 64000} },	 /* uA */
	[TBL_IPRECHG] =	{ .rt = {64000,   1024000, 64000} },	 /* uA */
	//[TBL_IILIM] =   { .rt = {100000,  3250000, 50000} },	 /* uA */
	[TBL_SYSVMIN] = { .rt = {3000000, 3700000, 100000} },	 /* uV */
	[TBL_BOOSTV] =	{ .rt = {4550000, 5510000, 64000} },	 /* uV */
	//[TBL_VBATCOMP] ={ .rt = {0,        224000, 32000} },	 /* uV */
	//[TBL_RBATCOMP] ={ .rt = {0,        140000, 20000} },	 /* uOhm */

	/* lookup tables */
	[TBL_BOOSTI] =	{ .lt = {bq25895_boosti_tbl, BQ25895_BOOSTI_TBL_SIZE} },
	[TBL_TREG] =	{ .lt = {bq25895_treg_tbl, BQ25895_TREG_TBL_SIZE} }

};

static void set_fields(uint32_t idx,const struct bq_field_s *tbl, uint8_t val )
{
	uint8_t data_rd;
	uint8_t data[2];
	hal_i2c_master_writeread(DEVICE_BQ25895_I2C_PORT,DEVICE_BQ25895_ADDRESS, &(tbl[idx].reg_addr), 1, &data_rd,1);
	data[0] = 0;
	data[1] = 0;
	data[0] = (tbl[idx].reg_addr);
	data[1] = (data_rd & ((0xff << ((tbl[idx].stop_fld)+1)) | ~(0xff << tbl[idx].start_fld))) | (val<<tbl[idx].start_fld);
	hal_i2c_master_write(DEVICE_BQ25895_I2C_PORT,DEVICE_BQ25895_ADDRESS, data, sizeof(data));
}

static uint8_t get_fields(uint32_t idx,const struct bq_field_s *tbl)
{
	uint8_t data_rd;
	hal_i2c_master_writeread(DEVICE_BQ25895_I2C_PORT,DEVICE_BQ25895_ADDRESS, &(tbl[idx].reg_addr), 1, &data_rd,1);
	data_rd =  (data_rd & ~((0xff << ((tbl[idx].stop_fld)+1)) | ~(0xff<< tbl[idx].start_fld))) >> tbl[idx].start_fld;
	return data_rd;
}

static int bq25895_chip_reset()
{
	int ret=0;
	int rst_check_counter = 10;

	set_fields(F_REG_RST,bq25895_reg_fields, 1);
	if (ret < 0)
		return ret;

	do {
		ret = get_fields(F_REG_RST,bq25895_reg_fields);
		if (ret < 0)
			return ret;

		sleep(1);
	} while (ret == 1 && --rst_check_counter);

	if (!rst_check_counter)
		return -1;

	return 0;
}

//static int bq25890_handle_interrupt(){ }

int pmic_hw_get_params(pmic_property_t parm_id, void *data_ptr)
{
	uint8_t idx = 0;
	struct bq25895_state state;
	switch(parm_id)
	{
		case PMIC_REGULATION_VOLTAGE:
			idx = get_fields(F_VREG, bq25895_reg_fields);
			*(uint32_t *)(data_ptr) = bq25895_find_val(idx, TBL_VREG);
			break;

		case PMIC_CHARGE_CURRENT:
			idx = get_fields(F_ICHG, bq25895_reg_fields);
			*(uint32_t *)(data_ptr) = bq25895_find_val(idx, TBL_ICHG);
			break;

		case PMIC_TERMINATION_CURRENT:
			idx = get_fields(F_ITERM, bq25895_reg_fields);
			*(uint32_t *)(data_ptr) = bq25895_find_val(idx, TBL_ITERM);
			break;

		case PMIC_PRECHARGE_CURRENT:
			idx = get_fields(F_IPRECHG, bq25895_reg_fields);
			*(uint32_t *)(data_ptr) = bq25895_find_val(idx, TBL_IPRECHG);
			break;

		case PMIC_MIN_SYSTEM_VOLTAGE:	//Minimum System Voltage Limit
			idx = get_fields(F_SYSVMIN, bq25895_reg_fields);
			*(uint32_t *)(data_ptr) = bq25895_find_val(idx, TBL_SYSVMIN);
			break;

		case PMIC_BOOST_MODE_VOLTAGE_REGULATION: //Boost Mode Voltage Regulation
			idx = get_fields(F_BOOSTV, bq25895_reg_fields);
			*(uint32_t *)(data_ptr) = bq25895_find_val(idx, TBL_BOOSTV);
			break;	

		case PMIC_THERMAL_REGULATION_THRESHOLD: //Thermal Regulation Threshold
			idx = get_fields(F_TREG, bq25895_reg_fields);
			*(uint32_t *)(data_ptr) = 1000 * bq25895_find_val(idx, TBL_TREG);
			break;

		case PMIC_STATUS:	//Charging Status: 00 – Not Charging, 01 – Pre-charge ( < VBATLOWV), 10 – Fast Charging, 11 – Charge Termination Done
			bq25895_get_chip_state(bq25895_reg_fields, &state);
			if (!state.online)
				*(int *)(data_ptr)= PMIC_DISCHARGING;
			else if (state.chrg_status == STATUS_NOT_CHARGING)
				*(int *)(data_ptr) = PMIC_NOT_CHARGING;
			else if (state.chrg_status == STATUS_PRE_CHARGING || state.chrg_status == STATUS_FAST_CHARGING)
				*(int *)(data_ptr)= PMIC_CHARGING;
			else if (state.chrg_status == STATUS_TERMINATION_DONE)
				*(int *)(data_ptr)= PMIC_FULL;
			break;

		case PMIC_CHARGE_TYPE:
			bq25895_get_chip_state(bq25895_reg_fields, &state);
			if (!state.online || state.chrg_status == STATUS_NOT_CHARGING || state.chrg_status == STATUS_TERMINATION_DONE)
				*(int *)(data_ptr) = PMIC_CHARGE_TYPE_NONE;
			else if (state.chrg_status == STATUS_PRE_CHARGING)
				*(int *)(data_ptr) = PMIC_CHARGE_TYPE_STANDARD;
			else if (state.chrg_status == STATUS_FAST_CHARGING)
				*(int *)(data_ptr) = PMIC_CHARGE_TYPE_FAST;
			else /* unreachable */
				*(int *)(data_ptr) = PMIC_CHARGE_TYPE_UNKNOWN;
			break;

		case PMIC_MANUFACTURER:
			memcpy(data_ptr, BQ25895_MANUFACTURER, 18);
			break;

		case PMIC_MODEL_NAME:
			memcpy(data_ptr, BQ25895_CHIP_NAME, 8);
			break;

		case PMIC_ONLINE:
			idx = get_fields(F_PG_STAT, bq25895_reg_fields);
			*(bool*)data_ptr = idx;
			break;

		case PMIC_HEALTH:
			bq25895_get_chip_state(bq25895_reg_fields, &state);
			if (!state.chrg_fault && !state.bat_fault && !state.boost_fault)
				*(int *)(data_ptr) = PMIC_HEALTH_GOOD;
			else if (state.bat_fault)
				*(int *)(data_ptr) = PMIC_HEALTH_OVERVOLTAGE;
			else if (state.chrg_fault == CHRG_FAULT_TIMER_EXPIRED)
				*(int *)(data_ptr) = PMIC_HEALTH_SAFETY_TIMER_EXPIRE;
			else if (state.chrg_fault == CHRG_FAULT_THERMAL_SHUTDOWN)
				*(int *)(data_ptr) = PMIC_HEALTH_OVERHEAT;
			else
				*(int *)(data_ptr) = PMIC_HEALTH_UNSPEC_FAILURE;
			break;

		default:
			break;
	}
	return 0;
}

int pmic_hw_set_params(pmic_property_t parm_id, void * value)
{
	uint8_t idx = 0;

	switch(parm_id)
	{
		case PMIC_REGULATION_VOLTAGE:	//Charge Voltage Limit
			idx = bq25890_find_idx(*(uint32_t *)value, TBL_VREG);
			set_fields(F_VREG, bq25895_reg_fields, idx);
			break;

		case PMIC_CHARGE_CURRENT:	//Fast Charge Current Limit
			idx = bq25890_find_idx(*(uint32_t *)value, TBL_ICHG);
			set_fields(F_ICHG, bq25895_reg_fields, idx);
			break;

		case PMIC_TERMINATION_CURRENT:	//Termination Current Limit
			idx = bq25890_find_idx(*(uint32_t *)value, TBL_ITERM);
			set_fields(F_ITERM, bq25895_reg_fields, idx);
			break;

		case PMIC_PRECHARGE_CURRENT:	//Precharge Current Limit
			idx = bq25890_find_idx(*(uint32_t *)value, TBL_IPRECHG);
			set_fields(F_IPRECHG, bq25895_reg_fields, idx);
			break;

		case PMIC_MIN_SYSTEM_VOLTAGE:	//Minimum System Voltage Limit
			idx = bq25890_find_idx(*(uint32_t *)value, TBL_SYSVMIN);
			set_fields(F_SYSVMIN, bq25895_reg_fields, idx);
			break;

		case PMIC_BOOST_MODE_VOLTAGE_REGULATION: //Boost Mode Voltage Regulation
			idx = bq25890_find_idx(*(uint32_t *)value, TBL_BOOSTV);
			set_fields(F_BOOSTV, bq25895_reg_fields, idx);
			break;

		case PMIC_THERMAL_REGULATION_THRESHOLD: //Thermal Regulation Threshold
			idx = bq25890_find_idx(*(uint32_t *)value, TBL_TREG);
			set_fields(F_TREG, bq25895_reg_fields, idx);
			break;
		default:
			break;
	}
	return 0;
}

int pmic_hw_init()
{
	int ret;
	int i;

	const struct {
		enum bq25895_fields id;
		uint32_t value;
	} init_data[] = {
		/* order of this table should not be changed */
		{F_ICHG,	 CHARGE_CURRENT},
		{F_VREG,	 BATTRY_REGULATION_VOLTAGE},
		{F_ITERM,	 TERMINATION_CURRENT},
		{F_IPRECHG,	 PRECHARGE_CURRENT},
		{F_SYSVMIN,	 MINIMUM_SYS_VOLTAGE},
		{F_BOOSTV,	 BOOST_VOLTAGE},
		{F_BOOSTI,	 BOOST_MAX_CURRENT},
	};

	ret = bq25895_chip_reset();
	if (ret < 0) {
		return ret;
	}

	/* disable watchdog */
	set_fields(F_WD,bq25895_reg_fields, 0);
	if (ret < 0) {
		return ret;
	}
	set_fields(F_CONV_RATE,bq25895_reg_fields, 1);
	if (ret < 0) {
		return ret;
	}
	ret = get_fields(F_CHG_CFG,bq25895_reg_fields);
	sleep(1);
	set_fields(F_CHG_CFG,bq25895_reg_fields, 1);

	/* initialize currents/voltages and other parameters */
	uint8_t idx;
	for (i = 0; i < ARRAY_SIZE(init_data); i++) {
		sleep(2);
		idx = bq25890_find_idx(init_data[i].value, i);
		set_fields(init_data[i].id, bq25895_reg_fields, idx);
		if (ret < 0) {
			return ret;
		}
	}
	set_fields(F_BATFET_DIS,bq25895_reg_fields, 0);
	sleep(1);
	return 0;
}

static uint8_t bq25890_find_idx(uint32_t value, enum bq25895_table_ids id)
{
	uint8_t idx;

	if (id >= TBL_BOOSTI) {
		const uint32_t *tbl = bq25895_tables[id].lt.tbl;
		uint32_t tbl_size = bq25895_tables[id].lt.size;

		for (idx = 1; idx < tbl_size && tbl[idx] <= value; idx++)
			;
	} else {
		const struct bq25895_range *rtbl = &bq25895_tables[id].rt;
		uint8_t rtbl_size;

		rtbl_size = (rtbl->max - rtbl->min) / rtbl->step + 1;

		for (idx = 1;
				idx < rtbl_size && (idx * rtbl->step + rtbl->min <= value);
				idx++)
			;
	}

	return idx - 1;
}

static uint32_t bq25895_find_val(uint8_t idx, enum bq25895_table_ids id)
{
	const struct bq25895_range *rtbl;

	/* lookup table? */
	if (id >= TBL_BOOSTI)
		return bq25895_tables[id].lt.tbl[idx];

	/* range table */
	rtbl = &bq25895_tables[id].rt;

	return (rtbl->min + idx * rtbl->step);
}

static int bq25895_get_chip_state(const struct bq_field_s *bq, struct bq25895_state *state)
{
	int i, ret;

	struct {
		enum bq25895_fields id;
		uint8_t *data;
	} state_fields[] = {
		{F_CHG_STAT,	&state->chrg_status},
		{F_PG_STAT,		&state->online},
		{F_VSYS_STAT,	&state->vsys_status},
		{F_BOOST_FAULT, &state->boost_fault},
		{F_BAT_FAULT,	&state->bat_fault},
		{F_CHG_FAULT,	&state->chrg_fault}
	};

	for (i = 0; i < ARRAY_SIZE(state_fields); i++) {
		ret = get_fields(state_fields[i].id, bq);
		if (ret < 0)
			return ret;

		*state_fields[i].data = ret;
	}
#if 0
	printf("S:CHG/PG/VSYS=%d/%d/%d, F:CHG/BOOST/BAT=%d/%d/%d\n",
		state->chrg_status, state->online, state->vsys_status,
		state->chrg_fault, state->boost_fault, state->bat_fault);
#endif
	return 0;
}


#endif
