/**
 * @file test_rule_engine_memory_accessor.cpp
 *
 * @brief Rule engine data layout computation tests
 *
 * Tests rule engine data layout computation functions, ensuring they correctly
 * handle memory access, data retrieval, and manipulation.
 *
 * @author Borjan Bozhinovski
 * @date July 21, 2025
 */

#include "test_rule_engine.hpp"

extern "C" {
#include "rule_engine_memory_accessor.h"
}

class RuleEngineMemoryAccessor : public RuleEngineTestFixture
{
  protected:
    void SetUp() override
    {
        RuleEngineTestFixture::SetUp();
    }

    void TearDown() override
    {
        RuleEngineTestFixture::TearDown();
    }
};

TEST_F(RuleEngineMemoryAccessor, verifyRead)
{
    typedef struct DATA_LAYOUT
    {
        uint32_t a;
        int16_t b;
        int8_t c;
    } PACKED DATA_LAYOUT_T;

    DATA_LAYOUT_T myData = {.a = 0x12345678, .b = 0x4268, .c = 0x7A};

    void *addr;
    int32_t value;
    EXPECT_EQ((addr = rule_engine_memory_accessor_get_address(&myData, 0, 0, 0)), (uint8_t *)&myData + 0);
    EXPECT_EQ(rule_engine_memory_accessor_read_value(addr, 4, &value), 1) << "Reading 'a' should succeed";
    EXPECT_EQ(value, 0x12345678) << "Value of 'a' should be 0x12345678";
    EXPECT_EQ((addr = rule_engine_memory_accessor_get_address(&myData, 4, 0, 0)), (uint8_t *)&myData + 4);
    EXPECT_EQ(rule_engine_memory_accessor_read_value(addr, 2, &value), 1) << "Reading 'b' should succeed";
    EXPECT_EQ(value, 0x4268) << "Value of 'b' should be 0x4268";
    EXPECT_EQ((addr = rule_engine_memory_accessor_get_address(&myData, 6, 0, 0)), (uint8_t *)&myData + 6);
    EXPECT_EQ(rule_engine_memory_accessor_read_value(addr, 1, &value), 1) << "Reading 'c' should succeed";
    EXPECT_EQ(value, 0x7A) << "Value of 'c' should be 0x7A";
}

TEST_F(RuleEngineMemoryAccessor, verifyWrite)
{
    typedef struct DATA_LAYOUT
    {
        uint32_t a;
        int8_t c;
        int16_t b;
    } PACKED DATA_LAYOUT_T;

    DATA_LAYOUT_T myData = {.a = 0x12345678, .c = 0x7A, .b = 0x4268};

    void *addr;
    int32_t value;
    addr = rule_engine_memory_accessor_get_address(&myData, 0, 0, 0);
    EXPECT_TRUE(rule_engine_memory_accessor_write_value(addr, 0x87654321, 4)) << "Writing value 0x87654321 to 'a' should succeed";
    EXPECT_EQ(rule_engine_memory_accessor_read_value(addr, 4, &value), 1) << "Reading 'a' should succeed";
    EXPECT_EQ(value, 0x87654321) << "Value of 'a' should be 0x87654321";
    addr = rule_engine_memory_accessor_get_address(&myData, 4, 0, 0);
    EXPECT_TRUE(rule_engine_memory_accessor_write_value(addr, 0x7B, 1)) << "Writing value 0x7B to 'c' should succeed";
    EXPECT_EQ(rule_engine_memory_accessor_read_value(addr, 1, &value), 1) << "Reading 'c' should succeed";
    EXPECT_EQ(value, 0x7B) << "Value of 'c' should be 0x7B";
    addr = rule_engine_memory_accessor_get_address(&myData, 5, 0, 0);
    EXPECT_TRUE(rule_engine_memory_accessor_write_value(addr, 0x1234, 2)) << "Writing value 0x1234 to 'b' should succeed";
    EXPECT_EQ(rule_engine_memory_accessor_read_value(addr, 2, &value), 1) << "Reading 'b' should succeed";
    EXPECT_EQ(value, 0x1234) << "Value of 'b' should be 0x1234";
}

TEST_F(RuleEngineMemoryAccessor, simpleArray)
{
    int32_t value;

    typedef struct ARRAY
    {
        uint32_t a[5];
    } PACKED ARRAY_T;

    ARRAY_T myArray = {.a = {1, 2, 3, 4, 5}};

    size_t num_of_elements = rule_engine_memory_accessor_get_array_length(sizeof(myArray), 0, 4);
    EXPECT_EQ(num_of_elements, ELEMENTS(myArray.a)) << "Number of elements in the array should be 5";

    int32_t index = rule_engine_memory_accessor_find_value(myArray.a, sizeof(myArray), 0, 4, 4, 3, 0);
    EXPECT_EQ(index, 2) << "Value 3 should be found at index 2";

    // get value 3 at index 2(myArray[2])
    rule_engine_memory_accessor_get_data(myArray.a, sizeof(myArray), index * 4, 4, &value);
    EXPECT_EQ(value, 3) << "Value at index 2 should be 3";

    // set at index 2(myArray[2]) to 10
    EXPECT_EQ(rule_engine_memory_accessor_set_data(myArray.a, sizeof(myArray), index * 4, 4, 10), 1) << "Setting value at index 2 should succeed";
    // Verify the value is updated trough the index
    rule_engine_memory_accessor_get_data(myArray.a, sizeof(myArray), index * 4, 4, &value);
    EXPECT_EQ(value, 10) << "Value at index 2 should now be 10";

    // get value 10 at index 2(myArray[2]) directly
    rule_engine_memory_accessor_get_data(myArray.a, sizeof(myArray), 8, 4, &value);
    EXPECT_EQ(value, 10) << "Value at index 2 should be 10";

    // get value 4 at index 3(myArray[3]) directly
    rule_engine_memory_accessor_get_data(myArray.a, sizeof(myArray), 12, 4, &value);
    EXPECT_EQ(value, 4) << "Value at index 3 should be 4";

    // get value 5 at index 4(myArray[4]) directly
    rule_engine_memory_accessor_get_data(myArray.a, sizeof(myArray), 16, 4, &value);
    EXPECT_EQ(value, 5) << "Value at index 4 should be 5";
}

