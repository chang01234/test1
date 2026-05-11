//------------------------------------------------------------------------------
// Module:      msglin.c
//
//------------------------------------------------------------------------------
// Description: Provide mailboxes for the LIN network.
//              Handle decoding of incoming message parameters which are written
//              into the parameter storage. Handle packaging of parameter 
//              variables into the transmitted messages.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include "msglin.h"
#include "pstore.h"
#include "truma.h"
#include "hella.h"
#include "dometic.h"
#include "toptron.h"
#include "connector.h"
#include "configuration.h"
#include "frame_util.h"
#include "diag.h"
#include "msglin_func.h"
#include "sorted_list.h"

//------------------------------------------------------------------------------
// Local constants
//------------------------------------------------------------------------------
// FLAGS
//#define LIN_FLAG_INIT  (0x1)
//#define LIN_FLAG_TXREQ (0x2)
#define HEATER_TRUMA
#define DEVICE_LIST_DEPTH 50

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

// Diagnostic Related Variables
uint8_t diag_init_process_active = 0;
static uint32_t diag_counter_var = 0;

//------------------------------------------------------------------------------
// Local types.
//------------------------------------------------------------------------------

typedef void (*lin_frame_init_t)(pstore_table_t eIndex);
typedef void (*lin_txrx_handler_t)(uint8_t busno, uint8_t *data);

typedef struct
{
    uint8_t nad;
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
    uint8_t var_id; // variant ID
    uint8_t dev;
    lin_txrx_handler_t func;
} device_type_mapping_t;

typedef struct
{
	uint8_t schedule;
	uint8_t dev;
	uint8_t ftype; /* Frame type, ctrl=1, info=0 */
	uint8_t flag;
	uint8_t frame_len;
    lin_txrx_handler_t func;
    lin_frame_init_t   initfunc;
} scheduler_t;

//Diagnostic Scheduler
typedef struct
{
    uint8_t schedule;
    uint8_t dev;
    uint8_t nad;
    uint8_t device_name;
    uint8_t master_comm_type;
    uint8_t ftype; /* Frame type, ctrl, info */
    uint8_t flag;
    uint8_t frame_len;
    lin_txrx_handler_t func;
    lin_txrx_handler_t recv_fun;
    uint8_t diag_state; // Adding Diagnostic State
} dia_scheduler_t;

//------------------------------------------------------------------------------
// Local variables.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

// Init handlers
static void MsgLin_DO_AC_C_Reset( pstore_table_t eIndex );
//static void MsgLin_TO_Charger0_C_Reset( pstore_table_t eIndex );

#define DIA_MSG_CTRL 2
#define DIA_MSG_INFO 0

typedef enum
{
    DIA_TRANSMIT_ACTIVE = 1,
    DIA_PREPARE_MASTER_REQ_RESP,
    DIA_RESPONSE_WAITING,
    DIA_RESPONSE_RECV,
    DIA_SILENT,
    DIA_RESPONSE_ACTIVE,
    DIA_PLAN_TO_TRANSMIT
} dia_state_t;

#define NAD_NOT_REQUIRE 0x00
#define GEN_DEVICE 0

static const device_type_mapping_t dev_mapping[] = {
    // NAD,   Supplier  , Func  , variant,   dev, Rx function 
    {HE_NAD, HELL_SUP_ID, 0xf10a, 0x6, IBS_3_ID, MsgLin_HE_Bat_IBS_Frm6_I_RxHandle},
    {HE_NAD, HELL_SUP_ID, 0xf10a, 0x3, IBS_3_ID, MsgLin_HE_Bat_IBS_Frm6_I_RxHandle}, // Solna LAB Variant 3
    {HE_NAD, HELL_SUP_ID, 0xf60a, 0x4, IBS_3_ID, MsgLin_HE_Bat_IBS_Frm6_I_RxHandle},
};

static scheduler_t scheduler[] = {
	//      Ctrl
    //schedule  Frame identifier        ftype  flag frame_len                func                           initfunc
#if 0
	{0, 0x1,   1, 0, 8, MsgLin_TO_Lightbox_C_TxHandle, NULL},
	{0, 0x5,   1, 0, 1, NULL, NULL},
	{1, TOPTRON_LIGHTBOX_INFO_ID, 0, 0, 8, MsgLin_TO_Lightbox_I_RxHandle, NULL},
	{1, 0x4,   0, 0, 1, NULL, NULL},
	{1, 0x6,   0, 0, 1, NULL, NULL},
#endif
	{0, AC_CTRL_ID,  1, 0, DOMETIC_AC_CTRL_SIZE, MsgLin_DO_AC_C_TxHandle, MsgLin_DO_AC_C_Reset},
	{1, AC_INFO_ID,  0, 0, DOMETIC_AC_INFO_SIZE, MsgLin_DO_AC_I_RxHandle, NULL},
#if 0
	{0, CH0_CTRL_ID,  1, 0, 4, MsgLin_TO_Charger0_C_TxHandle, MsgLin_TO_Charger0_C_Reset},
	{1, CH0_INFO_ID,  0, 0, 2, MsgLin_TO_Charger0_I_RxHandle, NULL},
	{1, 0x19,  0, 0, 1, NULL, NULL},
	{1, 0x1b,  0, 0, 1, NULL, NULL},
#endif
	{1, IBS_1_ID,  0, 0, 7, MsgLin_HE_Bat_IBS_Frm2_I_RxHandle, NULL},
	{1, IBS_2_ID,  0, 0, 6, MsgLin_HE_Bat_IBS_Frm5_I_RxHandle, NULL},
	{1, IBS_3_ID,  0, 0, 6, NULL, NULL},
#ifdef HEATER_TRUMA
	{0, 0x37,  1, 0, 8, MsgLin_TR_Wtr_Htr_C_TxHandle, NULL},
	{1, 0x38,  0, 0, 8, MsgLin_TR_Wtr_Htr_I_RxHandle, NULL},
	{0, 0x39,  1, 0, 8, MsgLin_TR_Air_Htr_C_TxHandle, NULL},
	{1, 0x3a,  0, 0, 8, MsgLin_TR_Air_Htr_I_RxHandle, NULL},
#endif
#ifdef HEATER_DOMETIC
	{0, 0x37,  1, 0, 8, MsgLin_DO_SH_Wtr_Htr_C_TxHandle, NULL},
	{1, 0x38,  0, 0, 8, MsgLin_DO_SH_Wtr_Htr_I_RxHandle, NULL},
	{0, 0x39,  1, 0, 8, MsgLin_DO_SH_Air_Htr_C_TxHandle, NULL},
	{1, 0x3a,  0, 0, 8, MsgLin_DO_SH_Air_Htr_I_RxHandle, NULL},
#endif
#if 0
	{1, 0x34,  0, 0, 8, NULL, NULL},
	{1, 0x0c,  0, 0, 8, NULL, NULL},
	{1, 0x1f,  0, 0, 8, NULL, NULL},
#endif
	//Diagnostic Message Response Information
	{1, 0x3c,  1, 0, 8, NULL, NULL},
	{1, 0x3d,  0, 0, 8, MsgLin_Generic_Prod_Info_Diag_I_RxHandle, NULL} // always last
};

