/*! \file drv_mcp2518fd.h
	\brief Microchip MCP2518 driver

    This header file provides the API function prototypes for the CAN FD SPI
    controller.
*/

// DOM-IGNORE-BEGIN
// DOM-IGNORE-END

#ifndef DRV_MCP2518FD_H
#define DRV_MCP2518FD_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
#include <stdint.h>
#include <stdbool.h>
#include "drv_mcp2518fd_register.h"
#include "hal_spi_master.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif
// DOM-IGNORE-END

typedef spi_device_handle_t MCP2518_Device;

// *****************************************************************************
// *****************************************************************************
//! Reset DUT

int8_t DRV_CANFDSPI_Reset(MCP2518_Device device);


// *****************************************************************************
// *****************************************************************************
// Section: SPI Access Functions

// *****************************************************************************
//! SPI Read Byte

int8_t DRV_CANFDSPI_ReadByte(MCP2518_Device device, uint16_t address,
        uint8_t *rxd);

// *****************************************************************************
//! SPI Write Byte

int8_t DRV_CANFDSPI_WriteByte(MCP2518_Device device, uint16_t address,
        uint8_t txd);

// *****************************************************************************
//! SPI Read Word

int8_t DRV_CANFDSPI_ReadWord(MCP2518_Device device, uint16_t address,
        uint32_t *rxd);

// *****************************************************************************
//! SPI Write Word

int8_t DRV_CANFDSPI_WriteWord(MCP2518_Device device, uint16_t address,
        uint32_t txd);

/// *****************************************************************************
//! SPI Read Word

int8_t DRV_CANFDSPI_ReadHalfWord(MCP2518_Device device, uint16_t address,
        uint16_t *rxd);

// *****************************************************************************
//! SPI Write Word

int8_t DRV_CANFDSPI_WriteHalfWord(MCP2518_Device device, uint16_t address,
        uint16_t txd);

// *****************************************************************************
//! SPI Read Byte Array

int8_t DRV_CANFDSPI_ReadByteArray(MCP2518_Device device, uint16_t address,
        uint8_t *rxd, uint16_t nBytes);

// *****************************************************************************
//! SPI Write Byte Array

int8_t DRV_CANFDSPI_WriteByteArray(MCP2518_Device device, uint16_t address,
        uint8_t *txd, uint16_t nBytes);

// *****************************************************************************
//! SPI SFR Write Byte Safe
/*!
 * Writes Byte to SFR at address using SPI CRC. Byte gets only written if CRC matches.
 *
 * Remark: The function doesn't check if the address is an SFR address.
 */

int8_t DRV_CANFDSPI_WriteByteSafe(MCP2518_Device device, uint16_t address,
        uint8_t txd);

// *****************************************************************************
//! SPI RAM Write Word Safe
/*!
 * Writes Word to RAM at address using SPI CRC. Word gets only written if CRC matches.
 *
 * Remark: The function doesn't check if the address is a RAM address.
 */

int8_t DRV_CANFDSPI_WriteWordSafe(MCP2518_Device device, uint16_t address,
        uint32_t txd);

// *****************************************************************************
//! SPI Read Byte Array with CRC

int8_t DRV_CANFDSPI_ReadByteArrayWithCRC(MCP2518_Device device, uint16_t address,
        uint8_t *rxd, uint16_t nBytes, bool fromRam, bool* crcIsCorrect);

// *****************************************************************************
//! SPI Write Byte Array with CRC

int8_t DRV_CANFDSPI_WriteByteArrayWithCRC(MCP2518_Device device, uint16_t address,
        uint8_t *txd, uint16_t nBytes, bool fromRam);

// *****************************************************************************
//! SPI Read Word Array

int8_t DRV_CANFDSPI_ReadWordArray(MCP2518_Device device, uint16_t address,
        uint32_t *rxd, uint16_t nWords);

// *****************************************************************************
//! SPI Write Word Array