TEST_F(RuleEngineMemoryAccessor, simpleFlexibleArray)
{
    int32_t value;

    typedef struct FLEXIBLE_ARRAY
    {
        uint32_t a[0];  // Flexible array member
    } PACKED FLEXIBLE_ARRAY_T;

    size_t array_size = sizeof(FLEXIBLE_ARRAY_T) + 4 * sizeof(uint32_t);  // 4 elements
    FLEXIBLE_ARRAY_T *myArray = (FLEXIBLE_ARRAY_T *)hal_mem_malloc(array_size, HAL_MEM_INTERNAL_RAM);
    myArray->a[0] = 11;
    myArray->a[1] = 21;
    myArray->a[2] = 31;
    myArray->a[3] = 41;

    size_t num_of_elements = rule_engine_memory_accessor_get_array_length(array_size, 0, 4);
    EXPECT_EQ(num_of_elements, array_size / sizeof(uint32_t)) << "Number of elements in the array should be 4";

    int32_t index = rule_engine_memory_accessor_find_value(myArray, array_size, 0, 4, 4, 31, 0);
    EXPECT_EQ(index, 2) << "Value 31 should be found at index 2";

    // get value 3 at index 2(myArray[2])
    rule_engine_memory_accessor_get_data(myArray, array_size, index * 4, 4, &value);
    EXPECT_EQ(value, 31) << "Value at index 2 should be 31";

    // set at index 2(myArray[2]) to 10
    EXPECT_EQ(rule_engine_memory_accessor_set_data(myArray, array_size, index * 4, 4, 10), 1) << "Setting value at index 2 should succeed";
    // Verify the value is updated trough the index
    rule_engine_memory_accessor_get_data(myArray, array_size, index * 4, 4, &value);
    EXPECT_EQ(value, 10) << "Value at index 2 should now be 10";

    // get value 10 at index 2(myArray[2]) directly
    rule_engine_memory_accessor_get_data(myArray, array_size, 8, 4, &value);
    EXPECT_EQ(value, 10) << "Value at index 2 should be 10";

    // get value 41 at index 3(myArray[3]) directly
    rule_engine_memory_accessor_get_data(myArray, array_size, 12, 4, &value);
    EXPECT_EQ(value, 41) << "Value at index 3 should be 41";

    // Out of bounds access
    EXPECT_EQ(rule_engine_memory_accessor_get_data(myArray, array_size, 16, 4, &value), -1) << "Out of bounds access should return -1";
    EXPECT_EQ(rule_engine_memory_accessor_set_data(myArray, array_size, 16, 4, 100), -1) << "Out of bounds set should return -1";
    EXPECT_EQ(rule_engine_memory_accessor_find_value(myArray, array_size, 0, 4, 4, 100, 0), -1) << "Finding out of bounds value should return -1";

    hal_mem_free(myArray);
}

TEST_F(RuleEngineMemoryAccessor, simpleStructure)
{
    int32_t value_of_a;
    int32_t value_of_b;
    int32_t value_of_c;

    typedef struct STRUCTURE
    {
        uint8_t a;
        uint32_t b;
        uint16_t c;
    } PACKED STRUCTURE_T;

    STRUCTURE_T structure = {.a = 1, .b = 2, .c = 3};

    // Get number of elements in the structure
    rule_engine_memory_accessor_get_data(&structure, sizeof(structure), 0, 1, &value_of_a);
    EXPECT_EQ(value_of_a, 1) << "Value of 'a' should be 1";
    rule_engine_memory_accessor_get_data(&structure, sizeof(structure), 1, 4, &value_of_b);
    EXPECT_EQ(value_of_b, 2) << "Value of 'b' should be 2";
    rule_engine_memory_accessor_get_data(&structure, sizeof(structure), 5, 2, &value_of_c);
    EXPECT_EQ(value_of_c, 3) << "Value of 'c' should be 3";

    // Set new values
    value_of_a = 5;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&structure, sizeof(structure), 0, 1, value_of_a), 1) << " Setting value of 'a' should succeed";
    value_of_b = 10;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&structure, sizeof(structure), 1, 4, value_of_b), 1) << " Setting value of 'b' should succeed";
    value_of_c = 20;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&structure, sizeof(structure), 5, 2, value_of_c), 1) << " Setting value of 'c' should succeed";
    // Verify the values are updated
    rule_engine_memory_accessor_get_data(&structure, sizeof(structure), 0, 1, &value_of_a);
    EXPECT_EQ(value_of_a, 5) << "Value of 'a' should now be 5";
    rule_engine_memory_accessor_get_data(&structure, sizeof(structure), 1, 4, &value_of_b);
    EXPECT_EQ(value_of_b, 10) << "Value of 'b' should now be 10";
    rule_engine_memory_accessor_get_data(&structure, sizeof(structure), 5, 2, &value_of_c);
    EXPECT_EQ(value_of_c, 20) << "Value of 'c' should now be 20";
}

