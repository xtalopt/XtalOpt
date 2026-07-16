cmake_minimum_required(VERSION 3.12...3.27)

project(randSpg)

set(randSpg_SRCS
    src/crystal.cpp
    src/elemInfo.cpp
    src/randSpgCombinatorics.cpp
    src/randSpg.cpp)

add_library (RandSpg ${randSpg_SRCS})
target_include_directories(RandSpg PRIVATE ${randSpg_SOURCE_DIR}/include)

# C++11 is enforced by CMAKE_CXX_STANDARD set in the parent project.

# Always compile the generator with optimizations, even when no build type
#   is set; skipped for test builds (same behavior as before).
if(NOT BUILD_TESTS)
  target_compile_options(RandSpg PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>
    $<$<CXX_COMPILER_ID:MSVC>:/Ox>
  )
endif()

