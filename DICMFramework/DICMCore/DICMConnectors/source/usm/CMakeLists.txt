cmake_minimum_required(VERSION 3.5)

project(DICMConnectorUsm)

message(STATUS "Adding USM Connector (${PROJECT_NAME})")

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
if (CONFIG_CONNECTOR_WIFI)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMConnectorWifi)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE DICMBroker DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs idf::nvs_flash PUBLIC idf::freertos)
if (CONFIG_CONNECTOR_LINDEV)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMConnectorLinDev)
endif()