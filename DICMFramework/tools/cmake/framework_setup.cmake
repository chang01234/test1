# Macro 'setup_project'
# Setups the idf-project build generation and creates required cmake targets for the build
# arguments are in format "binary_name" "offset"
macro(setup_target)
    if(NOT CMAKE_BUILD_TYPE)
        message(STATUS "Defaulting to Normal build.")
        set(CMAKE_BUILD_TYPE Normal CACHE STRING "" FORCE)
    endif()
    if (NOT DEFINED ENV{IDF_TARGET} AND NOT DEFINED IDF_TARGET)
        if (${ARGC} GREATER 0)
            message(STATUS "No target set before. Setting IDF_TARGET to ${ARGV0}")
            # Not found in environment not as cmake variable
            set(IDF_TARGET ${ARGV0})
        else()
            # No argument so we use default "esp32"
            message(STATUS "No target set before. Setting IDF_TARGET to esp32")
            # Not found in environment not as cmake variable
            set(IDF_TARGET "esp32")
        endif()
    elseif (DEFINED IDF_TARGET AND NOT DEFINED ENV{IDF_TARGET})
        # IDF_TARGET defined, we use its value, and ignore any argument
        message (STATUS "IDF_TARGET is set to ${IDF_TARGET}")
        if (${ARGC} GREATER 0)
            if (NOT ${IDF_TARGET} STREQUAL ${ARGV0})
                message(WARNING "Mismatch between cmake and project setup:  ${IDF_TARGET} and ${ARGV0}. Building as ${IDF_TARGET}.")
            endif()
        endif()
    elseif (NOT DEFINED IDF_TARGET AND DEFINED ENV{IDF_TARGET})
        # Environment is set, so we use that and ignore any argument
        # Check against already set in environment
        if (${ARGC} GREATER 0)
            if (NOT $ENV{IDF_TARGET} STREQUAL ${ARGV0})
                message(WARNING "Mismatch between environment and project setup: $ENV{IDF_TARGET} and ${ARGV0}. Building as $ENV{IDF_TARGET}.")
            endif()
        endif()
        set(IDF_TARGET $ENV{IDF_TARGET})
    elseif (DEFINED IDF_TARGET AND DEFINED ENV{IDF_TARGET})
        # Both are defined. If they differ something is wrong.
        if (NOT $ENV{IDF_TARGET} STREQUAL ${IDF_TARGET})
            message(FATAL_ERROR "Mismatch between environment and cmake: $ENV{IDF_TARGET} and ${IDF_TARGET}.")
        endif()
    endif()
    message(STATUS "Setup build target to: ${IDF_TARGET}")
    if(NOT DEFINED ENV{IDF_TARGET})
        #Set environment
        message(STATUS "Set environment variable IDF_TARGET to ${IDF_TARGET}")
        set(ENV{IDF_TARGET} ${IDF_TARGET})
    endif()

    set(toolchain_path_norm $ENV{IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)
    cmake_path(NORMAL_PATH toolchain_path_norm OUTPUT_VARIABLE toolchain_path_norm)

    if (NOT DEFINED ENV{CMAKE_TOOLCHAIN_FILE} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(STATUS "Toolchain not defined. Setting CMAKE_TOOLCHAIN_FILE to: $ENV{IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake")
        set(CMAKE_TOOLCHAIN_FILE ${toolchain_path_norm})
    elseif (NOT DEFINED ENV{CMAKE_TOOLCHAIN_FILE} AND DEFINED CMAKE_TOOLCHAIN_FILE)
        # Check setting
        cmake_path(NORMAL_PATH CMAKE_TOOLCHAIN_FILE OUTPUT_VARIABLE CMAKE_TOOLCHAIN_FILE_NORM)
        if (NOT ${CMAKE_TOOLCHAIN_FILE_NORM} STREQUAL ${toolchain_path_norm})
            message(WARNING "CMAKE_TOOLCHAIN_FILE not set according to project setup. Changing to match project setup ${toolchain_path_norm}")
            set(CMAKE_TOOLCHAIN_FILE ${toolchain_path_norm})
        endif()
    elseif (DEFINED ENV{CMAKE_TOOLCHAIN_FILE} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        # Check setting
        cmake_path(NORMAL_PATH ENV{CMAKE_TOOLCHAIN_FILE} OUTPUT_VARIABLE CMAKE_TOOLCHAIN_FILE_NORM)
        if (NOT ${CMAKE_TOOLCHAIN_FILE_NORM} STREQUAL ${toolchain_path_norm})
            message(FATAL_ERROR "ENV{CMAKE_TOOLCHAIN_FILE} not set according to project setup. Changing to match project setup ${toolchain_path_norm}")
        endif()
        set(CMAKE_TOOLCHAIN_FILE ${toolchain_path_norm})
    endif()


    if (NOT DEFINED BUILD_PRODUCT_ID)
        # Check the product->fw id to build
        include(${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/default_product_id.cmake OPTIONAL RESULT_VARIABLE result)

        if (${result} STREQUAL "NOTFOUND")
            message (STATUS "Did not find file (${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/default_product_id.cmake): ${result}")
        else()
            message (STATUS "Found file: ${result}")
        endif()
    else()
        message(STATUS "Using build command line BUILD_PRODUCT_ID: ${BUILD_PRODUCT_ID}")
    endif()

endmacro()

# Macro 'setup_project'
# Setups the idf-project build generation and creates required cmake targets for the build
# arguments are in format "binary_name" "offset"
macro(setup_project)
    set(EXTRA_BINARIES "")
    if (NOT ${IDF_TARGET} STREQUAL "linux")
        if (${ARGC} GREATER_EQUAL 2)
            set (EXTRA_BINARIES ${ARGV0}.bin -binary -offset ${ARGV1})
        endif()
        if (${ARGC} GREATER_EQUAL 4)
            set (EXTRA_BINARIES ${EXTRA_BINARIES} ${ARGV2}.bin -binary -offset ${ARGV3})
        endif()
        if (${ARGC} GREATER_EQUAL 6)
            set (EXTRA_BINARIES ${EXTRA_BINARIES} ${ARGV4}.bin -binary -offset ${ARGV5})
        endif()
        message(STATUS "Merging extra binaries into full binary: ${EXTRA_BINARIES}")
    endif()
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # Get extra components to build
    list(APPEND EXTRA_IDF_COMPONENTS_NAMES "")
    list(APPEND EXTRA_IDF_COMPONENTS_PATHS "")
    list(APPEND EXTRA_MANAGED_COMPONENTS "")

    # Prepare to build ESP-IDF as library
    include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

    # Set priority list for configuration files
    # 1. configuration/${BUILD_PRODUCT_ID}/{5.x|4.x}
    # 2. configuration/${BUILD_PRODUCT_ID}
    # 3. configuration/{5.x|4.x}
    # 4. configuration
    if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
        if (DEFINED BUILD_PRODUCT_ID)
            list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/${BUILD_PRODUCT_ID}/5.x)
            list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/${BUILD_PRODUCT_ID})
        endif()
        list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/5.x)
    else()
        if (DEFINED BUILD_PRODUCT_ID)
            list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/${BUILD_PRODUCT_ID}/4.x)
            list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/${BUILD_PRODUCT_ID})
        endif()
        list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration/4.x)
    endif()
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFrameworkConfiguration/configuration)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/DICMFramework)

    # Set project application version string
    include(project_version OPTIONAL RESULT_VARIABLE result)
    if (${result} STREQUAL "NOTFOUND")
        message (FATAL_ERROR "Did not find file: ${result}")
    else()
        message (STATUS "Found file: ${result}")
    endif()

    # Add project specific components
    include(project_components OPTIONAL RESULT_VARIABLE result)
    if (${result} STREQUAL "NOTFOUND")
        message (FATAL_ERROR "Did not find file: ${result}")
    else()
        message (STATUS "Found file: ${result}")
    endif()

    # Enable the component manager for regular projects if not explicitly disabled.
    # Remove FALSE in case of manually updating repo with lates versions referenced in DICMFramework/component_mgr/idf_component.yml
    if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
        if(NOT "$ENV{IDF_COMPONENT_MANAGER}" EQUAL "0")
            idf_build_set_property(IDF_COMPONENT_MANAGER 1)
        endif()
        # Set component manager interface version
        idf_build_set_property(__COMPONENT_MANAGER_INTERFACE_VERSION 2)
    endif()

    # Get git tag-commit as version string
    git_describe(prod_repo_version "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "Building on product tag: ${prod_repo_version}")

    # Add other ESP-IDF components to the build
    foreach(name path IN ZIP_LISTS EXTRA_IDF_COMPONENTS_NAMES EXTRA_IDF_COMPONENTS_PATHS)
        message(STATUS "Adding ${name}:${path} to IDF build")
        if ("${path}" STREQUAL "")
        else()
            cmake_path(HAS_ROOT_PATH path outvar)
            if (outvar)
                idf_build_component("${path}")
            else()
                idf_build_component("${CMAKE_CURRENT_LIST_DIR}/${path}")
            endif()
        endif()
    endforeach()
    if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
        foreach(extra_lib  ${EXTRA_MANAGED_COMPONENTS})
            message(STATUS "Adding /DICMFramework/components/managed_components/${extra_lib} to IDF build")
            idf_build_component("${CMAKE_CURRENT_LIST_DIR}/DICMFramework/components/managed_components/${extra_lib}")
        endforeach()
        if (${IDF_TARGET} STREQUAL "linux")
            # always override and build these components
            message(STATUS "Adding freertos:${CMAKE_CURRENT_LIST_DIR}/DICMFramework/components/freertos to IDF build")
            idf_build_component("${CMAKE_CURRENT_LIST_DIR}/DICMFramework/components/freertos")
            message(STATUS "Adding esp_timer:${IDF_PATH}/tools/mocks/esp_timer to IDF build")
            idf_build_component("${IDF_PATH}/tools/mocks/esp_timer")
            message(STATUS "Adding esp_netif:${IDF_PATH}/tools/mocks/esp_netif to IDF build")
            idf_build_component("${IDF_PATH}/tools/mocks/esp_netif")
            message(STATUS "Adding driver:${IDF_PATH}/tools/mocks/driver to IDF build")
            idf_build_component("${IDF_PATH}/tools/mocks/driver")
            message(STATUS "Adding nvs_flash:DICMFramework/components/nvs_flash to IDF build")
            idf_build_component("${CMAKE_CURRENT_LIST_DIR}/DICMFramework/components/nvs_flash")
            message(STATUS "Adding esp_partition:DICMFramework/components/esp_partition to IDF build")
            idf_build_component("${CMAKE_CURRENT_LIST_DIR}/DICMFramework/components/esp_partition")
            message(STATUS "Adding esp_netif_linux:DICMFramework/components/esp_netif_linux to IDF build")
            idf_build_component("${CMAKE_CURRENT_LIST_DIR}/DICMFramework/components/esp_netif_linux")
            message(STATUS "Adding esp_app_format:DICMFramework/components/esp_app_format to IDF build")
            idf_build_component("${CMAKE_CURRENT_LIST_DIR}/DICMFramework/components/esp_app_format")
            list(APPEND extra_components spi_flash esp_timer esp_netif driver esp_partition esp_netif_linux esp_app_format)
        else()
            message(STATUS "Adding overridden esp_driver_uart:DICMFramework/DICMHal/extra_components/$ENV{IDF_VERSION}/esp_driver_uart to IDF build")
            idf_build_component("${CMAKE_CURRENT_LIST_DIR}/DICMFramework/DICMHal/extra_components/$ENV{IDF_VERSION}/esp_driver_uart")
            list(APPEND extra_components esp_driver_uart)
        endif()

    endif()
    if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
        if (${IDF_TARGET} STREQUAL "linux")
            set (components freertos esp_common json nvs_flash spiffs esp_app_format heap lwip esp_system esp_ringbuf esp_event soc partition_table esptool_py)
        else()
            set (components freertos esptool_py nvs_flash nvs_sec_provider spiffs app_update esp_https_ota esp_rom esp_phy json bt esp_adc esp_wifi mbedtls esp_http_server)
        endif()
        if (CONFIG_CONNECTOR_TLS)
        list(APPEND components esp-tls)
        endif()
    else()
        set (components ${IDF_TARGET} freertos esptool_py nvs_flash spiffs app_update esp_https_ota esp_rom json bt esp_adc_cal mdns esp_wifi mbedtls)
        list (APPEND EXTRA_LIBS "idf::${IDF_TARGET}")
    endif()
    list(APPEND components ${EXTRA_IDF_COMPONENTS_NAMES} ${extra_components} ${EXTRA_MANAGED_COMPONENTS})

    # Check for project specific include file with configurations, needed to get CONFIG_POWER_MANAGEMENT
    set(PROJECT_SPECIFIC_CONFIG project_specific)
    include(${PROJECT_SPECIFIC_CONFIG} OPTIONAL RESULT_VARIABLE result)
    if (${result} STREQUAL "NOTFOUND")
        message (FATAL_ERROR "Did not find file: ${result}")
    else()
        message (STATUS "Found file: ${result}")
    endif()

    if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
        set (SDKCONFIG_PATH 5.x)
        if (CONFIG_FLASHSIZE_8MB)
            set (SDKCONFIG_FLASH_8MB .8MB)
            set (SDKCONFIG_FLASH_8MB_PATH ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_FLASH_8MB})
        else()
            set (SDKCONFIG_FLASH_8MB )
            set (SDKCONFIG_FLASH_8MB_PATH )
        endif()
	else()
        set (SDKCONFIG_PATH 4.x)
	endif()

    set (IDF_SPECIFIC_CONFIG )
	set (SDKCONFIG_PM )
    if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
        if (CONFIG_POWER_MANAGEMENT)
            set (SDKCONFIG_PM .pm)
        endif()
        if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_PM}.$ENV{IDF_VERSION}")
            message (STATUS "Found ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_PM}.$ENV{IDF_VERSION}")
            set (IDF_SPECIFIC_CONFIG ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_PM}.$ENV{IDF_VERSION})
        else()
            message (STATUS "Did not find ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_PM}.$ENV{IDF_VERSION}")
        endif()
    else()
        set (SDKCONFIG_PM )
    endif()
    if (CONFIG_DEVELOPMENT_VIRTUAL_EFUSE)
        set (SDKCONFIG_SECURE_DEV_EFUSE ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults.secure.efuse)
    else()
        set (SDKCONFIG_SECURE_DEV_EFUSE)
    endif()
    if (CONFIG_CONNECTOR_TLS)
        set (SDKCONFIG_TLS ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults.tls)
    else()
        set (SDKCONFIG_TLS)
    endif()
    if (CONFIG_SECURE_PLATFORM_DEVELOPMENT OR CONFIG_SECURE_PLATFORM_PRODUCTION)
        if (CONFIG_BUILD_HMI_DATA)
            message(STATUS "::notice::Secure Plaform HMI build")
            set (SDKCONFIG_SECURE_PLATFORM ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_FLASH_8MB}.hmi.secure)
        else()
            message(STATUS "::notice::Secure Plaform build")
            set (SDKCONFIG_SECURE_PLATFORM ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_FLASH_8MB}.secure)
        endif()
    else()
        set (SDKCONFIG_SECURE_PLATFORM)
    endif()
    if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.0.0")
        if (CONFIG_SECURE_PLATFORM_PRODUCTION)
            set (SDKCONFIG_SECURE_BOOT ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults.boot.prod)
        else()
            if (CONFIG_NO_SECURE_PLATFORM_LEGACY_BOOT_KEY)
                set (SDKCONFIG_SECURE_BOOT)
            else()
               set (SDKCONFIG_SECURE_BOOT ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults.boot.dev)
            endif()
        endif()
    else()
        set (SDKCONFIG_SECURE_BOOT)
    endif()

    # Create ESP-IDF build process
    idf_build_process(${IDF_TARGET}
                        # try and trim the build; additional components
                        # will be included as needed based on dependency tree
                        #
                        # although esptool_py does not generate static library,
                        # processing the component is needed for flashing related
                        # targets and file generation
                        PROJECT_VER ${VERSION_STRING}
                        COMPONENTS ${components}
                        SDKCONFIG_DEFAULTS ${CMAKE_CURRENT_LIST_DIR}/DICMFramework/sdkconfig/${SDKCONFIG_PATH}/sdkconfig.defaults${SDKCONFIG_PM} ${IDF_SPECIFIC_CONFIG} ${SDKCONFIG_FLASH_8MB_PATH} ${CMAKE_CURRENT_LIST_DIR}/DICMFrameworkConfiguration/configuration/${BUILD_PRODUCT_ID}/${SDKCONFIG_PATH}/sdkconfig.defaults.project${SDKCONFIG_PM} ${SDKCONFIG_SECURE_PLATFORM} ${SDKCONFIG_SECURE_DEV_EFUSE} ${SDKCONFIG_SECURE_BOOT} ${SDKCONFIG_TLS}
                        SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
                        BUILD_DIR ${CMAKE_BINARY_DIR})
    if (${IDF_TARGET} STREQUAL "linux")
        find_library(LIB_BSD bsd)
        if(LIB_BSD)
            find_path(BSD_PATH bsd/string.h)
            message(STATUS "Found bsd : ${BSD_PATH}")
        else()
            message(FATAL_ERROR "Missing LIBBSD library. Install libbsd-dev package and/or check linker directories.")
        endif()
        # To fix build errors for linux target
        #target_compile_options(__idf_espressif__mdns PRIVATE -include bsd/string.h)
        target_compile_options(__idf_esp_ringbuf PRIVATE -include stdio.h)
        target_compile_options(__idf_esp_ringbuf PRIVATE -Wno-error=format=)
    endif()

    if (${CMAKE_BUILD_TYPE} STREQUAL "UnitTest")
        message(STATUS "Building project unit tests")
        include(CTest)
        # Download and unpack googletest at configure time
        configure_file(${CMAKE_SOURCE_DIR}/DICMFramework/tools/cmake/GTestCMakeLists.txt.in googletest-download/CMakeLists.txt)
        execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
        )
        execute_process(COMMAND "${CMAKE_COMMAND}" --build .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
        )
        option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)
        # Add googletest directly to our build. This adds the following targets:
        # gtest, gtest_main, gmock and gmock_main
        add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src ${CMAKE_BINARY_DIR}/googletest)
    endif()

    # Adding DICMApplication, DICMFramework DICMFrameworkConfiguration modules here
    add_subdirectory(DICMFramework)
    # DICMFrameworkConfiguration must exist here
    add_subdirectory(DICMFrameworkConfiguration)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/DICMApplication)
        message(STATUS "DICMApplication added")
        add_subdirectory(DICMApplication)
        set (DICMAPPLICATION_LIBS DICMApplication)
    endif()

    if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "UnitTest")
        # Create main executable
        foreach(extra_lib  ${EXTRA_MANAGED_COMPONENTS})
            list (APPEND EXTRA_LIBS "idf::${extra_lib}")
        endforeach()
        if (CONFIG_BUILD_HMI_DATA)
            set (HMI_DATA_VERSION "${HMI_DATA_INTERFACE_VERSION_MAJOR}.${HMI_DATA_INTERFACE_VERSION_MINOR}.${HMI_DATA_BUILD_VERSION}")
        endif()
        file(GLOB MAIN_SOURCES CONFIGURE_DEPENDS "DICMFramework/main/*.c")
        add_executable(${CMAKE_PROJECT_NAME}.elf ${MAIN_SOURCES}  ${EXTRA_BUILD_DEPS})
        target_link_libraries(${CMAKE_PROJECT_NAME}.elf PUBLIC DICMFramework DICMFrameworkConfiguration ${DICMAPPLICATION_LIBS} ${EXTRA_LIBS})
        #add_compile_definitions(${EXTRA_COMPILER_DEFINITIONS})

        # Create map file
        set(mapfile "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
        target_link_libraries(${CMAKE_PROJECT_NAME}.elf PRIVATE "-Wl,--cref -Wl,--Map=${mapfile}")
        set_property(TARGET ${CMAKE_PROJECT_NAME}.elf APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${mapfile}" )

        # Remove linker warning
        if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.3.2")
            # Add this symbol as a hint for esp_idf_size to guess the target name
            target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE "-Wl,--defsym=IDF_TARGET_${IDF_TARGET}=0")
            target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE "LINKER:--no-warn-rwx-segments")
    	endif()
        # Add commands to generate binaries
        # Use esptool to create the full combined binary
        set(ESPTOOLPY ${python} "$ENV{ESPTOOL_WRAPPER}" "${IDF_PATH}/components/esptool_py/esptool/esptool.py" --chip ${IDF_TARGET})
        idf_component_get_property(esptool_py_dir esptool_py COMPONENT_DIR)
        # Use dummy parameters for ESPPORT and ESPBAUD
