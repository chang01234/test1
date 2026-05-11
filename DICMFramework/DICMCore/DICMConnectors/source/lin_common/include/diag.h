//------------------------------------------------------------------------------
// Module:      diag.h
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef MAIN_DIAG_H_
#define MAIN_DIAG_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdint.h>
#include "payloads.h"
#include "pstore.h"

typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t sid; // service Identifier
    uint8_t id; // Identifier
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
}diag_generic_pid_ctrl_t;

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t var_id;
}diag_generic_pid_info_t; /* Diagnostic Generic Product Identifier Information Structure */

// Diagnostic Sleep command
typedef struct
{
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5; 
    uint8_t data6; 
    uint8_t data7; 
    uint8_t data8; 
} sleep_diag_ctrl_t;


extern void diag_generic_sleep_C_Stuff(uint8_t* pu8Data, sleep_diag_ctrl_t* pCtrl);

#define DIA_GENERIC_PID_REQ_FRAME_SIZE 8
extern void diag_generic_pid_C_Stuff(uint8_t* pu8Data, diag_generic_pid_ctrl_t* pCtrl);
extern void diag_generic_pid_I_Extract(diag_generic_pid_info_t* pInfo, uint8_t* pu8Data);

//Diagnostic Generic Transmit Handler
extern void  MsgLin_Generic_Prod_Info_Diag_C_TxHandle(uint8_t u8LinBus,  uint8_t* frame);
//Diagnostic Generic Message Receive Handler
extern void  MsgLin_Generic_Prod_Info_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

//Diagnostic PID Generic Receive Handlers
extern void  MsgLin_Generic_PID_Diag_I_RxHandle(uint8_t u8LinBus,  uint8_t* frame);

#endif  // DIAG_H
