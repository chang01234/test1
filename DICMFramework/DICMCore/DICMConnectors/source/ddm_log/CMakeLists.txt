cmake_minimum_required(VERSION 3.5)

project(DICMConnectorDdmlog)

message(STATUS "Adding DDM Log Connector (${PROJECT_NAME})")

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
if (IDF_VER_5)
set_source_files_properties(source/ddm_log_spec_parser.c PROPERTIES COMPILE_OPTIONS "-Wno-char-subscripts")
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE idf::spiffs DICMConnectorSystem DICMBroker DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs idf::console idf::json PUBLIC idf::freertos)
