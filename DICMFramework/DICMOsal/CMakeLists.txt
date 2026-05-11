cmake_minimum_required(VERSION 3.5)

project(DICMOsal)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "source/freertos/*.c")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

#set(CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT} -Wa,-alh=<OBJECT>.lst")

target_include_directories(${PROJECT_NAME} 
							PRIVATE
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/freertos>
							PUBLIC
								$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
							)
target_link_libraries(${PROJECT_NAME} PRIVATE idf::freertos)

target_link_libraries(${PROJECT_NAME} PRIVATE DICMCommonLibs DICMGeneralDefinitions DICMFrameworkConfiguration)
