cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
	include(unittest_setup)
	setup_unittest_mock_path(DICMCommonLibs unittest/mocks)
endif()

if (CONFIG_CONNECTOR_UART_DDM)
add_subdirectory(ddm)
endif()

project(DICMCommonLibs)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")
file(GLOB EDS_SRC_FILES CONFIGURE_DEPENDS "source/eds/source/*.c" "source/eds/source/sys/*.c" "source/port/eds_port/*.c")
file(GLOB DRIVERS_SRC_FILES CONFIGURE_DEPENDS "source/drivers/*.c")
file(GLOB DDM2_PARAM_SRC_FILES CONFIGURE_DEPENDS "ddm2/source/*.c")

set (BSEC_SRC_DIRS "")
set (BSEC_INC_DIRS "")
set (BSEC_SRC_FILES "")
if (USE_BSEC_2)
	list (APPEND BSEC_SRC_DIRS "source/drivers/bme68x/*.c")
	list (APPEND BSEC_INC_DIRS "include/drivers/bme68x" "include/bsec_2_x")
	file(GLOB BSEC_SRC_FILES CONFIGURE_DEPENDS ${BSEC_SRC_DIRS})
endif()
if (USE_BSEC_1)
    list (APPEND BSEC_SRC_DIRS "source/drivers/bme680/*.c")
	list (APPEND BSEC_INC_DIRS "include/drivers/bme680" "include/bsec_1_x")
	file(GLOB BSEC_SRC_FILES CONFIGURE_DEPENDS ${BSEC_SRC_DIRS})
endif()


add_library(${PROJECT_NAME} STATIC ${SRC_FILES} ${DDM2_PARAM_SRC_FILES} ${EDS_SRC_FILES} ${BSEC_SRC_FILES} ${DRIVERS_SRC_FILES})

if (BUILD_TESTING)
	setup_tests_no_wraps(DICMHal DICMHalMocks)
	setup_unit_tests(ddm2/include)
	setup_coverage()
endif()

#Need to in this order to find correct version of bsec include files 
if (USE_BSEC_2 OR USE_BSEC_1)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:/${BSEC_INC_DIRS}>)
endif()

target_include_directories(${PROJECT_NAME} 
							PRIVATE
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/eds/source/sys>
								${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
							PUBLIC
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ddm2/include>
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/eds/source>
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/port>
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/drivers>
						   )

target_link_libraries(${PROJECT_NAME} PRIVATE DICMHal DICMGeneralDefinitions DICMOsal idf::freertos)

if (USE_BSEC_1)
target_link_libraries(${PROJECT_NAME} PUBLIC "-L${CMAKE_CURRENT_SOURCE_DIR}/lib/bsec_1_x")
target_link_libraries(${PROJECT_NAME} PUBLIC algobsec)
endif()

if (USE_BSEC_2)
target_link_libraries(${PROJECT_NAME} PUBLIC "-L${CMAKE_CURRENT_SOURCE_DIR}/lib/bsec_2_x")
target_link_libraries(${PROJECT_NAME} PUBLIC algobsec)
endif()

if (IDF_VER_5)
set_source_files_properties(source/ame_calibration_xfer.c PROPERTIES COMPILE_OPTIONS "-Wno-strict-aliasing;-Wno-uninitialized")
set_source_files_properties(ddm2/source/ddm2_parameter_list.c PROPERTIES COMPILE_OPTIONS "-Wno-char-subscripts")
else()
set_source_files_properties(source/ame_calibration_xfer.c PROPERTIES COMPILE_OPTIONS "-Wno-strict-aliasing")
endif()
if (CONFIG_CONNECTOR_UART_DDM)
target_link_libraries(${PROJECT_NAME} PUBLIC DICMCommonLibsDDM)
endif()