//Diagnostic Scheduler
static dia_scheduler_t dia_scheduler[] = {
    //schedule  dev     nad     device_name             master_comm_type                    ftype        flag frame_len    func                                              recv_fun                                           diag_state

    //Hella Device Related
    {1,         0x3c,   HE_NAD,    HE,                  HE_PID_DIAG,                        DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_HE_PID_Diag_I_RxHandle,                      DIA_SILENT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_BATCAPREAD_DIAG,                 DIA_MSG_CTRL,    0,  8,       MsgLin_HE_BatCapRead_Diag_C_TxHandle,             MsgLin_HE_BatCapRead_Diag_I_RxHandle,               DIA_SILENT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_BATCAPWRITE_DIAG,                DIA_MSG_CTRL,    0,  8,       MsgLin_HE_BatCapWrite_Diag_C_TxHandle,            MsgLin_HE_BatCapWrite_Diag_I_RxHandle,              DIA_SILENT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_BATTYPEREAD_DIAG,                DIA_MSG_CTRL,    0,  8,       MsgLin_HE_BatTypeRead_Diag_C_TxHandle,            MsgLin_HE_BatTypeRead_Diag_I_RxHandle,              DIA_SILENT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_BATTYPEWRITE_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_HE_BatTypeWrite_Diag_C_TxHandle,           MsgLin_HE_BatTypeWrite_Diag_I_RxHandle,             DIA_SILENT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_BATTABLESTATE_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_HE_BatTableState_Diag_C_TxHandle,          MsgLin_HE_BatTableState_Diag_I_RxHandle,            DIA_SILENT},
#if 0
    //Hella Development Service Related
    {0,         0x3c,   HE_NAD,    HE,                  HE_BATTABLEONOFF_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_HE_BatTableOnOff_Diag_C_TxHandle,          MsgLin_HE_BatTableOnOff_Diag_I_RxHandle,            DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_U0MINMAXREAD_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_HE_U0MinMaxRead_Diag_C_TxHandle,           MsgLin_HE_U0MinMaxRead_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_U0MINMAXWRITE_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_HE_U0MinMaxWrite_Diag_C_TxHandle,          MsgLin_HE_U0MinMaxWrite_Diag_I_RxHandle,            DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_IBATTQUIESCENTREAD_DIAG,         DIA_MSG_CTRL,    0,  8,       MsgLin_HE_IBattQuiescentRead_Diag_C_TxHandle,     MsgLin_HE_IBattQuiescentRead_Diag_I_RxHandle,       DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   HE_NAD,    HE,                  HE_IBATTQUIESCENTWRITE_DIAG,        DIA_MSG_CTRL,    0,  8,       MsgLin_HE_IBattQuiescentWrite_Diag_C_TxHandle,    MsgLin_HE_IBattQuiescentWrite_Diag_I_RxHandle,      DIA_PLAN_TO_TRANSMIT},

    //TRUMA Vario Heater Related
    {1,         0x3c,   TR_NAD,    TR_VH,               TR_VH_PID_DIAG,                     DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_VH_PID_Diag_I_RxHandle,                   DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_VH,               TR_VH_SERIALREAD_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_TR_VH_SerialRead_Diag_C_TxHandle,          MsgLin_TR_VH_SerialRead_Diag_I_RxHandle,            DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_VH,               TR_VH_ASSIGNNAD_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TR_VH_AssignNad_Diag_C_TxHandle,           MsgLin_TR_VH_AssignNad_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_VH,               TR_VH_ASSIGNFRAME_DIAG,             DIA_MSG_CTRL,    0,  8,       MsgLin_TR_VH_AssignFrame_Diag_C_TxHandle,         MsgLin_TR_VH_AssignFrame_Diag_I_RxHandle,           DIA_PLAN_TO_TRANSMIT},

    //TRUMA Saphir Comfort Air Conditioning Related
    {1,         0x3c,   TR_NAD,    TR_SAPCOM_AC,        TR_SAPCOM_AC_PID_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_SapCom_AC_PID_Diag_I_RxHandle,            DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_AC,        TR_SAPCOM_AC_SERIALREAD_DIAG,       DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_AC_SerialRead_Diag_C_TxHandle,   MsgLin_TR_SapCom_AC_SerialRead_Diag_I_RxHandle,     DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_AC,        TR_SAPCOM_AC_CURRENTERROR_DIAG,     DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_AC_CError_Diag_C_TxHandle,       MsgLin_TR_SapCom_AC_CurrentError_Diag_I_RxHandle,   DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_AC,        TR_SAPCOM_AC_ASSIGNNAD_DIAG,        DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_AC_AssignNad_Diag_C_TxHandle,    MsgLin_TR_SapCom_AC_AssignNad_Diag_I_RxHandle,      DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_AC,        TR_SAPCOM_AC_ASSIGNFRAME_DIAG,      DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_AC_AssignFrame_Diag_C_TxHandle,  MsgLin_TR_SapCom_AC_AssignFrame_Diag_I_RxHandle,    DIA_PLAN_TO_TRANSMIT},

    //Truma Aventa Comfort Air Conditioning
    {1,         0x3c,   TR_NAD,    TR_AVECOM_AC,        TR_AVECOM_AC_PID_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_AveCom_AC_PID_Diag_I_RxHandle,            DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_AC,        TR_AVECOM_AC_SERIALREAD_DIAG,       DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_AC_SerialRead_Diag_C_TxHandle,   MsgLin_TR_AveCom_AC_SerialRead_Diag_I_RxHandle,     DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_AC,        TR_AVECOM_AC_CURRENTERROR_DIAG,     DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_AC_CError_Diag_C_TxHandle,       MsgLin_TR_AveCom_AC_CurrentError_Diag_I_RxHandle,   DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_AC,        TR_AVECOM_AC_ASSIGNNAD_DIAG,        DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_AC_AssignNad_Diag_C_TxHandle,    MsgLin_TR_AveCom_AC_AssignNad_Diag_I_RxHandle,      DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_AC,        TR_AVECOM_AC_ASSIGNFRAME_DIAG,      DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_AC_AssignFrame_Diag_C_TxHandle,  MsgLin_TR_AveCom_AC_AssignFrame_Diag_I_RxHandle,    DIA_PLAN_TO_TRANSMIT},

    //TRUMA Aventa Eco Air Conditioning Related
    {1,         0x3c,   TR_NAD,    TR_AVEECO_AC,       TR_AVEECO_AC_PID_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_AveEco_AC_PID_Diag_I_RxHandle,            DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_AC,       TR_AVEECO_AC_SERIALREAD_DIAG,        DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_AC_SerialRead_Diag_C_TxHandle,   MsgLin_TR_AveEco_AC_SerialRead_Diag_I_RxHandle,     DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_AC,       TR_AVEECO_AC_CURRENTERROR_DIAG,      DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_AC_CError_Diag_C_TxHandle,       MsgLin_TR_AveEco_AC_CurrentError_Diag_I_RxHandle,   DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_AC,       TR_AVEECO_AC_ASSIGNNAD_DIAG,         DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_AC_AssignNad_Diag_C_TxHandle,    MsgLin_TR_AveEco_AC_AssignNad_Diag_I_RxHandle,      DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_AC,       TR_AVEECO_AC_ASSIGNFRAME_DIAG,       DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_AC_AssignFrame_Diag_C_TxHandle,  MsgLin_TR_AveEco_AC_AssignFrame_Diag_I_RxHandle,    DIA_PLAN_TO_TRANSMIT},

    //Truma Combi Saphir Comfort Related
    {1,         0x3c,   TR_NAD,    TR_SAPCOM_COMBI,    TR_SAPCOM_COMBI_PID_DIAG,            DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_SapCom_Combi_PID_Diag_I_RxHandle,          DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_COMBI,    TR_SAPCOM_COMBI_SERIALREAD_DIAG,     DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_SerialRead_Diag_C_TxHandle,      MsgLin_TR_SapCom_Combi_SerialRead_Diag_I_RxHandle,   DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_COMBI,    TR_SAPCOM_COMBI_FWVER_DIAG,          DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_FwVer_Diag_C_TxHandle,           MsgLin_TR_SapCom_Combi_FwVer_Diag_I_RxHandle,        DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_COMBI,    TR_SAPCOM_COMBI_CURRENTERROR_DIAG,   DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_CurrentError_Diag_C_TxHandle,    MsgLin_TR_SapCom_Combi_CurrentError_Diag_I_RxHandle, DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_COMBI,    TR_SAPCOM_COMBI_ASSIGNNAD_DIAG,      DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_AssignNad_Diag_C_TxHandle,       MsgLin_TR_SapCom_Combi_AssignNad_Diag_I_RxHandle,    DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_SAPCOM_COMBI,    TR_SAPCOM_COMBI_ASSIGNFRAME_DIAG,    DIA_MSG_CTRL,    0,  8,       MsgLin_TR_SapCom_AssignFrame_Diag_C_TxHandle,     MsgLin_TR_SapCom_Combi_AssignFrame_Diag_I_RxHandle,  DIA_PLAN_TO_TRANSMIT},

    //Truma Combi Aventa Comfort Related
    {1,         0x3c,   TR_NAD,    TR_AVECOM_COMBI,    TR_AVECOM_COMBI_PID_DIAG,            DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_AveCom_Combi_PID_Diag_I_RxHandle,          DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_COMBI,    TR_AVECOM_COMBI_SERIALREAD_DIAG,     DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_SerialRead_Diag_C_TxHandle,      MsgLin_TR_AveCom_Combi_SerialRead_Diag_I_RxHandle,   DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_COMBI,    TR_AVECOM_COMBI_FWVER_DIAG,          DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_FwVer_Diag_C_TxHandle,           MsgLin_TR_AveCom_Combi_FwVer_Diag_I_RxHandle,        DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_COMBI,    TR_AVECOM_COMBI_CURRENTERROR_DIAG,   DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_CurrentError_Diag_C_TxHandle,    MsgLin_TR_AveCom_Combi_CurrentError_Diag_I_RxHandle, DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_COMBI,    TR_AVECOM_COMBI_ASSIGNNAD_DIAG,      DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_AssignNad_Diag_C_TxHandle,       MsgLin_TR_AveCom_Combi_AssignNad_Diag_I_RxHandle,    DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVECOM_COMBI,    TR_AVECOM_COMBI_ASSIGNFRAME_DIAG,    DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveCom_AssignFrame_Diag_C_TxHandle,     MsgLin_TR_AveCom_Combi_AssignFrame_Diag_I_RxHandle,  DIA_PLAN_TO_TRANSMIT},

    //TRUMA Combi Aventa Eco Related
    {1,         0x3c,   TR_NAD,    TR_AVEECO_COMBI,    TR_AVEECO_COMBI_PID_DIAG,            DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_AveEco_Combi_PID_Diag_I_RxHandle,          DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_COMBI,    TR_AVEECO_COMBI_SERIALREAD_DIAG,     DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_SerialRead_Diag_C_TxHandle,      MsgLin_TR_AveEco_Combi_SerialRead_Diag_I_RxHandle,   DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_COMBI,    TR_AVEECO_COMBI_FWVER_DIAG,          DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_FwVer_Diag_C_TxHandle,           MsgLin_TR_AveEco_Combi_FwVer_Diag_I_RxHandle,        DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_COMBI,    TR_AVEECO_COMBI_CURRENTERROR_DIAG,   DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_CurrentError_Diag_C_TxHandle,    MsgLin_TR_AveEco_Combi_CurrentError_Diag_I_RxHandle, DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_COMBI,    TR_AVEECO_COMBI_ASSIGNNAD_DIAG,      DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_AssignNad_Diag_C_TxHandle,       MsgLin_TR_AveEco_Combi_AssignNad_Diag_I_RxHandle,    DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_AVEECO_COMBI,    TR_AVEECO_COMBI_ASSIGNFRAME_DIAG,    DIA_MSG_CTRL,    0,  8,       MsgLin_TR_AveEco_AssignFrame_Diag_C_TxHandle,     MsgLin_TR_AveEco_Combi_AssignFrame_Diag_I_RxHandle,  DIA_PLAN_TO_TRANSMIT},
    //Truma Combi CP Plus Related
    {1,         0x3c,   TR_NAD,    TR_CPPLUS_COMBI,    TR_CPPLUS_COMBI_PID_DIAG,            DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TR_CPplus_Combi_PID_Diag_I_RxHandle,          DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TR_NAD,    TR_CPPLUS_COMBI,    TR_CPPLUS_COMBI_FWVER_DIAG,          DIA_MSG_CTRL,    0,  8,       MsgLin_TR_CPplus_FwVer_Diag_C_TxHandle,           MsgLin_TR_CPplus_Combi_FwVer_Diag_I_RxHandle,        DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TR_NAD,    TR_CPPLUS_COMBI,    TR_CPPLUS_COMBI_ASSIGNNAD_DIAG,      DIA_MSG_CTRL,    0,  8,       MsgLin_TR_CPplus_AssignNad_Diag_C_TxHandle,       MsgLin_TR_CPplus_Combi_AssignNad_Diag_I_RxHandle,    DIA_PLAN_TO_TRANSMIT},
    //Teleco IFS Device Related
    {1,         0x3c,   TE_NAD,    TE_IFS,             TE_IFS_PID_DIAG,                     DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TE_IFS_PID_Diag_I_RxHandle,                   DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TE_NAD,    TE_IFS,             TE_IFS_UCOM32_DIAG,                  DIA_MSG_CTRL,    0,  8,       MsgLin_TE_IFS_UCOM32_Diag_C_TxHandle,             MsgLin_TE_IFS_UCOM32_Diag_I_RxHandle,                DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_IFS,             TE_IFS_UCOM33_DIAG,                  DIA_MSG_CTRL,    0,  8,       MsgLin_TE_IFS_UCOM33_Diag_C_TxHandle,             MsgLin_TE_IFS_UCOM33_Diag_I_RxHandle,                DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_IFS,             TE_IFS_UCOM34_DIAG,                  DIA_MSG_CTRL,    0,  8,       MsgLin_TE_IFS_UCOM34_Diag_C_TxHandle,             MsgLin_TE_IFS_UCOM34_Diag_I_RxHandle,                DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_IFS,             TE_IFS_UCOM35_DIAG,                  DIA_MSG_CTRL,    0,  8,       MsgLin_TE_IFS_UCOM35_Diag_C_TxHandle,             MsgLin_TE_IFS_UCOM35_Diag_I_RxHandle,                DIA_PLAN_TO_TRANSMIT},
    //Teleco ActivSat Device Related
    {1,         0x3c,   TE_NAD,    TE_ASAT,            TE_ASAT_PID_DIAG,                    DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TE_ASAT_PID_Diag_I_RxHandle,                  DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TE_NAD,    TE_ASAT,            TE_ASAT_UCOM36_DIAG,                 DIA_MSG_CTRL,    0,  8,       MsgLin_TE_ASAT_UCOM36_Diag_C_TxHandle,            MsgLin_TE_ASAT_UCOM36_Diag_I_RxHandle,               DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_ASAT,            TE_ASAT_UCOM37_DIAG,                 DIA_MSG_CTRL,    0,  8,       MsgLin_TE_ASAT_UCOM37_Diag_C_TxHandle,            MsgLin_TE_ASAT_UCOM37_Diag_I_RxHandle,               DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_ASAT,            TE_ASAT_UCOM38_DIAG,                 DIA_MSG_CTRL,    0,  8,       MsgLin_TE_ASAT_UCOM38_Diag_C_TxHandle,            MsgLin_TE_ASAT_UCOM38_Diag_I_RxHandle,               DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_ASAT,            TE_ASAT_UCOM39_DIAG,                 DIA_MSG_CTRL,    0,  8,       MsgLin_TE_ASAT_UCOM39_Diag_C_TxHandle,            MsgLin_TE_ASAT_UCOM39_Diag_I_RxHandle,               DIA_PLAN_TO_TRANSMIT},
    //Teleco Flat Satellite BT Device Related
    {1,         0x3c,   TE_NAD,    TE_FSATBT,          TE_FSATBT_PID_DIAG,                  DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TE_FSATBT_PID_Diag_I_RxHandle,                DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TE_NAD,    TE_FSATBT,          TE_FSATBT_UCOM36_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_FSATBT_UCOM36_Diag_C_TxHandle,          MsgLin_TE_FSATBT_UCOM36_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_FSATBT,          TE_FSATBT_UCOM37_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_FSATBT_UCOM37_Diag_C_TxHandle,          MsgLin_TE_FSATBT_UCOM37_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_FSATBT,          TE_FSATBT_UCOM38_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_FSATBT_UCOM38_Diag_C_TxHandle,          MsgLin_TE_FSATBT_UCOM38_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_FSATBT,          TE_FSATBT_UCOM39_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_FSATBT_UCOM39_Diag_C_TxHandle,          MsgLin_TE_FSATBT_UCOM39_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},

    //Teleco Tele Satellite BT Device Related
    {1,         0x3c,   TE_NAD,    TE_TSATBT,          TE_TSATBT_PID_DIAG,                  DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TE_TSATBT_PID_Diag_I_RxHandle,                DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TE_NAD,    TE_TSATBT,          TE_TSATBT_UCOM36_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_TSATBT_UCOM36_Diag_C_TxHandle,          MsgLin_TE_TSATBT_UCOM36_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_TSATBT,          TE_TSATBT_UCOM37_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_TSATBT_UCOM37_Diag_C_TxHandle,          MsgLin_TE_TSATBT_UCOM37_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_TSATBT,          TE_TSATBT_UCOM38_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_TSATBT_UCOM38_Diag_C_TxHandle,          MsgLin_TE_TSATBT_UCOM38_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TE_NAD,    TE_TSATBT,          TE_TSATBT_UCOM39_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_TE_TSATBT_UCOM39_Diag_C_TxHandle,          MsgLin_TE_TSATBT_UCOM39_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},

    //Telair AC Device Related
    {1,         0x3c,   TA_NAD,    TA_AC,              TA_AC_PID_DIAG,                      DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TA_AC_PID_Diag_I_RxHandle,                   DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   TA_NAD,    TA_AC,              TA_AC_UCOM32_DIAG,                   DIA_MSG_CTRL,    0,  8,       MsgLin_TA_AC_UCOM32_Diag_C_TxHandle,              MsgLin_TA_AC_UCOM32_Diag_I_RxHandle,                DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   TA_NAD,    TA_AC,              TA_AC_UCOM33_DIAG,                   DIA_MSG_CTRL,    0,  8,       MsgLin_TA_AC_UCOM33_Diag_C_TxHandle,              MsgLin_TA_AC_UCOM33_Diag_I_RxHandle,                DIA_PLAN_TO_TRANSMIT},

    //KATHREIN CI Satelite Systems Device Related
    {1,         0x3c,   KA_NAD,    KA_SAT,             KA_SAT_PID_DIAG,                     DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_KA_Sat_PID_Diag_I_RxHandle,                  DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   KA_NAD,    KA_SAT,             KA_SAT_SWVER_DIAG,                   DIA_MSG_CTRL,    0,  8,       MsgLin_KA_Sat_SwVer_Diag_C_TxHandle,              MsgLin_KA_Sat_SwVer_Diag_I_RxHandle,                DIA_PLAN_TO_TRANSMIT},

    //Dometic Freshjet Device Related
    {1,         0x3c,   DO_NAD,    DO_FJET,            DO_FJET_PID_DIAG,                    DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_DO_FJet_PID_Diag_I_RxHandle,                 DIA_TRANSMIT_ACTIVE},

    //Dometic Freshwell Device Related
    {1,         0x3c,   DO_NAD,    DO_FWEL,            DO_FWEL_PID_DIAG,                    DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_DO_FWel_PID_Diag_I_RxHandle,                 DIA_TRANSMIT_ACTIVE},

    //Tenhaaft Automatic Satelite Systems Related
    {1,         0x3c,   TH_NAD,    TH_SAT,             TH_SAT_PID_DIAG,                     DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_TH_SAT_PID_Diag_I_RxHandle,                  DIA_TRANSMIT_ACTIVE},

    //Alde 3020 Device Related
    {1,         0x3c,   AL_NAD,    AL_3020,            AL_3020_PID_DIAG,                    DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_AL_3020_PID_Diag_I_RxHandle,                 DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   AL_NAD,    AL_3020,            AL_3020_ASSIGNNAD_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_AL_3020_AssignNad_Diag_C_TxHandle,         MsgLin_AL_3020_AssignNad_Diag_I_RxHandle,           DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   AL_NAD,    AL_3020,            AL_3020_ASSIGNFRAME_DIAG,            DIA_MSG_CTRL,    0,  8,       MsgLin_AL_3020_AssignFrame_Diag_C_TxHandle,       MsgLin_AL_3020_AssignFrame_Diag_I_RxHandle,         DIA_PLAN_TO_TRANSMIT},

    //Dometic Sharc Heater Related
    {1,         0x3c,   DO_SH_NAD, DO_SHARC,           DO_SH_PID_DIAG,                     DIA_MSG_CTRL,    0,  8,       MsgLin_Generic_Prod_Info_Diag_C_TxHandle,         MsgLin_DO_SH_PID_Diag_I_RxHandle,                   DIA_TRANSMIT_ACTIVE},
    {0,         0x3c,   DO_SH_NAD, DO_SHARC,           DO_SH_SERIALREAD_DIAG,              DIA_MSG_CTRL,    0,  8,       MsgLin_DO_SH_SerialRead_Diag_C_TxHandle,          MsgLin_DO_SH_SerialRead_Diag_I_RxHandle,            DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   DO_SH_NAD, DO_SHARC,           DO_SH_ASSIGNNAD_DIAG,               DIA_MSG_CTRL,    0,  8,       MsgLin_DO_SH_AssignNad_Diag_C_TxHandle,           MsgLin_DO_SH_AssignNad_Diag_I_RxHandle,             DIA_PLAN_TO_TRANSMIT},
    {0,         0x3c,   DO_SH_NAD, DO_SHARC,           DO_SH_ASSIGNFRAME_DIAG,             DIA_MSG_CTRL,    0,  8,       MsgLin_DO_SH_AssignFrame_Diag_C_TxHandle,         MsgLin_DO_SH_AssignFrame_Diag_I_RxHandle,           DIA_PLAN_TO_TRANSMIT},
#endif
    //To Receive All Diagnostic Response
    {1,         0x3d,   NAD_NOT_REQUIRE,    GEN_DEVICE,     GEN_DEVICE_RESPONSE,            DIA_MSG_INFO,    0,  8,      NULL,                                             MsgLin_Generic_Prod_Info_Diag_I_RxHandle,           DIA_RESPONSE_ACTIVE}
};

