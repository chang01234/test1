//------------------------------------------------------------------------------
// Module:      msglin_func.c
//
//------------------------------------------------------------------------------
// Description:
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
#include "teleco.h"
#include "teleair.h"
#include "kathrein.h"
#include "tenhaaft.h"
#include "alde.h"
#include "msglin_func.h"

//------------------------------------------------------------------------------
// Local constants
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
uint8_t diag_transmit_sid;

//------------------------------------------------------------------------------
// Local types.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/******************************************************************************
 *                 Section For All Receive Function Handlers
 * ****************************************************************************/

// Function:    MsgLin_HE_Bat_IBS_Frm2_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Hella battery sensor Info
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void  MsgLin_HE_Bat_IBS_Frm2_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    hella_ibs_frm2_t info;
    hella_ibs_frm2_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_BAT_CUR_I,  info.battery_current );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_BAT_VLT_I,  info.battery_voltage );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_BAT_TEMP_I, info.battery_temperature );
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_Bat_IBS_Frm5_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Hella battery sensor Info
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void  MsgLin_HE_Bat_IBS_Frm5_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    hella_ibs_frm5_t info;
    hella_ibs_frm5_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_STATE_CHG_I,      info.state_of_charge );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_STATE_HLTH_I,     info.state_of_health );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_EST_VOLT_DROP_I, info.est_voltage_drop );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_CHG_VLT_I,       info.opt_charging_volt );
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_Bat_IBS_Frm6_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Hella battery sensor Info
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void  MsgLin_HE_Bat_IBS_Frm6_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    hella_ibs_frm6_t info;
    hella_ibs_frm6_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_AVAIL_CAP_I,     info.avail_capacity );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_DISCHARGE_AH_I,  info.discharge_ah );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_NOMINAL_CAP_I,   info.nominal_capacity );
        MsgLin_Update_PStore( VAR_LIN_HE_IBS0_RECAL_I,         info.recalibrated );
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_Wtr_Htr_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Truma Water heater Info
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TR_Wtr_Htr_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    truma_waterheater_info_t info;
    uint16_t airTemp;
    truma_water_heater_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN_TR_WTR_HTR_TARGET_TEMP_I,      info.temp );
        MsgLin_Update_PStore( VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_I, info.energy_sel );
        MsgLin_Update_PStore( VAR_LIN_TR_WTR_HTR_POWER_LIMIT_I,      info.power_lim );
        MsgLin_Update_PStore( VAR_LIN_TR_WTR_HTR_FROST_CTRL_ON_I,    info.frost_ctrl );
        MsgLin_Update_PStore( VAR_LIN_TR_WTR_HTR_EL_AVAILABLE_I,     info.el_aval );
        MsgLin_Update_PStore( VAR_LIN_TR_WTR_HTR_WINDOW_SW_CLR_I,    info.sw_clr );
        MsgLin_Update_PStore( VAR_LIN_TR_WTR_HTR_MANUAL_MODE_I,      info.manual_mode );

        /* Special handling for Truma. If manual mode, then heater is operated with CP+ remote control. */
        if (info.manual_mode)
        {
            /* Copy Info to Ctrl */
            pstore_SetValue(VAR_LIN_TR_WTR_HTR_TARGET_TEMP_C, info.temp);
            pstore_SetValue(VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C, info.energy_sel);
            pstore_SetValue(VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C, info.power_lim);

            /* Air heater energy selection must follow water heater energy selection */
            pstore_SetValue(VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C, info.energy_sel);

            /* Air temp may have changed. Read Info and copy it to ctrl. */
            airTemp = pstore_u16GetValue(VAR_LIN_TR_AIR_HTR_TARGET_TEMP_I);
            pstore_SetValue(VAR_LIN_TR_AIR_HTR_TARGET_TEMP_C, airTemp);
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_Air_Htr_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Truma Air heater Info
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TR_Air_Htr_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    truma_airheater_info_t info;
    truma_air_heater_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN_TR_AIR_HTR_TARGET_TEMP_I,      info.temp );
        MsgLin_Update_PStore( VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_I, info.energy_sel );
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TO_Charger0_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Toptron Charger0 Info
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TO_Charger0_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    toptron_el603_charger_info_t info;
    toptron_charger_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN_TO_CHARGER0_CH_CURRENT_I,  info.charging_current );
        MsgLin_Update_PStore( VAR_LIN_TO_CHARGER0_SILENT_MODE_I, info.silent_mode );
        MsgLin_Update_PStore( VAR_LIN_TO_CHARGER0_REDUSED_PWR_I, info.reduced_power );
        MsgLin_Update_PStore( VAR_LIN_TO_CHARGER0_ERROR_I,       info.error_active );
        MsgLin_Update_PStore( VAR_LIN_TO_CHARGER0_CH_ACTIVE_I,   info.charging_acitve );
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_DO_AC_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Domectic AC Info
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_DO_AC_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    static int clear_sync[3] = {0};
    dometic_ac_info_t info;
    dometic_ac_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // TODO: This clear_sync code rely on that the sequence is
        // info_frame(local_change) -> ctrl_frame(sync) -> info_frame(no local change)
        // Verify or think again if this is correct.
        if (clear_sync[u8LinBus] != 0)
        {
            /* Clear sync bit in ctrl frame */
            pstore_SetValue( VAR_LIN0_DO_AC_SYNC_FRAME_C, 0 );
            clear_sync[u8LinBus] = 0;
        }

        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_MODE_A_I,       info.mode_a);
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_FAN_MODE_I,     info.fan_mode );
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_LIGHT_STATUS_I, info.light_status );
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_POWER_I,        info.power );
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_MODE_B_I,       info.mode_b );
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_FAN_SPEED_I,    info.fan_speed );
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_TARGET_TEMP_I,  info.target_temp );
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_LIGHT_DIM_LVL_I,info.dim_lvl );
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_LOCAL_CHANGE_I, info.local_change);
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_CI_ERROR_I,     info.ci_error);

        //LOG(I, "Fanmode=%d", info.fan_mode);
        
        /* Update the overall mode parameter, a combination of two mode bit fields. */
        MsgLin_Update_PStore( VAR_LIN0_DO_AC_MODE_CTRL_I,    dometic_ac_mode_get());

        /* Special handling for Dometic. Value might have changed by remote control or local buttons. */
        if (info.local_change)
        {
            /* Copy Info to Ctrl */
            pstore_SetValue( VAR_LIN0_DO_AC_MODE_A_C,       info.mode_a);
            pstore_SetValue( VAR_LIN0_DO_AC_FAN_MODE_C,     info.fan_mode );
            pstore_SetValue( VAR_LIN0_DO_AC_LIGHT_STATUS_C, info.light_status );
            pstore_SetValue( VAR_LIN0_DO_AC_POWER_C,        info.power );
            pstore_SetValue( VAR_LIN0_DO_AC_MODE_B_C,       info.mode_b );
            pstore_SetValue( VAR_LIN0_DO_AC_FAN_SPEED_C,    info.fan_speed );
            pstore_SetValue( VAR_LIN0_DO_AC_TARGET_TEMP_C,  info.target_temp );
            pstore_SetValue( VAR_LIN0_DO_AC_LIGHT_DIM_LVL_C,info.dim_lvl );

            /* Set sync frame */
            pstore_SetValue( VAR_LIN0_DO_AC_SYNC_FRAME_C, 1 );

            msglin_TxReq(u8LinBus, AC_CTRL_ID);
        }
    }
}

