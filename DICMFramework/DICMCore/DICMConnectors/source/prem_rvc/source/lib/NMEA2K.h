//------------------------------------------------------------------------------
// Module:      NMEA2K.h
//
//------------------------------------------------------------------------------
// Description: This module implements J1939 & NMEA2K protocol stacks, including
//              address claim, ISO requests, multi-packet and fast packet 
//              protocols. 
//------------------------------------------------------------------------------
// COPYRIGHT:   Seastar Solutions Inc.
//              3831, No 6 Road
//              Richmond, BC
//              Canada V6V 1P6
//
//              This source file and the information contained in it are 
//              confidential and proprietary to Seastar Solutions Inc. 
//              The reproduction or disclosure, in whole or in part, 
//              to anyone outside of Seastar Solutions without the written
//              approval of a SeaStar Solutions officer under a Non-Disclosure 
//              Agreement is expressly prohibited.
//
//              All rights reserved
//------------------------------------------------------------------------------
#ifndef  NMEA2K_H
#define  NMEA2K_H

//------------------------------------------------------------------------------
// Include files
//------------------------------------------------------------------------------
#include "StdDefs.h"

//------------------------------------------------------------------------------
// Public Constants
//------------------------------------------------------------------------------

#define NMEA2000                                    TRUE       // TRUE : NMEA2K is used. FALSE = J1939 is used

// Protocol stack setup definitions
#define NMEA2K_INSTANCE_COUNT                       1       // Number of NMEA2K protocol stack instances
#define NMEA2K_SUPPORT_FAST_PACKET_RX               TRUE    // Is NMEA2000 fastpacket needed
#define NMEA2K_SUPPORT_FAST_PACKET_TX               TRUE    // Is NMEA2000 fastpacket needed
#define NMEA2K_SUPPORT_MULTIPACKET                  TRUE    // Is MultiPacket Protocol needed
#define NMEA2K_COMMANDED_ADDRESS_ENABLED            FALSE   // Commanded address message can alter SA
#define NMEA2K_MAX_NUMBER_PACKET_BEFOFE_CTS         3       // Maximum number of Packet that can be received 
                                                            // or transmitted between two CTS
#define NMEA2K_NUMBER_RX_MULTI_PACKET_SESSION       4       // Number of Rx Multi Packet Session
#define NMEA2K_NUMBER_TX_MULTI_PACKET_SESSION       2       // Number of TX Multi Packet Session
#define NMEA2K_NUMBER_RX_FAST_PACKET_SESSION        6       // Needs to correspond to number of Fast Packet
                                                            // PGN defines in the NMEA 2000 Rx Mailbox
#define NMEA2K_NUMBER_TX_FAST_PACKET_SESSION        4       // Needs to correspond to number of Fast Packet
                                                            // PGN defines in the NMEA 2000 Tx Mailbox                                                        

#if (NMEA2000==TRUE)
#define  NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS          254     // NMEA2K NULL source address
#define  NMEA2K_ADDRESS_CLAIM_MAX_ADDRESS           251     // NMEA2K maximum source address
#else // J1939
#define  NMEA2K_ADDRESS_CLAIM_NULL_ADDRESS          254     // J1939 NULL source address
#define  NMEA2K_ADDRESS_CLAIM_MAX_ADDRESS           254     // J1939 maximum source address
#endif                             
#define  NMEA2K_GLOBAL_ADDRESS                      255        // Global destination address

#define NMEA2K_DATABASE_VERSION                     1301    // Version 1.301

// Types of transmit requests
#define NMEA2K_TXREQ_NONE                           0
#define NMEA2K_TXREQ_EVENT                          1
#define NMEA2K_TXREQ_PERIODIC                       2

// Product codes
#define NMEA2K_PRODUCT_CODE_TELEFLEX_PCM            11497

