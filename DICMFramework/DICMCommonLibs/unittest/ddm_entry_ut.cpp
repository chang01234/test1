/**
 * \file        ddm_entry.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       ddm_entry test implementation
 *
 * Tests for ddm_entry component.
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
#include "ddm_entry.h"
}

/* Depedencies */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

/* Mocks */
#include "ddm2_parameter_list_mock.hpp"
#include "hal_mem_mock.hpp"

using namespace testing;

//  Mocked data structures

static const char *netwthing = "netwthing";
static const char *device_class = "device_class";
static const char *property = "property";

const DDM2_PARAMETER_LIST_DATA Ddm2_parameter_list_data[DDM2_PARAMETER_COUNT] =
    {
        {/* Parameter 0x0 */
         /* parameter */ 0xdead0001,
         /* cloud */ 0xdead,
         /* netwthing */ netwthing,
         /* device_class*/ device_class,
         /* property */ property,
         /* in_type */ DDM2_TYPE_STRING,
         /* in_unit */ DDM2_UNIT_VOLT,
         /* out_type */ DDM2_TYPE_JUMBO,
         /* out_unit */ DDM2_UNIT_BOOL},
        {/* Parameter 0x1*/
         /* parameter */ 0xdead0002,
         /* cloud */ 0xdead,
         /* netwthing */ netwthing,
         /* device_class*/ device_class,
         /* property */ property,
         /* in_type */ DDM2_TYPE_INT32_T,
         /* in_unit */ DDM2_UNIT_ENUMERATION,
         /* out_type */ DDM2_TYPE_OTHER,
         /* out_unit */ DDM2_UNIT_AMPERE},
        {/* Parameter 0x2*/
         /* parameter */ 0xdead0003,
         /* cloud */ 0xdead,
         /* netwthing */ netwthing,
         /* device_class*/ device_class,
         /* property */ property,
         /* in_type */ DDM2_TYPE_NONE,
         /* in_unit */ DDM2_UNIT_PART,
         /* out_type */ DDM2_TYPE_VOID,
         /* out_unit */ DDM2_UNIT_DEGC},
        {/* Parameter 0x3*/
         /* parameter */ 0xdead0004,
         /* cloud */ 0xdead,
         /* netwthing */ netwthing,
         /* device_class*/ device_class,
         /* property */ property,
         /* in_type */ DDM2_TYPE_UINT32_T,
         /* in_unit */ DDM2_UNIT_DEG,
         /* out_type */ DDM2_TYPE_STRUCT,
         /* out_unit */ DDM2_UNIT_NONE},
};

/**
 * \brief       Setup empty, mocked test
 *
 * The constructor creates all necessary mocks that are available to tests.
 */
class MockedTest : public Test
{
  protected:
    MockedTest() {}

    ddm2_parameter_list_mock ddm2_parameter_list_mock_i;
    hal_mem_mock hal_mem_mock_i;
};

/**
 * \brief       Setup mocked test with initialized instance of DDM entry.
 *
 * The constructor creates all necessary mocks that are available to tests and
 * an instance of DDM entry that is initialized. Besides initialized DDM entry
 * instance, the DDM entry ID is also available.
 */
class MockedInitializedTest : public MockedTest
{
  protected:
    const uint32_t uut_ddm_entry_id = Ddm2_parameter_list_data[0].parameter;
    MockedInitializedTest()
    {
        EXPECT_CALL(ddm2_parameter_list_mock_i, ddm2_parameter_list_lookup(uut_ddm_entry_id))
            .WillRepeatedly(Return(0));
        ddm_entry__init(&uut_ddm_entry, uut_ddm_entry_id);
    }

    ddm_entry_t uut_ddm_entry;
};

/**
 * \brief       Setup mocked test with multiple initialized instances of DDM entries.
 *
 * The constructor creates all necessary mocks that are available to tests and
 * an instances of DDM entries that are initialized. Besides initialized DDM
 * entriy instances, the DDM entry IDs are also available.
 */