static parameter_changed_cb_t parameterFuncCb = NULL;
static SORTED_LIST_ENTRY function_table_data[DEVICE_LIST_DEPTH]={0};	   			//!< \~ Function table storage			(unique key)
static SORTED_LIST function_table=
{
	.pdata=function_table_data,
	.capacity=DEVICE_LIST_DEPTH
};

//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------
static void MsgLin_TR_Wtr_Htr_I_Reset( void );
static void MsgLin_TR_Air_Htr_I_Reset( void );
static void MsgLin_TR_Wtr_Htr_C_Reset( void );
static void MsgLin_TR_Air_Htr_C_Reset( void );
static int msglin_add_function(uint8_t device, void *func);
static void MsgLin_hella_battype_write_change(void);
static void MsgLin_hella_bat_cnom_write_change(void);

static void map_device(uint8_t nad)
{
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
    uint8_t var_id; // variant ID

	for (int i = 0; i < (int)ELEMENTS(dev_mapping); i++)
	{
        if (dev_mapping[i].nad == nad)
        {
            LOG(I, "Mapping HELLA");
            // Hella IBS
            supplier_id = pstore_u16GetValue(VAR_LIN_DIA_HE_IBS0_PID_SUP_ID_I);
            func_id = pstore_u16GetValue(VAR_LIN_DIA_HE_IBS0_PID_FUN_ID_I);
            var_id = pstore_u8GetValue(VAR_LIN_DIA_HE_IBS0_PID_VAR_ID_I);
            LOG(I, "Mapping HELLA %x %x %x", supplier_id, func_id, var_id);

            if ((dev_mapping[i].supplier_id == supplier_id) && (dev_mapping[i].func_id == func_id) && (dev_mapping[i].var_id == var_id))
            {
                for (int j = 0; j < (int)ELEMENTS(scheduler); j++)
                {
                    if (!scheduler[j].func && (scheduler[j].dev == dev_mapping[i].dev))
                    {
                        scheduler[j].func = dev_mapping[i].func;
                        LOG(I, "Added new receive function for dev=%x", dev_mapping[i].dev);
                        msglin_add_function(dev_mapping[i].dev, dev_mapping[i].func);
                    }
                }
            }
        }
    }
}