// NMEA2K Manufacturer codes                      
#define NMEA2K_MFG_SEASTAR                          1850
#define NMEA2K_MFG_RAYTHEON                         1851
#define NMEA2K_MFG_NAVIONICS                        1852
#define NMEA2K_MFG_JRC                              1853
#define NMEA2K_MFG_NORTHSTAR                        1854  
#define NMEA2K_MFG_FURUNO                           1855
#define NMEA2K_MFG_TRIMBLE                          1856
#define NMEA2K_MFG_SIMRAD                           1857
#define NMEA2K_MFG_LITTON                           1858
#define NMEA2K_MFG_KVASER                           1859
#define NMEA2K_MFG_METALMARINE                      1860
#define NMEA2K_MFG_VECTOR                           1861
#define NMEA2K_MFG_YAMAHA                           1862
#define NMEA2K_MFG_FIARA                            1863
#define NMEA2K_MFG_AIRMAR                           135
#define NMEA2K_MFG_MARETRON                         137
#define NMEA2K_MFG_LOWRANCE                         140
#define NMEA2K_MFG_MERCURY                          144
#define NMEA2K_MFG_NAUTIBUS_ELECTRONIC_GMBH         147
#define NMEA2K_MFG_BLUE_WATER_SYSTEMS               148
#define NMEA2K_MFG_OFFSHORE_SYSTEMS                 161
#define NMEA2K_MFG_BRP                              163
#define NMEA2K_MFG_VOLVO                            165
#define NMEA2K_MFG_YANMAR                           172   
#define NMEA2K_MFG_GARMIN                           229   
#define NMEA2K_MFG_HEMISPHERE                       88
#define NMEA2K_MFG_TRUEHEADING                      422
#define NMEA2K_MFG_NAVICO                           275

// NMEA2K class and function definitions
#define NMEA2K_CS_SYSTEM_TOOLS                      10
#define NMEA2K_CS_SAFETY_SYSTEMS                    20
#define NMEA2K_CS_INTERNETWORK_DEVICE               25
#define NMEA2K_CS_INTERNETWORK_FN_GATEWAY           130
#define NMEA2K_CS_INTERNETWORK_FN_ROUTER            140
#define NMEA2K_CS_INTERNETWORK_FN_BRIDGE            150
#define NMEA2K_CS_INTERNETWORK_FN_REPEATER          160

#define NMEA2K_CS_POWER_MANG_LIGHT_SYS              30
#define NMEA2K_CS_POWER_MANG_LIGHT_FN_SWITCH        130
#define NMEA2K_CS_POWER_MANG_LIGHT_FN_LOAD          140

#define NMEA2K_CS_STEERING_SYS                      40
#define NMEA2K_CS_STEERING_FN_FOLLOW_UP_CONTRL      130
#define NMEA2K_CS_STEERING_FN_MODE_CONTRL           140
#define NMEA2K_CS_STEERING_FN_AUTO_STEERING_CONTRL  150
#define NMEA2K_CS_STEERING_FN_HEADING_SENSORS       160
#define NMEA2K_CS_STEERING_FN_HELM                  170  // Added for EPS Helm - not listed in NMEA
#define NMEA2K_CS_STEERING_FN_JOYSTICK              180  // Added for Vessel Control - not listed in NMEA

#define NMEA2K_CS_PROPULSION_SYS                    50
#define NMEA2K_CS_PROPULSION_FN_ENG_ROOM_MONITOR    130
#define NMEA2K_CS_PROPULSION_FN_ENG_INTERFACE       140
#define NMEA2K_CS_PROPULSION_FN_ENG_CONTRL          150
#define NMEA2K_CS_PROPULSION_FN_ENG_GATEWAY         160
#define NMEA2K_CS_PROPULSION_FN_CONTRL_HEAD         170 
#define NMEA2K_CS_PROPULSION_FN_ACTUATOR            180
#define NMEA2K_CS_PROPULSION_FN_GAUGE_INTERFACE     190
#define NMEA2K_CS_PROPULSION_FN_GAUGE_LARGE         200
#define NMEA2K_CS_PROPULSION_FN_GAUGE_SMALL         210

#define NMEA2K_CS_NAV_SYS                           60
#define NMEA2K_CS_NAV_FN_SOUNDER_DEPTH              130 
#define NMEA2K_CS_NAV_FN_COMPASS                    140 
#define NMEA2K_CS_NAV_FN_GNSS                       145 
#define NMEA2K_CS_NAV_FN_LORAN_C                    150
#define NMEA2K_CS_NAV_FN_SPEED_SENSORS              155 
#define NMEA2K_CS_NAV_FN_TURN_RATE_INDICATOR        160 
#define NMEA2K_CS_NAV_FN_INTEGRATED_NAV             170 
#define NMEA2K_CS_NAV_FN_RADAR_AND_RADAR_PLOT       200 
#define NMEA2K_CS_NAV_FN_ECDIS                      205 
#define NMEA2K_CS_NAV_FN_ECS                        210 
#define NMEA2K_CS_NAV_FN_DIRECTION_FIND             220 

