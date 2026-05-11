/*
 * rvc_to_ddm.c
 *
 *  Created on: 4th June 2020
 *      Author: Stefan.Henningsohn
 */

#include "configuration.h"

#include "rvc_to_ddm.h"
#include "PImage.h"

typedef struct
{
	int32_t domain_value;
	int32_t system_value;
} domain_to_system_t;

static const domain_to_system_t rvc_htr_elsel_to_system_value [] = {
	{0,HTR0ESEL_GAS},
	{1,HTR0ESEL_GAS_PLUSEL_900_W},
	{2,HTR0ESEL_GAS_PLUSEL_1800_W},
	{3,HTR0ESEL_EL_900_W},
	{4,HTR0ESEL_EL_1800_W}
};

static const domain_to_system_t rvc_htr_mode_to_system_value [] = {
	{0,HTR0WTRTEMP_ECO},
	{1,HTR0WTRTEMP_HOT},
	{2,HTR0WTRTEMP_BOOST}
};

static const domain_to_system_t rvc_htr_system_units_to_system_value [] = {
	{0,HTR0SYSU_METRIC},
	{1,HTR0SYSU_IMPERIAL}
};

static const domain_to_system_t rvc_htr_fan_speed_to_system_value [] = {
	{0,FANSPEED_LEVEL0},
	{1,FANSPEED_LEVEL1},
	{2,FANSPEED_LEVEL2},
	{3,FANSPEED_LEVEL3},
	{4,FANSPEED_LEVEL4},
	{5,FANSPEED_LEVEL5},
	{6,FANSPEED_LEVEL6},
	{7,FANSPEED_LEVEL7}
};

static const domain_to_system_t rvc_gas_wtr_heating_to_system_value [] = {
	{0,0}, // Off
	{1,0}, // Purge, still not on, not sent by heater
	{2,1}, // On, Low
	{3,1}, // On, Medium
	{4,1}, // On, High
};

static const domain_to_system_t rvc_ac_wtr_heating_to_system_value [] = {
	{0,0}, // Off
	{1,1}, // Low, on
	{2,1}, // Med, not used but on in case of other
	{3,1}, // High, on
};

static const domain_to_system_t rvc_htr_wtr_temp_to_system_value [] = {
	{0,HTR0WTRTS_COLD},
	{1,HTR0WTRTS_WARM},
	{2,HTR0WTRTS_HOT},
};

typedef struct
{
	int32_t variable;	//Variable in system parameter store
	uint32_t ddm_parameter; //DDM parameter
	uint32_t bitmask;	//Bit mask in a DDM parameter, zero if not used
	uint32_t shift;		//Shift length of masked data
	int32_t min;		//Min value for converting to a DDM parameter
	int32_t max;		//Max value for converting to a DDM parameter
	int32_t def;		//Default value, when outside min and max value
	int32_t gain;		//Gain of RV-C value = 1/precision, 
	int32_t offset;		//Offset of RV-C value
	int32_t enum_size;	//Enum size if value should be converted to an enum, must be 0 if not used
	const domain_to_system_t *enum_table; //Pointer to enum table, NULL if not used
} conversion_table_t;

