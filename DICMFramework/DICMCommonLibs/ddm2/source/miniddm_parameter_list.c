/*!
    \file miniddm_parameter_list.c
    \brief Dometic BLE Sensor Application layer definitions.

    MiniDDM parameter list data
    GENERATED FILE, DO NOT EDIT!
    Exported from "Connectivity_parameters_under_work.xlsm"
        at: 2026-01-26 10:55:25 (UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna
        by: Andreas Lundeen
*/

#include "miniddm_parameter_list.h"

const int Miniddm_size_list[MINIDDM_TYPE_COUNT+1] =
{
    1,
    2,
    4,
    -1,
    -1,
    0,
    2,
    2,
    4,
    4,
    2,
    2,
    4,
    2,
    2,
    1,
    2,
    4,
    2,
    0,
};

const int Miniddm_factor_list[MINIDDM_TYPE_COUNT+1] =
{
    1,
    1,
    1,
    0,
    0,
    0,
    100,
    100,
    1000,
    1,
    1000,
    100,
    1000,
    1,
    100,
    1,
    1,
    1,
    100,
    0,
};

const MINIDDM_SENSOR_NODE_LIST Miniddm_sensor_node_list[] =
{
    {1,(uint8_t[]){0x03,}},    //Climate
    {1,(uint8_t[]){0x04,}},    //Air Quality
    {2,(uint8_t[]){0x01,0x09,}},    //Door
    {3,(uint8_t[]){0x01,0x02,0x05,}},    //Motion
    {2,(uint8_t[]){0x06,0x07,}},    //Wire break and voltage
    {7,(uint8_t[]){0x01,0x02,0x03,0x04,0x05,0x06,0x07,}},    //Test
    {1,(uint8_t[]){0x08,}},    //Differential Pressure
    {3,(uint8_t[]){0x01,0x09,0x03,}},    //Door Climate
    {0,(uint8_t[]){}},    //Extended sensor
};

const MINIDDM_SENSOR_LIST Miniddm_sensor_list[MINIDDM_SENSOR_LIST_SIZE] =
{
    {36,0,},    //Node
    {48,36,},    //Accelerometer
    {4,84,},    //HALL sensor
    {12,88,},    //BME280
    {24,100,},    //BME680
    {4,124,},    //PIR sensor
    {9,128,},    //Wire break sensor
    {14,137,},    //Voltage sensor
    {9,151,},    //Differential pressure sensor
    {11,160,},    //HALL analog sensor
};