#define NMEA2K_CS_COMM_SYS                          70
#define NMEA2K_CS_COMM_FN_EPIRB                     130
#define NMEA2K_CS_COMM_FN_AUTO_ID_SYS               140
#define NMEA2K_CS_COMM_FN_DSC                       150
#define NMEA2K_CS_COMM_FN_DATA_RECEIVER             160
#define NMEA2K_CS_COMM_FN_SATELLITE                 170
#define NMEA2K_CS_COMM_FN_RADIO_TELE_MF_HF          180
#define NMEA2K_CS_COMM_FN_RADIO_TELE_VHF            190

#define NMEA2K_CS_INST_GEN_SYS                      80
#define NMEA2K_CS_INST_GEN_FN_TIME_DATE_SYS         130
#define NMEA2K_CS_INST_GEN_FN_VOYAGE_DATA_RECORD    140
#define NMEA2K_CS_INST_GEN_FN_INT_INSTRUMENT        150
#define NMEA2K_CS_INST_GEN_FN_GEN_PURPOSE_DISP      160
#define NMEA2K_CS_INST_GEN_FN_GEN_SENSOR_BOX        170
#define NMEA2K_CS_INST_GEN_FN_WEATHER_INST          180
#define NMEA2K_CS_INST_GEN_FN_TRANS_GEN             190
#define NMEA2K_CS_INST_GEN_FN_NMEA0183_CONVERTER    200

#define NMEA2K_CS_ENVIRO_HVAC_SYS                   90
#define NMEA2K_CS_DECK_CARGO_FISH_EQUIP_SYS         100

//  NMEA2K final alignment PGN's
#define NMEA2K_ECUProprietaryFields_PGN             0xff00
#define NMEA2K_ECUProprietary1Fields_PGN            0x1ff01
#define NMEA2K_ECUProprietaryBinary_PGN             0x1ff02 
#define NMEA2K_AddressClaim_PGN                     0xee00  
#define NMEA2K_Request_PGN                          0xea00  
#define NMEA2K_ISODataTransport_PGN                 60160
#define NMEA2K_Acknowledge_PGN                      59392
#define NMEA2K_ISOConnectMgmt_PGN                   60416
#define NMEA2K_ISODataTransport_PGN                 60160
#define NMEA2K_NMEARequest_PGN                      126208
#define NMEA2K_NMEAPGNList_PGN                      126464
#define NMEA2K_GNSSOutputPosRapidUpdate_PGN         129025  
#define NMEA2K_DirectionData_PGN                    130577  
#define NMEA2K_NavigationData_PGN                   129284  
#define NMEA2K_WaterDepth_PGN                       128267  
#define NMEA2K_EnvironmentalParam_PGN               130310  
#define NMEA2K_RudderControl_PGN                    127245  
#define NMEA2K_SmallCraftStatus_PGN                 130576  
#define NMEA2K_SpeedHeadingRapidUpdate_PGN          128259  
#define NMEA2K_SetDriftRapidUpdate_PGN              129291  
#define NMEA2K_GNSSPositionData_PGN                 129029  
#define NMEA2K_CrossTrackError_PGN                  129283  
#define NMEA2K_GNSSControlStatus_PGN                129538  
#define NMEA2K_GNSSSatsInView_PGN                   129540  
#define NMEA2K_DynamicTransmissionStatus_PGN        127493  
#define NMEA2K_VesselHeading_PGN                    127250  
#define NMEA2K_TimeToFromMark_PGN                   129301  
#define NMEA2K_TimeandDate_PGN                      129033  
#define NMEA2K_BearingDistBetweenTwoMarks_PGN       129302  
#define NMEA2K_DistanceLog_PGN                      128275  
#define NMEA2K_COGSOGRapidUpdate_PGN                129026  
#define NMEA2K_ProductInformation_PGN               126996  
#define NMEA2K_BatteryStatus_PGN                    127508  
#define NMEA2K_FluidLevel_PGN_PGN                   127505  
#define NMEA2K_EngineParmsDynamicRapidUpdate_PGN    127488  
#define NMEA2K_EngineParmsDynamic_PGN               127489  
#define NMEA2K_SwitchFields_PGN                     127502  
#define NMEA2K_SwitchBankStatus_PGN                 127501  
#define NMEA2K_TripParametersSmallCraft_PGN         127497  

