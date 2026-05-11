/*
 * refrig.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"
#include "refrig.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#ifdef RVC_CONFIG_IMPL_REFRIGERATOR
static EXT_RAM_ATTR RVCDGN_zDGN_130515 l_130515_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_130514 l_130514_dgn;

static EXT_RAM_ATTR uint8_t l_connector_id;

void refrig_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    uint32_t class = RVCREFRIG0;
    int instance = broker_register_instance(&class, connector_id);
    ASSERT(instance > -1);
    memset(&l_130515_dgn, 0xFF, sizeof(l_130515_dgn));
    l_130515_dgn.u6Instance = 1;
}

/**
 * @brief Refrigerator cmd received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130514Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_130514 zDGN;
    int32_t value;
    int32_t updated_data = false;

    RVC_DGN_130514_Extract(&zDGN, p_data);
    LOG(D, "u6Instance:%d u2Light:%d u16SetTemperature:%d u4RefrigeratorMode:%d", zDGN.u6Instance, zDGN.u2Light, zDGN.u16SetTemperature, zDGN.u4RefrigeratorMode);

    // when u6Instance is 0, this is a group control command.
    if ((zDGN.u6Instance == 0) || (zDGN.u6Instance == l_130515_dgn.u6Instance))
    {
        l_130514_dgn.u6Instance = zDGN.u6Instance;

        LOG(D, "new u2Cavity:%d old u2Cavity:%d", zDGN.u2Cavity, l_130515_dgn.u2Cavity);
        if ((zDGN.u2Cavity != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Cavity != l_130515_dgn.u2Cavity))  // need to update all parameter signals
        {
            updated_data = true;
            value = l_130514_dgn.u2Cavity = zDGN.u2Cavity;
            convert_rvc_to_ddm_system_value(RVCREFRIG0CAV, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCREFRIG0CAV, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        LOG(D, "new u2Light:%d old u2Light:%d", zDGN.u2Light, l_130515_dgn.u2Light);
        if ((zDGN.u2Light != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Light != l_130515_dgn.u2Light))
        {
            updated_data = true;
            value = l_130514_dgn.u2Light = zDGN.u2Light;
            convert_rvc_to_ddm_system_value(RVCREFRIG0LGT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCREFRIG0LGT, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        LOG(D, "new u16SetTemperature:%d old u16SetTemperature:%d", zDGN.u16SetTemperature, l_130515_dgn.u16SetTemperature);
        if ((zDGN.u16SetTemperature != NMEA2K_UINT16_NO_DATA) && (zDGN.u16SetTemperature != l_130515_dgn.u16SetTemperature))
        {
            updated_data = true;
            value = l_130514_dgn.u16SetTemperature = zDGN.u16SetTemperature;
            convert_rvc_to_ddm_system_value(RVCREFRIG0SETTEMP, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCREFRIG0SETTEMP, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        LOG(D, "new u4FuelSource:%d old u4FuelSource:%d", zDGN.u4FuelSource, l_130515_dgn.u4FuelSource);
        if ((zDGN.u4FuelSource != NMEA2K_UINT4_NO_DATA) && (zDGN.u4FuelSource != l_130515_dgn.u4FuelSource))
        {
            updated_data = true;
            value = l_130514_dgn.u4FuelSource = zDGN.u4FuelSource;
            convert_rvc_to_ddm_system_value(RVCREFRIG0FUEL, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCREFRIG0FUEL, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }

        LOG(D, "new u4RefrigeratorMode:%d old u4RefrigeratorMode:%d", zDGN.u4RefrigeratorMode, l_130515_dgn.u4RefrigeratorMode);
        if ((zDGN.u4RefrigeratorMode != NMEA2K_UINT4_NO_DATA) && (zDGN.u4RefrigeratorMode != l_130515_dgn.u4RefrigeratorMode))
        {
            updated_data = true;
            value = l_130514_dgn.u4RefrigeratorMode = zDGN.u4RefrigeratorMode;
            convert_rvc_to_ddm_system_value(RVCREFRIG0MODE, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCREFRIG0MODE, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (updated_data == true)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCREFRIG0SYNC, &updated_data, sizeof(updated_data), l_connector_id, (TickType_t)portMAX_DELAY);
        }

        return true;
    }
    else
    {
        LOG(D, "Wrong instance received: %d", zDGN.u6Instance);
        return false;
    }
}

/**
 * @brief RVCREFRIG0 class handler function
 *
 * Initiates a transmit of DGN Refrigerator Status (130515) frame
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCREFRIG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    LOG(D, "handleRVCREFRIG0! frame.control:0X%02X", p_frame->frame.control);

    int32_t value;

    if (p_frame->frame.control == DDMP2_CONTROL_SET)
    {
        value = p_frame->frame.set.value.int32;
        if (convert_ddm_system_value_to_rvc_value(p_frame->frame.set.parameter, &value, true))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCREFRIG0INST:
                l_130515_dgn.u6Instance = value;
                LOG(D, "instance:%d", value);
                break;
            case RVCREFRIG0CAV:
                l_130515_dgn.u2Cavity = value;
                LOG(D, "cavity:%d", value);
                break;
            case RVCREFRIG0LGT:
                l_130515_dgn.u2Light = value;
                LOG(D, "light:%d", value);
                break;
            case RVCREFRIG0DOOR:
                l_130515_dgn.u2DoorSwitch = value;
                LOG(D, "door:%d", value);
                break;
            case RVCREFRIG0CTEMP:
                l_130515_dgn.u16CurrentTemperature = value;
                LOG(D, "cureent temp:%d", value);
                break;
            case RVCREFRIG0SETTEMP:
                l_130515_dgn.u16SetTemperature = value;
                LOG(D, "set temp:%d", value);
                break;
            case RVCREFRIG0FUEL:
                l_130515_dgn.u4FuelSource = value;
                LOG(D, "fuel source:%d", value);
                break;
            case RVCREFRIG0MODE:
                l_130515_dgn.u4RefrigeratorMode = value;
                LOG(D, "mode:%d", value);
                break;
            case RVCREFRIG0COMPSPD:
                l_130515_dgn.u8CompressorSpeed = value;
                LOG(D, "compressor:%d", value);
                break;
            case RVCREFRIG0SYNC:
                if (value == 1)  // updated_data
                {
                    NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, l_130515_dgn.u6Instance);
                    LOG(D, "RVC SetTX about device status.");
                }
                break;
            default:
                break;
            }
        }
    }
}

/**
 * @brief Prepare a Refrigerator device frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be filled
 */
bool transmit130515Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    RVC_DGN_130515_Stuff(p_data, &l_130515_dgn);
    return true;
}

#endif
