#include "truma.h"
#include "frame_util.h"
#include <string.h>

//-----------------------------------------------------------------------------
// Function:    truma_water_heater_C_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Water Heater Ctrl frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_water_heater_C_Extract
(
	truma_waterheater_ctrl_t* pCtrl,     // out: Extracted parameters
    uint8_t*              pu8Data        // in : DGN frame buffer
)
{
    //           dest                          src
    DGN_TO_WORD( pCtrl->temp,      	pu8Data[0]  );
    DGN_TO_BITS( pCtrl->energy_sel, pu8Data[2], 0, 8 );
    DGN_TO_WORD( pCtrl->power_lim,  pu8Data[3]  );
}

//-----------------------------------------------------------------------------
// Function:    truma_water_heater_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Water Heater Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_water_heater_I_Extract
(
	truma_waterheater_info_t* pInfo,     // out: Extracted parameters
    uint8_t*              pu8Data        // in : DGN frame buffer
)
{
    //           dest                          src
    DGN_TO_WORD( pInfo->temp,      	 pu8Data[0]  );
    DGN_TO_BITS( pInfo->energy_sel,  pu8Data[2], 0, 8 );
    DGN_TO_WORD( pInfo->power_lim,   pu8Data[3]  );
	DGN_TO_BITS( pInfo->frost_ctrl,  pu8Data[5], 0, 1 );
	DGN_TO_BITS( pInfo->el_aval,     pu8Data[5], 1, 1 );
	DGN_TO_BITS( pInfo->sw_clr,      pu8Data[5], 2, 1 );
	DGN_TO_BITS( pInfo->manual_mode, pu8Data[5], 3, 1 );
}

//-----------------------------------------------------------------------------
// Function:    truma_air_heater_C_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Air Heater Ctrl frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_air_heater_C_Extract
(
	truma_airheater_ctrl_t* pCtrl,     // out: Extracted parameters
    uint8_t*              pu8Data        // in : DGN frame buffer
)
{
    //           dest                          src
    DGN_TO_WORD( pCtrl->temp,      	pu8Data[0]  );
    DGN_TO_BITS( pCtrl->energy_sel, pu8Data[3], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    truma_air_heater_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Air Heater Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_air_heater_I_Extract
(
	truma_airheater_info_t* pInfo,     // out: Extracted parameters
    uint8_t*              pu8Data        // in : DGN frame buffer
)
{
    //           dest                          src
    DGN_TO_WORD( pInfo->temp,      	pu8Data[0]  );
    DGN_TO_BITS( pInfo->energy_sel, pu8Data[4], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    truma_air_heater_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Air heater into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_air_heater_C_Stuff
(
    uint8_t*              pu8Data,    // out: LIN frame buffer
	truma_airheater_ctrl_t* pCtrl     // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, TRUMA_AIR_HEATER_SIZE );

	// Fill
    //           dest         source
    WORD_TO_DGN( pu8Data[0],  pCtrl->temp  );
    BITS_TO_DGN( pu8Data[3],  pCtrl->energy_sel, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    truma_water_heater_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Water heater into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_water_heater_C_Stuff
(
    uint8_t*              pu8Data,    // out: LIN frame buffer
	truma_waterheater_ctrl_t* pCtrl   // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, TRUMA_WATER_HEATER_SIZE );

	// Fill
    //           dest         source
    WORD_TO_DGN( pu8Data[0],  pCtrl->temp );
    BITS_TO_DGN( pu8Data[2],  pCtrl->energy_sel, 0, 8 );
    WORD_TO_DGN( pu8Data[3],  pCtrl->power_lim );
}

//-----------------------------------------------------------------------------
// Function:    truma_heater_energy_selection_set
//-----------------------------------------------------------------------------
// Description: Set heater energy selection from system value to LIN value.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
int32_t truma_heater_energy_selection_set(pstore_table_t eIndex, int32_t i32Value)
{
    uint16_t tmp_pl;

    (void)eIndex; // Not used in this special function

	/* Valid range 0=Gas, 1=Gas+El900W, 2=Gas+El1800W, 3=El900W, 4=El1800W */

    // Get value of current power limit reading from info frame
    tmp_pl = pstore_u16GetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_I);

    // Gas/El/Both?
    if (i32Value == 0)
    {
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C, 0x1); // Fuel/Gas
        pstore_SetValue(VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C, 0x1);
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C, tmp_pl); // Dont care?
    }
    else if (i32Value == 1)
    {
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C, 0x3); // Both
        pstore_SetValue(VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C, 0x3);
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C, 900);
    }
    else if (i32Value == 2)
    {
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C, 0x3); // Both
        pstore_SetValue(VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C, 0x3);
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C, 1800);
    }
    else if (i32Value == 3)
    {
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C, 0x2); // Electricity
        pstore_SetValue(VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C, 0x2);
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C, 900);
    }
    else if (i32Value == 4)
    {
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C, 0x2); // Electricity
        pstore_SetValue(VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C, 0x2);
        pstore_SetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C, 1800);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Function:    truma_heater_energy_selection_convert
