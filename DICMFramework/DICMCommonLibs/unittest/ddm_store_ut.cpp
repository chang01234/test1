/**
 * \file        ddm_store.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       ddm_store test implementation
 *
 * Tests for ddm_store component.
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
#include "ddm_store.h"

/* Depedencies */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

/* Mocks */
#include "ddm2_parameter_list_mock.hpp"
#include "ddm_entry_mock.hpp"
#include "hal_mem_mock.hpp"
#include "sorted_container_mock.hpp"

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

    sorted_container_mock sorted_container_mock_i;
    ddm_entry_mock ddm_entry_mock_i;
    hal_mem_mock hal_mem_mock_i;
};

/**
 * \brief       Setup mocked test with initialized instance of DDM store.
 *
 * The constructor creates all necessary mocks that are available to tests and
 * an instance of DDM store that is initialized. Besides initialized DDM store
 * instance an instance of sorted_container is also available for function call
 * checks.
 */
class MockedInitializedTest : public MockedTest
{
  protected:
    MockedInitializedTest()
    {
        ddm_store__init(&uut_ddm_store, &uut_sorted_container);
    }
    ddm_store_t uut_ddm_store;
    SORTED_CONTAINER uut_sorted_container;
};

/*
 * Test initialization of DDM store. Only validation possible here is code
 * compilation and verification that no depedency (ddm_entry, sorted_container,
 * hal_mem) function call is being made.
 */
TEST_F(MockedTest, ddm_store_init)
{
    /* Preconditions */

    /* Unit Under Test - UUT */
    ddm_store_t ddm_store;
    SORTED_CONTAINER sorted_container;
    ddm_store__init(&ddm_store, &sorted_container);

    /* Validation */
    /* Only validation possible here is code compilation. */
}

/*
 * Test dynamic creation of ddm store that succedes.
 */
TEST_F(MockedTest, ddm_store_create)
{
    /* Preconditions */
    SORTED_CONTAINER sorted_container_instance;
    ddm_store_t ddm_store_instance;
    EXPECT_CALL(sorted_container_mock_i, sorted_container__create(sizeof(ddm_entry_t), 2))
        .Times(1)
        .WillRepeatedly(Return(&sorted_container_instance));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__destroy)
        .Times(0);
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer(sizeof(ddm_store_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM))
        .Times(1)
        .WillRepeatedly(Return(&ddm_store_instance));

    /* Unit Under Test - UUT */
    ddm_store_t *ddm_store = ddm_store__create(2);

    /* Validation */
    EXPECT_EQ(ddm_store, &ddm_store_instance);
}

/*
 * Try to dynamically create a DDM store, but the allocation of SORTED_CONTAINER
 * structure fails.
 */
TEST_F(MockedTest, ddm_store_create_fail_alloc_sorted_container)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__create(sizeof(ddm_entry_t), 2))
        .Times(1)
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__destroy)
        .Times(0);
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer(sizeof(ddm_store_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM))
        .Times(0);

    /* Unit Under Test - UUT */
    ddm_store_t *ddm_store = ddm_store__create(2);

    /* Validation */
    EXPECT_EQ(ddm_store, nullptr);
}

/*
 * Try to dynamically create a DDM store, but the allocation of ddm_store_t
 * structure fails.
 */
TEST_F(MockedTest, ddm_store_create_fail_alloc_ddm_store)
{
    /* Preconditions */
    SORTED_CONTAINER sorted_container_instance;
    EXPECT_CALL(sorted_container_mock_i, sorted_container__create(sizeof(ddm_entry_t), 2))
        .Times(1)
        .WillRepeatedly(Return(&sorted_container_instance));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__destroy(&sorted_container_instance))
        .Times(1);
    EXPECT_CALL(hal_mem_mock_i, hal_mem_malloc_prefer(sizeof(ddm_store_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM))
        .Times(1)
        .WillRepeatedly(Return(nullptr));

    /* Unit Under Test - UUT */
    ddm_store_t *ddm_store = ddm_store__create(2);

    /* Validation */
    EXPECT_EQ(ddm_store, nullptr);
}

/*
 * Test termination of DDM store which is empty.
 */
TEST_F(MockedInitializedTest, ddm_store_terminate_empty)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&uut_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate)
        .Times(0);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete_all(&uut_sorted_container))
        .Times(1);

    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__terminate)
        .Times(0);

    /* Unit Under Test - UUT */
    ddm_store__terminate(&uut_ddm_store);

    /* Validation */
    /* Validation will be performed by gtest */
}

