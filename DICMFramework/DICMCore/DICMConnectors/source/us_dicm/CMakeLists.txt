cmake_minimum_required(VERSION 3.5)

if(BUILD_TESTING)
    include(unittest_setup)
endif()

project(DICMConnectorUs_Dicm)

message(STATUS "Adding US DICM Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

if (BUILD_TESTING)
    # Add mock source files when testing
    list(APPEND SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/mock_esp_ota.c)
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

if (BUILD_TESTING)
    # Setup Unittest target and coverage target
    setup_tests(DICMFramework)
    setup_coverage()
    
    # Add test directory to include path for mock headers
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
endif()

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMBroker DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs PUBLIC idf::json idf::freertos)

if (NOT ${IDF_TARGET} STREQUAL "linux")
    target_link_libraries(${PROJECT_NAME} PUBLIC idf::app_update)
endif()

if (CONFIG_BUILD_HMI_DATA)
    target_link_libraries(${PROJECT_NAME} PRIVATE DICMConnectorHmi)
endif()