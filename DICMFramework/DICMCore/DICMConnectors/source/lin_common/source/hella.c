#include "hella.h"
#include "frame_util.h"
#include "pstore.h"
#include <string.h>

//-----------------------------------------------------------------------------
// Function:    hella_ibs_frm2_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from IBS frame2 frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_ibs_frm2_I_Extract
(
	hella_ibs_frm2_t*             pInfo,     // out: Extracted parameters
    uint8_t*                      pu8Data    // in : DGN frame buffer
)
{
	uint8_t temp1_u8;
	uint8_t temp2_u8 = 0;
	uint8_t temp3_u8 = 0;

    //           dest                      src
    DGN_TO_WORD( pInfo->battery_current,   pu8Data[0]);
    DGN_TO_BITS( temp1_u8,                 pu8Data[2], 0, 8 );
    DGN_TO_WORD( pInfo->battery_voltage,   pu8Data[3]);
    DGN_TO_BITS( temp2_u8,                 pu8Data[5], 0, 8 );
    DGN_TO_BITS( temp3_u8,                 pu8Data[6], 0, 1 );
    DGN_TO_BITS( pInfo->ibs_error,         pu8Data[6], 7, 1 );

	pInfo->battery_current |= ((uint32_t)temp1_u8 << 16);
	pInfo->battery_temperature = (((uint16_t)temp3_u8 << 8) | temp2_u8);
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_frm5_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from IBS frame5 frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_ibs_frm5_I_Extract
(
	hella_ibs_frm5_t*             pInfo,     // out: Extracted parameters
    uint8_t*                      pu8Data    // in : DGN frame buffer
)
{
    //           dest                      src
    DGN_TO_BITS( pInfo->state_of_charge,   pu8Data[0], 0, 8);
    DGN_TO_BITS( pInfo->state_of_health,   pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->est_voltage_drop,  pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->opt_charging_volt, pu8Data[3], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_frm6_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from IBS frame6 frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_ibs_frm6_I_Extract
(
	hella_ibs_frm6_t*             pInfo,     // out: Extracted parameters
    uint8_t*                      pu8Data    // in : DGN frame buffer
)
{
    //           dest                      src
    DGN_TO_WORD( pInfo->avail_capacity,    pu8Data[0]);
    DGN_TO_WORD( pInfo->discharge_ah,      pu8Data[2]);
    DGN_TO_BITS( pInfo->nominal_capacity,  pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->recalibrated,      pu8Data[5], 0, 1 );
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_temperature_convert
//-----------------------------------------------------------------------------
// Description: Convert battery temp from LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t hella_ibs_temperature_convert(int32_t i32Value)
{
	int32_t temp = -40 + (((i32Value & 0x1ff) * 5) / 10);

	return temp;
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_current_convert
//-----------------------------------------------------------------------------
// Description: Convert battery current from LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t hella_ibs_current_convert(int32_t i32Value)
{
	// Convert according to spec, mA
	int32_t current = -2000000 + (i32Value & 0xFFFFFF);

	return current;
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_soc_convert
//-----------------------------------------------------------------------------
// Description: Convert battery State of Charge from LIN value to system value
//              before publishing.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t hella_ibs_soc_convert(int32_t i32Value)
{
	// Convert according to spec, percent
	int32_t proc = i32Value / 2;

	return proc;
}

//-----------------------------------------------------------------------------
// Function:    hella_batcapread_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery Cap Read Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_batcapread_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_bcap_re_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_batcapwrite_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery Cap Write Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_batcapwrite_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_bcap_wr_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->C_Nom, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_battyperead_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery Type Read Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_battyperead_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_btype_re_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_battypewrite_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery Type Write Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_battypewrite_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_btype_wr_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_battablestate_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery Table State Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_battablestate_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_btable_st_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_pid_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Product Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_pid_diag_I_Extract
(
    hella_pid_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,           pu8Data[2], 0, 8 );
    DGN_TO_WORD( pInfo->supplier_id,    pu8Data[3]);
    DGN_TO_WORD( pInfo->func_id,        pu8Data[5]);
    DGN_TO_BITS( pInfo->var_id,         pu8Data[7], 0, 8);
}
//-----------------------------------------------------------------------------
// Function:    hella_batcapread_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery Cap Read Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_batcapread_diag_I_Extract
(
    hella_bcap_re_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data1,       pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data2,       pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,       pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,       pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_batcapwrite_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery Cap Write Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_batcapwrite_diag_I_Extract
(
    hella_bcap_wr_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->C_Nom,       pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data2,       pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,       pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,       pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_battyperead_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery Type Read Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_battyperead_diag_I_Extract
(
    hella_btype_re_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data1,       pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data2,       pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,       pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,       pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_battypewrite_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery Type Write Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_battypewrite_diag_I_Extract
(
    hella_btype_wr_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data1,       pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data2,       pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,       pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,       pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_battablestate_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery Table State Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_battablestate_diag_I_Extract
(
    hella_btable_st_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data1,       pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data2,       pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,       pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,       pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_btableonoff_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery Table On Off Read Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_btableonoff_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_btable_onoff_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_bu0minmaxread_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery U0 Minimum and Maximum Read
//              Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_bu0minmaxread_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_bu0minmax_re_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_bu0minmaxwrite_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery U0 Minimum and Maximum
//              Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_bu0minmaxwrite_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_bu0minmax_wr_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    WORD_TO_DGN( pu8Data[4],  pCtrl->u0_min);
    WORD_TO_DGN( pu8Data[6],  pCtrl->u0_max);
}

//-----------------------------------------------------------------------------
// Function:    hella_ibattquiescentread_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery QIescent Read Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_ibattquiescentread_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_ibquiescent_re_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_ibattquiescentwrite_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuffing the Diagnostic Hella Battery Quiescent Write Master Request
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_ibattquiescentwrite_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    hella_ibquiescent_wr_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_HELLA_BATTERY_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->lid, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->ibattquiescent, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->ichargemin, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}


//-----------------------------------------------------------------------------
// Function:    hella_btableonoff_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery Table On Off Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_btableonoff_diag_I_Extract
(
    hella_btable_onoff_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data1,       pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data2,       pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,       pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,       pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_bu0minmaxread_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery U0 Minimum and Maximum
//              Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_bu0minmaxread_diag_I_Extract
(
    hella_bu0minmax_re_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_WORD( pInfo->u0min,       pu8Data[3] );
    DGN_TO_WORD( pInfo->u0max,       pu8Data[5] );
    DGN_TO_BITS( pInfo->data4,       pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_bu0minmaxwrite_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery U0 Minimum and Maximum
//              Write Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_bu0minmaxwrite_diag_I_Extract
(
    hella_bu0minmax_wr_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,         pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,         pu8Data[3], 0, 8 );
    DGN_TO_WORD( pInfo->u0min,       pu8Data[4]);
    DGN_TO_WORD( pInfo->u0max,       pu8Data[6]);
}

//-----------------------------------------------------------------------------
// Function:    hella_ibattquiescentread_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery I Battery Qiescent Read
//              Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void hella_ibattquiescentread_diag_I_Extract
(
    hella_ibquiescent_re_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,                pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,                pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->ibattquiescent,     pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->ichargemin,         pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data2,              pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,              pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,              pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_ibattquiescentwrite_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract the Diagnostic Hella Battery Iquiescent Write Information
//
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void hella_ibattquiescentwrite_diag_I_Extract
(
    hella_ibquiescent_wr_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,                pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,                pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->lid,                pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->ibattquiescent,     pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->ichargemin,         pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data3,              pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data4,              pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_battype_set
//-----------------------------------------------------------------------------
// Description: Set battery type from system value to LIN value.
//-----------------------------------------------------------------------------
// Return:      Zero for failure
//-----------------------------------------------------------------------------
int32_t hella_ibs_battype_set(pstore_table_t eIndex, int32_t i32Value)
{
	/* Valid range 0 - 2 */

    switch (i32Value)
    {
        case 1:
		    pstore_SetValue(eIndex, 0x14); // GEL
            break;
        case 2:
		    pstore_SetValue(eIndex, 0x1E); // AGM
            break;
        case 0:
        default:
		    pstore_SetValue(eIndex, 0xA);  // Flooded (=Starter)
            break;
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_battype_convert
//-----------------------------------------------------------------------------
// Description: Convert battery type from LIN value to system value.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t hella_ibs_battype_convert(int32_t i32Value)
{
    int32_t result;

    switch (i32Value)
    {
        case 0x14:
		    result = 0x1; // GEL
            break;
        case 0x1E:
		    result = 0x2; // AGM
            break;
        case 0x4:
        default:
		    result = 0;  // Flooded (=Starter)
            break;
    }

    return result;
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_cnominal_set
//-----------------------------------------------------------------------------
// Description: Set battery C nominal from system value to LIN value.
//-----------------------------------------------------------------------------
// Return:      Zero for failure
//-----------------------------------------------------------------------------
int32_t hella_ibs_cnominal_set(pstore_table_t eIndex, int32_t i32Value)
{
	/* Valid range max 500 Ah (for MCA-HS1 250 Ah) */
    if (1 /*MCA-HS2*/)
    {
        pstore_SetValue(eIndex, (uint8_t)(i32Value/2));
    }
    else
    {
        pstore_SetValue(eIndex, (uint8_t)i32Value);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Function:    hella_ibs_cnominal_convert
//-----------------------------------------------------------------------------
// Description: Convert battery C nominal from LIN value to system value.
//-----------------------------------------------------------------------------
// Return:      Converted value
//-----------------------------------------------------------------------------
int32_t hella_ibs_cnominal_convert(int32_t i32Value)
{
	/* Valid range max 500 Ah (for MCA-HS1 250 Ah) */
    if (1 /*MCA-HS2*/)
    {
        return i32Value * 2;
    }
    else
    {
        return i32Value;
    }
}
