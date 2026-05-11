/**
 * \file        sorted_list.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       sorted_list test implementation
 *
 * Tests for sorted_list component.
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
extern "C" {
#include "sorted_list.h"
}

/* Depedencies */
#include <array>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

/* Mocks */
#include "hal_mem_mock.hpp"

using namespace testing;

/**
 * \brief       Setup empty, mocked test
 *
 * The constructor creates all necessary mocks that are available to tests.
 */
class MockedTest : public Test
{
  protected:
    MockedTest() = default;

    hal_mem_mock hal_mem_mock_i;
};

class MockedInitializedTest : public Test
{
  protected:
    static constexpr uint32_t INITIALIZED_SORTED_LIST_SIZE = 8;
    MockedInitializedTest()
    {
        /* Manually construct a sorted_list instance since we don't have init function. */
        uut_sorted_list.capacity = INITIALIZED_SORTED_LIST_SIZE;
        uut_sorted_list.entry_count = 0u;
        uut_sorted_list.pdata = sorted_list_entries;
    }

    SORTED_LIST_ENTRY sorted_list_entries[INITIALIZED_SORTED_LIST_SIZE];
    SORTED_LIST uut_sorted_list;
};

constexpr uint32_t MockedInitializedTest::INITIALIZED_SORTED_LIST_SIZE;

/*
 * Test dynamic creation of sorted container that succedes. The capacity is 3
 * elements.
 */
TEST_F(MockedTest, sorted_list_create)
{
    /* Preconditions */
    SORTED_LIST sorted_list;
    SORTED_LIST_ENTRY sorted_list_entry[3];

    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer(sizeof(SORTED_LIST_ENTRY) * 3, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM))
        .Times(1)
        .WillRepeatedly(Return(&sorted_list_entry[0]));
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer(sizeof(sorted_list), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM))
        .Times(1)
        .WillRepeatedly(Return(&sorted_list));
    /* Unit Under Test - UUT */
    SORTED_LIST *sc = sorted_list_create(3);

    /* Validation */
    EXPECT_EQ(sc, &sorted_list);
}

TEST_F(MockedTest, sorted_list_destroy)
{
    /* Preconditions */
    SORTED_LIST sorted_list;

    EXPECT_CALL(hal_mem_mock_i, hal_mem_free)
        .Times(2);
    /* Unit Under Test - UUT */
    sorted_list_destroy(&sorted_list);

    /* Validation */
}

/*
 * Based on existing tests from Jens
 * Insert multiple values into a list, the number of insertions is greater than
 * the capacity of sorted list.
 */
TEST_F(MockedInitializedTest, sorted_list_unique_add_overcapacity)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    SORTED_LIST_RETURN_VALUE retval;
    for (uint32_t i = 0; i < (INITIALIZED_SORTED_LIST_SIZE * 2); i++)
    {
        SORTED_LIST_KEY_TYPE key;
        SORTED_LIST_VALUE_TYPE value;

        key = i;
        value = i;
        retval = sorted_list_unique_add(&uut_sorted_list, key, value);
        if (i < INITIALIZED_SORTED_LIST_SIZE)
        {
            EXPECT_EQ(retval, SORTED_LIST_ENTRY_INSERTED);
        }
        else
        {
            EXPECT_EQ(retval, SORTED_LIST_FAIL);
        }
    }

    /* Validation */
    EXPECT_EQ(uut_sorted_list.capacity, INITIALIZED_SORTED_LIST_SIZE);
    EXPECT_LE(uut_sorted_list.entry_count, uut_sorted_list.capacity);
}

/*
 * Based on existing tests from Jens
 * Insert multiple values into a list, the number of insertions is equal to the
 * capacity of the sorted list. Same key is inserted twice. Check unique keys.
 */