int8_t DRV_CANFDSPI_WriteWordArray(MCP2518_Device device, uint16_t address,
        uint32_t *txd, uint16_t nWords);


// *****************************************************************************
// *****************************************************************************
// Section: Configuration

// *****************************************************************************
//! CAN Control register configuration

int8_t DRV_CANFDSPI_Configure(MCP2518_Device device, CAN_CONFIG* config);

// *****************************************************************************
//! Reset Configure object to reset values
int8_t DRV_CANFDSPI_ConfigureObjectReset(CAN_CONFIG* config);


// *****************************************************************************
// *****************************************************************************
// Section: Operating mode

// *****************************************************************************
//! Select Operation Mode

int8_t DRV_CANFDSPI_OperationModeSelect(MCP2518_Device device,
        CAN_OPERATION_MODE opMode);

// *****************************************************************************
//! Get Operation Mode

CAN_OPERATION_MODE DRV_CANFDSPI_OperationModeGet(MCP2518_Device device);

// *****************************************************************************
//! Enable Low Power Mode

int8_t DRV_CANFDSPI_LowPowerModeEnable(MCP2518_Device device);

// *****************************************************************************
//! Disable Low Power Mode

int8_t DRV_CANFDSPI_LowPowerModeDisable(MCP2518_Device device);


// *****************************************************************************
// *****************************************************************************
// Section: CAN Transmit

// *****************************************************************************
//! Configure Transmit FIFO

int8_t DRV_CANFDSPI_TransmitChannelConfigure(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_CONFIG* config);

// *****************************************************************************
//! Reset TransmitChannelConfigure object to reset values

int8_t DRV_CANFDSPI_TransmitChannelConfigureObjectReset(CAN_TX_FIFO_CONFIG* config);

// *****************************************************************************
//! Configure Transmit Queue

int8_t DRV_CANFDSPI_TransmitQueueConfigure(MCP2518_Device device,
        CAN_TX_QUEUE_CONFIG* config);

// *****************************************************************************
//! Reset TransmitQueueConfigure object to reset values

int8_t DRV_CANFDSPI_TransmitQueueConfigureObjectReset(CAN_TX_QUEUE_CONFIG* config);

// *****************************************************************************
//! TX Channel Load
/*!
 * Loads data into Transmit channel
 * Requests transmission, if flush==true
 */

int8_t DRV_CANFDSPI_TransmitMessage(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_TX_MSGOBJ* txObj,
        uint8_t *txd, uint32_t txdNumBytes, bool flush);

// *****************************************************************************
//! TX Queue Load

/*!
 * Loads data into Transmit Queue
 * Requests transmission, if flush==true
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueLoad(MCP2518_Device device,
        CAN_TX_MSGOBJ* txObj,
        uint8_t *txd, uint32_t txdNumBytes, bool flush)
{
    return DRV_CANFDSPI_TransmitMessage(device, CAN_TXQUEUE_CH0, txObj, txd, txdNumBytes, flush);
}

// *****************************************************************************
//! TX Channel Flush
/*!
 * Set TXREG of one channel
 */