#if 0
 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SHARC_WH_I_RxHandle
 //------------------------------------------------------------------------------
 // Description: Frame received for Domectic SHARC Water Heater Info
 //------------------------------------------------------------------------------
 // Return:      None.
 //------------------------------------------------------------------------------
  void MsgLin_DO_SHARC_WH_I_RxHandle
 (
     uint8_t    u8LinBus,    // in: LIN bus the message was received from
     uint8_t*   frame        // in: Rx data
 )
 {
     // Extract message content
     static int clear_sync[3] = {0};
     dometic_sharc_wtr_info_t info;
     dometic_sharc_wh_I_Extract( &info, frame );

     // Which bus?
     if (u8LinBus == 0)
     {
         // TODO: This clear_sync code rely on that the sequence is
         // info_frame(local_change) -> ctrl_frame(sync) -> info_frame(no local change)
         // Verify or think again if this is correct.
         if (clear_sync[u8LinBus] != 0)
         {
             /* Clear sync bit in ctrl frame */
             pstore_SetValue( VAR_LIN0_DO_SHARC_WH_SYNC_C, 0 );
             clear_sync[u8LinBus] = 0;
         }

         // Copy content to the application parameter storage
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_MODE_I,                 info.mode);
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_SLEEP_ON_I,             info.sleep_on );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_ONLINE_I,               info.online );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_ENERGY_SOURCE_I,        info.energy_source );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_ACTIVE_FAULT_CODE_I,    info.active_fault_code );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_WARNING_FAULT_CODE_I,   info.warning_fault_act );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_AH_CRI_FAULT_ACTIVE_I,  info.ah_cri_fault_act );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_WH_CRI_FAULT_ACTIVE_I,  info.wh_cri_fault_act );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_REMOTE_CHANGE_I,        info.remote_change );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_LOCAL_CHANGE_I,         info.local_change );
         MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_RESPONSE_ERROR_I,       info.response_error );

         /* Special handling for Dometic SHARC Water Heater. Value might have changed by remote control or local buttons. */
         if (info.local_change)
         {
             /* Copy Info to Ctrl */
             pstore_SetValue( VAR_LIN0_DO_SHARC_WH_MODE_C,              info.mode );
             pstore_SetValue( VAR_LIN0_DO_SHARC_WH_SLEEP_ON_C,          info.sleep_on );
             pstore_SetValue( VAR_LIN0_DO_SHARC_WH_WAKEUP_C,            info.online );
             pstore_SetValue( VAR_LIN0_DO_SHARC_WH_ENERGY_SOURCE_C,     info.energy_source );

             /* lock and c_mode needs to identify */

             /* Set sync frame */
             pstore_SetValue( VAR_LIN0_DO_SHARC_WH_SYNC_C, 1 );
             clear_sync[u8LinBus] = 1;

             msglin_TxReq(u8LinBus, SH_WH_CTRL_ID);
         }
     }
 }

  //------------------------------------------------------------------------------
  // Function:    MsgLin_DO_SHARC_AH_I_RxHandle
  //------------------------------------------------------------------------------
  // Description: Frame received for Domectic SHARC Air  Heater Info
  //------------------------------------------------------------------------------
  // Return:      None.
  //------------------------------------------------------------------------------
   void MsgLin_DO_SHARC_AH_I_RxHandle
  (
      uint8_t    u8LinBus,    // in: LIN bus the message was received from
      uint8_t*    frame        // in: Rx data
  )
  {
      // Extract message content
      static int clear_sync[3] = {0};
      dometic_sharc_ah_info_t info;
      dometic_sharc_ah_I_Extract( &info, frame );

      // Which bus?
      if (u8LinBus == 0)
      {
          // TODO: This clear_sync code rely on that the sequence is
          // info_frame(local_change) -> ctrl_frame(sync) -> info_frame(no local change)
          // Verify or think again if this is correct.
          if (clear_sync[u8LinBus] != 0)
          {
              /* Clear sync bit in ctrl frame */
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_SYNC_C, 0 );
              clear_sync[u8LinBus] = 0;
          }

          // Copy content to the application parameter storage
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_TARGET_TEMP_I,                 info.tar_room_temp);
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_SLEEP_ON_I,                    info.sleep_on );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_ONLINE_I,                      info.online );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_ENERGY_SOURCE_I,               info.energy_source );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_MODE_I,                        info.mode );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_SILENT_MODE_MAX_FAN_SPEED_I,   info.si_mod_max_fan_speed );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_VENT_MODE_MIN_FAN_SPEED_I,     info.vent_mod_min_fan_speed );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_AC_STATUS_I,                   info.ac_status );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_TIMER_OFF_I,                   info.air_htr_timer_off_status);
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_TIMER_ON_I,                    info.air_htr_timer_on_status );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_WH_TIMER_ON_I,                    info.wtr_htr_timer_on_status );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_REMOTE_CHANGE_I,               info.remote_change );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_LOCAL_CHANGE_I,                info.local_change );
          MsgLin_Update_PStore( VAR_LIN0_DO_SHARC_AH_RESPONSE_ERROR_I,              info.response_error );

          /* Special handling for Dometic SHARC Water Heater. Value might have changed by remote control or local buttons. */
          if (info.local_change)
          {
              /* Copy Info to Ctrl */
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_TARGET_TEMP_C,              info.tar_room_temp);
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_SLEEP_ON_C,                 info.sleep_on );
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_WAKEUP_C,                   info.online );
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_ENERGY_SOURCE_C,            info.energy_source );
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_MODE_C,                     info.mode);
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_SILENT_MODE_MAX_FAN_SPEED_C,info.si_mod_max_fan_speed );
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_VENT_MODE_MIN_FAN_SPEED_C,  info.vent_mod_min_fan_speed );
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_TIMER_OFF_C,                info.air_htr_timer_off_status );
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_TIMER_ON_C,                 info.air_htr_timer_on_status);
              pstore_SetValue( VAR_LIN0_DO_SHARC_WH_TIMER_ON_C,                 info.wtr_htr_timer_on_status );

              /* lock and c_mode needs to identify */

              /* Set sync frame */
              pstore_SetValue( VAR_LIN0_DO_SHARC_AH_SYNC_C, 1 );
              clear_sync[u8LinBus] = 1;

              msglin_TxReq(u8LinBus, SH_AH_CTRL_ID);
          }
      }
  }
#endif

