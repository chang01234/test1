/*!
    \file   mps.h
    \brief  Mobile Power Solutions NBUS/DDM2 conversion functions, header
    \author Jens Björnhager
*/

#ifndef MPS_H
#define MPS_H

#include <stdint.h>

#include "ddm2.h"
#include "ddm_store.h"

typedef enum MPS_INSTANCE_INDEX_81_ENUM
{
    MPS_INSTANCE_INDEX_81_MCHRG = 0,
    MPS_INSTANCE_INDEX_81_MCHRGH,
    MPS_INSTANCE_INDEX_81_MSOLAR,
} MPS_INSTANCE_INDEX_81_ENUM;

typedef enum MPS_INSTANCE_INDEX_83_ENUM
{
    MPS_INSTANCE_INDEX_83_MCHRG = 0,
    MPS_INSTANCE_INDEX_83_MCHRGH,
} MPS_INSTANCE_INDEX_83_ENUM;

typedef enum MPS_INSTANCE_INDEX_85_ENUM
{
    MPS_INSTANCE_INDEX_85_MBAT = 0,
} MPS_INSTANCE_INDEX_85_ENUM;

typedef enum MPS_INSTANCE_INDEX_87_ENUM
{
    MPS_INSTANCE_INDEX_87_MINVERT = 0,
} MPS_INSTANCE_INDEX_87_ENUM;

typedef enum MPS_INSTANCE_INDEX_C6_ENUM
{
    MPS_INSTANCE_INDEX_C6_MCHRG = 0,
} MPS_INSTANCE_INDEX_C6_ENUM;

typedef enum MPS_INSTANCE_INDEX_82_ENUM
{
    MPS_INSTANCE_INDEX_82_HMI = 0,
} MPS_INSTANCE_INDEX_82_ENUM;

typedef enum MPS_B0_LANGUAGE
{
    MPS_B0_LANGUAGE_ITALIAN = 1,
    MPS_B0_LANGUAGE_ENGLISH,
    MPS_B0_LANGUAGE_GERMAN,
    MPS_B0_LANGUAGE_FRENCH,
    MPS_B0_LANGUAGE_SPANISH,
    MPS_B0_LANGUAGE_DUTCH,
} MPS_B0_LANGUAGE;

typedef enum MPS_B0_CHARGING_PROFILE
{
    MPS_B0_CHARGING_PROFILE_GEL = 1,
    MPS_B0_CHARGING_PROFILE_WET,
    MPS_B0_CHARGING_PROFILE_AGM1,
    MPS_B0_CHARGING_PROFILE_AGM2,
    MPS_B0_CHARGING_PROFILE_LIFEPO4_1,
    MPS_B0_CHARGING_PROFILE_LIFEPO4_2,
    MPS_B0_CHARGING_PROFILE_LIFEPO4_3,
    MPS_B0_CHARGING_PROFILE_LIFEPO4_4,
    MPS_B0_CHARGING_PROFILE_CUSTOM = 15,
} MPS_B0_CHARGING_PROFILE;

typedef MPS_B0_CHARGING_PROFILE MPS_26_CHARGING_PROFILE;

typedef enum MPS_B0_DEVICE_STATE
{
    MPS_B0_DEVICE_STATE_SLEEP = 0,
    MPS_B0_DEVICE_STATE_RUN,
    MPS_B0_DEVICE_STATE_STANDBY,
    MPS_B0_DEVICE_STATE_ERROR,
} MPS_B0_DEVICE_STATE;

typedef enum B2_BATTERY_TECHNOLOGY
{
    MPS_B2_BATTERY_TECHNOLOGY_AGM = 0,
    MPS_B2_BATTERY_TECHNOLOGY_GEL,
    MPS_B2_BATTERY_TECHNOLOGY_WET,
    MPS_B2_BATTERY_TECHNOLOGY_LITHIO,
} B2_BATTERY_TECHNOLOGY;

// Small endian; needs to be byteswapped before entering the conversion functions

// Real-time data frame types

typedef union MPS_NBUS_RAW_DATA
{
    uint8_t u8[4];
    uint16_t u16[2];
    uint32_t u32;
} PACKED MPS_NBUS_RAW_DATA;

typedef struct MPS_NBUS_01
{
    uint32_t free0 : 16;
    uint32_t voltage : 16;
} PACKED MPS_NBUS_01;