class MockedMultipleInitializedTest : public MockedTest
{
  protected:
    static const uint32_t ddm_entries = 9;
    uint32_t uut_ddm_entry_id[ddm_entries];
    MockedMultipleInitializedTest()
    {
        for (auto i = 0u; i < ddm_entries; i++)
        {
            uut_ddm_entry_id[i] = Ddm2_parameter_list_data[i].parameter;
            EXPECT_CALL(ddm2_parameter_list_mock_i, ddm2_parameter_list_lookup(uut_ddm_entry_id[i]))
                .WillRepeatedly(Return(i));
            ddm_entry__init(&uut_ddm_entry[i], uut_ddm_entry_id[i]);
        }
    }

    ddm_entry_t uut_ddm_entry[ddm_entries];
};

/*
 * Test initialization of ddm entry that succedes. The parameter ID that is
 * requested is 0xdead.
 */
TEST_F(MockedTest, ddm_entry_init)
{
    /* Preconditions */

    // With returned index (0) returned by ddm2_parameter_list_lookup() we are
    // ensuring that the `entry` EXPECT_EQ are loooking at parameter for id 0x1
    EXPECT_CALL(ddm2_parameter_list_mock_i, ddm2_parameter_list_lookup(0xdead))
        .Times(1)
        .WillRepeatedly(Return(0));

    /* Unit Under Test - UUT */
    ddm_entry_t entry;
    int retval = ddm_entry__init(&entry, 0xdead);

    /* Validation */
    EXPECT_EQ(retval, 0);
}

/*
 * Test initialization of ddm entry that fails because of missing parameter in
 * ddm2_parameter_list. The missing parameter ID is 0xffff.
 */
TEST_F(MockedTest, ddm_entry_init_fail_missing_parameter)
{
    /* Preconditions */
    EXPECT_CALL(ddm2_parameter_list_mock_i, ddm2_parameter_list_lookup(0xffff))
        .Times(1)
        .WillRepeatedly(Return(-1));

    /* Unit Under Test - UUT */
    ddm_entry_t entry;
    int retval = ddm_entry__init(&entry, 0xffff);

    /* Validation */
    EXPECT_EQ(retval, -1);
}

TEST_F(MockedInitializedTest, ddm_entry__is_value_none_true)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__is_value_none(&uut_ddm_entry), true);
}

TEST_F(MockedInitializedTest, ddm_entry__parameter_id)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__parameter_id(&uut_ddm_entry), uut_ddm_entry_id);
}

TEST_F(MockedInitializedTest, ddm_entry__in_type_string)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__in_type(&uut_ddm_entry), DDM2_TYPE_STRING);
}

TEST_F(MockedInitializedTest, ddm_entry__in_unit_volt)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__in_unit(&uut_ddm_entry), DDM2_UNIT_VOLT);
}

TEST_F(MockedInitializedTest, ddm_entry__out_type_jumbo)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__out_type(&uut_ddm_entry), DDM2_TYPE_JUMBO);
}

TEST_F(MockedInitializedTest, ddm_entry__out_unit_bool)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__out_unit(&uut_ddm_entry), DDM2_UNIT_BOOL);
}

TEST_F(MockedInitializedTest, ddm_entry__has_changed_false)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__has_changed(&uut_ddm_entry), false);
}

TEST_F(MockedInitializedTest, ddm_entry__is_subscribed_false)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__is_subscribed(&uut_ddm_entry), false);
}

