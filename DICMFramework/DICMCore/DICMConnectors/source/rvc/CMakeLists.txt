cmake_minimum_required(VERSION 3.5)

project(DICMConnectorRvc)

message(STATUS "Adding Generic RVC Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")
file(GLOB DGN_SRC_FILES CONFIGURE_DEPENDS "source/dgn/*.c")
file(GLOB CONNECTOR_RVC_SRC_FILES CONFIGURE_DEPENDS "source/lib/*.c")
list(APPEND SRC_FILES ${DGN_SRC_FILES} ${CONNECTOR_RVC_SRC_FILES})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/dgn>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/lib>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE $<$<BOOL:${CONFIG_CONNECTOR_HMI}>:DICMConnectorHmi> DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker PUBLIC idf::freertos)
