#include <gtest/gtest.h>

extern "C" {
#include "ddm2.h"
}

#include "DICMFrameworkTestFixture.hpp"

TEST(DDM2, test_ddmp2_create_publish)
{
    uint8_t data[16] = { 0x00, 0x01, 0x02, 0x03,
                         0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0A, 0x0B,
                         0x0C, 0x0D, 0x0E, 0x0F };
    DDMP2_FRAME publish_frame;

    EXPECT_EQ(1, ddmp2_create_publish(&publish_frame, 0, data, sizeof(data), 0));
    EXPECT_TRUE(0 == ::memcmp(&publish_frame.frame.publish.value, data, sizeof(data)));
}

TEST(DDM2, test_ddmp2_create_fragment)
{
    uint8_t value[DDMP2_MAX_FRAGMENT_VALUE_SIZE] = { 0xAA };
    DDMP2_FRAME fragment_frame;

    EXPECT_EQ(1, ddmp2_create_fragment(&fragment_frame, 0, value, DDMP2_MAX_FRAGMENT_VALUE_SIZE, 0));
    EXPECT_TRUE(0 == ::memcmp(fragment_frame.frame.fragment.value, value, DDMP2_MAX_FRAGMENT_VALUE_SIZE));
}
