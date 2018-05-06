cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/files_and_directories.cmake")

# Set some cache variables
set(trident_lib_name "trident" CACHE INTERNAL "")
set(TRIDENT_PIN_COUNT 28 CACHE NUMBER "Number of PINs of the target platform")
set(TRIDENT_LOGGING_LVL "0" CACHE STRING "Trident logging level, 0(no messages), 1(error messages), 2(info messages)")
set_property(CACHE TRIDENT_LOGGING_LVL PROPERTY STRINGS "0" "1" "2")

if(TRIDENT_LOGGING_LVL GREATER "1" OR TRIDENT_LOGGING_LVL EQUAL "1")
    set(TRIDENT_LOGGING_ERROR "ON" CACHE INTERNAL "")
else()
    set(TRIDENT_LOGGING_ERROR "OFF" CACHE INTERNAL "")
endif()

if(TRIDENT_LOGGING_LVL GREATER "2" OR TRIDENT_LOGGING_LVL EQUAL "2")
    set(TRIDENT_LOGGING_INFO "ON" CACHE INTERNAL "")
else()
    set(TRIDENT_LOGGING_INFO "OFF" CACHE INTERNAL "")
endif()

if(TRIDENT_LOGGING_LVL GREATER 0)
    set(TRIDENT_LOGGING_LAYER0_ENABLED "ON" CACHE BOOL "Logging set for trident layer 0")
    set(TRIDENT_LOGGING_LAYER1_ENABLED "ON" CACHE BOOL "Logging set for trident layer 1")
    set(TRIDENT_LOGGING_LAYER2_ENABLED "ON" CACHE BOOL "Logging set for trident layer 2")
    set(TRIDENT_LOGGING_APPLICATION_ENABLED "ON" CACHE BOOL "Logging set for trident applications")
else()
    set(TRIDENT_LOGGING_LAYER0_ENABLED "OFF" CACHE BOOL "Logging set for trident layer 0" FORCE)
    set(TRIDENT_LOGGING_LAYER1_ENABLED "OFF" CACHE BOOL "Logging set for trident layer 1" FORCE)
    set(TRIDENT_LOGGING_LAYER2_ENABLED "OFF" CACHE BOOL "Logging set for trident layer 2" FORCE)
    set(TRIDENT_LOGGING_APPLICATION_ENABLED "OFF" CACHE BOOL "Logging set for trident applications" FORCE)
endif()

set(TRIDENT_LOGGING_FILE "${CMAKE_BINARY_DIR}/trident_log.log" CACHE FILEPATH "Logging target")

set(CMAKE_C_FLAGS "-std=gnu99 -lm" CACHE INTERNAL "")
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra" CACHE INTERNAL "")

# Assemble trident source files
gatherSources(trident_sources QUIET "src/trident/")

# Create variables for the caller
set(trident_config_header "${CMAKE_BINARY_DIR}/gensrc/trident_config.h" CACHE INTERNAL "")

add_library(${trident_lib_name}
  ${trident_sources}
  ${trident_sources_HEADERS}
  ${trident_config_header}
)
set_target_properties(${trident_lib_name} PROPERTIES LINKER_LANGUAGE C)
set(trident_LIBRARIES
   ${trident_lib_name}
   CACHE INTERNAL ""
)

set(trident_INCLUDES
  "${CMAKE_BINARY_DIR}/gensrc"
  "${CMAKE_CURRENT_LIST_DIR}/src"
  CACHE INTERNAL ""
)

# Configurate header function and 
function(trident_reconfigure)
    if(NOT DEFINED trident_config_header) 
        message(FATAL_ERROR "trident_config_header variable not defined")
    endif()
    if(NOT DEFINED trident_template_dir OR NOT IS_DIRECTORY "${trident_template_dir}" OR NOT EXISTS "${trident_template_dir}")
        message(FATAL_ERROR "trident_template_dir not valid: ${trident_template_dir}")
    endif()
    set(__config_hdr_file "${trident_template_dir}/trident_config.template.h")
    if(NOT EXISTS "${__config_hdr_file}")
        message(FATAL_ERROR "template file for trident configuration header does not exist: ${__config_hdr_file}")
    endif()
    
    configure_file("${__config_hdr_file}" ${trident_config_header})
endfunction()

set(trident_template_dir "${CMAKE_CURRENT_LIST_DIR}/cmake/templates" CACHE INTERNAL "")

trident_reconfigure()

