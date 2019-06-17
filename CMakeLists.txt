cmake_minimum_required(VERSION 3.0)
project(XtalOpt)

set(XtalOpt_VERSION_MAJOR 12)
set(XtalOpt_VERSION_MINOR 1)
set(XtalOpt_VERSION_PATCH 0)
set(XtalOpt_VERSION "${XtalOpt_VERSION_MAJOR}.${XtalOpt_VERSION_MINOR}")
set(XtalOpt_VERSION_FULL "${XtalOpt_VERSION}.${XtalOpt_VERSION_PATCH}")

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

include(MacroEnsureVersion)

option( BUILD_TESTS
 	"Whether to compile the test suite as well as the main code."
 	OFF )

if(BUILD_TESTS)
  enable_testing()

  option( RUN_CODE_COV
  "If this is true, then code coverage will be ran."
  ON )

  # We will only run codecov stuff on gcc
  if(CMAKE_COMPILER_IS_GNUCXX AND RUN_CODE_COV)
    string(CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -g -O0 "
                                  "-fprofile-arcs -ftest-coverage")
    string(CONCAT CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -g -O0 "
                                "-fprofile-arcs -ftest-coverage")
    string(CONCAT CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} "
                                         "-fprofile-arcs -ftest-coverage")
  endif(CMAKE_COMPILER_IS_GNUCXX AND RUN_CODE_COV)
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

# The main packages
option( ENABLE_EXAMPLESEARCH
  "Build the examplesearch extension - DOES NOT WORK CURRENTLY"
  OFF )
option( ENABLE_XTALOPT
  "Build the XtalOpt extension"
  ON )
option( ENABLE_RANDOMDOCK
  "Build the RandomDock extension - DOES NOT WORK CURRENTLY"
  OFF )
option( ENABLE_GAPC
  "Build the evolutionary cluster search extension - DOES NOT WORK CURRENTLY"
  OFF )

option(ENABLE_MOLECULAR
       "Whether or not to compile the molecular version of XtalOpt"
       OFF)

option(RDKIT_STATIC
       "Whether or not to statically compile RDKit. Does nothing if ENABLE_MOLECULAR is off"
       OFF)

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

if(ENABLE_MOLECULAR)
  add_definitions(-DENABLE_MOLECULAR)
endif(ENABLE_MOLECULAR)

add_subdirectory(external)
include_directories(external)

# Use this to set the DEP_SEARCH_DIRS variable for searching for dependencies
include(DepSearchDirs)

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif(BUILD_TESTS)