TEST_F(RuleEngineMemoryAccessor, arrayOfStructure)
{
    int32_t value_of_a;
    int32_t value_of_b;

    typedef struct ARRAY_OF_STRUCTURE
    {
        struct STRUCTURE_T
        {
            int32_t a;
            int8_t b;
        } PACKED structure[3];
    } PACKED ARRAY_OF_STRUCTURE_T;

    ARRAY_OF_STRUCTURE_T array_of_structure = {
        .structure = {
            {.a = 1, .b = 2},
            {.a = 3, .b = 4},
            {.a = 5, .b = 6}}};

    // Get number of elements in the array of structure
    EXPECT_EQ(rule_engine_memory_accessor_get_array_length(sizeof(array_of_structure), 0, 5), 3) << "Number of elements in the array of structures should be 3";

    // Get elements in the array of structures
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 1) << "Value of 'a' in first structure should be 1";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 2) << "Value of 'b' in first structure should be 2";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 3) << "Value of 'a' in second structure should be 3";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 4) << "Value of 'b' in second structure should be 4";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 5) << "Value of 'a' in third structure should be 5";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 6) << "Value of 'b' in third structure should be 6";

    // Set new values
    value_of_a = 10;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), 0, 4, value_of_a), 1) << "Setting value of 'a' in first structure should succeed";
    value_of_b = 20;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), 4, 1, value_of_b), 1) << "Setting value of 'b' in first structure should succeed";
    value_of_a = 30;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), 5, 4, value_of_a), 1) << "Setting value of 'a' in second structure should succeed";
    value_of_b = 40;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), 9, 1, value_of_b), 1) << "Setting value of 'b' in second structure should succeed";
    value_of_a = 50;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), 10, 4, value_of_a), 1) << "Setting value of 'a' in third structure should succeed";
    value_of_b = 60;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), 14, 1, value_of_b), 1) << "Setting value of 'b' in third structure should succeed";

    // Verify the values are updated
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 10) << "Value of 'a' in first structure should now be 10";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 20) << "Value of 'b' in first structure should now be 20";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 30) << "Value of 'a' in second structure should now be 30";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value of 'b' in second structure should now be 40";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value of 'a' in third structure should now be 50";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 60) << "Value of 'b' in third structure should now be 60";

    // find value in the array of structures for first member
    int32_t index = rule_engine_memory_accessor_find_value(&array_of_structure, sizeof(array_of_structure), 0, 5, 4, 50, 0);
    EXPECT_EQ(index, 2) << "Value 50 should be found in third structure at index 2";
    // Get value using the index
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), index * 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value at index 2 should be 50";

    // find value in the array of structures for second member
    index = rule_engine_memory_accessor_find_value(&array_of_structure, sizeof(array_of_structure), 4, 5, 1, 40, 0);
    EXPECT_EQ(index, 1) << "Value 40 should be found in second structure at index 1";
    // Get value using the index
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), index * 5 + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value at index 1 should be 40";
}

