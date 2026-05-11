/**
 * \file
 * \date        2021-10-26
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Pool allocator implementation
 *
 * Implementation of pool allocator.
 *
 * \li          2021-10-08  (NR) Initial implementation
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
#include "pool_allocator.h"

/* Depends */
#include "hal_mem.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>

static void linked_list__init(struct pool_allocator__list *list)
{
    list->next = list;
}

static bool linked_list__is_empty(const struct pool_allocator__list *list)
{
    return !!(list->next == list);
}

static struct pool_allocator__list *linked_list__next(
    struct pool_allocator__list *node)
{
    return node->next;
}

static void linked_list__remove_from(
    struct pool_allocator__list *prev_node,
    struct pool_allocator__list *node)
{
    prev_node->next = node->next;
    node->next = NULL;
}

static void linked_list__add_after(
    struct pool_allocator__list *prev_node,
    struct pool_allocator__list *node)
{
    node->next = prev_node->next;
    prev_node->next = node;
}

static void build_list(struct pool_allocator *pa)
{
    uint8_t *elements = pa->p__element_storage;

    linked_list__init(&pa->p__elements);

    for (size_t i = 0u; i < pa->p__nbr_of_elements; i++)
    {
        struct pool_allocator__list *element_entry;

        element_entry = (struct pool_allocator__list *)(elements + (i * pa->p__element_size));
        linked_list__add_after(&pa->p__elements, element_entry);
    }
}

void pool_allocator__init(struct pool_allocator *pa,
                          uint8_t *elements,
                          size_t element_size,
                          size_t nbr_of_elements)
{
    /* This assertion ensures that each element is large enough to store a linked list node. */
    assert(element_size >= sizeof(struct pool_allocator__list));
    pa->p__elements.next = NULL;
    pa->p__element_storage = elements;
    pa->p__element_size = element_size;
    pa->p__nbr_of_elements = nbr_of_elements;
    memset(pa->p__element_storage, 0, element_size * nbr_of_elements);  // Needed for dynamically allocated pool allocator
    build_list(pa);
}

void pool_allocator__terminate(struct pool_allocator *pa)
{
    /* Set pointers to NULL, so when pool_allocator is terminated, if anyone tries
     * to use it past termination we will get the exception handler and catch this
     * inappropriate use.
     */
    pa->p__elements.next = NULL;
    pa->p__element_storage = NULL;
    pa->p__element_size = 0u;
    pa->p__nbr_of_elements = 0u;
}

struct pool_allocator *pool_allocator__create(size_t element_size, size_t nbr_of_elements)
{
    struct pool_allocator *pa;
    uint8_t *element_storage;

    /* Enforce minimum element size to prevent memory corruption.
     * The pool allocator uses a memory overlay technique where:
     * - The same memory area holds user data when allocated
     * - The same memory area stores list pointers when free
     * 
     * Without this minimum size enforcement, small element requests would cause
     * memory corruption when blocks are managed within the free list. The issue occurs because:
     * 
     * 1. p__element_storage is the shared memory region used for BOTH:
     *    - Storing 'user data' when blocks are allocated (calling pool_allocator__new())
     *    - Storing 'linked list pointers' when blocks are free (building the free list with build_list())
     * 
     * 2. When build_list() creates the free list, each element in p__element_storage
     *    is cast to struct pool_allocator__list to set up the list linkage
     * 
     * 3. If an element is smaller than sizeof(struct pool_allocator__list),
     *    operations like linked_list__add_after() would write beyond the allocated
     *    memory bounds, corrupting adjacent memory
     *
     * This ensures that each element can safely hold a linked list node when free:
     * 1. Even if users request small elements (e.g. element_size = sizeof(char) = 1 byte),
     *    we must ensure each element is at least sizeof(struct pool_allocator__list) bytes
     * 2. Architecture considerations compound this issue:
     *    - On 32-bit systems, pointers are 4 bytes, so the element size must be at least 4 bytes
     *      to hold the pointer to the next element in the free list.
     *    - On 64-bit systems, pointers are 8 bytes, so the element size must be at least 8 bytes
     *      to hold the pointer to the next element in the free list.
     */
    if (element_size < sizeof(struct pool_allocator__list))
    {
        element_size = sizeof(struct pool_allocator__list);
    }

    element_storage = hal_mem_malloc_prefer(element_size * nbr_of_elements, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (element_storage == NULL)
    {
        return NULL;
    }
    pa = hal_mem_malloc_prefer(sizeof(*pa), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (pa == NULL)
    {
        hal_mem_free(element_storage);
        return NULL;
    }
    pool_allocator__init(pa, element_storage, element_size, nbr_of_elements);
    return pa;
}

void pool_allocator__destroy(struct pool_allocator *pa)
{
    uint8_t *element_storage = pa->p__element_storage;

    pool_allocator__terminate(pa);
    hal_mem_free(pa);
    hal_mem_free(element_storage);
}

void *pool_allocator__new(struct pool_allocator *pa)
{
    void *entry;

    if (pa->p__elements.next == NULL)
    {
        build_list(pa);
    }
    if (!linked_list__is_empty(&pa->p__elements))
    {
        struct pool_allocator__list *current;

        current = linked_list__next(&pa->p__elements);
        linked_list__remove_from(&pa->p__elements, current);
        entry = current;
    }
    else
    {
        entry = NULL;
    }

    return entry;
}

void pool_allocator__delete(struct pool_allocator *pa, void *entry)
{
    // Check whether we have built the free list
    if (pa->p__elements.next != NULL)
    {
        struct pool_allocator__list *current = entry;
        linked_list__add_after(&pa->p__elements, current);
    }
}

size_t pool_allocator__element_size(const POOL_ALLOCATOR *pa)
{
    return pa->p__element_size;
}

size_t pool_allocator__capacity(const POOL_ALLOCATOR *pa)
{
    return pa->p__nbr_of_elements;
}
