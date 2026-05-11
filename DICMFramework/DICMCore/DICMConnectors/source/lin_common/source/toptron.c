#include "toptron.h"
#include "frame_util.h"
#include <string.h>

//-----------------------------------------------------------------------------
// Function:    toptron_lightbox_C_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Lightbox Ctrl frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void toptron_lightbox_C_Extract
(
	toptron_lightbox_ctrl_t*      pCtrl,     // out: Extracted parameters
    uint8_t*                      pu8Data    // in : DGN frame buffer
)
{
    //           dest                      src
    DGN_TO_BITS( pCtrl->light_bed_left,    pu8Data[3], 0, 4 );
    DGN_TO_BITS( pCtrl->light_bed_right,   pu8Data[3], 4, 4 );
    DGN_TO_BITS( pCtrl->light_ceiling,     pu8Data[4], 0, 4 );
    DGN_TO_BITS( pCtrl->light_wall,        pu8Data[4], 4, 4 );
}

//-----------------------------------------------------------------------------
// Function:    toptron_lightbox_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Lightbox Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void toptron_lightbox_I_Extract
(
	toptron_lightbox_info_t*      pInfo,     // out: Extracted parameters
    uint8_t*                      pu8Data    // in : DGN frame buffer
)
{
    //           dest                      src
    DGN_TO_BITS( pInfo->temp_out,          pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->temp_in,           pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->bat_voltage,       pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->fresh_wtr_lvl,     pu8Data[3], 0, 4 );
    DGN_TO_BITS( pInfo->light_bed_left,    pu8Data[3], 4, 4 );
    DGN_TO_BITS( pInfo->light_bed_right,   pu8Data[4], 0, 4 );
    DGN_TO_BITS( pInfo->light_ceiling,     pu8Data[4], 4, 4 );
    DGN_TO_BITS( pInfo->light_wall,        pu8Data[5], 0, 4 );
    DGN_TO_BITS( pInfo->mains_connected,   pu8Data[7], 7, 1 );
}

//-----------------------------------------------------------------------------
// Function:    toptron_lightbox_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Lightbox into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void toptron_lightbox_C_Stuff
(
    uint8_t*                      pu8Data,  // out: LIN frame buffer
	toptron_lightbox_ctrl_t* pCtrl     // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, TOPTRON_LIGHTBOX_CTRL_SIZE );

	// Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[3],  pCtrl->light_bed_left,  0, 4 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->light_bed_right, 4, 4 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->light_ceiling,   0, 4 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->light_wall,      4, 4 );
}

//-----------------------------------------------------------------------------
// Function:    toptron_charger_C_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Charger X Ctrl frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void toptron_charger_C_Extract
(
	toptron_el603_charger_ctrl_t* pCtrl,     // out: Extracted parameters
    uint8_t*                      pu8Data    // in : DGN frame buffer
)
{
    //           dest                      src
    DGN_TO_BITS( pCtrl->charging_voltage,  pu8Data[0], 0, 8 );
    DGN_TO_WORD( pCtrl->battery_voltage,   pu8Data[1] );
    DGN_TO_BITS( pCtrl->silent_mode,       pu8Data[3], 0, 1 );
}

//-----------------------------------------------------------------------------
// Function:    toptron_charger_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Charger X Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void toptron_charger_I_Extract
(
	toptron_el603_charger_info_t* pInfo,     // out: Extracted parameters
    uint8_t*                      pu8Data    // in : DGN frame buffer
)
{
    //           dest                      src
    DGN_TO_BITS( pInfo->charging_current,  pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->silent_mode,       pu8Data[1], 0, 1 );
	DGN_TO_BITS( pInfo->reduced_power,     pu8Data[1], 1, 1 );
	DGN_TO_BITS( pInfo->error_active,      pu8Data[1], 2, 1 );
	DGN_TO_BITS( pInfo->charging_acitve,   pu8Data[1], 3, 1 );
}

//-----------------------------------------------------------------------------
// Function:    toptron_charger_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Charger X into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void toptron_charger_C_Stuff
(
    uint8_t*                      pu8Data,  // out: LIN frame buffer
	toptron_el603_charger_ctrl_t* pCtrl     // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, TOPTRON_CHARGER_CTRL_SIZE );

	// Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->charging_voltage, 0, 8 );
    WORD_TO_DGN( pu8Data[1],  pCtrl->battery_voltage );
    BITS_TO_DGN( pu8Data[3],  pCtrl->silent_mode, 0, 1 );
}

//-----------------------------------------------------------------------------
// Function:    toptron_charger_current_convert
//-----------------------------------------------------------------------------
// Description: Convert current from LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t toptron_charger_current_convert(int32_t i32Value)
{
	// Convert according to spec, but to mA
	int32_t current = ((i32Value * 2000) / 10);

	return current;
}