TEST_F(RuleEngineMemoryAccessor, flexibleArrayOfStructure)
{
    int32_t value_of_a;
    int32_t value_of_b;

    typedef struct FLEXIBLE_ARRAY_OF_STRUCTURE
    {
        struct STRUCTURE_T
        {
            int32_t a;
            int8_t b;
        } PACKED structure[0];
    } PACKED FLEXIBLE_ARRAY_OF_STRUCTURE_T;

    size_t array_size = sizeof(FLEXIBLE_ARRAY_OF_STRUCTURE_T) + 2 * 5;  // 2 * 5(sizeof(STRUCTURE_T)) elements
    FLEXIBLE_ARRAY_OF_STRUCTURE_T *flexible_array_of_structure = (FLEXIBLE_ARRAY_OF_STRUCTURE_T *)hal_mem_malloc(array_size, HAL_MEM_INTERNAL_RAM);

    // Get number of elements in the array of structure
    EXPECT_EQ(rule_engine_memory_accessor_get_array_length(array_size, 0, 5), 2) << "Number of elements in the array of structures should be 2";

    array_size += 2 * 5;  // Add two more structures to the flexible array
    flexible_array_of_structure = (FLEXIBLE_ARRAY_OF_STRUCTURE_T *)hal_mem_realloc_prefer(flexible_array_of_structure, array_size, HAL_MEM_INTERNAL_RAM, HAL_MEM_SPIRAM);
    // Get number of elements in the array of structure
    EXPECT_EQ(rule_engine_memory_accessor_get_array_length(array_size, 0, 5), 4) << "Number of elements in the array of structures should be 4";

    array_size -= 1 * 5;  // Remove one structure from the flexible array
    flexible_array_of_structure = (FLEXIBLE_ARRAY_OF_STRUCTURE_T *)hal_mem_realloc_prefer(flexible_array_of_structure, array_size, HAL_MEM_INTERNAL_RAM, HAL_MEM_SPIRAM);
    // Get number of elements in the array of structure
    EXPECT_EQ(rule_engine_memory_accessor_get_array_length(array_size, 0, 5), 3) << "Number of elements in the array of structures should be 3";

    flexible_array_of_structure->structure[0].a = 1;
    flexible_array_of_structure->structure[0].b = 2;
    flexible_array_of_structure->structure[1].a = 3;
    flexible_array_of_structure->structure[1].b = 4;
    flexible_array_of_structure->structure[2].a = 5;
    flexible_array_of_structure->structure[2].b = 6;

    // Get elements in the array of structures
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 1) << "Value of 'a' in first structure should be 1";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 2) << "Value of 'b' in first structure should be 2";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 3) << "Value of 'a' in second structure should be 3";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 4) << "Value of 'b' in second structure should be 4";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 5) << "Value of 'a' in third structure should be 5";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 6) << "Value of 'b' in third structure should be 6";

    // Set new values
    value_of_a = 10;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, 0, 4, value_of_a), 1) << "Setting value of 'a' in first structure should succeed";
    value_of_b = 20;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, 4, 1, value_of_b), 1) << "Setting value of 'b' in first structure should succeed";
    value_of_a = 30;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, 5, 4, value_of_a), 1) << "Setting value of 'a' in second structure should succeed";
    value_of_b = 40;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, 9, 1, value_of_b), 1) << "Setting value of 'b' in second structure should succeed";
    value_of_a = 50;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, 10, 4, value_of_a), 1) << "Setting value of 'a' in third structure should succeed";
    value_of_b = 60;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, 14, 1, value_of_b), 1) << "Setting value of 'b' in third structure should succeed";

    // Verify the values are updated
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 10) << "Value of 'a' in first structure should now be 10";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 20) << "Value of 'b' in first structure should now be 20";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 30) << "Value of 'a' in second structure should now be 30";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value of 'b' in second structure should now be 40";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value of 'a' in third structure should now be 50";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 60) << "Value of 'b' in third structure should now be 60";

    LOG(P, "array of  struct pointer of 2 array member: %p", (uint8_t *)flexible_array_of_structure + 10);
    // find value in the array of structures for first member
    int32_t index = rule_engine_memory_accessor_find_value(flexible_array_of_structure, array_size, 0, 5, 4, 50, 0);
    EXPECT_EQ(index, 2) << "Value 50 should be found in third structure at index 2";
    // Get value using the index
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, index * 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value at index 2 should be 50";

    // find value in the array of structures for second member
    index = rule_engine_memory_accessor_find_value(flexible_array_of_structure, array_size, 4, 5, 1, 40, 0);
    EXPECT_EQ(index, 1) << "Value 40 should be found in second structure at index 1";
    // Get value using the index
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, index * 5 + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value at index 1 should be 40";

    hal_mem_free(flexible_array_of_structure);
}

TEST_F(RuleEngineMemoryAccessor, arrayOfStructureMemberBeforeArray)
{
    int32_t value_of_a;
    int32_t value_of_b;

    typedef struct ARRAY_OF_STRUCTURE
    {
        int16_t x;
        struct STRUCTURE_T
        {
            int32_t a;
            int8_t b;
        } PACKED structure[3];
    } PACKED ARRAY_OF_STRUCTURE_T;

    ARRAY_OF_STRUCTURE_T array_of_structure = {
        .x = 100,
        .structure = {
            {.a = 1, .b = 2},
            {.a = 3, .b = 4},
            {.a = 5, .b = 6}}};

    // Offset for the first structure (2 bytes for x)
    size_t offset = sizeof(array_of_structure.x);

    // Get number of elements in the array of structure
    EXPECT_EQ(rule_engine_memory_accessor_get_array_length(sizeof(array_of_structure), 2, 5), 3) << "Number of elements in the array of structures should be 3";

    // Get elements in the array of structures
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 1) << "Value of 'a' in first structure should be 1";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 2) << "Value of 'b' in first structure should be 2";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 3) << "Value of 'a' in second structure should be 3";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 4) << "Value of 'b' in second structure should be 4";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 5) << "Value of 'a' in third structure should be 5";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 6) << "Value of 'b' in third structure should be 6";

    // Set new values
    value_of_a = 10;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 0, 4, value_of_a), 1) << "Setting value of 'a' in first structure should succeed";
    value_of_b = 20;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 4, 1, value_of_b), 1) << "Setting value of 'b' in first structure should succeed";
    value_of_a = 30;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 5, 4, value_of_a), 1) << "Setting value of 'a' in second structure should succeed";
    value_of_b = 40;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 9, 1, value_of_b), 1) << "Setting value of 'b' in second structure should succeed";
    value_of_a = 50;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 10, 4, value_of_a), 1) << "Setting value of 'a' in third structure should succeed";
    value_of_b = 60;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 14, 1, value_of_b), 1) << "Setting value of 'b' in third structure should succeed";

    // Verify the values are updated
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 10) << "Value of 'a' in first structure should now be 10";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 20) << "Value of 'b' in first structure should now be 20";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 30) << "Value of 'a' in second structure should now be 30";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value of 'b' in second structure should now be 40";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value of 'a' in third structure should now be 50";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 60) << "Value of 'b' in third structure should now be 60";

    // find value in the array of structures for first member
    int32_t index = rule_engine_memory_accessor_find_value(&array_of_structure, sizeof(array_of_structure), offset, 5, 4, 50, 0);
    EXPECT_EQ(index, 2) << "Value 50 should be found in third structure at index 2";
    // Get value using the index
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), index * 5 + offset, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value at index 2 should be 50";

    // find value in the array of structures for second member
    size_t offset_of_first_member = sizeof(array_of_structure.structure[0].a);  // Offset for the first member
    index = rule_engine_memory_accessor_find_value(&array_of_structure, sizeof(array_of_structure), offset + offset_of_first_member, 5, 1, 40, 0);
    EXPECT_EQ(index, 1) << "Value 40 should be found in second structure at index 1";
    // Get value using the index
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), index * 5 + offset + offset_of_first_member, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value at index 1 should be 40";
}