//-----------------------------------------------------------------------------
// Description: Convert heater energy selection LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t truma_heater_energy_selection_convert(int32_t i32Value)
{
	int32_t res = 0;
    uint16_t tmp_pl;

    tmp_pl = pstore_u16GetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_I);

	/* Valid range 1=Fuel, 2=El, 3=Both */    
    if (i32Value == 1)
    {
        res = 0;
    }
    else if ((i32Value == 2) && (tmp_pl = 900))
    {
        res = 3;
    }
    else if ((i32Value == 2) && (tmp_pl = 1800))
    {
        res = 4;
    }
    else if ((i32Value == 3) && (tmp_pl = 900))
    {
        res = 1;
    }
    else if ((i32Value == 3) && (tmp_pl = 1800))
    {
        res = 2;
    }

	return res;
}

//-----------------------------------------------------------------------------
// Function:    truma_heater_power_limit_convert
//-----------------------------------------------------------------------------
// Description: Convert heater power limit LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t truma_heater_power_limit_convert(int32_t i32Value)
{
	int32_t res = 0;
    uint8_t tmp_esel;

    tmp_esel = pstore_u8GetValue(VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C);
    
	/* Wanted output 0=Gas, 1=Gas+El900W, 2=Gas+El1800W, 3=El900W, 4=El1800W*/

    /* Electricity selection 1=Fuel, 2=El, 3=Both */
    if ((i32Value == 900) && (tmp_esel == 2))
    {
        res = 3;
    }
    else if ((i32Value == 900) && (tmp_esel == 3))
    {
        res = 1;
    }
    else if ((i32Value == 1800) && (tmp_esel == 2))
    {
        res = 4;
    }
    else if ((i32Value == 1800) && (tmp_esel == 3))
    {
        res = 2;
    }

	return res;
}

