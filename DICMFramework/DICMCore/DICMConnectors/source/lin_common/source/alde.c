#include "alde.h"
#include "frame_util.h"
#include <string.h>

//-----------------------------------------------------------------------------
// Function:    alde_3020_pid_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic ALDE 3020 PID
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void alde_3020_pid_diag_I_Extract
(
    alde_3020_pid_diag_info_t*    pInfo,
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
// Function:    alde_3020_assignnad_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for alde3020 Assign NAD
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void alde_3020_assignnad_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    alde_3020_nad_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, ALDE_REQ_FRAME_SIZE_DIAG );

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
// Function:    alde_3020_assignnad_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic alde 3020 Assign NAD
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void alde_3020_assignnad_diag_I_Extract
(
    alde_3020_nad_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,            pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,            pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,            pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->data1,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,          pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,          pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,          pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,          pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    alde_3020_assignframe_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Alde 3020 Assign Frame
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void alde_3020_assignframe_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    alde_3020_frame_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, ALDE_REQ_FRAME_SIZE_DIAG );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->nad, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->pci, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->sid, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->start_index, 0, 8 );
    DWRD_TO_DGN( pu8Data[4],  pCtrl->product_id );
}

//-----------------------------------------------------------------------------
// Function:    alde_3020_assignframe_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Alde 3020 Assign Frame
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void alde_3020_assignframe_diag_I_Extract
(
    alde_3020_frame_diag_info_t*    pInfo,
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
// Function:    alde_negative_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic ALDE Negative
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void alde_negative_diag_I_Extract
(
    alde_gen_neg_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,              pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,              pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->sid,              pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->req_sid,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,            pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,            pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,            pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,            pu8Data[7], 0, 8 );
}
