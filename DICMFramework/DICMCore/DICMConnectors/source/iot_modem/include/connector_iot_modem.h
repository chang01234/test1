/*! \file connector_iot_modem.h
	\brief IOT Modem header

	IOT Modem connector.
 */

#ifndef CONNECTOR_IOT_MODEM_H_
#define CONNECTOR_IOT_MODEM_H_

#if defined(IOT_MODEM) || defined(IOT_MODEM_OLD)
#include "configuration.h"
#include "connector.h"
extern CONNECTOR connector_iot_modem;
#endif

#endif /* CONNECTOR_IOT_MODEM_H_ */
