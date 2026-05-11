/*
 * solar_charge_controller.c
 *
 *  Created on: 30 Oct. 2025
 *      Author: Leo
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"

#include "common.h"
#include "dgnnode.h"
#include "product_database.h"
#include "rvc_to_ddm.h"
#include "solar_charge_controller.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS
static EXT_RAM_ATTR list_t l_130739_dgn;
static EXT_RAM_ATTR list_t l_130737_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_2
static EXT_RAM_ATTR list_t l_130693_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_3
static EXT_RAM_ATTR list_t l_130692_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_4
static EXT_RAM_ATTR list_t l_130691_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_5
static EXT_RAM_ATTR list_t l_130690_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_6
static EXT_RAM_ATTR list_t l_130689_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_BATTERY_STATUS
static EXT_RAM_ATTR list_t l_130688_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS
static EXT_RAM_ATTR list_t l_130559_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS
static EXT_RAM_ATTR list_t l_130738_dgn;
static EXT_RAM_ATTR list_t l_130736_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_2
static EXT_RAM_ATTR list_t l_130558_dgn;
static EXT_RAM_ATTR list_t l_130557_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_3
static EXT_RAM_ATTR list_t l_130556_dgn;
static EXT_RAM_ATTR list_t l_130555_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_4
static EXT_RAM_ATTR list_t l_130554_dgn;
static EXT_RAM_ATTR list_t l_130553_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_5
static EXT_RAM_ATTR list_t l_130511_dgn;
static EXT_RAM_ATTR list_t l_130510_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_STATUS
static EXT_RAM_ATTR list_t l_130735_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_CONFIGURATION_STATUS
static EXT_RAM_ATTR list_t l_130734_dgn;
static EXT_RAM_ATTR list_t l_130733_dgn;
#endif

static EXT_RAM_ATTR uint8_t l_connector_id;
static EXT_RAM_ATTR list_t *l_prod;

void solar_charge_controller_init(uint8_t connector_id, list_t *p_prod)
{
    l_prod = p_prod;
    l_connector_id = connector_id;

#ifdef RVC_CONFIG_SOLAR_CONTROLLER
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS
    LIST_INIT(&l_130739_dgn);
    LIST_INIT(&l_130737_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_2
    LIST_INIT(&l_130693_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_3
    LIST_INIT(&l_130692_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_4
    LIST_INIT(&l_130691_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_5
    LIST_INIT(&l_130690_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_6
    LIST_INIT(&l_130689_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_BATTERY_STATUS
    LIST_INIT(&l_130688_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS
    LIST_INIT(&l_130559_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS
    LIST_INIT(&l_130738_dgn);
    LIST_INIT(&l_130736_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_2
    LIST_INIT(&l_130558_dgn);
    LIST_INIT(&l_130557_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_3
    LIST_INIT(&l_130556_dgn);
    LIST_INIT(&l_130555_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_4
    LIST_INIT(&l_130554_dgn);
    LIST_INIT(&l_130553_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_5
    LIST_INIT(&l_130511_dgn);
    LIST_INIT(&l_130510_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_STATUS
    LIST_INIT(&l_130735_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_CONFIGURATION_STATUS
    LIST_INIT(&l_130734_dgn);
    LIST_INIT(&l_130733_dgn);
#endif
#endif
}

#ifdef RVC_CONFIG_SOLAR_CONTROLLER
// Structure to hold DGN list information for iteration
typedef struct
{
    list_t *list;
    uint32_t class_base;
    const char *name;
    bool unregister_class;  // true if we should unregister the class instance
} dgn_list_info_t;

// Define all DGN lists to process
static const dgn_list_info_t dgn_lists[] = {
#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS
    {&l_130739_dgn, RVCSOLAR0, "RVCSOLAR0", true},
    {&l_130737_dgn, RVCSOLAR0, "RVCSOLAR0", false},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_2
    {&l_130693_dgn, RVCSOLARTWO0, "RVCSOLARTWO0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_3
    {&l_130692_dgn, RVCSOLARTHR0, "RVCSOLARTHR0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_4
    {&l_130691_dgn, RVCSOLARFOUR0, "RVCSOLARFOUR0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_5
    {&l_130690_dgn, RVCSOLARFIVE0, "RVCSOLARFIVE0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_6
    {&l_130689_dgn, RVCSOLARSIX0, "RVCSOLARSIX0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_BATTERY_STATUS
    {&l_130688_dgn, RVCSOLARBAT0, "RVCSOLARBAT0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS
    {&l_130559_dgn, RVCSOLARARR0, "RVCSOLARARR0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS
    {&l_130738_dgn, RVCSOLARCFG0, "RVCSOLARCFG0", true},
    {&l_130736_dgn, RVCSOLARCFG0, "RVCSOLARCFG0", false},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_2
    {&l_130558_dgn, RVCSOLARCFGTWO0, "RVCSOLARCFGTWO0", true},
    {&l_130557_dgn, RVCSOLARCFGTWO0, "RVCSOLARCFGTWO0", false},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_3
    {&l_130556_dgn, RVCSOLARCFGTHR0, "RVCSOLARCFGTHR0", true},
    {&l_130555_dgn, RVCSOLARCFGTHR0, "RVCSOLARCFGTHR0", false},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_4
    {&l_130554_dgn, RVCSOLARCFGFOUR0, "RVCSOLARCFGFOUR0", true},
    {&l_130553_dgn, RVCSOLARCFGFOUR0, "RVCSOLARCFGFOUR0", false},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_5
    {&l_130511_dgn, RVCSOLARCFGFIVE0, "RVCSOLARCFGFIVE0", true},
    {&l_130510_dgn, RVCSOLARCFGFIVE0, "RVCSOLARCFGFIVE0", false},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_STATUS
    {&l_130735_dgn, RVCSOLAREQ0, "RVCSOLAREQ0", true},
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_CONFIGURATION_STATUS
    {&l_130734_dgn, RVCSOLAREQCFG0, "RVCSOLAREQCFG0", true},
    {&l_130733_dgn, RVCSOLAREQCFG0, "RVCSOLAREQCFG0", false},
#endif
};
#endif

/**
 * @brief Remove all solar charge controller nodes associated with a specific source address
 *
 * @param sa Source address of the device
 */
