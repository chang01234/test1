#ifndef MAIN_TENHAAFT_H_
#define MAIN_TENHAAFT_H_

#include <stdint.h>
#include "payloads.h"
#include "pstore.h"

typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t rsid; // Response service Identifier
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
    uint8_t var_id; // variant ID
}tenhaaft_sat_pid_diag_info_t;

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
}tenhaaft_gen_neg_diag_info_t;

#define TENHAAFT_REQ_FRAME_SIZE_DIAG 8

extern void tenhaaft_sat_pid_diag_I_Extract(tenhaaft_sat_pid_diag_info_t* pInfo, uint8_t* pu8Data);

extern void tenhaaft_generic_negative_I_Extract(tenhaaft_gen_neg_diag_info_t* pInfo, uint8_t* pu8Data);

#endif //MAIN_TENHAAFT_H_
