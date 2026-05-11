/**
 * \file
 * \date        2021-10-20
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Sorted container
 *
 * Sorted container is a component that allows inserting a key and a data
 * structure pair into a memory pool. Once a data structure is inserted into
 * the sorted container, it can be efficiently searched and retrieved.
 *
 * \li          2021-10-08  (NR) Initial implementation
 * \li          2022-07-07  (NR) Added Dometic style typedef, sorted_container__capacity
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

#ifndef SORTED_CONTAINER_H_
#define SORTED_CONTAINER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pool_allocator.h"
#include <stdint.h>

#if __SIZEOF_POINTER__ == 8
#include "sorted_list64.h"
typedef SORTED_LIST64 LOOKUP_LIST;
#define DECLARE_LOOKUP_LIST        DECLARE_SORTED_LIST64
#define DECLARE_LOOKUP_LIST_EXTRAM DECLARE_SORTED_LIST64_EXTRAM
#else
#include "sorted_list.h"
typedef SORTED_LIST LOOKUP_LIST;
#define DECLARE_LOOKUP_LIST        DECLARE_SORTED_LIST
#define DECLARE_LOOKUP_LIST_EXTRAM DECLARE_SORTED_LIST_EXTRAM
#endif

/**
 * \brief		Declare and define a sorted container (private file scope).
 *
 * This macro will statically allocate the sorted list entries, data and sorted
 * container structure, where the \a name structure will have `extern` linkage.
 *
 * \param		name Name of allocated DDM store structure.
 * \param		size Maximum number of entries in the DDM store.
 * \param       type Type of data that needs to be stored.
 */
