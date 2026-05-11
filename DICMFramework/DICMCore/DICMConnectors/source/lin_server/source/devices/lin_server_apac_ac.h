/**
 * @file lin_server_apac_ac.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief APAC AC implementation
 * @date 2024-06-25
 */

#ifndef LIN_SERVER_APAC_AC__
#define LIN_SERVER_APAC_AC__

#include <stdint.h>

#include "lin_server_definition.h"

/** LIN frame defines for APAC project */
#define DOMETIC_APAC_AC_CTRL_FRAME_ID                   (0x08) //!< LIN control frame
#define DOMETIC_APAC_AC_INFO_FRAME_ID                   (0x17) //!< LIN information frame

#define DOMETIC_APAC_AC_CTRL_FRAME_LEN                  (8) //!< LIN control frame len
#define DOMETIC_APAC_AC_INFO_FRAME_LEN                  (8) //!< LIN info frame len

/** LocalChange synchornization mechanism bits position definition */
#define DOMETIC_APAC_AC_LOCAL_CHANGE_BIT_POSITION       (0x00) //!< LocalChange bit position within the byte
#define DOMETIC_APAC_AC_SYNC_FRAME_BIT_POSITION         (0x01) //!< SyncFrame bit position within the byte

typedef struct lin_server_device_frame_apac_ac
{
    union
    {
        /* Control frame */
        union
        {
            struct
            {
                union
                {
                    struct
                    {
                        uint8_t Mode        : 3; // AC Function Mode
                        uint8_t FanSet      : 3; // Fan setting
                        uint8_t Light       : 1; // Lighting system status
                        uint8_t Power       : 1; // Air Conditioner status
                    };
                    uint8_t remote_data_1;
                } remote_data_1;

                union
                {
                    struct
                    {
                        uint8_t Temp         : 4; // Target temperature
                        uint8_t SleepMode    : 1; // Sleep Function
                        uint8_t TimerOffMode : 1; // Turning off timer flag
                        uint8_t TimerOnMode  : 1; // Turning on timer flag
                        uint8_t Pure         : 1; // Air purifier
                    };
                    uint8_t remote_data_2;
                } remote_data_2;

                union
                {
                    struct
                    {
                    };
                    uint8_t OnTimerMSBits   : 8; // Timer on minute left
                    uint8_t remote_data_3;
                } remote_data_3;

                union
                {
                    struct
                    {
                        uint8_t OffTimerMSBits : 8; // Timer off minute left
                    };
                    uint8_t remote_data_4;
                } remote_data_4;

                union
                {
                    struct
                    {
                        uint8_t TimerMinLSBits : 4; // Residual timer time within the 10min set
                        uint8_t Flap           : 4; // Oscillator status
                    };
                    uint8_t remote_data_5;
                } remote_data_5;

                union
                {
                    struct
                    {
                        uint8_t CF             : 1; // Selection of Celsius/Fahrenheit visualization on ADB
                        uint8_t Reserved       : 7; // Not used field
                    };
                    uint8_t remote_data_6;
                } remote_data_6;

                uint8_t reserverd              : 8; // byte_7

                union
                {
                    struct
                    {
                        uint8_t RemoteCtrlDIs  : 1; // Remote control disable
                        uint8_t SyncFrame      : 1; // Master LIN synchronized to AC
                        uint8_t TimerUpdate    : 1; // Update Timer
                        uint8_t Reserved       : 5; // Not used field
                    };
                    uint8_t ctrl_status;
                } ctrl_status;
            };
            uint8_t ctrl_frame[8];
        } ctrl_frame;

        /* Info frame*/
        union
        {
            struct
            {
                union
                {
                    struct
                    {
                        uint8_t Mode            : 3; // AC Function Mode
                        uint8_t FanSet          : 3; // Fan setting
                        uint8_t Light           : 1; // Lighting system status
                        uint8_t Power           : 1; // Air Conditioner status
                    };
                    uint8_t remote_data_1;
                } remote_data_1;

                union
                {
                    struct
                    {
                        uint8_t Temp            : 4; // Target temperature
                        uint8_t SleepMode       : 1; // Sleep Function
                        uint8_t TimerOffMode    : 1; // Turning off timer flag
                        uint8_t TimerOnMode     : 1; // Turning on timer flag
                        uint8_t Pure            : 1; // Air purifier
                    };
                    uint8_t remote_data_2;
                } remote_data_2;

                union
                {
                    struct
                    {
                        uint8_t OnTimerMSBits   : 8; // Turning on minutes left
                    };
                    uint8_t remote_data_3;
                } remote_data_3;

                union
                {
                    struct
                    {
                        uint8_t OffTimerMSBits : 8; // Timer off minute left
                    };
                    uint8_t remote_data_4;
                } remote_data_4;

                union
                {
                    struct
                    {
                        uint8_t TimerMinLSBits : 4; // Residual timer time within the 10min set
                        uint8_t Flap           : 4; // Oscillator status
                    };
                    uint8_t remote_data_5;
                } remote_data_5;

                union
                {
                    struct
                    {
                        uint8_t CF             : 1; // Selection of Celsius/Fahrenheit visualization on ADB
                        uint8_t Reserved       : 7; // Not used field
                    };
                    uint8_t remote_data_6;
                } remote_data_6;

                union
                {
                    struct
                    {
                        uint8_t CompRun        : 1; // Compressor Status
                        uint8_t Reserved       : 1; // Not used field
                        uint8_t IntTemp        : 6; // Internal temperature Level
                    };
                    uint8_t system_status;
                } system_status;

                union
                {
                    struct
                    {
                        uint8_t LocalChange    : 1; // Status Modified from external control
                        uint8_t NoMain         : 1; // Mains Failure
                        uint8_t Error          : 1; // Temp. Probe error
                        uint8_t TimerOnReq     : 1; // AC On due to a timer on timeout
                        uint8_t TimerOffReq    : 1; // AC Off due to a timer off timeout
                        uint8_t Reserved       : 1; // Not used field
                        uint8_t NotInit        : 1; // Card not initialized
                        uint8_t LinError       : 1; // Error on LIN bus
                    };
                    uint8_t info_status;
                } info_status;
            };
            uint8_t info_frame[8];
        } info_frame;
    };
} LIN_PACKED lin_server_device_frame_apac_ac_t;

extern const lin_server_slave_device_t lin_server_apac_ac_device;

#endif // LIN_SERVER_APAC_AC__