TEST_F(MockedInitializedTest, sorted_list_unique_add_multiple_unique)
{
    /* Preconditions */
    struct test_data
    {
        uint32_t key;
        uint32_t value;
        SORTED_LIST_RETURN_VALUE retval;
    };
    std::array<test_data, 18> test_data =
        {{
            {1, 1, SORTED_LIST_ENTRY_INSERTED},
            {1, 1, SORTED_LIST_NO_CHANGE},
            {1, 2, SORTED_LIST_ENTRY_UPDATED},
            {1, 2, SORTED_LIST_NO_CHANGE},
            {1, 1, SORTED_LIST_ENTRY_UPDATED},
            {2, 2, SORTED_LIST_ENTRY_INSERTED},
            {1, 1, SORTED_LIST_NO_CHANGE},
            {2, 2, SORTED_LIST_NO_CHANGE},
            {2, 2, SORTED_LIST_NO_CHANGE},
            {3, 2, SORTED_LIST_ENTRY_INSERTED},
            {3, 1, SORTED_LIST_ENTRY_UPDATED},
            {3, 1, SORTED_LIST_NO_CHANGE},
            {4, 1, SORTED_LIST_ENTRY_INSERTED},
            {5, 1, SORTED_LIST_ENTRY_INSERTED},
            {6, 1, SORTED_LIST_ENTRY_INSERTED},
            {7, 1, SORTED_LIST_ENTRY_INSERTED},
            {8, 1, SORTED_LIST_ENTRY_INSERTED},
            {9, 1, SORTED_LIST_FAIL},
        }};

    /* Unit Under Test - UUT */
    SORTED_LIST_RETURN_VALUE retval;
    for (const auto &e : test_data)
    {
        retval = sorted_list_unique_add(&uut_sorted_list, e.key, e.value);
        /* If `i` is even then the entry is created/inserted, if it is odd the
         * existing entry is updated.
         */
        EXPECT_EQ(retval, e.retval);
    }

    /* Validation */
    EXPECT_EQ(uut_sorted_list.capacity, INITIALIZED_SORTED_LIST_SIZE);
    EXPECT_LE(uut_sorted_list.entry_count, uut_sorted_list.capacity);

    {
        SORTED_LIST_KEY_TYPE key;

        if (uut_sorted_list.entry_count)
        {
            key = uut_sorted_list.pdata[0].key;
        }

        for (int i = 1; i < uut_sorted_list.entry_count; i++)
        {
            ASSERT_GT(uut_sorted_list.pdata[i].key, key);

            key = uut_sorted_list.pdata[i].key;
        }
    }
}

/*
 * Based on existing tests from Jens
 * Insert multiple values into a list, the number of insertions is greater than
 * the capacity of sorted list.
 */
TEST_F(MockedInitializedTest, sorted_list_single_add_overcapacity)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    SORTED_LIST_RETURN_VALUE retval;
    for (uint32_t i = 0; i < (INITIALIZED_SORTED_LIST_SIZE * 2); i++)
    {
        SORTED_LIST_KEY_TYPE key;
        SORTED_LIST_VALUE_TYPE value;

        key = i;
        value = i;
        retval = sorted_list_single_add(&uut_sorted_list, key, value);
        if (i < INITIALIZED_SORTED_LIST_SIZE)
        {
            EXPECT_EQ(retval, SORTED_LIST_ENTRY_INSERTED);
        }
        else
        {
            EXPECT_EQ(retval, SORTED_LIST_FAIL);
        }
    }

    /* Validation */
    EXPECT_EQ(uut_sorted_list.capacity, INITIALIZED_SORTED_LIST_SIZE);
    EXPECT_LE(uut_sorted_list.entry_count, uut_sorted_list.capacity);
}

/*
 * Based on existing tests from Jens
 * Insert multiple values into a list, the number of insertions is equal to the
 * capacity of the sorted list. Same key is inserted twice. Check unique keys.
 */
TEST_F(MockedInitializedTest, sorted_list_single_add_multiple_no_unique)
{
    /* Preconditions */
    struct test_data
    {
        uint32_t key;
        uint32_t value;
        SORTED_LIST_RETURN_VALUE retval;
    };
    std::array<test_data, 18> test_data =
        {{
            {1, 1, SORTED_LIST_ENTRY_INSERTED},
            {1, 1, SORTED_LIST_NO_CHANGE},
            {1, 2, SORTED_LIST_ENTRY_INSERTED},
            {1, 2, SORTED_LIST_NO_CHANGE},
            {1, 1, SORTED_LIST_NO_CHANGE},
            {2, 2, SORTED_LIST_ENTRY_INSERTED},
            {1, 1, SORTED_LIST_NO_CHANGE},
            {2, 2, SORTED_LIST_NO_CHANGE},
            {2, 2, SORTED_LIST_NO_CHANGE},
            {3, 2, SORTED_LIST_ENTRY_INSERTED},
            {3, 1, SORTED_LIST_ENTRY_INSERTED},
            {3, 1, SORTED_LIST_NO_CHANGE},
            {4, 1, SORTED_LIST_ENTRY_INSERTED},
            {5, 1, SORTED_LIST_ENTRY_INSERTED},
            {6, 1, SORTED_LIST_ENTRY_INSERTED},
            {7, 1, SORTED_LIST_FAIL},
            {8, 1, SORTED_LIST_FAIL},
            {9, 1, SORTED_LIST_FAIL},
        }};

    /* Unit Under Test - UUT */
    SORTED_LIST_RETURN_VALUE retval;
    for (const auto &e : test_data)
    {
        retval = sorted_list_single_add(&uut_sorted_list, e.key, e.value);
        /* If `i` is even then the entry is created/inserted, if it is odd the
         * existing entry is dismissed.
         */
        EXPECT_EQ(retval, e.retval);
    }

    /* Validation */
    EXPECT_EQ(uut_sorted_list.capacity, INITIALIZED_SORTED_LIST_SIZE);
    EXPECT_LE(uut_sorted_list.entry_count, uut_sorted_list.capacity);

    {
        SORTED_LIST_KEY_TYPE key = uut_sorted_list.pdata[0].key;
        SORTED_LIST_VALUE_TYPE value = uut_sorted_list.pdata[0].value;

        for (int i = 1; i < uut_sorted_list.entry_count; i++)
        {
            /*
             * Keys must be in ascending order or they can be equal.
             * If keys are equal, then the values must be in ascending order.
             */
            EXPECT_TRUE((uut_sorted_list.pdata[i].key > key) ||
                        ((uut_sorted_list.pdata[i].key == key) && (uut_sorted_list.pdata[i].value > value)));

            key = uut_sorted_list.pdata[i].key;
            value = uut_sorted_list.pdata[i].value;
        }
    }
}

