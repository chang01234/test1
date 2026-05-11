/*
 * thermostat.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"
#include "rvc_to_ddm.h"
#include "thermostat.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z1) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z1)

static EXT_RAM_ATTR RVCDGN_zDGN_130972 l_130972_dgn[MAX_NUM_OF_ZONE_INSTANCES];
static EXT_RAM_ATTR RVCDGN_zDGN_130810 l_130810_dgn[MAX_NUM_OF_ZONE_INSTANCES];
static EXT_RAM_ATTR RVCDGN_zDGN_130808 l_130808_dgn[MAX_NUM_OF_ZONE_INSTANCES];
static EXT_RAM_ATTR RVCDGN_zDGN_131042 l_131042_dgn[MAX_NUM_OF_ZONE_INSTANCES];
static EXT_RAM_ATTR RVCDGN_zDGN_131042 l_130809_dgn[MAX_NUM_OF_ZONE_INSTANCES];
static EXT_RAM_ATTR RVCDGN_zDGN_130807 l_130807_dgn[MAX_NUM_OF_ZONE_INSTANCES][4];
static EXT_RAM_ATTR RVCDGN_zDGN_130805 l_130805_dgn[MAX_NUM_OF_ZONE_INSTANCES][4];
static EXT_RAM_ATTR RVCDGN_zDGN_130806 l_130806_dgn[MAX_NUM_OF_ZONE_INSTANCES][4];
static EXT_RAM_ATTR RVCDGN_zDGN_130804 l_130804_dgn[MAX_NUM_OF_ZONE_INSTANCES][4];
static EXT_RAM_ATTR uint8_t l_th_1_curr_schedule_instance;
static EXT_RAM_ATTR uint8_t l_th_2_curr_schedule_instance;

static EXT_RAM_ATTR uint8_t l_connector_id;

void thermostat_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    uint32_t class = RVCTH0;
    // Register devices
    int instance = broker_register_instance(&class, connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));
    l_131042_dgn[instance].u8Instance = 1;
    l_131042_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_131042_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u8Instance = 1;
    l_130809_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_130809_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;

    class = RVCTHTWO0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130810_dgn[instance].u8Instance = 1;
    l_130810_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130810_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;
    l_130810_dgn[instance].u8NumberOfScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u8Instance = 1;
    l_130808_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;

    class = RVCTHSCHED0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    memset(l_130807_dgn, 0xFF, MAX_NUM_OF_ZONE_INSTANCES * 4 * sizeof(l_130807_dgn[0][0]));
    l_130807_dgn[instance][0].u8Instance = 1;
    l_130807_dgn[instance][1].u8Instance = 1;
    l_130807_dgn[instance][2].u8Instance = 1;
    l_130807_dgn[instance][3].u8Instance = 1;
    memset(l_130805_dgn, 0xFF, MAX_NUM_OF_ZONE_INSTANCES * 4 * sizeof(l_130805_dgn[0][0]));
    l_130805_dgn[instance][0].u8Instance = 1;
    l_130805_dgn[instance][1].u8Instance = 1;
    l_130805_dgn[instance][2].u8Instance = 1;
    l_130805_dgn[instance][3].u8Instance = 1;
    l_th_1_curr_schedule_instance = 0;

    class = RVCTHSCHEDTWO0;
    instance = broker_register_instance(&class, connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_th_2_curr_schedule_instance = 0;
    memset(l_130806_dgn, 0xFF, MAX_NUM_OF_ZONE_INSTANCES * 4 * sizeof(l_130806_dgn[0][0]));
    l_130806_dgn[instance][0].u8Instance = 1;
    l_130806_dgn[instance][1].u8Instance = 1;
    l_130806_dgn[instance][2].u8Instance = 1;
    l_130806_dgn[instance][3].u8Instance = 1;
    memset(l_130804_dgn, 0xFF, MAX_NUM_OF_ZONE_INSTANCES * 4 * sizeof(l_130804_dgn[0][0]));
    l_130804_dgn[instance][0].u8Instance = 1;
    l_130804_dgn[instance][1].u8Instance = 1;
    l_130804_dgn[instance][2].u8Instance = 1;
    l_130804_dgn[instance][3].u8Instance = 1;

    class = RVCTHASTAT0;
    broker_register_instance(&class, connector_id);

    l_130972_dgn[0].u8Instance = 1;
    l_130972_dgn[0].u16AmbientTemp = NMEA2K_UINT16_NO_DATA;

#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z2) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z2)
    class = RVCTH0;
    // Register devices
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_131042_dgn[instance].u8Instance = 2;
    l_131042_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_131042_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u8Instance = 2;
    l_130809_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_130809_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;

    class = RVCTHTWO0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130810_dgn[instance].u8Instance = 2;
    l_130810_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130810_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;
    l_130810_dgn[instance].u8NumberOfScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u8Instance = 2;
    l_130808_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;

    class = RVCTHSCHED0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130807_dgn[instance][0].u8Instance = 2;
    l_130807_dgn[instance][1].u8Instance = 2;
    l_130807_dgn[instance][2].u8Instance = 2;
    l_130807_dgn[instance][3].u8Instance = 2;
    l_130805_dgn[instance][0].u8Instance = 2;
    l_130805_dgn[instance][1].u8Instance = 2;
    l_130805_dgn[instance][2].u8Instance = 2;
    l_130805_dgn[instance][3].u8Instance = 2;

    class = RVCTHSCHEDTWO0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130806_dgn[instance][0].u8Instance = 2;
    l_130806_dgn[instance][1].u8Instance = 2;
    l_130806_dgn[instance][2].u8Instance = 2;
    l_130806_dgn[instance][3].u8Instance = 2;
    l_130804_dgn[instance][0].u8Instance = 2;
    l_130804_dgn[instance][1].u8Instance = 2;
    l_130804_dgn[instance][2].u8Instance = 2;
    l_130804_dgn[instance][3].u8Instance = 2;

    class = RVCTHASTAT0;
    broker_register_instance(&class, l_connector_id);

    l_130972_dgn[1].u8Instance = 2;
    l_130972_dgn[1].u16AmbientTemp = NMEA2K_UINT16_NO_DATA;
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z3) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z3)
    class = RVCTH0;
    // Register devices
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_131042_dgn[instance].u8Instance = 3;
    l_131042_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_131042_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u8Instance = 3;
    l_130809_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_130809_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;

    class = RVCTHTWO0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130810_dgn[instance].u8Instance = 3;
    l_130810_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130810_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;
    l_130810_dgn[instance].u8NumberOfScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u8Instance = 3;
    l_130808_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;

    class = RVCTHSCHED0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130807_dgn[instance][0].u8Instance = 3;
    l_130807_dgn[instance][1].u8Instance = 3;
    l_130807_dgn[instance][2].u8Instance = 3;
    l_130807_dgn[instance][3].u8Instance = 3;
    l_130805_dgn[instance][0].u8Instance = 3;
    l_130805_dgn[instance][1].u8Instance = 3;
    l_130805_dgn[instance][2].u8Instance = 3;
    l_130805_dgn[instance][3].u8Instance = 3;

    class = RVCTHSCHEDTWO0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130806_dgn[instance][0].u8Instance = 3;
    l_130806_dgn[instance][1].u8Instance = 3;
    l_130806_dgn[instance][2].u8Instance = 3;
    l_130806_dgn[instance][3].u8Instance = 3;
    l_130804_dgn[instance][0].u8Instance = 3;
    l_130804_dgn[instance][1].u8Instance = 3;
    l_130804_dgn[instance][2].u8Instance = 3;
    l_130804_dgn[instance][3].u8Instance = 3;

    class = RVCTHASTAT0;
    broker_register_instance(&class, l_connector_id);

    l_130972_dgn[2].u8Instance = 3;
    l_130972_dgn[2].u16AmbientTemp = NMEA2K_UINT16_NO_DATA;
#endif
#if defined(RVC_CONFIG_IMPL_THERMOSTAT_Z4) || defined(RVC_CONFIG_INTERF_THERMOSTAT_Z4)
    class = RVCTH0;
    // Register devices
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_131042_dgn[instance].u8Instance = 4;
    l_131042_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_131042_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_131042_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_131042_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u8Instance = 4;
    l_130809_dgn[instance].u2FanMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u2SheduleMode = NMEA2K_UINT2_NO_DATA;
    l_130809_dgn[instance].u4OperatingMode = NMEA2K_UINT4_NO_DATA;
    l_130809_dgn[instance].u8FanSpeed = NMEA2K_UINT8_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempCool = NMEA2K_UINT16_NO_DATA;
    l_130809_dgn[instance].u16SetPointTempHeat = NMEA2K_UINT16_NO_DATA;
    class = RVCTHTWO0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130810_dgn[instance].u8Instance = 4;
    l_130810_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130810_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;
    l_130810_dgn[instance].u8NumberOfScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u8Instance = 4;
    l_130808_dgn[instance].u8CurrentScheduleInstance = NMEA2K_UINT8_NO_DATA;
    l_130808_dgn[instance].u2ReducedNoiseMode = NMEA2K_UINT2_NO_DATA;

    class = RVCTHSCHED0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130807_dgn[instance][0].u8Instance = 4;
    l_130807_dgn[instance][1].u8Instance = 4;
    l_130807_dgn[instance][2].u8Instance = 4;
    l_130807_dgn[instance][3].u8Instance = 4;
    l_130805_dgn[instance][0].u8Instance = 4;
    l_130805_dgn[instance][1].u8Instance = 4;
    l_130805_dgn[instance][2].u8Instance = 4;
    l_130805_dgn[instance][3].u8Instance = 4;

    class = RVCTHSCHEDTWO0;
    instance = broker_register_instance(&class, l_connector_id);
    ASSERT((instance > -1) && (instance <= MAX_NUM_OF_ZONE_INSTANCES));

    l_130806_dgn[instance][0].u8Instance = 4;
    l_130806_dgn[instance][1].u8Instance = 4;
    l_130806_dgn[instance][2].u8Instance = 4;
    l_130806_dgn[instance][3].u8Instance = 4;
    l_130804_dgn[instance][0].u8Instance = 4;
    l_130804_dgn[instance][1].u8Instance = 4;
    l_130804_dgn[instance][2].u8Instance = 4;
    l_130804_dgn[instance][3].u8Instance = 4;

    class = RVCTHASTAT0;
    broker_register_instance(&class, l_connector_id);

    l_130972_dgn[3].u8Instance = 4;
    l_130972_dgn[3].u16AmbientTemp = NMEA2K_UINT16_NO_DATA;
#endif
}

/**
 * @brief RVCTHASTAT0 class set handler function
 *
 * Initiates a transmit of DGN Thermostat Ambient Status (130972) frame
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCTHASTAT0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCTHASTAT0TEMP:
            value = l_130972_dgn[instance].u16AmbientTemp;
            if (l_130972_dgn[instance].u16AmbientTemp != NMEA2K_UINT16_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(RVCTHASTAT0TEMP, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHASTAT0INST:
            value = l_130972_dgn[instance].u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        default:
            break;
        }
    }
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
    else
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
        {
        case RVCTHASTAT0TEMP:
            value = p_frame->frame.set.value.int32;
            if (convert_ddm_system_value_to_rvc_value(RVCTHASTAT0TEMP, &value, true))
            {
                l_130972_dgn[instance].u16AmbientTemp = value;
            }
            break;

        case RVCTHASTAT0SYNC:
            // Set the command frame
            NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
            break;
        default:
            break;
        }
    }
#endif
}
/**
 * @brief RVC2TH0 class handler function
 *
 * Initiates a transmit of DGN Thermostat Status/Command 2 (130810/130808) frame
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVC2TH0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
    RVCDGN_zDGN_130808 *rec_dgn = &l_130808_dgn[instance];
    RVCDGN_zDGN_130810 *send_dgn = &l_130810_dgn[instance];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
    RVCDGN_zDGN_130810 *rec_dgn = &l_130810_dgn[instance];
    RVCDGN_zDGN_130808 *send_dgn = &l_130808_dgn[instance];
#endif
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCTHTWO0INST:
            value = rec_dgn->u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCTHTWO0CSINST:
            if (rec_dgn->u8CurrentScheduleInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8CurrentScheduleInstance;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
        case RVCTHTWO0NSINST:
            if (rec_dgn->u8NumberOfScheduleInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8NumberOfScheduleInstance;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
#endif
        case RVCTHTWO0NOISE:
            if (rec_dgn->u2ReducedNoiseMode != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2ReducedNoiseMode;
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
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
        {
        case RVCTHTWO0CSINST:
            send_dgn->u8CurrentScheduleInstance = value;
            break;
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
        case RVCTHTWO0NSINST:
            send_dgn->u8NumberOfScheduleInstance = value;
            break;
#endif
        case RVCTHTWO0NOISE:
            send_dgn->u2ReducedNoiseMode = value;
            break;
        case RVCTHTWO0SYNC:
            // Set the command frame
            NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief RVCTH0 class handler function
 *
 * Initiates a transmit of DGN Thermostat Status (131042) frame
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCTH0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
    RVCDGN_zDGN_130809 *rec_dgn = &l_130809_dgn[instance];
    RVCDGN_zDGN_131042 *send_dgn = &l_131042_dgn[instance];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
    RVCDGN_zDGN_131042 *rec_dgn = &l_131042_dgn[instance];
    RVCDGN_zDGN_130809 *send_dgn = &l_130809_dgn[instance];
#endif
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCTH0INST:
            value = rec_dgn->u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCTH0MODE:
            if (rec_dgn->u4OperatingMode != NMEA2K_UINT4_NO_DATA)
            {
                value = rec_dgn->u4OperatingMode;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTH0FMODE:
            if (rec_dgn->u2FanMode != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2FanMode;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTH0SMODE:
            if (rec_dgn->u2SheduleMode != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2SheduleMode;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTH0FSPD:
            if (rec_dgn->u8FanSpeed != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8FanSpeed;
                convert_rvc_to_ddm_system_value(RVCTH0FSPD, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTH0HSET:
            if (rec_dgn->u16SetPointTempHeat != NMEA2K_UINT16_NO_DATA)
            {
                value = rec_dgn->u16SetPointTempHeat;
                convert_rvc_to_ddm_system_value(RVCTH0HSET, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTH0CSET:
            if (rec_dgn->u16SetPointTempCool != NMEA2K_UINT16_NO_DATA)
            {
                value = rec_dgn->u16SetPointTempCool;
                convert_rvc_to_ddm_system_value(RVCTH0CSET, &value);
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
            case RVCTH0MODE:
                send_dgn->u4OperatingMode = value;
                break;
            case RVCTH0FMODE:
                send_dgn->u2FanMode = value;
                break;
            case RVCTH0SMODE:
                send_dgn->u2SheduleMode = value;
                break;
            case RVCTH0FSPD:
                send_dgn->u8FanSpeed = value;
                break;
            case RVCTH0HSET:
                send_dgn->u16SetPointTempHeat = value;
                break;
            case RVCTH0CSET:
                send_dgn->u16SetPointTempCool = value;
                break;
            case RVCTH0SYNC:
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
 * @brief RVC2THSCHED0 class handler function
 *
 * Initiates a transmit of DGN Thermostat Schedule 2 Status (130806) frame
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVC2THSCHED0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
    RVCDGN_zDGN_130804 *rec_dgn = &l_130804_dgn[instance][l_th_2_curr_schedule_instance];
    RVCDGN_zDGN_130806 *send_dgn = &l_130806_dgn[instance][l_th_2_curr_schedule_instance];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
    RVCDGN_zDGN_130806 *rec_dgn = &l_130806_dgn[instance][l_th_2_curr_schedule_instance];
    RVCDGN_zDGN_130804 *send_dgn = &l_130804_dgn[instance][l_th_2_curr_schedule_instance];
#endif

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCTHSCHEDTWO0INST:
            value = rec_dgn->u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCTHSCHEDTWO0SINST:
            if (rec_dgn->u8ScheduleModeInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8ScheduleModeInstance;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHEDTWO0SUN:
            if (rec_dgn->u2Sunday != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2Sunday;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHEDTWO0MON:
            if (rec_dgn->u2Monday != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2Monday;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHEDTWO0TUE:
            if (rec_dgn->u2Tuesday != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2Tuesday;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHEDTWO0WED:
            if (rec_dgn->u2Wednesday != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2Wednesday;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHEDTWO0THU:
            if (rec_dgn->u2Thursday != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2Thursday;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHEDTWO0FRI:
            if (rec_dgn->u2Friday != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2Friday;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHEDTWO0SAT:
            if (rec_dgn->u2Saturday != NMEA2K_UINT2_NO_DATA)
            {
                value = rec_dgn->u2Saturday;
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
            case RVCTHSCHEDTWO0SINST:
                l_th_2_curr_schedule_instance = value;
                // TODO Check value of schedule instance!!
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
                l_130804_dgn[instance][l_th_2_curr_schedule_instance].u8ScheduleModeInstance = value;
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
                l_130806_dgn[instance][l_th_2_curr_schedule_instance].u8ScheduleModeInstance = value;
#endif
                break;
            case RVCTHSCHEDTWO0SUN:
                send_dgn->u2Sunday = value;
                break;
            case RVCTHSCHEDTWO0MON:
                send_dgn->u2Monday = value;
                break;
            case RVCTHSCHEDTWO0TUE:
                send_dgn->u2Tuesday = value;
                break;
            case RVCTHSCHEDTWO0WED:
                send_dgn->u2Wednesday = value;
                break;
            case RVCTHSCHEDTWO0THU:
                send_dgn->u2Thursday = value;
                break;
            case RVCTHSCHEDTWO0FRI:
                send_dgn->u2Friday = value;
                break;
            case RVCTHSCHEDTWO0SAT:
                send_dgn->u2Saturday = value;
                break;
            case RVCTHSCHEDTWO0SYNC:
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
 * @brief RVCTHSCHED0 class handler function
 *
 * Initiates a transmit of DGN Thermostat Schedule Status/Command (130807/130805) frame
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCTHSCHED0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
    RVCDGN_zDGN_130805 *rec_dgn = &l_130805_dgn[instance][l_th_1_curr_schedule_instance];
    RVCDGN_zDGN_130807 *send_dgn = &l_130807_dgn[instance][l_th_1_curr_schedule_instance];
#endif
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
    RVCDGN_zDGN_130807 *rec_dgn = &l_130807_dgn[instance][l_th_1_curr_schedule_instance];
    RVCDGN_zDGN_130805 *send_dgn = &l_130805_dgn[instance][l_th_1_curr_schedule_instance];
#endif

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        // Return back last received command
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCTHSCHED0INST:
            value = rec_dgn->u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCTHSCHED0SINST:
            if (rec_dgn->u8ScheduleModeInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8ScheduleModeInstance;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHED0HOUR:
            if (rec_dgn->u8StartHour != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8StartHour;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHED0MIN:
            if (rec_dgn->u8StartMinute != NMEA2K_UINT8_NO_DATA)
            {
                value = rec_dgn->u8StartMinute;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHED0HSET:
            if (rec_dgn->u16SetpointTempHeat != NMEA2K_UINT16_NO_DATA)
            {
                value = rec_dgn->u16SetpointTempHeat;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTHSCHED0CSET:
            if (rec_dgn->u16SetpointTempCool != NMEA2K_UINT16_NO_DATA)
            {
                value = rec_dgn->u16SetpointTempCool;
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
            case RVCTHSCHED0SINST:
                l_th_1_curr_schedule_instance = value;
#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1
                l_130805_dgn[instance][l_th_1_curr_schedule_instance].u8ScheduleModeInstance = value;
#endif
#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
                l_130807_dgn[instance][l_th_1_curr_schedule_instance].u8ScheduleModeInstance = value;
#endif
                break;
            case RVCTHSCHED0HOUR:
                send_dgn->u8StartHour = value;
                break;
            case RVCTHSCHED0MIN:
                send_dgn->u8StartMinute = value;
                break;
            case RVCTHSCHED0HSET:
                send_dgn->u16SetpointTempHeat = value;
                break;
            case RVCTHSCHED0CSET:
                send_dgn->u16SetpointTempCool = value;
                break;
            case RVCTHSCHED0SYNC:
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

#ifdef RVC_CONFIG_IMPL_THERMOSTAT_Z1
/**
 * @brief Thermostat 1 command received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130809Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_130809 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;
    RVCDGN_DGN_130809_Extract(&zDGN, p_data);
    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130809_dgn[i].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u2SheduleMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130809_dgn[class_instance].u2SheduleMode = zDGN.u2SheduleMode;
        convert_rvc_to_ddm_system_value(RVCTH0SMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0SMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FanMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130809_dgn[class_instance].u2FanMode = zDGN.u2FanMode;
        convert_rvc_to_ddm_system_value(RVCTH0FMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0FMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4OperatingMode != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        value = l_130809_dgn[class_instance].u4OperatingMode = zDGN.u4OperatingMode;
        convert_rvc_to_ddm_system_value(RVCTH0MODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0MODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8FanSpeed != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130809_dgn[class_instance].u8FanSpeed = zDGN.u8FanSpeed;
        convert_rvc_to_ddm_system_value(RVCTH0FSPD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0FSPD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SetPointTempCool != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130809_dgn[class_instance].u16SetPointTempCool = zDGN.u16SetPointTempCool;
        convert_rvc_to_ddm_system_value(RVCTH0CSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0CSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SetPointTempHeat != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130809_dgn[class_instance].u16SetPointTempHeat = zDGN.u16SetPointTempHeat;
        convert_rvc_to_ddm_system_value(RVCTH0HSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0HSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Process received RVC DGN 130808 (Thermostat command 2)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130808Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    RVCDGN_zDGN_130808 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;
    RVCDGN_DGN_130808_Extract(&zDGN, p_data);

    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130808_dgn[i].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u2ReducedNoiseMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        value = l_130808_dgn[class_instance].u2ReducedNoiseMode = zDGN.u2ReducedNoiseMode;
        convert_rvc_to_ddm_system_value(RVCTHTWO0NOISE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHTWO0NOISE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8CurrentScheduleInstance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130808_dgn[class_instance].u8CurrentScheduleInstance = zDGN.u8CurrentScheduleInstance;
        convert_rvc_to_ddm_system_value(RVCTHTWO0CSINST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHTWO0CSINST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHTWO0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Thermostat Schedule command received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130805Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_130805 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130805_Extract(&zDGN, p_data);
    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return false;
    }
    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130805_dgn[i][zDGN.u8ScheduleModeInstance].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }

    if (zDGN.u8ScheduleModeInstance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130805_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8ScheduleModeInstance = zDGN.u8ScheduleModeInstance;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0SINST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0SINST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8StartHour != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130805_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8StartHour = zDGN.u8StartHour;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0HOUR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0HOUR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8StartMinute != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = l_130805_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8StartMinute = zDGN.u8StartMinute;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0MIN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0MIN | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SetpointTempCool != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130805_dgn[class_instance][zDGN.u8ScheduleModeInstance].u16SetpointTempCool = zDGN.u16SetpointTempCool;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0CSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0CSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SetpointTempHeat != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        value = l_130805_dgn[class_instance][zDGN.u8ScheduleModeInstance].u16SetpointTempHeat = zDGN.u16SetpointTempHeat;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0HSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0HSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Thermostat Schedule 2 command received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130804Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_130804 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130804_Extract(&zDGN, p_data);

    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return false;
    }
    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130804_dgn[i][zDGN.u8ScheduleModeInstance].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }
    if ((zDGN.u8ScheduleModeInstance != NMEA2K_UINT8_NO_DATA) && (zDGN.u8ScheduleModeInstance != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8ScheduleModeInstance))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8ScheduleModeInstance = zDGN.u8ScheduleModeInstance;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0SINST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SINST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Sunday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Sunday != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Sunday))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Sunday = zDGN.u2Sunday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0SUN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SUN | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Monday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Monday != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Monday))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Monday = zDGN.u2Monday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0MON, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0MON | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Tuesday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Tuesday != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Tuesday))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Tuesday = zDGN.u2Tuesday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0TUE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0TUE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Wednesday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Wednesday != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Wednesday))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Wednesday = zDGN.u2Wednesday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0WED, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0WED | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Thursday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Thursday != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Thursday))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Thursday = zDGN.u2Thursday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0THU, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0THU | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Friday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Friday != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Friday))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Friday = zDGN.u2Friday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0FRI, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0FRI | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Saturday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Saturday != l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Saturday))
    {
        updated_data = true;
        value = l_130804_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Saturday = zDGN.u2Saturday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0SAT, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SAT | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Prepare a Thermostat Ambient Status frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130972Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // Stuff message data
        RVCDGN_DGN_130972_Stuff(p_data, &l_130972_dgn[instance]);
        return true;
    }
    return false;
}

/**
 * @brief Prepare a Thermostat 2 Status frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130810Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // Stuff message data
        RVCDGN_DGN_130810_Stuff(p_data, &l_130810_dgn[instance]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %d", instance);
        return false;
    }
}

/**
 * @brief Prepare a Thermostat 1 Status frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131042Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // LOG(I,"Operating mode: %d Fan Mode: %d, Schedule mode: %d, Fanspeed:%d, TempHeat: %d, TempCool:%d", zDGN.u4OperatingMode,zDGN.u2FanMode,zDGN.u2SheduleMode,zDGN.u8FanSpeed,zDGN.u16SetPointTempHeat,zDGN.u16SetPointTempCool );
        // Stuff message data
        RVCDGN_DGN_131042_Stuff(p_data, &l_131042_dgn[instance]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %d", instance);
        return false;
    }
}

/**
 * @brief Prepare a Thermostat Schedule Status 2 frame
 *
 * @param instance Bits 0-3 class instance, Bits 4-7 Explicit schedule instance to be sent
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130806Dgn(uint8_t instance, uint8_t *p_data)
{
    if ((instance & 0x0F) < MAX_NUM_OF_ZONE_INSTANCES)
    {
        uint8_t sched_inst = l_th_2_curr_schedule_instance;
        if (((instance & 0xF0) >> 4) && (((instance & 0xF0) >> 4) <= 4))
        {
            // Explicit requested
            sched_inst = ((instance & 0xF0) >> 4) - 1;
        }
        // Stuff message data
        RVCDGN_DGN_130806_Stuff(p_data, &l_130806_dgn[(instance & 0x0F)][sched_inst]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %x", instance);
        return false;
    }
}

/**
 * @brief Prepare a Thermostat Schedule Status frame
 *
 * @param instance Bits 0-3 class instance, Bits 4-7 Explicit schedule instance to be sent
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130807Dgn(uint8_t instance, uint8_t *p_data)
{
    if ((instance & 0x0F) < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // LOG(I,"Operating mode: %d Fan Mode: %d, Schedule mode: %d, Fanspeed:%d, TempHeat: %d, TempCool:%d", zDGN.u4OperatingMode,zDGN.u2FanMode,zDGN.u2SheduleMode,zDGN.u8FanSpeed,zDGN.u16SetPointTempHeat,zDGN.u16SetPointTempCool );
        uint8_t sched_inst = l_th_1_curr_schedule_instance;
        if (((instance & 0xF0) >> 4) && (((instance & 0xF0) >> 4) <= 4))
        {
            // Explicit requested
            sched_inst = ((instance & 0xF0) >> 4) - 1;
        }
        // Stuff message data
        RVCDGN_DGN_130807_Stuff(p_data, &l_130807_dgn[(instance & 0x0F)][sched_inst]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %x", instance);
        return false;
    }
}

#endif

#ifdef RVC_CONFIG_INTERF_THERMOSTAT_Z1

/**
 * @brief Thermostat 2 status received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130810Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    RVCDGN_zDGN_130810 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;
    RVCDGN_DGN_130810_Extract(&zDGN, p_data);

    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130810_dgn[i].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }
    if ((zDGN.u2ReducedNoiseMode != NMEA2K_UINT2_NO_DATA) && (zDGN.u2ReducedNoiseMode != l_130810_dgn[class_instance].u2ReducedNoiseMode))
    {
        updated_data = true;
        value = l_130810_dgn[class_instance].u2ReducedNoiseMode = zDGN.u2ReducedNoiseMode;
        convert_rvc_to_ddm_system_value(RVCTHTWO0NOISE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHTWO0NOISE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8CurrentScheduleInstance != NMEA2K_UINT8_NO_DATA) && (zDGN.u8CurrentScheduleInstance != l_130810_dgn[class_instance].u8CurrentScheduleInstance))
    {
        updated_data = true;
        value = l_130810_dgn[class_instance].u8CurrentScheduleInstance = zDGN.u8CurrentScheduleInstance;
        convert_rvc_to_ddm_system_value(RVCTHTWO0CSINST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHTWO0CSINST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8NumberOfScheduleInstance != NMEA2K_UINT8_NO_DATA) && (zDGN.u8NumberOfScheduleInstance != l_130810_dgn[class_instance].u8NumberOfScheduleInstance))
    {
        updated_data = true;
        value = l_130810_dgn[class_instance].u8NumberOfScheduleInstance = zDGN.u8NumberOfScheduleInstance;
        convert_rvc_to_ddm_system_value(RVCTHTWO0NSINST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHTWO0NSINST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHTWO0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Thermostat 1 status received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131042Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_131042 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;
    RVCDGN_DGN_131042_Extract(&zDGN, p_data);
    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_131042_dgn[i].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }
    if ((zDGN.u2SheduleMode != NMEA2K_UINT2_NO_DATA) && (zDGN.u2SheduleMode != l_131042_dgn[class_instance].u2SheduleMode))
    {
        updated_data = true;
        value = l_131042_dgn[class_instance].u2SheduleMode = zDGN.u2SheduleMode;
        convert_rvc_to_ddm_system_value(RVCTH0SMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0SMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2FanMode != NMEA2K_UINT2_NO_DATA) && (zDGN.u2FanMode != l_131042_dgn[class_instance].u2FanMode))
    {
        updated_data = true;
        value = l_131042_dgn[class_instance].u2FanMode = zDGN.u2FanMode;
        convert_rvc_to_ddm_system_value(RVCTH0FMODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0FMODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u4OperatingMode != NMEA2K_UINT4_NO_DATA) && (zDGN.u4OperatingMode != l_131042_dgn[class_instance].u4OperatingMode))
    {
        updated_data = true;
        value = l_131042_dgn[class_instance].u4OperatingMode = zDGN.u4OperatingMode;
        convert_rvc_to_ddm_system_value(RVCTH0MODE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0MODE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8FanSpeed != NMEA2K_UINT8_NO_DATA) && (zDGN.u8FanSpeed != l_131042_dgn[class_instance].u8FanSpeed))
    {
        updated_data = true;
        value = l_131042_dgn[class_instance].u8FanSpeed = zDGN.u8FanSpeed;
        convert_rvc_to_ddm_system_value(RVCTH0FSPD, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0FSPD | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u16SetPointTempCool != NMEA2K_UINT16_NO_DATA) && (zDGN.u16SetPointTempCool != l_131042_dgn[class_instance].u16SetPointTempCool))
    {
        updated_data = true;
        value = l_131042_dgn[class_instance].u16SetPointTempCool = zDGN.u16SetPointTempCool;
        convert_rvc_to_ddm_system_value(RVCTH0CSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0CSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u16SetPointTempHeat != NMEA2K_UINT16_NO_DATA) && (zDGN.u16SetPointTempHeat != l_131042_dgn[class_instance].u16SetPointTempHeat))
    {
        updated_data = true;
        value = l_131042_dgn[class_instance].u16SetPointTempHeat = zDGN.u16SetPointTempHeat;
        convert_rvc_to_ddm_system_value(RVCTH0HSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0HSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTH0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Thermostat Schedule 2 status received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130806Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_130806 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130806_Extract(&zDGN, p_data);

    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return false;
    }
    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130806_dgn[i][zDGN.u8ScheduleModeInstance].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }
    if ((zDGN.u8ScheduleModeInstance != NMEA2K_UINT8_NO_DATA) && (zDGN.u8ScheduleModeInstance != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8ScheduleModeInstance))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8ScheduleModeInstance = zDGN.u8ScheduleModeInstance;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0SINST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SINST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Sunday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Sunday != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Sunday))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Sunday = zDGN.u2Sunday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0SUN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SUN | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Monday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Monday != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Monday))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Monday = zDGN.u2Monday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0MON, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0MON | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Tuesday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Tuesday != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Tuesday))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Tuesday = zDGN.u2Tuesday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0TUE, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0TUE | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Wednesday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Wednesday != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Wednesday))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Wednesday = zDGN.u2Wednesday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0WED, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0WED | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Thursday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Thursday != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Thursday))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Thursday = zDGN.u2Thursday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0THU, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0THU | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Friday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Friday != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Friday))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Friday = zDGN.u2Friday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0FRI, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0FRI | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u2Saturday != NMEA2K_UINT2_NO_DATA) && (zDGN.u2Saturday != l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Saturday))
    {
        updated_data = true;
        value = l_130806_dgn[class_instance][zDGN.u8ScheduleModeInstance].u2Saturday = zDGN.u2Saturday;
        convert_rvc_to_ddm_system_value(RVCTHSCHEDTWO0SAT, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SAT | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHEDTWO0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Thermostat Schedule status received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130807Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_130807 zDGN;
    int32_t value;
    int class_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130807_Extract(&zDGN, p_data);
    if (zDGN.u8ScheduleModeInstance > RVCDGN_SCHEDULE_MODE_RETURN)
    {
        LOG(E, "Too large u8ScheduleModeInstance, %d", zDGN.u8ScheduleModeInstance);
        return false;
    }
    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130807_dgn[i][zDGN.u8ScheduleModeInstance].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }

    if ((zDGN.u8ScheduleModeInstance != NMEA2K_UINT8_NO_DATA) && (zDGN.u8ScheduleModeInstance != l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8ScheduleModeInstance))
    {
        updated_data = true;
        value = l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8ScheduleModeInstance = zDGN.u8ScheduleModeInstance;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0SINST, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0SINST | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8StartHour != NMEA2K_UINT8_NO_DATA) && (zDGN.u8StartHour != l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8StartHour))
    {
        updated_data = true;
        value = l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8StartHour = zDGN.u8StartHour;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0HOUR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0HOUR | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8StartMinute != NMEA2K_UINT8_NO_DATA) && (zDGN.u8StartMinute != l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8StartMinute))
    {
        updated_data = true;
        value = l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u8StartMinute = zDGN.u8StartMinute;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0MIN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0MIN | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u16SetpointTempCool != NMEA2K_UINT16_NO_DATA) && (zDGN.u16SetpointTempCool != l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u16SetpointTempCool))
    {
        updated_data = true;
        value = l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u16SetpointTempCool = zDGN.u16SetpointTempCool;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0CSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0CSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u16SetpointTempHeat != NMEA2K_UINT16_NO_DATA) && (zDGN.u16SetpointTempHeat != l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u16SetpointTempHeat))
    {
        updated_data = true;
        value = l_130807_dgn[class_instance][zDGN.u8ScheduleModeInstance].u16SetpointTempHeat = zDGN.u16SetpointTempHeat;
        convert_rvc_to_ddm_system_value(RVCTHSCHED0HSET, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0HSET | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHSCHED0SYNC | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Thermostat Ambient status received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130972Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    // Extract message content
    RVCDGN_zDGN_130972 zDGN;
    int32_t value;
    int class_instance = -1;

    RVCDGN_DGN_130972_Extract(&zDGN, p_data);

    for (int i = 0; i < MAX_NUM_OF_ZONE_INSTANCES; i++)
    {
        if (zDGN.u8Instance == l_130972_dgn[i].u8Instance)
        {
            class_instance = i;
        }
    }
    if (class_instance < 0)
    {
        LOG(E, "Wrong instance received: %d", zDGN.u8Instance);
        return false;
    }
    if ((zDGN.u16AmbientTemp != NMEA2K_UINT16_NO_DATA) && (zDGN.u16AmbientTemp != l_130972_dgn[class_instance].u16AmbientTemp))
    {
        value = l_130972_dgn[class_instance].u16AmbientTemp = zDGN.u16AmbientTemp;
        convert_rvc_to_ddm_system_value(RVCTHASTAT0TEMP, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTHASTAT0TEMP | DDM2_PARAMETER_INSTANCE(class_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a Thermostat 2 Command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130808Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // Stuff message data
        RVCDGN_DGN_130808_Stuff(p_data, &l_130808_dgn[instance]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %d", instance);
        return false;
    }
}

/**
 * @brief Prepare a Thermostat 1 Command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130809Dgn(uint8_t instance, uint8_t *p_data)
{
    if (instance < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // Stuff message data
        RVCDGN_DGN_130809_Stuff(p_data, &l_130809_dgn[instance]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %d", instance);
        return false;
    }
}

/**
 * @brief Prepare a Thermostat Schedule 2 Command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130804Dgn(uint8_t instance, uint8_t *p_data)
{
    if ((instance & 0x0F) < MAX_NUM_OF_ZONE_INSTANCES)
    {
        uint8_t sched_inst = l_th_2_curr_schedule_instance;
        if (((instance & 0xF0) >> 4) && (((instance & 0xF0) >> 4) <= 4))
        {
            // Explicit requested
            sched_inst = ((instance & 0xF0) >> 4) - 1;
        }
        // Stuff message data
        RVCDGN_DGN_130804_Stuff(p_data, &l_130804_dgn[(instance & 0x0F)][sched_inst]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %x", instance);
        return false;
    }
}

/**
 * @brief Prepare a Thermostat Schedule Command frame
 *
 * @param instance Bits 0-3 class instance, Bits 4-7 Explicit schedule instance to be sent
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130805Dgn(uint8_t instance, uint8_t *p_data)
{
    if ((instance & 0x0F) < MAX_NUM_OF_ZONE_INSTANCES)
    {
        // LOG(I,"Operating mode: %d Fan Mode: %d, Schedule mode: %d, Fanspeed:%d, TempHeat: %d, TempCool:%d", zDGN.u4OperatingMode,zDGN.u2FanMode,zDGN.u2SheduleMode,zDGN.u8FanSpeed,zDGN.u16SetPointTempHeat,zDGN.u16SetPointTempCool );
        uint8_t sched_inst = l_th_1_curr_schedule_instance;
        if (((instance & 0xF0) >> 4) && (((instance & 0xF0) >> 4) <= 4))
        {
            // Explicit requested
            sched_inst = ((instance & 0xF0) >> 4) - 1;
        }
        // Stuff message data
        RVCDGN_DGN_130805_Stuff(p_data, &l_130805_dgn[(instance & 0x0F)][sched_inst]);
        return true;
    }
    else
    {
        LOG(E, "Wrong class instance %x", instance);
        return false;
    }
}

#endif
