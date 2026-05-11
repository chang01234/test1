idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    return() # This component is not supported by the POSIX/Linux simulator
endif()

# Always compiled source files
set(srcs)

# Always included headers
set(includes "twai/include")

# Always included linker fragments
set(ldfragments "")

# TWAI related source files
if(CONFIG_SOC_TWAI_SUPPORTED)
    list(APPEND srcs "twai/mytwai.c")

    list(APPEND ldfragments "twai/linker.lf")
endif()

if(BOOTLOADER_BUILD)
    # Bootloader shall NOT depend on the drivers
    idf_component_register()
else()
    # (REQUIRES cannot hide soc headers, since many arguments in the driver headers are chip-dependent)
    # (Legacy drivers requires `esp_adc`, due to ADC HW resource mutex logics are there.
    # Can be removed together with legacy drivers)
    idf_component_register(SRCS "${srcs}"
                           INCLUDE_DIRS ${includes}
                           PRIV_REQUIRES efuse esp_timer esp_mm
                           REQUIRES esp_pm esp_ringbuf freertos soc hal esp_hw_support driver
                           LDFRAGMENTS ${ldfragments}
                          )
endif()
# This component is a modified version from IDF version 5.3.1.
idf_build_get_property(myIDFVER IDF_VER - ESP-IDF)
#Remove 'v'
string(SUBSTRING ${myIDFVER} 0 1 myVer)
if (${myVer} STREQUAL "v")
    string(SUBSTRING ${myIDFVER} 1 -1 myVer)
else()
    set (myVer ${myIDFVER})
endif()

if (NOT ${myVer} VERSION_GREATER_EQUAL "5.3.1")
    message(FATAL_ERROR "Version check not OK (require: 5.3.1 build: ${myVer})")
else()
    message(STATUS "Version check OK (require: 5.3.1 build: ${myVer})")
endif()