static int diag_schedule_next(uint8_t *device, uint8_t *data, uint8_t *size, int *scan_tx, int search, int *done)
{
    int res = 0;
    //uint8_t txreq;
    uint8_t dia_schedule_index = 0;
    //uint8_t next_schedule_index = 0;
    
     /*
     * Explanation about below Implemented Logic
     *
     * DIA_TRANSMIT_ACTIVE   -   Product Identifier Master Request is kept as DIA_TRANSMIT_ACTIVE
     *                           under diag_state column in the Diagnostic Scheduler
     *
     * DIA_RESPONSE_WAITING  -   Once Master Request Transferred from Diagnostic Scheduler then that request
     *                           put as DIA_RESPONSE_WAITING under diag_state column in the Diagnostic Scheduler
     *
     * DIA_RESPONSE_RECV     -   Once response received from Device then that request put as DIA_RESPONSE_RECV
     *
     * DIA_SILENT            -   When next New Master Request transmitted from Diagnostic Scheduler
     *                           then previous Master Request which already received the response from device
     *                           put as DIA_SILENT.
     *
     * DIA_RESPONSE_ACTIVE    -  Not Require to transfer. This is use to receive the various device responses
     *
     * DIA_PLAN_TO_TRANSMIT   -  Master Request Planned to Transmit. Wait for this request to transfer
     *
     */

    /*Diagnostic Transmit Process Starts:
     * When All Diagnostic Master Requests transferred from Diagnostic Scheduler(dia_scheduler)
     * then only Control will come out from the below while loop and start execute the
     * normal process.*/
    while (search)
    {
        if(dia_schedule_index < ELEMENTS(dia_scheduler))
        {
            //LOG(I, "loop%d state=%d", dia_schedule_index, dia_scheduler[dia_schedule_index].diag_state);

            if(dia_scheduler[dia_schedule_index].diag_state == DIA_SILENT )
            {
                dia_schedule_index++;
                continue;
            }
            else if(dia_scheduler[dia_schedule_index].diag_state == DIA_TRANSMIT_ACTIVE) // Diagnostic PID Control Frame, send out.
            {
                diag_init_process_active = 1;
                *size = dia_scheduler[dia_schedule_index].frame_len;
                *device = dia_scheduler[dia_schedule_index].dev;
                if(dia_scheduler[dia_schedule_index].ftype == DIA_MSG_CTRL && dia_scheduler[dia_schedule_index].func)
                {
                    // Call function for Stuffing the Frame
                    dia_scheduler[dia_schedule_index].func(0, data);

                     // Indicate Tx
                     *scan_tx = 1;

                     // Ok, to tx
                     res = 1;

                     // Clear Tx flag
                     dia_scheduler[dia_schedule_index].flag &= ((uint8_t)~LIN_FLAG_TXREQ);
                     dia_scheduler[dia_schedule_index].diag_state = DIA_PREPARE_MASTER_REQ_RESP;
                     diag_counter_var = 0;
                }
            }
            else if(dia_scheduler[dia_schedule_index].diag_state == DIA_RESPONSE_RECV)
            {
#if 1
                // Clear and map
                dia_scheduler[dia_schedule_index].diag_state = DIA_SILENT;
                diag_init_process_active = 0;
                map_device(dia_scheduler[dia_schedule_index].nad);
                *done = 1;
                goto diag_done;
#endif
#if 0
                res = 0;

                // Previous Master Request Received Response from device. Look for another one.
                next_schedule_index = dia_schedule_index + 1;
                while (next_schedule_index < ELEMENTS(dia_scheduler))
                {
                    if (dia_scheduler[next_schedule_index].ftype == DIA_MSG_CTRL &&
                        dia_scheduler[next_schedule_index].diag_state == DIA_PLAN_TO_TRANSMIT &&
                        dia_scheduler[next_schedule_index].func)
                    {
                        *size = dia_scheduler[next_schedule_index].frame_len;
                        *device = dia_scheduler[next_schedule_index].dev;
                        // Call function
                        dia_scheduler[next_schedule_index].func(0, data);

                        // Indicate Tx
                        *scan_tx = 1;

                        // Ok to Tx
                        res = 1;

                        //dia_scheduler[dia_schedule_index].diag_state = DIA_SILENT;
                        dia_scheduler[next_schedule_index].diag_state = DIA_PREPARE_MASTER_REQ_RESP;
                        dia_schedule_index = next_schedule_index;
                        diag_init_process_active = 1;
                        diag_counter_var = 0;
                    }
                    else
                    {
                        next_schedule_index++;
                    }
                }

                if (!res)
                {
                    diag_init_process_active = 0;
                    goto diag_done;
                }
#if 0
                else if((dia_schedule_index == ELEMENTS(dia_scheduler) - 1) &&
                        (dia_scheduler[schedule_index].ftype == DIA_MSG_CTRL) &&
                        (dia_scheduler[schedule_index].func))
                {
                    //Reached to Last Master Request
                    dia_scheduler[dia_schedule_index].diag_state = DIA_SILENT;
                }
                else
                {
                    // Not Require to Transfer
                    dia_schedule_index++;
                    continue;
                }
                }
#endif
#endif
            }
            else if(dia_scheduler[dia_schedule_index].diag_state == DIA_PREPARE_MASTER_REQ_RESP)
            {
                LOG(I, "Prepare");
                *size = 8;
                *device = 0x3d;
                *scan_tx = 0; // Indicate scan
                res = 1; // OK to scan
                dia_scheduler[dia_schedule_index].diag_state = DIA_RESPONSE_WAITING;
            }
            else if(dia_scheduler[dia_schedule_index].diag_state == DIA_RESPONSE_WAITING)
            {
                //Response Not Received from Device

                /* Question: Needs to know, How long is it waiting for the device response?.
                 * if waiting period beyond the specified duration, we Assume that we have to resend the
                 * Master Request
                 */
                if (diag_counter_var < 6)
                {
                    LOG(I, "No response, No tx.");
                    diag_counter_var++;
                    res = 0; // Ignore scan or tx
                    //dia_scheduler[dia_schedule_index].diag_state = DIA_PREPARE_MASTER_REQ_RESP;
                    //continue;
                }
                else
                {
                    //Resend the Master Request
                    dia_scheduler[dia_schedule_index].diag_state = DIA_TRANSMIT_ACTIVE;
                    continue; // Move to while loop beginning
                }
            }
            else
            {
                *done = 1;
                diag_init_process_active = 0;
                goto diag_done;
            }
#if 0
            else
            {
                // Any other Diagnostic State(which not require to transfer) then skip the row in Diagnostic Scheduler
                dia_schedule_index++;
                continue;
            }
#endif
        }
        else
        {
            //No further Diagnostic Message to Transfer. So Deactivate the Diagnostic Process.
            diag_init_process_active = 0;
        }

        if (diag_init_process_active)
        {
            return res;
        }
    }//Come out Diagnostic Process Loop

