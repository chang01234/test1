/*
 * dgnnode.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdint.h>
#include <string.h>

#include "dgnnode.h"

#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hal_mem.h"

static SemaphoreHandle_t list_mutex;  // Global mutex

void DgnNodeInit(void)
{
    TRUE_CHECK((list_mutex = xSemaphoreCreateRecursiveMutex()) != NULL);
}

DgnNode_t *DgnNodeCreate(uint8_t rvc_instance, uint8_t ddm_instance, uint8_t source_address, const void *dgn_data, size_t dgn_data_size)
{
    DgnNode_t *node;
    void *node_data;

    node = hal_mem_malloc_prefer(sizeof(DgnNode_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (node == NULL)
    {
        return NULL;
    }
    node_data = hal_mem_malloc_prefer(dgn_data_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (node_data == NULL)
    {
        return NULL;
    }

    // Copy data to DgnNode_t
    memcpy(node_data, dgn_data, dgn_data_size);

    // fill in data
    node->source_address = source_address;
    node->rvc_instance = rvc_instance;
    node->ddm_instance = ddm_instance;
    node->dgn_data = node_data;
    node->dgn_data_size = dgn_data_size;
    node->list_node.le_next = NULL;
    node->list_node.le_prev = NULL;

    return node;
}

void DgnNodeDelete(DgnNode_t *dgn_node)
{
    // Remove from a list
    LIST_REMOVE(dgn_node, list_node);

    hal_mem_free(dgn_node->dgn_data);
    hal_mem_free(dgn_node);
}

void DgnNodeInsert(DgnNode_t *dgn_node, list_t *list)
{
    TRUE_CHECK(xSemaphoreTakeRecursive(list_mutex, portMAX_DELAY));
    LIST_INSERT_HEAD(list, dgn_node, list_node);
    TRUE_CHECK(xSemaphoreGiveRecursive(list_mutex));
}

void DgnNodeUpdate(DgnNode_t *dgn_node, const void *dgn_data)
{
    TRUE_CHECK(xSemaphoreTakeRecursive(list_mutex, portMAX_DELAY));
    if (dgn_node != NULL)
    {
        memmove(dgn_node->dgn_data, dgn_data, dgn_node->dgn_data_size);
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(list_mutex));
}

MAYBE_UNUSED DgnNode_t *DgnNodeFindByDdmInstance(list_t *list, uint8_t ddm_instance)
{
    DgnNode_t *ret_node = NULL;
    DgnNode_t *current_node;

    TRUE_CHECK(xSemaphoreTakeRecursive(list_mutex, portMAX_DELAY));
    LIST_FOREACH(current_node, list, list_node)
    {
        if (current_node->ddm_instance == ddm_instance)
        {
            ret_node = current_node;
            break;
        }
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(list_mutex));
    return ret_node;
}

DgnNode_t *DgnNodeFindBySourceAddress(list_t *list, uint8_t source_address)
{
    DgnNode_t *ret_node = NULL;
    DgnNode_t *current_node;
    TRUE_CHECK(xSemaphoreTakeRecursive(list_mutex, portMAX_DELAY));
    LIST_FOREACH(current_node, list, list_node)
    {
        if (current_node->source_address == source_address)
        {
            ret_node = current_node;
            break;
        }
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(list_mutex));
    return ret_node;
}

MAYBE_UNUSED DgnNode_t *DgnNodeFindByRVCInstance(list_t *list, uint8_t rvc_instance)
{
    DgnNode_t *ret_node = NULL;
    DgnNode_t *current_node;

    TRUE_CHECK(xSemaphoreTakeRecursive(list_mutex, portMAX_DELAY));
    LIST_FOREACH(current_node, list, list_node)
    {
        if (current_node->rvc_instance == rvc_instance)
        {
            ret_node = current_node;
            break;
        }
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(list_mutex));
    return ret_node;
}

void DgnNodeLoopAndExecute(list_t *list, DgnNode_exefcn_t exe_func, void *arg)
{
    if (exe_func == NULL)
    {
        LOG(E, "Called with exe_func = NULL");
        return;
    }
    DgnNode_t *current_node, *tcurrent_node;

    TRUE_CHECK(xSemaphoreTakeRecursive(list_mutex, portMAX_DELAY));
    LIST_FOREACH_SAFE(current_node, list, list_node, tcurrent_node)
    {
        if (exe_func(current_node, arg))
        {
            break;
        }
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(list_mutex));
}
