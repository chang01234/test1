#include <gtest/gtest.h>

extern "C" {
#include "ddm_entry.h"
}

#include "DICMFrameworkTestFixture.hpp"

TEST(DDMEntry, test_ddm_entry_other)
{
    uint8_t data[128] = {0};
    struct ddm_entry entry;

    EXPECT_EQ(0, ddm_entry__init(&entry, GW0AWSSC));
    EXPECT_EQ(true, ddm_entry__set__value(&entry, data, sizeof(data)));
    EXPECT_EQ(false, ddm_entry__set__value(&entry, data, sizeof(data)));
    memset(data, 1, sizeof(data));
    EXPECT_EQ(true, ddm_entry__set__value(&entry, data, sizeof(data)));
}

TEST(DDMEntry, test_ddm_entry_other_memcpy_overlap)
{
    uint8_t data[128] = {0};
    struct ddm_entry entry;

    EXPECT_EQ(0, ddm_entry__init(&entry, GW0AWSSC));
    EXPECT_EQ(true, ddm_entry__set__value(&entry, data, sizeof(data)));
    EXPECT_EQ(true, ddm_entry__set__value(&entry, entry.p__value.storage.other,
                                          entry.p__value.size - 1));
}

TEST(DDMEntry, test_ddm_entry_struct)
{
    uint8_t data[128] = {0};
    struct ddm_entry entry;

    EXPECT_EQ(0, ddm_entry__init(&entry, GW0INV));
    EXPECT_EQ(true, ddm_entry__set__value_struct(&entry, data, sizeof(data)));
    EXPECT_EQ(false, ddm_entry__set__value_struct(&entry, data, sizeof(data)));
    memset(data, 1, sizeof(data));
    EXPECT_EQ(true, ddm_entry__set__value_struct(&entry, data, sizeof(data)));
}

TEST(DDMEntry, test_ddm_entry_struct_memcpy_overlap)
{
    uint8_t data[128] = {0};
    struct ddm_entry entry;

    EXPECT_EQ(0, ddm_entry__init(&entry, GW0INV));
    EXPECT_EQ(true, ddm_entry__set__value_struct(&entry, data, sizeof(data)));
    EXPECT_EQ(true, ddm_entry__set__value_struct(&entry, entry.p__value.storage.structure,
                                                entry.p__value.size - 1));
}