//------------------------------------------------------------------------------
// Function:    MsgLin_TO_Lightbox_I_RxHandle
//------------------------------------------------------------------------------
// Description: Frame received for Toptron Lightbox
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TO_Lightbox_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    // Extract message content
    toptron_lightbox_info_t info;
    toptron_lightbox_I_Extract( &info, frame );

    // Which bus?
    if (u8LinBus == 0)
    {
        // Copy content to the application parameter storage
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_TEMP_OUT_I,      info.temp_out );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_TEMP_IN_I,       info.temp_in );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_BAT_VOLT_I,      info.bat_voltage );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_FRESH_WTR_LVL_I, info.fresh_wtr_lvl );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_LIGHT_BED_L_I,   info.light_bed_left );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_LIGHT_BED_R_I,   info.light_bed_right );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_LIGHT_CEILING_I, info.light_ceiling );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_LIGHT_WALL_I,    info.light_wall );
        MsgLin_Update_PStore( VAR_LIN_TO_LIGHTBOX_MAINS_CONN_I,    info.mains_connected );
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella PID Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_pid_diag_info_t info;
    uint8_t req_sid;

    DGN_TO_BITS( req_sid, frame[3], 0, 8 );
    if (diag_transmit_sid == req_sid)
    {
        //Negative Response
        LOG(I, "Negative Response Received For MsgLin_HE_PID_Diag_I_RxHandle");
    }
    else
    {
        hella_pid_diag_I_Extract( &info, frame );
        // Which bus?
        if (u8LinBus == 0)
        {
            // Copy content to the application parameter storage

            MsgLin_Update_PStore( VAR_LIN_DIA_HE_IBS0_PID_NAD_I,      info.nad );
            MsgLin_Update_PStore( VAR_LIN_DIA_HE_IBS0_PID_PCI_I,      info.pci );
            MsgLin_Update_PStore( VAR_LIN_DIA_HE_IBS0_PID_RSID_I,     info.rsid );
            MsgLin_Update_PStore( VAR_LIN_DIA_HE_IBS0_PID_SUP_ID_I,   info.supplier_id );
            MsgLin_Update_PStore( VAR_LIN_DIA_HE_IBS0_PID_FUN_ID_I,   info.func_id );
            MsgLin_Update_PStore( VAR_LIN_DIA_HE_IBS0_PID_VAR_ID_I,   info.var_id );

            LOG(W, "HELLA func_id=%x", info.func_id);
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatCapRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella PID Master Battery Cap Read Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatCapRead_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_bcap_re_diag_info_t info;
    uint8_t req_sid;

    DGN_TO_BITS( req_sid, frame[3], 0, 8 );
    if (diag_transmit_sid == req_sid)
    {
        //Negative Response
        LOG(I, "Negative Response Received For MsgLin_HE_BatCapRead_Diag_I_RxHandle");
    }
    else
    {
        hella_batcapread_diag_I_Extract( &info, frame );
        MsgLin_Update_PStore(VAR_LIN_DIA_HE_IBS0_CNOMINAL_I, info.lid);

        LOG(W, "BatteryCapacity=%x", info.lid);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatCapWrite_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella PID Master Battery Cap Write Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_BatCapWrite_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_bcap_wr_diag_info_t info;
    uint8_t req_sid;

    DGN_TO_BITS( req_sid, frame[3], 0, 8 );
    if (diag_transmit_sid == req_sid)
    {
        //Negative Response
        LOG(I, "Negative Response Received For MsgLin_HE_BatCapWrite_Diag_I_RxHandle");
    }
    else
    {
        hella_batcapwrite_diag_I_Extract( &info, frame );
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
            LOG(I, "sid:     0x%x", info.sid);
            LOG(I, "lid:  0x%x", info.lid);
            MsgLin_Update_PStore(VAR_LIN_DIA_HE_IBS0_CNOMINAL_I, info.C_Nom);
            LOG(I, "data1: 0x%x", info.C_Nom);
            LOG(I, "data2:  0x%x", info.data2);
            LOG(I, "data3: 0x%x", info.data3);
            LOG(I, "data4:  0x%x", info.data4);
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTypeRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella PID Master Battery Type Read Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_BatTypeRead_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_btype_re_diag_info_t info;
    uint8_t req_sid;

    DGN_TO_BITS( req_sid, frame[3], 0, 8 );
    if (diag_transmit_sid == req_sid)
    {
        //Negative Response
        LOG(I, "Negative Response Received For MsgLin_HE_BatTypeRead_Diag_I_RxHandle");
    }
    else
    {
        hella_battyperead_diag_I_Extract( &info, frame );
        // Which bus?
        if (u8LinBus == 0)
        {
            if (info.lid == 0xA)
            {
                LOG(W, "Battype=Starter");
            }
            else if (info.lid == 0x14)
            {
                LOG(W, "Battype=Gel");
            }
            else if (info.lid == 0x1E)
                LOG(W, "Battype=AGM");
        }

        MsgLin_Update_PStore(VAR_LIN_DIA_HE_IBS0_BATTYPE_I, info.lid);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTypeWrite_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella PID Master Battery Type Write Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_BatTypeWrite_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_btype_wr_diag_info_t info;
    uint8_t req_sid;

    DGN_TO_BITS( req_sid, frame[3], 0, 8 );
    if (diag_transmit_sid == req_sid)
    {
        //Negative Response
        LOG(I, "Negative Response Received For MsgLin_HE_BatTypeWrite_Diag_I_RxHandle");
    }
    else
    {
        hella_battypewrite_diag_I_Extract( &info, frame );
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
            LOG(I, "sid:     0x%x", info.sid);
            LOG(I, "lid:  0x%x", info.lid);
            LOG(I, "data1: 0x%x", info.data1); // Battery type
            LOG(I, "data2:  0x%x", info.data2);
            LOG(I, "data3: 0x%x", info.data3);
            LOG(I, "data4:  0x%x", info.data4);

            // Copy content to the application parameter storage
            MsgLin_Update_PStore(VAR_LIN_DIA_HE_IBS0_BATTYPE_I, info.data1);
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTableState_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella PID Master Battery Table State Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_BatTableState_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_btable_st_diag_info_t info;
    uint8_t req_sid;

    DGN_TO_BITS( req_sid, frame[3], 0, 8 );
    if (diag_transmit_sid == req_sid)
    {
        //Negative Response
        LOG(I, "Negative Response Received For MsgLin_HE_BatTableState_Diag_I_RxHandle");
    }
    else
    {
        hella_battablestate_diag_I_Extract( &info, frame );
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
            LOG(I, "sid:     0x%x", info.sid);
            LOG(I, "lid:  0x%x", info.lid);
            LOG(I, "data1: 0x%x", info.data1);
            LOG(I, "data2:  0x%x", info.data2);
            LOG(I, "data3: 0x%x", info.data3);
            LOG(I, "data4:  0x%x", info.data4);
        }
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTableOnOff_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Table On Off Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatTableOnOff_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_btable_onoff_diag_info_t info;
    hella_btableonoff_diag_I_Extract( &info, frame );
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
        LOG(I, "sid:     0x%x", info.sid);
        LOG(I, "lid:  0x%x",    info.lid);
        LOG(I, "data1: 0x%x",   info.data1);
        LOG(I, "data2:  0x%x",  info.data2);
        LOG(I, "data3: 0x%x",   info.data3);
        LOG(I, "data4:  0x%x",  info.data4);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_U0MinMaxRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery U0 Minimum and Maximum Read Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_U0MinMaxRead_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_bu0minmax_re_diag_info_t info;
    hella_bu0minmaxread_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",         info.nad);
        LOG(I, "pci:     0x%x",         info.pci);
        LOG(I, "sid:     0x%x",         info.sid);
        LOG(I, "U0 Minimum:  0x%x",     info.u0min);
        LOG(I, "U0 Maximum: 0x%x",      info.u0max);
        LOG(I, "data4:  0x%x",          info.data4);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_U0MinMaxWrite_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella U0 Minimum and Maximum Write Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_U0MinMaxWrite_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_bu0minmax_wr_diag_info_t info;
    hella_bu0minmaxwrite_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",     info.nad);
        LOG(I, "pci:     0x%x",     info.pci);
        LOG(I, "sid:     0x%x",     info.sid);
        LOG(I, "lid:  0x%x",        info.lid);
        LOG(I, "U0 Minimum: 0x%x",  info.u0min);
        LOG(I, "U0 Maximum:  0x%x", info.u0max);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_IBattQuiescentRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella PID Master Battery Type Write Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_IBattQuiescentRead_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_ibquiescent_re_diag_info_t info;
    hella_ibattquiescentread_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",         info.nad);
        LOG(I, "pci:     0x%x",         info.pci);
        LOG(I, "sid:     0x%x",         info.sid);
        LOG(I, "ibattquiescent:  0x%x", info.ibattquiescent);
        LOG(I, "ichargemin: 0x%x",      info.ichargemin);
        LOG(I, "data2:  0x%x",          info.data2);
        LOG(I, "data3: 0x%x",           info.data3);
        LOG(I, "data4:  0x%x",          info.data4);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_IBattQuiescentWrite_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella IBattery Quiescent Write Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void  MsgLin_HE_IBattQuiescentWrite_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    hella_ibquiescent_wr_diag_info_t info;
    hella_ibattquiescentwrite_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",         info.nad);
        LOG(I, "pci:     0x%x",         info.pci);
        LOG(I, "sid:     0x%x",         info.sid);
        LOG(I, "lid:  0x%x",            info.lid);
        LOG(I, "ibattquiescent: 0x%x",  info.ibattquiescent);
        LOG(I, "ichargemin:  0x%x",     info.ichargemin);
        LOG(I, "data3: 0x%x",           info.data3);
        LOG(I, "data4:  0x%x",          info.data4);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_VH_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_VH_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_VAR_ID_I,   info.var_id );

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
// Function:    MsgLin_TR_VH_SerialRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Serial Read Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_VH_SerialRead_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_serial_diag_info_t info;
    truma_generic_serial_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",         info.nad);
        LOG(I, "pci:     0x%x",         info.pci);
        LOG(I, "rsid:     0x%x",        info.rsid);
        LOG(I, "Serial Number:  0x%"PRIx32"",  info.serial_number);
        LOG(I, "data8: 0x%x",           info.data8);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_VH_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Assign Nad Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_VH_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_VH_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Vario Heater Assign Frame Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_VH_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_frame_diag_info_t info;
    truma_generic_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Saphir AC PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_AC_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_AC_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_AC_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_AC_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_AC_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_AC_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TR_SapCom_AC_SerialRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma AC Serial Read Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_SerialRead_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_serial_diag_info_t info;
    truma_generic_serial_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",         info.nad);
        LOG(I, "pci:     0x%x",         info.pci);
        LOG(I, "rsid:     0x%x",        info.rsid);
        LOG(I, "Serial Number:  0x%"PRIx32"",  info.serial_number);
        LOG(I, "data8: 0x%x",           info.data8);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_CurrentError_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma AC Current Error Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_CurrentError_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_cerror_diag_info_t info;
    truma_generic_currenterror_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Format:  0x%x", info.format);
        LOG(I, "class: 0x%x",   info.clas);
        LOG(I, "data 3: 0x%x",  info.data3);
        LOG(I, "Code: 0x%x",    info.code);
        LOG(I, "data 5: 0x%x",  info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma AC Assign Nad Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma AC Assign Frame Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_frame_diag_info_t info;
    truma_generic_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Aventa Comfort AC PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_AC_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_AC_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_AC_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_AC_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_AC_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_AC_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TR_AveCom_AC_SerialRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma AC Serial Read Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_SerialRead_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_serial_diag_info_t info;
    truma_generic_serial_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",         info.nad);
        LOG(I, "pci:     0x%x",         info.pci);
        LOG(I, "rsid:     0x%x",        info.rsid);
        LOG(I, "Serial Number:  0x%"PRIx32"",  info.serial_number);
        LOG(I, "data8: 0x%x",           info.data8);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_CurrentError_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma AC Current Error Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_CurrentError_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_cerror_diag_info_t info;
    truma_generic_currenterror_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Format:  0x%x", info.format);
        LOG(I, "class: 0x%x",   info.clas);
        LOG(I, "data 3: 0x%x",  info.data3);
        LOG(I, "Code: 0x%x",    info.code);
        LOG(I, "data 5: 0x%x",  info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma AC Assign Nad Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Comfort AC Assign Frame
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_frame_diag_info_t info;
    truma_generic_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Aventa Eco AC PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_AC_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_AC_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_AC_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_AC_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_AC_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_AC_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TR_AveEco_AC_SerialRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco AC Serial Read
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_SerialRead_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_serial_diag_info_t info;
    truma_generic_serial_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x",         info.nad);
        LOG(I, "pci:     0x%x",         info.pci);
        LOG(I, "rsid:     0x%x",        info.rsid);
        LOG(I, "Serial Number:  0x%"PRIx32"",  info.serial_number);
        LOG(I, "data8: 0x%x",           info.data8);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_CurrentError_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco AC Current Error
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_CurrentError_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_cerror_diag_info_t info;
    truma_generic_currenterror_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Format:  0x%x", info.format);
        LOG(I, "class: 0x%x",   info.clas);
        LOG(I, "data 3: 0x%x",  info.data3);
        LOG(I, "Code: 0x%x",    info.code);
        LOG(I, "data 5: 0x%x",  info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco AC Assign Nad
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco AC Assign Frame
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_frame_diag_info_t info;
    truma_generic_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_Combi_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Combi Saphir Comfort PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_Combi_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_COMBI_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_COMBI_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_COMBI_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_COMBI_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_COMBI_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_SAPCOM_COMBI_PID_VAR_ID_I,   info.var_id ); */

        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "sup_id:  0x%x", info.supplier_id);
        LOG(I, "func_id: 0x%x", info.func_id);
        LOG(I, "var_id:  0x%x", info.var_id);
    }
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_Combi_SerialRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Serial Read
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_Combi_SerialRead_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_serial_diag_info_t info;
    truma_generic_serial_diag_I_Extract( &info, frame );
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
        LOG(I, "Serial Number:  0x%"PRIx32"", info.serial_number);
        LOG(I, "data8: 0x%x", info.data8);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_Combi_FwVer_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Saphire Comfort Firmware Version
//              Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_Combi_FwVer_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_fwver_diag_info_t info;
    truma_generic_fwver_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Major:  0x%x",  info.major);
        LOG(I, "Minor: 0x%x",   info.minor);
        LOG(I, "Revision:  0x%x", info.revision);
        LOG(I, "Build:  0x%x", info.build);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_Combi_CurrentError_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Current Error
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_Combi_CurrentError_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_cerror_diag_info_t info;
    truma_generic_currenterror_diag_I_Extract( &info, frame );
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
        LOG(I, "Format:  0x%x", info.format);
        LOG(I, "class: 0x%x",   info.clas);
        LOG(I, "data 3: 0x%x",  info.data3);
        LOG(I, "Code: 0x%x",    info.code);
        LOG(I, "data 5: 0x%x",  info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_Combi_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Assign Nad
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_Combi_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_Combi_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Assign Frame
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_Combi_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_frame_diag_info_t info;
    truma_generic_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_Combi_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Combi Aventa Comfort PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_Combi_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_COMBI_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_COMBI_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_COMBI_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_COMBI_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_COMBI_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVECOM_COMBI_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TR_AveCom_Combi_SerialRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Serial Read
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_Combi_SerialRead_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_serial_diag_info_t info;
    truma_generic_serial_diag_I_Extract( &info, frame );
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
        LOG(I, "Serial Number:  0x%"PRIx32"", info.serial_number);
        LOG(I, "data8: 0x%x", info.data8);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_Combi_FwVer_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Combi Aventa Comfort Firmware Version Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_Combi_FwVer_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_fwver_diag_info_t info;
    truma_generic_fwver_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Major:  0x%x",  info.major);
        LOG(I, "Minor: 0x%x",   info.minor);
        LOG(I, "Revision:  0x%x", info.revision);
        LOG(I, "Build:  0x%x", info.build);
    }

}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_Combi_CurrentError_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Current Error
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_Combi_CurrentError_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_cerror_diag_info_t info;
    truma_generic_currenterror_diag_I_Extract( &info, frame );
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
        LOG(I, "Format:  0x%x", info.format);
        LOG(I, "class: 0x%x",   info.clas);
        LOG(I, "data 3: 0x%x",  info.data3);
        LOG(I, "Code: 0x%x",    info.code);
        LOG(I, "data 5: 0x%x",  info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_Combi_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Assign Nad
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_Combi_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_Combi_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Assign Frame
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_Combi_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_frame_diag_info_t info;
    truma_generic_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_Combi_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Combi Aventa Eco PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_Combi_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_COMBI_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_COMBI_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_COMBI_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_COMBI_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_COMBI_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_AVEECO_COMBI_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TR_AveEco_Combi_SerialRead_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Eco Serial Read
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_Combi_SerialRead_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_serial_diag_info_t info;
    truma_generic_serial_diag_I_Extract( &info, frame );
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
        LOG(I, "Serial Number:  0x%"PRIx32"", info.serial_number);
        LOG(I, "data8: 0x%x", info.data8);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_Combi_FwVer_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Combi Aventa ECO Firmware Version Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_Combi_FwVer_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_fwver_diag_info_t info;
    truma_generic_fwver_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Major:  0x%x",  info.major);
        LOG(I, "Minor: 0x%x",   info.minor);
        LOG(I, "Revision:  0x%x", info.revision);
        LOG(I, "Build:  0x%x", info.build);
    }

}
//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_Combi_CurrentError_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Eco Current Error
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_Combi_CurrentError_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_cerror_diag_info_t info;
    truma_generic_currenterror_diag_I_Extract( &info, frame );
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
        LOG(I, "Format:  0x%x", info.format);
        LOG(I, "class: 0x%x",   info.clas);
        LOG(I, "data 3: 0x%x",  info.data3);
        LOG(I, "Code: 0x%x",    info.code);
        LOG(I, "data 5: 0x%x",  info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_Combi_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Eco Assign Nad
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_Combi_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_Combi_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Eco Assign Frame
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_Combi_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_frame_diag_info_t info;
    truma_generic_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_CPplus_Combi_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Combi CP Plus PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_CPplus_Combi_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_pid_diag_info_t info;
    truma_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_CPPLUS_COMBI_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_CPPLUS_COMBI_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_CPPLUS_COMBI_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_CPPLUS_COMBI_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_CPPLUS_COMBI_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TR_CPPLUS_COMBI_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TR_CPplus_Combi_FwVer_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TRUMA Combi CP Plus Firmware Version Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_CPplus_Combi_FwVer_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_fwver_diag_info_t info;
    truma_generic_fwver_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Major:  0x%x",  info.major);
        LOG(I, "Minor: 0x%x",   info.minor);
        LOG(I, "Revision:  0x%x", info.revision);
        LOG(I, "Build:  0x%x", info.build);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_CPplus_Combi_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi CP Plus Assign Nad
//              Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_CPplus_Combi_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    truma_gen_nad_diag_info_t info;
    truma_generic_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO IFS PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_pid_diag_info_t info;
    teleco_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TE_IFS_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_IFS_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_IFS_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_IFS_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_IFS_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_IFS_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TE_IFS_UCOM32_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO IFS User Command 32 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM32_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom32_diag_info_t info;
    teleco_generic_ucom32_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "custom:  0x%x", info.custom);
        LOG(I, "data2:  0x%x",  info.data2);
        LOG(I, "data3:  0x%x",  info.data3);
        LOG(I, "data4:  0x%x",  info.data4);
        LOG(I, "data5:  0x%x",  info.data5);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM33_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO IFS User Command 33 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM33_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom33_diag_info_t info;
    teleco_generic_ucom33_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "hardware Type:  0x%x", info.hw_type);
        LOG(I, "hardware version:  0x%x",  info.hw_ver);
        LOG(I, "firmware type:  0x%x",  info.fw_type);
        LOG(I, "firmware version:  0x%x",  info.fw_ver);
        LOG(I, "firmware suppliar version:  0x%x",  info.fw_sub_ver);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM34_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO IFS User Command 34 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM34_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom34_diag_info_t info;
    teleco_generic_ucom34_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "custom digit 1:  0x%x", info.custom_digit1);
        LOG(I, "custom digit 0:  0x%x",  info.custom_digit0);
        LOG(I, "hardware version:  0x%x",  info.hw_ver);
        LOG(I, "firmware Type:  0x%x",  info.fw_type);
        LOG(I, "data5:  0x%x",  info.data5);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM35_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO IFS User Command 35 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM35_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom35_diag_info_t info;
    teleco_generic_ucom35_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "custom digit 1:  0x%x", info.fw_ver_major);
        LOG(I, "custom digit 0:  0x%x",  info.fw_ver_minor);
        LOG(I, "hardware version:  0x%x",  info.db_digit2);
        LOG(I, "firmware Type:  0x%x",  info.db_digit1);
        LOG(I, "data5:  0x%x",  info.db_digit0);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_ASAT_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Active Satellite PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_pid_diag_info_t info;
    teleco_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TE_ASAT_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_ASAT_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_ASAT_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_ASAT_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_ASAT_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_ASAT_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TE_ASAT_UCOM36_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO ASAT User Command 36 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM36_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom36_diag_info_t info;
    teleco_generic_ucom36_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "hardware type:  0x%x", info.hw_type);
        LOG(I, "hardware version:  0x%x",  info.hw_ver);
        LOG(I, "firmware type:  0x%x",  info.fw_type);
        LOG(I, "firmware version:  0x%x",  info.fw_ver);
        LOG(I, "firmware suppliar version:  0x%x",  info.fw_sub_ver);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_ASAT_UCOM37_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Activ Satellite User Command 37 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM37_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom37_diag_info_t info;
    teleco_generic_ucom37_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Custom digit1:  0x%x", info.custom_digit1);
        LOG(I, "custom digit0:  0x%x",  info.custom_digit0);
        LOG(I, "critical type1:  0x%x",  info.crit_type1);
        LOG(I, "critical type0:  0x%x",  info.crit_type0);
        LOG(I, "critical version5:  0x%x",  info.crit_ver5);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_ASAT_UCOM38_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO ASAT User Command 38 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM38_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom38_diag_info_t info;
    teleco_generic_ucom38_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "critical version 4:  0x%x", info.crit_ver4);
        LOG(I, "critical version 3:  0x%x",  info.crit_ver3);
        LOG(I, "critical version 2:  0x%x",  info.crit_ver2);
        LOG(I, "critical version 1:  0x%x",  info.crit_ver1);
        LOG(I, "critical version 0:  0x%x",  info.crit_ver0);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_ASAT_UCOM39_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Active Satellite User Command 39 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM39_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom39_diag_info_t info;
    teleco_generic_ucom39_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "sat_list1:  0x%x", info.sat_list1);
        LOG(I, "sat_list0:  0x%x",  info.sat_list0);
        LOG(I, "auto_onoff:  0x%x",  info.auto_onoff);
        LOG(I, "smart_conf1:  0x%x",  info.smart_conf1);
        LOG(I, "smart_conf0:  0x%x",  info.smart_conf0);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Flat Satellite PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_pid_diag_info_t info;
    teleco_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TE_FSATBT_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_FSATBT_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_FSATBT_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_FSATBT_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_FSATBT_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_FSATBT_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TE_FSATBT_UCOM36_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Flat Satellite BT User Command 36 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM36_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom36_diag_info_t info;
    teleco_generic_ucom36_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "hardware type:  0x%x", info.hw_type);
        LOG(I, "hardware version:  0x%x",  info.hw_ver);
        LOG(I, "firmware type:  0x%x",  info.fw_type);
        LOG(I, "firmware version:  0x%x",  info.fw_ver);
        LOG(I, "firmware suppliar version:  0x%x",  info.fw_sub_ver);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_UCOM37_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Flat Satellite BT User Command 37 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM37_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom37_diag_info_t info;
    teleco_generic_ucom37_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Custom digit1:  0x%x", info.custom_digit1);
        LOG(I, "custom digit0:  0x%x",  info.custom_digit0);
        LOG(I, "critical type1:  0x%x",  info.crit_type1);
        LOG(I, "critical type0:  0x%x",  info.crit_type0);
        LOG(I, "critical version5:  0x%x",  info.crit_ver5);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_UCOM38_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO FSAT BT User Command 38 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM38_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom38_diag_info_t info;
    teleco_generic_ucom38_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "critical version 4:  0x%x", info.crit_ver4);
        LOG(I, "critical version 3:  0x%x",  info.crit_ver3);
        LOG(I, "critical version 2:  0x%x",  info.crit_ver2);
        LOG(I, "critical version 1:  0x%x",  info.crit_ver1);
        LOG(I, "critical version 0:  0x%x",  info.crit_ver0);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_UCOM39_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Flat Satellite BT User Command 39 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM39_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom39_diag_info_t info;
    teleco_generic_ucom39_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "sat_list1:  0x%x", info.sat_list1);
        LOG(I, "sat_list0:  0x%x",  info.sat_list0);
        LOG(I, "Not_used:  0x%x",  info.auto_onoff);
        LOG(I, "smart_conf1:  0x%x",  info.smart_conf1);
        LOG(I, "smart_conf0:  0x%x",  info.smart_conf0);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Tele Satellite PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_pid_diag_info_t info;
    teleco_generic_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TE_TSATBT_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_TSATBT_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_TSATBT_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_TSATBT_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_TSATBT_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TE_TSATBT_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TE_TSATBT_UCOM36_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Tele Satellite BT User Command 36 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM36_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom36_diag_info_t info;
    teleco_generic_ucom36_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "hardware type:  0x%x", info.hw_type);
        LOG(I, "hardware version:  0x%x",  info.hw_ver);
        LOG(I, "firmware type:  0x%x",  info.fw_type);
        LOG(I, "firmware version:  0x%x",  info.fw_ver);
        LOG(I, "firmware suppliar version:  0x%x",  info.fw_sub_ver);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_UCOM37_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Tele Satellite BT User Command 37 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM37_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom37_diag_info_t info;
    teleco_generic_ucom37_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "Custom digit1:  0x%x", info.custom_digit1);
        LOG(I, "custom digit0:  0x%x",  info.custom_digit0);
        LOG(I, "critical type1:  0x%x",  info.crit_type1);
        LOG(I, "critical type0:  0x%x",  info.crit_type0);
        LOG(I, "critical version5:  0x%x",  info.crit_ver5);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_UCOM38_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO TSAT BT User Command 38 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM38_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom38_diag_info_t info;
    teleco_generic_ucom38_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "critical version 4:  0x%x", info.crit_ver4);
        LOG(I, "critical version 3:  0x%x",  info.crit_ver3);
        LOG(I, "critical version 2:  0x%x",  info.crit_ver2);
        LOG(I, "critical version 1:  0x%x",  info.crit_ver1);
        LOG(I, "critical version 0:  0x%x",  info.crit_ver0);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_UCOM39_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELECO Tele Satellite BT User Command 39 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM39_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    teleco_gen_ucom39_diag_info_t info;
    teleco_generic_ucom39_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "sat_list1:  0x%x", info.sat_list1);
        LOG(I, "sat_list0:  0x%x",  info.sat_list0);
        LOG(I, "Not_used:  0x%x",  info.auto_onoff);
        LOG(I, "smart_conf1:  0x%x",  info.smart_conf1);
        LOG(I, "smart_conf0:  0x%x",  info.smart_conf0);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TA_AC_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELAIR AC PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TA_AC_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    telair_ac_pid_diag_info_t info;
    telair_ac_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TA_AC_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TA_AC_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TA_AC_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TA_AC_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TA_AC_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TA_AC_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TA_AC_UCOM32_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELAIR AC User Command 32 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TA_AC_UCOM32_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    telair_ac_ucom32_diag_info_t info;
    telair_ac_ucom32_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "custom:  0x%x", info.custom);
        LOG(I, "data2:  0x%x",  info.data2);
        LOG(I, "data3:  0x%x",  info.data3);
        LOG(I, "data4:  0x%x",  info.data4);
        LOG(I, "data5:  0x%x",  info.data5);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_TA_AC_UCOM33_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic TELAIR AC User Command 33 Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TA_AC_UCOM33_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    telair_ac_ucom33_diag_info_t info;
    telair_ac_ucom33_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "hardware Type:  0x%x", info.hw_type);
        LOG(I, "hardware version:  0x%x",  info.hw_ver);
        LOG(I, "firmware type:  0x%x",  info.fw_type);
        LOG(I, "firmware version:  0x%x",  info.fw_ver);
        LOG(I, "firmware suppliar version:  0x%x",  info.fw_sub_ver);

    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_KA_Sat_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Kathrein CI Satelite Systems PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_KA_Sat_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    kathrein_sat_pid_diag_info_t info;
    kathrein_satelite_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_KA_SAT_PID_NAD_I,    info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_KA_SAT_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_KA_SAT_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_KA_SAT_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_KA_SAT_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_KA_SAT_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_KA_Sat_SwVer_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Kathrein CI Satelite System Software Version Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_KA_Sat_SwVer_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    kathrein_sat_sw_ver_diag_info_t info;
    kathrein_satelite_sw_ver_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
         * Temporally Print to  LOG
         */
        LOG(I, "Found:");
        LOG(I, "nad:     0x%x", info.nad);
        LOG(I, "pci:     0x%x", info.pci);
        LOG(I, "rsid:     0x%x",info.rsid);
        LOG(I, "language code:  0x%x", info.lan_code);
        LOG(I, "month:  0x%x",  info.month);
        LOG(I, "date:  0x%x",  info.date);
        LOG(I, "year:  0x%x",  info.year);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_DO_FJet_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Dometic Freshjet PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_DO_FJet_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    dometic_gen_pid_diag_info_t info;
    dometic_gen_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_DO_FJET_PID_NAD_I,    info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FJET_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FJET_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FJET_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FJET_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FJET_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_DO_FWel_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Dometic Freshwell PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_DO_FWel_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    dometic_gen_pid_diag_info_t info;
    dometic_gen_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_DO_FWEL_PID_NAD_I,    info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FWEL_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FWEL_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FWEL_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FWEL_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_DO_FWEL_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_TH_SAT_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Tenhaaft Satelite PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TH_SAT_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    tenhaaft_sat_pid_diag_info_t info;
    tenhaaft_sat_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO update the values to correct table once we know the destiation.
        /*MsgLin_Update_PStore( VAR_LIN_DIA_TH_SAT_PID_NAD_I,    info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_TH_SAT_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_TH_SAT_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_TH_SAT_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TH_SAT_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_TH_SAT_PID_VAR_ID_I,   info.var_id ); */

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
// Function:    MsgLin_AL_3020_PID_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Alde 3020 PID Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_AL_3020_PID_Diag_I_RxHandle
(
    uint8_t    u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    alde_3020_pid_diag_info_t info;
    alde_3020_pid_diag_I_Extract( &info, frame );
    // Which bus?
    if (u8LinBus == 0)
    {
        /*
        //TODO update the values to correct table once we know the destiation.
        MsgLin_Update_PStore( VAR_LIN_DIA_AL_3020_PID_NAD_I,      info.nad );
        MsgLin_Update_PStore( VAR_LIN_DIA_AL_3020_PID_PCI_I,      info.pci );
        MsgLin_Update_PStore( VAR_LIN_DIA_AL_3020_PID_RSID_I,     info.rsid );
        MsgLin_Update_PStore( VAR_LIN_DIA_AL_3020_PID_SUP_ID_I,   info.supplier_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_AL_3020_PID_FUN_ID_I,   info.func_id );
        MsgLin_Update_PStore( VAR_LIN_DIA_AL_3020_PID_VAR_ID_I,   info.var_id );*/

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
// Function:    MsgLin_AL_3020_AssignNad_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Alde 3020 Assign Nad Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_AL_3020_AssignNad_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    alde_3020_nad_diag_info_t info;
    alde_3020_assignnad_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

//------------------------------------------------------------------------------
// Function:    MsgLin_AL_3020_AssignFrame_Diag_I_RxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Alde 3020 Assign Frame Master Request Receive Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_AL_3020_AssignFrame_Diag_I_RxHandle
(
    uint8_t     u8LinBus,    // in: LIN bus the message was received from
    uint8_t*    frame        // in: Rx data
)
{
    alde_3020_frame_diag_info_t info;
    alde_3020_assignframe_diag_I_Extract( &info, frame );
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
        LOG(I, "rsid:    0x%x", info.rsid);
        LOG(I, "data 1:  0x%x", info.data1);
        LOG(I, "data 2:  0x%x", info.data2);
        LOG(I, "data 3:  0x%x", info.data3);
        LOG(I, "data 4:  0x%x", info.data4);
        LOG(I, "data 5:  0x%x", info.data5);
    }

}

 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SH_PID_Diag_I_RxHandle
 //------------------------------------------------------------------------------
 // Description: Diagnostic Dometic SHARC PID Receive Handler
 //
 //
 // NOTE:        Added by Dometic Sweden.
 //------------------------------------------------------------------------------
 // Return:      None
 //------------------------------------------------------------------------------
  void MsgLin_DO_SH_PID_Diag_I_RxHandle
 (
     uint8_t    u8LinBus,    // in: LIN bus the message was received from
     uint8_t*    frame        // in: Rx data
 )
 {
     dometic_sharc_pid_diag_info_t info;
     dometic_sharc_pid_diag_I_Extract( &info, frame );
     // Which bus?
     if (u8LinBus == 0)
     {
         //TODO update the values to correct table once we know the destiation.
         /*MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_NAD_I,      info.nad );
         MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_PCI_I,      info.pci );
         MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_RSID_I,     info.rsid );
         MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_SUP_ID_I,   info.supplier_id );
         MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_FUN_ID_I,   info.func_id );
         MsgLin_Update_PStore( VAR_LIN_DIA_TR_VH_PID_VAR_ID_I,   info.var_id );*/

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
 // Function:    MsgLin_DO_SH_SerialRead_Diag_I_RxHandle
 //------------------------------------------------------------------------------
 // Description: Diagnostic Dometic SHARC Serial Read Master Request Receive Handler
 //
 //
 // NOTE:        Added by Dometic Sweden.
 //------------------------------------------------------------------------------
 // Return:      None
 //------------------------------------------------------------------------------
  void MsgLin_DO_SH_SerialRead_Diag_I_RxHandle
 (
     uint8_t     u8LinBus,    // in: LIN bus the message was received from
     uint8_t*    frame        // in: Rx data
 )
 {
     dometic_sharc_serial_diag_info_t info;
     dometic_sharc_serial_diag_I_Extract( &info, frame );
     // Which bus?
     if (u8LinBus == 0)
     {
         //TODO update the values to correct table once we know the destiation.
         /*
          * Temporally Print to  LOG
          */
         LOG(I, "Found:");
         LOG(I, "nad:     0x%x",         info.nad);
         LOG(I, "pci:     0x%x",         info.pci);
         LOG(I, "rsid:     0x%x",        info.rsid);
         LOG(I, "Serial Number:  0x%"PRIx32"",  info.serial_number);
         LOG(I, "data8: 0x%x",           info.data8);
     }

 }

 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SH_AssignNad_Diag_I_RxHandle
 //------------------------------------------------------------------------------
 // Description: Diagnostic Dometic SHARC Assign Nad Master Request Receive Handler
 //
 //
 // NOTE:        Added by Dometic Sweden.
 //------------------------------------------------------------------------------
 // Return:      None
 //------------------------------------------------------------------------------
  void MsgLin_DO_SH_AssignNad_Diag_I_RxHandle
 (
     uint8_t     u8LinBus,    // in: LIN bus the message was received from
     uint8_t*    frame        // in: Rx data
 )
 {
     dometic_sharc_nad_diag_info_t info;
     dometic_sharc_assignnad_diag_I_Extract( &info, frame );
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
         LOG(I, "rsid:    0x%x", info.rsid);
         LOG(I, "data 1:  0x%x", info.data1);
         LOG(I, "data 2:  0x%x", info.data2);
         LOG(I, "data 3:  0x%x", info.data3);
         LOG(I, "data 4:  0x%x", info.data4);
         LOG(I, "data 5:  0x%x", info.data5);
     }

 }

 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SH_AssignFrame_Diag_I_RxHandle
 //------------------------------------------------------------------------------
 // Description: Diagnostic Dometic SHARC Assign Frame Master Request Receive Handler
 //
 //
 // NOTE:        Added by Dometic Sweden.
 //------------------------------------------------------------------------------
 // Return:      None
 //------------------------------------------------------------------------------
  void MsgLin_DO_SH_AssignFrame_Diag_I_RxHandle
 (
     uint8_t     u8LinBus,    // in: LIN bus the message was received from
     uint8_t*    frame        // in: Rx data
 )
 {
     dometic_sharc_frame_diag_info_t info;
     dometic_sharc_assignframe_diag_I_Extract( &info, frame );
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
         LOG(I, "rsid:    0x%x", info.rsid);
         LOG(I, "data 1:  0x%x", info.data1);
         LOG(I, "data 2:  0x%x", info.data2);
         LOG(I, "data 3:  0x%x", info.data3);
         LOG(I, "data 4:  0x%x", info.data4);
         LOG(I, "data 5:  0x%x", info.data5);
     }

 }

/******************************************************************************
 *                 Section For All Transmit Function Handlers
 * ****************************************************************************/

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_Wtr_Htr_C_TxHandle
//------------------------------------------------------------------------------
// Description: Frame to transmit for Truma Water heater Ctrl
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TR_Wtr_Htr_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_waterheater_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        ctrl.temp          = pstore_u16GetValue( VAR_LIN_TR_WTR_HTR_TARGET_TEMP_C );
        ctrl.energy_sel    = pstore_u8GetValue( VAR_LIN_TR_WTR_HTR_ENERGY_SELECTION_C );
        ctrl.power_lim     = pstore_u16GetValue( VAR_LIN_TR_WTR_HTR_POWER_LIMIT_C );
    }

    // Stuff message data
    truma_water_heater_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_Air_Htr_C_TxHandle
