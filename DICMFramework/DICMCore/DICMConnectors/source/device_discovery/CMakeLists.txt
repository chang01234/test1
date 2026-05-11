cmake_minimum_required(VERSION 3.5)

project(DICMConnectorDeviceDiscovery)

message(STATUS "Adding Device Discovery/mDNS Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker PUBLIC idf::freertos)
if (CONFIG_CONNECTOR_WIFI)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMConnectorWifi)
endif()
if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
target_link_libraries(${PROJECT_NAME} PUBLIC idf::esp_ringbuf idf::esp_netif)
endif()