/*
 * Test termination of DDM store which has some instances. The number of
 * instances is 3, and the termination function shall call
 * `ddm_entry__terminate()` for each of them.
 */
TEST_F(MockedInitializedTest, ddm_store_terminate_non_empty)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&uut_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(3));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate)
        .Times(3);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete_all(&uut_sorted_container))
        .Times(1);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__terminate)
        .Times(3);

    /* Unit Under Test - UUT */
    ddm_store__terminate(&uut_ddm_store);

    /* Validation */
    /* Validation will be performed by gtest */
}

/*
 * Destroy a dynamically allocated ddm store which has no instances in it.
 */
TEST_F(MockedInitializedTest, ddm_store_destroy_empty)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&uut_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate)
        .Times(0);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete_all(&uut_sorted_container))
        .Times(1);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__destroy(&uut_sorted_container))
        .Times(1);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__terminate)
        .Times(0);
    EXPECT_CALL(hal_mem_mock_i, hal_mem_free(&uut_ddm_store))
        .Times(1);

    /* Unit Under Test - UUT */
    ddm_store__destroy(&uut_ddm_store);

    /* Validation */
    /* Validation will be performed by gtest */
}

/*
 * Destroy a dynamically allocated ddm store which has 3 instances in it.
 */
TEST_F(MockedInitializedTest, ddm_store_destroy_non_empty)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&uut_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(3));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate)
        .Times(3);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete_all(&uut_sorted_container))
        .Times(1);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__destroy(&uut_sorted_container))
        .Times(1);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__terminate)
        .Times(3);
    EXPECT_CALL(hal_mem_mock_i, hal_mem_free(&uut_ddm_store))
        .Times(1);

    /* Unit Under Test - UUT */
    ddm_store__destroy(&uut_ddm_store);

    /* Validation */
    /* Validation will be performed by gtest */
}

/*
 * Create a new entry that succedes. The parameter ID is 0xdead.
 */
TEST_F(MockedInitializedTest, ddm_store_new_entry)
{
    /* Preconditions */
    ddm_entry_t ddm_entry;
    EXPECT_CALL(sorted_container_mock_i, sorted_container__new(&uut_sorted_container, 0xdead))
        .Times(1)
        .WillRepeatedly(Return(&ddm_entry));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete)
        .Times(0);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(&ddm_entry, 0xdead))
        .Times(1)
        .WillRepeatedly(Return(0));

    /* Unit Under Test - UUT */
    ddm_entry_t *new_ddm_entry = ddm_store__new_entry(&uut_ddm_store, 0xdead);

    /* Validation */
    EXPECT_EQ(new_ddm_entry, &ddm_entry);
}

/*
 * Create a new entry that fails because the parameter ID is non-existant in
 * DDM2 parameter list. The parameter ID is 0xdead.
 */
TEST_F(MockedInitializedTest, ddm_store_new_entry_fail_wrong_parameter)
{
    /* Preconditions */
    ddm_entry_t ddm_entry;
    EXPECT_CALL(sorted_container_mock_i, sorted_container__new(&uut_sorted_container, 0xdead))
        .Times(1)
        .WillRepeatedly(Return(&ddm_entry));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete(&uut_sorted_container, 0xdead))
        .Times(1);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(&ddm_entry, 0xdead))
        .Times(1)
        .WillRepeatedly(Return(-1));

    /* Unit Under Test - UUT */
    ddm_entry_t *new_ddm_entry = ddm_store__new_entry(&uut_ddm_store, 0xdead);

    /* Validation */
    EXPECT_EQ(new_ddm_entry, nullptr);
}

/*
 * Create a new entry that fails because the allocation of memory for ddm_entry_t
 * fails. The parameter ID is 0xdead.
 */
TEST_F(MockedInitializedTest, ddm_store_new_entry_fail_alloc_ddm_entry)
{
    /* Preconditions */
    ddm_entry_t ddm_entry;
    EXPECT_CALL(sorted_container_mock_i, sorted_container__new(&uut_sorted_container, 0xdead))
        .Times(1)
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete)
        .Times(0);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(&ddm_entry, 0xdead))
        .Times(0);

    /* Unit Under Test - UUT */
    ddm_entry_t *new_ddm_entry = ddm_store__new_entry(&uut_ddm_store, 0xdead);

    /* Validation */
    EXPECT_EQ(new_ddm_entry, nullptr);
}

/*
 * Test load_entries() which succedes. The entries contains 6 DDM parameters of
 * every supported type by ddm_store. The ddm store uut_ddm_store shall contain
 * the mentioned values.
 */