TEST_F(RuleEngineMemoryAccessor, flexibleArrayOfStructureMemberBeforeArray)
{
    int32_t value_of_a;
    int32_t value_of_b;

    typedef struct FLEXIBLE_ARRAY_OF_STRUCTURE
    {
        int16_t x;
        struct STRUCTURE_T
        {
            int32_t a;
            int8_t b;
        } PACKED structure[0];
    } PACKED FLEXIBLE_ARRAY_OF_STRUCTURE_T;

    size_t array_size = sizeof(FLEXIBLE_ARRAY_OF_STRUCTURE_T) + 3 * 5;  // 3 * 5(sizeof(STRUCTURE_T)) elements
    FLEXIBLE_ARRAY_OF_STRUCTURE_T *flexible_array_of_structure = (FLEXIBLE_ARRAY_OF_STRUCTURE_T *)hal_mem_malloc(array_size, HAL_MEM_INTERNAL_RAM);

    flexible_array_of_structure->x = 100;
    flexible_array_of_structure->structure[0].a = 1;
    flexible_array_of_structure->structure[0].b = 2;
    flexible_array_of_structure->structure[1].a = 3;
    flexible_array_of_structure->structure[1].b = 4;
    flexible_array_of_structure->structure[2].a = 5;
    flexible_array_of_structure->structure[2].b = 6;

    // Offset for the first structure (2 bytes for x)
    size_t offset = sizeof(flexible_array_of_structure->x);

    // Get number of elements in the array of structure
    EXPECT_EQ(rule_engine_memory_accessor_get_array_length(array_size, offset, 5), 3) << "Number of elements in the array of structures should be 3";

    // Get elements in the array of structures
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 1) << "Value of 'a' in first structure should be 1";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 2) << "Value of 'b' in first structure should be 2";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 3) << "Value of 'a' in second structure should be 3";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 4) << "Value of 'b' in second structure should be 4";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 5) << "Value of 'a' in third structure should be 5";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 6) << "Value of 'b' in third structure should be 6";

    // Set new values
    value_of_a = 10;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, offset + 0, 4, value_of_a), 1) << "Setting value of 'a' in first structure should succeed";
    value_of_b = 20;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, offset + 4, 1, value_of_b), 1) << "Setting value of 'b' in first structure should succeed";
    value_of_a = 30;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, offset + 5, 4, value_of_a), 1) << "Setting value of 'a' in second structure should succeed";
    value_of_b = 40;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, offset + 9, 1, value_of_b), 1) << "Setting value of 'b' in second structure should succeed";
    value_of_a = 50;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, offset + 10, 4, value_of_a), 1) << "Setting value of 'a' in third structure should succeed";
    value_of_b = 60;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flexible_array_of_structure, array_size, offset + 14, 1, value_of_b), 1) << "Setting value of 'b' in third structure should succeed";

    // Verify the values are updated
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 10) << "Value of 'a' in first structure should now be 10";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 20) << "Value of 'b' in first structure should now be 20";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 30) << "Value of 'a' in second structure should now be 30";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value of 'b' in second structure should now be 40";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value of 'a' in third structure should now be 50";
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 60) << "Value of 'b' in third structure should now be 60";

    // find value in the array of structures for first member
    int32_t index = rule_engine_memory_accessor_find_value(flexible_array_of_structure, array_size, offset, 5, 4, 50, 0);
    EXPECT_EQ(index, 2) << "Value 50 should be found in third structure at index 2";
    // Get value using the index
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, index * 5 + offset, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value at index 2 should be 50";

    // find value in the array of structures for second member
    size_t offset_of_first_member = sizeof(flexible_array_of_structure->structure[0].a);  // Offset for the first member
    index = rule_engine_memory_accessor_find_value(flexible_array_of_structure, array_size, offset + offset_of_first_member, 5, 1, 40, 0);
    EXPECT_EQ(index, 1) << "Value 40 should be found in second structure at index 1";
    // Get value using the index
    rule_engine_memory_accessor_get_data(flexible_array_of_structure, array_size, index * 5 + offset + offset_of_first_member, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value at index 1 should be 40";

    hal_mem_free(flexible_array_of_structure);
}

