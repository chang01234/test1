cmake_minimum_required(VERSION 3.5)

project(DICMConnectorLinServer)

message(STATUS "Adding LIN Server Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c" "source/devices/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/devices>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMConnectorLinCommon DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker PUBLIC idf::freertos)