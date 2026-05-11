/**
 * \file        fifo_fixed.c
 * \date        2024-06-27
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Fixed size FIFO implementation.
 *
 * \li          2024-06-27  (NR) Initial implementation
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

/* Implements */
#include "fifo_fixed.h"

/* Depends */
#include <string.h>

static void delete_oldest_indexed(fifo_fixed_t * fifo)
{
    fifo->tail = (fifo->tail + 1) % fifo->max_items;
    fifo->free_items++;
    fifo->occupied_items--;
}

void fifo_fixed_init(fifo_fixed_t * fifo, void * items, size_t item_size, uint32_t max_items)
{
    fifo->items = items;
    fifo->head = 0;
    fifo->tail = 0;
    fifo->occupied_items = 0;
    fifo->free_items = max_items;
    fifo->max_items = max_items;
    fifo->item_size = item_size;
}

void fifo_fixed_deinit(fifo_fixed_t *fifo)
{
    fifo->item_size = 0;
    fifo->max_items = 0;
    fifo->free_items = 0;
    fifo->occupied_items = 0;
    fifo->tail = 0;
    fifo->head = 0;
    fifo->items = NULL;
}

void fifo_fixed_enqueue_back(fifo_fixed_t *fifo, const void *item)
{
    void * destination;

    if (fifo->free_items == 0)
    {
        delete_oldest_indexed(fifo);
    }
    destination = (char *)fifo->items + (fifo->head * fifo->item_size);
    fifo->head = (fifo->head + 1) % fifo->max_items;
    fifo->free_items--;
    fifo->occupied_items++;
    memcpy(destination, item, fifo->item_size);
}

void fifo_fixed_enqueue_front(fifo_fixed_t *fifo, const void *item)
{
    void * destination;

    if (fifo->free_items == 0)
    {
        delete_oldest_indexed(fifo);
    }
    fifo->tail = (fifo->tail - 1 + fifo->max_items) % fifo->max_items;
    fifo->free_items--;
    fifo->occupied_items++;
    destination = (char *)fifo->items + (fifo->tail * fifo->item_size);
    memcpy(destination, item, fifo->item_size);
}

void fifo_fixed_dequeue(fifo_fixed_t *fifo, void *item)
{
    void * source;

    source = (char *)fifo->items + (fifo->tail * fifo->item_size);
    memcpy(item, source, fifo->item_size);
    delete_oldest_indexed(fifo);
}

void fifo_fixed_delete_all(fifo_fixed_t *fifo)
{
    fifo->head = 0;
    fifo->tail = 0;
    fifo->occupied_items = 0;
    fifo->free_items = fifo->max_items;
}

extern inline size_t fifo_fixed_get_item_size(const fifo_fixed_t * fifo);

extern inline uint32_t fifo_fixed_get_occupied_items(const fifo_fixed_t * fifo);

extern inline bool fifo_fixed_is_empty(const fifo_fixed_t * fifo);

extern inline bool fifo_fixed_is_full(const fifo_fixed_t * fifo);

const void * fifo_fixed_peek(const fifo_fixed_t * fifo, uint32_t index)
{
    const void * source;

    uint32_t new_tail = (fifo->tail + index) % fifo->max_items;
    source = (char *)fifo->items + (new_tail * fifo->item_size);
    return source;
}