TEST_F(RuleEngineMemoryAccessor, arrayOfStructureMemberAfterArray)
{
    int32_t value_of_a;
    int32_t value_of_b;

    typedef struct ARRAY_OF_STRUCTURE
    {
        struct STRUCTURE_T
        {
            int32_t a;
            int8_t b;
        } PACKED structure[3];
        int16_t x;
    } PACKED ARRAY_OF_STRUCTURE_T;

    ARRAY_OF_STRUCTURE_T array_of_structure = {
        .structure = {
            {.a = 1, .b = 2},
            {.a = 3, .b = 4},
            {.a = 5, .b = 6}},
        .x = 100,
    };

    size_t offset = 0;
    size_t trailing = sizeof(array_of_structure.x);  // Offset after the end of the array for the trailing member 'x'

    // rule_engine_memory_accessor_get_array_lengths in the array of structure is not used as we already know the number of elements :)

    // Get elements in the array of structures
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 1) << "Value of 'a' in first structure should be 1";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 2) << "Value of 'b' in first structure should be 2";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 3) << "Value of 'a' in second structure should be 3";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 4) << "Value of 'b' in second structure should be 4";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 5) << "Value of 'a' in third structure should be 5";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 6) << "Value of 'b' in third structure should be 6";

    // Set new values
    value_of_a = 10;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 0, 4, value_of_a), 1) << "Setting value of 'a' in first structure should succeed";
    value_of_b = 20;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 4, 1, value_of_b), 1) << "Setting value of 'b' in first structure should succeed";
    value_of_a = 30;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 5, 4, value_of_a), 1) << "Setting value of 'a' in second structure should succeed";
    value_of_b = 40;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 9, 1, value_of_b), 1) << "Setting value of 'b' in second structure should succeed";
    value_of_a = 50;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 10, 4, value_of_a), 1) << "Setting value of 'a' in third structure should succeed";
    value_of_b = 60;
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&array_of_structure, sizeof(array_of_structure), offset + 14, 1, value_of_b), 1) << "Setting value of 'b' in third structure should succeed";

    // Verify the values are updated
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 10) << "Value of 'a' in first structure should now be 10";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 20) << "Value of 'b' in first structure should now be 20";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 30) << "Value of 'a' in second structure should now be 30";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value of 'b' in second structure should now be 40";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value of 'a' in third structure should now be 50";
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 60) << "Value of 'b' in third structure should now be 60";

    // find value in the array of structures for first member
    int32_t index = rule_engine_memory_accessor_find_value(&array_of_structure, sizeof(array_of_structure), offset, 5, 4, 50, trailing);
    EXPECT_EQ(index, 2) << "Value 50 should be found in third structure at index 2";
    // Get value using the index
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), index * 5 + offset, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50) << "Value at index 2 should be 50";

    // find value in the array of structures for second member
    size_t offset_of_first_member = sizeof(array_of_structure.structure[0].a);  // Offset for the first member
    index = rule_engine_memory_accessor_find_value(&array_of_structure, sizeof(array_of_structure), offset + offset_of_first_member, 5, 1, 40, trailing);
    EXPECT_EQ(index, 1) << "Value 40 should be found in second structure at index 1";
    // Get value using the index
    rule_engine_memory_accessor_get_data(&array_of_structure, sizeof(array_of_structure), index * 5 + offset + offset_of_first_member, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value at index 1 should be 40";
}

TEST_F(RuleEngineMemoryAccessor, arrayOfStructureMemberBeforeAndAfterArray)
{
    int32_t value_of_a;
    int32_t value_of_b;

    typedef struct ARRAY_OF_STRUCTURE
    {
        int16_t pre;
        struct STRUCTURE_T
        {
            int32_t a;
            int8_t b;
        } PACKED arr[3];
        int16_t post;
    } PACKED ARRAY_OF_STRUCTURE_T;

    ARRAY_OF_STRUCTURE_T s = {
        .pre = 123,
        .arr = {
            {.a = 1, .b = 2},
            {.a = 3, .b = 4},
            {.a = 5, .b = 6}},
        .post = 456};

    size_t offset = sizeof(s.pre);
    size_t stride = sizeof(s.arr[0]);
    size_t trailing = sizeof(s.post);

    // Count elements
    EXPECT_EQ(rule_engine_memory_accessor_get_array_length(sizeof(s), offset, stride), 3) << "Should count 3 elements in array";

    // Get elements
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 1);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 2);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 3);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 4);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 5);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 6);

    // Set elements
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&s, sizeof(s), offset + 0, 4, 10), 1);
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&s, sizeof(s), offset + 4, 1, 20), 1);
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&s, sizeof(s), offset + 5, 4, 30), 1);
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&s, sizeof(s), offset + 9, 1, 40), 1);
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&s, sizeof(s), offset + 10, 4, 50), 1);
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&s, sizeof(s), offset + 14, 1, 60), 1);

    // Verify set
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 0, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 10);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 4, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 20);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 5, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 30);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 9, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 10, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), offset + 14, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 60);

    // Find value in array (first member)
    int32_t idx = rule_engine_memory_accessor_find_value(&s, sizeof(s), offset, stride, 4, 50, trailing);
    EXPECT_EQ(idx, 2) << "Value 50 should be found in third structure at index 2";

    rule_engine_memory_accessor_get_data(&s, sizeof(s), idx * stride + offset, 4, &value_of_a);
    EXPECT_EQ(value_of_a, 50);

    // Find value in array (second member)
    size_t off_b = sizeof(s.arr[0].a);
    idx = rule_engine_memory_accessor_find_value(&s, sizeof(s), offset + off_b, stride, 1, 40, trailing);
    EXPECT_EQ(idx, 1) << "Value 40 should be found in second structure at index 1";
    rule_engine_memory_accessor_get_data(&s, sizeof(s), idx * stride + offset + off_b, 1, &value_of_b);
    EXPECT_EQ(value_of_b, 40) << "Value at index 1 should be 40";

    // Check pre and post members are unchanged
    rule_engine_memory_accessor_get_data(&s, sizeof(s), 0, 2, &value_of_a);
    EXPECT_EQ(value_of_a, 123);
    rule_engine_memory_accessor_get_data(&s, sizeof(s), 17, 2, &value_of_b);
    EXPECT_EQ(value_of_b, 456);
}