// Conversion table for Status frames
static const conversion_table_t conv_table[] = {
	// DGN 103559
	{.variable=VAR_DGN_130559_ENERGY_SOURCE,    .ddm_parameter=HTR0ESEL,    .bitmask=0x0, .min=0, .max=4,   .def=0, .gain=1, .offset=0,   .enum_size=5, .enum_table=rvc_htr_elsel_to_system_value},
	{.variable=VAR_DGN_130559_AIR_HEATER_CMD,   .ddm_parameter=HTR0AON,     .bitmask=0x0, .min=0, .max=1,   .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130559_AIR_HEATER_MODE,  .ddm_parameter=HTR0AMD,     .bitmask=0x0, .min=0, .max=2,   .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130559_WTR_HEATER_CMD,   .ddm_parameter=HTR0WTRON,   .bitmask=0x0, .min=0, .max=1,   .def=0,.gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130559_TARGET_ROOM_TEMP, .ddm_parameter=HTR0ATEMP,   .bitmask=0x0, .min=-5000, .max=15000, .def=0, .gain=100, .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130559_SILENT_FAN_MAX,   .ddm_parameter=HTR0SMAXFAN, .bitmask=0x0, .min=0, .max=7,   .def=3, .gain=1, .offset=0,   .enum_size=8, .enum_table=rvc_htr_fan_speed_to_system_value},
	{.variable=VAR_DGN_130559_VENT_FAN_MIN,     .ddm_parameter=HTR0VMINFAN, .bitmask=0x0, .min=0, .max=7,   .def=4, .gain=1, .offset=0,   .enum_size=8, .enum_table=rvc_htr_fan_speed_to_system_value},
	{.variable=VAR_DGN_130559_WTR_HEATER_MODE,  .ddm_parameter=HTR0WTRTEMP, .bitmask=0x0, .min=0, .max=2,   .def=0, .gain=1, .offset=0,   .enum_size=3, .enum_table=rvc_htr_mode_to_system_value},
	{.variable=VAR_DGN_130559_UNDERVOLT_THRES,  .ddm_parameter=HTR0UVTH,    .bitmask=0x0, .min=0, .max=253, .def=105, .gain=10,.offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130559_SYSTEM_UNITS,     .ddm_parameter=HTR0SYSU,    .bitmask=0x0, .min=0, .max=1,   .def=0, .gain=1, .offset=0,   .enum_size=2, .enum_table=rvc_htr_system_units_to_system_value},
	// DGN 103552
	{.variable=VAR_DGN_130552_COMFORT_SW_VER_MA, .ddm_parameter=HTR0CVER,   .bitmask=0xff, .shift=8, .min=0, .max=253, .def=0, .gain=1, .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130552_COMFORT_SW_VER_MI, .ddm_parameter=HTR0CVER,   .bitmask=0xff, .shift=0, .min=0, .max=253, .def=0, .gain=1, .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130552_BURNER_SW_VER_MA,  .ddm_parameter=HTR0BVER,   .bitmask=0xff, .shift=8, .min=0, .max=253, .def=0, .gain=1, .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130552_BURNER_SW_VER_MI,  .ddm_parameter=HTR0BVER,   .bitmask=0xff, .shift=0, .min=0, .max=253, .def=0, .gain=1, .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130552_PCBA_VER,          .ddm_parameter=HTR0PCBA,   .bitmask=0x0,  .shift=0, .min=0, .max=253, .def=0, .gain=1, .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130552_PROTOCOL_VER_MA,   .ddm_parameter=HTR0PROT,   .bitmask=0xff, .shift=8, .min=0, .max=253, .def=0, .gain=1, .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130552_PROTOCOL_VER_MI,   .ddm_parameter=HTR0PROT,   .bitmask=0xff, .shift=0, .min=0, .max=253, .def=0, .gain=1, .offset=0, .enum_size=0, .enum_table=NULL},
	// DGN 103553
	{.variable=VAR_DGN_130553_WARNING_FAULT_ACTIVE,   .ddm_parameter=HTR0ERRST,   .bitmask=0x1, .shift=0, .min=0, .max=1,    .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130553_CRITICAL_FAULT_ACTIVE,  .ddm_parameter=HTR0ERRST,   .bitmask=0x1, .shift=1, .min=0, .max=1,    .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130553_ACTIVE_FAULT_CODE_1,    .ddm_parameter=HTR0ERRCD1,  .bitmask=0x0, .min=0, .max=1021, .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130553_ACTIVE_FAULT_CODE_2,    .ddm_parameter=HTR0ERRCD2,  .bitmask=0x0, .min=0, .max=1021, .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130553_ACTIVE_FAULT_CODE_3,    .ddm_parameter=HTR0ERRCD3,  .bitmask=0x0, .min=0, .max=1021, .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130553_ACTIVE_FAULT_CODE_4,    .ddm_parameter=HTR0ERRCD4,  .bitmask=0x0, .min=0, .max=1021, .def=0, .gain=1, .offset=0,   .enum_size=0, .enum_table=NULL},
	// DGN 130555
	{.variable=VAR_DGN_130555_AIR_HTR_ON_STAT,      .ddm_parameter=HTR0AHTONST,  .bitmask=0x0, .min=0,  .max=1,  .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_AIR_HTR_ON_HOUR,      .ddm_parameter=HTR0AHTONH,   .bitmask=0x0, .min=0,  .max=23, .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_AIR_HTR_ON_MIN,       .ddm_parameter=HTR0AHTONM,   .bitmask=0x0, .min=0,  .max=59, .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_AIR_HTR_OFF_STAT,     .ddm_parameter=HTR0AHTOFFST, .bitmask=0x0, .min=0,  .max=1,  .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_AIR_HTR_OFF_HOUR,     .ddm_parameter=HTR0AHTOFFH,  .bitmask=0x0, .min=0,  .max=23, .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_AIR_HTR_OFF_MIN,      .ddm_parameter=HTR0AHTOFFM,  .bitmask=0x0, .min=0,  .max=59, .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_WTR_HTR_ON_STAT,      .ddm_parameter=HTR0WTRTST,   .bitmask=0x0, .min=0,  .max=1,  .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_WTR_HTR_ON_HOUR,      .ddm_parameter=HTR0WTRTONH,  .bitmask=0x0, .min=0,  .max=23, .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_WTR_HTR_ON_MIN,       .ddm_parameter=HTR0WTRTONM,  .bitmask=0x0, .min=0,  .max=59, .def=0, .gain=1,   .offset=0, .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME, .ddm_parameter=HTR0WTRTKET,  .bitmask=0x0, .min=0,  .max=96, .def=6, .gain=-15, .offset=0, .enum_size=0, .enum_table=NULL},
	// DGN 130556
	{.variable=VAR_DGN_130556_ROOM_TEMP,   .ddm_parameter=TH0ITEMP, .bitmask=0x0, .min=-5000, .max=15000, .gain=100, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130556_BUTTON_FAV,  .ddm_parameter=TH0BUT0,  .bitmask=0x0, .min=0,     .max=1, .gain=1,   .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130556_BUTTON_MENU, .ddm_parameter=TH0BUT1,  .bitmask=0x0, .min=0,     .max=1, .gain=1,   .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130556_BUTTON_HOME, .ddm_parameter=TH0BUT2,  .bitmask=0x0, .min=0,     .max=1, .gain=1,   .offset=0,   .enum_size=0, .enum_table=NULL},
	// DGN 130557
	{.variable=VAR_DGN_130557_ROOM_TEMP,      .ddm_parameter=HTR0RTS,       .bitmask=0x0, .min=-5000,  .max=15000, .gain=100, .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130557_WATER_TEMP,     .ddm_parameter=HTR0WTRTS,     .bitmask=0x0, .min=0,      .max=2, .gain=1,   .offset=0,   .enum_size=3, .enum_table=rvc_htr_wtr_temp_to_system_value},
	{.variable=VAR_DGN_130557_AC_PRESENT,     .ddm_parameter=HTR0ACST,      .bitmask=0x0, .min=0,      .max=1, .gain=1,   .offset=0,   .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_130557_GAS_HEATER_WTR, .ddm_parameter=HTR0GASWTRHST, .bitmask=0x0, .min=0,      .max=3, .gain=1,   .offset=0,   .enum_size=5, .enum_table=rvc_gas_wtr_heating_to_system_value},
	{.variable=VAR_DGN_130557_AC_HEATER_WTR,  .ddm_parameter=HTR0ACWTRHST,  .bitmask=0x0, .min=0,      .max=3, .gain=1,   .offset=0,   .enum_size=4, .enum_table=rvc_ac_wtr_heating_to_system_value},
	// DGN 131071
	{.variable=VAR_DGN_131071_YEAR,     .ddm_parameter=HTR0DATEY, .bitmask=0x0, .min=20,  .max=250, .def=20, .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_131071_MONTH,    .ddm_parameter=HTR0DATEM, .bitmask=0x0, .min=1,   .max=12,  .def=1,  .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_131071_DAY,      .ddm_parameter=HTR0DATED, .bitmask=0x0, .min=1,   .max=31,  .def=1,  .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_131071_HOUR,     .ddm_parameter=HTR0TIMEH, .bitmask=0x0, .min=0,   .max=23,  .def=0,  .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_131071_MINUTE,   .ddm_parameter=HTR0TIMEM, .bitmask=0x0, .min=0,   .max=59,  .def=0,  .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_131071_SECOND,   .ddm_parameter=HTR0TIMES, .bitmask=0x0, .min=0,   .max=59,  .def=0,  .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
	{.variable=VAR_DGN_131071_TIMEZONE, .ddm_parameter=HTR0TTZ,   .bitmask=0x0, .min=0,   .max=253, .def=0,  .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
	// DGN 65259 
	{.variable=VAR_DGN_65259_MODEL,     .ddm_parameter=HTR0MDL,   .bitmask=0x0, .min=0,   .max=65535, .def=0,  .gain=1,   .offset=0,    .enum_size=0, .enum_table=NULL},
};

static void convert_rvc_bitmask_to_ddm_system_value(uint32_t ddm_parameter, uint32_t shift, int32_t *i32Value)
{
	const conversion_table_t *ptr = &conv_table[0];
	int32_t sum = 0;
	int32_t value;
	int32_t bitmask = 0;

	for (int i=0; i < (int)ELEMENTS(conv_table); i++, ptr++)
	{
		if (ptr->ddm_parameter == ddm_parameter)
		{
			value = PIMAGE_GetValue(ptr->variable);
			if ((value < ptr->min) || (value > ptr->max))
			{
				// The value is outside limits, use default value
				value = ptr->def;
			}

			value &= ptr->bitmask;
			value <<= ptr->shift;
			sum += value;
			if (shift == ptr->shift)
			{
				// Store bitmask to be used in later calculations
				bitmask = ptr->bitmask;
			}
		}
	}
	
	if (*i32Value == 0)
	{
		sum &= (int32_t)(~(bitmask << shift));
	}
	else
	{
		sum |= (*i32Value) << shift;
	}

	*i32Value = sum;	
}

int convert_rvc_to_ddm_system_value(int32_t variable, int32_t* i32Value)
{
	int result = 0;
	const conversion_table_t *ptr = &conv_table[0];
	int factor;
	int index;

	for (int i=0; i < (int)ELEMENTS(conv_table); i++, ptr++)
	{
		if (ptr->variable == variable)
		{
			if (ptr->variable == VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME)
				LOG(I, "water keep on time =%d",(*i32Value));

			if (((*i32Value) < ptr->min) || ((*i32Value) > ptr->max))
			{
				// The value is outside limits, use default value
				*i32Value = ptr->def;
			}

			if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(ptr->ddm_parameter))) == -1)
			{
				return 0;
			}
			
			factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].out_unit];

			if (ptr->enum_size == 0)
			{
				if (ptr->bitmask != 0)
				{
					// This value is part of a bit mask so collect the bits together
					convert_rvc_bitmask_to_ddm_system_value(ptr->ddm_parameter, ptr->shift, i32Value);
				}
				else
				{
					if (!factor)
					{
						factor = 1;
					}

					// Increase with factor first to avoid loosing accuracy
					(*i32Value) *= factor;
					if (ptr->gain < 0)
					{
						*i32Value = (((*i32Value) * abs(ptr->gain)) + ptr->offset);
					}
					else
					{
						*i32Value = (((*i32Value) / ptr->gain) + ptr->offset);
					}
				}
				
				result = 1;
			}
			else
			{
				for (int j=0; j<ptr->enum_size; j++)
				{
					if (ptr->enum_table[j].domain_value == (*i32Value))
					{
						*i32Value = ptr->enum_table[j].system_value;
						result = 1;
						break;
					}
				}
			}

			break;
		}
	}
	return result;
}