//------------------------------------------------------------------------------
// Description: Frame to transmit for Truma Air heater Ctrl
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TR_Air_Htr_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_airheater_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        ctrl.temp          = pstore_u16GetValue( VAR_LIN_TR_AIR_HTR_TARGET_TEMP_C );
        ctrl.energy_sel    = pstore_u8GetValue( VAR_LIN_TR_AIR_HTR_ENERGY_SELECTION_C );
    }

    // Stuff message data
    truma_air_heater_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TO_Charger0_C_TxHandle
//------------------------------------------------------------------------------
// Description: Frame to transmit for Toptron Charger0 Ctrl
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TO_Charger0_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    toptron_el603_charger_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        ctrl.charging_voltage = pstore_u8GetValue( VAR_LIN_TO_CHARGER0_CH_VOLTAGE_C );
        ctrl.battery_voltage  = pstore_u16GetValue( VAR_LIN_TO_CHARGER0_BAT_VOLTAGE_C );
        ctrl.silent_mode      = pstore_u8GetValue( VAR_LIN_TO_CHARGER0_SILENT_MODE_C );
    }

    // Stuff message data
    toptron_charger_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_DO_AC_C_TxHandle
//------------------------------------------------------------------------------
// Description: Frame to transmit for Dometic AC Ctrl
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_DO_AC_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    dometic_ac_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        ctrl.mode_a       = pstore_u8GetValue( VAR_LIN0_DO_AC_MODE_A_C );
        ctrl.fan_mode     = pstore_u8GetValue( VAR_LIN0_DO_AC_FAN_MODE_C );
        ctrl.light_status = pstore_u8GetValue( VAR_LIN0_DO_AC_LIGHT_STATUS_C );
        ctrl.power        = pstore_u8GetValue( VAR_LIN0_DO_AC_POWER_C );
        ctrl.mode_b       = pstore_u8GetValue( VAR_LIN0_DO_AC_MODE_B_C );
        ctrl.fan_speed    = pstore_u8GetValue( VAR_LIN0_DO_AC_FAN_SPEED_C );
        ctrl.target_temp  = pstore_u8GetValue( VAR_LIN0_DO_AC_TARGET_TEMP_C );
        ctrl.dim_lvl      = pstore_u8GetValue( VAR_LIN0_DO_AC_LIGHT_DIM_LVL_C );
        ctrl.sync_frame   = pstore_u8GetValue( VAR_LIN0_DO_AC_SYNC_FRAME_C );
    }

    // Stuff message data
    dometic_ac_C_Stuff( frame, &ctrl );
}

