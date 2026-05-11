/*
 * dgnnode.h
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#ifndef DGNNODE_H_
#define DGNNODE_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/queue.h>

typedef struct DgnNode
{
    uint8_t source_address;
    uint8_t rvc_instance;
    int ddm_instance;
    void *dgn_data;
    size_t dgn_data_size;
    LIST_ENTRY(DgnNode) list_node;
} DgnNode_t;

typedef LIST_HEAD(list_head, DgnNode) list_t;
typedef bool (*DgnNode_exefcn_t)(DgnNode_t *dgn_node, void *arg);
void DgnNodeInit(void);
DgnNode_t *DgnNodeCreate(uint8_t rvc_instance, uint8_t ddm_instance, uint8_t source_address, const void *dgn_data, size_t dgn_data_size);
void DgnNodeDelete(DgnNode_t *dgn_node);
void DgnNodeInsert(DgnNode_t *dgn_node, list_t *list);
void DgnNodeUpdate(DgnNode_t *dgn_node, const void *dgn_data);
DgnNode_t *DgnNodeFindByDdmInstance(list_t *list, uint8_t ddm_instance);
DgnNode_t *DgnNodeFindBySourceAddress(list_t *list, uint8_t source_address);
DgnNode_t *DgnNodeFindByRVCInstance(list_t *list, uint8_t rvc_instance);
void DgnNodeLoopAndExecute(list_t *list, DgnNode_exefcn_t exe_func, void *arg);
#endif /* DGNNODE_H_ */
