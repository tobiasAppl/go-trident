cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

find_package(trident 1.0.0.1 REQUIRED PATHS ../..)

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${trident_INCLUDES})

if(TRIDENT_PLATFORM STREQUAL "raspbian")
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -lrt -lm")
endif()

project(send_word C)

add_executable(send_word main.c)
target_link_libraries(send_word ${trident_LIBRARIES})

