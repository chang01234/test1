/**
 * @file lin_server_freshjet.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief Freshjet 2200 & 3000 implementation
 * @date 2023-12-28
 */

#ifndef LIN_SERVER_FRESHJET__
#define LIN_SERVER_FRESHJET__

#include <stdint.h>

#include "lin_server_definition.h"

/** LIN frame defines for Freshjet project */
#define DOMETIC_FRESHJET_CTRL_FRAME_ID					(0x08)	//!< LIN control frame
#define DOMETIC_FRESHJET_INFO_FRAME_ID					(0x17)	//!< LIN information frame

#define DOMETIC_FRESHJET_CTRL_FRAME_LEN					(8) //!< LIN control frame len
#define DOMETIC_FRESHJET_INFO_FRAME_LEN					(8) //!< LIN info frame len

/** LocalChange synchornization mechanism bits position definition */
#define DOMETIC_FRESHJET_LOCAL_CHANGE_BIT_POSITION		(0x00)	//!< LocalChange bit position within the byte
#define DOMETIC_FRESHJET_SYNC_FRAME_BIT_POSITION		(0x02)	//!< SyncFrame bit position within the byte

typedef struct lin_server_frame_def_freshjet_2200_3000
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
						uint8_t itm    : 1; // iFeel Function
						uint8_t md_A   : 1; // Function mode A
						uint8_t fmd    : 1; // Ventilation Mode
						uint8_t sleep  : 1; // Sleep Function
						uint8_t toffa  : 1; // Turining off timer flag
						uint8_t tona   : 1; // Turining on timer flag
						uint8_t lgt    : 1; // Lighting system status
						uint8_t on     : 1; // Air Conditioner status
					};
					uint8_t remote_data_1;
				} remote_data_1;

				union
				{
					struct
					{
						uint8_t md_B    : 2; // Function mode B
						uint8_t fspd    : 2; // Fan speed
						uint8_t ttemp   : 4; // Target Temperature
					};
					uint8_t remote_data_2;
				} remote_data_2;

				union
				{
					struct
					{
					};
						 uint8_t OnTimerMSBits 	: 8; // Timer on minute left
					uint8_t remote_data_3;
				} remote_data_3;

				union
				{
					struct
					{
						uint8_t OffTimerMSBits 	: 8; // Timer off minute left
					};
					uint8_t remote_data_4;
				} remote_data_4;

				union
				{
					struct
					{
						uint8_t TimerMinLSBits 	: 4; // Residual timer time within the 10min set
						uint8_t iFeelTemp 		: 4; // External temperature for iFeel Function
					};
					uint8_t remote_data_5;
				} remote_data_5;

				union
				{
					struct
					{
						uint8_t EconomyOn 		: 1; // Economy Function Status(Input to set the economy mode (disable the compressor))
						uint8_t InverterOn 		: 1; // External inverter Function Status
						uint8_t UVLamp 			: 1; // UV Lamp Function Status
						uint8_t dmr			 	: 5; // Lighting system level
					};
					uint8_t remote_data_6;
				} remote_data_6;

				uint8_t reserverd : 8; // byte_7

				union
				{
					struct
					{
						uint8_t RemoteCtrlDIs 	: 1; // Remote control disable
						uint8_t RemoteOnDisable : 1; // Remote Input disable
						uint8_t SyncFrame 		: 1; // Master CI synchronized to AC
						uint8_t TimerUpdate 	: 1; // Update Timer
						uint8_t SetInverterOff 	: 1; // Disable the Output for external inverter
						uint8_t SetInverterOn 	: 1; // Enable the output for external inverter
						uint8_t Reserved 		: 2; // Reserved
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
						uint8_t itm    : 1; // iFeel Function
						uint8_t md_A   : 1; // Function mode A
						uint8_t fm     : 1; // Ventilation Mode
						uint8_t sleep  : 1; // Sleep Function
						uint8_t toffa  : 1; // Turining off timer flag
						uint8_t tona   : 1; // Turining on timer flag
						uint8_t lgt    : 1; // Lighting system status
						uint8_t on     : 1; // Air Conditioner status
					};
					uint8_t remote_data_1;
				} remote_data_1;

				union
				{
					struct
					{
						uint8_t md_B    : 2; // Function mode B
						uint8_t fspd    : 2; // Fan speed
						uint8_t ttemp   : 4; // Target Temperature
					};
					uint8_t remote_data_2;
				} remote_data_2;

				union
				{
					struct
					{
						uint8_t OnTimerMSBits 	: 8; // Turning on minutes left
					};
					uint8_t remote_data_3;
				} remote_data_3;

				union
				{
					struct
					{
						uint8_t OffTimerMSBits 	: 8; // Timer off minute left
					};
					uint8_t remote_data_4;
				} remote_data_4;

				union
				{
					struct
					{
						uint8_t TimerMinLSBits 	: 4; // Residual timer time within the 10min set
						uint8_t iFeelTemp 		: 4; // External temperature for iFeel Function
					};
					uint8_t remote_data_5;
				} remote_data_5;

				union
				{
					struct
					{
						uint8_t EconomyOn 		: 1; // Economy Function Status(Input to set the economy mode (disable the compressor))
						uint8_t InverterOn 		: 1; // External inverter Function Status
						uint8_t UVLamp 			: 1; // UV Lamp Function Status
						uint8_t dmr			 	: 5; // Lighting system level
					};
					uint8_t remote_data_6;
				} remote_data_6;

				union
				{
					struct
					{
						uint8_t CompRun 		: 1; // Compressor Status
						uint8_t UVLampOn 		: 1; // UV Lamp Status
						uint8_t IntTemp			: 5; // Internal temperature Level
					};
					uint8_t system_status;
				} system_status;

				union
				{
					struct
					{
						uint8_t LocalChange 	: 1; // Status Modified from external control
						uint8_t NoMain		 	: 1; // Mains Failure
						uint8_t Error 		 	: 1; // Temp. Probe error
						uint8_t TimerOnReq	 	: 1; // AC On due to a timer on timeout
						uint8_t TimerOffReq 	: 1; // AC Off due to a timer off timeout
						uint8_t RemoteOn 		: 1; // AC On due to a Remote input request
						uint8_t NotInit 		: 1; // Card not initialized
						uint8_t CIError 		: 1; // Error on CI bus
					};
					uint8_t info_status;
				} info_status;
			};
			uint8_t info_frame[8];
		} info_frame;
	};
} LIN_PACKED lin_server_device_frame_freshjet_2200_3000_t;

extern const lin_server_slave_device_t lin_server_freshjet_2200_3000_device;

#endif //LIN_SERVER_FRESHJET__
