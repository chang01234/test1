//------------------------------------------------------------------------------
// Module:      msglin_func.h
//
//------------------------------------------------------------------------------
// Description:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef MSGLIN_FUNC_H
#define MSGLIN_FUNC_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
//#include "pstore.h"		 // Parameter storage

//------------------------------------------------------------------------------
// Global definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Global types
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
//Diagnostic Related Variables
extern uint8_t diag_transmit_sid;
//------------------------------------------------------------------------------
// Global function prototypes
//------------------------------------------------------------------------------

/*******************************************************************************
 *                               Receive handlers Section
 * ****************************************************************************/
//Description:
//Gateway receive the data from respective LIN devices
extern void  MsgLin_HE_Bat_IBS_Frm2_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_Bat_IBS_Frm5_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_Bat_IBS_Frm6_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_Wtr_Htr_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_Air_Htr_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TO_Charger0_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_DO_AC_I_RxHandle(uint8_t u8LinBus, uint8_t* frame);
extern void  MsgLin_TO_Lightbox_I_RxHandle(uint8_t u8LinBus, uint8_t* frame);
extern void  MsgLin_DO_SHARC_WH_I_RxHandle(uint8_t u8LinBus, uint8_t* frame);
extern void  MsgLin_DO_SHARC_AH_I_RxHandle(uint8_t u8LinBus, uint8_t* frame);

/*******************************Diagnostic Message Receive Handler**************/
//Hella Device
extern void  MsgLin_HE_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatCapRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatCapWrite_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatTypeRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatTypeWrite_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatTableState_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Hella Development Service
extern void  MsgLin_HE_BatTableOnOff_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_U0MinMaxRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_U0MinMaxWrite_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_IBattQuiescentRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_IBattQuiescentWrite_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//TRUMA Vario Heater
extern void  MsgLin_TR_VH_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_VH_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_VH_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_VH_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//TRUMA Saphir Comfort Air Conditioning
extern void  MsgLin_TR_SapCom_AC_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AC_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AC_CurrentError_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AC_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AC_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//TRUMA Aventa Comfort Air Conditioning
extern void  MsgLin_TR_AveCom_AC_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AC_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AC_CurrentError_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AC_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AC_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//TRUMA Aventa Eco Air Conditioning
extern void  MsgLin_TR_AveEco_AC_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AC_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AC_CurrentError_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AC_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AC_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//TRUMA Combi Saphir Comfort
extern void  MsgLin_TR_SapCom_Combi_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_Combi_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_Combi_FwVer_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_Combi_CurrentError_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_Combi_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_Combi_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//TRUMA Combi Aventa Comfort
extern void  MsgLin_TR_AveCom_Combi_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_Combi_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_Combi_FwVer_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_Combi_CurrentError_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_Combi_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_Combi_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//TRUMA Combi Aventa Eco
extern void  MsgLin_TR_AveEco_Combi_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_Combi_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_Combi_FwVer_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_Combi_CurrentError_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_Combi_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_Combi_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//TRUMA Combi CP Plus
extern void  MsgLin_TR_CPplus_Combi_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_CPplus_Combi_FwVer_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_CPplus_Combi_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Teleco IFS Device
extern void  MsgLin_TE_IFS_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_IFS_UCOM32_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_IFS_UCOM33_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_IFS_UCOM34_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_IFS_UCOM35_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Teleco Active Satellite Device
extern void  MsgLin_TE_ASAT_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_ASAT_UCOM36_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_ASAT_UCOM37_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_ASAT_UCOM38_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_ASAT_UCOM39_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Teleco Flat Satellite BT Device
extern void  MsgLin_TE_FSATBT_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_FSATBT_UCOM36_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_FSATBT_UCOM37_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_FSATBT_UCOM38_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_FSATBT_UCOM39_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Teleco Tele Satellite BT Device
extern void  MsgLin_TE_TSATBT_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_TSATBT_UCOM36_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_TSATBT_UCOM37_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_TSATBT_UCOM38_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_TSATBT_UCOM39_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Telair AC Device
extern void  MsgLin_TA_AC_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TA_AC_UCOM32_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TA_AC_UCOM33_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Kathrein CI Satellite Systems
extern void  MsgLin_KA_Sat_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_KA_Sat_SwVer_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Dometic Freshjet
extern void  MsgLin_DO_FJet_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Dometic Freshwell
extern void  MsgLin_DO_FWel_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Tenhaaft Automatic Satelite Systems
extern void  MsgLin_TH_SAT_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Alde 3020
extern void  MsgLin_AL_3020_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_AL_3020_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_AL_3020_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Dometic Sharc
extern void  MsgLin_DO_SH_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_DO_SH_SerialRead_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_DO_SH_AssignNad_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_DO_SH_AssignFrame_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

