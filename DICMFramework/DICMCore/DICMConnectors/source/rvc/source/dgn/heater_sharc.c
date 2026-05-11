/*
 * heater_sharc.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"
#include "heater_sharc.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#ifdef RVC_CONFIG_INTERF_SHARC_HTR
static EXT_RAM_ATTR PROPDGN_zDGN_130559 l_130559_dgn;
static EXT_RAM_ATTR PROPDGN_zDGN_130558 l_130558_dgn;
static EXT_RAM_ATTR PROPDGN_zDGN_130552 l_130552_dgn;
static EXT_RAM_ATTR PROPDGN_zDGN_130553 l_130553_dgn;
static EXT_RAM_ATTR PROPDGN_zDGN_130555 l_130555_dgn;
static EXT_RAM_ATTR PROPDGN_zDGN_130554 l_130554_dgn;
static EXT_RAM_ATTR PROPDGN_zDGN_130557 l_130557_dgn;
static EXT_RAM_ATTR PROPDGN_zDGN_130556 l_130556_dgn;

static EXT_RAM_ATTR uint8_t l_connector_id;

void heater_sharc_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    uint32_t class = RVCHTR0;
    int instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130559_dgn, 0xFF, sizeof(l_130559_dgn));
    l_130559_dgn.u8HeaterInstance = 1;
    l_130559_dgn.i16TargetRoomTemp = NMEA2K_INT16_NO_DATA;
    memset(&l_130558_dgn, 0xFF, sizeof(l_130558_dgn));
    l_130558_dgn.u8HeaterInstance = 1;
    l_130558_dgn.i16TargetRoomTemp = NMEA2K_INT16_NO_DATA;

    class = RVCHTRST0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130557_dgn, 0xFF, sizeof(l_130557_dgn));
    l_130557_dgn.u8HeaterInstance = 1;
    l_130557_dgn.i16RoomTemp = NMEA2K_INT16_NO_DATA;

    class = RVCHMI0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130556_dgn, 0xFF, sizeof(l_130556_dgn));
    l_130556_dgn.u8HMIInstance = 1;
    l_130556_dgn.i16RoomTemperature = NMEA2K_INT16_NO_DATA;

    class = RVCHTRSCHED0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130555_dgn, 0xFF, sizeof(l_130555_dgn));
    l_130555_dgn.u8HeaterInstance = 1;
    memset(&l_130554_dgn, 0xFF, sizeof(l_130554_dgn));
    l_130554_dgn.u8HeaterInstance = 1;

    class = RVCHTRFAULT0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130553_dgn, 0xFF, sizeof(l_130553_dgn));
    l_130553_dgn.u8HeaterInstance = 1;
    l_130553_dgn.u10FaultCode1 = NMEA2K_UINT10_NO_DATA;
    l_130553_dgn.u10FaultCode2 = NMEA2K_UINT10_NO_DATA;
    l_130553_dgn.u10FaultCode3 = NMEA2K_UINT10_NO_DATA;
    l_130553_dgn.u10FaultCode4 = NMEA2K_UINT10_NO_DATA;

    class = RVCHTRVER0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130552_dgn, 0xFF, sizeof(l_130552_dgn));
    l_130552_dgn.u8HeaterInstance = 1;
}

/**
 * @brief Prepare a Heater Operation Command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130558PropDgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    PROPDGN_DGN_130558_Stuff(p_data, &l_130558_dgn);
    return true;
}

/**
 * @brief Heater operation feedback DGN received (130559)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130559PropDgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    PROPDGN_zDGN_130559 zDGN;
    int32_t value;
    bool updated_data = false;

    // Extract message content
    PROPDGN_DGN_130559_Extract(&zDGN, p_data);
    if (zDGN.u8HeaterInstance != l_130559_dgn.u8HeaterInstance)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8HeaterInstance);
        return false;
    }
    // Compare with last data
    if ((zDGN.i16TargetRoomTemp != NMEA2K_INT16_NO_DATA) && (zDGN.i16TargetRoomTemp != l_130559_dgn.i16TargetRoomTemp))
    {
        updated_data = true;
        value = l_130559_dgn.i16TargetRoomTemp = zDGN.i16TargetRoomTemp;
        convert_rvc_to_ddm_system_value(RVCHTR0TTEMP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0TTEMP, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2AirHeaterCmd != NMEA2K_UINT2_NO_DATA) && (zDGN.u2AirHeaterCmd != l_130559_dgn.u2AirHeaterCmd))
    {
        updated_data = true;
        value = l_130559_dgn.u2AirHeaterCmd = zDGN.u2AirHeaterCmd;
        convert_rvc_to_ddm_system_value(RVCHTR0AIR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0TTEMP, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2SystemUnits != NMEA2K_UINT2_NO_DATA) && (zDGN.u2SystemUnits != l_130559_dgn.u2SystemUnits))
    {
        updated_data = true;
        value = l_130559_dgn.u2SystemUnits = zDGN.u2SystemUnits;
        convert_rvc_to_ddm_system_value(RVCHTR0SYSU, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0SYSU, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2WaterHeaterCmd != NMEA2K_UINT2_NO_DATA) && (zDGN.u2WaterHeaterCmd != l_130559_dgn.u2WaterHeaterCmd))
    {
        updated_data = true;
        value = l_130559_dgn.u2WaterHeaterCmd = zDGN.u2WaterHeaterCmd;
        convert_rvc_to_ddm_system_value(RVCHTR0WTR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0WTR, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4AirHeaterMode != NMEA2K_UINT4_NO_DATA) && (zDGN.u4AirHeaterMode != l_130559_dgn.u4AirHeaterMode))
    {
        updated_data = true;
        value = l_130559_dgn.u4AirHeaterMode = zDGN.u4AirHeaterMode;
        convert_rvc_to_ddm_system_value(RVCHTR0AMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0AMODE, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4WaterHeaterMode != NMEA2K_UINT4_NO_DATA) && (zDGN.u4WaterHeaterMode != l_130559_dgn.u4WaterHeaterMode))
    {
        updated_data = true;
        value = l_130559_dgn.u4WaterHeaterMode = zDGN.u4WaterHeaterMode;
        convert_rvc_to_ddm_system_value(RVCHTR0WTRMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0WTRMODE, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4EnergySource != NMEA2K_UINT4_NO_DATA) && (zDGN.u4EnergySource != l_130559_dgn.u4EnergySource))
    {
        updated_data = true;
        value = l_130559_dgn.u4EnergySource = zDGN.u4EnergySource;
        convert_rvc_to_ddm_system_value(RVCHTR0ESRC, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0ESRC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4SilentModeMaxFan != NMEA2K_UINT4_NO_DATA) && (zDGN.u4SilentModeMaxFan != l_130559_dgn.u4SilentModeMaxFan))
    {
        updated_data = true;
        value = l_130559_dgn.u4SilentModeMaxFan = zDGN.u4SilentModeMaxFan;
        convert_rvc_to_ddm_system_value(RVCHTR0SILFMAX, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0SILFMAX, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4VentModeFanMin != NMEA2K_UINT4_NO_DATA) && (zDGN.u4VentModeFanMin != l_130559_dgn.u4VentModeFanMin))
    {
        updated_data = true;
        value = l_130559_dgn.u4VentModeFanMin = zDGN.u4VentModeFanMin;
        convert_rvc_to_ddm_system_value(RVCHTR0VFMIN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0VFMIN, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8UnderVoltThreshold != NMEA2K_UINT8_NO_DATA) && (zDGN.u8UnderVoltThreshold != l_130559_dgn.u8UnderVoltThreshold))
    {
        updated_data = true;
        value = l_130559_dgn.u8UnderVoltThreshold = zDGN.u8UnderVoltThreshold;
        convert_rvc_to_ddm_system_value(RVCHTR0UVTHRES, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0UVTHRES, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTR0SYNC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCHTR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCHTR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCHTR0INST:
            value = l_130559_dgn.u8HeaterInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCHTR0ESRC:
            if (l_130559_dgn.u4EnergySource != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130559_dgn.u4EnergySource;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0AIR:
            if (l_130559_dgn.u2AirHeaterCmd != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130559_dgn.u2AirHeaterCmd;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0WTR:
            if (l_130559_dgn.u2WaterHeaterCmd != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130559_dgn.u2WaterHeaterCmd;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0AMODE:
            if (l_130559_dgn.u4AirHeaterMode != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130559_dgn.u4AirHeaterMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0WTRMODE:
            if (l_130559_dgn.u4WaterHeaterMode != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130559_dgn.u4WaterHeaterMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0TTEMP:
            if (l_130559_dgn.i16TargetRoomTemp != NMEA2K_INT16_NO_DATA)
            {
                value = l_130559_dgn.i16TargetRoomTemp;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0SILFMAX:
            if (l_130559_dgn.u4SilentModeMaxFan != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130559_dgn.u4SilentModeMaxFan;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0VFMIN:
            if (l_130559_dgn.u4VentModeFanMin != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130559_dgn.u4VentModeFanMin;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0UVTHRES:
            if (l_130559_dgn.u8UnderVoltThreshold != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130559_dgn.u8UnderVoltThreshold;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTR0SYSU:
            if (l_130559_dgn.u2SystemUnits != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130559_dgn.u2SystemUnits;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        value = p_frame->frame.set.value.int32;
        if (convert_ddm_system_value_to_rvc_value(p_frame->frame.set.parameter, &value, true))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCHTR0ESRC:
                l_130559_dgn.u4EnergySource = value;
                break;
            case RVCHTR0AIR:
                l_130559_dgn.u2AirHeaterCmd = value;
                break;
            case RVCHTR0WTR:
                l_130559_dgn.u2WaterHeaterCmd = value;
                break;
            case RVCHTR0AMODE:
                l_130559_dgn.u4AirHeaterMode = value;
                break;
            case RVCHTR0WTRMODE:
                l_130559_dgn.u4WaterHeaterMode = value;
                break;
            case RVCHTR0TTEMP:
                l_130559_dgn.i16TargetRoomTemp = value;
                break;
            case RVCHTR0SILFMAX:
                l_130559_dgn.u4SilentModeMaxFan = value;
                break;
            case RVCHTR0VFMIN:
                l_130559_dgn.u4VentModeFanMin = value;
                break;
            case RVCHTR0UVTHRES:
                l_130559_dgn.u8UnderVoltThreshold = value;
                break;
            case RVCHTR0SYSU:
                l_130559_dgn.u2SystemUnits = value;
                break;
            case RVCHTR0SYNC:
                // Set the command frame
                NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
                break;
            default:
                break;
            }
        }
    }
}

/**
 * @brief Heater version numbers DGN received (130552)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130552PropDgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    PROPDGN_zDGN_130552 zDGN;
    int32_t value;
    bool updated_data = false;
    PROPDGN_DGN_130552_Extract(&zDGN, p_data);

    if (zDGN.u8HeaterInstance != l_130552_dgn.u8HeaterInstance)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8HeaterInstance);
        return false;
    }
    // Compare with last data
    if ((zDGN.u8PcbaVersion != NMEA2K_UINT8_NO_DATA) && (zDGN.u8PcbaVersion != l_130552_dgn.u8PcbaVersion))
    {
        updated_data = true;
        value = l_130552_dgn.u8PcbaVersion = zDGN.u8PcbaVersion;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0PCBA, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8ProtocolMajor != NMEA2K_UINT8_NO_DATA) && (zDGN.u8ProtocolMajor != l_130552_dgn.u8ProtocolMajor))
    {
        updated_data = true;
        value = l_130552_dgn.u8ProtocolMajor = zDGN.u8ProtocolMajor;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0PMAJ, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8ProtocolMinor != NMEA2K_UINT8_NO_DATA) && (zDGN.u8ProtocolMinor != l_130552_dgn.u8ProtocolMinor))
    {
        updated_data = true;
        value = l_130552_dgn.u8ProtocolMinor = zDGN.u8ProtocolMinor;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0PMIN, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8BurnerSwMajor != NMEA2K_UINT8_NO_DATA) && (zDGN.u8BurnerSwMajor != l_130552_dgn.u8BurnerSwMajor))
    {
        updated_data = true;
        value = l_130552_dgn.u8BurnerSwMajor = zDGN.u8BurnerSwMajor;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0BMAJ, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8BurnerSwMinor != NMEA2K_UINT8_NO_DATA) && (zDGN.u8BurnerSwMinor != l_130552_dgn.u8BurnerSwMinor))
    {
        updated_data = true;
        value = l_130552_dgn.u8BurnerSwMinor = zDGN.u8BurnerSwMinor;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0BMIN, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8ComfortSwMajor != NMEA2K_UINT8_NO_DATA) && (zDGN.u8ComfortSwMajor != l_130552_dgn.u8ComfortSwMajor))
    {
        updated_data = true;
        value = l_130552_dgn.u8ComfortSwMajor = zDGN.u8ComfortSwMajor;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0CMAJ, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8ComfortSwMinor != NMEA2K_UINT8_NO_DATA) && (zDGN.u8ComfortSwMinor != l_130552_dgn.u8ComfortSwMinor))
    {
        updated_data = true;
        value = l_130552_dgn.u8ComfortSwMinor = zDGN.u8ComfortSwMinor;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0CMIN, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRVER0SYNC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCHTRVER0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCHTRVER0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCHTRVER0INST:
            value = l_130552_dgn.u8HeaterInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCHTRVER0PCBA:
            if (l_130552_dgn.u8PcbaVersion != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130552_dgn.u8PcbaVersion;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRVER0PMAJ:
            if (l_130552_dgn.u8ProtocolMajor != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130552_dgn.u8ProtocolMajor;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRVER0PMIN:
            if (l_130552_dgn.u8ProtocolMinor != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130552_dgn.u8ProtocolMinor;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRVER0BMAJ:
            if (l_130552_dgn.u8BurnerSwMajor != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130552_dgn.u8BurnerSwMajor;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRVER0BMIN:
            if (l_130552_dgn.u8BurnerSwMinor != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130552_dgn.u8BurnerSwMinor;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRVER0CMAJ:
            if (l_130552_dgn.u8ComfortSwMajor != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130552_dgn.u8ComfortSwMajor;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRVER0CMIN:
            if (l_130552_dgn.u8ComfortSwMinor != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130552_dgn.u8ComfortSwMinor;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Heater active fault codes DGN received (130553)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130553PropDgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    PROPDGN_zDGN_130553 zDGN;
    int32_t value;
    bool updated_data = false;
    PROPDGN_DGN_130553_Extract(&zDGN, p_data);

    if (zDGN.u8HeaterInstance != l_130553_dgn.u8HeaterInstance)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8HeaterInstance);
        return false;
    }
    // Compare with last data
    if ((zDGN.u10FaultCode1 != NMEA2K_UINT10_NO_DATA) && (zDGN.u10FaultCode1 != l_130553_dgn.u10FaultCode1))
    {
        updated_data = true;
        value = l_130553_dgn.u10FaultCode1 = zDGN.u10FaultCode1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRFAULT0ONE, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u10FaultCode2 != NMEA2K_UINT10_NO_DATA) && (zDGN.u10FaultCode2 != l_130553_dgn.u10FaultCode2))
    {
        updated_data = true;
        value = l_130553_dgn.u10FaultCode2 = zDGN.u10FaultCode2;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRFAULT0TWO, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u10FaultCode3 != NMEA2K_UINT10_NO_DATA) && (zDGN.u10FaultCode3 != l_130553_dgn.u10FaultCode3))
    {
        updated_data = true;
        value = l_130553_dgn.u10FaultCode3 = zDGN.u10FaultCode3;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRFAULT0THREE, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u10FaultCode4 != NMEA2K_UINT10_NO_DATA) && (zDGN.u10FaultCode4 != l_130553_dgn.u10FaultCode4))
    {
        updated_data = true;
        value = l_130553_dgn.u10FaultCode4 = zDGN.u10FaultCode4;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRFAULT0FOUR, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2CriticalFault != NMEA2K_UINT2_NO_DATA) && (zDGN.u2CriticalFault != l_130553_dgn.u2CriticalFault))
    {
        updated_data = true;
        value = l_130553_dgn.u2CriticalFault = zDGN.u2CriticalFault;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRFAULT0CRIT, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2WarningFault != NMEA2K_UINT2_NO_DATA) && (zDGN.u2WarningFault != l_130553_dgn.u2WarningFault))
    {
        updated_data = true;
        value = l_130553_dgn.u2WarningFault = zDGN.u2WarningFault;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRFAULT0WARN, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRFAULT0SYNC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCHTRFAULT0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCHTRFAULT0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCHTRFAULT0INST:
            value = l_130553_dgn.u8HeaterInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCHTRFAULT0WARN:
            if (l_130553_dgn.u2WarningFault != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130553_dgn.u2WarningFault;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRFAULT0CRIT:
            if (l_130553_dgn.u2CriticalFault != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130553_dgn.u2CriticalFault;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRFAULT0ONE:
            if (l_130553_dgn.u10FaultCode1 != NMEA2K_UINT10_NO_DATA)
            {
                value = l_130553_dgn.u10FaultCode1;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRFAULT0TWO:
            if (l_130553_dgn.u10FaultCode2 != NMEA2K_UINT10_NO_DATA)
            {
                value = l_130553_dgn.u10FaultCode2;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRFAULT0THREE:
            if (l_130553_dgn.u10FaultCode3 != NMEA2K_UINT10_NO_DATA)
            {
                value = l_130553_dgn.u10FaultCode3;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRFAULT0FOUR:
            if (l_130553_dgn.u10FaultCode4 != NMEA2K_UINT10_NO_DATA)
            {
                value = l_130553_dgn.u10FaultCode4;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Prepare a Heater Scheduling Command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130554PropDgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    PROPDGN_DGN_130554_Stuff(p_data, &l_130554_dgn);
    return true;
}

/**
 * @brief Heater scheduling status DGN received (130555)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130555PropDgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    PROPDGN_zDGN_130555 zDGN;
    int32_t value;
    bool updated_data = false;
    PROPDGN_DGN_130555_Extract(&zDGN, p_data);

    // Valid instance?
    if (zDGN.u8HeaterInstance != l_130553_dgn.u8HeaterInstance)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8HeaterInstance);
        return false;
    }
    // Compare with last data
    if ((zDGN.u2AirHtrTimerOnStat != NMEA2K_UINT2_NO_DATA) && (zDGN.u2AirHtrTimerOnStat != l_130555_dgn.u2AirHtrTimerOnStat))
    {
        updated_data = true;
        value = l_130555_dgn.u2AirHtrTimerOnStat = zDGN.u2AirHtrTimerOnStat;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0ATONST, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2AirHtrTimerOffStat != NMEA2K_UINT2_NO_DATA) && (zDGN.u2AirHtrTimerOffStat != l_130555_dgn.u2AirHtrTimerOffStat))
    {
        updated_data = true;
        value = l_130555_dgn.u2AirHtrTimerOffStat = zDGN.u2AirHtrTimerOffStat;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0ATOFFST, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u6AirHtrTimerOnHour != NMEA2K_UINT6_NO_DATA) && (zDGN.u6AirHtrTimerOnHour != l_130555_dgn.u6AirHtrTimerOnHour))
    {
        updated_data = true;
        value = l_130555_dgn.u6AirHtrTimerOnHour = zDGN.u6AirHtrTimerOnHour;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0ATONH, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0ATONH, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8AirHtrTimerOnMin != NMEA2K_UINT8_NO_DATA) && (zDGN.u8AirHtrTimerOnMin != l_130555_dgn.u8AirHtrTimerOnMin))
    {
        updated_data = true;
        value = l_130555_dgn.u8AirHtrTimerOnMin = zDGN.u8AirHtrTimerOnMin;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0ATONM, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0ATONM, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u6AirHtrTimerOffHour != NMEA2K_UINT6_NO_DATA) && (zDGN.u6AirHtrTimerOffHour != l_130555_dgn.u6AirHtrTimerOffHour))
    {
        updated_data = true;
        value = l_130555_dgn.u6AirHtrTimerOffHour = zDGN.u6AirHtrTimerOffHour;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0ATOFFH, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0ATOFFH, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8AirHtrTimerOffMin != NMEA2K_UINT8_NO_DATA) && (zDGN.u8AirHtrTimerOffMin != l_130555_dgn.u8AirHtrTimerOffMin))
    {
        updated_data = true;
        value = l_130555_dgn.u8AirHtrTimerOffMin = zDGN.u8AirHtrTimerOffMin;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0ATOFFH, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0ATOFFM, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2WtrHtrTimerOnStat != NMEA2K_UINT2_NO_DATA) && (zDGN.u2WtrHtrTimerOnStat != l_130555_dgn.u2WtrHtrTimerOnStat))
    {
        updated_data = true;
        value = l_130555_dgn.u2WtrHtrTimerOnStat = zDGN.u2WtrHtrTimerOnStat;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0WTONST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0WTONST, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u6WtrHtrTimerOnHour != NMEA2K_UINT6_NO_DATA) && (zDGN.u6WtrHtrTimerOnHour != l_130555_dgn.u6WtrHtrTimerOnHour))
    {
        updated_data = true;
        value = l_130555_dgn.u6WtrHtrTimerOnHour = zDGN.u6WtrHtrTimerOnHour;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0WTONH, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0WTONH, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8WtrHtrTimerOnMin != NMEA2K_UINT8_NO_DATA) && (zDGN.u8WtrHtrTimerOnMin != l_130555_dgn.u8WtrHtrTimerOnMin))
    {
        updated_data = true;
        value = l_130555_dgn.u8WtrHtrTimerOnMin = zDGN.u8WtrHtrTimerOnMin;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0WTONM, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0WTONM, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8WtrHtrKeepOnTime != NMEA2K_UINT8_NO_DATA) && (zDGN.u8WtrHtrKeepOnTime != l_130555_dgn.u8WtrHtrKeepOnTime))
    {
        updated_data = true;
        value = l_130555_dgn.u8WtrHtrKeepOnTime = zDGN.u8WtrHtrKeepOnTime;
        convert_rvc_to_ddm_system_value(RVCHTRSCHED0WKEEP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0WKEEP, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRSCHED0SYNC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCHTRSCHED0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCHTRSCHED0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        // Return status
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCHTRSCHED0INST:
            value = l_130555_dgn.u8HeaterInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCHTRSCHED0ATOFFST:
            if (l_130555_dgn.u2AirHtrTimerOffStat != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130555_dgn.u2AirHtrTimerOffStat;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0ATOFFH:
            if (l_130555_dgn.u6AirHtrTimerOffHour != NMEA2K_UINT6_NO_DATA)
            {
                value = l_130555_dgn.u6AirHtrTimerOffHour;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0ATOFFM:
            if (l_130555_dgn.u8AirHtrTimerOffMin != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130555_dgn.u8AirHtrTimerOffMin;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0ATONST:
            if (l_130555_dgn.u2AirHtrTimerOnStat != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130555_dgn.u2AirHtrTimerOnStat;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0ATONH:
            if (l_130555_dgn.u6AirHtrTimerOnHour != NMEA2K_UINT6_NO_DATA)
            {
                value = l_130555_dgn.u6AirHtrTimerOnHour;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0ATONM:
            if (l_130555_dgn.u8AirHtrTimerOnMin != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130555_dgn.u8AirHtrTimerOnMin;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0WTONST:
            if (l_130555_dgn.u2WtrHtrTimerOnStat != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130555_dgn.u2WtrHtrTimerOnStat;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0WTONH:
            if (l_130555_dgn.u6WtrHtrTimerOnHour != NMEA2K_UINT6_NO_DATA)
            {
                value = l_130555_dgn.u6WtrHtrTimerOnHour;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0WTONM:
            if (l_130555_dgn.u8WtrHtrTimerOnMin != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130555_dgn.u8WtrHtrTimerOnMin;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRSCHED0WKEEP:
            if (l_130555_dgn.u8WtrHtrKeepOnTime != NMEA2K_UINT8_NO_DATA)
            {
                value = l_130555_dgn.u8WtrHtrKeepOnTime;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        value = p_frame->frame.set.value.int32;
        if (convert_ddm_system_value_to_rvc_value(p_frame->frame.set.parameter, &value, true))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCHTRSCHED0ATOFFST:
                l_130555_dgn.u2AirHtrTimerOffStat = value;
                break;
            case RVCHTRSCHED0ATOFFH:
                l_130555_dgn.u6AirHtrTimerOffHour = value;
                break;
            case RVCHTRSCHED0ATOFFM:
                l_130555_dgn.u8AirHtrTimerOffMin = value;
                break;
            case RVCHTRSCHED0ATONST:
                l_130555_dgn.u2AirHtrTimerOnStat = value;
                break;
            case RVCHTRSCHED0ATONH:
                l_130555_dgn.u6AirHtrTimerOnHour = value;
                break;
            case RVCHTRSCHED0ATONM:
                l_130555_dgn.u8AirHtrTimerOnMin = value;
                break;
            case RVCHTRSCHED0WTONST:
                l_130555_dgn.u2WtrHtrTimerOnStat = value;
                break;
            case RVCHTRSCHED0WTONH:
                l_130555_dgn.u6WtrHtrTimerOnHour = value;
                break;
            case RVCHTRSCHED0WTONM:
                l_130555_dgn.u8WtrHtrTimerOnMin = value;
                break;
            case RVCHTRSCHED0WKEEP:
                l_130555_dgn.u8WtrHtrKeepOnTime = value;
                break;
            case RVCHTRSCHED0SYNC:
                // Set the command frame
                NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
                break;
            default:
                break;
            }
        }
    }
}

/**
 * @brief Heater status DGN received (130557)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130557PropDgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    int32_t value;
    bool updated_data = false;
    PROPDGN_zDGN_130557 zDGN;
    PROPDGN_DGN_130557_Extract(&zDGN, p_data);
    // Valid instance?
    if (zDGN.u8HeaterInstance != l_130557_dgn.u8HeaterInstance)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8HeaterInstance);
        return false;
    }
    // Compare with last data
    if ((zDGN.i16RoomTemp != NMEA2K_INT16_NO_DATA) && (zDGN.i16RoomTemp != l_130557_dgn.i16RoomTemp))
    {
        updated_data = true;
        value = l_130557_dgn.i16RoomTemp = zDGN.i16RoomTemp;
        convert_rvc_to_ddm_system_value(RVCHTRST0RTEMP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0RTEMP, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2ACPresent != NMEA2K_UINT2_NO_DATA) && (zDGN.u2ACPresent != l_130557_dgn.u2ACPresent))
    {
        updated_data = true;
        value = l_130557_dgn.u2ACPresent = zDGN.u2ACPresent;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0ACAVL, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u3ACHeaterAir != NMEA2K_UINT3_NO_DATA) && (zDGN.u3ACHeaterAir != l_130557_dgn.u3ACHeaterAir))
    {
        updated_data = true;
        value = l_130557_dgn.u3ACHeaterAir = zDGN.u3ACHeaterAir;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0ACAIRST, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u3ACHeaterWater != NMEA2K_UINT3_NO_DATA) && (zDGN.u3ACHeaterWater != l_130557_dgn.u3ACHeaterWater))
    {
        updated_data = true;
        value = l_130557_dgn.u3ACHeaterWater = zDGN.u3ACHeaterWater;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0ACWTRST, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u3WaterTemp != NMEA2K_UINT3_NO_DATA) && (zDGN.u3WaterTemp != l_130557_dgn.u3WaterTemp))
    {
        updated_data = true;
        value = l_130557_dgn.u3WaterTemp = zDGN.u3WaterTemp;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0WTEMP, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4GasHeaterAir != NMEA2K_UINT4_NO_DATA) && (zDGN.u4GasHeaterAir != l_130557_dgn.u4GasHeaterAir))
    {
        updated_data = true;
        value = l_130557_dgn.u4GasHeaterAir = zDGN.u4GasHeaterAir;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0GAIRST, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4GasHeaterWater != NMEA2K_UINT4_NO_DATA) && (zDGN.u4GasHeaterWater != l_130557_dgn.u4GasHeaterWater))
    {
        updated_data = true;
        value = l_130557_dgn.u4GasHeaterWater = zDGN.u4GasHeaterWater;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0GWTRST, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHTRST0SYNC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCHTRST0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCHTRST0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCHTRST0INST:
            value = l_130557_dgn.u8HeaterInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCHTRST0RTEMP:
            if (l_130557_dgn.i16RoomTemp != NMEA2K_INT16_NO_DATA)
            {
                value = l_130557_dgn.i16RoomTemp;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRST0ACAVL:
            if (l_130557_dgn.u2ACPresent != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130557_dgn.u2ACPresent;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRST0ACAIRST:
            if (l_130557_dgn.u3ACHeaterAir != NMEA2K_UINT3_NO_DATA)
            {
                value = l_130557_dgn.u3ACHeaterAir;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRST0ACWTRST:
            if (l_130557_dgn.u3ACHeaterWater != NMEA2K_UINT3_NO_DATA)
            {
                value = l_130557_dgn.u3ACHeaterWater;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRST0WTEMP:
            if (l_130557_dgn.u3WaterTemp != NMEA2K_UINT3_NO_DATA)
            {
                value = l_130557_dgn.u3WaterTemp;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRST0GAIRST:
            if (l_130557_dgn.u4GasHeaterAir != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130557_dgn.u4GasHeaterAir;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHTRST0GWTRST:
            if (l_130557_dgn.u4GasHeaterWater != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130557_dgn.u4GasHeaterWater;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Prepare a HMI status frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130556PropDgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    PROPDGN_DGN_130556_Stuff(p_data, &l_130556_dgn);
    return true;
}

/**
 * @brief RVCHMI0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCHMI0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCHMI0INST:
            value = l_130556_dgn.u8HMIInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCHMI0RTEMP:
            if (l_130556_dgn.i16RoomTemperature != NMEA2K_INT16_NO_DATA)
            {
                value = l_130556_dgn.i16RoomTemperature;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHMI0COM:
            if (l_130556_dgn.u2HeaterCommunication != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130556_dgn.u2HeaterCommunication;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHMI0VOLT:
            if (l_130556_dgn.u2InputVoltage != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130556_dgn.u2InputVoltage;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHMI0INSTST:
            if (l_130556_dgn.u2HMIInstanceStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130556_dgn.u2HMIInstanceStatus;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHMI0ICIRC:
            if (l_130556_dgn.u2InternalCircuitry != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130556_dgn.u2InternalCircuitry;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHMI0FAVBTN:
            if (l_130556_dgn.u2FavoriteButton != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130556_dgn.u2FavoriteButton;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHMI0MENUBTN:
            if (l_130556_dgn.u2MenuButton != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130556_dgn.u2MenuButton;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHMI0HOMEBTN:
            if (l_130556_dgn.u2HomeButton != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130556_dgn.u2HomeButton;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        value = p_frame->frame.set.value.int32;
        if (convert_ddm_system_value_to_rvc_value(p_frame->frame.set.parameter, &value, true))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCHMI0RTEMP:
                l_130556_dgn.i16RoomTemperature = value;
                break;
            case RVCHMI0COM:
                l_130556_dgn.u2HeaterCommunication = value;
                break;
            case RVCHMI0VOLT:
                l_130556_dgn.u2InputVoltage = value;
                break;
            case RVCHMI0INSTST:
                l_130556_dgn.u2HMIInstanceStatus = value;
                break;
            case RVCHMI0ICIRC:
                l_130556_dgn.u2InternalCircuitry = value;
                break;
            case RVCHMI0FAVBTN:
                l_130556_dgn.u2FavoriteButton = value;
                break;
            case RVCHMI0MENUBTN:
                l_130556_dgn.u2MenuButton = value;
                break;
            case RVCHMI0HOMEBTN:
                l_130556_dgn.u2HomeButton = value;
                break;
            case RVCHMI0SYNC:
                // Set the command frame
                NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
                break;
            default:
                break;
            }
        }
    }
}

#endif