const MINIDDM_PARAMETER_LIST Miniddm_parameter_list[MINIDDM_PARAMETER_LIST_SIZE] =
{
    {0x01,MINIDDM_TYPE_U16,},    //Manufacturer (Node)
    {0x02,MINIDDM_TYPE_U8,},    //Product type
    {0x03,MINIDDM_TYPE_U8,},    //Node type
    {0x04,MINIDDM_TYPE_BINARY,},    //Id
    {0x05,MINIDDM_TYPE_STRING,},    //Name
    {0x06,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x07,MINIDDM_TYPE_U16,},    //Advertising interval
    {0x08,MINIDDM_TYPE_U16,},    //Advertising interval connected
    {0x09,MINIDDM_TYPE_U16,},    //Connection interval
    {0x0a,MINIDDM_TYPE_U8,},    //Slave latency
    {0x0b,MINIDDM_TYPE_BINARY,},    //GAP SHA1 hash
    {0x0c,MINIDDM_TYPE_BINARY,},    //Config blob
    {0x0d,MINIDDM_TYPE_U8,},    //Function
    {0x0e,MINIDDM_TYPE_U8,},    //Location
    {0x0f,MINIDDM_TYPE_VOID,},    //Enter OTA DFU
    {0x10,MINIDDM_TYPE_SECONDS_2,},    //Nonconnectable connection interval
    {0x11,MINIDDM_TYPE_SECONDS_2,},    //Connectable connection interval
    {0x12,MINIDDM_TYPE_BINARY,},    //GAP blob
    {0x13,MINIDDM_TYPE_SECONDS_2,},    //Update advertise interval
    {0x14,MINIDDM_TYPE_U8,},    //LED blink
    {0x15,MINIDDM_TYPE_SECONDS_2,},    //Connectable connection interval with WL
    {0x16,MINIDDM_TYPE_U8,},    //Peer delete
    {0x17,MINIDDM_TYPE_STRING,},    //Serial number
    {0x18,MINIDDM_TYPE_STRING,},    //Manufacturing date
    {0x19,MINIDDM_TYPE_STRING,},    //Manufacturer name
    {0x1a,MINIDDM_TYPE_STRING,},    //Product
    {0x1b,MINIDDM_TYPE_STRING,},    //SKU
    {0x1c,MINIDDM_TYPE_STRING,},    //Firmware version
    {0x1d,MINIDDM_TYPE_STRING,},    //PCB
    {0x1e,MINIDDM_TYPE_STRING,},    //BOM
    {0x1f,MINIDDM_TYPE_STRING,},    //Item description
    {0x20,MINIDDM_TYPE_STRING,},    //Model number
    {0x21,MINIDDM_TYPE_STRING,},    //Model name
    {0x22,MINIDDM_TYPE_STRING,},    //EAN13
    {0x23,MINIDDM_TYPE_VOLT_2,},    //Battery voltage
    {0x24,MINIDDM_TYPE_U8,},    //External power sense
    {0x01,MINIDDM_TYPE_I8,},    //Status (Accelerometer)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_G_2,},    //Value Accel X
    {0x04,MINIDDM_TYPE_G_2,},    //Value Accel Y
    {0x05,MINIDDM_TYPE_G_2,},    //Value Accel Z
    {0x06,MINIDDM_TYPE_DEGREES_2,},    //Value Longitudinal Tilt Angle
    {0x07,MINIDDM_TYPE_DEGREES_2,},    //Value Lateral Tilt Angle
    {0x08,MINIDDM_TYPE_CELSIUS_2,},    //Value Temp
    {0x09,MINIDDM_TYPE_U8,},    //Value Whoami
    {0x0a,MINIDDM_TYPE_U8,},    //Event Accel
    {0x0b,MINIDDM_TYPE_U8,},    //Send Accel X
    {0x0c,MINIDDM_TYPE_U8,},    //Send Accel Y
    {0x0d,MINIDDM_TYPE_U8,},    //Send Accel Z
    {0x0e,MINIDDM_TYPE_U8,},    //Send Tilt Angle
    {0x0f,MINIDDM_TYPE_U8,},    //Send Temp
    {0x10,MINIDDM_TYPE_SECONDS_4,},    //Sampling Period
    {0x11,MINIDDM_TYPE_U8,},    //Cal Flat Done
    {0x12,MINIDDM_TYPE_U8,},    //Cal Flat Start
    {0x13,MINIDDM_TYPE_U8,},    //Cal Tilt Done
    {0x14,MINIDDM_TYPE_U8,},    //Cal Tilt Start
    {0x15,MINIDDM_TYPE_U8,},    //Cal Tilt Position
    {0x16,MINIDDM_TYPE_U8,},    //Set Data Rate
    {0x17,MINIDDM_TYPE_U8,},    //Set Power Mode
    {0x18,MINIDDM_TYPE_U8,},    //Set Filter Bandwidth
    {0x19,MINIDDM_TYPE_U8,},    //Set Full Scale
    {0x1a,MINIDDM_TYPE_U8,},    //Set Filter Path
    {0x1b,MINIDDM_TYPE_U8,},    //Set 6D Threshold
    {0x1c,MINIDDM_TYPE_U8,},    //Set Tap Thrhld X
    {0x1d,MINIDDM_TYPE_U8,},    //Set Tap Thrhld Y
    {0x1e,MINIDDM_TYPE_U8,},    //Set Tap Thrhld Z
    {0x1f,MINIDDM_TYPE_U8,},    //Set Tap Recg Axis
    {0x20,MINIDDM_TYPE_U8,},    //Set Tap Axis Priority
    {0x21,MINIDDM_TYPE_U8,},    //Set Ff Threshold
    {0x22,MINIDDM_TYPE_U8,},    //Set Ff Duration
    {0x23,MINIDDM_TYPE_U8,},    //Set Wkup Threshold
    {0x24,MINIDDM_TYPE_U8,},    //Set Wkup Duration
    {0x25,MINIDDM_TYPE_U8,},    //Set Dt Duration
    {0x26,MINIDDM_TYPE_U8,},    //Set Dt Quiet Time
    {0x27,MINIDDM_TYPE_U8,},    //Set Dt Shock Duration
    {0x28,MINIDDM_TYPE_U8,},    //Set Tap Mode
    {0x29,MINIDDM_TYPE_U8,},    //Set Tap Recogn X
    {0x2a,MINIDDM_TYPE_U8,},    //Set Tap Recogn Y
    {0x2b,MINIDDM_TYPE_U8,},    //Set Tap Recogn Z
    {0x2c,MINIDDM_TYPE_U8,},    //Single Tap Enable
    {0x2d,MINIDDM_TYPE_U8,},    //Double Tap Enable
    {0x2e,MINIDDM_TYPE_U8,},    //Freefall Enable
    {0x2f,MINIDDM_TYPE_U8,},    //Tilt Enable
    {0x30,MINIDDM_TYPE_U8,},    //Wakeup Enable
    {0x01,MINIDDM_TYPE_I8,},    //Status (HALL sensor)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_U8,},    //Open
    {0x04,MINIDDM_TYPE_U8,},    //Send open
    {0x01,MINIDDM_TYPE_I8,},    //Status (BME280)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_CELSIUS_2,},    //Temperature
    {0x04,MINIDDM_TYPE_PERCENT_2,},    //Humidity
    {0x05,MINIDDM_TYPE_PASCAL_4,},    //Pressure
    {0x06,MINIDDM_TYPE_U8,},    //Send temperature
    {0x07,MINIDDM_TYPE_U8,},    //Send humidity
    {0x08,MINIDDM_TYPE_U8,},    //Send pressure
    {0x09,MINIDDM_TYPE_U8,},    //Oversampling temperature
    {0x0a,MINIDDM_TYPE_U8,},    //Oversampling humidity
    {0x0b,MINIDDM_TYPE_U8,},    //Oversampling pressure
    {0x0c,MINIDDM_TYPE_SECONDS_4,},    //Sampling period
    {0x01,MINIDDM_TYPE_I8,},    //Status (BME680)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_CELSIUS_2,},    //Temperature
    {0x04,MINIDDM_TYPE_PERCENT_2,},    //Humidity
    {0x05,MINIDDM_TYPE_PASCAL_4,},    //Pressure
    {0x06,MINIDDM_TYPE_U32,},    //Gas
    {0x07,MINIDDM_TYPE_U16,},    //Air Quality
    {0x08,MINIDDM_TYPE_U8,},    //IAQ
    {0x09,MINIDDM_TYPE_U16,},    //CO2
    {0x0a,MINIDDM_TYPE_U16,},    //VOC
    {0x0b,MINIDDM_TYPE_U8,},    //Stabilization status
    {0x0c,MINIDDM_TYPE_U8,},    //Run-in status
    {0x0d,MINIDDM_TYPE_U8,},    //Send temperature
    {0x0e,MINIDDM_TYPE_U8,},    //Send humidity
    {0x0f,MINIDDM_TYPE_U8,},    //Send pressure
    {0x10,MINIDDM_TYPE_U8,},    //Send gas
    {0x11,MINIDDM_TYPE_U8,},    //Send air quality
    {0x12,MINIDDM_TYPE_U8,},    //Send IAQ
    {0x13,MINIDDM_TYPE_U8,},    //Send CO2
    {0x14,MINIDDM_TYPE_U8,},    //Send VOC
    {0x15,MINIDDM_TYPE_U8,},    //Send Stabilization status
    {0x16,MINIDDM_TYPE_U8,},    //Send Run-In status
    {0x17,MINIDDM_TYPE_U8,},    //Sample rate
    {0x18,MINIDDM_TYPE_U8,},    //Sample rate gas
    {0x01,MINIDDM_TYPE_I8,},    //Status (PIR sensor)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_U8,},    //Event
    {0x04,MINIDDM_TYPE_U8,},    //Send event
    {0x01,MINIDDM_TYPE_I8,},    //Status (Wire break sensor)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_U8,},    //Sub function
    {0x04,MINIDDM_TYPE_U8,},    //Sub location
    {0x05,MINIDDM_TYPE_U8,},    //Open
    {0x06,MINIDDM_TYPE_VOLT_2,},    //ADC open
    {0x07,MINIDDM_TYPE_U8,},    //Send open
    {0x08,MINIDDM_TYPE_U8,},    //Send ADC
    {0x09,MINIDDM_TYPE_SECONDS_4,},    //Sampling period
    {0x01,MINIDDM_TYPE_I8,},    //Status (Voltage sensor)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_U8,},    //Sub function
    {0x04,MINIDDM_TYPE_U8,},    //Sub location
    {0x05,MINIDDM_TYPE_VOLT_2,},    //ADC 12V
    {0x06,MINIDDM_TYPE_VOLT_2,},    //ADC batt
    {0x07,MINIDDM_TYPE_VOLT_2,},    //ADC bilge
    {0x08,MINIDDM_TYPE_U8,},    //Vout sense
    {0x09,MINIDDM_TYPE_U8,},    //Power Status
    {0x0a,MINIDDM_TYPE_U8,},    //Send ADC 12V
    {0x0b,MINIDDM_TYPE_U8,},    //Send ADC batt
    {0x0c,MINIDDM_TYPE_U8,},    //Send ADC bilge
    {0x0d,MINIDDM_TYPE_U8,},    //Send Vout sense
    {0x0e,MINIDDM_TYPE_SECONDS_4,},    //Sampling period
    {0x01,MINIDDM_TYPE_I8,},    //Status (Differential pressure sensor)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_PASCAL_4,},    //Differential pressure
    {0x04,MINIDDM_TYPE_CELSIUS_2,},    //Temperature
    {0x05,MINIDDM_TYPE_U32,},    //Product Identifier
    {0x06,MINIDDM_TYPE_U8,},    //Send differential pressure
    {0x07,MINIDDM_TYPE_U8,},    //Send temperature
    {0x08,MINIDDM_TYPE_SECONDS_4,},    //Sampling period
    {0x09,MINIDDM_TYPE_U8,},    //Measurement command
    {0x01,MINIDDM_TYPE_I8,},    //Status (HALL analog sensor)
    {0x02,MINIDDM_TYPE_VOID,},    //Factory reset
    {0x03,MINIDDM_TYPE_MILLITESLA_2,},    //Value Magnetic Field X
    {0x04,MINIDDM_TYPE_MILLITESLA_2,},    //Value Magnetic Field Y
    {0x05,MINIDDM_TYPE_MILLITESLA_2,},    //Value Magnetic Field Z
    {0x06,MINIDDM_TYPE_U8,},    //Event
    {0x07,MINIDDM_TYPE_U8,},    //Send Mag Field X
    {0x08,MINIDDM_TYPE_U8,},    //Send Mag Field Y
    {0x09,MINIDDM_TYPE_U8,},    //Send Mag Field Z
    {0x0a,MINIDDM_TYPE_U8,},    //Send event
    {0x0b,MINIDDM_TYPE_SECONDS_4,},    //Sampling period
};
