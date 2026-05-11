cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
    include(unittest_setup)
endif()

project(DICMRuleEngineLibrary)

message(STATUS "Adding Rule Engine Library (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

if (BUILD_TESTING)
    # Setup Unittest target and coverage target
    setup_tests(DICMFramework)
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

if (IDF_VER_5)
set_source_files_properties(source/rule_engine_exp_eval.c PROPERTIES COMPILE_OPTIONS "-Wno-char-subscripts")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC DICMConnectorSystem PRIVATE DICMFrameworkConfiguration DICMGeneralDefinitions DICMHal DICMCommonLibs DICMBroker idf::json PUBLIC idf::freertos)
