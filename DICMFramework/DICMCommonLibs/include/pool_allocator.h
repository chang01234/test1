/**
 * \file
 * \date        2021-10-26
 * \author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \authors     (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       Pool allocator
 *
 * Pool allocator allow to have a very quick memory allocator. Each allocation
 * is of fixed size. The size is defined at the moment of creation of the
 * allocator.
 *
 * \li          2021-10-26  (NR) Initial implementation
 * \li          2022-07-07  (NR) Added Dometic style typedef, pool_allocator__capacity
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
#ifndef POOL_ALLOCATOR_H_
#define POOL_ALLOCATOR_H_

#if __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * \brief       Pool allocator single linked list (used only internally).
 */
struct pool_allocator__list
{
    struct pool_allocator__list *next;
};

/**
 * \brief       Pool allocator handle structure.
 *
 * This structure is allocated automatically by macros defined in this header.
 *
 * \note        All members of this structure are private to pool_allocator
 *              component.
 */
typedef struct pool_allocator
{
    struct pool_allocator__list p__elements;
    void *p__element_storage;
    size_t p__element_size;
    size_t p__nbr_of_elements;
} POOL_ALLOCATOR;

/**
 * \brief       Calculate the storage array size in bytes.
 *
 * This macro is to be used when allocating array of bytes which will be used
 * as pool allocator storage.
 *
 * \param       name Name of the structure used internally to calculate the
 *                   size in bytes.
 * \param       nbr_of_elements Specify how many elements should be allocated
 *                              in pool.
 * \param       element_type Specify the type of element which is to be used in
 *                           allocation.
 *
 * \return      Size of array in bytes.
 */

#define POOL_ALLOCATOR__SIZE_IN_BYTES(name, nbr_of_elements, element_type) \
    sizeof(element_type[nbr_of_elements])

/**
 * \brief       Calculate size of one element that is to be used in allocator
 *              array.
 *
 * \param       name Name of the structure used internally to calculate the
 *                   size in bytes.
 * \param       element_type the type of element which is to be used in
 *                           allocation.
 *
 * \return      Size of element in bytes.
 */
#define POOL_ALLOCATOR__ELEMENT_SIZE_IN_BYTES(name, element_type) \
    POOL_ALLOCATOR__SIZE_IN_BYTES(name, 1, element_type)

#define POOL_ALLOCATOR__DECLARE(name, nbr_of_elements, element_type) \
    static uint8_t name##_elements[POOL_ALLOCATOR__SIZE_IN_BYTES(    \
        name,                                                        \
        nbr_of_elements,                                             \
        element_type)];                                              \
    static struct pool_allocator name = {                            \
        .p__elements = {                                             \
            .next = NULL,                                            \
        },                                                           \
        .p__element_storage = name##_elements,                       \
        .p__element_size = sizeof(element_type),                     \
        .p__nbr_of_elements = nbr_of_elements}

#define POOL_ALLOCATOR__DECLARE_PUBLIC(name, nbr_of_elements, element_type) \
    static uint8_t name##_elements[POOL_ALLOCATOR__SIZE_IN_BYTES(           \
        name,                                                               \
        nbr_of_elements,                                                    \
        element_type)];                                                     \
    static struct pool_allocator name = {                                   \
        .p__elements = {                                                    \
            .next = NULL,                                                   \
        },                                                                  \
        .p__element_storage = name##_elements,                              \
        .p__element_size = sizeof(element_type),                            \
        .p__nbr_of_elements = nbr_of_elements}

#define POOL_ALLOCATOR__DECLARE_EXTRAM(name, nbr_of_elements, element_type)    \
    static EXT_RAM_ATTR uint8_t name##_elements[POOL_ALLOCATOR__SIZE_IN_BYTES( \
        name,                                                                  \
        nbr_of_elements,                                                       \
        element_type)];                                                        \
    static struct pool_allocator name = {                                      \
        .p__elements = {                                                       \
            .next = NULL,                                                      \
        },                                                                     \
        .p__element_storage = name##_elements,                                 \
        .p__element_size = sizeof(element_type),                               \
        .p__nbr_of_elements = nbr_of_elements}

