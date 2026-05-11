/**
 * @file lin_server_tempra_battery.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief TEMPRA Battery implementation
 * @date 2024-12-24
 */

#ifndef LIN_SERVER_TEMPRA_BATTERY__
#define LIN_SERVER_TEMPRA_BATTERY__

#include <stdint.h>

#include "lin_common.h"
#include "lin_server_definition.h"

/** LIN frame defines for TEMPRA project */
#define TEMPRA_INFO_FRAME_IBS_FRM2_ID (0x22)  //!< IBS_FRM2
#define TEMPRA_INFO_FRAME_IBS_FRM5_ID (0x25)  //!< IBS_FRM5
#define TEMPRA_INFO_FRAME_IBS_FRM6_ID (0x26)  //!< IBS_FRM6

#define TEMPRA_INFO_FRAME_IBS_FRM2_LEN (7)  //!< IBS_FRM2 len
#define TEMPRA_INFO_FRAME_IBS_FRM5_LEN (8)  //!< IBS_FRM5 len
#define TEMPRA_INFO_FRAME_IBS_FRM6_LEN (6)  //!< IBS_FRM6 len

/* Should be an ddm2 enum? */
enum tempra_ddm_model
{
    TLB150 = 0x01,
    TLB120,
    TLB100,
    TLB150F,
    TLB120F,
    TLB100F,
};

typedef struct lin_server_device_frame_tempra
{
    union
    {
        /* IBS_FRM2 */
        union
        {
            struct
            {
                uint8_t Current_LSB : 8;
                uint8_t Current : 8;
                uint8_t Current_MSB : 8;
                uint16_t Voltage;

                union
                {
                    struct
                    {
                        uint16_t Temperature : 9;
                        uint16_t Batteries_Errors : 7;
                    };
                    uint16_t temperature_batt_errors;
                } temperature_batt_errors;
            } LIN_PACKED;
            uint8_t info_frame_ibs_frm2[7];
        } info_frame_ibs_frm2;

        /* IBS_FRM6 */
        union
        {
            struct
            {
                uint16_t Available_Capacity;
                uint16_t RFU;
                uint8_t Nominal_Capacity_LSB;  // D4: Nominal capacity byte 4
                uint8_t Nominal_Capacity_MSB;  // D5: Nominal capacity byte 5 (0xFF for values <256)
            } LIN_PACKED;
            uint8_t info_frame_ibs_frm6[6];
        } info_frame_ibs_frm6;

        /* IBS_FRM5 */
        union
        {
            struct
            {
                uint8_t SoC;
                uint8_t SoH;
                uint32_t RFU;

                union
                {
                    struct
                    {
                        uint16_t Cell_Temp_Out_Of_Range_On_Charge : 1;
                        uint16_t Cell_Temp_Out_Of_Range_On_Discharge : 1;
                        uint16_t Cell_BMS_Temp_Out_Of_Range : 1;
                        uint16_t Battery_Pack_Voltage_Less_Than_10_2V : 1;
                        uint16_t HW_Error : 1;
                        uint16_t SOC_Too_Low_or_Not_Aligned : 1;
                        uint16_t Voltage_Pack_Hihger_Than_15_3V : 1;
                        uint16_t Current_Between_202_and_206A_or_Above_260A : 1;
                        uint16_t Overcurrent_Prealarm : 1;
                        uint16_t Short_Circuit_Event : 1;
                        uint16_t Cell_Voltage_below_2200mV : 1;
                        uint16_t Cell_Voltage_over_3850mV : 1;
                        uint16_t Heating_System_Active : 1;
                        uint16_t RFU : 3;
                    } LIN_PACKED;
                    uint16_t Events;
                } Events;
            } LIN_PACKED;
            uint8_t info_frame_ibs_frm5[8];
        } info_frame_ibs_frm5;
    };
} LIN_PACKED lin_server_device_frame_tempra_t;

extern const lin_server_slave_device_t lin_server_tempra_device;

#endif  // LIN_SERVER_TEMPRA_BATTERY__
