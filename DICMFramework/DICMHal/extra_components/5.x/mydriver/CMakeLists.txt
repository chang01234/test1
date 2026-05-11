idf_build_get_property(target IDF_TARGET)

# Always compiled source files
# TWAI related source files
if(CONFIG_SOC_TWAI_SUPPORTED)
    list(APPEND srcs "twai/mytwai.c")
endif()

# UART related source files
if(CONFIG_SOC_UART_SUPPORTED)
    list(APPEND srcs "uart/myuart.c")
endif()

# Always included headers
set(includes "twai/include"
             "uart/include")

# (REQUIRES cannot hide soc headers, since many arguments in the driver headers are chip-dependent)
# (Legacy drivers requires `esp_adc`, due to ADC HW resource mutex logics are there.
# Can be removed together with legacy drivers)
if (${IDF_TARGET} STREQUAL "linux")
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ${includes}
                    PRIV_REQUIRES esp_timer
                    REQUIRES esp_ringbuf freertos soc hal esp_hw_support driver
                    LDFRAGMENTS linker.lf)
else()
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ${includes}
                    PRIV_REQUIRES efuse esp_timer
                    REQUIRES esp_pm esp_ringbuf freertos soc hal esp_hw_support driver
                    LDFRAGMENTS linker.lf)
endif()
# This component is a modified version from IDF version 5.1.1.
idf_build_get_property(myIDFVER IDF_VER - ESP-IDF)
#Remove 'v'
string(SUBSTRING ${myIDFVER} 0 1 myVer)
if (${myVer} STREQUAL "v")
    string(SUBSTRING ${myIDFVER} 1 -1 myVer)
else()
    set (myVer ${myIDFVER})
endif()

if (NOT ${myVer} VERSION_EQUAL  "5.1.1")
    message(FATAL_ERROR "Version check not OK (require: 5.1.1 build: ${myVer})")
else()
    message(STATUS "Version check OK (require: 5.1.1 build: ${myVer})")
endif()
