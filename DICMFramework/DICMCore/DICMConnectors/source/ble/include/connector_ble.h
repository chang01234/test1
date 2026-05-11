/*! \file connector_ble.h
    \brief BLE GAP/GATT connector
*/

#ifndef CONNECTOR_BLE_H_
#define CONNECTOR_BLE_H_

#include "configuration.h"
#include "connector.h"
#include <stdint.h>

#define INVALID_NODE_INSTANCE (-1)

int connector_ble_connector_slot(const int connector_id);

extern CONNECTOR connector_ble;

#endif /* CONNECTOR_BLE_H_ */