//-----------------------------------------------------------------------------
// Function:    truma_wtr_heater_set
//-----------------------------------------------------------------------------
// Description: Set water heater target temp from system value to LIN value.
//-----------------------------------------------------------------------------
// Return:      Zero for failure
//-----------------------------------------------------------------------------
int32_t truma_wtr_heater_set(pstore_table_t eIndex, int32_t i32Value)
{
	/* Valid range OFF=-273°C=0 (0x0000) ECO=40°C=1 HOT=60°C=2 */

	if (i32Value == 0)
	{
		pstore_SetValue(eIndex, 0);
	}
	else if (i32Value == 1)
	{
		i32Value = 0x0C3A;
		pstore_SetValue(eIndex, i32Value);
	}
    else if (i32Value == 2)
    {
		i32Value = 0x0D02;
		pstore_SetValue(eIndex, i32Value);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Function:    truma_water_heater_convert
//-----------------------------------------------------------------------------
// Description: Convert water heater target temp from LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t truma_water_heater_convert(int32_t i32Value)
{
	int32_t res = 0;

	/* Valid range OFF = -273°C (0x0000) 40°C … 60°C (0x0C3A … 0x0D02), 0.1 ^C/bit */

	if (i32Value >= 0x0C3A && i32Value <= 0xD02)
	{
		res = ((i32Value - 0x0ADC) + 50) / 10;
		res &= 0xFF;
	}

    // ECO
    if (res == 40)
    {
        res = 1;
    }
    // Hot
    else if (res == 60)
    {
        res = 2;
    }
    // Off
    else
    {
        res = 0;
    }

	return res;
}

//-----------------------------------------------------------------------------
// Function:    truma_water_heater_status_set
//-----------------------------------------------------------------------------
// Description: Set water heater on/off from system value to LIN value.
//-----------------------------------------------------------------------------
// Return:      Zero for failure
//-----------------------------------------------------------------------------
int32_t truma_water_heater_status_set(pstore_table_t eIndex, int32_t i32Value)
{
	/* Valid range 0 or 1*/

	if (i32Value == 0)
	{
		pstore_SetValue(eIndex, i32Value);
	}
	else if (i32Value == 1)
	{
		i32Value = 0x0C3A;
		pstore_SetValue(eIndex, i32Value);
	}

    return 1;
}

//-----------------------------------------------------------------------------
// Function:    truma_air_heater_status_set
//-----------------------------------------------------------------------------
// Description: Set air heater on/off from system value to LIN value.
//-----------------------------------------------------------------------------
// Return:      Zero for failure
//-----------------------------------------------------------------------------
int32_t truma_air_heater_status_set(pstore_table_t eIndex, int32_t i32Value)
{
	/* Valid range OFF = -273°C (0x0000) 5°C … 30°C (0x0ADC … 0x0BD6), 0.1 ^C/bit */

	if (i32Value == 0)
	{
		pstore_SetValue(eIndex, i32Value);
	}
	else if (i32Value == 1)
	{
		i32Value = 0x0ADC;
		pstore_SetValue(eIndex, i32Value);
	}

    return 1;
}

//-----------------------------------------------------------------------------
// Function:    truma_air_heater_set
//-----------------------------------------------------------------------------
// Description: Set air heater target temp from system value to LIN value.
//-----------------------------------------------------------------------------
// Return:      Zero for failure
//-----------------------------------------------------------------------------
int32_t truma_air_heater_set(pstore_table_t eIndex, int32_t i32Value)
{
	/* Valid range OFF = -273°C (0x0000) 5°C … 30°C (0x0ADC … 0x0BD6), 0.1 ^C/bit */

	if (i32Value == 0)
	{
		pstore_SetValue(eIndex, 0);
	}
	else if (i32Value >= 5 && i32Value <= 30)
	{
		i32Value = 0x0ADC + ((i32Value - 5) * 10);
		pstore_SetValue(eIndex, i32Value);
	}

    return 1;
}

//-----------------------------------------------------------------------------
// Function:    truma_air_heater_convert
//-----------------------------------------------------------------------------
// Description: Convert Air heater target temp from LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t truma_air_heater_convert(int32_t i32Value)
{
	int32_t res = 0;

	/* Valid range OFF = -273°C (0x0000) 5°C … 30°C (0x0ADC … 0x0BD6), 0.1 ^C/bit */

	if (i32Value >= 0xADC && i32Value <= 0xBD6)
	{
		res = ((i32Value - 0x0ADC) + 50) / 10;
		res &= 0xFF;
	}

	return res;
}

//-----------------------------------------------------------------------------
// Function:    truma_air_heater_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Air heater into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_air_heater_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    truma_airheater_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TRUMA_REQ_FRAME_SIZE_DIAG );

	// Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
	BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
	BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->id, 0, 8 );
    WORD_TO_DGN( pu8Data[4],  pCtrl->supplier_id );
	WORD_TO_DGN( pu8Data[6],  pCtrl->func_id );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_pid_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic TRUMA Generic PID
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_pid_diag_I_Extract
(
    truma_gen_pid_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,        pu8Data[2], 0, 8 );
    DGN_TO_WORD( pInfo->supplier_id, pu8Data[3]);
    DGN_TO_WORD( pInfo->func_id,     pu8Data[5]);
    DGN_TO_BITS( pInfo->var_id,      pu8Data[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_serial_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Truma Generic Serial
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_serial_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    truma_gen_serial_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TRUMA_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->data1, 0, 8 );
    WORD_TO_DGN( pu8Data[4],  pCtrl->supplier_id );
    WORD_TO_DGN( pu8Data[6],  pCtrl->func_id );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_serial_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic TRUMA Generic Serial
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_serial_diag_I_Extract
(
    truma_gen_serial_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,           pu8Data[2], 0, 8 );
    DGN_TO_DWRD( pInfo->serial_number,  pu8Data[3]);
    DGN_TO_BITS( pInfo->data8,          pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_assignnad_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Truma Generic Assign NAD
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_assignnad_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    truma_gen_nad_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TRUMA_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    WORD_TO_DGN( pu8Data[3],  pCtrl->supplier_id );
    WORD_TO_DGN( pu8Data[5],  pCtrl->func_id );
    BITS_TO_DGN( pu8Data[7],  pCtrl->new_nad, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_assignnad_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Truma Generic Assign NAD
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_assignnad_diag_I_Extract
(
    truma_gen_nad_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,           pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->data1,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,          pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,          pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,          pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,          pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    truma_generic_assignframe_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Truma Generic Assign Frame
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_assignframe_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    truma_gen_frame_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TRUMA_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->start_index, 0, 8 );
    DWRD_TO_DGN( pu8Data[4],  pCtrl->product_id );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_assignframe_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Generic Assign Frame
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_assignframe_diag_I_Extract
(
    truma_gen_frame_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,           pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->data1,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,          pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,          pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,          pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,          pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_currenterror_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Truma AC Current Error
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_currenterror_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    truma_gen_cerror_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TRUMA_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->id, 0, 8 );
    WORD_TO_DGN( pu8Data[4],  pCtrl->suppliar_id );
    WORD_TO_DGN( pu8Data[6],  pCtrl->function_id );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_currenterror_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Truma AC Current Error
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_currenterror_diag_I_Extract
(
    truma_gen_cerror_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,           pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->format,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->clas,           pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,          pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->code,           pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,          pu8Data[7], 0, 8 );
}


//-----------------------------------------------------------------------------
// Function:    truma_generic_negative_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic TRUMA Generic Negative
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_negative_diag_I_Extract
(
    truma_gen_neg_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,              pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,              pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,             pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->req_sid,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,            pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,            pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,            pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,            pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_fwver_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Truma AC Firmware Version
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_fwver_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    truma_gen_fwver_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TRUMA_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->id, 0, 8 );
    WORD_TO_DGN( pu8Data[4],  pCtrl->suppliar_id );
    WORD_TO_DGN( pu8Data[6],  pCtrl->function_id );
}

//-----------------------------------------------------------------------------
// Function:    truma_generic_fwver_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Truma Generic Firmware Version
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void truma_generic_fwver_diag_I_Extract
(
    truma_gen_fwver_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,           pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->major,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->minor,          pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->revision,       pu8Data[5], 0, 8 );
    DGN_TO_WORD( pInfo->build,          pu8Data[6]);

}
