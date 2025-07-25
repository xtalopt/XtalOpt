cmake_minimum_required(VERSION 3.0...3.27)
project(XtalOpt)

set(XtalOpt_VERSION_MAJOR 14)
set(XtalOpt_VERSION_MINOR 2)
set(XtalOpt_VERSION_PATCH 0)

# Version definitions
set(XtalOpt_VERSION "${XtalOpt_VERSION_MAJOR}.${XtalOpt_VERSION_MINOR}")
set(XtalOpt_VERSION_FULL "${XtalOpt_VERSION}.${XtalOpt_VERSION_PATCH}")
# The string variable "XTALOPT_VER" can be used anywhere inside code
add_definitions(-DXTALOPT_VER="${XtalOpt_VERSION_FULL}")

set(CMAKE_MODULE_PATH ${XtalOpt_SOURCE_DIR}/cmake)

# Place xtalopt.exe in build dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# C++11 is required
include(MacroUseCXX11)
use_cxx11()

# Use CPACK
include(XtalOptCPack)

# Automoc, AutoUIC, and AutoRCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Concurrent Network Widgets REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Qwt REQUIRED)
find_package(Threads REQUIRED)
include_directories(${QWT_INCLUDE_DIRS})

include_directories(${Qt5Network_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# Include Eigen3 as a system dir so we can ignore compiler warnings from it
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${XtalOpt_SOURCE_DIR}/src/)

include_directories(external/qhull/src)

include(MacroEnsureVersion)

option( XTALOPT_DEBUG
  "Enable writing output.log file to the working directory"
  OFF )
if(XTALOPT_DEBUG)
  add_definitions(-DXTALOPT_DEBUG)
endif(XTALOPT_DEBUG)

option( BUILD_TESTS
 	"Whether to compile the test suite as well as the main code."
 	OFF )

if(BUILD_TESTS)
  enable_testing()

endif(BUILD_TESTS)

option( ENABLE_SSH
 	"Enable SSH. Only the local queue interface will be available if disabled."
 	ON )

option( USE_CLI_SSH
 	"Use command line ssh/scp commands for remote communication. Use this if on linux/mac and Kerberos authentication is needed."
 	OFF )

option( ENABLE_RPATH
        "Enable rpath support on Linux and Mac. This will automatically be turned on if BUILD_INDEPENDENT_PACKAGE is true"
        ON )

option( INSTALL_DEPENDENCIES
        "Attempt to automatically find and install non-system dependencies to CMAKE_INSTALL_PREFIX/lib. This will automatically be turned on if BUILD_INDEPENDENT_PACKAGE is true"
        OFF )

option ( BUILD_INDEPENDENT_PACKAGE
         "Builds an independent package to be distributed. This will automatically switch on ENABLE_RPATH and INSTALL_DEPENDENCIES"
         OFF )

option( USE_SYSTEM_OBABEL
        "Instead of downloading a static obabel, use a system obabel instead? You should set the environment variable OBABEL_EXECUTABLE before running the program if you choose to do this"
        OFF )
option( USE_SYSTEM_GENXRDPATTERN
        "Instead of downloading a static genXrdPattern, use a system genXrdPattern instead? You should set the environment variable GENXRDPATTERN_EXECUTABLE before running the program if you choose to do this"
        OFF )

# This will only actually download if USE_SYSTEM_OBABEL is off
include(DownloadOBabel)
DownloadObabel()

# This will only actually download if USE_SYSTEM_GENXRDPATTERN is off
include(DownloadGenXrdPattern)
DownloadGenXrdPattern()

if(BUILD_INDEPENDENT_PACKAGE)
  set(ENABLE_RPATH ON)
  set(INSTALL_DEPENDENCIES ON)
endif(BUILD_INDEPENDENT_PACKAGE)

if(ENABLE_SSH)
  add_definitions(-DENABLE_SSH)
  message(STATUS "SSH Enabled")
  # Pull in libssh
  if(USE_CLI_SSH)
    add_definitions(-DUSE_CLI_SSH)
    message(STATUS "Using command-line SSH interface")
  else(USE_CLI_SSH)
    message(STATUS "Using libssh SSH interface")
    set(LibSSH_FIND_VERSION ON)
    set(LibSSH_MIN_VERSION "0.4.8")
    find_package(LibSSH REQUIRED)
    if(NOT LIBSSH_FOUND)
      message(FATAL_ERROR "libssh not found!")
    endif()
    macro_ensure_version(${LibSSH_MIN_VERSION} ${LibSSH_VERSION}
                         LIBSSH_VERSION_OK)
    if(NOT LIBSSH_VERSION_OK)
      message(FATAL_ERROR
      "libssh too old! Installed version is ${LibSSH_VERSION}, need at least "
      "libssh ${LibSSH_MIN_VERSION}")
    endif()
    include_directories(${LIBSSH_INCLUDE_DIRS})
  endif(USE_CLI_SSH)
endif()

# Set -fPIC on x86_64
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")

add_subdirectory(external)
include_directories(external)

# Use this to set the DEP_SEARCH_DIRS variable for searching for dependencies
include(DepSearchDirs)

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif(BUILD_TESTS)
