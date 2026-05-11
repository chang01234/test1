cmake_minimum_required(VERSION 3.5)

project(DICMConnectorRs485Master)

message(STATUS "Adding RS485 Master Connector (${PROJECT_NAME})")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c" "source/device/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} 
                            PRIVATE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/device>
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} 
                        PRIVATE 
                            DICMHal 
                            DICMFrameworkConfiguration 
                            DICMGeneralDefinitions 
                            DICMCommonLibs 
                            DICMBroker
                            idf::espressif__esp-modbus
                        PUBLIC 
                            idf::freertos)

message(STATUS "Modbus RTU frame header wrapper enabled (0xFA) at project level")
# Use target_link_options with PUBLIC to ensure linker options propagate to targets that link this library
target_link_options(${PROJECT_NAME} PUBLIC
    "-Wl,--wrap=mb_port_ser_send_data"
    "-Wl,--wrap=mb_port_ser_recv_data"
    "-Wl,--undefined=__wrap_mb_port_ser_send_data"
    "-Wl,--undefined=__wrap_mb_port_ser_recv_data"
)
