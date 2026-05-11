# Macro 'setup_tests'
# Setups unit tests to be built. Requires a directory named test to contain all Google Test source files

if (BUILD_TESTING)
include(CodeCoverage)
include(GoogleTest)
set(UNITEST_DISABLE_WRAPS FALSE)

macro(setup_tests_no_wraps)
    set(UNITEST_DISABLE_WRAPS TRUE)
    setup_tests(${ARGN})
endmacro()

macro(setup_tests)
    set(PROJECT_UNITTEST_NAME ${PROJECT_NAME}_UnitTest_)
    file(GLOB UNITTEST_CPP_FILES CONFIGURE_DEPENDS "test/*.cpp")
    file(GLOB UNITTEST_HPP_FILES CONFIGURE_DEPENDS "test/*.hpp")

    message(STATUS "Adding ${PROJECT_NAME} UnitTests (${PROJECT_UNITTEST_NAME})")

    append_coverage_compiler_flags()
    set(build_deps "")
    foreach(cpp_file IN LISTS UNITTEST_CPP_FILES)
        get_filename_component(FILE_NAME ${cpp_file} NAME_WE)
        add_executable(${PROJECT_UNITTEST_NAME}${FILE_NAME} ${cpp_file} ${UNITTEST_HPP_FILES})
        target_compile_features(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE cxx_std_17)
        target_include_directories(${PROJECT_UNITTEST_NAME}${FILE_NAME}
                                PRIVATE
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                    ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                    ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                                    ${CMAKE_SOURCE_DIR}/DICMFramework/test/include
                                PUBLIC
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                )
        if (NOT UNITEST_DISABLE_WRAPS)
            # Wrap broker/connector functions so we can hook and log into test class
            target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE "-Wl,--wrap=connector_send_frame_to_broker")
            target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE "-Wl,--wrap=connector_forward_frame_to_broker")
            target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE "-Wl,--wrap=connector_send_frame_to_connector")
            target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE "-Wl,--wrap=connector_forward_frame_to_connector")
        else()
            target_compile_definitions(${PROJECT_UNITTEST_NAME}${FILE_NAME} PUBLIC DISABLE_WRAP_FUNCTIONS)
        endif()
        target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PUBLIC ${PROJECT_NAME})
        target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PUBLIC GTest::gtest)
        if (${ARGC} GREATER_EQUAL 0)
            foreach(extra_lib ${ARGN})
                target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PUBLIC ${extra_lib})
            endforeach()
        endif()
        set(mapfile "${PROJECT_BINARY_DIR}/${PROJECT_UNITTEST_NAME}${FILE_NAME}.map")
        target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE "-Wl,--cref -Wl,--Map=${mapfile}")
        set_property(TARGET ${PROJECT_UNITTEST_NAME}${FILE_NAME} APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${mapfile}" )
        
        list(APPEND build_deps ${PROJECT_UNITTEST_NAME}${FILE_NAME})
        gtest_add_tests( TARGET ${PROJECT_UNITTEST_NAME}${FILE_NAME}
                        EXTRA_ARGS --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/
    #                    TEST_SUFFIX .noArgs
                        TEST_LIST   CTestsList
                        )
        set_tests_properties(${CTestsList} PROPERTIES RUN_SERIAL TRUE)
        set_tests_properties(${CTestsList} PROPERTIES LABELS ${PROJECT_NAME})
        set_tests_properties(${CTestsList} PROPERTIES TIMEOUT 10)
        set_tests_properties(${CTestsList} PROPERTIES FIXTURES_SETUP ${PROJECT_NAME})
    endforeach()
endmacro()

# Separate setup macro for building gmock unit tests on a file-by-file basis. Assumed that the test file is located in unittest directory 
# and named <source_file>_ut.cpp. This file will need to include any gmock dependencies directly in the cpp-file.
macro(setup_unit_tests)
    if(NOT PROJECT_UNITTEST_NAME)
        set(PROJECT_UNITTEST_NAME ${PROJECT_NAME}_UnitTest_)
    endif()
    file(GLOB UNITTEST_UT_CPP_FILES CONFIGURE_DEPENDS "unittest/*.cpp")
    file(GLOB UNITTEST_UT_HPP_FILES CONFIGURE_DEPENDS "unittest/*.hpp")
    append_coverage_compiler_flags()
    # Create an interface library mainly used when building mock library of module under test
    add_library(${PROJECT_NAME}API INTERFACE )
    target_include_directories(${PROJECT_NAME}API
                               INTERFACE
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include
                                )
    if (${ARGC} GREATER_EQUAL 0)
        foreach(extra_include ${ARGN})
            # Add extra include directories if required, i.e. module does not follow standard look or have nested include directories
            target_include_directories(${PROJECT_NAME}API INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/${extra_include})
        endforeach()
    endif()
    
    foreach(cpp_ut_file IN LISTS UNITTEST_UT_CPP_FILES)
        get_filename_component(FILE_NAME ${cpp_ut_file} NAME_WE)
        string(REPLACE "_ut" "" base_string ${FILE_NAME})
        add_executable(${PROJECT_UNITTEST_NAME}${FILE_NAME} ${cpp_ut_file} ${CMAKE_CURRENT_SOURCE_DIR}/source/${base_string}.c ${UNITTEST_UT_HPP_FILES})
        target_compile_features(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE cxx_std_17)
        target_include_directories(${PROJECT_UNITTEST_NAME}${FILE_NAME}
                                PRIVATE
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/unittest
                                )
        if (${ARGC} GREATER_EQUAL 0)
            foreach(extra_include ${ARGN})
                # Add extra include directories if required, i.e. module does not follow standard look or have nested include directories
                target_include_directories(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/${extra_include})
            endforeach()
        endif()
                                
        target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PUBLIC ${PROJECT_NAME}_UnittestMocks GTest::gmock_main)
        set(mapfile "${PROJECT_BINARY_DIR}/${PROJECT_UNITTEST_NAME}${FILE_NAME}.map")
        target_link_libraries(${PROJECT_UNITTEST_NAME}${FILE_NAME} PRIVATE "-Wl,--cref -Wl,--Map=${mapfile}")
        set_property(TARGET ${PROJECT_UNITTEST_NAME}${FILE_NAME} APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${mapfile}" )
        
        list(APPEND build_deps ${PROJECT_UNITTEST_NAME}${FILE_NAME})
        gtest_add_tests(TARGET ${PROJECT_UNITTEST_NAME}${FILE_NAME}
                        EXTRA_ARGS --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/
    #                    TEST_SUFFIX .noArgs
                        TEST_LIST   CUTTestsList
                        )
        set_tests_properties(${CUTTestsList} PROPERTIES RUN_SERIAL TRUE)
        set_tests_properties(${CUTTestsList} PROPERTIES LABELS ${PROJECT_NAME})
        set_tests_properties(${CUTTestsList} PROPERTIES TIMEOUT 10)
        set_tests_properties(${CUTTestsList} PROPERTIES FIXTURES_SETUP ${PROJECT_NAME})
    endforeach()
endmacro()

# Macro to collect all unit test targets as dependency to the "module" code coverage report target
macro(setup_coverage)
    setup_target_for_coverage_gcovr_html(
                    NAME ${PROJECT_NAME}_coverage                    # New target name
                    EXECUTABLE ctest --schedule-random -V -L ${PROJECT_NAME}          # Executable in PROJECT_BINARY_DIR
                    DEPENDENCIES ${PROJECT_NAME} ${build_deps}        # Dependencies to build first
                    BASE_DIRECTORY ${PROJECT_SOURCE_DIR}        # Base directory for report
                                                                #  (defaults to PROJECT_SOURCE_DIR)
                    EXCLUDE ".*/test/*" "test/*" ".*/unittest/*" "unittest/*" ".*/mocks/*" "/mocks/*"       # Patterns to exclude (can be relative
                                                        #  to BASE_DIRECTORY, with CMake 3.4+)
    )
endmacro()

# The following macros are used to setup path to a mocklibrary that is used when building unit tests
# Ex: setup_unittest_mock_path(DICMCommonLibs unittest/mocks)
macro(setup_unittest_mock_path)
    if (${ARGC} EQUAL 2)
        set(PARENT_PROJECT_NAME ${ARGV0})   # used to pass information about which module that are under test
        add_subdirectory(${ARGV1})          # relative path where mock library is located
        unset(PARENT_PROJECT_NAME)
    else()
        message(FATAL_ERROR "Not correct number of arguments: ${ARGC}")
    endif()
endmacro()
# setup_unittest_mock_path() is required to be called before this macro
macro(setup_unittest_mock_lib)
    # Create the mock library
    project(${PARENT_PROJECT_NAME}_UnittestMocks)

    file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.cpp")

    add_library(${PROJECT_NAME} STATIC ${SRC_FILES} )
    target_include_directories(${PROJECT_NAME} 
                               PUBLIC
                                  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                               )

    target_compile_features(${PROJECT_NAME}  PRIVATE cxx_std_17)
    target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gmock)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PARENT_PROJECT_NAME}API)    # Add interface library of our own library
endmacro()
endif() # BUILD_TESTING