#if 0
 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SHARC_WH_C_TxHandle
 //------------------------------------------------------------------------------
 // Description: Frame to transmit for Dometic SHARC Water Heater Ctrl
 //------------------------------------------------------------------------------
 // Return:      None.
 //------------------------------------------------------------------------------
  void MsgLin_DO_SHARC_WH_C_TxHandle
 (
     uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
     uint8_t*    frame        // out: Tx data
 )
 {
     dometic_sharc_wtr_ctrl_t ctrl;

#if 0
     // Which bus?
     if (u8LinBus == 0)
     {
// Changed: Need to fix.
         ctrl.mode              = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_MODE_C );
         ctrl.sleep_on          = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_SLEEP_ON_C );
         ctrl.wakeup            = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_WAKEUP_C );
         ctrl.energy_source     = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_ENERGY_SOURCE_C );
         ctrl.lock              = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_LOCK_C );
         ctrl.sync              = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_SYNC_C );
         ctrl.c_mode            = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_C_MODE_C );
     }

     // Stuff message data
     dometic_sharc_wh_C_Stuff( frame, &ctrl );
#endif
 }

  //------------------------------------------------------------------------------
  // Function:    MsgLin_DO_SHARC_AH_C_TxHandle
  //------------------------------------------------------------------------------
  // Description: Frame to transmit for Dometic SHARC Air Heater Ctrl
  //------------------------------------------------------------------------------
  // Return:      None.
  //------------------------------------------------------------------------------
   void MsgLin_DO_SHARC_AH_C_TxHandle
  (
      uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
      uint8_t*    frame        // out: Tx data
  )
  {
      dometic_sharc_air_ctrl_t ctrl;

#if 0
      // Which bus?
      if (u8LinBus == 0)
      {
// Changed: Need to fix.
          ctrl.tar_room_temp              = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_TARGET_TEMP_C );
          ctrl.sleep_on                   = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_SLEEP_ON_C );
          ctrl.wakeup                     = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_WAKEUP_C );
          ctrl.energy_source              = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_ENERGY_SOURCE_C );
          ctrl.mode                       = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_MODE_C );
          ctrl.si_mod_max_fan_speed       = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_SILENT_MODE_MAX_FAN_SPEED_C );
          ctrl.vent_mod_min_fan_speed     = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_VENT_MODE_MIN_FAN_SPEED_C );
          ctrl.air_htr_timer_off_status   = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_TIMER_OFF_C );
          ctrl.air_htr_timer_on_status    = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_TIMER_ON_C );
          ctrl.wtr_htr_timer_on_status    = pstore_u8GetValue( VAR_LIN0_DO_SHARC_WH_TIMER_ON_C );
          ctrl.lock                       = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_LOCK_C );
          ctrl.sync                       = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_SYNC_C );
          ctrl.c_mode                     = pstore_u8GetValue( VAR_LIN0_DO_SHARC_AH_C_MODE_C );
      }

      // Stuff message data
      dometic_sharc_ah_C_Stuff( frame, &ctrl );