TEST_F(MockedInitializedTest, ddm_store_load_entries)
{
    /* Preconditions */
    struct ddm_value_struct
    {
        int a;
    } ddm_value_struct;
    uint8_t ddm_value_other[5];
    char hello_string[] = "hello";
    ddm_entry_t ddm_entry_instance[6];
    static const ddm_store_ddm_t entries[6] =
        {
            {0xdead0010, {{.i32 = 1}, 0, DDM2_TYPE_INT32_T}},
            {0xdead0020, {{.u32 = 2u}, 0, DDM2_TYPE_UINT32_T}},
            {0xdead0030, {{.str = hello_string}, 0, DDM2_TYPE_STRING}},
            {0xdead0040, {{.str = nullptr}, 0, DDM2_TYPE_STRING}},
            {0xdead0050, {{.structure = &ddm_value_struct}, sizeof(ddm_value_struct), DDM2_TYPE_STRUCT}},
            {0xdead0060, {{.other = ddm_value_other}, sizeof(ddm_value_other), DDM2_TYPE_OTHER}},
        };

    {
        InSequence seq;

        /* Loading of 0xdead0010 parameter */
        EXPECT_CALL(sorted_container_mock_i, sorted_container__new(
                                                 &uut_sorted_container,
                                                 0xdead0010 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(&ddm_entry_instance[0]));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(
                                          &ddm_entry_instance[0],
                                          0xdead0010 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(0));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__set__value_i32(&ddm_entry_instance[0], 1))
            .Times(1)
            .WillRepeatedly(Return(true));

        /* Loading of 0xdead0020 parameter */
        EXPECT_CALL(sorted_container_mock_i, sorted_container__new(
                                                 &uut_sorted_container,
                                                 0xdead0020 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(&ddm_entry_instance[1]));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(
                                          &ddm_entry_instance[1],
                                          0xdead0020 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(0));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__set__value_u32(&ddm_entry_instance[1], 2u))
            .Times(1)
            .WillRepeatedly(Return(true));

        /* Loading of 0xdead0030 parameter */
        EXPECT_CALL(sorted_container_mock_i, sorted_container__new(
                                                 &uut_sorted_container,
                                                 0xdead0030 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(&ddm_entry_instance[2]));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(
                                          &ddm_entry_instance[2],
                                          0xdead0030 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(0));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__set__value_str(&ddm_entry_instance[2], hello_string, 5))
            .Times(1)
            .WillRepeatedly(Return(true));

        /* Loading of 0xdead0040 parameter */
        EXPECT_CALL(sorted_container_mock_i, sorted_container__new(
                                                 &uut_sorted_container,
                                                 0xdead0040 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(&ddm_entry_instance[3]));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(
                                          &ddm_entry_instance[3],
                                          0xdead0040 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(0));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__set__value_str(&ddm_entry_instance[3], _, 0))  // Ignore second argument
            .Times(1)
            .WillRepeatedly(Return(true));

        /* Loading of 0xdead0050 parameter */
        EXPECT_CALL(sorted_container_mock_i, sorted_container__new(
                                                 &uut_sorted_container,
                                                 0xdead0050 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(&ddm_entry_instance[4]));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(
                                          &ddm_entry_instance[4],
                                          0xdead0050 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(0));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__set__value_struct(
                                          &ddm_entry_instance[4],
                                          &ddm_value_struct,
                                          sizeof(ddm_value_struct)))
            .Times(1)
            .WillRepeatedly(Return(true));

        /* Loading of 0xdead0060 parameter */
        EXPECT_CALL(sorted_container_mock_i, sorted_container__new(
                                                 &uut_sorted_container,
                                                 0xdead0060 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(&ddm_entry_instance[5]));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init(
                                          &ddm_entry_instance[5],
                                          0xdead0060 | DDM2_PARAMETER_INSTANCE(0x02)))
            .Times(1)
            .WillRepeatedly(Return(0));
        EXPECT_CALL(ddm_entry_mock_i, ddm_entry__set__value_other(
                                          &ddm_entry_instance[5],
                                          &ddm_value_other,
                                          sizeof(ddm_value_other)))
            .Times(1)
            .WillRepeatedly(Return(true));
    }
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete)
        .Times(0);

    /* Unit Under Test - UUT */
    ddm_store__load_entries(&uut_ddm_store, entries, 6, 0x02);

    /* Validation */
    /* Validation will be performed by gtest */
}

/*
 * Test deletion of all DDM entries in a store which is empty.
 */
