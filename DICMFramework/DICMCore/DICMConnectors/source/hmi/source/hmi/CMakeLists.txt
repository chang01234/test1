cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0079 NEW)
#Add ulp
set(ulp_app_name ulp_hmi)
set(ulp_s_sources ulp/ulp.S)
set(ulp_exp_dep_srcs "ulp.c")
set (COMPONENT_LIB ${PROJECT_NAME})

#message(STATUS "idf_path=${idf_path}")
ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")