// No data definitions
#define NMEA2K_INT8_NO_DATA                         0x7F
#define NMEA2K_INT16_NO_DATA                        0x7FFF
#define NMEA2K_INT32_NO_DATA                        0x7FFFFFFF
#define NMEA2K_UINT1_NO_DATA                        0x1
#define NMEA2K_UINT2_NO_DATA                        0x3
#define NMEA2K_UINT3_NO_DATA                        0x7
#define NMEA2K_UINT4_NO_DATA                        0xF
#define NMEA2K_UINT5_NO_DATA                        0x1F
#define NMEA2K_UINT6_NO_DATA                        0x3F
#define NMEA2K_UINT7_NO_DATA                        0x7F
#define NMEA2K_UINT8_NO_DATA                        0xFF
#define NMEA2K_UINT10_NO_DATA                       0x3FF
#define NMEA2K_UINT16_NO_DATA                       0xFFFF
#define NMEA2K_UINT32_NO_DATA                       0xFFFFFFFF

// Error data definitions
#define NMEA2K_INT8_ERROR_DATA                      0x7E
#define NMEA2K_INT16_ERROR_DATA                     0x7FFE
#define NMEA2K_INT32_ERROR_DATA                     0x7FFFFFFE
#define NMEA2K_UINT2_ERROR_DATA                     0x2
#define NMEA2K_UINT3_ERROR_DATA                     0x6
#define NMEA2K_UINT4_ERROR_DATA                     0xE
#define NMEA2K_UINT5_ERROR_DATA                     0x1E
#define NMEA2K_UINT6_ERROR_DATA                     0x3E
#define NMEA2K_UINT7_ERROR_DATA                     0x7E
#define NMEA2K_UINT8_ERROR_DATA                     0xFE
#define NMEA2K_UINT10_ERROR_DATA                    0x3FE
#define NMEA2K_UINT16_ERROR_DATA                    0xFFFE
#define NMEA2K_UINT32_ERROR_DATA                    0xFFFFFFFE

#define NMEA2K_LEVEL_A_CERTIFICATION                0
#define NMEA2K_LEVEL_B_CERTIFICATION                1

//------------------------------------------------------------------------------
// Public types
//------------------------------------------------------------------------------
// Structure of the RX Mailbox used to exchange messages with the application
typedef struct RxMsg_Struct
{
    bit     Enable        :1;  // [ 0:Disabled , 1:Enabled ]
    bit     Trunk         :1;  // [ 0:No Trunk allowed, 1: Trunk of MultiPacket or Fast Packet Message is allowed ]
    bit     Rx_InProgress :1;  // Flag used by the NMEA 2000 Driver Only. This flag is used to avoid that
                               // a Rx PGN was overwritten by a single Packet PGN when a Multi-Packet
                               // reception is in progress;
    bit     Pgn_Type      :1;  // 0 = Regular PGN, 1 = Fast Packet PGN
    uint32  PGN;               // PGN with Group Extension (Including DataPage and Reserved)
    uint8   Ageing_Ctr;        // Aging Counter (Increment each 100ms, Stack at 0xFF, Clear at Msg Reception)
    uint16  MsgSize;           // Message Size
    uint8   Org_Addr;          // Address of the PGN Originator
    uint8   Org_Filter;        // 0xFF         => Filter is not used
                               // Others Value => The PGN is dedicated to only one Originator and work as a filter
    uint8   Dst_Addr;          // Destination Address (Can by MyAddress or 0xFF when the message is global)
    uint16  BufferSize;        // Data Buffer Size (Fixed and defined by the application)
    uint8   *Data;             // Data Buffer Pointer
    void    (*Handle)(uint8 u8CanPort, struct RxMsg_Struct *pzRxMsg); // Handle function pointer
}
NMEA2K_RxMsg_Struct;                      

// Structure of the TX Mailbox used to exchange messages with the application
typedef struct TxMsg_Struct
{
    bit     TxReq        :2;  // Transmit Request [0=No, 1=External Req, 2=Periodic Request]
    bit     TxReady      :1;  // Message ready to transmit
    bit     Period       :1;  // Periodic Transmit [0:Single, 1:Periodic]
    bit     TxInProgress :1;  // Flag used when the transmission is Multi-
                              // Packet. This flag need to initialized at 0;
    bit     Pgn_Type     :1;  // 0 = Regular PGN, 1 = Fast Packet PGN
    bit     Instance     :2;  // Range = 0 - 3. 
    uint16  CntrSet;          // Transmit Period Setting (Time Base of 10mS)
    uint16  Cntr;             // Transmit trigger counter
    uint32  PGN;              // Message Identification Character;
    uint8   Priority;         // Priority of the PGN
    uint8   Dest_Addr;        // Destination Address (Needed only if message is in PDU1 Format)
    uint8   Req_Addr;         // Address of the node that sent an ISO request (Needed only for ACK abnd NACK )
    uint16  DataSize;         // Data Buffer Size
    uint8   *Data;            // Data Buffer Pointer
    void    (*Handle)(uint8 u8CanPort, struct TxMsg_Struct *pzTxMsg); // Handle function pointer
} 
NMEA2K_TxMsg_Struct;         

