cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
    include(unittest_setup)
endif()
project(DICMConnectorUnitTest)

message(STATUS "Adding UnitTest Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

if (BUILD_TESTING)
    # Setup Unittest target and coverage target
    setup_tests(DICMFramework)
    setup_coverage()
endif()

target_include_directories(${PROJECT_NAME}
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            )
target_link_libraries(${PROJECT_NAME} PRIVATE DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker)