#endif
}
#endif

//------------------------------------------------------------------------------
// Function:    MsgLin_TO_Lightbox_C_TxHandle
//------------------------------------------------------------------------------
// Description: Frame to transmit for Toptron Lightbox
//------------------------------------------------------------------------------
// Return:      None.
//------------------------------------------------------------------------------
 void MsgLin_TO_Lightbox_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    toptron_lightbox_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        ctrl.light_bed_left   = pstore_u8GetValue( VAR_LIN_TO_LIGHTBOX_LIGHT_BED_L_C );
        ctrl.light_bed_right  = pstore_u8GetValue( VAR_LIN_TO_LIGHTBOX_LIGHT_BED_R_C );
        ctrl.light_ceiling    = pstore_u8GetValue( VAR_LIN_TO_LIGHTBOX_LIGHT_CEILING_C );
        ctrl.light_wall       = pstore_u8GetValue( VAR_LIN_TO_LIGHTBOX_LIGHT_WALL_C );
    }

    // Stuff message data
    toptron_lightbox_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatCapRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Cap Read Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatCapRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_bcap_re_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the values as per shared pdf "doc13-16-ibs-lin-diagnostic-comands-parametrization.pdf".*/

        ctrl.nad = 0x01;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.lid = 0x39;
        ctrl.data1 = 0xff;
        ctrl.data2 = 0x7f;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_batcapread_diag_C_Stuff( frame, &ctrl );
    // Store the SID value into global variable to check negative response
    diag_transmit_sid = ctrl.sid;
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatCapWrite_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Cap Write Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatCapWrite_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_bcap_wr_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf. */
        ctrl.nad = 0x01;
        ctrl.pci = 0x03;
        ctrl.sid = 0xB5;
        ctrl.lid = 0x39;
        ctrl.C_Nom = pstore_u8GetValue(VAR_LIN_HE_IBS0_CNOMINAL_C);
        ctrl.data2 = 0xff;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_batcapwrite_diag_C_Stuff( frame, &ctrl );
    // Store the SID value into global variable to check negative response
    diag_transmit_sid = ctrl.sid;
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTypeRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Type Read Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatTypeRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_btype_re_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf*/
        ctrl.nad = 0x01;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.lid = 0x3A;
        ctrl.data1 = 0xff;
        ctrl.data2 = 0x7f;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_battyperead_diag_C_Stuff( frame, &ctrl );
    // Store the SID value into global variable to check negative response
    diag_transmit_sid = ctrl.sid;
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTypeWrite_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Type Write Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatTypeWrite_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_btype_wr_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf*/

        ctrl.nad = 0x01;
        ctrl.pci = 0x03;
        ctrl.sid = 0xB5;
        ctrl.lid = 0x3a;
        ctrl.data1 = pstore_u8GetValue(VAR_LIN_HE_IBS0_BATTYPE_C);
        ctrl.data2 = 0xff;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_battypewrite_diag_C_Stuff( frame, &ctrl );
    // Store the SID value into global variable to check negative response
    diag_transmit_sid = ctrl.sid;
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTableState_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Table State Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatTableState_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_btable_st_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf */

        ctrl.nad = 0x01;
        ctrl.pci = 0x01;
        ctrl.sid = 0x30;
        ctrl.lid = 0xff;
        ctrl.data1 = 0xff;
        ctrl.data2 = 0xff;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_battablestate_diag_C_Stuff( frame, &ctrl );
    // Store the SID value into global variable to check negative response
    diag_transmit_sid = ctrl.sid;
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_BatTableOnOff_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Table On Off Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_BatTableOnOff_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_btable_onoff_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the values as per shared pdf.*/

        ctrl.nad = 0x82;
        ctrl.pci = 0x06;
        ctrl.sid = 0x31;
        ctrl.lid = 0x00;
        ctrl.data1 = 0x11;
        ctrl.data2 = 0x22;
        ctrl.data3 = 0x33;
        ctrl.data4 = 0x44;
    }

    // Stuff message data
    hella_btableonoff_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_U0MinMaxRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery U0 Minimum and Maximum Read
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_U0MinMaxRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_bu0minmax_re_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the values as per shared pdf "doc13-16-ibs-lin-diagnostic-comands-parametrization.pdf". */
        ctrl.nad = 0x82;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.lid = 0x30;
        ctrl.data1 = 0xff;
        ctrl.data2 = 0x7f;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_bu0minmaxread_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_U0MinMaxWrite_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery U0 Minimum and Maximum Write
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_U0MinMaxWrite_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_bu0minmax_wr_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the values as per shared pdf "doc13-16-ibs-lin-diagnostic-comands-parametrization.pdf".*/

        ctrl.nad = 0x82;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB5;
        ctrl.lid = 0x30;
        ctrl.u0_min = 0xffff;
        ctrl.u0_max = 0xffff;
    }

    // Stuff message data
    hella_bu0minmaxwrite_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_IBattQuiescentRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Quiescent Read Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_IBattQuiescentRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_ibquiescent_re_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf */

        ctrl.nad = 0x82;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.lid = 0x3C;
        ctrl.data1 = 0x7f;
        ctrl.data2 = 0xff;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_ibattquiescentread_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_HE_IBattQuiescentWrite_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Hella Battery Quiescent Write Master Request
