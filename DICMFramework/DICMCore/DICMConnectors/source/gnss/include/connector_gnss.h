/*! \file connector_gnss.h
	\brief GNSS connector
*/

#ifndef CONNECTOR_GNSS_H_
#define CONNECTOR_GNSS_H_

#include "configuration.h"

#ifdef CONNECTOR_GNSS

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "connector.h"

extern CONNECTOR connector_gnss;

#endif /* CONNECTOR_GNSS */
#endif /* CONNECTOR_GNSS_H */