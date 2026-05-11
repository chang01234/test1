#ifndef MAIN_ALDE_H_
#define MAIN_ALDE_H_

#include <stdint.h>
#include "payloads.h"
#include "pstore.h"

typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t rsid; // service Identifier
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
    uint8_t var_id; // variant ID
}alde_3020_pid_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t new_nad;
}alde_3020_nad_diag_ctrl_t;

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
}alde_3020_nad_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t start_index;
    uint32_t product_id;
}alde_3020_frame_diag_ctrl_t;

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
}alde_3020_frame_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t req_sid;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}alde_gen_neg_diag_info_t;


#define ALDE_REQ_FRAME_SIZE_DIAG 8
extern void alde_3020_pid_diag_I_Extract(alde_3020_pid_diag_info_t* pInfo, uint8_t* pu8Data);

extern void alde_3020_assignnad_diag_C_Stuff(uint8_t* pu8Data, alde_3020_nad_diag_ctrl_t* pCtrl);
extern void alde_3020_assignnad_diag_I_Extract(alde_3020_nad_diag_info_t* pInfo, uint8_t* pu8Data);

extern void alde_3020_assignframe_diag_C_Stuff(uint8_t* pu8Data, alde_3020_frame_diag_ctrl_t* pCtrl);
extern void alde_3020_assignframe_diag_I_Extract(alde_3020_frame_diag_info_t* pInfo, uint8_t* pu8Data);

extern void alde_negative_diag_I_Extract(alde_gen_neg_diag_info_t* pInfo, uint8_t* pu8Data);

#endif //MAIN_ALDE_H_
