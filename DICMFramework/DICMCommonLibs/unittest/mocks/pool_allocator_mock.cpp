/**
 * \file        pool_allocator_mock.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       pool_allocator mock implementation
 *
 * Implementation of pool_allocator mocks.
 *
 * \li          2024-09-12  (NR) Initial implementation
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
#include "pool_allocator_mock.hpp"

/* Depends */
#include <cassert>

Ipool_allocator * pool_allocator_mock_ptr;

void pool_allocator__init(POOL_ALLOCATOR * pa,
        uint8_t * elements,
        size_t element_size,
        size_t nbr_of_elements)
{
    assert(pool_allocator_mock_ptr);
    pool_allocator_mock_ptr->pool_allocator__init(pa, elements, element_size, nbr_of_elements);
}

void pool_allocator__terminate(POOL_ALLOCATOR * pa)
{
    assert(pool_allocator_mock_ptr);
    pool_allocator_mock_ptr->pool_allocator__terminate(pa);
}

POOL_ALLOCATOR * pool_allocator__create(size_t element_size, size_t nbr_of_elements)
{
    assert(pool_allocator_mock_ptr);
    return pool_allocator_mock_ptr->pool_allocator__create(element_size, nbr_of_elements);
}

void pool_allocator__destroy(POOL_ALLOCATOR * pa)
{
    assert(pool_allocator_mock_ptr);
    pool_allocator_mock_ptr->pool_allocator__destroy(pa);
}

void * pool_allocator__new(POOL_ALLOCATOR * pa)
{
    assert(pool_allocator_mock_ptr);
    return pool_allocator_mock_ptr->pool_allocator__new(pa);
}

void pool_allocator__delete(POOL_ALLOCATOR * pa, void * entry)
{
    assert(pool_allocator_mock_ptr);
    pool_allocator_mock_ptr->pool_allocator__delete(pa, entry);
}

size_t pool_allocator__element_size(const POOL_ALLOCATOR * pa)
{
    assert(pool_allocator_mock_ptr);
    return pool_allocator_mock_ptr->pool_allocator__element_size(pa);
}

size_t pool_allocator__capacity(const POOL_ALLOCATOR * pa)
{
    assert(pool_allocator_mock_ptr);
    return pool_allocator_mock_ptr->pool_allocator__capacity(pa);
}