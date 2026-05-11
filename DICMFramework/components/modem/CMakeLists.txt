set(srcs "source/esp_modem.c"
        "source/esp_modem_dce_service.c"
        "source/sim800.c"
        "source/bg96.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include
                    REQUIRES driver)