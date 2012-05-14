project(XtalOpt)

cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${XtalOpt_SOURCE_DIR}/cmake/modules)

set(QT_MIN_VERSION "4.6.0")
find_package(Qt4 REQUIRED)
find_package(OpenBabel2 REQUIRED)
find_package(Avogadro REQUIRED)

include(${Avogadro_USE_FILE})
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
		     ${OPENBABEL2_INCLUDE_DIR}
		     ${XtalOpt_SOURCE_DIR}/src/)

link_directories(${OPENBABEL2_LIBRARIES_DIRS})
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
    message(STATUS "LibSSH found: ${LibSSH_VERSION} ${LIBSSH_INCLUDE_DIRS} ${LIBSSH_LIBRARIES}")
    include_directories(${LIBSSH_INCLUDE_DIRS})
  endif(USE_CLI_SSH)
endif()

# This is a modified version of the avogadro_plugin function that produces
# a SHARED library instead of a MODULE
function(process_plugin plugin_name src_list)
  string(REPLACE ".cpp" ".h" hdr_list "${src_list}")
  qt4_wrap_cpp(moc_files ${hdr_list})
  if(NOT "${ARGV2}" STREQUAL "")
    # Process the UI file for this plugin
    qt4_wrap_ui(plugin_UIS_H ${ARGV2})
  endif(NOT "${ARGV2}" STREQUAL "")
  if(NOT "${ARGV3}" STREQUAL "")
    # Process the RC file and add it to the plugin
    qt4_add_resources(plugin_RC_SRCS ${ARGV3})
  endif(NOT "${ARGV3}" STREQUAL "")
  # Build static lib for linking to tests
  if(BUILD_TESTS)
    add_library(${plugin_name}_static STATIC ${src_list} ${plugin_UIS_H}
                ${plugin_RC_SRCS} ${moc_files})
    target_link_libraries(${plugin_name}_static avogadro)
  endif(BUILD_TESTS)
  add_library(${plugin_name} SHARED ${src_list} ${plugin_UIS_H}
              ${plugin_RC_SRCS} ${moc_files})
  target_link_libraries(${plugin_name} avogadro)

  install(TARGETS ${plugin_name} DESTINATION "${Avogadro_PLUGIN_DIR}/contrib")

  set_target_properties(${plugin_name} PROPERTIES
                        OUTPUT_NAME ${plugin_name}
                        PREFIX "")
endfunction(process_plugin)

# Set -fPIC on x86_64
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")

# USING_DYNAMIC_LIBS needs to be defined for OpenBabel to properly export some symbols
if (MSVC)
  add_definitions(-DUSING_DYNAMIC_LIBS)
endif (MSVC)

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif(BUILD_TESTS)
