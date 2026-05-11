#include <algorithm>  // for std::min
#include <cstdlib>    // for rand, srand
#include <cstring>    // for memcmp

extern "C" {
#include "ringbuffer2.h"
}

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

#define RINGBUFFER_SIZE 16
#define RINGBUFFER_CAPACITY (RINGBUFFER_SIZE - 1)
#define DATA_SIZE       25600

#define RAND_RANGE(min, max) ((rand() % ((max) - (min) + 1)) + (min))  //!< \~ Random number in range

class RingbufferTest : public Test
{
  protected:
    void SetUp() override
    {
        rb = nullptr;
    }

    void TearDown() override
    {
        if (rb != nullptr)
        {
            ringbuffer2_destroy(rb);
            rb = nullptr;
        }
    }

    // Helper method to create a ringbuffer
    void CreateBuffer(size_t size = RINGBUFFER_SIZE)
    {
        ASSERT_TRUE(ringbuffer2_create(&rb, size, nullptr, nullptr));
        ASSERT_NE(rb, nullptr);
    }

    // Helper method to fill buffer to capacity
    void FillBuffer()
    {
        CreateBuffer();

        // Ring buffer capacity is size - 1, so for RINGBUFFER_SIZE=16, capacity is 15
        const uint8_t Fill_data[RINGBUFFER_CAPACITY] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

        ASSERT_TRUE(ringbuffer2_write(rb, Fill_data, RINGBUFFER_CAPACITY));
    }

    RINGBUFFER2 *rb;
    static constexpr size_t TEST_RINGBUFFER_SIZE = RINGBUFFER_SIZE;
    static constexpr size_t TEST_DATA_SIZE = DATA_SIZE;
};

TEST_F(RingbufferTest, comprehensive_read_write_test)
{
    static uint8_t Write_data[TEST_DATA_SIZE];
    uint8_t read_data[TEST_RINGBUFFER_SIZE];
    size_t test_data_read_index = 0;
    size_t test_data_write_index = 0;

    srand(12345);  // Fixed seed for reproducible tests

    // Initialize test data with sequential pattern
    for (unsigned int u = 0; u < TEST_DATA_SIZE; u++)
    {
        Write_data[u] = static_cast<uint8_t>(u);
    }

    CreateBuffer();

    while (test_data_read_index < TEST_DATA_SIZE)
    {
        const size_t Write_size = std::min(TEST_DATA_SIZE - test_data_write_index,
                                           static_cast<size_t>(RAND_RANGE(0, TEST_RINGBUFFER_SIZE - 1)));

        if (ringbuffer2_write(rb, &Write_data[test_data_write_index], Write_size))
        {
            test_data_write_index += Write_size;
        }

        const size_t Read_size = RAND_RANGE(0, TEST_RINGBUFFER_SIZE - 1);
        const size_t Read_bytes = ringbuffer2_read(rb, read_data, Read_size);

        ASSERT_EQ(memcmp(&Write_data[test_data_read_index], read_data, Read_bytes), 0)
            << "Read data mismatch at index " << test_data_read_index
            << " with " << Read_bytes << " bytes";

        test_data_read_index += Read_bytes;
    }
}

// Edge case tests with specific setups

TEST_F(RingbufferTest, read_from_empty_buffer)
{
    CreateBuffer();

    uint8_t read_data[TEST_RINGBUFFER_SIZE];

    // Try to read from empty buffer - should return 0 bytes
    size_t bytes_read = ringbuffer2_read(rb, read_data, TEST_RINGBUFFER_SIZE);
    EXPECT_EQ(bytes_read, 0) << "Reading from empty buffer should return 0 bytes";

    // Try to read smaller amount
    bytes_read = ringbuffer2_read(rb, read_data, 1);
    EXPECT_EQ(bytes_read, 0) << "Reading 1 byte from empty buffer should return 0 bytes";
}

TEST_F(RingbufferTest, write_to_full_buffer)
{
    FillBuffer();  // Buffer is now at capacity

    const uint8_t Extra_data[] = {0xaa, 0xbb, 0xcc};

    // Try to write to full buffer - should fail gracefully
    bool write_success = ringbuffer2_write(rb, Extra_data, sizeof(Extra_data));
    EXPECT_FALSE(write_success) << "Writing to full buffer should fail";

    // Try to write single byte to full buffer
    write_success = ringbuffer2_write(rb, Extra_data, 1);
    EXPECT_FALSE(write_success) << "Writing 1 byte to full buffer should fail";
}

TEST_F(RingbufferTest, boundary_conditions)
{
    CreateBuffer();

    const uint8_t Test_data[] = {1, 2, 3, 4, 5};
    uint8_t read_data[TEST_RINGBUFFER_SIZE];

    // Test writing maximum capacity (buffer size - 1)
    const size_t Max_capacity = TEST_RINGBUFFER_SIZE - 1;
    uint8_t max_data[Max_capacity];
    for (size_t i = 0; i < Max_capacity; i++)
    {
        max_data[i] = static_cast<uint8_t>(i + 1);
    }
    
    ASSERT_TRUE(ringbuffer2_write(rb, max_data, Max_capacity));

    // Test reading maximum capacity
    const size_t Bytes_read = ringbuffer2_read(rb, read_data, Max_capacity);
    EXPECT_EQ(Bytes_read, Max_capacity);

    // Verify data integrity
    EXPECT_EQ(memcmp(max_data, read_data, Max_capacity), 0);
}

TEST_F(RingbufferTest, zero_size_operations)
{
    CreateBuffer();

    const uint8_t Test_data[] = {1, 2, 3};
    uint8_t read_data[TEST_RINGBUFFER_SIZE];

    // Test that 0-byte operations are permitted and succeed

    const bool Write_success = ringbuffer2_write(rb, Test_data, 0);
    EXPECT_TRUE(Write_success) << "Writing 0 bytes should succeed";

    size_t bytes_read = ringbuffer2_read(rb, read_data, 0);
    EXPECT_EQ(bytes_read, 0) << "Reading 0 bytes should return 0 bytes read";

    // Verify that 0-byte write doesn't affect buffer state
    const uint8_t Single_byte = 0x42;
    ASSERT_TRUE(ringbuffer2_write(rb, &Single_byte, 1));

    bytes_read = ringbuffer2_read(rb, read_data, 1);
    EXPECT_EQ(bytes_read, 1);
    EXPECT_EQ(read_data[0], 0x42) << "Buffer state should be unaffected by 0-byte operations";
}
