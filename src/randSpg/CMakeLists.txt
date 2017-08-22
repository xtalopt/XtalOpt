project(randSpg)

cmake_minimum_required(VERSION 2.6)

set(randSpg_SRCS
    src/crystal.cpp
    src/elemInfo.cpp
    src/randSpgCombinatorics.cpp
    src/randSpg.cpp)

include_directories(${randSpg_SOURCE_DIR}/include)

add_library (RandSpg ${randSpg_SRCS})

# C++11 is required. MSVC should not need a flag
# If we are using APPLE, we have to use c++11 flags when we
# first execute cmake
if(UNIX AND NOT APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif(UNIX AND NOT APPLE)

option(BUILD_RELEASE
       "Whether to compile with optimizations or not"
       ON)

if(BUILD_TESTS)
  set(BUILD_RELEASE OFF)
endif()

# Let's perform some optimizations!
if (BUILD_RELEASE)
  if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  endif(UNIX)
  if(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
  endif(MSVC)
else (BUILD_RELEASE)

endif (BUILD_RELEASE)

