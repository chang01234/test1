//------------------------------------------------------------------------------
// Module:      diag.c
//
//------------------------------------------------------------------------------

#include "diag.h"
#include "truma.h"
#include "msglin.h"
#include "msglin_func.h"
#include "frame_util.h"
#include <string.h>
#include "configuration.h"

extern uint8_t diag_init_process_active;

//-----------------------------------------------------------------------------
// Function:    diag_generic_sleep_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Generic PID into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void diag_generic_sleep_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    sleep_diag_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_GENERIC_PID_REQ_FRAME_SIZE );

    // Fill
    //           dest         source
    BITS_TO_DGN( pu8Data[0],  pCtrl->data1, 0, 8 );
    BITS_TO_DGN( pu8Data[1],  pCtrl->data2, 0, 8 );
    BITS_TO_DGN( pu8Data[2],  pCtrl->data3, 0, 8 );
    BITS_TO_DGN( pu8Data[3],  pCtrl->data4, 0, 8 );
    BITS_TO_DGN( pu8Data[4],  pCtrl->data5, 0, 8 );
    BITS_TO_DGN( pu8Data[5],  pCtrl->data6, 0, 8 );
    BITS_TO_DGN( pu8Data[6],  pCtrl->data7, 0, 8 );
    BITS_TO_DGN( pu8Data[7],  pCtrl->data8, 0, 8 );
}

//-----------------------------------------------------------------------------
// Function:    diag_generic_pid_C_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters for Diagnostic Generic PID into a frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void diag_generic_pid_C_Stuff
(
    uint8_t*    pu8Data,    // out: LIN frame buffer
    diag_generic_pid_ctrl_t*    pCtrl     // in : Parameters to stuff
)
{
        // Clear
    (void)memset( pu8Data, 0, DIA_GENERIC_PID_REQ_FRAME_SIZE );

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
// Function:    diag_generic_pid_I_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from Diagnostic Generic PID
//              Info frame buffer.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void diag_generic_pid_I_Extract
(
    diag_generic_pid_info_t*    pInfo,
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

//------------------------------------------------------------------------------
// Function:    MsgLin_Generic_Prod_Info_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Process the Generic PID to all device
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
void MsgLin_Generic_Prod_Info_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    diag_generic_pid_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        // The product identification(PID) is same for all devices.
        ctrl.nad = 0x01;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x00;
        ctrl.supplier_id = 0x7fff;
        ctrl.func_id = 0xffff;
    }

    // Stuff message data
    diag_generic_pid_C_Stuff( frame, &ctrl );
    // Store the SID value into global variable to check negative response
    diag_transmit_sid = ctrl.sid;
}

//------------------------------------------------------------------------------
// Function:    MsgLin_Generic_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Generic PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
void MsgLin_Generic_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    diag_generic_pid_info_t info;
    diag_generic_pid_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.

        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x", info.rsid);
        LOG(I, "sup_id:  0x%x", info.supplier_id);
        LOG(I, "func_id: 0x%x", info.func_id);
        LOG(I, "var_id:  0x%x", info.var_id);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_Generic_Prod_Info_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Process the Generic Response from  all device
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------

void MsgLin_Generic_Prod_Info_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    uint8_t temp_nad;
    uint16_t temp_supp_id;
    //uint16_t temp_fun_id;

    if (diag_init_process_active)
    {
        LOG(I, "MsgLin_Generic_Prod_Info_Diag_I_RxHandle");
        MsgLin_Initial_Process_Diag(u8LinBus,frame);

    }
    else
    {
        //Process the PID Request after Diagnostic Initial Process Completed.
        DGN_TO_BITS( temp_nad,         frame[0], 0, 8 );
        DGN_TO_WORD( temp_supp_id,     frame[3]);
        //DGN_TO_WORD( temp_fun_id,     frame[5]);

        switch(temp_nad)
        {
        case HE_NAD:
            if(temp_supp_id == HELL_SUP_ID)// Further check to verify whether received response from Hella Device
            {
                LOG(I, "Hella Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }
            break;
        case TR_NAD:// Truma device and Teleair device have same NAD
        //case TA_NAD:
            if(temp_supp_id == TRUM_SUP_ID)// Further check to verify whether received response from TRUMA Device
            {
                LOG(I, "Truma Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }

            if(temp_supp_id == TELEAIR_SUP_ID)// Further check to verify whether received response from TELEAIR Device
            {
                LOG(I, "Teleair Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }
            break;

        case TE_NAD:// Teleco device, Kathrein Device and Tenhaaft device have same NAD
        //case KA_NAD:
        //case TH_NAD:
            if(temp_supp_id == TELECO_SUP_ID)// Further check to verify whether received response from Teleco Device
            {
                LOG(I, "Teleco Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }

            if(temp_supp_id == KATH_SUP_ID)// Further check to verify whether received response from KATHREIN Device
            {
                LOG(I, "Kathrein Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }

            if(temp_supp_id == TENH_SUP_ID)// Further check to verify whether received response from TENHAAFT Device
            {
                LOG(I, "Tenhaaft Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }
            break;

        case DO_NAD:// Dometic device and Alde device have same NAD
        //case AL_NAD:
            if(temp_supp_id == DOME_SUP_ID)// Further check to verify whether received response from Dometic Device
            {
                LOG(I, "Dometic Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }

            if(temp_supp_id == ALDE_SUP_ID)// Further check to verify whether received response from ALDE Device
            {
                LOG(I, "ALDE Device PID");
                MsgLin_Generic_PID_Diag_I_RxHandle(u8LinBus, frame);

            }

            break;
        default:
            break;
        }
    }
}
