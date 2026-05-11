/**
 * \file        batt_mgmt_ddm_class_desc.h
 * \date        2024-05-08
 * \author      Kire Janev (kire.janev@dometic.com)
 * \brief       Header file for class descriptor functions that are used for managing battery and product DDM2 instances.
 *
 * \li          2024-05-08 (KJ) Initial implementation
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

#ifndef BATT_MGMT_DDM_CLASS_DESC_H_
#define BATT_MGMT_DDM_CLASS_DESC_H_

#define PROD_CLASS_DESC_ITEMS_SIZE        3
#define PROD_CLASS_DESC_PROP_PARAM_INDEX  0
#define PROD_CLASS_DESC_CLIST_PARAM_INDEX 1
#define PROD_CLASS_DESC_MDL_PARAM_INDEX   2

#include "ddm2_parameter_list.h"
#include "ddm_wrapper.h"
#include "hal_mem.h"
#include "sorted_list.h"
#include <string.h>
#include <sys/queue.h>

typedef struct _ddm_params_items
{
    uint32_t ddm2_param;
    ddmw_item_t ddm2_item;
} ddm_params_items_t;

typedef struct _ddm_class_desc
{
    uint8_t ddm_instance;
    ddm_params_items_t *params_items;
    size_t ddm_class_desc_size;
    LIST_ENTRY(_ddm_class_desc) list_node;
} ddm_class_desc_t;

typedef LIST_HEAD(list_head, _ddm_class_desc) list_t;

ddm_class_desc_t *ddm_class_desc_create(uint32_t parameter_class);
ddm_class_desc_t *ddm_class_desc_prod_create(void);
void ddm_class_desc_init(uint8_t ddm_instance, uint32_t parameter_class, ddm_class_desc_t *node);
void ddm_class_desc_prod_init(uint8_t ddm_instance, ddm_class_desc_t *node);
void ddm_class_desc_delete(ddm_class_desc_t *ddm_class_desc);
void ddm_class_desc_insert(ddm_class_desc_t *ddm_class_desc, list_t *list);
void ddm_class_desc_update(ddm_class_desc_t *ddm_class_desc, uint32_t ddm2_param, int32_t value);
ddm_class_desc_t *ddm_class_desc_find_by_ddm_instance(list_t *list, uint8_t ddm_instance);

#endif /* BATT_MGMT_DDM_CLASS_DESC_H_ */