TEST_F(MockedInitializedTest, ddm_entry__flags_0)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__flags(&uut_ddm_entry), 0x0);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__parameter_id_0)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__parameter_id(&uut_ddm_entry[0]), uut_ddm_entry_id[0]);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__parameter_id_1)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__parameter_id(&uut_ddm_entry[1]), uut_ddm_entry_id[1]);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__parameter_id_2)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__parameter_id(&uut_ddm_entry[2]), uut_ddm_entry_id[2]);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__parameter_id_3)
{
    /* Preconditions */
    /* Unit Under Test - UUT */
    /* Validation */
    EXPECT_EQ(ddm_entry__parameter_id(&uut_ddm_entry[3]), uut_ddm_entry_id[3]);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__init_copy_none)
{
    /* Preconditions */
    ddm_entry_t ddm_entry_instance;

    /* Unit Under Test - UUT */
    int status = ddm_entry__init_copy(&ddm_entry_instance, &uut_ddm_entry[1]);

    /* Validation */
    EXPECT_EQ(status, 0);
    EXPECT_EQ(ddm_entry__is_value_none(&ddm_entry_instance), true);
    EXPECT_EQ(ddm_entry__is_value_int32(&ddm_entry_instance), false);
    EXPECT_EQ(ddm_entry__parameter_id(&ddm_entry_instance), uut_ddm_entry_id[1]);
    EXPECT_EQ(ddm_entry__in_type(&ddm_entry_instance), DDM2_TYPE_INT32_T);
    EXPECT_EQ(ddm_entry__in_unit(&ddm_entry_instance), DDM2_UNIT_ENUMERATION);
    EXPECT_EQ(ddm_entry__out_type(&ddm_entry_instance), DDM2_TYPE_OTHER);
    EXPECT_EQ(ddm_entry__out_unit(&ddm_entry_instance), DDM2_UNIT_AMPERE);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__init_copy_int32)
{
    /* Preconditions */
    ddm_entry_t ddm_entry_instance;
    ddm_entry__set__value_i32(&uut_ddm_entry[1], 5);

    /* Unit Under Test - UUT */
    int status = ddm_entry__init_copy(&ddm_entry_instance, &uut_ddm_entry[1]);

    /* Validation */
    EXPECT_EQ(status, 0);
    EXPECT_EQ(ddm_entry__is_value_none(&ddm_entry_instance), false);
    EXPECT_EQ(ddm_entry__is_value_int32(&ddm_entry_instance), true);
    EXPECT_EQ(ddm_entry__parameter_id(&ddm_entry_instance), uut_ddm_entry_id[1]);
    EXPECT_EQ(ddm_entry__in_type(&ddm_entry_instance), DDM2_TYPE_INT32_T);
    EXPECT_EQ(ddm_entry__in_unit(&ddm_entry_instance), DDM2_UNIT_ENUMERATION);
    EXPECT_EQ(ddm_entry__out_type(&ddm_entry_instance), DDM2_TYPE_OTHER);
    EXPECT_EQ(ddm_entry__out_unit(&ddm_entry_instance), DDM2_UNIT_AMPERE);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__copy_none)
{
    /* Preconditions */

    /* Unit Under Test - UUT */
    ddm_entry__init_copy(&uut_ddm_entry[0], &uut_ddm_entry[1]);

    /* Validation */
    EXPECT_EQ(ddm_entry__is_value_none(&uut_ddm_entry[0]), true);
    EXPECT_EQ(ddm_entry__is_value_int32(&uut_ddm_entry[0]), false);
    EXPECT_EQ(ddm_entry__parameter_id(&uut_ddm_entry[0]), uut_ddm_entry_id[1]);
    EXPECT_EQ(ddm_entry__in_type(&uut_ddm_entry[0]), DDM2_TYPE_INT32_T);
    EXPECT_EQ(ddm_entry__in_unit(&uut_ddm_entry[0]), DDM2_UNIT_ENUMERATION);
    EXPECT_EQ(ddm_entry__out_type(&uut_ddm_entry[0]), DDM2_TYPE_OTHER);
    EXPECT_EQ(ddm_entry__out_unit(&uut_ddm_entry[0]), DDM2_UNIT_AMPERE);
}

TEST_F(MockedMultipleInitializedTest, ddm_entry__copy_int32)
{
    /* Preconditions */
    ddm_entry__set__value_i32(&uut_ddm_entry[1], 5);

    /* Unit Under Test - UUT */
    ddm_entry__copy(&uut_ddm_entry[0], &uut_ddm_entry[1]);

    /* Validation */
    EXPECT_EQ(ddm_entry__is_value_none(&uut_ddm_entry[0]), false);
    EXPECT_EQ(ddm_entry__is_value_int32(&uut_ddm_entry[0]), true);
    EXPECT_EQ(ddm_entry__parameter_id(&uut_ddm_entry[0]), uut_ddm_entry_id[1]);
    EXPECT_EQ(ddm_entry__in_type(&uut_ddm_entry[0]), DDM2_TYPE_INT32_T);
    EXPECT_EQ(ddm_entry__in_unit(&uut_ddm_entry[0]), DDM2_UNIT_ENUMERATION);
    EXPECT_EQ(ddm_entry__out_type(&uut_ddm_entry[0]), DDM2_TYPE_OTHER);
    EXPECT_EQ(ddm_entry__out_unit(&uut_ddm_entry[0]), DDM2_UNIT_AMPERE);
}