void remove_solar_charge_controller_nodes(uint8_t sa)
{
#ifdef RVC_CONFIG_SOLAR_CONTROLLER
    DgnNode_t *dgn_node = NULL;

    LOG(D, "Removing solar charge controller nodes for SA=%02X", sa);

    size_t num_lists = sizeof(dgn_lists) / sizeof(dgn_lists[0]);

    // Process all DGN lists
    for (size_t i = 0; i < num_lists; i++)
    {
        dgn_node = DgnNodeFindBySourceAddress(dgn_lists[i].list, sa);
        if (dgn_node != NULL)
        {
            LOG(D, "Removing %s node: SA=%02X, DDM_inst=%d, RVC_inst=%d",
                dgn_lists[i].name, dgn_node->source_address,
                dgn_node->ddm_instance, dgn_node->rvc_instance);

            // Unregister the class instance if needed (status messages do this, commands don't)
            if (dgn_lists[i].unregister_class)
            {
                uint32_t class = dgn_lists[i].class_base | DDM2_PARAMETER_INSTANCE(dgn_node->ddm_instance);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, class, &Zero, sizeof(Zero),
                                               l_connector_id, (TickType_t)portMAX_DELAY);
            }

            // Remove the node
            DgnNodeDelete(dgn_node);
        }
    }

    LOG(D, "Completed removing solar charge controller nodes for SA=%02X", sa);
#endif
}

#ifdef RVC_CONFIG_SOLAR_CONTROLLER
/**
 * @brief Update the proprietary field
 * @param prodnode_ddminst Instance of product node
 * @param add_class_inst DDMP class with a specific instance
 * @param rvc_inst Instance from RVC
 * @return None
 */
static void update_prop_field(int prodnode_ddminst, uint32_t add_class_inst, uint8_t rvc_inst)
{
    prodxprop_type_t type = {0};
    type.type.cls = PRODXPROP_TYPE_CLASS_POWER;
    type.type.intf = PRODXPROP_TYPE_INTERFACE_RVC;
    ProdDBUpdateCache((const void *)&rvc_inst, sizeof(uint8_t), FIELD_PROP_INST, prodnode_ddminst);
    ProdDBUpdateCache((const void *)&type, sizeof(type), FIELD_PROP_TYPE, prodnode_ddminst);
    ProdDBUpdateCache((const void *)&add_class_inst, sizeof(uint32_t), FIELD_PROP_CLASS, prodnode_ddminst);
    // Publish PROP
    ProdDBUpdateCache((const void *)&type, 0, FIELD_PROP, prodnode_ddminst);
}

#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS

