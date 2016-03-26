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
endif()

option( BUILD_CGI
        "Whether to compile the CGI handler in addition to the spgGen code."
        OFF )
if(BUILD_CGI)
  add_executable (spgGen.cgi cgi/spgGen-cgi.cpp)
  target_link_libraries (spgGen.cgi SpgGenerate)
endif(BUILD_CGI)
