#include "tenhaaft.h"
#include "frame_util.h"
#include <string.h>

//-----------------------------------------------------------------------------
// Function:    tenhaaft_sat_pid_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Tenhaaft Satelite PID
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void tenhaaft_sat_pid_diag_I_Extract
(
    tenhaaft_sat_pid_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,         pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,         pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,         pu8Data[2], 0, 8 );
    DGN_TO_WORD( pInfo->supplier_id, pu8Data[3]);
    DGN_TO_WORD( pInfo->func_id,     pu8Data[5]);
    DGN_TO_BITS( pInfo->var_id,      pu8Data[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    tenhaaft_generic_negative_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic TENHAAFT Negative
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void tenhaaft_generic_negative_I_Extract
(
    tenhaaft_gen_neg_diag_info_t*    pInfo,
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

