/*
 * hella.h
 *
 *  Created on: 15 Oct 2019
 *      Author: Stefan.Henningsohn
 */

#ifndef MAIN_HELLA_H_
#define MAIN_HELLA_H_

#include <stdint.h>
#include "pstore.h"

#define HELLA_IBS_FRAME2_ID (0x22)
#define HELLA_IBS_FRAME5_ID (0x25)
#define HELLA_IBS_FRAME6_ID (0x26)

typedef struct
{
	uint32_t battery_current;
	uint16_t battery_voltage;
	uint16_t battery_temperature;
	uint8_t ibs_error;
} hella_ibs_frm2_t;

typedef struct
{
	uint8_t state_of_charge;
	uint8_t state_of_health;
	uint8_t est_voltage_drop;
	uint8_t opt_charging_volt;
} hella_ibs_frm5_t;

typedef struct
{
	uint16_t avail_capacity;
	uint16_t discharge_ah;
	uint8_t nominal_capacity;
	uint8_t recalibrated;
} hella_ibs_frm6_t;

//Diagnostic Hella Battery Cap Read Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1; 
    uint8_t data2; 
    uint8_t data3; 
    uint8_t data4; 
} hella_bcap_re_diag_ctrl_t;

//Diagnostic Hella Battery Cap Write Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t C_Nom; // C_Nom
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_bcap_wr_diag_ctrl_t;

//Diagnostic Hella Battery Type Read Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1; 
    uint8_t data2; 
    uint8_t data3; 
    uint8_t data4; 
} hella_btype_re_diag_ctrl_t;

//Diagnostic Hella Battery Type Write Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1; 
    uint8_t data2; 
    uint8_t data3; 
    uint8_t data4; 
} hella_btype_wr_diag_ctrl_t;

//Diagnostic Hella Battery Table State Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1; 
    uint8_t data2; 
    uint8_t data3; 
    uint8_t data4; 
} hella_btable_st_diag_ctrl_t;

// Diagnostic Hella Product Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t rsid; // service Identifier
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
    uint8_t var_id; // variant ID
} hella_pid_diag_info_t;

// Diagnostic Hella Battery Cap Read Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_bcap_re_diag_info_t;

// Diagnostic Hella Battery Cap Read Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t C_Nom;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_bcap_wr_diag_info_t;

// Diagnostic Hella Battery Type Read Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_btype_re_diag_info_t;

// Diagnostic Hella Battery Type Write Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_btype_wr_diag_info_t;

// Diagnostic Hella Battery Table State Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_btable_st_diag_info_t;

//Diagnostic Hella Battery Table On Off Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_btable_onoff_diag_ctrl_t;

//Diagnostic Hella Battery UO min /U0 Max Read Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_bu0minmax_re_diag_ctrl_t;

//Diagnostic Hella Battery U0 Minimum Maximum Write Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint16_t u0_min;
    uint16_t u0_max;
} hella_bu0minmax_wr_diag_ctrl_t;

//Diagnostic Hella Battery IBattquiescent read Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_ibquiescent_re_diag_ctrl_t;

//Diagnostic Hella Battery IBattquiescent Read Control Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t ibattquiescent;
    uint8_t ichargemin;
    uint8_t data3;
    uint8_t data4;
} hella_ibquiescent_wr_diag_ctrl_t;

// Diagnostic Hella Battery table On Off Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_btable_onoff_diag_info_t;

// Diagnostic Hella Battery U0 Minimum and Maximum Read Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint16_t u0min;
    uint16_t u0max;
    uint8_t data4;
} hella_bu0minmax_re_diag_info_t;

// Diagnostic Hella Battery U0 Minimum and Maximum Write Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint16_t u0min;
    uint16_t u0max;
} hella_bu0minmax_wr_diag_info_t;

// Diagnostic Hella Battery IBattQuieScent Read Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t ibattquiescent;
    uint8_t ichargemin;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
} hella_ibquiescent_re_diag_info_t;

// Diagnostic Hella Battery IBattQuiescent Write Details Information Structure
typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t lid;
    uint8_t ibattquiescent;
    uint8_t ichargemin;
    uint8_t data3;
    uint8_t data4;
} hella_ibquiescent_wr_diag_info_t;


extern void hella_ibs_frm2_I_Extract(hella_ibs_frm2_t* pInfo,uint8_t* pu8Data);
extern void hella_ibs_frm5_I_Extract(hella_ibs_frm5_t* pInfo,uint8_t* pu8Data);
extern void hella_ibs_frm6_I_Extract(hella_ibs_frm6_t* pInfo,uint8_t* pu8Data);

extern int32_t hella_ibs_temperature_convert(int32_t i32Value);
extern int32_t hella_ibs_current_convert(int32_t i32Value);
extern int32_t hella_ibs_soc_convert(int32_t i32Value);
extern int32_t hella_ibs_battype_convert(int32_t i32Value);

extern int32_t hella_ibs_battype_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t hella_ibs_cnominal_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t hella_ibs_cnominal_convert(int32_t i32Value);

#define DIA_HELLA_BATTERY_SIZE 8

extern void hella_batcapread_diag_C_Stuff( uint8_t* pu8Data, hella_bcap_re_diag_ctrl_t * );
extern void hella_batcapwrite_diag_C_Stuff( uint8_t* pu8Data, hella_bcap_wr_diag_ctrl_t * );
extern void hella_battyperead_diag_C_Stuff( uint8_t* pu8Data, hella_btype_re_diag_ctrl_t * );
extern void hella_battypewrite_diag_C_Stuff( uint8_t* pu8Data, hella_btype_wr_diag_ctrl_t * );
extern void hella_battablestate_diag_C_Stuff( uint8_t* pu8Data, hella_btable_st_diag_ctrl_t * );

extern void hella_btableonoff_diag_C_Stuff( uint8_t* pu8Data, hella_btable_onoff_diag_ctrl_t * );
extern void hella_bu0minmaxread_diag_C_Stuff( uint8_t* pu8Data, hella_bu0minmax_re_diag_ctrl_t * );
extern void hella_bu0minmaxwrite_diag_C_Stuff( uint8_t* pu8Data, hella_bu0minmax_wr_diag_ctrl_t * );
extern void hella_ibattquiescentread_C_Stuff( uint8_t* pu8Data, hella_ibquiescent_re_diag_ctrl_t * );
extern void hella_ibattquiescentwrite_diag_C_Stuff( uint8_t* pu8Data, hella_ibquiescent_wr_diag_ctrl_t * );

extern void hella_pid_diag_I_Extract(hella_pid_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_batcapread_diag_I_Extract(hella_bcap_re_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_batcapwrite_diag_I_Extract(hella_bcap_wr_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_battyperead_diag_I_Extract(hella_btype_re_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_battypewrite_diag_I_Extract(hella_btype_wr_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_battablestate_diag_I_Extract(hella_btable_st_diag_info_t* pInfo,uint8_t* pu8Data);

extern void hella_btableonoff_diag_I_Extract(hella_btable_onoff_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_bu0minmaxread_diag_I_Extract(hella_bu0minmax_re_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_bu0minmaxwrite_diag_I_Extract(hella_bu0minmax_wr_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_ibattquiescentread_diag_I_Extract(hella_ibquiescent_re_diag_info_t* pInfo,uint8_t* pu8Data);
extern void hella_ibattquiescentwrite_diag_I_Extract(hella_ibquiescent_wr_diag_info_t* pInfo,uint8_t* pu8Data);

#endif //MAIN_HELLA_H_
