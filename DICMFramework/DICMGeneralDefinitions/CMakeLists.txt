cmake_minimum_required(VERSION 3.5)

project(DICMGeneralDefinitions)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(${PROJECT_NAME} INTERFACE idf::log idf::freertos)
