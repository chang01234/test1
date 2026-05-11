/*
 * truma.h
 *
 *  Created on: 8 Nov 2019
 *      Author: Stefan.Henningsohn
 */

#ifndef MAIN_TRUMA_H_
#define MAIN_TRUMA_H_

#include <stdint.h>
#include "payloads.h"
#include "pstore.h"

#define TRUMA_WTR_DEFAULT_CTRL_ID (0x37)
#define TRUMA_WTR_DEFAULT_INFO_ID (0x38)
#define TRUMA_AIR_DEFAULT_CTRL_ID (0x39)
#define TRUMA_AIR_DEFAULT_INFO_ID (0x3A)

#define PAYLOAD_WATER_HEATER_CTRL    PAYLOAD_TRUMA_START + 0

typedef struct
{
	uint16_t temp;
	uint8_t energy_sel;
    uint16_t power_lim;
} truma_waterheater_ctrl_t;

#define PAYLOAD_WATER_HEATER_INFO     PAYLOAD_TRUMA_START + 1

typedef struct
{
	uint16_t temp;
	uint8_t energy_sel;
    uint16_t power_lim;
    uint8_t frost_ctrl  : 1;
    uint8_t el_aval     : 1;
    uint8_t sw_clr      : 1;
    uint8_t manual_mode : 1;
} truma_waterheater_info_t;

#define PAYLOAD_AIR_HEATER_CTRL    PAYLOAD_TRUMA_START + 2

typedef struct
{
	uint16_t temp;
	uint8_t energy_sel;
} truma_airheater_ctrl_t;

#define PAYLOAD_AIR_HEATER_INFO     PAYLOAD_TRUMA_START + 3

typedef struct
{
	uint16_t temp;
	uint8_t energy_sel;
} truma_airheater_info_t;

typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t id; // Identifier
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID 

}truma_airheater_diag_ctrl_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t var_id;
}truma_gen_pid_diag_info_t; /* Diagnostic Truma Generic Product Identifier Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t data1;
    uint16_t supplier_id;
    uint16_t func_id;
}truma_gen_serial_diag_ctrl_t; /* Diagnostic Truma Generic Serial Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint32_t serial_number;
    uint8_t data8;
}truma_gen_serial_diag_info_t; /* Diagnostic Truma Generic Serial Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t new_nad;
}truma_gen_nad_diag_ctrl_t; /* Diagnostic Truma Generic Assign NAD Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}truma_gen_nad_diag_info_t; /* Diagnostic Truma Generic Assign NAD Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t start_index;
    uint32_t product_id;
}truma_gen_frame_diag_ctrl_t; /* Diagnostic Truma Generic Assign Frame Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}truma_gen_frame_diag_info_t; /* Diagnostic Truma Generic Assign Frame Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t id;
    uint16_t suppliar_id;
    uint16_t function_id;
}truma_gen_cerror_diag_ctrl_t; /* Diagnostic Generic Current Error Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t format;
    uint8_t clas;
    uint8_t data3;
    uint8_t code;
    uint8_t data5;
}truma_gen_cerror_diag_info_t; /* Diagnostic Truma Generic Current Error Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t id;
    uint16_t suppliar_id;
    uint16_t function_id;
}truma_gen_fwver_diag_ctrl_t; /* Diagnostic Generic Firmware Version Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
    uint16_t build;
}truma_gen_fwver_diag_info_t; /* Diagnostic Truma Generic Firmware Version Information Structure */


typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t req_sid;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}truma_gen_neg_diag_info_t; /* Diagnostic Truma Generic Negative Information Structure */


#define TRUMA_WATER_HEATER_SIZE 8
extern void truma_water_heater_C_Extract(truma_waterheater_ctrl_t* pCtrl, uint8_t* pu8Data);
extern void truma_water_heater_I_Extract(truma_waterheater_info_t* pInfo, uint8_t* pu8Data);
extern void truma_water_heater_C_Stuff(uint8_t* pu8Data, truma_waterheater_ctrl_t* pCtrl);

#define TRUMA_AIR_HEATER_SIZE 8
extern void truma_air_heater_C_Extract(truma_airheater_ctrl_t* pCtrl, uint8_t* pu8Data);
extern void truma_air_heater_I_Extract(truma_airheater_info_t* pInfo, uint8_t* pu8Data);
extern void truma_air_heater_C_Stuff(uint8_t* pu8Data, truma_airheater_ctrl_t* pCtrl);

extern int32_t truma_heater_energy_selection_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t truma_heater_energy_selection_convert(int32_t i32Value);
extern int32_t truma_heater_power_limit_convert(int32_t i32Value);


extern int32_t truma_wtr_heater_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t truma_water_heater_convert(int32_t i32Value);
extern int32_t truma_water_heater_status_set(pstore_table_t eIndex, int32_t i32Value);

extern int32_t truma_air_heater_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t truma_air_heater_convert(int32_t i32Value);
extern int32_t truma_air_heater_status_set(pstore_table_t eIndex, int32_t i32Value);

#define TRUMA_REQ_FRAME_SIZE_DIAG 8
extern void truma_air_heater_diag_C_Stuff(uint8_t* pu8Data, truma_airheater_diag_ctrl_t* pCtrl);
extern void truma_generic_pid_diag_I_Extract(truma_gen_pid_diag_info_t* pInfo, uint8_t* pu8Data);

extern void truma_generic_serial_diag_C_Stuff(uint8_t* pu8Data, truma_gen_serial_diag_ctrl_t* pCtrl);
extern void truma_generic_serial_diag_I_Extract(truma_gen_serial_diag_info_t* pInfo, uint8_t* pu8Data);

extern void truma_generic_assignnad_diag_C_Stuff(uint8_t* pu8Data, truma_gen_nad_diag_ctrl_t* pCtrl);
extern void truma_generic_assignnad_diag_I_Extract(truma_gen_nad_diag_info_t* pInfo, uint8_t* pu8Data);

extern void truma_generic_assignframe_diag_C_Stuff(uint8_t* pu8Data, truma_gen_frame_diag_ctrl_t* pCtrl);
extern void truma_generic_assignframe_diag_I_Extract(truma_gen_frame_diag_info_t* pInfo, uint8_t* pu8Data);

extern void truma_generic_currenterror_diag_C_Stuff(uint8_t* pu8Data, truma_gen_cerror_diag_ctrl_t* pCtrl);
extern void truma_generic_currenterror_diag_I_Extract(truma_gen_cerror_diag_info_t* pInfo, uint8_t* pu8Data);

extern void truma_generic_fwver_diag_C_Stuff(uint8_t* pu8Data, truma_gen_fwver_diag_ctrl_t* pCtrl);
extern void truma_generic_fwver_diag_I_Extract(truma_gen_fwver_diag_info_t* pInfo, uint8_t* pu8Data);

extern void truma_generic_negative_diag_I_Extract(truma_gen_neg_diag_info_t* pInfo, uint8_t* pu8Data);

#endif //MAIN_TRUMA_H_