//              Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_HE_IBattQuiescentWrite_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    hella_ibquiescent_wr_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf.*/

        ctrl.nad = 0x82;
        ctrl.pci = 0x04;
        ctrl.sid = 0xB5;
        ctrl.lid = 0x3C;
        ctrl.ibattquiescent = 0xff;
        ctrl.ichargemin = 0x7f;
        ctrl.data3 = 0xff;
        ctrl.data4 = 0xff;
    }

    // Stuff message data
    hella_ibattquiescentwrite_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_VH_SerialRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Serial Read Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_VH_SerialRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_serial_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.data1 = 0x3a;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;

    }

    // Stuff message data
    truma_generic_serial_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_VH_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Assign NAD Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_VH_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf "doc13-29-truma-varioheat_frameset.pdf".*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_VH_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Assign Frame Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_VH_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf "doc13-29-truma-varioheat_frameset.pdf".*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    truma_generic_assignframe_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_SerialRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Saphir Comfort AC Serial Read Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_SerialRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_serial_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.data1 = 0x3a;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;

    }

    // Stuff message data
    truma_generic_serial_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_CError_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Saphir AC Current Error Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_CError_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_cerror_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x23;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;

    }

    // Stuff message data
    truma_generic_currenterror_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Saphir Truma AC Assign NAD Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf "doc13-29-truma-varioheat_frameset.pdf".*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AC_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Saphir AC Assign Frame Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AC_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    truma_generic_assignframe_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_SerialRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Comfort AC Serial Read Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_SerialRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_serial_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.data1 = 0x3a;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;

    }

    // Stuff message data
    truma_generic_serial_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_CError_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Comfort AC Current Error Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_CError_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_cerror_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x23;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;

    }

    // Stuff message data
    truma_generic_currenterror_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Aventa Comfort Truma AC Assign NAD Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AC_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Comfort AC Assign Frame
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AC_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    truma_generic_assignframe_diag_C_Stuff( frame, &ctrl );
}



//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_SerialRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco AC Serial Read Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_SerialRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_serial_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.data1 = 0x3a;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;

    }

    // Stuff message data
    truma_generic_serial_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_CError_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco AC Current Error Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_CError_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_cerror_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x23;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;

    }

    // Stuff message data
    truma_generic_currenterror_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Aventa Eco Truma AC Assign NAD Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AC_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco AC Assign Frame
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AC_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    truma_generic_assignframe_diag_C_Stuff( frame, &ctrl );
}



