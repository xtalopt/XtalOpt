project(XtalOpt)

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${XtalOpt_SOURCE_DIR}/cmake/modules)

# Place xtalopt.exe in build dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# C++11 is required
include(MacroUseCXX11)
use_cxx11()

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Concurrent Widgets REQUIRED)
find_package(Eigen3 REQUIRED)

include_directories(${Qt5Widgets_INCLUDE_DIRS})

include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${EIGEN3_INCLUDE_DIR}
		     ${XtalOpt_SOURCE_DIR}/src/)

include(MacroEnsureVersion)

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
    include_directories(${LIBSS_INCLUDE_DIRS})
  endif(USE_CLI_SSH)
endif()

# Set -fPIC on x86_64
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif(BUILD_TESTS)
