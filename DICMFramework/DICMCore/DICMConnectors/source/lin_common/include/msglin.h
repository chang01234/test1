//------------------------------------------------------------------------------
// Module:      msglin.h
//
//------------------------------------------------------------------------------
// Description: Provide mailboxes for the LIN network.
//              Handle decoding of incoming message parameters which are written
//              into the parameter storage. Handle packaging of parameter 
//              variables into the transmitted messages.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef MSGLIN_H
#define MSGLIN_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "pstore.h"

//------------------------------------------------------------------------------
// Global definitions
//------------------------------------------------------------------------------

#define AC_CTRL_ID  DOMETIC_AC_DEFAULT_CTRL_ID
#define AC_INFO_ID  DOMETIC_AC_DEFAULT_INFO_ID
#define CH0_CTRL_ID TOPTRON_CHARGER0_CTRL_ID
#define CH0_INFO_ID TOPTRON_CHARGER0_INFO_ID
#define IBS_1_ID    HELLA_IBS_FRAME2_ID
#define IBS_2_ID    HELLA_IBS_FRAME5_ID
#define IBS_3_ID    HELLA_IBS_FRAME6_ID
#define SH_WH_CTRL_ID  DOMETIC_SHARC_WH_CTRL_FRAME_ID
#define SH_WH_INFO_ID  DOMETIC_SHARC_WH_INFO_FRAME_ID
#define SH_AH_CTRL_ID  DOMETIC_SHARC_AH_CTRL_FRAME_ID
#define SH_AH_INFO_ID  DOMETIC_SHARC_AH_INFO_FRAME_ID

// FLAGS
#define LIN_FLAG_INIT  (0x1)
#define LIN_FLAG_TXREQ (0x2)

//------------------------------------------------------------------------------
// Global types
//------------------------------------------------------------------------------
typedef void (*parameter_changed_cb_t)(pstore_table_t eIndex, int32_t i32Value);

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Global function prototypes
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function:    msglin_schedule_all
//-----------------------------------------------------------------------------
// Description: Schedule all devices.
//-----------------------------------------------------------------------------
// Return:      None
//-----------------------------------------------------------------------------
extern void msglin_schedule_all(void);

//-----------------------------------------------------------------------------
// Function:    msglin_schedule_next
//-----------------------------------------------------------------------------
// Description: Message scheduler.
//-----------------------------------------------------------------------------
// Return:      0 = Do not scan or tx, 1 = OK to scan or tx
//-----------------------------------------------------------------------------
extern int msglin_schedule_next(uint8_t *device, uint8_t *data, uint8_t *size, int *scan_tx, int *done);

//-----------------------------------------------------------------------------
// Function:    MSGCAN_Initialize
//-----------------------------------------------------------------------------
// Description: Initialization of EPS receive and transmit mail boxes.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
extern void msglin_Initialize
( 
	parameter_changed_cb_t funcCb // in: Application function cb
);

//-----------------------------------------------------------------------------
// Function:    msglin_ProcessRx
//-----------------------------------------------------------------------------
// Description: Process a Rx message from a LIN bus
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
extern void msglin_ProcessRx
(
    uint8_t busno,    // in : LIN bus number where message is recevied
    uint8_t device,   // in : Device address on bus
    uint8_t *data,    // in : Pointer to data
	uint8_t size      // in : Size of data
);

//-----------------------------------------------------------------------------
// Function:    msglin_TxReq
//-----------------------------------------------------------------------------
// Description: Schedule a device for Tx.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
extern void msglin_TxReq( uint8_t busno, uint8_t device );
extern void msglin_TxReq_Diag(uint8_t busno, uint8_t device, uint32_t parameter );

extern void msglin_InitBeforeTxReq( uint8_t busno, uint8_t device, pstore_table_t eIndex);

extern int MsgLin_isDiagFrame(uint8_t device);
extern void MsgLin_Update_PStore(pstore_table_t eIndex, int32_t i32Value);

//Diagnostic Initial Process
extern void  MsgLin_Initial_Process_Diag(uint8_t    u8LinBus, uint8_t*    frame);

extern void MsgLin_Sleep_Frame(uint8_t u8LinBus, uint8_t* frame);

