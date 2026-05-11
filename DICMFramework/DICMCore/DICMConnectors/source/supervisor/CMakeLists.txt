cmake_minimum_required(VERSION 3.5)

project(DICMConnectorSupervisor)

message(STATUS "Adding Supervisor Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

#Add rules folder to FS image
add_to_file_system_image(rules)

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMRuleEngineLibrary DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker PUBLIC idf::freertos)
