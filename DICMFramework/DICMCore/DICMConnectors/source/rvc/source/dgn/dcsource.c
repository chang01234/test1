/*
 * dcsource.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"

#include "common.h"
#include "dcsource.h"
#include "ddm2_parameter_list.h"
#include "dgnnode.h"
#include "product_database.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
static EXT_RAM_ATTR list_t l_131069_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
static EXT_RAM_ATTR list_t l_131068_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
static EXT_RAM_ATTR list_t l_131067_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
static EXT_RAM_ATTR list_t l_130761_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
static EXT_RAM_ATTR list_t l_130760_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
static EXT_RAM_ATTR list_t l_130759_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
static EXT_RAM_ATTR list_t l_130732_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
static EXT_RAM_ATTR list_t l_130731_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
static EXT_RAM_ATTR list_t l_130730_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
static EXT_RAM_ATTR list_t l_130729_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
static EXT_RAM_ATTR list_t l_130725_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
static EXT_RAM_ATTR list_t l_130552_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
static EXT_RAM_ATTR list_t l_130535_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
static EXT_RAM_ATTR list_t l_130551_dgn;
static EXT_RAM_ATTR list_t l_130550_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
static EXT_RAM_ATTR list_t l_130548_dgn;
static EXT_RAM_ATTR list_t l_130549_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
static EXT_RAM_ATTR list_t l_130512_dgn;
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
static EXT_RAM_ATTR list_t l_130724_dgn;
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
static EXT_RAM_ATTR list_t l_130526_dgn;
#endif
#ifdef RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS
static EXT_RAM_ATTR list_t l_130768_dgn;
static EXT_RAM_ATTR list_t l_130767_dgn;
#endif
#if defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3) ||                              \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6) ||                              \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9) ||                              \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11) || defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12) ||                           \
    defined(RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13) || defined(RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1) || defined(RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2) || \
    defined(RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS)
static EXT_RAM_ATTR uint8_t l_connector_id;
static EXT_RAM_ATTR list_t *l_prod;

static void update_prop_field(int prodnode_ddminst, uint32_t add_class_inst, uint8_t rvc_inst);

void dcsource_init(uint8_t connector_id, list_t *p_prod)
{
    l_prod = p_prod;
    l_connector_id = connector_id;
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
    LIST_INIT(&l_131069_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
    LIST_INIT(&l_131068_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
    LIST_INIT(&l_131067_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
    LIST_INIT(&l_130761_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
    LIST_INIT(&l_130760_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
    LIST_INIT(&l_130759_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
    LIST_INIT(&l_130732_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
    LIST_INIT(&l_130731_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
    LIST_INIT(&l_130730_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
    LIST_INIT(&l_130729_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
    LIST_INIT(&l_130725_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
    LIST_INIT(&l_130552_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
    LIST_INIT(&l_130535_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
    LIST_INIT(&l_130551_dgn);
    LIST_INIT(&l_130550_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
    LIST_INIT(&l_130548_dgn);
    LIST_INIT(&l_130549_dgn);
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
    LIST_INIT(&l_130512_dgn);
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
    {
        LIST_INIT(&l_130724_dgn);
        uint32_t class_inst = RVCDCSRCCMD0;
        int instance = broker_register_instance(&class_inst, connector_id);
        ASSERT(instance > -1);
    }
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
    {
        LIST_INIT(&l_130526_dgn);
        uint32_t class_inst = RVCDCSRCCFGTHR0;
        int instance = broker_register_instance(&class_inst, connector_id);
        ASSERT(instance > -1);
    }
#endif
#ifdef RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS
    LIST_INIT(&l_130768_dgn);
    LIST_INIT(&l_130767_dgn);
#endif
}

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
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
    {&l_131069_dgn, RVCDCSRC0, "RVCDCSRC0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
    {&l_131068_dgn, RVCDCSRCTWO0, "RVCDCSRCTWO0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
    {&l_131067_dgn, RVCDCSRCTHR0, "RVCDCSRCTHR0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
    {&l_130761_dgn, RVCDCSRCFOUR0, "RVCDCSRCFOUR0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
    {&l_130760_dgn, RVCDCSRCFIVE0, "RVCDCSRCFIVE0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
    {&l_130759_dgn, RVCDCSRCSIX0, "RVCDCSRCSIX0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
    {&l_130732_dgn, RVCDCSRCSEV0, "RVCDCSRCSEV0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
    {&l_130731_dgn, RVCDCSRCEIG0, "RVCDCSRCEIG0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
    {&l_130730_dgn, RVCDCSRCNINE0, "RVCDCSRCNINE0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
    {&l_130729_dgn, RVCDCSRCTEN0, "RVCDCSRCTEN0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
    {&l_130725_dgn, RVCDCSRCELE0, "RVCDCSRCELE0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
    {&l_130552_dgn, RVCDCSRCTWE0, "RVCDCSRCTWE0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
    {&l_130535_dgn, RVCDCSRCTHI0, "RVCDCSRCTHI0", true},
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
    {&l_130551_dgn, RVCDCSRCCFG0, "RVCDCSRCCFG0 status", true},
    {&l_130550_dgn, RVCDCSRCCFG0, "RVCDCSRCCFG0 command", false},  // Don't unregister
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2
    {&l_130548_dgn, RVCDCSRCCFGTWO0, "RVCDCSRCCFGTWO0 status", true},    // Don't unregister
    {&l_130549_dgn, RVCDCSRCCFGTWO0, "RVCDCSRCCFGTWO0 command", false},  // Don't unregister
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
    {&l_130512_dgn, RVCDCSRCCONN0, "RVCDCSRCCONN0", true},
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
    {&l_130724_dgn, RVCDCSRCCMD0, "RVCDCSRCCMD0", false},  // Command classes registered with instance 0
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
    {&l_130526_dgn, RVCDCSRCCFGTHR0, "RVCDCSRCCFGTHR0", false},  // Command classes registered with instance 0
#endif
#ifdef RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS
    {&l_130768_dgn, RVCDCDISCONN0, "RVCDCDISCONN0 status", true},
#endif
};

/**
 * @brief Remove all DC source nodes associated with a specific source address
 *
 * @param sa Source address of the device
 */
