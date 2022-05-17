cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(pa2 VERSION 1.0)

add_executable(
    pa2
    ./src/config.cpp
    ./src/cost.cpp
    ./src/data.cpp
    ./src/main.cpp
    ./src/partition.cpp
    ./src/sa.cpp
    ./src/starting_partition.cpp
)
set_property(TARGET pa2 PROPERTY CXX_STANDARD 17)

# External libs
include_directories(
  external/GSL/include
  external/fmt/include
  external/range-v3/include
  external/parallel-hashmap
)

# LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if( supported )
  message(STATUS "LTO enabled")
  set_property(TARGET pa2 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "LTO not supported: ${error}")
endif()


# Linux-specific options
if(NOT WIN32)
  add_compile_options(-Wall -Wextra)
  install(TARGETS pa2 DESTINATION bin)
endif()

# Export compile commands for clangd
if(DEFINED ENV{CMAKE_EXPORT_COMPILE_COMMANDS})
  message(STATUS "Exporting compile commands")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()