typedef enum
{
    // Diagnostic Hella Master Request

    //HELLA DEVICE
    HE_PID_DIAG = 0,
    HE_BATCAPREAD_DIAG,
    HE_BATCAPWRITE_DIAG,
    HE_BATTYPEREAD_DIAG,
    HE_BATTYPEWRITE_DIAG,
    HE_BATTABLESTATE_DIAG,
    //HELLA DEVELOPMENT SERVICE
    HE_BATTABLEONOFF_DIAG,
    HE_U0MINMAXREAD_DIAG,
    HE_U0MINMAXWRITE_DIAG,
    HE_IBATTQUIESCENTREAD_DIAG,
    HE_IBATTQUIESCENTWRITE_DIAG,

    //TRUMA VARIO HEATER
    TR_VH_PID_DIAG,
    TR_VH_SERIALREAD_DIAG,

    //NEEDS TO ADD THE BELOW FUNCTION HANDLERS IN THE DIAGNOSTIC SCHEDULER
    TR_VH_ASSIGNNAD_DIAG,
    TR_VH_ASSIGNFRAME_DIAG,
    //TRUMA SAPHIR COMFORT AIR CONDITIONING
    TR_SAPCOM_AC_PID_DIAG,
    TR_SAPCOM_AC_SERIALREAD_DIAG,
    TR_SAPCOM_AC_CURRENTERROR_DIAG,
    TR_SAPCOM_AC_ASSIGNNAD_DIAG,
    TR_SAPCOM_AC_ASSIGNFRAME_DIAG,
    //TRUMA AVENTA COMFORT AIR CONDITIONING
    TR_AVECOM_AC_PID_DIAG,
    TR_AVECOM_AC_SERIALREAD_DIAG,
    TR_AVECOM_AC_CURRENTERROR_DIAG,
    TR_AVECOM_AC_ASSIGNNAD_DIAG,
    TR_AVECOM_AC_ASSIGNFRAME_DIAG,
    //TRUMA AVENTA ECO AIR CONDITIONING
    TR_AVEECO_AC_PID_DIAG,
    TR_AVEECO_AC_SERIALREAD_DIAG,
    TR_AVEECO_AC_CURRENTERROR_DIAG,
    TR_AVEECO_AC_ASSIGNNAD_DIAG,
    TR_AVEECO_AC_ASSIGNFRAME_DIAG,
    //TRUMA COMBI SAPHIR COMFORT
    TR_SAPCOM_COMBI_PID_DIAG,
    TR_SAPCOM_COMBI_SERIALREAD_DIAG,
    TR_SAPCOM_COMBI_FWVER_DIAG,
    TR_SAPCOM_COMBI_CURRENTERROR_DIAG,
    TR_SAPCOM_COMBI_ASSIGNNAD_DIAG,
    TR_SAPCOM_COMBI_ASSIGNFRAME_DIAG,
    //TRUMA COMBI AVENTA COMFORT
    TR_AVECOM_COMBI_PID_DIAG,
    TR_AVECOM_COMBI_SERIALREAD_DIAG,
    TR_AVECOM_COMBI_FWVER_DIAG,
    TR_AVECOM_COMBI_CURRENTERROR_DIAG,
    TR_AVECOM_COMBI_ASSIGNNAD_DIAG,
    TR_AVECOM_COMBI_ASSIGNFRAME_DIAG,
    //TRUMA COMBI AVENTA ECO
    TR_AVEECO_COMBI_PID_DIAG,
    TR_AVEECO_COMBI_SERIALREAD_DIAG,
    TR_AVEECO_COMBI_FWVER_DIAG,
    TR_AVEECO_COMBI_CURRENTERROR_DIAG,
    TR_AVEECO_COMBI_ASSIGNNAD_DIAG,
    TR_AVEECO_COMBI_ASSIGNFRAME_DIAG,
    //TRUMA COMBI CP PLUS
    TR_CPPLUS_COMBI_PID_DIAG,
    TR_CPPLUS_COMBI_FWVER_DIAG,
    TR_CPPLUS_COMBI_ASSIGNNAD_DIAG,

    //TELECO IFS DEVICE
    TE_IFS_PID_DIAG,
    TE_IFS_UCOM32_DIAG,
    TE_IFS_UCOM33_DIAG,
    TE_IFS_UCOM34_DIAG,
    TE_IFS_UCOM35_DIAG,
    //TELECO ACTIVE SATELLITE DEVICE
    TE_ASAT_PID_DIAG,
    TE_ASAT_UCOM36_DIAG,
    TE_ASAT_UCOM37_DIAG,
    TE_ASAT_UCOM38_DIAG,
    TE_ASAT_UCOM39_DIAG,
    //TELECO FLAT SATELLITE BT DEVICE
    TE_FSATBT_PID_DIAG,
    TE_FSATBT_UCOM36_DIAG,
    TE_FSATBT_UCOM37_DIAG,
    TE_FSATBT_UCOM38_DIAG,
    TE_FSATBT_UCOM39_DIAG,
    //TELECO TELE SATELLITE BT DEVICE
    TE_TSATBT_PID_DIAG,
    TE_TSATBT_UCOM36_DIAG,
    TE_TSATBT_UCOM37_DIAG,
    TE_TSATBT_UCOM38_DIAG,
    TE_TSATBT_UCOM39_DIAG,

    //TELAIR AC DEVICE
    TA_AC_PID_DIAG,
    TA_AC_UCOM32_DIAG,
    TA_AC_UCOM33_DIAG,

    //KATHREIN CI SATELLITE SYSTEMS
    KA_SAT_PID_DIAG,
    KA_SAT_SWVER_DIAG,

    //DOMETIC FRESHJET
    DO_FJET_PID_DIAG,
    //DOMETIC FRESHWELL
    DO_FWEL_PID_DIAG,

    //TENHAAFT AUTOMATIC SATELITE SYSTEMS
    TH_SAT_PID_DIAG,

    //ALDE 3020
    AL_3020_PID_DIAG,
    AL_3020_ASSIGNNAD_DIAG,
    AL_3020_ASSIGNFRAME_DIAG,

    //DOMETIC SHARC HEATER
    DO_SH_PID_DIAG,
    DO_SH_SERIALREAD_DIAG,
    DO_SH_ASSIGNNAD_DIAG,
    DO_SH_ASSIGNFRAME_DIAG,

    GEN_DEVICE_RESPONSE,
    TOTAL_MASTER_REQ_DIAG
} diagnostic_master_req_t;

