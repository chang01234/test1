cmake_minimum_required(VERSION 3.5)

project(DICMFrameworkMocks)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.c" )

add_compile_definitions(${EXTRA_COMPILER_DEFINITIONS})
add_compile_options(${EXTRA_COMPILER_OPTIONS})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
#set(CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT} -Wa,-alh=<OBJECT>.lst")
target_include_directories(${PROJECT_NAME} 
							PRIVATE
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
							PUBLIC
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE DICMOsal DICMHal DICMCommonLibs DICMGeneralDefinitions DICMConnectors idf::esp_netif idf::lwip idf::driver idf::nvs_flash )
if (NOT IDF_VER_5)
target_link_libraries(${PROJECT_NAME} PUBLIC idf::${IDF_TARGET})
endif()