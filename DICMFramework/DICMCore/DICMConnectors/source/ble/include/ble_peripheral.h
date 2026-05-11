/*! \file ble_peripheral.h
    \brief Bluetooth Low Energy server
*/
#ifndef BLE_PERIPHERAL_H
#define BLE_PERIPHERAL_H

#include <stdint.h>

void ble_peripheral_dicm_update_advertising_name(const char *adv_name);
uint32_t ble_peripheral_get_bond_mode(void);
void ble_peripheral_set_manufacturer_name(const char *const name);

#endif  // BLE_PERIPHERAL_H
