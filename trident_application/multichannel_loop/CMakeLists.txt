cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

find_package(trident 1.0.0.1 REQUIRED PATHS ../..)

if(TRIDENT_PLATFORM STREQUAL "simulation")
    set(TRIDENT_SIMULATION_IPC_WIRE_CONFIG "${CMAKE_CURRENT_LIST_DIR}/simulation_ipc_wire.cfg" CACHE FILEPATH "Fixed ipc wire config file for trident simulation builds" FORCE)
    if(NOT EXISTS "${TRIDENT_SIMULATION_IPC_WIRE_CONFIG}" OR IS_DIRECTORY "${TRIDENT_SIMULATION_IPC_WIRE_CONFIG}")
        message(FATAL_ERROR "Application specific wire config file does not exist: ${TRIDENT_SIMULATION_IPC_WIRE_CONFIG}")
    endif()
    trident_reconfigure()
endif()

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${trident_INCLUDES}
)

project(multichannel_loop C)

add_executable(multichannel_loop main.c ${TRIDENT_SIMULATION_IPC_WIRE_CONFIG})
target_link_Libraries(multichannel_loop ${trident_LIBRARIES})

