# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
if (CCACHE_ENABLE)
	find_program(CCACHE_FOUND ccache)
	if (CCACHE_FOUND)
		message(STATUS "ccache will be used for faster recompilation")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	else()
		message(WARNING "enabled ccache in build but ccache program not found")
	endif()
endif()


#hmi_data
create_hmi_data_target()
#set(CMAKE_C_COMPILE_OBJECT "${CMAKE_C_COMPILE_OBJECT} -Wa,-alh=<OBJECT>.lst")