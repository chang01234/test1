cmake_minimum_required(VERSION 3.5)

project(DICMConnectorSmartEco)

message(STATUS "Adding Smart Eco (Rule Engine) Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")


add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
#Add rules folder to SPIFFS image
add_to_file_system_image(rules)

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PUBLIC DICMRuleEngineLibrary PRIVATE DICMConnectorSystem DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker idf::json PUBLIC idf::freertos)
