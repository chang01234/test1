idf_component_register(
                    REQUIRES spiffs)

if (NOT CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH)
    message(FATAL_ERROR "CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH must be set in configuration")
endif()

#Depends on DICMFramework target to allow modules to put/copy its files to CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH before building partition
spiffs_create_partition_image(${CONFIG_FILE_SYSTEM_PARTITION_NAME} ${CONFIG_FILE_SYSTEM_SOURCE_IMAGE_PATH} FLASH_IN_PROJECT DEPENDS DICMFramework)