diag_done:
    /* Once Diagnostic Process completed below process is beeing continiously executing */
    return res;
}

//-----------------------------------------------------------------------------
// Function:    msglin_schedule_all
//-----------------------------------------------------------------------------
// Description: Schedule all devices.
//-----------------------------------------------------------------------------
// Return:      None
//-----------------------------------------------------------------------------
void msglin_schedule_all(void)
{
	for (int i = 0; i < (int)ELEMENTS(scheduler); i++)
	{
        if (scheduler[i].dev == 0x37 || scheduler[i].dev == 0x39)
        {
            /* Truma ctrl frames shall always be txed */
		    scheduler[i].schedule = 1;
        }
	}
}

//-----------------------------------------------------------------------------
// Function:    msglin_schedule_next
//-----------------------------------------------------------------------------
// Description: Message scheduler
//-----------------------------------------------------------------------------
// Return:     0 = Do not scan or tx, 1 = OK to scan or tx
//-----------------------------------------------------------------------------
int msglin_schedule_next(uint8_t *device, uint8_t *data, uint8_t *size, int *scan_tx, int *done)
{
	static uint8_t schedule_index = 0;
    int res = 0;
    uint8_t txreq;
    //uint8_t dia_schedule_index = 0;
    //uint8_t next_schedule_index = 0;

    *device = 0;
    *done = 0;

	do {
		if (schedule_index < ELEMENTS(scheduler))
		{
            //LOG(I, "Schedule id=%x", scheduler[schedule_index].dev);
            if (scheduler[schedule_index].dev == 0x3c)
            {
                res = diag_schedule_next(device, data, size, scan_tx, true, done);
                if (!(*done))
                {
                    break; // Leave do loop
                }
                res = 0;
            }
            else
            {
                txreq = scheduler[schedule_index].flag & LIN_FLAG_TXREQ ? 1 : 0;

                if (scheduler[schedule_index].schedule || txreq)
                {
                    *size = scheduler[schedule_index].frame_len;
                    *device = scheduler[schedule_index].dev;

                    // If we have a Tx request or if we have a Schedule for a control frame (Truma specific)
                    // then force a transmit of next scheduled device but not for Diag msg (0x3c)
                    if (txreq || (scheduler[schedule_index].schedule && scheduler[schedule_index].ftype && ((*device) != 0x3c)))
                    {
                        *scan_tx = 1;
                    }
                    else
                    {
                        *scan_tx = 0;
                    }

#if 0
                if (txreq && ((scheduler[schedule_index].flag & LIN_FLAG_INIT) == 0) && scheduler[schedule_index].initfunc)
                {
                    /* Not initialized. Init first time. Call function. */
                    scheduler[schedule_index].initfunc();

                    /* Indicate initialized */
                    scheduler[schedule_index].flag |= LIN_FLAG_INIT;
                }
#endif
                    /* If we have a ctrl frame and a function pointer? */
                    if (scheduler[schedule_index].ftype && scheduler[schedule_index].func)
                    {
                        // Call function
                        scheduler[schedule_index].func(0, data);


                        // Clear Tx flag
                        scheduler[schedule_index].flag &= ((uint8_t)~LIN_FLAG_TXREQ);
                    }

                    // Indicate OK to scan or tx
                    res = 1;
			    }
            }

			schedule_index++;

			if (schedule_index >= (ELEMENTS(scheduler)-1))
			{
                //LOG(W, "Clearing schedule index");
				schedule_index = 0;
			}
		}
	} while ((*device == 0) && !(*done));

	return res;
}

