/*
 * dimmer.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"
#include "dimmer.h"
#include "rvc_to_ddm.h"
#include "thermostat.h"  // MAX_NUM_OF_ZONE_INSTANCES

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1) || defined(RVC_CONFIG_INTERF_DC_DIMMER_1)

#if defined(RVC_CONFIG_IMPL_DC_DIMMER_2) || defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
#define MAX_NUM_OF_DIMMER_INSTANCES (2)
#elif defined(RVC_CONFIG_IMPL_DC_DIMMER_1) || defined(RVC_CONFIG_INTERF_DC_DIMMER_1)
#define MAX_NUM_OF_DIMMER_INSTANCES (1)
#else
#define MAX_NUM_OF_DIMMER_INSTANCES (0)
#endif

static EXT_RAM_ATTR RVCDGN_zDGN_130778 l_130778_dgn[MAX_NUM_OF_DIMMER_INSTANCES];
static EXT_RAM_ATTR RVCDGN_zDGN_130779 l_130779_dgn[MAX_NUM_OF_DIMMER_INSTANCES];

static EXT_RAM_ATTR uint8_t l_connector_id;

void dimmer_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    uint32_t class = RVCDIMTHR0;
    int instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    l_130778_dgn[instance].u8Instance = 1;
    l_130778_dgn[instance].u2OverrideStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2EnableStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2InterlockStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2LoadStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2LockStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2OvercurrentStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2ReservedField1 = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2UnderCurrent = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u8MasterMemoryValue = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8DelayDuration = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8Group = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8LastCommand = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8OperatingStatusBrightness = NMEA2K_UINT8_NO_DATA;
    class = RVCDIMTWO0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);

    l_130779_dgn[instance].u8Instance = 1;
    l_130779_dgn[instance].u8ReservedField2 = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8RampTime = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u2InterlockStatus = NMEA2K_UINT2_NO_DATA;
    l_130779_dgn[instance].u6ReservedField1 = NMEA2K_UINT6_NO_DATA;
    l_130779_dgn[instance].u8Command = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8DelayDuration = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8DesiredLevelBrightness = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8Group = NMEA2K_UINT8_NO_DATA;

#if defined(RVC_CONFIG_IMPL_DC_DIMMER_2) || defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
    class = RVCDIMTHR0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130778_dgn[instance].u8Instance = 2;
    l_130778_dgn[instance].u2OverrideStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2EnableStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2InterlockStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2LoadStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2LockStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2OvercurrentStatus = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2ReservedField1 = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u2UnderCurrent = NMEA2K_UINT2_NO_DATA;
    l_130778_dgn[instance].u8MasterMemoryValue = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8DelayDuration = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8Group = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8LastCommand = NMEA2K_UINT8_NO_DATA;
    l_130778_dgn[instance].u8OperatingStatusBrightness = NMEA2K_UINT8_NO_DATA;
    class = RVCDIMTWO0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130779_dgn[instance].u8Instance = 2;
    l_130779_dgn[instance].u8ReservedField2 = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8RampTime = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u2InterlockStatus = NMEA2K_UINT2_NO_DATA;
    l_130779_dgn[instance].u6ReservedField1 = NMEA2K_UINT6_NO_DATA;
    l_130779_dgn[instance].u8Command = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8DelayDuration = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8DesiredLevelBrightness = NMEA2K_UINT8_NO_DATA;
    l_130779_dgn[instance].u8Group = NMEA2K_UINT8_NO_DATA;
#endif
}

/**
 * @brief RVC3DIM0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVC3DIM0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
    if (instance >= MAX_NUM_OF_DIMMER_INSTANCES)
    {
        LOG(E, "Wrong instance received: %d", instance);
        return;
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDIMTHR0INST:
            value = l_130778_dgn[instance].u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDIMTHR0GROUP:
            value = l_130778_dgn[instance].u8Group;
            if (l_130778_dgn[instance].u8Group != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0LVLBS:
            value = l_130778_dgn[instance].u8OperatingStatusBrightness;
            if (l_130778_dgn[instance].u8OperatingStatusBrightness != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0LOCK:
            value = l_130778_dgn[instance].u2LockStatus;
            if (l_130778_dgn[instance].u2LockStatus != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0OCURR:
            value = l_130778_dgn[instance].u2OvercurrentStatus;
            if (l_130778_dgn[instance].u2OvercurrentStatus != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0ORIDE:
            value = l_130778_dgn[instance].u2OverrideStatus;
            if (l_130778_dgn[instance].u2OverrideStatus != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0EN:
            value = l_130778_dgn[instance].u2EnableStatus;
            if (l_130778_dgn[instance].u2EnableStatus != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0DEL_DUR:
            value = l_130778_dgn[instance].u8DelayDuration;
            if (l_130778_dgn[instance].u8DelayDuration != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0LCMD:
            value = l_130778_dgn[instance].u8LastCommand;
            if (l_130778_dgn[instance].u8LastCommand != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0ILOCK:
            value = l_130778_dgn[instance].u2InterlockStatus;
            if (l_130778_dgn[instance].u2InterlockStatus != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0LOAD:
            value = l_130778_dgn[instance].u2LoadStatus;
            if (l_130778_dgn[instance].u2LoadStatus != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0UCURR:
            value = l_130778_dgn[instance].u2UnderCurrent;
            if (l_130778_dgn[instance].u2UnderCurrent != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTHR0MEM:
            value = l_130778_dgn[instance].u8MasterMemoryValue;
            if (l_130778_dgn[instance].u8MasterMemoryValue != NMEA2K_UINT8_NO_DATA)
            {
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
            case RVCDIMTHR0GROUP:
                l_130778_dgn[instance].u8Group = value;
                break;
            case RVCDIMTHR0LVLBS:
                l_130778_dgn[instance].u8OperatingStatusBrightness = value;
                break;
            case RVCDIMTHR0LOCK:
                l_130778_dgn[instance].u2LockStatus = value;
                break;
            case RVCDIMTHR0OCURR:
                l_130778_dgn[instance].u2OvercurrentStatus = value;
                break;
            case RVCDIMTHR0ORIDE:
                l_130778_dgn[instance].u2OverrideStatus = value;
                break;
            case RVCDIMTHR0EN:
                l_130778_dgn[instance].u2EnableStatus = value;
                break;
            case RVCDIMTHR0DEL_DUR:
                l_130778_dgn[instance].u8DelayDuration = value;
                break;
            case RVCDIMTHR0LCMD:
                l_130778_dgn[instance].u8LastCommand = value;
                break;
            case RVCDIMTHR0ILOCK:
                l_130778_dgn[instance].u2InterlockStatus = value;
                break;
            case RVCDIMTHR0LOAD:
                l_130778_dgn[instance].u2LoadStatus = value;
                break;
            case RVCDIMTHR0UCURR:
                l_130778_dgn[instance].u2UnderCurrent = value;
                break;
            case RVCDIMTHR0MEM:
                l_130778_dgn[instance].u8MasterMemoryValue = value;
                break;
            case RVCDIMTHR0SYNC:
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
 * @brief RVC2DIM0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVC2DIM0(uint32_t dgn, DDMP2_FRAME *p_frame)
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
    if (instance >= MAX_NUM_OF_DIMMER_INSTANCES)
    {
        LOG(E, "Wrong instance received: %d", instance);
        return;
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDIMTWO0INST:
            value = l_130779_dgn[instance].u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDIMTWO0GROUP:
            value = l_130779_dgn[instance].u8Group;
            if (l_130779_dgn[instance].u8Group != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTWO0LVLBS:
            value = l_130779_dgn[instance].u8DesiredLevelBrightness;
            if (l_130779_dgn[instance].u8DesiredLevelBrightness != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTWO0ILOCK:
            value = l_130779_dgn[instance].u2InterlockStatus;
            if (l_130779_dgn[instance].u2InterlockStatus != NMEA2K_UINT2_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTWO0CMD:
            value = l_130779_dgn[instance].u8Command;
            if (l_130779_dgn[instance].u8Command != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTWO0DEL_DUR:
            value = l_130779_dgn[instance].u8DelayDuration;
            if (l_130779_dgn[instance].u8DelayDuration != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDIMTWO0RTIME:
            value = l_130779_dgn[instance].u8RampTime;
            if (l_130779_dgn[instance].u8RampTime != NMEA2K_UINT8_NO_DATA)
            {
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
            case RVCDIMTWO0GROUP:
                l_130779_dgn[instance].u8Group = value;
                break;
            case RVCDIMTWO0LVLBS:
                l_130779_dgn[instance].u8DesiredLevelBrightness = value;
                break;
            case RVCDIMTWO0ILOCK:
                l_130779_dgn[instance].u2InterlockStatus = value;
                break;
            case RVCDIMTWO0DEL_DUR:
                l_130779_dgn[instance].u8DelayDuration = value;
                break;
            case RVCDIMTWO0CMD:
                l_130779_dgn[instance].u8Command = value;
                break;
            case RVCDIMTWO0INST:
                l_130779_dgn[instance].u8Instance = value;
                break;
            case RVCDIMTWO0RTIME:
                l_130779_dgn[instance].u8RampTime = value;
                break;
            case RVCDIMTWO0SYNC:
                // Set the command frame
                NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
                break;
            default:
                break;
            }
            // publish back the value for the command because it will be not received back
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.set.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}

#if defined(RVC_CONFIG_INTERF_DC_DIMMER_1)
/**
 * @brief Prepare DC Dimmer Command 2 frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130779Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= MAX_NUM_OF_DIMMER_INSTANCES)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_130779_Stuff(p_data, &l_130779_dgn[instance]);
    return true;
}

/**
 * @brief Process received RVC DGN 130778 (DC Dimmer Status 3)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130778Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    bool updated_data = false;
    RVCDGN_zDGN_130778 zDGN;
    RVCDGN_DGN_130778_Extract(&zDGN, p_data);
    int class_instance = 0;
    // Valid instance and valid message type?
    if (zDGN.u8Instance == l_130778_dgn[0].u8Instance)
    {
        class_instance = 0;
    }
#if defined(RVC_CONFIG_INTERF_DC_DIMMER_2)
    else if (zDGN.u8Instance == l_130778_dgn[1].u8Instance)
    {
        class_instance = 1;
    }
#endif
    else
    {
        LOG(W, "DC Dimmer Command 2: wrong instance %d", zDGN.u8Instance);
        return false;
    }
    // Compare with last data
    if ((zDGN.u2InterlockStatus != NMEA2K_UINT2_NO_DATA) && (zDGN.u2InterlockStatus != l_130778_dgn[class_instance].u2InterlockStatus))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u2InterlockStatus = zDGN.u2InterlockStatus;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0ILOCK, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0ILOCK | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Group != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Group != l_130778_dgn[class_instance].u8Group))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u8Group = zDGN.u8Group;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0GROUP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0GROUP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8OperatingStatusBrightness != NMEA2K_UINT8_NO_DATA) && (zDGN.u8OperatingStatusBrightness != l_130778_dgn[class_instance].u8OperatingStatusBrightness))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u8OperatingStatusBrightness = zDGN.u8OperatingStatusBrightness;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0LVLBS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0LVLBS | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8DelayDuration != NMEA2K_UINT8_NO_DATA) && (zDGN.u8DelayDuration != l_130778_dgn[class_instance].u8DelayDuration))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u8DelayDuration = zDGN.u8DelayDuration;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0DEL_DUR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0DEL_DUR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8LastCommand != NMEA2K_UINT8_NO_DATA) && (zDGN.u8LastCommand != l_130778_dgn[class_instance].u8LastCommand))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u8LastCommand = zDGN.u8LastCommand;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0LCMD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0LCMD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2LockStatus != NMEA2K_UINT2_NO_DATA) && (zDGN.u2LockStatus != l_130778_dgn[class_instance].u2LockStatus))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u2LockStatus = zDGN.u2LockStatus;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0LOCK, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0LOCK | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2OvercurrentStatus != NMEA2K_UINT2_NO_DATA) && (zDGN.u2OvercurrentStatus != l_130778_dgn[class_instance].u2OvercurrentStatus))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u2OvercurrentStatus = zDGN.u2OvercurrentStatus;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0OCURR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0OCURR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2OverrideStatus != NMEA2K_UINT2_NO_DATA) && (zDGN.u2OverrideStatus != l_130778_dgn[class_instance].u2OverrideStatus))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u2OverrideStatus = zDGN.u2OverrideStatus;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0ORIDE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0ORIDE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2EnableStatus != NMEA2K_UINT2_NO_DATA) && (zDGN.u2EnableStatus != l_130778_dgn[class_instance].u2EnableStatus))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u2EnableStatus = zDGN.u2EnableStatus;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0EN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0EN | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2LoadStatus != NMEA2K_UINT2_NO_DATA) && (zDGN.u2LoadStatus != l_130778_dgn[class_instance].u2LoadStatus))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u2LoadStatus = zDGN.u2LoadStatus;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0LOAD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0LOAD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2UnderCurrent != NMEA2K_UINT2_NO_DATA) && (zDGN.u2UnderCurrent != l_130778_dgn[class_instance].u2UnderCurrent))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u2UnderCurrent = zDGN.u2UnderCurrent;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0UCURR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0UCURR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8MasterMemoryValue != NMEA2K_UINT8_NO_DATA) && (zDGN.u8MasterMemoryValue != l_130778_dgn[class_instance].u8MasterMemoryValue))
    {
        updated_data = true;
        value = l_130778_dgn[class_instance].u8MasterMemoryValue = zDGN.u8MasterMemoryValue;
        convert_rvc_to_ddm_system_value(RVCDIMTHR0MEM, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0MEM | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTHR0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}
#endif

#if defined(RVC_CONFIG_IMPL_DC_DIMMER_1)
/**
 * @brief Process received RVC DGN 130779 (DC Dimmer Command 2)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130779Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    int32_t value;
    bool updated_data = false;
    RVCDGN_zDGN_130779 zDGN;
    RVCDGN_DGN_130779_Extract(&zDGN, p_data);
    int class_instance = 0;
    // Valid instance and valid message type?
    if (zDGN.u8Instance == l_130779_dgn[0].u8Instance)
    {
        class_instance = 0;
    }
#if defined(RVC_CONFIG_IMPL_DC_DIMMER_2)
    else if (zDGN.u8Instance == l_130779_dgn[1].u8Instance)
    {
        class_instance = 1;
    }
#endif
    else
    {
        LOG(W, "DC Dimmer Command 2: wrong instance %d", zDGN.u8Instance);
        return false;
    }

    // Compare with last data
    if ((zDGN.u2InterlockStatus != NMEA2K_UINT2_NO_DATA) && (zDGN.u2InterlockStatus != l_130779_dgn[class_instance].u2InterlockStatus))
    {
        updated_data = true;
        value = l_130779_dgn[class_instance].u2InterlockStatus = zDGN.u2InterlockStatus;
        convert_rvc_to_ddm_system_value(RVCDIMTWO0ILOCK, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTWO0ILOCK | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Group != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Group != l_130779_dgn[class_instance].u8Group))
    {
        updated_data = true;
        value = l_130779_dgn[class_instance].u8Group = zDGN.u8Group;
        convert_rvc_to_ddm_system_value(RVCDIMTWO0GROUP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTWO0GROUP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8DesiredLevelBrightness != NMEA2K_UINT8_NO_DATA) && (zDGN.u8DesiredLevelBrightness != l_130779_dgn[class_instance].u8DesiredLevelBrightness))
    {
        updated_data = true;
        value = l_130779_dgn[class_instance].u8DesiredLevelBrightness = zDGN.u8DesiredLevelBrightness;
        convert_rvc_to_ddm_system_value(RVCDIMTWO0LVLBS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTWO0LVLBS | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8DelayDuration != NMEA2K_UINT8_NO_DATA) && (zDGN.u8DelayDuration != l_130779_dgn[class_instance].u8DelayDuration))
    {
        updated_data = true;
        value = l_130779_dgn[class_instance].u8DelayDuration = zDGN.u8DelayDuration;
        convert_rvc_to_ddm_system_value(RVCDIMTWO0DEL_DUR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTWO0DEL_DUR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8RampTime != NMEA2K_UINT8_NO_DATA) && (zDGN.u8RampTime != l_130779_dgn[class_instance].u8RampTime))
    {
        updated_data = true;
        value = l_130779_dgn[class_instance].u8RampTime = zDGN.u8RampTime;
        convert_rvc_to_ddm_system_value(RVCDIMTWO0RTIME, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTWO0ILOCK | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDIMTWO0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Prepare DC Dimmer Status 3 frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130778Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance >= MAX_NUM_OF_DIMMER_INSTANCES)
    {
        return false;
    }
    // Stuff message data
    RVCDGN_DGN_130778_Stuff(p_data, &l_130778_dgn[instance]);
    return true;
}

#endif

#endif
