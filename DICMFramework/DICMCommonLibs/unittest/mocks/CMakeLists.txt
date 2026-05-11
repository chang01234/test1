cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
    include(unittest_setup)
    setup_unittest_mock_lib()
endif()

