/*
 * aircond.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "aircond.h"
#include "configuration.h"
#include "connector.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#if defined(RVC_CONFIG_IMPL_AIR_CONDITIONER) || defined(RVC_CONFIG_INTERF_AIR_CONDITIONER)
static EXT_RAM_ATTR RVCDGN_zDGN_131040 l_131040_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_131041 l_131041_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_130505 l_130505_dgn;

static EXT_RAM_ATTR uint8_t l_connector_id;

void aircond_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    uint32_t class = RVCAC0;
    broker_register_instance(&class, connector_id);

    memset(&l_130505_dgn, 0xFF, sizeof(l_130505_dgn));
    l_130505_dgn.u8Instance = 1;
    memset(&l_131041_dgn, 0xFF, sizeof(l_131041_dgn));
    l_131041_dgn.u8Instance = 1;
    memset(&l_131040_dgn, 0xFF, sizeof(l_131040_dgn));
    l_131040_dgn.u8Instance = 1;
}

/**
 * @brief RVCAC0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCAC0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = 0;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
    }
    else if (p_frame->frame.control == DDMP2_CONTROL_SET)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    }

    int32_t value;
#ifdef RVC_CONFIG_IMPL_AIR_CONDITIONER
    RVCDGN_zDGN_131040 *rec_dgn = &l_131040_dgn;
    RVCDGN_zDGN_131041 *send_dgn = &l_131041_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
    RVCDGN_zDGN_131041 *rec_dgn = &l_131041_dgn;
    RVCDGN_zDGN_131040 *send_dgn = &l_131040_dgn;
#endif
    if (instance >= 1)
    {
        LOG(E, "Wrong instance received: %d", instance);
        return;
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCAC0INST:
            value = rec_dgn->u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCAC0MODE:
            if (rec_dgn->u8OperatingMode != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8OperatingMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCAC0MFSPD:
            if (rec_dgn->u8MaxFanSpeed != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8MaxFanSpeed;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCAC0MOLVL:
            if (rec_dgn->u8MaxAirConOutLvl != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8MaxAirConOutLvl;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCAC0FSPD:
            if (rec_dgn->u8FanSpeed != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8FanSpeed;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCAC0OLVL:
            if (rec_dgn->u8AirConOutLvl != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8AirConOutLvl;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCAC0DBAND:
            if (rec_dgn->u8DeadBand != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8DeadBand;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCAC0SDBAND:
            if (rec_dgn->u8SecondStageDeadBand != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8SecondStageDeadBand;
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
            case RVCAC0MODE:
                send_dgn->u8OperatingMode = value;
                break;
            case RVCAC0MFSPD:
                send_dgn->u8MaxFanSpeed = value;
                break;
            case RVCAC0MOLVL:
                send_dgn->u8MaxAirConOutLvl = value;
                break;
            case RVCAC0FSPD:
                send_dgn->u8FanSpeed = value;
                break;
            case RVCAC0OLVL:
                send_dgn->u8AirConOutLvl = value;
                break;
            case RVCAC0DBAND:
                send_dgn->u8DeadBand = value;
                break;
            case RVCAC0SDBAND:
                send_dgn->u8SecondStageDeadBand = value;
                break;
            case RVCAC0SYNC:
                // Set the command frame
                NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
                break;
            default:
                break;
            }
        }
    }
}

#ifdef RVC_CONFIG_IMPL_AIR_CONDITIONER
/**
 * @brief Air Conditioner Command (131040) received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131040Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    bool updated_data = false;
    RVCDGN_zDGN_131040 zDGN;
    RVCDGN_DGN_131040_Extract(&zDGN, p_data);
    int class_instance = 0;
    // Valid instance and valid message type?
    if (zDGN.u8Instance != l_131040_dgn.u8Instance)
    {
        LOG(W, "Air conditioner Command instance wrong %d", zDGN.u8Instance);
        return false;
    }

    // Compare with last data
    if (zDGN.u8AirConOutLvl != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131040_dgn.u8AirConOutLvl = zDGN.u8AirConOutLvl;
        convert_rvc_to_ddm_system_value(RVCAC0OLVL, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0OLVL | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DeadBand != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131040_dgn.u8DeadBand = zDGN.u8DeadBand;
        convert_rvc_to_ddm_system_value(RVCAC0DBAND, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0DBAND | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8FanSpeed != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131040_dgn.u8FanSpeed = zDGN.u8FanSpeed;
        convert_rvc_to_ddm_system_value(RVCAC0FSPD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0FSPD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8MaxAirConOutLvl != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131040_dgn.u8MaxAirConOutLvl = zDGN.u8MaxAirConOutLvl;
        convert_rvc_to_ddm_system_value(RVCAC0MOLVL, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0MOLVL | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8MaxFanSpeed != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131040_dgn.u8MaxFanSpeed = zDGN.u8MaxFanSpeed;
        convert_rvc_to_ddm_system_value(RVCAC0MFSPD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0MFSPD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8OperatingMode != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131040_dgn.u8OperatingMode = zDGN.u8OperatingMode;
        convert_rvc_to_ddm_system_value(RVCAC0MODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0MODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8SecondStageDeadBand != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131040_dgn.u8SecondStageDeadBand = zDGN.u8SecondStageDeadBand;
        convert_rvc_to_ddm_system_value(RVCAC0SDBAND, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0SDBAND | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Prepare Air Conditioner status (131041) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131041Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= 1)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_131041_Stuff(p_data, &l_131041_dgn);
    return true;
}

/**
 * @brief Prepare Air Conditioner 2 status (130505) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130505Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= 1)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_130505_Stuff(p_data, &l_130505_dgn);
    return true;
}

/**
 * @brief RVC2AC0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVC2AC0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
    if (instance >= 1)
    {
        LOG(E, "Wrong instance received: %d", instance);
        return;
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCACTWO0INST:
            value = l_130505_dgn.u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCACTWO0COMPSTAT:
            if (l_130505_dgn.u4CompressorStatus != NMEA2K_UINT4_NO_DATA)
            {
                value = l_130505_dgn.u4CompressorStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCACTWO0NOISE:
            if (l_130505_dgn.u2ReducedNoiseMode != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130505_dgn.u2ReducedNoiseMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCACTWO0EXTTEMP:
            if (l_130505_dgn.u16ExtTemp != NMEA2K_UINT16_NO_DATA)
            {
                value = l_130505_dgn.u16ExtTemp;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCACTWO0COILTEMP:
            if (l_130505_dgn.u16CoilTemp != NMEA2K_UINT16_NO_DATA)
            {
                value = l_130505_dgn.u16CoilTemp;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCACTWO0COILTEMPERR:
            if (l_130505_dgn.u2CoilTempError != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130505_dgn.u2CoilTempError;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCACTWO0COILFREEZE:
            if (l_130505_dgn.u2CoilFreezeDetected != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130505_dgn.u2CoilFreezeDetected;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCACTWO0EXTTEMPERR:
            if (l_130505_dgn.u2ExtTempError != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130505_dgn.u2ExtTempError;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCACTWO0DEFROST:
            if (l_130505_dgn.u2DefrostCycleActive != NMEA2K_UINT2_NO_DATA)
            {
                value = l_130505_dgn.u2DefrostCycleActive;
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
            case RVCACTWO0COMPSTAT:
                l_130505_dgn.u4CompressorStatus = value;
                break;
            case RVCACTWO0NOISE:
                l_130505_dgn.u2ReducedNoiseMode = value;
                break;
            case RVCACTWO0EXTTEMP:
                l_130505_dgn.u16ExtTemp = value;
                break;
            case RVCACTWO0COILTEMP:
                l_130505_dgn.u16CoilTemp = value;
                break;
            case RVCACTWO0COILTEMPERR:
                l_130505_dgn.u2CoilTempError = value;
                break;
            case RVCACTWO0COILFREEZE:
                l_130505_dgn.u2CoilFreezeDetected = value;
                break;
            case RVCACTWO0EXTTEMPERR:
                l_130505_dgn.u2ExtTempError = value;
                break;
            case RVCACTWO0DEFROST:
                l_130505_dgn.u2DefrostCycleActive = value;
                break;
            case RVCACTWO0SYNC:
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
#ifdef RVC_CONFIG_INTERF_AIR_CONDITIONER
/**
 * @brief Air Conditioner Status (131041) received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131041Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    bool updated_data = false;
    RVCDGN_zDGN_131041 zDGN;
    RVCDGN_DGN_131041_Extract(&zDGN, p_data);
    int class_instance = 0;
    // Valid instance and valid message type?
    if (zDGN.u8Instance != l_131041_dgn.u8Instance)
    {
        LOG(W, "Air conditioner Status instance wrong %d", zDGN.u8Instance);
        return false;
    }

    // Compare with last data
    if (zDGN.u8AirConOutLvl != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131041_dgn.u8AirConOutLvl = zDGN.u8AirConOutLvl;
        convert_rvc_to_ddm_system_value(RVCAC0OLVL, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0OLVL | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DeadBand != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131041_dgn.u8DeadBand = zDGN.u8DeadBand;
        convert_rvc_to_ddm_system_value(RVCAC0DBAND, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0DBAND | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8FanSpeed != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131041_dgn.u8FanSpeed = zDGN.u8FanSpeed;
        convert_rvc_to_ddm_system_value(RVCAC0FSPD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0FSPD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8MaxAirConOutLvl != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131041_dgn.u8MaxAirConOutLvl = zDGN.u8MaxAirConOutLvl;
        convert_rvc_to_ddm_system_value(RVCAC0MOLVL, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0MOLVL | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8MaxFanSpeed != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131041_dgn.u8MaxFanSpeed = zDGN.u8MaxFanSpeed;
        convert_rvc_to_ddm_system_value(RVCAC0MFSPD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0MFSPD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8OperatingMode != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131041_dgn.u8OperatingMode = zDGN.u8OperatingMode;
        convert_rvc_to_ddm_system_value(RVCAC0MODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0MODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8SecondStageDeadBand != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_131041_dgn.u8SecondStageDeadBand = zDGN.u8SecondStageDeadBand;
        convert_rvc_to_ddm_system_value(RVCAC0SDBAND, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0SDBAND | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCAC0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Prepare Air Conditioner command (131040) frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131040Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= 1)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_131040_Stuff(p_data, &l_131040_dgn);
    return true;
}
#endif

#endif
