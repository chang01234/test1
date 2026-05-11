/**
 * \file
 * \date        2021-10-08
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Sorted container implementation
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

#include "sorted_container.h"

#include <stdbool.h>
#include <string.h>

#include "hal_mem.h"

#if __SIZEOF_POINTER__ == 8
typedef SORTED_LIST64_ENTRY LOOKUP_LIST_ENTRY;
typedef SORTED_LIST64_KEY_TYPE LOOKUP_LIST_KEY_TYPE;
typedef SORTED_LIST64_VALUE_TYPE LOOKUP_LIST_VALUE_TYPE;
typedef SORTED_LIST64_RETURN_VALUE LOOKUP_LIST_RETURN_VALUE;
#define LOOKUP_LIST_UNIQUE_CREATE(count)                               sorted_list64_create(count)
#define LOOKUP_LIST_UNIQUE_DESTROY(sorted_list)                        sorted_list64_destroy(sorted_list)
#define LOOKUP_LIST_UNIQUE_UNIQUE_GET(value, sorted_list, key, remove) sorted_list64_unique_get(value, sorted_list, key, remove)
#define LOOKUP_LIST_UNIQUE_UNIQUE_ADD(sorted_list, key, value)         sorted_list64_unique_add(sorted_list, key, value)
#define LOOKUP_LIST_UNIQUE_REMOVE(sorted_list, key)                    sorted_list64_unique_remove(sorted_list, key)
#define LOOKUP_LIST_UNIQUE_CLEAR(sorted_list)                          sorted_list64_clear(sorted_list)
#define LOOKUP_LIST_OK                                                 SORTED_LIST64_OK
#else
typedef SORTED_LIST_ENTRY LOOKUP_LIST_ENTRY;
typedef SORTED_LIST_KEY_TYPE LOOKUP_LIST_KEY_TYPE;
typedef SORTED_LIST_VALUE_TYPE LOOKUP_LIST_VALUE_TYPE;
typedef SORTED_LIST_RETURN_VALUE LOOKUP_LIST_RETURN_VALUE;
#define LOOKUP_LIST_UNIQUE_CREATE(count)                               sorted_list_create(count)
#define LOOKUP_LIST_UNIQUE_DESTROY(sorted_list)                        sorted_list_destroy(sorted_list)
#define LOOKUP_LIST_UNIQUE_UNIQUE_GET(value, sorted_list, key, remove) sorted_list_unique_get(value, sorted_list, key, remove)
#define LOOKUP_LIST_UNIQUE_UNIQUE_ADD(sorted_list, key, value)         sorted_list_unique_add(sorted_list, key, value)
#define LOOKUP_LIST_UNIQUE_REMOVE(sorted_list, key)                    sorted_list_unique_remove(sorted_list, key)
#define LOOKUP_LIST_UNIQUE_CLEAR(sorted_list)                          sorted_list_clear(sorted_list)
#define LOOKUP_LIST_OK                                                 SORTED_LIST_OK
#endif

void sorted_container__init(
    struct sorted_container *sc,
    LOOKUP_LIST *sorted_list,
    POOL_ALLOCATOR *pool)
{
    sc->p__sorted_list = sorted_list;
    sc->p__pool = pool;
}

void sorted_container__terminate(struct sorted_container *sc)
{
    /* Set pointers to NULL, so when sorted_container is terminated, if anyone tries
     * to use it past termination we will get the exception handler and catch this
     * inappropriate use.
     */
    sc->p__sorted_list = NULL;
    sc->p__pool = NULL;
}

struct sorted_container *sorted_container__create(size_t element_size, size_t nbr_of_elements)
{
    POOL_ALLOCATOR *pool;
    LOOKUP_LIST *sorted_list;
    struct sorted_container *sc;

