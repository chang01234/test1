#ifndef MAIN_TELEAIR_H_
#define MAIN_TELEAIR_H_

#include <stdint.h>
#include "payloads.h"
#include "pstore.h"

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t var_id;
}telair_ac_pid_diag_info_t;

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
}telair_gen_ucom_diag_ctrl_t;

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
}telair_ac_ucom32_diag_info_t;

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
}telair_ac_ucom33_diag_info_t;

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
}telair_gen_neg_diag_info_t;

#define TELEAIR_AC_REQ_FRAME_SIZE_DIAG    8

extern void telair_ac_pid_diag_I_Extract(telair_ac_pid_diag_info_t* pInfo, uint8_t* pu8Data);

extern void telair_generic_ucom_diag_C_Stuff(uint8_t* pu8Data, telair_gen_ucom_diag_ctrl_t* pCtrl);
extern void telair_ac_ucom32_diag_I_Extract(telair_ac_ucom32_diag_info_t* pInfo, uint8_t* pu8Data);
extern void telair_ac_ucom33_diag_I_Extract(telair_ac_ucom33_diag_info_t* pInfo, uint8_t* pu8Data);

extern void telair_generic_negative_diag_I_Extract(telair_gen_neg_diag_info_t* pInfo, uint8_t* pu8Data);


#endif //MAIN_TELEAIR_H_
