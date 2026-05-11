/**
 * \file        sorted_container.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       sorted_container test implementation
 *
 * Tests for sorted_container component.
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

/* Unit Under Test - UUT */
#include "sorted_container.h"

/* Depedencies */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

/* Mocks */
#include "hal_mem_mock.hpp"
#include "pool_allocator_mock.hpp"
#include "sorted_list64_mock.hpp"

using namespace testing;

/**
 * \brief       Setup empty, mocked test
 *
 * The constructor creates all necessary mocks that are available to tests.
 */
class MockedTest : public Test
{
  protected:
    MockedTest() {}

    pool_allocator_mock pool_allocator_i;
    sorted_list64_mock sorted_list64_mock_i;
    hal_mem_mock hal_mem_mock_i;
};

/**
 * \brief       Setup mocked test with initialized instance of pool allocator.
 *
 * The constructor creates all necessary mocks that are available to tests and
 * an instance of sorted container that is initialized. Besides initialized
 * sorted container instance an instance of sorted_list and pool_allocator is
 * also available for function call checks.
 */
class MockedInitializedTest : public MockedTest
{
  protected:
    MockedInitializedTest()
    {
        sorted_container__init(&uut_sorted_container, &uut_sorted_list, &uut_pool_allocator);
    }
    SORTED_CONTAINER uut_sorted_container;
    SORTED_LIST64 uut_sorted_list;
    POOL_ALLOCATOR uut_pool_allocator;
};

/*
 * Test initialization of sorted container. Only validation possible here is code
 * compilation and verification that no depedency (pool_allocator, hal_mem, ...)
 * function call is being made.
 */
TEST_F(MockedTest, sorted_container_init)
{
    /* Preconditions */
    SORTED_CONTAINER sc;
    SORTED_LIST64 sorted_list;
    POOL_ALLOCATOR pool_allocator;

    /* Unit Under Test - UUT */
    sorted_container__init(&sc, &sorted_list, &pool_allocator);

    /* Validation */
    /* Only validation possible here is code compilation. */
}

/*
 * Test dynamic creation of sorted container that succedes. The element stored
 * is `int` and the capacity is 3 elements.
 */
TEST_F(MockedTest, sorted_container_create)
{
    /* Preconditions */
    SORTED_CONTAINER sorted_container;
    SORTED_LIST64 sorted_list;
    POOL_ALLOCATOR pool_allocator;

    EXPECT_CALL(pool_allocator_i, pool_allocator__create(sizeof(int), 3))
        .Times(1)
        .WillRepeatedly(Return(&pool_allocator));
    EXPECT_CALL(pool_allocator_i, pool_allocator__destroy)
        .Times(0);
    EXPECT_CALL(sorted_list64_mock_i, sorted_list64_create(3))
        .Times(1)
        .WillRepeatedly(Return(&sorted_list));
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer(sizeof(SORTED_CONTAINER), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM))
        .Times(1)
        .WillRepeatedly(Return(&sorted_container));

    /* Unit Under Test - UUT */
    SORTED_CONTAINER *sc = sorted_container__create(sizeof(int), 3);

    /* Validation */
    EXPECT_EQ(sc, &sorted_container);
}

/*
 * Try to dynamically create a sorted_container, but the allocation of
 * POOL_ALLOCATOR structure fails.
 */
TEST_F(MockedTest, sorted_container_create_fail_alloc_pool_allocator)
{
    /* Preconditions */
    EXPECT_CALL(pool_allocator_i, pool_allocator__create(sizeof(int), 3))
        .Times(1)
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(pool_allocator_i, pool_allocator__destroy)
        .Times(0);
    EXPECT_CALL(sorted_list64_mock_i, sorted_list64_destroy)
        .Times(0);
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer)
        .Times(0);

    /* Unit Under Test - UUT */
    SORTED_CONTAINER *sc = sorted_container__create(sizeof(int), 3);

    /* Validation */
    EXPECT_EQ(sc, nullptr);
}

/*
 * Try to dynamically create a sorted_container, but the allocation of
 * SORTED_LIST64 structure fails.
 */
TEST_F(MockedTest, sorted_container_create_fail_alloc_sorted_list)
{
    /* Preconditions */
    POOL_ALLOCATOR pool_allocator;

    EXPECT_CALL(pool_allocator_i, pool_allocator__create(sizeof(int), 3))
        .Times(1)
        .WillRepeatedly(Return(&pool_allocator));
    EXPECT_CALL(pool_allocator_i, pool_allocator__destroy(&pool_allocator))
        .Times(1);
    EXPECT_CALL(sorted_list64_mock_i, sorted_list64_create(3))
        .Times(1)
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer)
        .Times(0);

    /* Unit Under Test - UUT */
    SORTED_CONTAINER *sc = sorted_container__create(sizeof(int), 3);

    /* Validation */
    EXPECT_EQ(sc, nullptr);
}

/*
 * Try to dynamically create a sorted_container, but the allocation of
 * SORTED_LIST64 structure fails.
 */
TEST_F(MockedTest, sorted_container_create_fail_alloc_sorted_container)
{
    /* Preconditions */
    SORTED_LIST64 sorted_list;
    POOL_ALLOCATOR pool_allocator;

    EXPECT_CALL(pool_allocator_i, pool_allocator__create(sizeof(int), 3))
        .Times(1)
        .WillRepeatedly(Return(&pool_allocator));
    EXPECT_CALL(pool_allocator_i, pool_allocator__destroy(&pool_allocator))
        .Times(1);
    EXPECT_CALL(sorted_list64_mock_i, sorted_list64_create(3))
        .Times(1)
        .WillRepeatedly(Return(&sorted_list));
    EXPECT_CALL(sorted_list64_mock_i, sorted_list64_destroy(&sorted_list))
        .Times(1);
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer)
        .Times(1)
        .WillRepeatedly(Return(nullptr));

    /* Unit Under Test - UUT */
    SORTED_CONTAINER *sc = sorted_container__create(sizeof(int), 3);

    /* Validation */
    EXPECT_EQ(sc, nullptr);
}

/*
 * Test termination of sorted_container.
 */
TEST_F(MockedInitializedTest, sorted_container_terminate)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    sorted_container__terminate(&uut_sorted_container);

    /* Validation */
    /* Only validation possible here is code compilation. */
}

/*
 * Test dynamic destruction of sorted_container.
 */
TEST_F(MockedInitializedTest, sorted_container_destroy)
{
    /* Preconditions */
    EXPECT_CALL(hal_mem_mock_i, hal_mem_free(&uut_sorted_container))
        .Times(1);
    EXPECT_CALL(sorted_list64_mock_i, sorted_list64_destroy(&uut_sorted_list))
        .Times(1);
    EXPECT_CALL(pool_allocator_i, pool_allocator__destroy(&uut_pool_allocator))
        .Times(1);

    /* Unit Under Test - UUT */
    sorted_container__destroy(&uut_sorted_container);

    /* Validation */
    /* Validation will be performed by gtest */
}
