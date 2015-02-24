# Install script for directory: /Users/Zeek/src/xtalopt-release9/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/Zeek")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/Zeek/src/xtalopt-release9/build/src/globalsearch/cmake_install.cmake")
  include("/Users/Zeek/src/xtalopt-release9/build/src/spglib/cmake_install.cmake")
  include("/Users/Zeek/src/xtalopt-release9/build/src/xtalcomp/cmake_install.cmake")
  include("/Users/Zeek/src/xtalopt-release9/build/src/xtalopt/cmake_install.cmake")
  include("/Users/Zeek/src/xtalopt-release9/build/src/randomdock/cmake_install.cmake")

endif()