int8_t DRV_CANFDSPI_TransmitChannelFlush(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Transmit Channel Status Get

int8_t DRV_CANFDSPI_TransmitChannelStatusGet(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_STATUS* status);

// *****************************************************************************
//! Transmit FIFO Reset

int8_t DRV_CANFDSPI_TransmitChannelReset(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Transmit FIFO Update
/*!
 * Sets UINC of the transmit channel.
 * Requests transmission, if flush==true
 */

int8_t DRV_CANFDSPI_TransmitChannelUpdate(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, bool flush);

// *****************************************************************************
//! TX Queue Flush

/*!
 * Set TXREG of one channel
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueFlush(MCP2518_Device device)
{
    return DRV_CANFDSPI_TransmitChannelFlush(device, CAN_TXQUEUE_CH0);
}

// *****************************************************************************
//! Transmit Queue Status Get

static inline int8_t DRV_CANFDSPI_TransmitQueueStatusGet(MCP2518_Device device,
        CAN_TX_FIFO_STATUS* status)
{
    return DRV_CANFDSPI_TransmitChannelStatusGet(device, CAN_TXQUEUE_CH0, status);
}

// *****************************************************************************
//! Transmit Queue Reset

static inline int8_t DRV_CANFDSPI_TransmitQueueReset(MCP2518_Device device)
{
    return DRV_CANFDSPI_TransmitChannelReset(device, CAN_TXQUEUE_CH0);
}

// *****************************************************************************
//! Transmit Queue Update

/*!
 * Sets UINC of the transmit channel.
 * Requests transmission, if flush==true
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueUpdate(MCP2518_Device device, bool flush)
{
    return DRV_CANFDSPI_TransmitChannelUpdate(device, CAN_TXQUEUE_CH0, flush);
}

// *****************************************************************************
//! Request transmissions using TXREQ register

int8_t DRV_CANFDSPI_TransmitRequestSet(MCP2518_Device device,
        CAN_TXREQ_CHANNEL txreq);

// *****************************************************************************
//! Get TXREQ register

int8_t DRV_CANFDSPI_TransmitRequestGet(MCP2518_Device device,
        uint32_t* txreq);

// *****************************************************************************
//! Abort transmission of single FIFO

int8_t DRV_CANFDSPI_TransmitChannelAbort(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Abort transmission of TXQ

static inline int8_t DRV_CANFDSPI_TransmitQueueAbort(MCP2518_Device device)
{
    return DRV_CANFDSPI_TransmitChannelAbort(device, CAN_TXQUEUE_CH0);
}

// *****************************************************************************
//! Abort All transmissions

int8_t DRV_CANFDSPI_TransmitAbortAll(MCP2518_Device device);

// *****************************************************************************
//! Set Transmit Bandwidth Sharing Delay
int8_t DRV_CANFDSPI_TransmitBandWidthSharingSet(MCP2518_Device device,
        CAN_TX_BANDWITH_SHARING txbws);


// *****************************************************************************
// *****************************************************************************
// Section: CAN Receive

// *****************************************************************************
//! Filter Object Configuration
/*!
 * Configures ID of filter object
 */

int8_t DRV_CANFDSPI_FilterObjectConfigure(MCP2518_Device device,
        CAN_FILTER filter, CAN_FILTEROBJ_ID* id);

// *****************************************************************************
//! Filter Mask Configuration
/*!
 * Configures Mask of filter object
 */

int8_t DRV_CANFDSPI_FilterMaskConfigure(MCP2518_Device device,
        CAN_FILTER filter, CAN_MASKOBJ_ID* mask);

// *****************************************************************************
//! Link Filter to FIFO
/*!
 * Initializes the Pointer from Filter to FIFO
 * Enables or disables the Filter
 */

int8_t DRV_CANFDSPI_FilterToFifoLink(MCP2518_Device device,
        CAN_FILTER filter, CAN_FIFO_CHANNEL channel, bool enable);

// *****************************************************************************
//! Filter Enable

int8_t DRV_CANFDSPI_FilterEnable(MCP2518_Device device, CAN_FILTER filter);

// *****************************************************************************
//! Filter Disable

int8_t DRV_CANFDSPI_FilterDisable(MCP2518_Device device, CAN_FILTER filter);

// *****************************************************************************
//! Set Device Net Filter Count
int8_t DRV_CANFDSPI_DeviceNetFilterCountSet(MCP2518_Device device,
        CAN_DNET_FILTER_SIZE dnfc);

// *****************************************************************************
//! Configure Receive FIFO

int8_t DRV_CANFDSPI_ReceiveChannelConfigure(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_CONFIG* config);

// *****************************************************************************
//! Reset ReceiveChannelConfigure object to reset value

int8_t DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(CAN_RX_FIFO_CONFIG* config);

// *****************************************************************************
//! Receive Channel Status Get

int8_t DRV_CANFDSPI_ReceiveChannelStatusGet(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_STATUS* status);

// *****************************************************************************
//! Get Received Message
/*!
 * Reads Received message from channel
 */
/*
int8_t DRV_CANFDSPI_ReceiveMessage(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_RX_MSGOBJ* rxObj,
        uint8_t *rxd, uint8_t nBytes);
*/
int8_t DRV_CANFDSPI_ReceiveMessage(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, uint8_t *rxd, uint8_t nBytes);
// *****************************************************************************
//! Receive FIFO Reset

int8_t DRV_CANFDSPI_ReceiveChannelReset(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Receive FIFO Update
/*!
 * Sets UINC of the receive channel.
 */

int8_t DRV_CANFDSPI_ReceiveChannelUpdate(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel);


// *****************************************************************************
// *****************************************************************************
// Section: Transmit Event FIFO

// *****************************************************************************
//! Transmit Event FIFO Status Get

int8_t DRV_CANFDSPI_TefStatusGet(MCP2518_Device device,
        CAN_TEF_FIFO_STATUS* status);

// *****************************************************************************
//! Get Transmit Event FIFO Message
/*!
 * Reads Transmit Event FIFO message
 */

int8_t DRV_CANFDSPI_TefMessageGet(MCP2518_Device device,
        CAN_TEF_MSGOBJ* tefObj);

// *****************************************************************************
//! Transmit Event FIFO Reset

int8_t DRV_CANFDSPI_TefReset(MCP2518_Device device);

// *****************************************************************************
//! Transmit Event FIFO Update
/*!
 * Sets UINC of the TEF.
 */

int8_t DRV_CANFDSPI_TefUpdate(MCP2518_Device device);

// *****************************************************************************
//! Configure Transmit Event FIFO

int8_t DRV_CANFDSPI_TefConfigure(MCP2518_Device device, CAN_TEF_CONFIG* config);

// *****************************************************************************
//! Reset TefConfigure object to reset value

int8_t DRV_CANFDSPI_TefConfigureObjectReset(CAN_TEF_CONFIG* config);


// *****************************************************************************
// *****************************************************************************
// Section: Module Events

// *****************************************************************************
//! Module Event Get
/*!
 * Reads interrupt Flags
 */

int8_t DRV_CANFDSPI_ModuleEventGet(MCP2518_Device device,
        CAN_MODULE_EVENT* flags);

// *****************************************************************************
//! Module Event Enable
/*!
 * Enables interrupts
 */

int8_t DRV_CANFDSPI_ModuleEventEnable(MCP2518_Device device,
        CAN_MODULE_EVENT flags);

// *****************************************************************************
//! Module Event Disable
/*!
 * Disables interrupts
 */

int8_t DRV_CANFDSPI_ModuleEventDisable(MCP2518_Device device,
        CAN_MODULE_EVENT flags);

// *****************************************************************************
//! Module Event Clear
/*!
 * Clears interrupt Flags
 */

int8_t DRV_CANFDSPI_ModuleEventClear(MCP2518_Device device,
        CAN_MODULE_EVENT flags);

// *****************************************************************************
//! Get RX Code

int8_t DRV_CANFDSPI_ModuleEventRxCodeGet(MCP2518_Device device,
        CAN_RXCODE* rxCode);

// *****************************************************************************
//! Get TX Code

int8_t DRV_CANFDSPI_ModuleEventTxCodeGet(MCP2518_Device device,
        CAN_TXCODE* txCode);

// *****************************************************************************
//! Get Filter Hit

int8_t DRV_CANFDSPI_ModuleEventFilterHitGet(MCP2518_Device device,
        CAN_FILTER* filterHit);

// *****************************************************************************
//! Get ICODE

int8_t DRV_CANFDSPI_ModuleEventIcodeGet(MCP2518_Device device,
        CAN_ICODE* icode);

// *****************************************************************************
// *****************************************************************************
// Section: Transmit FIFO Events

// *****************************************************************************
//! Transmit FIFO Event Get
/*!
 * Reads Transmit FIFO interrupt Flags
 */

int8_t DRV_CANFDSPI_TransmitChannelEventGet(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT* flags);

// *****************************************************************************
//! Transmit Queue Event Get

/*!
 * Reads Transmit Queue interrupt Flags
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventGet(MCP2518_Device device,
        CAN_TX_FIFO_EVENT* flags)
{
    return DRV_CANFDSPI_TransmitChannelEventGet(device, CAN_TXQUEUE_CH0, flags);
}

// *****************************************************************************
//! Get pending interrupts of all transmit FIFOs

int8_t DRV_CANFDSPI_TransmitEventGet(MCP2518_Device device, uint32_t* txif);

// *****************************************************************************
//! Get pending TXATIF of all transmit FIFOs

int8_t DRV_CANFDSPI_TransmitEventAttemptGet(MCP2518_Device device,
        uint32_t* txatif);

// *****************************************************************************
//! Transmit FIFO Index Get
/*!
 * Reads Transmit FIFO Index
 */

int8_t DRV_CANFDSPI_TransmitChannelIndexGet(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, uint8_t* idx);

// *****************************************************************************
//! Transmit FIFO Event Enable
/*!
 * Enables Transmit FIFO interrupts
 */

int8_t DRV_CANFDSPI_TransmitChannelEventEnable(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit FIFO Event Disable
/*!
 * Disables Transmit FIFO interrupts
 */

int8_t DRV_CANFDSPI_TransmitChannelEventDisable(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit FIFO Event Clear
/*!
 * Clears Transmit FIFO Attempts Exhausted interrupt Flag
 */

int8_t DRV_CANFDSPI_TransmitChannelEventAttemptClear(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel);


// *****************************************************************************
//! Transmit Queue Index Get

/*!
 * Reads Transmit Queue Index
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueIndexGet(MCP2518_Device device, uint8_t* idx)
{
    return DRV_CANFDSPI_TransmitChannelIndexGet(device, CAN_TXQUEUE_CH0, idx);
}

// *****************************************************************************
//! Transmit Queue Event Enable

/*!
 * Enables Transmit Queue interrupts
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventEnable(MCP2518_Device device,
        CAN_TX_FIFO_EVENT flags)
{
    return DRV_CANFDSPI_TransmitChannelEventEnable(device, CAN_TXQUEUE_CH0, flags);
}

// *****************************************************************************
//! Transmit Queue Event Disable

/*!
 * Disables Transmit FIFO interrupts
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventDisable(MCP2518_Device device,
        CAN_TX_FIFO_EVENT flags)
{
    return DRV_CANFDSPI_TransmitChannelEventDisable(device, CAN_TXQUEUE_CH0, flags);
}

// *****************************************************************************
//! Transmit Queue Event Clear

/*!
 * Clears Transmit FIFO Attempts Exhausted interrupt Flag
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventAttemptClear(MCP2518_Device device)
{
    return DRV_CANFDSPI_TransmitChannelEventAttemptClear(device, CAN_TXQUEUE_CH0);
}


// *****************************************************************************
// *****************************************************************************
// Section: Receive FIFO Events

// *****************************************************************************
//! Receive FIFO Event Get
/*!
 * Reads Receive FIFO interrupt Flags
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventGet(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT* flags);

// *****************************************************************************
//! Get pending interrupts of all receive FIFOs

int8_t DRV_CANFDSPI_ReceiveEventGet(MCP2518_Device device, uint32_t* rxif);

// *****************************************************************************
//!Get pending RXOVIF of all receive FIFOs

int8_t DRV_CANFDSPI_ReceiveEventOverflowGet(MCP2518_Device device, uint32_t* rxovif);

// *****************************************************************************
//! Receive FIFO Index Get
/*!
 * Reads Receive FIFO Index
 */

int8_t DRV_CANFDSPI_ReceiveChannelIndexGet(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, uint8_t* idx);

// *****************************************************************************
//! Receive FIFO Event Enable
/*!
 * Enables Receive FIFO interrupts
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventEnable(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT flags);

// *****************************************************************************
//! Receive FIFO Event Disable
/*!
 * Disables Receive FIFO interrupts
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventDisable(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT flags);

// *****************************************************************************
//! Receive FIFO Event Clear
/*!
 * Clears Receive FIFO Overflow interrupt Flag
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventOverflowClear(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel);


// *****************************************************************************
// *****************************************************************************
// Section: Transmit Event FIFO Events

// *****************************************************************************
//! Transmit Event FIFO Event Get
/*!
 * Reads Transmit Event FIFO interrupt Flags
 */

int8_t DRV_CANFDSPI_TefEventGet(MCP2518_Device device,
        CAN_TEF_FIFO_EVENT* flags);

// *****************************************************************************
//! Transmit Event FIFO Event Enable
/*!
 * Enables Transmit Event FIFO interrupts
 */

int8_t DRV_CANFDSPI_TefEventEnable(MCP2518_Device device,
        CAN_TEF_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit Event FIFO Event Disable
/*!
 * Disables Transmit Event FIFO interrupts
 */

int8_t DRV_CANFDSPI_TefEventDisable(MCP2518_Device device,
        CAN_TEF_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit Event FIFO Event Clear
/*!
 * Clears Transmit Event FIFO Overflow interrupt Flag
 */

int8_t DRV_CANFDSPI_TefEventOverflowClear(MCP2518_Device device);


// *****************************************************************************
// *****************************************************************************
// Section: Error Handling

// *****************************************************************************
//! Transmit Error Count Get

int8_t DRV_CANFDSPI_ErrorCountTransmitGet(MCP2518_Device device,
        uint8_t* tec);

// *****************************************************************************
//! Receive Error Count Get

int8_t DRV_CANFDSPI_ErrorCountReceiveGet(MCP2518_Device device,
        uint8_t* rec);

// *****************************************************************************
//! Error State Get

int8_t DRV_CANFDSPI_ErrorStateGet(MCP2518_Device device,
        CAN_ERROR_STATE* flags);

// *****************************************************************************
//! Error Counts and Error State Get
/*!
 * Returns content of complete CiTREC
 */

int8_t DRV_CANFDSPI_ErrorCountStateGet(MCP2518_Device device,
        uint8_t* tec, uint8_t* rec, CAN_ERROR_STATE* flags);

// *****************************************************************************
//! Get Bus Diagnostic Registers: all data at once, since we want to keep them in synch

int8_t DRV_CANFDSPI_BusDiagnosticsGet(MCP2518_Device device,
        CAN_BUS_DIAGNOSTIC* bd);

// *****************************************************************************
//! Clear Bus Diagnostic Registers

int8_t DRV_CANFDSPI_BusDiagnosticsClear(MCP2518_Device device);


// *****************************************************************************
// *****************************************************************************
// Section: ECC

// *****************************************************************************
//! Enable ECC

int8_t DRV_CANFDSPI_EccEnable(MCP2518_Device device);

// *****************************************************************************
//! Disable ECC

int8_t DRV_CANFDSPI_EccDisable(MCP2518_Device device);

// *****************************************************************************
//! ECC Event Get

int8_t DRV_CANFDSPI_EccEventGet(MCP2518_Device device,
        CAN_ECC_EVENT* flags);

// *****************************************************************************
//! Set ECC Parity

int8_t DRV_CANFDSPI_EccParitySet(MCP2518_Device device,
        uint8_t parity);

// *****************************************************************************
//! Get ECC Parity

int8_t DRV_CANFDSPI_EccParityGet(MCP2518_Device device,
        uint8_t* parity);

// *****************************************************************************
//! Get ECC Error Address

int8_t DRV_CANFDSPI_EccErrorAddressGet(MCP2518_Device device,
        uint16_t* a);

// *****************************************************************************
//! ECC Event Enable

int8_t DRV_CANFDSPI_EccEventEnable(MCP2518_Device device,
        CAN_ECC_EVENT flags);

// *****************************************************************************
//! ECC Event Disable

int8_t DRV_CANFDSPI_EccEventDisable(MCP2518_Device device,
        CAN_ECC_EVENT flags);

// *****************************************************************************
//! ECC Event Clear

int8_t DRV_CANFDSPI_EccEventClear(MCP2518_Device device,
        CAN_ECC_EVENT flags);

// *****************************************************************************
//! Initialize RAM

int8_t DRV_CANFDSPI_RamInit(MCP2518_Device device, uint8_t d);


// *****************************************************************************
// *****************************************************************************
// Section: CRC

// *****************************************************************************
//! CRC Event Enable

int8_t DRV_CANFDSPI_CrcEventEnable(MCP2518_Device device,
        CAN_CRC_EVENT flags);

// *****************************************************************************
//! CRC Event Disable

int8_t DRV_CANFDSPI_CrcEventDisable(MCP2518_Device device,
        CAN_CRC_EVENT flags);

// *****************************************************************************
//! CRC Event Clear

int8_t DRV_CANFDSPI_CrcEventClear(MCP2518_Device device,
        CAN_CRC_EVENT flags);

// *****************************************************************************
//! CRC Event Get

int8_t DRV_CANFDSPI_CrcEventGet(MCP2518_Device device, CAN_CRC_EVENT* flags);

// *****************************************************************************
//! Get CRC Value from device

int8_t DRV_CANFDSPI_CrcValueGet(MCP2518_Device device, uint16_t* crc);


// *****************************************************************************
// *****************************************************************************
// Section: Time Stamp

// *****************************************************************************
//! Time Stamp Enable

int8_t DRV_CANFDSPI_TimeStampEnable(MCP2518_Device device);

// *****************************************************************************
//! Time Stamp Disable

int8_t DRV_CANFDSPI_TimeStampDisable(MCP2518_Device device);

// *****************************************************************************
//! Time Stamp Get

int8_t DRV_CANFDSPI_TimeStampGet(MCP2518_Device device, uint32_t* ts);

// *****************************************************************************
//! Time Stamp Set

int8_t DRV_CANFDSPI_TimeStampSet(MCP2518_Device device, uint32_t ts);

// *****************************************************************************
//! Time Stamp Mode Configure

int8_t DRV_CANFDSPI_TimeStampModeConfigure(MCP2518_Device device,
        CAN_TS_MODE mode);

// *****************************************************************************
//! Time Stamp Prescaler Set

int8_t DRV_CANFDSPI_TimeStampPrescalerSet(MCP2518_Device device,
        uint16_t ps);


// *****************************************************************************
// *****************************************************************************
// Section: Oscillator and Bit Time

// *****************************************************************************
//! Enable oscillator to wake-up from sleep

int8_t DRV_CANFDSPI_OscillatorEnable(MCP2518_Device device);

// *****************************************************************************
//! Set Oscillator Control

int8_t DRV_CANFDSPI_OscillatorControlSet(MCP2518_Device device,
        CAN_OSC_CTRL ctrl);

int8_t DRV_CANFDSPI_OscillatorControlObjectReset(CAN_OSC_CTRL* ctrl);


// *****************************************************************************
//! Get Oscillator Status

int8_t DRV_CANFDSPI_OscillatorStatusGet(MCP2518_Device device,
        CAN_OSC_STATUS* status);

// *****************************************************************************
//! Configure Bit Time registers (based on CAN clock speed)

int8_t DRV_CANFDSPI_BitTimeConfigure(MCP2518_Device device,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode,
        CAN_SYSCLK_SPEED clk);

// *****************************************************************************
//! Configure Nominal bit time for 40MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureNominal40MHz(MCP2518_Device device,
        CAN_BITTIME_SETUP bitTime);

// *****************************************************************************
//! Configure Data bit time for 40MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureData40MHz(MCP2518_Device device,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode);

// *****************************************************************************
//! Configure Nominal bit time for 20MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureNominal20MHz(MCP2518_Device device,
        CAN_BITTIME_SETUP bitTime);

// *****************************************************************************
//! Configure Data bit time for 20MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureData20MHz(MCP2518_Device device,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode);

// *****************************************************************************
//! Configure Nominal bit time for 10MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureNominal10MHz(MCP2518_Device device,
        CAN_BITTIME_SETUP bitTime);

// *****************************************************************************
//! Configure Data bit time for 10MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureData10MHz(MCP2518_Device device,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode);


// *****************************************************************************
// *****************************************************************************
// Section: GPIO

// *****************************************************************************
//! Initialize GPIO Mode

int8_t DRV_CANFDSPI_GpioModeConfigure(MCP2518_Device device,
        GPIO_PIN_MODE gpio0, GPIO_PIN_MODE gpio1);

// *****************************************************************************
//! Initialize GPIO Direction

int8_t DRV_CANFDSPI_GpioDirectionConfigure(MCP2518_Device device,
        GPIO_PIN_DIRECTION gpio0, GPIO_PIN_DIRECTION gpio1);

// *****************************************************************************
//! Enable Transceiver Standby Control

int8_t DRV_CANFDSPI_GpioStandbyControlEnable(MCP2518_Device device);

// *****************************************************************************
//! Disable Transceiver Standby Control

int8_t DRV_CANFDSPI_GpioStandbyControlDisable(MCP2518_Device device);

// *****************************************************************************
//! Configure Open Drain Interrupts

int8_t DRV_CANFDSPI_GpioInterruptPinsOpenDrainConfigure(MCP2518_Device device,
        GPIO_OPEN_DRAIN_MODE mode);

// *****************************************************************************
//! Configure Open Drain TXCAN

int8_t DRV_CANFDSPI_GpioTransmitPinOpenDrainConfigure(MCP2518_Device device,
        GPIO_OPEN_DRAIN_MODE mode);

// *****************************************************************************
//! GPIO Output Pin Set

int8_t DRV_CANFDSPI_GpioPinSet(MCP2518_Device device,
        GPIO_PIN_POS pos, GPIO_PIN_STATE latch);

// *****************************************************************************
//! GPIO Input Pin Read

int8_t DRV_CANFDSPI_GpioPinRead(MCP2518_Device device,
        GPIO_PIN_POS pos, GPIO_PIN_STATE* state);

// *****************************************************************************
//! Configure CLKO Pin

int8_t DRV_CANFDSPI_GpioClockOutputConfigure(MCP2518_Device device,
        GPIO_CLKO_MODE mode);


// *****************************************************************************
// *****************************************************************************
// Section: Miscellaneous

// *****************************************************************************
//! DLC to number of actual data bytes conversion

uint32_t DRV_CANFDSPI_DlcToDataBytes(CAN_DLC dlc);

// *****************************************************************************
//! FIFO Index Get

int8_t DRV_CANFDSPI_FifoIndexGet(MCP2518_Device device,
        CAN_FIFO_CHANNEL channel, uint8_t* mi);

// *****************************************************************************
//! Calculate CRC16

uint16_t DRV_CANFDSPI_CalculateCRC16(uint8_t* data, uint16_t size);

// *****************************************************************************
//! Data bytes to DLC conversion

CAN_DLC DRV_CANFDSPI_DataBytesToDlc(uint8_t n);


#endif // DRV_MCP2518FD_H
