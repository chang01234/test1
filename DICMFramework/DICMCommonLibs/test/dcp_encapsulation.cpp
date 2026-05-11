#include <gtest/gtest.h>

extern "C" {
#include "dcp_encapsulation.h"
}

#include "DICMFrameworkTestFixture.hpp"

TEST(DCPEncapsulation, test_dcp)
{
    EXPECT_TRUE(0 != dcp_encapsulation_init());
}
