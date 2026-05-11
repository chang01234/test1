/*
 * inverter_charger.c
 *
 *  Created on: 11 Nov. 2025
 *      Author: Leo
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"

#include "common.h"
#include "dgnnode.h"
#include "inverter_charger.h"
#include "product_database.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_1
static EXT_RAM_ATTR list_t l_131031_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_2
static EXT_RAM_ATTR list_t l_131030_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_3
static EXT_RAM_ATTR list_t l_131029_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_4
static EXT_RAM_ATTR list_t l_130959_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_DC_STATUS
static EXT_RAM_ATTR list_t l_130792_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_STATUS
static EXT_RAM_ATTR list_t l_131028_dgn;
static EXT_RAM_ATTR list_t l_131027_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_1
static EXT_RAM_ATTR list_t l_131026_dgn;
static EXT_RAM_ATTR list_t l_131024_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_2
static EXT_RAM_ATTR list_t l_131025_dgn;
static EXT_RAM_ATTR list_t l_131023_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_3
static EXT_RAM_ATTR list_t l_130766_dgn;
static EXT_RAM_ATTR list_t l_130765_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_4
static EXT_RAM_ATTR list_t l_130715_dgn;
static EXT_RAM_ATTR list_t l_130714_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS
static EXT_RAM_ATTR list_t l_130749_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS_2
static EXT_RAM_ATTR list_t l_130507_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_1
static EXT_RAM_ATTR list_t l_131018_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_3
static EXT_RAM_ATTR list_t l_131016_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS
static EXT_RAM_ATTR list_t l_131015_dgn;
static EXT_RAM_ATTR list_t l_131013_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS_2
static EXT_RAM_ATTR list_t l_130723_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS
static EXT_RAM_ATTR list_t l_131014_dgn;
static EXT_RAM_ATTR list_t l_131012_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_2
static EXT_RAM_ATTR list_t l_130966_dgn;
static EXT_RAM_ATTR list_t l_130965_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_3
static EXT_RAM_ATTR list_t l_130764_dgn;
static EXT_RAM_ATTR list_t l_130763_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_4
static EXT_RAM_ATTR list_t l_130751_dgn;
static EXT_RAM_ATTR list_t l_130750_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_EQUALIZATION_CONFIGURATION_STATUS
static EXT_RAM_ATTR list_t l_130968_dgn;
static EXT_RAM_ATTR list_t l_130967_dgn;
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_ACFAULT_CONFIG_STATUS_1
static EXT_RAM_ATTR list_t l_130953_dgn;
static EXT_RAM_ATTR list_t l_130951_dgn;
#endif

#define UINT_HZ_GAIN (128)  //!< \~ Precision = 1/128 Hz
#define UINT_S_GAIN  (2)    //!< \~ Precision = 0.5 s

static EXT_RAM_ATTR uint8_t l_connector_id;
static EXT_RAM_ATTR list_t *l_prod;

void inverter_charger_init(uint8_t connector_id, list_t *p_prod)
{
    l_prod = p_prod;
    l_connector_id = connector_id;

#ifdef RVC_CONFIG_INVERTER_CHARGER
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_1
    LIST_INIT(&l_131031_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_2
    LIST_INIT(&l_131030_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_3
    LIST_INIT(&l_131029_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_4
    LIST_INIT(&l_130959_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_DC_STATUS
    LIST_INIT(&l_130792_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_STATUS
    LIST_INIT(&l_131027_dgn);
    LIST_INIT(&l_131028_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_1
    LIST_INIT(&l_131024_dgn);
    LIST_INIT(&l_131026_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_2
    LIST_INIT(&l_131025_dgn);
    LIST_INIT(&l_131023_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_3
    LIST_INIT(&l_130766_dgn);
    LIST_INIT(&l_130765_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_4
    LIST_INIT(&l_130715_dgn);
    LIST_INIT(&l_130714_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS
    LIST_INIT(&l_130749_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS_2
    LIST_INIT(&l_130507_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_1
    LIST_INIT(&l_131018_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_3
    LIST_INIT(&l_131016_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS
    LIST_INIT(&l_131013_dgn);
    LIST_INIT(&l_131013_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS_2
    LIST_INIT(&l_130723_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS
    LIST_INIT(&l_131012_dgn);
    LIST_INIT(&l_131014_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_2
    LIST_INIT(&l_130966_dgn);
    LIST_INIT(&l_130965_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_3
    LIST_INIT(&l_130764_dgn);
    LIST_INIT(&l_130763_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_4
    LIST_INIT(&l_130751_dgn);
    LIST_INIT(&l_130750_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_EQUALIZATION_CONFIGURATION_STATUS
    LIST_INIT(&l_130968_dgn);
    LIST_INIT(&l_130967_dgn);
#endif

#ifdef RVC_CONFIG_INTERF_CHARGER_ACFAULT_CONFIG_STATUS_1
    LIST_INIT(&l_130953_dgn);
    LIST_INIT(&l_130951_dgn);
#endif
#endif
}

#ifdef RVC_CONFIG_INVERTER_CHARGER
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
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_1
    {&l_131031_dgn, RVCINVERTAC0, "RVCINVERTAC0", true},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_2
    {&l_131030_dgn, RVCINVERTACTWO0, "RVCINVERTACTWO0", true},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_3
    {&l_131029_dgn, RVCINVERTACTHREE0, "RVCINVERTACTHREE0", true},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_4
    {&l_130959_dgn, RVCINVERTACFOUR0, "RVCINVERTACFOUR0", true},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_DC_STATUS
    {&l_130792_dgn, RVCINVERTDC0, "RVCINVERTDC0", true},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_STATUS
    {&l_131028_dgn, RVCINVERT0, "RVCINVERT0", true},
    {&l_131027_dgn, RVCINVERT0, "RVCINVERT0", false},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_1
    {&l_131026_dgn, RVCINVERTCFG0, "RVCINVERTCFG0", true},
    {&l_131024_dgn, RVCINVERTCFG0, "RVCINVERTCFG0", false},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_2
    {&l_131025_dgn, RVCINVERTCFGTWO0, "RVCINVERTCFGTWO0", true},
    {&l_131023_dgn, RVCINVERTCFGTWO0, "RVCINVERTCFGTWO0", false},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_3
    {&l_130766_dgn, RVCINVERTCFGTHREE0, "RVCINVERTCFGTHREE0", true},
    {&l_130765_dgn, RVCINVERTCFGTHREE0, "RVCINVERTCFGTHREE0", false},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_4
    {&l_130715_dgn, RVCINVERTCFGFOUR0, "RVCINVERTCFGFOUR0", true},
    {&l_130714_dgn, RVCINVERTCFGFOUR0, "RVCINVERTCFGFOUR0", false},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS
    {&l_130749_dgn, RVCINVERTTEMP0, "RVCINVERTTEMP0", true},
#endif
#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS_2
    {&l_130507_dgn, RVCINVERTTEMPTWO0, "RVCINVERTTEMPTWO0", true},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_1
    {&l_131018_dgn, RVCCHRGAC0, "RVCCHRGAC0", true},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_3
    {&l_131016_dgn, RVCCHRGACTHREE0, "RVCCHRGACTHREE0", true},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS
    {&l_131015_dgn, RVCCHRG0, "RVCCHRG0", true},
    {&l_131013_dgn, RVCCHRG0, "RVCCHRG0", false},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS_2
    {&l_130723_dgn, RVCCHRGTWO0, "RVCCHRGTWO0", true},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS
    {&l_131014_dgn, RVCCHRGCFG0, "RVCCHRGCFG0", true},
    {&l_131012_dgn, RVCCHRGCFG0, "RVCCHRGCFG0", false},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_2
    {&l_130966_dgn, RVCCHRGCFGTWO0, "RVCCHRGCFGTWO0", true},
    {&l_130965_dgn, RVCCHRGCFGTWO0, "RVCCHRGCFGTWO0", false},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_3
    {&l_130764_dgn, RVCCHRGCFGTHREE0, "RVCCHRGCFGTHREE0", true},
    {&l_130763_dgn, RVCCHRGCFGTHREE0, "RVCCHRGCFGTHREE0", false},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_4
    {&l_130751_dgn, RVCCHRGCFGFOUR0, "RVCCHRGCFGFOUR0 status", true},
    {&l_130750_dgn, RVCCHRGCFGFOUR0, "RVCCHRGCFGFOUR0 status", false},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_EQUALIZATION_CONFIGURATION_STATUS
    {&l_130968_dgn, RVCCHRGEQCFG0, "RVCCHRGEQCFG0", true},
    {&l_130967_dgn, RVCCHRGEQCFG0, "RVCCHRGEQCFG0", false},
#endif
#ifdef RVC_CONFIG_INTERF_CHARGER_ACFAULT_CONFIG_STATUS_1
    {&l_130953_dgn, RVCCHRGACFAULTCFG0, "RVCCHRGACFAULTCFG0", true},
    {&l_130951_dgn, RVCCHRGACFAULTCFG0, "RVCCHRGACFAULTCFG0", false},
#endif
};
#endif

/**
 * @brief Remove all inverter charger nodes associated with a specific source address
 * @param sa Source address of the device
 */