    pool = pool_allocator__create(element_size, nbr_of_elements);
    if (pool == NULL)
    {
        return NULL;
    }
    sorted_list = LOOKUP_LIST_UNIQUE_CREATE(nbr_of_elements);
    if (sorted_list == NULL)
    {
        pool_allocator__destroy(pool);
        return NULL;
    }
    sc = hal_mem_malloc_prefer(sizeof(*sc), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (sc == NULL)
    {
        LOOKUP_LIST_UNIQUE_DESTROY(sorted_list);
        pool_allocator__destroy(pool);
        return NULL;
    }
    sorted_container__init(sc, sorted_list, pool);
    return sc;
}

void sorted_container__destroy(struct sorted_container *sc)
{
    LOOKUP_LIST *sorted_list;
    POOL_ALLOCATOR *pool;

    sorted_list = sc->p__sorted_list;
    pool = sc->p__pool;
    sorted_container__terminate(sc);
    hal_mem_free(sc);
    LOOKUP_LIST_UNIQUE_DESTROY(sorted_list);
    pool_allocator__destroy(pool);
}

void *sorted_container__new(struct sorted_container *sc, uint32_t key)
{
    bool does_exist;
    void *slot;

    does_exist = LOOKUP_LIST_UNIQUE_UNIQUE_GET(
                     (LOOKUP_LIST_VALUE_TYPE *)&slot,
                     sc->p__sorted_list,
                     (LOOKUP_LIST_KEY_TYPE)key,
                     0) == LOOKUP_LIST_OK;
    /* Is this a new key entry or an existing one? */
    if (!does_exist)
    {
        int retval;
        slot = pool_allocator__new(sc->p__pool);

        if (slot == NULL)
        {
            return NULL;
        }
        retval = LOOKUP_LIST_UNIQUE_UNIQUE_ADD(sc->p__sorted_list,
                                               (LOOKUP_LIST_KEY_TYPE)key,
                                               (LOOKUP_LIST_VALUE_TYPE)slot);

        switch (retval)
        {
        case 0:
            pool_allocator__delete(sc->p__pool, slot);
            return NULL;
        case 1:
            /* Can't happen since we already did check if the entry exists */
            break;
        default:
            break;
        }
    }
    return slot;
}

void *sorted_container__access(struct sorted_container *sc, uint32_t key)
{
    void *slot;

    slot = NULL;
    /* At this point I don't care what this funciton will return since the
     * slot variable would be updated only when a key exists
     */
    (void)LOOKUP_LIST_UNIQUE_UNIQUE_GET(
        (LOOKUP_LIST_VALUE_TYPE *)&slot,
        sc->p__sorted_list,
        (LOOKUP_LIST_KEY_TYPE)key,
        0);
    return slot;
}

void sorted_container__delete(struct sorted_container *sc, uint32_t key)
{
    void *slot;

    slot = sorted_container__access(sc, key);
    if (slot != NULL)
    {
        LOOKUP_LIST_UNIQUE_REMOVE(sc->p__sorted_list, key);
        memset(slot, 0, pool_allocator__element_size(sc->p__pool));
        pool_allocator__delete(sc->p__pool, slot);
    }
}

void sorted_container__delete_all(struct sorted_container *sc)
{
    uint32_t occupied;

    occupied = sorted_container__occupied(sc);
    for (uint32_t i = 0; i < occupied; i++)
    {
        void *slot = (void *)sc->p__sorted_list->pdata[i].value;
        pool_allocator__delete(sc->p__pool, slot);
    }
    LOOKUP_LIST_UNIQUE_CLEAR(sc->p__sorted_list);
}

size_t sorted_container__occupied(const struct sorted_container *sc)
{
    return sc->p__sorted_list->entry_count;
}

void sorted_container__iterate(
    const struct sorted_container *sc,
    uint32_t index,
    void **data,
    uint32_t *key)
{
    if (index < sorted_container__occupied(sc))
    {
        *data = (void *)sc->p__sorted_list->pdata[index].value;
        *key = sc->p__sorted_list->pdata[index].key;
    }
    else
    {
        *data = NULL;
        *key = 0;
    }
}

size_t sorted_container__capacity(const SORTED_CONTAINER *sc)
{
    return pool_allocator__capacity(sc->p__pool);
}
