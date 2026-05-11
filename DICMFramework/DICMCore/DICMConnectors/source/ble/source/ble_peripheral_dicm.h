/*! \file ble_peripheral_dicm.h
 *  \brief Header file for the DICM peripheral service.
 *  \author Jens Björnhager
 *
 *  This file contains the definitions and function prototypes for the DICM peripheral service.
 */

#ifndef BLE_PERIPHERAL_DICM_H_
#define BLE_PERIPHERAL_DICM_H_

#include "bluetooth_le.h"
#include "configuration.h"
#include "host/ble_gatt.h"

uint8_t *ble_peripheral_dicm_get_advertisement_data(size_t *data_size);
uint8_t *ble_peripheral_dicm_get_scan_response_data(size_t *data_size);
struct ble_gatt_svc_def *ble_peripheral_dicm_get_service(void);

#endif  // BLE_PERIPHERAL_DICM_H_
