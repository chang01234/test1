/**
 * @file lin_server_bridge_nrx.h
 * @author Felix Qin (felix.qin@dometic.com)
 * @brief BRIDGE NRX implementation
 * @date 2024-07-31
 */

#ifndef LIN_SERVER_BRIDGE_NRX__
#define LIN_SERVER_BRIDGE_NRX__

#include <stdint.h>

#include "lin_server_definition.h"

/** LIN frame defines for APAC project */
#define DOMETIC_BRIDGE_NRX_CTRL_FRAME_ID                   (0x0B) //!< LIN control frame
#define DOMETIC_BRIDGE_NRX_INFO_FRAME_ID                   (0x0C) //!< LIN information frame

#define DOMETIC_BRIDGE_NRX_CTRL_FRAME_LEN                  (8) //!< LIN control frame len
#define DOMETIC_BRIDGE_NRX_INFO_FRAME_LEN                  (8) //!< LIN info frame len

/** LocalChange synchornization mechanism bits position definition */
// #define DOMETIC_BRIDGE_NRX_LOCAL_CHANGE_BIT_POSITION       (0x00) //!< LocalChange bit position within the byte
// #define DOMETIC_BRIDGE_NRX_SYNC_FRAME_BIT_POSITION         (0x01) //!< SyncFrame bit position within the byte

typedef struct lin_server_device_frame_bridge_nrx
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
                        uint8_t Reserved 	: 3; // Reserved
                        uint8_t Mode        : 2; // User mode
                        uint8_t Reserved_1 	: 3; // Reserved
                    };
                    uint8_t remote_data_1;
                } remote_data_1;

                union
                {
                    struct
                    {
                        uint8_t Temperature     : 4; // Set temperature
                        uint8_t Reserved 		: 4; // Reserved
                    };
                    uint8_t remote_data_2;
                } remote_data_2;

                union
                {
                    struct
                    {
                        uint8_t Reserved 		: 7; // Reserved
                        uint8_t NewProtocol     : 1; // New Protocol
                    };
                    uint8_t remote_data_3;
                } remote_data_3;

                uint8_t reserverd_4              : 8; // byte_3
                uint8_t reserverd_5              : 8; // byte_4
                uint8_t reserverd_6              : 8; // byte_5
                uint8_t reserverd_7              : 8; // byte_6
                union
                {
                    struct
                    {
                        uint8_t Page            : 2; // Page
                        uint8_t Lock            : 1; // Set to 1 for only remotely control
                        uint8_t Sync            : 1; // Not used; Synchronize request from master
                        uint8_t C_mode          : 1; // 1:One short com. 0:Continuous com
                        uint8_t Reserved 		: 2; // Reserved
                        uint8_t Power           : 1; // Power on/off
                    };
                    uint8_t remote_data_8;
                } remote_data_8;
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
                        uint8_t Reserved 		 : 3; // Reserved
                        uint8_t Mode             : 2; // User mode
                        uint8_t CI_BUS           : 1; // on/off Default set as 1
                        uint8_t CompressorStatus : 1; // Lighting system status
                        uint8_t Reserved_1 		 : 1; // Reserved
                    };
                    uint8_t remote_data_1;
                } remote_data_1;

                union
                {
                    struct
                    {
                        uint8_t ActualFridge   : 4; // Actual fridge
                        uint8_t Reserved 	   : 4; // Reserved
                    };
                    uint8_t remote_data_2;
                } remote_data_2;

                uint8_t reserverd_3              : 8; // byte_2
                union
                {
                    struct
                    {
                        uint8_t C_type          : 1; // Cooling type
                        uint8_t Reserved 		: 7; // Reserved
                    };
                    uint8_t remote_data_4;
                } remote_data_4;

                union
                {
                    struct
                    {
                        uint8_t FreshTemp   : 8; // Fresh temp
                    };
                    uint8_t remote_data_5;
                } remote_data_5;

                union
                {
                    struct
                    {
                         uint8_t Reserved 		: 8; // Reserved
                    };
                    uint8_t remote_data_6;
                } remote_data_6;

                union
                {
                    struct
                    {
                        uint8_t Error           : 7; // Error code is different with Dometic Generic CI-BUS protocol,So Al-Type should be 1
                        uint8_t AIarmType       : 1; // AIarm_Type
                    };
                    uint8_t remote_data_7;
                } remote_data_7;

                union
                {
                    struct
                    {
                        uint8_t Page            : 2; // Page
                        uint8_t Lock            : 1; // Set to 1 for only remotely control
                        uint8_t Reserved 		: 1; // Reserved
                        uint8_t LChange         : 1; // Not used; local change
                        uint8_t Reserved_1 		: 2; // Reserved
                        uint8_t Power           : 1; // Power on/off
                    };
                    uint8_t remote_data_8;
                } remote_data_8;
            };
            uint8_t info_frame[8];
        } info_frame;
    };
} LIN_PACKED lin_server_device_frame_bridge_nrx_t;

extern const lin_server_slave_device_t lin_server_bridge_nrx_device;

#endif // LIN_SERVER_BRIDGE_NRX__
