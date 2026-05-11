idf_build_get_property(target IDF_TARGET)

set(srcs
    "mytwai.c")

set(includes "include" )


idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ${includes}
                    PRIV_INCLUDE_DIRS "include/driver"
                    PRIV_REQUIRES efuse esp_timer esp_ipc
                    REQUIRES esp_pm esp_ringbuf freertos soc hal esp_hw_support driver
                    LDFRAGMENTS linker.lf)
# (REQUIRES cannot hide soc headers, since many arguments in the driver headers are chip-dependent)

# This component is a modified version from IDF version 4.4.3.
idf_build_get_property(myIDFVER IDF_VER - ESP-IDF)
#Remove 'v'
string(SUBSTRING ${myIDFVER} 0 1 myVer)
if (${myVer} STREQUAL "v")
    string(SUBSTRING ${myIDFVER} 1 -1 myVer)
else()
    set (myVer ${myIDFVER})
endif()

if (NOT ${myVer} VERSION_EQUAL  "4.4.3")
    message(FATAL_ERROR "Version check not OK (require: 4.4.3 build: ${myVer})")
else()
    message(STATUS "Version check OK (require: 4.4.3 build: ${myVer})")
endif()