void remove_dcsource_nodes(uint8_t sa)
{
    DgnNode_t *dgn_node = NULL;

    LOG(D, "Removing DC source nodes for SA=%02X", sa);

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
                uint32_t class_inst = dgn_lists[i].class_base | DDM2_PARAMETER_INSTANCE(dgn_node->ddm_instance);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, class_inst, &Zero, sizeof(Zero),
                                               l_connector_id, (TickType_t)portMAX_DELAY);
            }

            // Remove the node
            DgnNodeDelete(dgn_node);
        }
    }

    LOG(D, "Completed removing DC source nodes for SA=%02X", sa);
}

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

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_1
/**
 * @brief DC Source Status 1 DGN received (131069)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131069Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131069 zDGN;
    void *value = NULL;
    int32_t val;
    uint32_t u_val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    RVCDGN_DGN_131069_Extract(&zDGN, p_data);

    // Compare with last data
    DgnNode_t *prod_node = NULL;
    prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            val = zDGN.u8DCInstance;
            value = &val;
            convert_rvc_to_ddm_rvc_params(RVCDCSRC0INST, value, RVC_INST_UINT8);
            dgn_node = DgnNodeFindBySourceAddress(&l_131069_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst;
                class_inst = RVCDCSRC0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131069));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_131069_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRC0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRC0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            val = zDGN.u8DevicePriority;
            value = &val;
            convert_rvc_to_ddm_rvc_params(RVCDCSRC0PRIO, value, RVC_STD_UINT8);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRC0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DCVoltage != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            val = zDGN.u16DCVoltage;
            value = &val;
            convert_rvc_to_ddm_rvc_params(RVCDCSRC0VOLT, value, RVC_VACDC_UINT16);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRC0VOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u32DCCurrent != NMEA2K_UINT32_NO_DATA)
        {
            updated_data = true;
            u_val = zDGN.u32DCCurrent;
            value = &u_val;
            convert_rvc_to_ddm_rvc_params(RVCDCSRC0CURR, value, RVC_AACDC_UINT32);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRC0CURR | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            val = 1;
            value = &val;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRC0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRC0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    uint32_t u_val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131069 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_131069_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131069_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRC0INST:
            val = zDGN.u8DCInstance;
            value = &val;
            convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRC0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                val = zDGN.u8DevicePriority;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRC0VOLT:
            if (zDGN.u16DCVoltage != NMEA2K_UINT16_NO_DATA)
            {
                val = zDGN.u16DCVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRC0CURR:
            if (zDGN.u32DCCurrent != NMEA2K_UINT32_NO_DATA)
            {
                u_val = zDGN.u32DCCurrent;
                value = &u_val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT32);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(uint32_t), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_2
/**
 * @brief DC Source Status 2 DGN received (131068)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131068Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131068 zDGN;
    RVCDGN_zDGN_131068 zDGNPrev;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    memset(&zDGNPrev, 0xFF, sizeof(zDGNPrev));
    RVCDGN_DGN_131068_Extract(&zDGN, p_data);
    // Compare with last data
    DgnNode_t *prod_node = NULL;
    prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCTWO0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_131068_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst;
                class_inst = RVCDCSRCTWO0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131068));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_131068_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCTWO0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
                RVCDGN_DGN_131068_Extract(&zDGNPrev, dgn_node->dgn_data);
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWO0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCTWO0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWO0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16SourceTemperature != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16SourceTemperature;
            convert_rvc_to_ddm_system_value(RVCDCSRCTWO0STEMP, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWO0STEMP | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8StateOfCharge != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8StateOfCharge;
            convert_rvc_to_ddm_system_value(RVCDCSRCTWO0SOC, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWO0SOC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        // Time remaining is a special case. If the BMS sends 0xFFFF is most likely means it cannot determine the remaining time (too low discharge current). We will consider that as time=0.
        if (zDGN.u16TimeRemaining != NMEA2K_UINT16_NO_DATA)
        {
            value = zDGN.u16TimeRemaining;
        }
        else
        {
            // Default to 0 when we don't have any data
            value = 0;
        }
        if (((zDGNPrev.u16TimeRemaining != zDGN.u16TimeRemaining) && (value != 0)) || (zDGNPrev.u8DCInstance == 0xFF))
        {
            convert_rvc_to_ddm_system_value(RVCDCSRCTWO0TIMEREM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWO0TIMEREM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            updated_data = true;
        }
        // If no value is received we should interpreted as Time to empty
        if (zDGN.u2TimeRemainingInterpr == NMEA2K_UINT2_NO_DATA)
        {
            value = RVC2DCSRC0TIMEREMTYPE_TIME_TO_EMPTY;
        }
        else
        {
            value = zDGN.u2TimeRemainingInterpr;
        }
        if (zDGNPrev.u2TimeRemainingInterpr == NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWO0TIMEREMTYPE | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

/**
 * @brief RVCDCSRCTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131068 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_131068_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131068_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCTWO0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCTWO0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTWO0STEMP:
            if (zDGN.u16SourceTemperature != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16SourceTemperature;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTWO0SOC:
            if (zDGN.u8StateOfCharge != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8StateOfCharge;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTWO0TIMEREM:
            if (zDGN.u16TimeRemaining != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16TimeRemaining;
            }
            else
            {
                // Default to 0 when we don't have any data
                value = 0;
            }
            convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCTWO0TIMEREMTYPE:
            if (zDGN.u2TimeRemainingInterpr != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u2TimeRemainingInterpr;
            }
            // If no value is received we should interpreted as Time to empty
            else
            {
                value = RVC2DCSRC0TIMEREMTYPE_TIME_TO_EMPTY;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_3
/**
 * @brief DC Source Status 3 DGN received (131067)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131067Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131067 zDGN;
    int32_t value;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    RVCDGN_DGN_131067_Extract(&zDGN, p_data);
    DgnNode_t *dgn_node = NULL;
    DgnNode_t *prod_node = NULL;
    prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        // Compare with last data
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHR0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_131067_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst;
                class_inst = RVCDCSRCTHR0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131067));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_131067_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCTHR0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHR0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHR0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHR0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8StateOfHealth != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8StateOfHealth;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHR0SOH, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHR0SOH | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16CapacityRemaining != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16CapacityRemaining;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHR0CAP, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHR0CAP | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8RelativeCapacity != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8RelativeCapacity;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHR0RELCAP, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHR0RELCAP | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16ACRMSRipple != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16ACRMSRipple;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHR0RIPPLE, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHR0RIPPLE | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

/**
 * @brief RVCDCSRCTHR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCTHR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131067 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_131067_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131067_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCTHR0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCTHR0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTHR0SOH:
            if (zDGN.u8StateOfHealth != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8StateOfHealth;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTHR0CAP:
            if (zDGN.u16CapacityRemaining != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16CapacityRemaining;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTHR0RELCAP:
            if (zDGN.u8RelativeCapacity != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8RelativeCapacity;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTHR0RIPPLE:
            if (zDGN.u16ACRMSRipple != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16ACRMSRipple;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_4
bool receive130761Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130761 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130761_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCFOUR0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130761_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCFOUR0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130761));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130761_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCFOUR0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFOUR0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCFOUR0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFOUR0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DesiredChargeState != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DesiredChargeState;
            convert_rvc_to_ddm_system_value(RVCDCSRCFOUR0CHGST, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFOUR0CHGST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DesiredDCVoltage != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16DesiredDCVoltage;
            convert_rvc_to_ddm_system_value(RVCDCSRCFOUR0VOLT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFOUR0VOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DesiredDCCurrent != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16DesiredDCCurrent;
            convert_rvc_to_ddm_system_value(RVCDCSRCFOUR0CURR, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFOUR0CURR | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8BatteryType != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8BatteryType;
            convert_rvc_to_ddm_system_value(RVCDCSRCFOUR0TYPE, &value);
            if (value > RVC4DCSRC0TYPE_LITHIUM_IRON_PHOSPHATE)
            {
                value = RVC4DCSRC0TYPE_CUSTOM;  // Map all unknown types to 'Other'
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFOUR0TYPE | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFOUR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRCFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130761 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130761_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130761_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCFOUR0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCFOUR0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCFOUR0CHGST:
            if (zDGN.u8DesiredChargeState != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DesiredChargeState;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCFOUR0VOLT:
            if (zDGN.u16DesiredDCVoltage != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16DesiredDCVoltage;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCFOUR0CURR:
            if (zDGN.u16DesiredDCCurrent != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16DesiredDCCurrent;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCFOUR0TYPE:
            if (zDGN.u8BatteryType != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8BatteryType;
                if (value > RVC4DCSRC0TYPE_LITHIUM_IRON_PHOSPHATE)
                {
                    value = RVC4DCSRC0TYPE_CUSTOM;  // Map all unknown types to 'Other'
                }
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_5
/**
 * @brief DC Source Status 5 DGN received (130760)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130760Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130760 zDGN;
    void *value = NULL;
    int32_t val;
    uint32_t u_val;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    RVCDGN_DGN_130760_Extract(&zDGN, p_data);

    // Compare with last data
    DgnNode_t *prod_node = NULL;
    prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            val = zDGN.u8DCInstance;
            value = &val;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCFIVE0INST, value, RVC_INST_UINT8);
            dgn_node = DgnNodeFindBySourceAddress(&l_130760_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCFIVE0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130760));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130760_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCFIVE0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFIVE0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            val = zDGN.u8DevicePriority;
            value = &val;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCFIVE0PRIO, value, RVC_STD_UINT8);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFIVE0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u32HPDCVoltage != NMEA2K_UINT32_NO_DATA)
        {
            updated_data = true;
            u_val = zDGN.u32HPDCVoltage;
            value = &u_val;
            convert_rvc_to_ddm_rvc_params_gain(RVCDCSRCFIVE0VOLT, value, RVC_STD_UINT32, 1000);  // user supplied gain as not standard
            if (u_val > INT32_MAX)
            {
                LOG(E, "Received (u32HPDCVoltage) value too large: %" PRIu32 " for parameter", zDGN.u32HPDCVoltage);
                u_val = INT32_MAX;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFIVE0VOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(uint32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            val = 1;
            value = &val;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCFIVE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

/**
 * @brief RVCDCSRCFIVE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCFIVE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    uint32_t u_val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130760 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130760_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130760_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCFIVE0INST:
            val = zDGN.u8DCInstance;
            value = &val;
            convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCFIVE0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                val = zDGN.u8DevicePriority;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCFIVE0VOLT:
            if (zDGN.u32HPDCVoltage != NMEA2K_UINT32_NO_DATA)
            {
                u_val = zDGN.u32HPDCVoltage;
                value = &u_val;
                convert_rvc_to_ddm_rvc_params_gain(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT32, 1000);  // user supplied gain as not standard
                if (u_val > INT32_MAX)
                {
                    LOG(E, "Received (u32HPDCVoltage) value too large: %" PRIu32 " for parameter", zDGN.u32HPDCVoltage);
                    u_val = INT32_MAX;
                }

                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, value, sizeof(uint32_t), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_6
bool receive130759Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130759 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130759_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130759_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCSIX0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130759));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130759_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCSIX0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2HighVoltageLimitStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2HighVoltageLimitStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0HVLIM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0HVLIM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2HighVoltageDisconnectStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2HighVoltageDisconnectStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0HVDIS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0HVDIS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2LowVoltageLimitStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2LowVoltageLimitStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0LVLIM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0LVLIM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2LowVoltageDisconnectStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2LowVoltageDisconnectStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0LVDIS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0LVDIS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2LowSOCLimitStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2LowSOCLimitStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0LSOCLIM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0LSOCLIM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2LowSOCDisconnectStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2LowSOCDisconnectStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0LSOCDIS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0LSOCDIS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2LowDCSourceTempLimitStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2LowDCSourceTempLimitStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0LDCTEMPLIM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0LDCTEMPLIM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2LowDCSourceTempDisconnectStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2LowDCSourceTempDisconnectStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0LDCTEMPDIS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0LDCTEMPDIS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2HighDCSourceTempLimitStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2HighDCSourceTempLimitStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0HDCTEMPLIM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0HDCTEMPLIM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2HighDCSourceTempDisconnectStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2HighDCSourceTempDisconnectStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0HDCTEMPDIS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0HDCTEMPDIS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2HighCurrentDCSourceLimit != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2HighCurrentDCSourceLimit;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0HDCCURRLIM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0HDCCURRLIM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2HighCurrentDCSourceDisconnect != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2HighCurrentDCSourceDisconnect;
            convert_rvc_to_ddm_system_value(RVCDCSRCSIX0HDCCURRDIS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0HDCCURRDIS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSIX0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    else
    {
        request_info_request_dgn(PRODUCT_ID_MSG_DGN, sa);
    }
    return true;
}

void handleRVCDCSRCSIX0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130759 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130759_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130759_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCSIX0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCSIX0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0HVLIM:
            if (zDGN.u2HighVoltageLimitStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2HighVoltageLimitStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0HVDIS:
            if (zDGN.u2HighVoltageDisconnectStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2HighVoltageDisconnectStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0LVLIM:
            if (zDGN.u2LowVoltageLimitStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2LowVoltageLimitStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0LVDIS:
            if (zDGN.u2LowVoltageDisconnectStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2LowVoltageDisconnectStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0LSOCLIM:
            if (zDGN.u2LowSOCLimitStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2LowSOCLimitStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0LSOCDIS:
            if (zDGN.u2LowSOCDisconnectStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2LowSOCDisconnectStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0LDCTEMPLIM:
            if (zDGN.u2LowDCSourceTempLimitStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2LowDCSourceTempLimitStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0LDCTEMPDIS:
            if (zDGN.u2LowDCSourceTempDisconnectStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2LowDCSourceTempDisconnectStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0HDCCURRLIM:
            if (zDGN.u2HighCurrentDCSourceLimit != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2HighCurrentDCSourceLimit;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0HDCCURRDIS:
            if (zDGN.u2HighCurrentDCSourceDisconnect != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2HighCurrentDCSourceDisconnect;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0HDCTEMPLIM:
            if (zDGN.u2HighDCSourceTempLimitStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2HighDCSourceTempLimitStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSIX0HDCTEMPDIS:
            if (zDGN.u2HighDCSourceTempDisconnectStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2HighDCSourceTempDisconnectStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_7
bool receive130732Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130732 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130732_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCSEV0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130732_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCSEV0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130732));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130732_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCSEV0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSEV0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCSEV0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSEV0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16TodayInputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16TodayInputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCSEV0INPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSEV0INPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16TodayOutputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16TodayOutputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCSEV0OUTPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSEV0OUTPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCSEV0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRCSEV0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130732 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130732_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130732_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCSEV0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCSEV0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSEV0INPUT:
            if (zDGN.u16TodayInputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16TodayInputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCSEV0OUTPUT:
            if (zDGN.u16TodayOutputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16TodayOutputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_8
bool receive130731Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130731 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130731_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCEIG0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130731_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCEIG0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130731));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130731_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCEIG0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCEIG0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCEIG0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCEIG0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16YesterdayInputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16YesterdayInputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCEIG0INPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCEIG0INPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16YesterdayOutputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16YesterdayOutputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCEIG0OUTPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCEIG0OUTPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCEIG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRCEIG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130731 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130731_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130731_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCEIG0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCEIG0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCEIG0INPUT:
            if (zDGN.u16YesterdayInputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16YesterdayInputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCEIG0OUTPUT:
            if (zDGN.u16YesterdayOutputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16YesterdayOutputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_9
bool receive130730Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130730 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130730_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCNINE0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130730_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCNINE0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130730));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130730_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCNINE0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCNINE0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCNINE0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCNINE0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DayBeforeYestInputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16DayBeforeYestInputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCNINE0INPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCNINE0INPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DayBeforeYestOutputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16DayBeforeYestOutputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCNINE0OUTPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCNINE0OUTPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCNINE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRCNINE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130730 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130730_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130730_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCNINE0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCNINE0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCNINE0INPUT:
            if (zDGN.u16DayBeforeYestInputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16DayBeforeYestInputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCNINE0OUTPUT:
            if (zDGN.u16DayBeforeYestOutputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16DayBeforeYestOutputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_10
bool receive130729Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130729 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130729_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCTEN0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130729_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCTEN0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130729));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130729_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCTEN0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTEN0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCTEN0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTEN0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16LastSevenDaysInputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16LastSevenDaysInputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCTEN0INPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTEN0INPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16LastSevenDaysOutputAmpHours != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16LastSevenDaysOutputAmpHours;
            convert_rvc_to_ddm_system_value(RVCDCSRCTEN0OUTPUT, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTEN0OUTPUT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTEN0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRCTEN0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130729 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130729_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130729_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCTEN0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCTEN0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTEN0INPUT:
            if (zDGN.u16LastSevenDaysInputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16LastSevenDaysInputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTEN0OUTPUT:
            if (zDGN.u16LastSevenDaysOutputAmpHours != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16LastSevenDaysOutputAmpHours;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_11
/**
 * @brief DC Source Status 11 DGN received (130725)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130725Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130725 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    RVCDGN_DGN_130725_Extract(&zDGN, p_data);
    // Compare with last data
    DgnNode_t *prod_node = NULL;
    prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130725_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst;
                class_inst = RVCDCSRCELE0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130725));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130725_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCELE0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2DischargeStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2DischargeStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0DISCHGST, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0DISCHGST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2ChargeStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2ChargeStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0CHGST, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0CHGST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2ChargeDetected != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2ChargeDetected;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0CHGDET, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0CHGDET | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2ReserveStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2ReserveStatus;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0RESST, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0RESST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16FullCapacity != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16FullCapacity;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0CAPACITY, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0CAPACITY | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DCPower != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16DCPower;
            convert_rvc_to_ddm_system_value(RVCDCSRCELE0POWER, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0POWER | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCELE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    else
    {
        request_info_request_dgn(PRODUCT_ID_MSG_DGN, sa);
    }
    return true;
}

/**
 * @brief RVCDCSRCELE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCELE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130725 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130725_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    RVCDGN_DGN_130725_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCELE0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCELE0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCELE0DISCHGST:
            if (zDGN.u2DischargeStatus != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u2DischargeStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCELE0CHGST:
            if (zDGN.u2ChargeStatus != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u2ChargeStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCELE0CHGDET:
            if (zDGN.u2ChargeDetected != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u2ChargeDetected;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCELE0RESST:
            if (zDGN.u2ReserveStatus != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u2ReserveStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCELE0CAPACITY:
            if (zDGN.u16FullCapacity != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16FullCapacity;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCELE0POWER:
            if (zDGN.u16DCPower != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16DCPower;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_12
bool receive130552Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130552 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130552_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCTWE0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130552_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCTWE0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130552));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130552_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCTWE0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWE0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCTWE0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWE0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16Cycles != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16Cycles;
            convert_rvc_to_ddm_system_value(RVCDCSRCTWE0CYCLES, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWE0CYCLES | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DeepsetDischargeDepth != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16DeepsetDischargeDepth;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCTWE0DEEP, &value, RVC_AMPHOUR_UINT16);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWE0DEEP | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16AverageDischargeDepth != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16AverageDischargeDepth;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCTWE0AVERAGE, &value, RVC_AMPHOUR_UINT16);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWE0AVERAGE | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTWE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRCTWE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130552 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130552_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130552_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCTWE0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCTWE0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTWE0CYCLES:
            if (zDGN.u16Cycles != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16Cycles;
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTWE0DEEP:
            if (zDGN.u16DeepsetDischargeDepth != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16DeepsetDischargeDepth;
                convert_rvc_to_ddm_rvc_params(RVCDCSRCTWE0DEEP, &value, RVC_AMPHOUR_UINT16);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTWE0AVERAGE:
            if (zDGN.u16AverageDischargeDepth != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16AverageDischargeDepth;
                convert_rvc_to_ddm_rvc_params(RVCDCSRCTWE0AVERAGE, &value, RVC_AMPHOUR_UINT16);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_STATUS_13
bool receive130535Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130535 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130535_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHI0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130535_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCTHI0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130535));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130535_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCTHI0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHI0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DevicePriority;
            convert_rvc_to_ddm_system_value(RVCDCSRCTHI0PRIO, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHI0PRIO | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16LowestDCSourceVoltage != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16LowestDCSourceVoltage;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCTHI0LOWVOLT, &value, RVC_VACDC_UINT16);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHI0LOWVOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16HighestDCSourceVoltage != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16HighestDCSourceVoltage;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCTHI0HIGHVOLT, &value, RVC_VACDC_UINT16);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHI0HIGHVOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCTHI0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

void handleRVCDCSRCTHI0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130535 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130535_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130535_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCTHI0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCTHI0PRIO:
            if (zDGN.u8DevicePriority != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DevicePriority;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTHI0LOWVOLT:
            if (zDGN.u16LowestDCSourceVoltage != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16LowestDCSourceVoltage;
                convert_rvc_to_ddm_rvc_params(RVCDCSRCTHI0LOWVOLT, &value, RVC_VACDC_UINT16);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCTHI0HIGHVOLT:
            if (zDGN.u16HighestDCSourceVoltage != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16HighestDCSourceVoltage;
                convert_rvc_to_ddm_rvc_params(RVCDCSRCTHI0HIGHVOLT, &value, RVC_VACDC_UINT16);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_1
/**
 * @brief DC Source Configuration Status 1 DGN received (130551)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130551Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130551 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130551_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCCFG0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130551_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCCFG0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                ddm_instance = 0;
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130551));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for DC_SRC_CONF_STATUS_1", ddm_instance);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130551_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCCFG0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8PeukertExponent != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8PeukertExponent;
            convert_rvc_to_ddm_system_value(RVCDCSRCCFG0EXP, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0EXP | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8TempCoefficient != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8TempCoefficient;
            convert_rvc_to_ddm_system_value(RVCDCSRCCFG0COEFF, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0COEFF | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8ChargeEfficiency != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8ChargeEfficiency;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCCFG0FACT, &value, RVC_PART_UINT8);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0FACT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8TimeRemAveraging != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8TimeRemAveraging;
            convert_rvc_to_ddm_system_value(RVCDCSRCCFG0PERIOD, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0PERIOD | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16FullCapacity != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16FullCapacity;
            convert_rvc_to_ddm_system_value(RVCDCSRCCFG0CAPACITY, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0CAPACITY | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8TailCurrent != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8TailCurrent;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCCFG0TAILCURR, &value, RVC_PART_UINT8);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0TAILCURR | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

/**
 * @brief Prepare a DC source configuration command 1 frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130550Dgn(uint8_t instance, uint8_t *p_data)
{
    // Only instance 0 is supported for all commands
    if (instance == 0)
    {
        // Stuff message data
        DgnNode_t *dgn_node = NULL;
        dgn_node = DgnNodeFindByDdmInstance(&l_130550_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130550_Stuff(p_data, dgn_node->dgn_data);
            ESP_LOG_BUFFER_HEXDUMP("dc src conf data", dgn_node->dgn_data, RVCDGN_DGN_130550_SIZE, ESP_LOG_INFO);
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
}

/**
 * @brief RVCDCSRCCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130551 zDGN;
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_130551_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130551_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for DC_SOURCE_CONF_STATUS_1", DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter), RVCDCSRCCFG0);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCCFG0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCCFG0EXP:
            if (zDGN.u8PeukertExponent != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8PeukertExponent;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFG0COEFF:
            if (zDGN.u8TempCoefficient != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8TempCoefficient;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFG0FACT:
            if (zDGN.u8ChargeEfficiency != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8ChargeEfficiency;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFG0PERIOD:
            if (zDGN.u8TimeRemAveraging != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8TimeRemAveraging;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFG0CAPACITY:
            if (zDGN.u16FullCapacity != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16FullCapacity;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFG0TAILCURR:
            if (zDGN.u8TailCurrent != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8TailCurrent;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif

#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONNECTION_STATUS
/**
 * @brief DC Source Connection Status DGN received (130512)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130512Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130512 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    // Extract message content
    RVCDGN_DGN_130512_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DeviceInstance == 0)
        {
            LOG(E, "Invalid Device Instance %d", zDGN.u8DeviceInstance);
            return false;
        }
        if (zDGN.u8DeviceInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DeviceInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCCONN0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130512_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCCONN0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8DeviceInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130512));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130512_dgn);
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCONN0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8DeviceDSA != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DeviceDSA;
            convert_rvc_to_ddm_system_value(RVCDCSRCCONN0DSA, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCONN0DSA | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u4Function != NMEA2K_UINT4_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u4Function;
            convert_rvc_to_ddm_system_value(RVCDCSRCCONN0FUNC, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCONN0FUNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8PrimaryDCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8PrimaryDCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCCONN0PRIM, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCONN0PRIM | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8SecondaryDCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8SecondaryDCInstance;
            convert_rvc_to_ddm_system_value(RVCDCSRCCONN0SEC, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCONN0SEC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCONN0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

/**
 * @brief RVCDCSRCCONN0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCCONN0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130512 zDGN;

    dgn_node = DgnNodeFindByDdmInstance(&l_130512_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130512_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCCONN0INST:
            value = zDGN.u8DeviceInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCCONN0DSA:
            if (zDGN.u8DeviceDSA != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DeviceDSA;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCONN0FUNC:
            if (zDGN.u4Function != NMEA2K_UINT4_NO_DATA)
            {
                value = zDGN.u4Function;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCONN0PRIM:
            if (zDGN.u8PrimaryDCInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8PrimaryDCInstance;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCONN0SEC:
            if (zDGN.u8SecondaryDCInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8SecondaryDCInstance;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_COMMAND
/**
 * @brief Prepare a DC source command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130724Dgn(uint8_t instance, uint8_t *p_data)
{
    // Only instance 0 is supported for all commands
    if (instance == 0)
    {
        // Stuff message data
        DgnNode_t *dgn_node = NULL;
        dgn_node = DgnNodeFindByDdmInstance(&l_130724_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130724_Stuff(p_data, dgn_node->dgn_data);
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
}

/**
 * @brief RVCDCSRCCMD0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCCMD0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130724 zDGN;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
    }
    else if (p_frame->frame.control == DDMP2_CONTROL_SET)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    }

    dgn_node = DgnNodeFindByDdmInstance(&l_130724_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130724_Extract(&zDGN, dgn_node->dgn_data);
    }
    else
    {
        uint8_t data_dgn[RVCDGN_DGN_130724_SIZE] = {0xFF};
        dgn_node = DgnNodeCreate(0, instance, RVC_INITIAL_SOURCE_ADDRESS, data_dgn, sizeof(RVCDGN_zDGN_130724));
        if (dgn_node == NULL)
        {
            LOG(E, "DgnNode with DDM instance %d cannot be allocated for DC_SOURCE_COMMAND", instance);
            return;
        }
        DgnNodeInsert(dgn_node, &l_130724_dgn);
        RVCDGN_DGN_130724_Extract(&zDGN, dgn_node->dgn_data);
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCCMD0INST:
            value = zDGN.u8DCInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCCMD0PWRST:
            if (zDGN.u2DesiredPowerOnOffStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2DesiredPowerOnOffStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCMD0CHGST:
            if (zDGN.u2DesiredChargeOnOffStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2DesiredChargeOnOffStatus;
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
            case RVCDCSRCCMD0INST:
                zDGN.u8DCInstance = value;
                break;
            case RVCDCSRCCMD0PWRST:
                zDGN.u2DesiredPowerOnOffStatus = value;
                break;
            case RVCDCSRCCMD0CHGST:
                zDGN.u2DesiredChargeOnOffStatus = value;
                break;
            case RVCDCSRCCMD0SYNC:
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
#ifdef RVC_CONFIG_INTERF_DC_SOURCE_CONFIGURATION_STATUS_2

// Offset are parameter index
static const rvc_data_type_t RVCDCSRCCFGTWO0_type[] = {
    RVC_STD_UINT8,           // AVL
    RVC_STD_UINT8,           // SYNC
    RVC_INST_UINT8,          // INST
    RVC_STD_UINT8,           // CLRHIST
    RVC_STD_UINT8,           // SETCAP
    RVC_VACDC_UINT16,        // CHGVOLT
    RVC_STD_UINT8_GAIN1000,  // SHTVOLT
    RVC_AACDC_UINT16,        // SHTCURR
    RVC_STD_UINT8,           // RSTBATH
    RVC_STD_UINT8,           // BATTYPE
};

/**
 * @brief DC Source Configuration Status 2 DGN received (130549)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130549Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130549 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130549_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8DCInstance == 0)
        {
            LOG(E, "Invalid DC Instance %d", zDGN.u8DCInstance);
            return false;
        }
        if (zDGN.u8DCInstance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8DCInstance;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCCFGTWO0INST, &value, RVCDCSRCCFGTWO0_type[DDM2_PARAMETER_PROPERTY_FIELD(RVCDCSRCCFGTWO0INST)]);
            dgn_node = DgnNodeFindBySourceAddress(&l_130549_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCSRCCFGTWO0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                ddm_instance = 0;
                dgn_node = DgnNodeCreate(zDGN.u8DCInstance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130549));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for DC_SRC_CONF_STATUS_2", ddm_instance);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130549_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCSRCCFGTWO0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8DCInstance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFGTWO0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16ChargedVoltage != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16ChargedVoltage;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCCFGTWO0CHGVOLT, &value, RVCDCSRCCFGTWO0_type[DDM2_PARAMETER_PROPERTY_FIELD(RVCDCSRCCFGTWO0CHGVOLT)]);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFGTWO0CHGVOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u8ShuntVoltage != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8ShuntVoltage;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCCFGTWO0SHTVOLT, &value, RVCDCSRCCFGTWO0_type[DDM2_PARAMETER_PROPERTY_FIELD(RVCDCSRCCFGTWO0SHTVOLT)]);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFGTWO0SHTVOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16ShuntCurrent != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16ShuntCurrent;
            convert_rvc_to_ddm_rvc_params(RVCDCSRCCFGTWO0SHTCURR, &value, RVCDCSRCCFGTWO0_type[DDM2_PARAMETER_PROPERTY_FIELD(RVCDCSRCCFGTWO0SHTCURR)]);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFGTWO0SHTCURR | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCSRCCFGTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

/**
 * @brief Prepare a DC source configuration command 2 frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130548Dgn(uint8_t instance, uint8_t *p_data)
{
    // Only instance 0 is supported for all commands
    if (instance == 0)
    {
        // Stuff message data
        DgnNode_t *dgn_node = NULL;
        dgn_node = DgnNodeFindByDdmInstance(&l_130548_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130548_Stuff(p_data, dgn_node->dgn_data);
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
}

/**
 * @brief RVCDCSRCCFGTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130549 zDGN;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_130549_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130549_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for DC_SOURCE_CONF_STATUS_2", DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter), RVCDCSRCCFG0);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        bool to_send = false;
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCCFGTWO0INST:
            value = zDGN.u8DCInstance;
            to_send = true;
            break;
        case RVCDCSRCCFGTWO0CLRHIST:
            if (zDGN.u2ClearHistory != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2ClearHistory;
                to_send = true;
            }
            break;
        case RVCDCSRCCFGTWO0SETCAP:
            if (zDGN.u2SetCapacity100 != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2SetCapacity100;
                to_send = true;
            }
            break;
        case RVCDCSRCCFGTWO0CHGVOLT:
            if (zDGN.u16ChargedVoltage != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16ChargedVoltage;
                to_send = true;
            }
            break;
        case RVCDCSRCCFGTWO0SHTVOLT:
            if (zDGN.u8ShuntVoltage != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8ShuntVoltage;
                to_send = true;
            }
            break;
        case RVCDCSRCCFGTWO0SHTCURR:
            if (zDGN.u16ShuntCurrent != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16ShuntCurrent;
                to_send = true;
            }
            break;
        case RVCDCSRCCFGTWO0RSTBATH:
            if (zDGN.u2ResetBatteryHealth != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2ResetBatteryHealth;
                to_send = true;
            }
            break;
        case RVCDCSRCCFGTWO0BATTYPE:
            if (zDGN.u4BatteryType != NMEA2K_UINT4_NO_DATA)
            {
                value = zDGN.u4BatteryType;
                to_send = true;
            }
            break;
        default:
            break;
        }
        if (to_send)
        {
            rvc_data_type_t type = RVCDCSRCCFGTWO0_type[DDM2_PARAMETER_PROPERTY_FIELD(p_frame->frame.subscribe.parameter)];
            if (convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, type))
            {
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
        }
    }
    else if (p_frame->frame.control == DDMP2_CONTROL_SET)
    {
        value = p_frame->frame.set.value.int32;
        if (convert_ddm_to_rvc_value(p_frame->frame.set.parameter, &value, RVCDCSRCCFGTWO0_type[DDM2_PARAMETER_PROPERTY_FIELD(p_frame->frame.set.parameter)]))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCDCSRCCFGTWO0INST:
                zDGN.u8DCInstance = value;
                break;
            case RVCDCSRCCFGTWO0CLRHIST:
                zDGN.u2ClearHistory = value;
                break;
            case RVCDCSRCCFGTWO0SETCAP:
                zDGN.u2SetCapacity100 = value;
                break;
            case RVCDCSRCCFGTWO0CHGVOLT:
                zDGN.u16ChargedVoltage = value;
                break;
            case RVCDCSRCCFGTWO0SHTVOLT:
                zDGN.u8ShuntVoltage = value;
                break;
            case RVCDCSRCCFGTWO0SHTCURR:
                zDGN.u16ShuntCurrent = value;
                break;
            case RVCDCSRCCFGTWO0RSTBATH:
                zDGN.u2ResetBatteryHealth = value;
                break;
            case RVCDCSRCCFGTWO0BATTYPE:
                zDGN.u4BatteryType = value;
                break;
            case RVCDCSRCCFGTWO0SYNC:
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
#ifdef RVC_CONFIG_IMPL_DC_SOURCE_CONFIGURATION_COMMAND_3
/**
 * @brief Prepare a DC source configuration command 3 frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130526Dgn(uint8_t instance, uint8_t *p_data)
{
    // Only instance 0 is supported for all commands
    if (instance == 0)
    {
        // Stuff message data
        DgnNode_t *dgn_node = NULL;
        dgn_node = DgnNodeFindByDdmInstance(&l_130526_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130526_Stuff(p_data, dgn_node->dgn_data);
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
}

/**
 * @brief RVCDCSRCCFGTHR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCSRCCFGTHR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130526 zDGN;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
    }
    else if (p_frame->frame.control == DDMP2_CONTROL_SET)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    }

    dgn_node = DgnNodeFindByDdmInstance(&l_130526_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130526_Extract(&zDGN, dgn_node->dgn_data);
    }
    else
    {
        uint8_t data_dgn[RVCDGN_DGN_130526_SIZE] = {0xFF};
        dgn_node = DgnNodeCreate(0, instance, RVC_INITIAL_SOURCE_ADDRESS, data_dgn, sizeof(RVCDGN_zDGN_130526));
        if (dgn_node == NULL)
        {
            LOG(E, "DgnNode with DDM instance %d cannot be allocated for DC_SOURCE_CONFIGURATION_COMMAND_3", instance);
        }
        DgnNodeInsert(dgn_node, &l_130526_dgn);
        RVCDGN_DGN_130526_Extract(&zDGN, dgn_node->dgn_data);
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCSRCCFGTHR0INST:
            value = zDGN.u8DeviceInstance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCSRCCFGTHR0DSA:
            if (zDGN.u8DeviceDSA != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8DeviceDSA;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFGTHR0FUNC:
            if (zDGN.u4Function != NMEA2K_UINT4_NO_DATA)
            {
                value = zDGN.u4Function;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFGTHR0PRIM:
            if (zDGN.u8PrimaryDCInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8PrimaryDCInstance;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCSRCCFGTHR0SEC:
            if (zDGN.u8SecondaryDCInstance != NMEA2K_UINT8_NO_DATA)
            {
                value = zDGN.u8SecondaryDCInstance;
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
            case RVCDCSRCCFGTHR0INST:
                zDGN.u8DeviceInstance = value;
                break;
            case RVCDCSRCCFGTHR0DSA:
                zDGN.u8DeviceDSA = value;
                break;
            case RVCDCSRCCFGTHR0FUNC:
                zDGN.u4Function = value & 0x0F;
                break;
            case RVCDCSRCCFGTHR0PRIM:
                zDGN.u8PrimaryDCInstance = value;
                break;
            case RVCDCSRCCFGTHR0SEC:
                zDGN.u8SecondaryDCInstance = value;
                break;
            case RVCDCSRCCFGTHR0SYNC:
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

#ifdef RVC_CONFIG_INTERF_DC_DISCONNECT_STATUS
/**
 * @brief DC Disconnect Status DGN received (130768)
 *
 * @param p_data DGN data pointer
 * @param sa Source address
 * @param size Data size
 * @return true if message has been handled
 */
