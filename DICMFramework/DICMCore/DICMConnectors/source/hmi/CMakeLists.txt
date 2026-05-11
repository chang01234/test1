cmake_minimum_required(VERSION 3.5)

project(DICMConnectorHmi)

message(STATUS "Adding HMI Connector (${PROJECT_NAME})")

set(hmi_code_sources "")
foreach(hmi_code_source IN LISTS EXTRA_HMI_CODE_SOURCE_DIRS)
	message(STATUS "Adding HMI code source dirs: ${hmi_code_source}")
	list (APPEND hmi_code_sources ${hmi_code_source}/*.c)
endforeach()

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c" "source/hmi/*.c" ${hmi_code_sources})

#if(EXCLUDE_HMI_DATA_SOURCES)
#    foreach(src ${EXCLUDE_HMI_DATA_SOURCES})
#        get_filename_component(src "${src}" ABSOLUTE)
#        list(REMOVE_ITEM SRC_FILES "${src}")
#    endforeach()
#endif()

if (NOT CONFIG_HMI_NO_USE_ULP)
    set (ULP_LIB idf::ulp)
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/hmi>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                                ${EXTRA_HMI_CODE_INCLUDE_DIRS}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/hmi>
                                ${HMI_DATA_INCLUDE_DIRS}
							)

target_link_libraries(${PROJECT_NAME} INTERFACE	idf::soc ${ULP_LIB} idf::driver)
if (IDF_VER_5)
target_link_libraries(${PROJECT_NAME} PUBLIC idf::esp_partition PRIVATE idf::freertos ${ULP_LIB} idf::esp_partition idf::esp_adc idf::nvs_flash DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker)
else()
target_link_libraries(${PROJECT_NAME} PRIVATE idf::freertos ${ULP_LIB} idf::esp_adc_cal idf::nvs_flash DICMHal DICMFrameworkConfiguration DICMGeneralDefinitions DICMCommonLibs DICMBroker)
endif()
include(hmi_versions.cmake RESULT_VARIABLE version_file_found)
if (${version_file_found} STREQUAL "NOTFOUND")
    message (FATAL_ERROR "Did not find file: hmi_versions.cmake")
else()
    message (STATUS "Found file: ${version_file_found}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HMI_DATA_INTERFACE_VERSION_MAJOR=${HMI_DATA_INTERFACE_VERSION_MAJOR})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HMI_DATA_INTERFACE_VERSION_MINOR=${HMI_DATA_INTERFACE_VERSION_MINOR})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HMI_DATA_INTERFACE_VERSION_BUILD=${HMI_DATA_INTERFACE_VERSION_BUILD})
endif()


if (NOT CONFIG_HMI_NO_USE_ULP)
    # Add ULP
    set(ulp_app_name ulp_hmi)
    set(ulp_s_sources source/hmi/ulp/ulp.S)
    set(ulp_exp_dep_srcs "source/hmi/ulp.c")
    set (COMPONENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set (COMPONENT_LIB ${PROJECT_NAME})
    set (COMPONENT_TARGET ${PROJECT_NAME})

    ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")
endif()