typedef struct MPS_NBUS_02
{
    uint32_t current : 16;
    uint32_t voltage : 16;
} PACKED MPS_NBUS_02;

typedef struct MPS_NBUS_07
{
    uint32_t leisure_capacity : 16;
    uint32_t start_capacity : 16;
} PACKED MPS_NBUS_07;

typedef struct MPS_NBUS_0B
{
    uint32_t free0 : 24;
    uint32_t soc : 8;
} PACKED MPS_NBUS_0B;

typedef struct MPS_NBUS_0C
{
    uint32_t free0 : 16;
    uint32_t temperature : 16;
} PACKED MPS_NBUS_0C;

typedef struct MPS_NBUS_0E
{
    uint32_t free0 : 24;
    uint32_t battery_health : 8;
} PACKED MPS_NBUS_0E;

typedef struct MPS_NBUS_10
{
    uint32_t free0 : 16;
    uint32_t temperature : 16;
} PACKED MPS_NBUS_10;

typedef struct MPS_NBUS_11
{
    uint32_t free0 : 16;
    uint32_t temperature : 16;
} PACKED MPS_NBUS_11;

typedef struct MPS_NBUS_20
{
    uint32_t free0 : 5;
    uint32_t passthrough : 1;  // bit 3 byte 4
    uint32_t free1 : 1;
    uint32_t eco_mode : 1;  // bit 1 byte 4
    uint32_t workload : 8;
    uint32_t free2 : 8;
    uint32_t output_voltage_ac : 8;
} PACKED MPS_NBUS_20;

typedef struct MPS_NBUS_26
{
    uint32_t free0 : 8;
    uint32_t charging_profile : 4;
    uint32_t free1 : 20;
} PACKED MPS_NBUS_26;

typedef struct MPS_NBUS_35
{
    uint32_t free0 : 16;
    uint32_t energy : 16;
} PACKED MPS_NBUS_35;

typedef struct MPS_NBUS_36
{
    uint32_t free0 : 16;
    uint32_t capacity_remaining : 16;
} PACKED MPS_NBUS_36;

typedef struct MPS_NBUS_54
{
    uint8_t serial_number_prefix[3];  // Contains characters, mind the order
    uint8_t sub_address;
} PACKED MPS_NBUS_54;

typedef struct MPS_NBUS_55
{
    uint32_t serial_number : 24;
    uint32_t sub_address : 8;
} PACKED MPS_NBUS_55;

typedef struct MPS_NBUS_60
{
    uint32_t free0 : 16;
    uint32_t charging_phase : 4;  // last 4 bit of second byte
    uint32_t free2 : 4;
    uint32_t silent_mode : 1;  // last bit of first byte
    uint32_t free3 : 5;
    uint32_t status : 2;  // first two bit
} PACKED MPS_NBUS_60;

typedef struct MPS_NBUS_8560  // Format of 60 from battery
{
    uint32_t free0 : 16;
    uint32_t htr : 1;
    uint32_t dcdc : 1;
    uint32_t free1 : 2;
    uint32_t free2 : 4;
    uint32_t silent_mode : 1;  // last bit of first byte
    uint32_t free3 : 5;
    uint32_t poles : 2;  // first two bit
} PACKED MPS_NBUS_8560;

typedef struct MPS_NBUS_99
{
    uint32_t free0 : 20;
    uint32_t charging_active : 1;  // fourth bit of second byte
    uint32_t error : 1;            // third bit of second byte
    uint32_t reduced_power : 1;    // second bit of second byte
    uint32_t silent_mode : 1;      // first bit of second byte
    uint32_t current : 8;
} PACKED MPS_NBUS_99;

typedef struct MPS_NBUS_A1  // Not used, taken from NET frame instead
{
    uint32_t free0 : 16;
    uint32_t firmware_version : 16;
} PACKED MPS_NBUS_A1;

typedef MPS_NBUS_A1 MPS_NBUS_A2;  // Same as MPS_NBUS_A1

// Settable data frame types

typedef struct MPS_NBUS_B0
{
    uint32_t free0 : 16;
    uint32_t charging_profile : 4;
    uint32_t language : 4;
    uint32_t free1 : 2;
    uint32_t device_state : 2;
    uint32_t ble_on : 1;
    uint32_t free2 : 1;
    uint32_t silent_mode : 1;
    uint32_t free3 : 1;
} PACKED MPS_NBUS_B0;

