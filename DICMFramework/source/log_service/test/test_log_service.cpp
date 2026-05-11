extern "C" {
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
int log_service_vprintf(const char *format, va_list varlist);
bool log_service_idle_cb(char *log_buffer, size_t log_buffer_size);
}
#include "DICMFrameworkTestFixture.hpp"

extern "C" int testFormatFunction(const char *format, ...)
{
    va_list list;
    va_start(list, format);

    int num = log_service_vprintf(format, list);
    va_end(list);
    return num;
}

class LogServiceTestFixture : public DICMFrameworkTestFixture
{
  protected:
    void SetUp() override
    {
        DICMFrameworkTestFixture::SetUp();
        DICMFrameworkTestFixture::SetupFramework();
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};

TEST_F(LogServiceTestFixture, testStringFormat)
{
    char log_buffer[1024];

    // Test normal format string
    int num = testFormatFunction("%s", "Testing");
    EXPECT_TRUE(num == strlen("Testing")) << "Should print correct number of characters " << strlen("Testing") << " " << num;
    memset(log_buffer, 0, sizeof(log_buffer));
    bool ret = log_service_idle_cb(log_buffer, 1024);
    EXPECT_FALSE(ret) << "log_service_idle_cb() should return false";
    EXPECT_TRUE(strcmp(log_buffer, "Testing") == 0) << "log_buffer should equal 'Testing': " << log_buffer;

    // Test right aligned minimum chars
    num = testFormatFunction("%20s", "Testing");
    EXPECT_TRUE(num == 20) << "Should print correct number of characters " << strlen("Testing") << " " << num;
    memset(log_buffer, 0, sizeof(log_buffer));
    ret = log_service_idle_cb(log_buffer, 1024);
    EXPECT_FALSE(ret) << "log_service_idle_cb() should return false";
    EXPECT_TRUE(strcmp(log_buffer, "             Testing") == 0) << "log_buffer should equal '             Testing': " << log_buffer;

    // Test right aligned max chars (limiting string)
    num = testFormatFunction("%.10s", "Longer Testing");
    EXPECT_TRUE(num == 10) << "Should print correct number of characters 10 is " << num;
    memset(log_buffer, 0, sizeof(log_buffer));
    ret = log_service_idle_cb(log_buffer, 1024);
    EXPECT_FALSE(ret) << "log_service_idle_cb() should return false";
    EXPECT_TRUE(strcmp(log_buffer, "Longer Tes") == 0) << "log_buffer should equal 'Longer Tes': " << log_buffer;

    // Test right aligned max chars (limit longer than string)
    num = testFormatFunction("%.20s", "Longer Testing");
    EXPECT_TRUE(num == 14) << "Should print correct number of characters 14 is " << num;
    memset(log_buffer, 0, sizeof(log_buffer));
    ret = log_service_idle_cb(log_buffer, 1024);
    EXPECT_FALSE(ret) << "log_service_idle_cb() should return false";
    EXPECT_TRUE(strcmp(log_buffer, "Longer Testing") == 0) << "log_buffer should equal 'Longer Testing': " << log_buffer;

    // Test left aligned minimum chars
    num = testFormatFunction("%-20s", "Testing");
    EXPECT_TRUE(num == 20) << "Should print correct number of characters 20 is " << num;
    memset(log_buffer, 0, sizeof(log_buffer));
    ret = log_service_idle_cb(log_buffer, 1024);
    EXPECT_FALSE(ret) << "log_service_idle_cb() should return false";
    EXPECT_TRUE(strcmp(log_buffer, "Testing             ") == 0) << "log_buffer should equal 'Testing             ': " << log_buffer;

    // Test left aligned max chars
    num = testFormatFunction("%-.20s", "Even Longer and longer Testing");
    EXPECT_TRUE(num == 20) << "Should print correct number of characters 20 is " << num;
    memset(log_buffer, 0, sizeof(log_buffer));
    ret = log_service_idle_cb(log_buffer, 1024);
    EXPECT_FALSE(ret) << "log_service_idle_cb() should return false";
    EXPECT_TRUE(strcmp(log_buffer, "Even Longer and long") == 0) << "log_buffer should equal 'Even Longer and long': " << log_buffer;
}
