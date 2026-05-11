#include "teleair.h"
#include "frame_util.h"
#include <string.h>

//-----------------------------------------------------------------------------
// Function:    telair_ac_pid_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic PID info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------

void telair_ac_pid_diag_I_Extract
(
    telair_ac_pid_diag_info_t*    pInfo,
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
// Function:    telair_generic_ucom_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Telair Generic User Command 32
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void telair_generic_ucom_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    telair_gen_ucom_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TELEAIR_AC_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->ucommand, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data4, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data5, 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    telair_ac_ucom32_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic User Command 32
//              info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void telair_ac_ucom32_diag_I_Extract
(
    telair_ac_ucom32_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,           pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->custom,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,          pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,          pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,          pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,          pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    telair_ac_ucom33_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic User Command 33
//              info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void telair_ac_ucom33_diag_I_Extract
(
    telair_ac_ucom33_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,                pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,                pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->hw_type,            pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->hw_ver,             pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->fw_type,            pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->fw_ver,             pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->fw_sub_ver,         pu8Data[7], 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    telair_generic_negative_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleair Generic negative
//              info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void telair_generic_negative_diag_I_Extract
(
    telair_gen_neg_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,             pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,             pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,             pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->req_sid,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,           pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,           pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,           pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,           pu8Data[7], 0, 8 );
}