typedef enum
{
    HE = 1,             //Hella Device

    TR_VH,              //TRUMA Vario Heater
    TR_SAPCOM_AC,       //TRUMA Saphir Comfort Air Conditioning
    TR_AVECOM_AC,       //TRUMA Aventa Comfort Air Conditioning
    TR_AVEECO_AC,       //TRUMA Aventa Eco Air Conditioning
    TR_SAPCOM_COMBI,    //TRUMA Combi Saphir Comfort
    TR_AVECOM_COMBI,    //TRUMA Combi Aventa Comfort
    TR_AVEECO_COMBI,    //TRUMA Combi Aventa Eco
    TR_CPPLUS_COMBI,    //TRUMA Combi CP Plus

    TE_IFS,             //Teleco IFS Device
    TE_ASAT,            //Teleco Active Satellite Device
    TE_FSATBT,          //Teleco Flat Satellite BT Device
    TE_TSATBT,          //Teleco Tele Satellite BT Device

    TA_AC,              //Telair AC Device

    KA_SAT,             //Kathrein CI Satellite Systems

    DO_FJET,            //Dometic Freshjet
    DO_FWEL,            //Dometic Freshwell

    TH_SAT,             //Tenhaaft Automatic Satelite Systems

    AL_3020,            //Alde 3020
    DO_SHARC            //Dometic Sharc

}diagnostic_connected_device;

#define HE_NAD 0x01     //Hella NAD

#define TR_NAD 0x02     //Truma NAD
#define TA_NAD 0x02     //TeleAir NAD
#define DO_SH_NAD 0x02  //Dometic Sharc Heater NAD

#define TE_NAD 0x19     //Teleco NAD
#define KA_NAD 0x19     //Kathrein NAD
#define TH_NAD 0x19     //Tenhaaft NAD

#define DO_NAD 0x10     //Dometic NAD
#define AL_NAD 0x10     //ALDE NAD

//Define the Supplier ID
#define ALDE_SUP_ID 0x41DE
#define DOME_SUP_ID 0x1234
#define HELL_SUP_ID 0x0036
#define KATH_SUP_ID 0x1919
#define TELECO_SUP_ID 0x6226
#define TELEAIR_SUP_ID 0x6226
#define TENH_SUP_ID 0x7468
#define TRUM_SUP_ID 0x4617
#endif  // MSGLIN_H