#define SORTED_CONTAINER__DECLARE(name, size, type)   \
    POOL_ALLOCATOR__DECLARE(name##_pool, size, type); \
    DECLARE_LOOKUP_LIST(name##_sorted_list, size);    \
    static struct sorted_container name = {           \
        .p__sorted_list = &name##_sorted_list,        \
        .p__pool = &name##_pool,                      \
    }

/**
 * \brief		Declare and define a sorted container (accessible outside file
 *              scope).
 *
 * This macro will statically allocate the sorted list entries, data and sorted
 * container structure, where the \a name structure will have `extern` linkage.
 *
 * \param		name Name of allocated DDM store structure.
 * \param		size Maximum number of entries in the DDM store.
 * \param       type Type of data that needs to be stored.
 */
#define SORTED_CONTAINER__DECLARE_PUBLIC(name, size, type) \
    POOL_ALLOCATOR__DECLARE(name##_pool, size, type);      \
    DECLARE_LOOKUP_LIST(name##_sorted_list, size);         \
    struct sorted_container name = {                       \
        .p__sorted_list = &name##_sorted_list,             \
        .p__pool = &name##_pool,                           \
    }

/**
 * \brief		Declare and define a sorted container (private file scope) in
 *              EXTRAM.
 *
 * This macro will statically allocate the sorted list entries, data and sorted
 * container structure, where the \a name structure will have `extern` linkage.
 * The sorted list entries and sorted container data is allocated in EXTRAM.
 *
 * \param		name Name of allocated DDM store structure.
 * \param		size Maximum number of entries in the DDM store.
 * \param       type Type of data that needs to be stored.
 */
#define SORTED_CONTAINER__DECLARE_EXTRAM(name, size, type)   \
    POOL_ALLOCATOR__DECLARE_EXTRAM(name##_pool, size, type); \
    DECLARE_LOOKUP_LIST_EXTRAM(name##_sorted_list, size);    \
    static struct sorted_container name = {                  \
        .p__sorted_list = &name##_sorted_list,               \
        .p__pool = &name##_pool,                             \
    }

/**
 * \brief		Declare and define a sorted container (accessible outside file
 *              scope) in EXTRAM.
 *
 * This macro will statically allocate the sorted list entries, data and sorted
 * container structure, where the \a name structure will have `extern` linkage.
 * The sorted list entries and sorted container data is allocated in EXTRAM.
 *
 * \param		name Name of allocated DDM store structure.
 * \param		size Maximum number of entries in the DDM store.
 * \param       type Type of data that needs to be stored.
 */
#define SORTED_CONTAINER__DECLARE_EXTRAM_PUBLIC(name, size, type) \
    POOL_ALLOCATOR__DECLARE_EXTRAM(name##_pool, size, type);      \
    DECLARE_LOOKUP_LIST_EXTRAM(name##_sorted_list, size);         \
    struct sorted_container name = {                              \
        .p__sorted_list = &name##_sorted_list,                    \
        .p__pool = &name##_pool,                                  \
    }

/**
 * \brief       Sorted container handle structure
 *
 * This structure binds together memory pool allocator and sorted list. Is is
 * not intended to be allocated by the user. This structure is allocated
 * automatically by macros defined in this header.
 *
 * \note        All members of this structure are private to sorted_container
 *              component.
 */
typedef struct sorted_container
{
    LOOKUP_LIST *p__sorted_list;
    POOL_ALLOCATOR *p__pool;
} SORTED_CONTAINER;

/**
 * \brief       Initialize statically allocated sorted container.
 *
 * \param       sc Pointer to sorted container handle structure that will be initialized.
 * \param       sorted_list Pointer to sorted list instance which will be used by sorted
 *              container. The sorted list instance must be already initialized before
 *              call to this function.
 * \param       pool Pointer to pool allocator instance which will be used by sorted
 *              container. The pool allocator instance must be already initialized before
 *              call to this function.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer to allocated sorted container
 *              structure.
 * \pre         Parameter \a sorted_list must be a non-NULL pointer.
 * \pre         Parameter \a pool must be a non-NULL pointer.
 */
void sorted_container__init(
    SORTED_CONTAINER *sc,
    LOOKUP_LIST *sorted_list,
    POOL_ALLOCATOR *pool);

/**
 * \brief       Terminate a previosly initialized, statically allocated sorted container.
 *
 * This function will terminate sorted container. The termination will put the sorted
 * container in invalid state so it can not be used after termination.
 *
 * \param       sc Pointer to sorted container handle structure that will be terminated.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer to initialized sorted container
 *              structure.
 */
void sorted_container__terminate(SORTED_CONTAINER *sc);

/**
 * \brief       Create dynamically allocated sorted container.
 *
 * \param       element_size is the size of elements (that are stored in sorted container)
 *              in bytes.
 * \param       nbr_of_elements is the capacity of sorted container.
 * \pre         Parameter \a element_size must be a non zero value.
 * \pre         Parameter \a nbr_of_elements must be at least one.
 *
 * \return      Pointer to a new memory space containing sorted container initialized instance.
 * \retval      NULL no available memory is available in the heap memory.
 */
SORTED_CONTAINER *sorted_container__create(size_t element_size, size_t nbr_of_elements);

/**
 * \brief       Destroy dynamically allocated sorted container instance.
 *
 *              This function will terminate and then free the memory used by sorted
 *              container.
 *
 * \param       sc Pointer to sorted container handle structure that will be destroyed.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer to the previosly initialized
 *              sorted container instance.
 */
void sorted_container__destroy(SORTED_CONTAINER *sc);

/**
 * \brief       Allocate a new memory space for a container.
 *
 * The function will return pointer to newly allocated containers. In case that
 * the given key was used before, it will return pointer to existing container.
 *
 * All allocations will return container which is zeroed out.
 *
 * \param       sc Pointer to sorted container handle structure.
 * \param       key A key which is to be associated with allocated memory.
 *
 * \return      Pointer to a new memory space for the container.
 * \retval      NULL no available memory slot is available in the pool.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer.
 *
 */
void *sorted_container__new(SORTED_CONTAINER *sc, uint32_t key);

/**
 * \brief       Access a previosly inserted container
 *
 * This function will return pointer to container which was previosly inserted
 * with \ref sorted_container__new funciton.
 *
 * \param       sc Pointer to sorted container handle structure.
 * \param       key A key which is to be associated with container.
 *
 * \return      Pointer to the container.
 * \retval      NULL the element with the given \a key was not inserted in
 *              sorted container.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer.
 */
void *sorted_container__access(SORTED_CONTAINER *sc, uint32_t key);

/**
 * \brief       Delete a container from sorted containers.
 *
 * Before deletion of the container its contents will be zeroed out.
 *
 * A key can be deleted multiple times, when a key is not found the function
 * will just exit.
 *
 * \param       sc Pointer to initialized sorted container handle structure.
 * \param       key A key which is associated with allocated memory.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer.
 */
void sorted_container__delete(SORTED_CONTAINER *sc, uint32_t key);

/**
 * \brief       Delete all containers in sorted containers instance.
 *
 * \param       sc Pointer to initialized sorted container handle structure.
 * \note        This function can be called multiple times over same sorted container instance.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer.
 */
void sorted_container__delete_all(struct sorted_container *sc);

/**
 * \brief       Return how many containers are allocated in sorted container.
 *
 * \param       sc Pointer to sorted container handle structure.
 *
 * \return      The number of allocated containers.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer.
 */
size_t sorted_container__occupied(const SORTED_CONTAINER *sc);

/**
 * \brief       Iterate over inserted containers.
 *
 * \param       sc Pointer to sorted container handle structure.
 * \param       index Iterator index, valid values are from zero (0) up to
 *              occupied value - 1. The occupied value may be fetched by
 *              \ref sorted_container__occupied function.
 * \param[out]  data Pointer to void * pointer. This pointer will be set to
 *              point to indexed container.
 * \param[out]  key Pointer to uint32_t variable which will be filled after the
 *              call. This variable will hold value of \a key which is
 *              associated to the container pointed by \a data.
 *
 * \pre         Parameter \a sc must be a non-NULL pointer.
 * \pre         Parameter \a data must be a non-NULL pointer.
 * \pre         Parameter \a key must be a non-NULL pointer.
 *
 * \note        The function will fail if \a index is equal to or greate than
 *              sorted container occupied attribute; in this case \a data is
 *              set to NULL and \a key is set to zero (0).
 *
 * \code        Example of iterating over sorted container
 *
 * for (size_t i = 0; i < sorted_container__occupied(&my_container); i++)
 * {
 *     void * data;
 *     uint32_t key;
 *
 *     sorted_container__iterate(&my_container, i, &data, &key);
 *
 *     // Use data pointer and key in your code
 * }
 * \endcode
 */
void sorted_container__iterate(
    const SORTED_CONTAINER *sc,
    uint32_t index,
    void **data,
    uint32_t *key);

/**
 * \brief       Returns capacity of the sorted container in number of elements
 *
 * \param       sc Pointer to sorted container handle structure.
 *
 * \return      Number of elements in the sorted container.
 *
 * \pre			Parameter \a sc must be a non-NULL pointer.
 */
size_t sorted_container__capacity(const SORTED_CONTAINER *sc);

#ifdef __cplusplus
}
#endif

#endif /* SORTED_CONTAINER_H_ */
