cmake_minimum_required(VERSION 3.15)

project(aoc-mmmod
  LANGUAGES C
)

if (NOT MSVC)
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()

set(CMAKE_C_STANDARD 11)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT error)
if (IPO_SUPPORTED)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (NOT MSVC)
  add_compile_options(-m32)
  add_compile_options(-Wall)
endif()
add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(NOMINMAX)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
if (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
  add_compile_definitions(NDEBUG)
endif()

add_library(aoc-mmmod SHARED)
target_compile_definitions(aoc-mmmod
  PRIVATE MMMOD_STANDALONE
)
target_sources(aoc-mmmod
  PRIVATE hook.c
          main.c
  PUBLIC mmmod.h
)
set_target_properties(aoc-mmmod PROPERTIES
    PREFIX ""
    OUTPUT_NAME "language_x1_p1"
    SUFFIX ".dll"
    POSITION_INDEPENDENT_CODE OFF)

add_library(aoc-mmmod-voobly SHARED)
target_compile_definitions(aoc-mmmod-voobly
  PRIVATE MMMOD_VOOBLY
)
target_sources(aoc-mmmod-voobly
  PRIVATE hook.c
          main.c
  PUBLIC mmmod.h
)
set_target_properties(aoc-mmmod-voobly PROPERTIES
    PREFIX ""
    OUTPUT_NAME "userpatch"
    SUFFIX ".dll"
    POSITION_INDEPENDENT_CODE OFF)
