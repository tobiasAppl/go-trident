cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

project(pingpong_stresstest C)

find_package(trident 1.0.0.1 REQUIRED PATHS ../..)
#message("trident_LIBRARIES=${trident_LIBRARIES}")
#message("trident_INCLUDES=${trident_INCLUDES}")

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${trident_INCLUDES})

set(trident_DIR "${trident_DIR}" CACHE INTERNAL "")

if(TRIDENT_PLATFORM STREQUAL "raspbian")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -lrt -lm")
endif()

add_executable(pingpong_stresstest main.c)
target_link_libraries(pingpong_stresstest ${trident_LIBRARIES})