#define POOL_ALLOCATOR__DECLARE_EXTRAM_PUBLIC(name, nbr_of_elements, element_type) \
    static EXT_RAM_ATTR uint8_t name##_elements[POOL_ALLOCATOR__SIZE_IN_BYTES(     \
        name,                                                                      \
        nbr_of_elements,                                                           \
        element_type)];                                                            \
    static struct pool_allocator name = {                                          \
        .p__elements = {                                                           \
            .next = NULL,                                                          \
        },                                                                         \
        .p__element_storage = name##_elements,                                     \
        .p__element_size = sizeof(element_type),                                   \
        .p__nbr_of_elements = nbr_of_elements}

/**
 * \brief       Initialize a statically allocated pool allocator.
 *
 * This function is used to initialize a pool allocator manually. It is not
 * needed when a pool is allocated statically using the macros defined in this
 * header.
 *
 * \param       pa Pointer to \ref pool_allocator handle structure that will
 *              be initialized.
 * \param       elements Pointer to array used to allocate space for pool.
 * \param       element_size Size of one element in bytes.
 * \param       nbr_of_elements Number of elements in array. See \ref ELEMENTS
 *              macro for usage.
 *
 * \pre			Parameter \a pa must be a non-NULL pointer.
 * \pre			Parameter \a elements must be a non-NULL pointer.
 */
void pool_allocator__init(POOL_ALLOCATOR *pa,
                          uint8_t *elements,
                          size_t element_size,
                          size_t nbr_of_elements);

/**
 * \brief       Terminate previosly initialized statically allocated pool allocator.
 *
 * \param       pa Pointer to \ref pool_allocator handle structure that will
 *              be terminated.
 */
void pool_allocator__terminate(POOL_ALLOCATOR *pa);

/**
 * \brief       Create dynamically allocated pool allocator.
 *
 * \param       element_size is the size of elements (that are stored in pool allocator)
 *              in bytes.
 * \param       nbr_of_elements is the capacity of pool allocator.
 * \pre         Parameter \a element_size must be a non zero value.
 * \pre         Parameter \a nbr_of_elements must be at least one.
 *
 * \return      Pointer to a new memory space containing pool allocator initialized instance.
 * \retval      NULL no available memory is available in the heap memory.
 */
POOL_ALLOCATOR *pool_allocator__create(size_t element_size, size_t nbr_of_elements);

/**
 * \brief       Destroy dynamically allocated pool allocator instance.
 *
 *              This function will terminate and then free the memory used by pool
 *              allocator.
 *
 * \param       pa Pointer to pool allocator instance that will be destroyed.
 *
 * \pre         Parameter \a pa must be a non-NULL pointer to the previosly initialized
 *              pool allocator instance.
 */
void pool_allocator__destroy(POOL_ALLOCATOR *pa);

/**
 * \brief       Allocate memory from the pool.
 *
 * If this function is called for the first time, it will build the memory pool
 * list of free blocks. After this initialization step, it will return the first
 * available free element. On next call, the function will detect that element
 * pool is already initialized and skip the initialization.
 *
 * \param       pa Pointer to \ref pool_allocator handle structure.
 *
 * \return      Allocated element memory.
 * \retval      NULL no available memory slot is available in the pool.
 *
 * \pre			Parameter \a pa must be a non-NULL pointer.
 */
void *pool_allocator__new(POOL_ALLOCATOR *pa);

/**
 * \brief       Free memory and return to the pool.
 *
 * \param       pa Pointer to \ref pool_allocator handle structure.
 * \param       entry Pointer to previosly allocated memory.
 *
 * \pre			Parameter \a pa must be a non-NULL pointer.
 * \pre			Parameter \a entry must be a non-NULL pointer.
 *
 * \note        Each allocated entry must be returned to the same pool that it
 *              was allocated from. It is undefined behaviour if the element is
 *              returned to a different pool.
 */
void pool_allocator__delete(POOL_ALLOCATOR *pa, void *entry);

/**
 * \brief       Return how many bytes are occupied by a single entry (element).
 *
 * \param       pa Pointer to \ref pool_allocator handle structure.
 *
 * \return      Size of a single element of this pool in bytes.
 *
 * \pre			Parameter \a pa must be a non-NULL pointer.
 */
size_t pool_allocator__element_size(const POOL_ALLOCATOR *pa);

/**
 * \brief       Returns capacity of the pool in number of elements
 *
 * \param       pa Pointer to \ref pool_allocator handle structure.
 *
 * \return      Number of elements in pool.
 *
 * \pre			Parameter \a pa must be a non-NULL pointer.
 */
size_t pool_allocator__capacity(const POOL_ALLOCATOR *pa);

#if __cplusplus
}
#endif

#endif /* POOL_ALLOCATOR_H_ */