void remove_inverter_charger_nodes(uint8_t sa)
{
#ifdef RVC_CONFIG_INVERTER_CHARGER
    DgnNode_t *dgn_node = NULL;

    LOG(D, "Removing inverter charger nodes for SA=%02X", sa);

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

    LOG(D, "Completed removing inverter charger nodes for SA=%02X", sa);
#endif
}

#ifdef RVC_CONFIG_INVERTER_CHARGER
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

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_1
/**
 * @brief Inverter AC Status 1 DGN received (131031)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131031Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131031 zDGN;
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
    RVCDGN_DGN_131031_Extract(&zDGN, p_data);

    if (zDGN.u4Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u4Instance);
        return false;
    }

    if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4Instance;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131031_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTAC0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131031));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131031_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u4Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Line;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0LINE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InputOutput;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0IO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RMSVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RMSVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RMSCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RMSCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16Frequency != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16Frequency;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0FREQ;
        convert_rvc_to_ddm_rvc_params_gain(rvc_ddmp_parameter, value, RVC_STD_UINT16, UINT_HZ_GAIN);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultOpenGround != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultOpenGround;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0OPENGROUND;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultOpenNeutral != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultOpenNeutral;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0OPENNEUTRAL;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultReversePolarity != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultReversePolarity;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0REVPOL;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultGroundCurrent != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultGroundCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTAC0GROUNDCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTAC0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTAC0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTAC0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131031 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_131031_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131031_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTAC0INST:
            if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTAC0LINE:
            if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Line;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTAC0IO:
            if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2InputOutput;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTAC0VOLT:
            if (zDGN.u16RMSVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RMSVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCINVERTAC0CURR:
            if (zDGN.u16RMSCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RMSCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCINVERTAC0FREQ:
            if (zDGN.u16Frequency != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16Frequency;
                value = &val;
                convert_rvc_to_ddm_rvc_params_gain(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT16, UINT_HZ_GAIN);
            }
            break;
        case RVCINVERTAC0OPENGROUND:
            if (zDGN.u2FaultOpenGround != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultOpenGround;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTAC0OPENNEUTRAL:
            if (zDGN.u2FaultOpenNeutral != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultOpenNeutral;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTAC0REVPOL:
            if (zDGN.u2FaultReversePolarity != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultReversePolarity;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTAC0GROUNDCURR:
            if (zDGN.u2FaultGroundCurrent != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultGroundCurrent;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_2
/**
 * @brief Inverter AC Status 2 DGN received (131030)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131030Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131030 zDGN;
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
    RVCDGN_DGN_131030_Extract(&zDGN, p_data);

    if (zDGN.u4Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u4Instance);
        return false;
    }

    if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4Instance;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTWO0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131030_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTACTWO0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131030));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131030_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u4Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Line;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTWO0LINE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InputOutput;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTWO0IO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16PeakVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16PeakVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTWO0PVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16PeakCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16PeakCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTWO0PCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16GroundCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16GroundCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTWO0GCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8Capacity != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Capacity;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTWO0CAP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTACTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTACTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTACTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131030 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_131030_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131030_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTACTWO0INST:
            if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTACTWO0LINE:
            if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Line;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACTWO0IO:
            if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2InputOutput;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACTWO0PVOLT:
            if (zDGN.u16PeakVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16PeakVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCINVERTACTWO0PCURR:
            if (zDGN.u16PeakCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16PeakCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCINVERTACTWO0GCURR:
            if (zDGN.u16GroundCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16GroundCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCINVERTACTWO0CAP:
            if (zDGN.u8Capacity != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Capacity;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT8);
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

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_3
/**
 * @brief Inverter AC Status 3 DGN received (131029)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131029Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131029 zDGN;
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
    RVCDGN_DGN_131029_Extract(&zDGN, p_data);

    if (zDGN.u4Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u4Instance);
        return false;
    }
    if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4Instance;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131029_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTACTHREE0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131029));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131029_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u4Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Line;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0LINE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InputOutput;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0IO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Waveform != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Waveform;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0WAVEFORM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4PhaseStatus != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4PhaseStatus;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0PHASE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RealPower != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RealPower;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0REALPOW;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_W_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ReactivePower != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ReactivePower;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0REACTPOW;
        convert_rvc_to_ddm_system_value(rvc_ddmp_parameter, (int32_t *)value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8HarmonicDistortion != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8HarmonicDistortion;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0HARMDIST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_PART_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ComplementaryLeg != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ComplementaryLeg;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACTHREE0COMPLLEG;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTACTHREE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTACTHREE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTACTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131029 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_131029_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131029_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTACTHREE0INST:
            if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTACTHREE0LINE:
            if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Line;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACTHREE0IO:
            if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2InputOutput;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACTHREE0WAVEFORM:
            if (zDGN.u2Waveform != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Waveform;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACTHREE0PHASE:
            if (zDGN.u4PhaseStatus != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4PhaseStatus;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACTHREE0REALPOW:
            if (zDGN.u16RealPower != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RealPower;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_W_UINT16);
            }
            break;
        case RVCINVERTACTHREE0REACTPOW:
            if (zDGN.u16ReactivePower != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16ReactivePower;
                value = &val;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, (int32_t *)value);
            }
            break;
        case RVCINVERTACTHREE0HARMDIST:
            if (zDGN.u8HarmonicDistortion != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8HarmonicDistortion;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_PART_UINT8);
            }
            break;
        case RVCINVERTACTHREE0COMPLLEG:
            if (zDGN.u8ComplementaryLeg != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8ComplementaryLeg;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_AC_STATUS_4
/**
 * @brief Inverter AC Status 2 DGN received (130959)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130959Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130959 zDGN;
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
    RVCDGN_DGN_130959_Extract(&zDGN, p_data);

    if (zDGN.u4Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u4Instance);
        return false;
    }

    if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4Instance;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130959_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTACFOUR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130959));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130959_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u4Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Line;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0LINE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InputOutput;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0IO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8VoltageFault != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8VoltageFault;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0VOLTFAULT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultSurgeProtection != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultSurgeProtection;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0SURGEFAULT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultHighFrequency != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultHighFrequency;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0HFREQFAULT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultLowFrequency != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultLowFrequency;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0LFREQFAULT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2BypassModeActive != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2BypassModeActive;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0BYPASS;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4QualificationStatus != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4QualificationStatus;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTACFOUR0QUAL;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTACFOUR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTACFOUR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTACFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130959 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130959_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130959_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTACFOUR0INST:
            if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTACFOUR0LINE:
            if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Line;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACFOUR0IO:
            if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2InputOutput;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACFOUR0VOLTFAULT:
            if (zDGN.u8VoltageFault != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8VoltageFault;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACFOUR0SURGEFAULT:
            if (zDGN.u2FaultSurgeProtection != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultSurgeProtection;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACFOUR0HFREQFAULT:
            if (zDGN.u2FaultHighFrequency != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultHighFrequency;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACFOUR0LFREQFAULT:
            if (zDGN.u2FaultLowFrequency != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultLowFrequency;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACFOUR0BYPASS:
            if (zDGN.u2BypassModeActive != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2BypassModeActive;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTACFOUR0QUAL:
            if (zDGN.u4QualificationStatus != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4QualificationStatus;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_DC_STATUS
/**
 * @brief Inverter DC Status DGN received (130792)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130792Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130792 zDGN;
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
    RVCDGN_DGN_130792_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCINVERTDC0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130792_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTDC0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130792));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130792_dgn);
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
    if (zDGN.u16DCVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTDC0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16DCAmperage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCAmperage;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTDC0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTDC0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTDC0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTDC0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130792 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130792_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130792_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTDC0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTDC0VOLT:
            if (zDGN.u16DCVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16DCVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCINVERTDC0CURR:
            if (zDGN.u16DCAmperage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16DCAmperage;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_STATUS
/**
 * @brief Prepare a Inverter Command frame（131027）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131027Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_131027_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_131027_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Inverter Status DGN received (131028)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131028Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131028 zDGN;
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
    RVCDGN_DGN_131028_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCINVERT0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131028_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERT0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131028));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131028_dgn);
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
    if (zDGN.u8Status != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Status;
        value = &val;
        rvc_ddmp_parameter = RVCINVERT0STATUS;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2BatteryTemperatureSensorPresent != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2BatteryTemperatureSensorPresent;
        value = &val;
        rvc_ddmp_parameter = RVCINVERT0BATTEMP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2LoadSenseEnabled != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2LoadSenseEnabled;
        value = &val;
        rvc_ddmp_parameter = RVCINVERT0LOADSENSE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InverterEnabled != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InverterEnabled;
        value = &val;
        rvc_ddmp_parameter = RVCINVERT0ENABLE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2PassThroughEnable != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2PassThroughEnable;
        value = &val;
        rvc_ddmp_parameter = RVCINVERT0PASSTHR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2GeneratorSupportEnabled != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2GeneratorSupportEnabled;
        value = &val;
        rvc_ddmp_parameter = RVCINVERT0GENERATOR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERT0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERT0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERT0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_131028 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_131028_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_131028_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for INVERTER_STATUS", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERT0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERT0STATUS:
            if (zDGN.u8Status != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Status;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERT0BATTEMP:
            if (zDGN.u2BatteryTemperatureSensorPresent != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2BatteryTemperatureSensorPresent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERT0LOADSENSE:
            if (zDGN.u2LoadSenseEnabled != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2LoadSenseEnabled;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERT0ENABLE:
            if (zDGN.u2InverterEnabled != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2InverterEnabled;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERT0PASSTHR:
            if (zDGN.u2PassThroughEnable != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2PassThroughEnable;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERT0GENERATOR:
            if (zDGN.u2GeneratorSupportEnabled != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2GeneratorSupportEnabled;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_1
/**
 * @brief Prepare a Inverter Configuration Command 1 frame（131024）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131024Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_131024_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_131024_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Inverter Configuration Status 1 DGN received (131026)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131026Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131026 zDGN;
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
    RVCDGN_DGN_131026_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCINVERTCFG0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131026_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTCFG0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131026));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131026_dgn);
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
    if (zDGN.u16LoadSensePowerThreshold != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16LoadSensePowerThreshold;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFG0LOADSENSEPOWTH;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_W_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16LoadSenseInterval != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16LoadSenseInterval;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFG0LOADSENSEINT;
        convert_rvc_to_ddm_rvc_params_gain(rvc_ddmp_parameter, value, RVC_STD_UINT16, UINT_S_GAIN);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16DCSourceShutdownVoltageMinimum != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCSourceShutdownVoltageMinimum;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFG0SHUTDOWNVOLTMIN;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InverterEnableOnStartup != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InverterEnableOnStartup;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFG0ENABLESTART;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2LoadSenseEnableOnStartup != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2LoadSenseEnableOnStartup;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFG0LOADSENSESTART;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2ACPassThroughEnableOnStartup != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2ACPassThroughEnableOnStartup;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFG0PASSTHRSTART;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_131026 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_131026_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_131026_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for INVERTER_CONFIGURATION_STATUS_1", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTCFG0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTCFG0LOADSENSEPOWTH:
            if (zDGN.u16LoadSensePowerThreshold != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16LoadSensePowerThreshold;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_W_UINT16);
            }
            break;
        case RVCINVERTCFG0LOADSENSEINT:
            if (zDGN.u16LoadSenseInterval != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16LoadSenseInterval;
                convert_rvc_to_ddm_rvc_params_gain(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16, UINT_S_GAIN);
            }
            break;
        case RVCINVERTCFG0SHUTDOWNVOLTMIN:
            if (zDGN.u16DCSourceShutdownVoltageMinimum != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16DCSourceShutdownVoltageMinimum;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCINVERTCFG0ENABLESTART:
            if (zDGN.u2InverterEnableOnStartup != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2InverterEnableOnStartup;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTCFG0LOADSENSESTART:
            if (zDGN.u2LoadSenseEnableOnStartup != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2LoadSenseEnableOnStartup;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTCFG0PASSTHRSTART:
            if (zDGN.u2ACPassThroughEnableOnStartup != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2ACPassThroughEnableOnStartup;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_2
/**
 * @brief Prepare a Inverter Configuration Command 2 frame (131023)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131023Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_131023_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_131023_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Inverter Configuration Status 2 DGN received (131025)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131025Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131025 zDGN;
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
    RVCDGN_DGN_131025_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCINVERTCFGTWO0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131025_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTCFGTWO0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131025));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131025_dgn);
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
    if (zDGN.u16DCSourceShutdownVoltageMaximum != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCSourceShutdownVoltageMaximum;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGTWO0SHUTDOWNVOLTMAX;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16DCSourceWarningVoltageMinimum != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCSourceWarningVoltageMinimum;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGTWO0WARNVOLTMIN;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16DCSourceWarningVoltageMaximum != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCSourceWarningVoltageMaximum;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGTWO0WARNVOLTMAX;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTCFGTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTCFGTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_131025 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_131025_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_131025_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for INVERTER_CONFIGURATION_STATUS_2", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTCFGTWO0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTCFGTWO0SHUTDOWNVOLTMAX:
            if (zDGN.u16DCSourceShutdownVoltageMaximum != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16DCSourceShutdownVoltageMaximum;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCINVERTCFGTWO0WARNVOLTMIN:
            if (zDGN.u16DCSourceWarningVoltageMinimum != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16DCSourceWarningVoltageMinimum;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCINVERTCFGTWO0WARNVOLTMAX:
            if (zDGN.u16DCSourceWarningVoltageMaximum != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16DCSourceWarningVoltageMaximum;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_3
/**
 * @brief Prepare a Inverter Configuration Command 3 frame (130765)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130765Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130765_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130765_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Inverter Configuration Status 3 DGN received (130766)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130766Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130766 zDGN;
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
    RVCDGN_DGN_130766_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCINVERTCFGTHREE0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130766_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTCFGTHREE0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130766));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130766_dgn);
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
    if (zDGN.u16DCSourceShutdownDelay != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCSourceShutdownDelay;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGTHREE0SHUTDOWNDELAY;
        convert_rvc_to_ddm_rvc_params_gain(rvc_ddmp_parameter, value, RVC_STD_UINT16, UINT_S_GAIN);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8StackMode != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8StackMode;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGTHREE0STACKMODE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16DCSourceShutdownRecoveryLevel != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16DCSourceShutdownRecoveryLevel;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGTHREE0SHUTDOWNRECLVL;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16GeneratorSupportEngageCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16GeneratorSupportEngageCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGTHREE0GENCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTCFGTHREE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTCFGTHREE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTCFGTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130766 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130766_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130766_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for INVERTER_CONFIGURATION_STATUS_3", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTCFGTWO0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTCFGTHREE0SHUTDOWNDELAY:
            if (zDGN.u16DCSourceShutdownDelay != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16DCSourceShutdownDelay;
                convert_rvc_to_ddm_rvc_params_gain(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16, UINT_S_GAIN);
            }
            break;
        case RVCINVERTCFGTHREE0STACKMODE:
            if (zDGN.u8StackMode != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8StackMode;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCINVERTCFGTHREE0SHUTDOWNRECLVL:
            if (zDGN.u16DCSourceShutdownRecoveryLevel != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16DCSourceShutdownRecoveryLevel;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCINVERTCFGTHREE0GENCURR:
            if (zDGN.u16GeneratorSupportEngageCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16GeneratorSupportEngageCurrent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AACDC_UINT16);
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

#ifdef RVC_CONFIG_INTERF_INVERTER_CONFIGURATION_STATUS_4
/**
 * @brief Prepare a Inverter Configuration Command 4 frame (130714)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130714Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130714_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130714_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Inverter Configuration Status 4 DGN received (130715)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130715Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130715 zDGN;
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
    RVCDGN_DGN_130715_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCINVERTCFGFOUR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130715_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTCFGFOUR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130715));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130715_dgn);
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
    if (zDGN.u16OutputACVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16OutputACVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGFOUR0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8OutputFrequency != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8OutputFrequency;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGFOUR0FREQ;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_HZ_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ACOutputPowerLimit != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ACOutputPowerLimit;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGFOUR0POWLIMIT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_W_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ACOutputTimeLimit != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ACOutputTimeLimit;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTCFGFOUR0TIMELIMIT;
        convert_rvc_to_ddm_rvc_params_gain(rvc_ddmp_parameter, value, RVC_STD_UINT16, UINT_S_GAIN);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTCFGFOUR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTCFGFOUR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTCFGFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130715 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130715_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130715_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for INVERTER_CONFIGURATION_STATUS_4", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTCFGFOUR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTCFGFOUR0VOLT:
            if (zDGN.u16OutputACVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16OutputACVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCINVERTCFGFOUR0FREQ:
            if (zDGN.u8OutputFrequency != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8OutputFrequency;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_HZ_UINT8);
            }
            break;
        case RVCINVERTCFGFOUR0POWLIMIT:
            if (zDGN.u16ACOutputPowerLimit != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16ACOutputPowerLimit;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_W_UINT16);
            }
            break;
        case RVCINVERTCFGFOUR0TIMELIMIT:
            if (zDGN.u16ACOutputTimeLimit != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16ACOutputTimeLimit;
                convert_rvc_to_ddm_rvc_params_gain(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16, UINT_S_GAIN);
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

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS
/**
 * @brief Inverter Temperature Status DGN received (130749)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130749Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130749 zDGN;
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
    RVCDGN_DGN_130749_Extract(&zDGN, p_data);

    if (zDGN.u8Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u8Instance);
        return false;
    }

    if (zDGN.u8Instance != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8Instance;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTTEMP0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130749_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTTEMP0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130749));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130749_dgn);
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
    if (zDGN.u16FET1Temperature != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16FET1Temperature;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTTEMP0FETONE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16TransformerTemperature != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16TransformerTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTTEMP0TRANSF;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16FET2Temperature != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16FET2Temperature;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTTEMP0FETTWO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTTEMP0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTTEMP0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTTEMP0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130749 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130749_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130749_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTTEMP0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTTEMP0FETONE:
            if (zDGN.u16FET1Temperature != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16FET1Temperature;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_DEGC_UINT16);
            }
            break;
        case RVCINVERTTEMP0TRANSF:
            if (zDGN.u16TransformerTemperature != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16TransformerTemperature;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_DEGC_UINT16);
            }
            break;
        case RVCINVERTTEMP0FETTWO:
            if (zDGN.u16FET2Temperature != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16FET2Temperature;
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

#ifdef RVC_CONFIG_INTERF_INVERTER_TEMPERATURE_STATUS_2
/**
 * @brief Inverter Temperature Status 2 DGN received (130507)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130507Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130507 zDGN;
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
    RVCDGN_DGN_130507_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCINVERTTEMPTWO0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130507_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCINVERTTEMPTWO0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130507));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130507_dgn);
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
    if (zDGN.u16ControlPowerBoardTemperature != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ControlPowerBoardTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTTEMPTWO0PB;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16CapacitorTemperature != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16CapacitorTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTTEMPTWO0CAPACITOR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16AmbientTemperature != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16AmbientTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCINVERTTEMPTWO0AMBIENT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCINVERTTEMPTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCINVERTTEMPTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCINVERTTEMPTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130507 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130507_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130507_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCINVERTTEMPTWO0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCINVERTTEMPTWO0PB:
            if (zDGN.u16ControlPowerBoardTemperature != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16ControlPowerBoardTemperature;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_DEGC_UINT16);
            }
            break;
        case RVCINVERTTEMPTWO0CAPACITOR:
            if (zDGN.u16CapacitorTemperature != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16CapacitorTemperature;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_DEGC_UINT16);
            }
            break;
        case RVCINVERTTEMPTWO0AMBIENT:
            if (zDGN.u16AmbientTemperature != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16AmbientTemperature;
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

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_1
/**
 * @brief Charger AC Status 1 DGN received (131018)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131018Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131018 zDGN;
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
    RVCDGN_DGN_131018_Extract(&zDGN, p_data);

    if (zDGN.u4Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u4Instance);
        return false;
    }
    if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4Instance;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131018_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGAC0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131018));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131018_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u4Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Line;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0LINE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InputOutput;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0IO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RMSVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RMSVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RMSCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RMSCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16Frequency != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16Frequency;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0FREQ;
        convert_rvc_to_ddm_rvc_params_gain(rvc_ddmp_parameter, value, RVC_STD_UINT16, UINT_HZ_GAIN);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultOpenGround != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultOpenGround;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0OPENGROUND;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultOpenNeutral != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultOpenNeutral;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0OPENNEUTRAL;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultReversePolarity != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultReversePolarity;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0REVPOL;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2FaultGroundCurrent != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2FaultGroundCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGAC0GROUNDCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGAC0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGAC0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGAC0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131018 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_131018_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131018_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGAC0INST:
            if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGAC0LINE:
            if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Line;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGAC0IO:
            if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2InputOutput;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGAC0VOLT:
            if (zDGN.u16RMSVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RMSVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCCHRGAC0CURR:
            if (zDGN.u16RMSCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RMSCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCCHRGAC0FREQ:
            if (zDGN.u16Frequency != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16Frequency;
                value = &val;
                convert_rvc_to_ddm_rvc_params_gain(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT16, UINT_HZ_GAIN);
            }
            break;
        case RVCCHRGAC0OPENGROUND:
            if (zDGN.u2FaultOpenGround != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultOpenGround;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGAC0OPENNEUTRAL:
            if (zDGN.u2FaultOpenNeutral != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultOpenNeutral;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGAC0REVPOL:
            if (zDGN.u2FaultReversePolarity != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultReversePolarity;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGAC0GROUNDCURR:
            if (zDGN.u2FaultGroundCurrent != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2FaultGroundCurrent;
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

#ifdef RVC_CONFIG_INTERF_CHARGER_AC_STATUS_3
/**
 * @brief Charger AC Status 3 DGN received (131016)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131016Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131016 zDGN;
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
    RVCDGN_DGN_131016_Extract(&zDGN, p_data);

    if (zDGN.u4Instance == 0)
    {
        LOG(E, "Invalid Instance %d", zDGN.u4Instance);
        return false;
    }
    if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4Instance;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131016_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGACTHREE0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(*(uint8_t *)value, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131016));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131016_dgn);
            if (prod_node->ddm_instance != -1)
            {
                uint32_t inst = class | DDM2_PARAMETER_INSTANCE(ddm_instance);
                update_prop_field(prod_node->ddm_instance, inst, zDGN.u4Instance);
            }
        }
        else
        {
            ddm_instance = dgn_node->ddm_instance;
        }
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Line;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0LINE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2InputOutput != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2InputOutput;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0IO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2Waveform != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2Waveform;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0WAVEFORM;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4PhaseStatus != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4PhaseStatus;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0PHASE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RealPower != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RealPower;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0REALPOW;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_W_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ReactivePower != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ReactivePower;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0REACTPOW;
        convert_rvc_to_ddm_system_value(rvc_ddmp_parameter, (int32_t *)value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8HarmonicDistortion != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8HarmonicDistortion;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0HARMDIST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_PART_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ComplementaryLeg != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ComplementaryLeg;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACTHREE0COMPLLEG;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGACTHREE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGACTHREE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGACTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_131016 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_131016_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_131016_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGACTHREE0INST:
            if (zDGN.u4Instance != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGACTHREE0LINE:
            if (zDGN.u2Line != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Line;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGACTHREE0IO:
            if (zDGN.u2InputOutput != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2InputOutput;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGACTHREE0WAVEFORM:
            if (zDGN.u2Waveform != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u2Waveform;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGACTHREE0PHASE:
            if (zDGN.u4PhaseStatus != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u4PhaseStatus;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGACTHREE0REALPOW:
            if (zDGN.u16RealPower != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16RealPower;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_W_UINT16);
            }
            break;
        case RVCCHRGACTHREE0REACTPOW:
            if (zDGN.u16ReactivePower != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16ReactivePower;
                value = &val;
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, (int32_t *)value);
            }
            break;
        case RVCCHRGACTHREE0HARMDIST:
            if (zDGN.u8HarmonicDistortion != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8HarmonicDistortion;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_PART_UINT8);
            }
            break;
        case RVCCHRGACTHREE0COMPLLEG:
            if (zDGN.u8ComplementaryLeg != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8ComplementaryLeg;
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

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS
/**
 * @brief Prepare a Charger Command frame（131013）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131013Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_131013_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_131013_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Charger Status DGN received (131015)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131015Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131015 zDGN;
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
    RVCDGN_DGN_131015_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRG0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131015_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRG0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131015));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131015_dgn);
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
        rvc_ddmp_parameter = RVCCHRG0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ChargeCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ChargeCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCCHRG0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ChargeCurrentPercentOfMaximum != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ChargeCurrentPercentOfMaximum;
        value = &val;
        rvc_ddmp_parameter = RVCCHRG0CURRMAX;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_PART_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8OperatingState != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8OperatingState;
        value = &val;
        rvc_ddmp_parameter = RVCCHRG0OPERST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2DefaultStateOnPowerUp != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2DefaultStateOnPowerUp;
        value = &val;
        rvc_ddmp_parameter = RVCCHRG0DEFST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2AutoRechargeEnable != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2AutoRechargeEnable;
        value = &val;
        rvc_ddmp_parameter = RVCCHRG0AUTO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4ForceCharge != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4ForceCharge;
        value = &val;
        rvc_ddmp_parameter = RVCCHRG0FORCE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_131015 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_131015_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_131015_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for CHARGER_STATUS", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRG0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRG0VOLT:
            if (zDGN.u16ChargeVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16ChargeVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCCHRG0CURR:
            if (zDGN.u16ChargeCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16ChargeCurrent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AACDC_UINT16);
            }
            break;
        case RVCCHRG0CURRMAX:
            if (zDGN.u8ChargeCurrentPercentOfMaximum != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ChargeCurrentPercentOfMaximum;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_PART_UINT8);
            }
            break;
        case RVCCHRG0OPERST:
            if (zDGN.u8OperatingState != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8OperatingState;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRG0DEFST:
            if (zDGN.u2DefaultStateOnPowerUp != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2DefaultStateOnPowerUp;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRG0AUTO:
            if (zDGN.u2AutoRechargeEnable != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2AutoRechargeEnable;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRG0FORCE:
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

#ifdef RVC_CONFIG_INTERF_CHARGER_STATUS_2
/**
 * @brief Charger Status 2 DGN received (130723)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130723Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130723 zDGN;
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
    RVCDGN_DGN_130723_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRGTWO0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130723_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGTWO0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130723));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130723_dgn);
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
        rvc_ddmp_parameter = RVCCHRGTWO0PRIO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ChargingVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ChargingVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGTWO0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16ChargingCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16ChargingCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGTWO0CURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ChargerTemperature != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ChargerTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGTWO0TEMP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    void *value = NULL;
    int32_t val;
    DgnNode_t *dgn_node;
    RVCDGN_zDGN_130723 zDGN;
    bool is_publish = false;

    dgn_node = DgnNodeFindByDdmInstance(&l_130723_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
    if (!dgn_node)
    {
        return;
    }
    RVCDGN_DGN_130723_Extract(&zDGN, dgn_node->dgn_data);

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGTWO0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8Instance;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGTWO0PRIO:
            if (zDGN.u8ChargerPriority != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8ChargerPriority;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGTWO0VOLT:
            if (zDGN.u16ChargingVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16ChargingVoltage;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_VACDC_UINT16);
            }
            break;
        case RVCCHRGTWO0CURR:
            if (zDGN.u16ChargingCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u16ChargingCurrent;
                value = &val;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, value, RVC_AACDC_UINT16);
            }
            break;
        case RVCCHRGTWO0TEMP:
            if (zDGN.u8ChargerTemperature != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                val = zDGN.u8ChargerTemperature;
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

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS
/**
 * @brief Prepare a Charger Configuration Command frame（131012）
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131012Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_131012_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_131012_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Charger Configuration Status DGN received (131014)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131014Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_131014 zDGN;
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
    RVCDGN_DGN_131014_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRGCFG0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_131014_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGCFG0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_131014));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_131014_dgn);
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
        rvc_ddmp_parameter = RVCCHRGCFG0ALGO;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ChargerMode != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ChargerMode;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFG0MODE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2BatterySensorPresent != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2BatterySensorPresent;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFG0BATSENSOR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2ChargerInstallationLine != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2ChargerInstallationLine;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFG0LINE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u4BatteryType != NMEA2K_UINT4_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u4BatteryType;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFG0TYPE;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16BatteryBankSize != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BatteryBankSize;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFG0BANK;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AMPHOUR_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16MaximumChargingCurrent != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16MaximumChargingCurrent;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFG0MAXCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_131014 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter);
        dgn_node = DgnNodeFindByDdmInstance(&l_131014_dgn, instance);
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_131014_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for CHARGER_CONFIGURATION_STATUS", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGCFG0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGCFG0ALGO:
            if (zDGN.u8ChargingAlgorithm != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ChargingAlgorithm;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGCFG0MODE:
            if (zDGN.u8ChargerMode != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ChargerMode;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGCFG0BATSENSOR:
            if (zDGN.u2BatterySensorPresent != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2BatterySensorPresent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGCFG0LINE:
            if (zDGN.u2ChargerInstallationLine != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2ChargerInstallationLine;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGCFG0TYPE:
            if (zDGN.u4BatteryType != NMEA2K_UINT4_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u4BatteryType;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGCFG0BANK:
            if (zDGN.u16BatteryBankSize != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BatteryBankSize;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AMPHOUR_UINT16);
            }
            break;
        case RVCCHRGCFG0MAXCURR:
            if (zDGN.u16MaximumChargingCurrent != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16MaximumChargingCurrent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AACDC_UINT16);
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

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_2
/**
 * @brief Prepare a Charger Configuration Command 2 frame (130965)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130965Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130965_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130965_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Charger Configuration Status 2 DGN received (130966)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130966Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130966 zDGN;
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
    RVCDGN_DGN_130966_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRGCFGTWO0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130966_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGCFGTWO0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130966));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130966_dgn);
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
    if (zDGN.u8MaximumChargeCurrentAsPercent != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8MaximumChargeCurrentAsPercent;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTWO0MAXCURR;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_PART_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ChargeRateLimitAsPercentOfBankSize != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ChargeRateLimitAsPercentOfBankSize;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTWO0RATELIMIT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_PART_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ShoreBreakerSize != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ShoreBreakerSize;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTWO0BREAKER;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_AACDC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8DefaultBatteryTemperature != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8DefaultBatteryTemperature;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTWO0BATTEMP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_DEGC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16RechargeVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16RechargeVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTWO0RECHARGEVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGCFGTWO0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGCFGTWO0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGCFGTWO0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130966 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130966_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130966_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for CHARGER_CONFIGURATION_STATUS_2", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGCFGTWO0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGCFGTWO0MAXCURR:
            if (zDGN.u8MaximumChargeCurrentAsPercent != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8MaximumChargeCurrentAsPercent;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_PART_UINT8);
            }
            break;
        case RVCCHRGCFGTWO0RATELIMIT:
            if (zDGN.u8ChargeRateLimitAsPercentOfBankSize != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ChargeRateLimitAsPercentOfBankSize;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_PART_UINT8);
            }
            break;
        case RVCCHRGCFGTWO0BREAKER:
            if (zDGN.u8ShoreBreakerSize != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ShoreBreakerSize;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_AACDC_UINT8);
            }
            break;
        case RVCCHRGCFGTWO0BATTEMP:
            if (zDGN.u8DefaultBatteryTemperature != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8DefaultBatteryTemperature;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_DEGC_UINT8);
            }
            break;
        case RVCCHRGCFGTWO0RECHARGEVOLT:
            if (zDGN.u16RechargeVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16RechargeVoltage;
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

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_3
/**
 * @brief Prepare a Charger Configuration Command 3 frame (130763)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130763Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130763_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130763_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Charger Configuration Status 3 DGN received (130764)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130764Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130764 zDGN;
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
    RVCDGN_DGN_130764_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRGCFGTHREE0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130764_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGCFGTHREE0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130764));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130764_dgn);
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
    if (zDGN.u16BulkVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BulkVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTHREE0BVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16AbsorptionVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16AbsorptionVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTHREE0AVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16FloatVoltage != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16FloatVoltage;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTHREE0FVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8TemperatureCompensationConstant != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8TemperatureCompensationConstant;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGTHREE0TEMPCOMP;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGCFGTHREE0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGCFGTHREE0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGCFGTHREE0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130764 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130764_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130764_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for CHARGER_CONFIGURATION_STATUS_3", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGCFGTHREE0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGCFGTHREE0BVOLT:
            if (zDGN.u16BulkVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BulkVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCCHRGCFGTHREE0AVOLT:
            if (zDGN.u16AbsorptionVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16AbsorptionVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCCHRGCFGTHREE0FVOLT:
            if (zDGN.u16FloatVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16FloatVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCCHRGCFGTHREE0TEMPCOMP:
            if (zDGN.u8TemperatureCompensationConstant != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8TemperatureCompensationConstant;
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

#ifdef RVC_CONFIG_INTERF_CHARGER_CONFIGURATION_STATUS_4
/**
 * @brief Prepare a Charger Configuration Command 4 frame (130750)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130750Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130750_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130750_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Charger Configuration Status 4 DGN received (130751)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130751Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130751 zDGN;
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
    RVCDGN_DGN_130751_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRGCFGFOUR0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130751_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGCFGFOUR0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130751));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130751_dgn);
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
    if (zDGN.u16BulkTime != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16BulkTime;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGFOUR0BTIME;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16AbsorptionTime != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16AbsorptionTime;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGFOUR0ATIME;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16FloatTime != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16FloatTime;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGCFGFOUR0FTIME;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGCFGFOUR0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGCFGFOUR0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGCFGFOUR0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130751 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130751_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130751_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for CHARGER_CONFIGURATION_STATUS_4", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGCFGFOUR0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGCFGFOUR0BTIME:
            if (zDGN.u16BulkTime != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16BulkTime;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16);
            }
            break;
        case RVCCHRGCFGFOUR0ATIME:
            if (zDGN.u16AbsorptionTime != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16AbsorptionTime;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16);
            }
            break;
        case RVCCHRGCFGFOUR0FTIME:
            if (zDGN.u16FloatTime != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16FloatTime;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16);
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

#ifdef RVC_CONFIG_INTERF_CHARGER_EQUALIZATION_CONFIGURATION_STATUS
/**
 * @brief Prepare a Charger Equalization Configuration Command frame (1309687)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130967Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130967_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130967_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Charger Equalization Configuration Status DGN received (130968)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130968Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130968 zDGN;
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
    RVCDGN_DGN_130968_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRGEQCFG0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130968_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGEQCFG0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130968));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130968_dgn);
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
        rvc_ddmp_parameter = RVCCHRGEQCFG0VOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u16EqualizationTime != NMEA2K_UINT16_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u16EqualizationTime;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGEQCFG0TIME;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT16);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGEQCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGEQCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGEQCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130968 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130968_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130968_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for CHARGER_EQUALIZATION_CONFIGURATION_STATUS", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGEQCFG0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGEQCFG0VOLT:
            if (zDGN.u16EqualizationVoltage != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16EqualizationVoltage;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT16);
            }
            break;
        case RVCCHRGEQCFG0TIME:
            if (zDGN.u16EqualizationTime != NMEA2K_UINT16_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u16EqualizationTime;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT16);
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

#ifdef RVC_CONFIG_INTERF_CHARGER_ACFAULT_CONFIG_STATUS_1
/**
 * @brief Prepare a Charger ACFAULT Configuration Command 1 frame (130951)
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit130951Dgn(uint8_t instance, uint8_t *p_data)
{
    DgnNode_t *dgn_node = NULL;
    dgn_node = DgnNodeFindByDdmInstance(&l_130951_dgn, instance);
    if (dgn_node != NULL)
    {
        RVCDGN_DGN_130951_Stuff(p_data, dgn_node->dgn_data);
        return true;
    }

    return false;
}

/**
 * @brief Charger AC FAULT Configuration Status 1 DGN received (130953)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive130953Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    RVCDGN_zDGN_130953 zDGN;
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
    RVCDGN_DGN_130953_Extract(&zDGN, p_data);

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
        rvc_ddmp_parameter = RVCCHRGACFAULTCFG0INST;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_INST_UINT8);
        dgn_node = DgnNodeFindByRVCInstance(&l_130953_dgn, *(uint8_t *)value);
        if (dgn_node == NULL)
        {
            uint32_t class;
            class = RVCCHRGACFAULTCFG0;
            ddm_instance = broker_register_instance(&class, l_connector_id);
            if (ddm_instance == -1)
            {
                LOG(E, "Registration failed for class %08x", class);
                return false;
            }
            dgn_node = DgnNodeCreate(zDGN.u8Instance, ddm_instance, sa, p_data, sizeof(RVCDGN_zDGN_130953));
            if (dgn_node == NULL)
            {
                LOG(E, "DgnNode with DDM instance %d cannot be allocated for class %08x", ddm_instance, class);
                return false;
            }
            DgnNodeInsert(dgn_node, &l_130953_dgn);
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
    if (zDGN.u8ExtremeLowVoltageLevel != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ExtremeLowVoltageLevel;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACFAULTCFG0EXTLVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8LowVoltageLevel != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8LowVoltageLevel;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACFAULTCFG0LVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8HighVoltageLevel != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8HighVoltageLevel;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACFAULTCFG0HVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8ExtremeHighVoltageLevel != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8ExtremeHighVoltageLevel;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACFAULTCFG0EXTHVOLT;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_VACDC_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u8QualificationTime != NMEA2K_UINT8_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u8QualificationTime;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACFAULTCFG0QUALTIME;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (zDGN.u2BypassMode != NMEA2K_UINT2_NO_DATA)
    {
        updated_data = true;
        val = zDGN.u2BypassMode;
        value = &val;
        rvc_ddmp_parameter = RVCCHRGACFAULTCFG0BYPASS;
        convert_rvc_to_ddm_rvc_params(rvc_ddmp_parameter, value, RVC_STD_UINT8);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_ddmp_parameter | DDM2_PARAMETER_INSTANCE(ddm_instance), value, sizeof(int32_t), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    if (true == updated_data)
    {
        DgnNodeUpdate(dgn_node, p_data);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCCHRGACFAULTCFG0SYNC | DDM2_PARAMETER_INSTANCE(ddm_instance), &One, sizeof(One), l_connector_id, (TickType_t)portMAX_DELAY);
    }

    return true;
}

/**
 * @brief RVCCHRGACFAULTCFG0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCCHRGACFAULTCFG0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    int32_t value;
    DgnNode_t *dgn_node = NULL;
    uint8_t instance = 0;
    RVCDGN_zDGN_130953 zDGN;
    bool is_publish = false;

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        dgn_node = DgnNodeFindByDdmInstance(&l_130953_dgn, DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.subscribe.parameter));
        if (dgn_node != NULL)
        {
            RVCDGN_DGN_130953_Extract(&zDGN, dgn_node->dgn_data);
        }
        else
        {
            LOG(E, "DgnNode with DDM instance %d not found for CHARGER_ACFAULT_CONFIG_STATUS_1", instance);
            return;
        }
    }

    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCCHRGACFAULTCFG0INST:
            if (zDGN.u8Instance != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8Instance;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_INST_UINT8);
            }
            break;
        case RVCCHRGACFAULTCFG0EXTLVOLT:
            if (zDGN.u8ExtremeLowVoltageLevel != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ExtremeLowVoltageLevel;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT8);
            }
            break;
        case RVCCHRGACFAULTCFG0LVOLT:
            if (zDGN.u8LowVoltageLevel != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8LowVoltageLevel;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT8);
            }
            break;
        case RVCCHRGACFAULTCFG0HVOLT:
            if (zDGN.u8HighVoltageLevel != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8HighVoltageLevel;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT8);
            }
            break;
        case RVCCHRGACFAULTCFG0EXTHVOLT:
            if (zDGN.u8ExtremeHighVoltageLevel != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8ExtremeHighVoltageLevel;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_VACDC_UINT8);
            }
            break;
        case RVCCHRGACFAULTCFG0QUALTIME:
            if (zDGN.u8QualificationTime != NMEA2K_UINT8_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u8QualificationTime;
                convert_rvc_to_ddm_rvc_params(p_frame->frame.subscribe.parameter, &value, RVC_STD_UINT8);
            }
            break;
        case RVCCHRGACFAULTCFG0BYPASS:
            if (zDGN.u2BypassMode != NMEA2K_UINT2_NO_DATA)
            {
                is_publish = true;
                value = zDGN.u2BypassMode;
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
