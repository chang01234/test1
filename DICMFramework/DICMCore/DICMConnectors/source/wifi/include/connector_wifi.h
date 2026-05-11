/*! \file connector_wifi.h
	\brief BASE64 WiFi connector
*/

#ifndef CONNECTOR_WIFI_H_
#define CONNECTOR_WIFI_H_

#include "configuration.h"
#include "lwip/sockets.h"		//struct sockaddr_in

#include "connector.h"

#define RECEIVE_BUFFER_SIZE						4096			//!< Configure receive buffer size in bytes for each socket.

#define WIFI_EXTENDED_LOG						0				//!< Enable or disable Wi-Fi verbose messages in the log.

#ifndef CONNECTOR_WIFI_MAX_CONNECTIONS
#define CONNECTOR_WIFI_MAX_CONNECTIONS			4				//!< Configure maximum number of concurrent connections in TCP stack.
#endif

#define WIFI_APLIST_SIZE						1				//!< Configure number of AP entries in the list

extern CONNECTOR connector_wifi;

/**
 * @brief	Get the SSID and clear text PWD for the first AP entry in the list
 *
 * @param	ssid Pointer to array which will hold SSID. The length of that array needs to be at
 *			least @ref WIFI_NETWORK__SSID_LENGTH.
 * @param	pwd Pointer to array which will hold password. The length of that array needs to be at
 *			least @ref WIFI_NETWORK__PASSWORD_LENGTH.
 * @return	Operation status
 * @retval	1 - operation completed successfully.
 */
int connector_wifi_get_ap_info(char * const ssid, char * const pwd);

#endif /* CONNECTOR_WIFI_H_ */
