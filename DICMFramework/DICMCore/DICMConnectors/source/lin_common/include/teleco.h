#ifndef MAIN_TELECO_H_
#define MAIN_TELECO_H_

#include <stdint.h>
#include "payloads.h"
#include "pstore.h"


#endif //MAIN_TELECO_H_

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t var_id;
}teleco_gen_pid_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t ucommand;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}teleco_gen_usr_com_diag_ctrl_t;//Diagnostic Teleco Generic User Command CI BUS Interface

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t custom;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}teleco_gen_ucom32_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t hw_type;
    uint8_t hw_ver;
    uint8_t fw_type;
    uint8_t fw_ver;
    uint8_t fw_sub_ver;
}teleco_gen_ucom33_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t custom_digit1;
    uint8_t custom_digit0;
    uint8_t hw_ver;
    uint8_t fw_type;
    uint8_t data5;
}teleco_gen_ucom34_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t fw_ver_major;
    uint8_t fw_ver_minor;
    uint8_t db_digit2;
    uint8_t db_digit1;
    uint8_t db_digit0;
}teleco_gen_ucom35_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t hw_type;
    uint8_t hw_ver;
    uint8_t fw_type;
    uint8_t fw_ver;
    uint8_t fw_sub_ver;
}teleco_gen_ucom36_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t custom_digit1;
    uint8_t custom_digit0;
    uint8_t crit_type1;
    uint8_t crit_type0;
    uint8_t crit_ver5;
}teleco_gen_ucom37_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t crit_ver4;
    uint8_t crit_ver3;
    uint8_t crit_ver2;
    uint8_t crit_ver1;
    uint8_t crit_ver0;
}teleco_gen_ucom38_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t sat_list1;
    uint8_t sat_list0;
    uint8_t auto_onoff;
    uint8_t smart_conf1;
    uint8_t smart_conf0;
}teleco_gen_ucom39_diag_info_t;


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
}teleco_gen_neg_diag_info_t;

#define TELECO_GEN_REQ_FRAME_SIZE_DIAG    8

extern void teleco_generic_pid_diag_I_Extract(teleco_gen_pid_diag_info_t* pInfo, uint8_t* pu8Data);

extern void teleco_generic_ucom_diag_C_Stuff(uint8_t* pu8Data, teleco_gen_usr_com_diag_ctrl_t* pCtrl);

extern void teleco_generic_ucom32_diag_I_Extract(teleco_gen_ucom32_diag_info_t* pInfo, uint8_t* pu8Data);
extern void teleco_generic_ucom33_diag_I_Extract(teleco_gen_ucom33_diag_info_t* pInfo, uint8_t* pu8Data);
extern void teleco_generic_ucom34_diag_I_Extract(teleco_gen_ucom34_diag_info_t* pInfo, uint8_t* pu8Data);
extern void teleco_generic_ucom35_diag_I_Extract(teleco_gen_ucom35_diag_info_t* pInfo, uint8_t* pu8Data);
extern void teleco_generic_ucom36_diag_I_Extract(teleco_gen_ucom36_diag_info_t* pInfo, uint8_t* pu8Data);
extern void teleco_generic_ucom37_diag_I_Extract(teleco_gen_ucom37_diag_info_t* pInfo, uint8_t* pu8Data);
extern void teleco_generic_ucom38_diag_I_Extract(teleco_gen_ucom38_diag_info_t* pInfo, uint8_t* pu8Data);
extern void teleco_generic_ucom39_diag_I_Extract(teleco_gen_ucom39_diag_info_t* pInfo, uint8_t* pu8Data);

extern void teleco_generic_negative_diag_I_Extract(teleco_gen_neg_diag_info_t* pInfo, uint8_t* pu8Data);


