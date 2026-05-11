//------------------------------------------------------------------------------
// Module:      NMEA2K.c
//------------------------------------------------------------------------------
// Description: This module implements J1939 & NMEA2K protocol stacks, including
//              address claim, ISO requests, multi-packet and fast packet
//              protocols.
//------------------------------------------------------------------------------
// Copyright:   SeaStar Solutions Inc.
//              3831, No 6 Road
//              Richmond, BC
//              Canada V6V 1P6
//
//              This source file and the information contained in it are
//              confidential and proprietary to SeaStar Solutions Inc.
//              The reproduction or disclosure, in whole or in part,
//              to anyone outside of SeaStar Solutions without the written
//              approval of a SeaStar Solutions officer under a Non-Disclosure
//              Agreement is expressly prohibited.
//
//              All rights reserved
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <string.h>  // for mem... functions

#include "HALCAN.h"
#include "NMEA2K.h"
#include "configuration.h"
#if CONFIG_RVC_US
#include "esp_random.h"
// COTEK sends only 4 packets for the PID though it claims to be 5.
#define TEST_FIX_COTEK_4_5_PID (1)
#endif
//------------------------------------------------------------------------------
// Private Definitions
//------------------------------------------------------------------------------
#define NMEA2K_RESERVED_FIFO_SPACE 2  // Used to define how many element in the FIFO
                                      // are reserved for the J1939 Process.
                                      // This reserved space is used to transmit all the kind
                                      // of acknowledge, request, response to Multi Packet.
                                      // This assition was done to avoid that the J1939 RX Mailbox
                                      // Fill completely the Mailbox.

#define NMEA2K_MAX_PF_PDU1FORMAT 239  // Maximum value that PDU format can have (EF)

// Request message parameters
#define NMEA2K_REQUEST_PGN      59904  // 0x00EA00  PDU1 format
#define NMEA2K_REQUEST_PRIORITY 6      // Default priority

// Acknowledgment message parameters
#define NMEA2K_ACKNOWLEDGMENT_PGN           59392  // 0x00E800  PDU1 format
#define NMEA2K_ACKNOWLEDGMENT_PRIORITY      6      // Default priority
#define NMEA2K_GROUP_FUNCTION_VALUE         0xFF   // Group function of PGN being acknowledged
#define NMEA2K_POSITIVE_ACKNOWLEDGMENT      0
#define NMEA2K_NEGATIVE_ACKNOWLEDGMENT      1
#define NMEA2K_PGN_SUPPORTED_ACCESS_DENIED  2
#define NMEA2K_PGN_SUPPORTED_CANNOT_RESPOND 3

// Address claimed message parameters
#define NMEA2K_ADDRESS_CLAIM_ENABLED  TRUE   // Address claim after power on self test
#define NMEA2K_ADDRESS_CLAIM_PGN      60928  // 0x00EE00  PDU1 format
#define NMEA2K_ADDRESS_CLAIM_PRIORITY 6      // Default priority

// Multi Packet Transmission Message Parameter
#define NMEA2K_MULTI_PACKET_TPCM_PGN 60416  // PGN used for the Transport Protocol Connection Management

#if CONFIG_RVC_US
/// change mult-packet prority from 7 to 6
#define NMEA2K_MULTI_PACKET_PRIORITY 6  // Priority Status of the Multi Packet Message
#else
#define NMEA2K_MULTI_PACKET_PRIORITY 7  // Priority Status of the Multi Packet Message
#endif

#define NMEA2K_MULTI_PACKET_TPDT_PGN    60160  // PGN Used for the Data Transfert
#define NMEA2K_MULTI_PACKET_TPCMRTS     16     // Control Byte Value for TP.CM_RTS
#define NMEA2K_MULTI_PACKET_TPCMCTS     17     // Control Byte value for TP.CM_CTS
#define NMEA2K_MULTI_PACKET_ENOFMSGACK  19     // Control Byte value for TP.CM_EndOfMsgACK
#define NMEA2K_MULTI_PACKET_TPCONNABORT 255    // Control Byte Value for TP.Conn_Abort
#define NMEA2K_MULTI_PACKET_TPCMBAM     32     // Control Byte Value for TP.CM_BAM

// Fast Packet Timing Adjustment
#define NMEA2K_FAST_PACKET_RX_TIME_OUT    75  // Maximum time between the reception of 2 Fast Packet of the same message
#define NMEA2K_FAST_PACKET_TX_INTER_FRAME 0   // Time between the transmission of 2 packet in step of 10 mS

//------------------------------------------------------------------------------
// Private Types
//------------------------------------------------------------------------------
// 8-byte NAME structure
typedef struct nmea2k_NAME_Fields_Struct
{
    uint8 IdentityNumber_Byte1;  // RVC optional, serial number(LSB)
    uint8 IdentityNumber_Byte2;  // RVC optional, serial number
    struct
    {
        bit IdentityNumber_Byte3 : 5;  // RVC optional, serial number
        bit ManufacturerCode_Byte3 : 3;
    } Byte3;
    uint8 ManufacturerCode_Byte4;
    struct
    {
        bit EcuInstance : 3;             // RVC node instance, default 0
        bit DeviceFunctionInstance : 5;  // RVC Function instance, default 0
    } Byte5;                             //
    uint8 DeviceFunction;                // RVC compatibility field, normally 0
    struct
    {
        bit Reserved : 1;       // RVC, always 0
        bit VehicleSystem : 7;  // RVC optional, normally 0
    } Byte7;
    struct
    {
        bit VehicleSystemInstance : 4;  // RVC Compatibility field, normally 0
        bit IndustryGroup : 3;          // RVC Compatibility field, normally 0
        bit ArbitraryAddressCapable : 1;
    } Byte8;
} nmea2k_NAME_Fields_Struct_Type;

// ----------------------------------------------------------------------------
// BIG ENDIAN STRUCTURE DEFINITIONS
// ----------------------------------------------------------------------------
#if CPU_BIT_ORDER == MSB_FIRST
// Message ID structure
typedef union nmea2k_Id_Struct
{
    uint32 Identifier;  // 29-bits
    struct
    {
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 Priority : 3;
            uint8 UnusedMSB : 3;
        } BitField;
        uint8 PduFormat;
        uint8 PduSpecific;
        uint8 SourceAddress;
    } IdField;
} nmea2k_Id_Struct_Type;

// Modified version of the standard J1939 29 bits identifier
// which reduces processing time and code complexity
typedef union nmea2k_MailBox_ID_Struct
{
    uint32 Identifier;
    struct
    {
        uint8 Unused;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 UnusedMSB : 6;
        } BitField;
        uint8 PduFormat;
        uint8 PduSpecific;
    } IdField;
} nmea2k_MailBox_ID_Struct_Type;

#endif

// ----------------------------------------------------------------------------
// LITTLE ENDIAN STRUCTURE DEFINITIONS
// ----------------------------------------------------------------------------
#if CPU_BIT_ORDER == LSB_FIRST

// Message ID structure
typedef union nmea2k_Id_Struct
{
    uint32 Identifier;  // 29-bits
    struct
    {
        uint8 SourceAddress;
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 Priority : 3;
            uint8 UnusedMSB : 3;
        } BitField;
    } IdField;
} nmea2k_Id_Struct_Type;

// Modified version of the standard J1939 29 bits identifier
// which reduces processing time and code complexity
typedef union nmea2k_MailBox_ID_Struct
{
    uint32 Identifier;
    struct
    {
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 UnusedMSB : 6;
        } BitField;
        uint8 Unused;
    } IdField;
} nmea2k_MailBox_ID_Struct_Type;

#endif

// Single frame message structure
typedef struct nmea2k_SingleFrame_Message_Struct
{
    nmea2k_Id_Struct_Type MsgId;  // 29-bits message identifier
    uint8 ByteCntr;               // Single frame data byte counter
    uint8 *BytePtr;               // Single frame data byte pointer
} nmea2k_SingleFrame_Message_Struct_Type;

// Request message structure
typedef struct nmea2k_Request_Message_Struct
{
    uint8 PduSpecific;
    uint8 PduFormat;
    struct
    {
        bit DataPage : 1;
        bit Reserved : 1;
        bit UnusedMSB : 6;
    } BitField;
#if CONFIG_RVC_US
    /// RVC has 8 bytes in the request
    uint8 rvcExtra[5];
#endif
} nmea2k_Request_Message_Struct_Type;

// Acknowlegment message structure used in address claiming
typedef struct nmea2k_Acknowledgment_Message_Struct
{
    uint8 AckStatus;
    uint8 GroupFnct;
    uint8 Byte3;
    uint8 Byte4;
    uint8 Byte5;
    uint8 PduSpecific;
    uint8 PduFormat;
    struct
    {
        bit DataPage : 1;
        bit UnusedMSB : 7;
    } BitField;
} nmea2k_Acknowledgment_Message_Struct_Type;

// Address claim state variable structure
typedef struct nmea2k_Address_Claim_State_Variables_Struct
{
    bit IdleState : 1;
    bit StartRandomDelay : 1;
    bit Start250MsDelay : 1;
    bit DelayComplete : 1;
    bit SelectStartAddress : 1;
    bit FetchNextMyAddress : 1;
    bit CommandedAddress : 1;
    bit AddressClaimed : 1;
    bit CannotClaimAddress : 1;
    bit IWin : 1;
    bit ILose : 1;
    bit CanMsgSent : 1;
    bit NoMessage : 1;
    bit RequestPgn : 1;
} nmea2k_Address_Claim_State_Variables_Struct_Type;

// Multi packet receive session structure
typedef struct nmea2k_RxMultiPacket_State_Variables_Struct
{
    bit InProgress : 1;        // Reception of MultiPacket (0=Idle, 1=InProgress)
    uint8 DelayTimer;          // Delay timer use to determine if a Multi Packet session
                               // needs to be abort.
    uint8 DA;                  // Destination address of the MultiPacket message
                               // It can take only two values, Global(255) or specific(nmea2k_MyAddress[nmea2k_u8Instance])
    uint8 Originator;          // Source Address of the Data Originator
    uint32 PGN;                // PGN of the requested data with Reserved and DataPage
    uint16 Msg_Size;           // Total Message Size, Number of bytes
    uint16 Trunk_Size;         // When "Trunk_Size" of Bytes was received, the data are no more
                               // written in the Buffer. But the session continue. The Data
                               // are simply not proceed. This feature can be usefull for
                               // Multi Packet PGN with variable length like FMI Code.
    uint8 Num_Byte_Rx;         // Number of received bytes. Use to determine if the message
                               // was received completely.
    uint8 Packet_Size;         // Total Number of Packet
    uint8 Sequence_Number;     // Counter use to keep track of the packet sequence number (1-255)
    uint8 Max_Num_Packet_CTS;  // Maximum number of packets that can be received before
                               // sending a CTS. This number is the smallest one between
                               // the Originator and the Responder
    uint8 Num_Packet_CTS;      // The Sequence number is used to determine if it's the
                               // time to transmit a CTS following Max_Num_Packet_CTS value
                               // Num_Packet_CTS is loaded with Max_Num_Packet_CTS and decrement
                               // at the reception of each packet. When it reach ZERO, CTS is send.

    uint8 *Byte_Pgm_Ptr;            // Pointer to the Byte that need to be Program
    NMEA2K_RxMsg_Struct *RxMsgPtr;  // Pointer to the Receive Mailbox. Value == NULL when not used
} nmea2k_RxMultiPacket_State_Variables_Struct_Type;

// Multi packet transmit session structure
typedef struct nmea2k_TxMultiPacket_State_Variables_Struct
{
    bit InProgress : 1;             // Reception of MultiPacket (0=Idle, 1=InProgress)
    uint8 DelayTimer;               // Delay timer use to determine if a Multi Packet session
                                    // needs to be abort.
    uint8 NextTxTimer;              // Used to determine when the next Data Transfert is needed
    uint8 DA;                       // Destination address of the MultiPacket message
                                    // It can take any values between 0 and 255 ( 255 = Global Session)
    uint8 Originator;               // Source Address of the Data Originator (It correspond to my address)
    uint32 PGN;                     // PGN of the transmitted Data with Reserved and DataPage
    uint16 Msg_Size;                // Total Message Size, Number of bytes
    uint8 Packet_Size;              // Total Number of Packet
    uint8 Sequence_Number;          // Counter use to keep track of the packet sequence number (1-255)
    uint8 Max_Num_Packet_CTS;       // Maximum number of packets that can be transmit before
                                    // reception of a CTS. This number is the smallest one between
                                    // the Originator and the Responder
    uint8 Num_Packet_CTS;           // The Sequence number is used to determine if it's the
                                    // time to receive a CTS following Max_Num_Packet_CTS value
                                    // Num_Packet_CTS is loaded with Max_Num_Packet_CTS and decrement
                                    // at the reception of each packet. When it reach ZERO, CTS is send.
    uint8 *Data_Pgm_Ptr;            // Pointer to the Beginning of the Data that needs to be transmitted
    NMEA2K_TxMsg_Struct *TxMsgPtr;  // Pointer to the Transmit Mailbox. Value == NULL when not used
} nmea2k_TxMultiPacket_State_Variables_Struct_Type;

// Fast packet receive session structure
typedef struct nmea2k_RxFastPacket_State_Variables_Struct
{
    bit InProgress : 1;             // Reception of FastPacket (0=Idle, 1=InProgress)
    uint8 SeqCounter;               // Sequence Counter used to distinguish different Fast Packet session
    uint8 Frame_Counter;            // Number of frame received
    uint8 DelayTimer;               // Delay timer use to determine if a Multi Packet session
                                    // needs to be abort.
    uint8 Num_Byte_Rx;              // Number of received bytes. Use to determine if the message
                                    // was received completely.
    uint8 *Byte_Pgm_Ptr;            // Pointer to the next Byte that need to be Program
    NMEA2K_RxMsg_Struct *RxMsgPtr;  // Fixed pointer to the J1939 Rx Mailbox element
                                    // The pointer is initialize by J1939_Initialization
} nmea2k_RxFastPacket_State_Variables_Struct_Type;

// Fast packet transmit session structure
typedef struct nmea2k_TxFastPacket_State_Variables_Struct
{
    bit InProgress : 1;      // Reception of FastPacket (0=Idle, 1=InProgress)
    uint8 SeqCounter;        // This is the sequence counter used during transmission of PGN
    uint8 u8NextSeqCounter;  // This is the next sequence counter available for transmission
    uint8 TxFrame_Counter;   // Number of the last frame transmitted
    uint8 NextTxTimer;       // Used to determine when the next Data Transfert is needed

    uint8 Num_Byte_Tx;  // Number of received bytes. Use to determine if the message
                        // was received completely.

    uint8 *Byte_Tx_Ptr;                             // Pointer to the next Byte that need to Sent
    NMEA2K_TxMsg_Struct *TxMsgPtr;                  // Fixed pointer to the J1939 Rx Mailbox element
                                                    // The pointer is initialize by J1939_Initialization
} nmea2k_TxFastPacket_State_Variables_Struct_Type;  // Bit Mapping is not important

// RV-C address claim request data
typedef struct rvc_address_claim_request_Struct
{
    uint8 byte0;
    uint8 byte1;
    uint8 byte2;
    uint8 byte3_7[5];
} rvc_address_claim_request_Struct_Type;

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------
#if NMEA2K_COMMANDED_ADDRESS_ENABLED
uint8 nmea2k_Commanded[NMEA2K_INSTANCE_COUNT];
#endif
static uint8 PropDGN_DA = 0;
//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------
// Status of the J1939 application
static NMEA2K_Status_Struct nmea2k_Status[NMEA2K_INSTANCE_COUNT];

// Instance index and count
static uint8 nmea2k_u8Instance = 0;
static uint8 nmea2k_u8InstanceCount = 0;

// Pointer to the RX and TX J1939 Mailbox
static NMEA2K_Parameter_Struct *nmea2k_Parameter[NMEA2K_INSTANCE_COUNT];
static nmea2k_Address_Claim_State_Variables_Struct_Type nmea2k_AddrClaim[NMEA2K_INSTANCE_COUNT];

static void (*Nmea2k_Dll_G7_AddressClaim[NMEA2K_INSTANCE_COUNT])(void);
static uint16 nmea2k_FreeRunTmr[NMEA2K_INSTANCE_COUNT];

#define MIN_DELAY (250)
#define MAX_DELAY (400)
static uint16 nmea2k_DelayTmr[NMEA2K_INSTANCE_COUNT];
static uint8 nmea2k_MyAddress[NMEA2K_INSTANCE_COUNT];  // Used Locally by the address claim process. When the address claim process
                                                       // is finished J1939_Status is updated if needed.
static uint8 nmea2k_Answered[NMEA2K_INSTANCE_COUNT];

#if NMEA2K_SUPPORT_MULTIPACKET
// Use to keep track of the Multi Packet session states and variables
static EXT_RAM_ATTR nmea2k_RxMultiPacket_State_Variables_Struct_Type nmea2k_RxMultiPacket[NMEA2K_INSTANCE_COUNT][NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION];
static EXT_RAM_ATTR nmea2k_TxMultiPacket_State_Variables_Struct_Type nmea2k_TxMultiPacket[NMEA2K_INSTANCE_COUNT][NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION];
#endif

// Use to keep track of the Multi Packet session states and variables
#if NMEA2K_SUPPORT_FAST_PACKET_RX
static EXT_RAM_ATTR nmea2k_RxFastPacket_State_Variables_Struct_Type RxFastPacket[NMEA2K_INSTANCE_COUNT][NMEA2K_NUMBER_RX_FAST_PACKET_SESSION];
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_TX
static nmea2k_TxFastPacket_State_Variables_Struct_Type TxFastPacket[NMEA2K_INSTANCE_COUNT][NMEA2K_NUMBER_TX_FAST_PACKET_SESSION];
#endif

static uint8 nmea2k_u8Timer_10ms[NMEA2K_INSTANCE_COUNT];   // Timer used for triggering 10ms events
static uint8 nmea2k_u8Timer_100ms[NMEA2K_INSTANCE_COUNT];  // Timer used for triggering 100ms events

static uint8 nmea2k_u8CanPortToInst[NMEA2K_INSTANCE_COUNT];

static address_claimed_callback_t l_address_claimed_cb;
static dgn_tx_ready_callback_t l_dgn_tx_ready_cb;
static mp_dgn_rx_callback_t l_mp_dgn_raw_rx_cb;
#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
static standard_dgn_rx_callback_t l_std_dgn_raw_rx_cb;
#endif
//------------------------------------------------------------------------------
// Private Functions
//------------------------------------------------------------------------------
static void Nmea2k_TxManagerTask(void);
static void Nmea2k_RxManagerTask(void);
static void Nmea2k_PopulateNAME(nmea2k_NAME_Fields_Struct_Type *pzNAME, const NMEA2K_Parameter_Struct *pParams);
static uint8 Nmea2k_Tx_ISOAddressClaim(uint8 SA);
static uint8 Nmea2k_Tx_ISOAcknowledgment(uint8 DA, uint8 SA, uint32 PGN, uint8 GroupFnct, uint8 Status);
static uint8 Nmea2k_Tx_ISORequest(uint8 DA, uint8 SA, uint32 PGN);
static void Nmea2k_Rx_ISOAddressClaim(HALCAN_zMSG *MessagePtr);
// static void     Nmea2k_Rx_ISOAcknowledgment(HALCAN_zMSG *MessagePtr);
static void Nmea2k_Rx_ISORequest(HALCAN_zMSG *MessagePtr);
static bool Nmea2k_Dll_TxSingleFrameMessage(nmea2k_SingleFrame_Message_Struct_Type *Message);
// static uint8    Nmea2k_Dll_RxSingleFrameMessage(nmea2k_SingleFrame_Message_Struct_Type *Message);
static uint16 Nmea2k_Dll_GetRandomTimeDelay(uint8 *NameFld);
static uint8 Nmea2k_DLL_Fill_nmea2kRxMailbox(HALCAN_zMSG *Msg_Can, uint32 pgn, uint8 SA, uint8 DA);
static uint8 Nmea2k_Dll_SetTx_nmea2kTxMailbox(uint32 PGN_Number, uint8 u8ReplyDestAddr);
static void Nmea2k_DLL_Update_TxCounter(void);
static void Nmea2k_DLL_Upd_AgeingCtr_nmea2kRxMailbox(void);

#if NMEA2K_SUPPORT_MULTIPACKET
static NMEA2K_RxMsg_Struct *Nmea2k_DLL_Check_nmea2kRxMailbox_MultiPacket(uint32 pgn, uint8 SA);
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_RX
static uint8 Nmea2k_Dll_RxFastPacketMsg(HALCAN_zMSG *Msg_Can, NMEA2K_RxMsg_Struct *Msg_nmea2k, uint8 SA, uint8 DA);
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_TX
static uint8 Nmea2k_Dll_TxFastPacket_Init(NMEA2K_TxMsg_Struct *Msg_nmea2k);
static void Nmea2k_Dll_TxFastPacket(void);
#endif

static void Nmea2k_Dll_FastPacket_TimeOut_10mS(void);

#if NMEA2K_SUPPORT_MULTIPACKET
// Multi Packet PGN Reception and Transission
static void Nmea2k_Rx_TP_CM_RTS(HALCAN_zMSG *MessagePtr, uint8 SA);
static void Nmea2k_Rx_TP_Conn_Abort(HALCAN_zMSG *MessagePtr, uint8 Originator_Addr);
static uint8 Nmea2k_Tx_TP_Conn_Abort(uint8 Second_Node_Addr, uint8 Abort_Reason, uint32 PGN);
static uint8 Nmea2k_Tx_TP_CM_CTS(nmea2k_RxMultiPacket_State_Variables_Struct_Type *RxMultiPacket_PTR);
static uint8 Nmea2k_Tx_TP_CM_EndOfMsgACK(nmea2k_RxMultiPacket_State_Variables_Struct_Type *RxMultiPacket_PTR);
static void Nmea2k_Rx_TP_CM_BAM(HALCAN_zMSG *MessagePtr, uint8 Originator_Address);
static void Nmea2k_Rx_DataTransfert(HALCAN_zMSG *MessagePtr, uint8 Originator_Addr, uint8 Dest_Addr);
static bool Nmea2k_Tx_TP_CM_RTS(nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR);
static bool Nmea2k_Tx_TP_CM_BAM(nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR);
static void Nmea2k_Rx_TP_CM_CTS(HALCAN_zMSG *MessagePtr, uint8 Originator_Addr);
static uint8 Nmea2k_Tx_DataTransfert(nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR);
static void Nmea2k_Rx_TP_CM_EndOfMsgACK(HALCAN_zMSG *MessagePtr, uint8 Originator_Addr);
static uint8 Nmea2k_Dll_TxMultiPacket_Init(NMEA2K_TxMsg_Struct *Msg_nmea2k);
static void Nmea2k_Dll_MultiPacket_TimeOut(void);
static void Nmea2k_Dll_MultiPacket_Transmit(void);
static void Nmea2k_Dll_MultiPacket_UpdCtr(void);
#endif

// Address claim grafcet states and transitions
static void Nmea2k_Dll_S0_AddressClaim_Idle(void);
static void Nmea2k_Dll_S1_AddressClaim_RandomDelay(void);
static void Nmea2k_Dll_S2_AddressClaim_Transmit(void);
static void Nmea2k_Dll_S3_AddressClaim_RxCanFrame(void);
static void Nmea2k_Dll_S4_AddressClaim_FetchNextMyAddress(void);
static void Nmea2k_Dll_S5_AddressClaim_RxCanFrame(void);
static void Nmea2k_Dll_S6_AddressClaim_RandomDelay(void);
static void Nmea2k_Dll_S7_AddressClaim_Transmit(void);
static void Nmea2k_Dll_S8_AddressClaim_RxCanFrame(void);
static void Nmea2k_Dll_S9_AddressClaim_Transmit(void);
static void Nmea2k_Dll_T0_AddressClaim_Idle(void);
static void Nmea2k_Dll_T1_AddressClaim_RandomDelay(void);
static void Nmea2k_Dll_T3_AddressClaim_RxCanFrame(void);
static void Nmea2k_Dll_T4_AddressClaim_FetchNextMyAddress(void);
static void Nmea2k_Dll_T5_AddressClaim_RxCanFrame(void);
static void Nmea2k_Dll_T6_AddressClaim_RandomDelay(void);
static void Nmea2k_Dll_T8_AddressClaim_RxCanFrame(void);

