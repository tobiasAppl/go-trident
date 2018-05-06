cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

find_package(trident 1.0.0.1 REQUIRED PATHS ../..)

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${trident_INCLUDES})

project(reset_barrier_controller C)

add_executable(reset_barrier_controller main.c)
target_link_libraries(reset_barrier_controller ${trident_LIBRARIES})

