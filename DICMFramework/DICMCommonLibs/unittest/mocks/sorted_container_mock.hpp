/**
 * \file        sorted_container_mock.hpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * 
 * \brief       Mock of sorted_container component.
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

#ifndef SORTED_CONTAINER_MOCK_HPP_
#define SORTED_CONTAINER_MOCK_HPP_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "sorted_container.h"

struct Isorted_container;
extern Isorted_container * sorted_container_mock_ptr;

struct Isorted_container
{
    Isorted_container() { sorted_container_mock_ptr = this; }
    virtual ~Isorted_container() { sorted_container_mock_ptr = NULL; }
    virtual void sorted_container__init(
        SORTED_CONTAINER *, 
        LOOKUP_LIST *,
        POOL_ALLOCATOR *) = 0;
    virtual void sorted_container__terminate(SORTED_CONTAINER *) = 0;
    virtual SORTED_CONTAINER * sorted_container__create(size_t, size_t) = 0;
    virtual void sorted_container__destroy(SORTED_CONTAINER *) = 0;
    virtual void * sorted_container__new(SORTED_CONTAINER *, uint32_t) = 0;
    virtual void * sorted_container__access(SORTED_CONTAINER *, uint32_t) = 0;
    virtual void sorted_container__delete(SORTED_CONTAINER *, uint32_t) = 0;
    virtual void sorted_container__delete_all(SORTED_CONTAINER *) = 0;
    virtual size_t sorted_container__occupied(const SORTED_CONTAINER *) = 0;
    virtual void sorted_container__iterate(
        const SORTED_CONTAINER *,
        uint32_t,
        void **,
        uint32_t *) = 0;
    virtual size_t sorted_container__capacity(const SORTED_CONTAINER *) = 0;
};

struct sorted_container_mock : public Isorted_container
{
    MOCK_METHOD(void, sorted_container__init, (
        SORTED_CONTAINER *, 
        LOOKUP_LIST *,
        POOL_ALLOCATOR *));
    MOCK_METHOD(void, sorted_container__terminate, (SORTED_CONTAINER *));
    MOCK_METHOD(SORTED_CONTAINER *, sorted_container__create, (size_t, size_t));
    MOCK_METHOD(void, sorted_container__destroy, (SORTED_CONTAINER *));
    MOCK_METHOD(void *, sorted_container__new, (SORTED_CONTAINER *, uint32_t));
    MOCK_METHOD(void *, sorted_container__access, (SORTED_CONTAINER *, uint32_t));
    MOCK_METHOD(void, sorted_container__delete, (SORTED_CONTAINER *, uint32_t));
    MOCK_METHOD(void, sorted_container__delete_all, (SORTED_CONTAINER *));
    MOCK_METHOD(size_t, sorted_container__occupied, (const SORTED_CONTAINER *));
    MOCK_METHOD(void, sorted_container__iterate, (
        const SORTED_CONTAINER *,
        uint32_t,
        void **,
        uint32_t *));
    MOCK_METHOD(size_t, sorted_container__capacity, (const SORTED_CONTAINER *));
};

#endif /* SORTED_CONTAINER_MOCK_HPP_ */