bool receive130768Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130768 zDGN;
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    int32_t ddm_instance = -1;
    bool updated_data = false;

    RVCDGN_DGN_130768_Extract(&zDGN, p_data);
    DgnNode_t *prod_node = DgnNodeFindBySourceAddress(l_prod, sa);
    if (prod_node != NULL)
    {
        if (zDGN.u8Instance == 0)
        {
            LOG(E, "Invalid DC Disconnect Instance %d", zDGN.u8Instance);
            return false;
        }
        if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u8Instance;
            convert_rvc_to_ddm_system_value(RVCDCDISCONN0INST, &value);
            dgn_node = DgnNodeFindBySourceAddress(&l_130768_dgn, sa);
            if (dgn_node == NULL)
            {
                uint32_t class_inst = RVCDCDISCONN0;
                ddm_instance = broker_register_instance(&class_inst, l_connector_id);
                if (ddm_instance == -1)
                {
                    LOG(E, "Registration failed for class %08x", class_inst);
                    return false;
                }
                dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130768));
                if (dgn_node == NULL)
                {
                    LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class_inst);
                    return false;
                }
                DgnNodeInsert(dgn_node, &l_130768_dgn);
                if (prod_node->ddm_instance != -1)
                {
                    uint32_t inst = RVCDCDISCONN0 | DDM2_PARAMETER_INSTANCE(ddm_instance);
                    update_prop_field(prod_node->ddm_instance, inst, zDGN.u8Instance);
                }
            }
            else
            {
                ddm_instance = dgn_node->ddm_instance;
            }
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCDISCONN0INST | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2CircuitStatus != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2CircuitStatus;
            convert_rvc_to_ddm_system_value(RVCDCDISCONN0STS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCDISCONN0STS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2LastCommand != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2LastCommand;
            convert_rvc_to_ddm_system_value(RVCDCDISCONN0CMD, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCDISCONN0CMD | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u2BypassDetect != NMEA2K_UINT2_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u2BypassDetect;
            convert_rvc_to_ddm_system_value(RVCDCDISCONN0BYPASS, &value);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCDISCONN0BYPASS | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u16DCSwitchedVoltage != NMEA2K_UINT16_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u16DCSwitchedVoltage;
            convert_rvc_to_ddm_rvc_params(RVCDCDISCONN0VOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, RVC_VACDC_UINT16);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCDISCONN0VOLT | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (zDGN.u32DCSwitchedCurrent != NMEA2K_UINT32_NO_DATA)
        {
            updated_data = true;
            value = zDGN.u32DCSwitchedCurrent;
            convert_rvc_to_ddm_rvc_params(RVCDCDISCONN0CURR | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, RVC_AACDC_UINT32);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCDISCONN0CURR | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
        if (true == updated_data)
        {
            value = 1;
            DgnNodeUpdate(dgn_node, p_data);
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCDCDISCONN0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
        }
    }
    return true;
}

/**
 * @brief Prepare a DC disconnect command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130767Dgn(uint8_t instance, uint8_t *p_data)
{
    // Only instance 0 is supported for all commands
    if (instance == 0)
    {
        // Stuff message data
        DgnNode_t *dgn_node = NULL;
        dgn_node = DgnNodeFindByDdmInstance(&l_130767_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130767_Stuff(p_data, dgn_node->dgn_data);
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
}

/**
 * @brief RVCDCDISCONN0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCDCDISCONN0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130768 zDGN;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_130768_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130768_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for DC_DISCONNECT_STATUS", DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter), RVCDCDISCONN0);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCDCDISCONN0INST:
            value = zDGN.u8Instance;
            connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            break;
        case RVCDCDISCONN0STS:
            if (zDGN.u2CircuitStatus != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2CircuitStatus;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCDISCONN0CMD:
            if (zDGN.u2LastCommand != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2LastCommand;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCDISCONN0BYPASS:
            if (zDGN.u2BypassDetect != NMEA2K_UINT2_NO_DATA)
            {
                value = zDGN.u2BypassDetect;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCDISCONN0VOLT:
            if (zDGN.u16DCSwitchedVoltage != NMEA2K_UINT16_NO_DATA)
            {
                value = zDGN.u16DCSwitchedVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCDCDISCONN0CURR:
            if (zDGN.u32DCSwitchedCurrent != NMEA2K_UINT32_NO_DATA)
            {
                value = zDGN.u32DCSwitchedCurrent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AACDC_UINT32);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
}
#endif
