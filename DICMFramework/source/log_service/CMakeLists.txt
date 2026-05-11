cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
    include(unittest_setup)
endif()

project(DICMLogService)

message(STATUS "Adding DICM Log Service (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

if (BUILD_TESTING)
    # Setup Unittest target and coverage target
    setup_tests_no_wraps(DICMFramework)
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
target_link_libraries(${PROJECT_NAME} PRIVATE DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker PUBLIC idf::freertos)
if (CONFIG_LOG_SERVICE_WEB_SERVER)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMConnectorWifi idf::esp_http_server)
target_add_binary_data(${PROJECT_NAME} "html/weblog.html" TEXT)
target_add_binary_data(${PROJECT_NAME} "html/favicon.ico" TEXT)
endif()