//-----------------------------------------------------------------------------
// Function:    msglin_add_function
//-----------------------------------------------------------------------------
// Description: Initialization of receive and transmit mail boxes.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static int msglin_add_function(uint8_t device, void *func)
{
	return sorted_list_unique_add(&function_table, device, (uint32_t)func);
}

//-----------------------------------------------------------------------------
// Function:    MSGCAN_Initialize
//-----------------------------------------------------------------------------
// Description: Initialization of receive and transmit mail boxes.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void msglin_Initialize
( 
	parameter_changed_cb_t funcCb // in: Application function cb
)
{
	parameterFuncCb = funcCb;
    pstore_Initialize();

    for (int i = 0; i < (int)ELEMENTS(scheduler); i++)
    {
        if ((scheduler[i].func != NULL) && (scheduler[i].ftype == 0))
        {
            msglin_add_function(scheduler[i].dev, scheduler[i].func);
        }
    }

    for (int i = 0; i < (int)ELEMENTS(dia_scheduler); i++)
    {
        //Diagnostic Message
        if ((dia_scheduler[i].func != NULL) && (dia_scheduler[i].ftype == 0) && (dia_scheduler[i].dev == 0x3D))
        {
            //Receive Function handler and device ID Added into List
            msglin_add_function(dia_scheduler[i].dev, dia_scheduler[i].func);
        }
    }

	// Reset all parameters
    MsgLin_TR_Wtr_Htr_I_Reset();
    MsgLin_TR_Air_Htr_I_Reset();
    MsgLin_TR_Wtr_Htr_C_Reset();
    MsgLin_TR_Air_Htr_C_Reset();
}