TEST_F(MockedInitializedTest, ddm_store_delete_all_empty)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&uut_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate)
        .Times(0);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete_all(&uut_sorted_container))
        .Times(1);

    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__terminate)
        .Times(0);

    /* Unit Under Test - UUT */
    ddm_store__delete_all(&uut_ddm_store);

    /* Validation */
    /* Validation will be performed by gtest */
}

/*
 * Test deletion of all DDM entries in a store which has some instances. The
 * number of instances is 3, and the termination function shall call
 * `ddm_entry__terminate()` for each of them.
 */
TEST_F(MockedInitializedTest, ddm_store_delete_all_non_empty)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&uut_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(3));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate)
        .Times(3);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete_all(&uut_sorted_container))
        .Times(1);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__terminate)
        .Times(3);

    /* Unit Under Test - UUT */
    ddm_store__delete_all(&uut_ddm_store);

    /* Validation */
    /* Validation will be performed by gtest */
}

/*
 * Test ddm_store__access() which succedes. The searched ddm_parameter is 0xdead.
 */
TEST_F(MockedInitializedTest, ddm_store_access)
{
    /* Preconditions */
    ddm_entry_t ddm_entry_instance;
    const uint32_t parameter_id = 0xdeadbeef;

    EXPECT_CALL(sorted_container_mock_i, sorted_container__access(&uut_sorted_container, parameter_id))
        .Times(1)
        .WillRepeatedly(Return(&ddm_entry_instance));

    /* Unit Under Test - UUT */
    ddm_entry_t *ddm_entry = ddm_store__access(&uut_ddm_store, parameter_id);

    /* Validation */
    EXPECT_EQ(ddm_entry, &ddm_entry_instance);
}

/*
 * Test ddm_store__access() which fails. The searched ddm_parameter is 0xdead
 * and it does not exist in sorted container.
 */
TEST_F(MockedInitializedTest, ddm_store_access_fail_wrong_parameter)
{
    /* Preconditions */
    EXPECT_CALL(sorted_container_mock_i, sorted_container__access(&uut_sorted_container, 0xdead))
        .Times(1)
        .WillRepeatedly(Return(nullptr));

    /* Unit Under Test - UUT */
    ddm_entry_t *ddm_entry = ddm_store__access(&uut_ddm_store, 0xdead);

    /* Validation */
    EXPECT_EQ(ddm_entry, nullptr);
}

/*
 * Test copying of entries for source store to uut store. The operation shall
 * succeed.
 */
TEST_F(MockedInitializedTest, ddm_store_copy_entries)
{
    /* Preconditions */
    SORTED_CONTAINER source_sorted_container;
    ddm_store_t source_store;
    ddm_store__init(&source_store, &source_sorted_container);

    EXPECT_CALL(sorted_container_mock_i, sorted_container__capacity(&uut_sorted_container))
        .WillRepeatedly(Return(6));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&source_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(3));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate(&source_sorted_container, _, _, _))
        .Times(3);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__new(&uut_sorted_container, _))
        .Times(3)
        .WillRepeatedly(ReturnNew<ddm_entry_t>());
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete)
        .Times(0);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__parameter_id)
        .Times(3)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init)
        .Times(3)
        .WillRepeatedly(Return(0));
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__copy)
        .Times(3);

    /* Unit Under Test - UUT */
    int retval = ddm_store__copy_entries(&uut_ddm_store, &source_store);

    /* Validation */
    EXPECT_EQ(retval, 0);
}

/*
 * Test copying of entries for source store to uut store. The operation shall
 * fail because there is no enough free space in uut_ddm_store.
 */
TEST_F(MockedInitializedTest, ddm_store_copy_entries_fail_no_space)
{
    /* Preconditions */
    SORTED_CONTAINER source_sorted_container;
    ddm_store_t source_store;
    ddm_store__init(&source_store, &source_sorted_container);

    EXPECT_CALL(sorted_container_mock_i, sorted_container__capacity(&uut_sorted_container))
        .WillRepeatedly(Return(4));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__occupied(&source_sorted_container))
        .Times(1)
        .WillRepeatedly(Return(3));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__iterate(&source_sorted_container, _, _, _))
        .Times(1);
    EXPECT_CALL(sorted_container_mock_i, sorted_container__new(&uut_sorted_container, _))
        .Times(1)
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(sorted_container_mock_i, sorted_container__delete)
        .Times(0);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__parameter_id)
        .Times(1);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__init)
        .Times(0);
    EXPECT_CALL(ddm_entry_mock_i, ddm_entry__copy)
        .Times(0);

    /* Unit Under Test - UUT */
    int retval = ddm_store__copy_entries(&uut_ddm_store, &source_store);

    /* Validation */
    EXPECT_EQ(retval, -1);
}
