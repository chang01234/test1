/**
 * @file lin_server_device_definition.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server Device definitions for all of the supported devices.
 * @date 2023-12-28
 */

#ifndef LIN_SERVER_DEVICE_DEFINITION__
#define LIN_SERVER_DEVICE_DEFINITION__

#include "lin_server_freshjet_2200_3000.h"
#include "lin_server_apac_ac.h"
#include "lin_server_bridge_nrx.h"
#include "lin_server_tempra_battery.h"

/**
 * @brief    Device types supported by LIN Server module.
 */
typedef enum lin_server_device_type
{
    LIN_SERVER_DEVICE_TYPE_FRESHJET_2200_3000,
	LIN_SERVER_DEVICE_TYPE_APAC_AC,
	LIN_SERVER_DEVICE_TYPE_BRIDGE_NRX,
	LIN_SERVER_DEVICE_TYPE_TEMPRA,
} lin_server_device_type_t;

/**
 * @brief   Frame definition supported by LIN Server module.
 *
 *          Each device should have it's own frame descriptor
 *          by defining the corresponding frame bytes and bits
 *          depending on the project definition
 */
typedef struct lin_server_device_frame_signals
{
	union
	{
		uint8_t raw_frame[LIN_FRAME_DATA_LEN];

		lin_server_device_frame_freshjet_2200_3000_t freshjet_frame;
		lin_server_device_frame_apac_ac_t apac_ac_frame;
		lin_server_device_frame_bridge_nrx_t bridge_nrx_frame;
		lin_server_device_frame_tempra_t tempra_frame;
	};
} lin_server_device_frame_signals_t;

typedef struct lin_server_device_frame
{
	/* It is important lin_server_device_frame_signals_t to
	 * be the first structure in this frame definition, so we
	 * can extract the byte position of LocalChange bit at compile
	 * time using the offsetof macro */
	lin_server_device_frame_signals_t frame_signals;
	lin_server_device_frame_data_t frame_data;
} lin_server_device_frame_t;

#endif //LIN_SERVER_DEVICE_DEFINITION__