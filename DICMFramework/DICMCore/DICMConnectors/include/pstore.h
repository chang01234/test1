//------------------------------------------------------------------------------
// Module:      pstore.h
//------------------------------------------------------------------------------
// Description: Parameter Storage

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef PSTORE_H
#define PSTORE_H

//------------------------------------------------------------------------------
// Include files
//------------------------------------------------------------------------------
#include <stdint.h>

//------------------------------------------------------------------------------
// Public Constants
//------------------------------------------------------------------------------
typedef enum
{
	// LIN TRUMA WATER HEATER CTRL
	VAR_LIN_TR_WTR_HTR_TARGET_TEMP_C = 0,
    VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C,
    VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C,

    // LIN TRUMA WATER HEATER INFO
	VAR_LIN_TR_WTR_HTR_TARGET_TEMP_I,
    VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_I,
    VAR_LIN_TR_WTR_HTR_POWER_LIMIT_I,
    VAR_LIN_TR_WTR_HTR_FROST_CTRL_ON_I,
    VAR_LIN_TR_WTR_HTR_EL_AVAILABLE_I,
    VAR_LIN_TR_WTR_HTR_WINDOW_SW_CLR_I,
    VAR_LIN_TR_WTR_HTR_MANUAL_MODE_I,
    VAR_LIN_TR_HTR_MODEL_I,

    // LIN TRUMA AIR HEATER CTRL
    VAR_LIN_TR_AIR_HTR_STATUS_C,
	VAR_LIN_TR_AIR_HTR_TARGET_TEMP_C,
    VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C,

    // LIN TRUMA AIR HEATER INFO
    VAR_LIN_TR_AIR_HTR_STATUS_I,
	VAR_LIN_TR_AIR_HTR_TARGET_TEMP_I,
    VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_I,

    // LIN HELLA BATTERY IBS
    VAR_LIN_HE_IBS0_BAT_CUR_I,
    VAR_LIN_HE_IBS0_BAT_VLT_I,
    VAR_LIN_HE_IBS0_BAT_TEMP_I,

    VAR_LIN_HE_IBS0_STATE_CHG_I,
    VAR_LIN_HE_IBS0_STATE_HLTH_I,
    VAR_LIN_HE_IBS0_EST_VOLT_DROP_I,
    VAR_LIN_HE_IBS0_CHG_VLT_I,

    VAR_LIN_HE_IBS0_AVAIL_CAP_I,
    VAR_LIN_HE_IBS0_DISCHARGE_AH_I,
    VAR_LIN_HE_IBS0_NOMINAL_CAP_I,
    VAR_LIN_HE_IBS0_RECAL_I,

    // LIN Diagnostic commands for Hella
    VAR_LIN_HE_IBS0_BATTYPE_C,
    VAR_LIN_HE_IBS0_CNOMINAL_C,

    // LIN DOMETIC AC CTRL
	VAR_LIN0_DO_AC_MODE_CTRL_C, // Summary mode, not going out on LIN
	VAR_LIN0_DO_AC_MODE_A_C,
	VAR_LIN0_DO_AC_MODE_B_C,
	VAR_LIN0_DO_AC_FAN_MODE_C,
	VAR_LIN0_DO_AC_LIGHT_STATUS_C,
	VAR_LIN0_DO_AC_POWER_C,
	VAR_LIN0_DO_AC_FAN_SPEED_C,
	VAR_LIN0_DO_AC_TARGET_TEMP_C,
	VAR_LIN0_DO_AC_LIGHT_DIM_LVL_C,
	VAR_LIN0_DO_AC_SYNC_FRAME_C,

    //LIN DOMETIC SHARC WATER HEATER CONTROL
    VAR_LIN0_DO_SHARC_WH_MODE_C,
    VAR_LIN0_DO_SHARC_WH_SLEEP_ON_C,
    VAR_LIN0_DO_SHARC_WH_WAKEUP_C,
    VAR_LIN0_DO_SHARC_WH_ENERGY_SOURCE_C,
    VAR_LIN0_DO_SHARC_WH_LOCK_C,
    VAR_LIN0_DO_SHARC_WH_SYNC_C,
    VAR_LIN0_DO_SHARC_WH_C_MODE_C,

    //LIN DOMETIC SHARC AIR HEATER CONTROL
    VAR_LIN0_DO_SHARC_AH_TARGET_TEMP_C,
    VAR_LIN0_DO_SHARC_AH_SLEEP_ON_C,
    VAR_LIN0_DO_SHARC_AH_WAKEUP_C,
    VAR_LIN0_DO_SHARC_AH_ENERGY_SOURCE_C,
    VAR_LIN0_DO_SHARC_AH_MODE_C,
    VAR_LIN0_DO_SHARC_AH_SILENT_MODE_MAX_FAN_SPEED_C,
    VAR_LIN0_DO_SHARC_AH_VENT_MODE_MIN_FAN_SPEED_C,
    VAR_LIN0_DO_SHARC_AH_TIMER_OFF_C,
    VAR_LIN0_DO_SHARC_AH_TIMER_ON_C,
    VAR_LIN0_DO_SHARC_WH_TIMER_ON_C,
    VAR_LIN0_DO_SHARC_AH_LOCK_C,
    VAR_LIN0_DO_SHARC_AH_SYNC_C,
    VAR_LIN0_DO_SHARC_AH_C_MODE_C,

    // LIN DOMETIC AC INFO
	VAR_LIN0_DO_AC_MODE_CTRL_I, // Summary mode, not coming from LIN
	VAR_LIN0_DO_AC_MODE_A_I,
	VAR_LIN0_DO_AC_MODE_B_I,
	VAR_LIN0_DO_AC_FAN_MODE_I,
	VAR_LIN0_DO_AC_LIGHT_STATUS_I,
	VAR_LIN0_DO_AC_POWER_I,
	VAR_LIN0_DO_AC_FAN_SPEED_I,
	VAR_LIN0_DO_AC_TARGET_TEMP_I,
	VAR_LIN0_DO_AC_LIGHT_DIM_LVL_I,
	VAR_LIN0_DO_AC_LOCAL_CHANGE_I,
	VAR_LIN0_DO_AC_CI_ERROR_I,

    // LIN DOMETIC SHARC AIR/WATER HEATER CTRL
    VAR_LIN0_DO_SH_AIR_TEMP_C,
    VAR_LIN0_DO_SH_AIR_ON_C,
    VAR_LIN0_DO_SH_AIR_ESEL_C,
    VAR_LIN0_DO_SH_AIR_MODE_C,
    VAR_LIN0_DO_SH_AIR_SMAXFAN_C,
    VAR_LIN0_DO_SH_AIR_VMINFAN_C,
    VAR_LIN0_DO_SH_AIR_SYNC_FRAME_C,
    VAR_LIN0_DO_SH_WTR_TEMP_C,
    VAR_LIN0_DO_SH_WTR_ON_C,
    VAR_LIN0_DO_SH_WTR_ESEL_C,
    VAR_LIN0_DO_SH_WTR_SYNC_FRAME_C,

    // LIN DOMETIC SHARC AIR/WATER HEATER INFO
    VAR_LIN0_DO_SH_AIR_TEMP_I,
    VAR_LIN0_DO_SH_AIR_ON_I,
    VAR_LIN0_DO_SH_AIR_ESEL_I,
    VAR_LIN0_DO_SH_AIR_MODE_I,
    VAR_LIN0_DO_SH_AIR_SMAXFAN_I,
    VAR_LIN0_DO_SH_AIR_VMINFAN_I,
    VAR_LIN0_DO_SH_AIR_REMOTE_CHANGE_I,
    VAR_LIN0_DO_SH_AIR_LOCAL_CHANGE_I,
    VAR_LIN0_DO_SH_WTR_TEMP_I,
    VAR_LIN0_DO_SH_WTR_ON_I,
    VAR_LIN0_DO_SH_WTR_ESEL_I,
    VAR_LIN0_DO_SH_WTR_ERRST_I,
    VAR_LIN0_DO_SH_WTR_ERRCD_I,
    VAR_LIN0_DO_SH_WTR_REMOTE_CHANGE_I,
    VAR_LIN0_DO_SH_WTR_LOCAL_CHANGE_I,
    VAR_LIN0_DO_SH_WTR_CI_ERROR_I,

    // LIN TOPTRON CHARGER0 CTRL
    VAR_LIN_TO_CHARGER0_CH_VOLTAGE_C,
    VAR_LIN_TO_CHARGER0_BAT_VOLTAGE_C,
    VAR_LIN_TO_CHARGER0_SILENT_MODE_C,

    // LIN TOPTRON CHARGER0 INFO
    VAR_LIN_TO_CHARGER0_CH_CURRENT_I,
    VAR_LIN_TO_CHARGER0_SILENT_MODE_I,
    VAR_LIN_TO_CHARGER0_REDUSED_PWR_I,
    VAR_LIN_TO_CHARGER0_ERROR_I,
    VAR_LIN_TO_CHARGER0_CH_ACTIVE_I,

    //LIN TOPTRON LIGHTBOX
    VAR_LIN_TO_LIGHTBOX_LIGHT_BED_L_C,
    VAR_LIN_TO_LIGHTBOX_LIGHT_BED_R_C,
    VAR_LIN_TO_LIGHTBOX_LIGHT_CEILING_C,
    VAR_LIN_TO_LIGHTBOX_LIGHT_WALL_C,

    VAR_LIN_TO_LIGHTBOX_TEMP_OUT_I,
    VAR_LIN_TO_LIGHTBOX_TEMP_IN_I,
    VAR_LIN_TO_LIGHTBOX_BAT_VOLT_I,
    VAR_LIN_TO_LIGHTBOX_FRESH_WTR_LVL_I,
    VAR_LIN_TO_LIGHTBOX_LIGHT_BED_L_I,
    VAR_LIN_TO_LIGHTBOX_LIGHT_BED_R_I,
    VAR_LIN_TO_LIGHTBOX_LIGHT_CEILING_I,
    VAR_LIN_TO_LIGHTBOX_LIGHT_WALL_I,
    VAR_LIN_TO_LIGHTBOX_MAINS_CONN_I,

    // LIN CONTROL PANEL FOR HOBBY by Dataschalt
    VAR_LIN_DA_CTRLP_LED1_C,
    VAR_LIN_DA_CTRLP_LED2_C,
    VAR_LIN_DA_CTRLP_LED3_C,
    VAR_LIN_DA_CTRLP_LED4_C,
    VAR_LIN_DA_CTRLP_LED5_C,
    VAR_LIN_DA_CTRLP_LED6_C,
    VAR_LIN_DA_CTRLP_LED7_C,
    VAR_LIN_DA_CTRLP_LED8_C,
    VAR_LIN_DA_CTRLP_LED9_C,
    VAR_LIN_DA_CTRLP_LED10_C,
    VAR_LIN_DA_CTRLP_LED11_C,
    VAR_LIN_DA_CTRLP_LED12_C,
    VAR_LIN_DA_CTRLP_LED13_C,
    VAR_LIN_DA_CTRLP_LED14_C,

    VAR_LIN_DA_CTRLP_BUTTON1_I,
    VAR_LIN_DA_CTRLP_BUTTON2_I,
    VAR_LIN_DA_CTRLP_BUTTON3_I,
    VAR_LIN_DA_CTRLP_BUTTON4_I,
    VAR_LIN_DA_CTRLP_BUTTON5_I,
    VAR_LIN_DA_CTRLP_BUTTON6_I,
    VAR_LIN_DA_CTRLP_BUTTON7_I,
    VAR_LIN_DA_CTRLP_BUTTON8_I,
    VAR_LIN_DA_CTRLP_BUTTON9_I,
    VAR_LIN_DA_CTRLP_BUTTON10_I,
    VAR_LIN_DA_CTRLP_BUTTON11_I,
    VAR_LIN_DA_CTRLP_BUTTON12_I,
    VAR_LIN_DA_CTRLP_BUTTON13_I,
    VAR_LIN_DA_CTRLP_BUTTON14_I,
    VAR_LIN_DA_CTRLP_BUTTON15_I,
    VAR_LIN_DA_CTRLP_BUTTON16_I,
    VAR_LIN_DA_CTRLP_BUTTON17_I,

    //Diagnostic Hella Battery Sensor
    //VAR_LIN_DIA_HE_IBS0_BAT_SEN_C,
    //VAR_LIN_DIA_HE_IBS0_BAT_TYPE_C,

    VAR_LIN_DIA_HE_IBS0_CNOMINAL_I,
    VAR_LIN_DIA_HE_IBS0_BATTYPE_I,

    //Diagnostic Hella PID
    VAR_LIN_DIA_HE_IBS0_PID_NAD_I,
    VAR_LIN_DIA_HE_IBS0_PID_PCI_I,
    VAR_LIN_DIA_HE_IBS0_PID_RSID_I,
    VAR_LIN_DIA_HE_IBS0_PID_SUP_ID_I,
    VAR_LIN_DIA_HE_IBS0_PID_FUN_ID_I,
    VAR_LIN_DIA_HE_IBS0_PID_VAR_ID_I,

    //Diagnostic TRUMA Vario Heater PID
    VAR_LIN_DIA_TR_VH_PID_NAD_I,
    VAR_LIN_DIA_TR_VH_PID_PCI_I,
    VAR_LIN_DIA_TR_VH_PID_RSID_I,
    VAR_LIN_DIA_TR_VH_PID_SUP_ID_I,
    VAR_LIN_DIA_TR_VH_PID_FUN_ID_I,
    VAR_LIN_DIA_TR_VH_PID_VAR_ID_I,


	VAR_PSTORE_ENUM_COUNT

} pstore_table_t;

//------------------------------------------------------------------------------
// Public types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------
void     pstore_Initialize( void );
int32_t  pstore_SetValue(    pstore_table_t eIndex, int32_t i32Value );
int32_t  pstore_GetValue(    pstore_table_t eIndex );
uint8_t  pstore_u8GetValue(  pstore_table_t eIndex );
int8_t   pstore_s8GetValue(  pstore_table_t eIndex );
uint16_t pstore_u16GetValue( pstore_table_t eIndex );
int16_t  pstore_s16GetValue( pstore_table_t eIndex );

//------------------------------------------------------------------------------
// Public Macros
//------------------------------------------------------------------------------


#endif // PSTORE_H