//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM32_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco IFS User Command 32 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM32_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x20;// Send Master Request User Command 32
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM33_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco IFS User Command 33 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM33_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x21;// Send Master Request User Command 33
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM34_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco IFS User Command 34 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM34_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x22;// Send Master Request User Command 33
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM35_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco IFS User Command 35 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_IFS_UCOM35_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x23;// Send Master Request User Command 35
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TE_ASAT_UCOM36_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Activ Sat User Command 36 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM36_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x24;// Send Master Request User Command 36
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_ASAT_UCOM37_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Activ Satellite User Command 37 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM37_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x25;// Send Master Request User Command 37
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_ASAT_UCOM38_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Activ Satellite User Command 38 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM38_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x26;// Send Master Request User Command 33
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_IFS_UCOM39_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Activ Satellite User Command 39 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_ASAT_UCOM39_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x27;// Send Master Request User Command 39
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_UCOM36_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Flat Sat User Command 36 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM36_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x24;// Send Master Request User Command 36
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_UCOM37_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Flat Satellite BT User Command 37 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM37_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x25;// Send Master Request User Command 37
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_UCOM38_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Flat Satellite BT User Command 38 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM38_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x26;// Send Master Request User Command 33
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_FSATBT_UCOM39_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Flat Satellite BT User Command 39 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_FSATBT_UCOM39_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x27;// Send Master Request User Command 39
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_UCOM36_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Tele Satelite User Command 36 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM36_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x19;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x24;// Send Master Request User Command 36
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_UCOM37_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Tele Satellite BT User Command 37 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM37_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x19;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x25;// Send Master Request User Command 37
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_UCOM38_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Tele Satellite BT User Command 38 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM38_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x19;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x26;// Send Master Request User Command 33
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TE_TSATBT_UCOM39_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Teleco Tele Satellite BT User Command 39 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TE_TSATBT_UCOM39_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    teleco_gen_usr_com_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x19;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x27;// Send Master Request User Command 39
    }

    // Stuff message data
    teleco_generic_ucom_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TA_AC_UCOM32_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Telair AC User Command 32 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TA_AC_UCOM32_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    telair_gen_ucom_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x20;// Send Master Request User Command 32
    }

    // Stuff message data
    telair_generic_ucom_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TA_AC_UCOM33_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Telair AC User Command 33 Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TA_AC_UCOM33_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    telair_gen_ucom_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.ucommand = 0x21;// Send Master Request User Command 33
    }

    // Stuff message data
    telair_generic_ucom_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_KA_Sat_SwVer_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Kathrein CI Satelite System Software Version Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_KA_Sat_SwVer_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    kathrein_sat_sw_ver_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the dummy values as per shared pdf.*/
        ctrl.nad = 0x19;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.custom = 0x35;// Send Master Request for TB List Date
    }

    // Stuff message data
    kathrein_satelite_sw_ver_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_AL_3020_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Alde 3020 Assign NAD Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_AL_3020_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    alde_3020_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values ".*/
        ctrl.nad = 0x10;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x41DE;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    alde_3020_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_AL_3020_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Alde 3020 Assign Frame Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_AL_3020_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    alde_3020_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values .*/
        ctrl.nad = 0x10;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    alde_3020_assignframe_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_SerialRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Serial Read Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_SerialRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_serial_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.data1 = 0x3a;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;

    }

    // Stuff message data
    truma_generic_serial_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_FwVer_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphire Comfort Firmware Version
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_FwVer_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_fwver_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x04;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x20;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;
    }

    // Stuff message data
    truma_generic_fwver_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_CurrentError_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Current Error Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_CurrentError_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_cerror_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x23;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;

    }

    // Stuff message data
    truma_generic_currenterror_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Assign NAD
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf "doc13-29-truma-varioheat_frameset.pdf".*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_SapCom_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Saphir Comfort Assign Frame
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_SapCom_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    truma_generic_assignframe_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_SerialRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Serial Read Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_SerialRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_serial_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.data1 = 0x3a;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;

    }

    // Stuff message data
    truma_generic_serial_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_FwVer_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Comfort Firmware Version
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_FwVer_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_fwver_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x04;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x20;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;
    }

    // Stuff message data
    truma_generic_fwver_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_CurrentError_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Current Error Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_CurrentError_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_cerror_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x23;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;

    }

    // Stuff message data
    truma_generic_currenterror_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Assign NAD
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf "doc13-29-truma-varioheat_frameset.pdf".*/
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Assign Frame
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveCom_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    truma_generic_assignframe_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_SerialRead_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Eco Serial Read Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_SerialRead_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_serial_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.data1 = 0x3a;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;

    }

    // Stuff message data
    truma_generic_serial_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_FwVer_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Aventa Eco  Firmware Version
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_FwVer_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_fwver_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x04;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x20;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;
    }

    // Stuff message data
    truma_generic_fwver_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_CurrentError_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Eco Current Error Master
//              Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_CurrentError_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_cerror_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x23;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;

    }

    // Stuff message data
    truma_generic_currenterror_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveCom_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Assign NAD
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_AveEco_AssignFrame_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Eco Assign Frame
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_AveEco_AssignFrame_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_frame_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values as per shared pdf */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.start_index = 0xff;
        ctrl.product_id = 0xffffffff;
    }

    // Stuff message data
    truma_generic_assignframe_diag_C_Stuff( frame, &ctrl );
}


//------------------------------------------------------------------------------
// Function:    MsgLin_TR_CPplus_FwVer_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi CP Plus Firmware Version
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_CPplus_FwVer_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_fwver_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x04;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB2;
        ctrl.id = 0x20;
        ctrl.suppliar_id = 0x4617;
        ctrl.function_id = 0xffff;
    }

    // Stuff message data
   truma_generic_fwver_diag_C_Stuff( frame, &ctrl );
}

//------------------------------------------------------------------------------
// Function:    MsgLin_TR_CPplus_AssignNad_Diag_C_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Truma Combi Aventa Comfort Assign NAD
//              Master Request Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_TR_CPplus_AssignNad_Diag_C_TxHandle
(
    uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
    uint8_t*    frame        // out: Tx data
)
{
    truma_gen_nad_diag_ctrl_t ctrl;

    // Which bus?
    if (u8LinBus == 0)
    {
        //TODO

        /* Assigning the Dummy values */
        ctrl.nad = 0x02;
        ctrl.pci = 0x06;
        ctrl.sid = 0xB0;
        ctrl.supplier_id = 0x4617;
        ctrl.func_id = 0xffff;
        ctrl.new_nad = 0x03;

    }

    // Stuff message data
    truma_generic_assignnad_diag_C_Stuff( frame, &ctrl );
}

 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SH_SerialRead_Diag_C_TxHandle
 //------------------------------------------------------------------------------
 // Description: Diagnostic Dometic Serial Read Master Request Transmit Handler
 //
 //
 // NOTE:        Added by Dometic Sweden.
 //------------------------------------------------------------------------------
 // Return:      None
 //------------------------------------------------------------------------------
  void MsgLin_DO_SH_SerialRead_Diag_C_TxHandle
 (
     uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
     uint8_t*    frame        // out: Tx data
 )
 {
     dometic_sharc_serial_diag_ctrl_t ctrl;

     // Which bus?
     if (u8LinBus == 0)
     {
         //TODO

         /* Assigning the dummy values as per shared pdf*/
         ctrl.nad = 0x02;
         ctrl.pci = 0x06;
         ctrl.sid = 0xB2;
         ctrl.data1 = 0x01;
         ctrl.supplier_id = 0x4617;
         ctrl.func_id = 0xffff;

     }

     // Stuff message data
     dometic_sharc_serial_diag_C_Stuff( frame, &ctrl );
 }

 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SH_AssignNad_Diag_C_TxHandle
 //------------------------------------------------------------------------------
 // Description: Diagnostic Dometic SHARC Assign NAD Master Request Transmit Handler
 //
 //
 // NOTE:        Added by Dometic Sweden.
 //------------------------------------------------------------------------------
 // Return:      None
 //------------------------------------------------------------------------------
  void MsgLin_DO_SH_AssignNad_Diag_C_TxHandle
 (
     uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
     uint8_t*    frame        // out: Tx data
 )
 {
     dometic_sharc_nad_diag_ctrl_t ctrl;

     // Which bus?
     if (u8LinBus == 0)
     {
         //TODO

         /* Assigning the Dummy values as per shared pdf "doc13-29-truma-varioheat_frameset.pdf".*/
         ctrl.nad = 0x02;
         ctrl.pci = 0x06;
         ctrl.sid = 0xB0;
         ctrl.supplier_id = 0x4617;
         ctrl.func_id = 0xffff;
         ctrl.new_nad = 0xff;

     }

     // Stuff message data
     dometic_sharc_assignnad_diag_C_Stuff( frame, &ctrl );
 }

 //------------------------------------------------------------------------------
 // Function:    MsgLin_DO_SH_AssignFrame_Diag_C_TxHandle
 //------------------------------------------------------------------------------
 // Description: Diagnostic Dometic Assign Frame Master Request Transmit Handler
 //
 //
 // NOTE:        Added by Dometic Sweden.
 //------------------------------------------------------------------------------
 // Return:      None
 //------------------------------------------------------------------------------
  void MsgLin_DO_SH_AssignFrame_Diag_C_TxHandle
 (
     uint8_t    u8LinBus,    // in:  LIN bus the message should be Txed on
     uint8_t*    frame        // out: Tx data
 )
 {
     dometic_sharc_frame_diag_ctrl_t ctrl;

     // Which bus?
     if (u8LinBus == 0)
     {
         //TODO

         /* Assigning the Dummy values as per shared pdf*/
         ctrl.nad = 0x02;
         ctrl.pci = 0x06;
         ctrl.sid = 0xB7;
         ctrl.start_index = 0xff;
         ctrl.product_id = 0xffffffff;
     }

     // Stuff message data
     dometic_sharc_assignframe_diag_C_Stuff( frame, &ctrl );
 }


//------------------------------------------------------------------------------
// Function:    MsgLin_Sleep_TxHandle
//------------------------------------------------------------------------------
// Description: Diagnostic Sleep Command Transmit Handler
//
//
// NOTE:        Added by Dometic Sweden.
//------------------------------------------------------------------------------
// Return:      None
//------------------------------------------------------------------------------
 void MsgLin_Sleep_TxHandle
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
