cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
    include(unittest_setup)
endif()

project(DICMCommonLibsDDM)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.c")
list(FILTER SRC_FILES EXCLUDE REGEX "json.c")

message(STATUS "${SRC_FILES}")

if(BUILD_TESTING)
    setup_tests_no_wraps(DICMHalMocks)
    setup_coverage()
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            )
target_link_libraries(${PROJECT_NAME} )
