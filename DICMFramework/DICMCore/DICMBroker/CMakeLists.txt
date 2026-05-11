cmake_minimum_required(VERSION 3.5)

project(DICMBroker)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/*.c" "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
							PRIVATE
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
								${DICM_FRAMEWORK_EXTRA_INTERNAL_INCLUDES}
							PUBLIC
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE 
									 DICMGeneralDefinitions DICMHal DICMConnectors DICMCommonLibs DICMFrameworkConfiguration idf::freertos  
							PUBLIC  idf::esp_rom idf::nvs_flash)
if (NOT ${IDF_TARGET} STREQUAL "linux")
target_link_libraries(${PROJECT_NAME} PRIVATE idf::esp_wifi idf::esp_https_ota PUBLIC idf::mbedtls)
else()
target_link_libraries(${PROJECT_NAME} PRIVATE idf::esp_netif)
endif()
if (NOT IDF_VER_5)
target_link_libraries(${PROJECT_NAME} PUBLIC idf::${IDF_TARGET})
else()
target_link_libraries(${PROJECT_NAME} PUBLIC idf::esp_ringbuf)
endif()