/*
 * battery.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"

#include "battery.h"
#include "common.h"
#include "dgnnode.h"
#include "product_database.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

static EXT_RAM_ATTR uint8_t l_connector_id;
static EXT_RAM_ATTR list_t *l_prod;
#if defined(RVC_CONFIG_INTERF_BATTERY_SUMMARY)
static void update_prop_field(int prodnode_ddminst, uint32_t add_class_inst);
static EXT_RAM_ATTR list_t l_130545_dgn;
#endif
void battery_init(uint8_t connector_id, list_t *p_prod)
{
    l_prod = p_prod;
    l_connector_id = connector_id;
#if defined(RVC_CONFIG_INTERF_BATTERY_SUMMARY)
    LIST_INIT(&l_130545_dgn);
#endif
}

#if defined(RVC_CONFIG_INTERF_BATTERY_SUMMARY)
/**
 * @brief Battery Summary DGN received (130545)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130545Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130545 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    RVCDGN_DGN_130545_Extract(&zDGN, p_data);
    if (zDGN.u8BatteryInstance == 0)
    {
        LOG(E, "Invalid Battery Instance %d", zDGN.u8BatteryInstance);
        return false;
    }

    DgnNode_t *prod_node = NULL;
    prod_node = DgnNodeFindBySourceAddress(l_prod, sa);

    // Compare with last data
    if (zDGN.u8BatteryInstance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8BatteryInstance;
        convert_rvc_to_ddm_system_value(RVCBATSUM0INST, &value);
        dgn_node = DgnNodeFindBySourceAddress(&l_130545_dgn, sa);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCBATSUM0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8BatteryInstance, ddm_instance, sa, p_data, size);
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130545_dgn);
            if (prod_node && (prod_node->ddm_instance != -1))
            {
                uint32_t inst = RVCBATSUM0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8DCInstance;
        convert_rvc_to_ddm_system_value(RVCBATSUM0DC, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0DC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8SeriesString != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8SeriesString;
        convert_rvc_to_ddm_system_value(RVCBATSUM0SERIES, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0SERIES | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ModuleCount != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8ModuleCount;
        convert_rvc_to_ddm_system_value(RVCBATSUM0MODCNT, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0MODCNT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8CellsPerModule != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u8CellsPerModule;
        convert_rvc_to_ddm_system_value(RVCBATSUM0CELLS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0CELLS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4VoltageStatus != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u4VoltageStatus;
        convert_rvc_to_ddm_system_value(RVCBATSUM0VOLTSTS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0VOLTSTS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4TemperatureStatus != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        value = zDGN.u4TemperatureStatus;
        convert_rvc_to_ddm_system_value(RVCBATSUM0TEMPSTS, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0TEMPSTS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (sa != 0xFF)
    {
        updated_data = true;
        int32_t sa_value = sa;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0ADDR | DDM2_PARAMETER_INSTANCE(ddm_instance), &sa_value, sizeof(sa_value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief RVCBATSUM0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCBATSUM0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130545 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130545_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130545_Extract(&zDGN, dgn_node->dgn_data);

        if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
            {
            case RVCBATSUM0INST:
                value = zDGN.u8BatteryInstance;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
                break;
            case RVCBATSUM0DC:
                if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
                {
                    value = zDGN.u8DCInstance;
                    convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            case RVCBATSUM0SERIES:
                if (zDGN.u8SeriesString != NMEA2K_UINT8_NO_DATA)
                {
                    value = zDGN.u8SeriesString;
                    convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            case RVCBATSUM0MODCNT:
                if (zDGN.u8ModuleCount != NMEA2K_UINT8_NO_DATA)
                {
                    value = zDGN.u8ModuleCount;
                    convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            case RVCBATSUM0CELLS:
                if (zDGN.u8CellsPerModule != NMEA2K_UINT8_NO_DATA)
                {
                    value = zDGN.u8CellsPerModule;
                    convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            case RVCBATSUM0VOLTSTS:
                if (zDGN.u4VoltageStatus != NMEA2K_UINT4_NO_DATA)
                {
                    value = zDGN.u4VoltageStatus;
                    convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            case RVCBATSUM0TEMPSTS:
                if (zDGN.u4TemperatureStatus != NMEA2K_UINT4_NO_DATA)
                {
                    value = zDGN.u4TemperatureStatus;
                    convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
                }
                break;
            case RVCBATSUM0ADDR:
            {
                value = (int32_t)dgn_node->source_address;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
            case RVCBATSUM0SYNC:
            {
                // Always publish sync as 1, since we have DgnNode for the instance, which means that the DGN has been processed at least once
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
            default:
                break;
            }
        }
    }
}

static void update_prop_field(int prodnode_ddminst, uint32_t add_class_inst)
{
    prodxprop_type_t type = {0};
    type.type.cls = PRODXPROP_TYPE_CLASS_POWER;
    type.type.intf = PRODXPROP_TYPE_INTERFACE_RVC;
    ProdDBUpdateCache((const void *)&type, sizeof(type), FIELD_PROP_TYPE, prodnode_ddminst);
    ProdDBUpdateCache((const void *)&add_class_inst, sizeof(uint32_t), FIELD_PROP_CLASS, prodnode_ddminst);
    // Publish PROP
    ProdDBUpdateCache((const void *)&type, 0, FIELD_PROP, prodnode_ddminst);
}

#endif

void remove_battery_nodes(uint8_t sa)
{
#ifdef RVC_CONFIG_INTERF_BATTERY_SUMMARY
    DgnNode_t *dgn_node = NULL;
    DgnNode_t *temp_node = NULL;

    // Remove all nodes with the same address
    LIST_FOREACH_SAFE(dgn_node, &l_130545_dgn, list_node, temp_node)
    {
        if (dgn_node->source_address == sa)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCBATSUM0 | DDM2_PARAMETER_INSTANCE(dgn_node->ddm_instance), &Zero, sizeof(Zero), l_connector_id, (TickType_t)portMAX_DELAY);
            DgnNodeDelete(dgn_node);
        }
    }
#endif
}