//-----------------------------------------------------------------------------
// Function:    msglin_Process
//-----------------------------------------------------------------------------
// Description: Manage message timeouts and fault detection.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void msglin_ProcessRx( uint8_t busno, uint8_t device, uint8_t *data, uint8_t size )
{
    SORTED_LIST_VALUE_TYPE sort_list;
    int match;

    /* Remove parity */
    device &= 0x3F;

	match = sorted_list_unique_get(&sort_list, &function_table, device, 0);
    if (match)
    {
        ((lin_txrx_handler_t)(sort_list))(busno, data);
    }
    else
    {
        // Check if the device is present in the list of known devices that
        // needs to be mapped. If present, activate the DIAG frame for
        // Production info (PID)
        for (int i = 0; i < (int)ELEMENTS(dev_mapping); i++)
        {
            if (dev_mapping[i].dev == device)
            {
                for (int j = 0; j < (int)ELEMENTS(dia_scheduler); j++)
                {
                    // TODO: this needs to be handled by the connector in some other way. Added two battery for test.
                    if (((dia_scheduler[j].nad == dev_mapping[i].nad) && (dia_scheduler[j].master_comm_type == HE_PID_DIAG)) ||
                        ((dia_scheduler[j].nad == dev_mapping[i].nad) && (dia_scheduler[j].master_comm_type == HE_BATCAPREAD_DIAG)) ||
                        ((dia_scheduler[j].nad == dev_mapping[i].nad) && (dia_scheduler[j].master_comm_type == HE_BATTYPEREAD_DIAG)))
                    {
                        dia_scheduler[j].diag_state = DIA_TRANSMIT_ACTIVE;
                        diag_init_process_active = 1;
                        // TODO: enable break when only one check above
                        // break;
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    msglin_TxReq
//-----------------------------------------------------------------------------
// Description: Schedule a device for Tx.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void msglin_TxReq( uint8_t busno, uint8_t device )
{
    (void)busno; // TODO: For now

    for (int i = 0; i < (int)ELEMENTS(scheduler); i++)
    {
        if (scheduler[i].dev == device)
        {
            scheduler[i].flag |= LIN_FLAG_TXREQ;
            break;
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    msglin_TxReq_Diag
//-----------------------------------------------------------------------------
// Description: Schedule a device for Diagnostic Tx.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void msglin_TxReq_Diag( uint8_t busno, uint8_t device, uint32_t parameter )
{
    (void)busno; // TODO
    (void)device; // TODO

    switch(parameter)
    {
    case IBS0BATTYP:
        //Activate the Hella Battery Type Master Request
        MsgLin_hella_battype_write_change();
        break;

    case IBS0CNOM:
        //Activate the Hella Battery Type Master Request
        MsgLin_hella_bat_cnom_write_change();
        break;

    default:
        break;
    }
}

//-----------------------------------------------------------------------------
// Function:    msglin_InitBeforeTxReq
//-----------------------------------------------------------------------------
// Description: Init a ctrl frame before first Tx.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void msglin_InitBeforeTxReq( uint8_t busno, uint8_t device, pstore_table_t eIndex)
{
    (void)busno; // TODO: For now

    for (int i = 0; i < (int)ELEMENTS(scheduler); i++)
    {
        if ((scheduler[i].dev == device) && ((scheduler[i].flag & LIN_FLAG_INIT) == 0))
        {
            if (scheduler[i].initfunc)
            {
                scheduler[i].initfunc(eIndex);
                scheduler[i].flag |= LIN_FLAG_INIT;
            }
            break;
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    MsgLin_TR_Wtr_Htr_I_Reset
//-----------------------------------------------------------------------------
// Description: Reset Truma water heater Info frame.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgLin_TR_Wtr_Htr_I_Reset( void )
{
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_TARGET_TEMP_I,      0 );
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_I, 0 );
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_POWER_LIMIT_I,      0 );
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_FROST_CTRL_ON_I,    0 );
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_EL_AVAILABLE_I,     0 );
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_WINDOW_SW_CLR_I,    0 );
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_MANUAL_MODE_I,      0 );
}

//-----------------------------------------------------------------------------
// Function:    MsgLin_TR_Air_Htr_I_Reset
//-----------------------------------------------------------------------------
// Description: Reset Truma air heater Info frame.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgLin_TR_Air_Htr_I_Reset( void )
{
    pstore_SetValue( VAR_LIN_TR_AIR_HTR_TARGET_TEMP_I,      0 );
    pstore_SetValue( VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_I, 0 );
}

//-----------------------------------------------------------------------------
// Function:    MsgLin_TR_Wtr_Htr_C_Reset
//-----------------------------------------------------------------------------
// Description: Reset Truma water heater Ctrl frame.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgLin_TR_Wtr_Htr_C_Reset( void )
{
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_TARGET_TEMP_C,      0 );
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C, 2);
    pstore_SetValue( VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C,      900 );
}

//-----------------------------------------------------------------------------
// Function:    MsgLin_TR_Air_Htr_C_Reset
//-----------------------------------------------------------------------------
// Description: Reset Truma air heater Ctrl frame.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgLin_TR_Air_Htr_C_Reset( void )
{
    pstore_SetValue( VAR_LIN_TR_AIR_HTR_TARGET_TEMP_C,      0 );
    pstore_SetValue( VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C, 2 );
}

//-----------------------------------------------------------------------------
// Function:    MsgLin_DO_AC_C_Reset
//-----------------------------------------------------------------------------
// Description: Reset Dometic AC Ctrl frame.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgLin_DO_AC_C_Reset( pstore_table_t eIndex )
{
    if (eIndex != VAR_LIN0_DO_AC_MODE_A_C)
        pstore_SetValue( VAR_LIN0_DO_AC_MODE_A_C,        pstore_u8GetValue(VAR_LIN0_DO_AC_MODE_A_I));
    if (eIndex != VAR_LIN0_DO_AC_MODE_B_C)
        pstore_SetValue( VAR_LIN0_DO_AC_MODE_B_C,        pstore_u8GetValue(VAR_LIN0_DO_AC_MODE_B_I));
    if (eIndex != VAR_LIN0_DO_AC_FAN_MODE_C)
        pstore_SetValue( VAR_LIN0_DO_AC_FAN_MODE_C,      pstore_u8GetValue(VAR_LIN0_DO_AC_FAN_MODE_I) );
    if (eIndex != VAR_LIN0_DO_AC_LIGHT_STATUS_C)
        pstore_SetValue( VAR_LIN0_DO_AC_LIGHT_STATUS_C,  pstore_u8GetValue(VAR_LIN0_DO_AC_LIGHT_STATUS_I) );
    if (eIndex != VAR_LIN0_DO_AC_POWER_C)
        pstore_SetValue( VAR_LIN0_DO_AC_POWER_C,         pstore_u8GetValue(VAR_LIN0_DO_AC_POWER_I) );
    if (eIndex != VAR_LIN0_DO_AC_FAN_SPEED_C)
        pstore_SetValue( VAR_LIN0_DO_AC_FAN_SPEED_C,     pstore_u8GetValue(VAR_LIN0_DO_AC_FAN_SPEED_I) );
    if (eIndex != VAR_LIN0_DO_AC_TARGET_TEMP_C)
        pstore_SetValue( VAR_LIN0_DO_AC_TARGET_TEMP_C,   pstore_u8GetValue(VAR_LIN0_DO_AC_TARGET_TEMP_I) );
    if (eIndex != VAR_LIN0_DO_AC_LIGHT_DIM_LVL_C)
        pstore_SetValue( VAR_LIN0_DO_AC_LIGHT_DIM_LVL_C, pstore_u8GetValue(VAR_LIN0_DO_AC_LIGHT_DIM_LVL_I) );
}

#if 0
//-----------------------------------------------------------------------------
// Function:    MsgLin_TO_Charger0_C_Reset
//-----------------------------------------------------------------------------
// Description: Reset Toptron Charger 0Ctrl frame.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void MsgLin_TO_Charger0_C_Reset( pstore_table_t eIndex )
{
    if (eIndex != VAR_LIN_TO_CHARGER0_SILENT_MODE_C)
        pstore_SetValue( VAR_LIN_TO_CHARGER0_SILENT_MODE_C, pstore_u8GetValue(VAR_LIN_TO_CHARGER0_SILENT_MODE_I));
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgLin_Update_PStore
//------------------------------------------------------------------------------
// Description: Update PStore if value has changed and call callback function
//              to further process changed value
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
void MsgLin_Update_PStore(pstore_table_t eIndex, int32_t i32Value)
{
	// Get parameter value
	int32_t curVal = pstore_GetValue(eIndex);

	// Check if values differ
	if (i32Value != curVal)
	{
		//LOG(I, "MsgCan_Update_PImage differs");
		// Value has changed so set new value
		pstore_SetValue(eIndex, i32Value);

		// Check if callback is registered
		if (parameterFuncCb != NULL)
		{
			//LOG(I, "MsgCan_Update_PImage calling callback function");
			// Call registered callback function
			parameterFuncCb(eIndex, i32Value);
		}
	}
}

//------------------------------------------------------------------------------
// Function:    MsgLin_Initial_Process_Diag
//------------------------------------------------------------------------------
// Description: Process the Generic Response from  all device
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------

void MsgLin_Initial_Process_Diag
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    for (int i = 0; i < (int)ELEMENTS(dia_scheduler); i++)
    {
        if ((dia_scheduler[i].dev == 0x3c) && (dia_scheduler[i].diag_state == DIA_RESPONSE_WAITING))
        {
            if (dia_scheduler[i].recv_fun)
            {
                LOG(I, "DIAG received");
                //Store the Receive Information
                dia_scheduler[i].recv_fun(u8LinBus, frame);
                dia_scheduler[i].diag_state = DIA_RESPONSE_RECV;
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_isDiagFrame
//------------------------------------------------------------------------------
// Description: Checking the Frame whether it is diagnostic Frame
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      Integer value
//------------------------------------------------------------------------------
int MsgLin_isDiagFrame(uint8_t device)
{
    /* Remove parity */
    device &= 0x3F;

    if ((device == 0x3C) || (device == 0x3D))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_hella_battype_write_change
//------------------------------------------------------------------------------
// Description: Activate the Hella Battery Type Write Master Request from the
//              Diagnostic Scheduler
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
static void MsgLin_hella_battype_write_change(void)
{
    for (int i = 0; i < (int)ELEMENTS(dia_scheduler); i++)
    {
        if ((dia_scheduler[i].dev == 0x3c) &&
                (dia_scheduler[i].nad == HE_NAD) &&
                    (dia_scheduler[i].master_comm_type == HE_BATTYPEWRITE_DIAG))
        {
            //Activate the Master Request in the Scheduler
            dia_scheduler[i].diag_state = DIA_TRANSMIT_ACTIVE;
            //update the Diagnostic Initial Process
            diag_init_process_active = 1;
            break;
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_hella_bat_cnom_write_change
//------------------------------------------------------------------------------
// Description: Activate the Hella Battery Type Write Master Request from the
//              Diagnostic Scheduler
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
static void MsgLin_hella_bat_cnom_write_change(void)
{
    for (int i = 0; i < (int)ELEMENTS(dia_scheduler); i++)
    {
        if ((dia_scheduler[i].dev == 0x3c) &&
                (dia_scheduler[i].nad == HE_NAD) &&
                    (dia_scheduler[i].master_comm_type == HE_BATCAPWRITE_DIAG))
        {
            //Activate the Master Request in the Scheduler
            dia_scheduler[i].diag_state = DIA_TRANSMIT_ACTIVE;
            //update the Diagnostic Initial Process
            diag_init_process_active = 1;
            break;
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_Sleep_Frame
//------------------------------------------------------------------------------
// Description: Diagnostic Sleep Command Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
void MsgLin_Sleep_Frame
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*   frame        // out: Tx data
)
{
    sleep_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        ctrl.data1 = 0;
        ctrl.data2 = 0xff;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
        ctrl.data5 = 0xff;
        ctrl.data6 = 0xff;
        ctrl.data7 = 0xff;
        ctrl.data8 = 0xff;
    }

    // Stuff message data
    diag_generic_sleep_C_Stuff(frame, &ctrl);
}