if (CONFIG_BUILD_HMI_DATA)
        # Read offset from partition table file
        partition_table_get_partition_info(offset "--partition-name hmi_data" "offset")
        set(HMI_ADDRESS ${offset})
        message(STATUS "HMI_ADDRESS ${HMI_ADDRESS}")
        set (MERGE_EXTRA_BIN ";${HMI_ADDRESS};hmi_data.bin")
else()
        set (MERGE_EXTRA_BIN "")
endif()
        if (NOT ${IDF_TARGET} STREQUAL "linux")
            list(APPEND MERGE_CMD merge_bin)
            list(APPEND MERGE_CMD "-o" "${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}_v${VERSION_STRING}-full.bin" "@flash_args${MERGE_EXTRA_BIN}")
            add_custom_command(
                TARGET gen_project_binary
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
                COMMAND ${CMAKE_COMMAND}
                -B ${CMAKE_BINARY_DIR}
                -D "IDF_PATH=${IDF_PATH}"
                -D "ESPPORT=COM1"
                -D "ESPBAUD=921600"
                -D "SERIAL_TOOL=\"${ESPTOOLPY}\""
                -D "SERIAL_TOOL_ARGS=\"${MERGE_CMD}\""
                -D "WORKING_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
                -P "${esptool_py_dir}/run_serial_tool.cmake"
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}_v${VERSION_STRING}-ota.bin
                COMMAND ${CMAKE_COMMAND} -E copy ${mapfile} ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}_v${VERSION_STRING}.map
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}_v${VERSION_STRING}.elf
                COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${PPATH}" ${python} ${CMAKE_SOURCE_DIR}/DICMFramework/tools/python/crc32/calculate_crc32.py ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}_v${VERSION_STRING}-ota.bin
                POST_BUILD
                USES_TERMINAL
                )
        endif()

        # Create config target for version string to be put in GitVersion.h
        add_dependencies(${CMAKE_PROJECT_NAME}.elf GetGitVersion)
        add_custom_target( GetGitVersion
            COMMAND ${CMAKE_COMMAND}
                -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                -D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/DICMFramework/tools/cmake/GitVersion.h.in
                -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/config/GitVersion.h
                -P ${CMAKE_CURRENT_SOURCE_DIR}/DICMFramework/tools/cmake/generate_version.cmake
        )

        if (CONFIG_BUILD_HMI_DATA)
            message (STATUS "Building to generate hmi_data")

            add_subdirectory(${CMAKE_SOURCE_DIR}/DICMFramework/tools/hmi_data_build)

            add_custom_target(generate_hmi_data ALL
                            DEPENDS ${CMAKE_BINARY_DIR}/hmi_data.bin ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}-HMI_v${HMI_DATA_VERSION}.bin
                            COMMAND ${CMAKE_COMMAND} -E echo "Extraxting hmi_data.bin to be flashed in hmi_data partition"
                            COMMENT "Extraxting hmi_data.bin to be flashed in hmi_data partition"
                            VERBATIM
                            )
            set_property(TARGET generate_hmi_data APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/hmi_data.bin" )

            add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/hmi_data.bin ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}-HMI_v${HMI_DATA_VERSION}.bin
                                COMMAND ${CMAKE_OBJCOPY} -O binary --only-section=.hmi_data.rodata* ${HMI_DATA_BUILD_TARGET_PATH} ${CMAKE_BINARY_DIR}/hmi_data.bin
                                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
                                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/hmi_data.bin ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}-HMI_v${HMI_DATA_VERSION}.bin
                                COMMENT "Extraxting hmi_data.bin to be flashed in hmi_data partition"
                                VERBATIM
                                DEPENDS ${HMI_DATA_BUILD_TARGET_PATH}
                                POST_BUILD)
            add_dependencies(gen_project_binary generate_hmi_data)
            partition_table_get_partition_info(offset_1 "--partition-name hmi_data_1" "offset")

            esptool_py_custom_target(hmi_data-flash hmi_data "generate_hmi_data")
            esptool_py_flash_target_image(hmi_data-flash hmi_data "${HMI_ADDRESS}" "${CMAKE_BINARY_DIR}/hmi_data.bin")
            esptool_py_custom_target(hmi_data_1-flash hmi_data_1 "generate_hmi_data")
            esptool_py_flash_target_image(hmi_data_1-flash hmi_data_1 "${offset_1}" "${CMAKE_BINARY_DIR}/hmi_data.bin")
            # Uncomment if normal flash should also flash the HMI data partition
            # esptool_py_flash_target_image(flash hmi_data "0x3d0000" "${CMAKE_BINARY_DIR}/hmi_data.bin")
        endif()
        if (CONFIG_FILE_SYSTEM_PARTITION_NAME AND CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH)
            message (STATUS "Building to generate ${CONFIG_FILE_SYSTEM_PARTITION_NAME} File System partition")
            add_custom_target(gen_spiffs_binary ALL
                            DEPENDS spiffs_${CONFIG_FILE_SYSTEM_PARTITION_NAME}_bin
                            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${CONFIG_FILE_SYSTEM_PARTITION_NAME}.bin to bin directory"
                            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                            COMMENT "Copying ${CONFIG_FILE_SYSTEM_PARTITION_NAME}.bin to bin directory"
                            VERBATIM
                            )
            set_property(TARGET gen_spiffs_binary APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/${CONFIG_FILE_SYSTEM_PARTITION_NAME}.bin" )
            add_custom_command(
                TARGET gen_spiffs_binary
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CONFIG_FILE_SYSTEM_PARTITION_NAME}.bin ${CMAKE_SOURCE_DIR}/bin/${CMAKE_PROJECT_NAME}-FS_v${CONFIG_FS_VERSION}.bin
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                DEPENDS spiffs_${CONFIG_FILE_SYSTEM_PARTITION_NAME}_bin
                VERBATIM
                POST_BUILD
                )
        endif()
        # Add project specific build steps here
            include(project_post_build OPTIONAL RESULT_VARIABLE result)
            if (${result} STREQUAL "NOTFOUND")
                message (STATUS "Did not find \"project_post_build.cmake\" file: ${result}")
            else()
                message (STATUS "Found file: ${result}")
            endif()

        #set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
    else()
        get_property(tmp GLOBAL PROPERTY COVERAGE_NAMES)
        #message(STATUS "COVERAGE_NAMES: ${tmp}")
        add_custom_target(all_coverage
                             COMMAND ;
                             VERBATIM
                             COMMENT "Generating all code coverage files"
                             DEPENDS ${tmp}
                          )
    endif()