typedef struct MPS_NBUS_B1
{
    uint32_t second : 6;
    uint32_t minute : 6;
    uint32_t hour : 5;
    uint32_t year : 6;
    uint32_t month : 4;
    uint32_t day : 5;
} PACKED MPS_NBUS_B1;

typedef struct MPS_NBUS_B2
{
    uint32_t free : 16;
    uint32_t battery_technology : 4;
    uint32_t battery_capacity : 12;
} PACKED MPS_NBUS_B2;

typedef struct MPS_NBUS_B3
{
    uint32_t free0 : 4;
    uint32_t panel_power2 : 12;
    uint32_t free1 : 4;
    uint32_t panel_power1 : 12;
} PACKED MPS_NBUS_B3;

typedef struct MPS_NBUS_B4
{
    uint32_t free0 : 6;
    uint32_t vfloat : 10;
    uint32_t free1 : 6;
    uint32_t vabs : 10;
} PACKED MPS_NBUS_B4;

typedef struct MPS_NBUS_B5
{
    uint32_t free0 : 6;
    uint32_t maintenance : 1;
    uint32_t desulfation : 1;
    uint32_t imax : 8;
    uint32_t free1 : 6;
    uint32_t vrestart : 10;
} PACKED MPS_NBUS_B5;

//! \~ Main MPS N-BUS realtime data frame struct overlay
typedef struct MPS_NBUS_REALTIME_DATA
{
    uint8_t record_type;
    uint8_t device_address;
    uint8_t sub_address;
    uint8_t data_type;
    union
    {
        MPS_NBUS_RAW_DATA raw_data;
        MPS_NBUS_01 nbus_01;
        MPS_NBUS_02 nbus_02;
        MPS_NBUS_07 nbus_07;
        MPS_NBUS_0B nbus_0b;
        MPS_NBUS_0C nbus_0c;
        MPS_NBUS_0E nbus_0e;
        MPS_NBUS_10 nbus_10;
        MPS_NBUS_11 nbus_11;
        MPS_NBUS_20 nbus_20;
        MPS_NBUS_26 nbus_26;
        MPS_NBUS_35 nbus_35;
        MPS_NBUS_36 nbus_36;
        MPS_NBUS_54 nbus_54;
        MPS_NBUS_55 nbus_55;
        MPS_NBUS_60 nbus_60;
        MPS_NBUS_8560 nbus_8560;
        MPS_NBUS_99 nbus_99;
        MPS_NBUS_A1 nbus_a1;
        MPS_NBUS_A2 nbus_a2;
        MPS_NBUS_B0 nbus_b0;
        MPS_NBUS_B1 nbus_b1;
        MPS_NBUS_B2 nbus_b2;
        MPS_NBUS_B3 nbus_b3;
        MPS_NBUS_B4 nbus_b4;
        MPS_NBUS_B5 nbus_b5;
    };
} MPS_NBUS_REALTIME_DATA;

//! \~ MST+NET event data frame struct overlay
typedef struct MPS_NBUS_NETWORK_DATA
{
    uint8_t device_address;
    uint8_t sub_address;
    uint8_t data_type;
    uint8_t iad;
    uint16_t model;
    uint8_t firmware_version;
    uint8_t firmware_release;
    uint8_t processor;
    uint8_t flash;
    uint8_t stid;
} PACKED MPS_NBUS_NETWORK_DATA;

typedef enum MPS_NBUS_WRITE_FRAME_TYPE_ENUM
{
    MPS_NBUS_WRITE_FRAME_TYPE_IMP = 0,  //!< \~ Set frame type (APP+IMP)
    MPS_NBUS_WRITE_FRAME_TYPE_PWR,      //!< \~ Set frame type (APP+PWR)
    MPS_NBUS_WRITE_FRAME_TYPE_RPA,      //!< \~ Set frame type (APP+RPA)
} MPS_NBUS_WRITE_FRAME_TYPE_ENUM;

