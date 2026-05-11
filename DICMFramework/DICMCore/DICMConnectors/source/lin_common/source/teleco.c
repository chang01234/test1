#include "teleco.h"
#include "frame_util.h"
#include <string.h>


//-----------------------------------------------------------------------------
// Function:    teleco_generic_pid_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic TELECO Generic PID
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_pid_diag_I_Extract
(
    teleco_gen_pid_diag_info_t*    pInfo,
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
// Function:    teleco_generic_ucom_diag_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Teleco generic user command
//              into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom_diag_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    teleco_gen_usr_com_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, TELECO_GEN_REQ_FRAME_SIZE_DIAG );

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
// Function:    teleco_generic_ucom32_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 32
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom32_diag_I_Extract
(
    teleco_gen_ucom32_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,             pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,             pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,            pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->custom,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->data2,           pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->data3,           pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->data4,           pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,           pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    teleco_generic_ucom33_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 33
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom33_diag_I_Extract
(
    teleco_gen_ucom33_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,             pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,             pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,            pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->hw_type,         pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->hw_ver,          pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->fw_type,         pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->fw_ver,          pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->fw_sub_ver,      pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    teleco_generic_ucom34_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 34
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom34_diag_I_Extract
(
    teleco_gen_ucom34_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,                    pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                    pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,                   pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->custom_digit1,          pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->custom_digit0,          pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->hw_ver,                 pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->fw_type,                pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->data5,                  pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    teleco_generic_ucom35_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 35
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom35_diag_I_Extract
(
    teleco_gen_ucom35_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                          src
    DGN_TO_BITS( pInfo->nad,                    pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                    pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,                   pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->fw_ver_major,           pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->fw_ver_minor,           pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->db_digit2,              pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->db_digit1,              pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->db_digit0,              pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    teleco_generic_ucom36_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 36
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom36_diag_I_Extract
(
    teleco_gen_ucom36_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                               src
    DGN_TO_BITS( pInfo->nad,                    pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                    pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,                   pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->hw_type,                pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->hw_ver,                 pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->fw_type,                pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->fw_ver,                 pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->fw_sub_ver,             pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    teleco_generic_ucom37_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 37
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom37_diag_I_Extract
(
    teleco_gen_ucom37_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                               src
    DGN_TO_BITS( pInfo->nad,                        pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                        pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,                       pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->custom_digit1,              pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->custom_digit0,              pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->crit_type1,                 pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->crit_type0,                 pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->crit_ver5,                  pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    teleco_generic_ucom38_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 38
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom38_diag_I_Extract
(
    teleco_gen_ucom38_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                               src
    DGN_TO_BITS( pInfo->nad,                        pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                        pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,                       pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->crit_ver4,                  pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->crit_ver3,                  pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->crit_ver2,                  pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->crit_ver1,                  pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->crit_ver0,                  pu8Data[7], 0, 8 );

}


//-----------------------------------------------------------------------------
// Function:    teleco_generic_ucom39_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco User Command 39
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_ucom39_diag_I_Extract
(
    teleco_gen_ucom39_diag_info_t*    pInfo,
    uint8_t*    pu8Data
)
{
    //           dest                               src
    DGN_TO_BITS( pInfo->nad,                        pu8Data[0], 0, 8 );
    DGN_TO_BITS( pInfo->pci,                        pu8Data[1], 0, 8 );
    DGN_TO_BITS( pInfo->rsid,                       pu8Data[2], 0, 8 );
    DGN_TO_BITS( pInfo->sat_list1,                  pu8Data[3], 0, 8 );
    DGN_TO_BITS( pInfo->sat_list0,                  pu8Data[4], 0, 8 );
    DGN_TO_BITS( pInfo->auto_onoff,                 pu8Data[5], 0, 8 );
    DGN_TO_BITS( pInfo->smart_conf1,                pu8Data[6], 0, 8 );
    DGN_TO_BITS( pInfo->smart_conf0,                pu8Data[7], 0, 8 );

}

//-----------------------------------------------------------------------------
// Function:    teleco_generic_negative_diag_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Teleco Generic Negative
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void teleco_generic_negative_diag_I_Extract
(
    teleco_gen_neg_diag_info_t*    pInfo,
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
