/**
 * \file        sorted_container_mock.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       sorted_container_mock mock implementation
 *
 * Implementation of sorted_container_mock mocks.
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
#include "sorted_container_mock.hpp"

/* Depends */
#include <cassert>

Isorted_container * sorted_container_mock_ptr;

void sorted_container__init(
    SORTED_CONTAINER * sc, 
    LOOKUP_LIST * sorted_list,
    POOL_ALLOCATOR * pool)
{
    assert(sorted_container_mock_ptr);
    sorted_container_mock_ptr->sorted_container__init(sc, sorted_list, pool);
}

void sorted_container__terminate(SORTED_CONTAINER * sc)
{
    assert(sorted_container_mock_ptr);
    sorted_container_mock_ptr->sorted_container__terminate(sc);
}

SORTED_CONTAINER * sorted_container__create(size_t element_size, size_t nbr_of_elements)
{
    assert(sorted_container_mock_ptr);
    return sorted_container_mock_ptr->sorted_container__create(element_size, nbr_of_elements);
}

void sorted_container__destroy(SORTED_CONTAINER * sc)
{
    assert(sorted_container_mock_ptr);
    sorted_container_mock_ptr->sorted_container__destroy(sc);
}

void * sorted_container__new(SORTED_CONTAINER * sc, uint32_t key)
{
    assert(sorted_container_mock_ptr);
    return sorted_container_mock_ptr->sorted_container__new(sc, key);
}

void * sorted_container__access(SORTED_CONTAINER *sc, uint32_t key)
{
    assert(sorted_container_mock_ptr);
    return sorted_container_mock_ptr->sorted_container__access(sc, key);
}

void sorted_container__delete(SORTED_CONTAINER *sc, uint32_t key)
{
    assert(sorted_container_mock_ptr);
    sorted_container_mock_ptr->sorted_container__delete(sc, key);
}

void sorted_container__delete_all(struct sorted_container *sc)
{
    assert(sorted_container_mock_ptr);
    sorted_container_mock_ptr->sorted_container__delete_all(sc);
}

size_t sorted_container__occupied(const SORTED_CONTAINER * sc)
{
    assert(sorted_container_mock_ptr);
    return sorted_container_mock_ptr->sorted_container__occupied(sc);
}

void sorted_container__iterate(
        const SORTED_CONTAINER * sc,
        uint32_t index,
        void ** data,
        uint32_t * key)
{
    assert(sorted_container_mock_ptr);
    sorted_container_mock_ptr->sorted_container__iterate(sc, index, data, key);
}

size_t sorted_container__capacity(const SORTED_CONTAINER * sc)
{
    assert(sorted_container_mock_ptr);
    return sorted_container_mock_ptr->sorted_container__capacity(sc);
}