/**
 * @brief Solar controller status DGN received (130739)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130739Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130739 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130739_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130739_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLAR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130739));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130739_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ChargeVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ChargeVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ChargeCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ChargeCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ChargeCurrentPercentOfMaximum != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ChargeCurrentPercentOfMaximum;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0CURRPERC;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_PART_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8OperatingState != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8OperatingState;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0OPER;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2PowerUpState != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2PowerUpState;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0POWUP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2ClearHistory != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2ClearHistory;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0CLRHIST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4ForceCharge != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4ForceCharge;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAR0FORCE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLAR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a solar controller command frame（130737）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130737Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130737_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130737_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief RVCSOLAR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLAR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130739 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_130739_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130739_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for SOLAR_CONTROLLER_STATUS", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLAR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLAR0VOLT:
            if (zDGN.u16ChargeVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16ChargeVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLAR0CURR:
            if (zDGN.u16ChargeCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16ChargeCurrent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AACDC_UINT16);
            }
            break;
        case RVCSOLAR0CURRPERC:
            if (zDGN.u8ChargeCurrentPercentOfMaximum != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ChargeCurrentPercentOfMaximum;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_PART_UINT8);
            }
            break;
        case RVCSOLAR0OPER:
            if (zDGN.u8OperatingState != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8OperatingState;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLAR0POWUP:
            if (zDGN.u2PowerUpState != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2PowerUpState;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLAR0CLRHIST:
            if (zDGN.u2ClearHistory != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2ClearHistory;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLAR0FORCE:
            if (zDGN.u4ForceCharge != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u4ForceCharge;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_2
/**
 * @brief Solar controller status 2 DGN received (130693)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130693Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130693 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130693_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARTWO0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130693_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARTWO0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130693));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130693_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RatedBatteryVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RatedBatteryVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARTWO0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RatedChargingCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RatedChargingCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARTWO0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLARTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130693 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130693_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130693_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARTWO0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARTWO0VOLT:
            if (zDGN.u16RatedBatteryVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RatedBatteryVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARTWO0CURR:
            if (zDGN.u16RatedChargingCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RatedChargingCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_3
/**
 * @brief Solar controller status 3 DGN received (130692)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130692Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130692 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130692_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARTHR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130692_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARTHR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130692));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130692_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RatedSolarInputVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RatedSolarInputVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARTHR0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RatedSolarInputCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RatedSolarInputCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARTHR0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RatedSolarOverPower != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RatedSolarOverPower;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARTHR0OPOW;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_W_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARTHR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLARTHR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARTHR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130692 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130692_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130692_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARTHR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARTHR0VOLT:
            if (zDGN.u16RatedSolarInputVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RatedSolarInputVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARTHR0CURR:
            if (zDGN.u16RatedSolarInputCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RatedSolarInputCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCSOLARTHR0OPOW:
            if (zDGN.u16RatedSolarOverPower != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RatedSolarOverPower;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_W_UINT16);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_4
/**
 * @brief Solar controller status 4 DGN received (130691)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130691Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130691 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130691_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARFOUR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130691_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARFOUR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130691));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130691_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16TodayAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16TodayAmpHoursToBattery;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARFOUR0TODAY;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AMPHOUR_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16YesterdayAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16YesterdayAmpHoursToBattery;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARFOUR0YESTERDAY;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AMPHOUR_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16DayBeforeYesterdayAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DayBeforeYesterdayAmpHoursToBattery;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARFOUR0YESTERDAY2;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AMPHOUR_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARFOUR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLARFOUR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130691 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130691_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130691_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARFOUR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARFOUR0TODAY:
            if (zDGN.u16TodayAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16TodayAmpHoursToBattery;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AMPHOUR_UINT16);
            }
            break;
        case RVCSOLARFOUR0YESTERDAY:
            if (zDGN.u16YesterdayAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16YesterdayAmpHoursToBattery;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AMPHOUR_UINT16);
            }
            break;
        case RVCSOLARFOUR0YESTERDAY2:
            if (zDGN.u16DayBeforeYesterdayAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16DayBeforeYesterdayAmpHoursToBattery;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AMPHOUR_UINT16);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_5
/**
 * @brief Solar controller status 5 DGN received (130690)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130690Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130690 zDGN;
    void *value = NULL;
    int32_t val;
    uint32_t u_val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130690_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARFIVE0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130690_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARFIVE0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130690));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130690_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16Last7DaysAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16Last7DaysAmpHoursToBattery;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARFIVE0DAYS7;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AMPHOUR_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u32CumulativePowerGeneration != NMEA2K_UINT32_NO_DATA)
    {
        updated_data = true;
        u_val = zDGN.u32CumulativePowerGeneration;
        value = &u_val;
        rvc_ddmp_parameter = RVCSOLARFIVE0POWER;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT32);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(uint32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARFIVE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLARFIVE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARFIVE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    uint32_t u_val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130690 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130690_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130690_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARFIVE0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;

        case RVCSOLARFIVE0DAYS7:
            if (zDGN.u16Last7DaysAmpHoursToBattery != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16Last7DaysAmpHoursToBattery;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AMPHOUR_UINT16);
            }

            break;

        case RVCSOLARFIVE0POWER:
            if (zDGN.u32CumulativePowerGeneration != NMEA2K_UINT32_NO_DATA)
            {
                is_publish = true;
                u_val = zDGN.u32CumulativePowerGeneration;
                value = &u_val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT32);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_STATUS_6
/**
 * @brief Solar controller status 6 DGN received (130689)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130689Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130689 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130689_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARSIX0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130689_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARSIX0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130689));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130689_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16TotalNumberOfOperatingDays != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16TotalNumberOfOperatingDays;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARSIX0DAYS;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SolarChargeControllerMeasuredTemperature != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16SolarChargeControllerMeasuredTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARSIX0TEMP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARSIX0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLARSIX0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARSIX0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130689 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130689_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130689_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARSIX0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARSIX0DAYS:
            if (zDGN.u16TotalNumberOfOperatingDays != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16TotalNumberOfOperatingDays;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT16);
            }
            break;
        case RVCSOLARSIX0TEMP:
            if (zDGN.u16SolarChargeControllerMeasuredTemperature != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16SolarChargeControllerMeasuredTemperature;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_DEGC_UINT16);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_BATTERY_STATUS
/**
 * @brief Solar controller battery status DGN received (130688)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130688Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130688 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130688_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARBAT0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130688_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARBAT0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130688));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130688_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16MeasuredVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16MeasuredVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARBAT0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16MeasuredCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16MeasuredCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARBAT0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8MeasuredTemperature != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8MeasuredTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARBAT0TEMP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARBAT0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLARBAT0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARBAT0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130688 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130688_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130688_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARBAT0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARBAT0VOLT:
            if (zDGN.u16MeasuredVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16MeasuredVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARBAT0CURR:
            if (zDGN.u16MeasuredCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16MeasuredCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCSOLARBAT0TEMP:
            if (zDGN.u8MeasuredTemperature != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8MeasuredTemperature;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_DEGC_UINT8);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_SOLAR_ARRAY_STATUS
/**
 * @brief Solar controller solar array status DGN received (130559)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130559Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130559 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130559_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARARR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130559_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARARR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130559));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130559_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SolarArrayMeasuredVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16SolarArrayMeasuredVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARARR0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16SolarArrayMeasuredInputCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16SolarArrayMeasuredInputCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARARR0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARARR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLARARR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARARR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130559 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130559_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130559_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARARR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARARR0VOLT:
            if (zDGN.u16SolarArrayMeasuredVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16SolarArrayMeasuredVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARARR0CURR:
            if (zDGN.u16SolarArrayMeasuredInputCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16SolarArrayMeasuredInputCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS
/**
 * @brief Solar controller configuration status DGN received (130738)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130738Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130738 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130738_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130738_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARCFG0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130738));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130738_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ChargingAlgorithm != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ChargingAlgorithm;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0ALGO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ControllerMode != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ControllerMode;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0MODE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2BatterySensorPresent != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2BatterySensorPresent;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0SENSOR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2LinkageMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2LinkageMode;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0LINK;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4BatteryType != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4BatteryType;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0TYPE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16BatteryBankSize != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BatteryBankSize;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0SIZE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AMPHOUR_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8MaximumChargingCurrent != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8MaximumChargingCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFG0MAXCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a solar controller configuration command frame（130736）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130736Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130736_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130736_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief RVCSOLARCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130738 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_130738_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130738_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for SOLAR_CONTROLLER_CONFIGUTATION_STATUS", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARCFG0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARCFG0ALGO:
            if (zDGN.u8ChargingAlgorithm != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ChargingAlgorithm;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFG0MODE:
            if (zDGN.u8ControllerMode != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ControllerMode;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFG0SENSOR:
            if (zDGN.u2BatterySensorPresent != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2BatterySensorPresent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFG0LINK:
            if (zDGN.u2LinkageMode != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2LinkageMode;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFG0TYPE:
            if (zDGN.u4BatteryType != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u4BatteryType;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFG0SIZE:
            if (zDGN.u16BatteryBankSize != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BatteryBankSize;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AMPHOUR_UINT16);
            }
            break;
        case RVCSOLARCFG0MAXCURR:
            if (zDGN.u8MaximumChargingCurrent != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8MaximumChargingCurrent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AACDC_UINT8);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_2
/**
 * @brief Solar controller configuration status 2 DGN received (130558)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130558Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130558 zDGN;
    void *value = NULL;
    int32_t val;
    uint32_t u_val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130558_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGTWO0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130558_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARCFGTWO0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130558));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130558_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16BulkAbsorptionVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BulkAbsorptionVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGTWO0BVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16FloatVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16FloatVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGTWO0FVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ChargeReturnVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        u_val = zDGN.u16ChargeReturnVoltage;
        value = &u_val;
        rvc_ddmp_parameter = RVCSOLARCFGTWO0CVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(uint32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARCFGTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a solar controller configuration command 2 frame（130557）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130557Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130557_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130557_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief RVCSOLARCFGTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130558 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130558_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130558_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for SOLAR_CONTROLLER_CONFIGURATION_STATUS_2", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARCFGTWO0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARCFGTWO0BVOLT:
            if (zDGN.u16BulkAbsorptionVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BulkAbsorptionVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARCFGTWO0FVOLT:
            if (zDGN.u16FloatVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16FloatVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;

        case RVCSOLARCFGTWO0CVOLT:
            if (zDGN.u16ChargeReturnVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16ChargeReturnVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;

        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_3
/**
 * @brief Solar controller configuration status 3 DGN received (130556)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130556Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130556 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130556_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGTHR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130556_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARCFGTHR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130556));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130556_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16UnderVoltageWarningVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16UnderVoltageWarningVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGTHR0UVWLIM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16BatteryHighVoltageLimitVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BatteryHighVoltageLimitVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGTHR0HVLIM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16BatteryLowVoltageLimitVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BatteryLowVoltageLimitVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGTHR0LVLIM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARCFGTHR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a solar controller configuration command 3 frame（130555）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130555Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130555_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130555_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief RVCSOLARCFGTHR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARCFGTHR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130556 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130556_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130556_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for SOLAR_CONTROLLER_CONFIGURATION_STATUS_3", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARCFGTHR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARCFGTHR0UVWLIM:
            if (zDGN.u16UnderVoltageWarningVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16UnderVoltageWarningVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARCFGTHR0HVLIM:
            if (zDGN.u16BatteryHighVoltageLimitVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BatteryHighVoltageLimitVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARCFGTHR0LVLIM:
            if (zDGN.u16BatteryLowVoltageLimitVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BatteryLowVoltageLimitVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_4
/**
 * @brief Solar controller configuration status 4 DGN received (130554)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130554Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130554 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130554_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFOUR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130554_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARCFGFOUR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130554));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130554_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16BatteryHighVoltageLimitReturnVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BatteryHighVoltageLimitReturnVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFOUR0HVLIMR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16BatteryLowVoltageLimitReturnVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BatteryLowVoltageLimitReturnVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFOUR0LVLIMR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8BatteryLowVoltageLimitTimeDelay != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8BatteryLowVoltageLimitTimeDelay;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFOUR0LVLIMD;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8AbsorptionDuration != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8AbsorptionDuration;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFOUR0DUR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8TemperatureCompensationFactor != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8TemperatureCompensationFactor;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFOUR0FACTOR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARCFGFOUR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a solar controller configuration command 4 frame（130553）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130553Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130553_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130553_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief RVCSOLARCFGFOUR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARCFGFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130554 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130554_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130554_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for SOLAR_CONTROLLER_CONFIGURATION_STATUS_4", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARCFGFOUR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARCFGFOUR0HVLIMR:
            if (zDGN.u16BatteryHighVoltageLimitReturnVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BatteryHighVoltageLimitReturnVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLARCFGFOUR0LVLIMR:
            if (zDGN.u16BatteryLowVoltageLimitReturnVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BatteryLowVoltageLimitReturnVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;

        case RVCSOLARCFGFOUR0LVLIMD:
            if (zDGN.u8BatteryLowVoltageLimitTimeDelay != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8BatteryLowVoltageLimitTimeDelay;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFGFOUR0DUR:
            if (zDGN.u8AbsorptionDuration != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8AbsorptionDuration;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFGFOUR0FACTOR:
            if (zDGN.u8TemperatureCompensationFactor != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8TemperatureCompensationFactor;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_CONTROLLER_CONFIGURATION_STATUS_5
/**
 * @brief Solar controller configuration status 5 DGN received (130511)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130511Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130511 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130511_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFIVE0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130511_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLARCFGFIVE0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130511));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130511_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ChargerPriority != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ChargerPriority;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFIVE0PRIO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ExternalTemperatureSensorHighTemperatureLimit != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ExternalTemperatureSensorHighTemperatureLimit;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFIVE0ETEMPHLIM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ExternalTemperatureSensorLowTemperatureLimit != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ExternalTemperatureSensorLowTemperatureLimit;
        value = &val;
        rvc_ddmp_parameter = RVCSOLARCFGFIVE0ETEMPLLIM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLARCFGFIVE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a solar controller configuration command 5 frame（130510）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130510Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130510_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130510_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief RVCSOLARCFGFIVE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLARCFGFIVE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130511 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130511_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130511_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for SOLAR_CONTROLLER_CONFIGURATION_STATUS_5", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLARCFGFIVE0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLARCFGFIVE0PRIO:
            if (zDGN.u8ChargerPriority != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ChargerPriority;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLARCFGFIVE0ETEMPHLIM:
            if (zDGN.u8ExternalTemperatureSensorHighTemperatureLimit != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ExternalTemperatureSensorHighTemperatureLimit;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_DEGC_UINT8);
            }
            break;

        case RVCSOLARCFGFIVE0ETEMPLLIM:
            if (zDGN.u8ExternalTemperatureSensorLowTemperatureLimit != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ExternalTemperatureSensorLowTemperatureLimit;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_DEGC_UINT8);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_STATUS
/**
 * @brief Solar equalization status DGN received (130735)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130735Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130735 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130735_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQ0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130735_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLAREQ0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130735));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130735_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16TimeRemaining != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16TimeRemaining;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQ0REM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2PreChargingStatus != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2PreChargingStatus;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQ0PRECHG;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8TimeSinceLastEqualization != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8TimeSinceLastEqualization;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQ0LAST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLAREQ0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCSOLAREQ0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLAREQ0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130735 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130735_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130735_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLAREQ0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLAREQ0REM:
            if (zDGN.u16TimeRemaining != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16TimeRemaining;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT16);
            }
            break;
        case RVCSOLAREQ0PRECHG:
            if (zDGN.u2PreChargingStatus != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2PreChargingStatus;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCSOLAREQ0LAST:
            if (zDGN.u8TimeSinceLastEqualization != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8TimeSinceLastEqualization;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_SOLAR_EQUALIZATION_CONFIGURATION_STATUS
/**
 * @brief Solar equalization configuration status DGN received (130734)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130734Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130734 zDGN;
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;
    uint32_t rvc_ddmp_parameter = 0;

    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (!prod_node)
    {
        LOG(E, "DgnNode not found (SA: %x)", sa);
        return false;
    }

    // Extract message content
    RVCDGN_DGN_130734_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }
    if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQCFG0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130734_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCSOLAREQCFG0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130734));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130734_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16EqualizationVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16EqualizationVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQCFG0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16EqualizationTime != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16EqualizationTime;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQCFG0TIME;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8EqualizationInterval != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8EqualizationInterval;
        value = &val;
        rvc_ddmp_parameter = RVCSOLAREQCFG0INT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCSOLAREQCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief Prepare a solar quualization configuration command frame（130733）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130733Dgn(uint8_t instance, uint8_t *p_data)
{
    // Stuff message data
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130733_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130733_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief RVCSOLAREQCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCSOLAREQCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130734 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130734_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130734_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for SOLAR_EQUALIZATION_CONFIGURATION_STATUS", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCSOLAREQCFG0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCSOLAREQCFG0VOLT:
            if (zDGN.u16EqualizationVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16EqualizationVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCSOLAREQCFG0TIME:
            if (zDGN.u16EqualizationTime != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16EqualizationTime;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16);
            }
            break;

        case RVCSOLAREQCFG0INT:
            if (zDGN.u8EqualizationInterval != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8EqualizationInterval;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        default:
            break;
        }

        if (is_publish)
        {
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
}
#endif