TEST_F(RuleEngineMemoryAccessor, deeplyNestedStructures)
{
    int32_t value;

    // Define our deeply nested structure types
    typedef struct deep_nested_level3
    {
        int value;
    } PACKED DEEP_NESTED_LEVEL3_T;

    typedef struct deep_nested_level2
    {
        DEEP_NESTED_LEVEL3_T level3;
        int mid_value;
        DEEP_NESTED_LEVEL3_T level3_arr[3];  // Static array at level 2
    } PACKED DEEP_NESTED_LEVEL2_T;

    typedef struct deep_nested
    {
        DEEP_NESTED_LEVEL2_T level2;
        int top_value;
        DEEP_NESTED_LEVEL2_T level2_arr[2];  // Static array at level 1
    } PACKED DEEP_NESTED_T;

    // Create and initialize the structure
    DEEP_NESTED_T nested = {
        .level2 = {
            .level3 = {.value = 100},
            .mid_value = 200,
            .level3_arr = {
                {.value = 301},
                {.value = 302},
                {.value = 303}}},
        .top_value = 400,
        .level2_arr = {{.level3 = {.value = 501}, .mid_value = 502, .level3_arr = {{.value = 511}, {.value = 512}, {.value = 513}}}, {.level3 = {.value = 601}, .mid_value = 602, .level3_arr = {{.value = 611}, {.value = 612}, {.value = 613}}}}};

    // Level 1 - Direct value access
    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 0, sizeof(int), &value);
    EXPECT_EQ(value, 100) << "Level 3 value should be 100";  // nested.level2.level3.value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 4, sizeof(int), &value);
    EXPECT_EQ(value, 200) << "Mid value should be 200";  // nested.level2.mid_value

    // Level 2 - Access to array elements within the first nested level
    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 8, sizeof(int), &value);
    EXPECT_EQ(value, 301) << "Level 3 array[0] value should be 301";  // nested.level2.level3_arr[0].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 12, sizeof(int), &value);
    EXPECT_EQ(value, 302) << "Level 3 array[1] value should be 302";  // nested.level2.level3_arr[1].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 16, sizeof(int), &value);
    EXPECT_EQ(value, 303) << "Level 3 array[2] value should be 303";  // nested.level2.level3_arr[2].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 20, sizeof(int), &value);
    EXPECT_EQ(value, 400) << "Top value should be 400";  // nested.top_value

    // Level 3 - Access to elements in the array of level2 structures
    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 24, sizeof(int), &value);
    EXPECT_EQ(value, 501) << "Level 3 value in level2_arr[0] should be 501";  // nested.level2_arr[0].level3.value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 28, sizeof(int), &value);
    EXPECT_EQ(value, 502) << "Mid value in level2_arr[0] should be 502";  // nested.level2_arr[0].mid_value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 32, sizeof(int), &value);
    EXPECT_EQ(value, 511)
        << "Level 3 array value at level2_arr[0].level3_arr[0] should be 511";  // nested.level2_arr[0].level3_arr[0].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 36, sizeof(int), &value);
    EXPECT_EQ(value, 512)
        << "Level 3 array value at level2_arr[0].level3_arr[1] should be 512";  // nested.level2_arr[0].level3_arr[1].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 40, sizeof(int), &value);
    EXPECT_EQ(value, 513)
        << "Level 3 array value at level2_arr[0].level3_arr[2] should be 513";  // nested.level2_arr[0].level3_arr[2].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 44, sizeof(int), &value);
    EXPECT_EQ(value, 601)
        << "Level 3 value in level2_arr[1] should be 601";  // nested.level2_arr[1].level3.value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 48, sizeof(int), &value);
    EXPECT_EQ(value, 602)
        << "Mid value in level2_arr[1] should be 602";  // nested.level2_arr[1].mid_value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 52, sizeof(int), &value);
    EXPECT_EQ(value, 611)
        << "Level 3 array value at level2_arr[1].level3_arr[0] should be 611";  // nested.level2_arr[1].level3_arr[0].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 56, sizeof(int), &value);
    EXPECT_EQ(value, 612)
        << "Level 3 array value at level2_arr[1].level3_arr[1] should be 612";  // nested.level2_arr[1].level3_arr[1].value

    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 60, sizeof(int), &value);
    EXPECT_EQ(value, 613)
        << "Level 3 array value at level2_arr[1].level3_arr[2] should be 613";  // nested.level2_arr[1].level3_arr[2].value

    // Test setting values in deeply nested structures
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&nested, sizeof(nested), 0, sizeof(int), 999), 1)
        << "Setting level 3 value should succeed";  // Setting nested.level2.level3.value
    EXPECT_EQ(nested.level2.level3.value, 999) << "Level 3 value should now be 999";

    // Set value in the most deeply nested array
    EXPECT_EQ(rule_engine_memory_accessor_set_data(&nested, sizeof(nested), 52, sizeof(int), 888), 1)
        << "Setting deeply nested array value should succeed";  // Setting nested.level2_arr[1].level3_arr[1].value
    EXPECT_EQ(nested.level2_arr[1].level3_arr[0].value, 888) << "Deeply nested array value should now be 888";

    // Find value in level2_arr[1].level3_arr
    int expected_index = rule_engine_memory_accessor_find_value(&nested, sizeof(nested), 52, sizeof(int), sizeof(int), 612, 0);
    EXPECT_EQ(expected_index, 1) << "Finding value 612 in level2_arr[1].level3_arr should succeed at index 1";
    // use the index to get the value
    rule_engine_memory_accessor_get_data(&nested, sizeof(nested), 52 + expected_index * sizeof(int), sizeof(int), &value);
    EXPECT_EQ(value, 612)
        << "Value at index 1 should be 612";  // nested.level2_arr[1].level3_arr[1].value
}

