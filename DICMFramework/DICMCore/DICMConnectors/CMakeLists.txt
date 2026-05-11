cmake_minimum_required(VERSION 3.5)

if (BUILD_TESTING)
    include(unittest_setup)
endif()

project(DICMConnectors)

set (CONFIG_DICM_SUPPORT_CAN_CALIBRATION_PROTOCOL 1)

set(DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/source" )

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c")

set (BSEC_INC_DIRS "")
set (BSEC_SRC_FILES "")

set (BSEC_INTEGRATION_INC "")

if (USE_BSEC_2)
	set (BSEC_SRC_DIRS "source/bsec/bsec_2_x/*.c")
	list (APPEND BSEC_INC_DIRS "source/bsec/bsec_2_x")
	file(GLOB BSEC_SRC_FILES CONFIGURE_DEPENDS ${BSEC_SRC_DIRS})
	list (APPEND DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/${BSEC_INC_DIRS}")
endif()
if (USE_BSEC_1)
	list (APPEND BSEC_INTEGRATION_SRC "source/bsec/bsec_1_x/bsec_integration.c")
	list (APPEND BSEC_INTEGRATION_INC "source/bsec/bsec_1_x")
endif()


###############################################################################
# Add Core connectors to build
###############################################################################
if (CONFIG_CONNECTOR_WIFI)
list(APPEND ConnectorLIBS DICMConnectorWifi)
add_subdirectory(source/wifi)
endif()

if (CONFIG_CONNECTOR_MQTT)
list(APPEND ConnectorLIBS DICMConnectorMqtt)
add_subdirectory(source/mqtt)
endif()

if (CONFIG_CONNECTOR_TLS)
list(APPEND ConnectorLIBS DICMConnectorTLS)
add_subdirectory(source/tls)
endif()

if (CONFIG_CONNECTOR_BLE)
list(APPEND ConnectorLIBS DICMConnectorBle)
add_subdirectory(source/ble)
endif()

if (CONFIG_CONNECTOR_USM)
list(APPEND ConnectorLIBS DICMConnectorUsm)
add_subdirectory(source/usm)
endif()

if (CONFIG_CONNECTOR_US_DICM)
list(APPEND ConnectorLIBS DICMConnectorUs_Dicm)
add_subdirectory(source/us_dicm)
endif()

if (CONFIG_CONNECTOR_UART)
list(APPEND ConnectorLIBS DICMConnectorUart)
add_subdirectory(source/uart)
endif()

if (CONFIG_CONNECTOR_UART_DDM)
list(APPEND ConnectorLIBS DICMConnectorUartDdm)
add_subdirectory(source/uart_ddm)
endif()

if (CONFIG_CONNECTOR_SYSTEM)
list(APPEND ConnectorLIBS DICMConnectorSystem)
add_subdirectory(source/system)
endif()

if (CONFIG_CONNECTOR_SYSAPPL_SERVICE)
list(APPEND ConnectorLIBS DICMConnectorSysApplService)
add_subdirectory(source/sysappl_service)
endif()

if (CONFIG_CONNECTOR_BATT_MGMT_SERVICE)
list(APPEND ConnectorLIBS DICMConnectorBattMgmtService)
add_subdirectory(source/battery_management_service)
endif()

if (CONFIG_CONNECTOR_IC_MGMT_SERVICE)
list(APPEND ConnectorLIBS DICMConnectorICMgmtService)
add_subdirectory(source/inverter_charger_management_service)
endif()

if (CONFIG_CONNECTOR_PREM_RVC)
list(APPEND ConnectorLIBS DICMConnectorPrem_Rvc)
add_subdirectory(source/prem_rvc)
endif()
if (CONFIG_CONNECTOR_RVC)
list(APPEND ConnectorLIBS DICMConnectorRvc)
add_subdirectory(source/rvc)
endif()

if (CONFIG_CONNECTOR_LINDEV OR CONFIG_CONNECTOR_LIN_SERVER)
list(APPEND ConnectorLIBS DICMConnectorLinCommon)
add_subdirectory(source/lin_common)
endif()

if (CONFIG_CONNECTOR_LINDEV)
list(APPEND ConnectorLIBS DICMConnectorLinDev)
add_subdirectory(source/lin_dev)
endif()

if (CONFIG_CONNECTOR_LIN_SERVER)
list(APPEND ConnectorLIBS DICMConnectorLinServer)
add_subdirectory(source/lin_server)
endif()

if (CONFIG_CONNECTOR_HMI)
list(APPEND ConnectorLIBS DICMConnectorHmi)
add_subdirectory(source/hmi)
endif()

if (CONFIG_CONNECTOR_DEVICE_DISCOVERY)
list(APPEND ConnectorLIBS DICMConnectorDeviceDiscovery)
add_subdirectory(source/device_discovery)
endif()

if (CONFIG_CONNECTOR_CLIMATE_ZONE_FEATURE)
list(APPEND ConnectorLIBS DICMConnectorClimateZoneFeature)
add_subdirectory(source/feature_climate_zone)
endif()

if (CONFIG_CONNECTOR_SMART_ECO_FEATURE)
list(APPEND ConnectorLIBS DICMConnectorSmartEcoFeature)
add_subdirectory(source/feature_smart_eco)
endif()

if (CONFIG_CONNECTOR_FEATURE_MGR)
list(APPEND ConnectorLIBS DICMConnectorFeatureMgr)
add_subdirectory(source/feature_mgr)
endif()

if (CONFIG_CONNECTOR_PROD_SNIFFER_SERVICE)
list(APPEND ConnectorLIBS DICMConnectorProdSnifferService)
add_subdirectory(source/prod_sniffer)
endif()

if (CONFIG_CONNECTOR_EVENT_NOTIFICATION)
list(APPEND ConnectorLIBS DICMConnectorEventNotification)
add_subdirectory(source/event_notification)
endif()

if (CONFIG_CONNECTOR_DDM_LOG)
list(APPEND ConnectorLIBS DICMConnectorDdmlog)
add_subdirectory(source/ddm_log)
endif()

if (CONFIG_CONNECTOR_FULL_CLIMATE_RULE_ENGINE)
list(APPEND ConnectorLIBS DICMConnectorFCRuleEngine)
add_subdirectory(source/fc_rule_engine)
endif()

if (CONFIG_CONNECTOR_SMART_ECO_RULE_ENGINE)
list(APPEND ConnectorLIBS DICMConnectorSmartEco)
add_subdirectory(source/smart_eco_rule_engine)
endif()

if (CONFIG_CONNECTOR_NFC)
list(APPEND ConnectorLIBS DICMConnectorNfc)
add_subdirectory(source/nfc)
endif()

if (CONFIG_CONNECTOR_NMEA2K)
list(APPEND ConnectorLIBS DICMConnectorNmea2k)
add_subdirectory(source/nmea2k)
endif()

# Add all connectors/services that depend on Rule Engine library here
if (CONFIG_CONNECTOR_FULL_CLIMATE_RULE_ENGINE OR CONFIG_CONNECTOR_SUPERVISOR OR CONFIG_CONNECTOR_SMART_ECO_RULE_ENGINE)
list(APPEND ConnectorLIBS DICMRuleEngineLibrary)
add_subdirectory(source/rule_engine_lib)
endif()

if (CONFIG_CONNECTOR_SUPERVISOR)
list(APPEND ConnectorLIBS DICMConnectorSupervisor)
add_subdirectory(source/supervisor)
endif()

if (CONFIG_CONNECTOR_RCS)
list(APPEND ConnectorLIBS DICMConnectorRcs)
add_subdirectory(source/rcs)
endif()

if (CONFIG_CONNECTOR_UNITTEST)
list(APPEND ConnectorLIBS DICMConnectorUnitTest)
add_subdirectory(source/unittest)
endif()
############################################################################################
if (CONFIG_CONNECTOR_TESLA)
list(APPEND ConnectorLIBS DICMConnectorTesla)
add_subdirectory(source/tesla)
endif()

if (CONFIG_CONNECTOR_PMIC)
list(APPEND ConnectorLIBS DICMConnectorPmic)
add_subdirectory(source/pmic)
endif()

if (CONFIG_CONNECTOR_LIGHT)
list(APPEND ConnectorLIBS DICMConnectorLight)
add_subdirectory(source/light)
endif()

if (CONFIG_CONNECTOR_SENSORS)
list(APPEND ConnectorLIBS DICMConnectorSensors)
add_subdirectory(source/sensors)
endif()

if (CONFIG_CONNECTOR_BSS)
list(APPEND ConnectorLIBS DICMConnectorBss)
add_subdirectory(source/bss)
endif()

if (CONFIG_CONNECTOR_BTS)
list(APPEND ConnectorLIBS DICMConnectorBts)
add_subdirectory(source/bts)
endif()

if (CONFIG_CONNECTOR_BATTERY_SERVICE)
list(APPEND ConnectorLIBS DICMConnectorBattery_Service)
add_subdirectory(source/battery_service)
endif()

if (CONFIG_CONNECTOR_GNSS)
list(APPEND ConnectorLIBS DICMConnectorGnss)
add_subdirectory(source/gnss)
endif()

if (CONFIG_CONNECTOR_TMS)
list(APPEND ConnectorLIBS DICMConnectorTms)
add_subdirectory(source/tms)
endif()

if (CONFIG_CONNECTOR_RS485_MASTER)
list(APPEND ConnectorLIBS DICMConnectorRs485Master)
add_subdirectory(source/rs485_master)
endif()

if (CONFIG_CONNECTOR_ACCELEROMETER)
list(APPEND ConnectorLIBS DICMConnectorAccelerometer)
add_subdirectory(source/accelerometer)
endif()

if (CONFIG_CONNECTOR_IOT_MODEM)
list(APPEND ConnectorLIBS DICMConnectorIotModem)
add_subdirectory(source/iot_modem)
endif()

if (CONFIG_CONNECTOR_BZ_SERVICE)
list(APPEND ConnectorLIBS DICMConnectorBuzzerService)
add_subdirectory(source/buzzer_service)
endif()

if (CONFIG_CONNECTOR_SIMULATOR_INVENTILATE OR CONFIG_CONNECTOR_SIMULATOR_SHAPE OR CONFIG_CONNECTOR_SIMULATOR_SHARC OR CONFIG_CONNECTOR_SIMULATOR_CFX2 OR CONFIG_CONNECTOR_SIMULATOR_SHAPEX OR CONFIG_CONNECTOR_SIMULATOR_EVENT_NOTIFICATION OR CONFIG_CONNECTOR_SIMULATOR_THUNDERSTORM OR CONFIG_CONNECTOR_SIMULATOR_PROD_MBAT)
list(APPEND ConnectorLIBS DICMSoftwareSimulatorLibrary)
add_subdirectory(source/software_simulator_lib)
list(APPEND ConnectorLIBS DICMConnectorSoftwareSimulators)
add_subdirectory(source/software_simulators)
endif()

if (CONFIG_CONNECTOR_MODEM)
message(FATAL_ERROR "Does not build!")
message(STATUS "Adding MODEM Connector")
file(GLOB CONNECTOR_MODEM_SRC_FILES CONFIGURE_DEPENDS "source/modem/source/*.c" "source/modem/source/aws_wrapper/*.c")
file(GLOB CONNECTOR_MODEM_EXTRA_SRC_FILES CONFIGURE_DEPENDS "source/modem/source/dev/bg95/*.c" "source/modem/source/state_machine/src/*.c")
list(APPEND PUBLIC_INCLUDE_DIRS "source/modem/include")
list(APPEND SRC_FILES ${CONNECTOR_MODEM_SRC_FILES} ${CONNECTOR_MODEM_EXTRA_SRC_FILES})
list(APPEND DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES "source/modem/source")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES} ${BSEC_SRC_FILES})
if (BUILD_TESTING)
    # Setup Unittest target and coverage target
    setup_tests(DICMFramework)
    setup_coverage()
endif()

target_include_directories(${PROJECT_NAME}
                            PRIVATE
                                ${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
                                ${DICM_FRAMEWORKCONNECTORS_EXTRA_INTERNAL_INCLUDES}
                            PUBLIC
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#                                ${PUBLIC_INCLUDE_DIRS}
							)

#Add prod_conf folder to SPIFFS image
add_to_file_system_image(prod_conf)
message(STATUS "Adding prod_conf to SPIFFS image")

if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
target_link_libraries(${PROJECT_NAME} PUBLIC idf::esp_ringbuf)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE idf::json DICMBroker DICMOsal DICMHal DICMCommonLibs DICMGeneralDefinitions DICMFrameworkConfiguration idf::freertos PUBLIC ${ConnectorLIBS})

if (USE_BSEC_2 OR USE_BSEC_1)
	target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${BSEC_INC_DIRS}>)
endif()

#message("ConnectorLIBS ${ConnectorLIBS}")
#get_property(libs TARGET ${PROJECT_NAME} PROPERTY LINK_LIBRARIES)
#message("libs ${libs}")