endmacro()

# Macro 'build_project'
# Finalizes the idf-project build generation and generates support files required for flashing
macro(build_project)
    if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "UnitTest")
        idf_build_get_property(idf_path IDF_PATH)
        idf_build_get_property(python PYTHON)

        add_custom_target(dicm-version
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
            COMMAND ${CMAKE_COMMAND} -E echo "---" > ${CMAKE_SOURCE_DIR}/bin/version.yml
            COMMAND ${CMAKE_COMMAND} -E echo " fwid: ${CMAKE_PROJECT_NAME}" >> ${CMAKE_SOURCE_DIR}/bin/version.yml
            COMMAND ${CMAKE_COMMAND} -E echo " fwversion: ${CMAKE_PROJECT_NAME}_v${VERSION_STRING}" >> ${CMAKE_SOURCE_DIR}/bin/version.yml
            COMMAND ${CMAKE_COMMAND} -E echo " idf: $ENV{IDF_VERSION}" >> ${CMAKE_SOURCE_DIR}/bin/version.yml
            COMMAND ${CMAKE_COMMAND} -E echo " target: ${IDF_TARGET}" >> ${CMAKE_SOURCE_DIR}/bin/version.yml
            COMMAND ${CMAKE_COMMAND} -E echo " git: ${prod_repo_version}" >> ${CMAKE_SOURCE_DIR}/bin/version.yml
            USES_TERMINAL
            VERBATIM
        )

        if ($ENV{IDF_VERSION} VERSION_GREATER_EQUAL "5.3.1")

            set(idf_size ${python} -m esp_idf_size)
            add_custom_target(size
                COMMAND ${CMAKE_COMMAND}
                -D "IDF_SIZE_TOOL=${idf_size}"
                -D "MAP_FILE=${mapfile}"
                -D "OUTPUT_JSON=${OUTPUT_JSON}"
                -P "${idf_path}/tools/cmake/run_size_tool.cmake"
                DEPENDS ${mapfile}
                USES_TERMINAL
                VERBATIM
            )
            add_custom_target(size-files
                COMMAND ${CMAKE_COMMAND}
                -D "IDF_SIZE_TOOL=${idf_size}"
                -D "IDF_SIZE_MODE=--files"
                -D "MAP_FILE=${mapfile}"
                -D "OUTPUT_JSON=${OUTPUT_JSON}"
                -P "${idf_path}/tools/cmake/run_size_tool.cmake"
                DEPENDS ${mapfile}
                USES_TERMINAL
                VERBATIM
            )
            add_custom_target(size-components
                COMMAND ${CMAKE_COMMAND}
                -D "IDF_SIZE_TOOL=${idf_size}"
                -D "IDF_SIZE_MODE=--archives"
                -D "MAP_FILE=${mapfile}"
                -D "OUTPUT_JSON=${OUTPUT_JSON}"
                -P "${idf_path}/tools/cmake/run_size_tool.cmake"
                DEPENDS ${mapfile}
                USES_TERMINAL
                VERBATIM
            )
        else()

            set(idf_size ${python} ${idf_path}/tools/idf_size.py)

            # Add size targets, depend on map file, run idf_size.py
            add_custom_target(size
                DEPENDS ${mapfile}
                COMMAND ${idf_size} --target ${IDF_TARGET} ${mapfile}
                )
            add_custom_target(size-files
                DEPENDS ${mapfile}
                COMMAND ${idf_size} --target ${IDF_TARGET} --files ${mapfile}
                )
            add_custom_target(size-components
                DEPENDS ${mapfile}
                COMMAND ${idf_size} --target ${IDF_TARGET} --archives ${mapfile}
                )

            unset(idf_size)
        endif()
        idf_build_executable(${CMAKE_PROJECT_NAME}.elf)

        # Write and generate project-files
        include(${idf_path}/tools/cmake/project.cmake)
        __project_info("")
    endif()
