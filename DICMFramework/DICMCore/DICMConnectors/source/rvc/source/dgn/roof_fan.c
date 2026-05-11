/*
 * roof_fan.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"
#include "esp_attr.h"
#include "roof_fan.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#if defined(RVC_CONFIG_IMPL_ROOF_FAN) || defined(RVC_CONFIG_INTERF_ROOF_FAN)
static EXT_RAM_ATTR RVCDGN_zDGN_130530 l_130530_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_130531 l_130531_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_130726 l_130726_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_130727 l_130727_dgn;

static EXT_RAM_ATTR uint8_t l_connector_id;
#endif

#if defined(RVC_CONFIG_IMPL_ROOF_FAN) || defined(RVC_CONFIG_INTERF_ROOF_FAN)
void roof_fan_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    uint32_t class = RVCRFANTWO0;
    int instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130530_dgn, 0xFF, sizeof(l_130530_dgn));
    l_130530_dgn.u8Instance = 1;
    memset(&l_130531_dgn, 0xFF, sizeof(l_130531_dgn));
    l_130531_dgn.u8Instance = 1;

    class = RVCRFAN0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    memset(&l_130726_dgn, 0xFF, sizeof(l_130726_dgn));
    l_130726_dgn.u8Instance = 1;
    memset(&l_130727_dgn, 0xFF, sizeof(l_130727_dgn));
    l_130727_dgn.u8Instance = 1;
}
#endif

#if defined(RVC_CONFIG_IMPL_ROOF_FAN) || defined(RVC_CONFIG_INTERF_ROOF_FAN)
/**
 * @brief RVCRFAN0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCRFAN0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
    RVCDGN_zDGN_130726 *p_rec_dgn = &l_130726_dgn;
    RVCDGN_zDGN_130727 *p_send_dgn = &l_130727_dgn;
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
    RVCDGN_zDGN_130727 *p_rec_dgn = &l_130727_dgn;
    RVCDGN_zDGN_130726 *p_send_dgn = &l_130726_dgn;
#endif
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCRFAN0INST:
            value = p_rec_dgn->u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCRFAN0SYST:
            if (p_rec_dgn->u2SystemStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2SystemStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFAN0FMODE:
            if (p_rec_dgn->u2FanMode != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2FanMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFAN0SMODE:
            if (p_rec_dgn->u2SpeedMode != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2SpeedMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFAN0LGT:
            if (p_rec_dgn->u2Light != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2Light;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFAN0FSET:
            if (p_rec_dgn->u8FanSpeedSetting != NMEA2K_UINT8_NO_DATA)
            {
                value = p_rec_dgn->u8FanSpeedSetting;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFAN0WDIR:
            if (p_rec_dgn->u2WindDirectionSwitch != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2WindDirectionSwitch;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
        case RVCRFAN0DPOS:
            if (p_rec_dgn->u4DomePosition != NMEA2K_UINT4_NO_DATA)
            {
                value = p_rec_dgn->u4DomePosition;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#endif
        case RVCRFAN0TEMP:
            if (p_rec_dgn->u16AmbientTemp != NMEA2K_UINT16_NO_DATA)
            {
                value = p_rec_dgn->u16AmbientTemp;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFAN0SETTEMP:
            if (p_rec_dgn->u16SetPoint != NMEA2K_UINT16_NO_DATA)
            {
                value = p_rec_dgn->u16SetPoint;
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
            case RVCRFAN0SYST:
                p_send_dgn->u2SystemStatus = value;
                break;
            case RVCRFAN0FMODE:
                p_send_dgn->u2FanMode = value;
                break;
            case RVCRFAN0SMODE:
                p_send_dgn->u2SpeedMode = value;
                break;
            case RVCRFAN0LGT:
                p_send_dgn->u2Light = value;
                break;
            case RVCRFAN0FSET:
                p_send_dgn->u8FanSpeedSetting = value;
                break;
            case RVCRFAN0WDIR:
                p_send_dgn->u2WindDirectionSwitch = value;
                break;
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
            case RVCRFAN0DPOS:
                p_send_dgn->u4DomePosition = value;
                break;
#endif
            case RVCRFAN0TEMP:
                p_send_dgn->u16AmbientTemp = value;
                break;
            case RVCRFAN0SETTEMP:
                p_send_dgn->u16SetPoint = value;
                break;
            case RVCRFAN0SYNC:
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
 * @brief RVCRFANTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCRFANTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
    RVCDGN_zDGN_130530 *p_rec_dgn = &l_130530_dgn;
    RVCDGN_zDGN_130531 *p_send_dgn = &l_130531_dgn;
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
    RVCDGN_zDGN_130531 *p_rec_dgn = &l_130531_dgn;
    RVCDGN_zDGN_130530 *p_send_dgn = &l_130530_dgn;
#endif
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCRFANTWO0INST:
            value = p_rec_dgn->u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCRFANTWO0DMODE:
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
            if (p_rec_dgn->u8DomeCommand != NMEA2K_UINT8_NO_DATA)
            {
                value = p_rec_dgn->u8DomeCommand;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
            if (p_rec_dgn->u8DomeMode != NMEA2K_UINT8_NO_DATA)
            {
                value = p_rec_dgn->u8DomeMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
#endif
            break;
        case RVCRFANTWO0DPOS:
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
            if (p_rec_dgn->u8DesiredDomePosition != NMEA2K_UINT8_NO_DATA)
            {
                value = p_rec_dgn->u8DesiredDomePosition;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
            if (p_rec_dgn->u8DomePosition != NMEA2K_UINT8_NO_DATA)
            {
                value = p_rec_dgn->u8DomePosition;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
#endif
            break;
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
        case RVCRFANTWO0RAINSNS:
            if (p_rec_dgn->u2RainSensor != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2RainSensor;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#endif
        case RVCRFANTWO0RAINSNSOV:
            if (p_rec_dgn->u2RainSensorOverride != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2RainSensorOverride;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
        case RVCRFANTWO0DSTATE:
            if (p_rec_dgn->u2SetpointCtrldDome != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2SetpointCtrldDome;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
            if (p_rec_dgn->u2SetpointCtrldDomeState != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2SetpointCtrldDomeState;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
#endif
            break;

        case RVCRFANTWO0DCFOFF:
            if (p_rec_dgn->u2AutoCloseDomeonFanOff != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2AutoCloseDomeonFanOff;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFANTWO0FOFFDC:
            if (p_rec_dgn->u2AutoFanOffonDomeClose != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2AutoFanOffonDomeClose;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
        case RVCRFANTWO0FSPDST:
            if (p_rec_dgn->u2FanSpeedIncDec != NMEA2K_UINT2_NO_DATA)
            {
                value = p_rec_dgn->u2FanSpeedIncDec;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCRFANTWO0FSPDSTSET:
            if (p_rec_dgn->u6FanSpeedIncDecStep != NMEA2K_UINT6_NO_DATA)
            {
                value = p_rec_dgn->u6FanSpeedIncDecStep;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
        case RVCRFANTWO0FSPDSTSUP:
            if (p_rec_dgn->u6FanStepsSupported != NMEA2K_UINT6_NO_DATA)
            {
                value = p_rec_dgn->u6FanStepsSupported;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#endif
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
            case RVCRFANTWO0DMODE:
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
                p_send_dgn->u8DomeMode = value;
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
                p_send_dgn->u8DomeCommand = value;
#endif
                break;
            case RVCRFANTWO0DPOS:
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
                p_send_dgn->u8DomePosition = value;
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
                p_send_dgn->u8DesiredDomePosition = value;
#endif
                break;
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
            case RVCRFANTWO0RAINSNS:
                p_send_dgn->u2RainSensor = value;
                break;
#endif
            case RVCRFANTWO0RAINSNSOV:
                p_send_dgn->u2RainSensorOverride = value;
                break;
            case RVCRFANTWO0DSTATE:
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
                p_send_dgn->u2SetpointCtrldDomeState = value;
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
                p_send_dgn->u2SetpointCtrldDome = value;
#endif
                break;
            case RVCRFANTWO0DCFOFF:
                p_send_dgn->u2AutoCloseDomeonFanOff = value;
                break;
            case RVCRFANTWO0FOFFDC:
                p_send_dgn->u2AutoFanOffonDomeClose = value;
                break;
#if defined(RVC_CONFIG_IMPL_ROOF_FAN)
            case RVCRFANTWO0FSPDSTSUP:
                p_send_dgn->u6FanStepsSupported = value;
                break;
#endif
#if defined(RVC_CONFIG_INTERF_ROOF_FAN)
            case RVCRFANTWO0FSPDST:
                p_send_dgn->u2FanSpeedIncDec = value;
                break;
            case RVCRFANTWO0FSPDSTSET:
                p_send_dgn->u6FanSpeedIncDecStep = value;
                break;
#endif
            case RVCRFANTWO0SYNC:
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

#ifdef RVC_CONFIG_IMPL_ROOF_FAN
/**
 * @brief RVC DGN Roof Fan Command DGN (130726) received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130726Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    int class_instance = 0;
    bool updated_data = false;
    // Extract message content
    RVCDGN_zDGN_130726 zDGN;
    RVCDGN_DGN_130726_Extract(&zDGN, p_data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance != l_130726_dgn.u8Instance)
    {
        LOG(W, "Roof Fan Command instance wrong %d", zDGN.u8Instance);
        return false;
    }
    // Compare with last data
    if (zDGN.u2SystemStatus != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u2SystemStatus = zDGN.u2SystemStatus;
        convert_rvc_to_ddm_system_value(RVCRFAN0SYST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SYST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FanMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u2FanMode = zDGN.u2FanMode;
        convert_rvc_to_ddm_system_value(RVCRFAN0FMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0FMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2SpeedMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u2SpeedMode = zDGN.u2SpeedMode;
        convert_rvc_to_ddm_system_value(RVCRFAN0SMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Light != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u2Light = zDGN.u2Light;
        convert_rvc_to_ddm_system_value(RVCRFAN0LGT, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0LGT | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8FanSpeedSetting != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u8FanSpeedSetting = zDGN.u8FanSpeedSetting;
        convert_rvc_to_ddm_system_value(RVCRFAN0FSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0FSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2WindDirectionSwitch != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u2WindDirectionSwitch = zDGN.u2WindDirectionSwitch;
        convert_rvc_to_ddm_system_value(RVCRFAN0WDIR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0WDIR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16AmbientTemp != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u16AmbientTemp = zDGN.u16AmbientTemp;
        convert_rvc_to_ddm_system_value(RVCRFAN0TEMP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0TEMP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SetPoint != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130726_dgn.u16SetPoint = zDGN.u16SetPoint;
        convert_rvc_to_ddm_system_value(RVCRFAN0SETTEMP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SETTEMP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVC DGN Roof Fan 2 Command DGN (130530) received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130530Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    int class_instance = 0;
    bool updated_data = false;
    // Extract message content
    RVCDGN_zDGN_130530 zDGN;
    RVCDGN_DGN_130530_Extract(&zDGN, p_data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance != l_130530_dgn.u8Instance)
    {
        LOG(W, "Roof Fan 2 Command instance wrong %d", zDGN.u8Instance);
        return false;
    }
    // Compare with last data
    if (zDGN.u8DomeCommand != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u8DomeCommand = zDGN.u8DomeCommand;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DesiredDomePosition != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u8DesiredDomePosition = zDGN.u8DesiredDomePosition;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DPOS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DPOS | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2RainSensorOverride != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u2RainSensorOverride = zDGN.u2RainSensorOverride;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0RAINSNSOV, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0RAINSNSOV | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2SetpointCtrldDome != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u2SetpointCtrldDome = zDGN.u2SetpointCtrldDome;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DSTATE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DSTATE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2AutoCloseDomeonFanOff != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u2AutoCloseDomeonFanOff = zDGN.u2AutoCloseDomeonFanOff;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DCFOFF, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DCFOFF | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2AutoFanOffonDomeClose != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u2AutoFanOffonDomeClose = zDGN.u2AutoFanOffonDomeClose;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0FOFFDC, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0FOFFDC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FanSpeedIncDec != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u2FanSpeedIncDec = zDGN.u2FanSpeedIncDec;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0FSPDST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0FSPDST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u6FanSpeedIncDecStep != NMEA2K_UINT6_NO_DATA)
    {
        updated_data = true;
        value = l_130530_dgn.u6FanSpeedIncDecStep = zDGN.u6FanSpeedIncDecStep;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0FSPDSTSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0FSPDSTSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare RVC DGN Roof Fan status (130727) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130727Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= 1)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_130727_Stuff(p_data, &l_130727_dgn);
    return true;
}

/**
 * @brief Prepare RVC DGN Roof Fan 2 status (130531) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130531Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= 1)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_130531_Stuff(p_data, &l_130531_dgn);
    return true;
}
#endif
#ifdef RVC_CONFIG_INTERF_ROOF_FAN
/**
 * @brief RVC DGN Roof Fan Status DGN (130727) received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130727Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    int class_instance = 0;
    bool updated_data = false;
    // Extract message content
    RVCDGN_zDGN_130727 zDGN;
    RVCDGN_DGN_130727_Extract(&zDGN, p_data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance != l_130727_dgn.u8Instance)
    {
        LOG(W, "Roof Fan Status instance wrong %d", zDGN.u8Instance);
        return false;
    }
    // Compare with last data
    if (zDGN.u2SystemStatus != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u2SystemStatus = zDGN.u2SystemStatus;
        convert_rvc_to_ddm_system_value(RVCRFAN0SYST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SYST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FanMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u2FanMode = zDGN.u2FanMode;
        convert_rvc_to_ddm_system_value(RVCRFAN0FMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0FMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2SpeedMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u2SpeedMode = zDGN.u2SpeedMode;
        convert_rvc_to_ddm_system_value(RVCRFAN0SMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Light != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u2Light = zDGN.u2Light;
        convert_rvc_to_ddm_system_value(RVCRFAN0LGT, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0LGT | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8FanSpeedSetting != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u8FanSpeedSetting = zDGN.u8FanSpeedSetting;
        convert_rvc_to_ddm_system_value(RVCRFAN0FSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0FSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2WindDirectionSwitch != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u2WindDirectionSwitch = zDGN.u2WindDirectionSwitch;
        convert_rvc_to_ddm_system_value(RVCRFAN0WDIR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0WDIR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4DomePosition != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u4DomePosition = zDGN.u4DomePosition;
        convert_rvc_to_ddm_system_value(RVCRFAN0DPOS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0DPOS | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16AmbientTemp != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u16AmbientTemp = zDGN.u16AmbientTemp;
        convert_rvc_to_ddm_system_value(RVCRFAN0TEMP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0TEMP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SetPoint != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130727_dgn.u16SetPoint = zDGN.u16SetPoint;
        convert_rvc_to_ddm_system_value(RVCRFAN0SETTEMP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SETTEMP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFAN0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVC DGN Roof Fan 2 Status DGN (130531) received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130531Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    int class_instance = 0;
    bool updated_data = false;
    // Extract message content
    RVCDGN_zDGN_130531 zDGN;
    RVCDGN_DGN_130531_Extract(&zDGN, p_data);

    // Valid instance and valid message type?
    if (zDGN.u8Instance != l_130531_dgn.u8Instance)
    {
        LOG(W, "Roof Fan 2 Status instance wrong %d", zDGN.u8Instance);
        return false;
    }
    // Compare with last data
    if (zDGN.u8DomeMode != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u8DomeMode = zDGN.u8DomeMode;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DomePosition != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u8DomePosition = zDGN.u8DomePosition;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DPOS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DPOS | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2RainSensor != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u2RainSensor = zDGN.u2RainSensor;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0RAINSNS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0RAINSNS | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2RainSensorOverride != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u2RainSensorOverride = zDGN.u2RainSensorOverride;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0RAINSNSOV, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0RAINSNSOV | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2SetpointCtrldDomeState != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u2SetpointCtrldDomeState = zDGN.u2SetpointCtrldDomeState;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DSTATE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DSTATE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2AutoCloseDomeonFanOff != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u2AutoCloseDomeonFanOff = zDGN.u2AutoCloseDomeonFanOff;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0DCFOFF, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0DCFOFF | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2AutoFanOffonDomeClose != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u2AutoFanOffonDomeClose = zDGN.u2AutoFanOffonDomeClose;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0FOFFDC, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0FOFFDC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u6FanStepsSupported != NMEA2K_UINT6_NO_DATA)
    {
        updated_data = true;
        value = l_130531_dgn.u6FanStepsSupported = zDGN.u6FanStepsSupported;
        convert_rvc_to_ddm_system_value(RVCRFANTWO0FSPDSTSUP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0FSPDSTSUP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCRFANTWO0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare RVC DGN Roof Fan command (130726) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130726Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= 1)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_130726_Stuff(p_data, &l_130726_dgn);
    return true;
}

/**
 * @brief Prepare RVC DGN Roof Fan 2 command (130530) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130530Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= 1)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_130530_Stuff(p_data, &l_130530_dgn);
    return true;
}
#endif