//-----------------------------------------------------------------------------
// Function:    NMEA2K_Initialize
//-----------------------------------------------------------------------------
// Description: Initializes one instance of the NMEA2K protocol stack.
//-----------------------------------------------------------------------------
// Return:      TRUE = No error, FALSE = Too many instances.
//-----------------------------------------------------------------------------
bool NMEA2K_Initialize(
    NMEA2K_Parameter_Struct *Parameter  // Pointer to the NMEA2K parameter structure.
)
{
    bool bResult = FALSE;
    uint8 i;

    l_mp_dgn_raw_rx_cb = NULL;
#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
    l_std_dgn_raw_rx_cb = NULL;
#endif
    l_dgn_tx_ready_cb = NULL;
    l_address_claimed_cb = NULL;

#if NMEA2K_SUPPORT_FAST_PACKET_RX || NMEA2K_SUPPORT_FAST_PACKET_TX
    uint8 j;
#endif

    // Not too many instances?
    if (nmea2k_u8InstanceCount < NMEA2K_INSTANCE_COUNT)
    {
#if NMEA2K_SUPPORT_MULTIPACKET
        nmea2k_RxMultiPacket_State_Variables_Struct_Type *nmea2k_RxMultiPacket_PTR;
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_RX
        NMEA2K_RxMsg_Struct *nmea2k_RxMsgPtr;
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_TX
        NMEA2K_TxMsg_Struct *nmea2k_TxMsgPtr;
#endif

        // Store mailbox pointer
        nmea2k_Parameter[nmea2k_u8InstanceCount] = Parameter;

        // Initialize source addresses
        nmea2k_MyAddress[nmea2k_u8InstanceCount] = NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS;
        nmea2k_Answered[nmea2k_u8InstanceCount] = NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS;

#if NMEA2K_COMMANDED_ADDRESS_ENABLED
        nmea2k_Commanded[nmea2k_u8InstanceCount] = NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS;
#endif

        // Preferred source address valid?
        if (*(nmea2k_Parameter[nmea2k_u8InstanceCount]->PreferredSA) <= NMEA2K_ADDRESS_CLAIM_MAX_ADDRESS)
        {
            // Initialize source address for this node
            nmea2k_MyAddress[nmea2k_u8InstanceCount] = *(nmea2k_Parameter[nmea2k_u8InstanceCount]->PreferredSA);
        }

        // Initialize address claim state variables
        nmea2k_Status[nmea2k_u8InstanceCount].AddClaim = FALSE;
        nmea2k_Status[nmea2k_u8InstanceCount].CAN_BusOK = 1;
        nmea2k_Status[nmea2k_u8InstanceCount].SA = nmea2k_MyAddress[nmea2k_u8InstanceCount];

#if NMEA2K_SUPPORT_MULTIPACKET
        // For all possible multi-packet sessions...
        for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
        {
            // Initialize the multi-packet structure
            nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8InstanceCount][i];
            nmea2k_RxMultiPacket_PTR->InProgress = 0;
            nmea2k_RxMultiPacket_PTR->RxMsgPtr = NULL;
        }
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_RX

        // Clear the entire fast packet RX structure
        (void)memset(RxFastPacket[nmea2k_u8InstanceCount], 0, sizeof(RxFastPacket[0]));

        // Initialize loop variables
        nmea2k_RxMsgPtr = nmea2k_Parameter[nmea2k_u8InstanceCount]->RxMailbox;
        j = 0;

        // For all receive mailbox entries...
        for (i = 0; i < nmea2k_Parameter[nmea2k_u8InstanceCount]->RxMailboxSize; i++)
        {
            // Fast packet message?
            if (nmea2k_RxMsgPtr->Pgn_Type == 1)
            {
                //     Fail (bResult) init if j is greater than NMEA2K_NUMBER_RX_FAST_PACKET_SESSION. Also alert if memory is wasted.
                //       Consider bResult from MSGCAN and throw fault so designer knows the array is too small (or large?).
                //       Even better use preprocessing to determine array size. Would save memory and guarentee array length.
                // Store receive mailbox pointer for this fast-packet handler
                RxFastPacket[nmea2k_u8InstanceCount][j].RxMsgPtr = nmea2k_RxMsgPtr;
                j++;
            }
            nmea2k_RxMsgPtr++;
        }
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_TX

        // Clear the entire fast packet TX structure
        (void)memset(TxFastPacket[nmea2k_u8InstanceCount], 0, sizeof(TxFastPacket[0]));

        // Initialize loop variables
        nmea2k_TxMsgPtr = nmea2k_Parameter[nmea2k_u8InstanceCount]->TxMailbox;
        j = 0;

        // For all transmit mailbox entries...
        for (i = 0; i < nmea2k_Parameter[nmea2k_u8InstanceCount]->TxMailboxSize; i++)
        {
            // Fast packet message?
            if (nmea2k_TxMsgPtr->Pgn_Type == 1)
            {
                // Fail init if j is greater than NMEA2K_NUMBER_TX_FAST_PACKET_SESSION
                // Store transmit mailbox pointer for this fast-packet handler
                TxFastPacket[nmea2k_u8InstanceCount][j].TxMsgPtr = nmea2k_TxMsgPtr;
                j++;
            }
            nmea2k_TxMsgPtr++;
        }

#endif

        // State machine initialization
        if (nmea2k_MyAddress[nmea2k_u8InstanceCount] != NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS)
        {
            Nmea2k_Dll_G7_AddressClaim[nmea2k_u8InstanceCount] = Nmea2k_Dll_S0_AddressClaim_Idle;
        }
        else
        {
            Nmea2k_Dll_G7_AddressClaim[nmea2k_u8InstanceCount] = Nmea2k_Dll_S6_AddressClaim_RandomDelay;
        }

        // Map instance to CAN port for easy indexing
        nmea2k_u8CanPortToInst[Parameter->u8CanPort] = nmea2k_u8InstanceCount;

        // Initialize timers and timeouts
        nmea2k_u8Timer_10ms[nmea2k_u8InstanceCount] = 0;
        nmea2k_u8Timer_100ms[nmea2k_u8InstanceCount] = 0;

        // Increment number of instances used.
        nmea2k_u8InstanceCount++;

        // Successfully initialized
        bResult = TRUE;
    }
    // else
    // Provide a way to trigger a fault or something else to warn designer.

    return bResult;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_UpdateTimers
