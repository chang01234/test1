/**
 * \file        ic_mgmt_ddm_class_desc.c
 * \date        2025-12-10
 * \author      Kire Janev (kire.janev@dometic.com)
 * \brief       Class descriptor functions that are used for managing inverter charger and product DDM2 instances.
 *
 * \li          2025-12-10 (KJ) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#include "ic_mgmt_ddm_class_desc.h"

ddm_class_desc_t *ddm_class_desc_create(uint32_t parameter_class)
{
    ddm_class_desc_t *node = NULL;
    ddm_params_items_t *node_data = NULL;

    SORTED_LIST_VALUE_TYPE value;
    SORTED_LIST_RETURN_VALUE result = sorted_list_unique_get(&value, (SORTED_LIST *)&Parameter_list_index_table, parameter_class, 0);

    if (result == SORTED_LIST_OK)
    {
        TRUE_CHECK_RETURN0((node = hal_mem_malloc_prefer(sizeof(ddm_class_desc_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);

        unsigned int class_size = value & 0xffff;

        TRUE_CHECK_RETURN0((node_data = (ddm_params_items_t *)hal_mem_malloc_prefer(class_size * sizeof(ddm_params_items_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);

        node->params_items = node_data;
    }

    return node;
}

ddm_class_desc_t *ddm_class_desc_prod_create(void)
{
    ddm_class_desc_t *node = NULL;
    ddm_params_items_t *node_data = NULL;

    TRUE_CHECK_RETURN0((node = hal_mem_malloc_prefer(sizeof(ddm_class_desc_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);

    unsigned int class_size = PROD_CLASS_DESC_ITEMS_SIZE;  // PROD<X> has two items (PROD0PROP and PROD0CLIST)

    TRUE_CHECK_RETURN0((node_data = (ddm_params_items_t *)hal_mem_malloc_prefer(class_size * sizeof(ddm_params_items_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);

    node->params_items = node_data;

    return node;
}

void ddm_class_desc_init(uint8_t ddm_instance, uint32_t parameter_class, ddm_class_desc_t *node)
{
    SORTED_LIST_VALUE_TYPE value;
    SORTED_LIST_RETURN_VALUE result = sorted_list_unique_get(&value, (SORTED_LIST *)&Parameter_list_index_table, parameter_class, 0);

    if (result == SORTED_LIST_OK)
    {
        unsigned int class_size = value & 0xffff;

        for (unsigned int i = 0; i < class_size; i++)
        {
            node->params_items[i].ddm2_param = (parameter_class | i) | DDM2_PARAMETER_INSTANCE(ddm_instance);
        }

        node->ddm_instance = ddm_instance;
        node->ddm_class_desc_size = class_size;
        node->list_node.le_next = NULL;
        node->list_node.le_prev = NULL;
    }
}

void ddm_class_desc_prod_init(uint8_t ddm_instance, ddm_class_desc_t *node)
{
    node->params_items[PROD_CLASS_DESC_PROP_PARAM_INDEX].ddm2_param = PROD0PROP | DDM2_PARAMETER_INSTANCE(ddm_instance);
    node->params_items[PROD_CLASS_DESC_CLIST_PARAM_INDEX].ddm2_param = PROD0CLIST | DDM2_PARAMETER_INSTANCE(ddm_instance);
    node->params_items[PROD_CLASS_DESC_MDL_PARAM_INDEX].ddm2_param = PROD0MDL | DDM2_PARAMETER_INSTANCE(ddm_instance);
    node->ddm_instance = ddm_instance;
    node->ddm_class_desc_size = PROD_CLASS_DESC_ITEMS_SIZE;
    node->list_node.le_next = NULL;
    node->list_node.le_prev = NULL;
}

void ddm_class_desc_delete(ddm_class_desc_t *ddm_class_desc)
{
    if (ddm_class_desc != NULL)
    {
        // Remove from a list
        LIST_REMOVE(ddm_class_desc, list_node);

        hal_mem_free(ddm_class_desc->params_items);
        hal_mem_free(ddm_class_desc);
    }
}

void ddm_class_desc_insert(ddm_class_desc_t *ddm_class_desc, list_t *list)
{
    LIST_INSERT_HEAD(list, ddm_class_desc, list_node);
}

void ddm_class_desc_update(ddm_class_desc_t *ddm_class_desc, uint32_t ddm2_param, int32_t value)
{
    if (ddm_class_desc != NULL)
    {
        for (unsigned int i = 0; i < ddm_class_desc->ddm_class_desc_size; i++)
        {
            if (ddm2_param == ddm_class_desc->params_items[i].ddm2_param)
            {
                ddmw_set_i32(&ddm_class_desc->params_items[i].ddm2_item, value);
            }
        }
    }
}

ddm_class_desc_t *ddm_class_desc_find_by_ddm_instance(list_t *list, uint8_t ddm_instance)
{
    ddm_class_desc_t *current_node = NULL;

    LIST_FOREACH(current_node, list, list_node)
    {
        if (current_node->ddm_instance == ddm_instance)
        {
            break;
        }
    }
    return current_node;
}
