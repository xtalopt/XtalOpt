cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_package(PythonInterp 3 REQUIRED)

set(PYBIND11_CPP_STANDARD -std=c++11)

find_package(pybind11 REQUIRED)

message(STATUS "Found pybind11: ${pybind11_VERSION}")
include_directories(${PYBIND11_INCLUDE_DIR})

include_directories(${Qt5Concurrent_INCLUDE_DIRS})

pybind11_add_module(xtaloptpybind MODULE xtaloptpybind.cpp)
target_link_libraries(xtaloptpybind PRIVATE xtalopt_static)