//-----------------------------------------------------------------------------
// Description: This function updates the millisecond time bases used by the
//              address claim process and used to trigger periodical transmits.
//
//              Ideally this function should be called every 1 ms, but for
//              flexibility the elapsed time between function calls is passed
//              as a parameter for situations when this is not possible.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void NMEA2K_UpdateTimers(
    uint8 u8ElapsedTime  // Time that has elapsed since the last call of this function in ms
)
{
    // For all instances of the protocol stack...
    for (nmea2k_u8Instance = 0; nmea2k_u8Instance < nmea2k_u8InstanceCount; nmea2k_u8Instance++)
    {
        // Increment the timers by the elapsed time
        nmea2k_u8Timer_10ms[nmea2k_u8Instance] += u8ElapsedTime;
        nmea2k_u8Timer_100ms[nmea2k_u8Instance] += u8ElapsedTime;
        nmea2k_FreeRunTmr[nmea2k_u8Instance] += u8ElapsedTime;

        // Update the address claim state machine
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance]();

        // Process all pending 10ms timeout events?
        while (nmea2k_u8Timer_10ms[nmea2k_u8Instance] >= 10)
        {
            // Check if messages need to be sent
            Nmea2k_DLL_Update_TxCounter();

#if NMEA2K_SUPPORT_MULTIPACKET
            // Update multi-packet timers and delays
            Nmea2k_Dll_MultiPacket_UpdCtr();
#endif

            // Update fast packet timers and delays
            Nmea2k_Dll_FastPacket_TimeOut_10mS();

            nmea2k_u8Timer_10ms[nmea2k_u8Instance] -= 10;
        }

        // Process all pending 100ms timeout events
        while (nmea2k_u8Timer_100ms[nmea2k_u8Instance] >= 100)
        {
            // Update RX mailbox aging counters
            Nmea2k_DLL_Upd_AgeingCtr_nmea2kRxMailbox();

            nmea2k_u8Timer_100ms[nmea2k_u8Instance] -= 100;
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_ProcessRx
//-----------------------------------------------------------------------------
// Description: This function is called by the application and is used to
//              refresh RX J1939 processes.
//
//              Must be called a often as possible. Doing so speeds up the
//              field flashing process for instance.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void NMEA2K_ProcessRx(void)
{
    // For all instances of the protocol stack...
    for (nmea2k_u8Instance = 0; nmea2k_u8Instance < nmea2k_u8InstanceCount; nmea2k_u8Instance++)
    {
        // Receive all messages
        Nmea2k_RxManagerTask();
    }
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_ProcessTx
//-----------------------------------------------------------------------------
// Description: This function is called by the application and is used to
//              refresh TX J1939 processes.
//
//              Must be called a often as possible. Doing so speeds up the
//              field flashing process for instance.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void NMEA2K_ProcessTx(void)
{
    // For all instances of the protocol stack...
    for (nmea2k_u8Instance = 0; nmea2k_u8Instance < nmea2k_u8InstanceCount; nmea2k_u8Instance++)
    {

#if NMEA2K_SUPPORT_MULTIPACKET
        // Check for multi-packet timeouts
        Nmea2k_Dll_MultiPacket_TimeOut();

        // Transmit a multi-packet frame if required
        Nmea2k_Dll_MultiPacket_Transmit();
#endif

#if NMEA2K_SUPPORT_FAST_PACKET_TX
        // Transmit a fast-packet frame if required
        Nmea2k_Dll_TxFastPacket();
#endif

        // Transmit all messages
        Nmea2k_TxManagerTask();
    }
}

void NMEA2K_SetAddressClaimedCallback(address_claimed_callback_t address_claimed_cb)
{
    l_address_claimed_cb = address_claimed_cb;
}

void NMEA2K_SetDGNTxReadyCallback(dgn_tx_ready_callback_t dgn_tx_ready_cb)
{
    l_dgn_tx_ready_cb = dgn_tx_ready_cb;
}

void NMEA2K_SetMPRawDataCallback(mp_dgn_rx_callback_t mp_raw_data_cb)
{
    l_mp_dgn_raw_rx_cb = mp_raw_data_cb;
}

#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
void NMEA2K_SetStandardDGNCallback(standard_dgn_rx_callback_t std_raw_data_cb)
{
    l_std_dgn_raw_rx_cb = std_raw_data_cb;
}
#endif

//-----------------------------------------------------------------------------
// Function:    NMEA2K_GetSourceAddr
//-----------------------------------------------------------------------------
// Description: Get the source address for this device on the specified CAN
//              port
//-----------------------------------------------------------------------------
// Return:      My source address
//-----------------------------------------------------------------------------
uint8 NMEA2K_GetSourceAddr(uint8 u8CanPort)
{
    return nmea2k_MyAddress[nmea2k_u8CanPortToInst[u8CanPort]];
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_GetDestAddrProp
//-----------------------------------------------------------------------------
// Description: Get the destination address from the proprietary DGN
//-----------------------------------------------------------------------------
// Return:      The destination address
//-----------------------------------------------------------------------------
uint8 NMEA2K_GetDestAddrProp(void)
{
    return PropDGN_DA;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_SetRxEnable
//-----------------------------------------------------------------------------
// Description: Enable/Disable reception of all messages that matches the
//              CAN port, PGN.
//-----------------------------------------------------------------------------
// Return:      TRUE if successful, FALSE otherwise
//-----------------------------------------------------------------------------
bool NMEA2K_SetRxEnable(
    uint8 u8CanPort,  // Requested CAN port
    uint32 u32PGN,    // Requested PGN
    bool bEnable      // TRUE = Enable; FALSE = Disable
)
{
    uint8 u8Index;
    bool bResult = FALSE;
    NMEA2K_Parameter_Struct *pzN2KParams;

    // Invalid CAN port
    if (u8CanPort >= BSP_CAN_COUNT)
    {
        return FALSE;
    }

    // Find the NMEA2K instance associated with the CAN port
    pzN2KParams = nmea2k_Parameter[nmea2k_u8CanPortToInst[u8CanPort]];

    // find matching rx messages
    for (u8Index = 0; u8Index < pzN2KParams->RxMailboxSize; u8Index++)
    {
        // Matching mailbox found?
        if (pzN2KParams->RxMailbox[u8Index].PGN == u32PGN)
        {
            pzN2KParams->RxMailbox[u8Index].Enable = bEnable;
            bResult = TRUE;
        }
    }

    return bResult;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_SetRxAddrFilter
//-----------------------------------------------------------------------------
// Description: Set the address filter on all messages that matches the
//              CAN port, PGN.
//-----------------------------------------------------------------------------
// Return:      TRUE if successful, FALSE otherwise
//-----------------------------------------------------------------------------
bool NMEA2K_SetRxAddrFilter(
    uint8 u8CanPort,    // Requested CAN port
    uint32 u32PGN,      // Requested PGN
    uint8 u8AddrFilter  // Requested address filter (0xFF = accept all addresses)
)
{
    uint8 u8Index;
    bool bResult = FALSE;
    NMEA2K_Parameter_Struct *pzN2KParams;

    // Invalid CAN port
    if (u8CanPort >= BSP_CAN_COUNT)
    {
        return FALSE;
    }

    // Find the NMEA2K instance associated with the CAN port
    pzN2KParams = nmea2k_Parameter[nmea2k_u8CanPortToInst[u8CanPort]];

    // find matching rx messages
    for (u8Index = 0; u8Index < pzN2KParams->RxMailboxSize; u8Index++)
    {
        // Matching mailbox found?
        if (pzN2KParams->RxMailbox[u8Index].PGN == u32PGN)
        {
            pzN2KParams->RxMailbox[u8Index].Org_Filter = u8AddrFilter;
            bResult = TRUE;
        }
    }

    return bResult;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_SetTxRequest
//-----------------------------------------------------------------------------
// Description: Set a transmit request on the message that matches the CAN
//              port, PGN, and instance. A TX request will only be
//              successfully set if there is not an existing transmit already
//              in progress.
//-----------------------------------------------------------------------------
// Return:      TRUE if successful, FALSE otherwise
//-----------------------------------------------------------------------------
bool NMEA2K_SetTxRequest(
    uint8 u8CanPort,  // Requested CAN port
    uint32 u32PGN,    // Requested PGN
    uint8 u8Instance  // Requested PGN instance
)
{
    uint8 u8Index;
    bool bResult = FALSE;
    NMEA2K_Parameter_Struct *pzN2KParams;

    // Invalid CAN port
    if (u8CanPort >= BSP_CAN_COUNT)
    {
        return FALSE;
    }

    // Find the NMEA2K instance associated with the CAN port
    pzN2KParams = nmea2k_Parameter[nmea2k_u8CanPortToInst[u8CanPort]];

    // find matching tx mailbox
    for (u8Index = 0; u8Index < pzN2KParams->TxMailboxSize; u8Index++)
    {
        // LOG(I, "Txreq1 %d %d %d %d %d", (int)pzN2KParams->TxMailbox[u8Index].PGN, pzN2KParams->TxMailbox[u8Index].Instance, pzN2KParams->TxMailbox[u8Index].TxReq, pzN2KParams->TxMailbox[u8Index].TxReady, pzN2KParams->TxMailbox[u8Index].TxInProgress);
        // PGN and instance match found AND
        // no request already pending?
        if ((pzN2KParams->TxMailbox[u8Index].PGN == u32PGN) &&
            (pzN2KParams->TxMailbox[u8Index].Instance == u8Instance) &&
            (pzN2KParams->TxMailbox[u8Index].TxReq == NMEA2K_TXREQ_NONE) &&
            (pzN2KParams->TxMailbox[u8Index].TxReady == FALSE) &&
            (pzN2KParams->TxMailbox[u8Index].TxInProgress == FALSE))
        {
            pzN2KParams->TxMailbox[u8Index].TxReq = NMEA2K_TXREQ_EVENT;
            bResult = TRUE;
            LOG(D, "Txreq2: DGN: %d", u32PGN);
            break;
        }
    }

    return bResult;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_SetTxPeriodic
//-----------------------------------------------------------------------------
// Description: Set the transmit periodic flag and transmit period on the
//              message that matches the CAN port, PGN and instance.
//-----------------------------------------------------------------------------
// Return:      TRUE if successful, FALSE otherwise
//-----------------------------------------------------------------------------
bool NMEA2K_SetTxPeriodic(
    uint8 u8CanPort,   // Requested CAN port
    uint32 u32PGN,     // Requested PGN
    uint8 u8Instance,  // Requested PGN instance
    bool bEnable,      // TRUE = enabled, FALSE = disabled
    uint8 u8Period     // Broadcast period (10ms/bit)
)
{
    uint8 u8Index;
    bool bResult = FALSE;
    NMEA2K_Parameter_Struct *pzN2KParams;

    // Invalid CAN port
    if (u8CanPort >= BSP_CAN_COUNT)
    {
        return FALSE;
    }

    // Find the NMEA2K instance associated with the CAN port
    pzN2KParams = nmea2k_Parameter[nmea2k_u8CanPortToInst[u8CanPort]];

    // find matching tx mailbox
    for (u8Index = 0; u8Index < pzN2KParams->TxMailboxSize; u8Index++)
    {
        // Matching mailbox found? AND
        // valid period requested if enabling periodic transmission
        if ((pzN2KParams->TxMailbox[u8Index].PGN == u32PGN) &&
            (pzN2KParams->TxMailbox[u8Index].Instance == u8Instance))
        {
            // Apply settings
            pzN2KParams->TxMailbox[u8Index].Period = bEnable;
            pzN2KParams->TxMailbox[u8Index].CntrSet = u8Period;

            // Turning periodic transmission off?
            if (!bEnable)
            {
                // Clear pending request
                pzN2KParams->TxMailbox[u8Index].TxReq = NMEA2K_TXREQ_NONE;
            }

            bResult = TRUE;
            break;
        }
    }

    return bResult;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_TxISORequest
//-----------------------------------------------------------------------------
// Description: Send an ISO request for the specified PGN to the specified
//              destination address on the specified CAN port.
//-----------------------------------------------------------------------------
// Return:      TRUE if successful, FALSE otherwise
//-----------------------------------------------------------------------------
bool NMEA2K_TxISORequest(
    uint8 u8CanPort,   // CAN port to use
    uint8 u8DestAddr,  // Destination address. 255 = All
    uint32 u32PGN      // PGN to request
)
{
    bool bResult;

    // Invalid CAN port
    if (u8CanPort >= BSP_CAN_COUNT)
    {
        return FALSE;
    }

    // Find the NMEA2K instance associated with the CAN port and set the instance
    nmea2k_u8Instance = nmea2k_u8CanPortToInst[u8CanPort];

    // Transmit the ISO request
    bResult = Nmea2k_Tx_ISORequest(u8DestAddr, nmea2k_MyAddress[nmea2k_u8Instance], u32PGN);

    return bResult;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_TxSingleFramePGN
//-----------------------------------------------------------------------------
// Description: Transmit a single frame PGN by pushing on the output queue.
//-----------------------------------------------------------------------------
// Return:      TRUE if successful, FALSE otherwise
//-----------------------------------------------------------------------------
bool NMEA2K_TxSingleFramePGN(
    uint8 *pau8Data,   // in : PGN data buffer
    uint8 u8DataSize,  // in : Size of data buffer (Maximum 8)
    uint32 u32PGN,     // in : PGN number
    uint8 u8Priority,  // in : PGN priority ( 0 - 7 )
    uint8 u8SrcAddr,   // in : PGN source address
    uint8 u8DestAddr,  // in : PGN destination address (for PDU formats less than 240)
    uint8 u8CanPort    // in : CAN port to use
)
{
    uint8 u8Index;
    HALCAN_zMSG zHalCanMsg;
    nmea2k_Id_Struct_Type zMsgId;

    // Validate CAN port
    if (u8CanPort >= BSP_CAN_COUNT)
    {
        return FALSE;
    }

    // Compose 29-bit identifier fields
    zMsgId.Identifier = (u32PGN << 8);
    zMsgId.IdField.BitField.Priority = (u8Priority & 0x07);
    zMsgId.IdField.SourceAddress = u8SrcAddr;

    // Destination specific PGN?
    if (zMsgId.IdField.PduFormat < 240)
    {
        // Apply destination address
        zMsgId.IdField.PduSpecific = u8DestAddr;
    }

    // Build message meta data
    zHalCanMsg.u32Id = zMsgId.Identifier;
    zHalCanMsg.eIdType = HALCAN_IDTYPE_EXTENDED;
    zHalCanMsg.u8Length = u8DataSize;
    if (zHalCanMsg.u8Length > 8)
    {
        zHalCanMsg.u8Length = 8;
    }

    // Copy message content accross
    for (u8Index = 0; u8Index < zHalCanMsg.u8Length; u8Index++)
    {
        zHalCanMsg.au8Data[u8Index] = pau8Data[u8Index];
    }

    // TX mailbox message pushed to queue
    return HALCAN_Write(u8CanPort, &zHalCanMsg);
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_ExtractPGN
//-----------------------------------------------------------------------------
// Description: Extract PGN from CAN message ID
// CAN ID    :       P P P R P F F F F F F F F S S S S S S S S A A A A A A A A
// J1939 PGN : 0 0 0 0 0 0 R P F F F F F F F F S S S S S S S S
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
uint32 NMEA2K_ExtractPGN(uint32 u32MsgId)
{

    nmea2k_Id_Struct_Type zMsgId;

    // Get message 29-bit CAN ID
    zMsgId.Identifier = u32MsgId;

    // Remove source address and priority bits
    u32MsgId = (u32MsgId >> 8) & 0x0003FFFF;

    // Destination specific PGN?
    if (zMsgId.IdField.PduFormat <= NMEA2K_MAX_PF_PDU1FORMAT)
    {
        // Remove destination address from PGN
        u32MsgId ^= zMsgId.IdField.PduSpecific;
    }

    return u32MsgId;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_ExtractSA
//-----------------------------------------------------------------------------
// Description: Extract source address from CAN message ID
// CAN ID    :       P P P R P F F F F F F F F S S S S S S S S A A A A A A A A
// J1939 PGN : 0 0 0 0 0 0 R P F F F F F F F F S S S S S S S S
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
uint8 NMEA2K_ExtractSA(uint32 u32MsgId)
{

    return (uint8)u32MsgId;
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_ExtractDA
//-----------------------------------------------------------------------------
// Description: Extract destination address from CAN message ID
// CAN ID    :       P P P R P F F F F F F F F S S S S S S S S A A A A A A A A
// J1939 PGN : 0 0 0 0 0 0 R P F F F F F F F F S S S S S S S S
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
uint8 NMEA2K_ExtractDA(uint32 u32MsgId)
{

    nmea2k_Id_Struct_Type zMsgId;

    // Get message 29-bit CAN ID
    zMsgId.Identifier = u32MsgId;

    // Destination specific PGN?
    if (zMsgId.IdField.PduFormat <= NMEA2K_MAX_PF_PDU1FORMAT)
    {
        // Return destination address
        return zMsgId.IdField.PduSpecific;
    }
    else
    {
        // Not specific - destination is global
        return NMEA2K_GLOBAL_ADDRESS;
    }
}

uint16_t NMEA2K_getFreeRunTimer(uint8 inst)
{
    return nmea2k_FreeRunTmr[inst];
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_IsAddressClaimed
//-----------------------------------------------------------------------------
// Description: Determines if source address of the node has been claimed.
//-----------------------------------------------------------------------------
// Return:      TRUE: Address claimed. FALSE: Address not claimed.
//-----------------------------------------------------------------------------
bool NMEA2K_IsAddressClaimed(uint8 u8CanPort)
{
    bool bAddressClaimed = FALSE;

    // CAN port valid?
    if (u8CanPort < NMEA2K_INSTANCE_COUNT)
    {
        // Get address claiming status
        bAddressClaimed = nmea2k_AddrClaim[nmea2k_u8CanPortToInst[u8CanPort]].AddressClaimed;
    }
    return bAddressClaimed;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_PopulateNAME
//-----------------------------------------------------------------------------
// Description: Populates the address claim message name fields.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_PopulateNAME(
    nmea2k_NAME_Fields_Struct_Type *pzNAME,  // out: Populated name fields.
    const NMEA2K_Parameter_Struct *pParams   // in:  NMEA2K parameter structure
)
{
    uint16 Temp;

    // Build own NAME using the Name Fields define by the application
    pzNAME->Byte8.ArbitraryAddressCapable = pParams->Name_Fields.Self_Cfg_Addr & 0x01;
    pzNAME->Byte8.IndustryGroup = pParams->Name_Fields.Industry_Group & 0x07;
    pzNAME->Byte8.VehicleSystemInstance = pParams->Name_Fields.Vehicle_Sys_Inst & 0x0F;
    pzNAME->Byte7.VehicleSystem = pParams->Name_Fields.Vehicle_System & 0x7F;
    pzNAME->Byte7.Reserved = pParams->Name_Fields.Reserved;
    pzNAME->DeviceFunction = pParams->Name_Fields.Function;
    pzNAME->Byte5.DeviceFunctionInstance = pParams->Name_Fields.Function_Instance & 0x1F;
    pzNAME->Byte5.EcuInstance = pParams->Name_Fields.ECU_Instance & 0x07;
    Temp = pParams->Name_Fields.Manufacturer_Code << 5;
    pzNAME->ManufacturerCode_Byte4 = U16_UPPER_U8(Temp);
    pzNAME->Byte3.ManufacturerCode_Byte3 = U16_LOWER_U8(pParams->Name_Fields.Manufacturer_Code) & 0x07;
    pzNAME->Byte3.IdentityNumber_Byte3 = U16_LOWER_U8(U32_UPPER_U16(pParams->Name_Fields.Identity_Number));
    pzNAME->IdentityNumber_Byte2 = U16_UPPER_U8(U32_LOWER_U16(pParams->Name_Fields.Identity_Number));
    pzNAME->IdentityNumber_Byte1 = U16_LOWER_U8(U32_LOWER_U16(pParams->Name_Fields.Identity_Number));
}

#if CONFIG_RVC_US
/**
 * Address claim request for RVC.
 * It's a special request, different than Nmea2k/ISO standard.
 * CAN-ID:
 * 	18EAxxEE
 * DATA: 3 bytes.
 * 	00EE00
 * @param SA Source Address
 * @return True if request sent successfully.
 */
static uint8 Nmea2k_Tx_RVCAddressClaimRequest(uint8 SA)
{
    rvc_address_claim_request_Struct_Type req_data = {0, 0xEE, 0, {0, 0, 0, 0, 0}};

    nmea2k_SingleFrame_Message_Struct_Type message;
    message.BytePtr = (uint8 *)&req_data;
    message.ByteCntr = sizeof(req_data);

    // Set single frame identifier
    message.MsgId.Identifier = 0xEA << 16;  // NMEA2K_ADDRESS_CLAIM_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_ADDRESS_CLAIM_PRIORITY;
    message.MsgId.IdField.BitField.Reserved = 0;
    //
    message.MsgId.IdField.PduSpecific = SA;
    message.MsgId.IdField.SourceAddress = 0xFE;

    // Push PGN frame into the transmit queue
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        // Success
        return TRUE;
    }

    return FALSE;
}
#endif

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_ISOAddressClaim
//-----------------------------------------------------------------------------
// Description: Transmit the address claim PGN to a specific destination.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Address claim message sent to TX queue successfully.
//              FALSE : Tx queue full, no PGN sent
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Tx_ISOAddressClaim(
    uint8 SA  // in : Source address
)
{
    nmea2k_SingleFrame_Message_Struct_Type message;
    nmea2k_NAME_Fields_Struct_Type MyName;

    // Build own NAME using the Name Fields define by the application
    Nmea2k_PopulateNAME(&MyName, nmea2k_Parameter[nmea2k_u8Instance]);

    // Set single frame data
    message.BytePtr = (uint8 *)&MyName;
    message.ByteCntr = sizeof(nmea2k_NAME_Fields_Struct_Type);

    // Set single frame identifier
    message.MsgId.Identifier = NMEA2K_ADDRESS_CLAIM_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_ADDRESS_CLAIM_PRIORITY;
    message.MsgId.IdField.BitField.Reserved = 0;
    /// FIXED-WQL, RV-C is using 0 in PduSpecific.
#if CONFIG_RVC_US
    message.MsgId.IdField.PduSpecific = 0;  // NMEA2K_GLOBAL_ADDRESS; // Destination always global
#else
    message.MsgId.IdField.PduSpecific = NMEA2K_GLOBAL_ADDRESS;  // Destination always global
#endif
    message.MsgId.IdField.SourceAddress = SA;

    // Push PGN frame into the transmit queue
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        // Success
        return TRUE;
    }

    // Failure
    return FALSE;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_ISOAddressClaim
//-----------------------------------------------------------------------------
// Description: Process a received address claim PGN.
//
//              Address claim variables IWIN and ILOSE are set by this function
//              if we have source addresses contention for more then one
//              network device who claim this address.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_ISOAddressClaim(
    HALCAN_zMSG *MessagePtr  // in : MessagePtr - J1939 message structure pointer
)
{
    nmea2k_Id_Struct_Type *NmeaID = (nmea2k_Id_Struct_Type *)(&MessagePtr->u32Id);
    nmea2k_NAME_Fields_Struct_Type MyName;
    uint8 *MyNamePtr;
    uint8 *RxNamePtr;
    uint8 NameCnt;

    // Build own NAME using the Name Fields define by the application
    Nmea2k_PopulateNAME(&MyName, nmea2k_Parameter[nmea2k_u8Instance]);

    // An address claim message was received
    nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = FALSE;

    // Source address conflict?
    if (NmeaID->IdField.SourceAddress == nmea2k_MyAddress[nmea2k_u8Instance])
    {
        // Set data byte pointers to MSB byte of NAME fields
        // (MSB of a structure have the same alignment in little and big endian)
        RxNamePtr = (uint8 *)(&MessagePtr->au8Data[0]);
        MyNamePtr = (uint8 *)(&MyName);
        RxNamePtr += sizeof(nmea2k_NAME_Fields_Struct_Type);
        MyNamePtr += sizeof(nmea2k_NAME_Fields_Struct_Type);
        NameCnt = sizeof(nmea2k_NAME_Fields_Struct_Type);

        // J1939 address claim NAME field challenge loop...
        while (NameCnt--)
        {
            // Next MSB byte...
            RxNamePtr--;
            MyNamePtr--;

            // MSB byte different?
            if (*MyNamePtr != *RxNamePtr)
            {
                // The lowest value have the highest priority and WIN the challenge
                if (*MyNamePtr < *RxNamePtr)
                {
                    nmea2k_AddrClaim[nmea2k_u8Instance].IWin = TRUE;
                }
                else
                {
                    nmea2k_AddrClaim[nmea2k_u8Instance].ILose = TRUE;
                }

                break;
            }
        }
        if (nmea2k_AddrClaim[nmea2k_u8Instance].IWin ||
            nmea2k_AddrClaim[nmea2k_u8Instance].ILose)
        {
        }
        else
        {
            if (nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed)
            {
                nmea2k_AddrClaim[nmea2k_u8Instance].IWin = TRUE;
                LOG(W, "I already claimed the address, I win");
                return;
            }
            else
            {
                nmea2k_AddrClaim[nmea2k_u8Instance].ILose = TRUE;
                LOG(W, "Not claimed the address yet, I Lose");
                return;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_ISOAcknowledgment
//-----------------------------------------------------------------------------
// Description: Transmit an acknowledgment to the device who sent a specific
//              command or request.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame message sent to Tx queue.
//              FALSE : Tx queue full, no message sent.
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Tx_ISOAcknowledgment(
    uint8 DA,         // in : Destination address (0..255)
    uint8 SA,         // in : Source address (0..251)
    uint32 PGN,       // in : Parameter Group Number requested
    uint8 GroupFnct,  // in : Group function of PGN being acknowledged
    uint8 Status      // in : Acknowledgment status to be transmit (0..3)
)
{
    nmea2k_SingleFrame_Message_Struct_Type message;
    nmea2k_Acknowledgment_Message_Struct_Type ackData;
    nmea2k_Id_Struct_Type idToAck;

    // Set PGN number to be acknowledged
    idToAck.Identifier = PGN << 8;
    idToAck.Identifier &= 0x03FFFF00;

    // Set acknowledgment data bytes
    ackData.AckStatus = Status;
    ackData.GroupFnct = GroupFnct;
    ackData.Byte3 = 0xFF;
    ackData.Byte4 = 0xFF;
    ackData.Byte5 = 0xFF;
    ackData.PduSpecific = idToAck.IdField.PduSpecific;
    ackData.PduFormat = idToAck.IdField.PduFormat;
    ackData.BitField.DataPage = idToAck.IdField.BitField.DataPage;
    ackData.BitField.UnusedMSB = 0;

    // Set data to be transmitted
    message.BytePtr = (uint8 *)&ackData;
    message.ByteCntr = sizeof(ackData);

    // Set 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_ACKNOWLEDGMENT_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_ACKNOWLEDGMENT_PRIORITY;
    message.MsgId.IdField.BitField.Reserved = 0;
    message.MsgId.IdField.SourceAddress = SA;

#if (NMEA2000 == TRUE)
    // Appendix B.1 PGN 59392:
    // The destination address of this PGN shall always contain a destination specific address
    // Note 1: Version 1.000 of the NMEA 2000 Standard required the destination address to be the global address
    message.MsgId.IdField.PduSpecific = DA;
#else
    // SAE J1939-21, section 5.4.4:
    // The global destination address makes it possible to filter on one CAN Identifier for all Acknowledgment messages.
    message.MsgId.IdField.PduSpecific = NMEA2K_GLOBAL_ADDRESS;
#endif

    // Push PGN frame into the transmit queue
    return Nmea2k_Dll_TxSingleFrameMessage(&message);
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_ISOAcknowledgment
//-----------------------------------------------------------------------------
// Description: Receive an acknowledgment from the device we have requested PGN
//              information.
//
//              If the PGN requested is not supported by the device, we will
//              receive a NACK. This is only true if the request was sent to a
//              specific address.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame message sent to Tx queue.
//              FALSE : Tx queue full, no message sent.
//-----------------------------------------------------------------------------
/*
static void Nmea2k_Rx_ISOAcknowledgment(CQU_Msg_Struct *MessagePtr)
{
  // Function not implemented yet.
}
*/

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_ISORequest
//-----------------------------------------------------------------------------
// Description: Request the transmission of a PGN from other network device(s).
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame message sent to Tx queue.
//              FALSE : Tx queue full, no message sent.
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Tx_ISORequest(
    uint8 DA,   // in : Destination address (0..255)
    uint8 SA,   // in : Source address (0..251)
    uint32 PGN  // in : Parameter Group Number being requested
)
{
    nmea2k_SingleFrame_Message_Struct_Type message;
    nmea2k_Request_Message_Struct_Type reqData;
    nmea2k_Id_Struct_Type reqPgn;

    // Set 24-bits PGN number being requested
    reqPgn.Identifier = PGN << 8;
    reqPgn.Identifier &= 0x03FFFF00;

    // Set request data bytes
    reqData.PduSpecific = reqPgn.IdField.PduSpecific;
    reqData.PduFormat = reqPgn.IdField.PduFormat;
    reqData.BitField.DataPage = reqPgn.IdField.BitField.DataPage;
    reqData.BitField.Reserved = reqPgn.IdField.BitField.Reserved;
    reqData.BitField.UnusedMSB = 0;
#if CONFIG_RVC_US
    /// ISO reqeust message has 3 bytes data,
    /// RVC has another 5 bytes, total 8,
    /// initialize all extra RVC fields
    /// byte3 in PGN is the instance.
    reqData.rvcExtra[0] = 0xFF;
    if (((PGN >> 24) & 0xFF) != 0)
    {
        reqData.rvcExtra[0] = (PGN >> 24) & 0xFF;  // 0x05; //ff;
    }
    reqData.rvcExtra[1] = 0xff;
    reqData.rvcExtra[2] = 0xff;
    reqData.rvcExtra[3] = 0xff;
    reqData.rvcExtra[4] = 0xff;
#endif
    // Set data to be transmitted
    message.BytePtr = (uint8 *)&reqData;
    message.ByteCntr = sizeof(reqData);

    // set 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_REQUEST_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_REQUEST_PRIORITY;
    message.MsgId.IdField.BitField.Reserved = 0;
    message.MsgId.IdField.PduSpecific = DA;
    message.MsgId.IdField.SourceAddress = SA;

    // Requesting address claim PGN?
    if (PGN == NMEA2K_ADDRESS_CLAIM_PGN)
    {
        nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = FALSE;
        nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn = TRUE;
        // PGN transmission already supported by "G7_AddressClaim"
        // Do nothing here
    }

    // Global destination address?
    else if (DA == 0xFF)
    {
        // A module has to answer to own requests. so if the
        // PGN is locally supported, set transmit it.
        (void)Nmea2k_Dll_SetTx_nmea2kTxMailbox(PGN, DA);
    }

    // Push PGN frame into the transmit queue
    return Nmea2k_Dll_TxSingleFrameMessage(&message);
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_ISORequest
//-----------------------------------------------------------------------------
// Description: Request the transmission of a PGN from a network specific
//              device or global devices.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_ISORequest(
    HALCAN_zMSG *MessagePtr  // in : MessagePtr - Single frame message structure pointer
)
{
    uint8 u8ReplyDestAddr;

    // Local PGN pointers
    nmea2k_Request_Message_Struct_Type PgnRequest;

    // Local J1939 message ID structure
    union
    {
        nmea2k_Id_Struct_Type IdStruct;
        uint32 PgnNumber;
    } nmea2k;

    // Copy data into PGN Request structure
    (void)memcpy(&PgnRequest, &MessagePtr->au8Data[0], sizeof(PgnRequest));

    // Get the source address of the message requester
    nmea2k.IdStruct.Identifier = MessagePtr->u32Id;
    nmea2k_Answered[nmea2k_u8Instance] = nmea2k.IdStruct.IdField.SourceAddress;

    // Was ISO request destination specific?
    if (nmea2k.IdStruct.IdField.PduSpecific != NMEA2K_GLOBAL_ADDRESS)
    {
        // For multi-packet, reply using RTS/CTS
        u8ReplyDestAddr = nmea2k.IdStruct.IdField.SourceAddress;
    }
    else
    {
        // For multi-packet, reply using BAM
        u8ReplyDestAddr = NMEA2K_GLOBAL_ADDRESS;
    }

    // Extract requested 24-bits PGN number from single frame data field
    nmea2k.IdStruct.IdField.PduSpecific = PgnRequest.PduSpecific;
    nmea2k.IdStruct.IdField.PduFormat = PgnRequest.PduFormat;
    nmea2k.IdStruct.IdField.BitField.DataPage = PgnRequest.BitField.DataPage;
    nmea2k.IdStruct.IdField.BitField.Reserved = PgnRequest.BitField.Reserved;
    nmea2k.IdStruct.IdField.BitField.Priority = 0;
    nmea2k.IdStruct.IdField.BitField.UnusedMSB = 0;
    nmea2k.IdStruct.Identifier >>= 8;

    // Local address already claimed?
    if (nmea2k_AddrClaim[nmea2k_u8Instance].ILose == FALSE)
    {
        // Any supported PGN can be transmitted
        switch (nmea2k.PgnNumber)
        {
        case NMEA2K_ADDRESS_CLAIM_PGN:

            // Request for PGN address claim message
            nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = FALSE;
            nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn = TRUE;
            break;

        case NMEA2K_REQUEST_PGN:
        case NMEA2K_ACKNOWLEDGMENT_PGN:

            // Request for a request, send a Negative Acknowledgment
            Nmea2k_Tx_ISOAcknowledgment(nmea2k_Answered[nmea2k_u8Instance],
                                        nmea2k_MyAddress[nmea2k_u8Instance],
                                        nmea2k.PgnNumber, NMEA2K_GROUP_FUNCTION_VALUE,
                                        NMEA2K_PGN_SUPPORTED_ACCESS_DENIED);
            break;

        default:

            // Try to set TX mailbox flag of requested PGN.
            // Requested PGN not defined in TX Mailbox?
            if (Nmea2k_Dll_SetTx_nmea2kTxMailbox(nmea2k.PgnNumber, u8ReplyDestAddr) == FALSE)
            {
                // Was ISO request destination specific?
                if (u8ReplyDestAddr != NMEA2K_GLOBAL_ADDRESS)
                {
                    // Send a Negative Acknowledgment
                    (void)Nmea2k_Tx_ISOAcknowledgment(nmea2k_Answered[nmea2k_u8Instance],
                                                      nmea2k_MyAddress[nmea2k_u8Instance],
                                                      nmea2k.PgnNumber, NMEA2K_GROUP_FUNCTION_VALUE,
                                                      NMEA2K_NEGATIVE_ACKNOWLEDGMENT);
                }
            }
            break;
        }
    }

    // Address claim PGN requested?
    else if (nmea2k.PgnNumber == NMEA2K_ADDRESS_CLAIM_PGN)
    {
        // Only Address claim PGN can be transmitted if device has not claim an address yet
        nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = FALSE;
        nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn = TRUE;
    }
}

#if NMEA2K_SUPPORT_MULTIPACKET
//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_TP_CM_RTS
//-----------------------------------------------------------------------------
// Description: Reception of a "Request to send Message" sent by the Data
//              Originator.
//
//              Connection Mode Request To Send (TP.CM_RTS): Destination
//              Specific Control Byte = 16. The function check if it's possible
//              to start a Multi-Packet receiving session If Yes, the
//              "MultiPacket" state variable is update and the session is
//              started.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_TP_CM_RTS(
    HALCAN_zMSG *MessagePtr,  // in : MessagePtr - Single frame message structure pointer
    uint8 SA                  // in : SA - Source Address of the Originator
)
{
    // Local Structure use to analyze the informations included in the Data Frame for TP.CM_RTS
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMRTS_Struct
    {
        uint8 Control_Byte;
        uint8 Msg_Size_LSB;
        uint8 Msg_Size_MSB;
        uint8 Packet_Size;
        // Maximum number of Packet that can be sent by the originator
        // before that a CTS was send by the receiver
        uint8 Max_Num_Packet_CTS;
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            bit DataPage : 1;
            bit Reserved : 1;
            bit UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMRTS_Struct_Type;

    uint16 Msg_Size;
    uint8 i, Conn_Abort_Sent = 0;
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *nmea2k_RxMultiPacket_PTR;
    nmea2k_MailBox_ID_Struct_Type PGN_Requested;
    nmea2k_TPCMRTS_Struct_Type TPCMRTS;

    // Copy data into TPCMRTS struct
    (void)memcpy(&TPCMRTS, &MessagePtr->au8Data[0], sizeof(TPCMRTS));

    // Extract requested 24-bits PGN number from Data Field
    PGN_Requested.IdField.Unused = 0;
    PGN_Requested.IdField.PduSpecific = TPCMRTS.PduSpecific;
    PGN_Requested.IdField.PduFormat = TPCMRTS.PduFormat;
    PGN_Requested.IdField.BitField.DataPage = TPCMRTS.BitField.DataPage;
    PGN_Requested.IdField.BitField.Reserved = TPCMRTS.BitField.Reserved;
    PGN_Requested.IdField.BitField.UnusedMSB = 0;

    // For all receive multi-packet sessions...
    for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
    {
        nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];

        if ((nmea2k_RxMultiPacket_PTR->InProgress == 1) && (SA == nmea2k_RxMultiPacket_PTR->Originator) && (PGN_Requested.Identifier != nmea2k_RxMultiPacket_PTR->PGN))
        {
            // It's not possible to open a new Rx Session with this Node
            Conn_Abort_Sent = 1;
            (void)Nmea2k_Tx_TP_Conn_Abort(SA, 1, PGN_Requested.Identifier);
            break;
        }
        else if ((nmea2k_RxMultiPacket_PTR->InProgress == 1) && (SA == nmea2k_RxMultiPacket_PTR->Originator) && (PGN_Requested.Identifier == nmea2k_RxMultiPacket_PTR->PGN))
        {
            // A Rx session for this PGN is already open with this Node, Restart it !!!!
            nmea2k_RxMultiPacket_PTR->InProgress = 0;
            nmea2k_RxMultiPacket_PTR->RxMsgPtr->Rx_InProgress = 0;
            break;
        }
        else
        {
            nmea2k_RxMultiPacket_PTR = NULL;
        }
    }

    // If needed, Try to find a free Receive Multi-packet Session
    if (nmea2k_RxMultiPacket_PTR == NULL)
    {
        // For all receive multi-packet sessions...
        for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
        {
            nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];
            if (nmea2k_RxMultiPacket_PTR->InProgress == 0)
            {
                break;
            }
            else
            {
                nmea2k_RxMultiPacket_PTR = NULL;
            }
        }
        // If no Free session was find, send a Comm Abort
        if (nmea2k_RxMultiPacket_PTR == NULL)
        {
            (void)Nmea2k_Tx_TP_Conn_Abort(SA, 1, PGN_Requested.Identifier);
            Conn_Abort_Sent = 1;
        }
    }

    if (Conn_Abort_Sent == 0)
    {
        // Check if the PGN can be received by the J1939 Receive MailBox
        Msg_Size = TPCMRTS.Msg_Size_LSB;  // Determine the size of the message
        Msg_Size += (uint16)TPCMRTS.Msg_Size_MSB << 8;

        // Try to find a VALID Free J1939 Rx Mailbox for the new Message.
        nmea2k_RxMultiPacket_PTR->RxMsgPtr = Nmea2k_DLL_Check_nmea2kRxMailbox_MultiPacket(PGN_Requested.Identifier, SA);

        if (nmea2k_RxMultiPacket_PTR->RxMsgPtr != NULL)
        {
            // Check if the Truncation is allowed
            if (nmea2k_RxMultiPacket_PTR->RxMsgPtr->Trunk == 1)
            {
                nmea2k_RxMultiPacket_PTR->Trunk_Size = nmea2k_RxMultiPacket_PTR->RxMsgPtr->BufferSize;
            }
            // The Truncation is not allowed, we have to verify that the Mailbox is Big
            // enough to received the message.
            else if (nmea2k_RxMultiPacket_PTR->RxMsgPtr->BufferSize < Msg_Size)
            {
                nmea2k_RxMultiPacket_PTR->RxMsgPtr = NULL;
            }
            else
            {
                nmea2k_RxMultiPacket_PTR->Trunk_Size = Msg_Size;
            }
        }

        if (nmea2k_RxMultiPacket_PTR->RxMsgPtr != NULL)
        {
            nmea2k_RxMultiPacket_PTR->DA = nmea2k_MyAddress[nmea2k_u8Instance];  // The multi-Packet session is destination Specific
            nmea2k_RxMultiPacket_PTR->Originator = SA;                           // Keep track of the Originator Address
            nmea2k_RxMultiPacket_PTR->PGN = PGN_Requested.Identifier;            // Keep track of the PGN that is transmitted
            nmea2k_RxMultiPacket_PTR->Msg_Size = Msg_Size;                       // Keep Track of the message size (Total number of bytes)
            nmea2k_RxMultiPacket_PTR->Packet_Size = TPCMRTS.Packet_Size;         // Keep Track of the number of Packet in the message

            // Define how many packet can be receive before than the receiver send CTS.
            // EX: If the Originator have a lowest capability value => use it
            if (TPCMRTS.Max_Num_Packet_CTS < NMEA2K_MAX_NUMBER_PACKET_BEFOFE_CTS)
            {
                nmea2k_RxMultiPacket_PTR->Max_Num_Packet_CTS = TPCMRTS.Max_Num_Packet_CTS;
            }
            else
            {
                nmea2k_RxMultiPacket_PTR->Max_Num_Packet_CTS = NMEA2K_MAX_NUMBER_PACKET_BEFOFE_CTS;
            }

            nmea2k_RxMultiPacket_PTR->Num_Packet_CTS = nmea2k_RxMultiPacket_PTR->Max_Num_Packet_CTS;
            nmea2k_RxMultiPacket_PTR->Sequence_Number = 1;
            nmea2k_RxMultiPacket_PTR->Num_Byte_Rx = 0;  // Reset the number of bytes received

            nmea2k_RxMultiPacket_PTR->RxMsgPtr->MsgSize = Msg_Size;                             // Set the size of the message in the J1939
            nmea2k_RxMultiPacket_PTR->Byte_Pgm_Ptr = nmea2k_RxMultiPacket_PTR->RxMsgPtr->Data;  // The Byte Programming Pointer should
                                                                                                // need to pointed on the Good J1939 Mailbox
                                                                                                // Element

            if (Nmea2k_Tx_TP_CM_CTS(nmea2k_RxMultiPacket_PTR) == FALSE)  // If it's not possible to send the CTS
            {
                nmea2k_RxMultiPacket_PTR->RxMsgPtr = NULL;  // Don't start the Multi-Packet session
            }
            else
            {
                // The PGN is supported by the J1939 Mailbox, and the CTS was sent
                // finish to initialized Register, prepare the first reception
                nmea2k_RxMultiPacket_PTR->InProgress = 1;  // Multi-Packet session is now in Progress
                nmea2k_RxMultiPacket_PTR->RxMsgPtr->Rx_InProgress = 1;
                nmea2k_RxMultiPacket_PTR->RxMsgPtr->Org_Addr = SA;
                nmea2k_RxMultiPacket_PTR->RxMsgPtr->Dst_Addr = nmea2k_MyAddress[nmea2k_u8Instance];

                // Start a timer of 1.25 S. If the timer elapsed before than a first packet is received
                // The J1939 driver has to send a Connection Abort Message.
                nmea2k_RxMultiPacket_PTR->DelayTimer = 125;
            }
        }
        // The PGN is not supported
        else
        {
            // The PGN is not supported, Send a communication abort with Abort Reason = 255
            (void)Nmea2k_Tx_TP_Conn_Abort(SA, 255, PGN_Requested.Identifier);
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_TP_Conn_Abort
//-----------------------------------------------------------------------------
// Description: Reception of Connection Abort Message sent by the Originator
//              If the Originator wants to Abort the message that is currently
//              processed by the Multi Packet session, the session is stop.
//
//              Connection Abort (TP.Conn_Abort): Destination Specific
//              Control Byte = 255
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_TP_Conn_Abort(
    HALCAN_zMSG *MessagePtr,  // in : Single frame message structure pointer
    uint8 Originator_Addr     // in : Address of the Data Originator
)
{

    // Local Structure use to receive the informations TP.Conn_Abort
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMAbort_Struct
    {
        uint8 Control_Byte;  // Control Byte = 255 for Conn Abort
        uint8 Reason;        // Connection Abort Reason
        uint8 Not_Used1;     // Should be filed with FF
        uint8 Not_Used2;     // Should be filed with FF
        uint8 Not_Used3;     // Should be filed with FF
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            bit DataPage : 1;
            bit Reserved : 1;
            bit UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMAbort_Struct_Type;

    uint8 i;
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *nmea2k_RxMultiPacket_PTR;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;
    nmea2k_TPCMAbort_Struct_Type TPCMAbort;
    nmea2k_MailBox_ID_Struct_Type PGN_Requested;

    // Copy data into TPCMAbort struct
    (void)memcpy(&TPCMAbort, &MessagePtr->au8Data[0], sizeof(TPCMAbort));

    // Extract requested 24-bits PGN number from J1939 Data Field
    PGN_Requested.IdField.Unused = 0;
    PGN_Requested.IdField.PduSpecific = TPCMAbort.PduSpecific;
    PGN_Requested.IdField.PduFormat = TPCMAbort.PduFormat;
    PGN_Requested.IdField.BitField.DataPage = TPCMAbort.BitField.DataPage;
    PGN_Requested.IdField.BitField.Reserved = TPCMAbort.BitField.Reserved;
    PGN_Requested.IdField.BitField.UnusedMSB = 0;

    for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
    {
        nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];

        if ((nmea2k_RxMultiPacket_PTR->InProgress == 1) &&
            (nmea2k_RxMultiPacket_PTR->Originator == Originator_Addr) &&
            (nmea2k_RxMultiPacket_PTR->PGN == PGN_Requested.Identifier))
        {
            nmea2k_RxMultiPacket_PTR->InProgress = 0;  // Stop the Multi-Packet session
            nmea2k_RxMultiPacket_PTR->RxMsgPtr->Rx_InProgress = 0;
        }
    }

    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        TxMultiPacket_PTR = &nmea2k_TxMultiPacket[nmea2k_u8Instance][i];

        if ((TxMultiPacket_PTR->InProgress == 1) &&
            (TxMultiPacket_PTR->DA == Originator_Addr) &&
            (TxMultiPacket_PTR->PGN == PGN_Requested.Identifier) &&
            (TxMultiPacket_PTR->DA != 0xFF))
        {
            TxMultiPacket_PTR->InProgress = 0;
            TxMultiPacket_PTR->TxMsgPtr->TxInProgress = 0;
            TxMultiPacket_PTR->TxMsgPtr->TxReady = FALSE;
            if (l_dgn_tx_ready_cb)
            {
                l_dgn_tx_ready_cb(TxMultiPacket_PTR->PGN, DGN_MP_MSG_TX_READY);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_TP_Conn_Abort
//-----------------------------------------------------------------------------
// Description: Transmission of Connection Abort Message.
//
//              Connection Abort (TP.Conn_Abort): Destination Specific
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Tx_TP_Conn_Abort(
    uint8 Second_Node_Addr,  // in : Address the second node of the MultiPacket session
    uint8 Abort_Reason,      // in : Reason Why Multi-Packet session need to be abort (See J1939-21)
    uint32 PGN               // in : Parameter Group Number of the Multi-Packet session
)
{

    // Local Structure use to send the informations TP.Conn_Abort
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMAbort_Struct
    {
        uint8 Control_Byte;  // Control Byte = 255 for Conn Abort
        uint8 Reason;        // Connection Abort Reason
        uint8 Not_Used1;     // Should be filed with FF
        uint8 Not_Used2;     // Should be filed with FF
        uint8 Not_Used3;     // Should be filed with FF
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMAbort_Struct_Type;

    nmea2k_TPCMAbort_Struct_Type DATA;
    nmea2k_MailBox_ID_Struct_Type *PGN_Number = (nmea2k_MailBox_ID_Struct_Type *)(&PGN);
    nmea2k_SingleFrame_Message_Struct_Type message;

    // Start by Building the DATA Frame
    DATA.Control_Byte = NMEA2K_MULTI_PACKET_TPCONNABORT;
    DATA.Reason = Abort_Reason;
    DATA.Not_Used1 = 0xFF;
    DATA.Not_Used2 = 0xFF;
    DATA.Not_Used3 = 0xFF;
    DATA.PduSpecific = PGN_Number->IdField.PduSpecific;
    DATA.PduFormat = PGN_Number->IdField.PduFormat;
    DATA.BitField.DataPage = PGN_Number->IdField.BitField.DataPage;
    DATA.BitField.Reserved = PGN_Number->IdField.BitField.Reserved;
    DATA.BitField.UnusedMSB = PGN_Number->IdField.BitField.UnusedMSB;

    message.BytePtr = (uint8 *)(&DATA.Control_Byte);
    message.ByteCntr = sizeof(nmea2k_TPCMAbort_Struct_Type);

    // Second, Build the J1939 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_MULTI_PACKET_TPCM_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_MULTI_PACKET_PRIORITY;
    message.MsgId.IdField.PduSpecific = Second_Node_Addr;
    message.MsgId.IdField.SourceAddress = nmea2k_MyAddress[nmea2k_u8Instance];

    // Push PGN frame into the transmit queue
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        // Success
        return TRUE;
    }

    // Fail to transmit PGN frame...
    return FALSE;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_TP_CM_CTS
//-----------------------------------------------------------------------------
// Description: Transmission of CTS message in response to an accepted RTS.
//
//              Connection Abort (TP.CM_CTS): Destination Specific.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Tx_TP_CM_CTS(
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *RxMultiPacket_PTR  // in : Pointer to the current MultiPacket State structure to build the message
)
{

    // Local Structure use to transmit the informations TP.CM_CTS
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMCTS_Struct
    {
        uint8 Control_Byte;        // Control Byte = 17 for CTS
        uint8 Max_Num_Packet_CTS;  // Max number of packet that can be sent
        uint8 Next_Packet_Number;  // Next packet number to be sent
        uint8 Not_Used1;           // Should be filed with FF
        uint8 Not_Used2;           // Should be filed with FF
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMCTS_Struct_Type;

    nmea2k_TPCMCTS_Struct_Type DATA;
    nmea2k_MailBox_ID_Struct_Type *PGN_Number = (nmea2k_MailBox_ID_Struct_Type *)(&(RxMultiPacket_PTR->PGN));
    nmea2k_SingleFrame_Message_Struct_Type message;

    // Start by Building the DATA Frame
    DATA.Control_Byte = NMEA2K_MULTI_PACKET_TPCMCTS;
    DATA.Max_Num_Packet_CTS = RxMultiPacket_PTR->Max_Num_Packet_CTS;
    DATA.Next_Packet_Number = RxMultiPacket_PTR->Sequence_Number;
    DATA.Not_Used1 = 0xFF;
    DATA.Not_Used2 = 0xFF;
    DATA.PduSpecific = PGN_Number->IdField.PduSpecific;
    DATA.PduFormat = PGN_Number->IdField.PduFormat;
    DATA.BitField.DataPage = PGN_Number->IdField.BitField.DataPage;
    DATA.BitField.Reserved = PGN_Number->IdField.BitField.Reserved;
    DATA.BitField.UnusedMSB = PGN_Number->IdField.BitField.UnusedMSB;

    message.BytePtr = (uint8 *)(&DATA.Control_Byte);
    message.ByteCntr = sizeof(nmea2k_TPCMCTS_Struct_Type);

    // Second, Build the J1939 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_MULTI_PACKET_TPCM_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_MULTI_PACKET_PRIORITY;
    message.MsgId.IdField.PduSpecific = RxMultiPacket_PTR->Originator;
    message.MsgId.IdField.SourceAddress = RxMultiPacket_PTR->DA;

    // Push PGN frame into the transmit queue
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        // Success
        return TRUE;
    }

    // Fail to transmit PGN frame...
    return FALSE;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_TP_CM_EndOfMsgACK
//-----------------------------------------------------------------------------
// Description: Transmission of CTS message in response to a good RTS.
//
//              End Of Message Acknowledgment (TP.CM_EndOfMsgAck) :
//              Destination Specific. Control Byte = 19
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Tx_TP_CM_EndOfMsgACK(
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *RxMultiPacket_PTR  // Pointer to the current MultiPacket State structure to build the message
)
{
    // Local Structure use to send the informations TP.CM.EndOfMsgAck
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMAck_Struct
    {
        uint8 Control_Byte;  // Control Byte = 19 for End of Msg Ack
        uint8 Msg_Size_LSB;
        uint8 Msg_Size_MSB;
        uint8 Packet_Size;
        uint8 Not_Used1;  // Should be filed with FF
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMAck_Struct_Type;

    nmea2k_TPCMAck_Struct_Type DATA;
    nmea2k_MailBox_ID_Struct_Type *PGN_Number = (nmea2k_MailBox_ID_Struct_Type *)(&(RxMultiPacket_PTR->PGN));
    nmea2k_SingleFrame_Message_Struct_Type message;

    // Start by Building the DATA Frame
    DATA.Control_Byte = NMEA2K_MULTI_PACKET_ENOFMSGACK;
    DATA.Msg_Size_LSB = U16_LOWER_U8(RxMultiPacket_PTR->Msg_Size);
    DATA.Msg_Size_MSB = U16_UPPER_U8(RxMultiPacket_PTR->Msg_Size);
    DATA.Packet_Size = RxMultiPacket_PTR->Packet_Size;
    DATA.Not_Used1 = 0xFF;
    DATA.PduSpecific = PGN_Number->IdField.PduSpecific;
    DATA.PduFormat = PGN_Number->IdField.PduFormat;
    DATA.BitField.DataPage = PGN_Number->IdField.BitField.DataPage;
    DATA.BitField.Reserved = PGN_Number->IdField.BitField.Reserved;
    DATA.BitField.UnusedMSB = PGN_Number->IdField.BitField.UnusedMSB;

    message.BytePtr = (uint8 *)(&DATA.Control_Byte);
    message.ByteCntr = sizeof(nmea2k_TPCMAck_Struct_Type);

    // Second, Build the J1939 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_MULTI_PACKET_TPCM_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_MULTI_PACKET_PRIORITY;
    message.MsgId.IdField.PduSpecific = RxMultiPacket_PTR->Originator;
    message.MsgId.IdField.SourceAddress = RxMultiPacket_PTR->DA;

    // Push PGN frame into the transmit queue
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        // Success
        return TRUE;
    }

    // Fail to transmit PGN frame...
    return FALSE;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_TP_CM_BAM
//-----------------------------------------------------------------------------
// Description: Reception of Broadcast Announce Message sent by the Originator.
//
//              Broadcast Announcement Message (TP.CM_BAM): Global Destination
//              Control Byte = 32
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_TP_CM_BAM(
    HALCAN_zMSG *MessagePtr,  // in : Single frame message structure pointer
    uint8 Originator_Address  // in : Source Address of the Originator
)
{

    // Local Structure use to analyse the informations included in the Data Frame for TP.CM_BAM
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMBAM_Struct
    {
        uint8 Control_Byte;
        uint8 Msg_Size_LSB;
        uint8 Msg_Size_MSB;
        uint8 Packet_Size;
        uint8 Not_Used1;  // Should be loaded with FF
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            bit DataPage : 1;
            bit Reserved : 1;
            bit UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMBAM_Struct_Type;

    uint16 Msg_Size;
    uint8 i, Session_Abort = 0;
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *nmea2k_RxMultiPacket_PTR;
    nmea2k_MailBox_ID_Struct_Type PGN_Requested;
    nmea2k_TPCMBAM_Struct_Type TPCMBAM;

    // Copy data into TPCMBAM struct
    (void)memcpy(&TPCMBAM, &MessagePtr->au8Data[0], sizeof(TPCMBAM));

    // Extract requested 24-bits PGN number from J1939 Data Field
    PGN_Requested.IdField.Unused = 0;
    PGN_Requested.IdField.PduSpecific = TPCMBAM.PduSpecific;
    PGN_Requested.IdField.PduFormat = TPCMBAM.PduFormat;
    PGN_Requested.IdField.BitField.DataPage = TPCMBAM.BitField.DataPage;
    PGN_Requested.IdField.BitField.Reserved = TPCMBAM.BitField.Reserved;
    PGN_Requested.IdField.BitField.UnusedMSB = 0;

    uint32_t pgn;
    /* If proprietary DGN is received, remove the DA address */
    if (PGN_Requested.IdField.PduFormat == 0xEF)
    {
        pgn = PGN_Requested.Identifier & 0x3ff00;
        PropDGN_DA = PGN_Requested.IdField.PduSpecific;
    }
    else
    {
        pgn = PGN_Requested.Identifier & 0x3ffff;  // (uint32_t)TPCMBAM.PduSpecific | (uint32_t)(TPCMBAM.PduFormat<<8) | (uint32_t)(TPCMBAM.BitField.DataPage<<16);
    }

    for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
    {
        nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];

        if ((nmea2k_RxMultiPacket_PTR->InProgress == 1) &&
            (Originator_Address == nmea2k_RxMultiPacket_PTR->Originator))
        {
            // The Data Originator wants to start a Broadcast Multi-packet session
            // But one session is already Open. Stop the current session.
            nmea2k_RxMultiPacket_PTR->RxMsgPtr->Rx_InProgress = 0;
            nmea2k_RxMultiPacket_PTR->InProgress = 0;
            break;
        }
        else
        {
            nmea2k_RxMultiPacket_PTR = NULL;
        }
    }

    // If needed, Try to find a free Receive Multi-packet Session
    if (nmea2k_RxMultiPacket_PTR == NULL)
    {
        for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
        {
            nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];
            if (nmea2k_RxMultiPacket_PTR->InProgress == 0)
            {
                break;
            }
            else
            {
                nmea2k_RxMultiPacket_PTR = NULL;
            }
        }
    }

    // If the session doesn't need to be abort
    if ((nmea2k_RxMultiPacket_PTR != NULL) && (Session_Abort == 0))
    {
        // Check if the PGN can be received by the J1939 Receive MailBox
        Msg_Size = TPCMBAM.Msg_Size_LSB;  // Determine the size of the message
        Msg_Size += (uint16)TPCMBAM.Msg_Size_MSB << 8;

        // Try to find a VALID Free J1939 Rx Mailbox for the new Message.
        nmea2k_RxMultiPacket_PTR->RxMsgPtr = Nmea2k_DLL_Check_nmea2kRxMailbox_MultiPacket(pgn, Originator_Address);

        if (nmea2k_RxMultiPacket_PTR->RxMsgPtr != NULL)
        {
            // Check if the Truncation is allowed
            if (nmea2k_RxMultiPacket_PTR->RxMsgPtr->Trunk == 1)
            {
                nmea2k_RxMultiPacket_PTR->Trunk_Size = nmea2k_RxMultiPacket_PTR->RxMsgPtr->BufferSize;
            }
            // The Truncation is not allowed, we have to verify that the Mailbox is Big
            // enough to received the message.
            else if (nmea2k_RxMultiPacket_PTR->RxMsgPtr->BufferSize < Msg_Size)
            {
                nmea2k_RxMultiPacket_PTR->RxMsgPtr = NULL;
            }
            else
            {
                nmea2k_RxMultiPacket_PTR->Trunk_Size = Msg_Size;
            }
        }

        if (nmea2k_RxMultiPacket_PTR->RxMsgPtr != NULL)
        {
            // The PGN is supported by the J1939 Mailbox

            // initialized Register, prepare the first reception
            // and send the CTS

            nmea2k_RxMultiPacket_PTR->InProgress = 1;                     // Multi-Packet session is now in Progress
            nmea2k_RxMultiPacket_PTR->DA = 255;                           // The multi-Packet session is Global
            nmea2k_RxMultiPacket_PTR->Originator = Originator_Address;    // Keep track of the Originator Address
            nmea2k_RxMultiPacket_PTR->PGN = pgn;                          // Keep track of the PGN that is transmitted
            nmea2k_RxMultiPacket_PTR->Msg_Size = Msg_Size;                // Keep Track of the message size (Total number of bytes)
            nmea2k_RxMultiPacket_PTR->Packet_Size = TPCMBAM.Packet_Size;  // Keep Track of the number of Packet in the message

            nmea2k_RxMultiPacket_PTR->Sequence_Number = 1;
            nmea2k_RxMultiPacket_PTR->Num_Byte_Rx = 0;  // Reset the number of bytes received

            nmea2k_RxMultiPacket_PTR->RxMsgPtr->Rx_InProgress = 1;
            nmea2k_RxMultiPacket_PTR->RxMsgPtr->Org_Addr = Originator_Address;
            nmea2k_RxMultiPacket_PTR->RxMsgPtr->Dst_Addr = 0xFF;

            nmea2k_RxMultiPacket_PTR->RxMsgPtr->MsgSize = Msg_Size;                             // Set the size of the message in the J1939
            nmea2k_RxMultiPacket_PTR->Byte_Pgm_Ptr = nmea2k_RxMultiPacket_PTR->RxMsgPtr->Data;  // The Byte Programming Pointer should
                                                                                                // need to pointed on the Good J1939 Mailbox
                                                                                                // Element

            // Start a timer of 750 mS. If the timer elapsed before
            // than a first packet is received, the session is abort.
            nmea2k_RxMultiPacket_PTR->DelayTimer = 75;
        }
    }
    if (l_mp_dgn_raw_rx_cb)
    {
        l_mp_dgn_raw_rx_cb(MessagePtr->u32Id, MessagePtr->u8Length, MessagePtr->au8Data);
    }
}

#if TEST_FIX_COTEK_4_5_PID
/// COTAK sends only 4 packets for PD, byest
static const uint8_t TOK_FINAL_PACKET[] = {0x04, 0x31, 0x2a, 0x2a, 0xFF, 0xFF, 0xFF, 0xFF};
static bool patch_isTokFinalPID(uint8_t *p_data, nmea2k_RxMultiPacket_State_Variables_Struct_Type *p_rx_packet)
{
    if (p_rx_packet->Packet_Size == 5 && p_rx_packet->Sequence_Number == 4)
    {
        // LOG(W, "Got data:");
        for (int i = 0; i < 8; i++)
        {
            if (p_data[i] != TOK_FINAL_PACKET[i])
            {
                return false;
            }
            // printf("%x ", p_data[i]);
        }
        // printf("\n");
        return true;
    }
    return false;
}
#endif

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_DataTransfert
//-----------------------------------------------------------------------------
// Description: Reception of a Multi-Packet DATA.
//
//              Transport Protocol - Data Transfert (TP.DT).
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_DataTransfert(
    HALCAN_zMSG *MessagePtr,  // in : Single frame message structure pointer
    uint8 Originator_Addr,    // in : Address of the Data Originator
    uint8 Dest_Addr           // in : Destination address of the Data, this parameter is needed
    )                         //      in order to process Destination Specific and Global communication
{
    // Local Structure use to receive the informations TP.DT
    // See J1939-21 for reference
    typedef struct nmea2k_TPDT_Struct
    {
        uint8 SequenceNumber;
        uint8 DATA[7];
    } nmea2k_TPDT_Struct_Type;

    uint8 i, j;
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *nmea2k_RxMultiPacket_PTR;
    nmea2k_TPDT_Struct_Type TPDT;

    // Copy message into TPDT
    (void)memcpy(&TPDT, &MessagePtr->au8Data[0], sizeof(nmea2k_TPDT_Struct_Type));

    // Find the corresponding Multi-Packet session
    for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
    {
        nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];
        // Check if a Multi Packet session is in Progress AND if this Data are related to the current session
        if ((nmea2k_RxMultiPacket_PTR->InProgress == 1) && (nmea2k_RxMultiPacket_PTR->Originator == Originator_Addr) && (nmea2k_RxMultiPacket_PTR->DA == Dest_Addr))
        {
            break;
        }
        else
        {
            nmea2k_RxMultiPacket_PTR = NULL;
        }
    }

    if (nmea2k_RxMultiPacket_PTR != NULL)
    {
        // Check if this Packet correspond to the one that is waited by the MP session
        if (nmea2k_RxMultiPacket_PTR->Sequence_Number == TPDT.SequenceNumber)
        {
            // Check if it's the last Packet of Data
            if (nmea2k_RxMultiPacket_PTR->Sequence_Number == nmea2k_RxMultiPacket_PTR->Packet_Size
#if TEST_FIX_COTEK_4_5_PID
                || patch_isTokFinalPID(MessagePtr->au8Data, nmea2k_RxMultiPacket_PTR)
#endif
            )
            {
                j = 0;
                for (i = nmea2k_RxMultiPacket_PTR->Num_Byte_Rx; i < nmea2k_RxMultiPacket_PTR->Msg_Size; i++)  // Copy the remaining byte in the Memory
                {
                    if (nmea2k_RxMultiPacket_PTR->Num_Byte_Rx < nmea2k_RxMultiPacket_PTR->Trunk_Size)
                    {
                        *(nmea2k_RxMultiPacket_PTR->Byte_Pgm_Ptr) = TPDT.DATA[j];
                    }
                    nmea2k_RxMultiPacket_PTR->Byte_Pgm_Ptr++;
                    nmea2k_RxMultiPacket_PTR->Num_Byte_Rx++;
                    j++;
                }
                if (nmea2k_RxMultiPacket_PTR->DA != 255)  // If the session is destination specific
                {
                    (void)Nmea2k_Tx_TP_CM_EndOfMsgACK(nmea2k_RxMultiPacket_PTR);  // Send the End of Message Acknowledgement
                }

                // Message reception complete
                nmea2k_RxMultiPacket_PTR->RxMsgPtr->Rx_InProgress = 0;
                nmea2k_RxMultiPacket_PTR->RxMsgPtr->Ageing_Ctr = 0;  // Refresh the Ageing Counter
                nmea2k_RxMultiPacket_PTR->InProgress = 0;

                // Call the reception handler if it exists
                if (nmea2k_RxMultiPacket_PTR->RxMsgPtr->Handle != NULL)
                {
                    nmea2k_RxMultiPacket_PTR->RxMsgPtr->Handle(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, nmea2k_RxMultiPacket_PTR->RxMsgPtr);
                }
            }
            // This Packet is not the last One
            else
            {
                for (i = 0; i < 7; i++)  // Copy the 7 Byte of Data in the Memory
                {
                    if (nmea2k_RxMultiPacket_PTR->Num_Byte_Rx < nmea2k_RxMultiPacket_PTR->Trunk_Size)
                    {
                        *(nmea2k_RxMultiPacket_PTR->Byte_Pgm_Ptr) = TPDT.DATA[i];
                    }
                    nmea2k_RxMultiPacket_PTR->Byte_Pgm_Ptr++;  // Increment the Data Pointer
                    nmea2k_RxMultiPacket_PTR->Num_Byte_Rx++;   // Increment the number of byte received
                }
                nmea2k_RxMultiPacket_PTR->Sequence_Number++;  // Increment the sequence number to be able to receive next packet

                nmea2k_RxMultiPacket_PTR->DelayTimer = 75;  // Time Out before new reception = 750 mS
                if (nmea2k_RxMultiPacket_PTR->DA != 255)    // If the message is not Global, check if it's the time to transmit CTS
                {
                    if (--nmea2k_RxMultiPacket_PTR->Num_Packet_CTS == 0)
                    {
                        // It's the time to transmit a CTS
                        nmea2k_RxMultiPacket_PTR->Num_Packet_CTS = nmea2k_RxMultiPacket_PTR->Max_Num_Packet_CTS;
                        (void)Nmea2k_Tx_TP_CM_CTS(nmea2k_RxMultiPacket_PTR);
                        nmea2k_RxMultiPacket_PTR->DelayTimer = 125;  // Time Out before new reception = 1250 mS
                    }
                }
            }
        }
        if (l_mp_dgn_raw_rx_cb)
        {
            l_mp_dgn_raw_rx_cb(MessagePtr->u32Id, MessagePtr->u8Length, MessagePtr->au8Data);
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_TP_CM_RTS
//-----------------------------------------------------------------------------
// Description: Transmission of a RTS to start a Tx MultiPacket session.
//
//              Transport Protocol - Data Transfert (TP.DT).
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent
//-----------------------------------------------------------------------------
static bool Nmea2k_Tx_TP_CM_RTS(
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR  // Pointer to the current MultiPacket State structure to build the message
)
{

    // Local Structure use to send the informations TP.CM_RTS
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMAbort_Struct
    {
        uint8 Control_Byte;  // Control Byte = 16 for RTS
        uint8 Msg_Size_LSB;  // Message Size (2 Bytes)
        uint8 Msg_Size_MSB;
        uint8 Num_Packet;          // Total Number of packet
        uint8 Max_Num_Packet_CTS;  // Max number of packet that can be sent
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMRTS_Struct_Type;

    nmea2k_TPCMRTS_Struct_Type DATA;
    nmea2k_MailBox_ID_Struct_Type *PGN_Number = (nmea2k_MailBox_ID_Struct_Type *)(&(TxMultiPacket_PTR->PGN));
    nmea2k_SingleFrame_Message_Struct_Type message;

    // Start by Building the DATA Frame
    DATA.Control_Byte = NMEA2K_MULTI_PACKET_TPCMRTS;
    DATA.Msg_Size_LSB = U16_LOWER_U8(TxMultiPacket_PTR->Msg_Size);
    DATA.Msg_Size_MSB = U16_UPPER_U8(TxMultiPacket_PTR->Msg_Size);
    DATA.Num_Packet = TxMultiPacket_PTR->Packet_Size;
    DATA.Max_Num_Packet_CTS = TxMultiPacket_PTR->Max_Num_Packet_CTS;

    DATA.PduSpecific = PGN_Number->IdField.PduSpecific;
    DATA.PduFormat = PGN_Number->IdField.PduFormat;
    DATA.BitField.DataPage = PGN_Number->IdField.BitField.DataPage;
    DATA.BitField.Reserved = PGN_Number->IdField.BitField.Reserved;
    DATA.BitField.UnusedMSB = PGN_Number->IdField.BitField.UnusedMSB;

    message.BytePtr = (uint8 *)(&DATA.Control_Byte);
    message.ByteCntr = sizeof(nmea2k_TPCMRTS_Struct_Type);

    // Second, Build the J1939 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_MULTI_PACKET_TPCM_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_MULTI_PACKET_PRIORITY;
    message.MsgId.IdField.PduSpecific = TxMultiPacket_PTR->DA;
    message.MsgId.IdField.SourceAddress = TxMultiPacket_PTR->Originator;

    // Push PGN frame into the transmit queue
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        // Success
        return TRUE;
    }

    // Fail to transmit PGN frame...
    return FALSE;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_TP_CM_BAM
//-----------------------------------------------------------------------------
// Description: Transmission of a BAM to start a Global Tx MultiPacket
//              session.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent
//-----------------------------------------------------------------------------
static bool Nmea2k_Tx_TP_CM_BAM(
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR  // Pointer to the current MultiPacket State structure to build the message
)
{

    // Local Structure use to send the informations TP.CM_BAM
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMAbort_Struct
    {
        uint8 Control_Byte;  // Control Byte = 32 for BAM
        uint8 Msg_Size_LSB;  // Message Size (2 Bytes)
        uint8 Msg_Size_MSB;
        uint8 Num_Packet;  // Total Number of packet
        uint8 Unused;      // Unused
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            uint8 DataPage : 1;
            uint8 Reserved : 1;
            uint8 UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMBAM_Struct_Type;

    nmea2k_TPCMBAM_Struct_Type DATA;
    nmea2k_MailBox_ID_Struct_Type *PGN_Number = (nmea2k_MailBox_ID_Struct_Type *)(&(TxMultiPacket_PTR->PGN));
    nmea2k_SingleFrame_Message_Struct_Type message;

    // Start by Building the DATA Frame
    DATA.Control_Byte = NMEA2K_MULTI_PACKET_TPCMBAM;
    DATA.Msg_Size_LSB = U16_LOWER_U8(TxMultiPacket_PTR->Msg_Size);
    DATA.Msg_Size_MSB = U16_UPPER_U8(TxMultiPacket_PTR->Msg_Size);
    DATA.Num_Packet = TxMultiPacket_PTR->Packet_Size;
    DATA.Unused = 0xFF;

    /* If a proprietary DGN needs to be sent over a BAM, include the DA in the DGN */
    if (TxMultiPacket_PTR->PGN == 0xEF00)
    {
        DATA.PduSpecific = TxMultiPacket_PTR->DA;  // TODO Is this correct RVC_DESTINATION_ADDRESS;
        DATA.PduFormat = PGN_Number->IdField.PduFormat;
    }
    else
    {
        DATA.PduSpecific = PGN_Number->IdField.PduSpecific;
        DATA.PduFormat = PGN_Number->IdField.PduFormat;
    }
    DATA.BitField.DataPage = PGN_Number->IdField.BitField.DataPage;
    DATA.BitField.Reserved = PGN_Number->IdField.BitField.Reserved;
    DATA.BitField.UnusedMSB = PGN_Number->IdField.BitField.UnusedMSB;

    message.BytePtr = (uint8 *)(&DATA.Control_Byte);
    message.ByteCntr = sizeof(nmea2k_TPCMBAM_Struct_Type);

    // Second, Build the J1939 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_MULTI_PACKET_TPCM_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_MULTI_PACKET_PRIORITY;
    message.MsgId.IdField.PduSpecific = TxMultiPacket_PTR->DA;  // Should be 0xFF
    message.MsgId.IdField.SourceAddress = TxMultiPacket_PTR->Originator;

    // Push PGN frame into the transmit queue
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        // Success
        return TRUE;
    }

    // Fail to transmit PGN frame...
    return FALSE;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_TP_CM_CTS
//-----------------------------------------------------------------------------
// Description: Reception of a CTS probably in response to a RTS.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_TP_CM_CTS(
    HALCAN_zMSG *MessagePtr,  // in : Single frame message structure pointer
    uint8 Originator_Addr     // in : Address of the CTS Originator
)
{

    // Local Structure use to receive the informations TP.CM_CTS
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMCTS_Struct
    {
        uint8 Control_Byte;        // Control Byte = 17 for CTS
        uint8 Max_Num_Packet_CTS;  // Max number of packet that can be sent
        uint8 Next_Packet_Number;  // Next packet number to be sent
        uint8 Not_Used1;           // Should be filed with FF
        uint8 Not_Used2;           // Should be filed with FF
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            bit DataPage : 1;
            bit Reserved : 1;
            bit UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMCTS_Struct_Type;

    uint8 i;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;
    nmea2k_TPCMCTS_Struct_Type TPCMCTS;
    nmea2k_MailBox_ID_Struct_Type PGN_Requested;

    // Copy data into TPCMCTS struct
    (void)memcpy(&TPCMCTS, &MessagePtr->au8Data[0], sizeof(TPCMCTS));

    // Extract requested 24-bits PGN number from J1939 Data Field
    PGN_Requested.IdField.Unused = 0;
    PGN_Requested.IdField.PduSpecific = TPCMCTS.PduSpecific;
    PGN_Requested.IdField.PduFormat = TPCMCTS.PduFormat;
    PGN_Requested.IdField.BitField.DataPage = TPCMCTS.BitField.DataPage;
    PGN_Requested.IdField.BitField.Reserved = TPCMCTS.BitField.Reserved;
    PGN_Requested.IdField.BitField.UnusedMSB = 0;

    // Check which Tx Mailbox is corresponding to this CTS
    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        TxMultiPacket_PTR = &(nmea2k_TxMultiPacket[nmea2k_u8Instance][i]);
        if ((TxMultiPacket_PTR->InProgress == 1) &&
            (PGN_Requested.Identifier == TxMultiPacket_PTR->PGN) &&
            (Originator_Addr == TxMultiPacket_PTR->DA) && (Originator_Addr != 255))
        {
            break;
        }
        else
        {
            TxMultiPacket_PTR = NULL;
        }
    }

    if (TxMultiPacket_PTR != NULL)
    {
        if (TPCMCTS.Next_Packet_Number == 0)
        {
            // If (Next Packet Number == 0), The Responder wants to hold the connection open
            // but cannot receive any packet right now. A maximum of 500 mS later, the responder
            // should an other CTS to keep the connection Hold or to ask for an others packet.
            // NextTxTimer should be less than DelayTimer
            // DelayTimer timeout will kill the transfer.
            TxMultiPacket_PTR->DelayTimer = 50;   // Session Abort scheduled in 500mS
            TxMultiPacket_PTR->NextTxTimer = 60;  // Should be higher to avoid Data Transfert
            // Don't update the sequence number it's useless
            TxMultiPacket_PTR->Max_Num_Packet_CTS = TPCMCTS.Max_Num_Packet_CTS;  // This value is determined by the Reception Node
            TxMultiPacket_PTR->Num_Packet_CTS = TPCMCTS.Max_Num_Packet_CTS;
        }
        else
        {
            TxMultiPacket_PTR->DelayTimer = 20;                                  // Next Packet needs to be sent < 200 mS
            TxMultiPacket_PTR->NextTxTimer = 5;                                  // Next Tx scheduled in 50 mS
            TxMultiPacket_PTR->Sequence_Number = TPCMCTS.Next_Packet_Number;     // This value is determined by the Reception Node
            TxMultiPacket_PTR->Max_Num_Packet_CTS = TPCMCTS.Max_Num_Packet_CTS;  // This value is determined by the Reception Node
            TxMultiPacket_PTR->Num_Packet_CTS = TPCMCTS.Max_Num_Packet_CTS;
        }
    }
    // Else don't do anything
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Tx_DataTransfert
//-----------------------------------------------------------------------------
// Description: Transmission of Packet of Data.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent.
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Tx_DataTransfert(nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR)  // Pointer to the current MultiPacket State structure to build the message
{
    // Local Structure use to Transmit the informations TP.DT
    // See J1939-21 for reference
    typedef struct nmea2k_TPDT_Struct
    {
        uint8 SequenceNumber;
        uint8 Data[7];
    } nmea2k_TPDT_Struct_Type;

    nmea2k_TPDT_Struct_Type DATA;
    uint8 i;
    uint16 Num_Byte_Tx;
    uint8 *Data_Ptr;                                 // Local Pointer to the Data that needs to be transmitted
    nmea2k_SingleFrame_Message_Struct_Type message;  // Local J1939 single frame message structure

    // Determine the Data Pointer Position
    Data_Ptr = TxMultiPacket_PTR->Data_Pgm_Ptr;
    Data_Ptr += ((TxMultiPacket_PTR->Sequence_Number - 1) * 7);

    // Fill the Message Data
    DATA.SequenceNumber = TxMultiPacket_PTR->Sequence_Number;
    // Check if it's the last packet of Data
    if (TxMultiPacket_PTR->Sequence_Number == TxMultiPacket_PTR->Packet_Size)
    {
        // It's the last Packet of Data !!!!
        Num_Byte_Tx = ((TxMultiPacket_PTR->Sequence_Number - 1) * 7);
        for (i = 0; i < 7; i++)
        {
            if (Num_Byte_Tx < TxMultiPacket_PTR->Msg_Size)
            {
                DATA.Data[i] = *Data_Ptr;
                Num_Byte_Tx++;
                Data_Ptr++;
            }
            else
            {
                DATA.Data[i] = 0xFF;  // Fill the remaining Bytes with 0xFF
            }
        }
    }
    else
    {
        // It's not the last Packet of Data !!!
        for (i = 0; i < 7; i++)
        {
            DATA.Data[i] = *Data_Ptr;
            Data_Ptr++;
        }
    }

    message.BytePtr = (uint8 *)(&DATA.SequenceNumber);
    message.ByteCntr = sizeof(nmea2k_TPDT_Struct_Type);

    // Build the J1939 29-bit identifier fields
    message.MsgId.Identifier = NMEA2K_MULTI_PACKET_TPDT_PGN << 8;
    message.MsgId.IdField.BitField.Priority = NMEA2K_MULTI_PACKET_PRIORITY;
    message.MsgId.IdField.PduSpecific = TxMultiPacket_PTR->DA;
    message.MsgId.IdField.SourceAddress = TxMultiPacket_PTR->Originator;

    // Send the Packet on the CAN Network. If the Message is not placed on the Queue CAN
    // don't do anything. The driver will try to send it again at the next Tread
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)  // Send the Data Transfert Message
    {
        // Check if the Message is Global
        if (TxMultiPacket_PTR->DA == 255)
        {
            // Message is Global, Send the next Packet in 100 mS, don't wait for CTS
            TxMultiPacket_PTR->DelayTimer = 20;                                         // Tx Time Out set at 200mS
            TxMultiPacket_PTR->NextTxTimer = 5;                                         // Next Transmission schedule in 50 mS
            TxMultiPacket_PTR->Num_Packet_CTS = TxMultiPacket_PTR->Max_Num_Packet_CTS;  // Reception of CTS not needed, Set at Maximum
            if (TxMultiPacket_PTR->Sequence_Number >= TxMultiPacket_PTR->Packet_Size)   // Check if it's the last Packet
            {
                TxMultiPacket_PTR->InProgress = 0;
                if (TxMultiPacket_PTR->TxMsgPtr != NULL)
                {
                    TxMultiPacket_PTR->TxMsgPtr->TxInProgress = FALSE;
                    TxMultiPacket_PTR->TxMsgPtr->TxReady = FALSE;
                    TxMultiPacket_PTR->TxMsgPtr = NULL;
                    if (l_dgn_tx_ready_cb)
                    {
                        l_dgn_tx_ready_cb(TxMultiPacket_PTR->PGN, DGN_MP_MSG_TX_READY);
                    }
                }
            }
            TxMultiPacket_PTR->Sequence_Number++;  // Increase the sequence Number for next Packet
        }
        else
        {
            // The message is destination Specific !!!
            // Check if we have to wait for a CTS, send New Data or wait for End Of Msg Ack
            if (--TxMultiPacket_PTR->Num_Packet_CTS == 0)
            {
                // Yes we have to wait for a CTS before sending new Message
                TxMultiPacket_PTR->DelayTimer = 125;   // Delay maximum to receive a CTS
                TxMultiPacket_PTR->NextTxTimer = 135;  // Need to be set Higher to avoid transmission
                // Don't update the sequence Number, it's useless
                TxMultiPacket_PTR->Num_Packet_CTS = TxMultiPacket_PTR->Max_Num_Packet_CTS;
            }
            else if (TxMultiPacket_PTR->Sequence_Number == TxMultiPacket_PTR->Packet_Size)
            {
                // The last Packet of Data was Sent, Set the Timing to Wait for End of Msg Ack
                TxMultiPacket_PTR->DelayTimer = 125;   // Delay maximum to receive a CTS
                TxMultiPacket_PTR->NextTxTimer = 135;  // Need to be set Higher to avoid transmission
                TxMultiPacket_PTR->Sequence_Number++;
            }
            else
            {
                // CTS or End Of Msg Ack are not Needed => Prepare the Next Data Transfert
                TxMultiPacket_PTR->DelayTimer = 20;  // Time Out need to be Higher that NextTxTimer
                TxMultiPacket_PTR->NextTxTimer = 5;  // Next Transmission schedule in 150 mS
                TxMultiPacket_PTR->Sequence_Number++;
            }
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Rx_TP_CM_EndOfMsgACK
//-----------------------------------------------------------------------------
// Description: Reception of a End of Message acknowledge.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Rx_TP_CM_EndOfMsgACK(
    HALCAN_zMSG *MessagePtr,  // in : Single frame message structure pointer
    uint8 Originator_Addr     // in : Address of the End Of Msg ACK Originator
)
{

    // Local Structure use to send the informations TP.CM.EndOfMsgAck
    // This structure is used to simplify the Byte manipulation
    // See J1939-21 for reference
    typedef struct nmea2k_TPCMAck_Struct
    {
        uint8 Control_Byte;  // Control Byte = 19 for End of Msg Ack
        uint8 Msg_Size_LSB;
        uint8 Msg_Size_MSB;
        uint8 Packet_Size;
        uint8 Not_Used1;  // Should be filed with FF
        uint8 PduSpecific;
        uint8 PduFormat;
        struct
        {
            bit DataPage : 1;
            bit Reserved : 1;
            bit UnusedMSB : 6;
        } BitField;
    } nmea2k_TPCMAck_Struct_Type;

    uint8 i;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;
    nmea2k_TPCMAck_Struct_Type TPCMAck;
    nmea2k_MailBox_ID_Struct_Type PGN_Requested;

    // Copy data into TPCMAck struct
    (void)memcpy(&TPCMAck, &MessagePtr->au8Data[0], sizeof(TPCMAck));

    // Extract 24-bits PGN number
    PGN_Requested.IdField.Unused = 0;
    PGN_Requested.IdField.PduSpecific = TPCMAck.PduSpecific;
    PGN_Requested.IdField.PduFormat = TPCMAck.PduFormat;
    PGN_Requested.IdField.BitField.DataPage = TPCMAck.BitField.DataPage;
    PGN_Requested.IdField.BitField.Reserved = TPCMAck.BitField.Reserved;
    PGN_Requested.IdField.BitField.UnusedMSB = 0;

    // Check which Tx Mailbox is corresponding to this End of Msg Ack
    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        TxMultiPacket_PTR = &(nmea2k_TxMultiPacket[nmea2k_u8Instance][i]);
        if ((TxMultiPacket_PTR->InProgress == 1) &&
            (PGN_Requested.Identifier == TxMultiPacket_PTR->PGN) &&
            (Originator_Addr == TxMultiPacket_PTR->DA) &&
            (Originator_Addr != 255) &&
            (TxMultiPacket_PTR->Sequence_Number >= TxMultiPacket_PTR->Packet_Size))
        {
            break;
        }
        else
        {
            TxMultiPacket_PTR = NULL;
        }
    }

    if (TxMultiPacket_PTR != NULL)
    {
        TxMultiPacket_PTR->InProgress = 0;
        TxMultiPacket_PTR->TxMsgPtr->TxInProgress = 0;
        TxMultiPacket_PTR->TxMsgPtr->TxReady = FALSE;
        if (l_dgn_tx_ready_cb)
        {
            l_dgn_tx_ready_cb(TxMultiPacket_PTR->PGN, DGN_MP_MSG_TX_READY);
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_TxMultiPacket_Init
//-----------------------------------------------------------------------------
// Description: Initialization of a MultiPacket transmission.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent.
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Dll_TxMultiPacket_Init(
    NMEA2K_TxMsg_Struct *Msg_nmea2k  // in : Pointer to the Mailbox Element that needs to be transmitted
)
{

    uint8 i;
    uint16 Temp;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;

    // Check Tx MultiPacket session is free
    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        TxMultiPacket_PTR = &(nmea2k_TxMultiPacket[nmea2k_u8Instance][i]);
        if (TxMultiPacket_PTR->InProgress == 0)
        {
            break;
        }
        else
        {
            TxMultiPacket_PTR = NULL;
        }
    }

    // Be certain that a MultiPacket-transmission is not already in progress with this node
    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        if ((nmea2k_TxMultiPacket[nmea2k_u8Instance][i].InProgress == 1) &&
            (nmea2k_TxMultiPacket[nmea2k_u8Instance][i].DA == Msg_nmea2k->Dest_Addr))
        {
            TxMultiPacket_PTR = NULL;
        }
    }

    if (TxMultiPacket_PTR != NULL)  // A transmission of J1939 PGN is needed
    {
        // Fill the MultiPacket State Variables with the Good Information

        TxMultiPacket_PTR->DA = Msg_nmea2k->Dest_Addr;                        // Destination Address
        TxMultiPacket_PTR->Originator = nmea2k_MyAddress[nmea2k_u8Instance];  // Data Originator
        TxMultiPacket_PTR->PGN = Msg_nmea2k->PGN;                             // PGN
        TxMultiPacket_PTR->Msg_Size = Msg_nmea2k->DataSize;                   // Message Size
        Temp = (uint8)(Msg_nmea2k->DataSize / 7);                             // Determine the number of packets that needs to be sent
        if ((Msg_nmea2k->DataSize % 7) != 0)
        {
            Temp++;
        }
        TxMultiPacket_PTR->Packet_Size = (uint8)Temp;
        TxMultiPacket_PTR->Sequence_Number = 1;  // Sequence Number = 1, the first sequence
        TxMultiPacket_PTR->Max_Num_Packet_CTS = NMEA2K_MAX_NUMBER_PACKET_BEFOFE_CTS;
        TxMultiPacket_PTR->Num_Packet_CTS = NMEA2K_MAX_NUMBER_PACKET_BEFOFE_CTS;

        // If the Message is Destination Specific
        if (TxMultiPacket_PTR->DA != 255)
        {
            TxMultiPacket_PTR->DelayTimer = 125;
            TxMultiPacket_PTR->NextTxTimer = 135;
            if (Nmea2k_Tx_TP_CM_RTS(TxMultiPacket_PTR) == TRUE)
            {
                TxMultiPacket_PTR->InProgress = 1;
                Msg_nmea2k->TxInProgress = 1;
                TxMultiPacket_PTR->TxMsgPtr = Msg_nmea2k;
                TxMultiPacket_PTR->Data_Pgm_Ptr = Msg_nmea2k->Data;
                return (TRUE);
            }
        }
        else  // The message is Destination Global
        {
            TxMultiPacket_PTR->DelayTimer = 20;  // Time Out delay
            TxMultiPacket_PTR->NextTxTimer = 5;  // Delay between 2 packets is set at 50ms
            if (Nmea2k_Tx_TP_CM_BAM(TxMultiPacket_PTR) == TRUE)
            {
                TxMultiPacket_PTR->InProgress = 1;
                Msg_nmea2k->TxInProgress = 1;
                TxMultiPacket_PTR->TxMsgPtr = Msg_nmea2k;
                TxMultiPacket_PTR->Data_Pgm_Ptr = Msg_nmea2k->Data;
                return (TRUE);
            }
        }
    }
    return (FALSE);
}
#endif  // NMEA2K_SUPPORT_MULTIPACKET

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_TxSingleFrameMessage
//-----------------------------------------------------------------------------
// Description: Transmit a single frame message by pushing into the TX queue.
//-----------------------------------------------------------------------------
// Return:      TRUE  : Single frame PGN sent to Tx queue
//              FALSE : Tx queue full, no PGN sent.
//-----------------------------------------------------------------------------
static bool Nmea2k_Dll_TxSingleFrameMessage(
    nmea2k_SingleFrame_Message_Struct_Type *Message  // : Single frame PGN pointer
)
{
    bool status;
    HALCAN_zMSG mailbox;
    uint8 *mailboxDataPtr;

    // Put J1939 message directly into TX queue mailbox message
    mailbox.u32Id = Message->MsgId.Identifier;
    mailbox.eIdType = HALCAN_IDTYPE_EXTENDED;
    mailbox.u8Length = Message->ByteCntr;

    // Set MSCAN data destination pointer
    mailboxDataPtr = mailbox.au8Data;

    // Copy J1939 message data into TX queue mailbox message
    while (Message->ByteCntr--)
    {
        *mailboxDataPtr++ = *Message->BytePtr++;
    }

    // TX mailbox message pushed to queue
    status = HALCAN_Write(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, &mailbox);

    return status;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_RxSingleFrameMessage
//-----------------------------------------------------------------------------
// Description: This function fill the pointed J1939 message structure fields.
//-----------------------------------------------------------------------------
// Return:      TRUE  - RxMsg pulled from queue
//              FALSE - Rx queue is empty, no message pulled
//-----------------------------------------------------------------------------
/*
static uint8 Nmea2k_Dll_RxSingleFrameMessage(nmea2k_SingleFrame_Message_Struct_Type *Message)
{
    CQU_Msg_Struct *mailboxPtr;
    uint8                  *mailboxDataPtr;

    // Get MSCAN RX queue mailbox message pointer
    mailboxPtr = CQU_pReadRxQueue();

    // MSCAN RX queue mailbox message trap (queue is not empty)
    if (mailboxPtr != NULL)
    {

        switch (mailboxPtr->Ide)
        {
            case 0:
                // For now theses messages are ignored
                break;

            case 1:

                // Set MSCAN data source pointer
                mailboxDataPtr = &mailboxPtr->Data[0];

                // Fill J1939 message directly from RX queue mailbox message
                Message->MsgId.Identifier = mailboxPtr->Id;
                Message->ByteCntr = mailboxPtr->Dlc;

                // Fill J1939 message data from RX queue mailbox message
                while (mailboxPtr->Dlc--)
                {
                    *Message->BytePtr++ = *mailboxDataPtr++;
                }

                // RX mailbox message pulled from queue
                CQU_PullRxQueue();

                break;
        }
    }
    else
    {
        // No RX queue mailbox message available (queue is empty)
    }

    if (mailboxPtr == NULL) return (FALSE); // No more message, RX queue empty
    else                    return (TRUE);  // RX message pulled from queue
}
*/

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_GetRandomTimeDelay
//-----------------------------------------------------------------------------
// Description: This function computes a random delay based on NAME fields.
//-----------------------------------------------------------------------------
// Return:      J1939 random time delay ( 0..153ms ).
//-----------------------------------------------------------------------------
static uint16 Nmea2k_Dll_GetRandomTimeDelay(
    uint8 *NameFld  // J1939 name field structure pointer
)
{
#if CONFIG_RVC_US
    (void)NameFld;
    uint16 Time;
    Time = esp_random();

    // make it at least 250ms
    if (Time < MIN_DELAY)
    {
        Time = MIN_DELAY + (Time % MIN_DELAY);
    }
    if (Time > MAX_DELAY)
    {
        Time = MIN_DELAY + (Time % MAX_DELAY);  /// + (Time & 0x3F);
    }
#else
    uint8 i = sizeof(nmea2k_NAME_Fields_Struct_Type);
    uint8 Sum = 0;
    uint16 Time;

    // Keep only last byte of the NAME field bytes summation
    while (i--)
    {
        Sum += *NameFld++;
    }
    Time = (Sum * 3) / 5;
#endif
    return (uint16)(Time);
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_S0_AddressClaim_Idle
//-----------------------------------------------------------------------------
// Description: STATE 0  -
//              This is the idle state. The state machine is waiting to be
//              started. In this J1939 device we would transition from idle
//              state after successful completion of power on self test.
//              The machine stay in state 0 as long as the state variable
//              "IdleState" is true. When this variable going to false the
//              action "StartRandomDelay" is completed and go to state 1.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S0_AddressClaim_Idle(void)
{
    LOG(D, "Nmea2k_Dll_S0_AddressClaim_Idle");
    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].IdleState = !NMEA2K_ADDRESS_CLAIM_ENABLED;
    nmea2k_AddrClaim[nmea2k_u8Instance].StartRandomDelay = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].Start250MsDelay = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].SelectStartAddress = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].CommandedAddress = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].FetchNextMyAddress = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].CannotClaimAddress = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].IWin = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].ILose = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn = FALSE;

    // Grafcet control pointer return
    Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T0_AddressClaim_Idle;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_T0_AddressClaim_Idle
//-----------------------------------------------------------------------------
// Description: TRANSITION 0.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_T0_AddressClaim_Idle(void)
{
    LOG(D, "Nmea2k_Dll_T0_AddressClaim_Idle");

    // T0A
    if (nmea2k_AddrClaim[nmea2k_u8Instance].IdleState == FALSE)  // End of power on self test
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S1_AddressClaim_RandomDelay;
    }
    // T0B
    else
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T0_AddressClaim_Idle;  // Stay idle, wait to be started...
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S1_AddressClaim_RandomDelay
//-----------------------------------------------------------------------------
// Description:
//
// STATE 1 :
//   The action StartRandomDelay starts a timer with a random time out
//   period. ISO11783-5:2001(E) specifies how this random delay period
//   is calculated. Completing the action StartRandomDelay sets the
//   state variable "DelayComplete" to false. This state continuously
//   tests the timer that was started during the transition from state
//   0 to state 1. If the state variable "DelayComplete" is false the
//   machine will stay in state 1. When DelayComplete is true present
//   state is forced to 2, so on the state transition diagram there
//   is no forward slash after the state variable "DelayComplete".
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S1_AddressClaim_RandomDelay(void)
{
    // LOG(P, "Nmea2k_Dll_S1_AddressClaim_RandomDelay");
    nmea2k_NAME_Fields_Struct_Type MyName;
#if CONFIG_RVC_US
    nmea2k_AddrClaim[nmea2k_u8Instance].ILose = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].IWin = FALSE;
    Nmea2k_Tx_RVCAddressClaimRequest(nmea2k_MyAddress[nmea2k_u8Instance]);
#endif

    // First, Built my own Name using the Name Fields define by the High Application Level Soft
    Nmea2k_PopulateNAME(&MyName, nmea2k_Parameter[nmea2k_u8Instance]);

    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].SelectStartAddress = TRUE;
    nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].StartRandomDelay = TRUE;

    // Random timeout delay timer log time, it should be at least 250ms or more.
    nmea2k_DelayTmr[nmea2k_u8Instance] = nmea2k_FreeRunTmr[nmea2k_u8Instance] + Nmea2k_Dll_GetRandomTimeDelay((uint8 *)&MyName);

    // Grafcet control pointer return
    Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T1_AddressClaim_RandomDelay;
}

/**
 * @brief Compare free timer and delay timer to see if delay completed.
 * @note  the timer has only 2 bytes, it may overflow every 65536 ms
 * @retval None
 */
static void checkDelayComplete(void)
{
    if (!nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete)
    {

        uint16 free_timer = nmea2k_FreeRunTmr[nmea2k_u8Instance];
        uint16 delay_timer = nmea2k_DelayTmr[nmea2k_u8Instance];

        bool delay_complete = FALSE;

        if (free_timer > delay_timer)
        {
            /// eg. free_timer-(0xFFFF - 300), delaytimer = 100
            if ((free_timer - delay_timer) > (MAX_DELAY * 4))
            {
                LOG(W, "Possbily timer overflow free:%d, delay: %d", free_timer, delay_timer);
                return;
            }
            delay_complete = TRUE;
        }
        else
        {
            if (free_timer == delay_timer)
            {
                delay_complete = TRUE;
            }
            else
            {
                /// eg. freetimer - 100, delaytimer(0xFFFF - 300)
                if ((delay_timer - free_timer) > (MAX_DELAY * 4))
                {
                    delay_complete = TRUE;
                }
            }
        }
        if (delay_complete)
        {
            LOG(D, "2 --- Delay completed ...free: %d, delay: %d", free_timer, delay_timer);
            nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete = TRUE;
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_T1_AddressClaim_RandomDelay
//-----------------------------------------------------------------------------
// Description: TRANSITION 1:
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_T1_AddressClaim_RandomDelay(void)
{
    // if we got another address claim, then it should restart again....
#if CONFIG_RVC_US
    if (nmea2k_AddrClaim[nmea2k_u8Instance].ILose == TRUE)
    {
        //  ...lose challenge
        LOG(W, "We should try claim another addressssss");
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S4_AddressClaim_FetchNextMyAddress;
        return;
    }
#endif

    // Random timeout delay timer overrun trap...
    checkDelayComplete();

    // End of J1939 random delay time
    if (nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete == TRUE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S2_AddressClaim_Transmit;
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S2_AddressClaim_Transmit
//-----------------------------------------------------------------------------
// Description:
//
// STATE 2:
//   In this state we transmit the address claim message. The detailed
//   structure of an address claim message is described in ISO11783-5.
//   Basically we are broadcasting to all other J1939 devices that we
//   (this device) desires to claim (use) the address contained within
//   the address claim message. There are two possible transitions
//   from state 2 :
//
//   I) If the message was sent without error by the CAN controller
//   II) The message was NOT sent without error by the CAN controller
//
//   The CAN controller device driver will set the state variable
//   "CanMsgSent" true or false. If the message was sent without error,
//   we complete the action "Start250MsDelay" and force the present
//   state to state 3. If the message was not sent without error,
//   we complete the action StartRandomDelay and force the
//   present state to state 1.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S2_AddressClaim_Transmit(void)
{
    LOG(D, "Nmea2k_Dll_S2_AddressClaim_Transmit");
    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].StartRandomDelay = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].Start250MsDelay = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].CannotClaimAddress = FALSE;
    // Send change to upper layers
    if (l_address_claimed_cb)
    {
        l_address_claimed_cb();
    }

#if NMEA2K_COMMANDED_ADDRESS_ENABLED
    if (nmea2k_AddrClaim[nmea2k_u8Instance].CommandedAddress == TRUE)
    {
        // Address Claim Procedures
        nmea2k_AddrClaim[nmea2k_u8Instance].SelectStartAddress = FALSE;
        nmea2k_AddrClaim[nmea2k_u8Instance].FetchNextMyAddress = FALSE;
        // Update to request commanded address
        nmea2k_MyAddress[nmea2k_u8Instance] = nmea2k_Commanded[nmea2k_u8Instance];
    }
#endif

    // Transmit address claim message to all network devices
    nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent = Nmea2k_Tx_ISOAddressClaim(nmea2k_MyAddress[nmea2k_u8Instance]);

    // Message transmitted without error
    if (nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent == TRUE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S3_AddressClaim_RxCanFrame;
    }
    // Message NOT transmitted
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent == FALSE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S1_AddressClaim_RandomDelay;
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S3_AddressClaim_RxCanFrame
//-----------------------------------------------------------------------------
// Description:
//
// STATE 3:
//   This state wait for a response to the address claim message sent
//   in state 2. The device may only respond to the address claim messages in state 3.
//   Successful claiming of an address by an device consists of sending an address
//   claim message for the address to be claimed and not receiving contending claims
//   from other ECUs for 250ms.
//
//   The 250ms time out period was initiated during the transition from state 2 to
//   state 3. We wait in state 3 to allow other devices on the network to challenge
//   our address claim.
//
//   The NAME field is a 64-bit field. Details of the NAME field can be found in J1939
//   Standard and ISO11783. For the purpose of address claim we treat the NAME
//   field as a 64-bit unsigned integer, the smaller this integer, the higher is the
//   priority. It is NEVER permitted to have two or more devices on the network with
//   identical NAME field.
//
//   A device that have previously claimed the address we wish to claim AND has a
//   higher priority will challenge us by sending and address claim message containing
//   the address in dispute. It is also possible that another device that is powered
//   on during this 250ms period may try to claim the same address we wish to claim.
//   In either case we receive and process address claim messages that have the same
//   source address at the one we wish to claim. We process this received message in
//   state 3 and determine if the other device has a higher priority, or lower
//   priority then ourselves, this is done by comparing our 64-bit NAME field to the
//   other devices NAME field.
//
//   If we process a received address claim message and the 64-bit NAME field of this
//   received address claim message is numerically less than our own NAME field, then
//   the state variable "ILose" is set to true. This is because we have lost the
//   address claim challenge and complete FetchNextMyAddress action in state 4.
//
//   If we process a received address claim message and the 64-bit NAME field of this
//   received address claim message is numerically greater than our own NAME field,
//   then the state variable "IWin" is set to true. This is because we have won the
//   address claim challenge. We transition to state 2 so we can again send our
//   address claim message and start the 250ms time out period. Though we won the
//   challenge, by specification we must wait 250ms in order to start using this
//   claimed address for messages other than address claim or request for address
//   claim.
//
//   The action Start250MsDelay starts a time with a fixed timeout period of 250ms.
//   ISO11783-5 specifies this fixed delay. Completing the action Start250MsDelay
//   negates the state variable "DelayComplete". When the 250ms delay times out we
//   then go to state 5.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S3_AddressClaim_RxCanFrame(void)
{
    LOG(D, "Nmea2k_Dll_S3_AddressClaim_RxCanFrame");

    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].IWin = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].ILose = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = FALSE;

    // One shot no-retry fixed 250ms timeout delay
    if (nmea2k_AddrClaim[nmea2k_u8Instance].Start250MsDelay == FALSE)
    {
        // Address Claim Procedures
        nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete = FALSE;
        nmea2k_AddrClaim[nmea2k_u8Instance].Start250MsDelay = TRUE;

        // Log time of fixed 250ms timeout delay timer
        nmea2k_DelayTmr[nmea2k_u8Instance] = nmea2k_FreeRunTmr[nmea2k_u8Instance] + 250;
    }

    // Grafcet control pointer return
    Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T3_AddressClaim_RxCanFrame;
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_T3_AddressClaim_RxCanFrame
//-----------------------------------------------------------------------------
// Description:
//
// Wait to receive address claim messages from other network devices (3-bit XOR set)
// If received message address claim win challenge -> IWin == TRUE;
// If received message address claim lose challenge -> ILose == TRUE;
// Not either address claim or PGN request for address address -> NoMessage == TRUE;
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_T3_AddressClaim_RxCanFrame(void)
{
    // Fixed 250ms timeout delay timer overrun trap...
    checkDelayComplete();
    // if (nmea2k_FreeRunTmr[nmea2k_u8Instance] >= nmea2k_DelayTmr[nmea2k_u8Instance])
    // {

    //     LOG(W, "2 --- Delay completed ...%d", nmea2k_DelayTmr[nmea2k_u8Instance]);
    //     nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete = TRUE;
    // }

    // Address claim message received and...
    if (nmea2k_AddrClaim[nmea2k_u8Instance].IWin == TRUE)
    {
        //  ...win challenge
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S2_AddressClaim_Transmit;
    }
    // Address claim message received and...
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].ILose == TRUE)
    {
        //  ...lose challenge
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S4_AddressClaim_FetchNextMyAddress;
    }
    // End of ISO11783 fixed 250ms timeout delay
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete == TRUE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S5_AddressClaim_RxCanFrame;
    }

#if NMEA2K_COMMANDED_ADDRESS_ENABLED
    // Commanded address message received
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].CommandedAddress == TRUE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S2_AddressClaim_Transmit;
    }
#endif
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S4_AddressClaim_FetchNextMyAddress
//-----------------------------------------------------------------------------
// Description:
//
// STATE 4:
//   This where we try and generate a new address. The source address SA
//   that we are trying to claim is an 8-bit value, so there are 256 possible
//   addresses (0..255) except address 252, 253, 254, 255 that are reserved. So the
//   possible addresses we may claim are 0 through 251 for a total of 252 addresses.
//
//   ISO11783-5 does not specify any particular algorithm that a device must use in
//   order to choose a source address. However it is recommended that once a source
//   address is claimed that the device attempt to claim this same address when the
//   device is again powered down then powered on. The idea being that once a network
//   is operating and all of the device have a source address, there will be a minimum
//   of thrashing on the network whenever a device is powered on.
//
//   ISO11783-5 does specify that in the event a device cannot find an address to
//   claim it may NOT participate in network communications except to respond to a
//   request for address claim message.
//
//   In state 4 we generate a new source address by adding 1 to the present source
//   address, if the new source address is greater than 251 we are out of address
//   and must give up to state 6 and completing the action StartRandomDelay. If the
//   new source address is less than 252 we transition to state 2 where we send an
//   address claim message.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S4_AddressClaim_FetchNextMyAddress(void)
{
    LOG(D, "Nmea2k_Dll_S4_AddressClaim_FetchNextMyAddress");

    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].Start250MsDelay = FALSE;

    // FIXED-WQL, RV-C will decrease the current address by 1.
    // Next source address to be claimed except J1939 reserved source addresses
#if CONFIG_RVC_US
    if (--nmea2k_MyAddress[nmea2k_u8Instance] == 0)  // > NMEA2K_ADDRESS_CLAIM_MAX_ADDRESS)
    {
        // TODO-WQL, it should use Address Range
        nmea2k_MyAddress[nmea2k_u8Instance] = NMEA2K_ADDRESS_CLAIM_MAX_ADDRESS - 1;
    }
#else
    if (++nmea2k_MyAddress[nmea2k_u8Instance] > NMEA2K_ADDRESS_CLAIM_MAX_ADDRESS)
    {
        nmea2k_MyAddress[nmea2k_u8Instance] = 0;
    }
#endif

    // Source address rollover check
    nmea2k_AddrClaim[nmea2k_u8Instance].FetchNextMyAddress = (nmea2k_MyAddress[nmea2k_u8Instance] != *(nmea2k_Parameter[nmea2k_u8Instance]->PreferredSA)) ? TRUE : FALSE;

    // Grafcet control pointer return
    Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T4_AddressClaim_FetchNextMyAddress;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_T4_AddressClaim_FetchNextMyAddress
//-----------------------------------------------------------------------------
// Description:
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_T4_AddressClaim_FetchNextMyAddress(void)
{
    LOG(D, "Nmea2k_Dll_T4_AddressClaim_FetchNextMyAddress");

    // T4A
    if (nmea2k_AddrClaim[nmea2k_u8Instance].FetchNextMyAddress == TRUE)  // Valid new source address
    {
#if CONFIG_RVC_US
        // In RVC_US, it should start a new cycle of address claiming, send the request first, delay at least
        // 250 ms, then may send address_claim.
        LOG(W, "It should start random delay again....");
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S1_AddressClaim_RandomDelay;
#else
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S2_AddressClaim_Transmit;
#endif
    }
    // T4B
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].FetchNextMyAddress == FALSE)  // No more possible source address
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S6_AddressClaim_RandomDelay;
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S5_AddressClaim_RxCanFrame
//-----------------------------------------------------------------------------
// Description:
//
// STATE 5)
//   This state means we have successfully claimed an address on the
//   J1939 network. We gave the other devices 250ms to challenge our claimed
//   address and either no other device challenged our claim or we won the claim.
//   At this point the device is free to transmit message that pertain to it's normal
//   operation. Any message that is NOT either address claim or request for address
//   claim, is designated as "NoMessage", no processing is done for messages that are
//   defined as NoMessage. The only way to transition out of state 5 is to receive
//   either an address claim message or receive a request for address claim message.
//
//   While in state 5 it is possible for other devices to be powered on, these devices
//   may send address claim messages with the same source address SA that we have
//   claimed. So we compare the received NAME field to our own NAME field just like
//   state 3. The result of this compare will set one of the state variables "IWin"
//   or "ILose" to true.
//
//   We may also receive a request for address claim message, in which no comparison
//   of NAME field is required, just send an address claim message.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S5_AddressClaim_RxCanFrame(void)
{
    LOG(D, "Nmea2k_Dll_S5_AddressClaim_RxCanFrame: Address claim done");

    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].IWin = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].ILose = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].Start250MsDelay = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed = TRUE;

    // Advise the Application that the Address Claim is finished
    // If needed updated the current source address for the next Power-Up
    nmea2k_Status[nmea2k_u8Instance].AddClaim = TRUE;
    if (nmea2k_MyAddress[nmea2k_u8Instance] != nmea2k_Status[nmea2k_u8Instance].SA)
    {
        nmea2k_Status[nmea2k_u8Instance].SA = nmea2k_MyAddress[nmea2k_u8Instance];
    }

    // Grafcet control pointer return
    Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T5_AddressClaim_RxCanFrame;

    // Send change to upper layers
    if (l_address_claimed_cb)
    {
        l_address_claimed_cb();
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_T5_AddressClaim_RxCanFrame
//-----------------------------------------------------------------------------
// Description:
//
// Receive address claim messages from other network devices (4-bit XOR set)
// If received message address claim win challenge -> IWin == TRUE;
// If received message address claim lose challenge -> ILose == TRUE;
// If received message request for address claim -> RequestPgn == TRUE;
// Not either address claim or PGN request for address claim -> NoMessage == TRUE;
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_T5_AddressClaim_RxCanFrame(void)
{
    // T5A
    // Receive a request PGN for address claim
    // Receive address claim message and...
    if (((nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn == TRUE) || (nmea2k_AddrClaim[nmea2k_u8Instance].IWin == TRUE)))
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S9_AddressClaim_Transmit;  //  ...win challenge
    }
    // T5B
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].ILose == TRUE)  // Receive address claim message and...
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S4_AddressClaim_FetchNextMyAddress;  //  ...lose chalenge
    }

#if NMEA2K_COMMANDED_ADDRESS_ENABLED
    // T5D
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].CommandedAddress == TRUE)  // Commanded address message received
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S2_AddressClaim_Transmit;
    }
#endif
    // T5E
    else
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T5_AddressClaim_RxCanFrame;  // Wait Rx message...
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S6_AddressClaim_RandomDelay
//-----------------------------------------------------------------------------
// Description:
//
// STATE 6)
//   The transition from state 4 to state 6 means we have fail to
//   successfully claim a source address SA. In accordance with ISO11783-5 we wait a
//   random delay then transmit a can not claim source address message.The can not
//   claim message is an address claim message with the source address SA field equal
//   to 254. According to ISO11783-5, a device which cannot claim a source address
//   may only to respond for address claim messages.
//
//   The only way to attempt further address claiming is to cycle the device power.
//   In state 6 we wait for the random time out delay initiated during the transition
//   from state 4 to state 6. When the delay is completed we transition to state 7.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S6_AddressClaim_RandomDelay(void)
{
    LOG(D, "Nmea2k_Dll_S6_AddressClaim_RandomDelay");

    nmea2k_NAME_Fields_Struct_Type MyName;

    // First, Built my own Name using the Name Fields define by the High Application Level Soft
    Nmea2k_PopulateNAME(&MyName, nmea2k_Parameter[nmea2k_u8Instance]);

    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].StartRandomDelay = TRUE;

    // Random timeout delay timer log time
    nmea2k_DelayTmr[nmea2k_u8Instance] = nmea2k_FreeRunTmr[nmea2k_u8Instance] + Nmea2k_Dll_GetRandomTimeDelay((uint8 *)&MyName);

    // Grafcet control pointer return
    Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T6_AddressClaim_RandomDelay;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_T6_AddressClaim_RandomDelay
//-----------------------------------------------------------------------------
// Description:
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_T6_AddressClaim_RandomDelay(void)
{
    // Random timeout delay timer overrun trap...
    checkDelayComplete();
    // if (nmea2k_FreeRunTmr[nmea2k_u8Instance] >= nmea2k_DelayTmr[nmea2k_u8Instance])
    // {

    //     LOG(W, "3 --- Delay completed ... %d", nmea2k_DelayTmr[nmea2k_u8Instance] );
    //     nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete = TRUE;
    // }
    // End of j1939 random delay time
    if (nmea2k_AddrClaim[nmea2k_u8Instance].DelayComplete == TRUE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S7_AddressClaim_Transmit;
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S7_AddressClaim_Transmit
//-----------------------------------------------------------------------------
// Description:
//
// STATE 7)
//   We transmit an address claim message with source address SA equal
//   to 254. By definition this is called a cannot claim source address message. If
//   the CAN controller sent the message, we transition to state 8, other wise we
//   start a random delay and go back to state 6 to wait for the random delay period
//   to time out.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S7_AddressClaim_Transmit(void)
{
    LOG(D, "Nmea2k_Dll_S7_AddressClaim_Transmit");
    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].StartRandomDelay = FALSE;

    // This node can not claim an address...
    // Set this device address to J1939 null address
    nmea2k_MyAddress[nmea2k_u8Instance] = NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS;

    // Transmit address claim message to all network devices
    nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent = Nmea2k_Tx_ISOAddressClaim(nmea2k_MyAddress[nmea2k_u8Instance]);

    // Message transmitted without error
    if (nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent == TRUE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S8_AddressClaim_RxCanFrame;
    }
    // Message NOT trasmitted
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent == FALSE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S6_AddressClaim_RandomDelay;
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S8_AddressClaim_RxCanFrame
//-----------------------------------------------------------------------------
// Description:
//
// STATE 8)
//   We process received messages in this state. In accordance with
//   ISO11783-5, the ONLY message this device may respond to is a request for source
//   address SA message, ALL other messages are ignored. If a request message is
//   received, we initiate a random delay period and made transition to state 6 to
//   wait for the period to expire, then transition to state 7 to transmit a can not
//   claim address message.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S8_AddressClaim_RxCanFrame(void)
{
    LOG(D, "Nmea2k_Dll_S8_AddressClaim_RxCanFrame");

    // Address Claim Procedures
    nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn = FALSE;
    nmea2k_AddrClaim[nmea2k_u8Instance].CannotClaimAddress = TRUE;

    // Grafcet control pointer return
    Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T8_AddressClaim_RxCanFrame;
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_T8_AddressClaim_RxCanFrame
//-----------------------------------------------------------------------------
// Description:
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_T8_AddressClaim_RxCanFrame(void)
{
    LOG(D, "Nmea2k_Dll_T8_AddressClaim_RxCanFrame");

    // T8A
    if (nmea2k_AddrClaim[nmea2k_u8Instance].RequestPgn == TRUE)  // Receive a request PGN for address claim
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S6_AddressClaim_RandomDelay;
    }

#if NMEA2K_COMMANDED_ADDRESS_ENABLED
    // T8B
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].CommandedAddress == TRUE)  // Commanded address message received
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S2_AddressClaim_Transmit;
    }
#endif
    // T8C
    else
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_T8_AddressClaim_RxCanFrame;  // Wait request PGN or commanded address
    }
}

//-----------------------------------------------------------------------------
// Function: Nmea2k_Dll_S9_AddressClaim_Transmit
//-----------------------------------------------------------------------------
// Description:
//
// STATE 9)
//   This state simply transmit an address claim message containing our
//   claimed address. There are two reasons that would cause a transition to state 9.
//
//        I) We received an address claim message with an address that matches our
//           own claimed address and 64-bit NAME field of the received address claim
//           message is numerically greater than our own NAME field, so we are
//           re-asserting our claimed address.
//       II) We receive a request for address claimed message, so we are simply
//           responding with our NAME field and address.
//
//   Now it is possible that the CAN controller did not successfully send the message
//   in state 9, so in this case we go to a random delay, otherwise we go to state 5.
//-----------------------------------------------------------------------------
// Return: None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_S9_AddressClaim_Transmit(void)
{
    LOG(D, "Nmea2k_Dll_S9_AddressClaim_Transmit");

    // Transmit address claim message to all network devices
    nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent = Nmea2k_Tx_ISOAddressClaim(nmea2k_MyAddress[nmea2k_u8Instance]);

    // Message transmitted without error
    if (nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent == TRUE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S5_AddressClaim_RxCanFrame;
    }
    // Message NOT transmitted
    else if (nmea2k_AddrClaim[nmea2k_u8Instance].CanMsgSent == FALSE)
    {
        Nmea2k_Dll_G7_AddressClaim[nmea2k_u8Instance] = Nmea2k_Dll_S1_AddressClaim_RandomDelay;
    }
}

#if NMEA2K_SUPPORT_MULTIPACKET
//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_MultiPacket_TimeOut
//-----------------------------------------------------------------------------
// Description: When a Multi-Packet session is in progress many timing have to be
//              respected. If a time out occurs, the session needs to be aborted
//              In addition, if the session is destination specific the function
//              send a Connection Abort Message.
//-----------------------------------------------------------------------------
// Return:      None (MultiPacket structure is modified and Conn Abort can be sent)
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_MultiPacket_TimeOut(void)
{
    uint8 i;
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *nmea2k_RxMultiPacket_PTR;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;

    for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
    {
        nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];

        if (nmea2k_RxMultiPacket_PTR->InProgress == 1)  // If Multi-Packet session is in progress
        {
            if (nmea2k_RxMultiPacket_PTR->DelayTimer == 0)  // If a TimeOut has occurs
            {
                if (nmea2k_RxMultiPacket_PTR->DA != 255)  // If the Session is Destination Specific
                {
                    (void)Nmea2k_Tx_TP_Conn_Abort(nmea2k_RxMultiPacket_PTR->Originator, 3, nmea2k_RxMultiPacket_PTR->PGN);
                }
                nmea2k_RxMultiPacket_PTR->InProgress = 0;
                nmea2k_RxMultiPacket_PTR->RxMsgPtr->Rx_InProgress = 0;
            }
        }
    }
    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        TxMultiPacket_PTR = &nmea2k_TxMultiPacket[nmea2k_u8Instance][i];

        if (TxMultiPacket_PTR->InProgress == 1)  // If Multi-Packet session is in progress
        {
            // it's using this timeout to exit the transfer.
            if (TxMultiPacket_PTR->DelayTimer == 0)  // If a TimeOut has occurs
            {
                if (TxMultiPacket_PTR->DA != 255)  // If the Session is Destination Specific
                {
                    (void)Nmea2k_Tx_TP_Conn_Abort(TxMultiPacket_PTR->DA, 3, TxMultiPacket_PTR->PGN);
                }
                TxMultiPacket_PTR->InProgress = 0;
                TxMultiPacket_PTR->TxMsgPtr->TxInProgress = 0;
                TxMultiPacket_PTR->TxMsgPtr->TxReady = FALSE;
                // Don't clear the TXReady in the J1939 MailBox. The message was not sent completely
            }
        }
    }
}
#endif  // NMEA2K_SUPPORT_MULTIPACKET

#if NMEA2K_SUPPORT_MULTIPACKET
//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_MultiPacket_Transmit
//-----------------------------------------------------------------------------
// Description: This function check all the possible Tx Multi-Packet Session
//              If one of the session is active and DataTransfert is needed
//              Nmea2k_Tx_DataTransfert is called.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_MultiPacket_Transmit(void)
{
    uint8 i;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;

    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        TxMultiPacket_PTR = &nmea2k_TxMultiPacket[nmea2k_u8Instance][i];

        // If TxMulti Packet session is in progress
        if ((TxMultiPacket_PTR->InProgress == 1) && (TxMultiPacket_PTR->NextTxTimer == 0))
        {
            (void)Nmea2k_Tx_DataTransfert(TxMultiPacket_PTR);
        }
    }
}
#endif  // NMEA2K_SUPPORT_MULTIPACKET

#if NMEA2K_SUPPORT_MULTIPACKET
//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_MultiPacket_UpdCtr
//-----------------------------------------------------------------------------
// Description: This function check all the possible Multi Packet Session (Tx or Rx)
//              If an active Multi-Packet session is found, the Time-Out and Transmit
//              counter are decrement.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_MultiPacket_UpdCtr(void)
{
    uint8 i;
    nmea2k_RxMultiPacket_State_Variables_Struct_Type *nmea2k_RxMultiPacket_PTR;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;

    for (i = 0; i < NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION; i++)
    {
        nmea2k_RxMultiPacket_PTR = &nmea2k_RxMultiPacket[nmea2k_u8Instance][i];

        if (nmea2k_RxMultiPacket_PTR->InProgress == 1)  // If Multi-Packet session is in progress
        {
            if (nmea2k_RxMultiPacket_PTR->DelayTimer != 0)  // Decrement the Time-Out Counter until 0 is reach
            {
                nmea2k_RxMultiPacket_PTR->DelayTimer--;
            }
        }
    }

    for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
    {
        TxMultiPacket_PTR = &nmea2k_TxMultiPacket[nmea2k_u8Instance][i];

        if (TxMultiPacket_PTR->InProgress == 1)  // If Multi-Packet session is in progress
        {
            if (TxMultiPacket_PTR->DelayTimer != 0)  // Decrement the Time-Out Counter until 0 is reach
            {
                TxMultiPacket_PTR->DelayTimer--;
            }
            if (TxMultiPacket_PTR->NextTxTimer != 0)  // Decrement the Transmit Counter until 0 is reach
            {
                TxMultiPacket_PTR->NextTxTimer--;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    NMEA2K_OngoingMPSession
//-----------------------------------------------------------------------------
// Description: This function check all the possible Multi Packet Session (Tx)
//              If an active Multi-Packet session is found, the function returns true
//              else false.
//-----------------------------------------------------------------------------
// Return:      TRUE - an active multi-packet session is found
//              FALSE - an active multi-packet session was not found
//-----------------------------------------------------------------------------
bool NMEA2K_OngoingMPSession(void)
{
    uint8 i;
    nmea2k_TxMultiPacket_State_Variables_Struct_Type *TxMultiPacket_PTR;
    for (nmea2k_u8Instance = 0; nmea2k_u8Instance < nmea2k_u8InstanceCount; nmea2k_u8Instance++)
    {
        for (i = 0; i < NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION; i++)
        {
            TxMultiPacket_PTR = &nmea2k_TxMultiPacket[nmea2k_u8Instance][i];
            if (TxMultiPacket_PTR->InProgress == 1)  // If Multi-Packet session is in progress
            {
                return true;
            }
        }
    }
    return false;
}
#endif  // NMEA2K_SUPPORT_MULTIPACKET

#if NMEA2K_SUPPORT_FAST_PACKET_RX
//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_RxFastPacketMsg
//-----------------------------------------------------------------------------
// Description: This function needs to be called when a fast packet message is received
//              All the reception process is process in this function.
//-----------------------------------------------------------------------------
// Return:      TRUE  - The Message was process and updated
//              FALSE - The Message was not process
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Dll_RxFastPacketMsg(
    HALCAN_zMSG *Msg_Can,
    NMEA2K_RxMsg_Struct *Msg_nmea2k,
    uint8 SA,
    uint8 DA)
{
    uint8 i;
    uint8 FrameCounter;
    uint8 SequenceCounter;
    uint8 TotalMsgSize;
    uint8 ReturnStatus = 0;
    nmea2k_RxFastPacket_State_Variables_Struct_Type *RxFastPacketPtr;

    RxFastPacketPtr = &RxFastPacket[nmea2k_u8Instance][0];

    FrameCounter = Msg_Can->au8Data[0] & 0x1F;
    SequenceCounter = (Msg_Can->au8Data[0] & 0xE0) >> 5;

    // Find the Rx Fast Packet session corresponding to this PGN
    for (i = 0; i < NMEA2K_NUMBER_RX_FAST_PACKET_SESSION; i++)
    {
        if (RxFastPacketPtr->RxMsgPtr == Msg_nmea2k)
        {
            break;
        }
        else
        {
            RxFastPacketPtr++;
        }
    }

    // If the Reception is not already active and First frame of a fast packet message
    // Also check if the Originator Filter is used
    if (((Msg_nmea2k->Rx_InProgress == 0) && (FrameCounter == 0)) &&
        ((Msg_nmea2k->Org_Filter == 0xFF) || (Msg_nmea2k->Org_Filter == SA)))
    {
        TotalMsgSize = Msg_Can->au8Data[1];
        if (Msg_nmea2k->BufferSize >= TotalMsgSize)
        {
            Msg_nmea2k->MsgSize = TotalMsgSize;
            Msg_nmea2k->Org_Addr = SA;
            Msg_nmea2k->Dst_Addr = DA;

            if (TotalMsgSize <= 6)
            {
                // The message contains only one Frame, update J1939 Rx Mailbox right now
                Msg_nmea2k->Ageing_Ctr = 0;
                for (i = 0; i < TotalMsgSize; i++)
                {
                    Msg_nmea2k->Data[i] = Msg_Can->au8Data[i + 2];
                }

                // Call the reception handler if it exists
                if (Msg_nmea2k->Handle != NULL)
                {
                    Msg_nmea2k->Handle(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, Msg_nmea2k);
                }
            }
            else
            {
                // The message contains more than one Frame
                Msg_nmea2k->Rx_InProgress = 1;
                RxFastPacketPtr->InProgress = 1;
                RxFastPacketPtr->Frame_Counter = FrameCounter;
                RxFastPacketPtr->SeqCounter = SequenceCounter;
                RxFastPacketPtr->DelayTimer = NMEA2K_FAST_PACKET_RX_TIME_OUT;  // Time-Out will occurs if the next frame is not receive in 750 mS
                RxFastPacketPtr->Num_Byte_Rx = 6;
                RxFastPacketPtr->Byte_Pgm_Ptr = &Msg_nmea2k->Data[0];
                for (i = 0; i < 6; i++)
                {
                    *RxFastPacketPtr->Byte_Pgm_Ptr = Msg_Can->au8Data[i + 2];
                    RxFastPacketPtr->Byte_Pgm_Ptr++;
                }
            }
            ReturnStatus = 1;
        }
    }

    // If the reception is already active and it's not the first Frame
    else if ((Msg_nmea2k->Rx_InProgress == 1) && (FrameCounter != 0))
    {
        if ((RxFastPacketPtr->SeqCounter == SequenceCounter) && (SA == Msg_nmea2k->Org_Addr))
        {
            if (FrameCounter == (RxFastPacketPtr->Frame_Counter + 1))
            {
                RxFastPacketPtr->Frame_Counter++;
                RxFastPacketPtr->DelayTimer = NMEA2K_FAST_PACKET_RX_TIME_OUT;  // Time-Out will occurs if the next frame is not receive in 750 mS
                for (i = 0; ((i < 7) && (RxFastPacketPtr->Num_Byte_Rx < Msg_nmea2k->MsgSize)); i++)
                {
                    *RxFastPacketPtr->Byte_Pgm_Ptr = Msg_Can->au8Data[i + 1];
                    RxFastPacketPtr->Byte_Pgm_Ptr++;
                    RxFastPacketPtr->Num_Byte_Rx++;
                }
                if ((RxFastPacketPtr->Num_Byte_Rx) == Msg_nmea2k->MsgSize)
                {
                    // Message reception complete
                    Msg_nmea2k->Rx_InProgress = 0;
                    RxFastPacketPtr->InProgress = 0;
                    Msg_nmea2k->Ageing_Ctr = 0;

                    // Call the reception handler if it exists
                    if (Msg_nmea2k->Handle != NULL)
                    {
                        Msg_nmea2k->Handle(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, Msg_nmea2k);
                    }
                }
            }
            else  // The sequence of message is not respected, discard the session
            {
                Msg_nmea2k->Rx_InProgress = 0;
                RxFastPacketPtr->InProgress = 0;
            }
            ReturnStatus = 1;
        }
    }
    return (ReturnStatus);
}
#endif  // NMEA2K_SUPPORT_FAST_PACKET_RX

#if NMEA2K_SUPPORT_FAST_PACKET_TX
//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_TxFastPacket_Init
//-----------------------------------------------------------------------------
// Description: This function needs to be called when the transmission of a fast
//              packet is started.
//-----------------------------------------------------------------------------
// Return:      TRUE  - The Message was process and updated
//              FALSE - The Message was not process
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Dll_TxFastPacket_Init(NMEA2K_TxMsg_Struct *Msg_nmea2k)
{
    uint8 i;
    nmea2k_TxFastPacket_State_Variables_Struct_Type *TxFastPacketPtr = NULL;
    nmea2k_SingleFrame_Message_Struct_Type message;
    uint8 Data[8];
    bool bSamePgnInProgress = FALSE;

    // Loop through Tx Fast Packet array
    for (i = 0; i < NMEA2K_NUMBER_TX_FAST_PACKET_SESSION; i++)
    {
        // Fast packet session attached to a mailbox?
        if (TxFastPacket[nmea2k_u8Instance][i].TxMsgPtr != NULL)
        {
            // Fast packet PGN match?
            if (TxFastPacket[nmea2k_u8Instance][i].TxMsgPtr->PGN == Msg_nmea2k->PGN)
            {
                // Message pointer matches the message requested for initialization?
                if (TxFastPacket[nmea2k_u8Instance][i].TxMsgPtr == Msg_nmea2k)
                {
                    // Set pointer to this index in the Tx fast packet array
                    TxFastPacketPtr = &TxFastPacket[nmea2k_u8Instance][i];
                }

                // Transmission is in progress?
                if (TxFastPacket[nmea2k_u8Instance][i].InProgress == TRUE)
                {
                    // Set flag and break for loop
                    bSamePgnInProgress = TRUE;
                }
            }
        }
    }

    // This message was not found in the fast packet array?
    if (TxFastPacketPtr == NULL)
    {
        // Clear ready flag - cannot Tx this message
        // Consider having this lead to alerting designer
        Msg_nmea2k->TxReady = FALSE;

        // Return without initializing
        return FALSE;
    }

    // Tx of an instance of this PGN is already in progress?
    if (bSamePgnInProgress)
    {
        // Return without initializing - need to wait for other instance to complete transmission
        return FALSE;
    }

    // Set up various parameters
    TxFastPacketPtr->TxFrame_Counter = 0;
    TxFastPacketPtr->NextTxTimer = NMEA2K_FAST_PACKET_TX_INTER_FRAME;
    TxFastPacketPtr->Num_Byte_Tx = 0;
    TxFastPacketPtr->Byte_Tx_Ptr = Msg_nmea2k->Data;

    // Get next sequence counter and put it in byte 0
    TxFastPacketPtr->SeqCounter = TxFastPacketPtr->u8NextSeqCounter;
    Data[0] = (TxFastPacketPtr->SeqCounter << 5);

    // Increment sequence counter and manage rollover
    TxFastPacketPtr->u8NextSeqCounter += 1;
    TxFastPacketPtr->u8NextSeqCounter &= 0x07;

    // Set next sequence counter for all fast packet sessions of the same PGN.
    for (i = 0; i < NMEA2K_NUMBER_TX_FAST_PACKET_SESSION; i++)
    {
        // Fast packet session attached to a mailbox?
        if (TxFastPacket[nmea2k_u8Instance][i].TxMsgPtr != NULL)
        {
            // Fast packet PGN match?
            if (TxFastPacket[nmea2k_u8Instance][i].TxMsgPtr->PGN == TxFastPacketPtr->TxMsgPtr->PGN)
            {
                // Set next sequence counter
                TxFastPacket[nmea2k_u8Instance][i].u8NextSeqCounter = TxFastPacketPtr->u8NextSeqCounter;
            }
        }
    }

    // Set message data size
    Data[1] = (uint8)Msg_nmea2k->DataSize;

    for (i = 0; (i < 6) && (i < Msg_nmea2k->DataSize); i++)
    {
        Data[i + 2] = *TxFastPacketPtr->Byte_Tx_Ptr;
        TxFastPacketPtr->Byte_Tx_Ptr++;
        TxFastPacketPtr->Num_Byte_Tx++;
    }

    message.BytePtr = &Data[0];
    message.ByteCntr = TxFastPacketPtr->Num_Byte_Tx + 2;
    // J1939 29-bit identifier fields
    message.MsgId.Identifier = Msg_nmea2k->PGN << 8;
    message.MsgId.IdField.BitField.Priority = (Msg_nmea2k->Priority & 0x07);
    message.MsgId.IdField.SourceAddress = nmea2k_MyAddress[nmea2k_u8Instance];
    if (message.MsgId.IdField.PduFormat < 240)
    {
        message.MsgId.IdField.PduSpecific = Msg_nmea2k->Dest_Addr;
    }

    // Valid PGN frame transmission state trap...
    if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
    {
        if (Msg_nmea2k->DataSize > 6)
        {
            TxFastPacketPtr->InProgress = 1;
            Msg_nmea2k->TxInProgress = 1;
        }
        else
        {
            Msg_nmea2k->TxReady = FALSE;
        }
        return TRUE;
    }
    return FALSE;
}
#endif  // NMEA2K_SUPPORT_FAST_PACKET_TX

#if NMEA2K_SUPPORT_FAST_PACKET_TX
//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_TxFastPacket
//-----------------------------------------------------------------------------
// Description: This function go through all the Tx Fast Packet session.
//              The function transmit the scheduled fast packet if needed.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_TxFastPacket(void)
{
    uint8 i;
    nmea2k_SingleFrame_Message_Struct_Type message;
    NMEA2K_TxMsg_Struct *Msg_nmea2k;
    uint8 Data[8];

    nmea2k_TxFastPacket_State_Variables_Struct_Type *TxFastPacketPtr;

    TxFastPacketPtr = &TxFastPacket[nmea2k_u8Instance][0];

    // Go through all Fast-Packet session and transmit packet if needed
    for (i = 0; i < NMEA2K_NUMBER_TX_FAST_PACKET_SESSION; i++)
    {
        if ((TxFastPacketPtr->InProgress == 1) && (TxFastPacketPtr->NextTxTimer == 0))
        {
            if (HALCAN_TxFree(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort) > NMEA2K_RESERVED_FIFO_SPACE)

            {
                unsigned j;

                Msg_nmea2k = TxFastPacketPtr->TxMsgPtr;

                TxFastPacketPtr->TxFrame_Counter++;
                TxFastPacketPtr->NextTxTimer = NMEA2K_FAST_PACKET_TX_INTER_FRAME;

                Data[0] = ((TxFastPacketPtr->SeqCounter & 0x07) << 5) | (TxFastPacketPtr->TxFrame_Counter & 0x1F);
                for (j = 0; (j < 7) && (TxFastPacketPtr->Num_Byte_Tx < Msg_nmea2k->DataSize); j++)
                {
                    Data[j + 1] = *TxFastPacketPtr->Byte_Tx_Ptr;
                    TxFastPacketPtr->Byte_Tx_Ptr++;
                    TxFastPacketPtr->Num_Byte_Tx++;
                }
                for (; j < 7; j++)
                {
                    Data[j + 1] = 0xFF;
                }

                message.BytePtr = &Data[0];
                message.ByteCntr = j + 1;
                // J1939 29-bit identifier fields
                message.MsgId.Identifier = Msg_nmea2k->PGN << 8;
                message.MsgId.IdField.BitField.Priority = (Msg_nmea2k->Priority & 0x07);
                message.MsgId.IdField.SourceAddress = nmea2k_MyAddress[nmea2k_u8Instance];
                if (message.MsgId.IdField.PduFormat < 240)
                {
                    message.MsgId.IdField.PduSpecific = Msg_nmea2k->Dest_Addr;
                }

                // Valid PGN frame transmission state trap...
                if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
                {
                    if (TxFastPacketPtr->Num_Byte_Tx == Msg_nmea2k->DataSize)
                    {
                        TxFastPacketPtr->InProgress = 0;
                        Msg_nmea2k->TxInProgress = 0;
                        Msg_nmea2k->TxReady = FALSE;
                        if (l_dgn_tx_ready_cb)
                        {
                            l_dgn_tx_ready_cb(Msg_nmea2k->PGN, DGN_STANDARD_MSG_TX_READY);
                        }
                    }
                }
            }
        }
        TxFastPacketPtr++;
    }
}
#endif  // NMEA2K_SUPPORT_FAST_PACKET_TX

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_FastPacket_TimeOut_10mS
//-----------------------------------------------------------------------------
// Description: This function go through all the Fast Packet session.
//              For the RX Fast Packet the function decrement the time-out counter
//              and check if the session needs to be aborted
//              For the TX FastPacket, the next transmission scheduler timer is
//              decrement if needed.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_Dll_FastPacket_TimeOut_10mS(void)
{
#if NMEA2K_SUPPORT_FAST_PACKET_RX
    uint8 i;

    nmea2k_RxFastPacket_State_Variables_Struct_Type *RxFastPacketPtr;
    nmea2k_TxFastPacket_State_Variables_Struct_Type *TxFastPacketPtr;

    RxFastPacketPtr = &RxFastPacket[nmea2k_u8Instance][0];
    for (i = 0; i < NMEA2K_NUMBER_RX_FAST_PACKET_SESSION; i++)
    {
        if (RxFastPacketPtr->InProgress == 1)
        {
            if (RxFastPacketPtr->DelayTimer != 0)
            {
                RxFastPacketPtr->DelayTimer--;
            }
            if (RxFastPacketPtr->DelayTimer == 0)  // Check if the session needs to be aborted
            {
                RxFastPacketPtr->InProgress = 0;
                RxFastPacketPtr->RxMsgPtr->Rx_InProgress = 0;
            }
        }
        RxFastPacketPtr++;
    }
#endif  // NMEA2K_SUPPORT_FAST_PACKET_RX

#if NMEA2K_SUPPORT_FAST_PACKET_TX
    TxFastPacketPtr = &TxFastPacket[nmea2k_u8Instance][0];
    // Update the transmit Fast-Packet timer and transmit packet if needed
    for (i = 0; i < NMEA2K_NUMBER_TX_FAST_PACKET_SESSION; i++)
    {
        if ((TxFastPacketPtr->InProgress == 1) && (TxFastPacketPtr->NextTxTimer > 0))
        {
            TxFastPacketPtr->NextTxTimer--;
        }

        TxFastPacketPtr++;
    }
#endif  // NMEA2K_SUPPORT_FAST_PACKET_TX
}

/**********************************************************************************************/
/******************************                                  ******************************/
/******************************    J1939-71 APPLICATION LAYER    ******************************/
/******************************                                  ******************************/
/**********************************************************************************************/

//-----------------------------------------------------------------------------
// Function:    Nmea2k_TxManagerTask
//-----------------------------------------------------------------------------
// Description: Send all the message (Periodic or Single ) to TX queue.
//              The message are specified by the High application level in
//              the J1939_TxMsg_Struct.
//
//              The function works only with the structure FastPacket )
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_TxManagerTask(void)
{
    typedef union
    {
        uint32 L;
        struct
        {
            uint8 B0;
            uint8 B1;
            uint8 B2;
            uint8 B3;
        } Byte;
    } RequestPGN_Struct;

    RequestPGN_Struct RequestPgnNumber;  // Used only when the Mailbox contains Request PGN Msg

    nmea2k_SingleFrame_Message_Struct_Type message;
    NMEA2K_TxMsg_Struct *Msg_nmea2k;
    uint8 i;

    // LOG(I, "Nmea2k_TxManagerTask");

    i = nmea2k_Parameter[nmea2k_u8Instance]->TxMailboxSize;
    Msg_nmea2k = nmea2k_Parameter[nmea2k_u8Instance]->TxMailbox;  // Local Pointer to the Tx Mailbox

    // SEA J1939-81, section 4.2.2.3:
    // An ECU that cannot claim an address shall not send any message other than
    // the Cannot Claim Address message or a Request for Address Claim
    if ((nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed == TRUE) &&
        (nmea2k_MyAddress[nmea2k_u8Instance] != NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS))
    {
        // LOG(I, "123");

        // Check the complete J1939 TX Mailbox and send the message if needed
        while (i > 0)
        {
            // LOG(I, "456");
            //  Check how many element are free in the Queue CAN. Two element in the Queue CAN are reserved
            //  for the communication Hand Shaking and Process. Almost two element in the Queue CAN
            //  needs to be reserved for this.
            if (HALCAN_TxFree(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort) <= NMEA2K_RESERVED_FIFO_SPACE)
            {
                // FIFO full. Stop
                break;
                // LOG(I, "break1");
            }

            // Is there a request to transmit the message?
            if (Msg_nmea2k->TxReq)
            {
                // LOG(I, "Tx1");
                //  Call the TX handler if it exists
                if (Msg_nmea2k->Handle != NULL)
                {
                    Msg_nmea2k->Handle(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, Msg_nmea2k);
                }

                // Clear the TX request
                Msg_nmea2k->TxReq = NMEA2K_TXREQ_NONE;
            }
            // Multi-packet transmit not already in progress?
            if ((Msg_nmea2k->TxReady) && (Msg_nmea2k->TxInProgress == 0))
            {
                // LOG(I, "Tx2");
#if NMEA2K_SUPPORT_FAST_PACKET_TX
                if (Msg_nmea2k->Pgn_Type == 1)  // Check for Fast Packet Message
                {
                    (void)Nmea2k_Dll_TxFastPacket_Init(Msg_nmea2k);
                }
                else
#endif  // NMEA2K_SUPPORT_FAST_PACKET_TX

#if NMEA2K_SUPPORT_MULTIPACKET
                    if (Msg_nmea2k->DataSize > 8)  // Check the Size of the Message
                {
                    // This needs the Multi Packet Protocol
                    if (Nmea2k_Dll_TxMultiPacket_Init(Msg_nmea2k) == TRUE)
                    {
                        Msg_nmea2k->TxInProgress = 1;
                    }
                }
                else
#endif  // NMEA2K_SUPPORT_MULTIPACKET

                    if (Msg_nmea2k->PGN == NMEA2K_REQUEST_PGN)
                    {
#if CONFIG_RVC_US
                        RequestPgnNumber.Byte.B3 = *(Msg_nmea2k->Data + 3);
#else
                    RequestPgnNumber.Byte.B3 = 0;
#endif
                        RequestPgnNumber.Byte.B2 = *(Msg_nmea2k->Data + 2);
                        RequestPgnNumber.Byte.B1 = *(Msg_nmea2k->Data + 1);
                        RequestPgnNumber.Byte.B0 = *(Msg_nmea2k->Data);

                        if (Nmea2k_Tx_ISORequest(Msg_nmea2k->Dest_Addr, nmea2k_MyAddress[nmea2k_u8Instance], RequestPgnNumber.L) == TRUE)
                        {
                            Msg_nmea2k->TxReady = FALSE;  // Success PGN frame transmission, Clear Transmit Flag
                        }
                    }
                    else
                    {
                        // LOG(I, "Tx4");
                        //  The message size is lower than 8 Bytes, Simply transmit it as a single MSG
                        message.BytePtr = Msg_nmea2k->Data;
                        message.ByteCntr = (uint8)Msg_nmea2k->DataSize;
                        // J1939 29-bit identifier fields
                        message.MsgId.Identifier = Msg_nmea2k->PGN << 8;
                        message.MsgId.IdField.BitField.Priority = (Msg_nmea2k->Priority & 0x07);
                        message.MsgId.IdField.SourceAddress = nmea2k_MyAddress[nmea2k_u8Instance];
                        if (message.MsgId.IdField.PduFormat < 240)
                        {
                            message.MsgId.IdField.PduSpecific = Msg_nmea2k->Dest_Addr;
                        }

                        // Valid PGN frame transmission state trap...
                        if (Nmea2k_Dll_TxSingleFrameMessage(&message) == TRUE)
                        {
                            // LOG(I, "Tx5");
                            Msg_nmea2k->TxReady = FALSE;  // Success PGN frame transmission, Clear Transmit Flag
                            if (l_dgn_tx_ready_cb)
                            {
                                l_dgn_tx_ready_cb(Msg_nmea2k->PGN, DGN_STANDARD_MSG_TX_READY);
                            }
                        }
                        else
                        {
                            break;  // The Mailbox is Full, stop to Fill it
                        }
                    }
            }
            i--;
            Msg_nmea2k++;
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_RxManagerTask
//-----------------------------------------------------------------------------
// Description: Read all messages received in the RX queue.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_RxManagerTask(void)
{
    HALCAN_zMSG message;       // Message Queue Element
    nmea2k_Id_Struct_Type id;  // CAN Message id
    uint32 pgn;
    uint8 dsr;

#if NMEA2K_SUPPORT_MULTIPACKET
    uint8 Control_Byte;  // Used if the PGN received is Connection Management (60416)
#endif

    // Scan the Received Data Queue
    for (dsr = 0; dsr < UINT8_MAX; dsr++)
    {
        if (HALCAN_Read(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, &message) == FALSE)
        {
            break;  // No more message to read
        }

        // Handle standard message
        if (message.eIdType == HALCAN_IDTYPE_STANDARD)
        {
#ifdef CONNECTOR_RVC_INCLUDE_STANDARD_DGN
            if (l_std_dgn_raw_rx_cb)
            {
                l_std_dgn_raw_rx_cb(message.u32Id, message.u8Length, message.au8Data);
            }
#endif
            continue;
        }

        // Address claim "NoMessage" variable is always TRUE default message reception...
        //  ...except for PGN address claim or ISO request for PGN address claim messages
        //  ...then this flag set be FALSE (see below)
        nmea2k_AddrClaim[nmea2k_u8Instance].NoMessage = TRUE;

        // Remove source address and priority bits from PGN
        id.Identifier = message.u32Id;
        pgn = (id.Identifier >> 8) & 0x0003FFFF;

        // LOG(I, "pgn = %d %d", (int32_t)pgn, nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed);

        /******************* J1939 PDU1 Format - specific messages ****************************/
        // Specific network device address arbitration trap...
        if ((id.IdField.PduFormat <= NMEA2K_MAX_PF_PDU1FORMAT) && ((id.IdField.PduSpecific == nmea2k_MyAddress[nmea2k_u8Instance]) || (id.IdField.PduSpecific == NMEA2K_GLOBAL_ADDRESS) || ((id.IdField.PduSpecific == 0) && (pgn == NMEA2K_ADDRESS_CLAIM_PGN))))
        {
            LOG(D, "1: pgn = %d %d %d", (int32_t)pgn, nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed, nmea2k_AddrClaim[nmea2k_u8Instance].ILose);

            // Remove destination address from specific PGN number
            pgn ^= id.IdField.PduSpecific;
            // If proprietary DGN is received, extract the DA
            if (pgn == 0xEF00)
            {
                PropDGN_DA = id.IdField.PduSpecific;
            }
            // All messages can be received if device is able to claim an address
            if (nmea2k_AddrClaim[nmea2k_u8Instance].ILose == FALSE)
            {
                // Supported addressable destination PGN numbers arbitration trap...
                switch (pgn)
                {
                case NMEA2K_REQUEST_PGN:
                    if (nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed == TRUE)
                    {
                        Nmea2k_Rx_ISORequest(&message);
                        (void)Nmea2k_DLL_Fill_nmea2kRxMailbox(&message, pgn, id.IdField.SourceAddress, id.IdField.PduSpecific);
                    }
                    break;
                    // case NMEA2K_ACKNOWLEDGMENT_PGN:
                    //     Nmea2k_Rx_ISOAcknowledgment(message);
                    //     break;
                case NMEA2K_ADDRESS_CLAIM_PGN:
                    LOG(D, "got ISO/RVC address claim!!!!");
                    Nmea2k_Rx_ISOAddressClaim(&message);
                    (void)Nmea2k_DLL_Fill_nmea2kRxMailbox(&message, pgn, id.IdField.SourceAddress, id.IdField.PduSpecific);
                    break;

#if NMEA2K_SUPPORT_MULTIPACKET
                case NMEA2K_MULTI_PACKET_TPCM_PGN:
                    Control_Byte = message.au8Data[0];  // Determine which type of Connection Message
                    if ((Control_Byte == NMEA2K_MULTI_PACKET_TPCMRTS) && (id.IdField.PduSpecific == nmea2k_MyAddress[nmea2k_u8Instance]))
                    {
                        Nmea2k_Rx_TP_CM_RTS(&message, id.IdField.SourceAddress);
                    }
                    else if ((Control_Byte == NMEA2K_MULTI_PACKET_TPCONNABORT) && (id.IdField.PduSpecific == nmea2k_MyAddress[nmea2k_u8Instance]))
                    {
                        Nmea2k_Rx_TP_Conn_Abort(&message, id.IdField.SourceAddress);
                    }
                    else if (Control_Byte == NMEA2K_MULTI_PACKET_TPCMBAM)
                    {
                        Nmea2k_Rx_TP_CM_BAM(&message, id.IdField.SourceAddress);
                    }
                    else if ((Control_Byte == NMEA2K_MULTI_PACKET_TPCMCTS) && (id.IdField.PduSpecific == nmea2k_MyAddress[nmea2k_u8Instance]))
                    {
                        Nmea2k_Rx_TP_CM_CTS(&message, id.IdField.SourceAddress);
                    }
                    else if ((Control_Byte == NMEA2K_MULTI_PACKET_ENOFMSGACK) && (id.IdField.PduSpecific == nmea2k_MyAddress[nmea2k_u8Instance]))
                    {
                        Nmea2k_Rx_TP_CM_EndOfMsgACK(&message, id.IdField.SourceAddress);
                    }
                    break;
                case NMEA2K_MULTI_PACKET_TPDT_PGN:
                    Nmea2k_Rx_DataTransfert(&message, id.IdField.SourceAddress, id.IdField.PduSpecific);
                    break;
#endif  // NMEA2K_SUPPORT_MULTIPACKET

                default:
                    // Go check if this message is supported and process it if needed
                    // In any case, the module doesn't answer
                    (void)Nmea2k_DLL_Fill_nmea2kRxMailbox(&message, pgn, id.IdField.SourceAddress, id.IdField.PduSpecific);
                    break;
                }
            }
            else if (pgn == NMEA2K_REQUEST_PGN)
            {
                // Only this message can be received if device cannot claim an address
                Nmea2k_Rx_ISORequest(&message);
            }
        }

        //------------------- J1939 PDU2 Format - global messages ------------------------------
        // All messages can be received if device is able to claim an address
        else if ((nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed == TRUE) && (id.IdField.PduFormat > NMEA2K_MAX_PF_PDU1FORMAT))
        {
            LOG(D, "2: pgn = %d %d %d", (int32_t)pgn, nmea2k_AddrClaim[nmea2k_u8Instance].AddressClaimed, nmea2k_AddrClaim[nmea2k_u8Instance].ILose);

            // Go check if this message is supported and process it if needed
            (void)Nmea2k_DLL_Fill_nmea2kRxMailbox(&message, pgn, id.IdField.SourceAddress, 0xFF);
        }
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_DLL_Fill_nmea2kRxMailbox
//-----------------------------------------------------------------------------
// Description: The function check if the received PGN is supported by the application.
//              If the J1939 Rx Mailbox is big enough, the message is transfered
//-----------------------------------------------------------------------------
// Return:      TRUE  - The Message is not defined in the Mailbox OR was updated correctly OR
//                      was filtered out.
//              FALSE - The Message is define in the MailBox but failed to update.
//-----------------------------------------------------------------------------
static uint8 Nmea2k_DLL_Fill_nmea2kRxMailbox(HALCAN_zMSG *Msg_Can, uint32 pgn, uint8 SA, uint8 DA)
{
    NMEA2K_RxMsg_Struct *Msg_nmea2k;
    uint8 i, j;

    Msg_nmea2k = nmea2k_Parameter[nmea2k_u8Instance]->RxMailbox;

    for (i = 0; i < nmea2k_Parameter[nmea2k_u8Instance]->RxMailboxSize; i++)
    {
        // PGN found in the MailBox?
        if (pgn == (Msg_nmea2k->PGN & 0x0003FFFF))
        {
            // LOG(I, "11");
            //  Mailbox disabled?
            if (Msg_nmea2k->Enable == 0)
            {
                // Mailbox found but it is disabled. Stop the loop.
                break;
            }
            // Fast packet message?
            else if (Msg_nmea2k->Pgn_Type == 1)
            {
#if NMEA2K_SUPPORT_FAST_PACKET_RX
                if (Nmea2k_Dll_RxFastPacketMsg(Msg_Can, Msg_nmea2k, SA, DA))
                {
                    // Message buffer was found. Stop the loop.
                    break;
                }
#endif  // NMEA2K_SUPPORT_FAST_PACKET_RX
            }
            // No multipacket receive in progress?
            else if (Msg_nmea2k->Rx_InProgress == 0)
            {
                // LOG(I, "12");
                //  Address filter matches?
                if ((Msg_nmea2k->Org_Filter == 0xFF) || (Msg_nmea2k->Org_Filter == SA))
                {
                    // LOG(I, "123");

                    // Reception buffer is big enough?
                    if (Msg_nmea2k->BufferSize >= Msg_Can->u8Length)
                    {
                        Msg_nmea2k->MsgSize = Msg_Can->u8Length;
                        for (j = 0; j < Msg_Can->u8Length; j++)  // Transfer the Data
                        {
                            Msg_nmea2k->Data[j] = Msg_Can->au8Data[j];
                        }
                        // Message reception complete
                        Msg_nmea2k->Dst_Addr = DA;
                        Msg_nmea2k->Org_Addr = SA;
                        Msg_nmea2k->Ageing_Ctr = 0;  // Reset the Aging Counter

                        // Call the reception handler if it exists
                        if (Msg_nmea2k->Handle != NULL)
                        {
                            // LOG(I, "1234");

                            Msg_nmea2k->Handle(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, Msg_nmea2k);
                        }
                        // Message buffer was found. Stop the loop.
                        break;
                    }
                }
            }
        }
        else if ((i == (nmea2k_Parameter[nmea2k_u8Instance]->RxMailboxSize - 1)) && (Msg_nmea2k->PGN == 0x0003FFFF))
        {
            // Manage default handling, must be last in mailbox
            // Reception buffer is big enough?
            if (Msg_nmea2k->BufferSize >= Msg_Can->u8Length)
            {
                Msg_nmea2k->MsgSize = Msg_Can->u8Length;
                for (j = 0; j < Msg_Can->u8Length; j++)  // Transfer the Data
                {
                    Msg_nmea2k->Data[j] = Msg_Can->au8Data[j];
                }
                // Message reception complete
                Msg_nmea2k->Dst_Addr = DA;
                Msg_nmea2k->Org_Addr = SA;
                Msg_nmea2k->Ageing_Ctr = 0;  // Reset the Aging Counter

                // Call the reception handler if it exists
                if (Msg_nmea2k->Handle != NULL)
                {
                    // LOG(I, "1234");
                    uint32_t temp_dgn = Msg_nmea2k->PGN;
                    Msg_nmea2k->PGN = pgn;
                    Msg_nmea2k->Handle(nmea2k_Parameter[nmea2k_u8Instance]->u8CanPort, Msg_nmea2k);
                    Msg_nmea2k->PGN = temp_dgn;
                }
                // Message buffer was found. Stop the loop.
                break;
            }
        }
        Msg_nmea2k++;
    }

    // Function always returns true
    // FIXME: Remove obsolete return?
    return TRUE;
}

#if NMEA2K_SUPPORT_MULTIPACKET
//-----------------------------------------------------------------------------
// Function:    Nmea2k_DLL_Check_nmea2kRxMailbox_MultiPacket
//-----------------------------------------------------------------------------
// Description: The function check if the received PGN is supported by the application
//-----------------------------------------------------------------------------
// Return:      If the PGN is supported
//                        AND
//              If The reception of the PGN is not in progress
//                        AND
//              IF (The PGN can Be overwrite OR The buffer is Empty)
//
//              The function return a pointer to Mailbox element
//              and if not it return NULL
//-----------------------------------------------------------------------------
static NMEA2K_RxMsg_Struct *Nmea2k_DLL_Check_nmea2kRxMailbox_MultiPacket(uint32_t pgn, uint8 SA)
{
    NMEA2K_RxMsg_Struct *Msg_nmea2k;
    uint8 i;

    Msg_nmea2k = nmea2k_Parameter[nmea2k_u8Instance]->RxMailbox;

    for (i = 0; i < nmea2k_Parameter[nmea2k_u8Instance]->RxMailboxSize; i++)
    {
        if (pgn == Msg_nmea2k->PGN)
        {
            if (Msg_nmea2k->Rx_InProgress == 0)
            {
                // Verify the Data Originator Filter
                if ((Msg_nmea2k->Org_Filter == 0xFF) || (Msg_nmea2k->Org_Filter == SA))
                {
                    return Msg_nmea2k;  // Stop the Loop For, the message buffer was find
                }
            }
        }
        Msg_nmea2k++;
    }
    return NULL;
}
#endif  // NMEA2K_SUPPORT_MULTIPACKET

//-----------------------------------------------------------------------------
// Function:    Nmea2k_DLL_Upd_AgeingCtr_nmea2kRxMailbox
//-----------------------------------------------------------------------------
// Description: Increase the Aging Counter of all the element in the j1939RX Mailbox
//              The Aging Counter is stack at 255.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_DLL_Upd_AgeingCtr_nmea2kRxMailbox(void)
{
    NMEA2K_RxMsg_Struct *Msg_nmea2k;
    uint8 i;

    Msg_nmea2k = nmea2k_Parameter[nmea2k_u8Instance]->RxMailbox;

    for (i = 0; i < nmea2k_Parameter[nmea2k_u8Instance]->RxMailboxSize; i++)
    {
        if (Msg_nmea2k->Ageing_Ctr < 255)
        {
            Msg_nmea2k->Ageing_Ctr++;
        }
        Msg_nmea2k++;
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_DLL_Update_TxCounter
//-----------------------------------------------------------------------------
// Description: This function is used to update the different counter used for the periodic
//              transmission of PGN. This counter are a part of the j1939_TxMsg structure
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
static void Nmea2k_DLL_Update_TxCounter(void)
{
    NMEA2K_TxMsg_Struct *Msg_nmea2k;
    uint8 i;

    i = nmea2k_Parameter[nmea2k_u8Instance]->TxMailboxSize;
    Msg_nmea2k = nmea2k_Parameter[nmea2k_u8Instance]->TxMailbox;  // Local Pointer to the Tx Mailbox

    while (i > 0)
    {
        if (Msg_nmea2k->Period == TRUE)  // Check if it is a periodic message
        {
            if (Msg_nmea2k->Cntr == 0)
            {
                Msg_nmea2k->TxReq = NMEA2K_TXREQ_PERIODIC;  // IF counter is elapsed (zero), set transmit request
                Msg_nmea2k->Cntr = Msg_nmea2k->CntrSet;     // flag and reload counter set value
            }
            if (Msg_nmea2k->Cntr != 0)  // Decrement Counter if it is higher than 0
            {
                Msg_nmea2k->Cntr--;
            }
        }
        i--;
        Msg_nmea2k++;
    }
}

//-----------------------------------------------------------------------------
// Function:    Nmea2k_Dll_SetTx_nmea2kTxMailbox
//-----------------------------------------------------------------------------
// Description: This function check the J1939 Tx Mailbox to see if the requested PGN
//              is supported. If yes, a single transmission Flag is set.
//-----------------------------------------------------------------------------
// Return:      TRUE  - The PGN is supported and the transmission will occurs
//              FALSE - PGN is not Supported
//-----------------------------------------------------------------------------
static uint8 Nmea2k_Dll_SetTx_nmea2kTxMailbox(uint32 PGN_Number, uint8 u8ReplyDestAddr)
{
    NMEA2K_TxMsg_Struct *Msg_nmea2k;
    uint8 i;
    uint8 j = FALSE;

    i = nmea2k_Parameter[nmea2k_u8Instance]->TxMailboxSize;
    Msg_nmea2k = nmea2k_Parameter[nmea2k_u8Instance]->TxMailbox;  // Local Pointer to the Tx Mailbox

    while (i > 0)  // Check the complete J1939 TX Mailbox
    {
        // Note: The next IF contains a AND. This is used to remove
        //       the Priority field from the identifier
        if (PGN_Number == (Msg_nmea2k->PGN & 0x0003FFFF))
        {
            Msg_nmea2k->TxReq = NMEA2K_TXREQ_EVENT;                     // The PGN is supported, Set the Request
            Msg_nmea2k->Req_Addr = nmea2k_Answered[nmea2k_u8Instance];  // In case we must ACK or NACK the request
            Msg_nmea2k->Dest_Addr = u8ReplyDestAddr;
            j = TRUE;  // Set the Return Flag to TRUE
        }
        i--;
        Msg_nmea2k++;
    }
    return j;  // Return TRUE or FALSE depending if the PGN will be transmitted or not
}
