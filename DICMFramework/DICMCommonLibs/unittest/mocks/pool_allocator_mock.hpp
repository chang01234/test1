/**
 * \file        pool_allocator_mock.hpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * 
 * \brief       Mock of pool_allocator component.
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

#ifndef POOL_ALLOCATOR_MOCK_HPP_
#define POOL_ALLOCATOR_MOCK_HPP_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "pool_allocator.h"

struct Ipool_allocator;
extern Ipool_allocator * pool_allocator_mock_ptr;

struct Ipool_allocator
{
    Ipool_allocator() { pool_allocator_mock_ptr = this; }
    virtual ~Ipool_allocator() { pool_allocator_mock_ptr = NULL; }
    virtual void pool_allocator__init(POOL_ALLOCATOR * pa,
        uint8_t * elements,
        size_t element_size,
        size_t nbr_of_elements) = 0;
    virtual void pool_allocator__terminate(POOL_ALLOCATOR * pa) = 0;
    virtual POOL_ALLOCATOR * pool_allocator__create(size_t element_size, size_t nbr_of_elements) = 0;
    virtual void pool_allocator__destroy(POOL_ALLOCATOR * pa) = 0;
    virtual void * pool_allocator__new(POOL_ALLOCATOR * pa) = 0;
    virtual void pool_allocator__delete(POOL_ALLOCATOR * pa, void * entry) = 0;
    virtual size_t pool_allocator__element_size(const POOL_ALLOCATOR * pa) = 0;
    virtual size_t pool_allocator__capacity(const POOL_ALLOCATOR * pa) = 0;
};

struct pool_allocator_mock : public Ipool_allocator
{
    MOCK_METHOD(void, pool_allocator__init, (POOL_ALLOCATOR *,
        uint8_t *,
        size_t,
        size_t));
    MOCK_METHOD(void, pool_allocator__terminate, (POOL_ALLOCATOR *));
    MOCK_METHOD(POOL_ALLOCATOR *, pool_allocator__create, (size_t, size_t));
    MOCK_METHOD(void, pool_allocator__destroy, (POOL_ALLOCATOR *));
    MOCK_METHOD(void *, pool_allocator__new, (POOL_ALLOCATOR *));
    MOCK_METHOD(void, pool_allocator__delete, (POOL_ALLOCATOR *, void *));
    MOCK_METHOD(size_t, pool_allocator__element_size, (const POOL_ALLOCATOR *));
    MOCK_METHOD(size_t, pool_allocator__capacity, (const POOL_ALLOCATOR *));
};

#endif /* POOL_ALLOCATOR_MOCK_HPP_ */