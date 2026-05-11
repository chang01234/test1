cmake_minimum_required(VERSION 3.5)

project(DICMConnectorEventNotification)

message(STATUS "Adding Event Notification Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

# In order to add event types to the file system image, we need to create a custom target
# but not here, but do it in the DICMApplication project.
#Add rules folder to SPIFFS image
# add_to_file_system_image(event_types)

target_include_directories(${PROJECT_NAME}
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs $<$<BOOL:${CONFIG_CONNECTOR_MQTT}>:DICMConnectorMqtt> DICMBroker PUBLIC idf::freertos idf::json)