/*******************************************************************************
 *                               Transmit handlers Section
 * ****************************************************************************/
extern void  MsgLin_TR_Wtr_Htr_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_Air_Htr_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TO_Charger0_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_DO_AC_C_TxHandle(uint8_t u8LinBus, uint8_t* frame);
extern void  MsgLin_TO_Lightbox_C_TxHandle(uint8_t u8LinBus, uint8_t* frame);
extern void  MsgLin_DO_SHARC_WH_C_TxHandle(uint8_t u8LinBus, uint8_t* frame);
extern void  MsgLin_DO_SHARC_AH_C_TxHandle(uint8_t u8LinBus, uint8_t* frame);

/********************************Diagnostic Transmit Handler*******************/
//Hella
extern void  MsgLin_HE_BatCapRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatCapWrite_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatTypeRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatTypeWrite_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_BatTableState_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Hella Development Service
extern void  MsgLin_HE_BatTableOnOff_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_U0MinMaxRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_U0MinMaxWrite_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_IBattQuiescentRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_HE_IBattQuiescentWrite_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Truma Vario Heater
extern void  MsgLin_TR_VH_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_VH_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_VH_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Truma Saphir Comfort Air Conditioning
extern void  MsgLin_TR_SapCom_AC_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AC_CError_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AC_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AC_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Truma Aventa Comfort Air Conditioning
extern void  MsgLin_TR_AveCom_AC_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AC_CError_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AC_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AC_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Truma Aventa Eco Air Conditioning
extern void  MsgLin_TR_AveEco_AC_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AC_CError_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AC_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AC_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Truma Combi Saphir Comfort
extern void  MsgLin_TR_SapCom_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_FwVer_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_CurrentError_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_SapCom_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Truma Combi Aventa Comfort
extern void  MsgLin_TR_AveCom_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_FwVer_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_CurrentError_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveCom_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Truma Combi Aventa Eco
extern void  MsgLin_TR_AveEco_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_FwVer_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_CurrentError_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_AveEco_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Truma Combi CP Plus
extern void  MsgLin_TR_CPplus_FwVer_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TR_CPplus_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Teleco IFS
extern void  MsgLin_TE_IFS_UCOM32_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_IFS_UCOM33_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_IFS_UCOM34_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_IFS_UCOM35_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Teleco ActivSat
extern void  MsgLin_TE_ASAT_UCOM36_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_ASAT_UCOM37_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_ASAT_UCOM38_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_ASAT_UCOM39_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Teleco Flat Satellite BT Device
extern void  MsgLin_TE_FSATBT_UCOM36_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_FSATBT_UCOM37_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_FSATBT_UCOM38_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_FSATBT_UCOM39_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Teleco Tele Satellite BT Device
extern void  MsgLin_TE_TSATBT_UCOM36_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_TSATBT_UCOM37_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_TSATBT_UCOM38_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TE_TSATBT_UCOM39_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Telair Air conditioning
extern void  MsgLin_TA_AC_UCOM32_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_TA_AC_UCOM33_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

//KATHREIN CI Satelite Systems
extern void  MsgLin_KA_Sat_SwVer_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Alde 3020
extern void  MsgLin_AL_3020_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_AL_3020_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Dometic SHARC
extern void  MsgLin_DO_SH_SerialRead_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_DO_SH_AssignNad_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
extern void  MsgLin_DO_SH_AssignFrame_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

extern void  MsgLin_Sleep_TxHandle(uint8_t u8LinBus,  uint8_t* frame);

#endif  // MSGLIN_FUNC_H
