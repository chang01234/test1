/*! \file bluetooth_le.h

    \brief Main BLE header.

    Main Bluetooth low energy/GAP module.
*/

#include <stdint.h>

#ifndef BLUETOOTH_LE_H_
#define BLUETOOTH_LE_H_

#include "host/ble_uuid.h"

#include "configuration.h"

#define NOTIFY_BIT 0x01  //!< \~ Notify bit position in GATT configuration register

//! \~ Cool Box service characteristic UUIDs
typedef enum _SERVICE_UUID_ENUM
{
    UUID_SERVICE = 0x0400,  //!< \~ ICM service UUID
    UUID_INPUT,
    UUID_OUTPUT,
} SERVICE_UUID_ENUM;

#define GAP_FIELD_FLAGS             0x01
#define GAP_FIELD_LOCAL_NAME        0x09
#define GAP_FIELD_MANUFACTURER_DATA 0xff

#define BLE_DOMETIC_ID 0x0845

//! \~ GAP advertisement FLAGS field structure (3B)
typedef struct GAP_FLAGS
{
    uint8_t length;  // 1B
    uint8_t type;    // 1B
    uint8_t flags;   // 1B
} PACKED GAP_FLAGS;

//! \~ GAP advertisement LOCAL NAME field structure (<= 26B)
typedef struct GAP_LOCAL_NAME
{
    uint8_t length;          // 1B
    uint8_t type;            // 1B
    uint8_t local_name[24];  //<= 24B
} PACKED GAP_LOCAL_NAME;

//! \~ DICM GAP advertisement MANUFACTURER DATA field structure (<= 28B)
typedef struct DICM_ADVERTISEMENT_MANUFACTURER_DATA
{
    uint8_t length;               // 1B
    uint8_t type;                 // 1B
    uint16_t company_id;          // 2B
    uint8_t product_type;         // 1B
    uint8_t mac[6];               // 6B
    uint8_t sku[MAX_SKU_LENGTH];  //<= 17B
} PACKED DICM_ADVERTISEMENT_MANUFACTURER_DATA;

//! \~ DICM GAP advertisement MANUFACTURER DATA field structure (5B)
typedef struct DICM_SCAN_RESPONSE_MANUFACTURER_DATA
{
    uint8_t length;       // 1B
    uint8_t type;         // 1B
    uint16_t company_id;  // 2B
    uint8_t status_flag;  // 1B
} PACKED DICM_SCAN_RESPONSE_MANUFACTURER_DATA;

//! \~ DICM GAP advertisement data structure (<= 31B)
typedef struct DICM_ADVERTISEMENT_DATA
{
    GAP_FLAGS flags;                                         // 3B
    DICM_ADVERTISEMENT_MANUFACTURER_DATA manufacturer_data;  //<= 28B
} PACKED DICM_ADVERTISEMENT_DATA;

//! \~ DICM GAP scan response data structure (<= 31B)
typedef struct DICM_SCAN_RESPONSE_DATA
{
    DICM_SCAN_RESPONSE_MANUFACTURER_DATA manufacturer_data;  // 5B
    GAP_LOCAL_NAME local_name;                               //<= 26B
} PACKED DICM_SCAN_RESPONSE_DATA;

extern const ble_uuid128_t Dicm_service_uuid;
extern const ble_uuid128_t Dicm_input_uuid;
extern const ble_uuid128_t Dicm_output_uuid;

void bond_timer_create(void);
void bond_timer_start(void);
void bond_timer_stop(const BT0PAIR_OUT_ENUM reason);
void clear_bonds(void);
char *addr_str(const void *const addr);
void set_advertisement_interval(int32_t interval);
#ifdef CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE
bool do_bonding_records_exist(void);
#endif  // CONNECTOR_BLE_ENABLE_PAIRING_WHEN_NO_BOND_RECORD_AVAILABLE
int gatt_svr_chr_write(struct os_mbuf *mbuf, uint16_t min_len, uint16_t max_len, void *dst, uint16_t *len);

extern uint32_t bond_mode;

#endif /* BLUETOOTH_LE_H_ */
