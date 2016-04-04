project(spgGen)

cmake_minimum_required(VERSION 2.6)

set(spgGen_SRCS
    src/crystal.cpp
    src/elemInfo.cpp
    src/spgGenCombinatorics.cpp
    src/spgGen.cpp)

include_directories(${spgGen_SOURCE_DIR}/include)

add_library (SpgGenerate ${spgGen_SRCS})

# C++11 is required. MSVC should not need a flag
if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif(UNIX)

option(BUILD_RELEASE
       "Whether to compile with optimizations or not"
       ON)

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

