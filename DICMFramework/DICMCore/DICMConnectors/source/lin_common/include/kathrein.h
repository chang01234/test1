#ifndef MAIN_KATHREIN_H_
#define MAIN_KATHREIN_H_

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
}kathrein_sat_pid_diag_info_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t custom; //0x32-SW version of aerial, 0x33-SW version of controller unit, 0x34-SW version of CI bus adapter, 0x35-TP-List date
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;

}kathrein_sat_sw_ver_diag_ctrl_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint16_t lan_code;//Language Code
    uint8_t month;
    uint8_t date;
    uint8_t year;
}kathrein_sat_sw_ver_diag_info_t;

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
}kathrein_neg_diag_info_t;

#define KATHREIN_REQ_FRAME_SIZE_DIAG 8

extern void kathrein_satelite_pid_diag_I_Extract(kathrein_sat_pid_diag_info_t* pInfo, uint8_t* pu8Data);

extern void kathrein_satelite_sw_ver_diag_C_Stuff(uint8_t* pu8Data, kathrein_sat_sw_ver_diag_ctrl_t* pCtrl);
extern void kathrein_satelite_sw_ver_diag_I_Extract(kathrein_sat_sw_ver_diag_info_t* pInfo, uint8_t* pu8Data);

extern void kathrein_negative_diag_I_Extract(kathrein_neg_diag_info_t* pInfo, uint8_t* pu8Data);

#endif //MAIN_KATHREIN_H_
