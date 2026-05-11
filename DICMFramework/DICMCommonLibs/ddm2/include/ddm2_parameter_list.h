/*!
    \file ddm2_parameter_list.h
    \brief Application layer definitions.

    DDM2 parameter list defines
    GENERATED FILE, DO NOT EDIT!
    https://onedometic.atlassian.net/wiki/x/AwAFvQ
    Exported from "Connectivity_parameters_under_work.xlsm"
        at: 2026-01-26 10:53:51 (UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna
        by: Andreas Lundeen
*/

#ifndef DDM2_PARAMETER_LIST_H_
#define DDM2_PARAMETER_LIST_H_

#ifdef __IAR_SYSTEMS_ICC__
#include "stm8s.h"
#else
#include <stdint.h>
#endif
#include <stddef.h>
#include "sorted_list.h"
#include "ddm2.h"

#define DDM2_BIT_VAL(b) (uint32_t)(1 << b)
#define IS_DDM2_BIT_VAL_SET(a,b) (DDM2_BIT_VAL(b) & a)


#define GW0                                    0x00000000  // Gateway class
#define GW0AVL                                 0x00000000  // Available
#define GW0INV                                 0x00000001  // Inventory
#define GW0VER                                 0x00000002  // Gateway FW version
#define GW0OTA                                 0x00000003  // Over the air update
#define GW0UPT                                 0x00000004  // Uptime
#define GW0TST                                 0x00000005  // Test
#define GW0AWSSC                               0x00000006  // AWS Server Certificate PEM
#define GW0AWSCC                               0x00000007  // AWS Client Certificate PEM
#define GW0AWSCCPK                             0x00000008  // AWS Private key PEM
#define GW0AWSCA                               0x00000009  // AWS CA Certificate PEM
#define GW0OTASC                               0x0000000a  // OTA Server Certificate PEM
#define GW0OTACC                               0x0000000b  // OTA Client Certificate PEM
#define GW0OTACCPK                             0x0000000c  // OTA Private key PEM
#define GW0OTACA                               0x0000000d  // OTA CA Certificate PEM
#define GW0DSN                                 0x0000000e  // Dometic Serial Number
#define GW0SKU                                 0x0000000f  // Stock Keeping Unit
#define GW0PNC                                 0x00000010  // Product Numeric Code
#define GW0MAC                                 0x00000011  // MAC address
#define GW0THING                               0x00000012  // Thing ID
#define GW0PTYPE                               0x00000013  // Product Type
#define GW0CUPDT                               0x00000014  // Time between cloud updates
#define GW0RRSN                                0x00000015  // ESP32 reset reason
#define GW0UNUSED                              0x00000016  // UNUSED
#define GW0BATWIN                              0x00000017  // Voltage window
#define GW0TEMPWIN                             0x00000018  // Temperature window
#define GW0REMTWIN                             0x00000019  // Time window
#define GW0CUPD                                0x0000001a  // Command Utility Parameter Debug
#define GW0THTYID                              0x0000001b  // Thing type ID
#define GW0DEV1ST                              0x0000001c  // GW device 1 connected status
#define GW0ICT                                 0x0000001d  // Internet Connection Test
#define GW0OCNT                                0x0000001e  // OTA counter
#define GW0RCNT                                0x0000001f  // Restart counter
#define GW0ERRMSG                              0x00000020  // Last error message (app/gw)
#define GW0NAME                                0x00000021  // Display Name
#define GW0CONNURL                             0x00000022  // GW device connectivity URL
#define CFG0                                   0x00010000  // Configuration class
#define CFG0AVL                                0x00010000  // Available
#define CFG0DDEST                              0x00010001  // Debug server destination
#define CFG0DPORT                              0x00010002  // Debug server destination port
#define CFG0ITEMPTH                            0x00010003  // Notification Threshold Inside Temperature
#define CFG0WWTRTH                             0x00010004  // Notification Threshold Waste water
#define CFG0FWTRTH                             0x00010005  // Notification Threshold Fresh water
#define CFG0BATTH                              0x00010006  // Notification Threshold Battery
#define CFG0FWID                               0x00010007  // FW version id
#define CFG0OPATH                              0x00010008  // OTA URL path
#define CFG0OSTAT                              0x00010009  // OTA update status
#define CFG0NTWTH                              0x0001000a  // Network Thing type Id
#define CFG0LOGLVL                             0x0001000b  // Log level
#define CHGRSVC0                               0x01000000  // Charging Service class
#define CHGRSVC0AVL                            0x01000000  // Available
#define CHGRSVC0CHRGV                          0x01000001  // Charging voltage
#define CHGRSVC0BTRYV                          0x01000002  // Battery voltage
#define CHGRSVC0REMT                           0x01000003  // Hours remaining
#define IBS0                                   0x01010000  // Battery Service class
#define IBS0AVL                                0x01010000  // Available
#define IBS0I                                  0x01010001  // Charging current
#define IBS0V                                  0x01010002  // Charging voltage
#define IBS0SOC                                0x01010003  // State of charge
#define IBS0TEMP                               0x01010004  // Battery temperature
#define IBS0RCAL                               0x01010005  // Recalibrated state
#define IBS0OCGV                               0x01010006  // Optimal charging voltage
#define IBS0BATTYP                             0x01010007  // Battery type
#define IBS0SOH                                0x01010008  // State of Health
#define IBS0CNOM                               0x01010009  // Nominal Capacity (setting)
#define AC0                                    0x01020000  // Air Condition class
#define AC0AVL                                 0x01020000  // Available
#define AC0ON                                  0x01020001  // Power on or off
#define AC0FSPD                                0x01020002  // Fan speed
#define AC0MD                                  0x01020003  // Mode
#define AC0TTEMP                               0x01020004  // Target temperature
#define AC0LGT                                 0x01020005  // Light
#define AC0DMR                                 0x01020006  // Dimmer
#define AC0PWR                                 0x01020007  // Power (limit)
#define AC0MMD                                 0x01020008  // Manual Mode
#define AC0MDL                                 0x01020009  // Model
#define AC0ITEMP                               0x0102000a  // Internal temperature
#define AC0FS                                  0x0102000b  // Fan speed
#define AC0FMD                                 0x0102000c  // Fan mode
#define AC0LGTBS                               0x0102000d  // Brightness
#define AC0ELGT                                0x0102000e  // External Light
#define AC0PUR                                 0x0102000f  // Purifier
#define AC0SYSU                                0x01020010  // System units
#define AC0TSUP                                0x01020011  // Timer supported
#define AC0TMD                                 0x01020012  // Timer mode
#define AC0TONA                                0x01020013  // Timer on active
#define AC0TONH                                0x01020014  // Timer on hour
#define AC0TONM                                0x01020015  // Timer on minute
#define AC0TOFFA                               0x01020016  // Timer off active
#define AC0TOFFH                               0x01020017  // Timer off hour
#define AC0TOFFM                               0x01020018  // Timer off minute
#define AC0ERRC                                0x01020019  // Error code
#define AC0STATUS                              0x0102001a  // Error and Alert codes
#define AC0SLEEP                               0x0102001b  // Sleep mode
#define AC0HFAVL                               0x0102001c  // Heating function
#define AC0LFAVL                               0x0102001d  // Light function
#define AC0ACTEXT                              0x0102001e  // Actuators(external)
#define AC0REMCTRL                             0x0102001f  // Remote control disable
#define AC0TEST                                0x01020020  // Special test mode
#define AC0VER                                 0x01020021  // Firmware version
#define AC0OFFSET                              0x01020022  // Internal offset temperature value for target temperature
#define AC0OPERST                              0x01020023  // Operational state in auto mode
#define AC0ECO                                 0x01020024  // ECO mode
#define AC0FLAPS                               0x01020025  // Flaps status
#define AC0HTTEMP                              0x01020026  // Target temperature (Heat modes)
#define AC0ETEMP                               0x01020027  // External temperature
#define AC0HMODE                               0x01020028  // Heat modes
#define AC0FTDIFF                              0x01020029  // Furnace mode temp diff
#define AC0FEATURE                             0x0102002a  // Features available(bitmask)
#define AC0FILTER                              0x0102002b  // Filter Indication
#define AC0CURR                                0x0102002c  // Current (A)
#define AC0CURRLIM                             0x0102002d  // Limit current (A) setting
#define SYS0                                   0x01030000  // System class
#define SYS0AVL                                0x01030000  // Available
#define SYS0LINE                               0x01030001  // Line
#define SYS0MSW                                0x01030002  // Main Switch
#define SYS0NGNR                               0x01030003  // Engine running
#define SYS0ITEMP                              0x01030004  // Inside temperature
#define SYS0OTEMP                              0x01030005  // Outside temperature
#define SYS0HBTRY                              0x01030006  // House battery
#define SYS0SBTRY                              0x01030007  // Starter battery
#define SYS0DATE                               0x01030008  // Date
#define SYS0TIME                               0x01030009  // Time
#define SYS0LOGO                               0x0103000a  // logotype
#define SYS0FHTR                               0x0103000b  // Floor heater
#define SYS0HTR                                0x0103000c  // Heater
#define SYS0VER                                0x0103000d  // Version
#define LGT0                                   0x01040000  // Light class
#define LGT0AVL                                0x01040000  // Available
#define LGT0BLY                                0x01040001  // Bed 2 left
#define LGT0BRY                                0x01040002  // Bed 2 right
#define LGT0CEIL                               0x01040003  // Ceiling
#define LGT0WALL                               0x01040004  // Wall
#define LGT0BLX                                0x01040005  // Bed 1 left
#define LGT0BRX                                0x01040006  // Bed 1 right
#define LGT0WASH                               0x01040007  // Wash
#define LGT0WC                                 0x01040008  // WC
#define LGT0AMBX                               0x01040009  // ambient1
#define LGT0AMBY                               0x0104000a  // ambient2
#define LGT0AMBZ                               0x0104000b  // ambient3
#define LGT0ADDX                               0x0104000c  // addition1
#define LGT0ADDY                               0x0104000d  // addition2
#define LGT0ADDZ                               0x0104000e  // addition3
#define LGT0KTNX                               0x0104000f  // kitchen1
#define LGT0KTNY                               0x01040010  // kitchen2
#define LGT0AWN                                0x01040011  // awning
#define FWTR0                                  0x01050000  // Fresh water class
#define FWTR0AVL                               0x01050000  // Available
#define FWTR0LVL                               0x01050001  // Fresh water tank level
#define WWTR0                                  0x01060000  // Waste water class
#define WWTR0AVL                               0x01060000  // Available
#define WWTR0LVL                               0x01060001  // Waste water tank level
#define WWTRHTR0                               0x01070000  // Waste water heater class
#define WWTRHTR0AVL                            0x01070000  // Available
#define WWTRHTR0STS                            0x01070001  // Waste water heater status
#define WWTRHTR0ACT                            0x01070002  // Waste water heater active
#define CHGR0                                  0x01080000  // Charger class
#define CHGR0AVL                               0x01080000  // Available
#define CHGR0ACT                               0x01080001  // Charging active
#define CHGR0I                                 0x01080002  // Charging current
#define CHGR0SLN                               0x01080003  // Silent mode
#define CHGR0OH                                0x01080004  // Overheated
#define CHGR0ERR                               0x01080005  // Error
#define HTR0                                   0x01090000  // Heater class
#define HTR0AVL                                0x01090000  // Available
#define HTR0AON                                0x01090001  // Air on/off
#define HTR0ATEMP                              0x01090002  // Air temperature
#define HTR0WTRON                              0x01090003  // Water on/off
#define HTR0WTRTEMP                            0x01090004  // Water temperature
#define HTR0ESEL                               0x01090005  // Energy selection
#define HTR0MMD                                0x01090006  // Manual mode
#define HTR0MDL                                0x01090007  // Model
#define HTR0TEMP                               0x01090008  // Temperature value
#define HTR0EL                                 0x01090009  // El Off/1kW/2kW/3kW/0.5kW
#define HTR0GAS                                0x0109000a  // Gas on/off
#define HTR0AMD                                0x0109000b  // Air Heater Mode
#define HTR0SMAXFAN                            0x0109000c  // Silent mode max fan speed
#define HTR0VMINFAN                            0x0109000d  // Ventilation mode min fan speed
#define HTR0ERRST                              0x0109000e  // Error status
#define HTR0ERRCD1                             0x0109000f  // Active error code1
#define HTR0ERRCD2                             0x01090010  // Active error code2
#define HTR0ERRCD3                             0x01090011  // Active error code3
#define HTR0ERRCD4                             0x01090012  // Active error code4
#define HTR0UVTH                               0x01090013  // Under voltage threshold
#define HTR0SYSU                               0x01090014  // System units
#define HTR0AHTOFFST                           0x01090015  // Air heater timer off status
#define HTR0AHTOFFH                            0x01090016  // Air heater timer off (hour)
#define HTR0AHTOFFM                            0x01090017  // Air heater timer off (min)
#define HTR0AHTONST                            0x01090018  // Air heater timer in status
#define HTR0AHTONH                             0x01090019  // Air heater timer on (hour)
#define HTR0AHTONM                             0x0109001a  // Air heater timer on (min)
#define HTR0WTRTST                             0x0109001b  // Water heater timer status
#define HTR0WTRTONH                            0x0109001c  // Water heater timer on (hour)
#define HTR0WTRTONM                            0x0109001d  // Water heater timer on (min)
#define HTR0WTRTKET                            0x0109001e  // Water heater timer keep on time (min)
#define HTR0ACST                               0x0109001f  // AC status
#define HTR0DATEY                              0x01090020  // Date: Year
#define HTR0DATEM                              0x01090021  // Date: Month
#define HTR0DATED                              0x01090022  // Date: Day
#define HTR0WEEKD                              0x01090023  // Day of week
#define HTR0TIMEH                              0x01090024  // Time: Hour
#define HTR0TIMEM                              0x01090025  // Time: Minute
#define HTR0TIMES                              0x01090026  // Time: Second
#define HTR0TTZ                                0x01090027  // Timezone
#define HTR0ACWTRHST                           0x01090028  // AC water heater status
#define HTR0GASWTRHST                          0x01090029  // Gas water heater status
#define HTR0WTRTS                              0x0109002a  // Water temperature status
#define HTR0RTS                                0x0109002b  // Room temperature
#define HTR0CVER                               0x0109002c  // Comfort MCU version
#define HTR0BVER                               0x0109002d  // Burner MCU version
#define HTR0PCBA                               0x0109002e  // PCBA version
#define HTR0PROT                               0x0109002f  // Protocol version
#define FRG0                                   0x010a0000  // Fridge class
#define FRG0AVL                                0x010a0000  // Available
#define FRG0MD                                 0x010a0001  // Mode
#define FRG0LVL                                0x010a0002  // Level
#define FRG0FHTR                               0x010a0003  // Frame heater?
#define FRG0DRST                               0x010a0004  // Door status
#define FRG0ERRST                              0x010a0005  // Error status
#define SATKAT0                                0x010b0000  // Kathrein satellite class
#define SATKAT0AVL                             0x010b0000  // Available
#define SATKAT0MDL                             0x010b0001  // Model
#define SATKAT0STS                             0x010b0002  // Status
#define SATKAT0CMD                             0x010b0003  // Command
#define SATKAT0LAT                             0x010b0004  // Latitude
#define SATKAT0LON                             0x010b0005  // Longitude
#define SATKAT0OPOS                            0x010b0006  // Orbital position
#define HD0                                    0x010c0000  // Home Delivery class
#define HD0AVL                                 0x010c0000  // Available
#define HD0DSN                                 0x010c0001  // Dometic Serial Number
#define HD0SKU                                 0x010c0002  // Stock Keeping Unit
#define HD0PNC                                 0x010c0003  // Product Numeric Code
#define HD0FWVER                               0x010c0004  // Firmware version
#define HD0ON                                  0x010c0005  // Power on
#define HD0SETTEMP                             0x010c0006  // Temperature control
#define HD0OFS                                 0x010c0007  // Temperature offset
#define HD0ITEMP                               0x010c0008  // Temperature inside
#define HD0OTEMP                               0x010c0009  // Temperature outside
#define HD0MBDOOR                              0x010c000a  // Mailbox Door Open
#define HD0CBDOOR                              0x010c000b  // Cabinet Door Open
#define HD0MBLOCK                              0x010c000c  // Mailbox Lock Open
#define HD0CBLOCK                              0x010c000d  // Cabinet Lock Open
#define HD0ERR                                 0x010c000e  // Error status
#define HD0ALRT                                0x010c000f  // Alert status
#define HD0AC                                  0x010c0010  // AC power connected
#define HD0BAT                                 0x010c0011  // Battery connected
#define HD0CHRG                                0x010c0012  // Battery charging
#define HD0PTYPE                               0x010c0013  // Product Type
#define HD0COMPSTAT                            0x010c0014  // Compressor status
#define HD0HTRSTAT                             0x010c0015  // Heater status
#define HD0TEMPCM                              0x010c0016  // Temp ctrl mode
#define HD0TCMIN                               0x010c0017  // Temp ctrl min limit
#define HD0TCMAX                               0x010c0018  // Temp ctrl max limit
#define HD0MBLOCKS                             0x010c0019  // Mailbox Lock Status
#define HD0CBLOCKS                             0x010c001a  // Cabinet Lock Status
#define HD0BATV                                0x010c001b  // Battery voltage
#define HD0BATPROT                             0x010c001c  // Battery protection
#define HD0SETTEMP2                            0x010c001d  // Temperature control
#define HD0ITEMP2                              0x010c001e  // Temperature inside
#define HD0TEMPCM2                             0x010c001f  // Temp ctrl mode
#define HD0STATUS                              0x010c0020  // Error and Alert codes
#define HD0BATCRR                              0x010c0021  // Current in mA
#define GB0                                    0x010d0000  // Gas bottle class
#define GB0AVL                                 0x010d0000  // Available
#define GB0ETY                                 0x010d0001  // Empty status
#define MDM0                                   0x02000000  // Modem Network class
#define MDM0AVL                                0x02000000  // Available
#define MDM0VER                                0x02000001  // Version
#define MDM0RSSI                               0x02000002  // RSSI
#define MDM0IMSI                               0x02000003  // IMSI
#define MDM0IMEI                               0x02000004  // IMEI
#define MDM0AT                                 0x02000005  // AT command string
#define WIFI0                                  0x02010000  // WiFi manager class
#define WIFI0AVL                               0x02010000  // Available
#define WIFI0SCAN                              0x02010001  // Scan
#define WIFI0ADD                               0x02010002  // Add
#define WIFI0STS                               0x02010003  // Status
#define WIFI0CNW                               0x02010004  // Current network
#define WIFI0ON                                0x02010005  // Radio on
#define WIFI0EV                                0x02010006  // Event
#define WIFI0MD                                0x02010007  // Mode
#define WFWL0                                  0x02020000  // WiFi Network White List class
#define WFWL0AVL                               0x02020000  // Available
#define WFWL0SSID                              0x02020001  // SSID
#define WFWL0PW                                0x02020002  // Password
#define WFWL0DEL                               0x02020003  // Delete
#define BT0                                    0x02030000  // Bluetooth manager class
#define BT0AVL                                 0x02030000  // Available
#define BT0SCAN                                0x02030001  // Scan
#define BT0ADDWL                               0x02030002  // Add to whitelist
#define BT0PAIR                                0x02030003  // Pair
#define BT0ADDGAP                              0x02030004  // Add to whitelist (GAP only)
#define BT0ADVINTVL                            0x02030005  // GAP Advertising interval
#define BT0CLIENTS                             0x02030006  // Active BLE peripheral clients
#define BT0ON                                  0x02030007  // Turn BLE on/off
#define BTWL0                                  0x02040000  // Bluetooth whitelist class
#define BTWL0AVL                               0x02040000  // Available
#define BTWL0ADDRTP                            0x02040001  // Address type
#define BTWL0ADDR                              0x02040002  // Address
#define BTWL0DEL                               0x02040003  // Delete
#define BTWL0CNCT                              0x02040004  // Connect
#define BTWL0NODEI                             0x02040005  // BLE node instance
#define BTWL0SNS                               0x02040006  // BLE Sensor types
#define BTWL0SNSI                              0x02040007  // BLE sensor instances
#define BTWL0DCNCT                             0x02040008  // Disconnect
#define BTWL0NODEC                             0x02040009  // BLE node device DDM2 class
#define BTWL0RSSI                              0x0204000a  // Device connection RSSI
#define BTWL0PWD                               0x0204000b  // Password
#define SMS0                                   0x02050000  // SMS class
#define SMS0AVL                                0x02050000  // Available
#define SMS0NUM                                0x02050001  // Phone Number
#define SMS0MSG                                0x02050002  // Text message to send
#define GNSS0                                  0x02060000  // GNSS class
#define GNSS0AVL                               0x02060000  // Available
#define GNSS0LAT                               0x02060001  // Latitude
#define GNSS0LON                               0x02060002  // Longitude
#define GNSS0HDOP                              0x02060003  // HDOP
#define GNSS0ALT                               0x02060004  // Altitude
#define GNSS0FIX                               0x02060005  // Fix
#define GNSS0COG                               0x02060006  // COG
#define GNSS0SPKN                              0x02060007  // spkn
#define GNSS0DATE                              0x02060008  // Date
#define GNSS0NUMSAT                            0x02060009  // Number of satellites
#define MQTT0                                  0x02070000  // MQTT class
#define MQTT0AVL                               0x02070000  // Available
#define MQTT0STAT                              0x02070001  // Status
#define MQTT0TXKB                              0x02070002  // Tx kilobytes
#define MQTT0UPDATE                            0x02070003  // Update cloud
#define MQTT0PROVTMPLT                         0x02070004  // Provisioning template name
#define MQTT0CONNECT                           0x02070005  // Connect is allowed
#define UDEV0                                  0x02080000  // UART device class
#define UDEV0AVL                               0x02080000  // Available
#define UDEV0VER                               0x02080001  // Version
#define UDEV0SNR                               0x02080002  // Serial number
#define UDEV0BR                                0x02080003  // Brand
#define SD0                                    0x02090000  // Service discovery class
#define SD0AVL                                 0x02090000  // Available
#define SD0ACT                                 0x02090001  // Local service discovery is active
#define SD0LIST                                0x02090002  // List of discovered devices
#define SD0WLIST                               0x02090003  // Whitelist of required devices to connect to
#define SD0SSID                                0x02090004  // SoftAP credentials used for local network
#define BTC0                                   0x020a0000  // Bluetooth clients class
#define BTC0AVL                                0x020a0000  // Available
#define BTC0CONN                               0x020a0001  // Connected
#define BTC0NAME                               0x020a0002  // Device name
#define BTC0DEL                                0x020a0003  // Delete
#define TLS0                                   0x020b0000  // TLS support class
#define TLS0AVL                                0x020b0000  // Available
#define TLS0DEL                                0x020b0001  // Delete the Signed Server certificate
#define TLS0CSR                                0x020b0002  // Certificate Signing Request
#define TLS0CERT                               0x020b0003  // Signed Server certificate
#define TLS0CACERT                             0x020b0004  // CA certificate, public part
#define TLS0EXPIRY                             0x020b0005  // Certificate expiration time UTC
#define CPL0                                   0x03000000  // Control Panel class
#define CPL0AVL                                0x03000000  // Available
#define CPL0VER                                0x03000001  // Version
#define MB0                                    0x04000000  // Minibar class
#define MB0AVL                                 0x04000000  // Available
#define MB0DSN                                 0x04000001  // Dometic Serial Number
#define MB0SKU                                 0x04000002  // Stock Keeping Unit
#define MB0PNC                                 0x04000003  // Product Numeric Code
#define MB0VER                                 0x04000004  // Minibar FW version
#define MB0PWRON                               0x04000005  // Power control
#define MB0LGTON                               0x04000006  // Light control
#define MB0TCTRL                               0x04000007  // Compartment Temperature Setting
#define MB0TSTAT                               0x04000008  // Measured Compartment Temperature
#define MB0DOORI                               0x04000009  // Door indication
#define MB0DOORST                              0x0400000a  // Current door status
#define MB0SILMD                               0x0400000b  // Silent mode
#define MB0TEMPAL                              0x0400000c  // Temperature alarm
#define MB0DOORAL                              0x0400000d  // Door alarm
#define MB0ERRST                               0x0400000e  // Error status
#define RCS0                                   0x04010000  // Refridgerator control service class
#define RCS0AVL                                0x04010000  // Available
#define RCS0TALRM                              0x04010001  // Temperature alarm
#define RCS0TALT                               0x04010002  // Temperature alarm time
#define RCS0DALRM                              0x04010003  // Door alarm
#define RCS0DALT                               0x04010004  // Door alarm time
#define RCS0THIOULIM                           0x04010005  // Temp. high outer limit
#define RCS0THIINLIM                           0x04010006  // Temp. high inner limit
#define RCS0TLOINLIM                           0x04010007  // Temp. low inner limit
#define RCS0TLOOULIM                           0x04010008  // Temp. low outer limit
#define RCS0TALT2                              0x04010009  // Temperature alarm time 2
#define RCS0PWRFAIL                            0x0401000a  // Power failure
#define RCS0CDFAIL                             0x0401000b  // Cloud failure
#define RCS0THH                                0x0401000c  // RCS temperature last hour history
#define RCS0TDH                                0x0401000d  // RCS temperature last day history
#define RCS0TWH                                0x0401000e  // RCS tempertature last week history
#define RCS0ROOM                               0x0401000f  // Room number (as string)
#define NRX0                                   0x04020000  // NRX class
#define NRX0AVL                                0x04020000  // Available
#define NRX0DSN                                0x04020001  // Dometic Serial Number
#define NRX0SKU                                0x04020002  // Stock Keeping Unit
#define NRX0PNC                                0x04020003  // Product Numeric Code
#define NRX0VER                                0x04020004  // Firmware Version
#define NRX0PWRON                              0x04020005  // Power Control
#define NRX0LGTON                              0x04020006  // Internal Light Status
#define NRX0MODE                               0x04020007  // Mode Setting
#define NRX0LVL                                0x04020008  // Temperature Level Setting
#define NRX0TEMP                               0x04020009  // Measured Compartment Internal Temperature
#define NRX0DOORSTAT                           0x0402000a  // Current Door Status
#define NRX0COMPSTAT                           0x0402000b  // Compressor Running Status
#define NRX0FANSTAT                            0x0402000c  // FAN Running Speed
#define NRX0IONSTAT                            0x0402000d  // Ionizer Module Running Speed
#define NRX0LACSTAT                            0x0402000e  // LAC Heater Running Status
#define NRX0ERRST                              0x0402000f  // Error Status
#define TH0                                    0x05000000  // Thermostat class
#define TH0AVL                                 0x05000000  // Available
#define TH0ITEMP                               0x05000001  // Inside temperature
#define TH0BUT0                                0x05000002  // Button
#define TH0BUT1                                0x05000003  // Button
#define TH0BUT2                                0x05000004  // Button
#define TH0WUP                                 0x05000005  // Wakeup
#define TH0FAV                                 0x05000006  // Favourite
#define TH0LTO                                 0x05000007  // Long timeout in seconds
#define TH0STO                                 0x05000008  // Short timeout in seconds
#define TH0VER1                                0x05000009  // Version 1
#define TH0VER2                                0x0500000a  // Version 2
#define IV0                                    0x06000000  // Inventilate class
#define IV0AVL                                 0x06000000  // Available
#define IV0MODE                                0x06000001  // Inventilate Mode
#define IV0PWRON                               0x06000002  // Inventilate Power Mode
#define IV0FILST                               0x06000003  // Inventilate Filter Status
#define IV0STORAGE                             0x06000004  // Inventilate Storage Mode
#define IV0ERRST                               0x06000005  // Inventilate Error status
#define IV0WARN                                0x06000006  // Inventilate System Warning
#define IV0PWRSRC                              0x06000007  // Inventilate Power Source
#define IV0AQST                                0x06000008  // Air Quality Status
#define IV0PRST                                0x06000009  // Pressure Status
#define IV0BLREQ                               0x0600000a  // BLE Req
#define IV0IONST                               0x0600000b  // Inventilate Ionizer Status
#define IV0HMITST                              0x0600000c  // HMI Test
#define IV0SETT                                0x0600000d  // Inventilate Configuration
#define IV0SETCHRGCRNT                         0x0600000e  // Inventilate battery charging configuration
#define IV0STGT                                0x0600000f  // Select storage time
#define IVPMGR0                                0x06010000  // Inventilate Power Manager class
#define IVPMGR0AVL                             0x06010000  // Available
#define IVPMGR0STATE                           0x06010001  // Inventilate Power State
#define IVAQR0                                 0x06020000  // Inventilate IAQ Range class
#define IVAQR0AVL                              0x06020000  // Available
#define IVAQR0MIN                              0x06020001  // Min Range
#define IVAQR0MAX                              0x06020002  // Max Range
#define IVEOL0                                 0x06030000  // Inventilate EOL class
#define IVEOL0AVL                              0x06030000  // Available
#define IVEOL0REQ                              0x06030001  // Inventilate EOL Request
#define IVEOL0RESP                             0x06030002  // Inventilate EOL Response
#define CZ0                                    0x07000000  // Thermostat RV-C (Climate zone) class
#define CZ0AVL                                 0x07000000  // Available
#define CZ0NAME                                0x07000001  // name
#define CZ0OMD                                 0x07000002  // operating mode
#define CZ0FMODE                               0x07000003  // fan mode
#define CZ0SMODE                               0x07000004  // schedule mode
#define CZ0FSPD                                0x07000005  // fan speed
#define CZ0HSET                                0x07000006  // heat setpoint
#define CZ0CSET                                0x07000007  // cool setpoint
#define CZ0CSCH                                0x07000008  // current schedule
#define CZ0NSCH                                0x07000009  // nbr of schedule instances
#define CZ0ITEMP                               0x0700000a  // inside temp
#define CZ0AWAYIHS                             0x0700000b  // away int heat setp
#define CZ0AWAYICS                             0x0700000c  // away int cool setp
#define CZ0SLEEPTIMEH                          0x0700000d  // sleep start hour
#define CZ0SLEEPTIMEM                          0x0700000e  // sleep start min
#define CZ0SLEEPIHS                            0x0700000f  // sleep heat setp
#define CZ0SLEEPICS                            0x07000010  // sleep cool setp
#define CZ0AWAKETIMEH                          0x07000011  // wake start hour
#define CZ0AWAKETIMEM                          0x07000012  // wake start min
#define CZ0AWAKEIHS                            0x07000013  // wake heat setp
#define CZ0AWAKEICS                            0x07000014  // wake cool setp
#define CZ0AGS                                 0x07000015  // ags
#define CZ0ON                                  0x07000016  // power on
#define CZ0HMD                                 0x07000017  // Heating mode
#define CZ0MAXFAN                              0x07000018  // Max fan speed (auto)
#define CZ0MINFAN                              0x07000019  // Min fan speed (auto)
#define CZ0TH                                  0x0700001a  // target humidity
#define CZ0MOP                                 0x0700001b  // Max output protection
#define CZ0MOQM                                0x0700001c  // Max output QM
#define CZ0RAH                                 0x0700001d  // room air humidity
#define CZ0ZBINST                              0x0700001e  // Zone bus instance
#define CZ0ZDEL                                0x0700001f  // Zone delete
#define CZ0FTDIFF                              0x07000020  // Furnace Temp Diff
#define CZ0HPRIO                               0x07000021  // Heat source Priority
#define CZ0FILIND                              0x07000022  // Filter Indication
#define CZ0ECO                                 0x07000023  // ECO modifier
#define CZ0SLEEP                               0x07000024  // Sleep modifier
#define RAC0                                   0x07010000  // RV-C AC class
#define RAC0AVL                                0x07010000  // Available
#define RAC0CZ                                 0x07010001  // climate zone id
#define RAC0MS                                 0x07010002  // mode state
#define HP0                                    0x07020000  // Heat Pump class
#define HP0AVL                                 0x07020000  // Available
#define HP0CZ                                  0x07020001  // climate zone id
#define HP0MS                                  0x07020002  // mode state
#define FU0                                    0x07030000  // Furnace class
#define FU0AVL                                 0x07030000  // Available
#define FU0CZ                                  0x07030001  // climate zone id
#define FU0MS                                  0x07030002  // mode state
#define CZM0                                   0x07040000  // Climate Zone Manager class
#define CZM0AVL                                0x07040000  // Available
#define CZM0ADD                                0x07040001  // Add
#define CSM0                                   0x07050000  // Climate Schedule Manager class
#define CSM0AVL                                0x07050000  // Available
#define CSM0ADD                                0x07050001  // Add
#define CZS0                                   0x07060000  // Climate Zone Schedule class
#define CZS0AVL                                0x07060000  // Available
#define CZS0SU                                 0x07060001  // Sunday
#define CZS0MO                                 0x07060002  // Monday
#define CZS0TU                                 0x07060003  // Tuesday
#define CZS0WE                                 0x07060004  // Wednesday
#define CZS0TH                                 0x07060005  // Thursday
#define CZS0FR                                 0x07060006  // Friday
#define CZS0SA                                 0x07060007  // Saturday
#define CZS0STAH                               0x07060008  // Start hour
#define CZS0STAM                               0x07060009  // Start minute
#define CZS0STOH                               0x0706000a  // Stop hour
#define CZS0STOM                               0x0706000b  // Stop min
#define CZS0OMD                                0x0706000c  // operating mode
#define CZS0HSET                               0x0706000d  // heat setpoint
#define CZS0CSET                               0x0706000e  // cool setpoint
#define CZS0THUM                               0x0706000f  // target humidity
#define CZS0SDEL                               0x07060010  // Schedule delete
#define CZS0EN                                 0x07060011  // Scheduler enable
#define CC0                                    0x07070000  // Climate Control class
#define CC0AVL                                 0x07070000  // Available
#define CC0NAME                                0x07070001  // Name identifier
#define CC0ADD                                 0x07070002  // Add
#define CC0DEL                                 0x07070003  // Delete
#define CC0ACT                                 0x07070004  // Active
#define CC0DEVICES                             0x07070005  // Linked devices
#define CC0SETTEMP                             0x07070006  // Target temperature
#define CC0SETHUMID                            0x07070007  // Target humidity
#define CC0STS                                 0x07070008  // Status of zone
#define CC0PCY                                 0x07070009  // Policy of zone
#define CC0TEMP                                0x0707000a  // Current temperature
#define CC0VOC                                 0x0707000b  // Current air quality index
#define CC0HUMID                               0x0707000c  // Current humidity
#define CC0DELTAP                              0x0707000d  // Current delta pressure
#define CCS0                                   0x07080000  // Climate control schedule class
#define CCS0AVL                                0x07080000  // Available
#define CCS0ADD                                0x07080001  // Add a new schedule rule/action
#define CCS0DEL                                0x07080002  // Delete a schedule
#define CCS0LIST                               0x07080003  // List all scheduled rules/actions
#define CBS0                                   0x07090000  // Climate (zone device) basic schedule class
#define CBS0AVL                                0x07090000  // Available
#define CBS0ENA                                0x07090001  // Scheduling enabled
#define CBS0SMODE                              0x07090002  // Schedule mode/type
#define CBS0SLPDAYS                            0x07090003  // Enabled Weekdays for Sleep
#define CBS0SLPHOUR                            0x07090004  // Start time (hours) for Sleep
#define CBS0SLPMIN                             0x07090005  // Start time (min) for Sleep
#define CBS0WAKEDAYS                           0x07090006  // Enabled Weekdays for Wake
#define CBS0WAKEHOUR                           0x07090007  // Start time (hours) for Wake
#define CBS0WAKEMIN                            0x07090008  // Start time (min) for Wake
#define CBS0AWAYDAYS                           0x07090009  // Enabled Weekdays for Away
#define CBS0SLPCOOL                            0x0709000a  // Cool temperature setpoint for Sleep
#define CBS0SLPHEAT                            0x0709000b  // Heat temperature setpoint for Sleep
#define CBS0WAKECOOL                           0x0709000c  // Cool temperature setpoint for Wake
#define CBS0WAKEHEAT                           0x0709000d  // Heat temperature setpoint for Wake
#define CBS0AWAYCOOL                           0x0709000e  // Cool temperature setpoint for Away
#define CBS0AWAYHEAT                           0x0709000f  // Heat temperature setpoint for Away
#define RFAN0                                  0x08000000  // Roof fan class
#define RFAN0AVL                               0x08000000  // Available
#define RFAN0SYST                              0x08000001  // system status
#define RFAN0FM                                0x08000002  // fan mode
#define RFAN0SPDMD                             0x08000003  // speed mode
#define RFAN0LIGHT                             0x08000004  // light
#define RFAN0FSPDSET                           0x08000005  // fan speed setting
#define RFAN0WINDDRSW                          0x08000006  // wind dir switch
#define RFAN0DOMEPOS                           0x08000007  // dome position
#define RFAN0RAINSNS                           0x08000008  // rain sensor override
#define RFAN0AMBTEMP                           0x08000009  // ambient temperature
#define RFAN0SETPT                             0x0800000a  // setpoint
#define RFAN0DMMODE                            0x0800000b  // dome command/mode
#define RFAN0DSRDDMPOS                         0x0800000c  // dome position/desired dome position
#define RFAN0SETPCTLDM                         0x0800000d  // desired setpoint controlled dome
#define RFAN0DMCLSFOFF                         0x0800000e  // auto dome close on fan off
#define RFAN0FOFFDMCLS                         0x0800000f  // auto fan off on dome close
#define RFAN0FSPDINCDEC                        0x08000010  // fan speed increment/decrement
#define RFAN0FSPDIDSTP                         0x08000011  // fan speed increment/decrement step
#define RFAN0FSPDSTPSUP                        0x08000012  // fan steps(speeds) supported
#define RFAN0RAINSNSSTS                        0x08000013  // rain sensor
#define RFAN0MTNSNSEN                          0x08000014  // motion sensor status
#define RFAN0MTNSNSTIM                         0x08000015  // no motion detected time threshold
#define RFAN0LGTCMD                            0x08000016  // light command
#define RFAN0LGTDD                             0x08000017  // light duration/delay
#define RFAN0LGTLVL                            0x08000018  // light level
#define RFAN0LGTRGB                            0x08000019  // light RGB color
#define RFAN0LGTOC                             0x0800001a  // light overcurrent
#define RFAN0LGTLSTAT                          0x0800001b  // light load status
#define RFAN0CSCH                              0x0800001c  // current schedule
#define RFAN0NSCH                              0x0800001d  // number of schedules
#define RFAN0SLEEPTIMEH                        0x0800001e  // sleep time schedule hour
#define RFAN0SLEEPTIMEM                        0x0800001f  // sleep time schedule minute
#define RFAN0SLEEPFMODE                        0x08000020  // sleep time fan mode
#define RFAN0SLEEPFDIR                         0x08000021  // sleep time fan direction
#define RFAN0SLEEPFSETP                        0x08000022  // sleep time fan speed/temperature setpoint
#define RFAN0AWAKETIMEH                        0x08000023  // awake time schedule hour
#define RFAN0AWAKETIMEM                        0x08000024  // awake time schedule minute
#define RFAN0AWAKEFMODE                        0x08000025  // awake time fan mode
#define RFAN0AWAKEFDIR                         0x08000026  // awake time fan direction
#define RFAN0AWAKEFSETP                        0x08000027  // awake time fan speed/temperature setpoint
#define RFAN0AWAYTIMEH                         0x08000028  // away time schedule hour
#define RFAN0AWAYTIMEM                         0x08000029  // away time schedule minute
#define RFAN0AWAYFMODE                         0x0800002a  // away time fan mode
#define RFAN0AWAYFDIR                          0x0800002b  // away time fan direction
#define RFAN0AWAYFSETP                         0x0800002c  // away time fan speed/temperature setpoint
#define RFAN0ERR                               0x0800002d  // fan error
#define RFAN0ALARM                             0x0800002e  // Alarm enable
#define RFAN0FEATURE                           0x0800002f  // Available features
#define AW0                                    0x08010000  // Awning class
#define AW0AVL                                 0x08010000  // Available
#define AW0MOTION                              0x08010001  // motion
#define AW0POS                                 0x08010002  // position
#define AW0LOCKED                              0x08010003  // statusing is ocktatus
#define AW0MOVABLE                             0x08010004  // moveable
#define AW0USERL                               0x08010005  // user lock status
#define AW0BRAKEL                              0x08010006  // brake lock status
#define AW0PBRAKE                              0x08010007  // parking brake status
#define AW0IG                                  0x08010008  // ignitioning key status
#define AW0LOWV                                0x08010009  // low voltage status
#define AW0OC                                  0x0801000a  // overcurrent status
#define AW0ROC                                 0x0801000b  // retract overcurrent status
#define MTR0                                   0x08020000  // Motor data class
#define MTR0AVL                                0x08020000  // Available
#define MTR0DEVID                              0x08020001  // Device ID
#define MTR0SETSPD                             0x08020002  // Current Speeed
#define MTR0MINSPD                             0x08020003  // Minimum Speed
#define MTR0MAXSPD                             0x08020004  // Maximum Speed
#define MTR0TACHO                              0x08020005  // Tacho Feedback in RPM
#define MTR0DIR                                0x08020006  // Direction of Motor
#define GNTR0                                  0x09000000  // Generator class
#define GNTR0AVL                               0x09000000  // Available
#define GNTR0CHRGV                             0x09000001  // charger voltage
#define GNTR0CHRGCRR                           0x09000002  // charger current
#define GNTR0CHRGCRRPM                         0x09000003  // charger percent of max
#define GNTR0OPST                              0x09000004  // operating state
#define GNTR0CHRGAALG                          0x09000005  // charging algorithm
#define GNTR0BTRSNSP                           0x09000006  // sensor present
#define GNTR0LNKGMD                            0x09000007  // linkage mode
#define GNTR0BTRTYP                            0x09000008  // battery type
#define GNTR0BTRBKSZ                           0x09000009  // battery bank size
#define GNTR0MAXCHRGCRR                        0x0900000a  // max charging current
#define GNTR0DCOST                             0x0900000b  // DC output status
#define GNTR0DFSTPWUP                          0x0900000c  // DC default state power up
#define GNTR0TREM                              0x0900000d  // time remaining
#define GNTR0PRECHRGST                         0x0900000e  // pre-charging status
#define GNTR0EQVLTG                            0x0900000f  // equalization status
#define GNTR0EQT                               0x09000010  // equalization time
#define GNTR0ST                                0x09000011  // status
#define GNTR0ENGRT                             0x09000012  // engine runtime
#define GNTR0ENGL                              0x09000013  // engine load
#define GNTR0STBTRV                            0x09000014  // start battery voltage
#define GNTR0TMPSDSW                           0x09000015  // temp shutdown switch
#define GNTR0OILPRESDSW                        0x09000016  // oil pressure shutdown switch
#define GNTR0OILLVLSW                          0x09000017  // oil level switch
#define GNTR0CAULT                             0x09000018  // caution ligth
#define GNTR0ENGCTMP                           0x09000019  // engine coolant temp
#define GNTR0ENGOILP                           0x0900001a  // engine oil pressure
#define GNTR0ENGRPM                            0x0900001b  // engine rpm
#define GNTR0FUELR                             0x0900001c  // fuel rate
#define GNTR0CMD                               0x0900001d  // command
#define GNTR0TYPE                              0x0900001e  // type
#define GNTR0PRECT                             0x0900001f  // pre-crank time
#define GNTR0MAXCT                             0x09000020  // max crank time
#define GNTR0STOPT                             0x09000021  // stop time
#define GNTR0DEMAND                            0x09000022  // demand
#define GNTR0EXTACTR                           0x09000023  // external activity
#define GNTR0QUIETT                            0x09000024  // quiet time
#define GNTR0AUCHST                            0x09000025  // auto charger status
#define GNTR0AUCHVTH                           0x09000026  // auto charger voltage
#define GNTR0AUCHT                             0x09000027  // auto charger time
#define GNTR0QTBH                              0x09000028  // quite time begin hour
#define GNTR0QTBM                              0x09000029  // quite time begin minutes
#define GNTR0QTEH                              0x0900002a  // quite time end hour
#define GNTR0QTEM                              0x0900002b  // quite time end minutes
#define GNTR0EXSTAT                            0x0900002c  // exerciser status
#define GNTR0EXDW                              0x0900002d  // exerciser days of week
#define GNTR0EXSH                              0x0900002e  // exersiser start hour
#define GNTR0EXSM                              0x0900002f  // exersiser start minutes
#define GNTR0EXRT                              0x09000030  // exersiser run time
#define ACCH0                                  0x09010000  // AC Charger class
#define ACCH0AVL                               0x09010000  // Available
#define ACCH0CHV                               0x09010001  // charging voltage
#define ACCH0CHI                               0x09010002  // charging current
#define ACCH0CIPMAX                            0x09010003  // charge current percent of max
#define ACCH0OPST                              0x09010004  // operating state
#define ACCH0DEST                              0x09010005  // default state
#define ACCH0ARCEN                             0x09010006  // auto recharge enable
#define ACCH0FRC                               0x09010007  // force charge
#define ACCH0ACVRMS                            0x09010008  // AC input current vrms
#define ACCH0RPWR                              0x09010009  // real power
#define ITER0                                  0x09020000  // Inverter class
#define ITER0AVL                               0x09020000  // Available
#define ITER0MD                                0x09020001  // mode
#define ITER0IE                                0x09020002  // inverter enabled
#define ITER0LE                                0x09020003  // load sense enabled
#define ITER0PE                                0x09020004  // pass-through enabled
#define ITER0ES                                0x09020005  // inverter enabled on startup
#define ITER0VDC                               0x09020006  // DC voltage
#define ITER0ADC                               0x09020007  // DC current
#define ITER0AAC                               0x09020008  // RMS AC current
#define ITER0RPWR                              0x09020009  // RMS power?
#define DSRC0                                  0x09030000  // DC Source class
#define DSRC0AVL                               0x09030000  // Available
#define DSRC0VDC                               0x09030001  // VDC
#define DSRC0ADC                               0x09030002  // ADC
#define DSRC0PRIO                              0x09030003  // Priority
#define DSRC0HIS1                              0x09030004  // DC Source voltage 1 day before
#define DSRC0HIS2                              0x09030005  // DC Source voltage 2 days before
#define DSRC0HIS3                              0x09030006  // DC Source voltage 3 days before
#define DSRC0HIS4                              0x09030007  // DC Source voltage 4 days before
#define DSRC0HIS5                              0x09030008  // DC Source voltage 5 days before
#define PMIC0                                  0x09040000  // PMIC class
#define PMIC0AVL                               0x09040000  // Available
#define PMIC0BATRV                             0x09040001  // Regulation voltage
#define PMIC0CHRGC                             0x09040002  // Charge current
#define PMIC0TC                                0x09040003  // Termination current
#define PMIC0PCHRGC                            0x09040004  // Precharge current
#define PMIC0MSYSV                             0x09040005  // Min system volt limit
#define PMIC0BSTRV                             0x09040006  // Boost reg volt
#define PMIC0THRGTH                            0x09040007  // Thermal regulation thres
#define PMIC0BSTAT                             0x09040008  // Battery status
#define PMIC0CHTY                              0x09040009  // Charge type
#define PMIC0MID                               0x0904000a  // Manufacturer Id
#define PMIC0BMDL                              0x0904000b  // Battery model
#define PMIC0ONLINE                            0x0904000c  // Online
#define PMIC0BHLT                              0x0904000d  // Battery health
#define TS0                                    0x0a000000  // Tank class
#define TS0AVL                                 0x0a000000  // Available
#define TS0TYPE                                0x0a000001  // Type
#define TS0RLVL                                0x0a000002  // Relative level
#define TS0RES                                 0x0a000003  // Sensor resolution
#define TS0HIS1                                0x0a000004  // Relative level 1
#define TS0HIS2                                0x0a000005  // Relative level 2
#define TS0HIS3                                0x0a000006  // Relative level 3
#define TS0HIS4                                0x0a000007  // Relative level 4
#define TS0HIS5                                0x0a000008  // Relative level 5
#define TS0TEMP                                0x0a000009  // Tank temperature
#define WP0                                    0x0a010000  // Water pump class
#define WP0AVL                                 0x0a010000  // Available
#define WP0OPRTST                              0x0a010001  // operating status 
#define WP0PMPST                               0x0a010002  // Pump status
#define WP0WTRHKDTCT                           0x0a010003  // Water hookup detected
#define WP0CURSYSPRES                          0x0a010004  // Current system pressure
#define WP0PPRESSET                            0x0a010005  // Pump pressure setting
#define WP0RPRESSET                            0x0a010006  // Regulator pressure setting
#define WP0OPRTCURR                            0x0a010007  // Operating current
#define WP0OC                                  0x0a010008  // Overcurrent
#define WH0                                    0x0a020000  // Water Heater class
#define WH0AVL                                 0x0a020000  // Available
#define WH0OPSTAT                              0x0a020001  // Operating state
#define WH0HLVL                                0x0a020002  // Heat level
#define DCL0                                   0x0b000000  // DC Load class
#define DCL0AVL                                0x0b000000  // Available
#define DCL0CMD                                0x0b000001  // Command
#define DCL0DD                                 0x0b000002  // Duration/delay
#define DCL0LVL                                0x0b000003  // Level
#define DCL0OC                                 0x0b000004  // Overcurrent
#define DCL0TYPE                               0x0b000005  // Type
#define DIM0                                   0x0c000000  // DC Dimmer class
#define DIM0AVL                                0x0c000000  // Available
#define DIM0CMD                                0x0c000001  // Command
#define DIM0DD                                 0x0c000002  // Duration/delay
#define DIM0LVL                                0x0c000003  // Level
#define DIM0RGB                                0x0c000004  // RGB color
#define DIM0OC                                 0x0c000005  // Overcurrent
#define DIM0LSTAT                              0x0c000006  // Load status
#define DIM0NAME                               0x0c000007  // Name
#define DIM0TYPE                               0x0c000008  // Type / capabilities
#define SC0                                    0x0d000000  // Scenes class
#define SC0AVL                                 0x0d000000  // Available
#define SC0NAME                                0x0d000001  // Name
#define SC0STAT                                0x0d000002  // Status
#define SCE0                                   0x0d010000  // Scene Element Definition class
#define SCE0AVL                                0x0d010000  // Available
#define SCE0PARENT                             0x0d010001  // Parent
#define SCE0PNAME                              0x0d010002  // Parameter name
#define SCE0PAR                                0x0d010003  // Parameter
#define AUT0                                   0x0d020000  // Automation class
#define AUT0AVL                                0x0d020000  // Available
#define AUT0NAME                               0x0d020001  // Name
#define AUT0STATE                              0x0d020002  // State
#define AUT0STH                                0x0d020003  // Start time hours
#define AUT0STM                                0x0d020004  // Start time minutes
#define AUT0STD                                0x0d020005  // Start time days of the week
#define AUT0IST                                0x0d020006  // Instance
#define SNODE0                                 0x0e000000  // Sensor node class
#define SNODE0AVL                              0x0e000000  // Available
#define SNODE0MFGR                             0x0e000001  // Manufacturer
#define SNODE0TYPE                             0x0e000002  // Product type
#define SNODE0MDL                              0x0e000003  // Node type
#define SNODE0ID                               0x0e000004  // Id
#define SNODE0NAME                             0x0e000005  // Device name
#define SNODE0FR                               0x0e000006  // Factory reset
#define SNODE0ADVINT                           0x0e000007  // Advertising interval
#define SNODE0ADVINTC                          0x0e000008  // Advertising interval connected
#define SNODE0CONNINT                          0x0e000009  // Connection interval
#define SNODE0SLAVELAT                         0x0e00000a  // Slave latency
#define SNODE0SHA1                             0x0e00000b  // GAP SHA1 hash
#define SNODE0BLOB                             0x0e00000c  // Configuration blob
#define SNODE0FUNC                             0x0e00000d  // Function
#define SNODE0LOC                              0x0e00000e  // Location
#define SNODE0DFU                              0x0e00000f  // Enter OTA DFU
#define SNODE0NCINT                            0x0e000010  // Nonconnectable connection interval
#define SNODE0CINT                             0x0e000011  // Connectable connection interval
#define SNODE0GAPBLOB                          0x0e000012  // GAP blob
#define SNODE0UADVINT                          0x0e000013  // Update advertise interval
#define SNODE0LEDBLINK                         0x0e000014  // LED blink
#define SNODE0WLINT                            0x0e000015  // Connectable connection interval with WL
#define SNODE0PEERDEL                          0x0e000016  // Delete peer
#define SNODE0SN                               0x0e000017  // Serial number
#define SNODE0MGRDATE                          0x0e000018  // Manufacturing date
#define SNODE0MFGRNAME                         0x0e000019  // Manufacturer name
#define SNODE0PRODUCT                          0x0e00001a  // Product
#define SNODE0SKU                              0x0e00001b  // SKU
#define SNODE0VER                              0x0e00001c  // Firmware version
#define SNODE0PCB                              0x0e00001d  // PCB
#define SNODE0BOM                              0x0e00001e  // BOM
#define SNODE0ITEMDESC                         0x0e00001f  // Item description
#define SNODE0MDLNO                            0x0e000020  // Model number
#define SNODE0MDLNAME                          0x0e000021  // Model name
#define SNODE0EAN13                            0x0e000022  // EAN13
#define SNODE0BATTLVL                          0x0e000023  // Battery voltage
#define SNODE0EXTPWRSENSE                      0x0e000024  // External power sense
#define SACCM0                                 0x0e010000  // Accelerometer class
#define SACCM0AVL                              0x0e010000  // Available
#define SACCM0STATUS                           0x0e010001  // Status
#define SACCM0FR                               0x0e010002  // Factory reset
#define SACCM0ACCX                             0x0e010003  // Acceleration x
#define SACCM0ACCY                             0x0e010004  // Acceleration y
#define SACCM0ACCZ                             0x0e010005  // Acceleration z
#define SACCM0LONANGLE                         0x0e010006  // Longitudinal tilt angle
#define SACCM0LATANGLE                         0x0e010007  // Lateral tilt angle
#define SACCM0TEMP                             0x0e010008  // Temperature
#define SACCM0WAI                              0x0e010009  // WhoAmI
#define SACCM0EVENT                            0x0e01000a  // Acceleration event
#define SACCM0SENDACCX                         0x0e01000b  // Send acceleration x 
#define SACCM0SENDACCY                         0x0e01000c  // Send acceleration y
#define SACCM0SENDACCZ                         0x0e01000d  // Send acceleration z
#define SACCM0SENDTILT                         0x0e01000e  // Send tilt angle
#define SACCM0SENDTEMP                         0x0e01000f  // Send temperature
#define SACCM0SAMPP                            0x0e010010  // Sampling period
#define SACCM0CALFLATDONE                      0x0e010011  // Calibration flat done
#define SACCM0CALFLATSTART                     0x0e010012  // Calibration flat start
#define SACCM0CALTILTDONE                      0x0e010013  // Calibration tilt done
#define SACCM0CALTILTSTART                     0x0e010014  // Calibration tilt start
#define SACCM0CALTILTPOS                       0x0e010015  // Calibration tilt position
#define SACCM0DATARATE                         0x0e010016  // Set data rate
#define SACCM0POWERM                           0x0e010017  // Set power mode
#define SACCM0BW                               0x0e010018  // Set filter bandwidth
#define SACCM0FULLSCALE                        0x0e010019  // Set full scale
#define SACCM0FILTPATH                         0x0e01001a  // Set filter path 
#define SACCM0TILTT                            0x0e01001b  // Set 6d threshold
#define SACCM0TAPX                             0x0e01001c  // Set tap threshold x
#define SACCM0TAPY                             0x0e01001d  // Set tap threshold y
#define SACCM0TAPZ                             0x0e01001e  // Set tap threshold z
#define SACCM0TAPDIR                           0x0e01001f  // Set tap recognition axis 
#define SACCM0TAPP                             0x0e010020  // Set tap axis priority
#define SACCM0FALLTHRESH                       0x0e010021  // Set freefall threshold
#define SACCM0FALLDUR                          0x0e010022  // Set freefall duration
#define SACCM0WAKETHRESH                       0x0e010023  // Set wakeup threshold
#define SACCM0WAKEDUR                          0x0e010024  // Set wakeup duration
#define SACCM0DTAPDUR                          0x0e010025  // Set double tap duration
#define SACCM0QUIETT                           0x0e010026  // Set double tap quiet time
#define SACCM0SHOCKDUR                         0x0e010027  // Set double tap shock duration
#define SACCM0TAPMODE                          0x0e010028  // Set tap mode
#define SACCM0TAPRECOGX                        0x0e010029  // Set tap recognition x
#define SACCM0TAPRECOGY                        0x0e01002a  // Set tap recognition y
#define SACCM0TAPRECOGZ                        0x0e01002b  // Set tap recognition z
#define SACCM0STAPEVENT                        0x0e01002c  // Single tap enable
#define SACCM0DTAPEVENT                        0x0e01002d  // Double tap enable
#define SACCM0FFEVENT                          0x0e01002e  // Freefall enable
#define SACCM0TILTEVENT                        0x0e01002f  // Tilt enable
#define SACCM0WUEVENT                          0x0e010030  // Wakeup enable
#define SHALL0                                 0x0e020000  // HALL sensor class
#define SHALL0AVL                              0x0e020000  // Available
#define SHALL0STATUS                           0x0e020001  // Status
#define SHALL0FR                               0x0e020002  // Factory reset
#define SHALL0OPEN                             0x0e020003  // Open
#define SHALL0SENDOPEN                         0x0e020004  // Send open
#define SBMEA0                                 0x0e030000  // BME280 class
#define SBMEA0AVL                              0x0e030000  // Available
#define SBMEA0STATUS                           0x0e030001  // Status
#define SBMEA0FR                               0x0e030002  // Factory reset
#define SBMEA0TEMP                             0x0e030003  // Temperature
#define SBMEA0HUM                              0x0e030004  // Humidity
#define SBMEA0PRS                              0x0e030005  // Pressure
#define SBMEA0SENDTEMP                         0x0e030006  // Send temperature
#define SBMEA0SENDHUM                          0x0e030007  // Send humidity
#define SBMEA0SENDPRS                          0x0e030008  // Send pressure
#define SBMEA0OVERTEMP                         0x0e030009  // Oversampling temperature
#define SBMEA0OVERHUM                          0x0e03000a  // Oversampling humidity
#define SBMEA0OVERPRS                          0x0e03000b  // Oversampling pressure
#define SBMEA0SAMPP                            0x0e03000c  // Sampling period
#define SBMEB0                                 0x0e040000  // BME680 class
#define SBMEB0AVL                              0x0e040000  // Available
#define SBMEB0STATUS                           0x0e040001  // Status
#define SBMEB0FR                               0x0e040002  // Factory reset
#define SBMEB0TEMP                             0x0e040003  // Temperature
#define SBMEB0HUM                              0x0e040004  // Humidity
#define SBMEB0PRS                              0x0e040005  // Pressure
#define SBMEB0GAS                              0x0e040006  // Gas resistance
#define SBMEB0AQR                              0x0e040007  // Air quality value
#define SBMEB0IAQ                              0x0e040008  // Air quality index
#define SBMEB0CO2                              0x0e040009  // CO2 concentration
#define SBMEB0VOC                              0x0e04000a  // VOC concentration
#define SBMEB0SST                              0x0e04000b  // Stabilization status
#define SBMEB0RIS                              0x0e04000c  // Run-in status
#define SBMEB0SENDTEMP                         0x0e04000d  // Send temperature
#define SBMEB0SENDHUM                          0x0e04000e  // Send humidity
#define SBMEB0SENDPRS                          0x0e04000f  // Send pressure
#define SBMEB0SENDGAS                          0x0e040010  // Send gas resistance
#define SBMEB0SENDAQR                          0x0e040011  // Send air quality value
#define SBMEB0SENDIAQ                          0x0e040012  // Send air quality index
#define SBMEB0SENDCO2                          0x0e040013  // Send CO2
#define SBMEB0SENDVOC                          0x0e040014  // Send VOC
#define SBMEB0SENDSST                          0x0e040015  // Send stabilization status
#define SBMEB0SENDRIS                          0x0e040016  // Send run-in status
#define SBMEB0SAMPRATE                         0x0e040017  // Sample rate
#define SBMEB0SAMPRATEG                        0x0e040018  // Sample rate for gas sensor
#define SPIR0                                  0x0e050000  // PIR sensor class
#define SPIR0AVL                               0x0e050000  // Available
#define SPIR0STATUS                            0x0e050001  // Status
#define SPIR0FR                                0x0e050002  // Factory reset
#define SPIR0EVT                               0x0e050003  // Event
#define SPIR0SENDEVT                           0x0e050004  // Send event
#define SWB0                                   0x0e060000  // Wire break sensor class
#define SWB0AVL                                0x0e060000  // Available
#define SWB0STATUS                             0x0e060001  // Status
#define SWB0FR                                 0x0e060002  // Factory reset
#define SWB0SFUNC                              0x0e060003  // Sub function
#define SWB0SLOC                               0x0e060004  // Sub location
#define SWB0OPEN                               0x0e060005  // Open
#define SWB0ADCOPEN                            0x0e060006  // ADC open
#define SWB0SENDOPEN                           0x0e060007  // Send open
#define SWB0SENDADC                            0x0e060008  // Send ADC
#define SWB0SAMPP                              0x0e060009  // Sampling period
#define SV0                                    0x0e070000  // Voltage sensor class
#define SV0AVL                                 0x0e070000  // Available
#define SV0STATUS                              0x0e070001  // Status
#define SV0FR                                  0x0e070002  // Factory reset
#define SV0SFUNC                               0x0e070003  // Sub function
#define SV0SLOC                                0x0e070004  // Sub location
#define SV0ADCV                                0x0e070005  // ADC 12V
#define SV0ADCBATT                             0x0e070006  // ADC batt
#define SV0ADCBLG                              0x0e070007  // ADC bilge
#define SV0VOUTS                               0x0e070008  // Vout sense
#define SV0PSTS                                0x0e070009  // Power Status
#define SV0SENDADCV                            0x0e07000a  // Send adc 12V
#define SV0SENDADCBATT                         0x0e07000b  // Send ADC batt
#define SV0SENDADCBLG                          0x0e07000c  // Send ADC bilge
#define SV0SENDVOUTS                           0x0e07000d  // Send vout sense
#define SV0SAMPP                               0x0e07000e  // Sampling period
#define SDP0                                   0x0e080000  // Differential pressure sensor class
#define SDP0AVL                                0x0e080000  // Available
#define SDP0STATUS                             0x0e080001  // Status
#define SDP0FR                                 0x0e080002  // Factory reset
#define SDP0DP                                 0x0e080003  // Differential pressure
#define SDP0TEMP                               0x0e080004  // Temperature
#define SDP0PI                                 0x0e080005  // Product Identifier
#define SDP0SENDDP                             0x0e080006  // Send differential pressure
#define SDP0SENDTEMP                           0x0e080007  // Send temperature
#define SDP0SAMPP                              0x0e080008  // Sampling period
#define SDP0MC                                 0x0e080009  // Measurement command
#define SHALLAN0                               0x0e090000  // HALL analog sensor class
#define SHALLAN0AVL                            0x0e090000  // Available
#define SHALLAN0STATUS                         0x0e090001  // Status
#define SHALLAN0FR                             0x0e090002  // Factory reset
#define SHALLAN0MFX                            0x0e090003  // Value magnetic field X
#define SHALLAN0MFY                            0x0e090004  // Value magnetic field Y
#define SHALLAN0MFZ                            0x0e090005  // Value magnetic field Z
#define SHALLAN0EVT                            0x0e090006  // Event
#define SHALLAN0SENDMFX                        0x0e090007  // Send magnetic field X
#define SHALLAN0SENDMFY                        0x0e090008  // Send magnetic field Y
#define SHALLAN0SENDMFZ                        0x0e090009  // Send magnetic field Z
#define SHALLAN0SENDEVT                        0x0e09000a  // Send event
#define SHALLAN0SAMPP                          0x0e09000b  // Sampling period
#define TEST0                                  0x0f000000  // Test class
#define TEST0AVL                               0x0f000000  // Available
#define TEST0R                                 0x0f000001  // Read
#define TEST0RW                                0x0f000002  // Read/Write
#define DEMO0                                  0x0f010000  // Demo class
#define DEMO0AVL                               0x0f010000  // Available
#define DEMO0SETTEMP                           0x0f010001  // Set temperature
#define DEMO0MODE                              0x0f010002  // Fan mode
#define DEMO0UPT                               0x0f010003  // Uptime
#define SW0                                    0x10000000  // Switch class
#define SW0AVL                                 0x10000000  // Available
#define SW0TYPE                                0x10000001  // Switch type
#define SW0NAME                                0x10000002  // Switch name
#define SW0STNAME                              0x10000003  // Switch sequence state names
#define SW0ICON                                0x10000004  // Switch icon
#define SW0MSTATE                              0x10000005  // Switch main state
#define SW0SSTATE                              0x10000006  // Switch sub state 
#define SW0MIND                                0x10000007  // Switch main indicator
#define SW0SIND                                0x10000008  // Switch sub indicator
#define SW0ZONE                                0x10000009  // Switch zone
#define SW0ZDEL                                0x1000000a  // Zone delete
#define SW0MPP                                 0x1000000b  // Switch drawn on main page
#define SW0MPXP                                0x1000000c  // Switch main page X position
#define SW0MPYP                                0x1000000d  // Switch main page Y position
#define SW0ID                                  0x1000000e  // Switch unique ID
#define SZM0                                   0x10010000  // Switch Zone Manager class
#define SZM0AVL                                0x10010000  // Available
#define SZM0ADD                                0x10010001  // Instance of zone added
#define SZ0                                    0x10020000  // Switch Zone class
#define SZ0AVL                                 0x10020000  // Available
#define SZ0NAME                                0x10020001  // Name
#define SZ0ON                                  0x10020002  // On/Off
#define SZ0ZDEL                                0x10020003  // Zone delete
#define BTN0                                   0x10030000  // Switch Zone class
#define BTN0AVL                                0x10030000  // Available
#define BTN0BTN0                               0x10030001  // Inventilate Power Button
#define BTN0BTN1                               0x10030002  // Inventilate Mode Button
#define BTN0BTN2                               0x10030003  // Inventilate Storage Mode Button
#define VE0                                    0x11000000  // Vessel class
#define VE0AVL                                 0x11000000  // Available
#define VE0LAT                                 0x11000001  // Vessel latitude
#define VE0LNG                                 0x11000002  // Vessel longitude
#define VE0SPD                                 0x11000003  // Vessel speed
#define VE0HEAD                                0x11000004  // Vessel heading
#define VE0PITCH                               0x11000005  // Vessel pitch
#define VE0ROLL                                0x11000006  // Vessel roll
#define VE0PTAB                                0x11000007  // Vessel port trim tab
#define VE0STAB                                0x11000008  // Vessel stbd trim tab
#define VE0JP                                  0x11000009  // Vessel jackplate
#define NGN0                                   0x12000000  // Engine data class
#define NGN0AVL                                0x12000000  // Available
#define NGN0FUCOH                              0x12000001  // Fuel consumption L/hour
#define NGN0FUCO                               0x12000002  // Fuel consumption L/km
#define NGN0SP                                 0x12000003  // Speed
#define NGN0TRANS                              0x12000004  // Transmission
#define NGN0TRIM                               0x12000005  // Trim
#define NGN0HOURS                              0x12000006  // Hours (engine data)
#define NGN0WARN                               0x12000007  // Warning indicator
#define WE0                                    0x13000000  // Weather class
#define WE0AVL                                 0x13000000  // Available
#define WE0OTEMP                               0x13000001  // Outside Temperature
#define WE0SEATEMP                             0x13000002  // Sea temperature
#define WE0SEASAL                              0x13000003  // Sea salinity
#define WE0ATM                                 0x13000004  // Atmospheric Pressure
#define WE0AH                                  0x13000005  // Air Humidity
#define WE0WSP                                 0x13000006  // Wind speed
#define WE0WDIR                                0x13000007  // Wind Direction
#define WE0WADEP                               0x13000008  // Water depth
#define WE0TILVL                               0x13000009  // Tide Level
#define WE0TITEN                               0x1300000a  // Tide Tendency
#define WE0SEASP                               0x1300000b  // Sea Current Speed
#define WE0SEADIR                              0x1300000c  // Sea Current Direction
#define BMS0                                   0x14000000  // Battery Management Service class
#define BMS0AVL                                0x14000000  // Available
#define BMS0V                                  0x14000001  // BMS battery Voltage
#define BMS0FLO                                0x14000002  // BMS battery energy flow
#define BMS0ID                                 0x14000003  // BMS battery id
#define BMS0SOC                                0x14000004  // BMS battery state of charge
#define BMS0SOH                                0x14000005  // BMS battery state of health
#define BMS0VHH                                0x14000006  // BMS battery voltage last hour history
#define BMS0VDH                                0x14000007  // BMS battery voltage last day history
#define BMS0VWH                                0x14000008  // BMS battery voltage last week history
#define BMS0ALRTS                              0x14000009  // BMS battery alert status
#define BMS0ALRTE                              0x1400000a  // BMS battery alert enabled
#define BMS0ATHR                               0x1400000b  // BMS battery alert threshold
#define BMS0ALAT                               0x1400000c  // BMS battery alert latency
#define BMS0WRNS                               0x1400000d  // BMS battery warning status
#define BMS0WRNE                               0x1400000e  // BMS battery warning enabled
#define BMS0WTHR                               0x1400000f  // BMS battery warning threshold
#define BMS0WLAT                               0x14000010  // BMS battery warning latency
#define BOS0                                   0x14010000  // Bilge Operation Service class
#define BOS0AVL                                0x14010000  // Available
#define BOS0STA                                0x14010001  // BOS bilge status
#define BOS0CL1H                               0x14010002  // BOS bilge cycles in last hour
#define BOS0CL24H                              0x14010003  // BOS bilge cycles in last 24 hours
#define BOS0CRT                                0x14010004  // BOS bilge continuous run time
#define BOS0ID                                 0x14010005  // BOS bilge id
#define BOS0CHH                                0x14010006  // BOS bilge cycles hour history
#define BOS0CDH                                0x14010007  // BOS bilge cycles day history
#define BOS0CMH                                0x14010008  // BOS bilge cycles month history
#define BOS0ALRTS                              0x14010009  // BOS bilge alert status
#define BOS0ALRTE                              0x1401000a  // BOS bilge alert enabled
#define BOS0ACHT                               0x1401000b  // BOS bilge alert cycles per hour threshold
#define BOS0ACDT                               0x1401000c  // BOS bilge alert cycles per day threshold
#define BOS0ARTT                               0x1401000d  // BOS bilge alert run time threshold
#define BOS0WRNS                               0x1401000e  // BOS bilge warning status
#define BOS0WRNE                               0x1401000f  // BOS bilge warning enabled
#define BOS0WCHT                               0x14010010  // BOS bilge warning cycles per hour threshold
#define BOS0WCDT                               0x14010011  // BOS bilge warning cycles per day threshold
#define BOS0WRTT                               0x14010012  // BOS bilge warning run time threshold
#define TMS0                                   0x14020000  // Tank Monitoring Service class
#define TMS0AVL                                0x14020000  // Available
#define TMS0LVL                                0x14020001  // TMS tank level
#define TMS0TYPE                               0x14020002  // TMS tank type
#define TMS0ALRTS                              0x14020003  // TMS tank alert status
#define TMS0ALRTE                              0x14020004  // TMS tank alert enabled
#define TMS0ALTH                               0x14020005  // TMS tank low level alert threshold
#define TMS0AHTH                               0x14020006  // TMS tank high level alert threshold
#define TMS0ALAT                               0x14020007  // TMS tank level alert latency
#define TMS0WRNS                               0x14020008  // TMS tank warning status
#define TMS0WRNE                               0x14020009  // TMS tank warning enabled
#define TMS0WLTH                               0x1402000a  // TMS tank low warning alert threshold
#define TMS0WHTH                               0x1402000b  // TMS tank high warning alert threshold
#define TMS0WLAT                               0x1402000c  // TMS tank level warning latency
#define BSS0                                   0x14030000  // Boat Security Service class
#define BSS0AVL                                0x14030000  // Available
#define BSS0SEA                                0x14030001  // BSS armed
#define BSS0IEN                                0x14030002  // BSS intrusion enabled
#define BSS0ILA                                0x14030003  // BSS intrusion latency
#define BSS0IST                                0x14030004  // BSS intrusion status
#define BSS0ATE                                0x14030005  // BSS anti theft enabled
#define BSS0ATL                                0x14030006  // BSS anti theft latency
#define BSS0ATS                                0x14030007  // BSS anti theft status
#define BTS0                                   0x14040000  // Boat Tracking Service class
#define BTS0AVL                                0x14040000  // Available
#define BTS0LOC                                0x14040001  // BTS boat location
#define BTS0LAT                                0x14040002  // BTS boat latitude
#define BTS0LNG                                0x14040003  // BTS boat longitude
#define BTS0GFA                                0x14040004  // BTS geo fence armed
#define BTS0GFR                                0x14040005  // BTS geo fence radius
#define BTS0GFL                                0x14040006  // BTS geo fence latency
#define BTS0GFS                                0x14040007  // BTS geo fence status
#define BTS0LATO                               0x14040008  // BTS boat latitude origin
#define BTS0LNGO                               0x14040009  // BTS boat longitude origin
#define CLS0                                   0x14050000  // Climate Service class
#define CLS0AVL                                0x14050000  // Available
#define CLS0NAME                               0x14050001  // Name
#define CLS0SYSU                               0x14050002  // System units
#define USM0                                   0x14060000  // Update Service Manager class
#define USM0AVL                                0x14060000  // Available
#define USM0DD                                 0x14060001  // Download done
#define USM0STAT                               0x14060002  // Status
#define USM0PROG                               0x14060003  // Progress
#define USM0RRQ                                0x14060004  // Read request
#define USM0DATA                               0x14060005  // Data
#define USM0TBS                                0x14060006  // Transfer block size
#define USM0LIST                               0x14060007  // List of all fwids supported by the USM
#define USM0VER                                0x14060008  // Update process version to use/in use
#define USM0MODE                               0x14060009  // Update process mode to use/in use
#define USM0STATE                              0x1406000a  // State information about ongoing process
#define US0                                    0x14070000  // Update Service class
#define US0AVL                                 0x14070000  // Available
#define US0DATA                                0x14070001  // Data
#define US0STAT                                0x14070002  // Status
#define US0LIST                                0x14070003  // List fwids
#define US0BTR                                 0x14070004  // Block transfer result
#define US0RRQ                                 0x14070005  // Read request
#define US0PROG                                0x14070006  // Progress
#define SVC0                                   0x14080000  // System services class
#define SVC0AVL                                0x14080000  // Available
#define SVC0SYST                               0x14080001  // System time
#define SVC0TMZ                                0x14080002  // Time zone offset
#define SVC0NTPS                               0x14080003  // NTP server
#define SVC0TSRC                               0x14080004  // Time source
#define SVC0GPIOR                              0x14080005  // GPIO read
#define SVC0GPIOW                              0x14080006  // GPIO write
#define SVC0SYSTUPD                            0x14080007  // System time update
#define SUP0                                   0x14090000  // Supervision services class
#define SUP0AVL                                0x14090000  // Available
#define SUP0ACT                                0x14090001  // Active
#define SUP0TYPE                               0x14090002  // Type of supervision
#define SUP0PARAM                              0x14090003  // Parameter to get data from
#define SUP0STYPE                              0x14090004  // State supervision type
#define SUP0S1                                 0x14090005  // State 1 value
#define SUP0S2                                 0x14090006  // State 2 value
#define SUP0S1ALT                              0x14090007  // State 1 timeout [s]
#define SUP0S2ALT                              0x14090008  // State 2 timeout [s]
#define SUP0SALARM                             0x14090009  // State alarm status
#define SUP0LHUPLIM                            0x1409000a  // Level high upper limit
#define SUP0LHLOLIM                            0x1409000b  // Level high lower limit
#define SUP0LLUPLIM                            0x1409000c  // Level low upper limit
#define SUP0LLLOLIM                            0x1409000d  // Level low lower limit
#define SUP0LHALT                              0x1409000e  // Level High timeout [s]
#define SUP0LLALT                              0x1409000f  // Level Low timeout [s]
#define SUP0LALTINI                            0x14090010  // Level Initial timeout [s]
#define SUP0LALARM                             0x14090011  // Level alarm status
#define TKS0                                   0x15000000  // Tank sensor class
#define TKS0AVL                                0x15000000  // Available
#define TKS0SDEV                               0x15000001  // Sensor device
#define TKS0LVL                                0x15000002  // Level
#define LPGS0                                  0x15010000  // LPG Tank sensor class
#define LPGS0AVL                               0x15010000  // Available
#define LPGS0SDEV                              0x15010001  // Sensor device
#define LPGS0LVL                               0x15010002  // Level
#define LPGS0BV                                0x15010003  // Battery voltage
#define AL0                                    0x15020000  // Area Lights class
#define AL0AVL                                 0x15020000  // Available
#define AL0CMD                                 0x15020001  // Light command
#define MPC0                                   0x15030000  // Mopeka Pro Check class
#define MPC0AVL                                0x15030000  // Available
#define MPC0V                                  0x15030001  // Voltage
#define MPC0TEMP                               0x15030002  // Temperature
#define MPC0BUTTON                             0x15030003  // Button
#define MPC0RAWTL                              0x15030004  // Raw Tank Level
#define MPC0Q                                  0x15030005  // Quality
#define MPC0ID                                 0x15030006  // ID low 3 bytes
#define MPC0X                                  0x15030007  // X Acceleration
#define MPC0Y                                  0x15030008  // Y Acceleration
#define MPC0IDE                                0x15030009  // ID high 3 bytes
#define MPC0TH                                 0x1503000a  // Tank height
#define MPC0NAME                               0x1503000b  // Tank name
#define MPC0REGION                             0x1503000c  // Region
#define MPC0SIZE                               0x1503000d  // Tank size
#define MPC0THRESH                             0x1503000e  // Alarm threshold
#define MPC0UNIT                               0x1503000f  // Tank level unit
#define PVS0                                   0x16000000  // Password Validation Service class
#define PVS0AVL                                0x16000000  // Available
#define PVS0EKBLOB                             0x16000001  // Entered key blob
#define PVS0VSTAT                              0x16000002  // Validation status
#define PVS0ADDS                               0x16000003  // Add Status
#define PVS0ADD                                0x16000004  // Add
#define KEY0                                   0x16010000  // Key class
#define KEY0AVL                                0x16010000  // Available
#define KEY0DEL                                0x16010001  // Delete
#define KEY0NAME                               0x16010002  // Name
#define KEY0ENA                                0x16010003  // Key enable
#define KEY0DOOR                               0x16010004  // Door
#define KEY0DAY                                0x16010005  // Day
#define KEY0FRT                                0x16010006  // From Time
#define KEY0TOT                                0x16010007  // To Time
#define KEY0BLOB                               0x16010008  // Key blob
#define KEY0SENA                               0x16010009  // Schedule enable
#define PCS0                                   0x16020000  // Pincode Service class
#define PCS0AVL                                0x16020000  // Available
#define PCS0KIV                                0x16020001  // Key input enable
#define PCS0KPC                                0x16020002  // Key count
#define KP0                                    0x17000000  // Key pad class
#define KP0AVL                                 0x17000000  // Available
#define KP0KEYB                                0x17000001  // Key
#define KP0LONGPRESS                           0x17000002  // Longpress of key
#define BZ0                                    0x17010000  // Buzzer class
#define BZ0AVL                                 0x17010000  // Available
#define BZ0EVT                                 0x17010001  // Event
#define EW0                                    0x17020000  // Display class
#define EW0AVL                                 0x17020000  // Available
#define EW0ECD                                 0x17020001  // Error code
#define EW0WCD                                 0x17020002  // Warning code
#define EW0SECD                                0x17020003  // System error code
#define HMI0                                   0x17030000  // HMI Generic class
#define HMI0AVL                                0x17030000  // Available
#define HMI0VER                                0x17030001  // Version of HMI data
#define HMI0TEMPUNIT                           0x17030002  // Presented temperature unit (metric/imperial)
#define HMI0BACKLIGHT                          0x17030003  // Backlight control [0-100]
#define HMI0SOUND                              0x17030004  // Enabling/disabling of sound
#define HMI0TIMEFORMAT                         0x17030005  // Display time format
#define HMI0EVENT                              0x17030006  // Send and publish an event to HMI-module
#define HMI0VARDATA                            0x17030007  // Request varstate value
#define HMI0MUTE                               0x17030008  // Mute sound, overrides hmi0sound
#define HMI0CHILDLOCK                          0x17030009  // Childlock function on HMI
#define HMI0SCREENTIMEOUT                      0x1703000a  // Screen timeout to standby
#define LS0                                    0x18000000  // Log service class
#define LS0AVL                                 0x18000000  // Available
#define LS0ADD                                 0x18000001  // Add specification
#define LS0READ                                0x18000002  // Read log data request
#define LS0LSCFG                               0x18000003  // Query parameter, about specific features supported
#define LS0RAMSIZE                             0x18000004  // Max RAM size
#define LSCFG0                                 0x18010000  // Log specification class
#define LSCFG0AVL                              0x18010000  // Available
#define LSCFG0NAME                             0x18010001  // Name of log configuration.
#define LSCFG0CFG                              0x18010002  // Actual configuration string read from json object
#define LSCFG0READCFG                          0x18010003  // List of linked read log configurations
#define LSCFG0DEL                              0x18010004  // Remove specification
#define LSCFG0ACT                              0x18010005  // Activate
#define LSRCFG0                                0x18020000  // Log read specification class
#define LSRCFG0AVL                             0x18020000  // Available
#define LSRCFG0NAME                            0x18020001  // Name of log read configuration
#define LSRCFG0CFG                             0x18020002  // Actual log read configuration string from json object
#define LSRCFG0DATA                            0x18020003  // Data according to read configuration in JSON format
#define LSRCFG0BINDATA                         0x18020004  // Data according to read configuration in binary format
#define RULE0                                  0x19000000  // Rule Engine class
#define RULE0AVL                               0x19000000  // Available
#define RULE0NAME                              0x19000001  // Name
#define RULE0NUM                               0x19000002  // Number of rules
#define RULE0ACT                               0x19000003  // Active
#define MCCC0                                  0x1a000000  // Mobile Cooling Compressor controller class
#define MCCC0AVL                               0x1a000000  // Available
#define MCCC0PTYPE                             0x1a000001  // PRODUCT_TYPE
#define MCCC0NOCPT                             0x1a000002  // Number of compartments
#define MCCC0CPOW                              0x1a000003  // C(x)_POWER
#define MCCC0CTEMP                             0x1a000004  // C(x)_MEASURED_TEMPERATURE
#define MCCC0CSETTEMP                          0x1a000005  // C(x)_SET_TEMPERATURE
#define MCCC0ACPT                              0x1a000006  // ACTIVE_COMPARTMENT
#define MCCC0CDOOR                             0x1a000007  // C(x)_DOOR_OPEN
#define MCCC0CTEMPRNG                          0x1a000008  // C(x)_TEMPERATURE_RANGE
#define MCCC0CRECDRNG                          0x1a000009  // C(x)_RECOMMENDED_RANGE
#define MCCC0CTEMPOFS                          0x1a00000a  // C(x)_TEMPERATURE_OFFSET
#define MCCC0COOLERPOW                         0x1a00000b  // COOLER_POWER
#define MCCC0V                                 0x1a00000c  // BATTERY_VOLTAGE_LEVEL
#define MCCC0BATPROTLVL                        0x1a00000d  // BATTERY_PROTECTION_LEVEL
#define MCCC0COMPPOW                           0x1a00000e  // COMPRESSOR_POWER
#define MCCC0I                                 0x1a00000f  // DC_CURRENT_LEVEL
#define MCCC0POWSRC                            0x1a000010  // POWER_SOURCE
#define MCCC0ICEPOW                            0x1a000011  // ICEMAKER_POWER
#define MCCC0ERRST                             0x1a000012  // Error and alert status
#define MCCC0SN                                0x1a000013  // CC_SERIAL_NUMBER
#define MCCC0SKU                               0x1a000014  // Sku number of compressor controller board
#define MCCC0FWVER                             0x1a000015  // CC_FIRMWARE_VERSION
#define MCCC0PARTDET                           0x1a000016  // Partition board detected status
#define MCCC0ETEMP                             0x1a000017  // Ambient temperature (external to cooler)
#define MCCC0PARTDETENA                        0x1a000018  // Partition board sensor enabled
#define MCCC0BOOST                             0x1a000019  // Power boost status
#define RVCMGNT0                               0x1b000000  // RVC DGN Management class
#define RVCMGNT0AVL                            0x1b000000  // Available
#define RVCMGNT0REQ                            0x1b000001  // DGN request
#define RVCMGNT0ADDR                           0x1b000002  // Source address after address claim
#define RVCMGNT0RAW                            0x1b000003  // Send a raw DGN frame. Data format is a struct.
#define RVCMGNT0RAWRX                          0x1b000004  // Received DGN frame
#define RVCMGNT0RESETCONF                      0x1b000005  // General reset params
#define RVCMGNT0RESET                          0x1b000006  // Send general reset (0x17F00) to address
#define RVCMGNT0ACK                            0x1b000007  // ACK message (0xE800)
#define RVCMGNT0DOWNLOAD                       0x1b000008  // DOWNLOAD message (0x17D00)
#define RVCPIM0                                0x1b010000  // RVC DGN PIM (65259) class
#define RVCPIM0AVL                             0x1b010000  // Available
#define RVCPIM0SYNC                            0x1b010001  // DGN class sync command
#define RVCPIM0MAKE                            0x1b010002  // Make
#define RVCPIM0MDL                             0x1b010003  // Model
#define RVCPIM0SNR                             0x1b010004  // Serial number
#define RVCPIM0UNIT                            0x1b010005  // Unit
#define RVCTIME0                               0x1b020000  // RVC DGN Date Time (131071/131070) (status/command) class
#define RVCTIME0AVL                            0x1b020000  // Available
#define RVCTIME0SYNC                           0x1b020001  // DGN class sync command
#define RVCTIME0YEAR                           0x1b020002  // Year
#define RVCTIME0MONTH                          0x1b020003  // Month
#define RVCTIME0DAY                            0x1b020004  // Day
#define RVCTIME0HOUR                           0x1b020005  // Hour
#define RVCTIME0MIN                            0x1b020006  // Minute
#define RVCTIME0SEC                            0x1b020007  // Second
#define RVCTIME0TZ                             0x1b020008  // Timezone
#define RVCTIME0DAYOFWEEK                      0x1b020009  // Day of the week
#define RVCFURNACE0                            0x1b030000  // RVC DGN Furnace (131044/131043) (status/command) class
#define RVCFURNACE0AVL                         0x1b030000  // Available
#define RVCFURNACE0SYNC                        0x1b030001  // DGN class sync command
#define RVCFURNACE0INST                        0x1b030002  // Instance (zone)
#define RVCFURNACE0MODE                        0x1b030003  // Operating mode (manual=ignore thermostat commands)
#define RVCFURNACE0HSRC                        0x1b030004  // Heat source
#define RVCFURNACE0FSPD                        0x1b030005  // Circulation fan speed
#define RVCFURNACE0HLVL                        0x1b030006  // Heat ouput level
#define RVCFURNACE0DBAND                       0x1b030007  // Dead band (0-25 degrees)
#define RVCFURNACE0SDBAND                      0x1b030008  // Secondary stage Dead band (0-25 degrees)
#define RVCFURNACE0OCSTAT                      0x1b030009  // Zone over current status
#define RVCFURNACE0UCSTAT                      0x1b03000a  // Zone under current status
#define RVCFURNACE0TSTAT                       0x1b03000b  // Zone temperature status
#define RVCFURNACE0ISTAT                       0x1b03000c  // Zone analog input status
#define RVCTH0                                 0x1b040000  // RVC DGN Thermostat 1 (131042/130809) (status/command) class
#define RVCTH0AVL                              0x1b040000  // Available
#define RVCTH0SYNC                             0x1b040001  // DGN class sync command
#define RVCTH0INST                             0x1b040002  // Instance (zone)
#define RVCTH0MODE                             0x1b040003  // Operating mode
#define RVCTH0FMODE                            0x1b040004  // Fan mode
#define RVCTH0SMODE                            0x1b040005  // Schedule mode
#define RVCTH0FSPD                             0x1b040006  // Fan speed
#define RVCTH0HSET                             0x1b040007  // Setpoint temp - heat
#define RVCTH0CSET                             0x1b040008  // Setpoint temp - cool
#define RVCTH0STATUS                           0x1b040009  // DGN data
#define RVCTH0COMMAND                          0x1b04000a  // DGN data
#define RVCTHTWO0                              0x1b050000  // RVC DGN Thermostat 2 (130810/130808) (status/command) class
#define RVCTHTWO0AVL                           0x1b050000  // Available
#define RVCTHTWO0SYNC                          0x1b050001  // DGN class sync command
#define RVCTHTWO0INST                          0x1b050002  // Instance (zone)
#define RVCTHTWO0CSINST                        0x1b050003  // Current schedule instance
#define RVCTHTWO0NSINST                        0x1b050004  // Number of schedule instances
#define RVCTHTWO0NOISE                         0x1b050005  // Reduced noise mode
#define RVCTHSCHED0                            0x1b060000  // RVC DGN Thermostat Schedule 1 (130807/130805) (status/command) class
#define RVCTHSCHED0AVL                         0x1b060000  // Available
#define RVCTHSCHED0SYNC                        0x1b060001  // DGN class sync command
#define RVCTHSCHED0INST                        0x1b060002  // Instance (zone)
#define RVCTHSCHED0SINST                       0x1b060003  // Schedule mode instance
#define RVCTHSCHED0HOUR                        0x1b060004  // Start Hour, precision 1 hour
#define RVCTHSCHED0MIN                         0x1b060005  // Start minute, precision 1 m
#define RVCTHSCHED0HSET                        0x1b060006  // Setpoint temp - heat
#define RVCTHSCHED0CSET                        0x1b060007  // Setpoint temp - cool
#define RVCTHSCHEDTWO0                         0x1b070000  // RVC DGN Thermostat Schedule 2 (130806/130804) (status/command) class
#define RVCTHSCHEDTWO0AVL                      0x1b070000  // Available
#define RVCTHSCHEDTWO0SYNC                     0x1b070001  // DGN class sync command
#define RVCTHSCHEDTWO0INST                     0x1b070002  // Instance (zone)
#define RVCTHSCHEDTWO0SINST                    0x1b070003  // Schedule mode instance
#define RVCTHSCHEDTWO0SUN                      0x1b070004  // Sunday
#define RVCTHSCHEDTWO0MON                      0x1b070005  // Monday
#define RVCTHSCHEDTWO0TUE                      0x1b070006  // Tuesday
#define RVCTHSCHEDTWO0WED                      0x1b070007  // Wednesday
#define RVCTHSCHEDTWO0THU                      0x1b070008  // Thursday
#define RVCTHSCHEDTWO0FRI                      0x1b070009  // Friday
#define RVCTHSCHEDTWO0SAT                      0x1b07000a  // Saturday
#define RVCTHASTAT0                            0x1b080000  // RVC DGN Thermostat Ambient status (130972) class
#define RVCTHASTAT0AVL                         0x1b080000  // Available
#define RVCTHASTAT0SYNC                        0x1b080001  // DGN class sync command
#define RVCTHASTAT0INST                        0x1b080002  // Instance (zone)
#define RVCTHASTAT0TEMP                        0x1b080003  // Ambient temperature
#define RVCAC0                                 0x1b090000  // RVC DGN Air Conditioner (131041/131040) (status/command) class
#define RVCAC0AVL                              0x1b090000  // Available
#define RVCAC0SYNC                             0x1b090001  // DGN class sync command
#define RVCAC0INST                             0x1b090002  // Instance (zone)
#define RVCAC0MODE                             0x1b090003  // Operating mode (manual=ignore thermostat commands)
#define RVCAC0MFSPD                            0x1b090004  // Max fan speed
#define RVCAC0MOLVL                            0x1b090005  // Max air conditioning output level
#define RVCAC0FSPD                             0x1b090006  // Fan speed
#define RVCAC0OLVL                             0x1b090007  // Air conditioner output level
#define RVCAC0DBAND                            0x1b090008  // Dead band (0-25 degrees)
#define RVCAC0SDBAND                           0x1b090009  // Secondary stage Dead band (0-25 degrees)
#define RVCACTWO0                              0x1b0a0000  // RVC DGN Air Conditioner 2 status (130505) class
#define RVCACTWO0AVL                           0x1b0a0000  // Available
#define RVCACTWO0SYNC                          0x1b0a0001  // DGN class sync command
#define RVCACTWO0INST                          0x1b0a0002  // Instance (zone)
#define RVCACTWO0COMPSTAT                      0x1b0a0003  // Compressor status
#define RVCACTWO0NOISE                         0x1b0a0004  // Reduced noise mode
#define RVCACTWO0EXTTEMP                       0x1b0a0005  // Exterior temperature
#define RVCACTWO0COILTEMP                      0x1b0a0006  // Coil temperature
#define RVCACTWO0COILTEMPERR                   0x1b0a0007  // Coil temp error
#define RVCACTWO0COILFREEZE                    0x1b0a0008  // Coil freeze detected
#define RVCACTWO0EXTTEMPERR                    0x1b0a0009  // Exterior temperature error
#define RVCACTWO0DEFROST                       0x1b0a000a  // Defrost cycle active
#define RVCHPUMP0                              0x1b0b0000  // RVC DGN Heat Pump (130971/130970) (status/command) class
#define RVCHPUMP0AVL                           0x1b0b0000  // Available
#define RVCHPUMP0SYNC                          0x1b0b0001  // DGN class sync command
#define RVCHPUMP0INST                          0x1b0b0002  // Instance (zone)
#define RVCHPUMP0MODE                          0x1b0b0003  // Operating mode (manual=ignore thermostat commands)
#define RVCHPUMP0MOLVL                         0x1b0b0004  // Heat max output level
#define RVCHPUMP0OLVL                          0x1b0b0005  // Heat output level
#define RVCHPUMP0DBAND                         0x1b0b0006  // Dead band (0-25 degrees)
#define RVCHPUMP0SDBAND                        0x1b0b0007  // Secondary stage Dead band (0-25 degrees)
#define RVCHPUMP0FSPD                          0x1b0b0008  // Fan speed
#define RVCDIM0                                0x1b0c0000  // RVC DGN DC Dimmer 1 (131003/131001) (status/command) class
#define RVCDIM0AVL                             0x1b0c0000  // Available
#define RVCDIM0SYNC                            0x1b0c0001  // DGN class sync command
#define RVCDIM0INST                            0x1b0c0002  // Instance (zone)
#define RVCDIM0MSTBS                           0x1b0c0003  // Master brightness
#define RVCDIM0RBS                             0x1b0c0004  // Red brightness
#define RVCDIM0GBS                             0x1b0c0005  // Green brightness
#define RVCDIM0BBS                             0x1b0c0006  // Blue brightness
#define RVCDIM0WBS                             0x1b0c0007  // White brightness
#define RVCDIM0ON                              0x1b0c0008  // On duration (seconds) 0-14, 0 means always on
#define RVCDIM0OFF                             0x1b0c0009  // Off duration (seconds) 0-14, 0 means one-shot and then stay off
#define RVCDIMTWO0                             0x1b0d0000  // RVC DGN DC Dimmer 2 (131002/130779) (status/command) class
#define RVCDIMTWO0AVL                          0x1b0d0000  // Available
#define RVCDIMTWO0SYNC                         0x1b0d0001  // DGN class sync command
#define RVCDIMTWO0INST                         0x1b0d0002  // Instance (zone)
#define RVCDIMTWO0MCURR                        0x1b0d0003  // Master current (A)
#define RVCDIMTWO0RCURR                        0x1b0d0004  // Red current (A)
#define RVCDIMTWO0GCURR                        0x1b0d0005  // Green current (A)
#define RVCDIMTWO0BCURR                        0x1b0d0006  // Blue current (A)
#define RVCDIMTWO0WCURR                        0x1b0d0007  // White current (A)
#define RVCDIMTWO0MFAULT                       0x1b0d0008  // Master fault
#define RVCDIMTWO0RFAULT                       0x1b0d0009  // Red fault
#define RVCDIMTWO0GFAULT                       0x1b0d000a  // Green fault
#define RVCDIMTWO0BFAULT                       0x1b0d000b  // Blue fault
#define RVCDIMTWO0WFAULT                       0x1b0d000c  // White fault
#define RVCDIMTWO0GROUP                        0x1b0d000d  // Group (command)
#define RVCDIMTWO0LVLBS                        0x1b0d000e  // Desired level brightness (command)
#define RVCDIMTWO0CMD                          0x1b0d000f  // Command (command)
#define RVCDIMTWO0DEL_DUR                      0x1b0d0010  // Delay/duration (command)
#define RVCDIMTWO0ILOCK                        0x1b0d0011  // Interlock (command)
#define RVCDIMTWO0RTIME                        0x1b0d0012  // Ramp time (command)
#define RVCDIMTHR0                             0x1b0e0000  // RVC DGN DC Dimmer 3 status (130778) class
#define RVCDIMTHR0AVL                          0x1b0e0000  // Available
#define RVCDIMTHR0SYNC                         0x1b0e0001  // DGN class sync command
#define RVCDIMTHR0INST                         0x1b0e0002  // Instance (zone)
#define RVCDIMTHR0GROUP                        0x1b0e0003  // Group
#define RVCDIMTHR0LVLBS                        0x1b0e0004  // Operating brightness
#define RVCDIMTHR0LOCK                         0x1b0e0005  // Lock status
#define RVCDIMTHR0OCURR                        0x1b0e0006  // Overcurrent status
#define RVCDIMTHR0ORIDE                        0x1b0e0007  // Override status
#define RVCDIMTHR0EN                           0x1b0e0008  // Enable status
#define RVCDIMTHR0DEL_DUR                      0x1b0e0009  // Delay/duration
#define RVCDIMTHR0LCMD                         0x1b0e000a  // Last command
#define RVCDIMTHR0ILOCK                        0x1b0e000b  // Interlock status
#define RVCDIMTHR0LOAD                         0x1b0e000c  // Load status
#define RVCDIMTHR0UCURR                        0x1b0e000d  // Undercurrent status
#define RVCDIMTHR0MEM                          0x1b0e000e  // Last memory value
#define RVCRFAN0                               0x1b0f0000  // RVC DGN Roof Fan (130727/130726) (status/command) class
#define RVCRFAN0AVL                            0x1b0f0000  // Available
#define RVCRFAN0SYNC                           0x1b0f0001  // DGN class sync command
#define RVCRFAN0INST                           0x1b0f0002  // Instance (zone)
#define RVCRFAN0SYST                           0x1b0f0003  // System status
#define RVCRFAN0FMODE                          0x1b0f0004  // Fan mode
#define RVCRFAN0SMODE                          0x1b0f0005  // Speed mode
#define RVCRFAN0LGT                            0x1b0f0006  // Light
#define RVCRFAN0FSET                           0x1b0f0007  // Fan speed setting
#define RVCRFAN0WDIR                           0x1b0f0008  // Wind direction switch
#define RVCRFAN0DPOS                           0x1b0f0009  // Dome position switch (deprecated)
#define RVCRFAN0TEMP                           0x1b0f000a  // Ambient temperature
#define RVCRFAN0SETTEMP                        0x1b0f000b  // Set point
#define RVCRFANTWO0                            0x1b100000  // RVC DGN Roof Fan 2 (130531/130530) (status/command) class
#define RVCRFANTWO0AVL                         0x1b100000  // Available
#define RVCRFANTWO0SYNC                        0x1b100001  // DGN class sync command
#define RVCRFANTWO0INST                        0x1b100002  // Instance (zone)
#define RVCRFANTWO0DMODE                       0x1b100003  // Dome mode
#define RVCRFANTWO0DPOS                        0x1b100004  // Dome position 
#define RVCRFANTWO0RAINSNS                     0x1b100005  // Rain sensor
#define RVCRFANTWO0RAINSNSOV                   0x1b100006  // Rain sensor override
#define RVCRFANTWO0DSTATE                      0x1b100007  // Setpoint Controlled Dome State
#define RVCRFANTWO0DCFOFF                      0x1b100008  // Auto Close Dome on Fan Off
#define RVCRFANTWO0FOFFDC                      0x1b100009  // Auto Fan Off on Dome Close
#define RVCRFANTWO0FSPDST                      0x1b10000a  // Fan Speed Step Increment/Decrement
#define RVCRFANTWO0FSPDSTSUP                   0x1b10000b  // Fan Steps (Speeds) Supported
#define RVCRFANTWO0FSPDSTSET                   0x1b10000c  // Fan Speed Increment/Decrement Step
#define RVCHTR0                                0x1b110000  // RVC DGN Heater operation(130559/130558) (status/command) class
#define RVCHTR0AVL                             0x1b110000  // Available
#define RVCHTR0SYNC                            0x1b110001  // DGN class sync command
#define RVCHTR0INST                            0x1b110002  // Heater instance
#define RVCHTR0ESRC                            0x1b110003  // Energy source
#define RVCHTR0AIR                             0x1b110004  // Air heater command
#define RVCHTR0WTR                             0x1b110005  // Water heater command
#define RVCHTR0AMODE                           0x1b110006  // Air heater mode
#define RVCHTR0WTRMODE                         0x1b110007  // Water heater mode
#define RVCHTR0TTEMP                           0x1b110008  // Target room temperature
#define RVCHTR0SILFMAX                         0x1b110009  // Silent mode max fan speed
#define RVCHTR0VFMIN                           0x1b11000a  // Ventilation mode min fan speed
#define RVCHTR0UVTHRES                         0x1b11000b  // Under voltage threshold
#define RVCHTR0SYSU                            0x1b11000c  // System units
#define RVCHTRST0                              0x1b120000  // RVC DGN Heater status (130557) class
#define RVCHTRST0AVL                           0x1b120000  // Available
#define RVCHTRST0SYNC                          0x1b120001  // DGN class sync command
#define RVCHTRST0INST                          0x1b120002  // Heater instance
#define RVCHTRST0RTEMP                         0x1b120003  // Room temperature
#define RVCHTRST0WTEMP                         0x1b120004  // Water temperature
#define RVCHTRST0GAIRST                        0x1b120005  // Gas air heater state
#define RVCHTRST0GWTRST                        0x1b120006  // Gas water heater state
#define RVCHTRST0ACAVL                         0x1b120007  // AC present
#define RVCHTRST0ACAIRST                       0x1b120008  // AC air heater state
#define RVCHTRST0ACWTRST                       0x1b120009  // AC water heater state
#define RVCHMI0                                0x1b130000  // RVC DGN HMI status (130556) class
#define RVCHMI0AVL                             0x1b130000  // Available
#define RVCHMI0SYNC                            0x1b130001  // DGN class sync command
#define RVCHMI0INST                            0x1b130002  // HMI instance
#define RVCHMI0RTEMP                           0x1b130003  // Room temperature
#define RVCHMI0COM                             0x1b130004  // Heater communication
#define RVCHMI0VOLT                            0x1b130005  // Input voltage
#define RVCHMI0INSTST                          0x1b130006  // HMI Instance status
#define RVCHMI0ICIRC                           0x1b130007  // Internal circuitry
#define RVCHMI0FAVBTN                          0x1b130008  // Favorite button status
#define RVCHMI0MENUBTN                         0x1b130009  // Menu button status
#define RVCHMI0HOMEBTN                         0x1b13000a  // Home button status
#define RVCHTRSCHED0                           0x1b140000  // RVC DGN Heater scheduling (130555/130554) (status/command) class
#define RVCHTRSCHED0AVL                        0x1b140000  // Available
#define RVCHTRSCHED0SYNC                       0x1b140001  // DGN class sync command
#define RVCHTRSCHED0INST                       0x1b140002  // Heater instance
#define RVCHTRSCHED0ATOFFST                    0x1b140003  // Air heater timer off status
#define RVCHTRSCHED0ATOFFH                     0x1b140004  // Air heater timer off hour
#define RVCHTRSCHED0ATOFFM                     0x1b140005  // Air heater timer off minute
#define RVCHTRSCHED0ATONST                     0x1b140006  // Air heater timer on status
#define RVCHTRSCHED0ATONH                      0x1b140007  // Air heater timer on hour
#define RVCHTRSCHED0ATONM                      0x1b140008  // Air heater timer on minute
#define RVCHTRSCHED0WTONST                     0x1b140009  // Water heater timer on status
#define RVCHTRSCHED0WTONH                      0x1b14000a  // Water heater timer on hour
#define RVCHTRSCHED0WTONM                      0x1b14000b  // Water heater timer on minute
#define RVCHTRSCHED0WKEEP                      0x1b14000c  // Water heater keep on time
#define RVCHTRFAULT0                           0x1b150000  // RVC DGN Heater active faults codes (130553) class
#define RVCHTRFAULT0AVL                        0x1b150000  // Available
#define RVCHTRFAULT0SYNC                       0x1b150001  // DGN class sync command
#define RVCHTRFAULT0INST                       0x1b150002  // Heater instance
#define RVCHTRFAULT0WARN                       0x1b150003  // Warning fault active
#define RVCHTRFAULT0CRIT                       0x1b150004  // Critical fault active
#define RVCHTRFAULT0ONE                        0x1b150005  // Active fault code 1
#define RVCHTRFAULT0TWO                        0x1b150006  // Active fault code 2
#define RVCHTRFAULT0THREE                      0x1b150007  // Active fault code 3
#define RVCHTRFAULT0FOUR                       0x1b150008  // Active fault code 4
#define RVCHTRVER0                             0x1b160000  // RVC DGN Heater version numbers (130552) class
#define RVCHTRVER0AVL                          0x1b160000  // Available
#define RVCHTRVER0SYNC                         0x1b160001  // DGN class sync command
#define RVCHTRVER0INST                         0x1b160002  // Heater instance
#define RVCHTRVER0CMAJ                         0x1b160003  // Comfort MCU SW version major
#define RVCHTRVER0CMIN                         0x1b160004  // Comfort MCU SW version minor
#define RVCHTRVER0BMAJ                         0x1b160005  // Burner MCU SW version major
#define RVCHTRVER0BMIN                         0x1b160006  // Burner MCU SW version minor
#define RVCHTRVER0PCBA                         0x1b160007  // PCBA version
#define RVCHTRVER0PMAJ                         0x1b160008  // Protocol version major
#define RVCHTRVER0PMIN                         0x1b160009  // Protocol version minor
#define RVCPROP0                               0x1b170000  // RVC DGN Proprietary message (61184) class
#define RVCPROP0AVL                            0x1b170000  // Available
#define RVCPROP0SYNC                           0x1b170001  // DGN class sync command
#define RVCPROP0ADDR                           0x1b170002  // Address (rvc)
#define RVCPROP0DATA                           0x1b170003  // Data bytes 0-7
#define RVCDCSRC0                              0x1b180000  // RVC DGN DC Source Status 1 (131069) class
#define RVCDCSRC0AVL                           0x1b180000  // Available
#define RVCDCSRC0SYNC                          0x1b180001  // DGN class sync command
#define RVCDCSRC0INST                          0x1b180002  // DC instance
#define RVCDCSRC0PRIO                          0x1b180003  // Device priority
#define RVCDCSRC0VOLT                          0x1b180004  // DC voltage
#define RVCDCSRC0CURR                          0x1b180005  // DC current
#define RVCDCSRCTWO0                           0x1b190000  // RVC DGN DC Source Status 2 (131068) class
#define RVCDCSRCTWO0AVL                        0x1b190000  // Available
#define RVCDCSRCTWO0SYNC                       0x1b190001  // DGN class sync command
#define RVCDCSRCTWO0INST                       0x1b190002  // DC instance
#define RVCDCSRCTWO0PRIO                       0x1b190003  // Device priority
#define RVCDCSRCTWO0STEMP                      0x1b190004  // Source temperature
#define RVCDCSRCTWO0SOC                        0x1b190005  // State of charge (SOC)
#define RVCDCSRCTWO0TIMEREM                    0x1b190006  // Time remaining
#define RVCDCSRCTWO0TIMEREMTYPE                0x1b190007  // Time remaining interpretation
#define RVCDCSRCTHR0                           0x1b1a0000  // RVC DGN DC Source Status 3 (131067) class
#define RVCDCSRCTHR0AVL                        0x1b1a0000  // Available
#define RVCDCSRCTHR0SYNC                       0x1b1a0001  // DGN class sync command
#define RVCDCSRCTHR0INST                       0x1b1a0002  // DC instance
#define RVCDCSRCTHR0PRIO                       0x1b1a0003  // Device priority
#define RVCDCSRCTHR0SOH                        0x1b1a0004  // State of health 
#define RVCDCSRCTHR0CAP                        0x1b1a0005  // Capacity remaining (Ah)
#define RVCDCSRCTHR0RELCAP                     0x1b1a0006  // Relative capacity
#define RVCDCSRCTHR0RIPPLE                     0x1b1a0007  // AC RMS ripple
#define RVCDCSRCFOUR0                          0x1b1b0000  // RVC DGN DC Source Status 4 (130761) class
#define RVCDCSRCFOUR0AVL                       0x1b1b0000  // Available
#define RVCDCSRCFOUR0SYNC                      0x1b1b0001  // DGN class sync command
#define RVCDCSRCFOUR0INST                      0x1b1b0002  // DC instance
#define RVCDCSRCFOUR0PRIO                      0x1b1b0003  // Device priority
#define RVCDCSRCFOUR0CHGST                     0x1b1b0004  // Desired charge state
#define RVCDCSRCFOUR0VOLT                      0x1b1b0005  // Desired DC voltage
#define RVCDCSRCFOUR0CURR                      0x1b1b0006  // Desired DC current
#define RVCDCSRCFOUR0TYPE                      0x1b1b0007  // Battery type
#define RVCDCSRCFIVE0                          0x1b1c0000  // RVC DGN DC Source Status 5 (130760) class
#define RVCDCSRCFIVE0AVL                       0x1b1c0000  // Available
#define RVCDCSRCFIVE0SYNC                      0x1b1c0001  // DGN class sync command
#define RVCDCSRCFIVE0INST                      0x1b1c0002  // DC instance
#define RVCDCSRCFIVE0PRIO                      0x1b1c0003  // Device priority
#define RVCDCSRCFIVE0VOLT                      0x1b1c0004  // HP DC voltage
#define RVCDCSRCSIX0                           0x1b1d0000  // RVC DGN DC Source Status 6 (130759) class
#define RVCDCSRCSIX0AVL                        0x1b1d0000  // Available
#define RVCDCSRCSIX0SYNC                       0x1b1d0001  // DGN class sync command
#define RVCDCSRCSIX0INST                       0x1b1d0002  // DC instance
#define RVCDCSRCSIX0PRIO                       0x1b1d0003  // Device priority
#define RVCDCSRCSIX0HVLIM                      0x1b1d0004  // High voltage limit status
#define RVCDCSRCSIX0HVDIS                      0x1b1d0005  // High voltage disconnect status
#define RVCDCSRCSIX0LVLIM                      0x1b1d0006  // Low voltage limit status
#define RVCDCSRCSIX0LVDIS                      0x1b1d0007  // Low voltage disconnect status
#define RVCDCSRCSIX0LSOCLIM                    0x1b1d0008  // Low state of charge limit status
#define RVCDCSRCSIX0LSOCDIS                    0x1b1d0009  // Low state of charge disconnect status
#define RVCDCSRCSIX0LDCTEMPLIM                 0x1b1d000a  // Low DC source temperature limit status
#define RVCDCSRCSIX0LDCTEMPDIS                 0x1b1d000b  // Low DC source temperature disconnect status
#define RVCDCSRCSIX0HDCCURRLIM                 0x1b1d000c  // High current DC source limit status
#define RVCDCSRCSIX0HDCCURRDIS                 0x1b1d000d  // High current DC source disconnect status
#define RVCDCSRCSIX0HDCTEMPLIM                 0x1b1d000e  // High DC source temperature limit status
#define RVCDCSRCSIX0HDCTEMPDIS                 0x1b1d000f  // High DC source temperature disconnect status
#define RVCDCSRCSEV0                           0x1b1e0000  // RVC DGN DC Source Status 7 (130732) class
#define RVCDCSRCSEV0AVL                        0x1b1e0000  // Available
#define RVCDCSRCSEV0SYNC                       0x1b1e0001  // DGN class sync command
#define RVCDCSRCSEV0INST                       0x1b1e0002  // DC instance
#define RVCDCSRCSEV0PRIO                       0x1b1e0003  // Device priority
#define RVCDCSRCSEV0INPUT                      0x1b1e0004  // Today's Input Ampere Hours
#define RVCDCSRCSEV0OUTPUT                     0x1b1e0005  // Today's Output Ampere Hours
#define RVCDCSRCEIG0                           0x1b1f0000  // RVC DGN DC Source Status 8 (130731) class
#define RVCDCSRCEIG0AVL                        0x1b1f0000  // Available
#define RVCDCSRCEIG0SYNC                       0x1b1f0001  // DGN class sync command
#define RVCDCSRCEIG0INST                       0x1b1f0002  // DC instance
#define RVCDCSRCEIG0PRIO                       0x1b1f0003  // Device priority
#define RVCDCSRCEIG0INPUT                      0x1b1f0004  // Yesterday's Input Ampere Hours
#define RVCDCSRCEIG0OUTPUT                     0x1b1f0005  // Yesterday's Output Ampere Hours
#define RVCDCSRCNINE0                          0x1b200000  // RVC DGN DC Source Status 9 (130730) class
#define RVCDCSRCNINE0AVL                       0x1b200000  // Available
#define RVCDCSRCNINE0SYNC                      0x1b200001  // DGN class sync command
#define RVCDCSRCNINE0INST                      0x1b200002  // DC instance
#define RVCDCSRCNINE0PRIO                      0x1b200003  // Device priority
#define RVCDCSRCNINE0INPUT                     0x1b200004  // Day Before Yesterday's Input Ampere Hours
#define RVCDCSRCNINE0OUTPUT                    0x1b200005  // Day Before Yesterday's Output Ampere Hours
#define RVCDCSRCTEN0                           0x1b210000  // RVC DGN DC Source Status 10 (130729) class
#define RVCDCSRCTEN0AVL                        0x1b210000  // Available
#define RVCDCSRCTEN0SYNC                       0x1b210001  // DGN class sync command
#define RVCDCSRCTEN0INST                       0x1b210002  // DC instance
#define RVCDCSRCTEN0PRIO                       0x1b210003  // Device priority
#define RVCDCSRCTEN0INPUT                      0x1b210004  // Last 7 days Input Ampere Hours
#define RVCDCSRCTEN0OUTPUT                     0x1b210005  // Last 7 days Output Ampere Hours
#define RVCDCSRCELE0                           0x1b220000  // RVC DGN DC Source Status 11 (130725) class
#define RVCDCSRCELE0AVL                        0x1b220000  // Available
#define RVCDCSRCELE0SYNC                       0x1b220001  // DGN class sync command
#define RVCDCSRCELE0INST                       0x1b220002  // DC instance
#define RVCDCSRCELE0PRIO                       0x1b220003  // Device priority
#define RVCDCSRCELE0DISCHGST                   0x1b220004  // Discharge On/Off status
#define RVCDCSRCELE0CHGST                      0x1b220005  // Charge On/Off status
#define RVCDCSRCELE0CHGDET                     0x1b220006  // Charge detected
#define RVCDCSRCELE0RESST                      0x1b220007  // Reserve status
#define RVCDCSRCELE0CAPACITY                   0x1b220008  // Full capacity
#define RVCDCSRCELE0POWER                      0x1b220009  // DC power
#define RVCDCSRCTWE0                           0x1b230000  // RVC DGN DC Source Status 12 (130552) class
#define RVCDCSRCTWE0AVL                        0x1b230000  // Available
#define RVCDCSRCTWE0SYNC                       0x1b230001  // DGN class sync command
#define RVCDCSRCTWE0INST                       0x1b230002  // DC instance
#define RVCDCSRCTWE0PRIO                       0x1b230003  // Device priority
#define RVCDCSRCTWE0CYCLES                     0x1b230004  // Cycles
#define RVCDCSRCTWE0DEEP                       0x1b230005  // Deepest discharge depth
#define RVCDCSRCTWE0AVERAGE                    0x1b230006  // Average discharge depth
#define RVCDCSRCTHI0                           0x1b240000  // RVC DGN DC Source Status 13 (130535) class
#define RVCDCSRCTHI0AVL                        0x1b240000  // Available
#define RVCDCSRCTHI0SYNC                       0x1b240001  // DGN class sync command
#define RVCDCSRCTHI0INST                       0x1b240002  // DC instance
#define RVCDCSRCTHI0PRIO                       0x1b240003  // Device priority
#define RVCDCSRCTHI0LOWVOLT                    0x1b240004  // Lowest DC source voltage
#define RVCDCSRCTHI0HIGHVOLT                   0x1b240005  // Highest DC source voltage
#define RVCDCSRCCMD0                           0x1b250000  // RVC DGN DC Source Command (130724) class
#define RVCDCSRCCMD0AVL                        0x1b250000  // Available
#define RVCDCSRCCMD0SYNC                       0x1b250001  // DGN class sync command
#define RVCDCSRCCMD0INST                       0x1b250002  // DC instance
#define RVCDCSRCCMD0PWRST                      0x1b250003  // Desired Power On/Off status
#define RVCDCSRCCMD0CHGST                      0x1b250004  // Desired Charge On/Off status
#define RVCDCSRCCFG0                           0x1b260000  // RVC DGN DC Source Configuration (Status/Command) (130551/130550) class
#define RVCDCSRCCFG0AVL                        0x1b260000  // Available
#define RVCDCSRCCFG0SYNC                       0x1b260001  // DGN class sync command
#define RVCDCSRCCFG0INST                       0x1b260002  // DC instance
#define RVCDCSRCCFG0EXP                        0x1b260003  // Peukert exponent
#define RVCDCSRCCFG0COEFF                      0x1b260004  // Temperature coefficient
#define RVCDCSRCCFG0FACT                       0x1b260005  // Charge efficiency factor
#define RVCDCSRCCFG0PERIOD                     0x1b260006  // Time remaining averaging period
#define RVCDCSRCCFG0CAPACITY                   0x1b260007  // Full capacity
#define RVCDCSRCCFG0TAILCURR                   0x1b260008  // Tail current
#define RVCDCSRCCFGTWO0                        0x1b270000  // RVC DGN DC Source Configuration 2 (Status/Command ) (130549/130548) class
#define RVCDCSRCCFGTWO0AVL                     0x1b270000  // Available
#define RVCDCSRCCFGTWO0SYNC                    0x1b270001  // DGN class sync command
#define RVCDCSRCCFGTWO0INST                    0x1b270002  // DC instance
#define RVCDCSRCCFGTWO0CLRHIST                 0x1b270003  // Clear history
#define RVCDCSRCCFGTWO0SETCAP                  0x1b270004  // Set capacity to 100%
#define RVCDCSRCCFGTWO0CHGVOLT                 0x1b270005  // Charged voltage
#define RVCDCSRCCFGTWO0SHTVOLT                 0x1b270006  // Shunt voltage
#define RVCDCSRCCFGTWO0SHTCURR                 0x1b270007  // Shunt current
#define RVCDCSRCCFGTWO0RSTBATH                 0x1b270008  // Reset battery health to 100% 
#define RVCDCSRCCFGTWO0BATTYPE                 0x1b270009  // Sets the battery type
#define RVCDCSRCCONN0                          0x1b280000  // RVC DGN DC Source Connection Status (130512) class
#define RVCDCSRCCONN0AVL                       0x1b280000  // Available
#define RVCDCSRCCONN0SYNC                      0x1b280001  // DGN class sync command
#define RVCDCSRCCONN0INST                      0x1b280002  // Device instance
#define RVCDCSRCCONN0DSA                       0x1b280003  // Device DSA (Default Source Address)
#define RVCDCSRCCONN0FUNC                      0x1b280004  // Function
#define RVCDCSRCCONN0PRIM                      0x1b280005  // Primary DC instance
#define RVCDCSRCCONN0SEC                       0x1b280006  // Secondary DC instance
#define RVCDCSRCCFGTHR0                        0x1b290000  // RVC DGN DC Source Configuration 3 Command (130526) class
#define RVCDCSRCCFGTHR0AVL                     0x1b290000  // Available
#define RVCDCSRCCFGTHR0SYNC                    0x1b290001  // DGN class sync command
#define RVCDCSRCCFGTHR0INST                    0x1b290002  // Device instance
#define RVCDCSRCCFGTHR0DSA                     0x1b290003  // Device DSA (Default Source Address)
#define RVCDCSRCCFGTHR0FUNC                    0x1b290004  // Function
#define RVCDCSRCCFGTHR0PRIM                    0x1b290005  // Primary DC instance
#define RVCDCSRCCFGTHR0SEC                     0x1b290006  // Secondary DC instance
#define RVCDCDISCONN0                          0x1b2a0000  // RVC DGN DC Disconnection (Status/Command) (130768/130767) class
#define RVCDCDISCONN0AVL                       0x1b2a0000  // Available
#define RVCDCDISCONN0SYNC                      0x1b2a0001  // DGN class sync command
#define RVCDCDISCONN0INST                      0x1b2a0002  // Device instance
#define RVCDCDISCONN0STS                       0x1b2a0003  // Circuit Status
#define RVCDCDISCONN0CMD                       0x1b2a0004  // Command/Last command
#define RVCDCDISCONN0BYPASS                    0x1b2a0005  // Bypass Detect
#define RVCDCDISCONN0VOLT                      0x1b2a0006  // DC switched voltage
#define RVCDCDISCONN0CURR                      0x1b2a0007  // DC switched current
#define RVCBAT0                                0x1b2b0000  // RVC DGN Battery Status 1 (130709) class
#define RVCBAT0AVL                             0x1b2b0000  // Available
#define RVCBAT0SYNC                            0x1b2b0001  // DGN class sync command
#define RVCBAT0INST                            0x1b2b0002  // Battery instance
#define RVCBAT0DC                              0x1b2b0003  // DC instance
#define RVCBAT0VOLT                            0x1b2b0004  // DC voltage
#define RVCBAT0CURR                            0x1b2b0005  // DC current
#define RVCBATTWO0                             0x1b2c0000  // RVC DGN Battery Status 2 (130708) class
#define RVCBATTWO0AVL                          0x1b2c0000  // Available
#define RVCBATTWO0SYNC                         0x1b2c0001  // DGN class sync command
#define RVCBATTWO0INST                         0x1b2c0002  // Battery instance
#define RVCBATTWO0DC                           0x1b2c0003  // DC instance
#define RVCBATTWO0TEMP                         0x1b2c0004  // Source temperature
#define RVCBATTWO0SOC                          0x1b2c0005  // State of charge (SOC)
#define RVCBATTWO0TIME                         0x1b2c0006  // Time remaining
#define RVCBATTWO0TIMESTS                      0x1b2c0007  // Time remaining interpretation
#define RVCBATTHR0                             0x1b2d0000  // RVC DGN Battery Status 3 (130707) class
#define RVCBATTHR0AVL                          0x1b2d0000  // Available
#define RVCBATTHR0SYNC                         0x1b2d0001  // DGN class sync command
#define RVCBATTHR0INST                         0x1b2d0002  // Battery instance
#define RVCBATTHR0DC                           0x1b2d0003  // DC instance
#define RVCBATTHR0SOH                          0x1b2d0004  // State of health
#define RVCBATTHR0CAP                          0x1b2d0005  // Capacity remaining
#define RVCBATTHR0CAPREL                       0x1b2d0006  // Relative capacity
#define RVCBATTHR0RIPPLE                       0x1b2d0007  // AC RMS Ripple
#define RVCBATFOUR0                            0x1b2e0000  // RVC DGN Battery Status 4 (130706) class
#define RVCBATFOUR0AVL                         0x1b2e0000  // Available
#define RVCBATFOUR0SYNC                        0x1b2e0001  // DGN class sync command
#define RVCBATFOUR0INST                        0x1b2e0002  // Battery instance
#define RVCBATFOUR0DC                          0x1b2e0003  // DC instance
#define RVCBATFOUR0CHGST                       0x1b2e0004  // Desired charge state
#define RVCBATFOUR0VOLT                        0x1b2e0005  // Desired DC voltage
#define RVCBATFOUR0CURR                        0x1b2e0006  // Desired DC current
#define RVCBATFOUR0TYPE                        0x1b2e0007  // Battery Type
#define RVCBATSIX0                             0x1b2f0000  // RVC DGN Battery Status 6 (130704) class
#define RVCBATSIX0AVL                          0x1b2f0000  // Available
#define RVCBATSIX0SYNC                         0x1b2f0001  // DGN class sync command
#define RVCBATSIX0INST                         0x1b2f0002  // Battery instance
#define RVCBATSIX0DC                           0x1b2f0003  // DC instance
#define RVCBATSIX0HVLIM                        0x1b2f0004  // High Voltage Limit Status
#define RVCBATSIX0HVDIS                        0x1b2f0005  // High Voltage Disconnect Status
#define RVCBATSIX0LVLIM                        0x1b2f0006  // Low Voltage Limit Status
#define RVCBATSIX0LVDIS                        0x1b2f0007  // Low Voltage Disconnect Status
#define RVCBATSIX0LSOCLIM                      0x1b2f0008  // Low state of charge (SOC) limit
#define RVCBATSIX0LSOCDIS                      0x1b2f0009  // Low state of charge disconnect status
#define RVCBATSIX0LDCTEMPLIM                   0x1b2f000a  // Low DC source temperature limit status
#define RVCBATSIX0LDCTEMPDIS                   0x1b2f000b  // Low DC source temperature disconnect status
#define RVCBATSIX0HDCTEMPLIM                   0x1b2f000c  // High DC source temperature limit status
#define RVCBATSIX0HDCTEMPDIS                   0x1b2f000d  // High DC source temperature disconnect status
#define RVCBATSIX0HCURRLIM                     0x1b2f000e  // High Current DC Source Limit
#define RVCBATSIX0HCURRDIS                     0x1b2f000f  // High Current DC Source Disconnect
#define RVCBATSEV0                             0x1b300000  // RVC DGN Battery Status 7 (130703) class
#define RVCBATSEV0AVL                          0x1b300000  // Available
#define RVCBATSEV0SYNC                         0x1b300001  // DGN class sync command
#define RVCBATSEV0INST                         0x1b300002  // Battery instance
#define RVCBATSEV0DC                           0x1b300003  // DC instance
#define RVCBATSEV0INPUT                        0x1b300004  // Today's Input AmpereHours
#define RVCBATSEV0OUTPUT                       0x1b300005  // Today's Output AmpereHours
#define RVCBATEIG0                             0x1b310000  // RVC DGN Battery Status 8 (130702) class
#define RVCBATEIG0AVL                          0x1b310000  // Available
#define RVCBATEIG0SYNC                         0x1b310001  // DGN class sync command
#define RVCBATEIG0INST                         0x1b310002  // Battery instance
#define RVCBATEIG0DC                           0x1b310003  // DC instance
#define RVCBATEIG0INPUT                        0x1b310004  // Yesterday's Input AmpereHours
#define RVCBATEIG0OUTPUT                       0x1b310005  // Yesterday's Output AmpereHours
#define RVCBATNINE0                            0x1b320000  // RVC DGN Battery Status 9 (130701) class
#define RVCBATNINE0AVL                         0x1b320000  // Available
#define RVCBATNINE0SYNC                        0x1b320001  // DGN class sync command
#define RVCBATNINE0INST                        0x1b320002  // Battery instance
#define RVCBATNINE0DC                          0x1b320003  // DC instance
#define RVCBATNINE0INPUT                       0x1b320004  // Day Before Yesterday's Input AmpereHours
#define RVCBATNINE0OUTPUT                      0x1b320005  // Day Before Yesterday's Output AmpereHours
#define RVCBATTEN0                             0x1b330000  // RVC DGN Battery Status 10 (130700) class
#define RVCBATTEN0AVL                          0x1b330000  // Available
#define RVCBATTEN0SYNC                         0x1b330001  // DGN class sync command
#define RVCBATTEN0INST                         0x1b330002  // Battery instance
#define RVCBATTEN0DC                           0x1b330003  // DC instance
#define RVCBATTEN0INPUT                        0x1b330004  // Last 7 Days Input AmpereHours
#define RVCBATTEN0OUTPUT                       0x1b330005  // Last 7 Days Output AmpereHours
#define RVCBATELE0                             0x1b340000  // RVC DGN Battery Status 11 (130699) class
#define RVCBATELE0AVL                          0x1b340000  // Available
#define RVCBATELE0SYNC                         0x1b340001  // DGN class sync command
#define RVCBATELE0INST                         0x1b340002  // Battery instance
#define RVCBATELE0DC                           0x1b340003  // DC instance
#define RVCBATELE0DISCHGST                     0x1b340004  // Discharge On/Off status
#define RVCBATELE0CHGST                        0x1b340005  // Charge On/Off status
#define RVCBATELE0CHGDET                       0x1b340006  // Charge detected
#define RVCBATELE0RESST                        0x1b340007  // Reserve status
#define RVCBATELE0CAPACITY                     0x1b340008  // Full capacity
#define RVCBATELE0POWER                        0x1b340009  // DC power
#define RVCBATTWE0                             0x1b350000  // RVC DGN Battery Status 12 (130547) class
#define RVCBATTWE0AVL                          0x1b350000  // Available
#define RVCBATTWE0SYNC                         0x1b350001  // DGN class sync command
#define RVCBATTWE0INST                         0x1b350002  // Battery instance
#define RVCBATTWE0DC                           0x1b350003  // DC instance
#define RVCBATTWE0CYCLES                       0x1b350004  // Cycles
#define RVCBATTWE0DEEP                         0x1b350005  // Deepest discharge depth
#define RVCBATTWE0AVERAGE                      0x1b350006  // Average discharge depth
#define RVCBATTHI0                             0x1b360000  // RVC DGN Battery Status 13 (130546) class
#define RVCBATTHI0AVL                          0x1b360000  // Available
#define RVCBATTHI0SYNC                         0x1b360001  // DGN class sync command
#define RVCBATTHI0INST                         0x1b360002  // Battery instance
#define RVCBATTHI0DC                           0x1b360003  // DC instance
#define RVCBATTHI0LOWVOLT                      0x1b360004  // Lowest Battery voltage
#define RVCBATTHI0HIGHVOLT                     0x1b360005  // Highest Battery voltage
#define RVCBATCMD0                             0x1b370000  // RVC DGN Battery Command (130698) class
#define RVCBATCMD0AVL                          0x1b370000  // Available
#define RVCBATCMD0SYNC                         0x1b370001  // DGN class sync command
#define RVCBATCMD0INST                         0x1b370002  // Battery instance
#define RVCBATCMD0DC                           0x1b370003  // Set DC instance
#define RVCBATCMD0LOADST                       0x1b370004  // Desired Load On/Off status
#define RVCBATCMD0CHGST                        0x1b370005  // Desired Charge On/Off status
#define RVCBATCMD0HIST                         0x1b370006  // Clear History
#define RVCBATCMD0DETAILS                      0x1b370007  // Return module’s Cell details
#define RVCBATSUM0                             0x1b380000  // RVC DGN Battery Summary (130545) class
#define RVCBATSUM0AVL                          0x1b380000  // Available
#define RVCBATSUM0SYNC                         0x1b380001  // DGN class sync command
#define RVCBATSUM0INST                         0x1b380002  // Battery instance
#define RVCBATSUM0DC                           0x1b380003  // DC instance
#define RVCBATSUM0SERIES                       0x1b380004  // Series string
#define RVCBATSUM0MODCNT                       0x1b380005  // Module count
#define RVCBATSUM0CELLS                        0x1b380006  // Cells per module
#define RVCBATSUM0VOLTSTS                      0x1b380007  // Voltage Status
#define RVCBATSUM0TEMPSTS                      0x1b380008  // Temperature Status
#define RVCBATSUM0ADDR                         0x1b380009  // Address
#define RVCBATCELL0                            0x1b390000  // RVC DGN Cell Details (130525) class
#define RVCBATCELL0AVL                         0x1b390000  // Available
#define RVCBATCELL0SYNC                        0x1b390001  // DGN class sync command
#define RVCBATCELL0INST                        0x1b390002  // Battery instance
#define RVCBATCELL0MOD                         0x1b390003  // Module instance
#define RVCBATCELL0CELL                        0x1b390004  // Cell instance
#define RVCBATCELL0VOLTSTS                     0x1b390005  // Voltage status
#define RVCBATCELL0TEMPSTS                     0x1b390006  // Voltage status
#define RVCBATCELL0BAL                         0x1b390007  // Balancing
#define RVCBATCELL0VOLT                        0x1b390008  // Voltage
#define RVCBATCELL0TEMP                        0x1b390009  // Temperature
#define RVCSOLAR0                              0x1b3a0000  // RVC DGN Solar Controller (Status/Command) (130739/130737) class
#define RVCSOLAR0AVL                           0x1b3a0000  // Available
#define RVCSOLAR0SYNC                          0x1b3a0001  // DGN class sync command
#define RVCSOLAR0INST                          0x1b3a0002  // Solar controller instance
#define RVCSOLAR0VOLT                          0x1b3a0003  // Desired charge control voltage
#define RVCSOLAR0CURR                          0x1b3a0004  // Desired charge control current
#define RVCSOLAR0CURRPERC                      0x1b3a0005  // Charge current percent of maximum
#define RVCSOLAR0OPER                          0x1b3a0006  // Operating state
#define RVCSOLAR0POWUP                         0x1b3a0007  // Power-up state
#define RVCSOLAR0CLRHIST                       0x1b3a0008  // Clear history
#define RVCSOLAR0FORCE                         0x1b3a0009  // Force charge
#define RVCSOLARTWO0                           0x1b3b0000  // RVC DGN Solar Controller 2 Status (130693) class
#define RVCSOLARTWO0AVL                        0x1b3b0000  // Available
#define RVCSOLARTWO0SYNC                       0x1b3b0001  // DGN class sync command
#define RVCSOLARTWO0INST                       0x1b3b0002  // Solar controller instance
#define RVCSOLARTWO0VOLT                       0x1b3b0003  // Rated battery voltage
#define RVCSOLARTWO0CURR                       0x1b3b0004  // Rated charging current
#define RVCSOLARTHR0                           0x1b3c0000  // RVC DGN Solar Controller 3 Status (130692) class
#define RVCSOLARTHR0AVL                        0x1b3c0000  // Available
#define RVCSOLARTHR0SYNC                       0x1b3c0001  // DGN class sync command
#define RVCSOLARTHR0INST                       0x1b3c0002  // Solar controller instance
#define RVCSOLARTHR0VOLT                       0x1b3c0003  // Rated solar input voltage
#define RVCSOLARTHR0CURR                       0x1b3c0004  // Rated solar input current
#define RVCSOLARTHR0OPOW                       0x1b3c0005  // Rated solar over-power
#define RVCSOLARFOUR0                          0x1b3d0000  // RVC DGN Solar Controller 4 Status (130691) class
#define RVCSOLARFOUR0AVL                       0x1b3d0000  // Available
#define RVCSOLARFOUR0SYNC                      0x1b3d0001  // DGN class sync command
#define RVCSOLARFOUR0INST                      0x1b3d0002  // Solar controller instance
#define RVCSOLARFOUR0TODAY                     0x1b3d0003  // Today's amp-hours to battery
#define RVCSOLARFOUR0YESTERDAY                 0x1b3d0004  // Yesterday's amp-hours to battery
#define RVCSOLARFOUR0YESTERDAY2                0x1b3d0005  // Day before yesterday's amphours to battery
#define RVCSOLARFIVE0                          0x1b3e0000  // RVC DGN Solar Controller 5 Status (130690) class
#define RVCSOLARFIVE0AVL                       0x1b3e0000  // Available
#define RVCSOLARFIVE0SYNC                      0x1b3e0001  // DGN class sync command
#define RVCSOLARFIVE0INST                      0x1b3e0002  // Solar controller instance
#define RVCSOLARFIVE0DAYS7                     0x1b3e0003  // Last 7 days amp-hours to battery
#define RVCSOLARFIVE0POWER                     0x1b3e0004  // Cumulative power generation
#define RVCSOLARSIX0                           0x1b3f0000  // RVC DGN Solar Controller 6 Status (130689) class
#define RVCSOLARSIX0AVL                        0x1b3f0000  // Available
#define RVCSOLARSIX0SYNC                       0x1b3f0001  // DGN class sync command
#define RVCSOLARSIX0INST                       0x1b3f0002  // Solar controller instance
#define RVCSOLARSIX0DAYS                       0x1b3f0003  // Total number of operating days
#define RVCSOLARSIX0TEMP                       0x1b3f0004  // Solar charge controller measured temperature
#define RVCSOLARBAT0                           0x1b400000  // RVC DGN Solar Charge Controller Battery Status (130688) class
#define RVCSOLARBAT0AVL                        0x1b400000  // Available
#define RVCSOLARBAT0SYNC                       0x1b400001  // DGN class sync command
#define RVCSOLARBAT0INST                       0x1b400002  // Solar controller instance
#define RVCSOLARBAT0DC                         0x1b400003  // DC Source Instance
#define RVCSOLARBAT0PRIO                       0x1b400004  // Charger Priority
#define RVCSOLARBAT0VOLT                       0x1b400005  // Measured voltage
#define RVCSOLARBAT0CURR                       0x1b400006  // Measured current
#define RVCSOLARBAT0TEMP                       0x1b400007  // Measured temperature
#define RVCSOLARARR0                           0x1b410000  // RVC DGN Solar Charge Controller Solar Array Status (130559) class
#define RVCSOLARARR0AVL                        0x1b410000  // Available
#define RVCSOLARARR0SYNC                       0x1b410001  // DGN class sync command
#define RVCSOLARARR0INST                       0x1b410002  // Solar controller instance
#define RVCSOLARARR0VOLT                       0x1b410003  // Solar array measured voltage
#define RVCSOLARARR0CURR                       0x1b410004  // Solar array measured input current
#define RVCSOLARCFG0                           0x1b420000  // RVC DGN Solar Controller Configuration (Status/Command) (130738/130736) class
#define RVCSOLARCFG0AVL                        0x1b420000  // Available
#define RVCSOLARCFG0SYNC                       0x1b420001  // DGN class sync command
#define RVCSOLARCFG0INST                       0x1b420002  // Solar controller instance
#define RVCSOLARCFG0ALGO                       0x1b420003  // Charging algorithm
#define RVCSOLARCFG0MODE                       0x1b420004  // Controller mode
#define RVCSOLARCFG0SENSOR                     0x1b420005  // Battery sensor present
#define RVCSOLARCFG0LINK                       0x1b420006  // Linkage mode
#define RVCSOLARCFG0TYPE                       0x1b420007  // Battery type
#define RVCSOLARCFG0SIZE                       0x1b420008  // Battery bank size
#define RVCSOLARCFG0MAXCURR                    0x1b420009  // Maximum charging current
#define RVCSOLARCFGTWO0                        0x1b430000  // RVC DGN Solar Controller Configuration 2 (Status/Command) (130558/130557) class
#define RVCSOLARCFGTWO0AVL                     0x1b430000  // Available
#define RVCSOLARCFGTWO0SYNC                    0x1b430001  // DGN class sync command
#define RVCSOLARCFGTWO0INST                    0x1b430002  // Solar controller instance
#define RVCSOLARCFGTWO0BVOLT                   0x1b430003  // Bulk-absorption voltage
#define RVCSOLARCFGTWO0FVOLT                   0x1b430004  // Float voltage
#define RVCSOLARCFGTWO0CVOLT                   0x1b430005  // Charge return voltage
#define RVCSOLARCFGTHR0                        0x1b440000  // RVC DGN Solar Controller Configuration 3 (Status/Command) (130556/130555) class
#define RVCSOLARCFGTHR0AVL                     0x1b440000  // Available
#define RVCSOLARCFGTHR0SYNC                    0x1b440001  // DGN class sync command
#define RVCSOLARCFGTHR0INST                    0x1b440002  // Solar controller instance
#define RVCSOLARCFGTHR0UVWLIM                  0x1b440003  // Under-voltage warning voltage
#define RVCSOLARCFGTHR0HVLIM                   0x1b440004  // Battery high voltage limit voltage
#define RVCSOLARCFGTHR0LVLIM                   0x1b440005  // Battery low voltage limit voltage
#define RVCSOLARCFGFOUR0                       0x1b450000  // RVC DGN Solar Controller Configuration 4 (Status/Command) (130554/130553) class
#define RVCSOLARCFGFOUR0AVL                    0x1b450000  // Available
#define RVCSOLARCFGFOUR0SYNC                   0x1b450001  // DGN class sync command
#define RVCSOLARCFGFOUR0INST                   0x1b450002  // Solar controller instance
#define RVCSOLARCFGFOUR0HVLIMR                 0x1b450003  // Battery high voltage limit return voltage
#define RVCSOLARCFGFOUR0LVLIMR                 0x1b450004  // Battery low voltage limit return voltage
#define RVCSOLARCFGFOUR0LVLIMD                 0x1b450005  // Battery low voltage limit time delay
#define RVCSOLARCFGFOUR0DUR                    0x1b450006  // Absorption duration
#define RVCSOLARCFGFOUR0FACTOR                 0x1b450007  // Temperature compensation factor
#define RVCSOLARCFGFIVE0                       0x1b460000  // RVC DGN Solar Controller Configuration 5 (Status/Command) (130511/130510) class
#define RVCSOLARCFGFIVE0AVL                    0x1b460000  // Available
#define RVCSOLARCFGFIVE0SYNC                   0x1b460001  // DGN class sync command
#define RVCSOLARCFGFIVE0INST                   0x1b460002  // Solar controller instance
#define RVCSOLARCFGFIVE0PRIO                   0x1b460003  // Charger Priority
#define RVCSOLARCFGFIVE0ETEMPHLIM              0x1b460004  // External temperature sensor high temperature limit
#define RVCSOLARCFGFIVE0ETEMPLLIM              0x1b460005  // External temperature sensor low temperature limit
#define RVCSOLAREQ0                            0x1b470000  // RVC DGN Solar Equalization Status (130735) class
#define RVCSOLAREQ0AVL                         0x1b470000  // Available
#define RVCSOLAREQ0SYNC                        0x1b470001  // DGN class sync command
#define RVCSOLAREQ0INST                        0x1b470002  // Solar controller instance
#define RVCSOLAREQ0REM                         0x1b470003  // Time remaining
#define RVCSOLAREQ0PRECHG                      0x1b470004  // Pre-charging status
#define RVCSOLAREQ0LAST                        0x1b470005  // Time since last equalization
#define RVCSOLAREQCFG0                         0x1b480000  // RVC DGN Solar Equalization Configuration (Status/Command) (130734/130733) class
#define RVCSOLAREQCFG0AVL                      0x1b480000  // Available
#define RVCSOLAREQCFG0SYNC                     0x1b480001  // DGN class sync command
#define RVCSOLAREQCFG0INST                     0x1b480002  // Solar controller instance
#define RVCSOLAREQCFG0VOLT                     0x1b480003  // Equalization voltage
#define RVCSOLAREQCFG0TIME                     0x1b480004  // Equalization time
#define RVCSOLAREQCFG0INT                      0x1b480005  // Equalization interval (days)
#define RVCREFRIG0                             0x1b490000  // RVC DGN Refrigerator (Status/Command) (130515/130514) class
#define RVCREFRIG0AVL                          0x1b490000  // Available
#define RVCREFRIG0SYNC                         0x1b490001  // DGN class sync command
#define RVCREFRIG0INST                         0x1b490002  // Refrigerator instance
#define RVCREFRIG0CAV                          0x1b490003  // Cavity
#define RVCREFRIG0LGT                          0x1b490004  // Light
#define RVCREFRIG0DOOR                         0x1b490005  // Door switch
#define RVCREFRIG0CTEMP                        0x1b490006  // Current temperature
#define RVCREFRIG0SETTEMP                      0x1b490007  // Set temperature
#define RVCREFRIG0FUEL                         0x1b490008  // Fuel source
#define RVCREFRIG0MODE                         0x1b490009  // Refrigerator mode
#define RVCREFRIG0COMPSPD                      0x1b49000a  // Compressor speed
#define RVCDM0                                 0x1b4a0000  // RVC DGN Diagnostic Message (130762) class
#define RVCDM0AVL                              0x1b4a0000  // Available
#define RVCDM0SYNC                             0x1b4a0001  // DGN class sync command
#define RVCDM0OPST                             0x1b4a0002  // Operating status
#define RVCDM0YLAMPST                          0x1b4a0003  // Yellow lamp status (fault status)
#define RVCDM0RLAMPST                          0x1b4a0004  // Red lamp status (fault status)
#define RVCDM0DSA                              0x1b4a0005  // Default Source Address
#define RVCDM0SPN                              0x1b4a0006  // Service Point Number
#define RVCDM0FMI                              0x1b4a0007  // Failure Mode Identifier
#define RVCDM0OCNT                             0x1b4a0008  // Occurrence count
#define RVCDM0DSAEXT                           0x1b4a0009  // DSA extenstion, 0xFF if no extension defined
#define RVCDM0BANKSEL                          0x1b4a000a  // Bank select
#define RVCGENCFG0                             0x1b4b0000  // RVC DGN Generic Configuration Status (130776) class
#define RVCGENCFG0AVL                          0x1b4b0000  // Available
#define RVCGENCFG0SYNC                         0x1b4b0001  // DGN class sync command
#define RVCGENCFG0MC                           0x1b4b0002  // Manufacturing code
#define RVCGENCFG0FUNCINST                     0x1b4b0003  // Function instance
#define RVCGENCFG0FUNC                         0x1b4b0004  // Function
#define RVCGENCFG0FWREV                        0x1b4b0005  // Firmware revision
#define RVCGENCFG0CONFTYPE                     0x1b4b0006  // Configuration type
#define RVCGENCFG0CONFREV                      0x1b4b0007  // Configuration revision
#define RVCGENCFG0ADDR                         0x1b4b0008  // Source address
#define RVCGENRST0                             0x1b4c0000  // RVC DGN General Purpose Reset (98048) class
#define RVCGENRST0AVL                          0x1b4c0000  // Available
#define RVCGENRST0SYNC                         0x1b4c0001  // DGN class sync command
#define RVCGENRST0REBOOT                       0x1b4c0002  // Reboot
#define RVCGENRST0CLRFAULTS                    0x1b4c0003  // Clear faults
#define RVCGENRST0RSTDEFAULT                   0x1b4c0004  // Reset to default settings
#define RVCGENRST0RSTSTAT                      0x1b4c0005  // Reset statistics
#define RVCGENRST0TESTMODE                     0x1b4c0006  // Test mode
#define RVCGENRST0RSTOEM                       0x1b4c0007  // Reset to OEM-pecific settings
#define RVCGENRST0REBOOTBL                     0x1b4c0008  // Reboot/Enter bootloader mode
#define RVCCHRG0                               0x1b4d0000  // RVC DGN Charger Status/Command (131015/131013) class
#define RVCCHRG0AVL                            0x1b4d0000  // Available
#define RVCCHRG0SYNC                           0x1b4d0001  // DGN class sync command
#define RVCCHRG0INST                           0x1b4d0002  // Charger instance
#define RVCCHRG0VOLT                           0x1b4d0003  // Charger voltage
#define RVCCHRG0CURR                           0x1b4d0004  // Charger current
#define RVCCHRG0CURRMAX                        0x1b4d0005  // Charge current percent of maximum
#define RVCCHRG0OPERST                         0x1b4d0006  // Operating state
#define RVCCHRG0DEFST                          0x1b4d0007  // Default state on power- up
#define RVCCHRG0AUTO                           0x1b4d0008  // Auto recharge enable
#define RVCCHRG0FORCE                          0x1b4d0009  // Force charge
#define RVCCHRGTWO0                            0x1b4e0000  // RVC DGN Charger Status 2 (130723) class
#define RVCCHRGTWO0AVL                         0x1b4e0000  // Available
#define RVCCHRGTWO0SYNC                        0x1b4e0001  // DGN class sync command
#define RVCCHRGTWO0INST                        0x1b4e0002  // Charger instance
#define RVCCHRGTWO0PRIO                        0x1b4e0003  // Charger priority
#define RVCCHRGTWO0VOLT                        0x1b4e0004  // Charger voltage
#define RVCCHRGTWO0CURR                        0x1b4e0005  // Charger current
#define RVCCHRGTWO0TEMP                        0x1b4e0006  // Charger temperature
#define RVCCHRGTHREE0                          0x1b4f0000  // RVC Charger Status 3 (130506) class
#define RVCCHRGTHREE0AVL                       0x1b4f0000  // Available
#define RVCCHRGTHREE0SYNC                      0x1b4f0001  // DGN class sync command
#define RVCCHRGTHREE0INST                      0x1b4f0002  // Charger instance
#define RVCCHRGTHREE0DERST                     0x1b4f0003  // Derating status
#define RVCCHRGTHREE0DERREAS                   0x1b4f0004  // Derating reason
#define RVCCHRGCFG0                            0x1b500000  // RVC DGN Charger Configuration Status/Command (131014/131012) class
#define RVCCHRGCFG0AVL                         0x1b500000  // Available
#define RVCCHRGCFG0SYNC                        0x1b500001  // DGN class sync command
#define RVCCHRGCFG0INST                        0x1b500002  // Charger instance
#define RVCCHRGCFG0ALGO                        0x1b500003  // Charging algorithm
#define RVCCHRGCFG0MODE                        0x1b500004  // Charging mode
#define RVCCHRGCFG0BATSENSOR                   0x1b500005  // Battery sensor present
#define RVCCHRGCFG0LINE                        0x1b500006  // Charger installation line
#define RVCCHRGCFG0TYPE                        0x1b500007  // Battery type
#define RVCCHRGCFG0BANK                        0x1b500008  // Battery bank size
#define RVCCHRGCFG0MAXCURR                     0x1b500009  // Maximum charging current
#define RVCCHRGCFGTWO0                         0x1b510000  // RVC Charger Configuration 2 Status/Command (130966/130965) class
#define RVCCHRGCFGTWO0AVL                      0x1b510000  // Available
#define RVCCHRGCFGTWO0SYNC                     0x1b510001  // DGN class sync command
#define RVCCHRGCFGTWO0INST                     0x1b510002  // Charger instance
#define RVCCHRGCFGTWO0MAXCURR                  0x1b510003  // Maximum charge current as percent
#define RVCCHRGCFGTWO0RATELIMIT                0x1b510004  // Charge rate limit as percent of bank size
#define RVCCHRGCFGTWO0BREAKER                  0x1b510005  // Shore breaker size
#define RVCCHRGCFGTWO0BATTEMP                  0x1b510006  // Default battery temperature
#define RVCCHRGCFGTWO0RECHARGEVOLT             0x1b510007  // Recharge voltage
#define RVCCHRGCFGTHREE0                       0x1b520000  // RVC DGN Charger Configuration 3 Status/Command (130764/130763) class
#define RVCCHRGCFGTHREE0AVL                    0x1b520000  // Available
#define RVCCHRGCFGTHREE0SYNC                   0x1b520001  // DGN class sync command
#define RVCCHRGCFGTHREE0INST                   0x1b520002  // Charger instance
#define RVCCHRGCFGTHREE0BVOLT                  0x1b520003  // Bulk voltage
#define RVCCHRGCFGTHREE0AVOLT                  0x1b520004  // Absorption voltage
#define RVCCHRGCFGTHREE0FVOLT                  0x1b520005  // Float voltage
#define RVCCHRGCFGTHREE0TEMPCOMP               0x1b520006  // Temperature compensation constant (mV/K)
#define RVCCHRGCFGFOUR0                        0x1b530000  // RVC DGN Charger Configuration 4 Status/Command (130751/130750) class
#define RVCCHRGCFGFOUR0AVL                     0x1b530000  // Available
#define RVCCHRGCFGFOUR0SYNC                    0x1b530001  // DGN class sync command
#define RVCCHRGCFGFOUR0INST                    0x1b530002  // Charger instance
#define RVCCHRGCFGFOUR0BTIME                   0x1b530003  // Bulk time
#define RVCCHRGCFGFOUR0ATIME                   0x1b530004  // Absorption time
#define RVCCHRGCFGFOUR0FTIME                   0x1b530005  // Float time
#define RVCCHRGEQ0                             0x1b540000  // RVC DGN Charger Equalization Status (130969) class
#define RVCCHRGEQ0AVL                          0x1b540000  // Available
#define RVCCHRGEQ0SYNC                         0x1b540001  // DGN class sync command
#define RVCCHRGEQ0INST                         0x1b540002  // Charger instance
#define RVCCHRGEQ0TIME                         0x1b540003  // Time remaining
#define RVCCHRGEQ0PRECHGST                     0x1b540004  // Pre-charging status
#define RVCCHRGEQCFG0                          0x1b550000  // RVC DGN Charger Equalization Configuration Status/Command (130968/130967) class
#define RVCCHRGEQCFG0AVL                       0x1b550000  // Available
#define RVCCHRGEQCFG0SYNC                      0x1b550001  // DGN class sync command
#define RVCCHRGEQCFG0INST                      0x1b550002  // Charger instance
#define RVCCHRGEQCFG0VOLT                      0x1b550003  // Equalization voltage
#define RVCCHRGEQCFG0TIME                      0x1b550004  // Equalization time
#define RVCCHRGAC0                             0x1b560000  // RVC DGN Charger AC Status 1 (131018) class
#define RVCCHRGAC0AVL                          0x1b560000  // Available
#define RVCCHRGAC0SYNC                         0x1b560001  // DGN class sync command
#define RVCCHRGAC0INST                         0x1b560002  // Charger instance
#define RVCCHRGAC0LINE                         0x1b560003  // Line
#define RVCCHRGAC0IO                           0x1b560004  // Input / Output
#define RVCCHRGAC0VOLT                         0x1b560005  // RMS voltage
#define RVCCHRGAC0CURR                         0x1b560006  // RMS current
#define RVCCHRGAC0FREQ                         0x1b560007  // Frequency (Hz)
#define RVCCHRGAC0OPENGROUND                   0x1b560008  // Fault – open ground
#define RVCCHRGAC0OPENNEUTRAL                  0x1b560009  // Fault – open neutral
#define RVCCHRGAC0REVPOL                       0x1b56000a  // Fault – reverse polarity
#define RVCCHRGAC0GROUNDCURR                   0x1b56000b  // Fault – ground current
#define RVCCHRGACTWO0                          0x1b570000  // RVC DGN Charger AC Status 2 (131017) class
#define RVCCHRGACTWO0AVL                       0x1b570000  // Available
#define RVCCHRGACTWO0SYNC                      0x1b570001  // DGN class sync command
#define RVCCHRGACTWO0INST                      0x1b570002  // Charger instance
#define RVCCHRGACTWO0LINE                      0x1b570003  // Line
#define RVCCHRGACTWO0IO                        0x1b570004  // Input / Output
#define RVCCHRGACTWO0PVOLT                     0x1b570005  // Peak voltage
#define RVCCHRGACTWO0PCURR                     0x1b570006  // Peak current
#define RVCCHRGACTWO0GCURR                     0x1b570007  // Ground current
#define RVCCHRGACTWO0CAP                       0x1b570008  // Capacity
#define RVCCHRGACTHREE0                        0x1b580000  // RVC DGN Charger AC Status 3 (131016) class
#define RVCCHRGACTHREE0AVL                     0x1b580000  // Available
#define RVCCHRGACTHREE0SYNC                    0x1b580001  // DGN class sync command
#define RVCCHRGACTHREE0INST                    0x1b580002  // Charger instance
#define RVCCHRGACTHREE0LINE                    0x1b580003  // Line
#define RVCCHRGACTHREE0IO                      0x1b580004  // Input / Output
#define RVCCHRGACTHREE0WAVEFORM                0x1b580005  // Waveform
#define RVCCHRGACTHREE0PHASE                   0x1b580006  // Phase status
#define RVCCHRGACTHREE0REALPOW                 0x1b580007  // Real power
#define RVCCHRGACTHREE0REACTPOW                0x1b580008  // Reactive power
#define RVCCHRGACTHREE0HARMDIST                0x1b580009  // Harmonic distortion
#define RVCCHRGACTHREE0COMPLLEG                0x1b58000a  // Complementary leg
#define RVCCHRGACFOUR0                         0x1b590000  // RVC DGN Charger AC Status 4 (130954) class
#define RVCCHRGACFOUR0AVL                      0x1b590000  // Available
#define RVCCHRGACFOUR0SYNC                     0x1b590001  // DGN class sync command
#define RVCCHRGACFOUR0INST                     0x1b590002  // Charger instance
#define RVCCHRGACFOUR0LINE                     0x1b590003  // Line
#define RVCCHRGACFOUR0IO                       0x1b590004  // Input / Output
#define RVCCHRGACFOUR0VOLTFAULT                0x1b590005  // Voltage fault
#define RVCCHRGACFOUR0SURGEFAULT               0x1b590006  // Fault – Surge protection
#define RVCCHRGACFOUR0HFREQFAULT               0x1b590007  // Fault – High frequency
#define RVCCHRGACFOUR0LFREQFAULT               0x1b590008  // Fault – Low frequency
#define RVCCHRGACFOUR0BYPASS                   0x1b590009  // Bypass mode active
#define RVCCHRGACFOUR0QUAL                     0x1b59000a  // Qualification status
#define RVCCHRGACFAULTCFG0                     0x1b5a0000  // RVC DGN Charger AC Fault Configuration Status/Command (130953/130951) class
#define RVCCHRGACFAULTCFG0AVL                  0x1b5a0000  // Available
#define RVCCHRGACFAULTCFG0SYNC                 0x1b5a0001  // DGN class sync command
#define RVCCHRGACFAULTCFG0INST                 0x1b5a0002  // Charger instance
#define RVCCHRGACFAULTCFG0EXTLVOLT             0x1b5a0003  // Extreme low voltage level
#define RVCCHRGACFAULTCFG0LVOLT                0x1b5a0004  // Low voltage level
#define RVCCHRGACFAULTCFG0HVOLT                0x1b5a0005  // High voltage level
#define RVCCHRGACFAULTCFG0EXTHVOLT             0x1b5a0006  // Extreme high voltage level
#define RVCCHRGACFAULTCFG0QUALTIME             0x1b5a0007  // Qualification time
#define RVCCHRGACFAULTCFG0BYPASS               0x1b5a0008  // Bypass mode
#define RVCCHRGACFAULTCFGTWO0                  0x1b5b0000  // RVC DGN Charger AC Fault Configuration 2 Status/Command (130952/130950) class
#define RVCCHRGACFAULTCFGTWO0AVL               0x1b5b0000  // Available
#define RVCCHRGACFAULTCFGTWO0SYNC              0x1b5b0001  // DGN class sync command
#define RVCCHRGACFAULTCFGTWO0INST              0x1b5b0002  // Charger instance
#define RVCCHRGACFAULTCFGTWO0HFREQ             0x1b5b0003  // High frequency limit (Hz)
#define RVCCHRGACFAULTCFGTWO0LFREQ             0x1b5b0004  // Low frequency limit (Hz)
#define RVCINVERTAC0                           0x1b5c0000  // RVC DGN Inverter AC Status 1 (131031) class
#define RVCINVERTAC0AVL                        0x1b5c0000  // Available
#define RVCINVERTAC0SYNC                       0x1b5c0001  // DGN class sync command
#define RVCINVERTAC0INST                       0x1b5c0002  // Inverter instance
#define RVCINVERTAC0LINE                       0x1b5c0003  // Line
#define RVCINVERTAC0IO                         0x1b5c0004  // Input / Output
#define RVCINVERTAC0VOLT                       0x1b5c0005  // RMS voltage
#define RVCINVERTAC0CURR                       0x1b5c0006  // RMS current
#define RVCINVERTAC0FREQ                       0x1b5c0007  // Frequency (Hz)
#define RVCINVERTAC0OPENGROUND                 0x1b5c0008  // Fault – open ground
#define RVCINVERTAC0OPENNEUTRAL                0x1b5c0009  // Fault – open neutral
#define RVCINVERTAC0REVPOL                     0x1b5c000a  // Fault – reverse polarity
#define RVCINVERTAC0GROUNDCURR                 0x1b5c000b  // Fault – ground current
#define RVCINVERTACTWO0                        0x1b5d0000  // RVC DGN Inverter AC Status 2 (131030) class
#define RVCINVERTACTWO0AVL                     0x1b5d0000  // Available
#define RVCINVERTACTWO0SYNC                    0x1b5d0001  // DGN class sync command
#define RVCINVERTACTWO0INST                    0x1b5d0002  // Inverter instance
#define RVCINVERTACTWO0LINE                    0x1b5d0003  // Line
#define RVCINVERTACTWO0IO                      0x1b5d0004  // Input / Output
#define RVCINVERTACTWO0PVOLT                   0x1b5d0005  // Peak voltage
#define RVCINVERTACTWO0PCURR                   0x1b5d0006  // Peak current
#define RVCINVERTACTWO0GCURR                   0x1b5d0007  // Ground current
#define RVCINVERTACTWO0CAP                     0x1b5d0008  // Capacity
#define RVCINVERTACTHREE0                      0x1b5e0000  // RVC DGN Inverter AC Status 3 (131029) class
#define RVCINVERTACTHREE0AVL                   0x1b5e0000  // Available
#define RVCINVERTACTHREE0SYNC                  0x1b5e0001  // DGN class sync command
#define RVCINVERTACTHREE0INST                  0x1b5e0002  // Inverter instance
#define RVCINVERTACTHREE0LINE                  0x1b5e0003  // Line
#define RVCINVERTACTHREE0IO                    0x1b5e0004  // Input / Output
#define RVCINVERTACTHREE0WAVEFORM              0x1b5e0005  // Waveform
#define RVCINVERTACTHREE0PHASE                 0x1b5e0006  // Phase status
#define RVCINVERTACTHREE0REALPOW               0x1b5e0007  // Real power
#define RVCINVERTACTHREE0REACTPOW              0x1b5e0008  // Reactive power
#define RVCINVERTACTHREE0HARMDIST              0x1b5e0009  // Harmonic distortion
#define RVCINVERTACTHREE0COMPLLEG              0x1b5e000a  // Complementary leg
#define RVCINVERTACFOUR0                       0x1b5f0000  // RVC DGN Inverter AC Status 4 (130959) class
#define RVCINVERTACFOUR0AVL                    0x1b5f0000  // Available
#define RVCINVERTACFOUR0SYNC                   0x1b5f0001  // DGN class sync command
#define RVCINVERTACFOUR0INST                   0x1b5f0002  // Inverter instance
#define RVCINVERTACFOUR0LINE                   0x1b5f0003  // Line
#define RVCINVERTACFOUR0IO                     0x1b5f0004  // Input / Output
#define RVCINVERTACFOUR0VOLTFAULT              0x1b5f0005  // Voltage fault
#define RVCINVERTACFOUR0SURGEFAULT             0x1b5f0006  // Fault – Surge protection
#define RVCINVERTACFOUR0HFREQFAULT             0x1b5f0007  // Fault – High frequency
#define RVCINVERTACFOUR0LFREQFAULT             0x1b5f0008  // Fault – Low frequency
#define RVCINVERTACFOUR0BYPASS                 0x1b5f0009  // Bypass mode active
#define RVCINVERTACFOUR0QUAL                   0x1b5f000a  // Qualification status
#define RVCINVERT0                             0x1b600000  // RVC DGN Inverter Status/Command (131028/131027) class
#define RVCINVERT0AVL                          0x1b600000  // Available
#define RVCINVERT0SYNC                         0x1b600001  // DGN class sync command
#define RVCINVERT0INST                         0x1b600002  // Inverter instance
#define RVCINVERT0STATUS                       0x1b600003  // Status
#define RVCINVERT0BATTEMP                      0x1b600004  // Battery temperature sensor present
#define RVCINVERT0LOADSENSE                    0x1b600005  // Load sense enabled
#define RVCINVERT0ENABLE                       0x1b600006  // Inverter enabled
#define RVCINVERT0PASSTHR                      0x1b600007  // Pass-through enable
#define RVCINVERT0GENERATOR                    0x1b600008  // Generator support enabled
#define RVCINVERT0ENABLESTART                  0x1b600009  // Inverter enabled on startup
#define RVCINVERT0LOADSENSESTART               0x1b60000a  // Load sense enabled on startup
#define RVCINVERT0PASSTHRSTART                 0x1b60000b  // AC pass-through enable on startup
#define RVCINVERT0GENERATORSTART               0x1b60000c  // Generator support enabled on startup
#define RVCINVERTCFG0                          0x1b610000  // RVC DGN Inverter Configuration 1 Status/Command (131026/131024) class
#define RVCINVERTCFG0AVL                       0x1b610000  // Available
#define RVCINVERTCFG0SYNC                      0x1b610001  // DGN class sync command
#define RVCINVERTCFG0INST                      0x1b610002  // Inverter instance
#define RVCINVERTCFG0LOADSENSEPOWTH            0x1b610003  // Load sense power threshold
#define RVCINVERTCFG0LOADSENSEINT              0x1b610004  // Load sense interval
#define RVCINVERTCFG0SHUTDOWNVOLTMIN           0x1b610005  // DC source shutdown voltage – Minimum
#define RVCINVERTCFG0ENABLESTART               0x1b610006  // Inverter enabled on startup
#define RVCINVERTCFG0LOADSENSESTART            0x1b610007  // Load sense enabled on startup
#define RVCINVERTCFG0PASSTHRSTART              0x1b610008  // AC pass-through enable on startup
#define RVCINVERTCFGTWO0                       0x1b620000  // RVC DGN Inverter Configuration 2 Status/Command (131025/131023) class
#define RVCINVERTCFGTWO0AVL                    0x1b620000  // Available
#define RVCINVERTCFGTWO0SYNC                   0x1b620001  // DGN class sync command
#define RVCINVERTCFGTWO0INST                   0x1b620002  // Inverter instance
#define RVCINVERTCFGTWO0SHUTDOWNVOLTMAX        0x1b620003  // DC source shutdown voltage – Maximum
#define RVCINVERTCFGTWO0WARNVOLTMIN            0x1b620004  // DC source warning voltage - Minimum
#define RVCINVERTCFGTWO0WARNVOLTMAX            0x1b620005  // DC source warning voltage - Maximum
#define RVCINVERTCFGTHREE0                     0x1b630000  // RVC DGN Inverter Configuration 3 Status/Command (130766/130765) class
#define RVCINVERTCFGTHREE0AVL                  0x1b630000  // Available
#define RVCINVERTCFGTHREE0SYNC                 0x1b630001  // DGN class sync command
#define RVCINVERTCFGTHREE0INST                 0x1b630002  // Inverter instance
#define RVCINVERTCFGTHREE0SHUTDOWNDELAY        0x1b630003  // DC Source shutdown delay
#define RVCINVERTCFGTHREE0STACKMODE            0x1b630004  // Stack mode
#define RVCINVERTCFGTHREE0SHUTDOWNRECLVL       0x1b630005  // DC Source shutdown - Recovery Level
#define RVCINVERTCFGTHREE0GENCURR              0x1b630006  // Generator Support Engage Current
#define RVCINVERTCFGFOUR0                      0x1b640000  // RVC DGN Inverter Configuration 4 Status/Command (130715/130714) class
#define RVCINVERTCFGFOUR0AVL                   0x1b640000  // Available
#define RVCINVERTCFGFOUR0SYNC                  0x1b640001  // DGN class sync command
#define RVCINVERTCFGFOUR0INST                  0x1b640002  // Inverter instance
#define RVCINVERTCFGFOUR0VOLT                  0x1b640003  // Output AC voltage
#define RVCINVERTCFGFOUR0FREQ                  0x1b640004  // Output frequency (Hz)
#define RVCINVERTCFGFOUR0POWLIMIT              0x1b640005  // AC Output power limit
#define RVCINVERTCFGFOUR0TIMELIMIT             0x1b640006  // AC Output power time limit
#define RVCINVERTSTAT0                         0x1b650000  // RVC DGN Inverter Statistics Status (131022) class
#define RVCINVERTSTAT0AVL                      0x1b650000  // Available
#define RVCINVERTSTAT0SYNC                     0x1b650001  // DGN class sync command
#define RVCINVERTSTAT0INST                     0x1b650002  // Inverter instance
#define RVCINVERTSTAT0NUMUVOLT                 0x1b650003  // Number of DC under voltage detections
#define RVCINVERTSTAT0NUMOVERLOAD              0x1b650004  // Number of inverter AC output over-loads
#define RVCINVERTSTAT0NUMLOADSENSE             0x1b650005  // Number of times load sense has been engaged
#define RVCINVERTSTAT0LOWDCVOLT                0x1b650006  // Lowest DC voltage
#define RVCINVERTSTAT0HIGHDCVOLT               0x1b650007  // Highest DC voltage
#define RVCINVERTSTAT0LOWACIVOLT               0x1b650008  // Lowest AC input voltage
#define RVCINVERTSTAT0HIGHACIVOLT              0x1b650009  // Highest AC input voltage
#define RVCINVERTSTAT0LOWACOVOLT               0x1b65000a  // Lowest AC output voltage
#define RVCINVERTSTAT0HIGHACOVOLT              0x1b65000b  // Highest AC output voltage
#define RVCINVERTTEMP0                         0x1b660000  // RVC DGN Inverter Temperature Status (130749) class
#define RVCINVERTTEMP0AVL                      0x1b660000  // Available
#define RVCINVERTTEMP0SYNC                     0x1b660001  // DGN class sync command
#define RVCINVERTTEMP0INST                     0x1b660002  // Inverter instance
#define RVCINVERTTEMP0FETONE                   0x1b660003  // FET1 temperature
#define RVCINVERTTEMP0FETTWO                   0x1b660004  // FET2 temperature
#define RVCINVERTTEMP0TRANSF                   0x1b660005  // Transformer temperature
#define RVCINVERTTEMPTWO0                      0x1b670000  // RVC DGN Inverter Temperature Status 2 (130507) class
#define RVCINVERTTEMPTWO0AVL                   0x1b670000  // Available
#define RVCINVERTTEMPTWO0SYNC                  0x1b670001  // DGN class sync command
#define RVCINVERTTEMPTWO0INST                  0x1b670002  // Inverter instance
#define RVCINVERTTEMPTWO0PB                    0x1b670003  // Control/Power Board temperature
#define RVCINVERTTEMPTWO0CAPACITOR             0x1b670004  // Capacitor temperature
#define RVCINVERTTEMPTWO0AMBIENT               0x1b670005  // Ambient temperature
#define RVCINVERTDC0                           0x1b680000  // RVC DGN Inverter DC Status (130792) class
#define RVCINVERTDC0AVL                        0x1b680000  // Available
#define RVCINVERTDC0SYNC                       0x1b680001  // DGN class sync command
#define RVCINVERTDC0INST                       0x1b680002  // Inverter instance
#define RVCINVERTDC0VOLT                       0x1b680003  // DC voltage
#define RVCINVERTDC0CURR                       0x1b680004  // DC current
#define RVCATSAC0                              0x1b690000  // RVC DGN Transfer Switch AC Status 1 (130989) class
#define RVCATSAC0AVL                           0x1b690000  // Available
#define RVCATSAC0SYNC                          0x1b690001  // DGN class sync command
#define RVCATSAC0INST                          0x1b690002  // Transfer switch instance
#define RVCATSAC0SOURCE                        0x1b690003  // Source
#define RVCATSAC0IO                            0x1b690004  // Input / Output
#define RVCATSAC0LEG                           0x1b690005  // Leg
#define RVCATSAC0VOLT                          0x1b690006  // RMS voltage
#define RVCATSAC0CURR                          0x1b690007  // RMS current
#define RVCATSAC0FREQ                          0x1b690008  // Frequency (Hz)
#define RVCATSAC0OPENGROUND                    0x1b690009  // Fault – open ground
#define RVCATSAC0OPENNEUTRAL                   0x1b69000a  // Fault – open neutral
#define RVCATSAC0REVPOL                        0x1b69000b  // Fault – reverse polarity
#define RVCATSAC0GROUNDCURR                    0x1b69000c  // Fault – ground current
#define RVCATSACTWO0                           0x1b6a0000  // RVC DGN Transfer Switch AC Status 2 (130988) class
#define RVCATSACTWO0AVL                        0x1b6a0000  // Available
#define RVCATSACTWO0SYNC                       0x1b6a0001  // DGN class sync command
#define RVCATSACTWO0INST                       0x1b6a0002  // Transfer switch instance
#define RVCATSACTWO0SOURCE                     0x1b6a0003  // Source
#define RVCATSACTWO0IO                         0x1b6a0004  // Input / Output
#define RVCATSACTWO0LEG                        0x1b6a0005  // Leg
#define RVCATSACTWO0PVOLT                      0x1b6a0006  // Peak voltage
#define RVCATSACTWO0PCURR                      0x1b6a0007  // Peak current
#define RVCATSACTWO0GCURR                      0x1b6a0008  // Ground current
#define RVCATSACTWO0CAP                        0x1b6a0009  // Capacity
#define RVCATSACTHREE0                         0x1b6b0000  // RVC DGN Transfer Switch AC Status 3 (130987) class
#define RVCATSACTHREE0AVL                      0x1b6b0000  // Available
#define RVCATSACTHREE0SYNC                     0x1b6b0001  // DGN class sync command
#define RVCATSACTHREE0INST                     0x1b6b0002  // Transfer switch instance
#define RVCATSACTHREE0SOURCE                   0x1b6b0003  // Source
#define RVCATSACTHREE0IO                       0x1b6b0004  // Input / Output
#define RVCATSACTHREE0LEG                      0x1b6b0005  // Leg
#define RVCATSACTHREE0WAVEFORM                 0x1b6b0006  // Waveform
#define RVCATSACTHREE0PHASE                    0x1b6b0007  // Phase status
#define RVCATSACTHREE0REALPOW                  0x1b6b0008  // Real power
#define RVCATSACTHREE0REACTPOW                 0x1b6b0009  // Reactive power
#define RVCATSACTHREE0HARMDIST                 0x1b6b000a  // Harmonic distortion
#define RVCATSACTHREE0COMPLLEG                 0x1b6b000b  // Complementary leg
#define RVCATSACFOUR0                          0x1b6c0000  // RVC DGN Transfer Switch AC Status 4 (130949) class
#define RVCATSACFOUR0AVL                       0x1b6c0000  // Available
#define RVCATSACFOUR0SYNC                      0x1b6c0001  // DGN class sync command
#define RVCATSACFOUR0INST                      0x1b6c0002  // Transfer switch instance
#define RVCATSACFOUR0SOURCE                    0x1b6c0003  // Source
#define RVCATSACFOUR0IO                        0x1b6c0004  // Input / Output
#define RVCATSACFOUR0LEG                       0x1b6c0005  // Leg
#define RVCATSACFOUR0VOLTFAULT                 0x1b6c0006  // Voltage fault
#define RVCATSACFOUR0SURGEFAULT                0x1b6c0007  // Fault – Surge protection
#define RVCATSACFOUR0HFREQFAULT                0x1b6c0008  // Fault – High frequency
#define RVCATSACFOUR0LFREQFAULT                0x1b6c0009  // Fault – Low frequency
#define RVCATSACFOUR0BYPASS                    0x1b6c000a  // Bypass mode active
#define RVCATSACFOUR0QUAL                      0x1b6c000b  // Qualification status
#define RVCATSACFAULTCFG0                      0x1b6d0000  // RVC DGN ATS AC Fault Configuration Status (130948) class
#define RVCATSACFAULTCFG0AVL                   0x1b6d0000  // Available
#define RVCATSACFAULTCFG0SYNC                  0x1b6d0001  // DGN class sync command
#define RVCATSACFAULTCFG0INST                  0x1b6d0002  // Transfer switch instance
#define RVCATSACFAULTCFG0EXTLVOLT              0x1b6d0003  // Extreme low voltage level
#define RVCATSACFAULTCFG0LVOLT                 0x1b6d0004  // Low voltage level
#define RVCATSACFAULTCFG0HVOLT                 0x1b6d0005  // High voltage level
#define RVCATSACFAULTCFG0EXTHVOLT              0x1b6d0006  // Extreme high voltage level
#define RVCATSACFAULTCFG0QUALTIME              0x1b6d0007  // Qualification time
#define RVCATSACFAULTCFG0BYPASS                0x1b6d0008  // Bypass mode
#define PROD0                                  0x1c000000  // Product information class
#define PROD0AVL                               0x1c000000  // Available
#define PROD0NAME                              0x1c000001  // Display Name
#define PROD0SN                                0x1c000002  // Dometic Serial Number
#define PROD0SKU                               0x1c000003  // Stock Keeping Unit
#define PROD0PNC                               0x1c000004  // Product Numeric Code
#define PROD0FWVER                             0x1c000005  // Firmware version
#define PROD0HWVER                             0x1c000006  // Hardware version
#define PROD0MDL                               0x1c000007  // Model name
#define PROD0EAN                               0x1c000008  // EAN13
#define PROD0DESCRIPTION                       0x1c000009  // Descriptive text
#define PROD0CLIST                             0x1c00000a  // List of main DDM classes represented by product
#define PROD0UID                               0x1c00000b  // Unique Identification Number
#define PROD0PROP                              0x1c00000c  // Properties
#define PROD0MANUF                             0x1c00000d  // Manufacturer
#define PROD0RESET                             0x1c00000e  // Reset command
#define PROD0IND                               0x1c00000f  // Indicate request
#define PROD0FWID                              0x1c000010  // Base fwid of this product
#define EVM0                                   0x1d000000  // Event manager class
#define EVM0AVL                                0x1d000000  // Available
#define EVM0ID                                 0x1d000001  // Last event occured
#define EVM0ACK                                0x1d000002  // Event acknowledgement
#define EVM0CMD                                0x1d000003  // Command
#define EVM0NACK                               0x1d000004  // Number of events to acknowledge
#define EVM0REC                                0x1d000005  // Read event record
#define EVM0TRIG                               0x1d000006  // Generate event
#define EVN0                                   0x1d010000  // Event notification class
#define EVN0AVL                                0x1d010000  // Available
#define EVN0ID                                 0x1d010001  // Last event occured
#define EVN0ACK                                0x1d010002  // Event acknowledgement
#define EVN0REC                                0x1d010003  // Read event record
#define MCHRG0                                 0x1e000000  // Mobile Power Charger class
#define MCHRG0AVL                              0x1e000000  // Available
#define MCHRG0LINKED                           0x1e000001  // List of linked class instances
#define MCHRG0TYPE                             0x1e000002  // Type of charger
#define MCHRG0INST                             0x1e000003  // Charger instance
#define MCHRG0DCINST                           0x1e000004  // Corresponding DC instance
#define MCHRG0VOLT                             0x1e000005  // Charge control voltage
#define MCHRG0CURR                             0x1e000006  // Charge control current
#define MCHRG0OPER                             0x1e000007  // Operating state
#define MCHRG0CVOLT                            0x1e000008  // Charging voltage
#define MCHRG0CCURR                            0x1e000009  // Charging current
#define MCHRG0TEMP                             0x1e00000a  // Temperature
#define MCHRG0IVOLT                            0x1e00000b  // DC Input voltage
#define MCHRG0ICURR                            0x1e00000c  // DC Input current
#define MCHRG0PRECHG                           0x1e00000d  // Pre-charging status
#define MCHRG0FORCE                            0x1e00000e  // Force charge
#define MCHRG0MODE                             0x1e00000f  // Charger mode
#define MCHRG0MAXCURR                          0x1e000010  // Maximum charging current
#define MCHRG0POWUP                            0x1e000011  // Charge on startup
#define MCHRG0BVOLT                            0x1e000012  // Bulk-absorption voltage
#define MCHRG0FVOLT                            0x1e000013  // Float voltage
#define MCHRG0EQVOLT                           0x1e000014  // Equalization voltage
#define MCHRG0EQDUR                            0x1e000015  // Equalization duration
#define MCHRG0EQINT                            0x1e000016  // Equalization interval (days)
#define MCHRG0EQLAST                           0x1e000017  // Time since last equalization (days)
#define MCHRG0CRVOLT                           0x1e000018  // Charge return voltage
#define MCHRG0TFACT                            0x1e000019  // Temperature compensation factor (mV/^C)
#define MCHRG0ENA                              0x1e00001a  // Charger enable
#define MCHRG0THILIM                           0x1e00001b  // External temperature sensor high tempurature limit
#define MCHRG0TLOLIM                           0x1e00001c  // External temperature sensor low tempurature limit
#define MCHRG0ABSDUR                           0x1e00001d  // Absorption duration
#define MCHRG0PRIO                             0x1e00001e  // Charger priority
#define MCHRG0DERST                            0x1e00001f  // Derating status
#define MCHRG0DERREAS                          0x1e000020  // Derating reason
#define MCHRG0SLNT                             0x1e000021  // Silent mode
#define MCHRG0ALGO                             0x1e000022  // Charging algorithm
#define MCHRG0EXTTEMP                          0x1e000023  // External temperature
#define MCHRG0STATUS                           0x1e000024  // Charger fault/error status
#define MCHRG0FSTATUS                          0x1e000025  // Charge controller status
#define MCHRG0RVOLT                            0x1e000026  // Rated voltage
#define MCHRG0RCURR                            0x1e000027  // Rated current
#define MCHRGH0                                0x1e010000  // Mobile Power Charger History class
#define MCHRGH0AVL                             0x1e010000  // Available
#define MCHRGH0DCINST                          0x1e010001  // Corresponding DC instance
#define MCHRGH0TODAY                           0x1e010002  // Today's amp-hours to battery
#define MCHRGH0YESTERDAY                       0x1e010003  // Yesterday's amp-hours to battery
#define MCHRGH0YESTERDAY2                      0x1e010004  // Day before yesterday's amphours to battery
#define MCHRGH0DAYS7                           0x1e010005  // Last 7 days amp-hours to battery
#define MCHRGH0POWER                           0x1e010006  // Cumulative power usage
#define MCHRGH0OPERDAYS                        0x1e010007  // Total number of operating days
#define MCHRGH0CLRHIST                         0x1e010008  // Clear history
#define MCHRGH0LINKED                          0x1e010009  // List of linked class instances
#define MBAT0                                  0x1e020000  // Mobile Power Battery class
#define MBAT0AVL                               0x1e020000  // Available
#define MBAT0LINKED                            0x1e020001  // List of linked class instances
#define MBAT0DCINST                            0x1e020002  // Corresponding DC instance
#define MBAT0INST                              0x1e020003  // Battery instance
#define MBAT0TYPE                              0x1e020004  // Battery type
#define MBAT0BANK                              0x1e020005  // Bank size
#define MBAT0SERIES                            0x1e020006  // Series string
#define MBAT0MODCNT                            0x1e020007  // Module count
#define MBAT0CELLS                             0x1e020008  // Cells per module
#define MBAT0VOLT                              0x1e020009  // DC voltage
#define MBAT0CURR                              0x1e02000a  // DC current (negative-discharging,positive-charging)
#define MBAT0TEMP                              0x1e02000b  // Source temperature
#define MBAT0SOC                               0x1e02000c  // State of charge (SOC)
#define MBAT0TIME                              0x1e02000d  // Time remaining
#define MBAT0TIMESTS                           0x1e02000e  // Time remaining interpretation
#define MBAT0SOH                               0x1e02000f  // State of health
#define MBAT0CAPREM                            0x1e020010  // Capacity remaining
#define MBAT0CAPREL                            0x1e020011  // Relative capacity
#define MBAT0ACRIPPLE                          0x1e020012  // AC RMS ripple
#define MBAT0DCHGST                            0x1e020013  // Desired charge state
#define MBAT0DVOLT                             0x1e020014  // Desired DC voltage
#define MBAT0DCURR                             0x1e020015  // Desired DC current
#define MBAT0HPVOLT                            0x1e020016  // High precision DC voltage of the battery bank or battery.
#define MBAT0DV_DT                             0x1e020017  // DC Voltage Rate of Change (mV/s)
#define MBAT0DISCHGST                          0x1e020018  // Discharge On/Off status
#define MBAT0CHGST                             0x1e020019  // Charge On/Off status
#define MBAT0CHGDET                            0x1e02001a  // Charge detected
#define MBAT0RESST                             0x1e02001b  // Reserve status
#define MBAT0CAPACITY                          0x1e02001c  // Full capacity
#define MBAT0BAL                               0x1e02001d  // Balancing
#define MBAT0POLES                             0x1e02001e  // Poles status, poles connected to the cells
#define MBAT0HTR                               0x1e02001f  // Heater status
#define MBAT0DCDC                              0x1e020020  // Internal regulator status
#define MBAT0STATUS                            0x1e020021  // Battery fault/error status
#define MBAT0HMODE                             0x1e020022  // Heater mode
#define MBATHIST0                              0x1e030000  // Mobile Power Battery History class
#define MBATHIST0AVL                           0x1e030000  // Available
#define MBATHIST0INST                          0x1e030001  // Battery instance
#define MBATHIST0DCINST                        0x1e030002  // DC instance
#define MBATHIST0TODAYI                        0x1e030003  // Today's input Amp-Hours
#define MBATHIST0TODAYO                        0x1e030004  // Today's output Amp-Hours
#define MBATHIST0YESTERDAYI                    0x1e030005  // Yesterday's input Amp-Hours
#define MBATHIST0YESTERDAYO                    0x1e030006  // Yesterday's output Amp-Hours
#define MBATHIST0YESTERDAY2I                   0x1e030007  // Day before yesterday's input Amp-Hours
#define MBATHIST0YESTERDAY2O                   0x1e030008  // Day before yesterday's output Amp-Hours
#define MBATHIST0DAYS7I                        0x1e030009  // Last 7 days input Amp-Hours
#define MBATHIST0DAYS7O                        0x1e03000a  // Last 7 days output Amp-Hours
#define MBATHIST0DEEPDISCHG                    0x1e03000b  // The deepest discharge in Amp-Hours
#define MBATHIST0AVEDISCHG                     0x1e03000c  // Average discharge depth
#define MBATHIST0CYCLES                        0x1e03000d  // The number of charge cycles
#define MBATHIST0LOWVOLT                       0x1e03000e  // Lowest DC voltage
#define MBATHIST0HIGHVOLT                      0x1e03000f  // Highest DC voltage
#define MBATHIST0CLRHIST                       0x1e030010  // Clear history
#define MBATHIST0LINKED                        0x1e030011  // List of linked class instances
#define MBATCELL0                              0x1e040000  // Mobile Power Battery Cell class
#define MBATCELL0AVL                           0x1e040000  // Available
#define MBATCELL0LINKED                        0x1e040001  // List of linked class instances
#define MBATCELL0INST                          0x1e040002  // Cell instance
#define MBATCELL0BINST                         0x1e040003  // Battery instance
#define MBATCELL0VOLTSTS                       0x1e040004  // Cell voltage status
#define MBATCELL0TEMPSTS                       0x1e040005  // Cell temperature status
#define MBATCELL0BAL                           0x1e040006  // Balancing
#define MBATCELL0VOLT                          0x1e040007  // Cell voltage
#define MBATCELL0TEMP                          0x1e040008  // Cell temperature
#define MSOLAR0                                0x1e050000  // Mobile Power Solar Controller class
#define MSOLAR0AVL                             0x1e050000  // Available
#define MSOLAR0LINKED                          0x1e050001  // List of linked class instances
#define MSOLAR0DCINST                          0x1e050002  // DC instance
#define MSOLAR0INST                            0x1e050003  // Solar controller instance
#define MSOLAR0VOLT                            0x1e050004  // Measured voltage on solar array input
#define MSOLAR0CURR                            0x1e050005  // Measured current coming in from the solar array
#define MSOLAR0HBCO                            0x1e050006  // High Battery Cut Off (HBCO) Voltage
#define MSOLAR0HBCI                            0x1e050007  // High Battery Cut In (HBCI) Voltage
#define MSOLAR0LBCO                            0x1e050008  // Low Battery Cut Out (LBCO) Voltage
#define MSOLAR0LBCI                            0x1e050009  // Low Battery Cut In (LBCI) Voltage
#define MSOLAR0LBCOD                           0x1e05000a  // Low Battery Cut Out (LBCO) Delay
#define MSOLAR0PPWR                            0x1e05000b  // Panel power
#define MSOLAR0RPWR                            0x1e05000c  // Rated panel power
#define MSOLAR0RVOLT                           0x1e05000d  // Rated panel voltage
#define MACIN0                                 0x1e060000  // Mobile Power AC Input class
#define MACIN0AVL                              0x1e060000  // Available
#define MACIN0LINKED                           0x1e060001  // List of linked class instances
#define MACIN0DCINST                           0x1e060002  // DC instance
#define MACIN0RMSVOLT                          0x1e060003  // RMS voltage
#define MACIN0RMSCURR                          0x1e060004  // RMS current
#define MACIN0PEAKVOLT                         0x1e060005  // Peak voltage
#define MACIN0PEAKCURR                         0x1e060006  // Peak current
#define MACIN0FREQ                             0x1e060007  // Frequency (Hz)
#define MACIN0BYPASS                           0x1e060008  // Bypass mode
#define MACIN0QUALST                           0x1e060009  // AC input qualification status
#define MACIN0LOWFREQLIM                       0x1e06000a  // Low frequency limit (Hz)
#define MACIN0HIGHFREQLIM                      0x1e06000b  // High frequency limit (Hz)
#define MACIN0VOLTST                           0x1e06000c  // Voltage fault status
#define MACIN0FREQST                           0x1e06000d  // Frequency fault status
#define MACIN0GROUND                           0x1e06000e  // Open ground fault 
#define MACIN0NEUTRAL                          0x1e06000f  // Neutral fault
#define MACIN0REVPOL                           0x1e060010  // Reverse polarity detected
#define MACIN0MAXCURR                          0x1e060011  // Shore breaker size
#define MACIN0POWER                            0x1e060012  // AC input available power
#define MACIN0LVOLT                            0x1e060013  // Low Voltage Level
#define MINVERT0                               0x1e070000  // Mobile Power Inverter class
#define MINVERT0AVL                            0x1e070000  // Available
#define MINVERT0LINKED                         0x1e070001  // List of linked class instances
#define MINVERT0INST                           0x1e070002  // Inverter instance
#define MINVERT0DCINST                         0x1e070003  // DC instance
#define MINVERT0VOLT                           0x1e070004  // DC input voltage
#define MINVERT0CURR                           0x1e070005  // DC input current
#define MINVERT0ENABLE                         0x1e070006  // Inverter enable
#define MINVERT0PASSTHROUGH                    0x1e070007  // Pass-through enable
#define MINVERT0LOADSENSE                      0x1e070008  // Load sense enable
#define MINVERT0ENASTARTUP                     0x1e070009  // Inverter enable on startup
#define MINVERT0LSENSESTARTUP                  0x1e07000a  // Load sense enable on startup
#define MINVERT0PASSTHRSTARTUP                 0x1e07000b  // Pass-through enable on startup
#define MINVERT0OUTVOLT                        0x1e07000c  // Set output AC voltage
#define MINVERT0OUTFREQ                        0x1e07000d  // Set output AC frequency (Hz)
#define MINVERT0LSENSEINT                      0x1e07000e  // Load sense internval
#define MINVERT0LSENSEPOW                      0x1e07000f  // Load sense power threshold
#define MINVERT0STACK                          0x1e070010  // Stack mode
#define MINVERT0POWLIM                         0x1e070011  // Output power limit
#define MINVERT0FET1TEMP                       0x1e070012  // FET1 Temperature
#define MINVERT0FET2TEMP                       0x1e070013  // FET2 Temperature
#define MINVERT0TRANSFTEMP                     0x1e070014  // Transformer Temperature
#define MINVERT0CAPTEMP                        0x1e070015  // Capacitor Temperature
#define MINVERT0AMBTEMP                        0x1e070016  // Ambient Temperature
#define MINVERT0LOWACVOLT                      0x1e070017  // Lowest AC input voltage
#define MINVERT0HIGHACVOLT                     0x1e070018  // Highest AC input voltage
#define MINVERT0NUMACOVERLOAD                  0x1e070019  // Number of inverter AC output over-loads
#define MINVERT0NUMLOADSENSE                   0x1e07001a  // Number of times load sense has been enganged
#define MINVERT0SHDOWNVOLTMIN                  0x1e07001b  // DC source shutdown voltage – Minimum
#define MINVERT0SHDOWNVOLTMAX                  0x1e07001c  // DC source shutdown voltage – Maximum
#define MINVERT0WARNVOLTMIN                    0x1e07001d  // DC source warning voltage – Minimum
#define MINVERT0WARNVOLTMAX                    0x1e07001e  // DC source warning voltage – Maximum
#define MINVERT0RECVOLT                        0x1e07001f  // DC Source shutdown - Recovery Level
#define MINVERT0WRKLD                          0x1e070020  // Workload
#define MINVERT0STATUS                         0x1e070021  // Inverter faults/error status
#define MINVERT0PCBTEMP                        0x1e070022  // PCB temperature
#define MINVERT0MAXCURR                        0x1e070023  // Maximum AC current
#define MACOUT0                                0x1e080000  // Mobile Power AC Output class
#define MACOUT0AVL                             0x1e080000  // Available
#define MACOUT0LINKED                          0x1e080001  // List of linked class instances
#define MACOUT0DCINST                          0x1e080002  // DC instance
#define MACOUT0RMSVOLT                         0x1e080003  // Measured RMS voltage
#define MACOUT0RMSCURR                         0x1e080004  // Measured RMS current
#define MACOUT0PEAKVOLT                        0x1e080005  // Measured Peak voltage
#define MACOUT0PEAKCURR                        0x1e080006  // Measured Peak current
#define MACOUT0FREQ                            0x1e080007  // Measured Frequency (Hz)
#define MACOUT0PHASE                           0x1e080008  // Phase status
#define MACOUT0WAVE                            0x1e080009  // Waveform
#define MACOUT0POWER                           0x1e08000a  // AC output available power
#define MACOUT0LVOLT                           0x1e08000b  // Low Voltage Level
#define MPS0                                   0x1e090000  // Mobile Power BLE device class
#define MPS0AVL                                0x1e090000  // Available
#define MPS0PLIST                              0x1e090001  // Product list
#define MPS0RSTPWD                             0x1e090002  // Reset password
#define MPS0SOC                                0x1e090003  // System State of Chage (SOC)
#define MSHUNT0                                0x1e0a0000  // Mobile Power Shunt class
#define MSHUNT0AVL                             0x1e0a0000  // Available
#define MSHUNT0LINKED                          0x1e0a0001  // List of linked class instances
#define MSHUNT0TYPE                            0x1e0a0002  // Shunt monitoring device type
#define MSHUNT0RVOLT                           0x1e0a0003  // Rated voltage
#define MSHUNT0RCURR                           0x1e0a0004  // Rated current
#define MSHUNT0DISCHGVOLT                      0x1e0a0005  // Discharge voltage
#define MSHUNT0DISCHGFLOOR                     0x1e0a0006  // Discharge floor
#define MSHUNT0HVOLTLIM                        0x1e0a0007  // High voltage limit
#define MSHUNT0LTTEMPLIM                       0x1e0a0008  // Low temperature limit
#define MSHUNT0HTEMPLIM                        0x1e0a0009  // High temperature limit
#define MSHUNT0HCURRLIM                        0x1e0a000a  // High current limit
#define MSHUNT0SYSVOLT                         0x1e0a000b  // System voltage
#define MSHUNT0CALIBZERO                       0x1e0a000c  // Calibrate zero voltage
#define MSHUNT0EXTREL                          0x1e0a000d  // External relay
#define MSHUNT0STATUS                          0x1e0a000e  // Shunt fault/error status
#define MSHUNT0SETCAP                          0x1e0a000f  // Set capacity to 100%
#define MDCPROFILE0                            0x1e0b0000  // Mobile Power DC Profile class
#define MDCPROFILE0AVL                         0x1e0b0000  // Available
#define MDCPROFILE0LINKED                      0x1e0b0001  // List of linked class instances
#define MDCPROFILE0TYPE                        0x1e0b0002  // Battery type
#define MDCPROFILE0ALGO                        0x1e0b0003  // Charging algorithm
#define MDCPROFILE0BANK                        0x1e0b0004  // Bank size
#define MDCPROFILE0CHGVOLT                     0x1e0b0005  // Charged voltage
#define MDCPROFILE0DISCHGVOLT                  0x1e0b0006  // Discharged voltage
#define MDCPROFILE0CHGEFF                      0x1e0b0007  // Charge efficiency
#define MDCPROFILE0PEUKCOEFF                   0x1e0b0008  // Peukert coefficient
#define MDCPROFILE0TEMPCOEFF                   0x1e0b0009  // Temperature coefficient
#define MDCPROFILE0TAILCURR                    0x1e0b000a  // Tail current
#define MDCPROFILE0MAXCURR                     0x1e0b000b  // Current limitation
#define MDCPROFILE0HVDISCONN                   0x1e0b000c  // High voltage disconnect
#define MDCPROFILE0UVOLTWARN                   0x1e0b000d  // Under voltage warning
#define MDCPROFILE0EQVOLT                      0x1e0b000e  // Equalization voltage
#define MDCPROFILE0EQDUR                       0x1e0b000f  // Equalization duration
#define MDCPROFILE0EQINT                       0x1e0b0010  // Equalization interval
#define MDCPROFILE0BVOLT                       0x1e0b0011  // Bulk-absorption voltage
#define MDCPROFILE0ABSDUR                      0x1e0b0012  // Absorption duration
#define MDCPROFILE0FVOLT                       0x1e0b0013  // Float voltage
#define MDCPROFILE0CRVOLT                      0x1e0b0014  // Recharge voltage
#define MDCPROFILE0TFACT                       0x1e0b0015  // Temperature compensation factor (mV/^C)
#define MDCPROFILE0LVDISCONN                   0x1e0b0016  // Low voltage disconnect
#define MDCPROFILE0HVRETURN                    0x1e0b0017  // Battery high voltage limit return voltage
#define MDCPROFILE0LVRETURN                    0x1e0b0018  // Battery low voltage limit return voltage
#define MDCPROFILE0LVDELAY                     0x1e0b0019  // Battery low voltage limit time delay
#define MDCPROFILE0DCVOLT                      0x1e0b001a  // DC voltage
#define MDCPROFILE0DCCURR                      0x1e0b001b  // DC current
#define MDCPROFILE0TEMP                        0x1e0b001c  // Temperature
#define SYSAPPL0                               0x1f000000  // System Application Features class
#define SYSAPPL0AVL                            0x1f000000  // Available
#define SYSAPPL0SMARTECO                       0x1f000001  // SmartECO feature
#define GROUP0                                 0x1f010000  // Group/Feature class
#define GROUP0AVL                              0x1f010000  // Available
#define GROUP0UID                              0x1f010001  // Unique Identification Number
#define GROUP0ID                               0x1f010002  // Identification number per type
#define GROUP0NAME                             0x1f010003  // Name
#define GROUP0TYPE                             0x1f010004  // Type of feature this group implements
#define GROUP0RULES                            0x1f010005  // Rule engine class instances implementing this feature
#define GROUP0INTERFACE                        0x1f010006  // Product classes/group classes and/or other class instances representing this feature
#define GROUP0ENABLE                           0x1f010007  // Enabling feature
#define GROUP0ACTIVE                           0x1f010008  // Control feature
#define GROUP0ADDGROUP                         0x1f010009  // Request to create new class instance of same type
#define GROUP0DELGROUP                         0x1f01000a  // Delete the group instance
#define MULTIZONE0                             0x1f020000  // Multizone class
#define MULTIZONE0AVL                          0x1f020000  // Available
#define MULTIZONE0SELZONE                      0x1f020001  // Active selected zone
#define MULTIZONE0ZONES                        0x1f020002  // Available zones
#define MULTIZONE0NUMZONES                     0x1f020003  // Number of available zones
#define MULTIZONE0ITEMP                        0x1f020004  // Internal zone temperature (ambient)
#define MULTIZONE0ETEMP                        0x1f020005  // External zone temperature (outside)
#define MULTIZONE0INTERFACE                    0x1f020006  // Class instances (virtual) representing the zone
#define GROUPMGR0                              0x1f030000  // Group Management class
#define GROUPMGR0AVL                           0x1f030000  // Available
#define GROUPMGR0REQTYPE                       0x1f030001  // Query if specific group type is available/enabled in the system

#define GW0_BE                                 0x00000000  // Gateway class
#define GW0AVL_BE                              0x00000000  // Available
#define GW0INV_BE                              0x01000000  // Inventory
#define GW0VER_BE                              0x02000000  // Gateway FW version
#define GW0OTA_BE                              0x03000000  // Over the air update
#define GW0UPT_BE                              0x04000000  // Uptime
#define GW0TST_BE                              0x05000000  // Test
#define GW0AWSSC_BE                            0x06000000  // AWS Server Certificate PEM
#define GW0AWSCC_BE                            0x07000000  // AWS Client Certificate PEM
#define GW0AWSCCPK_BE                          0x08000000  // AWS Private key PEM
#define GW0AWSCA_BE                            0x09000000  // AWS CA Certificate PEM
#define GW0OTASC_BE                            0x0a000000  // OTA Server Certificate PEM
#define GW0OTACC_BE                            0x0b000000  // OTA Client Certificate PEM
#define GW0OTACCPK_BE                          0x0c000000  // OTA Private key PEM
#define GW0OTACA_BE                            0x0d000000  // OTA CA Certificate PEM
#define GW0DSN_BE                              0x0e000000  // Dometic Serial Number
#define GW0SKU_BE                              0x0f000000  // Stock Keeping Unit
#define GW0PNC_BE                              0x10000000  // Product Numeric Code
#define GW0MAC_BE                              0x11000000  // MAC address
#define GW0THING_BE                            0x12000000  // Thing ID
#define GW0PTYPE_BE                            0x13000000  // Product Type
#define GW0CUPDT_BE                            0x14000000  // Time between cloud updates
#define GW0RRSN_BE                             0x15000000  // ESP32 reset reason
#define GW0UNUSED_BE                           0x16000000  // UNUSED
#define GW0BATWIN_BE                           0x17000000  // Voltage window
#define GW0TEMPWIN_BE                          0x18000000  // Temperature window
#define GW0REMTWIN_BE                          0x19000000  // Time window
#define GW0CUPD_BE                             0x1a000000  // Command Utility Parameter Debug
#define GW0THTYID_BE                           0x1b000000  // Thing type ID
#define GW0DEV1ST_BE                           0x1c000000  // GW device 1 connected status
#define GW0ICT_BE                              0x1d000000  // Internet Connection Test
#define GW0OCNT_BE                             0x1e000000  // OTA counter
#define GW0RCNT_BE                             0x1f000000  // Restart counter
#define GW0ERRMSG_BE                           0x20000000  // Last error message (app/gw)
#define GW0NAME_BE                             0x21000000  // Display Name
#define GW0CONNURL_BE                          0x22000000  // GW device connectivity URL
#define CFG0_BE                                0x00000100  // Configuration class
#define CFG0AVL_BE                             0x00000100  // Available
#define CFG0DDEST_BE                           0x01000100  // Debug server destination
#define CFG0DPORT_BE                           0x02000100  // Debug server destination port
#define CFG0ITEMPTH_BE                         0x03000100  // Notification Threshold Inside Temperature
#define CFG0WWTRTH_BE                          0x04000100  // Notification Threshold Waste water
#define CFG0FWTRTH_BE                          0x05000100  // Notification Threshold Fresh water
#define CFG0BATTH_BE                           0x06000100  // Notification Threshold Battery
#define CFG0FWID_BE                            0x07000100  // FW version id
#define CFG0OPATH_BE                           0x08000100  // OTA URL path
#define CFG0OSTAT_BE                           0x09000100  // OTA update status
#define CFG0NTWTH_BE                           0x0a000100  // Network Thing type Id
#define CFG0LOGLVL_BE                          0x0b000100  // Log level
#define CHGRSVC0_BE                            0x00000001  // Charging Service class
#define CHGRSVC0AVL_BE                         0x00000001  // Available
#define CHGRSVC0CHRGV_BE                       0x01000001  // Charging voltage
#define CHGRSVC0BTRYV_BE                       0x02000001  // Battery voltage
#define CHGRSVC0REMT_BE                        0x03000001  // Hours remaining
#define IBS0_BE                                0x00000101  // Battery Service class
#define IBS0AVL_BE                             0x00000101  // Available
#define IBS0I_BE                               0x01000101  // Charging current
#define IBS0V_BE                               0x02000101  // Charging voltage
#define IBS0SOC_BE                             0x03000101  // State of charge
#define IBS0TEMP_BE                            0x04000101  // Battery temperature
#define IBS0RCAL_BE                            0x05000101  // Recalibrated state
#define IBS0OCGV_BE                            0x06000101  // Optimal charging voltage
#define IBS0BATTYP_BE                          0x07000101  // Battery type
#define IBS0SOH_BE                             0x08000101  // State of Health
#define IBS0CNOM_BE                            0x09000101  // Nominal Capacity (setting)
#define AC0_BE                                 0x00000201  // Air Condition class
#define AC0AVL_BE                              0x00000201  // Available
#define AC0ON_BE                               0x01000201  // Power on or off
#define AC0FSPD_BE                             0x02000201  // Fan speed
#define AC0MD_BE                               0x03000201  // Mode
#define AC0TTEMP_BE                            0x04000201  // Target temperature
#define AC0LGT_BE                              0x05000201  // Light
#define AC0DMR_BE                              0x06000201  // Dimmer
#define AC0PWR_BE                              0x07000201  // Power (limit)
#define AC0MMD_BE                              0x08000201  // Manual Mode
#define AC0MDL_BE                              0x09000201  // Model
#define AC0ITEMP_BE                            0x0a000201  // Internal temperature
#define AC0FS_BE                               0x0b000201  // Fan speed
#define AC0FMD_BE                              0x0c000201  // Fan mode
#define AC0LGTBS_BE                            0x0d000201  // Brightness
#define AC0ELGT_BE                             0x0e000201  // External Light
#define AC0PUR_BE                              0x0f000201  // Purifier
#define AC0SYSU_BE                             0x10000201  // System units
#define AC0TSUP_BE                             0x11000201  // Timer supported
#define AC0TMD_BE                              0x12000201  // Timer mode
#define AC0TONA_BE                             0x13000201  // Timer on active
#define AC0TONH_BE                             0x14000201  // Timer on hour
#define AC0TONM_BE                             0x15000201  // Timer on minute
#define AC0TOFFA_BE                            0x16000201  // Timer off active
#define AC0TOFFH_BE                            0x17000201  // Timer off hour
#define AC0TOFFM_BE                            0x18000201  // Timer off minute
#define AC0ERRC_BE                             0x19000201  // Error code
#define AC0STATUS_BE                           0x1a000201  // Error and Alert codes
#define AC0SLEEP_BE                            0x1b000201  // Sleep mode
#define AC0HFAVL_BE                            0x1c000201  // Heating function
#define AC0LFAVL_BE                            0x1d000201  // Light function
#define AC0ACTEXT_BE                           0x1e000201  // Actuators(external)
#define AC0REMCTRL_BE                          0x1f000201  // Remote control disable
#define AC0TEST_BE                             0x20000201  // Special test mode
#define AC0VER_BE                              0x21000201  // Firmware version
#define AC0OFFSET_BE                           0x22000201  // Internal offset temperature value for target temperature
#define AC0OPERST_BE                           0x23000201  // Operational state in auto mode
#define AC0ECO_BE                              0x24000201  // ECO mode
#define AC0FLAPS_BE                            0x25000201  // Flaps status
#define AC0HTTEMP_BE                           0x26000201  // Target temperature (Heat modes)
#define AC0ETEMP_BE                            0x27000201  // External temperature
#define AC0HMODE_BE                            0x28000201  // Heat modes
#define AC0FTDIFF_BE                           0x29000201  // Furnace mode temp diff
#define AC0FEATURE_BE                          0x2a000201  // Features available(bitmask)
#define AC0FILTER_BE                           0x2b000201  // Filter Indication
#define AC0CURR_BE                             0x2c000201  // Current (A)
#define AC0CURRLIM_BE                          0x2d000201  // Limit current (A) setting
#define SYS0_BE                                0x00000301  // System class
#define SYS0AVL_BE                             0x00000301  // Available
#define SYS0LINE_BE                            0x01000301  // Line
#define SYS0MSW_BE                             0x02000301  // Main Switch
#define SYS0NGNR_BE                            0x03000301  // Engine running
#define SYS0ITEMP_BE                           0x04000301  // Inside temperature
#define SYS0OTEMP_BE                           0x05000301  // Outside temperature
#define SYS0HBTRY_BE                           0x06000301  // House battery
#define SYS0SBTRY_BE                           0x07000301  // Starter battery
#define SYS0DATE_BE                            0x08000301  // Date
#define SYS0TIME_BE                            0x09000301  // Time
#define SYS0LOGO_BE                            0x0a000301  // logotype
#define SYS0FHTR_BE                            0x0b000301  // Floor heater
#define SYS0HTR_BE                             0x0c000301  // Heater
#define SYS0VER_BE                             0x0d000301  // Version
#define LGT0_BE                                0x00000401  // Light class
#define LGT0AVL_BE                             0x00000401  // Available
#define LGT0BLY_BE                             0x01000401  // Bed 2 left
#define LGT0BRY_BE                             0x02000401  // Bed 2 right
#define LGT0CEIL_BE                            0x03000401  // Ceiling
#define LGT0WALL_BE                            0x04000401  // Wall
#define LGT0BLX_BE                             0x05000401  // Bed 1 left
#define LGT0BRX_BE                             0x06000401  // Bed 1 right
#define LGT0WASH_BE                            0x07000401  // Wash
#define LGT0WC_BE                              0x08000401  // WC
#define LGT0AMBX_BE                            0x09000401  // ambient1
#define LGT0AMBY_BE                            0x0a000401  // ambient2
#define LGT0AMBZ_BE                            0x0b000401  // ambient3
#define LGT0ADDX_BE                            0x0c000401  // addition1
#define LGT0ADDY_BE                            0x0d000401  // addition2
#define LGT0ADDZ_BE                            0x0e000401  // addition3
#define LGT0KTNX_BE                            0x0f000401  // kitchen1
#define LGT0KTNY_BE                            0x10000401  // kitchen2
#define LGT0AWN_BE                             0x11000401  // awning
#define FWTR0_BE                               0x00000501  // Fresh water class
#define FWTR0AVL_BE                            0x00000501  // Available
#define FWTR0LVL_BE                            0x01000501  // Fresh water tank level
#define WWTR0_BE                               0x00000601  // Waste water class
#define WWTR0AVL_BE                            0x00000601  // Available
#define WWTR0LVL_BE                            0x01000601  // Waste water tank level
#define WWTRHTR0_BE                            0x00000701  // Waste water heater class
#define WWTRHTR0AVL_BE                         0x00000701  // Available
#define WWTRHTR0STS_BE                         0x01000701  // Waste water heater status
#define WWTRHTR0ACT_BE                         0x02000701  // Waste water heater active
#define CHGR0_BE                               0x00000801  // Charger class
#define CHGR0AVL_BE                            0x00000801  // Available
#define CHGR0ACT_BE                            0x01000801  // Charging active
#define CHGR0I_BE                              0x02000801  // Charging current
#define CHGR0SLN_BE                            0x03000801  // Silent mode
#define CHGR0OH_BE                             0x04000801  // Overheated
#define CHGR0ERR_BE                            0x05000801  // Error
#define HTR0_BE                                0x00000901  // Heater class
#define HTR0AVL_BE                             0x00000901  // Available
#define HTR0AON_BE                             0x01000901  // Air on/off
#define HTR0ATEMP_BE                           0x02000901  // Air temperature
#define HTR0WTRON_BE                           0x03000901  // Water on/off
#define HTR0WTRTEMP_BE                         0x04000901  // Water temperature
#define HTR0ESEL_BE                            0x05000901  // Energy selection
#define HTR0MMD_BE                             0x06000901  // Manual mode
#define HTR0MDL_BE                             0x07000901  // Model
#define HTR0TEMP_BE                            0x08000901  // Temperature value
#define HTR0EL_BE                              0x09000901  // El Off/1kW/2kW/3kW/0.5kW
#define HTR0GAS_BE                             0x0a000901  // Gas on/off
#define HTR0AMD_BE                             0x0b000901  // Air Heater Mode
#define HTR0SMAXFAN_BE                         0x0c000901  // Silent mode max fan speed
#define HTR0VMINFAN_BE                         0x0d000901  // Ventilation mode min fan speed
#define HTR0ERRST_BE                           0x0e000901  // Error status
#define HTR0ERRCD1_BE                          0x0f000901  // Active error code1
#define HTR0ERRCD2_BE                          0x10000901  // Active error code2
#define HTR0ERRCD3_BE                          0x11000901  // Active error code3
#define HTR0ERRCD4_BE                          0x12000901  // Active error code4
#define HTR0UVTH_BE                            0x13000901  // Under voltage threshold
#define HTR0SYSU_BE                            0x14000901  // System units
#define HTR0AHTOFFST_BE                        0x15000901  // Air heater timer off status
#define HTR0AHTOFFH_BE                         0x16000901  // Air heater timer off (hour)
#define HTR0AHTOFFM_BE                         0x17000901  // Air heater timer off (min)
#define HTR0AHTONST_BE                         0x18000901  // Air heater timer in status
#define HTR0AHTONH_BE                          0x19000901  // Air heater timer on (hour)
#define HTR0AHTONM_BE                          0x1a000901  // Air heater timer on (min)
#define HTR0WTRTST_BE                          0x1b000901  // Water heater timer status
#define HTR0WTRTONH_BE                         0x1c000901  // Water heater timer on (hour)
#define HTR0WTRTONM_BE                         0x1d000901  // Water heater timer on (min)
#define HTR0WTRTKET_BE                         0x1e000901  // Water heater timer keep on time (min)
#define HTR0ACST_BE                            0x1f000901  // AC status
#define HTR0DATEY_BE                           0x20000901  // Date: Year
#define HTR0DATEM_BE                           0x21000901  // Date: Month
#define HTR0DATED_BE                           0x22000901  // Date: Day
#define HTR0WEEKD_BE                           0x23000901  // Day of week
#define HTR0TIMEH_BE                           0x24000901  // Time: Hour
#define HTR0TIMEM_BE                           0x25000901  // Time: Minute
#define HTR0TIMES_BE                           0x26000901  // Time: Second
#define HTR0TTZ_BE                             0x27000901  // Timezone
#define HTR0ACWTRHST_BE                        0x28000901  // AC water heater status
#define HTR0GASWTRHST_BE                       0x29000901  // Gas water heater status
#define HTR0WTRTS_BE                           0x2a000901  // Water temperature status
#define HTR0RTS_BE                             0x2b000901  // Room temperature
#define HTR0CVER_BE                            0x2c000901  // Comfort MCU version
#define HTR0BVER_BE                            0x2d000901  // Burner MCU version
#define HTR0PCBA_BE                            0x2e000901  // PCBA version
#define HTR0PROT_BE                            0x2f000901  // Protocol version
#define FRG0_BE                                0x00000a01  // Fridge class
#define FRG0AVL_BE                             0x00000a01  // Available
#define FRG0MD_BE                              0x01000a01  // Mode
#define FRG0LVL_BE                             0x02000a01  // Level
#define FRG0FHTR_BE                            0x03000a01  // Frame heater?
#define FRG0DRST_BE                            0x04000a01  // Door status
#define FRG0ERRST_BE                           0x05000a01  // Error status
#define SATKAT0_BE                             0x00000b01  // Kathrein satellite class
#define SATKAT0AVL_BE                          0x00000b01  // Available
#define SATKAT0MDL_BE                          0x01000b01  // Model
#define SATKAT0STS_BE                          0x02000b01  // Status
#define SATKAT0CMD_BE                          0x03000b01  // Command
#define SATKAT0LAT_BE                          0x04000b01  // Latitude
#define SATKAT0LON_BE                          0x05000b01  // Longitude
#define SATKAT0OPOS_BE                         0x06000b01  // Orbital position
#define HD0_BE                                 0x00000c01  // Home Delivery class
#define HD0AVL_BE                              0x00000c01  // Available
#define HD0DSN_BE                              0x01000c01  // Dometic Serial Number
#define HD0SKU_BE                              0x02000c01  // Stock Keeping Unit
#define HD0PNC_BE                              0x03000c01  // Product Numeric Code
#define HD0FWVER_BE                            0x04000c01  // Firmware version
#define HD0ON_BE                               0x05000c01  // Power on
#define HD0SETTEMP_BE                          0x06000c01  // Temperature control
#define HD0OFS_BE                              0x07000c01  // Temperature offset
#define HD0ITEMP_BE                            0x08000c01  // Temperature inside
#define HD0OTEMP_BE                            0x09000c01  // Temperature outside
#define HD0MBDOOR_BE                           0x0a000c01  // Mailbox Door Open
#define HD0CBDOOR_BE                           0x0b000c01  // Cabinet Door Open
#define HD0MBLOCK_BE                           0x0c000c01  // Mailbox Lock Open
#define HD0CBLOCK_BE                           0x0d000c01  // Cabinet Lock Open
#define HD0ERR_BE                              0x0e000c01  // Error status
#define HD0ALRT_BE                             0x0f000c01  // Alert status
#define HD0AC_BE                               0x10000c01  // AC power connected
#define HD0BAT_BE                              0x11000c01  // Battery connected
#define HD0CHRG_BE                             0x12000c01  // Battery charging
#define HD0PTYPE_BE                            0x13000c01  // Product Type
#define HD0COMPSTAT_BE                         0x14000c01  // Compressor status
#define HD0HTRSTAT_BE                          0x15000c01  // Heater status
#define HD0TEMPCM_BE                           0x16000c01  // Temp ctrl mode
#define HD0TCMIN_BE                            0x17000c01  // Temp ctrl min limit
#define HD0TCMAX_BE                            0x18000c01  // Temp ctrl max limit
#define HD0MBLOCKS_BE                          0x19000c01  // Mailbox Lock Status
#define HD0CBLOCKS_BE                          0x1a000c01  // Cabinet Lock Status
#define HD0BATV_BE                             0x1b000c01  // Battery voltage
#define HD0BATPROT_BE                          0x1c000c01  // Battery protection
#define HD0SETTEMP2_BE                         0x1d000c01  // Temperature control
#define HD0ITEMP2_BE                           0x1e000c01  // Temperature inside
#define HD0TEMPCM2_BE                          0x1f000c01  // Temp ctrl mode
#define HD0STATUS_BE                           0x20000c01  // Error and Alert codes
#define HD0BATCRR_BE                           0x21000c01  // Current in mA
#define GB0_BE                                 0x00000d01  // Gas bottle class
#define GB0AVL_BE                              0x00000d01  // Available
#define GB0ETY_BE                              0x01000d01  // Empty status
#define MDM0_BE                                0x00000002  // Modem Network class
#define MDM0AVL_BE                             0x00000002  // Available
#define MDM0VER_BE                             0x01000002  // Version
#define MDM0RSSI_BE                            0x02000002  // RSSI
#define MDM0IMSI_BE                            0x03000002  // IMSI
#define MDM0IMEI_BE                            0x04000002  // IMEI
#define MDM0AT_BE                              0x05000002  // AT command string
#define WIFI0_BE                               0x00000102  // WiFi manager class
#define WIFI0AVL_BE                            0x00000102  // Available
#define WIFI0SCAN_BE                           0x01000102  // Scan
#define WIFI0ADD_BE                            0x02000102  // Add
#define WIFI0STS_BE                            0x03000102  // Status
#define WIFI0CNW_BE                            0x04000102  // Current network
#define WIFI0ON_BE                             0x05000102  // Radio on
#define WIFI0EV_BE                             0x06000102  // Event
#define WIFI0MD_BE                             0x07000102  // Mode
#define WFWL0_BE                               0x00000202  // WiFi Network White List class
#define WFWL0AVL_BE                            0x00000202  // Available
#define WFWL0SSID_BE                           0x01000202  // SSID
#define WFWL0PW_BE                             0x02000202  // Password
#define WFWL0DEL_BE                            0x03000202  // Delete
#define BT0_BE                                 0x00000302  // Bluetooth manager class
#define BT0AVL_BE                              0x00000302  // Available
#define BT0SCAN_BE                             0x01000302  // Scan
#define BT0ADDWL_BE                            0x02000302  // Add to whitelist
#define BT0PAIR_BE                             0x03000302  // Pair
#define BT0ADDGAP_BE                           0x04000302  // Add to whitelist (GAP only)
#define BT0ADVINTVL_BE                         0x05000302  // GAP Advertising interval
#define BT0CLIENTS_BE                          0x06000302  // Active BLE peripheral clients
#define BT0ON_BE                               0x07000302  // Turn BLE on/off
#define BTWL0_BE                               0x00000402  // Bluetooth whitelist class
#define BTWL0AVL_BE                            0x00000402  // Available
#define BTWL0ADDRTP_BE                         0x01000402  // Address type
#define BTWL0ADDR_BE                           0x02000402  // Address
#define BTWL0DEL_BE                            0x03000402  // Delete
#define BTWL0CNCT_BE                           0x04000402  // Connect
#define BTWL0NODEI_BE                          0x05000402  // BLE node instance
#define BTWL0SNS_BE                            0x06000402  // BLE Sensor types
#define BTWL0SNSI_BE                           0x07000402  // BLE sensor instances
#define BTWL0DCNCT_BE                          0x08000402  // Disconnect
#define BTWL0NODEC_BE                          0x09000402  // BLE node device DDM2 class
#define BTWL0RSSI_BE                           0x0a000402  // Device connection RSSI
#define BTWL0PWD_BE                            0x0b000402  // Password
#define SMS0_BE                                0x00000502  // SMS class
#define SMS0AVL_BE                             0x00000502  // Available
#define SMS0NUM_BE                             0x01000502  // Phone Number
#define SMS0MSG_BE                             0x02000502  // Text message to send
#define GNSS0_BE                               0x00000602  // GNSS class
#define GNSS0AVL_BE                            0x00000602  // Available
#define GNSS0LAT_BE                            0x01000602  // Latitude
#define GNSS0LON_BE                            0x02000602  // Longitude
#define GNSS0HDOP_BE                           0x03000602  // HDOP
#define GNSS0ALT_BE                            0x04000602  // Altitude
#define GNSS0FIX_BE                            0x05000602  // Fix
#define GNSS0COG_BE                            0x06000602  // COG
#define GNSS0SPKN_BE                           0x07000602  // spkn
#define GNSS0DATE_BE                           0x08000602  // Date
#define GNSS0NUMSAT_BE                         0x09000602  // Number of satellites
#define MQTT0_BE                               0x00000702  // MQTT class
#define MQTT0AVL_BE                            0x00000702  // Available
#define MQTT0STAT_BE                           0x01000702  // Status
#define MQTT0TXKB_BE                           0x02000702  // Tx kilobytes
#define MQTT0UPDATE_BE                         0x03000702  // Update cloud
#define MQTT0PROVTMPLT_BE                      0x04000702  // Provisioning template name
#define MQTT0CONNECT_BE                        0x05000702  // Connect is allowed
#define UDEV0_BE                               0x00000802  // UART device class
#define UDEV0AVL_BE                            0x00000802  // Available
#define UDEV0VER_BE                            0x01000802  // Version
#define UDEV0SNR_BE                            0x02000802  // Serial number
#define UDEV0BR_BE                             0x03000802  // Brand
#define SD0_BE                                 0x00000902  // Service discovery class
#define SD0AVL_BE                              0x00000902  // Available
#define SD0ACT_BE                              0x01000902  // Local service discovery is active
#define SD0LIST_BE                             0x02000902  // List of discovered devices
#define SD0WLIST_BE                            0x03000902  // Whitelist of required devices to connect to
#define SD0SSID_BE                             0x04000902  // SoftAP credentials used for local network
#define BTC0_BE                                0x00000a02  // Bluetooth clients class
#define BTC0AVL_BE                             0x00000a02  // Available
#define BTC0CONN_BE                            0x01000a02  // Connected
#define BTC0NAME_BE                            0x02000a02  // Device name
#define BTC0DEL_BE                             0x03000a02  // Delete
#define TLS0_BE                                0x00000b02  // TLS support class
#define TLS0AVL_BE                             0x00000b02  // Available
#define TLS0DEL_BE                             0x01000b02  // Delete the Signed Server certificate
#define TLS0CSR_BE                             0x02000b02  // Certificate Signing Request
#define TLS0CERT_BE                            0x03000b02  // Signed Server certificate
#define TLS0CACERT_BE                          0x04000b02  // CA certificate, public part
#define TLS0EXPIRY_BE                          0x05000b02  // Certificate expiration time UTC
#define CPL0_BE                                0x00000003  // Control Panel class
#define CPL0AVL_BE                             0x00000003  // Available
#define CPL0VER_BE                             0x01000003  // Version
#define MB0_BE                                 0x00000004  // Minibar class
#define MB0AVL_BE                              0x00000004  // Available
#define MB0DSN_BE                              0x01000004  // Dometic Serial Number
#define MB0SKU_BE                              0x02000004  // Stock Keeping Unit
#define MB0PNC_BE                              0x03000004  // Product Numeric Code
#define MB0VER_BE                              0x04000004  // Minibar FW version
#define MB0PWRON_BE                            0x05000004  // Power control
#define MB0LGTON_BE                            0x06000004  // Light control
#define MB0TCTRL_BE                            0x07000004  // Compartment Temperature Setting
#define MB0TSTAT_BE                            0x08000004  // Measured Compartment Temperature
#define MB0DOORI_BE                            0x09000004  // Door indication
#define MB0DOORST_BE                           0x0a000004  // Current door status
#define MB0SILMD_BE                            0x0b000004  // Silent mode
#define MB0TEMPAL_BE                           0x0c000004  // Temperature alarm
#define MB0DOORAL_BE                           0x0d000004  // Door alarm
#define MB0ERRST_BE                            0x0e000004  // Error status
#define RCS0_BE                                0x00000104  // Refridgerator control service class
#define RCS0AVL_BE                             0x00000104  // Available
#define RCS0TALRM_BE                           0x01000104  // Temperature alarm
#define RCS0TALT_BE                            0x02000104  // Temperature alarm time
#define RCS0DALRM_BE                           0x03000104  // Door alarm
#define RCS0DALT_BE                            0x04000104  // Door alarm time
#define RCS0THIOULIM_BE                        0x05000104  // Temp. high outer limit
#define RCS0THIINLIM_BE                        0x06000104  // Temp. high inner limit
#define RCS0TLOINLIM_BE                        0x07000104  // Temp. low inner limit
#define RCS0TLOOULIM_BE                        0x08000104  // Temp. low outer limit
#define RCS0TALT2_BE                           0x09000104  // Temperature alarm time 2
#define RCS0PWRFAIL_BE                         0x0a000104  // Power failure
#define RCS0CDFAIL_BE                          0x0b000104  // Cloud failure
#define RCS0THH_BE                             0x0c000104  // RCS temperature last hour history
#define RCS0TDH_BE                             0x0d000104  // RCS temperature last day history
#define RCS0TWH_BE                             0x0e000104  // RCS tempertature last week history
#define RCS0ROOM_BE                            0x0f000104  // Room number (as string)
#define NRX0_BE                                0x00000204  // NRX class
#define NRX0AVL_BE                             0x00000204  // Available
#define NRX0DSN_BE                             0x01000204  // Dometic Serial Number
#define NRX0SKU_BE                             0x02000204  // Stock Keeping Unit
#define NRX0PNC_BE                             0x03000204  // Product Numeric Code
#define NRX0VER_BE                             0x04000204  // Firmware Version
#define NRX0PWRON_BE                           0x05000204  // Power Control
#define NRX0LGTON_BE                           0x06000204  // Internal Light Status
#define NRX0MODE_BE                            0x07000204  // Mode Setting
#define NRX0LVL_BE                             0x08000204  // Temperature Level Setting
#define NRX0TEMP_BE                            0x09000204  // Measured Compartment Internal Temperature
#define NRX0DOORSTAT_BE                        0x0a000204  // Current Door Status
#define NRX0COMPSTAT_BE                        0x0b000204  // Compressor Running Status
#define NRX0FANSTAT_BE                         0x0c000204  // FAN Running Speed
#define NRX0IONSTAT_BE                         0x0d000204  // Ionizer Module Running Speed
#define NRX0LACSTAT_BE                         0x0e000204  // LAC Heater Running Status
#define NRX0ERRST_BE                           0x0f000204  // Error Status
#define TH0_BE                                 0x00000005  // Thermostat class
#define TH0AVL_BE                              0x00000005  // Available
#define TH0ITEMP_BE                            0x01000005  // Inside temperature
#define TH0BUT0_BE                             0x02000005  // Button
#define TH0BUT1_BE                             0x03000005  // Button
#define TH0BUT2_BE                             0x04000005  // Button
#define TH0WUP_BE                              0x05000005  // Wakeup
#define TH0FAV_BE                              0x06000005  // Favourite
#define TH0LTO_BE                              0x07000005  // Long timeout in seconds
#define TH0STO_BE                              0x08000005  // Short timeout in seconds
#define TH0VER1_BE                             0x09000005  // Version 1
#define TH0VER2_BE                             0x0a000005  // Version 2
#define IV0_BE                                 0x00000006  // Inventilate class
#define IV0AVL_BE                              0x00000006  // Available
#define IV0MODE_BE                             0x01000006  // Inventilate Mode
#define IV0PWRON_BE                            0x02000006  // Inventilate Power Mode
#define IV0FILST_BE                            0x03000006  // Inventilate Filter Status
#define IV0STORAGE_BE                          0x04000006  // Inventilate Storage Mode
#define IV0ERRST_BE                            0x05000006  // Inventilate Error status
#define IV0WARN_BE                             0x06000006  // Inventilate System Warning
#define IV0PWRSRC_BE                           0x07000006  // Inventilate Power Source
#define IV0AQST_BE                             0x08000006  // Air Quality Status
#define IV0PRST_BE                             0x09000006  // Pressure Status
#define IV0BLREQ_BE                            0x0a000006  // BLE Req
#define IV0IONST_BE                            0x0b000006  // Inventilate Ionizer Status
#define IV0HMITST_BE                           0x0c000006  // HMI Test
#define IV0SETT_BE                             0x0d000006  // Inventilate Configuration
#define IV0SETCHRGCRNT_BE                      0x0e000006  // Inventilate battery charging configuration
#define IV0STGT_BE                             0x0f000006  // Select storage time
#define IVPMGR0_BE                             0x00000106  // Inventilate Power Manager class
#define IVPMGR0AVL_BE                          0x00000106  // Available
#define IVPMGR0STATE_BE                        0x01000106  // Inventilate Power State
#define IVAQR0_BE                              0x00000206  // Inventilate IAQ Range class
#define IVAQR0AVL_BE                           0x00000206  // Available
#define IVAQR0MIN_BE                           0x01000206  // Min Range
#define IVAQR0MAX_BE                           0x02000206  // Max Range
#define IVEOL0_BE                              0x00000306  // Inventilate EOL class
#define IVEOL0AVL_BE                           0x00000306  // Available
#define IVEOL0REQ_BE                           0x01000306  // Inventilate EOL Request
#define IVEOL0RESP_BE                          0x02000306  // Inventilate EOL Response
#define CZ0_BE                                 0x00000007  // Thermostat RV-C (Climate zone) class
#define CZ0AVL_BE                              0x00000007  // Available
#define CZ0NAME_BE                             0x01000007  // name
#define CZ0OMD_BE                              0x02000007  // operating mode
#define CZ0FMODE_BE                            0x03000007  // fan mode
#define CZ0SMODE_BE                            0x04000007  // schedule mode
#define CZ0FSPD_BE                             0x05000007  // fan speed
#define CZ0HSET_BE                             0x06000007  // heat setpoint
#define CZ0CSET_BE                             0x07000007  // cool setpoint
#define CZ0CSCH_BE                             0x08000007  // current schedule
#define CZ0NSCH_BE                             0x09000007  // nbr of schedule instances
#define CZ0ITEMP_BE                            0x0a000007  // inside temp
#define CZ0AWAYIHS_BE                          0x0b000007  // away int heat setp
#define CZ0AWAYICS_BE                          0x0c000007  // away int cool setp
#define CZ0SLEEPTIMEH_BE                       0x0d000007  // sleep start hour
#define CZ0SLEEPTIMEM_BE                       0x0e000007  // sleep start min
#define CZ0SLEEPIHS_BE                         0x0f000007  // sleep heat setp
#define CZ0SLEEPICS_BE                         0x10000007  // sleep cool setp
#define CZ0AWAKETIMEH_BE                       0x11000007  // wake start hour
#define CZ0AWAKETIMEM_BE                       0x12000007  // wake start min
#define CZ0AWAKEIHS_BE                         0x13000007  // wake heat setp
#define CZ0AWAKEICS_BE                         0x14000007  // wake cool setp
#define CZ0AGS_BE                              0x15000007  // ags
#define CZ0ON_BE                               0x16000007  // power on
#define CZ0HMD_BE                              0x17000007  // Heating mode
#define CZ0MAXFAN_BE                           0x18000007  // Max fan speed (auto)
#define CZ0MINFAN_BE                           0x19000007  // Min fan speed (auto)
#define CZ0TH_BE                               0x1a000007  // target humidity
#define CZ0MOP_BE                              0x1b000007  // Max output protection
#define CZ0MOQM_BE                             0x1c000007  // Max output QM
#define CZ0RAH_BE                              0x1d000007  // room air humidity
#define CZ0ZBINST_BE                           0x1e000007  // Zone bus instance
#define CZ0ZDEL_BE                             0x1f000007  // Zone delete
#define CZ0FTDIFF_BE                           0x20000007  // Furnace Temp Diff
#define CZ0HPRIO_BE                            0x21000007  // Heat source Priority
#define CZ0FILIND_BE                           0x22000007  // Filter Indication
#define CZ0ECO_BE                              0x23000007  // ECO modifier
#define CZ0SLEEP_BE                            0x24000007  // Sleep modifier
#define RAC0_BE                                0x00000107  // RV-C AC class
#define RAC0AVL_BE                             0x00000107  // Available
#define RAC0CZ_BE                              0x01000107  // climate zone id
#define RAC0MS_BE                              0x02000107  // mode state
#define HP0_BE                                 0x00000207  // Heat Pump class
#define HP0AVL_BE                              0x00000207  // Available
#define HP0CZ_BE                               0x01000207  // climate zone id
#define HP0MS_BE                               0x02000207  // mode state
#define FU0_BE                                 0x00000307  // Furnace class
#define FU0AVL_BE                              0x00000307  // Available
#define FU0CZ_BE                               0x01000307  // climate zone id
#define FU0MS_BE                               0x02000307  // mode state
#define CZM0_BE                                0x00000407  // Climate Zone Manager class
#define CZM0AVL_BE                             0x00000407  // Available
#define CZM0ADD_BE                             0x01000407  // Add
#define CSM0_BE                                0x00000507  // Climate Schedule Manager class
#define CSM0AVL_BE                             0x00000507  // Available
#define CSM0ADD_BE                             0x01000507  // Add
#define CZS0_BE                                0x00000607  // Climate Zone Schedule class
#define CZS0AVL_BE                             0x00000607  // Available
#define CZS0SU_BE                              0x01000607  // Sunday
#define CZS0MO_BE                              0x02000607  // Monday
#define CZS0TU_BE                              0x03000607  // Tuesday
#define CZS0WE_BE                              0x04000607  // Wednesday
#define CZS0TH_BE                              0x05000607  // Thursday
#define CZS0FR_BE                              0x06000607  // Friday
#define CZS0SA_BE                              0x07000607  // Saturday
#define CZS0STAH_BE                            0x08000607  // Start hour
#define CZS0STAM_BE                            0x09000607  // Start minute
#define CZS0STOH_BE                            0x0a000607  // Stop hour
#define CZS0STOM_BE                            0x0b000607  // Stop min
#define CZS0OMD_BE                             0x0c000607  // operating mode
#define CZS0HSET_BE                            0x0d000607  // heat setpoint
#define CZS0CSET_BE                            0x0e000607  // cool setpoint
#define CZS0THUM_BE                            0x0f000607  // target humidity
#define CZS0SDEL_BE                            0x10000607  // Schedule delete
#define CZS0EN_BE                              0x11000607  // Scheduler enable
#define CC0_BE                                 0x00000707  // Climate Control class
#define CC0AVL_BE                              0x00000707  // Available
#define CC0NAME_BE                             0x01000707  // Name identifier
#define CC0ADD_BE                              0x02000707  // Add
#define CC0DEL_BE                              0x03000707  // Delete
#define CC0ACT_BE                              0x04000707  // Active
#define CC0DEVICES_BE                          0x05000707  // Linked devices
#define CC0SETTEMP_BE                          0x06000707  // Target temperature
#define CC0SETHUMID_BE                         0x07000707  // Target humidity
#define CC0STS_BE                              0x08000707  // Status of zone
#define CC0PCY_BE                              0x09000707  // Policy of zone
#define CC0TEMP_BE                             0x0a000707  // Current temperature
#define CC0VOC_BE                              0x0b000707  // Current air quality index
#define CC0HUMID_BE                            0x0c000707  // Current humidity
#define CC0DELTAP_BE                           0x0d000707  // Current delta pressure
#define CCS0_BE                                0x00000807  // Climate control schedule class
#define CCS0AVL_BE                             0x00000807  // Available
#define CCS0ADD_BE                             0x01000807  // Add a new schedule rule/action
#define CCS0DEL_BE                             0x02000807  // Delete a schedule
#define CCS0LIST_BE                            0x03000807  // List all scheduled rules/actions
#define CBS0_BE                                0x00000907  // Climate (zone device) basic schedule class
#define CBS0AVL_BE                             0x00000907  // Available
#define CBS0ENA_BE                             0x01000907  // Scheduling enabled
#define CBS0SMODE_BE                           0x02000907  // Schedule mode/type
#define CBS0SLPDAYS_BE                         0x03000907  // Enabled Weekdays for Sleep
#define CBS0SLPHOUR_BE                         0x04000907  // Start time (hours) for Sleep
#define CBS0SLPMIN_BE                          0x05000907  // Start time (min) for Sleep
#define CBS0WAKEDAYS_BE                        0x06000907  // Enabled Weekdays for Wake
#define CBS0WAKEHOUR_BE                        0x07000907  // Start time (hours) for Wake
#define CBS0WAKEMIN_BE                         0x08000907  // Start time (min) for Wake
#define CBS0AWAYDAYS_BE                        0x09000907  // Enabled Weekdays for Away
#define CBS0SLPCOOL_BE                         0x0a000907  // Cool temperature setpoint for Sleep
#define CBS0SLPHEAT_BE                         0x0b000907  // Heat temperature setpoint for Sleep
#define CBS0WAKECOOL_BE                        0x0c000907  // Cool temperature setpoint for Wake
#define CBS0WAKEHEAT_BE                        0x0d000907  // Heat temperature setpoint for Wake
#define CBS0AWAYCOOL_BE                        0x0e000907  // Cool temperature setpoint for Away
#define CBS0AWAYHEAT_BE                        0x0f000907  // Heat temperature setpoint for Away
#define RFAN0_BE                               0x00000008  // Roof fan class
#define RFAN0AVL_BE                            0x00000008  // Available
#define RFAN0SYST_BE                           0x01000008  // system status
#define RFAN0FM_BE                             0x02000008  // fan mode
#define RFAN0SPDMD_BE                          0x03000008  // speed mode
#define RFAN0LIGHT_BE                          0x04000008  // light
#define RFAN0FSPDSET_BE                        0x05000008  // fan speed setting
#define RFAN0WINDDRSW_BE                       0x06000008  // wind dir switch
#define RFAN0DOMEPOS_BE                        0x07000008  // dome position
#define RFAN0RAINSNS_BE                        0x08000008  // rain sensor override
#define RFAN0AMBTEMP_BE                        0x09000008  // ambient temperature
#define RFAN0SETPT_BE                          0x0a000008  // setpoint
#define RFAN0DMMODE_BE                         0x0b000008  // dome command/mode
#define RFAN0DSRDDMPOS_BE                      0x0c000008  // dome position/desired dome position
#define RFAN0SETPCTLDM_BE                      0x0d000008  // desired setpoint controlled dome
#define RFAN0DMCLSFOFF_BE                      0x0e000008  // auto dome close on fan off
#define RFAN0FOFFDMCLS_BE                      0x0f000008  // auto fan off on dome close
#define RFAN0FSPDINCDEC_BE                     0x10000008  // fan speed increment/decrement
#define RFAN0FSPDIDSTP_BE                      0x11000008  // fan speed increment/decrement step
#define RFAN0FSPDSTPSUP_BE                     0x12000008  // fan steps(speeds) supported
#define RFAN0RAINSNSSTS_BE                     0x13000008  // rain sensor
#define RFAN0MTNSNSEN_BE                       0x14000008  // motion sensor status
#define RFAN0MTNSNSTIM_BE                      0x15000008  // no motion detected time threshold
#define RFAN0LGTCMD_BE                         0x16000008  // light command
#define RFAN0LGTDD_BE                          0x17000008  // light duration/delay
#define RFAN0LGTLVL_BE                         0x18000008  // light level
#define RFAN0LGTRGB_BE                         0x19000008  // light RGB color
#define RFAN0LGTOC_BE                          0x1a000008  // light overcurrent
#define RFAN0LGTLSTAT_BE                       0x1b000008  // light load status
#define RFAN0CSCH_BE                           0x1c000008  // current schedule
#define RFAN0NSCH_BE                           0x1d000008  // number of schedules
#define RFAN0SLEEPTIMEH_BE                     0x1e000008  // sleep time schedule hour
#define RFAN0SLEEPTIMEM_BE                     0x1f000008  // sleep time schedule minute
#define RFAN0SLEEPFMODE_BE                     0x20000008  // sleep time fan mode
#define RFAN0SLEEPFDIR_BE                      0x21000008  // sleep time fan direction
#define RFAN0SLEEPFSETP_BE                     0x22000008  // sleep time fan speed/temperature setpoint
#define RFAN0AWAKETIMEH_BE                     0x23000008  // awake time schedule hour
#define RFAN0AWAKETIMEM_BE                     0x24000008  // awake time schedule minute
#define RFAN0AWAKEFMODE_BE                     0x25000008  // awake time fan mode
#define RFAN0AWAKEFDIR_BE                      0x26000008  // awake time fan direction
#define RFAN0AWAKEFSETP_BE                     0x27000008  // awake time fan speed/temperature setpoint
#define RFAN0AWAYTIMEH_BE                      0x28000008  // away time schedule hour
#define RFAN0AWAYTIMEM_BE                      0x29000008  // away time schedule minute
#define RFAN0AWAYFMODE_BE                      0x2a000008  // away time fan mode
#define RFAN0AWAYFDIR_BE                       0x2b000008  // away time fan direction
#define RFAN0AWAYFSETP_BE                      0x2c000008  // away time fan speed/temperature setpoint
#define RFAN0ERR_BE                            0x2d000008  // fan error
#define RFAN0ALARM_BE                          0x2e000008  // Alarm enable
#define RFAN0FEATURE_BE                        0x2f000008  // Available features
#define AW0_BE                                 0x00000108  // Awning class
#define AW0AVL_BE                              0x00000108  // Available
#define AW0MOTION_BE                           0x01000108  // motion
#define AW0POS_BE                              0x02000108  // position
#define AW0LOCKED_BE                           0x03000108  // statusing is ocktatus
#define AW0MOVABLE_BE                          0x04000108  // moveable
#define AW0USERL_BE                            0x05000108  // user lock status
#define AW0BRAKEL_BE                           0x06000108  // brake lock status
#define AW0PBRAKE_BE                           0x07000108  // parking brake status
#define AW0IG_BE                               0x08000108  // ignitioning key status
#define AW0LOWV_BE                             0x09000108  // low voltage status
#define AW0OC_BE                               0x0a000108  // overcurrent status
#define AW0ROC_BE                              0x0b000108  // retract overcurrent status
#define MTR0_BE                                0x00000208  // Motor data class
#define MTR0AVL_BE                             0x00000208  // Available
#define MTR0DEVID_BE                           0x01000208  // Device ID
#define MTR0SETSPD_BE                          0x02000208  // Current Speeed
#define MTR0MINSPD_BE                          0x03000208  // Minimum Speed
#define MTR0MAXSPD_BE                          0x04000208  // Maximum Speed
#define MTR0TACHO_BE                           0x05000208  // Tacho Feedback in RPM
#define MTR0DIR_BE                             0x06000208  // Direction of Motor
#define GNTR0_BE                               0x00000009  // Generator class
#define GNTR0AVL_BE                            0x00000009  // Available
#define GNTR0CHRGV_BE                          0x01000009  // charger voltage
#define GNTR0CHRGCRR_BE                        0x02000009  // charger current
#define GNTR0CHRGCRRPM_BE                      0x03000009  // charger percent of max
#define GNTR0OPST_BE                           0x04000009  // operating state
#define GNTR0CHRGAALG_BE                       0x05000009  // charging algorithm
#define GNTR0BTRSNSP_BE                        0x06000009  // sensor present
#define GNTR0LNKGMD_BE                         0x07000009  // linkage mode
#define GNTR0BTRTYP_BE                         0x08000009  // battery type
#define GNTR0BTRBKSZ_BE                        0x09000009  // battery bank size
#define GNTR0MAXCHRGCRR_BE                     0x0a000009  // max charging current
#define GNTR0DCOST_BE                          0x0b000009  // DC output status
#define GNTR0DFSTPWUP_BE                       0x0c000009  // DC default state power up
#define GNTR0TREM_BE                           0x0d000009  // time remaining
#define GNTR0PRECHRGST_BE                      0x0e000009  // pre-charging status
#define GNTR0EQVLTG_BE                         0x0f000009  // equalization status
#define GNTR0EQT_BE                            0x10000009  // equalization time
#define GNTR0ST_BE                             0x11000009  // status
#define GNTR0ENGRT_BE                          0x12000009  // engine runtime
#define GNTR0ENGL_BE                           0x13000009  // engine load
#define GNTR0STBTRV_BE                         0x14000009  // start battery voltage
#define GNTR0TMPSDSW_BE                        0x15000009  // temp shutdown switch
#define GNTR0OILPRESDSW_BE                     0x16000009  // oil pressure shutdown switch
#define GNTR0OILLVLSW_BE                       0x17000009  // oil level switch
#define GNTR0CAULT_BE                          0x18000009  // caution ligth
#define GNTR0ENGCTMP_BE                        0x19000009  // engine coolant temp
#define GNTR0ENGOILP_BE                        0x1a000009  // engine oil pressure
#define GNTR0ENGRPM_BE                         0x1b000009  // engine rpm
#define GNTR0FUELR_BE                          0x1c000009  // fuel rate
#define GNTR0CMD_BE                            0x1d000009  // command
#define GNTR0TYPE_BE                           0x1e000009  // type
#define GNTR0PRECT_BE                          0x1f000009  // pre-crank time
#define GNTR0MAXCT_BE                          0x20000009  // max crank time
#define GNTR0STOPT_BE                          0x21000009  // stop time
#define GNTR0DEMAND_BE                         0x22000009  // demand
#define GNTR0EXTACTR_BE                        0x23000009  // external activity
#define GNTR0QUIETT_BE                         0x24000009  // quiet time
#define GNTR0AUCHST_BE                         0x25000009  // auto charger status
#define GNTR0AUCHVTH_BE                        0x26000009  // auto charger voltage
#define GNTR0AUCHT_BE                          0x27000009  // auto charger time
#define GNTR0QTBH_BE                           0x28000009  // quite time begin hour
#define GNTR0QTBM_BE                           0x29000009  // quite time begin minutes
#define GNTR0QTEH_BE                           0x2a000009  // quite time end hour
#define GNTR0QTEM_BE                           0x2b000009  // quite time end minutes
#define GNTR0EXSTAT_BE                         0x2c000009  // exerciser status
#define GNTR0EXDW_BE                           0x2d000009  // exerciser days of week
#define GNTR0EXSH_BE                           0x2e000009  // exersiser start hour
#define GNTR0EXSM_BE                           0x2f000009  // exersiser start minutes
#define GNTR0EXRT_BE                           0x30000009  // exersiser run time
#define ACCH0_BE                               0x00000109  // AC Charger class
#define ACCH0AVL_BE                            0x00000109  // Available
#define ACCH0CHV_BE                            0x01000109  // charging voltage
#define ACCH0CHI_BE                            0x02000109  // charging current
#define ACCH0CIPMAX_BE                         0x03000109  // charge current percent of max
#define ACCH0OPST_BE                           0x04000109  // operating state
#define ACCH0DEST_BE                           0x05000109  // default state
#define ACCH0ARCEN_BE                          0x06000109  // auto recharge enable
#define ACCH0FRC_BE                            0x07000109  // force charge
#define ACCH0ACVRMS_BE                         0x08000109  // AC input current vrms
#define ACCH0RPWR_BE                           0x09000109  // real power
#define ITER0_BE                               0x00000209  // Inverter class
#define ITER0AVL_BE                            0x00000209  // Available
#define ITER0MD_BE                             0x01000209  // mode
#define ITER0IE_BE                             0x02000209  // inverter enabled
#define ITER0LE_BE                             0x03000209  // load sense enabled
#define ITER0PE_BE                             0x04000209  // pass-through enabled
#define ITER0ES_BE                             0x05000209  // inverter enabled on startup
#define ITER0VDC_BE                            0x06000209  // DC voltage
#define ITER0ADC_BE                            0x07000209  // DC current
#define ITER0AAC_BE                            0x08000209  // RMS AC current
#define ITER0RPWR_BE                           0x09000209  // RMS power?
#define DSRC0_BE                               0x00000309  // DC Source class
#define DSRC0AVL_BE                            0x00000309  // Available
#define DSRC0VDC_BE                            0x01000309  // VDC
#define DSRC0ADC_BE                            0x02000309  // ADC
#define DSRC0PRIO_BE                           0x03000309  // Priority
#define DSRC0HIS1_BE                           0x04000309  // DC Source voltage 1 day before
#define DSRC0HIS2_BE                           0x05000309  // DC Source voltage 2 days before
#define DSRC0HIS3_BE                           0x06000309  // DC Source voltage 3 days before
#define DSRC0HIS4_BE                           0x07000309  // DC Source voltage 4 days before
#define DSRC0HIS5_BE                           0x08000309  // DC Source voltage 5 days before
#define PMIC0_BE                               0x00000409  // PMIC class
#define PMIC0AVL_BE                            0x00000409  // Available
#define PMIC0BATRV_BE                          0x01000409  // Regulation voltage
#define PMIC0CHRGC_BE                          0x02000409  // Charge current
#define PMIC0TC_BE                             0x03000409  // Termination current
#define PMIC0PCHRGC_BE                         0x04000409  // Precharge current
#define PMIC0MSYSV_BE                          0x05000409  // Min system volt limit
#define PMIC0BSTRV_BE                          0x06000409  // Boost reg volt
#define PMIC0THRGTH_BE                         0x07000409  // Thermal regulation thres
#define PMIC0BSTAT_BE                          0x08000409  // Battery status
#define PMIC0CHTY_BE                           0x09000409  // Charge type
#define PMIC0MID_BE                            0x0a000409  // Manufacturer Id
#define PMIC0BMDL_BE                           0x0b000409  // Battery model
#define PMIC0ONLINE_BE                         0x0c000409  // Online
#define PMIC0BHLT_BE                           0x0d000409  // Battery health
#define TS0_BE                                 0x0000000a  // Tank class
#define TS0AVL_BE                              0x0000000a  // Available
#define TS0TYPE_BE                             0x0100000a  // Type
#define TS0RLVL_BE                             0x0200000a  // Relative level
#define TS0RES_BE                              0x0300000a  // Sensor resolution
#define TS0HIS1_BE                             0x0400000a  // Relative level 1
#define TS0HIS2_BE                             0x0500000a  // Relative level 2
#define TS0HIS3_BE                             0x0600000a  // Relative level 3
#define TS0HIS4_BE                             0x0700000a  // Relative level 4
#define TS0HIS5_BE                             0x0800000a  // Relative level 5
#define TS0TEMP_BE                             0x0900000a  // Tank temperature
#define WP0_BE                                 0x0000010a  // Water pump class
#define WP0AVL_BE                              0x0000010a  // Available
#define WP0OPRTST_BE                           0x0100010a  // operating status 
#define WP0PMPST_BE                            0x0200010a  // Pump status
#define WP0WTRHKDTCT_BE                        0x0300010a  // Water hookup detected
#define WP0CURSYSPRES_BE                       0x0400010a  // Current system pressure
#define WP0PPRESSET_BE                         0x0500010a  // Pump pressure setting
#define WP0RPRESSET_BE                         0x0600010a  // Regulator pressure setting
#define WP0OPRTCURR_BE                         0x0700010a  // Operating current
#define WP0OC_BE                               0x0800010a  // Overcurrent
#define WH0_BE                                 0x0000020a  // Water Heater class
#define WH0AVL_BE                              0x0000020a  // Available
#define WH0OPSTAT_BE                           0x0100020a  // Operating state
#define WH0HLVL_BE                             0x0200020a  // Heat level
#define DCL0_BE                                0x0000000b  // DC Load class
#define DCL0AVL_BE                             0x0000000b  // Available
#define DCL0CMD_BE                             0x0100000b  // Command
#define DCL0DD_BE                              0x0200000b  // Duration/delay
#define DCL0LVL_BE                             0x0300000b  // Level
#define DCL0OC_BE                              0x0400000b  // Overcurrent
#define DCL0TYPE_BE                            0x0500000b  // Type
#define DIM0_BE                                0x0000000c  // DC Dimmer class
#define DIM0AVL_BE                             0x0000000c  // Available
#define DIM0CMD_BE                             0x0100000c  // Command
#define DIM0DD_BE                              0x0200000c  // Duration/delay
#define DIM0LVL_BE                             0x0300000c  // Level
#define DIM0RGB_BE                             0x0400000c  // RGB color
#define DIM0OC_BE                              0x0500000c  // Overcurrent
#define DIM0LSTAT_BE                           0x0600000c  // Load status
#define DIM0NAME_BE                            0x0700000c  // Name
#define DIM0TYPE_BE                            0x0800000c  // Type / capabilities
#define SC0_BE                                 0x0000000d  // Scenes class
#define SC0AVL_BE                              0x0000000d  // Available
#define SC0NAME_BE                             0x0100000d  // Name
#define SC0STAT_BE                             0x0200000d  // Status
#define SCE0_BE                                0x0000010d  // Scene Element Definition class
#define SCE0AVL_BE                             0x0000010d  // Available
#define SCE0PARENT_BE                          0x0100010d  // Parent
#define SCE0PNAME_BE                           0x0200010d  // Parameter name
#define SCE0PAR_BE                             0x0300010d  // Parameter
#define AUT0_BE                                0x0000020d  // Automation class
#define AUT0AVL_BE                             0x0000020d  // Available
#define AUT0NAME_BE                            0x0100020d  // Name
#define AUT0STATE_BE                           0x0200020d  // State
#define AUT0STH_BE                             0x0300020d  // Start time hours
#define AUT0STM_BE                             0x0400020d  // Start time minutes
#define AUT0STD_BE                             0x0500020d  // Start time days of the week
#define AUT0IST_BE                             0x0600020d  // Instance
#define SNODE0_BE                              0x0000000e  // Sensor node class
#define SNODE0AVL_BE                           0x0000000e  // Available
#define SNODE0MFGR_BE                          0x0100000e  // Manufacturer
#define SNODE0TYPE_BE                          0x0200000e  // Product type
#define SNODE0MDL_BE                           0x0300000e  // Node type
#define SNODE0ID_BE                            0x0400000e  // Id
#define SNODE0NAME_BE                          0x0500000e  // Device name
#define SNODE0FR_BE                            0x0600000e  // Factory reset
#define SNODE0ADVINT_BE                        0x0700000e  // Advertising interval
#define SNODE0ADVINTC_BE                       0x0800000e  // Advertising interval connected
#define SNODE0CONNINT_BE                       0x0900000e  // Connection interval
#define SNODE0SLAVELAT_BE                      0x0a00000e  // Slave latency
#define SNODE0SHA1_BE                          0x0b00000e  // GAP SHA1 hash
#define SNODE0BLOB_BE                          0x0c00000e  // Configuration blob
#define SNODE0FUNC_BE                          0x0d00000e  // Function
#define SNODE0LOC_BE                           0x0e00000e  // Location
#define SNODE0DFU_BE                           0x0f00000e  // Enter OTA DFU
#define SNODE0NCINT_BE                         0x1000000e  // Nonconnectable connection interval
#define SNODE0CINT_BE                          0x1100000e  // Connectable connection interval
#define SNODE0GAPBLOB_BE                       0x1200000e  // GAP blob
#define SNODE0UADVINT_BE                       0x1300000e  // Update advertise interval
#define SNODE0LEDBLINK_BE                      0x1400000e  // LED blink
#define SNODE0WLINT_BE                         0x1500000e  // Connectable connection interval with WL
#define SNODE0PEERDEL_BE                       0x1600000e  // Delete peer
#define SNODE0SN_BE                            0x1700000e  // Serial number
#define SNODE0MGRDATE_BE                       0x1800000e  // Manufacturing date
#define SNODE0MFGRNAME_BE                      0x1900000e  // Manufacturer name
#define SNODE0PRODUCT_BE                       0x1a00000e  // Product
#define SNODE0SKU_BE                           0x1b00000e  // SKU
#define SNODE0VER_BE                           0x1c00000e  // Firmware version
#define SNODE0PCB_BE                           0x1d00000e  // PCB
#define SNODE0BOM_BE                           0x1e00000e  // BOM
#define SNODE0ITEMDESC_BE                      0x1f00000e  // Item description
#define SNODE0MDLNO_BE                         0x2000000e  // Model number
#define SNODE0MDLNAME_BE                       0x2100000e  // Model name
#define SNODE0EAN13_BE                         0x2200000e  // EAN13
#define SNODE0BATTLVL_BE                       0x2300000e  // Battery voltage
#define SNODE0EXTPWRSENSE_BE                   0x2400000e  // External power sense
#define SACCM0_BE                              0x0000010e  // Accelerometer class
#define SACCM0AVL_BE                           0x0000010e  // Available
#define SACCM0STATUS_BE                        0x0100010e  // Status
#define SACCM0FR_BE                            0x0200010e  // Factory reset
#define SACCM0ACCX_BE                          0x0300010e  // Acceleration x
#define SACCM0ACCY_BE                          0x0400010e  // Acceleration y
#define SACCM0ACCZ_BE                          0x0500010e  // Acceleration z
#define SACCM0LONANGLE_BE                      0x0600010e  // Longitudinal tilt angle
#define SACCM0LATANGLE_BE                      0x0700010e  // Lateral tilt angle
#define SACCM0TEMP_BE                          0x0800010e  // Temperature
#define SACCM0WAI_BE                           0x0900010e  // WhoAmI
#define SACCM0EVENT_BE                         0x0a00010e  // Acceleration event
#define SACCM0SENDACCX_BE                      0x0b00010e  // Send acceleration x 
#define SACCM0SENDACCY_BE                      0x0c00010e  // Send acceleration y
#define SACCM0SENDACCZ_BE                      0x0d00010e  // Send acceleration z
#define SACCM0SENDTILT_BE                      0x0e00010e  // Send tilt angle
#define SACCM0SENDTEMP_BE                      0x0f00010e  // Send temperature
#define SACCM0SAMPP_BE                         0x1000010e  // Sampling period
#define SACCM0CALFLATDONE_BE                   0x1100010e  // Calibration flat done
#define SACCM0CALFLATSTART_BE                  0x1200010e  // Calibration flat start
#define SACCM0CALTILTDONE_BE                   0x1300010e  // Calibration tilt done
#define SACCM0CALTILTSTART_BE                  0x1400010e  // Calibration tilt start
#define SACCM0CALTILTPOS_BE                    0x1500010e  // Calibration tilt position
#define SACCM0DATARATE_BE                      0x1600010e  // Set data rate
#define SACCM0POWERM_BE                        0x1700010e  // Set power mode
#define SACCM0BW_BE                            0x1800010e  // Set filter bandwidth
#define SACCM0FULLSCALE_BE                     0x1900010e  // Set full scale
#define SACCM0FILTPATH_BE                      0x1a00010e  // Set filter path 
#define SACCM0TILTT_BE                         0x1b00010e  // Set 6d threshold
#define SACCM0TAPX_BE                          0x1c00010e  // Set tap threshold x
#define SACCM0TAPY_BE                          0x1d00010e  // Set tap threshold y
#define SACCM0TAPZ_BE                          0x1e00010e  // Set tap threshold z
#define SACCM0TAPDIR_BE                        0x1f00010e  // Set tap recognition axis 
#define SACCM0TAPP_BE                          0x2000010e  // Set tap axis priority
#define SACCM0FALLTHRESH_BE                    0x2100010e  // Set freefall threshold
#define SACCM0FALLDUR_BE                       0x2200010e  // Set freefall duration
#define SACCM0WAKETHRESH_BE                    0x2300010e  // Set wakeup threshold
#define SACCM0WAKEDUR_BE                       0x2400010e  // Set wakeup duration
#define SACCM0DTAPDUR_BE                       0x2500010e  // Set double tap duration
#define SACCM0QUIETT_BE                        0x2600010e  // Set double tap quiet time
#define SACCM0SHOCKDUR_BE                      0x2700010e  // Set double tap shock duration
#define SACCM0TAPMODE_BE                       0x2800010e  // Set tap mode
#define SACCM0TAPRECOGX_BE                     0x2900010e  // Set tap recognition x
#define SACCM0TAPRECOGY_BE                     0x2a00010e  // Set tap recognition y
#define SACCM0TAPRECOGZ_BE                     0x2b00010e  // Set tap recognition z
#define SACCM0STAPEVENT_BE                     0x2c00010e  // Single tap enable
#define SACCM0DTAPEVENT_BE                     0x2d00010e  // Double tap enable
#define SACCM0FFEVENT_BE                       0x2e00010e  // Freefall enable
#define SACCM0TILTEVENT_BE                     0x2f00010e  // Tilt enable
#define SACCM0WUEVENT_BE                       0x3000010e  // Wakeup enable
#define SHALL0_BE                              0x0000020e  // HALL sensor class
#define SHALL0AVL_BE                           0x0000020e  // Available
#define SHALL0STATUS_BE                        0x0100020e  // Status
#define SHALL0FR_BE                            0x0200020e  // Factory reset
#define SHALL0OPEN_BE                          0x0300020e  // Open
#define SHALL0SENDOPEN_BE                      0x0400020e  // Send open
#define SBMEA0_BE                              0x0000030e  // BME280 class
#define SBMEA0AVL_BE                           0x0000030e  // Available
#define SBMEA0STATUS_BE                        0x0100030e  // Status
#define SBMEA0FR_BE                            0x0200030e  // Factory reset
#define SBMEA0TEMP_BE                          0x0300030e  // Temperature
#define SBMEA0HUM_BE                           0x0400030e  // Humidity
#define SBMEA0PRS_BE                           0x0500030e  // Pressure
#define SBMEA0SENDTEMP_BE                      0x0600030e  // Send temperature
#define SBMEA0SENDHUM_BE                       0x0700030e  // Send humidity
#define SBMEA0SENDPRS_BE                       0x0800030e  // Send pressure
#define SBMEA0OVERTEMP_BE                      0x0900030e  // Oversampling temperature
#define SBMEA0OVERHUM_BE                       0x0a00030e  // Oversampling humidity
#define SBMEA0OVERPRS_BE                       0x0b00030e  // Oversampling pressure
#define SBMEA0SAMPP_BE                         0x0c00030e  // Sampling period
#define SBMEB0_BE                              0x0000040e  // BME680 class
#define SBMEB0AVL_BE                           0x0000040e  // Available
#define SBMEB0STATUS_BE                        0x0100040e  // Status
#define SBMEB0FR_BE                            0x0200040e  // Factory reset
#define SBMEB0TEMP_BE                          0x0300040e  // Temperature
#define SBMEB0HUM_BE                           0x0400040e  // Humidity
#define SBMEB0PRS_BE                           0x0500040e  // Pressure
#define SBMEB0GAS_BE                           0x0600040e  // Gas resistance
#define SBMEB0AQR_BE                           0x0700040e  // Air quality value
#define SBMEB0IAQ_BE                           0x0800040e  // Air quality index
#define SBMEB0CO2_BE                           0x0900040e  // CO2 concentration
#define SBMEB0VOC_BE                           0x0a00040e  // VOC concentration
#define SBMEB0SST_BE                           0x0b00040e  // Stabilization status
#define SBMEB0RIS_BE                           0x0c00040e  // Run-in status
#define SBMEB0SENDTEMP_BE                      0x0d00040e  // Send temperature
#define SBMEB0SENDHUM_BE                       0x0e00040e  // Send humidity
#define SBMEB0SENDPRS_BE                       0x0f00040e  // Send pressure
#define SBMEB0SENDGAS_BE                       0x1000040e  // Send gas resistance
#define SBMEB0SENDAQR_BE                       0x1100040e  // Send air quality value
#define SBMEB0SENDIAQ_BE                       0x1200040e  // Send air quality index
#define SBMEB0SENDCO2_BE                       0x1300040e  // Send CO2
#define SBMEB0SENDVOC_BE                       0x1400040e  // Send VOC
#define SBMEB0SENDSST_BE                       0x1500040e  // Send stabilization status
#define SBMEB0SENDRIS_BE                       0x1600040e  // Send run-in status
#define SBMEB0SAMPRATE_BE                      0x1700040e  // Sample rate
#define SBMEB0SAMPRATEG_BE                     0x1800040e  // Sample rate for gas sensor
#define SPIR0_BE                               0x0000050e  // PIR sensor class
#define SPIR0AVL_BE                            0x0000050e  // Available
#define SPIR0STATUS_BE                         0x0100050e  // Status
#define SPIR0FR_BE                             0x0200050e  // Factory reset
#define SPIR0EVT_BE                            0x0300050e  // Event
#define SPIR0SENDEVT_BE                        0x0400050e  // Send event
#define SWB0_BE                                0x0000060e  // Wire break sensor class
#define SWB0AVL_BE                             0x0000060e  // Available
#define SWB0STATUS_BE                          0x0100060e  // Status
#define SWB0FR_BE                              0x0200060e  // Factory reset
#define SWB0SFUNC_BE                           0x0300060e  // Sub function
#define SWB0SLOC_BE                            0x0400060e  // Sub location
#define SWB0OPEN_BE                            0x0500060e  // Open
#define SWB0ADCOPEN_BE                         0x0600060e  // ADC open
#define SWB0SENDOPEN_BE                        0x0700060e  // Send open
#define SWB0SENDADC_BE                         0x0800060e  // Send ADC
#define SWB0SAMPP_BE                           0x0900060e  // Sampling period
#define SV0_BE                                 0x0000070e  // Voltage sensor class
#define SV0AVL_BE                              0x0000070e  // Available
#define SV0STATUS_BE                           0x0100070e  // Status
#define SV0FR_BE                               0x0200070e  // Factory reset
#define SV0SFUNC_BE                            0x0300070e  // Sub function
#define SV0SLOC_BE                             0x0400070e  // Sub location
#define SV0ADCV_BE                             0x0500070e  // ADC 12V
#define SV0ADCBATT_BE                          0x0600070e  // ADC batt
#define SV0ADCBLG_BE                           0x0700070e  // ADC bilge
#define SV0VOUTS_BE                            0x0800070e  // Vout sense
#define SV0PSTS_BE                             0x0900070e  // Power Status
#define SV0SENDADCV_BE                         0x0a00070e  // Send adc 12V
#define SV0SENDADCBATT_BE                      0x0b00070e  // Send ADC batt
#define SV0SENDADCBLG_BE                       0x0c00070e  // Send ADC bilge
#define SV0SENDVOUTS_BE                        0x0d00070e  // Send vout sense
#define SV0SAMPP_BE                            0x0e00070e  // Sampling period
#define SDP0_BE                                0x0000080e  // Differential pressure sensor class
#define SDP0AVL_BE                             0x0000080e  // Available
#define SDP0STATUS_BE                          0x0100080e  // Status
#define SDP0FR_BE                              0x0200080e  // Factory reset
#define SDP0DP_BE                              0x0300080e  // Differential pressure
#define SDP0TEMP_BE                            0x0400080e  // Temperature
#define SDP0PI_BE                              0x0500080e  // Product Identifier
#define SDP0SENDDP_BE                          0x0600080e  // Send differential pressure
#define SDP0SENDTEMP_BE                        0x0700080e  // Send temperature
#define SDP0SAMPP_BE                           0x0800080e  // Sampling period
#define SDP0MC_BE                              0x0900080e  // Measurement command
#define SHALLAN0_BE                            0x0000090e  // HALL analog sensor class
#define SHALLAN0AVL_BE                         0x0000090e  // Available
#define SHALLAN0STATUS_BE                      0x0100090e  // Status
#define SHALLAN0FR_BE                          0x0200090e  // Factory reset
#define SHALLAN0MFX_BE                         0x0300090e  // Value magnetic field X
#define SHALLAN0MFY_BE                         0x0400090e  // Value magnetic field Y
#define SHALLAN0MFZ_BE                         0x0500090e  // Value magnetic field Z
#define SHALLAN0EVT_BE                         0x0600090e  // Event
#define SHALLAN0SENDMFX_BE                     0x0700090e  // Send magnetic field X
#define SHALLAN0SENDMFY_BE                     0x0800090e  // Send magnetic field Y
#define SHALLAN0SENDMFZ_BE                     0x0900090e  // Send magnetic field Z
#define SHALLAN0SENDEVT_BE                     0x0a00090e  // Send event
#define SHALLAN0SAMPP_BE                       0x0b00090e  // Sampling period
#define TEST0_BE                               0x0000000f  // Test class
#define TEST0AVL_BE                            0x0000000f  // Available
#define TEST0R_BE                              0x0100000f  // Read
#define TEST0RW_BE                             0x0200000f  // Read/Write
#define DEMO0_BE                               0x0000010f  // Demo class
#define DEMO0AVL_BE                            0x0000010f  // Available
#define DEMO0SETTEMP_BE                        0x0100010f  // Set temperature
#define DEMO0MODE_BE                           0x0200010f  // Fan mode
#define DEMO0UPT_BE                            0x0300010f  // Uptime
#define SW0_BE                                 0x00000010  // Switch class
#define SW0AVL_BE                              0x00000010  // Available
#define SW0TYPE_BE                             0x01000010  // Switch type
#define SW0NAME_BE                             0x02000010  // Switch name
#define SW0STNAME_BE                           0x03000010  // Switch sequence state names
#define SW0ICON_BE                             0x04000010  // Switch icon
#define SW0MSTATE_BE                           0x05000010  // Switch main state
#define SW0SSTATE_BE                           0x06000010  // Switch sub state 
#define SW0MIND_BE                             0x07000010  // Switch main indicator
#define SW0SIND_BE                             0x08000010  // Switch sub indicator
#define SW0ZONE_BE                             0x09000010  // Switch zone
#define SW0ZDEL_BE                             0x0a000010  // Zone delete
#define SW0MPP_BE                              0x0b000010  // Switch drawn on main page
#define SW0MPXP_BE                             0x0c000010  // Switch main page X position
#define SW0MPYP_BE                             0x0d000010  // Switch main page Y position
#define SW0ID_BE                               0x0e000010  // Switch unique ID
#define SZM0_BE                                0x00000110  // Switch Zone Manager class
#define SZM0AVL_BE                             0x00000110  // Available
#define SZM0ADD_BE                             0x01000110  // Instance of zone added
#define SZ0_BE                                 0x00000210  // Switch Zone class
#define SZ0AVL_BE                              0x00000210  // Available
#define SZ0NAME_BE                             0x01000210  // Name
#define SZ0ON_BE                               0x02000210  // On/Off
#define SZ0ZDEL_BE                             0x03000210  // Zone delete
#define BTN0_BE                                0x00000310  // Switch Zone class
#define BTN0AVL_BE                             0x00000310  // Available
#define BTN0BTN0_BE                            0x01000310  // Inventilate Power Button
#define BTN0BTN1_BE                            0x02000310  // Inventilate Mode Button
#define BTN0BTN2_BE                            0x03000310  // Inventilate Storage Mode Button
#define VE0_BE                                 0x00000011  // Vessel class
#define VE0AVL_BE                              0x00000011  // Available
#define VE0LAT_BE                              0x01000011  // Vessel latitude
#define VE0LNG_BE                              0x02000011  // Vessel longitude
#define VE0SPD_BE                              0x03000011  // Vessel speed
#define VE0HEAD_BE                             0x04000011  // Vessel heading
#define VE0PITCH_BE                            0x05000011  // Vessel pitch
#define VE0ROLL_BE                             0x06000011  // Vessel roll
#define VE0PTAB_BE                             0x07000011  // Vessel port trim tab
#define VE0STAB_BE                             0x08000011  // Vessel stbd trim tab
#define VE0JP_BE                               0x09000011  // Vessel jackplate
#define NGN0_BE                                0x00000012  // Engine data class
#define NGN0AVL_BE                             0x00000012  // Available
#define NGN0FUCOH_BE                           0x01000012  // Fuel consumption L/hour
#define NGN0FUCO_BE                            0x02000012  // Fuel consumption L/km
#define NGN0SP_BE                              0x03000012  // Speed
#define NGN0TRANS_BE                           0x04000012  // Transmission
#define NGN0TRIM_BE                            0x05000012  // Trim
#define NGN0HOURS_BE                           0x06000012  // Hours (engine data)
#define NGN0WARN_BE                            0x07000012  // Warning indicator
#define WE0_BE                                 0x00000013  // Weather class
#define WE0AVL_BE                              0x00000013  // Available
#define WE0OTEMP_BE                            0x01000013  // Outside Temperature
#define WE0SEATEMP_BE                          0x02000013  // Sea temperature
#define WE0SEASAL_BE                           0x03000013  // Sea salinity
#define WE0ATM_BE                              0x04000013  // Atmospheric Pressure
#define WE0AH_BE                               0x05000013  // Air Humidity
#define WE0WSP_BE                              0x06000013  // Wind speed
#define WE0WDIR_BE                             0x07000013  // Wind Direction
#define WE0WADEP_BE                            0x08000013  // Water depth
#define WE0TILVL_BE                            0x09000013  // Tide Level
#define WE0TITEN_BE                            0x0a000013  // Tide Tendency
#define WE0SEASP_BE                            0x0b000013  // Sea Current Speed
#define WE0SEADIR_BE                           0x0c000013  // Sea Current Direction
#define BMS0_BE                                0x00000014  // Battery Management Service class
#define BMS0AVL_BE                             0x00000014  // Available
#define BMS0V_BE                               0x01000014  // BMS battery Voltage
#define BMS0FLO_BE                             0x02000014  // BMS battery energy flow
#define BMS0ID_BE                              0x03000014  // BMS battery id
#define BMS0SOC_BE                             0x04000014  // BMS battery state of charge
#define BMS0SOH_BE                             0x05000014  // BMS battery state of health
#define BMS0VHH_BE                             0x06000014  // BMS battery voltage last hour history
#define BMS0VDH_BE                             0x07000014  // BMS battery voltage last day history
#define BMS0VWH_BE                             0x08000014  // BMS battery voltage last week history
#define BMS0ALRTS_BE                           0x09000014  // BMS battery alert status
#define BMS0ALRTE_BE                           0x0a000014  // BMS battery alert enabled
#define BMS0ATHR_BE                            0x0b000014  // BMS battery alert threshold
#define BMS0ALAT_BE                            0x0c000014  // BMS battery alert latency
#define BMS0WRNS_BE                            0x0d000014  // BMS battery warning status
#define BMS0WRNE_BE                            0x0e000014  // BMS battery warning enabled
#define BMS0WTHR_BE                            0x0f000014  // BMS battery warning threshold
#define BMS0WLAT_BE                            0x10000014  // BMS battery warning latency
#define BOS0_BE                                0x00000114  // Bilge Operation Service class
#define BOS0AVL_BE                             0x00000114  // Available
#define BOS0STA_BE                             0x01000114  // BOS bilge status
#define BOS0CL1H_BE                            0x02000114  // BOS bilge cycles in last hour
#define BOS0CL24H_BE                           0x03000114  // BOS bilge cycles in last 24 hours
#define BOS0CRT_BE                             0x04000114  // BOS bilge continuous run time
#define BOS0ID_BE                              0x05000114  // BOS bilge id
#define BOS0CHH_BE                             0x06000114  // BOS bilge cycles hour history
#define BOS0CDH_BE                             0x07000114  // BOS bilge cycles day history
#define BOS0CMH_BE                             0x08000114  // BOS bilge cycles month history
#define BOS0ALRTS_BE                           0x09000114  // BOS bilge alert status
#define BOS0ALRTE_BE                           0x0a000114  // BOS bilge alert enabled
#define BOS0ACHT_BE                            0x0b000114  // BOS bilge alert cycles per hour threshold
#define BOS0ACDT_BE                            0x0c000114  // BOS bilge alert cycles per day threshold
#define BOS0ARTT_BE                            0x0d000114  // BOS bilge alert run time threshold
#define BOS0WRNS_BE                            0x0e000114  // BOS bilge warning status
#define BOS0WRNE_BE                            0x0f000114  // BOS bilge warning enabled
#define BOS0WCHT_BE                            0x10000114  // BOS bilge warning cycles per hour threshold
#define BOS0WCDT_BE                            0x11000114  // BOS bilge warning cycles per day threshold
#define BOS0WRTT_BE                            0x12000114  // BOS bilge warning run time threshold
#define TMS0_BE                                0x00000214  // Tank Monitoring Service class
#define TMS0AVL_BE                             0x00000214  // Available
#define TMS0LVL_BE                             0x01000214  // TMS tank level
#define TMS0TYPE_BE                            0x02000214  // TMS tank type
#define TMS0ALRTS_BE                           0x03000214  // TMS tank alert status
#define TMS0ALRTE_BE                           0x04000214  // TMS tank alert enabled
#define TMS0ALTH_BE                            0x05000214  // TMS tank low level alert threshold
#define TMS0AHTH_BE                            0x06000214  // TMS tank high level alert threshold
#define TMS0ALAT_BE                            0x07000214  // TMS tank level alert latency
#define TMS0WRNS_BE                            0x08000214  // TMS tank warning status
#define TMS0WRNE_BE                            0x09000214  // TMS tank warning enabled
#define TMS0WLTH_BE                            0x0a000214  // TMS tank low warning alert threshold
#define TMS0WHTH_BE                            0x0b000214  // TMS tank high warning alert threshold
#define TMS0WLAT_BE                            0x0c000214  // TMS tank level warning latency
#define BSS0_BE                                0x00000314  // Boat Security Service class
#define BSS0AVL_BE                             0x00000314  // Available
#define BSS0SEA_BE                             0x01000314  // BSS armed
#define BSS0IEN_BE                             0x02000314  // BSS intrusion enabled
#define BSS0ILA_BE                             0x03000314  // BSS intrusion latency
#define BSS0IST_BE                             0x04000314  // BSS intrusion status
#define BSS0ATE_BE                             0x05000314  // BSS anti theft enabled
#define BSS0ATL_BE                             0x06000314  // BSS anti theft latency
#define BSS0ATS_BE                             0x07000314  // BSS anti theft status
#define BTS0_BE                                0x00000414  // Boat Tracking Service class
#define BTS0AVL_BE                             0x00000414  // Available
#define BTS0LOC_BE                             0x01000414  // BTS boat location
#define BTS0LAT_BE                             0x02000414  // BTS boat latitude
#define BTS0LNG_BE                             0x03000414  // BTS boat longitude
#define BTS0GFA_BE                             0x04000414  // BTS geo fence armed
#define BTS0GFR_BE                             0x05000414  // BTS geo fence radius
#define BTS0GFL_BE                             0x06000414  // BTS geo fence latency
#define BTS0GFS_BE                             0x07000414  // BTS geo fence status
#define BTS0LATO_BE                            0x08000414  // BTS boat latitude origin
#define BTS0LNGO_BE                            0x09000414  // BTS boat longitude origin
#define CLS0_BE                                0x00000514  // Climate Service class
#define CLS0AVL_BE                             0x00000514  // Available
#define CLS0NAME_BE                            0x01000514  // Name
#define CLS0SYSU_BE                            0x02000514  // System units
#define USM0_BE                                0x00000614  // Update Service Manager class
#define USM0AVL_BE                             0x00000614  // Available
#define USM0DD_BE                              0x01000614  // Download done
#define USM0STAT_BE                            0x02000614  // Status
#define USM0PROG_BE                            0x03000614  // Progress
#define USM0RRQ_BE                             0x04000614  // Read request
#define USM0DATA_BE                            0x05000614  // Data
#define USM0TBS_BE                             0x06000614  // Transfer block size
#define USM0LIST_BE                            0x07000614  // List of all fwids supported by the USM
#define USM0VER_BE                             0x08000614  // Update process version to use/in use
#define USM0MODE_BE                            0x09000614  // Update process mode to use/in use
#define USM0STATE_BE                           0x0a000614  // State information about ongoing process
#define US0_BE                                 0x00000714  // Update Service class
#define US0AVL_BE                              0x00000714  // Available
#define US0DATA_BE                             0x01000714  // Data
#define US0STAT_BE                             0x02000714  // Status
#define US0LIST_BE                             0x03000714  // List fwids
#define US0BTR_BE                              0x04000714  // Block transfer result
#define US0RRQ_BE                              0x05000714  // Read request
#define US0PROG_BE                             0x06000714  // Progress
#define SVC0_BE                                0x00000814  // System services class
#define SVC0AVL_BE                             0x00000814  // Available
#define SVC0SYST_BE                            0x01000814  // System time
#define SVC0TMZ_BE                             0x02000814  // Time zone offset
#define SVC0NTPS_BE                            0x03000814  // NTP server
#define SVC0TSRC_BE                            0x04000814  // Time source
#define SVC0GPIOR_BE                           0x05000814  // GPIO read
#define SVC0GPIOW_BE                           0x06000814  // GPIO write
#define SVC0SYSTUPD_BE                         0x07000814  // System time update
#define SUP0_BE                                0x00000914  // Supervision services class
#define SUP0AVL_BE                             0x00000914  // Available
#define SUP0ACT_BE                             0x01000914  // Active
#define SUP0TYPE_BE                            0x02000914  // Type of supervision
#define SUP0PARAM_BE                           0x03000914  // Parameter to get data from
#define SUP0STYPE_BE                           0x04000914  // State supervision type
#define SUP0S1_BE                              0x05000914  // State 1 value
#define SUP0S2_BE                              0x06000914  // State 2 value
#define SUP0S1ALT_BE                           0x07000914  // State 1 timeout [s]
#define SUP0S2ALT_BE                           0x08000914  // State 2 timeout [s]
#define SUP0SALARM_BE                          0x09000914  // State alarm status
#define SUP0LHUPLIM_BE                         0x0a000914  // Level high upper limit
#define SUP0LHLOLIM_BE                         0x0b000914  // Level high lower limit
#define SUP0LLUPLIM_BE                         0x0c000914  // Level low upper limit
#define SUP0LLLOLIM_BE                         0x0d000914  // Level low lower limit
#define SUP0LHALT_BE                           0x0e000914  // Level High timeout [s]
#define SUP0LLALT_BE                           0x0f000914  // Level Low timeout [s]
#define SUP0LALTINI_BE                         0x10000914  // Level Initial timeout [s]
#define SUP0LALARM_BE                          0x11000914  // Level alarm status
#define TKS0_BE                                0x00000015  // Tank sensor class
#define TKS0AVL_BE                             0x00000015  // Available
#define TKS0SDEV_BE                            0x01000015  // Sensor device
#define TKS0LVL_BE                             0x02000015  // Level
#define LPGS0_BE                               0x00000115  // LPG Tank sensor class
#define LPGS0AVL_BE                            0x00000115  // Available
#define LPGS0SDEV_BE                           0x01000115  // Sensor device
#define LPGS0LVL_BE                            0x02000115  // Level
#define LPGS0BV_BE                             0x03000115  // Battery voltage
#define AL0_BE                                 0x00000215  // Area Lights class
#define AL0AVL_BE                              0x00000215  // Available
#define AL0CMD_BE                              0x01000215  // Light command
#define MPC0_BE                                0x00000315  // Mopeka Pro Check class
#define MPC0AVL_BE                             0x00000315  // Available
#define MPC0V_BE                               0x01000315  // Voltage
#define MPC0TEMP_BE                            0x02000315  // Temperature
#define MPC0BUTTON_BE                          0x03000315  // Button
#define MPC0RAWTL_BE                           0x04000315  // Raw Tank Level
#define MPC0Q_BE                               0x05000315  // Quality
#define MPC0ID_BE                              0x06000315  // ID low 3 bytes
#define MPC0X_BE                               0x07000315  // X Acceleration
#define MPC0Y_BE                               0x08000315  // Y Acceleration
#define MPC0IDE_BE                             0x09000315  // ID high 3 bytes
#define MPC0TH_BE                              0x0a000315  // Tank height
#define MPC0NAME_BE                            0x0b000315  // Tank name
#define MPC0REGION_BE                          0x0c000315  // Region
#define MPC0SIZE_BE                            0x0d000315  // Tank size
#define MPC0THRESH_BE                          0x0e000315  // Alarm threshold
#define MPC0UNIT_BE                            0x0f000315  // Tank level unit
#define PVS0_BE                                0x00000016  // Password Validation Service class
#define PVS0AVL_BE                             0x00000016  // Available
#define PVS0EKBLOB_BE                          0x01000016  // Entered key blob
#define PVS0VSTAT_BE                           0x02000016  // Validation status
#define PVS0ADDS_BE                            0x03000016  // Add Status
#define PVS0ADD_BE                             0x04000016  // Add
#define KEY0_BE                                0x00000116  // Key class
#define KEY0AVL_BE                             0x00000116  // Available
#define KEY0DEL_BE                             0x01000116  // Delete
#define KEY0NAME_BE                            0x02000116  // Name
#define KEY0ENA_BE                             0x03000116  // Key enable
#define KEY0DOOR_BE                            0x04000116  // Door
#define KEY0DAY_BE                             0x05000116  // Day
#define KEY0FRT_BE                             0x06000116  // From Time
#define KEY0TOT_BE                             0x07000116  // To Time
#define KEY0BLOB_BE                            0x08000116  // Key blob
#define KEY0SENA_BE                            0x09000116  // Schedule enable
#define PCS0_BE                                0x00000216  // Pincode Service class
#define PCS0AVL_BE                             0x00000216  // Available
#define PCS0KIV_BE                             0x01000216  // Key input enable
#define PCS0KPC_BE                             0x02000216  // Key count
#define KP0_BE                                 0x00000017  // Key pad class
#define KP0AVL_BE                              0x00000017  // Available
#define KP0KEYB_BE                             0x01000017  // Key
#define KP0LONGPRESS_BE                        0x02000017  // Longpress of key
#define BZ0_BE                                 0x00000117  // Buzzer class
#define BZ0AVL_BE                              0x00000117  // Available
#define BZ0EVT_BE                              0x01000117  // Event
#define EW0_BE                                 0x00000217  // Display class
#define EW0AVL_BE                              0x00000217  // Available
#define EW0ECD_BE                              0x01000217  // Error code
#define EW0WCD_BE                              0x02000217  // Warning code
#define EW0SECD_BE                             0x03000217  // System error code
#define HMI0_BE                                0x00000317  // HMI Generic class
#define HMI0AVL_BE                             0x00000317  // Available
#define HMI0VER_BE                             0x01000317  // Version of HMI data
#define HMI0TEMPUNIT_BE                        0x02000317  // Presented temperature unit (metric/imperial)
#define HMI0BACKLIGHT_BE                       0x03000317  // Backlight control [0-100]
#define HMI0SOUND_BE                           0x04000317  // Enabling/disabling of sound
#define HMI0TIMEFORMAT_BE                      0x05000317  // Display time format
#define HMI0EVENT_BE                           0x06000317  // Send and publish an event to HMI-module
#define HMI0VARDATA_BE                         0x07000317  // Request varstate value
#define HMI0MUTE_BE                            0x08000317  // Mute sound, overrides hmi0sound
#define HMI0CHILDLOCK_BE                       0x09000317  // Childlock function on HMI
#define HMI0SCREENTIMEOUT_BE                   0x0a000317  // Screen timeout to standby
#define LS0_BE                                 0x00000018  // Log service class
#define LS0AVL_BE                              0x00000018  // Available
#define LS0ADD_BE                              0x01000018  // Add specification
#define LS0READ_BE                             0x02000018  // Read log data request
#define LS0LSCFG_BE                            0x03000018  // Query parameter, about specific features supported
#define LS0RAMSIZE_BE                          0x04000018  // Max RAM size
#define LSCFG0_BE                              0x00000118  // Log specification class
#define LSCFG0AVL_BE                           0x00000118  // Available
#define LSCFG0NAME_BE                          0x01000118  // Name of log configuration.
#define LSCFG0CFG_BE                           0x02000118  // Actual configuration string read from json object
#define LSCFG0READCFG_BE                       0x03000118  // List of linked read log configurations
#define LSCFG0DEL_BE                           0x04000118  // Remove specification
#define LSCFG0ACT_BE                           0x05000118  // Activate
#define LSRCFG0_BE                             0x00000218  // Log read specification class
#define LSRCFG0AVL_BE                          0x00000218  // Available
#define LSRCFG0NAME_BE                         0x01000218  // Name of log read configuration
#define LSRCFG0CFG_BE                          0x02000218  // Actual log read configuration string from json object
#define LSRCFG0DATA_BE                         0x03000218  // Data according to read configuration in JSON format
#define LSRCFG0BINDATA_BE                      0x04000218  // Data according to read configuration in binary format
#define RULE0_BE                               0x00000019  // Rule Engine class
#define RULE0AVL_BE                            0x00000019  // Available
#define RULE0NAME_BE                           0x01000019  // Name
#define RULE0NUM_BE                            0x02000019  // Number of rules
#define RULE0ACT_BE                            0x03000019  // Active
#define MCCC0_BE                               0x0000001a  // Mobile Cooling Compressor controller class
#define MCCC0AVL_BE                            0x0000001a  // Available
#define MCCC0PTYPE_BE                          0x0100001a  // PRODUCT_TYPE
#define MCCC0NOCPT_BE                          0x0200001a  // Number of compartments
#define MCCC0CPOW_BE                           0x0300001a  // C(x)_POWER
#define MCCC0CTEMP_BE                          0x0400001a  // C(x)_MEASURED_TEMPERATURE
#define MCCC0CSETTEMP_BE                       0x0500001a  // C(x)_SET_TEMPERATURE
#define MCCC0ACPT_BE                           0x0600001a  // ACTIVE_COMPARTMENT
#define MCCC0CDOOR_BE                          0x0700001a  // C(x)_DOOR_OPEN
#define MCCC0CTEMPRNG_BE                       0x0800001a  // C(x)_TEMPERATURE_RANGE
#define MCCC0CRECDRNG_BE                       0x0900001a  // C(x)_RECOMMENDED_RANGE
#define MCCC0CTEMPOFS_BE                       0x0a00001a  // C(x)_TEMPERATURE_OFFSET
#define MCCC0COOLERPOW_BE                      0x0b00001a  // COOLER_POWER
#define MCCC0V_BE                              0x0c00001a  // BATTERY_VOLTAGE_LEVEL
#define MCCC0BATPROTLVL_BE                     0x0d00001a  // BATTERY_PROTECTION_LEVEL
#define MCCC0COMPPOW_BE                        0x0e00001a  // COMPRESSOR_POWER
#define MCCC0I_BE                              0x0f00001a  // DC_CURRENT_LEVEL
#define MCCC0POWSRC_BE                         0x1000001a  // POWER_SOURCE
#define MCCC0ICEPOW_BE                         0x1100001a  // ICEMAKER_POWER
#define MCCC0ERRST_BE                          0x1200001a  // Error and alert status
#define MCCC0SN_BE                             0x1300001a  // CC_SERIAL_NUMBER
#define MCCC0SKU_BE                            0x1400001a  // Sku number of compressor controller board
#define MCCC0FWVER_BE                          0x1500001a  // CC_FIRMWARE_VERSION
#define MCCC0PARTDET_BE                        0x1600001a  // Partition board detected status
#define MCCC0ETEMP_BE                          0x1700001a  // Ambient temperature (external to cooler)
#define MCCC0PARTDETENA_BE                     0x1800001a  // Partition board sensor enabled
#define MCCC0BOOST_BE                          0x1900001a  // Power boost status
#define RVCMGNT0_BE                            0x0000001b  // RVC DGN Management class
#define RVCMGNT0AVL_BE                         0x0000001b  // Available
#define RVCMGNT0REQ_BE                         0x0100001b  // DGN request
#define RVCMGNT0ADDR_BE                        0x0200001b  // Source address after address claim
#define RVCMGNT0RAW_BE                         0x0300001b  // Send a raw DGN frame. Data format is a struct.
#define RVCMGNT0RAWRX_BE                       0x0400001b  // Received DGN frame
#define RVCMGNT0RESETCONF_BE                   0x0500001b  // General reset params
#define RVCMGNT0RESET_BE                       0x0600001b  // Send general reset (0x17F00) to address
#define RVCMGNT0ACK_BE                         0x0700001b  // ACK message (0xE800)
#define RVCMGNT0DOWNLOAD_BE                    0x0800001b  // DOWNLOAD message (0x17D00)
#define RVCPIM0_BE                             0x0000011b  // RVC DGN PIM (65259) class
#define RVCPIM0AVL_BE                          0x0000011b  // Available
#define RVCPIM0SYNC_BE                         0x0100011b  // DGN class sync command
#define RVCPIM0MAKE_BE                         0x0200011b  // Make
#define RVCPIM0MDL_BE                          0x0300011b  // Model
#define RVCPIM0SNR_BE                          0x0400011b  // Serial number
#define RVCPIM0UNIT_BE                         0x0500011b  // Unit
#define RVCTIME0_BE                            0x0000021b  // RVC DGN Date Time (131071/131070) (status/command) class
#define RVCTIME0AVL_BE                         0x0000021b  // Available
#define RVCTIME0SYNC_BE                        0x0100021b  // DGN class sync command
#define RVCTIME0YEAR_BE                        0x0200021b  // Year
#define RVCTIME0MONTH_BE                       0x0300021b  // Month
#define RVCTIME0DAY_BE                         0x0400021b  // Day
#define RVCTIME0HOUR_BE                        0x0500021b  // Hour
#define RVCTIME0MIN_BE                         0x0600021b  // Minute
#define RVCTIME0SEC_BE                         0x0700021b  // Second
#define RVCTIME0TZ_BE                          0x0800021b  // Timezone
#define RVCTIME0DAYOFWEEK_BE                   0x0900021b  // Day of the week
#define RVCFURNACE0_BE                         0x0000031b  // RVC DGN Furnace (131044/131043) (status/command) class
#define RVCFURNACE0AVL_BE                      0x0000031b  // Available
#define RVCFURNACE0SYNC_BE                     0x0100031b  // DGN class sync command
#define RVCFURNACE0INST_BE                     0x0200031b  // Instance (zone)
#define RVCFURNACE0MODE_BE                     0x0300031b  // Operating mode (manual=ignore thermostat commands)
#define RVCFURNACE0HSRC_BE                     0x0400031b  // Heat source
#define RVCFURNACE0FSPD_BE                     0x0500031b  // Circulation fan speed
#define RVCFURNACE0HLVL_BE                     0x0600031b  // Heat ouput level
#define RVCFURNACE0DBAND_BE                    0x0700031b  // Dead band (0-25 degrees)
#define RVCFURNACE0SDBAND_BE                   0x0800031b  // Secondary stage Dead band (0-25 degrees)
#define RVCFURNACE0OCSTAT_BE                   0x0900031b  // Zone over current status
#define RVCFURNACE0UCSTAT_BE                   0x0a00031b  // Zone under current status
#define RVCFURNACE0TSTAT_BE                    0x0b00031b  // Zone temperature status
#define RVCFURNACE0ISTAT_BE                    0x0c00031b  // Zone analog input status
#define RVCTH0_BE                              0x0000041b  // RVC DGN Thermostat 1 (131042/130809) (status/command) class
#define RVCTH0AVL_BE                           0x0000041b  // Available
#define RVCTH0SYNC_BE                          0x0100041b  // DGN class sync command
#define RVCTH0INST_BE                          0x0200041b  // Instance (zone)
#define RVCTH0MODE_BE                          0x0300041b  // Operating mode
#define RVCTH0FMODE_BE                         0x0400041b  // Fan mode
#define RVCTH0SMODE_BE                         0x0500041b  // Schedule mode
#define RVCTH0FSPD_BE                          0x0600041b  // Fan speed
#define RVCTH0HSET_BE                          0x0700041b  // Setpoint temp - heat
#define RVCTH0CSET_BE                          0x0800041b  // Setpoint temp - cool
#define RVCTH0STATUS_BE                        0x0900041b  // DGN data
#define RVCTH0COMMAND_BE                       0x0a00041b  // DGN data
#define RVCTHTWO0_BE                           0x0000051b  // RVC DGN Thermostat 2 (130810/130808) (status/command) class
#define RVCTHTWO0AVL_BE                        0x0000051b  // Available
#define RVCTHTWO0SYNC_BE                       0x0100051b  // DGN class sync command
#define RVCTHTWO0INST_BE                       0x0200051b  // Instance (zone)
#define RVCTHTWO0CSINST_BE                     0x0300051b  // Current schedule instance
#define RVCTHTWO0NSINST_BE                     0x0400051b  // Number of schedule instances
#define RVCTHTWO0NOISE_BE                      0x0500051b  // Reduced noise mode
#define RVCTHSCHED0_BE                         0x0000061b  // RVC DGN Thermostat Schedule 1 (130807/130805) (status/command) class
#define RVCTHSCHED0AVL_BE                      0x0000061b  // Available
#define RVCTHSCHED0SYNC_BE                     0x0100061b  // DGN class sync command
#define RVCTHSCHED0INST_BE                     0x0200061b  // Instance (zone)
#define RVCTHSCHED0SINST_BE                    0x0300061b  // Schedule mode instance
#define RVCTHSCHED0HOUR_BE                     0x0400061b  // Start Hour, precision 1 hour
#define RVCTHSCHED0MIN_BE                      0x0500061b  // Start minute, precision 1 m
#define RVCTHSCHED0HSET_BE                     0x0600061b  // Setpoint temp - heat
#define RVCTHSCHED0CSET_BE                     0x0700061b  // Setpoint temp - cool
#define RVCTHSCHEDTWO0_BE                      0x0000071b  // RVC DGN Thermostat Schedule 2 (130806/130804) (status/command) class
#define RVCTHSCHEDTWO0AVL_BE                   0x0000071b  // Available
#define RVCTHSCHEDTWO0SYNC_BE                  0x0100071b  // DGN class sync command
#define RVCTHSCHEDTWO0INST_BE                  0x0200071b  // Instance (zone)
#define RVCTHSCHEDTWO0SINST_BE                 0x0300071b  // Schedule mode instance
#define RVCTHSCHEDTWO0SUN_BE                   0x0400071b  // Sunday
#define RVCTHSCHEDTWO0MON_BE                   0x0500071b  // Monday
#define RVCTHSCHEDTWO0TUE_BE                   0x0600071b  // Tuesday
#define RVCTHSCHEDTWO0WED_BE                   0x0700071b  // Wednesday
#define RVCTHSCHEDTWO0THU_BE                   0x0800071b  // Thursday
#define RVCTHSCHEDTWO0FRI_BE                   0x0900071b  // Friday
#define RVCTHSCHEDTWO0SAT_BE                   0x0a00071b  // Saturday
#define RVCTHASTAT0_BE                         0x0000081b  // RVC DGN Thermostat Ambient status (130972) class
#define RVCTHASTAT0AVL_BE                      0x0000081b  // Available
#define RVCTHASTAT0SYNC_BE                     0x0100081b  // DGN class sync command
#define RVCTHASTAT0INST_BE                     0x0200081b  // Instance (zone)
#define RVCTHASTAT0TEMP_BE                     0x0300081b  // Ambient temperature
#define RVCAC0_BE                              0x0000091b  // RVC DGN Air Conditioner (131041/131040) (status/command) class
#define RVCAC0AVL_BE                           0x0000091b  // Available
#define RVCAC0SYNC_BE                          0x0100091b  // DGN class sync command
#define RVCAC0INST_BE                          0x0200091b  // Instance (zone)
#define RVCAC0MODE_BE                          0x0300091b  // Operating mode (manual=ignore thermostat commands)
#define RVCAC0MFSPD_BE                         0x0400091b  // Max fan speed
#define RVCAC0MOLVL_BE                         0x0500091b  // Max air conditioning output level
#define RVCAC0FSPD_BE                          0x0600091b  // Fan speed
#define RVCAC0OLVL_BE                          0x0700091b  // Air conditioner output level
#define RVCAC0DBAND_BE                         0x0800091b  // Dead band (0-25 degrees)
#define RVCAC0SDBAND_BE                        0x0900091b  // Secondary stage Dead band (0-25 degrees)
#define RVCACTWO0_BE                           0x00000a1b  // RVC DGN Air Conditioner 2 status (130505) class
#define RVCACTWO0AVL_BE                        0x00000a1b  // Available
#define RVCACTWO0SYNC_BE                       0x01000a1b  // DGN class sync command
#define RVCACTWO0INST_BE                       0x02000a1b  // Instance (zone)
#define RVCACTWO0COMPSTAT_BE                   0x03000a1b  // Compressor status
#define RVCACTWO0NOISE_BE                      0x04000a1b  // Reduced noise mode
#define RVCACTWO0EXTTEMP_BE                    0x05000a1b  // Exterior temperature
#define RVCACTWO0COILTEMP_BE                   0x06000a1b  // Coil temperature
#define RVCACTWO0COILTEMPERR_BE                0x07000a1b  // Coil temp error
#define RVCACTWO0COILFREEZE_BE                 0x08000a1b  // Coil freeze detected
#define RVCACTWO0EXTTEMPERR_BE                 0x09000a1b  // Exterior temperature error
#define RVCACTWO0DEFROST_BE                    0x0a000a1b  // Defrost cycle active
#define RVCHPUMP0_BE                           0x00000b1b  // RVC DGN Heat Pump (130971/130970) (status/command) class
#define RVCHPUMP0AVL_BE                        0x00000b1b  // Available
#define RVCHPUMP0SYNC_BE                       0x01000b1b  // DGN class sync command
#define RVCHPUMP0INST_BE                       0x02000b1b  // Instance (zone)
#define RVCHPUMP0MODE_BE                       0x03000b1b  // Operating mode (manual=ignore thermostat commands)
#define RVCHPUMP0MOLVL_BE                      0x04000b1b  // Heat max output level
#define RVCHPUMP0OLVL_BE                       0x05000b1b  // Heat output level
#define RVCHPUMP0DBAND_BE                      0x06000b1b  // Dead band (0-25 degrees)
#define RVCHPUMP0SDBAND_BE                     0x07000b1b  // Secondary stage Dead band (0-25 degrees)
#define RVCHPUMP0FSPD_BE                       0x08000b1b  // Fan speed
#define RVCDIM0_BE                             0x00000c1b  // RVC DGN DC Dimmer 1 (131003/131001) (status/command) class
#define RVCDIM0AVL_BE                          0x00000c1b  // Available
#define RVCDIM0SYNC_BE                         0x01000c1b  // DGN class sync command
#define RVCDIM0INST_BE                         0x02000c1b  // Instance (zone)
#define RVCDIM0MSTBS_BE                        0x03000c1b  // Master brightness
#define RVCDIM0RBS_BE                          0x04000c1b  // Red brightness
#define RVCDIM0GBS_BE                          0x05000c1b  // Green brightness
#define RVCDIM0BBS_BE                          0x06000c1b  // Blue brightness
#define RVCDIM0WBS_BE                          0x07000c1b  // White brightness
#define RVCDIM0ON_BE                           0x08000c1b  // On duration (seconds) 0-14, 0 means always on
#define RVCDIM0OFF_BE                          0x09000c1b  // Off duration (seconds) 0-14, 0 means one-shot and then stay off
#define RVCDIMTWO0_BE                          0x00000d1b  // RVC DGN DC Dimmer 2 (131002/130779) (status/command) class
#define RVCDIMTWO0AVL_BE                       0x00000d1b  // Available
#define RVCDIMTWO0SYNC_BE                      0x01000d1b  // DGN class sync command
#define RVCDIMTWO0INST_BE                      0x02000d1b  // Instance (zone)
#define RVCDIMTWO0MCURR_BE                     0x03000d1b  // Master current (A)
#define RVCDIMTWO0RCURR_BE                     0x04000d1b  // Red current (A)
#define RVCDIMTWO0GCURR_BE                     0x05000d1b  // Green current (A)
#define RVCDIMTWO0BCURR_BE                     0x06000d1b  // Blue current (A)
#define RVCDIMTWO0WCURR_BE                     0x07000d1b  // White current (A)
#define RVCDIMTWO0MFAULT_BE                    0x08000d1b  // Master fault
#define RVCDIMTWO0RFAULT_BE                    0x09000d1b  // Red fault
#define RVCDIMTWO0GFAULT_BE                    0x0a000d1b  // Green fault
#define RVCDIMTWO0BFAULT_BE                    0x0b000d1b  // Blue fault
#define RVCDIMTWO0WFAULT_BE                    0x0c000d1b  // White fault
#define RVCDIMTWO0GROUP_BE                     0x0d000d1b  // Group (command)
#define RVCDIMTWO0LVLBS_BE                     0x0e000d1b  // Desired level brightness (command)
#define RVCDIMTWO0CMD_BE                       0x0f000d1b  // Command (command)
#define RVCDIMTWO0DEL_DUR_BE                   0x10000d1b  // Delay/duration (command)
#define RVCDIMTWO0ILOCK_BE                     0x11000d1b  // Interlock (command)
#define RVCDIMTWO0RTIME_BE                     0x12000d1b  // Ramp time (command)
#define RVCDIMTHR0_BE                          0x00000e1b  // RVC DGN DC Dimmer 3 status (130778) class
#define RVCDIMTHR0AVL_BE                       0x00000e1b  // Available
#define RVCDIMTHR0SYNC_BE                      0x01000e1b  // DGN class sync command
#define RVCDIMTHR0INST_BE                      0x02000e1b  // Instance (zone)
#define RVCDIMTHR0GROUP_BE                     0x03000e1b  // Group
#define RVCDIMTHR0LVLBS_BE                     0x04000e1b  // Operating brightness
#define RVCDIMTHR0LOCK_BE                      0x05000e1b  // Lock status
#define RVCDIMTHR0OCURR_BE                     0x06000e1b  // Overcurrent status
#define RVCDIMTHR0ORIDE_BE                     0x07000e1b  // Override status
#define RVCDIMTHR0EN_BE                        0x08000e1b  // Enable status
#define RVCDIMTHR0DEL_DUR_BE                   0x09000e1b  // Delay/duration
#define RVCDIMTHR0LCMD_BE                      0x0a000e1b  // Last command
#define RVCDIMTHR0ILOCK_BE                     0x0b000e1b  // Interlock status
#define RVCDIMTHR0LOAD_BE                      0x0c000e1b  // Load status
#define RVCDIMTHR0UCURR_BE                     0x0d000e1b  // Undercurrent status
#define RVCDIMTHR0MEM_BE                       0x0e000e1b  // Last memory value
#define RVCRFAN0_BE                            0x00000f1b  // RVC DGN Roof Fan (130727/130726) (status/command) class
#define RVCRFAN0AVL_BE                         0x00000f1b  // Available
#define RVCRFAN0SYNC_BE                        0x01000f1b  // DGN class sync command
#define RVCRFAN0INST_BE                        0x02000f1b  // Instance (zone)
#define RVCRFAN0SYST_BE                        0x03000f1b  // System status
#define RVCRFAN0FMODE_BE                       0x04000f1b  // Fan mode
#define RVCRFAN0SMODE_BE                       0x05000f1b  // Speed mode
#define RVCRFAN0LGT_BE                         0x06000f1b  // Light
#define RVCRFAN0FSET_BE                        0x07000f1b  // Fan speed setting
#define RVCRFAN0WDIR_BE                        0x08000f1b  // Wind direction switch
#define RVCRFAN0DPOS_BE                        0x09000f1b  // Dome position switch (deprecated)
#define RVCRFAN0TEMP_BE                        0x0a000f1b  // Ambient temperature
#define RVCRFAN0SETTEMP_BE                     0x0b000f1b  // Set point
#define RVCRFANTWO0_BE                         0x0000101b  // RVC DGN Roof Fan 2 (130531/130530) (status/command) class
#define RVCRFANTWO0AVL_BE                      0x0000101b  // Available
#define RVCRFANTWO0SYNC_BE                     0x0100101b  // DGN class sync command
#define RVCRFANTWO0INST_BE                     0x0200101b  // Instance (zone)
#define RVCRFANTWO0DMODE_BE                    0x0300101b  // Dome mode
#define RVCRFANTWO0DPOS_BE                     0x0400101b  // Dome position 
#define RVCRFANTWO0RAINSNS_BE                  0x0500101b  // Rain sensor
#define RVCRFANTWO0RAINSNSOV_BE                0x0600101b  // Rain sensor override
#define RVCRFANTWO0DSTATE_BE                   0x0700101b  // Setpoint Controlled Dome State
#define RVCRFANTWO0DCFOFF_BE                   0x0800101b  // Auto Close Dome on Fan Off
#define RVCRFANTWO0FOFFDC_BE                   0x0900101b  // Auto Fan Off on Dome Close
#define RVCRFANTWO0FSPDST_BE                   0x0a00101b  // Fan Speed Step Increment/Decrement
#define RVCRFANTWO0FSPDSTSUP_BE                0x0b00101b  // Fan Steps (Speeds) Supported
#define RVCRFANTWO0FSPDSTSET_BE                0x0c00101b  // Fan Speed Increment/Decrement Step
#define RVCHTR0_BE                             0x0000111b  // RVC DGN Heater operation(130559/130558) (status/command) class
#define RVCHTR0AVL_BE                          0x0000111b  // Available
#define RVCHTR0SYNC_BE                         0x0100111b  // DGN class sync command
#define RVCHTR0INST_BE                         0x0200111b  // Heater instance
#define RVCHTR0ESRC_BE                         0x0300111b  // Energy source
#define RVCHTR0AIR_BE                          0x0400111b  // Air heater command
#define RVCHTR0WTR_BE                          0x0500111b  // Water heater command
#define RVCHTR0AMODE_BE                        0x0600111b  // Air heater mode
#define RVCHTR0WTRMODE_BE                      0x0700111b  // Water heater mode
#define RVCHTR0TTEMP_BE                        0x0800111b  // Target room temperature
#define RVCHTR0SILFMAX_BE                      0x0900111b  // Silent mode max fan speed
#define RVCHTR0VFMIN_BE                        0x0a00111b  // Ventilation mode min fan speed
#define RVCHTR0UVTHRES_BE                      0x0b00111b  // Under voltage threshold
#define RVCHTR0SYSU_BE                         0x0c00111b  // System units
#define RVCHTRST0_BE                           0x0000121b  // RVC DGN Heater status (130557) class
#define RVCHTRST0AVL_BE                        0x0000121b  // Available
#define RVCHTRST0SYNC_BE                       0x0100121b  // DGN class sync command
#define RVCHTRST0INST_BE                       0x0200121b  // Heater instance
#define RVCHTRST0RTEMP_BE                      0x0300121b  // Room temperature
#define RVCHTRST0WTEMP_BE                      0x0400121b  // Water temperature
#define RVCHTRST0GAIRST_BE                     0x0500121b  // Gas air heater state
#define RVCHTRST0GWTRST_BE                     0x0600121b  // Gas water heater state
#define RVCHTRST0ACAVL_BE                      0x0700121b  // AC present
#define RVCHTRST0ACAIRST_BE                    0x0800121b  // AC air heater state
#define RVCHTRST0ACWTRST_BE                    0x0900121b  // AC water heater state
#define RVCHMI0_BE                             0x0000131b  // RVC DGN HMI status (130556) class
#define RVCHMI0AVL_BE                          0x0000131b  // Available
#define RVCHMI0SYNC_BE                         0x0100131b  // DGN class sync command
#define RVCHMI0INST_BE                         0x0200131b  // HMI instance
#define RVCHMI0RTEMP_BE                        0x0300131b  // Room temperature
#define RVCHMI0COM_BE                          0x0400131b  // Heater communication
#define RVCHMI0VOLT_BE                         0x0500131b  // Input voltage
#define RVCHMI0INSTST_BE                       0x0600131b  // HMI Instance status
#define RVCHMI0ICIRC_BE                        0x0700131b  // Internal circuitry
#define RVCHMI0FAVBTN_BE                       0x0800131b  // Favorite button status
#define RVCHMI0MENUBTN_BE                      0x0900131b  // Menu button status
#define RVCHMI0HOMEBTN_BE                      0x0a00131b  // Home button status
#define RVCHTRSCHED0_BE                        0x0000141b  // RVC DGN Heater scheduling (130555/130554) (status/command) class
#define RVCHTRSCHED0AVL_BE                     0x0000141b  // Available
#define RVCHTRSCHED0SYNC_BE                    0x0100141b  // DGN class sync command
#define RVCHTRSCHED0INST_BE                    0x0200141b  // Heater instance
#define RVCHTRSCHED0ATOFFST_BE                 0x0300141b  // Air heater timer off status
#define RVCHTRSCHED0ATOFFH_BE                  0x0400141b  // Air heater timer off hour
#define RVCHTRSCHED0ATOFFM_BE                  0x0500141b  // Air heater timer off minute
#define RVCHTRSCHED0ATONST_BE                  0x0600141b  // Air heater timer on status
#define RVCHTRSCHED0ATONH_BE                   0x0700141b  // Air heater timer on hour
#define RVCHTRSCHED0ATONM_BE                   0x0800141b  // Air heater timer on minute
#define RVCHTRSCHED0WTONST_BE                  0x0900141b  // Water heater timer on status
#define RVCHTRSCHED0WTONH_BE                   0x0a00141b  // Water heater timer on hour
#define RVCHTRSCHED0WTONM_BE                   0x0b00141b  // Water heater timer on minute
#define RVCHTRSCHED0WKEEP_BE                   0x0c00141b  // Water heater keep on time
#define RVCHTRFAULT0_BE                        0x0000151b  // RVC DGN Heater active faults codes (130553) class
#define RVCHTRFAULT0AVL_BE                     0x0000151b  // Available
#define RVCHTRFAULT0SYNC_BE                    0x0100151b  // DGN class sync command
#define RVCHTRFAULT0INST_BE                    0x0200151b  // Heater instance
#define RVCHTRFAULT0WARN_BE                    0x0300151b  // Warning fault active
#define RVCHTRFAULT0CRIT_BE                    0x0400151b  // Critical fault active
#define RVCHTRFAULT0ONE_BE                     0x0500151b  // Active fault code 1
#define RVCHTRFAULT0TWO_BE                     0x0600151b  // Active fault code 2
#define RVCHTRFAULT0THREE_BE                   0x0700151b  // Active fault code 3
#define RVCHTRFAULT0FOUR_BE                    0x0800151b  // Active fault code 4
#define RVCHTRVER0_BE                          0x0000161b  // RVC DGN Heater version numbers (130552) class
#define RVCHTRVER0AVL_BE                       0x0000161b  // Available
#define RVCHTRVER0SYNC_BE                      0x0100161b  // DGN class sync command
#define RVCHTRVER0INST_BE                      0x0200161b  // Heater instance
#define RVCHTRVER0CMAJ_BE                      0x0300161b  // Comfort MCU SW version major
#define RVCHTRVER0CMIN_BE                      0x0400161b  // Comfort MCU SW version minor
#define RVCHTRVER0BMAJ_BE                      0x0500161b  // Burner MCU SW version major
#define RVCHTRVER0BMIN_BE                      0x0600161b  // Burner MCU SW version minor
#define RVCHTRVER0PCBA_BE                      0x0700161b  // PCBA version
#define RVCHTRVER0PMAJ_BE                      0x0800161b  // Protocol version major
#define RVCHTRVER0PMIN_BE                      0x0900161b  // Protocol version minor
#define RVCPROP0_BE                            0x0000171b  // RVC DGN Proprietary message (61184) class
#define RVCPROP0AVL_BE                         0x0000171b  // Available
#define RVCPROP0SYNC_BE                        0x0100171b  // DGN class sync command
#define RVCPROP0ADDR_BE                        0x0200171b  // Address (rvc)
#define RVCPROP0DATA_BE                        0x0300171b  // Data bytes 0-7
#define RVCDCSRC0_BE                           0x0000181b  // RVC DGN DC Source Status 1 (131069) class
#define RVCDCSRC0AVL_BE                        0x0000181b  // Available
#define RVCDCSRC0SYNC_BE                       0x0100181b  // DGN class sync command
#define RVCDCSRC0INST_BE                       0x0200181b  // DC instance
#define RVCDCSRC0PRIO_BE                       0x0300181b  // Device priority
#define RVCDCSRC0VOLT_BE                       0x0400181b  // DC voltage
#define RVCDCSRC0CURR_BE                       0x0500181b  // DC current
#define RVCDCSRCTWO0_BE                        0x0000191b  // RVC DGN DC Source Status 2 (131068) class
#define RVCDCSRCTWO0AVL_BE                     0x0000191b  // Available
#define RVCDCSRCTWO0SYNC_BE                    0x0100191b  // DGN class sync command
#define RVCDCSRCTWO0INST_BE                    0x0200191b  // DC instance
#define RVCDCSRCTWO0PRIO_BE                    0x0300191b  // Device priority
#define RVCDCSRCTWO0STEMP_BE                   0x0400191b  // Source temperature
#define RVCDCSRCTWO0SOC_BE                     0x0500191b  // State of charge (SOC)
#define RVCDCSRCTWO0TIMEREM_BE                 0x0600191b  // Time remaining
#define RVCDCSRCTWO0TIMEREMTYPE_BE             0x0700191b  // Time remaining interpretation
#define RVCDCSRCTHR0_BE                        0x00001a1b  // RVC DGN DC Source Status 3 (131067) class
#define RVCDCSRCTHR0AVL_BE                     0x00001a1b  // Available
#define RVCDCSRCTHR0SYNC_BE                    0x01001a1b  // DGN class sync command
#define RVCDCSRCTHR0INST_BE                    0x02001a1b  // DC instance
#define RVCDCSRCTHR0PRIO_BE                    0x03001a1b  // Device priority
#define RVCDCSRCTHR0SOH_BE                     0x04001a1b  // State of health 
#define RVCDCSRCTHR0CAP_BE                     0x05001a1b  // Capacity remaining (Ah)
#define RVCDCSRCTHR0RELCAP_BE                  0x06001a1b  // Relative capacity
#define RVCDCSRCTHR0RIPPLE_BE                  0x07001a1b  // AC RMS ripple
#define RVCDCSRCFOUR0_BE                       0x00001b1b  // RVC DGN DC Source Status 4 (130761) class
#define RVCDCSRCFOUR0AVL_BE                    0x00001b1b  // Available
#define RVCDCSRCFOUR0SYNC_BE                   0x01001b1b  // DGN class sync command
#define RVCDCSRCFOUR0INST_BE                   0x02001b1b  // DC instance
#define RVCDCSRCFOUR0PRIO_BE                   0x03001b1b  // Device priority
#define RVCDCSRCFOUR0CHGST_BE                  0x04001b1b  // Desired charge state
#define RVCDCSRCFOUR0VOLT_BE                   0x05001b1b  // Desired DC voltage
#define RVCDCSRCFOUR0CURR_BE                   0x06001b1b  // Desired DC current
#define RVCDCSRCFOUR0TYPE_BE                   0x07001b1b  // Battery type
#define RVCDCSRCFIVE0_BE                       0x00001c1b  // RVC DGN DC Source Status 5 (130760) class
#define RVCDCSRCFIVE0AVL_BE                    0x00001c1b  // Available
#define RVCDCSRCFIVE0SYNC_BE                   0x01001c1b  // DGN class sync command
#define RVCDCSRCFIVE0INST_BE                   0x02001c1b  // DC instance
#define RVCDCSRCFIVE0PRIO_BE                   0x03001c1b  // Device priority
#define RVCDCSRCFIVE0VOLT_BE                   0x04001c1b  // HP DC voltage
#define RVCDCSRCSIX0_BE                        0x00001d1b  // RVC DGN DC Source Status 6 (130759) class
#define RVCDCSRCSIX0AVL_BE                     0x00001d1b  // Available
#define RVCDCSRCSIX0SYNC_BE                    0x01001d1b  // DGN class sync command
#define RVCDCSRCSIX0INST_BE                    0x02001d1b  // DC instance
#define RVCDCSRCSIX0PRIO_BE                    0x03001d1b  // Device priority
#define RVCDCSRCSIX0HVLIM_BE                   0x04001d1b  // High voltage limit status
#define RVCDCSRCSIX0HVDIS_BE                   0x05001d1b  // High voltage disconnect status
#define RVCDCSRCSIX0LVLIM_BE                   0x06001d1b  // Low voltage limit status
#define RVCDCSRCSIX0LVDIS_BE                   0x07001d1b  // Low voltage disconnect status
#define RVCDCSRCSIX0LSOCLIM_BE                 0x08001d1b  // Low state of charge limit status
#define RVCDCSRCSIX0LSOCDIS_BE                 0x09001d1b  // Low state of charge disconnect status
#define RVCDCSRCSIX0LDCTEMPLIM_BE              0x0a001d1b  // Low DC source temperature limit status
#define RVCDCSRCSIX0LDCTEMPDIS_BE              0x0b001d1b  // Low DC source temperature disconnect status
#define RVCDCSRCSIX0HDCCURRLIM_BE              0x0c001d1b  // High current DC source limit status
#define RVCDCSRCSIX0HDCCURRDIS_BE              0x0d001d1b  // High current DC source disconnect status
#define RVCDCSRCSIX0HDCTEMPLIM_BE              0x0e001d1b  // High DC source temperature limit status
#define RVCDCSRCSIX0HDCTEMPDIS_BE              0x0f001d1b  // High DC source temperature disconnect status
#define RVCDCSRCSEV0_BE                        0x00001e1b  // RVC DGN DC Source Status 7 (130732) class
#define RVCDCSRCSEV0AVL_BE                     0x00001e1b  // Available
#define RVCDCSRCSEV0SYNC_BE                    0x01001e1b  // DGN class sync command
#define RVCDCSRCSEV0INST_BE                    0x02001e1b  // DC instance
#define RVCDCSRCSEV0PRIO_BE                    0x03001e1b  // Device priority
#define RVCDCSRCSEV0INPUT_BE                   0x04001e1b  // Today's Input Ampere Hours
#define RVCDCSRCSEV0OUTPUT_BE                  0x05001e1b  // Today's Output Ampere Hours
#define RVCDCSRCEIG0_BE                        0x00001f1b  // RVC DGN DC Source Status 8 (130731) class
#define RVCDCSRCEIG0AVL_BE                     0x00001f1b  // Available
#define RVCDCSRCEIG0SYNC_BE                    0x01001f1b  // DGN class sync command
#define RVCDCSRCEIG0INST_BE                    0x02001f1b  // DC instance
#define RVCDCSRCEIG0PRIO_BE                    0x03001f1b  // Device priority
#define RVCDCSRCEIG0INPUT_BE                   0x04001f1b  // Yesterday's Input Ampere Hours
#define RVCDCSRCEIG0OUTPUT_BE                  0x05001f1b  // Yesterday's Output Ampere Hours
#define RVCDCSRCNINE0_BE                       0x0000201b  // RVC DGN DC Source Status 9 (130730) class
#define RVCDCSRCNINE0AVL_BE                    0x0000201b  // Available
#define RVCDCSRCNINE0SYNC_BE                   0x0100201b  // DGN class sync command
#define RVCDCSRCNINE0INST_BE                   0x0200201b  // DC instance
#define RVCDCSRCNINE0PRIO_BE                   0x0300201b  // Device priority
#define RVCDCSRCNINE0INPUT_BE                  0x0400201b  // Day Before Yesterday's Input Ampere Hours
#define RVCDCSRCNINE0OUTPUT_BE                 0x0500201b  // Day Before Yesterday's Output Ampere Hours
#define RVCDCSRCTEN0_BE                        0x0000211b  // RVC DGN DC Source Status 10 (130729) class
#define RVCDCSRCTEN0AVL_BE                     0x0000211b  // Available
#define RVCDCSRCTEN0SYNC_BE                    0x0100211b  // DGN class sync command
#define RVCDCSRCTEN0INST_BE                    0x0200211b  // DC instance
#define RVCDCSRCTEN0PRIO_BE                    0x0300211b  // Device priority
#define RVCDCSRCTEN0INPUT_BE                   0x0400211b  // Last 7 days Input Ampere Hours
#define RVCDCSRCTEN0OUTPUT_BE                  0x0500211b  // Last 7 days Output Ampere Hours
#define RVCDCSRCELE0_BE                        0x0000221b  // RVC DGN DC Source Status 11 (130725) class
#define RVCDCSRCELE0AVL_BE                     0x0000221b  // Available
#define RVCDCSRCELE0SYNC_BE                    0x0100221b  // DGN class sync command
#define RVCDCSRCELE0INST_BE                    0x0200221b  // DC instance
#define RVCDCSRCELE0PRIO_BE                    0x0300221b  // Device priority
#define RVCDCSRCELE0DISCHGST_BE                0x0400221b  // Discharge On/Off status
#define RVCDCSRCELE0CHGST_BE                   0x0500221b  // Charge On/Off status
#define RVCDCSRCELE0CHGDET_BE                  0x0600221b  // Charge detected
#define RVCDCSRCELE0RESST_BE                   0x0700221b  // Reserve status
#define RVCDCSRCELE0CAPACITY_BE                0x0800221b  // Full capacity
#define RVCDCSRCELE0POWER_BE                   0x0900221b  // DC power
#define RVCDCSRCTWE0_BE                        0x0000231b  // RVC DGN DC Source Status 12 (130552) class
#define RVCDCSRCTWE0AVL_BE                     0x0000231b  // Available
#define RVCDCSRCTWE0SYNC_BE                    0x0100231b  // DGN class sync command
#define RVCDCSRCTWE0INST_BE                    0x0200231b  // DC instance
#define RVCDCSRCTWE0PRIO_BE                    0x0300231b  // Device priority
#define RVCDCSRCTWE0CYCLES_BE                  0x0400231b  // Cycles
#define RVCDCSRCTWE0DEEP_BE                    0x0500231b  // Deepest discharge depth
#define RVCDCSRCTWE0AVERAGE_BE                 0x0600231b  // Average discharge depth
#define RVCDCSRCTHI0_BE                        0x0000241b  // RVC DGN DC Source Status 13 (130535) class
#define RVCDCSRCTHI0AVL_BE                     0x0000241b  // Available
#define RVCDCSRCTHI0SYNC_BE                    0x0100241b  // DGN class sync command
#define RVCDCSRCTHI0INST_BE                    0x0200241b  // DC instance
#define RVCDCSRCTHI0PRIO_BE                    0x0300241b  // Device priority
#define RVCDCSRCTHI0LOWVOLT_BE                 0x0400241b  // Lowest DC source voltage
#define RVCDCSRCTHI0HIGHVOLT_BE                0x0500241b  // Highest DC source voltage
#define RVCDCSRCCMD0_BE                        0x0000251b  // RVC DGN DC Source Command (130724) class
#define RVCDCSRCCMD0AVL_BE                     0x0000251b  // Available
#define RVCDCSRCCMD0SYNC_BE                    0x0100251b  // DGN class sync command
#define RVCDCSRCCMD0INST_BE                    0x0200251b  // DC instance
#define RVCDCSRCCMD0PWRST_BE                   0x0300251b  // Desired Power On/Off status
#define RVCDCSRCCMD0CHGST_BE                   0x0400251b  // Desired Charge On/Off status
#define RVCDCSRCCFG0_BE                        0x0000261b  // RVC DGN DC Source Configuration (Status/Command) (130551/130550) class
#define RVCDCSRCCFG0AVL_BE                     0x0000261b  // Available
#define RVCDCSRCCFG0SYNC_BE                    0x0100261b  // DGN class sync command
#define RVCDCSRCCFG0INST_BE                    0x0200261b  // DC instance
#define RVCDCSRCCFG0EXP_BE                     0x0300261b  // Peukert exponent
#define RVCDCSRCCFG0COEFF_BE                   0x0400261b  // Temperature coefficient
#define RVCDCSRCCFG0FACT_BE                    0x0500261b  // Charge efficiency factor
#define RVCDCSRCCFG0PERIOD_BE                  0x0600261b  // Time remaining averaging period
#define RVCDCSRCCFG0CAPACITY_BE                0x0700261b  // Full capacity
#define RVCDCSRCCFG0TAILCURR_BE                0x0800261b  // Tail current
#define RVCDCSRCCFGTWO0_BE                     0x0000271b  // RVC DGN DC Source Configuration 2 (Status/Command ) (130549/130548) class
#define RVCDCSRCCFGTWO0AVL_BE                  0x0000271b  // Available
#define RVCDCSRCCFGTWO0SYNC_BE                 0x0100271b  // DGN class sync command
#define RVCDCSRCCFGTWO0INST_BE                 0x0200271b  // DC instance
#define RVCDCSRCCFGTWO0CLRHIST_BE              0x0300271b  // Clear history
#define RVCDCSRCCFGTWO0SETCAP_BE               0x0400271b  // Set capacity to 100%
#define RVCDCSRCCFGTWO0CHGVOLT_BE              0x0500271b  // Charged voltage
#define RVCDCSRCCFGTWO0SHTVOLT_BE              0x0600271b  // Shunt voltage
#define RVCDCSRCCFGTWO0SHTCURR_BE              0x0700271b  // Shunt current
#define RVCDCSRCCFGTWO0RSTBATH_BE              0x0800271b  // Reset battery health to 100% 
#define RVCDCSRCCFGTWO0BATTYPE_BE              0x0900271b  // Sets the battery type
#define RVCDCSRCCONN0_BE                       0x0000281b  // RVC DGN DC Source Connection Status (130512) class
#define RVCDCSRCCONN0AVL_BE                    0x0000281b  // Available
#define RVCDCSRCCONN0SYNC_BE                   0x0100281b  // DGN class sync command
#define RVCDCSRCCONN0INST_BE                   0x0200281b  // Device instance
#define RVCDCSRCCONN0DSA_BE                    0x0300281b  // Device DSA (Default Source Address)
#define RVCDCSRCCONN0FUNC_BE                   0x0400281b  // Function
#define RVCDCSRCCONN0PRIM_BE                   0x0500281b  // Primary DC instance
#define RVCDCSRCCONN0SEC_BE                    0x0600281b  // Secondary DC instance
#define RVCDCSRCCFGTHR0_BE                     0x0000291b  // RVC DGN DC Source Configuration 3 Command (130526) class
#define RVCDCSRCCFGTHR0AVL_BE                  0x0000291b  // Available
#define RVCDCSRCCFGTHR0SYNC_BE                 0x0100291b  // DGN class sync command
#define RVCDCSRCCFGTHR0INST_BE                 0x0200291b  // Device instance
#define RVCDCSRCCFGTHR0DSA_BE                  0x0300291b  // Device DSA (Default Source Address)
#define RVCDCSRCCFGTHR0FUNC_BE                 0x0400291b  // Function
#define RVCDCSRCCFGTHR0PRIM_BE                 0x0500291b  // Primary DC instance
#define RVCDCSRCCFGTHR0SEC_BE                  0x0600291b  // Secondary DC instance
#define RVCDCDISCONN0_BE                       0x00002a1b  // RVC DGN DC Disconnection (Status/Command) (130768/130767) class
#define RVCDCDISCONN0AVL_BE                    0x00002a1b  // Available
#define RVCDCDISCONN0SYNC_BE                   0x01002a1b  // DGN class sync command
#define RVCDCDISCONN0INST_BE                   0x02002a1b  // Device instance
#define RVCDCDISCONN0STS_BE                    0x03002a1b  // Circuit Status
#define RVCDCDISCONN0CMD_BE                    0x04002a1b  // Command/Last command
#define RVCDCDISCONN0BYPASS_BE                 0x05002a1b  // Bypass Detect
#define RVCDCDISCONN0VOLT_BE                   0x06002a1b  // DC switched voltage
#define RVCDCDISCONN0CURR_BE                   0x07002a1b  // DC switched current
#define RVCBAT0_BE                             0x00002b1b  // RVC DGN Battery Status 1 (130709) class
#define RVCBAT0AVL_BE                          0x00002b1b  // Available
#define RVCBAT0SYNC_BE                         0x01002b1b  // DGN class sync command
#define RVCBAT0INST_BE                         0x02002b1b  // Battery instance
#define RVCBAT0DC_BE                           0x03002b1b  // DC instance
#define RVCBAT0VOLT_BE                         0x04002b1b  // DC voltage
#define RVCBAT0CURR_BE                         0x05002b1b  // DC current
#define RVCBATTWO0_BE                          0x00002c1b  // RVC DGN Battery Status 2 (130708) class
#define RVCBATTWO0AVL_BE                       0x00002c1b  // Available
#define RVCBATTWO0SYNC_BE                      0x01002c1b  // DGN class sync command
#define RVCBATTWO0INST_BE                      0x02002c1b  // Battery instance
#define RVCBATTWO0DC_BE                        0x03002c1b  // DC instance
#define RVCBATTWO0TEMP_BE                      0x04002c1b  // Source temperature
#define RVCBATTWO0SOC_BE                       0x05002c1b  // State of charge (SOC)
#define RVCBATTWO0TIME_BE                      0x06002c1b  // Time remaining
#define RVCBATTWO0TIMESTS_BE                   0x07002c1b  // Time remaining interpretation
#define RVCBATTHR0_BE                          0x00002d1b  // RVC DGN Battery Status 3 (130707) class
#define RVCBATTHR0AVL_BE                       0x00002d1b  // Available
#define RVCBATTHR0SYNC_BE                      0x01002d1b  // DGN class sync command
#define RVCBATTHR0INST_BE                      0x02002d1b  // Battery instance
#define RVCBATTHR0DC_BE                        0x03002d1b  // DC instance
#define RVCBATTHR0SOH_BE                       0x04002d1b  // State of health
#define RVCBATTHR0CAP_BE                       0x05002d1b  // Capacity remaining
#define RVCBATTHR0CAPREL_BE                    0x06002d1b  // Relative capacity
#define RVCBATTHR0RIPPLE_BE                    0x07002d1b  // AC RMS Ripple
#define RVCBATFOUR0_BE                         0x00002e1b  // RVC DGN Battery Status 4 (130706) class
#define RVCBATFOUR0AVL_BE                      0x00002e1b  // Available
#define RVCBATFOUR0SYNC_BE                     0x01002e1b  // DGN class sync command
#define RVCBATFOUR0INST_BE                     0x02002e1b  // Battery instance
#define RVCBATFOUR0DC_BE                       0x03002e1b  // DC instance
#define RVCBATFOUR0CHGST_BE                    0x04002e1b  // Desired charge state
#define RVCBATFOUR0VOLT_BE                     0x05002e1b  // Desired DC voltage
#define RVCBATFOUR0CURR_BE                     0x06002e1b  // Desired DC current
#define RVCBATFOUR0TYPE_BE                     0x07002e1b  // Battery Type
#define RVCBATSIX0_BE                          0x00002f1b  // RVC DGN Battery Status 6 (130704) class
#define RVCBATSIX0AVL_BE                       0x00002f1b  // Available
#define RVCBATSIX0SYNC_BE                      0x01002f1b  // DGN class sync command
#define RVCBATSIX0INST_BE                      0x02002f1b  // Battery instance
#define RVCBATSIX0DC_BE                        0x03002f1b  // DC instance
#define RVCBATSIX0HVLIM_BE                     0x04002f1b  // High Voltage Limit Status
#define RVCBATSIX0HVDIS_BE                     0x05002f1b  // High Voltage Disconnect Status
#define RVCBATSIX0LVLIM_BE                     0x06002f1b  // Low Voltage Limit Status
#define RVCBATSIX0LVDIS_BE                     0x07002f1b  // Low Voltage Disconnect Status
#define RVCBATSIX0LSOCLIM_BE                   0x08002f1b  // Low state of charge (SOC) limit
#define RVCBATSIX0LSOCDIS_BE                   0x09002f1b  // Low state of charge disconnect status
#define RVCBATSIX0LDCTEMPLIM_BE                0x0a002f1b  // Low DC source temperature limit status
#define RVCBATSIX0LDCTEMPDIS_BE                0x0b002f1b  // Low DC source temperature disconnect status
#define RVCBATSIX0HDCTEMPLIM_BE                0x0c002f1b  // High DC source temperature limit status
#define RVCBATSIX0HDCTEMPDIS_BE                0x0d002f1b  // High DC source temperature disconnect status
#define RVCBATSIX0HCURRLIM_BE                  0x0e002f1b  // High Current DC Source Limit
#define RVCBATSIX0HCURRDIS_BE                  0x0f002f1b  // High Current DC Source Disconnect
#define RVCBATSEV0_BE                          0x0000301b  // RVC DGN Battery Status 7 (130703) class
#define RVCBATSEV0AVL_BE                       0x0000301b  // Available
#define RVCBATSEV0SYNC_BE                      0x0100301b  // DGN class sync command
#define RVCBATSEV0INST_BE                      0x0200301b  // Battery instance
#define RVCBATSEV0DC_BE                        0x0300301b  // DC instance
#define RVCBATSEV0INPUT_BE                     0x0400301b  // Today's Input AmpereHours
#define RVCBATSEV0OUTPUT_BE                    0x0500301b  // Today's Output AmpereHours
#define RVCBATEIG0_BE                          0x0000311b  // RVC DGN Battery Status 8 (130702) class
#define RVCBATEIG0AVL_BE                       0x0000311b  // Available
#define RVCBATEIG0SYNC_BE                      0x0100311b  // DGN class sync command
#define RVCBATEIG0INST_BE                      0x0200311b  // Battery instance
#define RVCBATEIG0DC_BE                        0x0300311b  // DC instance
#define RVCBATEIG0INPUT_BE                     0x0400311b  // Yesterday's Input AmpereHours
#define RVCBATEIG0OUTPUT_BE                    0x0500311b  // Yesterday's Output AmpereHours
#define RVCBATNINE0_BE                         0x0000321b  // RVC DGN Battery Status 9 (130701) class
#define RVCBATNINE0AVL_BE                      0x0000321b  // Available
#define RVCBATNINE0SYNC_BE                     0x0100321b  // DGN class sync command
#define RVCBATNINE0INST_BE                     0x0200321b  // Battery instance
#define RVCBATNINE0DC_BE                       0x0300321b  // DC instance
#define RVCBATNINE0INPUT_BE                    0x0400321b  // Day Before Yesterday's Input AmpereHours
#define RVCBATNINE0OUTPUT_BE                   0x0500321b  // Day Before Yesterday's Output AmpereHours
#define RVCBATTEN0_BE                          0x0000331b  // RVC DGN Battery Status 10 (130700) class
#define RVCBATTEN0AVL_BE                       0x0000331b  // Available
#define RVCBATTEN0SYNC_BE                      0x0100331b  // DGN class sync command
#define RVCBATTEN0INST_BE                      0x0200331b  // Battery instance
#define RVCBATTEN0DC_BE                        0x0300331b  // DC instance
#define RVCBATTEN0INPUT_BE                     0x0400331b  // Last 7 Days Input AmpereHours
#define RVCBATTEN0OUTPUT_BE                    0x0500331b  // Last 7 Days Output AmpereHours
#define RVCBATELE0_BE                          0x0000341b  // RVC DGN Battery Status 11 (130699) class
#define RVCBATELE0AVL_BE                       0x0000341b  // Available
#define RVCBATELE0SYNC_BE                      0x0100341b  // DGN class sync command
#define RVCBATELE0INST_BE                      0x0200341b  // Battery instance
#define RVCBATELE0DC_BE                        0x0300341b  // DC instance
#define RVCBATELE0DISCHGST_BE                  0x0400341b  // Discharge On/Off status
#define RVCBATELE0CHGST_BE                     0x0500341b  // Charge On/Off status
#define RVCBATELE0CHGDET_BE                    0x0600341b  // Charge detected
#define RVCBATELE0RESST_BE                     0x0700341b  // Reserve status
#define RVCBATELE0CAPACITY_BE                  0x0800341b  // Full capacity
#define RVCBATELE0POWER_BE                     0x0900341b  // DC power
#define RVCBATTWE0_BE                          0x0000351b  // RVC DGN Battery Status 12 (130547) class
#define RVCBATTWE0AVL_BE                       0x0000351b  // Available
#define RVCBATTWE0SYNC_BE                      0x0100351b  // DGN class sync command
#define RVCBATTWE0INST_BE                      0x0200351b  // Battery instance
#define RVCBATTWE0DC_BE                        0x0300351b  // DC instance
#define RVCBATTWE0CYCLES_BE                    0x0400351b  // Cycles
#define RVCBATTWE0DEEP_BE                      0x0500351b  // Deepest discharge depth
#define RVCBATTWE0AVERAGE_BE                   0x0600351b  // Average discharge depth
#define RVCBATTHI0_BE                          0x0000361b  // RVC DGN Battery Status 13 (130546) class
#define RVCBATTHI0AVL_BE                       0x0000361b  // Available
#define RVCBATTHI0SYNC_BE                      0x0100361b  // DGN class sync command
#define RVCBATTHI0INST_BE                      0x0200361b  // Battery instance
#define RVCBATTHI0DC_BE                        0x0300361b  // DC instance
#define RVCBATTHI0LOWVOLT_BE                   0x0400361b  // Lowest Battery voltage
#define RVCBATTHI0HIGHVOLT_BE                  0x0500361b  // Highest Battery voltage
#define RVCBATCMD0_BE                          0x0000371b  // RVC DGN Battery Command (130698) class
#define RVCBATCMD0AVL_BE                       0x0000371b  // Available
#define RVCBATCMD0SYNC_BE                      0x0100371b  // DGN class sync command
#define RVCBATCMD0INST_BE                      0x0200371b  // Battery instance
#define RVCBATCMD0DC_BE                        0x0300371b  // Set DC instance
#define RVCBATCMD0LOADST_BE                    0x0400371b  // Desired Load On/Off status
#define RVCBATCMD0CHGST_BE                     0x0500371b  // Desired Charge On/Off status
#define RVCBATCMD0HIST_BE                      0x0600371b  // Clear History
#define RVCBATCMD0DETAILS_BE                   0x0700371b  // Return module’s Cell details
#define RVCBATSUM0_BE                          0x0000381b  // RVC DGN Battery Summary (130545) class
#define RVCBATSUM0AVL_BE                       0x0000381b  // Available
#define RVCBATSUM0SYNC_BE                      0x0100381b  // DGN class sync command
#define RVCBATSUM0INST_BE                      0x0200381b  // Battery instance
#define RVCBATSUM0DC_BE                        0x0300381b  // DC instance
#define RVCBATSUM0SERIES_BE                    0x0400381b  // Series string
#define RVCBATSUM0MODCNT_BE                    0x0500381b  // Module count
#define RVCBATSUM0CELLS_BE                     0x0600381b  // Cells per module
#define RVCBATSUM0VOLTSTS_BE                   0x0700381b  // Voltage Status
#define RVCBATSUM0TEMPSTS_BE                   0x0800381b  // Temperature Status
#define RVCBATSUM0ADDR_BE                      0x0900381b  // Address
#define RVCBATCELL0_BE                         0x0000391b  // RVC DGN Cell Details (130525) class
#define RVCBATCELL0AVL_BE                      0x0000391b  // Available
#define RVCBATCELL0SYNC_BE                     0x0100391b  // DGN class sync command
#define RVCBATCELL0INST_BE                     0x0200391b  // Battery instance
#define RVCBATCELL0MOD_BE                      0x0300391b  // Module instance
#define RVCBATCELL0CELL_BE                     0x0400391b  // Cell instance
#define RVCBATCELL0VOLTSTS_BE                  0x0500391b  // Voltage status
#define RVCBATCELL0TEMPSTS_BE                  0x0600391b  // Voltage status
#define RVCBATCELL0BAL_BE                      0x0700391b  // Balancing
#define RVCBATCELL0VOLT_BE                     0x0800391b  // Voltage
#define RVCBATCELL0TEMP_BE                     0x0900391b  // Temperature
#define RVCSOLAR0_BE                           0x00003a1b  // RVC DGN Solar Controller (Status/Command) (130739/130737) class
#define RVCSOLAR0AVL_BE                        0x00003a1b  // Available
#define RVCSOLAR0SYNC_BE                       0x01003a1b  // DGN class sync command
#define RVCSOLAR0INST_BE                       0x02003a1b  // Solar controller instance
#define RVCSOLAR0VOLT_BE                       0x03003a1b  // Desired charge control voltage
#define RVCSOLAR0CURR_BE                       0x04003a1b  // Desired charge control current
#define RVCSOLAR0CURRPERC_BE                   0x05003a1b  // Charge current percent of maximum
#define RVCSOLAR0OPER_BE                       0x06003a1b  // Operating state
#define RVCSOLAR0POWUP_BE                      0x07003a1b  // Power-up state
#define RVCSOLAR0CLRHIST_BE                    0x08003a1b  // Clear history
#define RVCSOLAR0FORCE_BE                      0x09003a1b  // Force charge
#define RVCSOLARTWO0_BE                        0x00003b1b  // RVC DGN Solar Controller 2 Status (130693) class
#define RVCSOLARTWO0AVL_BE                     0x00003b1b  // Available
#define RVCSOLARTWO0SYNC_BE                    0x01003b1b  // DGN class sync command
#define RVCSOLARTWO0INST_BE                    0x02003b1b  // Solar controller instance
#define RVCSOLARTWO0VOLT_BE                    0x03003b1b  // Rated battery voltage
#define RVCSOLARTWO0CURR_BE                    0x04003b1b  // Rated charging current
#define RVCSOLARTHR0_BE                        0x00003c1b  // RVC DGN Solar Controller 3 Status (130692) class
#define RVCSOLARTHR0AVL_BE                     0x00003c1b  // Available
#define RVCSOLARTHR0SYNC_BE                    0x01003c1b  // DGN class sync command
#define RVCSOLARTHR0INST_BE                    0x02003c1b  // Solar controller instance
#define RVCSOLARTHR0VOLT_BE                    0x03003c1b  // Rated solar input voltage
#define RVCSOLARTHR0CURR_BE                    0x04003c1b  // Rated solar input current
#define RVCSOLARTHR0OPOW_BE                    0x05003c1b  // Rated solar over-power
#define RVCSOLARFOUR0_BE                       0x00003d1b  // RVC DGN Solar Controller 4 Status (130691) class
#define RVCSOLARFOUR0AVL_BE                    0x00003d1b  // Available
#define RVCSOLARFOUR0SYNC_BE                   0x01003d1b  // DGN class sync command
#define RVCSOLARFOUR0INST_BE                   0x02003d1b  // Solar controller instance
#define RVCSOLARFOUR0TODAY_BE                  0x03003d1b  // Today's amp-hours to battery
#define RVCSOLARFOUR0YESTERDAY_BE              0x04003d1b  // Yesterday's amp-hours to battery
#define RVCSOLARFOUR0YESTERDAY2_BE             0x05003d1b  // Day before yesterday's amphours to battery
#define RVCSOLARFIVE0_BE                       0x00003e1b  // RVC DGN Solar Controller 5 Status (130690) class
#define RVCSOLARFIVE0AVL_BE                    0x00003e1b  // Available
#define RVCSOLARFIVE0SYNC_BE                   0x01003e1b  // DGN class sync command
#define RVCSOLARFIVE0INST_BE                   0x02003e1b  // Solar controller instance
#define RVCSOLARFIVE0DAYS7_BE                  0x03003e1b  // Last 7 days amp-hours to battery
#define RVCSOLARFIVE0POWER_BE                  0x04003e1b  // Cumulative power generation
#define RVCSOLARSIX0_BE                        0x00003f1b  // RVC DGN Solar Controller 6 Status (130689) class
#define RVCSOLARSIX0AVL_BE                     0x00003f1b  // Available
#define RVCSOLARSIX0SYNC_BE                    0x01003f1b  // DGN class sync command
#define RVCSOLARSIX0INST_BE                    0x02003f1b  // Solar controller instance
#define RVCSOLARSIX0DAYS_BE                    0x03003f1b  // Total number of operating days
#define RVCSOLARSIX0TEMP_BE                    0x04003f1b  // Solar charge controller measured temperature
#define RVCSOLARBAT0_BE                        0x0000401b  // RVC DGN Solar Charge Controller Battery Status (130688) class
#define RVCSOLARBAT0AVL_BE                     0x0000401b  // Available
#define RVCSOLARBAT0SYNC_BE                    0x0100401b  // DGN class sync command
#define RVCSOLARBAT0INST_BE                    0x0200401b  // Solar controller instance
#define RVCSOLARBAT0DC_BE                      0x0300401b  // DC Source Instance
#define RVCSOLARBAT0PRIO_BE                    0x0400401b  // Charger Priority
#define RVCSOLARBAT0VOLT_BE                    0x0500401b  // Measured voltage
#define RVCSOLARBAT0CURR_BE                    0x0600401b  // Measured current
#define RVCSOLARBAT0TEMP_BE                    0x0700401b  // Measured temperature
#define RVCSOLARARR0_BE                        0x0000411b  // RVC DGN Solar Charge Controller Solar Array Status (130559) class
#define RVCSOLARARR0AVL_BE                     0x0000411b  // Available
#define RVCSOLARARR0SYNC_BE                    0x0100411b  // DGN class sync command
#define RVCSOLARARR0INST_BE                    0x0200411b  // Solar controller instance
#define RVCSOLARARR0VOLT_BE                    0x0300411b  // Solar array measured voltage
#define RVCSOLARARR0CURR_BE                    0x0400411b  // Solar array measured input current
#define RVCSOLARCFG0_BE                        0x0000421b  // RVC DGN Solar Controller Configuration (Status/Command) (130738/130736) class
#define RVCSOLARCFG0AVL_BE                     0x0000421b  // Available
#define RVCSOLARCFG0SYNC_BE                    0x0100421b  // DGN class sync command
#define RVCSOLARCFG0INST_BE                    0x0200421b  // Solar controller instance
#define RVCSOLARCFG0ALGO_BE                    0x0300421b  // Charging algorithm
#define RVCSOLARCFG0MODE_BE                    0x0400421b  // Controller mode
#define RVCSOLARCFG0SENSOR_BE                  0x0500421b  // Battery sensor present
#define RVCSOLARCFG0LINK_BE                    0x0600421b  // Linkage mode
#define RVCSOLARCFG0TYPE_BE                    0x0700421b  // Battery type
#define RVCSOLARCFG0SIZE_BE                    0x0800421b  // Battery bank size
#define RVCSOLARCFG0MAXCURR_BE                 0x0900421b  // Maximum charging current
#define RVCSOLARCFGTWO0_BE                     0x0000431b  // RVC DGN Solar Controller Configuration 2 (Status/Command) (130558/130557) class
#define RVCSOLARCFGTWO0AVL_BE                  0x0000431b  // Available
#define RVCSOLARCFGTWO0SYNC_BE                 0x0100431b  // DGN class sync command
#define RVCSOLARCFGTWO0INST_BE                 0x0200431b  // Solar controller instance
#define RVCSOLARCFGTWO0BVOLT_BE                0x0300431b  // Bulk-absorption voltage
#define RVCSOLARCFGTWO0FVOLT_BE                0x0400431b  // Float voltage
#define RVCSOLARCFGTWO0CVOLT_BE                0x0500431b  // Charge return voltage
#define RVCSOLARCFGTHR0_BE                     0x0000441b  // RVC DGN Solar Controller Configuration 3 (Status/Command) (130556/130555) class
#define RVCSOLARCFGTHR0AVL_BE                  0x0000441b  // Available
#define RVCSOLARCFGTHR0SYNC_BE                 0x0100441b  // DGN class sync command
#define RVCSOLARCFGTHR0INST_BE                 0x0200441b  // Solar controller instance
#define RVCSOLARCFGTHR0UVWLIM_BE               0x0300441b  // Under-voltage warning voltage
#define RVCSOLARCFGTHR0HVLIM_BE                0x0400441b  // Battery high voltage limit voltage
#define RVCSOLARCFGTHR0LVLIM_BE                0x0500441b  // Battery low voltage limit voltage
#define RVCSOLARCFGFOUR0_BE                    0x0000451b  // RVC DGN Solar Controller Configuration 4 (Status/Command) (130554/130553) class
#define RVCSOLARCFGFOUR0AVL_BE                 0x0000451b  // Available
#define RVCSOLARCFGFOUR0SYNC_BE                0x0100451b  // DGN class sync command
#define RVCSOLARCFGFOUR0INST_BE                0x0200451b  // Solar controller instance
#define RVCSOLARCFGFOUR0HVLIMR_BE              0x0300451b  // Battery high voltage limit return voltage
#define RVCSOLARCFGFOUR0LVLIMR_BE              0x0400451b  // Battery low voltage limit return voltage
#define RVCSOLARCFGFOUR0LVLIMD_BE              0x0500451b  // Battery low voltage limit time delay
#define RVCSOLARCFGFOUR0DUR_BE                 0x0600451b  // Absorption duration
#define RVCSOLARCFGFOUR0FACTOR_BE              0x0700451b  // Temperature compensation factor
#define RVCSOLARCFGFIVE0_BE                    0x0000461b  // RVC DGN Solar Controller Configuration 5 (Status/Command) (130511/130510) class
#define RVCSOLARCFGFIVE0AVL_BE                 0x0000461b  // Available
#define RVCSOLARCFGFIVE0SYNC_BE                0x0100461b  // DGN class sync command
#define RVCSOLARCFGFIVE0INST_BE                0x0200461b  // Solar controller instance
#define RVCSOLARCFGFIVE0PRIO_BE                0x0300461b  // Charger Priority
#define RVCSOLARCFGFIVE0ETEMPHLIM_BE           0x0400461b  // External temperature sensor high temperature limit
#define RVCSOLARCFGFIVE0ETEMPLLIM_BE           0x0500461b  // External temperature sensor low temperature limit
#define RVCSOLAREQ0_BE                         0x0000471b  // RVC DGN Solar Equalization Status (130735) class
#define RVCSOLAREQ0AVL_BE                      0x0000471b  // Available
#define RVCSOLAREQ0SYNC_BE                     0x0100471b  // DGN class sync command
#define RVCSOLAREQ0INST_BE                     0x0200471b  // Solar controller instance
#define RVCSOLAREQ0REM_BE                      0x0300471b  // Time remaining
#define RVCSOLAREQ0PRECHG_BE                   0x0400471b  // Pre-charging status
#define RVCSOLAREQ0LAST_BE                     0x0500471b  // Time since last equalization
#define RVCSOLAREQCFG0_BE                      0x0000481b  // RVC DGN Solar Equalization Configuration (Status/Command) (130734/130733) class
#define RVCSOLAREQCFG0AVL_BE                   0x0000481b  // Available
#define RVCSOLAREQCFG0SYNC_BE                  0x0100481b  // DGN class sync command
#define RVCSOLAREQCFG0INST_BE                  0x0200481b  // Solar controller instance
#define RVCSOLAREQCFG0VOLT_BE                  0x0300481b  // Equalization voltage
#define RVCSOLAREQCFG0TIME_BE                  0x0400481b  // Equalization time
#define RVCSOLAREQCFG0INT_BE                   0x0500481b  // Equalization interval (days)
#define RVCREFRIG0_BE                          0x0000491b  // RVC DGN Refrigerator (Status/Command) (130515/130514) class
#define RVCREFRIG0AVL_BE                       0x0000491b  // Available
#define RVCREFRIG0SYNC_BE                      0x0100491b  // DGN class sync command
#define RVCREFRIG0INST_BE                      0x0200491b  // Refrigerator instance
#define RVCREFRIG0CAV_BE                       0x0300491b  // Cavity
#define RVCREFRIG0LGT_BE                       0x0400491b  // Light
#define RVCREFRIG0DOOR_BE                      0x0500491b  // Door switch
#define RVCREFRIG0CTEMP_BE                     0x0600491b  // Current temperature
#define RVCREFRIG0SETTEMP_BE                   0x0700491b  // Set temperature
#define RVCREFRIG0FUEL_BE                      0x0800491b  // Fuel source
#define RVCREFRIG0MODE_BE                      0x0900491b  // Refrigerator mode
#define RVCREFRIG0COMPSPD_BE                   0x0a00491b  // Compressor speed
#define RVCDM0_BE                              0x00004a1b  // RVC DGN Diagnostic Message (130762) class
#define RVCDM0AVL_BE                           0x00004a1b  // Available
#define RVCDM0SYNC_BE                          0x01004a1b  // DGN class sync command
#define RVCDM0OPST_BE                          0x02004a1b  // Operating status
#define RVCDM0YLAMPST_BE                       0x03004a1b  // Yellow lamp status (fault status)
#define RVCDM0RLAMPST_BE                       0x04004a1b  // Red lamp status (fault status)
#define RVCDM0DSA_BE                           0x05004a1b  // Default Source Address
#define RVCDM0SPN_BE                           0x06004a1b  // Service Point Number
#define RVCDM0FMI_BE                           0x07004a1b  // Failure Mode Identifier
#define RVCDM0OCNT_BE                          0x08004a1b  // Occurrence count
#define RVCDM0DSAEXT_BE                        0x09004a1b  // DSA extenstion, 0xFF if no extension defined
#define RVCDM0BANKSEL_BE                       0x0a004a1b  // Bank select
#define RVCGENCFG0_BE                          0x00004b1b  // RVC DGN Generic Configuration Status (130776) class
#define RVCGENCFG0AVL_BE                       0x00004b1b  // Available
#define RVCGENCFG0SYNC_BE                      0x01004b1b  // DGN class sync command
#define RVCGENCFG0MC_BE                        0x02004b1b  // Manufacturing code
#define RVCGENCFG0FUNCINST_BE                  0x03004b1b  // Function instance
#define RVCGENCFG0FUNC_BE                      0x04004b1b  // Function
#define RVCGENCFG0FWREV_BE                     0x05004b1b  // Firmware revision
#define RVCGENCFG0CONFTYPE_BE                  0x06004b1b  // Configuration type
#define RVCGENCFG0CONFREV_BE                   0x07004b1b  // Configuration revision
#define RVCGENCFG0ADDR_BE                      0x08004b1b  // Source address
#define RVCGENRST0_BE                          0x00004c1b  // RVC DGN General Purpose Reset (98048) class
#define RVCGENRST0AVL_BE                       0x00004c1b  // Available
#define RVCGENRST0SYNC_BE                      0x01004c1b  // DGN class sync command
#define RVCGENRST0REBOOT_BE                    0x02004c1b  // Reboot
#define RVCGENRST0CLRFAULTS_BE                 0x03004c1b  // Clear faults
#define RVCGENRST0RSTDEFAULT_BE                0x04004c1b  // Reset to default settings
#define RVCGENRST0RSTSTAT_BE                   0x05004c1b  // Reset statistics
#define RVCGENRST0TESTMODE_BE                  0x06004c1b  // Test mode
#define RVCGENRST0RSTOEM_BE                    0x07004c1b  // Reset to OEM-pecific settings
#define RVCGENRST0REBOOTBL_BE                  0x08004c1b  // Reboot/Enter bootloader mode
#define RVCCHRG0_BE                            0x00004d1b  // RVC DGN Charger Status/Command (131015/131013) class
#define RVCCHRG0AVL_BE                         0x00004d1b  // Available
#define RVCCHRG0SYNC_BE                        0x01004d1b  // DGN class sync command
#define RVCCHRG0INST_BE                        0x02004d1b  // Charger instance
#define RVCCHRG0VOLT_BE                        0x03004d1b  // Charger voltage
#define RVCCHRG0CURR_BE                        0x04004d1b  // Charger current
#define RVCCHRG0CURRMAX_BE                     0x05004d1b  // Charge current percent of maximum
#define RVCCHRG0OPERST_BE                      0x06004d1b  // Operating state
#define RVCCHRG0DEFST_BE                       0x07004d1b  // Default state on power- up
#define RVCCHRG0AUTO_BE                        0x08004d1b  // Auto recharge enable
#define RVCCHRG0FORCE_BE                       0x09004d1b  // Force charge
#define RVCCHRGTWO0_BE                         0x00004e1b  // RVC DGN Charger Status 2 (130723) class
#define RVCCHRGTWO0AVL_BE                      0x00004e1b  // Available
#define RVCCHRGTWO0SYNC_BE                     0x01004e1b  // DGN class sync command
#define RVCCHRGTWO0INST_BE                     0x02004e1b  // Charger instance
#define RVCCHRGTWO0PRIO_BE                     0x03004e1b  // Charger priority
#define RVCCHRGTWO0VOLT_BE                     0x04004e1b  // Charger voltage
#define RVCCHRGTWO0CURR_BE                     0x05004e1b  // Charger current
#define RVCCHRGTWO0TEMP_BE                     0x06004e1b  // Charger temperature
#define RVCCHRGTHREE0_BE                       0x00004f1b  // RVC Charger Status 3 (130506) class
#define RVCCHRGTHREE0AVL_BE                    0x00004f1b  // Available
#define RVCCHRGTHREE0SYNC_BE                   0x01004f1b  // DGN class sync command
#define RVCCHRGTHREE0INST_BE                   0x02004f1b  // Charger instance
#define RVCCHRGTHREE0DERST_BE                  0x03004f1b  // Derating status
#define RVCCHRGTHREE0DERREAS_BE                0x04004f1b  // Derating reason
#define RVCCHRGCFG0_BE                         0x0000501b  // RVC DGN Charger Configuration Status/Command (131014/131012) class
#define RVCCHRGCFG0AVL_BE                      0x0000501b  // Available
#define RVCCHRGCFG0SYNC_BE                     0x0100501b  // DGN class sync command
#define RVCCHRGCFG0INST_BE                     0x0200501b  // Charger instance
#define RVCCHRGCFG0ALGO_BE                     0x0300501b  // Charging algorithm
#define RVCCHRGCFG0MODE_BE                     0x0400501b  // Charging mode
#define RVCCHRGCFG0BATSENSOR_BE                0x0500501b  // Battery sensor present
#define RVCCHRGCFG0LINE_BE                     0x0600501b  // Charger installation line
#define RVCCHRGCFG0TYPE_BE                     0x0700501b  // Battery type
#define RVCCHRGCFG0BANK_BE                     0x0800501b  // Battery bank size
#define RVCCHRGCFG0MAXCURR_BE                  0x0900501b  // Maximum charging current
#define RVCCHRGCFGTWO0_BE                      0x0000511b  // RVC Charger Configuration 2 Status/Command (130966/130965) class
#define RVCCHRGCFGTWO0AVL_BE                   0x0000511b  // Available
#define RVCCHRGCFGTWO0SYNC_BE                  0x0100511b  // DGN class sync command
#define RVCCHRGCFGTWO0INST_BE                  0x0200511b  // Charger instance
#define RVCCHRGCFGTWO0MAXCURR_BE               0x0300511b  // Maximum charge current as percent
#define RVCCHRGCFGTWO0RATELIMIT_BE             0x0400511b  // Charge rate limit as percent of bank size
#define RVCCHRGCFGTWO0BREAKER_BE               0x0500511b  // Shore breaker size
#define RVCCHRGCFGTWO0BATTEMP_BE               0x0600511b  // Default battery temperature
#define RVCCHRGCFGTWO0RECHARGEVOLT_BE          0x0700511b  // Recharge voltage
#define RVCCHRGCFGTHREE0_BE                    0x0000521b  // RVC DGN Charger Configuration 3 Status/Command (130764/130763) class
#define RVCCHRGCFGTHREE0AVL_BE                 0x0000521b  // Available
#define RVCCHRGCFGTHREE0SYNC_BE                0x0100521b  // DGN class sync command
#define RVCCHRGCFGTHREE0INST_BE                0x0200521b  // Charger instance
#define RVCCHRGCFGTHREE0BVOLT_BE               0x0300521b  // Bulk voltage
#define RVCCHRGCFGTHREE0AVOLT_BE               0x0400521b  // Absorption voltage
#define RVCCHRGCFGTHREE0FVOLT_BE               0x0500521b  // Float voltage
#define RVCCHRGCFGTHREE0TEMPCOMP_BE            0x0600521b  // Temperature compensation constant (mV/K)
#define RVCCHRGCFGFOUR0_BE                     0x0000531b  // RVC DGN Charger Configuration 4 Status/Command (130751/130750) class
#define RVCCHRGCFGFOUR0AVL_BE                  0x0000531b  // Available
#define RVCCHRGCFGFOUR0SYNC_BE                 0x0100531b  // DGN class sync command
#define RVCCHRGCFGFOUR0INST_BE                 0x0200531b  // Charger instance
#define RVCCHRGCFGFOUR0BTIME_BE                0x0300531b  // Bulk time
#define RVCCHRGCFGFOUR0ATIME_BE                0x0400531b  // Absorption time
#define RVCCHRGCFGFOUR0FTIME_BE                0x0500531b  // Float time
#define RVCCHRGEQ0_BE                          0x0000541b  // RVC DGN Charger Equalization Status (130969) class
#define RVCCHRGEQ0AVL_BE                       0x0000541b  // Available
#define RVCCHRGEQ0SYNC_BE                      0x0100541b  // DGN class sync command
#define RVCCHRGEQ0INST_BE                      0x0200541b  // Charger instance
#define RVCCHRGEQ0TIME_BE                      0x0300541b  // Time remaining
#define RVCCHRGEQ0PRECHGST_BE                  0x0400541b  // Pre-charging status
#define RVCCHRGEQCFG0_BE                       0x0000551b  // RVC DGN Charger Equalization Configuration Status/Command (130968/130967) class
#define RVCCHRGEQCFG0AVL_BE                    0x0000551b  // Available
#define RVCCHRGEQCFG0SYNC_BE                   0x0100551b  // DGN class sync command
#define RVCCHRGEQCFG0INST_BE                   0x0200551b  // Charger instance
#define RVCCHRGEQCFG0VOLT_BE                   0x0300551b  // Equalization voltage
#define RVCCHRGEQCFG0TIME_BE                   0x0400551b  // Equalization time
#define RVCCHRGAC0_BE                          0x0000561b  // RVC DGN Charger AC Status 1 (131018) class
#define RVCCHRGAC0AVL_BE                       0x0000561b  // Available
#define RVCCHRGAC0SYNC_BE                      0x0100561b  // DGN class sync command
#define RVCCHRGAC0INST_BE                      0x0200561b  // Charger instance
#define RVCCHRGAC0LINE_BE                      0x0300561b  // Line
#define RVCCHRGAC0IO_BE                        0x0400561b  // Input / Output
#define RVCCHRGAC0VOLT_BE                      0x0500561b  // RMS voltage
#define RVCCHRGAC0CURR_BE                      0x0600561b  // RMS current
#define RVCCHRGAC0FREQ_BE                      0x0700561b  // Frequency (Hz)
#define RVCCHRGAC0OPENGROUND_BE                0x0800561b  // Fault – open ground
#define RVCCHRGAC0OPENNEUTRAL_BE               0x0900561b  // Fault – open neutral
#define RVCCHRGAC0REVPOL_BE                    0x0a00561b  // Fault – reverse polarity
#define RVCCHRGAC0GROUNDCURR_BE                0x0b00561b  // Fault – ground current
#define RVCCHRGACTWO0_BE                       0x0000571b  // RVC DGN Charger AC Status 2 (131017) class
#define RVCCHRGACTWO0AVL_BE                    0x0000571b  // Available
#define RVCCHRGACTWO0SYNC_BE                   0x0100571b  // DGN class sync command
#define RVCCHRGACTWO0INST_BE                   0x0200571b  // Charger instance
#define RVCCHRGACTWO0LINE_BE                   0x0300571b  // Line
#define RVCCHRGACTWO0IO_BE                     0x0400571b  // Input / Output
#define RVCCHRGACTWO0PVOLT_BE                  0x0500571b  // Peak voltage
#define RVCCHRGACTWO0PCURR_BE                  0x0600571b  // Peak current
#define RVCCHRGACTWO0GCURR_BE                  0x0700571b  // Ground current
#define RVCCHRGACTWO0CAP_BE                    0x0800571b  // Capacity
#define RVCCHRGACTHREE0_BE                     0x0000581b  // RVC DGN Charger AC Status 3 (131016) class
#define RVCCHRGACTHREE0AVL_BE                  0x0000581b  // Available
#define RVCCHRGACTHREE0SYNC_BE                 0x0100581b  // DGN class sync command
#define RVCCHRGACTHREE0INST_BE                 0x0200581b  // Charger instance
#define RVCCHRGACTHREE0LINE_BE                 0x0300581b  // Line
#define RVCCHRGACTHREE0IO_BE                   0x0400581b  // Input / Output
#define RVCCHRGACTHREE0WAVEFORM_BE             0x0500581b  // Waveform
#define RVCCHRGACTHREE0PHASE_BE                0x0600581b  // Phase status
#define RVCCHRGACTHREE0REALPOW_BE              0x0700581b  // Real power
#define RVCCHRGACTHREE0REACTPOW_BE             0x0800581b  // Reactive power
#define RVCCHRGACTHREE0HARMDIST_BE             0x0900581b  // Harmonic distortion
#define RVCCHRGACTHREE0COMPLLEG_BE             0x0a00581b  // Complementary leg
#define RVCCHRGACFOUR0_BE                      0x0000591b  // RVC DGN Charger AC Status 4 (130954) class
#define RVCCHRGACFOUR0AVL_BE                   0x0000591b  // Available
#define RVCCHRGACFOUR0SYNC_BE                  0x0100591b  // DGN class sync command
#define RVCCHRGACFOUR0INST_BE                  0x0200591b  // Charger instance
#define RVCCHRGACFOUR0LINE_BE                  0x0300591b  // Line
#define RVCCHRGACFOUR0IO_BE                    0x0400591b  // Input / Output
#define RVCCHRGACFOUR0VOLTFAULT_BE             0x0500591b  // Voltage fault
#define RVCCHRGACFOUR0SURGEFAULT_BE            0x0600591b  // Fault – Surge protection
#define RVCCHRGACFOUR0HFREQFAULT_BE            0x0700591b  // Fault – High frequency
#define RVCCHRGACFOUR0LFREQFAULT_BE            0x0800591b  // Fault – Low frequency
#define RVCCHRGACFOUR0BYPASS_BE                0x0900591b  // Bypass mode active
#define RVCCHRGACFOUR0QUAL_BE                  0x0a00591b  // Qualification status
#define RVCCHRGACFAULTCFG0_BE                  0x00005a1b  // RVC DGN Charger AC Fault Configuration Status/Command (130953/130951) class
#define RVCCHRGACFAULTCFG0AVL_BE               0x00005a1b  // Available
#define RVCCHRGACFAULTCFG0SYNC_BE              0x01005a1b  // DGN class sync command
#define RVCCHRGACFAULTCFG0INST_BE              0x02005a1b  // Charger instance
#define RVCCHRGACFAULTCFG0EXTLVOLT_BE          0x03005a1b  // Extreme low voltage level
#define RVCCHRGACFAULTCFG0LVOLT_BE             0x04005a1b  // Low voltage level
#define RVCCHRGACFAULTCFG0HVOLT_BE             0x05005a1b  // High voltage level
#define RVCCHRGACFAULTCFG0EXTHVOLT_BE          0x06005a1b  // Extreme high voltage level
#define RVCCHRGACFAULTCFG0QUALTIME_BE          0x07005a1b  // Qualification time
#define RVCCHRGACFAULTCFG0BYPASS_BE            0x08005a1b  // Bypass mode
#define RVCCHRGACFAULTCFGTWO0_BE               0x00005b1b  // RVC DGN Charger AC Fault Configuration 2 Status/Command (130952/130950) class
#define RVCCHRGACFAULTCFGTWO0AVL_BE            0x00005b1b  // Available
#define RVCCHRGACFAULTCFGTWO0SYNC_BE           0x01005b1b  // DGN class sync command
#define RVCCHRGACFAULTCFGTWO0INST_BE           0x02005b1b  // Charger instance
#define RVCCHRGACFAULTCFGTWO0HFREQ_BE          0x03005b1b  // High frequency limit (Hz)
#define RVCCHRGACFAULTCFGTWO0LFREQ_BE          0x04005b1b  // Low frequency limit (Hz)
#define RVCINVERTAC0_BE                        0x00005c1b  // RVC DGN Inverter AC Status 1 (131031) class
#define RVCINVERTAC0AVL_BE                     0x00005c1b  // Available
#define RVCINVERTAC0SYNC_BE                    0x01005c1b  // DGN class sync command
#define RVCINVERTAC0INST_BE                    0x02005c1b  // Inverter instance
#define RVCINVERTAC0LINE_BE                    0x03005c1b  // Line
#define RVCINVERTAC0IO_BE                      0x04005c1b  // Input / Output
#define RVCINVERTAC0VOLT_BE                    0x05005c1b  // RMS voltage
#define RVCINVERTAC0CURR_BE                    0x06005c1b  // RMS current
#define RVCINVERTAC0FREQ_BE                    0x07005c1b  // Frequency (Hz)
#define RVCINVERTAC0OPENGROUND_BE              0x08005c1b  // Fault – open ground
#define RVCINVERTAC0OPENNEUTRAL_BE             0x09005c1b  // Fault – open neutral
#define RVCINVERTAC0REVPOL_BE                  0x0a005c1b  // Fault – reverse polarity
#define RVCINVERTAC0GROUNDCURR_BE              0x0b005c1b  // Fault – ground current
#define RVCINVERTACTWO0_BE                     0x00005d1b  // RVC DGN Inverter AC Status 2 (131030) class
#define RVCINVERTACTWO0AVL_BE                  0x00005d1b  // Available
#define RVCINVERTACTWO0SYNC_BE                 0x01005d1b  // DGN class sync command
#define RVCINVERTACTWO0INST_BE                 0x02005d1b  // Inverter instance
#define RVCINVERTACTWO0LINE_BE                 0x03005d1b  // Line
#define RVCINVERTACTWO0IO_BE                   0x04005d1b  // Input / Output
#define RVCINVERTACTWO0PVOLT_BE                0x05005d1b  // Peak voltage
#define RVCINVERTACTWO0PCURR_BE                0x06005d1b  // Peak current
#define RVCINVERTACTWO0GCURR_BE                0x07005d1b  // Ground current
#define RVCINVERTACTWO0CAP_BE                  0x08005d1b  // Capacity
#define RVCINVERTACTHREE0_BE                   0x00005e1b  // RVC DGN Inverter AC Status 3 (131029) class
#define RVCINVERTACTHREE0AVL_BE                0x00005e1b  // Available
#define RVCINVERTACTHREE0SYNC_BE               0x01005e1b  // DGN class sync command
#define RVCINVERTACTHREE0INST_BE               0x02005e1b  // Inverter instance
#define RVCINVERTACTHREE0LINE_BE               0x03005e1b  // Line
#define RVCINVERTACTHREE0IO_BE                 0x04005e1b  // Input / Output
#define RVCINVERTACTHREE0WAVEFORM_BE           0x05005e1b  // Waveform
#define RVCINVERTACTHREE0PHASE_BE              0x06005e1b  // Phase status
#define RVCINVERTACTHREE0REALPOW_BE            0x07005e1b  // Real power
#define RVCINVERTACTHREE0REACTPOW_BE           0x08005e1b  // Reactive power
#define RVCINVERTACTHREE0HARMDIST_BE           0x09005e1b  // Harmonic distortion
#define RVCINVERTACTHREE0COMPLLEG_BE           0x0a005e1b  // Complementary leg
#define RVCINVERTACFOUR0_BE                    0x00005f1b  // RVC DGN Inverter AC Status 4 (130959) class
#define RVCINVERTACFOUR0AVL_BE                 0x00005f1b  // Available
#define RVCINVERTACFOUR0SYNC_BE                0x01005f1b  // DGN class sync command
#define RVCINVERTACFOUR0INST_BE                0x02005f1b  // Inverter instance
#define RVCINVERTACFOUR0LINE_BE                0x03005f1b  // Line
#define RVCINVERTACFOUR0IO_BE                  0x04005f1b  // Input / Output
#define RVCINVERTACFOUR0VOLTFAULT_BE           0x05005f1b  // Voltage fault
#define RVCINVERTACFOUR0SURGEFAULT_BE          0x06005f1b  // Fault – Surge protection
#define RVCINVERTACFOUR0HFREQFAULT_BE          0x07005f1b  // Fault – High frequency
#define RVCINVERTACFOUR0LFREQFAULT_BE          0x08005f1b  // Fault – Low frequency
#define RVCINVERTACFOUR0BYPASS_BE              0x09005f1b  // Bypass mode active
#define RVCINVERTACFOUR0QUAL_BE                0x0a005f1b  // Qualification status
#define RVCINVERT0_BE                          0x0000601b  // RVC DGN Inverter Status/Command (131028/131027) class
#define RVCINVERT0AVL_BE                       0x0000601b  // Available
#define RVCINVERT0SYNC_BE                      0x0100601b  // DGN class sync command
#define RVCINVERT0INST_BE                      0x0200601b  // Inverter instance
#define RVCINVERT0STATUS_BE                    0x0300601b  // Status
#define RVCINVERT0BATTEMP_BE                   0x0400601b  // Battery temperature sensor present
#define RVCINVERT0LOADSENSE_BE                 0x0500601b  // Load sense enabled
#define RVCINVERT0ENABLE_BE                    0x0600601b  // Inverter enabled
#define RVCINVERT0PASSTHR_BE                   0x0700601b  // Pass-through enable
#define RVCINVERT0GENERATOR_BE                 0x0800601b  // Generator support enabled
#define RVCINVERT0ENABLESTART_BE               0x0900601b  // Inverter enabled on startup
#define RVCINVERT0LOADSENSESTART_BE            0x0a00601b  // Load sense enabled on startup
#define RVCINVERT0PASSTHRSTART_BE              0x0b00601b  // AC pass-through enable on startup
#define RVCINVERT0GENERATORSTART_BE            0x0c00601b  // Generator support enabled on startup
#define RVCINVERTCFG0_BE                       0x0000611b  // RVC DGN Inverter Configuration 1 Status/Command (131026/131024) class
#define RVCINVERTCFG0AVL_BE                    0x0000611b  // Available
#define RVCINVERTCFG0SYNC_BE                   0x0100611b  // DGN class sync command
#define RVCINVERTCFG0INST_BE                   0x0200611b  // Inverter instance
#define RVCINVERTCFG0LOADSENSEPOWTH_BE         0x0300611b  // Load sense power threshold
#define RVCINVERTCFG0LOADSENSEINT_BE           0x0400611b  // Load sense interval
#define RVCINVERTCFG0SHUTDOWNVOLTMIN_BE        0x0500611b  // DC source shutdown voltage – Minimum
#define RVCINVERTCFG0ENABLESTART_BE            0x0600611b  // Inverter enabled on startup
#define RVCINVERTCFG0LOADSENSESTART_BE         0x0700611b  // Load sense enabled on startup
#define RVCINVERTCFG0PASSTHRSTART_BE           0x0800611b  // AC pass-through enable on startup
#define RVCINVERTCFGTWO0_BE                    0x0000621b  // RVC DGN Inverter Configuration 2 Status/Command (131025/131023) class
#define RVCINVERTCFGTWO0AVL_BE                 0x0000621b  // Available
#define RVCINVERTCFGTWO0SYNC_BE                0x0100621b  // DGN class sync command
#define RVCINVERTCFGTWO0INST_BE                0x0200621b  // Inverter instance
#define RVCINVERTCFGTWO0SHUTDOWNVOLTMAX_BE     0x0300621b  // DC source shutdown voltage – Maximum
#define RVCINVERTCFGTWO0WARNVOLTMIN_BE         0x0400621b  // DC source warning voltage - Minimum
#define RVCINVERTCFGTWO0WARNVOLTMAX_BE         0x0500621b  // DC source warning voltage - Maximum
#define RVCINVERTCFGTHREE0_BE                  0x0000631b  // RVC DGN Inverter Configuration 3 Status/Command (130766/130765) class
#define RVCINVERTCFGTHREE0AVL_BE               0x0000631b  // Available
#define RVCINVERTCFGTHREE0SYNC_BE              0x0100631b  // DGN class sync command
#define RVCINVERTCFGTHREE0INST_BE              0x0200631b  // Inverter instance
#define RVCINVERTCFGTHREE0SHUTDOWNDELAY_BE     0x0300631b  // DC Source shutdown delay
#define RVCINVERTCFGTHREE0STACKMODE_BE         0x0400631b  // Stack mode
#define RVCINVERTCFGTHREE0SHUTDOWNRECLVL_BE    0x0500631b  // DC Source shutdown - Recovery Level
#define RVCINVERTCFGTHREE0GENCURR_BE           0x0600631b  // Generator Support Engage Current
#define RVCINVERTCFGFOUR0_BE                   0x0000641b  // RVC DGN Inverter Configuration 4 Status/Command (130715/130714) class
#define RVCINVERTCFGFOUR0AVL_BE                0x0000641b  // Available
#define RVCINVERTCFGFOUR0SYNC_BE               0x0100641b  // DGN class sync command
#define RVCINVERTCFGFOUR0INST_BE               0x0200641b  // Inverter instance
#define RVCINVERTCFGFOUR0VOLT_BE               0x0300641b  // Output AC voltage
#define RVCINVERTCFGFOUR0FREQ_BE               0x0400641b  // Output frequency (Hz)
#define RVCINVERTCFGFOUR0POWLIMIT_BE           0x0500641b  // AC Output power limit
#define RVCINVERTCFGFOUR0TIMELIMIT_BE          0x0600641b  // AC Output power time limit
#define RVCINVERTSTAT0_BE                      0x0000651b  // RVC DGN Inverter Statistics Status (131022) class
#define RVCINVERTSTAT0AVL_BE                   0x0000651b  // Available
#define RVCINVERTSTAT0SYNC_BE                  0x0100651b  // DGN class sync command
#define RVCINVERTSTAT0INST_BE                  0x0200651b  // Inverter instance
#define RVCINVERTSTAT0NUMUVOLT_BE              0x0300651b  // Number of DC under voltage detections
#define RVCINVERTSTAT0NUMOVERLOAD_BE           0x0400651b  // Number of inverter AC output over-loads
#define RVCINVERTSTAT0NUMLOADSENSE_BE          0x0500651b  // Number of times load sense has been engaged
#define RVCINVERTSTAT0LOWDCVOLT_BE             0x0600651b  // Lowest DC voltage
#define RVCINVERTSTAT0HIGHDCVOLT_BE            0x0700651b  // Highest DC voltage
#define RVCINVERTSTAT0LOWACIVOLT_BE            0x0800651b  // Lowest AC input voltage
#define RVCINVERTSTAT0HIGHACIVOLT_BE           0x0900651b  // Highest AC input voltage
#define RVCINVERTSTAT0LOWACOVOLT_BE            0x0a00651b  // Lowest AC output voltage
#define RVCINVERTSTAT0HIGHACOVOLT_BE           0x0b00651b  // Highest AC output voltage
#define RVCINVERTTEMP0_BE                      0x0000661b  // RVC DGN Inverter Temperature Status (130749) class
#define RVCINVERTTEMP0AVL_BE                   0x0000661b  // Available
#define RVCINVERTTEMP0SYNC_BE                  0x0100661b  // DGN class sync command
#define RVCINVERTTEMP0INST_BE                  0x0200661b  // Inverter instance
#define RVCINVERTTEMP0FETONE_BE                0x0300661b  // FET1 temperature
#define RVCINVERTTEMP0FETTWO_BE                0x0400661b  // FET2 temperature
#define RVCINVERTTEMP0TRANSF_BE                0x0500661b  // Transformer temperature
#define RVCINVERTTEMPTWO0_BE                   0x0000671b  // RVC DGN Inverter Temperature Status 2 (130507) class
#define RVCINVERTTEMPTWO0AVL_BE                0x0000671b  // Available
#define RVCINVERTTEMPTWO0SYNC_BE               0x0100671b  // DGN class sync command
#define RVCINVERTTEMPTWO0INST_BE               0x0200671b  // Inverter instance
#define RVCINVERTTEMPTWO0PB_BE                 0x0300671b  // Control/Power Board temperature
#define RVCINVERTTEMPTWO0CAPACITOR_BE          0x0400671b  // Capacitor temperature
#define RVCINVERTTEMPTWO0AMBIENT_BE            0x0500671b  // Ambient temperature
#define RVCINVERTDC0_BE                        0x0000681b  // RVC DGN Inverter DC Status (130792) class
#define RVCINVERTDC0AVL_BE                     0x0000681b  // Available
#define RVCINVERTDC0SYNC_BE                    0x0100681b  // DGN class sync command
#define RVCINVERTDC0INST_BE                    0x0200681b  // Inverter instance
#define RVCINVERTDC0VOLT_BE                    0x0300681b  // DC voltage
#define RVCINVERTDC0CURR_BE                    0x0400681b  // DC current
#define RVCATSAC0_BE                           0x0000691b  // RVC DGN Transfer Switch AC Status 1 (130989) class
#define RVCATSAC0AVL_BE                        0x0000691b  // Available
#define RVCATSAC0SYNC_BE                       0x0100691b  // DGN class sync command
#define RVCATSAC0INST_BE                       0x0200691b  // Transfer switch instance
#define RVCATSAC0SOURCE_BE                     0x0300691b  // Source
#define RVCATSAC0IO_BE                         0x0400691b  // Input / Output
#define RVCATSAC0LEG_BE                        0x0500691b  // Leg
#define RVCATSAC0VOLT_BE                       0x0600691b  // RMS voltage
#define RVCATSAC0CURR_BE                       0x0700691b  // RMS current
#define RVCATSAC0FREQ_BE                       0x0800691b  // Frequency (Hz)
#define RVCATSAC0OPENGROUND_BE                 0x0900691b  // Fault – open ground
#define RVCATSAC0OPENNEUTRAL_BE                0x0a00691b  // Fault – open neutral
#define RVCATSAC0REVPOL_BE                     0x0b00691b  // Fault – reverse polarity
#define RVCATSAC0GROUNDCURR_BE                 0x0c00691b  // Fault – ground current
#define RVCATSACTWO0_BE                        0x00006a1b  // RVC DGN Transfer Switch AC Status 2 (130988) class
#define RVCATSACTWO0AVL_BE                     0x00006a1b  // Available
#define RVCATSACTWO0SYNC_BE                    0x01006a1b  // DGN class sync command
#define RVCATSACTWO0INST_BE                    0x02006a1b  // Transfer switch instance
#define RVCATSACTWO0SOURCE_BE                  0x03006a1b  // Source
#define RVCATSACTWO0IO_BE                      0x04006a1b  // Input / Output
#define RVCATSACTWO0LEG_BE                     0x05006a1b  // Leg
#define RVCATSACTWO0PVOLT_BE                   0x06006a1b  // Peak voltage
#define RVCATSACTWO0PCURR_BE                   0x07006a1b  // Peak current
#define RVCATSACTWO0GCURR_BE                   0x08006a1b  // Ground current
#define RVCATSACTWO0CAP_BE                     0x09006a1b  // Capacity
#define RVCATSACTHREE0_BE                      0x00006b1b  // RVC DGN Transfer Switch AC Status 3 (130987) class
#define RVCATSACTHREE0AVL_BE                   0x00006b1b  // Available
#define RVCATSACTHREE0SYNC_BE                  0x01006b1b  // DGN class sync command
#define RVCATSACTHREE0INST_BE                  0x02006b1b  // Transfer switch instance
#define RVCATSACTHREE0SOURCE_BE                0x03006b1b  // Source
#define RVCATSACTHREE0IO_BE                    0x04006b1b  // Input / Output
#define RVCATSACTHREE0LEG_BE                   0x05006b1b  // Leg
#define RVCATSACTHREE0WAVEFORM_BE              0x06006b1b  // Waveform
#define RVCATSACTHREE0PHASE_BE                 0x07006b1b  // Phase status
#define RVCATSACTHREE0REALPOW_BE               0x08006b1b  // Real power
#define RVCATSACTHREE0REACTPOW_BE              0x09006b1b  // Reactive power
#define RVCATSACTHREE0HARMDIST_BE              0x0a006b1b  // Harmonic distortion
#define RVCATSACTHREE0COMPLLEG_BE              0x0b006b1b  // Complementary leg
#define RVCATSACFOUR0_BE                       0x00006c1b  // RVC DGN Transfer Switch AC Status 4 (130949) class
#define RVCATSACFOUR0AVL_BE                    0x00006c1b  // Available
#define RVCATSACFOUR0SYNC_BE                   0x01006c1b  // DGN class sync command
#define RVCATSACFOUR0INST_BE                   0x02006c1b  // Transfer switch instance
#define RVCATSACFOUR0SOURCE_BE                 0x03006c1b  // Source
#define RVCATSACFOUR0IO_BE                     0x04006c1b  // Input / Output
#define RVCATSACFOUR0LEG_BE                    0x05006c1b  // Leg
#define RVCATSACFOUR0VOLTFAULT_BE              0x06006c1b  // Voltage fault
#define RVCATSACFOUR0SURGEFAULT_BE             0x07006c1b  // Fault – Surge protection
#define RVCATSACFOUR0HFREQFAULT_BE             0x08006c1b  // Fault – High frequency
#define RVCATSACFOUR0LFREQFAULT_BE             0x09006c1b  // Fault – Low frequency
#define RVCATSACFOUR0BYPASS_BE                 0x0a006c1b  // Bypass mode active
#define RVCATSACFOUR0QUAL_BE                   0x0b006c1b  // Qualification status
#define RVCATSACFAULTCFG0_BE                   0x00006d1b  // RVC DGN ATS AC Fault Configuration Status (130948) class
#define RVCATSACFAULTCFG0AVL_BE                0x00006d1b  // Available
#define RVCATSACFAULTCFG0SYNC_BE               0x01006d1b  // DGN class sync command
#define RVCATSACFAULTCFG0INST_BE               0x02006d1b  // Transfer switch instance
#define RVCATSACFAULTCFG0EXTLVOLT_BE           0x03006d1b  // Extreme low voltage level
#define RVCATSACFAULTCFG0LVOLT_BE              0x04006d1b  // Low voltage level
#define RVCATSACFAULTCFG0HVOLT_BE              0x05006d1b  // High voltage level
#define RVCATSACFAULTCFG0EXTHVOLT_BE           0x06006d1b  // Extreme high voltage level
#define RVCATSACFAULTCFG0QUALTIME_BE           0x07006d1b  // Qualification time
#define RVCATSACFAULTCFG0BYPASS_BE             0x08006d1b  // Bypass mode
#define PROD0_BE                               0x0000001c  // Product information class
#define PROD0AVL_BE                            0x0000001c  // Available
#define PROD0NAME_BE                           0x0100001c  // Display Name
#define PROD0SN_BE                             0x0200001c  // Dometic Serial Number
#define PROD0SKU_BE                            0x0300001c  // Stock Keeping Unit
#define PROD0PNC_BE                            0x0400001c  // Product Numeric Code
#define PROD0FWVER_BE                          0x0500001c  // Firmware version
#define PROD0HWVER_BE                          0x0600001c  // Hardware version
#define PROD0MDL_BE                            0x0700001c  // Model name
#define PROD0EAN_BE                            0x0800001c  // EAN13
#define PROD0DESCRIPTION_BE                    0x0900001c  // Descriptive text
#define PROD0CLIST_BE                          0x0a00001c  // List of main DDM classes represented by product
#define PROD0UID_BE                            0x0b00001c  // Unique Identification Number
#define PROD0PROP_BE                           0x0c00001c  // Properties
#define PROD0MANUF_BE                          0x0d00001c  // Manufacturer
#define PROD0RESET_BE                          0x0e00001c  // Reset command
#define PROD0IND_BE                            0x0f00001c  // Indicate request
#define PROD0FWID_BE                           0x1000001c  // Base fwid of this product
#define EVM0_BE                                0x0000001d  // Event manager class
#define EVM0AVL_BE                             0x0000001d  // Available
#define EVM0ID_BE                              0x0100001d  // Last event occured
#define EVM0ACK_BE                             0x0200001d  // Event acknowledgement
#define EVM0CMD_BE                             0x0300001d  // Command
#define EVM0NACK_BE                            0x0400001d  // Number of events to acknowledge
#define EVM0REC_BE                             0x0500001d  // Read event record
#define EVM0TRIG_BE                            0x0600001d  // Generate event
#define EVN0_BE                                0x0000011d  // Event notification class
#define EVN0AVL_BE                             0x0000011d  // Available
#define EVN0ID_BE                              0x0100011d  // Last event occured
#define EVN0ACK_BE                             0x0200011d  // Event acknowledgement
#define EVN0REC_BE                             0x0300011d  // Read event record
#define MCHRG0_BE                              0x0000001e  // Mobile Power Charger class
#define MCHRG0AVL_BE                           0x0000001e  // Available
#define MCHRG0LINKED_BE                        0x0100001e  // List of linked class instances
#define MCHRG0TYPE_BE                          0x0200001e  // Type of charger
#define MCHRG0INST_BE                          0x0300001e  // Charger instance
#define MCHRG0DCINST_BE                        0x0400001e  // Corresponding DC instance
#define MCHRG0VOLT_BE                          0x0500001e  // Charge control voltage
#define MCHRG0CURR_BE                          0x0600001e  // Charge control current
#define MCHRG0OPER_BE                          0x0700001e  // Operating state
#define MCHRG0CVOLT_BE                         0x0800001e  // Charging voltage
#define MCHRG0CCURR_BE                         0x0900001e  // Charging current
#define MCHRG0TEMP_BE                          0x0a00001e  // Temperature
#define MCHRG0IVOLT_BE                         0x0b00001e  // DC Input voltage
#define MCHRG0ICURR_BE                         0x0c00001e  // DC Input current
#define MCHRG0PRECHG_BE                        0x0d00001e  // Pre-charging status
#define MCHRG0FORCE_BE                         0x0e00001e  // Force charge
#define MCHRG0MODE_BE                          0x0f00001e  // Charger mode
#define MCHRG0MAXCURR_BE                       0x1000001e  // Maximum charging current
#define MCHRG0POWUP_BE                         0x1100001e  // Charge on startup
#define MCHRG0BVOLT_BE                         0x1200001e  // Bulk-absorption voltage
#define MCHRG0FVOLT_BE                         0x1300001e  // Float voltage
#define MCHRG0EQVOLT_BE                        0x1400001e  // Equalization voltage
#define MCHRG0EQDUR_BE                         0x1500001e  // Equalization duration
#define MCHRG0EQINT_BE                         0x1600001e  // Equalization interval (days)
#define MCHRG0EQLAST_BE                        0x1700001e  // Time since last equalization (days)
#define MCHRG0CRVOLT_BE                        0x1800001e  // Charge return voltage
#define MCHRG0TFACT_BE                         0x1900001e  // Temperature compensation factor (mV/^C)
#define MCHRG0ENA_BE                           0x1a00001e  // Charger enable
#define MCHRG0THILIM_BE                        0x1b00001e  // External temperature sensor high tempurature limit
#define MCHRG0TLOLIM_BE                        0x1c00001e  // External temperature sensor low tempurature limit
#define MCHRG0ABSDUR_BE                        0x1d00001e  // Absorption duration
#define MCHRG0PRIO_BE                          0x1e00001e  // Charger priority
#define MCHRG0DERST_BE                         0x1f00001e  // Derating status
#define MCHRG0DERREAS_BE                       0x2000001e  // Derating reason
#define MCHRG0SLNT_BE                          0x2100001e  // Silent mode
#define MCHRG0ALGO_BE                          0x2200001e  // Charging algorithm
#define MCHRG0EXTTEMP_BE                       0x2300001e  // External temperature
#define MCHRG0STATUS_BE                        0x2400001e  // Charger fault/error status
#define MCHRG0FSTATUS_BE                       0x2500001e  // Charge controller status
#define MCHRG0RVOLT_BE                         0x2600001e  // Rated voltage
#define MCHRG0RCURR_BE                         0x2700001e  // Rated current
#define MCHRGH0_BE                             0x0000011e  // Mobile Power Charger History class
#define MCHRGH0AVL_BE                          0x0000011e  // Available
#define MCHRGH0DCINST_BE                       0x0100011e  // Corresponding DC instance
#define MCHRGH0TODAY_BE                        0x0200011e  // Today's amp-hours to battery
#define MCHRGH0YESTERDAY_BE                    0x0300011e  // Yesterday's amp-hours to battery
#define MCHRGH0YESTERDAY2_BE                   0x0400011e  // Day before yesterday's amphours to battery
#define MCHRGH0DAYS7_BE                        0x0500011e  // Last 7 days amp-hours to battery
#define MCHRGH0POWER_BE                        0x0600011e  // Cumulative power usage
#define MCHRGH0OPERDAYS_BE                     0x0700011e  // Total number of operating days
#define MCHRGH0CLRHIST_BE                      0x0800011e  // Clear history
#define MCHRGH0LINKED_BE                       0x0900011e  // List of linked class instances
#define MBAT0_BE                               0x0000021e  // Mobile Power Battery class
#define MBAT0AVL_BE                            0x0000021e  // Available
#define MBAT0LINKED_BE                         0x0100021e  // List of linked class instances
#define MBAT0DCINST_BE                         0x0200021e  // Corresponding DC instance
#define MBAT0INST_BE                           0x0300021e  // Battery instance
#define MBAT0TYPE_BE                           0x0400021e  // Battery type
#define MBAT0BANK_BE                           0x0500021e  // Bank size
#define MBAT0SERIES_BE                         0x0600021e  // Series string
#define MBAT0MODCNT_BE                         0x0700021e  // Module count
#define MBAT0CELLS_BE                          0x0800021e  // Cells per module
#define MBAT0VOLT_BE                           0x0900021e  // DC voltage
#define MBAT0CURR_BE                           0x0a00021e  // DC current (negative-discharging,positive-charging)
#define MBAT0TEMP_BE                           0x0b00021e  // Source temperature
#define MBAT0SOC_BE                            0x0c00021e  // State of charge (SOC)
#define MBAT0TIME_BE                           0x0d00021e  // Time remaining
#define MBAT0TIMESTS_BE                        0x0e00021e  // Time remaining interpretation
#define MBAT0SOH_BE                            0x0f00021e  // State of health
#define MBAT0CAPREM_BE                         0x1000021e  // Capacity remaining
#define MBAT0CAPREL_BE                         0x1100021e  // Relative capacity
#define MBAT0ACRIPPLE_BE                       0x1200021e  // AC RMS ripple
#define MBAT0DCHGST_BE                         0x1300021e  // Desired charge state
#define MBAT0DVOLT_BE                          0x1400021e  // Desired DC voltage
#define MBAT0DCURR_BE                          0x1500021e  // Desired DC current
#define MBAT0HPVOLT_BE                         0x1600021e  // High precision DC voltage of the battery bank or battery.
#define MBAT0DV_DT_BE                          0x1700021e  // DC Voltage Rate of Change (mV/s)
#define MBAT0DISCHGST_BE                       0x1800021e  // Discharge On/Off status
#define MBAT0CHGST_BE                          0x1900021e  // Charge On/Off status
#define MBAT0CHGDET_BE                         0x1a00021e  // Charge detected
#define MBAT0RESST_BE                          0x1b00021e  // Reserve status
#define MBAT0CAPACITY_BE                       0x1c00021e  // Full capacity
#define MBAT0BAL_BE                            0x1d00021e  // Balancing
#define MBAT0POLES_BE                          0x1e00021e  // Poles status, poles connected to the cells
#define MBAT0HTR_BE                            0x1f00021e  // Heater status
#define MBAT0DCDC_BE                           0x2000021e  // Internal regulator status
#define MBAT0STATUS_BE                         0x2100021e  // Battery fault/error status
#define MBAT0HMODE_BE                          0x2200021e  // Heater mode
#define MBATHIST0_BE                           0x0000031e  // Mobile Power Battery History class
#define MBATHIST0AVL_BE                        0x0000031e  // Available
#define MBATHIST0INST_BE                       0x0100031e  // Battery instance
#define MBATHIST0DCINST_BE                     0x0200031e  // DC instance
#define MBATHIST0TODAYI_BE                     0x0300031e  // Today's input Amp-Hours
#define MBATHIST0TODAYO_BE                     0x0400031e  // Today's output Amp-Hours
#define MBATHIST0YESTERDAYI_BE                 0x0500031e  // Yesterday's input Amp-Hours
#define MBATHIST0YESTERDAYO_BE                 0x0600031e  // Yesterday's output Amp-Hours
#define MBATHIST0YESTERDAY2I_BE                0x0700031e  // Day before yesterday's input Amp-Hours
#define MBATHIST0YESTERDAY2O_BE                0x0800031e  // Day before yesterday's output Amp-Hours
#define MBATHIST0DAYS7I_BE                     0x0900031e  // Last 7 days input Amp-Hours
#define MBATHIST0DAYS7O_BE                     0x0a00031e  // Last 7 days output Amp-Hours
#define MBATHIST0DEEPDISCHG_BE                 0x0b00031e  // The deepest discharge in Amp-Hours
#define MBATHIST0AVEDISCHG_BE                  0x0c00031e  // Average discharge depth
#define MBATHIST0CYCLES_BE                     0x0d00031e  // The number of charge cycles
#define MBATHIST0LOWVOLT_BE                    0x0e00031e  // Lowest DC voltage
#define MBATHIST0HIGHVOLT_BE                   0x0f00031e  // Highest DC voltage
#define MBATHIST0CLRHIST_BE                    0x1000031e  // Clear history
#define MBATHIST0LINKED_BE                     0x1100031e  // List of linked class instances
#define MBATCELL0_BE                           0x0000041e  // Mobile Power Battery Cell class
#define MBATCELL0AVL_BE                        0x0000041e  // Available
#define MBATCELL0LINKED_BE                     0x0100041e  // List of linked class instances
#define MBATCELL0INST_BE                       0x0200041e  // Cell instance
#define MBATCELL0BINST_BE                      0x0300041e  // Battery instance
#define MBATCELL0VOLTSTS_BE                    0x0400041e  // Cell voltage status
#define MBATCELL0TEMPSTS_BE                    0x0500041e  // Cell temperature status
#define MBATCELL0BAL_BE                        0x0600041e  // Balancing
#define MBATCELL0VOLT_BE                       0x0700041e  // Cell voltage
#define MBATCELL0TEMP_BE                       0x0800041e  // Cell temperature
#define MSOLAR0_BE                             0x0000051e  // Mobile Power Solar Controller class
#define MSOLAR0AVL_BE                          0x0000051e  // Available
#define MSOLAR0LINKED_BE                       0x0100051e  // List of linked class instances
#define MSOLAR0DCINST_BE                       0x0200051e  // DC instance
#define MSOLAR0INST_BE                         0x0300051e  // Solar controller instance
#define MSOLAR0VOLT_BE                         0x0400051e  // Measured voltage on solar array input
#define MSOLAR0CURR_BE                         0x0500051e  // Measured current coming in from the solar array
#define MSOLAR0HBCO_BE                         0x0600051e  // High Battery Cut Off (HBCO) Voltage
#define MSOLAR0HBCI_BE                         0x0700051e  // High Battery Cut In (HBCI) Voltage
#define MSOLAR0LBCO_BE                         0x0800051e  // Low Battery Cut Out (LBCO) Voltage
#define MSOLAR0LBCI_BE                         0x0900051e  // Low Battery Cut In (LBCI) Voltage
#define MSOLAR0LBCOD_BE                        0x0a00051e  // Low Battery Cut Out (LBCO) Delay
#define MSOLAR0PPWR_BE                         0x0b00051e  // Panel power
#define MSOLAR0RPWR_BE                         0x0c00051e  // Rated panel power
#define MSOLAR0RVOLT_BE                        0x0d00051e  // Rated panel voltage
#define MACIN0_BE                              0x0000061e  // Mobile Power AC Input class
#define MACIN0AVL_BE                           0x0000061e  // Available
#define MACIN0LINKED_BE                        0x0100061e  // List of linked class instances
#define MACIN0DCINST_BE                        0x0200061e  // DC instance
#define MACIN0RMSVOLT_BE                       0x0300061e  // RMS voltage
#define MACIN0RMSCURR_BE                       0x0400061e  // RMS current
#define MACIN0PEAKVOLT_BE                      0x0500061e  // Peak voltage
#define MACIN0PEAKCURR_BE                      0x0600061e  // Peak current
#define MACIN0FREQ_BE                          0x0700061e  // Frequency (Hz)
#define MACIN0BYPASS_BE                        0x0800061e  // Bypass mode
#define MACIN0QUALST_BE                        0x0900061e  // AC input qualification status
#define MACIN0LOWFREQLIM_BE                    0x0a00061e  // Low frequency limit (Hz)
#define MACIN0HIGHFREQLIM_BE                   0x0b00061e  // High frequency limit (Hz)
#define MACIN0VOLTST_BE                        0x0c00061e  // Voltage fault status
#define MACIN0FREQST_BE                        0x0d00061e  // Frequency fault status
#define MACIN0GROUND_BE                        0x0e00061e  // Open ground fault 
#define MACIN0NEUTRAL_BE                       0x0f00061e  // Neutral fault
#define MACIN0REVPOL_BE                        0x1000061e  // Reverse polarity detected
#define MACIN0MAXCURR_BE                       0x1100061e  // Shore breaker size
#define MACIN0POWER_BE                         0x1200061e  // AC input available power
#define MACIN0LVOLT_BE                         0x1300061e  // Low Voltage Level
#define MINVERT0_BE                            0x0000071e  // Mobile Power Inverter class
#define MINVERT0AVL_BE                         0x0000071e  // Available
#define MINVERT0LINKED_BE                      0x0100071e  // List of linked class instances
#define MINVERT0INST_BE                        0x0200071e  // Inverter instance
#define MINVERT0DCINST_BE                      0x0300071e  // DC instance
#define MINVERT0VOLT_BE                        0x0400071e  // DC input voltage
#define MINVERT0CURR_BE                        0x0500071e  // DC input current
#define MINVERT0ENABLE_BE                      0x0600071e  // Inverter enable
#define MINVERT0PASSTHROUGH_BE                 0x0700071e  // Pass-through enable
#define MINVERT0LOADSENSE_BE                   0x0800071e  // Load sense enable
#define MINVERT0ENASTARTUP_BE                  0x0900071e  // Inverter enable on startup
#define MINVERT0LSENSESTARTUP_BE               0x0a00071e  // Load sense enable on startup
#define MINVERT0PASSTHRSTARTUP_BE              0x0b00071e  // Pass-through enable on startup
#define MINVERT0OUTVOLT_BE                     0x0c00071e  // Set output AC voltage
#define MINVERT0OUTFREQ_BE                     0x0d00071e  // Set output AC frequency (Hz)
#define MINVERT0LSENSEINT_BE                   0x0e00071e  // Load sense internval
#define MINVERT0LSENSEPOW_BE                   0x0f00071e  // Load sense power threshold
#define MINVERT0STACK_BE                       0x1000071e  // Stack mode
#define MINVERT0POWLIM_BE                      0x1100071e  // Output power limit
#define MINVERT0FET1TEMP_BE                    0x1200071e  // FET1 Temperature
#define MINVERT0FET2TEMP_BE                    0x1300071e  // FET2 Temperature
#define MINVERT0TRANSFTEMP_BE                  0x1400071e  // Transformer Temperature
#define MINVERT0CAPTEMP_BE                     0x1500071e  // Capacitor Temperature
#define MINVERT0AMBTEMP_BE                     0x1600071e  // Ambient Temperature
#define MINVERT0LOWACVOLT_BE                   0x1700071e  // Lowest AC input voltage
#define MINVERT0HIGHACVOLT_BE                  0x1800071e  // Highest AC input voltage
#define MINVERT0NUMACOVERLOAD_BE               0x1900071e  // Number of inverter AC output over-loads
#define MINVERT0NUMLOADSENSE_BE                0x1a00071e  // Number of times load sense has been enganged
#define MINVERT0SHDOWNVOLTMIN_BE               0x1b00071e  // DC source shutdown voltage – Minimum
#define MINVERT0SHDOWNVOLTMAX_BE               0x1c00071e  // DC source shutdown voltage – Maximum
#define MINVERT0WARNVOLTMIN_BE                 0x1d00071e  // DC source warning voltage – Minimum
#define MINVERT0WARNVOLTMAX_BE                 0x1e00071e  // DC source warning voltage – Maximum
#define MINVERT0RECVOLT_BE                     0x1f00071e  // DC Source shutdown - Recovery Level
#define MINVERT0WRKLD_BE                       0x2000071e  // Workload
#define MINVERT0STATUS_BE                      0x2100071e  // Inverter faults/error status
#define MINVERT0PCBTEMP_BE                     0x2200071e  // PCB temperature
#define MINVERT0MAXCURR_BE                     0x2300071e  // Maximum AC current
#define MACOUT0_BE                             0x0000081e  // Mobile Power AC Output class
#define MACOUT0AVL_BE                          0x0000081e  // Available
#define MACOUT0LINKED_BE                       0x0100081e  // List of linked class instances
#define MACOUT0DCINST_BE                       0x0200081e  // DC instance
#define MACOUT0RMSVOLT_BE                      0x0300081e  // Measured RMS voltage
#define MACOUT0RMSCURR_BE                      0x0400081e  // Measured RMS current
#define MACOUT0PEAKVOLT_BE                     0x0500081e  // Measured Peak voltage
#define MACOUT0PEAKCURR_BE                     0x0600081e  // Measured Peak current
#define MACOUT0FREQ_BE                         0x0700081e  // Measured Frequency (Hz)
#define MACOUT0PHASE_BE                        0x0800081e  // Phase status
#define MACOUT0WAVE_BE                         0x0900081e  // Waveform
#define MACOUT0POWER_BE                        0x0a00081e  // AC output available power
#define MACOUT0LVOLT_BE                        0x0b00081e  // Low Voltage Level
#define MPS0_BE                                0x0000091e  // Mobile Power BLE device class
#define MPS0AVL_BE                             0x0000091e  // Available
#define MPS0PLIST_BE                           0x0100091e  // Product list
#define MPS0RSTPWD_BE                          0x0200091e  // Reset password
#define MPS0SOC_BE                             0x0300091e  // System State of Chage (SOC)
#define MSHUNT0_BE                             0x00000a1e  // Mobile Power Shunt class
#define MSHUNT0AVL_BE                          0x00000a1e  // Available
#define MSHUNT0LINKED_BE                       0x01000a1e  // List of linked class instances
#define MSHUNT0TYPE_BE                         0x02000a1e  // Shunt monitoring device type
#define MSHUNT0RVOLT_BE                        0x03000a1e  // Rated voltage
#define MSHUNT0RCURR_BE                        0x04000a1e  // Rated current
#define MSHUNT0DISCHGVOLT_BE                   0x05000a1e  // Discharge voltage
#define MSHUNT0DISCHGFLOOR_BE                  0x06000a1e  // Discharge floor
#define MSHUNT0HVOLTLIM_BE                     0x07000a1e  // High voltage limit
#define MSHUNT0LTTEMPLIM_BE                    0x08000a1e  // Low temperature limit
#define MSHUNT0HTEMPLIM_BE                     0x09000a1e  // High temperature limit
#define MSHUNT0HCURRLIM_BE                     0x0a000a1e  // High current limit
#define MSHUNT0SYSVOLT_BE                      0x0b000a1e  // System voltage
#define MSHUNT0CALIBZERO_BE                    0x0c000a1e  // Calibrate zero voltage
#define MSHUNT0EXTREL_BE                       0x0d000a1e  // External relay
#define MSHUNT0STATUS_BE                       0x0e000a1e  // Shunt fault/error status
#define MSHUNT0SETCAP_BE                       0x0f000a1e  // Set capacity to 100%
#define MDCPROFILE0_BE                         0x00000b1e  // Mobile Power DC Profile class
#define MDCPROFILE0AVL_BE                      0x00000b1e  // Available
#define MDCPROFILE0LINKED_BE                   0x01000b1e  // List of linked class instances
#define MDCPROFILE0TYPE_BE                     0x02000b1e  // Battery type
#define MDCPROFILE0ALGO_BE                     0x03000b1e  // Charging algorithm
#define MDCPROFILE0BANK_BE                     0x04000b1e  // Bank size
#define MDCPROFILE0CHGVOLT_BE                  0x05000b1e  // Charged voltage
#define MDCPROFILE0DISCHGVOLT_BE               0x06000b1e  // Discharged voltage
#define MDCPROFILE0CHGEFF_BE                   0x07000b1e  // Charge efficiency
#define MDCPROFILE0PEUKCOEFF_BE                0x08000b1e  // Peukert coefficient
#define MDCPROFILE0TEMPCOEFF_BE                0x09000b1e  // Temperature coefficient
#define MDCPROFILE0TAILCURR_BE                 0x0a000b1e  // Tail current
#define MDCPROFILE0MAXCURR_BE                  0x0b000b1e  // Current limitation
#define MDCPROFILE0HVDISCONN_BE                0x0c000b1e  // High voltage disconnect
#define MDCPROFILE0UVOLTWARN_BE                0x0d000b1e  // Under voltage warning
#define MDCPROFILE0EQVOLT_BE                   0x0e000b1e  // Equalization voltage
#define MDCPROFILE0EQDUR_BE                    0x0f000b1e  // Equalization duration
#define MDCPROFILE0EQINT_BE                    0x10000b1e  // Equalization interval
#define MDCPROFILE0BVOLT_BE                    0x11000b1e  // Bulk-absorption voltage
#define MDCPROFILE0ABSDUR_BE                   0x12000b1e  // Absorption duration
#define MDCPROFILE0FVOLT_BE                    0x13000b1e  // Float voltage
#define MDCPROFILE0CRVOLT_BE                   0x14000b1e  // Recharge voltage
#define MDCPROFILE0TFACT_BE                    0x15000b1e  // Temperature compensation factor (mV/^C)
#define MDCPROFILE0LVDISCONN_BE                0x16000b1e  // Low voltage disconnect
#define MDCPROFILE0HVRETURN_BE                 0x17000b1e  // Battery high voltage limit return voltage
#define MDCPROFILE0LVRETURN_BE                 0x18000b1e  // Battery low voltage limit return voltage
#define MDCPROFILE0LVDELAY_BE                  0x19000b1e  // Battery low voltage limit time delay
#define MDCPROFILE0DCVOLT_BE                   0x1a000b1e  // DC voltage
#define MDCPROFILE0DCCURR_BE                   0x1b000b1e  // DC current
#define MDCPROFILE0TEMP_BE                     0x1c000b1e  // Temperature
#define SYSAPPL0_BE                            0x0000001f  // System Application Features class
#define SYSAPPL0AVL_BE                         0x0000001f  // Available
#define SYSAPPL0SMARTECO_BE                    0x0100001f  // SmartECO feature
#define GROUP0_BE                              0x0000011f  // Group/Feature class
#define GROUP0AVL_BE                           0x0000011f  // Available
#define GROUP0UID_BE                           0x0100011f  // Unique Identification Number
#define GROUP0ID_BE                            0x0200011f  // Identification number per type
#define GROUP0NAME_BE                          0x0300011f  // Name
#define GROUP0TYPE_BE                          0x0400011f  // Type of feature this group implements
#define GROUP0RULES_BE                         0x0500011f  // Rule engine class instances implementing this feature
#define GROUP0INTERFACE_BE                     0x0600011f  // Product classes/group classes and/or other class instances representing this feature
#define GROUP0ENABLE_BE                        0x0700011f  // Enabling feature
#define GROUP0ACTIVE_BE                        0x0800011f  // Control feature
#define GROUP0ADDGROUP_BE                      0x0900011f  // Request to create new class instance of same type
#define GROUP0DELGROUP_BE                      0x0a00011f  // Delete the group instance
#define MULTIZONE0_BE                          0x0000021f  // Multizone class
#define MULTIZONE0AVL_BE                       0x0000021f  // Available
#define MULTIZONE0SELZONE_BE                   0x0100021f  // Active selected zone
#define MULTIZONE0ZONES_BE                     0x0200021f  // Available zones
#define MULTIZONE0NUMZONES_BE                  0x0300021f  // Number of available zones
#define MULTIZONE0ITEMP_BE                     0x0400021f  // Internal zone temperature (ambient)
#define MULTIZONE0ETEMP_BE                     0x0500021f  // External zone temperature (outside)
#define MULTIZONE0INTERFACE_BE                 0x0600021f  // Class instances (virtual) representing the zone
#define GROUPMGR0_BE                           0x0000031f  // Group Management class
#define GROUPMGR0AVL_BE                        0x0000031f  // Available
#define GROUPMGR0REQTYPE_BE                    0x0100031f  // Query if specific group type is available/enabled in the system

#define DDM2_PARAMETER_COUNT 2421
#define DDM2_CLASS_COUNT 237

typedef enum DDM2_TYPE_ENUM
{
    DDM2_TYPE_INT32_T,
    DDM2_TYPE_STRING,
    DDM2_TYPE_OTHER,
    DDM2_TYPE_NONE,
    DDM2_TYPE_JUMBO,
    DDM2_TYPE_VOID,
    DDM2_TYPE_UINT32_T,
    DDM2_TYPE_STRUCT,
    DDM2_TYPE_COUNT,
} DDM2_TYPE_ENUM;

typedef struct GW0INV_T
{
    uint32_t inventory[0];    //inventory
} PACKED GW0INV_T;

// 
typedef struct ERROR_T
{
    uint16_t error[0];    //error
} PACKED ERROR_T;

typedef struct WIFI0SCAN_T
{
    uint8_t rssi[4];    //string
    uint8_t auto_mode[4];    //string
    uint8_t name[32];    //string
} PACKED WIFI0SCAN_T;

typedef struct BT0SCAN_T
{
    uint16_t manufacturer;    //snode0mfgr
    uint8_t node_type;    //gw0ptype
    uint8_t node_id;    //snode0mdl
    int8_t rssi;
    uint8_t ble_address_type;    //btwl0addrtp
    uint8_t ble_address[6];
    uint8_t name[16];    //string
    uint8_t model_name[16];    //string
    uint8_t bond;
    uint8_t sensor_ids[0];
} PACKED BT0SCAN_T;

// 
typedef struct BT0ADDWL_T
{
    uint8_t type;
    uint8_t addr[6];
    uint8_t password[0];
} PACKED BT0ADDWL_T;

// 
typedef struct HMI0EVENT_T
{
    uint32_t id;
    uint8_t data[0];
} PACKED HMI0EVENT_T;

typedef struct LSRCFG0BINDATA_T
{
    int32_t time;
    int32_t interval;
    int32_t data[0];
} PACKED LSRCFG0BINDATA_T;

// 
typedef struct BOOLARR_T
{
    int32_t bool_arr[0];    //bool
} PACKED BOOLARR_T;

typedef struct TEMPARR_T
{
    int32_t temp[0];    //^C
} PACKED TEMPARR_T;

// 
typedef struct TEMPRANGEARR_T
{
    struct TEMP_RANGE
    {
        int32_t mintemp;    //^C
        int32_t maxtemp;    //^C
    } temp_range[0];
} PACKED TEMPRANGEARR_T;

// 
typedef struct RVCMGNT0REQ_T
{
    uint32_t dgn;
    uint8_t addr;
} PACKED RVCMGNT0REQ_T;

// 
typedef struct RVCMGNT0RAWTX_T
{
    uint8_t is_ext_type;    //bool
    uint32_t dgn;
    uint8_t addr;
    uint8_t data[8];
} PACKED RVCMGNT0RAWTX_T;

// 
typedef struct RVCMGNT0RAWRX_T
{
    uint8_t is_ext_type;    //bool
    uint32_t dgn;
    uint8_t addr;
    uint8_t data[0];
} PACKED RVCMGNT0RAWRX_T;

// 
typedef struct RVCMGNT0ACK_T
{
    uint8_t ack_code;
    uint8_t inst;
    uint8_t inst_bank;
    uint8_t source_addr;
    uint32_t dgn;
    uint8_t to_from_addr;
} PACKED RVCMGNT0ACK_T;

// 
typedef struct RVCMGNT0DOWNLOAD_T
{
    uint8_t addr;
    uint8_t data[8];
} PACKED RVCMGNT0DOWNLOAD_T;

// 
typedef struct RVCPROP0DATA_T
{
    uint8_t data[8];
} PACKED RVCPROP0DATA_T;

// update = 1 -> add, = 0 -> remove class, default to add
typedef struct UPDLINKEDCLASS_T
{
    uint32_t updclass;    //instance
    uint8_t update[0];
} PACKED UPDLINKEDCLASS_T;

typedef struct LINKEDCLASS_T
{
    uint32_t classes[0];    //instance
} PACKED LINKEDCLASS_T;

typedef struct PROD0PROP_T
{
    uint8_t type;
    uint8_t addr;
    uint8_t inst;
    uint32_t classes[0];    //instance
} PACKED PROD0PROP_T;

typedef struct EVM0NACK_T
{
    int32_t nack;
    int32_t ids[0];
} PACKED EVM0NACK_T;

// Normal is just the event type but possible to attach variable length of i32 meta data
typedef struct EVM0TRIG_T
{
    int32_t type;
    uint32_t data[0];
} PACKED EVM0TRIG_T;

// 
typedef struct ARRAYI32_T
{
    int32_t arr[0];
} PACKED ARRAYI32_T;

typedef struct GROUPCLASSES_T
{
    struct GROUPS
    {
        uint32_t class_inst;    //instance
        uint8_t name[32];    //string
    } groups[0];
} PACKED GROUPCLASSES_T;

typedef enum DDM2_UNIT_ENUM
{
    DDM2_UNIT_BOOL,
    DDM2_UNIT_ENUMERATION,
    DDM2_UNIT_AMPERE,
    DDM2_UNIT_VOLT,
    DDM2_UNIT_PART,
    DDM2_UNIT_DEGC,
    DDM2_UNIT_DEG,
    DDM2_UNIT_DEGG,
    DDM2_UNIT_NONE,
    DDM2_UNIT_MINUTE,
    DDM2_UNIT_SECOND,
    DDM2_UNIT_HOUR,
    DDM2_UNIT_WATT,
    DDM2_UNIT_PASCAL,
    DDM2_UNIT_L_PER_HOUR,
    DDM2_UNIT_RPM,
    DDM2_UNIT_AMPHOURS,
    DDM2_UNIT_KM_PER_HOUR,
    DDM2_UNIT_G_PER_KG,
    DDM2_UNIT_L_PER_KM,
    DDM2_UNIT_METER,
    DDM2_UNIT_G,
    DDM2_UNIT_DIFFPASCAL,
    DDM2_UNIT_PPM,
    DDM2_UNIT_OHM,
    DDM2_UNIT_UNIX,
    DDM2_UNIT_MILLITESLA,
    DDM2_UNIT_PARAMETER,
    DDM2_UNIT_INSTANCE,
    DDM2_UNIT_CLASS,
    DDM2_UNIT_ERROR,
    DDM2_UNIT_INVENTORY,
    DDM2_UNIT_JSON,
    DDM2_UNIT_BITFIELD,
    DDM2_UNIT_PERCENT,
    DDM2_UNIT_DECIMAL,
    DDM2_UNIT_RESULT,
    DDM2_UNIT_COUNT,
} DDM2_UNIT_ENUM;

typedef struct DDM2_PARAMETER_LIST_DATA
{
    uint32_t parameter;
    int cloud;
    const char *netwthing;
    const char *device_class;
    const char *property;
    uint8_t in_type;
    uint8_t in_unit;
    uint8_t out_type;
    uint8_t out_unit;
} DDM2_PARAMETER_LIST_DATA;

typedef enum GW0PTYPE_ENUM
{
    GW0PTYPE_UNCONFIGURED_RUBICON_CFX3 = 0x00,
    GW0PTYPE_RUBICON_CFX3_SINGLE_ZONE = 0x01,
    GW0PTYPE_RUBICON_CFX3_SINGLE_ZONE_WITH_ICEMAKER = 0x02,
    GW0PTYPE_RUBICON_CFX3_DUAL_ZONE = 0x03,
    GW0PTYPE_DOMETIC_DICM_GATEWAY = 0x04,
    GW0PTYPE_DOMETIC_SENSOR = 0x05,
    GW0PTYPE_DOMETIC_GARNET = 0x06,
    GW0PTYPE_DOMETIC_MOPEKA = 0x07,
    GW0PTYPE_DOMETIC_AREALIGHTS = 0x08,
    GW0PTYPE_MARINE_GATEWAY = 0x10,
    GW0PTYPE_MARINE_DEVICE = 0x11,
    GW0PTYPE_MARINE_RESERVED_0X12 = 0x12,
    GW0PTYPE_MARINE_RESERVED_0X13 = 0x13,
    GW0PTYPE_MOBILE_POWER = 0x14,
} GW0PTYPE_ENUM;

typedef enum GW0RRSN_ENUM
{
    GW0RRSN_ESP_RST_UNKNOWN = 0,
    GW0RRSN_ESP_RST_POWERON = 1,
    GW0RRSN_ESP_RST_EXT = 2,
    GW0RRSN_ESP_RST_SW = 3,
    GW0RRSN_ESP_RST_PANIC = 4,
    GW0RRSN_ESP_RST_INT_WDT = 5,
    GW0RRSN_ESP_RST_TASK_WDT = 6,
    GW0RRSN_ESP_RST_WDT = 7,
    GW0RRSN_ESP_RST_DEEPSLEEP = 8,
    GW0RRSN_ESP_RST_BROWNOUT = 9,
    GW0RRSN_ESP_RST_SDIO = 10,
} GW0RRSN_ENUM;

typedef enum GW0CUPD_ENUM
{
    GW0CUPD_ACKNOWLEDGED = 0,
    GW0CUPD_CLOUD_UPDATE = 1,
    GW0CUPD_RESTART = 11223344,
    GW0CUPD_FACTORY_RESET = 19181716,
    GW0CUPD_TEST_TRIGGER = 1953719668,
} GW0CUPD_ENUM;

typedef enum CFG0OSTAT_ENUM
{
    CFG0OSTAT_OK = 0,
    CFG0OSTAT_ONGOING = 1,
    CFG0OSTAT_FAILED = 2,
} CFG0OSTAT_ENUM;

typedef enum IBS0BATTYP_ENUM
{
    IBS0BATTYP_FLOODED = 0,
    IBS0BATTYP_GEL = 1,
    IBS0BATTYP_AGM = 2,
    IBS0BATTYP_LITHIUM_ION = 3,
} IBS0BATTYP_ENUM;

typedef enum AC0FSPD_ENUM
{
    AC0FSPD_LOW = 0,
    AC0FSPD_MED = 1,
    AC0FSPD_HIGH = 2,
    AC0FSPD_MAX = 3,
    AC0FSPD_NIGHT = 4,
    AC0FSPD_AUTO = 5,
} AC0FSPD_ENUM;

typedef enum AC0MD_ENUM
{
    AC0MD_COOL = 0,
    AC0MD_HEAT = 1,
    AC0MD_FAN = 2,
    AC0MD_AUTO = 3,
    AC0MD_DRY = 4,
    AC0MD_TURBO = 5,
} AC0MD_ENUM;

typedef enum AC0MDL_ENUM
{
    AC0MDL_NONE = 0,
    AC0MDL_DOMETIC_FRESHJET = 1,
    AC0MDL_TRUMA_AVENTA_COMFORT = 2,
    AC0MDL_TRUMA_AVENTA_COMFORT_CP_PLUS = 3,
    AC0MDL_TRUMA_SAPHIR_COMPACT = 4,
    AC0MDL_TRUMA_SAPHIR_COMPACT_CP_PLUS = 5,
    AC0MDL_TRUMA_AVENTA_ECO = 6,
    AC0MDL_TRUMA_AVENTA_ECO_CP_PLUS = 7,
    AC0MDL_TRUMA_SAPHIR_COMFORT_RC = 8,
    AC0MDL_TRUMA_SAPHIR_COMFORT_RC_CP_PLUS = 9,
    AC0MDL_DOMETIC_FRESHWELL = 10,
    AC0MDL_UNKNOWN = 11,
    AC0MDL_DOMETIC_FJX4000_SERIES = 12,
    AC0MDL_DOMETIC_FJX7000_SERIES = 13,
    AC0MDL_DOMETIC_APAC_HARRIER = 14,
    AC0MDL_DOMETIC_APAC_IBIS4 = 15,
    AC0MDL_DOMETIC_APAC_CK_LITE = 16,
    AC0MDL_DOMETIC_FJZ4000_SERIES = 17,
    AC0MDL_DOMETIC_FJZ7000_SERIES = 18,
    AC0MDL_DOMETIC_FRESHJET_4_SERIES = 19,
    AC0MDL_DOMETIC_FRESHJET_5_SERIES = 20,
    AC0MDL_DOMETIC_FRESHJET_6_SERIES = 21,
    AC0MDL_DOMETIC_FRESHJET_7_SERIES = 22,
    AC0MDL_WAECO_FIXED_SPEED = 23,
    AC0MDL_WAECO_INVERTER = 24,
} AC0MDL_ENUM;

typedef enum AC0FMD_ENUM
{
    AC0FMD_AUTO = 0,
    AC0FMD_ON = 1,
} AC0FMD_ENUM;

typedef enum HTR0SYSU_ENUM
{
    HTR0SYSU_METRIC = 0,
    HTR0SYSU_IMPERIAL = 1,
} HTR0SYSU_ENUM;

typedef enum AC0TMD_ENUM
{
    AC0TMD_RELATIVE = 0,
    AC0TMD_ABSOLUTE = 1,
} AC0TMD_ENUM;

typedef enum AC0ACTEXT_IN_BITMASK_ENUM
{
    AC0ACTEXT_IN_INVERTERON_BITMASK = 1 << 4, // 0x10
    AC0ACTEXT_IN_FORCELOADSHED_BITMASK = 1 << 6, // 0x40
} AC0ACTEXT_IN_BITMASK_ENUM;

typedef struct AC0ACTEXT_IN_T
{
    union
    {
        struct
        {
            uint32_t unused_0 : 1;
            uint32_t unused_1 : 1;
            uint32_t unused_2 : 1;
            uint32_t unused_3 : 1;
            uint32_t inverteron : 1;
            uint32_t unused_5 : 1;
            uint32_t forceloadshed : 1;
            uint32_t unused_7 : 1;
            uint32_t unused_8 : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED AC0ACTEXT_IN_T;


#define AC0ACTEXT_IN_INVERTERON_GET(a) ( ((AC0ACTEXT_IN_INVERTERON_BITMASK) & (a)) >> 4 )
#define AC0ACTEXT_IN_INVERTERON_SET(a,b) (a) = ( (~(AC0ACTEXT_IN_INVERTERON_BITMASK) & (a)) | ( ((b) & 1) << 4) )
#define AC0ACTEXT_IN_FORCELOADSHED_GET(a) ( ((AC0ACTEXT_IN_FORCELOADSHED_BITMASK) & (a)) >> 6 )
#define AC0ACTEXT_IN_FORCELOADSHED_SET(a,b) (a) = ( (~(AC0ACTEXT_IN_FORCELOADSHED_BITMASK) & (a)) | ( ((b) & 1) << 6) )

typedef enum AC0ACTEXT_OUT_BITMASK_ENUM
{
    AC0ACTEXT_OUT_HEATER_BITMASK = 1 << 0, // 1
    AC0ACTEXT_OUT_COMPRESSOR_BITMASK = 1 << 1, // 2
    AC0ACTEXT_OUT_INVERTER_BITMASK = 1 << 4, // 0x10
    AC0ACTEXT_OUT_FANEVAP_BITMASK = 1 << 5, // 0x20
    AC0ACTEXT_OUT_FORCELOADSHED_BITMASK = 1 << 6, // 0x40
} AC0ACTEXT_OUT_BITMASK_ENUM;

typedef struct AC0ACTEXT_OUT_T
{
    union
    {
        struct
        {
            uint32_t heater : 1;
            uint32_t compressor : 1;
            uint32_t unused_2 : 1;
            uint32_t unused_3 : 1;
            uint32_t inverter : 1;
            uint32_t fanevap : 1;
            uint32_t forceloadshed : 1;
            uint32_t unused_7 : 1;
            uint32_t unused_8 : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED AC0ACTEXT_OUT_T;


#define AC0ACTEXT_OUT_HEATER_GET(a) ( ((AC0ACTEXT_OUT_HEATER_BITMASK) & (a)) >> 0 )
#define AC0ACTEXT_OUT_HEATER_SET(a,b) (a) = ( (~(AC0ACTEXT_OUT_HEATER_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define AC0ACTEXT_OUT_COMPRESSOR_GET(a) ( ((AC0ACTEXT_OUT_COMPRESSOR_BITMASK) & (a)) >> 1 )
#define AC0ACTEXT_OUT_COMPRESSOR_SET(a,b) (a) = ( (~(AC0ACTEXT_OUT_COMPRESSOR_BITMASK) & (a)) | ( ((b) & 1) << 1) )
#define AC0ACTEXT_OUT_INVERTER_GET(a) ( ((AC0ACTEXT_OUT_INVERTER_BITMASK) & (a)) >> 4 )
#define AC0ACTEXT_OUT_INVERTER_SET(a,b) (a) = ( (~(AC0ACTEXT_OUT_INVERTER_BITMASK) & (a)) | ( ((b) & 1) << 4) )
#define AC0ACTEXT_OUT_FANEVAP_GET(a) ( ((AC0ACTEXT_OUT_FANEVAP_BITMASK) & (a)) >> 5 )
#define AC0ACTEXT_OUT_FANEVAP_SET(a,b) (a) = ( (~(AC0ACTEXT_OUT_FANEVAP_BITMASK) & (a)) | ( ((b) & 1) << 5) )
#define AC0ACTEXT_OUT_FORCELOADSHED_GET(a) ( ((AC0ACTEXT_OUT_FORCELOADSHED_BITMASK) & (a)) >> 6 )
#define AC0ACTEXT_OUT_FORCELOADSHED_SET(a,b) (a) = ( (~(AC0ACTEXT_OUT_FORCELOADSHED_BITMASK) & (a)) | ( ((b) & 1) << 6) )

typedef enum AC0TEST_ENUM
{
    AC0TEST_NOTEST = 0,
    AC0TEST_LINTEST = 1,
} AC0TEST_ENUM;

typedef enum AC0OPERST_ENUM
{
    AC0OPERST_FANONLY = 0,
    AC0OPERST_COOL = 1,
    AC0OPERST_HEAT = 2,
} AC0OPERST_ENUM;

typedef enum AC0FLAPS_ENUM
{
    AC0FLAPS_FLAPS_STOPPED = 0,
    AC0FLAPS_FLAP2_ACTIVE = 3,
    AC0FLAPS_FLAP1_ACTIVE = 12,
    AC0FLAPS_FLAPS_ACTIVE = 15,
} AC0FLAPS_ENUM;

typedef enum AC0HMODE_ENUM
{
    AC0HMODE_AUTO = 0,
    AC0HMODE_PRIMARY_ONLY = 1,
    AC0HMODE_AUXHEAT_ONLY = 2,
} AC0HMODE_ENUM;

typedef enum AC0FEATURE_BITMASK_ENUM
{
    AC0FEATURE_HEATPUMP_BITMASK = 1 << 0, // 1
    AC0FEATURE_HEATSTRIP_BITMASK = 1 << 1, // 2
    AC0FEATURE_FURNACE_BITMASK = 1 << 2, // 4
    AC0FEATURE_DEHUMIDIFY_BITMASK = 1 << 3, // 8
    AC0FEATURE_AGS_BITMASK = 1 << 4, // 0x10
    AC0FEATURE_STAGE_DUAL_BASEMENT_ON_SINGLE_POWER_BOARD_BITMASK = 1 << 5, // 0x20
    AC0FEATURE_EXTSTAGE_DUAL_AC_UNITS_IN_SAME_ZONE_BITMASK = 1 << 6, // 0x40
    AC0FEATURE_OUTTEMP_BITMASK = 1 << 7, // 0x80
    AC0FEATURE_CURRENTLIMIT_BITMASK = 1 << 8, // 0x100
} AC0FEATURE_BITMASK_ENUM;

typedef struct AC0FEATURE_T
{
    union
    {
        struct
        {
            uint32_t heatpump : 1;
            uint32_t heatstrip : 1;
            uint32_t furnace : 1;
            uint32_t dehumidify : 1;
            uint32_t ags : 1;
            uint32_t stage_dual_basement_on_single_power_board : 1;
            uint32_t extstage_dual_ac_units_in_same_zone : 1;
            uint32_t outtemp : 1;
            uint32_t currentlimit : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED AC0FEATURE_T;


#define AC0FEATURE_HEATPUMP_GET(a) ( ((AC0FEATURE_HEATPUMP_BITMASK) & (a)) >> 0 )
#define AC0FEATURE_HEATPUMP_SET(a,b) (a) = ( (~(AC0FEATURE_HEATPUMP_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define AC0FEATURE_HEATSTRIP_GET(a) ( ((AC0FEATURE_HEATSTRIP_BITMASK) & (a)) >> 1 )
#define AC0FEATURE_HEATSTRIP_SET(a,b) (a) = ( (~(AC0FEATURE_HEATSTRIP_BITMASK) & (a)) | ( ((b) & 1) << 1) )
#define AC0FEATURE_FURNACE_GET(a) ( ((AC0FEATURE_FURNACE_BITMASK) & (a)) >> 2 )
#define AC0FEATURE_FURNACE_SET(a,b) (a) = ( (~(AC0FEATURE_FURNACE_BITMASK) & (a)) | ( ((b) & 1) << 2) )
#define AC0FEATURE_DEHUMIDIFY_GET(a) ( ((AC0FEATURE_DEHUMIDIFY_BITMASK) & (a)) >> 3 )
#define AC0FEATURE_DEHUMIDIFY_SET(a,b) (a) = ( (~(AC0FEATURE_DEHUMIDIFY_BITMASK) & (a)) | ( ((b) & 1) << 3) )
#define AC0FEATURE_AGS_GET(a) ( ((AC0FEATURE_AGS_BITMASK) & (a)) >> 4 )
#define AC0FEATURE_AGS_SET(a,b) (a) = ( (~(AC0FEATURE_AGS_BITMASK) & (a)) | ( ((b) & 1) << 4) )
#define AC0FEATURE_STAGE_DUAL_BASEMENT_ON_SINGLE_POWER_BOARD_GET(a) ( ((AC0FEATURE_STAGE_DUAL_BASEMENT_ON_SINGLE_POWER_BOARD_BITMASK) & (a)) >> 5 )
#define AC0FEATURE_STAGE_DUAL_BASEMENT_ON_SINGLE_POWER_BOARD_SET(a,b) (a) = ( (~(AC0FEATURE_STAGE_DUAL_BASEMENT_ON_SINGLE_POWER_BOARD_BITMASK) & (a)) | ( ((b) & 1) << 5) )
#define AC0FEATURE_EXTSTAGE_DUAL_AC_UNITS_IN_SAME_ZONE_GET(a) ( ((AC0FEATURE_EXTSTAGE_DUAL_AC_UNITS_IN_SAME_ZONE_BITMASK) & (a)) >> 6 )
#define AC0FEATURE_EXTSTAGE_DUAL_AC_UNITS_IN_SAME_ZONE_SET(a,b) (a) = ( (~(AC0FEATURE_EXTSTAGE_DUAL_AC_UNITS_IN_SAME_ZONE_BITMASK) & (a)) | ( ((b) & 1) << 6) )
#define AC0FEATURE_OUTTEMP_GET(a) ( ((AC0FEATURE_OUTTEMP_BITMASK) & (a)) >> 7 )
#define AC0FEATURE_OUTTEMP_SET(a,b) (a) = ( (~(AC0FEATURE_OUTTEMP_BITMASK) & (a)) | ( ((b) & 1) << 7) )
#define AC0FEATURE_CURRENTLIMIT_GET(a) ( ((AC0FEATURE_CURRENTLIMIT_BITMASK) & (a)) >> 8 )
#define AC0FEATURE_CURRENTLIMIT_SET(a,b) (a) = ( (~(AC0FEATURE_CURRENTLIMIT_BITMASK) & (a)) | ( ((b) & 1) << 8) )

typedef enum AC0FILTER_IN_ENUM
{
    AC0FILTER_IN_RESET = 2,
} AC0FILTER_IN_ENUM;

typedef enum AC0FILTER_OUT_ENUM
{
    AC0FILTER_OUT_COUNTING = 0,
    AC0FILTER_OUT_TIMEOUT = 1,
    AC0FILTER_OUT_UNKNOWN = 3,
} AC0FILTER_OUT_ENUM;

typedef enum AC0CURRLIM_ENUM
{
    AC0CURRLIM_4A = 0,
    AC0CURRLIM_5A = 1,
    AC0CURRLIM_6A = 2,
    AC0CURRLIM_7A = 3,
    AC0CURRLIM_UNLIMITED = 7,
} AC0CURRLIM_ENUM;

typedef enum SYS0LOGO_ENUM
{
    SYS0LOGO_OE0 = 0,
    SYS0LOGO_OE1 = 1,
    SYS0LOGO_OE2 = 2,
    SYS0LOGO_OE3 = 3,
} SYS0LOGO_ENUM;

typedef enum HTR0WTRTEMP_ENUM
{
    HTR0WTRTEMP_OFF = 0,
    HTR0WTRTEMP_ECO = 1,
    HTR0WTRTEMP_HOT = 2,
    HTR0WTRTEMP_BOOST = 3,
} HTR0WTRTEMP_ENUM;

typedef enum HTR0ESEL_ENUM
{
    HTR0ESEL_GAS = 0,
    HTR0ESEL_GAS_PLUSEL_900_W = 1,
    HTR0ESEL_GAS_PLUSEL_1800_W = 2,
    HTR0ESEL_EL_900_W = 3,
    HTR0ESEL_EL_1800_W = 4,
} HTR0ESEL_ENUM;

typedef enum HTR0MDL_ENUM
{
    HTR0MDL_NO_THERME_ID_0X3A_FOUND = 0,
    HTR0MDL_TRUMA_COMBI_GAS_6 = 1,
    HTR0MDL_TRUMA_COMBI_GAS_4 = 2,
    HTR0MDL_TRUMA_COMBI_GAS_6_E = 3,
    HTR0MDL_TRUMA_COMBI_GAS_4_E = 4,
    HTR0MDL_TRUMA_COMBI_GAS_2 = 5,
    HTR0MDL_TRUMA_COMBI_GAS_2_E = 6,
    HTR0MDL_TRUMA_FID_0113___UNKNOWN = 7,
    HTR0MDL_TRUMA_COMBI_D_6 = 8,
    HTR0MDL_TRUMA_COMBI_D_4 = 9,
    HTR0MDL_TRUMA_COMBI_D_6_E = 10,
    HTR0MDL_TRUMA_COMBI_D_4_E = 11,
    HTR0MDL_TRUMA_FID_1013___UNKNOWN = 12,
    HTR0MDL_TRUMA_VARIOHEAT_2_4_KW = 13,
    HTR0MDL_TRUMA_VARIOHEAT_3_7_KW = 14,
    HTR0MDL_TRUMA_FID_2016___UNKNOWN = 15,
    HTR0MDL_ALDE_3020 = 16,
    HTR0MDL_ULTRA_HEATER = 17,
    HTR0MDL_SHARC_CH6000 = 18,
    HTR0MDL_SHARC_CH6000E = 19,
    HTR0MDL_SHARC_CH4000 = 20,
    HTR0MDL_SHARC_CH4000E = 21,
} HTR0MDL_ENUM;

typedef enum HTR0EL_ENUM
{
    HTR0EL_OFF = 0,
    HTR0EL_0_5KW = 1,
    HTR0EL_1KW = 2,
    HTR0EL_2KW = 3,
    HTR0EL_3KW = 4,
} HTR0EL_ENUM;

typedef enum HTR0AMD_ENUM
{
    HTR0AMD_AUTO = 0,
    HTR0AMD_SILENT = 1,
    HTR0AMD_VENTILATION = 2,
} HTR0AMD_ENUM;

typedef enum FANSPEED_ENUM
{
    FANSPEED_LEVEL0 = 0,
    FANSPEED_LEVEL1 = 1,
    FANSPEED_LEVEL2 = 2,
    FANSPEED_LEVEL3 = 3,
    FANSPEED_LEVEL4 = 4,
    FANSPEED_LEVEL5 = 5,
    FANSPEED_LEVEL6 = 6,
    FANSPEED_LEVEL7 = 7,
} FANSPEED_ENUM;

typedef enum HTR0ERRST_ENUM
{
    HTR0ERRST_NO_ERRORS = 0,
    HTR0ERRST_WARNING_FAULT = 1,
    HTR0ERRST_CRITICAL_FAULT = 2,
} HTR0ERRST_ENUM;

typedef enum HTR0WTRTS_ENUM
{
    HTR0WTRTS_COLD = 0,
    HTR0WTRTS_WARM = 1,
    HTR0WTRTS_HOT = 2,
} HTR0WTRTS_ENUM;

typedef enum FRG0MD_ENUM
{
    FRG0MD_AUTO = 0,
    FRG0MD_MANUAL_AC = 1,
    FRG0MD_MANUAL_DC = 2,
    FRG0MD_MANUAL_GAS = 3,
} FRG0MD_ENUM;

typedef enum SATKAT0MDL_ENUM
{
    SATKAT0MDL_NO_DEVICE_ID_0X2F_FOUND = 0,
    SATKAT0MDL_KATHREIN_CAP_650 = 1,
    SATKAT0MDL_KATHREIN_CAP_750 = 2,
    SATKAT0MDL_KATHREIN_CAP_950 = 3,
    SATKAT0MDL_UNKNOWN = 4,
} SATKAT0MDL_ENUM;

typedef enum SATKAT0STS_ENUM
{
    SATKAT0STS_IDLE = 0x00,
    SATKAT0STS_STANDBY = 0x10,
    SATKAT0STS_BOOTING = 0x20,
    SATKAT0STS_PARK = 0x30,
    SATKAT0STS_SATELLITE_OK = 0x40,
    SATKAT0STS_SEARCHING = 0x50,
    SATKAT0STS_MOVING = 0x60,
    SATKAT0STS_RESET = 0x70,
    SATKAT0STS_UPDATE = 0x80,
    SATKAT0STS_OPTIMIZE = 0xa0,
    SATKAT0STS_ERROR = 0xf0,
} SATKAT0STS_ENUM;

typedef enum SATKAT0CMD_ENUM
{
    SATKAT0CMD_WAKEUP = 0x01,
    SATKAT0CMD_STANDBY = 0x02,
    SATKAT0CMD_RESET = 0x03,
    SATKAT0CMD_PARK = 0x04,
    SATKAT0CMD_LAST_SAT = 0x05,
    SATKAT0CMD_MOVE_SAT = 0x06,
    SATKAT0CMD_SET_GPS = 0x07,
    SATKAT0CMD_OK_KEY = 0x09,
    SATKAT0CMD_STOP_KEY = 0x0a,
} SATKAT0CMD_ENUM;

typedef enum SATKAT0OPOS_ENUM
{
    SATKAT0OPOS_19_2E_ASTRA = 192,
    SATKAT0OPOS_13_0E_EUTELSAT = 130,
    SATKAT0OPOS_23_5E_ASTRA = 235,
    SATKAT0OPOS_28_2E_ASTRA = 282,
    SATKAT0OPOS_5_0E_SIRIUS = 50,
    SATKAT0OPOS_9_0E_EUROBIRD = 90,
    SATKAT0OPOS_16_0E_EUTELSAT = 160,
    SATKAT0OPOS_42_0E_TURKSAT = 420,
    SATKAT0OPOS_30_0W_HISPASAT = 3300,
    SATKAT0OPOS_3_0W_ATLANTIC_BILD = 3550,
    SATKAT0OPOS_0_8W_THOR = 3590,
} SATKAT0OPOS_ENUM;

typedef enum HD0PTYPE_ENUM
{
    HD0PTYPE_QR = 0,
    HD0PTYPE_PASSIVE = 1,
    HD0PTYPE_ACTIVE = 2,
} HD0PTYPE_ENUM;

typedef enum HD0TEMPCM_ENUM
{
    HD0TEMPCM_OFF = 0,
    HD0TEMPCM_COOL = 1,
    HD0TEMPCM_HEAT = 2,
    HD0TEMPCM_AUTO = 3,
} HD0TEMPCM_ENUM;

typedef enum HD0BATPROT_ENUM
{
    HD0BATPROT_LOW = 0,
    HD0BATPROT_MED = 1,
    HD0BATPROT_HIGH = 2,
} HD0BATPROT_ENUM;

typedef enum WIFI0STS_ENUM
{
    WIFI0STS_UNKNOWN = 0,
    WIFI0STS_DISCONNECTED = 1,
    WIFI0STS_CONNECTING = 2,
    WIFI0STS_CONNECTED = 3,
    WIFI0STS_DISCONNECTED_RETRYING = 4,
} WIFI0STS_ENUM;

typedef enum WIFI0EV_ENUM
{
    WIFI0EV_UNKNOWN = 0,
    WIFI0EV_STA_STARTED = 1,
    WIFI0EV_STA_CONNECTED = 2,
    WIFI0EV_STA_DISCONNECTED = 3,
    WIFI0EV_SCAN_COMPLETED = 4,
    WIFI0EV_STA_DISCONN_WRONG_PASSWORD = 5,
    WIFI0EV_STA_DISCONN_NO_AP_FOUND = 6,
    WIFI0EV_AP_STARTED = 8,
    WIFI0EV_AP_STA_CONNECTED = 9,
    WIFI0EV_AP_STA_DISCONNECTED = 10,
    WIFI0EV_WPS_SUCCESS = 16,
    WIFI0EV_WPS_FAILED = 17,
    WIFI0EV_WPS_TIMEOUT = 18,
    WIFI0EV_WPS_PIN = 19,
} WIFI0EV_ENUM;

typedef enum WIFI0MD_ENUM
{
    WIFI0MD_NONE = 0,
    WIFI0MD_STA = 1,
    WIFI0MD_AP = 2,
    WIFI0MD_STA_AP = 3,
} WIFI0MD_ENUM;

typedef enum BT0PAIR_IN_ENUM
{
    BT0PAIR_IN_PAIRING_MODE_OFF = 0,
    BT0PAIR_IN_PAIRING_MODE_ON = 1,
    BT0PAIR_IN_CLEAR_BONDS = 3,
} BT0PAIR_IN_ENUM;

typedef enum BT0PAIR_OUT_ENUM
{
    BT0PAIR_OUT_PAIRING_MODE_INACTIVE = 0,
    BT0PAIR_OUT_PAIRING_MODE_ACTIVE = 1,
    BT0PAIR_OUT_DEVICE_PAIRED = 2,
    BT0PAIR_OUT_PAIRING_MODE_TIMEOUT = 3,
    BT0PAIR_OUT_DEVICE_PAIR_FAILED = 4,
} BT0PAIR_OUT_ENUM;

typedef enum BTWL0ADDRTP_ENUM
{
    BTWL0ADDRTP_PUBLIC = 0,
    BTWL0ADDRTP_RANDOM = 1,
    BTWL0ADDRTP_RESOLVABLE_PUBLIC = 2,
    BTWL0ADDRTP_RESOLVABLE_PRIVATE = 3,
} BTWL0ADDRTP_ENUM;

typedef enum BTWL0NODEI_ENUM
{
    BTWL0NODEI_UNCONNECTED = -1,
    BTWL0NODEI_CONNECTING = -2,
    BTWL0NODEI_CONNECT_FAILED = -3,
    BTWL0NODEI_DISCONNECTED = -4,
    BTWL0NODEI_AUTHENTICATION_ERROR = -5,
    BTWL0NODEI_INTERNAL_ERROR = -6,
    BTWL0NODEI_COMMUNICATION_ERROR = -7,
} BTWL0NODEI_ENUM;

typedef enum MQTT0STAT_ENUM
{
    MQTT0STAT_NOT_CONNECTED = 0,
    MQTT0STAT_PUBLISH_OK = 1,
    MQTT0STAT_RECONNECTING = 2,
    MQTT0STAT_RECONNECT_PUB_ROOT_OK = 3,
    MQTT0STAT_CONNECTED = 4,
    MQTT0STAT_CONNECTED_PUB_ROOT_OK = 5,
    MQTT0STAT_PUBLISH_FAIL = 8,
    MQTT0STAT_PUBLISH_FAIL_CONNECTED = 12,
    MQTT0STAT_PUBLISH_FAIL_CONNECTED_PUB_ROOT_OK = 13,
    MQTT0STAT_CONNECT_FAIL = 16,
} MQTT0STAT_ENUM;

typedef enum MB0ERRST_ENUM
{
    MB0ERRST_NO_ERRORS = 0,
    MB0ERRST_CUC_FAULT = 0x01,
    MB0ERRST_HEATING_ELEMENT_FAULT = 0x02,
    MB0ERRST_PELTIER_ELEMENT_FAULT = 0x04,
    MB0ERRST_FAN_SPEED_ERROR = 0x08,
    MB0ERRST_NTC1_SENSOR_ERROR = 0x10,
    MB0ERRST_NTC2_SENSOR_ERROR = 0x20,
} MB0ERRST_ENUM;

typedef enum NRX0MODE_ENUM
{
    NRX0MODE_PERFORMANCE_MODE = 0,
    NRX0MODE_QUIET_MODE = 1,
    NRX0MODE_ECO_MODE = 2,
    NRX0MODE_FREEZER_MODE = 3,
} NRX0MODE_ENUM;

typedef enum NRX0LEVEL_ENUM
{
    NRX0LEVEL_LEVEL1 = 0,
    NRX0LEVEL_LEVEL2 = 1,
    NRX0LEVEL_LEVEL3 = 2,
    NRX0LEVEL_LEVEL4 = 3,
    NRX0LEVEL_LEVEL5 = 4,
} NRX0LEVEL_ENUM;

typedef enum NRX0ERRST_ENUM
{
    NRX0ERRST_NO_ERRORS = 0,
    NRX0ERRST_BATTERY_VOLTAGE_FAULT = 0x01,
    NRX0ERRST_FAN_OVERCURRENT_PROTECTION = 0x02,
    NRX0ERRST_COMPRESSOR_STARTUP_FAILURE = 0x04,
    NRX0ERRST_COMPRESSOR_STALL_PROTECTION = 0x08,
    NRX0ERRST_CONTROLLER_OVERHEAT_PROTECTION = 0x10,
    NRX0ERRST_OPEN_DOOR_TIMEOUT_FAULT = 0x20,
    NRX0ERRST_NTC1_SENSOR_ERROR = 0x40,
} NRX0ERRST_ENUM;

typedef enum TH0FAV_ENUM
{
    TH0FAV_A = 0,
    TH0FAV_B = 1,
    TH0FAV_C = 2,
    TH0FAV_D = 3,
    TH0FAV_E = 4,
    TH0FAV_F = 5,
} TH0FAV_ENUM;

typedef enum IV0MODE_ENUM
{
    IV0MODE_OFF = 0,
    IV0MODE_AUTO = 1,
    IV0MODE_TURBO = 2,
    IV0MODE_SLEEP = 3,
} IV0MODE_ENUM;

typedef enum IV0PWRON_ENUM
{
    IV0PWRON_OFF = 0,
    IV0PWRON_ON = 1,
} IV0PWRON_ENUM;

typedef enum IV0FILST_ENUM
{
    IV0FILST_FILTER_CHANGE_NOT_REQ = 0,
    IV0FILST_FILTER_CHANGE_REQ = 1,
    IV0FILST_FILTER_RESET = 2,
} IV0FILST_ENUM;

typedef enum IV0STORAGE_ENUM
{
    IV0STORAGE_DEACTIVATE = 0,
    IV0STORAGE_ACTIVATE = 1,
} IV0STORAGE_ENUM;

typedef enum IV0ERRST_BITMASK_ENUM
{
    IV0ERRST_VOC_SENSOR_COMMUNICATION_ERROR_BITMASK = 1 << 0, // 1
    IV0ERRST_VOC_SENSOR_DATA_PLAUSIBLE_ERROR_BITMASK = 1 << 1, // 2
    IV0ERRST_DP_SENSOR_BOARD_DISCONNECTED_BITMASK = 1 << 2, // 4
    IV0ERRST_DP_SENSOR_DATA_PLAUSIBLE_ERROR_BITMASK = 1 << 4, // 0x10
    IV0ERRST_DP_SENSOR_BOARD_BATTERY_LOW_BITMASK = 1 << 5, // 0x20
    IV0ERRST_FAN1_RPM_MISMATCH_BITMASK = 1 << 7, // 0x80
    IV0ERRST_FAN1_TACHO_READ_DEVICE_INACTIVE_BITMASK = 1 << 8, // 0x100
    IV0ERRST_FAN1_NO_TACHO_DEVICE_ACTIVE_BITMASK = 1 << 9, // 0x200
    IV0ERRST_FAN2_RPM_MISMATCH_BITMASK = 1 << 10, // 0x400
    IV0ERRST_FAN2_TACHO_READ_DEVICE_INACTIVE_BITMASK = 1 << 11, // 0x800
    IV0ERRST_FAN2_NO_TACHO_DEVICE_ACTIVE_BITMASK = 1 << 12, // 0x1000
    IV0ERRST_MOTOR_RPM_MISMATCH_BITMASK = 1 << 13, // 0x2000
    IV0ERRST_MOTOR_TACHO_READ_DEVICE_INACTIVE_BITMASK = 1 << 14, // 0x4000
    IV0ERRST_MOTOR_NO_TACHO_DEVICE_ACTIVE_BITMASK = 1 << 15, // 0x8000
    IV0ERRST_BACKUP_BATTERY_LOW_BITMASK = 1 << 16, // 0x10000
    IV0ERRST_BACKUP_BATTERY_LOW_STANDBY_BITMASK = 1 << 17, // 0x20000
    IV0ERRST_COMM_ERROR_WITH_BATTERY_IC_BITMASK = 1 << 19, // 0x80000
    IV0ERRST_BATTERY_OVER_HEATING_BITMASK = 1 << 20, // 0x100000
    IV0ERRST_BATTERY_EXPIRED_BITMASK = 1 << 21, // 0x200000
    IV0ERRST_COMM_ERROR_WITH_LCD_DRIVER_IC_BITMASK = 1 << 22, // 0x400000
    IV0ERRST_TOUCH_BUTTON_EVENT_PROCESS_ERROR_BITMASK = 1 << 23, // 0x800000
    IV0ERRST_CAN_COMMUNICATION_ERROR_BITMASK = 1 << 24, // 0x1000000
    IV0ERRST_LIN_COMMUNICATION_ERROR_BITMASK = 1 << 25, // 0x2000000
} IV0ERRST_BITMASK_ENUM;

typedef struct IV0ERRST_T
{
    union
    {
        struct
        {
            uint32_t voc_sensor_communication_error : 1;
            uint32_t voc_sensor_data_plausible_error : 1;
            uint32_t dp_sensor_board_disconnected : 1;
            uint32_t unused_3 : 1;
            uint32_t dp_sensor_data_plausible_error : 1;
            uint32_t dp_sensor_board_battery_low : 1;
            uint32_t unused_6 : 1;
            uint32_t fan1_rpm_mismatch : 1;
            uint32_t fan1_tacho_read_device_inactive : 1;
            uint32_t fan1_no_tacho_device_active : 1;
            uint32_t fan2_rpm_mismatch : 1;
            uint32_t fan2_tacho_read_device_inactive : 1;
            uint32_t fan2_no_tacho_device_active : 1;
            uint32_t motor_rpm_mismatch : 1;
            uint32_t motor_tacho_read_device_inactive : 1;
            uint32_t motor_no_tacho_device_active : 1;
            uint32_t backup_battery_low : 1;
            uint32_t backup_battery_low_standby : 1;
            uint32_t unused_18 : 1;
            uint32_t comm_error_with_battery_ic : 1;
            uint32_t battery_over_heating : 1;
            uint32_t battery_expired : 1;
            uint32_t comm_error_with_lcd_driver_ic : 1;
            uint32_t touch_button_event_process_error : 1;
            uint32_t can_communication_error : 1;
            uint32_t lin_communication_error : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED IV0ERRST_T;


#define IV0ERRST_VOC_SENSOR_COMMUNICATION_ERROR_GET(a) ( ((IV0ERRST_VOC_SENSOR_COMMUNICATION_ERROR_BITMASK) & (a)) >> 0 )
#define IV0ERRST_VOC_SENSOR_COMMUNICATION_ERROR_SET(a,b) (a) = ( (~(IV0ERRST_VOC_SENSOR_COMMUNICATION_ERROR_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define IV0ERRST_VOC_SENSOR_DATA_PLAUSIBLE_ERROR_GET(a) ( ((IV0ERRST_VOC_SENSOR_DATA_PLAUSIBLE_ERROR_BITMASK) & (a)) >> 1 )
#define IV0ERRST_VOC_SENSOR_DATA_PLAUSIBLE_ERROR_SET(a,b) (a) = ( (~(IV0ERRST_VOC_SENSOR_DATA_PLAUSIBLE_ERROR_BITMASK) & (a)) | ( ((b) & 1) << 1) )
#define IV0ERRST_DP_SENSOR_BOARD_DISCONNECTED_GET(a) ( ((IV0ERRST_DP_SENSOR_BOARD_DISCONNECTED_BITMASK) & (a)) >> 2 )
#define IV0ERRST_DP_SENSOR_BOARD_DISCONNECTED_SET(a,b) (a) = ( (~(IV0ERRST_DP_SENSOR_BOARD_DISCONNECTED_BITMASK) & (a)) | ( ((b) & 1) << 2) )
#define IV0ERRST_DP_SENSOR_DATA_PLAUSIBLE_ERROR_GET(a) ( ((IV0ERRST_DP_SENSOR_DATA_PLAUSIBLE_ERROR_BITMASK) & (a)) >> 4 )
#define IV0ERRST_DP_SENSOR_DATA_PLAUSIBLE_ERROR_SET(a,b) (a) = ( (~(IV0ERRST_DP_SENSOR_DATA_PLAUSIBLE_ERROR_BITMASK) & (a)) | ( ((b) & 1) << 4) )
#define IV0ERRST_DP_SENSOR_BOARD_BATTERY_LOW_GET(a) ( ((IV0ERRST_DP_SENSOR_BOARD_BATTERY_LOW_BITMASK) & (a)) >> 5 )
#define IV0ERRST_DP_SENSOR_BOARD_BATTERY_LOW_SET(a,b) (a) = ( (~(IV0ERRST_DP_SENSOR_BOARD_BATTERY_LOW_BITMASK) & (a)) | ( ((b) & 1) << 5) )
#define IV0ERRST_FAN1_RPM_MISMATCH_GET(a) ( ((IV0ERRST_FAN1_RPM_MISMATCH_BITMASK) & (a)) >> 7 )
#define IV0ERRST_FAN1_RPM_MISMATCH_SET(a,b) (a) = ( (~(IV0ERRST_FAN1_RPM_MISMATCH_BITMASK) & (a)) | ( ((b) & 1) << 7) )
#define IV0ERRST_FAN1_TACHO_READ_DEVICE_INACTIVE_GET(a) ( ((IV0ERRST_FAN1_TACHO_READ_DEVICE_INACTIVE_BITMASK) & (a)) >> 8 )
#define IV0ERRST_FAN1_TACHO_READ_DEVICE_INACTIVE_SET(a,b) (a) = ( (~(IV0ERRST_FAN1_TACHO_READ_DEVICE_INACTIVE_BITMASK) & (a)) | ( ((b) & 1) << 8) )
#define IV0ERRST_FAN1_NO_TACHO_DEVICE_ACTIVE_GET(a) ( ((IV0ERRST_FAN1_NO_TACHO_DEVICE_ACTIVE_BITMASK) & (a)) >> 9 )
#define IV0ERRST_FAN1_NO_TACHO_DEVICE_ACTIVE_SET(a,b) (a) = ( (~(IV0ERRST_FAN1_NO_TACHO_DEVICE_ACTIVE_BITMASK) & (a)) | ( ((b) & 1) << 9) )
#define IV0ERRST_FAN2_RPM_MISMATCH_GET(a) ( ((IV0ERRST_FAN2_RPM_MISMATCH_BITMASK) & (a)) >> 10 )
#define IV0ERRST_FAN2_RPM_MISMATCH_SET(a,b) (a) = ( (~(IV0ERRST_FAN2_RPM_MISMATCH_BITMASK) & (a)) | ( ((b) & 1) << 10) )
#define IV0ERRST_FAN2_TACHO_READ_DEVICE_INACTIVE_GET(a) ( ((IV0ERRST_FAN2_TACHO_READ_DEVICE_INACTIVE_BITMASK) & (a)) >> 11 )
#define IV0ERRST_FAN2_TACHO_READ_DEVICE_INACTIVE_SET(a,b) (a) = ( (~(IV0ERRST_FAN2_TACHO_READ_DEVICE_INACTIVE_BITMASK) & (a)) | ( ((b) & 1) << 11) )
#define IV0ERRST_FAN2_NO_TACHO_DEVICE_ACTIVE_GET(a) ( ((IV0ERRST_FAN2_NO_TACHO_DEVICE_ACTIVE_BITMASK) & (a)) >> 12 )
#define IV0ERRST_FAN2_NO_TACHO_DEVICE_ACTIVE_SET(a,b) (a) = ( (~(IV0ERRST_FAN2_NO_TACHO_DEVICE_ACTIVE_BITMASK) & (a)) | ( ((b) & 1) << 12) )
#define IV0ERRST_MOTOR_RPM_MISMATCH_GET(a) ( ((IV0ERRST_MOTOR_RPM_MISMATCH_BITMASK) & (a)) >> 13 )
#define IV0ERRST_MOTOR_RPM_MISMATCH_SET(a,b) (a) = ( (~(IV0ERRST_MOTOR_RPM_MISMATCH_BITMASK) & (a)) | ( ((b) & 1) << 13) )
#define IV0ERRST_MOTOR_TACHO_READ_DEVICE_INACTIVE_GET(a) ( ((IV0ERRST_MOTOR_TACHO_READ_DEVICE_INACTIVE_BITMASK) & (a)) >> 14 )
#define IV0ERRST_MOTOR_TACHO_READ_DEVICE_INACTIVE_SET(a,b) (a) = ( (~(IV0ERRST_MOTOR_TACHO_READ_DEVICE_INACTIVE_BITMASK) & (a)) | ( ((b) & 1) << 14) )
#define IV0ERRST_MOTOR_NO_TACHO_DEVICE_ACTIVE_GET(a) ( ((IV0ERRST_MOTOR_NO_TACHO_DEVICE_ACTIVE_BITMASK) & (a)) >> 15 )
#define IV0ERRST_MOTOR_NO_TACHO_DEVICE_ACTIVE_SET(a,b) (a) = ( (~(IV0ERRST_MOTOR_NO_TACHO_DEVICE_ACTIVE_BITMASK) & (a)) | ( ((b) & 1) << 15) )
#define IV0ERRST_BACKUP_BATTERY_LOW_GET(a) ( ((IV0ERRST_BACKUP_BATTERY_LOW_BITMASK) & (a)) >> 16 )
#define IV0ERRST_BACKUP_BATTERY_LOW_SET(a,b) (a) = ( (~(IV0ERRST_BACKUP_BATTERY_LOW_BITMASK) & (a)) | ( ((b) & 1) << 16) )
#define IV0ERRST_BACKUP_BATTERY_LOW_STANDBY_GET(a) ( ((IV0ERRST_BACKUP_BATTERY_LOW_STANDBY_BITMASK) & (a)) >> 17 )
#define IV0ERRST_BACKUP_BATTERY_LOW_STANDBY_SET(a,b) (a) = ( (~(IV0ERRST_BACKUP_BATTERY_LOW_STANDBY_BITMASK) & (a)) | ( ((b) & 1) << 17) )
#define IV0ERRST_COMM_ERROR_WITH_BATTERY_IC_GET(a) ( ((IV0ERRST_COMM_ERROR_WITH_BATTERY_IC_BITMASK) & (a)) >> 19 )
#define IV0ERRST_COMM_ERROR_WITH_BATTERY_IC_SET(a,b) (a) = ( (~(IV0ERRST_COMM_ERROR_WITH_BATTERY_IC_BITMASK) & (a)) | ( ((b) & 1) << 19) )
#define IV0ERRST_BATTERY_OVER_HEATING_GET(a) ( ((IV0ERRST_BATTERY_OVER_HEATING_BITMASK) & (a)) >> 20 )
#define IV0ERRST_BATTERY_OVER_HEATING_SET(a,b) (a) = ( (~(IV0ERRST_BATTERY_OVER_HEATING_BITMASK) & (a)) | ( ((b) & 1) << 20) )
#define IV0ERRST_BATTERY_EXPIRED_GET(a) ( ((IV0ERRST_BATTERY_EXPIRED_BITMASK) & (a)) >> 21 )
#define IV0ERRST_BATTERY_EXPIRED_SET(a,b) (a) = ( (~(IV0ERRST_BATTERY_EXPIRED_BITMASK) & (a)) | ( ((b) & 1) << 21) )
#define IV0ERRST_COMM_ERROR_WITH_LCD_DRIVER_IC_GET(a) ( ((IV0ERRST_COMM_ERROR_WITH_LCD_DRIVER_IC_BITMASK) & (a)) >> 22 )
#define IV0ERRST_COMM_ERROR_WITH_LCD_DRIVER_IC_SET(a,b) (a) = ( (~(IV0ERRST_COMM_ERROR_WITH_LCD_DRIVER_IC_BITMASK) & (a)) | ( ((b) & 1) << 22) )
#define IV0ERRST_TOUCH_BUTTON_EVENT_PROCESS_ERROR_GET(a) ( ((IV0ERRST_TOUCH_BUTTON_EVENT_PROCESS_ERROR_BITMASK) & (a)) >> 23 )
#define IV0ERRST_TOUCH_BUTTON_EVENT_PROCESS_ERROR_SET(a,b) (a) = ( (~(IV0ERRST_TOUCH_BUTTON_EVENT_PROCESS_ERROR_BITMASK) & (a)) | ( ((b) & 1) << 23) )
#define IV0ERRST_CAN_COMMUNICATION_ERROR_GET(a) ( ((IV0ERRST_CAN_COMMUNICATION_ERROR_BITMASK) & (a)) >> 24 )
#define IV0ERRST_CAN_COMMUNICATION_ERROR_SET(a,b) (a) = ( (~(IV0ERRST_CAN_COMMUNICATION_ERROR_BITMASK) & (a)) | ( ((b) & 1) << 24) )
#define IV0ERRST_LIN_COMMUNICATION_ERROR_GET(a) ( ((IV0ERRST_LIN_COMMUNICATION_ERROR_BITMASK) & (a)) >> 25 )
#define IV0ERRST_LIN_COMMUNICATION_ERROR_SET(a,b) (a) = ( (~(IV0ERRST_LIN_COMMUNICATION_ERROR_BITMASK) & (a)) | ( ((b) & 1) << 25) )

typedef enum IV0WARN_ENUM
{
    IV0WARN_NO_WARNING = 0,
    IV0WARN_BATTERY_LOW = 1,
    IV0WARN_OVER_HEATING = 2,
    IV0WARN_OVER_VOLTAGE = 3,
    IV0WARN_OVER_CURRENT = 4,
} IV0WARN_ENUM;

typedef enum IV0PWRSRC_ENUM
{
    IV0PWRSRC_SOLAR_POWER_INPUT = 0,
    IV0PWRSRC_12V_CAR_BATTERY_INPUT = 1,
    IV0PWRSRC_BACKUP_BATTERY = 2,
} IV0PWRSRC_ENUM;

typedef enum IV0AQST_ENUM
{
    IV0AQST_AIR_QUALITY_GOOD = 0,
    IV0AQST_AIR_QUALITY_FAIR = 1,
    IV0AQST_AIR_QUALITY_BAD = 2,
    IV0AQST_CALIBRATION_ONGOING = 3,
    IV0AQST_AIR_QUALITY_UNKNOWN = 4,
} IV0AQST_ENUM;

typedef enum IV0PRST_ENUM
{
    IV0PRST_VALID_PRESS_LEVEL = 0,
    IV0PRST_OVER_PRESS = 1,
    IV0PRST_UNDER_PRESS = 2,
    IV0PRST_PRESS_STATUS_UNKNOWN = 3,
} IV0PRST_ENUM;

typedef enum IV0BLREQ_ENUM
{
    IV0BLREQ_IDLE = 0,
    IV0BLREQ_SCAN = 1,
    IV0BLREQ_PAIR = 2,
} IV0BLREQ_ENUM;

typedef enum IV0IONST_ENUM
{
    IV0IONST_OFF = 0,
    IV0IONST_ON = 1,
} IV0IONST_ENUM;

typedef enum IV0STGT_ENUM
{
    IV0STGT_IDLE_21HRS = 0,
    IV0STGT_RUN_3HRS = 1,
} IV0STGT_ENUM;

typedef enum IVPMGR0STATE_ENUM
{
    IVPMGR0STATE_STANDBY = 0,
    IVPMGR0STATE_ACTIVE = 1,
    IVPMGR0STATE_STORAGE = 2,
} IVPMGR0STATE_ENUM;

typedef enum TS0OMD_ENUM
{
    TS0OMD_OFF = 0,
    TS0OMD_COOL = 1,
    TS0OMD_HEAT = 2,
    TS0OMD_AUTO = 3,
    TS0OMD_VENT = 4,
    TS0OMD_AUX_HEAT = 5,
    TS0OMD_DEHUMIDIFY = 6,
} TS0OMD_ENUM;

typedef enum CZ0SMODE_ENUM
{
    CZ0SMODE_SLEEP = 0,
    CZ0SMODE_WAKE = 1,
    CZ0SMODE_AWAY = 2,
    CZ0SMODE_RETURN = 3,
    CZ0SMODE_STORAGE = 250,
} CZ0SMODE_ENUM;

typedef enum CZ0HMD_ENUM
{
    CZ0HMD_HEAT_PUMP_ONLY = 0,
    CZ0HMD_AUXILIARY_HEAT_ONLY = 1,
    CZ0HMD_AUTO = 2,
} CZ0HMD_ENUM;

typedef enum CZ0HPRIO_ENUM
{
    CZ0HPRIO_AUTO = 0,
    CZ0HPRIO_HEAT_PUMP = 1,
    CZ0HPRIO_FURNACE = 2,
    CZ0HPRIO_BOTH = 3,
} CZ0HPRIO_ENUM;

typedef enum CZ0FILIND_ENUM
{
    CZ0FILIND_FILTER_CHANGE_NOT_REQ = 0,
    CZ0FILIND_FILTER_CHANGE_REQ = 1,
    CZ0FILIND_FILTER_RESET = 2,
    CZ0FILIND_FILTER_DO_NOT_CHANGE = 3,
} CZ0FILIND_ENUM;

typedef enum MODESTATE_ENUM
{
    MODESTATE_OFF = 0,
    MODESTATE_WAITING_FOR_COMPRESSOR = 1,
    MODESTATE_COMPRESSOR_ON = 2,
    MODESTATE_LOAD_SHED = 3,
} MODESTATE_ENUM;

typedef enum CC0STS_BITMASK_ENUM
{
    CC0STS_COOL_BITMASK = 1 << 0, // 1
    CC0STS_HEAT_BITMASK = 1 << 1, // 2
    CC0STS_VENT_BITMASK = 1 << 2, // 4
    CC0STS_DRY_BITMASK = 1 << 3, // 8
    CC0STS_HUMID_BITMASK = 1 << 4, // 0x10
} CC0STS_BITMASK_ENUM;

typedef struct CC0STS_T
{
    union
    {
        struct
        {
            uint32_t cool : 1;
            uint32_t heat : 1;
            uint32_t vent : 1;
            uint32_t dry : 1;
            uint32_t humid : 1;
            uint32_t unused_5 : 1;
            uint32_t unused_6 : 1;
            uint32_t unused_7 : 1;
            uint32_t unused_8 : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED CC0STS_T;


#define CC0STS_COOL_GET(a) ( ((CC0STS_COOL_BITMASK) & (a)) >> 0 )
#define CC0STS_COOL_SET(a,b) (a) = ( (~(CC0STS_COOL_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define CC0STS_HEAT_GET(a) ( ((CC0STS_HEAT_BITMASK) & (a)) >> 1 )
#define CC0STS_HEAT_SET(a,b) (a) = ( (~(CC0STS_HEAT_BITMASK) & (a)) | ( ((b) & 1) << 1) )
#define CC0STS_VENT_GET(a) ( ((CC0STS_VENT_BITMASK) & (a)) >> 2 )
#define CC0STS_VENT_SET(a,b) (a) = ( (~(CC0STS_VENT_BITMASK) & (a)) | ( ((b) & 1) << 2) )
#define CC0STS_DRY_GET(a) ( ((CC0STS_DRY_BITMASK) & (a)) >> 3 )
#define CC0STS_DRY_SET(a,b) (a) = ( (~(CC0STS_DRY_BITMASK) & (a)) | ( ((b) & 1) << 3) )
#define CC0STS_HUMID_GET(a) ( ((CC0STS_HUMID_BITMASK) & (a)) >> 4 )
#define CC0STS_HUMID_SET(a,b) (a) = ( (~(CC0STS_HUMID_BITMASK) & (a)) | ( ((b) & 1) << 4) )

typedef enum CC0PCY_ENUM
{
    CC0PCY_COMFORT = 0,
    CC0PCY_AIR_QUALITY = 1,
    CC0PCY_ENERGY = 2,
    CC0PCY_QUIET = 3,
    CC0PCY_HUMIDITY = 4,
    CC0PCY_STORAGE = 5,
} CC0PCY_ENUM;

typedef enum CBS0SMODE_ENUM
{
    CBS0SMODE_SLEEP = 0,
    CBS0SMODE_WAKE = 1,
    CBS0SMODE_AWAY = 2,
    CBS0SMODE_RETURN = 3,
    CBS0SMODE_STORAGE = 250,
    CBS0SMODE_RESET_TO_CURRENT = 251,
} CBS0SMODE_ENUM;

typedef enum CBS0DAYS_BITMASK_ENUM
{
    CBS0DAYS_SUNDAY_BITMASK = 1 << 0, // 1
    CBS0DAYS_MONDAY_BITMASK = 1 << 1, // 2
    CBS0DAYS_TUESDAY_BITMASK = 1 << 2, // 4
    CBS0DAYS_WEDNESDAY_BITMASK = 1 << 3, // 8
    CBS0DAYS_THURSDAY_BITMASK = 1 << 4, // 0x10
    CBS0DAYS_FRIDAY_BITMASK = 1 << 5, // 0x20
    CBS0DAYS_SATURDAY_BITMASK = 1 << 6, // 0x40
} CBS0DAYS_BITMASK_ENUM;

typedef struct CBS0DAYS_T
{
    union
    {
        struct
        {
            uint32_t sunday : 1;
            uint32_t monday : 1;
            uint32_t tuesday : 1;
            uint32_t wednesday : 1;
            uint32_t thursday : 1;
            uint32_t friday : 1;
            uint32_t saturday : 1;
            uint32_t unused_7 : 1;
            uint32_t unused_8 : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED CBS0DAYS_T;


#define CBS0DAYS_SUNDAY_GET(a) ( ((CBS0DAYS_SUNDAY_BITMASK) & (a)) >> 0 )
#define CBS0DAYS_SUNDAY_SET(a,b) (a) = ( (~(CBS0DAYS_SUNDAY_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define CBS0DAYS_MONDAY_GET(a) ( ((CBS0DAYS_MONDAY_BITMASK) & (a)) >> 1 )
#define CBS0DAYS_MONDAY_SET(a,b) (a) = ( (~(CBS0DAYS_MONDAY_BITMASK) & (a)) | ( ((b) & 1) << 1) )
#define CBS0DAYS_TUESDAY_GET(a) ( ((CBS0DAYS_TUESDAY_BITMASK) & (a)) >> 2 )
#define CBS0DAYS_TUESDAY_SET(a,b) (a) = ( (~(CBS0DAYS_TUESDAY_BITMASK) & (a)) | ( ((b) & 1) << 2) )
#define CBS0DAYS_WEDNESDAY_GET(a) ( ((CBS0DAYS_WEDNESDAY_BITMASK) & (a)) >> 3 )
#define CBS0DAYS_WEDNESDAY_SET(a,b) (a) = ( (~(CBS0DAYS_WEDNESDAY_BITMASK) & (a)) | ( ((b) & 1) << 3) )
#define CBS0DAYS_THURSDAY_GET(a) ( ((CBS0DAYS_THURSDAY_BITMASK) & (a)) >> 4 )
#define CBS0DAYS_THURSDAY_SET(a,b) (a) = ( (~(CBS0DAYS_THURSDAY_BITMASK) & (a)) | ( ((b) & 1) << 4) )
#define CBS0DAYS_FRIDAY_GET(a) ( ((CBS0DAYS_FRIDAY_BITMASK) & (a)) >> 5 )
#define CBS0DAYS_FRIDAY_SET(a,b) (a) = ( (~(CBS0DAYS_FRIDAY_BITMASK) & (a)) | ( ((b) & 1) << 5) )
#define CBS0DAYS_SATURDAY_GET(a) ( ((CBS0DAYS_SATURDAY_BITMASK) & (a)) >> 6 )
#define CBS0DAYS_SATURDAY_SET(a,b) (a) = ( (~(CBS0DAYS_SATURDAY_BITMASK) & (a)) | ( ((b) & 1) << 6) )

typedef enum RFAN0SYST_ENUM
{
    RFAN0SYST_OFF = 0,
    RFAN0SYST_ON = 1,
} RFAN0SYST_ENUM;

typedef enum RFAN0FM_ENUM
{
    RFAN0FM_AUTO = 0,
    RFAN0FM_FORCED_ON = 1,
} RFAN0FM_ENUM;

typedef enum RFAN0SPDMD_ENUM
{
    RFAN0SPDMD_AUTO = 0,
    RFAN0SPDMD_MANUAL = 1,
} RFAN0SPDMD_ENUM;

typedef enum RFAN0WINDDRSW_ENUM
{
    RFAN0WINDDRSW_AIR_OUT = 0,
    RFAN0WINDDRSW_AIR_IN = 1,
} RFAN0WINDDRSW_ENUM;

typedef enum RFAN0DOMEPOS_ENUM
{
    RFAN0DOMEPOS_CLOSED = 0,
    RFAN0DOMEPOS_ONE_QUARTER_OPEN = 1,
    RFAN0DOMEPOS_HALF_OPEN = 2,
    RFAN0DOMEPOS_THREE_QUARTER_OPEN = 3,
    RFAN0DOMEPOS_OPEN = 4,
} RFAN0DOMEPOS_ENUM;

typedef enum RFAN0RAINSNS_ENUM
{
    RFAN0RAINSNS_USE_RAIN_SENSOR = 0,
    RFAN0RAINSNS_IGNORE_RAIN_SENSOR = 1,
} RFAN0RAINSNS_ENUM;

typedef enum RFAN0DMMODE_ENUM
{
    RFAN0DMMODE_STOP = 0,
    RFAN0DMMODE_OPEN = 1,
    RFAN0DMMODE_CLOSE = 2,
} RFAN0DMMODE_ENUM;

typedef enum RFAN0SETPCTLDM_ENUM
{
    RFAN0SETPCTLDM_NO_AUTOMATIC_DOME_CONTROL = 0,
    RFAN0SETPCTLDM_AUTOMATIC_DOME_CONTROL = 1,
} RFAN0SETPCTLDM_ENUM;

typedef enum RFAN0DMCLSFOFF_ENUM
{
    RFAN0DMCLSFOFF_LEAVE_DOME_OPEN_ON_FAN_OFF = 0,
    RFAN0DMCLSFOFF_AUTO_DOME_CLOSE_ON_FAN_OFF = 1,
} RFAN0DMCLSFOFF_ENUM;

typedef enum RFAN0FOFFDMCLS_ENUM
{
    RFAN0FOFFDMCLS_LEAVE_FAN_ON_ON_DOME_CLOSE = 0,
    RFAN0FOFFDMCLS_AUTO_FAN_OFF_ON_DOME_CLOSE = 1,
} RFAN0FOFFDMCLS_ENUM;

typedef enum RFAN0FSPDINCDEC_ENUM
{
    RFAN0FSPDINCDEC_DECREMENT_FAN_SPEED = 0,
    RFAN0FSPDINCDEC_INCREMENT_FAN_SPEED = 1,
} RFAN0FSPDINCDEC_ENUM;

typedef enum RFAN0RAINSNSSTS_ENUM
{
    RFAN0RAINSNSSTS_NO_RAIN_DETECTED = 0,
    RFAN0RAINSNSSTS_RAIN_DETECTED = 1,
    RFAN0RAINSNSSTS_SENSOR_ERROR = 2,
    RFAN0RAINSNSSTS_RAIN_SENSOR_NOT_INSTALLED = 3,
} RFAN0RAINSNSSTS_ENUM;

typedef enum DIM0CMD_ENUM
{
    DIM0CMD_SET_BRIGHTNESS = 0,
    DIM0CMD_ON = 1,
    DIM0CMD_ON_DELAY = 2,
    DIM0CMD_OFF = 3,
    DIM0CMD_STOP = 4,
    DIM0CMD_TOGGLE = 5,
    DIM0CMD_MEMORY_OFF = 6,
    DIM0CMD_RAMP_BRIGHTNESS = 17,
    DIM0CMD_RAMP_TOGGLE = 18,
    DIM0CMD_RAMP_UP = 19,
    DIM0CMD_RAMP_DOWN = 20,
    DIM0CMD_RAMP_UP_DOWN = 21,
    DIM0CMD_LOCK = 34,
    DIM0CMD_UNLOCK = 35,
    DIM0CMD_FLASH = 49,
    DIM0CMD_FLASH_MOMENTARY = 50,
} DIM0CMD_ENUM;

typedef enum CURRENTSTATUS_ENUM
{
    CURRENTSTATUS_NORMAL = 0,
    CURRENTSTATUS_IN_OVERCURRENT = 1,
    CURRENTSTATUS_UNKNOWN = 2,
    CURRENTSTATUS_OPEN_CIRCUIT = 3,
} CURRENTSTATUS_ENUM;

typedef enum DIM0LSTAT_ENUM
{
    DIM0LSTAT_OFF = 0,
    DIM0LSTAT_ON = 1,
    DIM0LSTAT_FAULT = 3,
    DIM0LSTAT_UNKNOWN = 4,
} DIM0LSTAT_ENUM;

typedef enum RFAN0SLEEPFMODE_ENUM
{
    RFAN0SLEEPFMODE_FAN_OFF = 0,
    RFAN0SLEEPFMODE_FAN_MANUAL = 1,
    RFAN0SLEEPFMODE_FAN_AUTO = 2,
} RFAN0SLEEPFMODE_ENUM;

typedef enum RFAN0FEATURE_BITMASK_ENUM
{
    RFAN0FEATURE_REVERSEAIRFLOW_BITMASK = 1 << 0, // 1
    RFAN0FEATURE_LIGHT_BITMASK = 1 << 1, // 2
    RFAN0FEATURE_MOTIONSENSOR_BITMASK = 1 << 2, // 4
} RFAN0FEATURE_BITMASK_ENUM;

typedef struct RFAN0FEATURE_T
{
    union
    {
        struct
        {
            uint32_t reverseairflow : 1;
            uint32_t light : 1;
            uint32_t motionsensor : 1;
            uint32_t unused_3 : 1;
            uint32_t unused_4 : 1;
            uint32_t unused_5 : 1;
            uint32_t unused_6 : 1;
            uint32_t unused_7 : 1;
            uint32_t unused_8 : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED RFAN0FEATURE_T;


#define RFAN0FEATURE_REVERSEAIRFLOW_GET(a) ( ((RFAN0FEATURE_REVERSEAIRFLOW_BITMASK) & (a)) >> 0 )
#define RFAN0FEATURE_REVERSEAIRFLOW_SET(a,b) (a) = ( (~(RFAN0FEATURE_REVERSEAIRFLOW_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define RFAN0FEATURE_LIGHT_GET(a) ( ((RFAN0FEATURE_LIGHT_BITMASK) & (a)) >> 1 )
#define RFAN0FEATURE_LIGHT_SET(a,b) (a) = ( (~(RFAN0FEATURE_LIGHT_BITMASK) & (a)) | ( ((b) & 1) << 1) )
#define RFAN0FEATURE_MOTIONSENSOR_GET(a) ( ((RFAN0FEATURE_MOTIONSENSOR_BITMASK) & (a)) >> 2 )
#define RFAN0FEATURE_MOTIONSENSOR_SET(a,b) (a) = ( (~(RFAN0FEATURE_MOTIONSENSOR_BITMASK) & (a)) | ( ((b) & 1) << 2) )

typedef enum AW0MOTION_ENUM
{
    AW0MOTION_NO_MOTION = 0,
    AW0MOTION_EXTENDING = 1,
    AW0MOTION_RETRACTING = 2,
} AW0MOTION_ENUM;

typedef enum MTR0DIR_ENUM
{
    MTR0DIR_NO_DIRECTION = 0,
    MTR0DIR_CLOCKWISE = 1,
    MTR0DIR_COUNTER_CLOCKWISE = 2,
} MTR0DIR_ENUM;

typedef enum GNTR0DCOST_ENUM
{
    GNTR0DCOST_DISABLE_DC_GENERATOR_DC_OUTPUT = 0,
    GNTR0DCOST_ENABLE_DC_GENERATOR_DC_OUTPUT = 1,
    GNTR0DCOST_START_EQUALIZATION = 2,
} GNTR0DCOST_ENUM;

typedef enum GNTR0ST_ENUM
{
    GNTR0ST_STOPPED = 0,
    GNTR0ST_PREHEAT = 1,
    GNTR0ST_CRANKING = 2,
    GNTR0ST_RUNNING = 3,
    GNTR0ST_PRIMING = 4,
    GNTR0ST_FAULT = 5,
    GNTR0ST_ENGINE_RUN_ONLY = 6,
    GNTR0ST_TEST_MODE = 7,
    GNTR0ST_VOLTAGE_ADJUST_MODE = 8,
    GNTR0ST_FAULT_BYPASS_MODE = 9,
    GNTR0ST_CONFIGURATION_MODE = 10,
} GNTR0ST_ENUM;

typedef enum GNTR0CMD_ENUM
{
    GNTR0CMD_STOP = 0,
    GNTR0CMD_START = 1,
    GNTR0CMD_MANUAL_PRIME = 2,
    GNTR0CMD_MANUAL_PREHEAT = 3,
} GNTR0CMD_ENUM;

typedef enum GNTR0TYPE_ENUM
{
    GNTR0TYPE_NOT_DEFINED = 0,
    GNTR0TYPE_RUN_CRANK_INPUTS = 1,
    GNTR0TYPE_CRANK_GLOW_AND_STOP_INPUTS = 2,
    GNTR0TYPE_PREHEAT_PRIME_INPUT = 3,
    GNTR0TYPE_SINGLE_ON_OFF_INPUT = 4,
} GNTR0TYPE_ENUM;

typedef enum GNTR0EXTACTR_ENUM
{
    GNTR0EXTACTR_NO_EXTERNAL_ACTIVITY = 0,
    GNTR0EXTACTR_EXTERNAL_ACTIVITY_DETECTED = 1,
    GNTR0EXTACTR_RESET_EXTERNAL_ACTIVITY_FLAG = 2,
} GNTR0EXTACTR_ENUM;

typedef enum GNTR0QUIETT_ENUM
{
    GNTR0QUIETT_UNIT_IS_NOT_IN_QUIET_TIME = 0,
    GNTR0QUIETT_UNIT_IS_IN_QUIET_TIME = 1,
    GNTR0QUIETT_DO_NOT_OVERRIDE_QUIET_TIME = 2,
    GNTR0QUIETT_OVERRIDE_QUIET_TIME = 3,
} GNTR0QUIETT_ENUM;

typedef enum GNTR0EXDW_ENUM
{
    GNTR0EXDW_SUN = 1,
    GNTR0EXDW_MON = 2,
    GNTR0EXDW_TUE = 4,
    GNTR0EXDW_WED = 8,
    GNTR0EXDW_THUR = 16,
    GNTR0EXDW_FRI = 32,
    GNTR0EXDW_SAT = 64,
} GNTR0EXDW_ENUM;

typedef enum ACCH0ACCHOS_ENUM
{
    ACCH0ACCHOS_DISABLE = 0,
    ACCH0ACCHOS_NOT_CHARGING = 1,
    ACCH0ACCHOS_BULK = 2,
    ACCH0ACCHOS_ABSORPTION = 3,
    ACCH0ACCHOS_OVERCHARGE = 4,
    ACCH0ACCHOS_EQUALIZE = 5,
    ACCH0ACCHOS_FLOAT = 6,
    ACCH0ACCHOS_CONSTANT_VOLTAGE_CURRENT = 7,
} ACCH0ACCHOS_ENUM;

typedef enum ACCH0ACCHFC_ENUM
{
    ACCH0ACCHFC_CHARGING_NOT_FORCED = 0,
    ACCH0ACCHFC_FORCE_CHARGE_TO_BULK = 1,
    ACCH0ACCHFC_FORCE_CHARGE_TO_FLOAT = 2,
} ACCH0ACCHFC_ENUM;

typedef enum ITER0MD_ENUM
{
    ITER0MD_DISABLED = 0,
    ITER0MD_INVERTING = 1,
    ITER0MD_AC_PASS_THROUGH = 2,
    ITER0MD_APS_ONLY = 3,
    ITER0MD_LOAD_SENSING = 4,
    ITER0MD_WAITING_TO_INVERT = 5,
} ITER0MD_ENUM;

typedef enum PMIC0BSTAT_ENUM
{
    PMIC0BSTAT_DISCHARGING = 0,
    PMIC0BSTAT_NOT_CHARGING = 1,
    PMIC0BSTAT_CHARGING = 2,
    PMIC0BSTAT_FULL = 3,
} PMIC0BSTAT_ENUM;

typedef enum PMIC0CHRGT_ENUM
{
    PMIC0CHRGT_NONE = 0,
    PMIC0CHRGT_STANDARD = 1,
    PMIC0CHRGT_FAST = 2,
    PMIC0CHRGT_UNKNOWN = 3,
} PMIC0CHRGT_ENUM;

typedef enum PMIC0BHLT_ENUM
{
    PMIC0BHLT_GOOD = 0,
    PMIC0BHLT_OVERVOLTAGE = 1,
    PMIC0BHLT_SAFETY_TIME_EXPIRE = 2,
    PMIC0BHLT_OVERHEAT = 3,
    PMIC0BHLT_UNSPEC_FAILURE = 4,
} PMIC0BHLT_ENUM;

typedef enum TS0TYPE_ENUM
{
    TS0TYPE_FRESH = 0,
    TS0TYPE_BLACK_WASTE = 1,
    TS0TYPE_GRAY = 2,
    TS0TYPE_LPG = 3,
    TS0TYPE_FUEL = 4,
    TS0TYPE_OIL = 5,
    TS0TYPE_LIVE_WELL = 6,
    TS0TYPE_BAIT_WELL = 7,
    TS0TYPE_2ND_FRESH = 16,
    TS0TYPE_2ND_BLACK = 17,
    TS0TYPE_2ND_GRAY = 18,
    TS0TYPE_2ND_LPG = 19,
} TS0TYPE_ENUM;

typedef enum WH0OPSTAT_ENUM
{
    WH0OPSTAT_OFF = 0,
    WH0OPSTAT_COMBUSTION = 1,
    WH0OPSTAT_ELECTRIC = 2,
    WH0OPSTAT_GAS_AND_ELECTRIC = 3,
    WH0OPSTAT_AUTO = 4,
    WH0OPSTAT_TEST_COMBUSTION = 5,
    WH0OPSTAT_TEST_ELECTRIC = 6,
} WH0OPSTAT_ENUM;

typedef enum WH0HLVL_ENUM
{
    WH0HLVL_DEFAULT = 0,
    WH0HLVL_ECO = 1,
    WH0HLVL_COMFORT = 2,
    WH0HLVL_ANTI_FREEZE = 3,
    WH0HLVL_DECALCIFICATION = 4,
} WH0HLVL_ENUM;

typedef enum DCL0CMD_ENUM
{
    DCL0CMD_SET_LEVEL_DELAY = 0,
    DCL0CMD_ON_DURATION = 1,
    DCL0CMD_ON_DELAY = 2,
    DCL0CMD_OFF_DELAY = 3,
    DCL0CMD_STOP_DELAY = 4,
    DCL0CMD_TOGGLE = 5,
    DCL0CMD_MEMORY_OFF = 6,
    DCL0CMD_LOCK = 34,
    DCL0CMD_FLASH = 49,
} DCL0CMD_ENUM;

typedef enum DCL0TYPE_ENUM
{
    DCL0TYPE_BILGE_PUMP = 0,
    DCL0TYPE_NAV_LIGHTS = 1,
    DCL0TYPE_ANCHOR_LIGHTS = 2,
    DCL0TYPE_COURTESY_LIGHTS = 3,
    DCL0TYPE_UNDERWATER_LIGHTS = 4,
    DCL0TYPE_HORN = 5,
    DCL0TYPE_WIPERS = 6,
    DCL0TYPE_SHADE = 7,
    DCL0TYPE_BLOWER = 8,
    DCL0TYPE_LIVE_WELL_PUMP = 9,
    DCL0TYPE_BAIT_WELL_PUMP = 10,
} DCL0TYPE_ENUM;

typedef enum DIM0TYPE_BITMASK_ENUM
{
    DIM0TYPE_DIMMABLE_BITMASK = 1 << 0, // 1
    DIM0TYPE_RGB_BITMASK = 1 << 1, // 2
} DIM0TYPE_BITMASK_ENUM;

typedef struct DIM0TYPE_T
{
    union
    {
        struct
        {
            uint32_t dimmable : 1;
            uint32_t rgb : 1;
            uint32_t unused_2 : 1;
            uint32_t unused_3 : 1;
            uint32_t unused_4 : 1;
            uint32_t unused_5 : 1;
            uint32_t unused_6 : 1;
            uint32_t unused_7 : 1;
            uint32_t unused_8 : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED DIM0TYPE_T;


#define DIM0TYPE_DIMMABLE_GET(a) ( ((DIM0TYPE_DIMMABLE_BITMASK) & (a)) >> 0 )
#define DIM0TYPE_DIMMABLE_SET(a,b) (a) = ( (~(DIM0TYPE_DIMMABLE_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define DIM0TYPE_RGB_GET(a) ( ((DIM0TYPE_RGB_BITMASK) & (a)) >> 1 )
#define DIM0TYPE_RGB_SET(a,b) (a) = ( (~(DIM0TYPE_RGB_BITMASK) & (a)) | ( ((b) & 1) << 1) )

typedef enum SC0STAT_ENUM
{
    SC0STAT_NOT_ACTIVATED = 0,
    SC0STAT_ACTIVATED = 1,
    SC0STAT_ACTIVATE = 2,
} SC0STAT_ENUM;

typedef enum SNODE0MFGR_ENUM
{
    SNODE0MFGR_DOMETIC = 0x0845,
    SNODE0MFGR_UNKNOWN = 0xffff,
} SNODE0MFGR_ENUM;

typedef enum SNODE0MDL_ENUM
{
    SNODE0MDL_CLIMATE = 0,
    SNODE0MDL_AIR_QUALITY = 1,
    SNODE0MDL_DOOR = 2,
    SNODE0MDL_MOTION = 3,
    SNODE0MDL_WIRE_BREAK_AND_VOLTAGE = 4,
    SNODE0MDL_TEST = 5,
    SNODE0MDL_DIFFERENTIAL_PRESSURE = 6,
    SNODE0MDL_DOOR__AND__CLIMATE = 7,
    SNODE0MDL_EXTENDED_SENSOR = 8,
    SNODE0MDL_N_A = 255,
} SNODE0MDL_ENUM;

typedef enum SACCM0EVENT_ENUM
{
    SACCM0EVENT_SINGLE_TAP = 1,
    SACCM0EVENT_DOUBLE_TAP = 2,
    SACCM0EVENT_TILT = 3,
    SACCM0EVENT_TAMPERING = 4,
    SACCM0EVENT_MOTION = 5,
    SACCM0EVENT_FREEFALL = 6,
} SACCM0EVENT_ENUM;

typedef enum SACCM0DATARATE_ENUM
{
    SACCM0DATARATE_POWERDOWN = 0,
    SACCM0DATARATE_1_6_HZ = 1,
    SACCM0DATARATE_12_5_HZ = 2,
    SACCM0DATARATE_25_HZ = 3,
    SACCM0DATARATE_50_HZ = 4,
    SACCM0DATARATE_100_HZ = 5,
    SACCM0DATARATE_200_HZ = 6,
    SACCM0DATARATE_400_HZ = 7,
    SACCM0DATARATE_800_HZ = 8,
    SACCM0DATARATE_1600_HZ = 9,
} SACCM0DATARATE_ENUM;

typedef enum SACCM0POWERM_ENUM
{
    SACCM0POWERM_HIGH_PERFORMANCE__PLUS_LN_DISABLED = 0x04,
    SACCM0POWERM_CONT_LP4__PLUS_LN_DISABLED = 0x03,
    SACCM0POWERM_CONT_LP3__PLUS_LN_DISABLED = 0x02,
    SACCM0POWERM_CONT_LP2__PLUS_LN_DISABLED = 0x01,
    SACCM0POWERM_CONT_LP1__PLUS_LN_DISABLED_12_BIT = 0x00,
    SACCM0POWERM_SINGLE_LP4__PLUS_LN_DISABLED = 0x0b,
    SACCM0POWERM_SINGLE_LP3__PLUS_LN_DISABLED = 0x0a,
    SACCM0POWERM_SINGLE_LP2__PLUS_LN_DISABLED = 0x09,
    SACCM0POWERM_SINGLE_LP1__PLUS_LN_DISABLED_12_BIT = 0x08,
    SACCM0POWERM_HIGH_PERFORMANCE__PLUS_LN_ENABLED = 0x14,
    SACCM0POWERM_CONT_LP4__PLUS_LN_ENABLED = 0x13,
    SACCM0POWERM_CONT_LP3__PLUS_LN_ENABLED = 0x12,
    SACCM0POWERM_CONT_LP2__PLUS_LN_ENABLED = 0x11,
    SACCM0POWERM_CONT_LP1__PLUS_LN_ENABLED_BIT = 0x10,
    SACCM0POWERM_SINGLE_LP4__PLUS_LN_ENABLED = 0x1B,
    SACCM0POWERM_SINGLE_LP3__PLUS_LN_ENABLED = 0x1A,
    SACCM0POWERM_SINGLE_LP2__PLUS_LN_ENABLED = 0x19,
    SACCM0POWERM_SINGLE_LP1__PLUS_LN_ENABLED_BIT = 0x18,
} SACCM0POWERM_ENUM;

typedef enum SACCM0BW_ENUM
{
    SACCM0BW_ODR_2_UP_TO_ODR_800_HZ__400_HZ_WHEN_ODR_1600_HZ = 0,
    SACCM0BW_ODR_4_HP_LP = 1,
    SACCM0BW_ODR_10_HP_LP = 2,
    SACCM0BW_ODR_20_HP_LP = 3,
} SACCM0BW_ENUM;

typedef enum SACCM0FULLSCALE_ENUM
{
    SACCM0FULLSCALE_2_G = 0,
    SACCM0FULLSCALE_4_G = 1,
    SACCM0FULLSCALE_8_G = 2,
    SACCM0FULLSCALE_16_G = 3,
} SACCM0FULLSCALE_ENUM;

typedef enum SACCM0FILTPATH_ENUM
{
    SACCM0FILTPATH_LOW_PASS_FILTER_PATH_SELECTED = 0,
    SACCM0FILTPATH_LIS2DW12_USER_OFFSET_ON_OUT = 1,
    SACCM0FILTPATH_HIGH_PASS_FILTER_PATH_SELECTED = 2,
} SACCM0FILTPATH_ENUM;

typedef enum SACCM0TILTT_ENUM
{
    SACCM0TILTT_80_DEGREES = 0,
    SACCM0TILTT_70_DEGREES = 1,
    SACCM0TILTT_60_DEGREES = 2,
    SACCM0TILTT_50_DEGREES = 3,
} SACCM0TILTT_ENUM;

typedef enum SACCM0TAPDIR_ENUM
{
    SACCM0TAPDIR_DISABLED = 0,
    SACCM0TAPDIR_X_DIRECTION = 1,
    SACCM0TAPDIR_Y_DIRECTION = 2,
    SACCM0TAPDIR_Z_DIRECTION = 3,
    SACCM0TAPDIR_X_AND_Y_DIRECTION = 4,
    SACCM0TAPDIR_Y_AND_Z_DIRECTION = 5,
    SACCM0TAPDIR_Z_AND_X_DIRECTION = 6,
    SACCM0TAPDIR_X_AND_Y_AND_Z_DIRECTION = 7,
} SACCM0TAPDIR_ENUM;

typedef enum SACCM0TAPP_ENUM
{
    SACCM0TAPP_X_MAX_PRIORITY__Y_MID_PRIORITY__Z_LOW_PRIORITY = 0,
    SACCM0TAPP_Y_MAX_PRIORITY__X_MID_PRIORITY__Z_LOW_PRIORITY = 1,
    SACCM0TAPP_X_MAX_PRIORITY__Z_MID_PRIORITY__Y_LOW_PRIORITY = 2,
    SACCM0TAPP_Z_MAX_PRIORITY__Y_MID_PRIORITY__X_LOW_PRIORITY = 3,
    SACCM0TAPP_Y_MAX_PRIORITY__Z_MID_PRIORITY__X_LOW_PRIORITY = 5,
    SACCM0TAPP_Z_MAX_PRIORITY__X_MID_PRIORITY__Y_LOW_PRIORITY = 6,
} SACCM0TAPP_ENUM;

typedef enum SACCM0FALLTHRESH_ENUM
{
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_5LSB_FS2G = 0,
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_7LSB_FS2G = 1,
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_8LSB_FS2G = 2,
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_10LSB_FS2G = 3,
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_11LSB_FS2G = 4,
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_13LSB_FS2G = 5,
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_15LSB_FS2G = 6,
    SACCM0FALLTHRESH_LIS2DW12_FF_TSH_16LSB_FS2G = 7,
} SACCM0FALLTHRESH_ENUM;

typedef enum SACCM0TAPMODE_ENUM
{
    SACCM0TAPMODE_SINGLE_ONLY = 0,
    SACCM0TAPMODE_BOTH_SINGLE_AND_DOUBLE = 1,
} SACCM0TAPMODE_ENUM;

typedef enum SACCM0TAPRECOGX_ENUM
{
    SACCM0TAPRECOGX_RECOGNITION_DISABLE = 0,
    SACCM0TAPRECOGX_RECOGNITION_ENABLE = 1,
} SACCM0TAPRECOGX_ENUM;

typedef enum SBMEB0SAMPRATE_ENUM
{
    SBMEB0SAMPRATE_DISABLED = 0,
    SBMEB0SAMPRATE_LP = 1,
    SBMEB0SAMPRATE_ULP = 2,
    SBMEB0SAMPRATE_CONT = 3,
} SBMEB0SAMPRATE_ENUM;

typedef enum SPIR0EVT_ENUM
{
    SPIR0EVT_MOTION_IN_DETECT = 1,
    SPIR0EVT_MOTION_OUT_DETECT = 2,
} SPIR0EVT_ENUM;

typedef enum SV0PSTS_ENUM
{
    SV0PSTS_12V_CONNECTED = 0,
    SV0PSTS_12V_NOT_CONNECTED = 1,
    SV0PSTS_LDO_FAILED = 2,
} SV0PSTS_ENUM;

typedef enum SV0MC_ENUM
{
    SV0MC_CONT_MEASURMENT_MF_AVERAGE = 0,
    SV0MC_CONT_MEASURMENT_MF_NONE = 1,
    SV0MC_CONT_MEASURMENT_DP_AVERAGE = 2,
    SV0MC_CONT_MEASURMENT_DP_NONE = 3,
    SV0MC_TRIG_MEASUREMENT_MF_NONE = 4,
    SV0MC_TRIG_MEASUREMENT_DP_NONE = 5,
} SV0MC_ENUM;

typedef enum SHALLAN0EVT_ENUM
{
    SHALLAN0EVT_NOT_DETECTED = 0,
    SHALLAN0EVT_CLOSED = 1,
    SHALLAN0EVT_OPEN = 2,
} SHALLAN0EVT_ENUM;

typedef enum DEMO0MODE_ENUM
{
    DEMO0MODE_MIN = 0,
    DEMO0MODE_LOW = 1,
    DEMO0MODE_MED = 2,
    DEMO0MODE_HIGH = 3,
    DEMO0MODE_MAX = 4,
} DEMO0MODE_ENUM;

typedef enum SW0TYPE_ENUM
{
    SW0TYPE_SINGLE = 0,
    SW0TYPE_DOUBLE = 1,
    SW0TYPE_SEQUENCE = 2,
    SW0TYPE_MODE = 3,
    SW0TYPE_SINGLE_NO_PRESET = 4,
    SW0TYPE_DIMMER = 5,
    SW0TYPE_RGB = 6,
} SW0TYPE_ENUM;

typedef enum SW0ICON_ENUM
{
    SW0ICON_NONE = 0,
    SW0ICON_LIGHT = 1,
    SW0ICON_PUMP = 2,
    SW0ICON_ENGINE = 3,
    SW0ICON_WIPER = 4,
    SW0ICON_HARDTOP_CONOPY = 5,
    SW0ICON_ACTUATOR = 6,
    SW0ICON_COOL = 7,
    SW0ICON_HEAT = 8,
    SW0ICON_ANCHOR = 9,
    SW0ICON_HORN = 10,
} SW0ICON_ENUM;

typedef enum SW0MSTATE_ENUM
{
    SW0MSTATE_OFF = 0,
    SW0MSTATE_ON_OR_UP = 1,
    SW0MSTATE_DOWN = 2,
} SW0MSTATE_ENUM;

typedef enum SW0MIND_ENUM
{
    SW0MIND_OFF = 0,
    SW0MIND_ON = 1,
    SW0MIND_1HZ_DEF_COLOR = 2,
    SW0MIND_5HZ_DEF_COLOR = 3,
    SW0MIND_1HZ_WARNING_COLOR = 4,
    SW0MIND_5HZ_WARNING_COLOR = 5,
    SW0MIND_INACTIVE = 6,
} SW0MIND_ENUM;

typedef enum NGN0TRANS_ENUM
{
    NGN0TRANS_REVERSE = 0,
    NGN0TRANS_NEUTRAL = 1,
    NGN0TRANS_FORWARD = 2,
} NGN0TRANS_ENUM;

typedef enum BMS0FLOW_ENUM
{
    BMS0FLOW_DISCHARGING = -1,
    BMS0FLOW_IDLING = 0,
    BMS0FLOW_CHARGING = 1,
} BMS0FLOW_ENUM;

typedef enum BMS0ID_ENUM
{
    BMS0ID_UNDEFINED = -1,
    BMS0ID_PORT_ENGINE = 0,
    BMS0ID_STBD_ENGINE = 1,
    BMS0ID_PORT_CENTER_ENGINE = 2,
    BMS0ID_STBD_CENTER_ENGINE = 3,
    BMS0ID_CENTER_ENGINE = 4,
    BMS0ID_CENTER_PORT_ENGINE = 5,
    BMS0ID_CENTER_STBD_ENGINE = 6,
    BMS0ID_HOUSE_BATTERY_1 = 7,
    BMS0ID_HOUSE_BATTERY_2 = 8,
    BMS0ID_HOUSE_BATTERY_3 = 9,
    BMS0ID_HOUSE_BATTERY_4 = 10,
    BMS0ID_HOUSE_BATTERY_5 = 11,
    BMS0ID_HOUSE_BATTERY_6 = 12,
    BMS0ID_HOUSE_BATTERY_7 = 13,
    BMS0ID_HOUSE_BATTERY_8 = 14,
    BMS0ID_HOUSE_BATTERY_9 = 15,
    BMS0ID_HOUSE_BATTERY_10 = 16,
    BMS0ID_N2K_BATTERY_1 = 17,
    BMS0ID_N2K_BATTERY_2 = 18,
    BMS0ID_N2K_BATTERY_3 = 19,
    BMS0ID_N2K_BATTERY_4 = 20,
    BMS0ID_N2K_BATTERY_5 = 21,
    BMS0ID_N2K_BATTERY_6 = 22,
    BMS0ID_N2K_BATTERY_7 = 23,
    BMS0ID_N2K_BATTERY_8 = 24,
} BMS0ID_ENUM;

typedef enum BOS0ID_ENUM
{
    BOS0ID_AFT_BILGE_1 = 0,
    BOS0ID_AFT_BILGE_2 = 1,
    BOS0ID_FWD_BILGE_1 = 2,
    BOS0ID_FWD_BILGE_2 = 3,
    BOS0ID_PORT_BILGE = 4,
    BOS0ID_STBD_BILGE = 5,
    BOS0ID_BILGE_1 = 6,
    BOS0ID_BILGE_2 = 7,
    BOS0ID_BILGE_3 = 8,
    BOS0ID_BILGE_4 = 9,
    BOS0ID_BILGE_5 = 10,
} BOS0ID_ENUM;

typedef enum BOS0ALRTS_ENUM
{
    BOS0ALRTS_NONE = 0,
    BOS0ALRTS_CONTINUOUS = 1,
    BOS0ALRTS_HOURLY = 2,
    BOS0ALRTS_DAILY = 4,
} BOS0ALRTS_ENUM;

typedef enum BOS0WRNS_ENUM
{
    BOS0WRNS_NONE = 0,
    BOS0WRNS_CONTINUOUS = 1,
    BOS0WRNS_HOURLY = 2,
    BOS0WRNS_DAILY = 4,
} BOS0WRNS_ENUM;

typedef enum USM0STAT_ENUM
{
    USM0STAT_OK = 0,
    USM0STAT_ERROR = 1,
} USM0STAT_ENUM;

typedef enum USM0MODE_ENUM
{
    USM0MODE_AUTOMATIC = 0,
    USM0MODE_MANUAL = 1,
} USM0MODE_ENUM;

typedef enum USM0STATE_ENUM
{
    USM0STATE_IDLE = 0,
    USM0STATE_FILE_DOWNLOAD = 1,
    USM0STATE_BUS_TRANSFER = 2,
    USM0STATE_EXTENDED_STATE = 3,
    USM0STATE_SECOND_EXTENDED_STATE = 4,
} USM0STATE_ENUM;

typedef enum SVC0TSRC_ENUM
{
    SVC0TSRC_NONE = 0,
    SVC0TSRC_RTC = 1,
    SVC0TSRC_EXTERNAL = 2,
    SVC0TSRC_SET = 3,
    SVC0TSRC_NTP = 4,
    SVC0TSRC_GNSS = 5,
} SVC0TSRC_ENUM;

typedef enum SVC0GPIOR_ENUM
{
    SVC0GPIOR_OUTPUT_PIN_OUT_OF_RANGE = -2,
    SVC0GPIOR_INPUT_PIN_OUT_OF_RANGE = -1,
    SVC0GPIOR_LOW = 0,
    SVC0GPIOR_HIGH = 1,
} SVC0GPIOR_ENUM;

typedef enum SUP0TYPE_ENUM
{
    SUP0TYPE_LEVEL = 0,
    SUP0TYPE_STATE = 1,
} SUP0TYPE_ENUM;

typedef enum SUP0STYPE_ENUM
{
    SUP0STYPE_S1 = 0,
    SUP0STYPE_S2 = 1,
    SUP0STYPE_BOTH = 2,
} SUP0STYPE_ENUM;

typedef enum MPCSVC0REGION_ENUM
{
    MPCSVC0REGION_UNCONFIGURED = 0,
    MPCSVC0REGION_AFGHANISTAN = 4,
    MPCSVC0REGION_ALBANIA = 8,
    MPCSVC0REGION_ALGERIA = 12,
    MPCSVC0REGION_AMERICAN_SAMOA = 16,
    MPCSVC0REGION_ANDORRA = 20,
    MPCSVC0REGION_ANGOLA = 24,
    MPCSVC0REGION_ANGUILLA = 660,
    MPCSVC0REGION_ANTARCTICA = 10,
    MPCSVC0REGION_ANTIGUA_AND_BARBUDA = 28,
    MPCSVC0REGION_ARGENTINA = 32,
    MPCSVC0REGION_ARMENIA = 51,
    MPCSVC0REGION_ARUBA = 533,
    MPCSVC0REGION_AUSTRALIA = 36,
    MPCSVC0REGION_AUSTRIA = 40,
    MPCSVC0REGION_AZERBAIJAN = 31,
    MPCSVC0REGION_BAHAMAS_THE = 44,
    MPCSVC0REGION_BAHRAIN = 48,
    MPCSVC0REGION_BANGLADESH = 50,
    MPCSVC0REGION_BARBADOS = 52,
    MPCSVC0REGION_BELARUS = 112,
    MPCSVC0REGION_BELGIUM = 56,
    MPCSVC0REGION_BELIZE = 84,
    MPCSVC0REGION_BENIN = 204,
    MPCSVC0REGION_BERMUDA = 60,
    MPCSVC0REGION_BHUTAN = 64,
    MPCSVC0REGION_BOLIVIA_PLURINATIONAL_STATE_OF = 68,
    MPCSVC0REGION_BONAIRE__SINT_EUSTATIUS_AND_SABA = 535,
    MPCSVC0REGION_BOSNIA_AND_HERZEGOVINA = 70,
    MPCSVC0REGION_BOTSWANA = 72,
    MPCSVC0REGION_BOUVET_ISLAND = 74,
    MPCSVC0REGION_BRAZIL = 76,
    MPCSVC0REGION_BRITISH_INDIAN_OCEAN_TERRITORY_THE = 86,
    MPCSVC0REGION_BRUNEI_DARUSSALAM = 96,
    MPCSVC0REGION_BULGARIA = 100,
    MPCSVC0REGION_BURKINA_FASO = 854,
    MPCSVC0REGION_BURUNDI = 108,
    MPCSVC0REGION_CABO_VERDE = 132,
    MPCSVC0REGION_CAMBODIA = 116,
    MPCSVC0REGION_CAMEROON = 120,
    MPCSVC0REGION_CANADA = 124,
    MPCSVC0REGION_CAYMAN_ISLANDS_THE = 136,
    MPCSVC0REGION_CENTRAL_AFRICAN_REPUBLIC_THE = 140,
    MPCSVC0REGION_CHAD = 148,
    MPCSVC0REGION_CHILE = 152,
    MPCSVC0REGION_CHINA = 156,
    MPCSVC0REGION_CHRISTMAS_ISLAND = 162,
    MPCSVC0REGION_COCOS_KEELING_ISLANDS_THE = 166,
    MPCSVC0REGION_COLOMBIA = 170,
    MPCSVC0REGION_COMOROS_THE = 174,
    MPCSVC0REGION_CONGO_THE_DEMOCRATIC_REPUBLIC_OF_THE = 180,
    MPCSVC0REGION_CONGO_THE = 178,
    MPCSVC0REGION_COOK_ISLANDS_THE = 184,
    MPCSVC0REGION_COSTA_RICA = 188,
    MPCSVC0REGION_CROATIA = 191,
    MPCSVC0REGION_CUBA = 192,
    MPCSVC0REGION_CURACAO = 531,
    MPCSVC0REGION_CYPRUS = 196,
    MPCSVC0REGION_CZECHIA = 203,
    MPCSVC0REGION_COTE_DIVOIRE = 384,
    MPCSVC0REGION_DENMARK = 208,
    MPCSVC0REGION_DJIBOUTI = 262,
    MPCSVC0REGION_DOMINICA = 212,
    MPCSVC0REGION_DOMINICAN_REPUBLIC_THE = 214,
    MPCSVC0REGION_ECUADOR = 218,
    MPCSVC0REGION_EGYPT = 818,
    MPCSVC0REGION_EL_SALVADOR = 222,
    MPCSVC0REGION_EQUATORIAL_GUINEA = 226,
    MPCSVC0REGION_ERITREA = 232,
    MPCSVC0REGION_ESTONIA = 233,
    MPCSVC0REGION_ESWATINI = 748,
    MPCSVC0REGION_ETHIOPIA = 231,
    MPCSVC0REGION_FALKLAND_ISLANDS_THE_MALVINAS = 238,
    MPCSVC0REGION_FAROE_ISLANDS_THE = 234,
    MPCSVC0REGION_FIJI = 242,
    MPCSVC0REGION_FINLAND = 246,
    MPCSVC0REGION_FRANCE = 250,
    MPCSVC0REGION_FRENCH_GUIANA = 254,
    MPCSVC0REGION_FRENCH_POLYNESIA = 258,
    MPCSVC0REGION_FRENCH_SOUTHERN_TERRITORIES_THE = 260,
    MPCSVC0REGION_GABON = 266,
    MPCSVC0REGION_GAMBIA_THE = 270,
    MPCSVC0REGION_GEORGIA = 268,
    MPCSVC0REGION_GERMANY = 276,
    MPCSVC0REGION_GHANA = 288,
    MPCSVC0REGION_GIBRALTAR = 292,
    MPCSVC0REGION_GREECE = 300,
    MPCSVC0REGION_GREENLAND = 304,
    MPCSVC0REGION_GRENADA = 308,
    MPCSVC0REGION_GUADELOUPE = 312,
    MPCSVC0REGION_GUAM = 316,
    MPCSVC0REGION_GUATEMALA = 320,
    MPCSVC0REGION_GUERNSEY = 831,
    MPCSVC0REGION_GUINEA = 324,
    MPCSVC0REGION_GUINEA_BISSAU = 624,
    MPCSVC0REGION_GUYANA = 328,
    MPCSVC0REGION_HAITI = 332,
    MPCSVC0REGION_HEARD_ISLAND_AND_MCDONALD_ISLANDS = 334,
    MPCSVC0REGION_HOLY_SEE_THE = 336,
    MPCSVC0REGION_HONDURAS = 340,
    MPCSVC0REGION_HONG_KONG = 344,
    MPCSVC0REGION_HUNGARY = 348,
    MPCSVC0REGION_ICELAND = 352,
    MPCSVC0REGION_INDIA = 356,
    MPCSVC0REGION_INDONESIA = 360,
    MPCSVC0REGION_IRAN_ISLAMIC_REPUBLIC_OF = 364,
    MPCSVC0REGION_IRAQ = 368,
    MPCSVC0REGION_IRELAND = 372,
    MPCSVC0REGION_ISLE_OF_MAN = 833,
    MPCSVC0REGION_ISRAEL = 376,
    MPCSVC0REGION_ITALY = 380,
    MPCSVC0REGION_JAMAICA = 388,
    MPCSVC0REGION_JAPAN = 392,
    MPCSVC0REGION_JERSEY = 832,
    MPCSVC0REGION_JORDAN = 400,
    MPCSVC0REGION_KAZAKHSTAN = 398,
    MPCSVC0REGION_KENYA = 404,
    MPCSVC0REGION_KIRIBATI = 296,
    MPCSVC0REGION_KOREA_THE_DEMOCRATIC_PEOPLES_REPUBLIC_OF = 408,
    MPCSVC0REGION_KOREA_THE_REPUBLIC_OF = 410,
    MPCSVC0REGION_KUWAIT = 414,
    MPCSVC0REGION_KYRGYZSTAN = 417,
    MPCSVC0REGION_LAO_PEOPLES_DEMOCRATIC_REPUBLIC_THE = 418,
    MPCSVC0REGION_LATVIA = 428,
    MPCSVC0REGION_LEBANON = 422,
    MPCSVC0REGION_LESOTHO = 426,
    MPCSVC0REGION_LIBERIA = 430,
    MPCSVC0REGION_LIBYA = 434,
    MPCSVC0REGION_LIECHTENSTEIN = 438,
    MPCSVC0REGION_LITHUANIA = 440,
    MPCSVC0REGION_LUXEMBOURG = 442,
    MPCSVC0REGION_MACAO = 446,
    MPCSVC0REGION_MADAGASCAR = 450,
    MPCSVC0REGION_MALAWI = 454,
    MPCSVC0REGION_MALAYSIA = 458,
    MPCSVC0REGION_MALDIVES = 462,
    MPCSVC0REGION_MALI = 466,
    MPCSVC0REGION_MALTA = 470,
    MPCSVC0REGION_MARSHALL_ISLANDS_THE = 584,
    MPCSVC0REGION_MARTINIQUE = 474,
    MPCSVC0REGION_MAURITANIA = 478,
    MPCSVC0REGION_MAURITIUS = 480,
    MPCSVC0REGION_MAYOTTE = 175,
    MPCSVC0REGION_MEXICO = 484,
    MPCSVC0REGION_MICRONESIA_FEDERATED_STATES_OF = 583,
    MPCSVC0REGION_MOLDOVA_THE_REPUBLIC_OF = 498,
    MPCSVC0REGION_MONACO = 492,
    MPCSVC0REGION_MONGOLIA = 496,
    MPCSVC0REGION_MONTENEGRO = 499,
    MPCSVC0REGION_MONTSERRAT = 500,
    MPCSVC0REGION_MOROCCO = 504,
    MPCSVC0REGION_MOZAMBIQUE = 508,
    MPCSVC0REGION_MYANMAR = 104,
    MPCSVC0REGION_NAMIBIA = 516,
    MPCSVC0REGION_NAURU = 520,
    MPCSVC0REGION_NEPAL = 524,
    MPCSVC0REGION_NETHERLANDS_THE = 528,
    MPCSVC0REGION_NEW_CALEDONIA = 540,
    MPCSVC0REGION_NEW_ZEALAND = 554,
    MPCSVC0REGION_NICARAGUA = 558,
    MPCSVC0REGION_NIGER_THE = 562,
    MPCSVC0REGION_NIGERIA = 566,
    MPCSVC0REGION_NIUE = 570,
    MPCSVC0REGION_NORFOLK_ISLAND = 574,
    MPCSVC0REGION_NORTH_MACEDONIA = 807,
    MPCSVC0REGION_NORTHERN_MARIANA_ISLANDS_THE = 580,
    MPCSVC0REGION_NORWAY = 578,
    MPCSVC0REGION_OMAN = 512,
    MPCSVC0REGION_PAKISTAN = 586,
    MPCSVC0REGION_PALAU = 585,
    MPCSVC0REGION_PALESTINE__STATE_OF = 275,
    MPCSVC0REGION_PANAMA = 591,
    MPCSVC0REGION_PAPUA_NEW_GUINEA = 598,
    MPCSVC0REGION_PARAGUAY = 600,
    MPCSVC0REGION_PERU = 604,
    MPCSVC0REGION_PHILIPPINES_THE = 608,
    MPCSVC0REGION_PITCAIRN = 612,
    MPCSVC0REGION_POLAND = 616,
    MPCSVC0REGION_PORTUGAL = 620,
    MPCSVC0REGION_PUERTO_RICO = 630,
    MPCSVC0REGION_QATAR = 634,
    MPCSVC0REGION_ROMANIA = 642,
    MPCSVC0REGION_RUSSIAN_FEDERATION_THE = 643,
    MPCSVC0REGION_RWANDA = 646,
    MPCSVC0REGION_REUNION = 638,
    MPCSVC0REGION_SAINT_BARTHELEMY = 652,
    MPCSVC0REGION_SAINT_HELENA__ASCENSION_AND_TRISTAN_DA_CUNHA = 654,
    MPCSVC0REGION_SAINT_KITTS_AND_NEVIS = 659,
    MPCSVC0REGION_SAINT_LUCIA = 662,
    MPCSVC0REGION_SAINT_MARTIN_FRENCH_PART = 663,
    MPCSVC0REGION_SAINT_PIERRE_AND_MIQUELON = 666,
    MPCSVC0REGION_SAINT_VINCENT_AND_THE_GRENADINES = 670,
    MPCSVC0REGION_SAMOA = 882,
    MPCSVC0REGION_SAN_MARINO = 674,
    MPCSVC0REGION_SAO_TOME_AND_PRINCIPE = 678,
    MPCSVC0REGION_SAUDI_ARABIA = 682,
    MPCSVC0REGION_SENEGAL = 686,
    MPCSVC0REGION_SERBIA = 688,
    MPCSVC0REGION_SEYCHELLES = 690,
    MPCSVC0REGION_SIERRA_LEONE = 694,
    MPCSVC0REGION_SINGAPORE = 702,
    MPCSVC0REGION_SINT_MAARTEN_DUTCH_PART = 534,
    MPCSVC0REGION_SLOVAKIA = 703,
    MPCSVC0REGION_SLOVENIA = 705,
    MPCSVC0REGION_SOLOMON_ISLANDS = 90,
    MPCSVC0REGION_SOMALIA = 706,
    MPCSVC0REGION_SOUTH_AFRICA = 710,
    MPCSVC0REGION_SOUTH_GEORGIA_AND_THE_SOUTH_SANDWICH_ISLANDS = 239,
    MPCSVC0REGION_SOUTH_SUDAN = 728,
    MPCSVC0REGION_SPAIN = 724,
    MPCSVC0REGION_SRI_LANKA = 144,
    MPCSVC0REGION_SUDAN_THE = 729,
    MPCSVC0REGION_SURINAME = 740,
    MPCSVC0REGION_SVALBARD_AND_JAN_MAYEN = 744,
    MPCSVC0REGION_SWEDEN = 752,
    MPCSVC0REGION_SWITZERLAND = 756,
    MPCSVC0REGION_SYRIAN_ARAB_REPUBLIC_THE = 760,
    MPCSVC0REGION_TAIWAN_PROVINCE_OF_CHINA = 158,
    MPCSVC0REGION_TAJIKISTAN = 762,
    MPCSVC0REGION_TANZANIA__THE_UNITED_REPUBLIC_OF = 834,
    MPCSVC0REGION_THAILAND = 764,
    MPCSVC0REGION_TIMOR_LESTE = 626,
    MPCSVC0REGION_TOGO = 768,
    MPCSVC0REGION_TOKELAU = 772,
    MPCSVC0REGION_TONGA = 776,
    MPCSVC0REGION_TRINIDAD_AND_TOBAGO = 780,
    MPCSVC0REGION_TUNISIA = 788,
    MPCSVC0REGION_TURKEY = 792,
    MPCSVC0REGION_TURKMENISTAN = 795,
    MPCSVC0REGION_TURKS_AND_CAICOS_ISLANDS_THE = 796,
    MPCSVC0REGION_TUVALU = 798,
    MPCSVC0REGION_UGANDA = 800,
    MPCSVC0REGION_UKRAINE = 804,
    MPCSVC0REGION_UNITED_ARAB_EMIRATES_THE = 784,
    MPCSVC0REGION_UNITED_KINGDOM_OF_GREAT_BRITAIN_AND_NORTHERN_IRELAND_THE = 826,
    MPCSVC0REGION_UNITED_STATES_MINOR_OUTLYING_ISLANDS_THE = 581,
    MPCSVC0REGION_UNITED_STATES_OF_AMERICA_THE = 840,
    MPCSVC0REGION_URUGUAY = 858,
    MPCSVC0REGION_UZBEKISTAN = 860,
    MPCSVC0REGION_VANUATU = 548,
    MPCSVC0REGION_VENEZUELA_BOLIVARIAN_REPUBLIC_OF = 862,
    MPCSVC0REGION_VIET_NAM = 704,
    MPCSVC0REGION_VIRGIN_ISLANDS_BRITISH = 92,
    MPCSVC0REGION_VIRGIN_ISLANDS_U_S_ = 850,
    MPCSVC0REGION_WALLIS_AND_FUTUNA = 876,
    MPCSVC0REGION_WESTERN_SAHARA = 732,
    MPCSVC0REGION_YEMEN = 887,
    MPCSVC0REGION_ZAMBIA = 894,
    MPCSVC0REGION_ZIMBABWE = 716,
    MPCSVC0REGION_ALAND_ISLANDS = 248,
} MPCSVC0REGION_ENUM;

typedef enum MPCSVC0SIZE_ENUM
{
    MPCSVC0SIZE_UNCONFIGURED = 0,
    MPCSVC0SIZE_20_LB_VERTICAL = 1,
    MPCSVC0SIZE_30_LB_VERTICAL = 2,
    MPCSVC0SIZE_40_LB_VERTICAL = 3,
    MPCSVC0SIZE_100_LB_VERTICAL = 4,
    MPCSVC0SIZE_120_GAL_VERTICAL = 5,
    MPCSVC0SIZE_120_GAL_HORIZONTAL = 6,
    MPCSVC0SIZE_150_GAL_HORIZONTAL = 7,
    MPCSVC0SIZE_250_GAL_HORIZONTAL = 8,
    MPCSVC0SIZE_500_GAL_HORIZONTAL = 9,
    MPCSVC0SIZE_1000_GAL_HORIZONTAL = 10,
} MPCSVC0SIZE_ENUM;

typedef enum MPCSVC0UNIT_ENUM
{
    MPCSVC0UNIT_PERCENT = 0,
    MPCSVC0UNIT_CENTIMETERS = 1,
    MPCSVC0UNIT_INCHES = 2,
} MPCSVC0UNIT_ENUM;

typedef enum KEY0DOOR_ENUM
{
    KEY0DOOR_NONE = 0,
    KEY0DOOR_UPPER = 1,
    KEY0DOOR_LOWER = 2,
    KEY0DOOR_BOTH = 3,
} KEY0DOOR_ENUM;

typedef enum HMI0TEMPUNIT_ENUM
{
    HMI0TEMPUNIT_CMETRIC = 0,
    HMI0TEMPUNIT_FIMPERIAL = 1,
} HMI0TEMPUNIT_ENUM;

typedef enum HMI0SOUND_ENUM
{
    HMI0SOUND_DISABLED = 0,
    HMI0SOUND_BEEPON = 1,
    HMI0SOUND_CLICKON = 2,
} HMI0SOUND_ENUM;

typedef enum HMI0TIMEFORMAT_ENUM
{
    HMI0TIMEFORMAT_12H = 0,
    HMI0TIMEFORMAT_24H = 1,
} HMI0TIMEFORMAT_ENUM;

typedef enum CC0PTYPE_ENUM
{
    CC0PTYPE_UNCONFIGURED_RUBICON_CFX3 = 0x00,
    CC0PTYPE_RUBICON_CFX3_SINGLE_ZONE = 0x01,
    CC0PTYPE_RUBICON_CFX3_SINGLE_ZONE_WITH_ICEMAKER = 0x02,
    CC0PTYPE_RUBICON_CFX3_DUAL_ZONE = 0x03,
} CC0PTYPE_ENUM;

typedef enum CC0ACPT_ENUM
{
    CC0ACPT_INACTIVE = 0,
    CC0ACPT_C0 = 1,
    CC0ACPT_C1 = 2,
} CC0ACPT_ENUM;

typedef enum CC0BATPROTLVL_ENUM
{
    CC0BATPROTLVL_LOW = 0,
    CC0BATPROTLVL_MEDIUM = 1,
    CC0BATPROTLVL_HIGH = 2,
} CC0BATPROTLVL_ENUM;

typedef enum CC0POWSRC_ENUM
{
    CC0POWSRC_AC = 0,
    CC0POWSRC_DC = 1,
    CC0POWSRC_BATTERY = 2,
} CC0POWSRC_ENUM;

typedef enum RVCMGNT0RESETCONF_BITMASK_ENUM
{
    RVCMGNT0RESETCONF_REBOOT_BITMASK = 1 << 0, // 1
    RVCMGNT0RESETCONF_CLEAR_FAULTS_BITMASK = 1 << 1, // 2
    RVCMGNT0RESETCONF_RESET_TO_DEFAULT_SETTINGS_BITMASK = 1 << 2, // 4
    RVCMGNT0RESETCONF_RESET_STATISTICS_BITMASK = 1 << 3, // 8
    RVCMGNT0RESETCONF_TEST_MODE_BITMASK = 1 << 4, // 16
    RVCMGNT0RESETCONF_RESET_TO_OEM_SPECIFIC_SETTINGS_BITMASK = 1 << 5, // 32
    RVCMGNT0RESETCONF_ENTER_BOOTLOADER_BITMASK = 1 << 6, // 64
} RVCMGNT0RESETCONF_BITMASK_ENUM;

typedef struct RVCMGNT0RESETCONF_T
{
    union
    {
        struct
        {
            uint32_t reboot : 1;
            uint32_t clear_faults : 1;
            uint32_t reset_to_default_settings : 1;
            uint32_t reset_statistics : 1;
            uint32_t test_mode : 1;
            uint32_t reset_to_oem_specific_settings : 1;
            uint32_t enter_bootloader : 1;
            uint32_t unused_7 : 1;
            uint32_t unused_8 : 1;
            uint32_t unused_9 : 1;
            uint32_t unused_10 : 1;
            uint32_t unused_11 : 1;
            uint32_t unused_12 : 1;
            uint32_t unused_13 : 1;
            uint32_t unused_14 : 1;
            uint32_t unused_15 : 1;
            uint32_t unused_16 : 1;
            uint32_t unused_17 : 1;
            uint32_t unused_18 : 1;
            uint32_t unused_19 : 1;
            uint32_t unused_20 : 1;
            uint32_t unused_21 : 1;
            uint32_t unused_22 : 1;
            uint32_t unused_23 : 1;
            uint32_t unused_24 : 1;
            uint32_t unused_25 : 1;
            uint32_t unused_26 : 1;
            uint32_t unused_27 : 1;
            uint32_t unused_28 : 1;
            uint32_t unused_29 : 1;
            uint32_t unused_30 : 1;
            uint32_t unused_31 : 1;
        };
        uint32_t data;
    };
} PACKED RVCMGNT0RESETCONF_T;


#define RVCMGNT0RESETCONF_REBOOT_GET(a) ( ((RVCMGNT0RESETCONF_REBOOT_BITMASK) & (a)) >> 0 )
#define RVCMGNT0RESETCONF_REBOOT_SET(a,b) (a) = ( (~(RVCMGNT0RESETCONF_REBOOT_BITMASK) & (a)) | ( ((b) & 1) << 0) )
#define RVCMGNT0RESETCONF_CLEAR_FAULTS_GET(a) ( ((RVCMGNT0RESETCONF_CLEAR_FAULTS_BITMASK) & (a)) >> 1 )
#define RVCMGNT0RESETCONF_CLEAR_FAULTS_SET(a,b) (a) = ( (~(RVCMGNT0RESETCONF_CLEAR_FAULTS_BITMASK) & (a)) | ( ((b) & 1) << 1) )
#define RVCMGNT0RESETCONF_RESET_TO_DEFAULT_SETTINGS_GET(a) ( ((RVCMGNT0RESETCONF_RESET_TO_DEFAULT_SETTINGS_BITMASK) & (a)) >> 2 )
#define RVCMGNT0RESETCONF_RESET_TO_DEFAULT_SETTINGS_SET(a,b) (a) = ( (~(RVCMGNT0RESETCONF_RESET_TO_DEFAULT_SETTINGS_BITMASK) & (a)) | ( ((b) & 1) << 2) )
#define RVCMGNT0RESETCONF_RESET_STATISTICS_GET(a) ( ((RVCMGNT0RESETCONF_RESET_STATISTICS_BITMASK) & (a)) >> 3 )
#define RVCMGNT0RESETCONF_RESET_STATISTICS_SET(a,b) (a) = ( (~(RVCMGNT0RESETCONF_RESET_STATISTICS_BITMASK) & (a)) | ( ((b) & 1) << 3) )
#define RVCMGNT0RESETCONF_TEST_MODE_GET(a) ( ((RVCMGNT0RESETCONF_TEST_MODE_BITMASK) & (a)) >> 4 )
#define RVCMGNT0RESETCONF_TEST_MODE_SET(a,b) (a) = ( (~(RVCMGNT0RESETCONF_TEST_MODE_BITMASK) & (a)) | ( ((b) & 1) << 4) )
#define RVCMGNT0RESETCONF_RESET_TO_OEM_SPECIFIC_SETTINGS_GET(a) ( ((RVCMGNT0RESETCONF_RESET_TO_OEM_SPECIFIC_SETTINGS_BITMASK) & (a)) >> 5 )
#define RVCMGNT0RESETCONF_RESET_TO_OEM_SPECIFIC_SETTINGS_SET(a,b) (a) = ( (~(RVCMGNT0RESETCONF_RESET_TO_OEM_SPECIFIC_SETTINGS_BITMASK) & (a)) | ( ((b) & 1) << 5) )
#define RVCMGNT0RESETCONF_ENTER_BOOTLOADER_GET(a) ( ((RVCMGNT0RESETCONF_ENTER_BOOTLOADER_BITMASK) & (a)) >> 6 )
#define RVCMGNT0RESETCONF_ENTER_BOOTLOADER_SET(a,b) (a) = ( (~(RVCMGNT0RESETCONF_ENTER_BOOTLOADER_BITMASK) & (a)) | ( ((b) & 1) << 6) )

typedef enum RVCFURNACE0HSRC_ENUM
{
    RVCFURNACE0HSRC_COMBUSTION = 0,
    RVCFURNACE0HSRC_AC_POWER_PRIM = 1,
    RVCFURNACE0HSRC_AC_POWER_SEC = 2,
    RVCFURNACE0HSRC_ENGINE = 3,
} RVCFURNACE0HSRC_ENUM;

typedef enum RVCTH0MODE_ENUM
{
    RVCTH0MODE_OFF = 0,
    RVCTH0MODE_COOL = 1,
    RVCTH0MODE_HEAT = 2,
    RVCTH0MODE_AUTO = 3,
    RVCTH0MODE_FAN_ONLY = 4,
    RVCTH0MODE_AUX_HEAT = 5,
    RVCTH0MODE_DEHUMIDIFY = 6,
    RVCTH0MODE_UNAVAILABLE = 15,
} RVCTH0MODE_ENUM;

typedef enum RVCTH0FMODE_ENUM
{
    RVCTH0FMODE_AUTO = 0,
    RVCTH0FMODE_ON = 1,
    RVCTH0FMODE_UNAVAILABLE = 3,
} RVCTH0FMODE_ENUM;

typedef enum RVCTH0SMODE_ENUM
{
    RVCTH0SMODE_DISABLED = 0,
    RVCTH0SMODE_ENABLED = 1,
    RVCTH0SMODE_UNAVAILABLE = 3,
} RVCTH0SMODE_ENUM;

typedef enum RVC2TH0NOISE_ENUM
{
    RVC2TH0NOISE_DISABLED = 0,
    RVC2TH0NOISE_ENABLED = 1,
    RVC2TH0NOISE_UNAVAILABLE = 3,
} RVC2TH0NOISE_ENUM;

typedef enum RVC2THSCHED0DAY_ENUM
{
    RVC2THSCHED0DAY_NOT_SCHEDULED = 0,
    RVC2THSCHED0DAY_SCHEDULED = 1,
    RVC2THSCHED0DAY_UNAVAILABLE = 3,
} RVC2THSCHED0DAY_ENUM;

typedef enum RVC2AC0COMPSTAT_ENUM
{
    RVC2AC0COMPSTAT_OFF = 0,
    RVC2AC0COMPSTAT_INTENDS_TO_START = 1,
    RVC2AC0COMPSTAT_IS_STARTING = 2,
    RVC2AC0COMPSTAT_IS_RUNNING = 3,
} RVC2AC0COMPSTAT_ENUM;

typedef enum RVC2DIM0CMD_ENUM
{
    RVC2DIM0CMD_SET_LEVEL_DELAY = 0,
    RVC2DIM0CMD_ON_DURATION = 1,
    RVC2DIM0CMD_ON_DELAY = 2,
    RVC2DIM0CMD_OFF_DELAY = 3,
    RVC2DIM0CMD_STOP = 4,
    RVC2DIM0CMD_TOGGLE = 5,
    RVC2DIM0CMD_MEMORY_OFF = 6,
    RVC2DIM0CMD_SAVE_SCENE = 7,
    RVC2DIM0CMD_RAMP_BRIGHTNESS = 17,
    RVC2DIM0CMD_RAMP_TOGGLE = 18,
    RVC2DIM0CMD_RAMP_UP = 19,
    RVC2DIM0CMD_RAMP_DOWN = 20,
    RVC2DIM0CMD_RAMP_UP_DOWN = 21,
    RVC2DIM0CMD_RAMP_UP_DOWN_TOGGLE = 22,
    RVC2DIM0CMD_LOCK = 33,
    RVC2DIM0CMD_UNLOCK = 34,
    RVC2DIM0CMD_FLASH = 49,
    RVC2DIM0CMD_FLASH_MOMENTARY = 50,
} RVC2DIM0CMD_ENUM;

typedef enum RVC2RFAN0RAINSNS_ENUM
{
    RVC2RFAN0RAINSNS_NO_RAIN_DETECTED = 0,
    RVC2RFAN0RAINSNS_RAIN_DETECTED = 1,
    RVC2RFAN0RAINSNS_SENSOR_ERROR = 2,
    RVC2RFAN0RAINSNS_RAIN_SENSOR_NOT_INSTALLED = 3,
} RVC2RFAN0RAINSNS_ENUM;

typedef enum RVCHTR0ESRC_ENUM
{
    RVCHTR0ESRC_LPG_ONLY = 0,
    RVCHTR0ESRC_LPG__PLUS_LOW_ELECTRIC_POWER = 1,
    RVCHTR0ESRC_LPG__PLUS_HIGH_ELECTRIC_POWER = 2,
    RVCHTR0ESRC_LOW_ELECTRIC_POWER_ONLY = 3,
    RVCHTR0ESRC_HIGH_ELECTRIC_POWER_ONLY = 4,
    RVCHTR0ESRC_ERROR = 14,
    RVCHTR0ESRC_IGNORE = 15,
} RVCHTR0ESRC_ENUM;

typedef enum RVCHTRST0WTEMP_ENUM
{
    RVCHTRST0WTEMP_COLD = 0,
    RVCHTRST0WTEMP_WARM = 1,
    RVCHTRST0WTEMP_HOT = 2,
    RVCHTRST0WTEMP_RESERVED_3 = 3,
    RVCHTRST0WTEMP_RESERVED_4 = 4,
    RVCHTRST0WTEMP_RESERVED_5 = 5,
    RVCHTRST0WTEMP_RESERVED_6 = 6,
    RVCHTRST0WTEMP_IGNORE = 7,
} RVCHTRST0WTEMP_ENUM;

typedef enum RVCHTRST0GAIRST_ENUM
{
    RVCHTRST0GAIRST_OFF = 0,
    RVCHTRST0GAIRST_PURGE = 1,
    RVCHTRST0GAIRST_IGNITE = 2,
    RVCHTRST0GAIRST_LOW = 3,
    RVCHTRST0GAIRST_MED = 4,
    RVCHTRST0GAIRST_HIGH = 5,
    RVCHTRST0GAIRST_IGNORE = 15,
} RVCHTRST0GAIRST_ENUM;

typedef enum RVCHTRST0GWTRST_ENUM
{
    RVCHTRST0GWTRST_OFF = 0,
    RVCHTRST0GWTRST_PURGE = 1,
    RVCHTRST0GWTRST_IGNITE = 2,
    RVCHTRST0GWTRST_ON = 3,
    RVCHTRST0GWTRST_IGNORE = 15,
} RVCHTRST0GWTRST_ENUM;

typedef enum RVCHTRST0ACAIRST_ENUM
{
    RVCHTRST0ACAIRST_OFF = 0,
    RVCHTRST0ACAIRST_LOW = 1,
    RVCHTRST0ACAIRST_MED_NOT_USED = 2,
    RVCHTRST0ACAIRST_HIGH = 3,
    RVCHTRST0ACAIRST_IGNORE = 7,
} RVCHTRST0ACAIRST_ENUM;

typedef enum RVCHTRST0ACWTRST_ENUM
{
    RVCHTRST0ACWTRST_OFF = 0,
    RVCHTRST0ACWTRST_LOW = 1,
    RVCHTRST0ACWTRST_MED_NOT_USED = 2,
    RVCHTRST0ACWTRST_HIGH = 3,
    RVCHTRST0ACWTRST_IGNORE = 7,
} RVCHTRST0ACWTRST_ENUM;

typedef enum RVCHTRFAULT0_ENUM
{
    RVCHTRFAULT0_BATTERY_VOLTAGE_LOW = 2,
    RVCHTRFAULT0_BATTERY_VOLTAGE_HIGH = 3,
    RVCHTRFAULT0_SHORT_CIRCUIT_OR_OVER_CURRENT_HMI = 4,
    RVCHTRFAULT0_FUSE_BLOWN = 5,
    RVCHTRFAULT0_GENERAL_SYSTEM_ERROR = 7,
    RVCHTRFAULT0_COMMUNICATION_ERROR_HMI = 8,
    RVCHTRFAULT0_SERVICE_NEEDED = 16,
    RVCHTRFAULT0_OVER_TEMPERATURE_PCBA_OR_COMBUSTION_AIR = 18,
    RVCHTRFAULT0_AIR_PRESSURE_SENSOR_ERROR = 403,
    RVCHTRFAULT0_CIRCULATION_AIR_TEMPERATURE_SENSOR_ERROR = 404,
    RVCHTRFAULT0_WATER_TEMPERATURE_SENSOR_1_ERROR = 406,
    RVCHTRFAULT0_WATER_TEMPERATURE_SENSOR_2_ERROR = 407,
    RVCHTRFAULT0_ROOM_TEMPERATURE_SENSOR_ERROR = 408,
    RVCHTRFAULT0_AIR_HEATER_FLAME_SENSOR_ERROR = 409,
    RVCHTRFAULT0_WATER_HEATER_FLAME_SENSOR_ERROR = 410,
    RVCHTRFAULT0_COMBUSTION_FAN_ERROR = 411,
    RVCHTRFAULT0_CIRCULATION_BLOWER_ERROR = 412,
    RVCHTRFAULT0_AIR_HEATER_GAS_VALVE_LOW_ERROR = 413,
    RVCHTRFAULT0_AIR_HEATER_GAS_VALVE_MEDIUM_ERROR = 414,
    RVCHTRFAULT0_WATER_HEATER_GAS_VALVE_ERROR = 415,
    RVCHTRFAULT0_MASTER_GAS_VALVE_ERROR = 416,
    RVCHTRFAULT0_GAS_VALVE_RELAY_ERROR = 417,
    RVCHTRFAULT0_AIR_HEATER_START_ERROR = 418,
    RVCHTRFAULT0_WATER_HEATER_START_ERROR = 419,
    RVCHTRFAULT0_AIR_HEATER_FLAME_OUT = 420,
    RVCHTRFAULT0_WATER_HEATER_FLAME_OUT = 421,
    RVCHTRFAULT0_AIR_HEATER_OVERHEAT = 422,
    RVCHTRFAULT0_WATER_HEATER_OVERHEAT = 423,
    RVCHTRFAULT0_WINDOW_OPENED = 424,
    RVCHTRFAULT0_WATER_FROZEN = 425,
} RVCHTRFAULT0_ENUM;

typedef enum RVC2DCSRC0TIMEREMTYPE_ENUM
{
    RVC2DCSRC0TIMEREMTYPE_TIME_TO_EMPTY = 0,
    RVC2DCSRC0TIMEREMTYPE_TIME_TO_FULL = 1,
} RVC2DCSRC0TIMEREMTYPE_ENUM;

typedef enum RVC4DCSRC0CHGST_ENUM
{
    RVC4DCSRC0CHGST_UNDEFINED = 0,
    RVC4DCSRC0CHGST_DO_NOT_CHARGE = 1,
    RVC4DCSRC0CHGST_BULK = 2,
    RVC4DCSRC0CHGST_ABSORPTION = 3,
    RVC4DCSRC0CHGST_OVERCHARGE = 4,
    RVC4DCSRC0CHGST_EQUALIZE = 5,
    RVC4DCSRC0CHGST_FLOAT = 6,
    RVC4DCSRC0CHGST_CONSTANT_VOLTAGE_CURRENT = 7,
} RVC4DCSRC0CHGST_ENUM;

typedef enum RVC4DCSRC0TYPE_ENUM
{
    RVC4DCSRC0TYPE_FLOODED = 0,
    RVC4DCSRC0TYPE_GEL = 1,
    RVC4DCSRC0TYPE_AGM = 2,
    RVC4DCSRC0TYPE_LITHIUM_IRON_PHOSPHATE = 3,
    RVC4DCSRC0TYPE_CUSTOM = 4,
} RVC4DCSRC0TYPE_ENUM;

typedef enum RVC2DCSRCCONN0FUNC_ENUM
{
    RVC2DCSRCCONN0FUNC_INVALID = 0,
    RVC2DCSRCCONN0FUNC_SOURCE_CONNECTION = 1,
    RVC2DCSRCCONN0FUNC_LOAD_CONNECTION = 2,
    RVC2DCSRCCONN0FUNC_PRIMARY_TO_SECONDARY_BRIDGE = 3,
    RVC2DCSRCCONN0FUNC_MAIN_BATTERY_CONTACTOR = 4,
} RVC2DCSRCCONN0FUNC_ENUM;

typedef enum RVCBATTSUM0VOLTSTS_ENUM
{
    RVCBATTSUM0VOLTSTS_NORMAL = 0,
    RVCBATTSUM0VOLTSTS_EXTREME_UNDER_VOLTAGE = 1,
    RVCBATTSUM0VOLTSTS_UNDER_VOLTAGE = 2,
    RVCBATTSUM0VOLTSTS_OVER_VOLTAGE = 3,
    RVCBATTSUM0VOLTSTS_EXTREME_OVER_VOLTAGE = 4,
} RVCBATTSUM0VOLTSTS_ENUM;

typedef enum RVCBATTSUM0TEMPSTS_ENUM
{
    RVCBATTSUM0TEMPSTS_NORMAL = 0,
    RVCBATTSUM0TEMPSTS_EXTREME_UNDER_TEMPERATURE = 1,
    RVCBATTSUM0TEMPSTS_UNDER_TEMPERATURE = 2,
    RVCBATTSUM0TEMPSTS_OVER_TEMPERATURE = 3,
    RVCBATTSUM0TEMPSTS_EXTREME_OVER_TEMPERATURE = 4,
} RVCBATTSUM0TEMPSTS_ENUM;

typedef enum RVCBATTCELL0VOLTSTS_ENUM
{
    RVCBATTCELL0VOLTSTS_INVALID = 0,
    RVCBATTCELL0VOLTSTS_NORMAL = 1,
    RVCBATTCELL0VOLTSTS_OVER_VOLTAGE = 2,
    RVCBATTCELL0VOLTSTS_UNDER_VOLTAGE = 3,
} RVCBATTCELL0VOLTSTS_ENUM;

typedef enum RVCBATTCELL0TEMPSTS_ENUM
{
    RVCBATTCELL0TEMPSTS_INVALID = 0,
    RVCBATTCELL0TEMPSTS_NORMAL = 1,
    RVCBATTCELL0TEMPSTS_OVER_TEMPERATURE = 2,
    RVCBATTCELL0TEMPSTS_UNDER_TEMPERATURE = 3,
} RVCBATTCELL0TEMPSTS_ENUM;

typedef enum RVCSOLAR0OPER_ENUM
{
    RVCSOLAR0OPER_DISABLE = 0,
    RVCSOLAR0OPER_ENABLE = 1,
    RVCSOLAR0OPER_START_EQUALIZATION = 2,
    RVCSOLAR0OPER_TOP_UP_BATTERY = 3,
} RVCSOLAR0OPER_ENUM;

typedef enum RVCSOLAR0FORCE_ENUM
{
    RVCSOLAR0FORCE_NOT_FORCED = 0,
    RVCSOLAR0FORCE_FORCE_CHARGE_TO_BULK = 1,
    RVCSOLAR0FORCE_FORCE_CHARGE_TO_FLOAT = 2,
} RVCSOLAR0FORCE_ENUM;

typedef enum RVCSOLARCFG0ALGO_ENUM
{
    RVCSOLARCFG0ALGO_CONSTANT_VOLTAGE = 0,
    RVCSOLARCFG0ALGO_CONSTANT_CURRENT = 1,
    RVCSOLARCFG0ALGO_3_STAGE = 2,
    RVCSOLARCFG0ALGO_2_STAGE = 3,
    RVCSOLARCFG0ALGO_TRICKLE = 4,
    RVCSOLARCFG0ALGO_CUSTOM2 = 249,
    RVCSOLARCFG0ALGO_CUSTOM1 = 250,
} RVCSOLARCFG0ALGO_ENUM;

typedef enum RVCSOLARCFG0MODE_ENUM
{
    RVCSOLARCFG0MODE_STAND_ALONE = 0,
    RVCSOLARCFG0MODE_PRIMARY = 1,
    RVCSOLARCFG0MODE_SECONDARY = 2,
    RVCSOLARCFG0MODE_LINKED_TO_DC_SOURCE = 3,
} RVCSOLARCFG0MODE_ENUM;

typedef enum RVCREFRIG0CAV_ENUM
{
    RVCREFRIG0CAV_FRIDGE = 0,
    RVCREFRIG0CAV_FREEZER = 1,
} RVCREFRIG0CAV_ENUM;

typedef enum RVCREFRIG0LGT_ENUM
{
    RVCREFRIG0LGT_ALL_OFF = 0,
    RVCREFRIG0LGT_ALL_ON = 1,
} RVCREFRIG0LGT_ENUM;

typedef enum RVCREFRIG0DOOR_ENUM
{
    RVCREFRIG0DOOR_CLOSED = 0,
    RVCREFRIG0DOOR_OPENED = 1,
} RVCREFRIG0DOOR_ENUM;

typedef enum RVCREFRIG0FUEL_ENUM
{
    RVCREFRIG0FUEL_GAS = 0,
    RVCREFRIG0FUEL_DC_VOLTAGE = 1,
    RVCREFRIG0FUEL_AC_VOLTAGE = 2,
} RVCREFRIG0FUEL_ENUM;

typedef enum RVCREFRIG0MODE_ENUM
{
    RVCREFRIG0MODE_OFF = 0,
    RVCREFRIG0MODE_ON = 1,
    RVCREFRIG0MODE_NIGHT = 2,
    RVCREFRIG0MODE_ECO = 3,
    RVCREFRIG0MODE_FREEZER = 4,
} RVCREFRIG0MODE_ENUM;

typedef enum RVCDM0OPST_ENUM
{
    RVCDM0OPST_OFF_STANDBY = 0,
    RVCDM0OPST_OFF_ACTIVE = 1,
    RVCDM0OPST_ON_STANDBY = 4,
    RVCDM0OPST_ON_ACTIVE = 5,
} RVCDM0OPST_ENUM;

typedef enum RVCCHRG0OPERST_ENUM
{
    RVCCHRG0OPERST_DISABLE = 0,
    RVCCHRG0OPERST_ENABLE_CHARGER = 1,
    RVCCHRG0OPERST_START_EQUALIZATION = 2,
} RVCCHRG0OPERST_ENUM;

typedef enum RVCCHRG0FORCE_ENUM
{
    RVCCHRG0FORCE_CHARGING_NOT_FORCED = 0,
    RVCCHRG0FORCE_FORCE_CHARGE_TO_BULK = 1,
    RVCCHRG0FORCE_FORCE_CHARGE_TO_FLOAT = 2,
} RVCCHRG0FORCE_ENUM;

typedef enum RVCCHRGTHREE0DERREAS_ENUM
{
    RVCCHRGTHREE0DERREAS_NOT_DERATING = 0,
    RVCCHRGTHREE0DERREAS_HIGH_INTERNAL_TEMPERATURE = 1,
    RVCCHRGTHREE0DERREAS_HIGH_BATTERY_TEMPERATURE = 2,
    RVCCHRGTHREE0DERREAS_BATTERY_VOLTAGE = 3,
    RVCCHRGTHREE0DERREAS_AC_INPUT_VOLTAGE = 4,
    RVCCHRGTHREE0DERREAS_AC_INPUT_CURRENT = 5,
} RVCCHRGTHREE0DERREAS_ENUM;

typedef enum RVCACTHREE0PHASE_ENUM
{
    RVCACTHREE0PHASE_NO_COMPLEMENTARY_LEG = 0,
    RVCACTHREE0PHASE_IN_PHASE_240_VAC_NOT_AVAILABLE = 1,
    RVCACTHREE0PHASE_180_DEGREES_OUT_OF_PHASE_240_VAC_AVAILABLE = 2,
    RVCACTHREE0PHASE_PHASE_RELATIONSHIP_IS_VARIABLE = 3,
    RVCACTHREE0PHASE_ERROR = 7,
    RVCACTHREE0PHASE_NO_DATA = 15,
} RVCACTHREE0PHASE_ENUM;

typedef enum RVCACFOUR0VOLTFAULT_ENUM
{
    RVCACFOUR0VOLTFAULT_VOLTAGE_OK = 0,
    RVCACFOUR0VOLTFAULT_EXTREMELY_LOW_VOLTAGE = 1,
    RVCACFOUR0VOLTFAULT_LOW_VOLTAGE = 2,
    RVCACFOUR0VOLTFAULT_HIGH_VOLTAGE = 3,
    RVCACFOUR0VOLTFAULT_EXTREMELY_HIGH_VOLTAGE = 4,
    RVCACFOUR0VOLTFAULT_OPEN_LINE_1_DETECTED = 5,
    RVCACFOUR0VOLTFAULT_OPEN_LINE_2_DETECTED = 6,
} RVCACFOUR0VOLTFAULT_ENUM;

typedef enum RVCACFOUR0QUAL_ENUM
{
    RVCACFOUR0QUAL_UNQUALIFIED_NO_AC_PRESENT = 0,
    RVCACFOUR0QUAL_UNQUALIFIED_BAD_AC = 1,
    RVCACFOUR0QUAL_WAITING_TO_QUALIFY = 2,
    RVCACFOUR0QUAL_QUALIFYING = 3,
    RVCACFOUR0QUAL_QUALIFIED_GOOD_AC = 4,
} RVCACFOUR0QUAL_ENUM;

typedef enum RVCINVERTAC0INST_ENUM
{
    RVCINVERTAC0INST_DISABLED = 0,
    RVCINVERTAC0INST_INVERT = 1,
    RVCINVERTAC0INST_AC_PASSTHRU = 2,
    RVCINVERTAC0INST_APS_ONLY = 3,
    RVCINVERTAC0INST_LOAD_SENSE_UNIT_IS_WAITING_FOR_A_LOAD_ = 4,
    RVCINVERTAC0INST_WAITING_TO_INVERT = 5,
    RVCINVERTAC0INST_GENERATOR_SUPPORT = 6,
} RVCINVERTAC0INST_ENUM;

typedef enum RVCINVERTCFGTHREE0STACKMODE_ENUM
{
    RVCINVERTCFGTHREE0STACKMODE_STAND_ALONE = 0,
    RVCINVERTCFGTHREE0STACKMODE_MASTER = 1,
    RVCINVERTCFGTHREE0STACKMODE_SLAVE = 2,
    RVCINVERTCFGTHREE0STACKMODE_LINE_2_MASTER = 3,
    RVCINVERTCFGTHREE0STACKMODE_LINE_1_MASTER = 4,
    RVCINVERTCFGTHREE0STACKMODE_LINE_2_SLAVE = 5,
    RVCINVERTCFGTHREE0STACKMODE_LINE_1_SLAVE = 6,
    RVCINVERTCFGTHREE0STACKMODE_PHASE_1_MASTER = 7,
    RVCINVERTCFGTHREE0STACKMODE_PHASE_2_MASTER = 8,
    RVCINVERTCFGTHREE0STACKMODE_PHASE_3_MASTER = 9,
    RVCINVERTCFGTHREE0STACKMODE_PHASE_1_SLAVE = 10,
    RVCINVERTCFGTHREE0STACKMODE_PHASE_2_SLAVE = 11,
    RVCINVERTCFGTHREE0STACKMODE_PHASE_3_SLAVE = 12,
} RVCINVERTCFGTHREE0STACKMODE_ENUM;

typedef enum PROD0RESET_ENUM
{
    PROD0RESET_IDLE = 0,
    PROD0RESET_RESTART = 1,
    PROD0RESET_RESET_TO_DEFAULT_SETTINGS = 2,
    PROD0RESET_RESET_TO_FACTORY_SETTINGS = 3,
    PROD0RESET_CLEAR_FAULTS = 4,
    PROD0RESET_NOT_SUPPORTED = 255,
} PROD0RESET_ENUM;

typedef enum EVM0CMD_ENUM
{
    EVM0CMD_CLEAR_ALL = 0,
    EVM0CMD_ACKNOWLEDGE_ALL = 1,
} EVM0CMD_ENUM;

typedef enum MCHRG0TYPE_ENUM
{
    MCHRG0TYPE_NONE = -1,
    MCHRG0TYPE_CONVERTER = 0,
    MCHRG0TYPE_SOLAR = 1,
    MCHRG0TYPE_BOOSTER = 2,
} MCHRG0TYPE_ENUM;

typedef enum MCHRG0OPER_ENUM
{
    MCHRG0OPER_DISABLE = 0,
    MCHRG0OPER_STANDBY = 1,
    MCHRG0OPER_PULSE = 2,
    MCHRG0OPER_RECONDITION = 3,
    MCHRG0OPER_BULK_2 = 4,
    MCHRG0OPER_BULK = 5,
    MCHRG0OPER_MPPT = 6,
    MCHRG0OPER_ABSORPTION = 7,
    MCHRG0OPER_DESULFATION = 8,
    MCHRG0OPER_FLOAT = 9,
    MCHRG0OPER_STOP = 10,
    MCHRG0OPER_OVERCHARGE = 11,
    MCHRG0OPER_EQUALIZE = 12,
    MCHRG0OPER_CONSTANT = 13,
} MCHRG0OPER_ENUM;

typedef enum MCHRG0FORCE_ENUM
{
    MCHRG0FORCE_CHARGING_IS_NOT_FORCED = 0,
    MCHRG0FORCE_FORCE_CHARGE_TO_BULK = 1,
    MCHRG0FORCE_FORCE_CHARGE_TO_FLOAT = 2,
} MCHRG0FORCE_ENUM;

typedef enum MCHRG0ALGO_ENUM
{
    MCHRG0ALGO_CONSTANT_VOLTAGE = 0,
    MCHRG0ALGO_CONSTANT_CURRENT = 1,
    MCHRG0ALGO_3_STAGE = 2,
    MCHRG0ALGO_2_STAGE = 3,
    MCHRG0ALGO_TRICKLE = 4,
    MCHRG0ALGO_GEL = 5,
    MCHRG0ALGO_WET = 6,
    MCHRG0ALGO_AGM1 = 7,
    MCHRG0ALGO_AGM2 = 8,
    MCHRG0ALGO_LIFEPO4_1 = 9,
    MCHRG0ALGO_LIFEPO4_2 = 10,
    MCHRG0ALGO_LIFEPO4_3 = 11,
    MCHRG0ALGO_LIFEPO4_4 = 12,
    MCHRG0ALGO_CUSTOM2 = 248,
    MCHRG0ALGO_CUSTOM1 = 249,
    MCHRG0ALGO_CUSTOM = 250,
} MCHRG0ALGO_ENUM;

typedef enum MCHRG0FSTATUS_ENUM
{
    MCHRG0FSTATUS_DISABLE = 0,
    MCHRG0FSTATUS_ENABLE_CONTROLLER = 1,
    MCHRG0FSTATUS_START_EQUALIZATION = 2,
    MCHRG0FSTATUS_TOP_UP_BATTERY = 3,
} MCHRG0FSTATUS_ENUM;

typedef enum MBAT0HTR_ENUM
{
    MBAT0HTR_OFF = 0,
    MBAT0HTR_ON = 1,
    MBAT0HTR_OFF_TEMPERATURE_ABOVE_LIMIT = 2,
    MBAT0HTR_OFF_VOLTAGE_TOO_LOW = 3,
    MBAT0HTR_OFF_DISCHARGE_CURR_TOO_HIGH = 4,
    MBAT0HTR_OFF_CHARGE_CURR_TOO_LOW = 5,
    MBAT0HTR_OFF_SOC_TOO_LOW = 6,
} MBAT0HTR_ENUM;

typedef enum MBAT0HMODE_ENUM
{
    MBAT0HMODE_AUTOMATIC = 0,
    MBAT0HMODE_ON = 1,
    MBAT0HMODE_OFF = 2,
} MBAT0HMODE_ENUM;

typedef enum MACIN0QUALST_ENUM
{
    MACIN0QUALST_UNQUALIFIED_NO_AC_PRESENT = 0,
    MACIN0QUALST_UNQUALIFIED_BAD_AC = 1,
    MACIN0QUALST_WAITING_TO_QUALIFY = 2,
    MACIN0QUALST_QUALIFYING = 3,
    MACIN0QUALST_QUALIFIED_GOOD = 4,
} MACIN0QUALST_ENUM;

typedef enum MACIN0VOLTST_ENUM
{
    MACIN0VOLTST_NO_FAULT = 0,
    MACIN0VOLTST_LOW_VOLT = 1,
    MACIN0VOLTST_HIGH_VOLT = 2,
    MACIN0VOLTST_OPEN_LINE = 3,
} MACIN0VOLTST_ENUM;

typedef enum MACIN0FREQST_ENUM
{
    MACIN0FREQST_NO_FAULT = 0,
    MACIN0FREQST_LOW_FREQ = 1,
    MACIN0FREQST_HIGH_FREQ = 2,
} MACIN0FREQST_ENUM;

typedef enum MINVERT0STACK_ENUM
{
    MINVERT0STACK_STAND_ALONE = 0,
    MINVERT0STACK_MASTER = 1,
    MINVERT0STACK_SLAVE = 2,
    MINVERT0STACK_LINE_2_MASTER = 3,
    MINVERT0STACK_LINE_1_MASTER = 4,
    MINVERT0STACK_LINE_2_SLAVE = 5,
    MINVERT0STACK_LINE_1_SLAVE = 6,
    MINVERT0STACK_PHASE_1_MASTER = 7,
    MINVERT0STACK_PHASE_2_MASTER = 8,
    MINVERT0STACK_PHASE_3_MASTER = 9,
    MINVERT0STACK_PHASE_1_SLAVE = 10,
    MINVERT0STACK_PHASE_2_SLAVE = 11,
    MINVERT0STACK_PHASE_3_SLAVE = 12,
} MINVERT0STACK_ENUM;

typedef enum MACOUT0PHASE_ENUM
{
    MACOUT0PHASE_NO_COMPL__LEG = 0,
    MACOUT0PHASE_IN_PHASE = 1,
    MACOUT0PHASE_180_DEGREEES_OUT_OF_PHASE = 2,
    MACOUT0PHASE_PHASE_RELATIONSHIP_VARIABLE = 3,
    MACOUT0PHASE_ERROR = 7,
} MACOUT0PHASE_ENUM;

typedef enum MSHUNT0TYPE_ENUM
{
    MSHUNT0TYPE_BATTERY = 0,
    MSHUNT0TYPE_APPLIANCE = 1,
} MSHUNT0TYPE_ENUM;

typedef enum SYSAPPL0SMARTECO_ENUM
{
    SYSAPPL0SMARTECO_OFF = 0,
    SYSAPPL0SMARTECO_ON = 1,
    SYSAPPL0SMARTECO_INHIBIT = 2,
} SYSAPPL0SMARTECO_ENUM;

typedef enum GROUP0TYPE_ENUM
{
    GROUP0TYPE_GENERIC = 0,
    GROUP0TYPE_CLIMATEZONE = 1,
    GROUP0TYPE_SMARTECO = 2,
    GROUP0TYPE_CLIMATECONTROL = 3,
    GROUP0TYPE_MOBILECOOLER = 4,
    GROUP0TYPE_POWERSYSTEM = 5,
} GROUP0TYPE_ENUM;

typedef enum GROUP0ENABLE_ENUM
{
    GROUP0ENABLE_OFF = 0,
    GROUP0ENABLE_ON = 1,
    GROUP0ENABLE_WAIT_OFF = 2,
} GROUP0ENABLE_ENUM;

typedef enum GROUP0ACTIVE_ENUM
{
    GROUP0ACTIVE_OFF = 0,
    GROUP0ACTIVE_ON = 1,
    GROUP0ACTIVE_INHIBIT = 2,
} GROUP0ACTIVE_ENUM;


typedef enum DDM2_global_error_codes_e
{
    GENERIC_NO_ERRORS = 0,    // 0x0 Indicates that there are no active errors.
    GENERIC_COMMUNICATION_ERROR = 1,    // 0x1 Generic communication error (any interface)
    GENERIC_CI_BUS_COMMUNICATION_ERROR = 2,    // 0x2 LIN-BUS/CI-BUS communication bus error
    GENERIC_RVC_BUS_COMMUNICATION_ERROR = 3,    // 0x3 CAN/RVC communication bus error
    GENERIC_CI_BUS_SELFTEST_ERROR = 4,    // 0x4 LIN-BUS/CI-BUS self test error
    GENERIC_DC_UNDER_VOLTAGE_ERROR = 16,    // 0x10 DC input undervoltage detection error
    GENERIC_DC_OVER_VOLTAGE_ERROR = 17,    // 0x11 DC input overvoltage detection error
    GENERIC_AC_UNDER_VOLTAGE_ERROR = 18,    // 0x12 AC input undervoltage protection detection error
    GENERIC_AC_OVER_VOLTAGE_ERROR = 19,    // 0x13 AC input overvoltage detection error
    GENERIC_AC_OVER_CURRENT_ERROR = 20,    // 0x14 AC input over current detection error
    GENERIC_NTC_OPEN_CIRCUIT_ERROR = 21,    // 0x15 NTC temperature sensor open circuit detection error
    GENERIC_NTC_SHORT_CIRCUIT_ERROR = 22,    // 0x16 NTC temperature sensor short circuit detection error
    GENERIC_DOOR_OPENED_ERROR = 23,    // 0x17 Door has been opened too long (Product dependent)
    GENERIC_CURRENT_SENSOR_ERROR = 24,    // 0x18 Current sensor error. No current sensed by system
    GENERIC_EEPROM_ERROR = 25,    // 0x19 Unable to read/use EEPROM
    GENERIC_SOLENOID_VALVE_ERROR = 26,    // 0x1A Faulty solenoid detection error (Product dependent)
    GENERIC_TEMPERATURE_OUT_OF_RANGE_ERROR = 27,    // 0x1B Generic temperature out of range detection error/alert
    GENERIC_POWER_FAILURE = 28,    // 0x1C Generic power failure
    AIRC_ROOM_TEMP_SENSOR_ERROR = 256,    // 0x100 Room temperature sensor fault detected
    AIRC_OUTDOOR_TEMP_SENSOR_ERROR = 257,    // 0x101 Outdoor ambient temperature sensor short circuit or open circuit fault detected
    AIRC_EVAPORATOR_TEMP_SENSOR_ERROR = 258,    // 0x102 Short circuit or open circuit of evaporator coil temperature sensor error detected
    AIRC_CONDENSOR_TEMP_SENSOR_ERROR = 259,    // 0x103 Short circuit or open circuit of condenser coil temperature sensor error detected
    AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR = 260,    // 0x104 Compressor discharge tube temperature sensor short circuit or open circuit error detected
    AIRC_COMPRESSOR_DRV_IPM_ERROR = 261,    // 0x105 Compressor driver or IPM module protection appears 6 times within 30 minutes
    AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_PROTECTION_ERROR = 262,    // 0x106 The temperature of the compressor discharge tube exceeds the protection value
    AIRC_EVAPORATOR_TEMP_SENSOR_COOL_PROTECTION_ERROR = 263,    // 0x107 Evaporator coil temperature is too low in cooling mode
    AIRC_CONDENSOR_TEMP_SENSOR_PROTECTION_ERROR = 264,    // 0x108 Condenser coil temperature is too high in cooling mode
    AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR = 265,    // 0x109 Evaporator coil temperature is too high in heating mode
    AIRC_COMPRESSOR_DRV_NO_START_ERROR = 266,    // 0x10A Compressor drive abnormal or compressor does not start
    AIRC_COMPRESSOR_DRV_IPM_PROTECTION_ERROR = 267,    // 0x10B IPM module itself over temperature or over current
    AIRC_OUTDOOR_TEMP_ERROR = 268,    // 0x10C The outdoor ambient temperature is too high or too low, Outdoor ring temperature below 0℃ (refrigeration) or outdoor temperature above 32℃ (heating)
    AIRC_EVAPORATOR_FAN_ERROR = 269,    // 0x10D The evaporator motor does not rotate or the fan speed is abnormal
    AIRC_CONDENSER_FAN_ERROR = 270,    // 0x10E The condenser motor does not rotate or the fan speed is abnormal
    AIRC_INVALID_ZONE_CONFIG = 271,    // 0x10F Invalid zone configuration
    AIRC_INVALID_ZONE_CONFIG_2 = 272,    // 0x110 Invalid zone configuration
    MC_COMPRESSOR_FAN_OVER_CURRENT_ERROR = 512,    // 0x200 
    MC_COMPRESSOR_START_FAIL_ERROR = 513,    // 0x201 
    MC_COMPRESSOR_SPEED_LOW_ERROR = 514,    // 0x202 
    MC_COMPRESSOR_OVER_TEMPERATURE_ERROR = 515,    // 0x203 
    MC_COMPRESSOR_FAN_SPEED_LOW_ERROR = 516,    // 0x204 
    MC_NTC_OPEN_CIRCUIT_ERROR = 517,    // 0x205 NTC temperature sensor open circuit detection error (first compartment)
    MC_NTC_SHORT_CIRCUIT_ERROR = 518,    // 0x206 NTC temperature sensor short circuit detection error (first compartment)
    MC_NTC1_OPEN_CIRCUIT_ERROR = 519,    // 0x207 NTC1 temperature sensor open circuit detection error (second compartment)
    MC_NTC1_SHORT_CIRCUIT_ERROR = 520,    // 0x208 NTC1 temperature sensor short circuit detection error (second compartment)
    MC_NTC2_OPEN_CIRCUIT_ERROR = 521,    // 0x209 NTC2 temperature sensor open circuit detection error (third compartment)
    MC_NTC2_SHORT_CIRCUIT_ERROR = 522,    // 0x20A NTC2 temperature sensor short circuit detection error (third compartment)
    MC_TEMPERATURE_OUT_OF_RANGE_ERROR = 523,    // 0x20B Temperature is out of range detection error
    MC_CONTROLLER_OVER_TEMPERATURE_ERROR = 524,    // 0x20C Controller over temperature detection error
    HEAT_FAN_SPEED_LOW_ERROR = 768,    // 0x300 Heater fan speed too low detection
    HEAT_NTC_OPEN_CIRCUIT_ERROR = 769,    // 0x301 NTC temperature sensor open circuit detection error
    HEAT_NTC_SHORT_CIRCUIT_ERROR = 770,    // 0x302 NTC temperature sensor short circuit detection error
    HEAT_LOW_CURRENT_ERROR = 771,    // 0x303 Heater fault too low current detection
    HEAT_OVER_CURRENT_ERROR = 772,    // 0x304 Heater fault over current detection
    MPS_DC_HIGH_VOLT_LIMIT_REACHED = 832,    // 0x340 Warning indicates whether DC Source (e.g. battery) has reached its upper operation voltage limit and charging sources should terminate
    MPS_DC_HIGH_VOLT_LIMIT_CHARGE_DISCONNECTED = 833,    // 0x341 Indicates whether the DC Source has been disconnected due to reaching its upper operation voltage limit
    MPS_DC_LOW_VOLT_LIMIT_REACHED = 834,    // 0x342 Warning indicates whether DC Source (e.g. battery) has reached its lower operation voltage limit and loads should terminate
    MPS_DC_LOW_VOLT_LIMIT_LOAD_DISCONNECTED = 835,    // 0x343 Indicates whether the DC Source has been disconnected due to reaching its lower operation voltage limit.
    MPS_DC_LOW_STATE_CHARGE_LIMIT_REACHED = 836,    // 0x344 Warning indicates whether DC Source (e.g. battery) has reached its lower state of charge limit and loads should be terminated.
    MPS_DC_LOW_STATE_CHARGE_LIMIT_LOAD_DISCONNECTED = 837,    // 0x345 Indicates whether the DC Source (e.g. battery) has been disconnected from the load due to reaching the lower state of charge limit.
    MPS_DC_LOW_TEMP_LIMIT_REACHED = 838,    // 0x346 Warning indicates whether DC Source (e.g. battery) has reached its lower temperature limit and charging sources or loads should terminate
    MPS_DC_LOW_TEMP_LIMIT_DISCONNECTED = 839,    // 0x347 Indicates whether the DC Source has been disconnected from the charge or load bus due to reaching its lower temperature limit.
    MPS_DC_HIGH_TEMP_LIMIT_REACHED = 840,    // 0x348 Warning indicates whether DC Source (e.g. battery) has reached its upper temperature limit and loads or charging sources should terminate
    MPS_DC_HIGH_TEMP_LIMIT_DISCONNECTED = 841,    // 0x349 Indicates whether the DC Source has been disconnected from the charge or load bus due to reaching its upper temperature limit
    MPS_DC_HIGH_CURR_LIMIT_REACHED = 842,    // 0x34A Warning indicates whether DC Source (e.g. battery) has reached its upper current limit and loads or charging sources should be disconnected
    MPS_DC_HIGH_CURR_LIMIT_DISCONNECTED = 843,    // 0x34B Indicates whether the DC Source has been disconnected from the charge or load bus due to reaching its upper current limit
    MPS_GEN_DEVICE_OVERTEMP = 844,    // 0x34C Generic device overtemperture detected
    MPS_GEN_DEVICE_FAULT = 845,    // 0x34D Generic device fault
    MPS_BAT_CELL_FAULT = 846,    // 0x34E Generic battery cell fault detected
    MPS_AC_OPEN_GROUND_ERROR = 847,    // 0x34F open ground fault detected
    MPS_AC_OPEN_NEUTRAL_ERROR = 848,    // 0x350 open neutral fault detected
    MPS_AC_REVERSE_POLARITY_ERROR = 849,    // 0x351 reverse polarity fault detected
    MPS_AC_GROUND_CURRENT_ERROR = 850,    // 0x352 ground current fault detected
    MPS_AC_EXTREME_LOW_VOLT_ERROR = 851,    // 0x353 Extreme low voltage detected
    MPS_AC_EXTREME_HIGH_VOLT_ERROR = 852,    // 0x354 Extreme high voltage detected
    MPS_AC_LOW_VOLT_ERROR = 853,    // 0x355 Low voltage detected
    MPS_AC_HIGH_VOLT_ERROR = 854,    // 0x356 High voltage detected
    MPS_AC_OPEN_LINE_1_VOLT_ERROR = 855,    // 0x357 Open line 1 voltage detected
    MPS_AC_OPEN_LINE_2_VOLT_ERROR = 856,    // 0x358 Open line 2 voltage detected
    MPS_AC_SURGE_PROTECTION_ERROR = 857,    // 0x359 Surge fault detected
    MPS_AC_HIGH_FREQ_ERROR = 858,    // 0x35A Frequency over high limit
    MPS_AC_LOW_FREQ_ERROR = 859,    // 0x35B Frequency over low limit
    MPS_AC_HIGH_CURR_ERROR = 860,    // 0x35C Overload/High current detected
} DDM2_global_error_codes_e;

int ddm2_parse_parameter_string(uint32_t * const parameter, const char * const string, size_t string_size);
int ddm2_parameter_list_lookup(const uint32_t parameter);
char * ddm2_parameter_name(const uint32_t parameter, void * const name_buffer, size_t * const name_buffer_size);
char * ddm2_instance_name(const uint32_t class_instance, void * const name_buffer, size_t * const name_buffer_size);
char * ddm2_class_name(const uint32_t class_param);

extern const DDM2_PARAMETER_LIST_DATA Ddm2_parameter_list_data[DDM2_PARAMETER_COUNT];
extern const SORTED_LIST Parameter_list_index_table;
extern const char * Ddm2_unit_affix_list[DDM2_UNIT_COUNT + 1];
extern const int Ddm2_unit_factor_list[DDM2_UNIT_COUNT + 1];

#endif /* DDM2_PARAMETER_LIST_H_ */
