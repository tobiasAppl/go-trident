cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

project(pingpong_benchmark_peer C)

find_package(trident 1.0.0.1 REQUIRED PATHS ../../..)
#message("trident_LIBRARIES=${trident_LIBRARIES}")
#message("trident_INCLUDES=${trident_INCLUDES}")

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${trident_INCLUDES})

add_executable(pingpong_benchmark_peer main.c)
target_link_libraries(pingpong_benchmark_peer ${trident_LIBRARIES})

