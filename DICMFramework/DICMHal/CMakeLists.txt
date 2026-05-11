cmake_minimum_required(VERSION 3.5)

if (${IDF_TARGET} STREQUAL "linux")
    # DICMHalMocks
    add_subdirectory(extra_components/5.x/mocks)
if (BUILD_TESTING)
    include(unittest_setup)
endif()
endif()

project(DICMHal C CXX)

if (NOT ${IDF_TARGET} STREQUAL "linux")
    file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c" "source/esp32/*.c")
    file(GLOB DRV_SRC_FILES CONFIGURE_DEPENDS "source/esp32/hal_devices/*.c")
else()
    file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c" "source/linux/*.c*")
endif()
add_library(${PROJECT_NAME} STATIC ${SRC_FILES} ${DRV_SRC_FILES})

if (BUILD_TESTING)
    # Setup Unittest target and coverage target
    setup_tests_no_wraps()
    setup_coverage()    
endif()
#set(CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT} -Wa,-alh=<OBJECT>.lst")

if (NOT ${IDF_TARGET} STREQUAL "linux")
    target_include_directories(${PROJECT_NAME} 
    							PRIVATE
    								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/esp32>
    							PUBLIC
    								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/esp32/hal_devices>
    							)
else()
    target_include_directories(${PROJECT_NAME} 
                                PRIVATE
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/linux>
                                PUBLIC
                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                )
    target_compile_definitions(${PROJECT_NAME} PUBLIC "SOC_GPIO_VALID_OUTPUT_GPIO_MASK=0xFFFFFFFF")
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
    target_link_libraries(${PROJECT_NAME} PUBLIC DICMHalMocks)
endif()

if (NOT ${IDF_TARGET} STREQUAL "linux")
    target_link_libraries(${PROJECT_NAME} PUBLIC idf::driver)
    target_link_libraries(${PROJECT_NAME} PRIVATE idf::freertos idf::nvs_flash PUBLIC ${EXTRA_HAL_DEP_COMPONENTS})
    target_link_libraries(${PROJECT_NAME} PRIVATE DICMGeneralDefinitions DICMFrameworkConfiguration)
    if (IDF_VER_5)
        #target_compile_definitions(${PROJECT_NAME} PRIVATE "EXT_RAM_ATTR=EXT_RAM_BSS_ATTR")
        target_link_libraries(${PROJECT_NAME} PUBLIC idf::esp_timer)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE idf::${IDF_TARGET})
    endif()
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC idf::driver)
    target_link_libraries(${PROJECT_NAME} PRIVATE idf::freertos PUBLIC ${EXTRA_HAL_DEP_COMPONENTS})
    target_link_libraries(${PROJECT_NAME} PRIVATE DICMGeneralDefinitions DICMFrameworkConfiguration)
    if (IDF_VER_5)
        target_link_libraries(${PROJECT_NAME} PUBLIC idf::esp_timer)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE idf::${IDF_TARGET})
    endif()
    
endif()