//! \~ MPS Set frame (APP+IMP)
typedef struct MPS_NBUS_IMP_FRAME
{
    char command[8];  //!< "APP+IMP="
    uint8_t data_type;
    union
    {
        MPS_NBUS_RAW_DATA raw_data;
        MPS_NBUS_B0 nbus_b0;
        MPS_NBUS_B1 nbus_b1;
        MPS_NBUS_B2 nbus_b2;
        MPS_NBUS_B3 nbus_b3;
        MPS_NBUS_B4 nbus_b4;
        MPS_NBUS_B5 nbus_b5;
    };
} PACKED MPS_NBUS_IMP_FRAME;

//! \~ MPS PWR frame (APP+PWR)
typedef struct MPS_NBUS_PWR_FRAME
{
    char command[8];  //!< "APP+PWR="
    uint8_t device_address;
    uint8_t sub_address;
    uint16_t data;
} PACKED MPS_NBUS_PWR_FRAME;

//! \~ MPS RPA frame (APP+RPA)
typedef struct MPS_NBUS_RPA_FRAME
{
    char command[7];  //!< "APP+RPA"
} PACKED MPS_NBUS_RPA_FRAME;

typedef struct MPS_NBUS_WRITE_FRAME
{
    MPS_NBUS_WRITE_FRAME_TYPE_ENUM frame_type;
    union
    {
        MPS_NBUS_IMP_FRAME imp_frame;
        MPS_NBUS_PWR_FRAME pwr_frame;
        MPS_NBUS_RPA_FRAME rpa_frame;
    };
} PACKED MPS_NBUS_WRITE_FRAME;

typedef struct MPS_GENERIC_FRAME_DATA
{
    uint16_t device_address;
    union
    {
        char string[8];
        uint32_t u32data[2];
        int32_t i32data[2];
    };
} MPS_GENERIC_FRAME_DATA;

typedef enum MPS_GENERIC_FRAME_DATA_ENUM
{
    MPS_GENERIC_FRAME_DATA_SERIAL_NUMBER_PREFIX,  //!< \~ 54: Serial number prefix (XXX2403090)
    MPS_GENERIC_FRAME_DATA_SERIAL_NUMBER,         //!< \~ 55: Serial number number (ACDXXXXXXX)
    MPS_GENERIC_FRAME_DATA_MODEL,                 //!< \~ MST+NET: Device model
    MPS_GENERIC_FRAME_DATA_FIRMWARE_VERSION,      //!< \~ Firmware version (X.Y.0)
    MPS_GENERIC_FRAME_DATA_DESCRIPTION,           //!< \~ Device description (MST+NET[4-6])
    MPS_GENERIC_FRAME_DATA_COUNT,                 //!< \~ Used for end the product class management
    MPS_GENERIC_FRAME_SETTING_PANEL_POWER,
    MPS_GENERIC_FRAME_SETTING_CHARGE_PROFILE
} MPS_GENERIC_FRAME_DATA_ENUM;

typedef struct MPS_CONTEXT
{
    ddm_store_t *mps_store;
    uint32_t mps_class_instance;  // Preparation for context switching
} MPS_CONTEXT;

typedef void (*NBUS_DDM2_COMPLETE_CB)(DDMP2_FRAME *const pframe, void *context);
typedef void (*NBUS_DDM2_CONVERT_CB)(const MPS_NBUS_REALTIME_DATA *const nbus, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *instance_start, const NBUS_DDM2_COMPLETE_CB complete_cb, void *context);

typedef void (*DDM2_NBUS_COMPLETE_CB)(MPS_NBUS_WRITE_FRAME *const nbus_set_frame, void *context);
typedef void (*DDM2_NBUS_CONVERT_CB)(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, const DDM2_NBUS_COMPLETE_CB complete_cb, void *context);

const char *lookup_description_from_mps_firmware_id_string(const uint32_t firmware_id);
const char *lookup_mps_firmware_id_string(const uint32_t firmware_id);
int mps_nbus_to_ddm2(const MPS_NBUS_REALTIME_DATA *const nbus_data, const MPS_CONTEXT *const mps_context, const SORTED_LIST_ENTRY *const instance_start, NBUS_DDM2_COMPLETE_CB complete_cb, void *context);
int mps_ddm2_to_nbus(const DDMP2_FRAME *const pframe, const MPS_CONTEXT *const mps_context, DDM2_NBUS_COMPLETE_CB complete_cb, void *context);
void mps_calculate(const MPS_CONTEXT *const mps_context, uint32_t parameter);

#endif  // MPS_H
