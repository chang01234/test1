cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
    include(unittest_setup)
endif()

project(DICMConnectorWifi)

message(STATUS "Adding WIFI Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")
#set(CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT} -Wa,-alh=<OBJECT>.lst")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

if (BUILD_TESTING)
    # Setup Unittest target and coverage target
    setup_tests(DICMFramework DICMCommonLibs)
    setup_coverage()
endif()

if (BUILD_TESTING)
target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
else()
target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            )
endif()
if (CONFIG_CONNECTOR_TLS)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker DICMConnectorTLS PUBLIC idf::esp_netif idf::esp_event idf::mbedtls idf::esp_wifi idf::freertos idf::esp-tls idf::json)
else()
target_link_libraries(${PROJECT_NAME} PRIVATE DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker PUBLIC idf::esp_netif idf::esp_event idf::mbedtls idf::esp_wifi idf::freertos idf::json)
endif()
