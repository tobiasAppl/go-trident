cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

find_package(trident 1.0.0.1 REQUIRED PATHS ../..)
#message("trident_LIBRARIES=${trident_LIBRARIES}")
#message("trident_INCLUDES=${trident_INCLUDES}")

include_directories(${trident_INCLUDES})
include_directories(${CMAKE_CURRENT_LIST_DIR})

project(init_pin C)

add_executable(init_pin main.c)
target_link_libraries(init_pin ${trident_LIBRARIES})