int convert_ddm_system_value_to_rvc_value(uint32_t parameter, int32_t* i32Value)
{
	int result = 0;
	const conversion_table_t *ptr = &conv_table[0];
	int factor;
	int index;
	int32_t value;

	if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) == -1)
	{
		return 0;
	}

	factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].in_unit];


	for (int i=0; i < (int)ELEMENTS(conv_table); i++, ptr++)
	{
		if (ptr->ddm_parameter == parameter)
		{
			if (ptr->enum_size == 0)
			{
				if (ptr->gain < 0)
				{
					value = (*i32Value) / abs(ptr->gain);
				}
				else
				{
					value = (*i32Value) * ptr->gain; // Increase with gain first to avoid loosing accuracy
				}
				if (factor)
				{
					value /= factor;
				}
				value += ptr->offset;
				*i32Value = value;
				result = 1;
			}
			else
			{
				for (int j=0; j<ptr->enum_size; j++)
				{
					if (ptr->enum_table[j].system_value == (*i32Value))
					{
						*i32Value = ptr->enum_table[j].domain_value;
						result = 1;
						break;
					}
				}
			}

			break;
		}
	}
	return result;
}

/**
 * @brief Fuction to get DDM parameter ID from conv_table based on Pimage variable
 *
 * @param index Pimage variable ingex.
 */
int32_t get_parameter_by_pimage_index(uint32_t index)
{
	int32_t ddm_parameter = -1;
	for (size_t instance = 0; instance < ELEMENTS(conv_table); instance++)
	{
		if (conv_table[instance].variable == (int32_t)index)
		{
			ddm_parameter = conv_table[instance].ddm_parameter;
		}
	}

	return ddm_parameter;
}