endmacro()

# Macro 'add_to_file_system_image'
# To be called by any library that want to add files to the FS image
# Note: CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH is required to be set
# Example: add_to_file_system_image(rules) will copy the directory 'rules' and all its files to CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH
macro(add_to_file_system_image)
    if(NOT CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH)
        message(FATAL_ERROR "not configured CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH")
    else()
        message(STATUS "CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH configured to ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH}")
    endif()
    #Copy file to CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH
    set (FILE_EXT json)
    if (${ARGC} EQUAL 2)
        set (FILE_EXT ${ARGV1})
    endif()

    file(MAKE_DIRECTORY ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH}/${ARGV0})
    file(GLOB MY_INPUT_FILES ${ARGV0}/*.${FILE_EXT})
    message(STATUS "${MY_INPUT_FILES}")
    foreach(InputFile IN LISTS MY_INPUT_FILES)
        cmake_path(GET InputFile FILENAME filename)
        add_custom_command(
            OUTPUT ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH}/${ARGV0}/${filename}
            COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${PPATH}" ${python} ${CMAKE_SOURCE_DIR}/DICMFramework/tools/python/json/compress_json.py ${InputFile} ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH}/${ARGV0}/${filename}
            # COMMAND ${CMAKE_COMMAND} -E copy_if_different ${InputFile} ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH}/${ARGV0}/
            MAIN_DEPENDENCY ${InputFile})
        add_custom_target(${FILE_EXT}_copy-${filename}-${PROJECT_NAME} DEPENDS ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH}/${ARGV0}/${filename})
        add_dependencies(${PROJECT_NAME} ${FILE_EXT}_copy-${filename}-${PROJECT_NAME})
    endforeach()
endmacro()

# Macro 'create_hmi_data_target'
# Is called automatically by DICMFramework/tools/hmi_data_build/CMakeLists.txt if CONFIG_BUILD_HMI_DATA is set in project_specific.cmake file.
# Note: HMI_DATA_SOURCE_DIRS is required to be set, list of source directories
# Note: HMI_DATA_INCLUDE_DIRS is required to be set, list of public include directories
# Note: EXCLUDE_HMI_DATA_SOURCES is optional to be set, list of source files to not include in the build
macro(create_hmi_data_target)
    project(hmi_data_build C CXX ASM)

    set(hmi_data_sources "")
    foreach(hmi_data_source IN LISTS HMI_DATA_SOURCE_DIRS)
        list (APPEND hmi_data_sources ${hmi_data_source}/*.c)
    endforeach()

    set(hmi_data_includes "")
    foreach(hmi_data_include IN LISTS HMI_DATA_INCLUDE_DIRS)
        list (APPEND hmi_data_includes ${hmi_data_include}/*.h)
    endforeach()

    file(GLOB SRC_FILES CONFIGURE_DEPENDS  ${hmi_data_sources} ${hmi_data_includes})
    set(ORIG_SRC_FILES ${SRC_FILES})
    if(EXCLUDE_HMI_DATA_SOURCES)
        foreach(src ${EXCLUDE_HMI_DATA_SOURCES})
            get_filename_component(src "${src}" ABSOLUTE)
            list(REMOVE_ITEM SRC_FILES "${src}")
        endforeach()
    endif()

    add_compile_definitions(BUILD_WITH_HMI_DATA)
    add_compile_options(-nostdlib)
    add_executable(${PROJECT_NAME}.elf ${CMAKE_SOURCE_DIR}/DICMFramework/tools/hmi_data_build/main.c ${SRC_FILES} )

    target_include_directories(${PROJECT_NAME}.elf
                                PUBLIC
                                        ${HMI_DATA_INCLUDE_DIRS}
                                PRIVATE
                                        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/DICMFramework/DICMCommonLibs/include>
                                        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/DICMFramework/DICMCommonLibs/ddm2/include>
                                        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/DICMFramework/DICMHal/include>
                                        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/DICMFramework/tools/hmi_data_build>
                              )
    target_link_libraries(${PROJECT_NAME}.elf PRIVATE "-L $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/DICMFramework/tools/hmi_data_build>")
    target_link_libraries(${PROJECT_NAME}.elf PRIVATE "-T hmi.ld")
    target_link_libraries(${PROJECT_NAME}.elf PUBLIC "-u def_data_header")
    # Add link dependency to linker file
    set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/DICMFramework/tools/hmi_data_build>/hmi.ld)
    # Create map file
    set(mapfile "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map")
    target_link_libraries(${PROJECT_NAME}.elf PRIVATE "-Wl,--cref -Wl,--Map=${mapfile}")
    set_property(TARGET ${PROJECT_NAME}.elf APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${mapfile}" )

    # Add build dependency
    add_dependencies(${PROJECT_NAME}.elf DICMFramework)
    set(HMI_DATA_BUILD_TARGET_PATH  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf" CACHE INTERNAL "HMI_DATA_BUILD_TARGET_PATH")

    add_custom_target(build_hmi_source
                        DEPENDS ${ORIG_SRC_FILES}
                        VERBATIM
                      )

    file(GLOB_RECURSE GEN_SRC_FILES CONFIGURE_DEPENDS "${GEN_HMI_DATA_SOURCE_DIR}*.txt")
    file(GLOB_RECURSE TOOL_PY_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/DICMFramework/tools/python/readpng/*.py")
    list (APPEND PPATH ${CMAKE_SOURCE_DIR}/DICMFramework/DICMCommonLibs/ddm2/source)
    list (APPEND PPATH ${GEN_HMI_DATA_SOURCE_DIR})
    if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        # On non-Windows systems, we need to replace the path separator
        string(REPLACE ";" ":" PPATH "${PPATH}")
    endif()
    add_custom_command(OUTPUT ${ORIG_SRC_FILES}
                        COMMAND ${CMAKE_COMMAND} -E env ${python} -m pip install pypng --ignore-installed --disable-pip-version-check
                        COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${PPATH}" "QUIET=True" "HMI_DATA_VERSION=${HMI_DATA_VERSION}" ${python} ${CMAKE_SOURCE_DIR}/DICMFramework/tools/python/readpng/readpng.py ${GEN_HMI_DATA_EXE_ARGS}
                        WORKING_DIRECTORY ${GEN_HMI_DATA_SOURCE_DIR}
                        #COMMAND cmd /C "${GEN_HMI_DATA_SOURCE_DIR}${GEN_HMI_DATA_EXE_FILE}"
                        DEPENDS ${GEN_SRC_FILES} ${TOOL_PY_FILES}
                        USES_TERMINAL
                        VERBATIM
                       )
    add_dependencies(${PROJECT_NAME}.elf build_hmi_source)
endmacro()
