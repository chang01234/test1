/*
 * heatpump.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/queue.h>

#include "configuration.h"
#include "connector.h"
#include "dgnnode.h"
#include "heatpump.h"
#include "rvc_to_ddm.h"
#include "thermostat.h"  // MAX_NUM_OF_ZONE_INSTANCES

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#ifdef RVC_CONFIG_INTERF_HEAT_PUMP
static EXT_RAM_ATTR list_t l_130971_dgn;
static EXT_RAM_ATTR list_t l_130970_dgn;

static EXT_RAM_ATTR uint8_t l_connector_id;

void heatpump_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    LIST_INIT(&l_130971_dgn);
    LIST_INIT(&l_130970_dgn);
}

/**
 * @brief Prepare a Heat Pump command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130970Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // Stuff message data
        DgnNode_t *dgn_node = NULL;
        dgn_node = DgnNodeFindByDdmInstance(&l_130970_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130970_Stuff(p_data, dgn_node->dgn_data);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        LOG(E, "Wrong class instance %d", instance);
        return false;
    }
    return false;
}

/**
 * @brief Heat Pump Status DGN received (130971)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130971Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130971 zDGN;
    int32_t value;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    RVCDGN_DGN_130971_Extract(&zDGN, p_data);
    DgnNode_t *dgn_node = NULL;
    // Compare with last data
    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Heat Pump Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8Instance;
        convert_rvc_to_ddm_system_value(RVCHPUMP0INST, &value);
        dgn_node = DgnNodeFindBySourceAddress(&l_130971_dgn, sa);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCHPUMP0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130971));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130971_dgn);

            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130970));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130970_dgn);
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8OperatingMode != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8OperatingMode;
        convert_rvc_to_ddm_system_value(RVCHPUMP0MODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0MODE | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8MaxHeatOutputLevel != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8MaxHeatOutputLevel;
        convert_rvc_to_ddm_system_value(RVCHPUMP0MOLVL, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0MOLVL | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8HeatOutputLevel != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8HeatOutputLevel;
        convert_rvc_to_ddm_system_value(RVCHPUMP0OLVL, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0OLVL | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DeadBand != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8DeadBand;
        convert_rvc_to_ddm_system_value(RVCHPUMP0DBAND, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0DBAND | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8SecondStageDeadBand != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8SecondStageDeadBand;
        convert_rvc_to_ddm_system_value(RVCHPUMP0SDBAND, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0SDBAND | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8FanSpeed != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8FanSpeed;
        convert_rvc_to_ddm_system_value(RVCHPUMP0FSPD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0FSPD | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCHPUMP0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCHPUMP0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCHPUMP0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130971 zDGN;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130971_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130971_Extract(&zDGN, dgn_node->dgn_data);
        }
    }
    else if (p_frame->frame.control == DDMP2_CONTROL_SET)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_130970_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130970_Extract(&zDGN, dgn_node->dgn_data);
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCHPUMP0INST:
            value = zDGN.u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCHPUMP0MODE:
            if (zDGN.u8OperatingMode != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8OperatingMode;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHPUMP0MOLVL:
            if (zDGN.u8MaxHeatOutputLevel != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8MaxHeatOutputLevel;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHPUMP0OLVL:
            if (zDGN.u8HeatOutputLevel != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8HeatOutputLevel;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHPUMP0DBAND:
            if (zDGN.u8DeadBand != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DeadBand;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHPUMP0SDBAND:
            if (zDGN.u8SecondStageDeadBand != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8SecondStageDeadBand;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCHPUMP0FSPD:
            if (zDGN.u8FanSpeed != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8FanSpeed;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
    else if (p_frame->frame.control == DDMP2_CONTROL_SET)
    {
        value = p_frame->frame.set.value.int32;

        if (convert_ddm_system_value_to_rvc_value(p_frame->frame.set.parameter, &value, true))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCHPUMP0MODE:
                zDGN.u8OperatingMode = value;
                break;
            case RVCHPUMP0MOLVL:
                zDGN.u8MaxHeatOutputLevel = value;
                break;
            case RVCHPUMP0OLVL:
                zDGN.u8HeatOutputLevel = value;
                break;
            case RVCHPUMP0DBAND:
                zDGN.u8DeadBand = value;
                break;
            case RVCHPUMP0SDBAND:
                zDGN.u8SecondStageDeadBand = value;
                break;
            case RVCHPUMP0FSPD:
                zDGN.u8FanSpeed = value;
                break;
            case RVCHPUMP0SYNC:
                // Set the command frame
                NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
                break;
            default:
                break;
            }
            if (dgn_node != NULL)
            {
                DgnNodeUpdate(dgn_node, &zDGN);
            }
        }
    }
}

#endif
