# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Get git tag-commit as version string
git_describe(plat_repo_version "${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "Building on DICMFramework tag: ${plat_repo_version}")

# Check for DICMFramework defaults include file with configurations
include(FrameworkDefaults RESULT_VARIABLE result)
if (${result} STREQUAL "NOTFOUND")
    message (FATAL_ERROR "Did not find file: ${result}")
else()
    message (STATUS "Found file: ${result}")
endif()
add_compile_definitions(${EXTRA_COMPILER_DEFINITIONS})
add_compile_options(${EXTRA_COMPILER_OPTIONS})

project(DICMFramework)

# Add DICM Framework include. 
set(DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_subdirectory(DICMCore)
add_subdirectory(DICMCommonLibs)
add_subdirectory(DICMGeneralDefinitions)
add_subdirectory(DICMHal)
add_subdirectory(DICMOsal)

if (CONFIG_LOG_SERVICE)
list(APPEND EXTRA_LIBS DICMLogService)
add_subdirectory(source/log_service)
endif()
if (CONFIG_POWER_MANAGEMENT)
list(APPEND EXTRA_LIBS DICMPowerManagement)
add_subdirectory(source/pm)
endif()


file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

if (${IDF_TARGET} STREQUAL "linux")
add_subdirectory(components/mocks)
foreach(src_file ${SRC_FILES})
    #message(STATUS "src_file ${src_file}")
    cmake_path(GET src_file FILENAME filename)
    if (NOT ${filename} STREQUAL "rtc_watchdog.c")
        list(APPEND SOURCE_FILES "${src_file}")
    endif()
endforeach()
else()
set(SOURCE_FILES ${SRC_FILES})
endif()

add_library(${PROJECT_NAME} STATIC OBJECT ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            )
if (IDF_VER_5)
if (${IDF_TARGET} STREQUAL "linux")
    target_link_libraries(${PROJECT_NAME} PRIVATE idf::espressif__mdns PUBLIC idf::esp_netif idf::lwip idf::esp_app_format)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE idf::espressif__mdns PUBLIC idf::esp_psram idf::esp_phy idf::nvs_sec_provider)
endif()
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE idf::${IDF_TARGET} idf::mdns)
endif()

if (${IDF_TARGET} STREQUAL "linux")
target_link_libraries(${PROJECT_NAME} PRIVATE idf::json
                       PUBLIC ${EXTRA_LIBS} DICMBroker DICMConnectors DICMHal DICMOsal DICMCommonLibs DICMGeneralDefinitions DICMFrameworkConfiguration DICMFrameworkMocks idf::freertos idf::nvs_flash idf::spiffs idf::heap)
else()
target_link_libraries(${PROJECT_NAME} PRIVATE idf::json idf::esp_https_ota idf::app_update
                       PUBLIC ${EXTRA_LIBS} DICMBroker DICMConnectors DICMHal DICMOsal DICMCommonLibs DICMGeneralDefinitions DICMFrameworkConfiguration idf::freertos idf::nvs_flash idf::spiffs)
endif()

add_dependencies(${PROJECT_NAME} GetDICMGitVersion)
add_custom_target(GetDICMGitVersion
    COMMAND ${CMAKE_COMMAND}
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D INPUT_DICM_FILE=${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/GitDICMVersion.h.in
        -D OUTPUT_DICM_FILE=${CMAKE_BINARY_DIR}/config/DICMVersion.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/generate_dicmversion.cmake
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating DICMVersion.h"
    VERBATIM
)

if (CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH)
    if (NOT CONFIG_FS_VERSION)
        message(FATAL_ERROR "CONFIG_FS_VERSION needs to be set")
    endif()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/file_system_version.in ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH}/version)
endif()