/*
 * Based on existing tests from Jens
 * Insert multiple values into a list, the number of insertions is greater than
 * the capacity of sorted list.
 */
TEST_F(MockedInitializedTest, sorted_list_multuple_add_overcapacity)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    SORTED_LIST_RETURN_VALUE retval;
    for (uint32_t i = 0; i < (INITIALIZED_SORTED_LIST_SIZE * 2); i++)
    {
        SORTED_LIST_KEY_TYPE key;
        SORTED_LIST_VALUE_TYPE value;

        key = i;
        value = i;
        retval = sorted_list_multiple_add(&uut_sorted_list, key, value);
        if (i < INITIALIZED_SORTED_LIST_SIZE)
        {
            EXPECT_EQ(retval, SORTED_LIST_ENTRY_INSERTED);
        }
        else
        {
            EXPECT_EQ(retval, SORTED_LIST_FAIL);
        }
    }

    /* Validation */
    EXPECT_EQ(uut_sorted_list.capacity, INITIALIZED_SORTED_LIST_SIZE);
    EXPECT_LE(uut_sorted_list.entry_count, uut_sorted_list.capacity);
}

/*
 * Based on existing tests from Jens
 * Insert multiple values into a list, the number of insertions is equal to the
 * capacity of the sorted list. Same key is inserted twice. Check unique keys.
 */
TEST_F(MockedInitializedTest, sorted_list_multiple_add_multiple_non_unique)
{
    /* Preconditions */
    struct test_data
    {
        uint32_t key;
        uint32_t value;
        SORTED_LIST_RETURN_VALUE retval;
    };
    std::array<test_data, 18> test_data =
        {{
            {1, 1, SORTED_LIST_ENTRY_INSERTED},
            {1, 1, SORTED_LIST_ENTRY_INSERTED},
            {1, 2, SORTED_LIST_ENTRY_INSERTED},
            {1, 2, SORTED_LIST_ENTRY_INSERTED},
            {1, 1, SORTED_LIST_ENTRY_INSERTED},
            {2, 2, SORTED_LIST_ENTRY_INSERTED},
            {1, 1, SORTED_LIST_ENTRY_INSERTED},
            {2, 2, SORTED_LIST_ENTRY_INSERTED},
            {2, 2, SORTED_LIST_FAIL},
            {3, 2, SORTED_LIST_FAIL},
            {3, 1, SORTED_LIST_FAIL},
            {3, 1, SORTED_LIST_FAIL},
            {4, 1, SORTED_LIST_FAIL},
            {5, 1, SORTED_LIST_FAIL},
            {6, 1, SORTED_LIST_FAIL},
            {7, 1, SORTED_LIST_FAIL},
            {8, 1, SORTED_LIST_FAIL},
            {9, 1, SORTED_LIST_FAIL},
        }};

    /* Unit Under Test - UUT */
    SORTED_LIST_RETURN_VALUE retval;
    for (const auto &e : test_data)
    {
        retval = sorted_list_multiple_add(&uut_sorted_list, e.key, e.value);
        EXPECT_EQ(retval, e.retval);
    }

    /* Validation */
    EXPECT_EQ(uut_sorted_list.capacity, INITIALIZED_SORTED_LIST_SIZE);
    EXPECT_LE(uut_sorted_list.entry_count, uut_sorted_list.capacity);

    {
        SORTED_LIST_KEY_TYPE key = uut_sorted_list.pdata[0].key;
        SORTED_LIST_VALUE_TYPE value = uut_sorted_list.pdata[0].value;

        for (int i = 1; i < uut_sorted_list.entry_count; i++)
        {
            /*
             * Keys must be in ascending order or they can be equal.
             * If keys are equal, then the values must be in ascending order.
             */
            EXPECT_TRUE((uut_sorted_list.pdata[i].key > key) ||
                        ((uut_sorted_list.pdata[i].key == key) && (uut_sorted_list.pdata[i].value >= value)));

            key = uut_sorted_list.pdata[i].key;
            value = uut_sorted_list.pdata[i].value;
        }
    }
}
