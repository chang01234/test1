#include "kathrein.h"
#include "frame_util.h"
#include <string.h>


//-----------------------------------------------------------------------------
// Function:    kathrein_satelite_pid_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic KATHREIN Satelite PID
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void kathrein_satelite_pid_diag_I_Extract
(
    kathrein_sat_pid_diag_info_t*    pInfo,
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
// Function:    kathrein_satelite_sw_ver_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Kathrein Satelite Software
//              version into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void kathrein_satelite_sw_ver_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    kathrein_sat_sw_ver_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, KATHREIN_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->custom, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data4, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    kathrein_satelite_sw_ver_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Kathrein Satelite Software
//              version Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void kathrein_satelite_sw_ver_diag_I_Extract
(
    kathrein_sat_sw_ver_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,              pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,              pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,             pu8Data[2], 0, 8 );
    DGN_TO_WORD( pInfo->lan_code,         pu8Data[3]);
    DGN_TO_BITS( pInfo->month,            pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->date,             pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->year,             pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    kathrein_negative_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic KATHREIN Negative
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void kathrein_negative_diag_I_Extract
(
    kathrein_neg_diag_info_t*    pInfo,
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