TEST_F(RuleEngineMemoryAccessor, deeplyNestedWithFlexibleArrays)
{
    int32_t value;

    // Define structures with flexible arrays - properly C99 compliant
    // In C99, a flexible array member must be the last element of a structure
    typedef struct deep_flex_level3
    {
        int value;
    } PACKED DEEP_FLEX_LEVEL3_T;

    typedef struct deep_flex_level2
    {
        DEEP_FLEX_LEVEL3_T level3;
        int mid_value;
    } PACKED DEEP_FLEX_LEVEL2_T;

    typedef struct deep_flex_root
    {
        DEEP_FLEX_LEVEL2_T level2;
        int top_value;
        int flex_arr[];  // Flexible array properly at the end of the root structure
    } PACKED DEEP_FLEX_ROOT_T;

    // Calculate sizes with flexible arrays
    size_t flex_count = 3;
    size_t root_size = sizeof(DEEP_FLEX_ROOT_T) + (flex_count * sizeof(int));

    // Allocate memory for the flexible structure
    DEEP_FLEX_ROOT_T *flex_struct = (DEEP_FLEX_ROOT_T *)hal_mem_malloc(root_size, HAL_MEM_INTERNAL_RAM);
    ASSERT_TRUE(flex_struct != NULL) << "Memory allocation for flexible structure failed";

    // Initialize the structure
    flex_struct->level2.level3.value = 100;
    flex_struct->level2.mid_value = 200;
    flex_struct->top_value = 300;

    // Initialize flexible array (properly at the end of the structure)
    flex_struct->flex_arr[0] = 400;
    flex_struct->flex_arr[1] = 401;
    flex_struct->flex_arr[2] = 402;

    // Test reading values - using explicit memory addresses with comments about what they refer to
    rule_engine_memory_accessor_get_data(flex_struct, root_size, 0, sizeof(int), &value);
    EXPECT_EQ(value, 100)
        << "Level 3 value should be 100";  // flex_struct->level2.level3.value at offset 0

    rule_engine_memory_accessor_get_data(flex_struct, root_size, 4, sizeof(int), &value);
    EXPECT_EQ(value, 200)
        << "Mid value should be 200";  // flex_struct->level2.mid_value at offset 4

    rule_engine_memory_accessor_get_data(flex_struct, root_size, 8, sizeof(int), &value);
    EXPECT_EQ(value, 300)
        << "Top value should be 300";  // flex_struct->top_value at offset 8

    // Test flexible array values with explicit memory addresses
    // The flexible array is now properly at the end of the structure
    rule_engine_memory_accessor_get_data(flex_struct, root_size, 12, sizeof(int), &value);
    EXPECT_EQ(value, 400)
        << "Flexible array value at index 0 should be 400";  // flex_struct->flex_arr[0] at offset 12

    rule_engine_memory_accessor_get_data(flex_struct, root_size, 16, sizeof(int), &value);
    EXPECT_EQ(value, 401)
        << "Flexible array value at index 1 should be 401";  // flex_struct->flex_arr[1] at offset 16

    rule_engine_memory_accessor_get_data(flex_struct, root_size, 20, sizeof(int), &value);
    EXPECT_EQ(value, 402)
        << "Flexible array value at index 2 should be 402";  // flex_struct->flex_arr[2] at offset 20

    // Test setting values in the flexible array
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flex_struct, root_size, 16, sizeof(int), 999), 1)
        << "Setting flexible array value should succeed";
    EXPECT_EQ(flex_struct->flex_arr[1], 999) << "Flexible array value at index 1 should now be 999";

    // Out of bounds access beyond the allocated flexible array
    int beyond_bounds_offset = 12 + (flex_count * sizeof(int));  // Offset to element beyond array bounds
    EXPECT_EQ(rule_engine_memory_accessor_get_data(flex_struct, root_size, beyond_bounds_offset, sizeof(int), &value), -1)
        << "Access beyond flexible array bounds should return -1";
    EXPECT_EQ(rule_engine_memory_accessor_set_data(flex_struct, root_size, beyond_bounds_offset, sizeof(int), 1234), -1)
        << "Setting beyond flexible array bounds should return -1";

    // Get flexible array length
    size_t flex_length = rule_engine_memory_accessor_get_array_length(root_size, 12, sizeof(int));
    EXPECT_EQ(flex_length, flex_count) << "Flexible array length should be " << flex_count;

    // Clean up
    hal_mem_free(flex_struct);
}
