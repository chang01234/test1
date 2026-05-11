if (${IDF_TARGET} STREQUAL "linux")
idf_component_register(
                        REQUIRES ${EXTRA_MANAGED_COMPONENTS})
else()
idf_component_register(
                        REQUIRES ${EXTRA_MANAGED_COMPONENTS} esp_psram)
endif()