// Communication parameters as defined by the application
typedef struct
{
    uint8               u8CanPort;      // BSP_eCAN port used for this instance
    NMEA2K_RxMsg_Struct *RxMailbox;     // RxMailbox Address
    uint8               RxMailboxSize;  // RxMailbox Size
    NMEA2K_TxMsg_Struct *TxMailbox;     // TxMailbox Address
    uint8               TxMailboxSize;  // TxMailbox Size
    uint8               *PreferredSA;   // Preferred Source Address (This first address
                                        // that the module will try to claim at Power Up)  
    struct
    {
                                        // Parameter used in the NMEA 2000 Name Fields (See J1939-81 for details)
                                        // All the Name parameter have an associated length. Be certain to set 
                                        // this values lower or equal to the maximum.
                                        // Ex: The Industry Group is a 3 Bits fields => Max possible values = 7
        bit     Self_Cfg_Addr  :1;      // Self Configurable Address
        uint8   Industry_Group;         // Defined by the SAE Committee, see NMEA 2000 Appendix B1
        uint8   Vehicle_Sys_Inst;       // Vehicle System Instance (Indicate the occurrence of a particular Vehicle System)
        uint8   Vehicle_System;         // Defined by the SAE Committee, see NMEA 2000 Appendix B Table B12
        bit     Reserved       :1;      // Reserved for future assignment (Should be set to Zero)
        uint8   Function;               // Defined by the SAE Committee, see NMEA 2000 Appendix B Table B11
        uint8   Function_Instance;      //
        uint8   ECU_Instance;           //
        uint16  Manufacturer_Code;      // Defined by the SAE Committee, see NMEA 2000 Appendix B Table B10
        uint32  Identity_Number;        // Unique Identity Number assigned by the manufacturer
                                        // This number is unique for each module and needs to be assigned 
                                        // by the test Jig. It needs to be save in Flash.
    }
    Name_Fields;                         
} 
NMEA2K_Parameter_Struct;                

// Status communication instance
typedef struct
{
    uint8  AddClaim  :1;        // Address Claim Process (0:In Progress 1:Done)
    uint8  CAN_BusOK :1;        // CAN Bus State (0: CAN Bus is down, 1: CAN Bus is working)
    uint8  SA;                  // Current Source Address (Address used after the address claim Challenge)
                                // As a first try, the module try to claimed the address specified
                                // in J1939_Name_Struct. This register is the result of the address
                                // claim challenge.
} 
NMEA2K_Status_Struct;                    

typedef void (*address_claimed_callback_t)(void);
//------------------------------------------------------------------------------
// Public variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public functions
//------------------------------------------------------------------------------
extern bool   NMEA2K_Initialize( NMEA2K_Parameter_Struct *Parameter );
extern void   NMEA2K_UpdateTimers( uint8 u8ElapsedTime );
extern uint32 NMEA2K_ProcessRx( void );
extern void   NMEA2K_ProcessTx( void );
extern void   NMEA2K_SetAddressClaimedCallback(address_claimed_callback_t address_claimed_cb);
extern uint8  NMEA2K_GetSourceAddr( uint8 u8CanPort );
extern bool   NMEA2K_SetRxEnable( uint8 u8CanPort, uint32 u32PGN, bool bEnable);
extern bool   NMEA2K_SetRxAddrFilter( uint8 u8CanPort, uint32 u32PGN, uint8 u8AddrFilter );
extern bool   NMEA2K_SetTxRequest( uint8 u8CanPort, uint32 u32PGN, uint8 u8Instance );
extern bool   NMEA2K_SetTxPeriodic( uint8 u8CanPort, uint32 u32PGN, uint8 u8Instance, bool bEnable, uint8 u8Period );
extern bool   NMEA2K_TxISORequest( uint8 u8CanPort, uint8 u8DestAddr, uint32 u32PGN );
extern uint32 NMEA2K_ExtractPGN( uint32 MsgId );
extern uint8  NMEA2K_ExtractSA( uint32 MsgId );
extern uint8  NMEA2K_ExtractDA( uint32 MsgId );
extern bool   NMEA2K_IsAddressClaimed( uint8 u8CanPort );
extern bool   NMEA2K_TxSingleFramePGN( uint8 *pau8Data,	uint8 u8DataSize, uint32 u32PGN, uint8 u8Priority, uint8 u8SrcAddr,	uint8 u8DestAddr, uint8 u8CanPort );

#endif 

