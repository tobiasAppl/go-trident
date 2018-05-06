cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

include("${CMAKE_CURRENT_LIST_DIR}/../tridentConfig.cmake")

project("LIB_${trident_lib_name}" C)

include_directories(${trident_INCLUDES})

#add_library(${trident_lib_name} ${trident_SOURCES})
