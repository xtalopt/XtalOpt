cmake_minimum_required(VERSION 3.12...3.31)
project(XtalOpt)

set(XtalOpt_VERSION_MAJOR 15)
set(XtalOpt_VERSION_MINOR 0)
set(XtalOpt_VERSION_PATCH 0.rc10)

# Version definitions
set(XtalOpt_VERSION "${XtalOpt_VERSION_MAJOR}.${XtalOpt_VERSION_MINOR}")
set(XtalOpt_VERSION_FULL "${XtalOpt_VERSION}.${XtalOpt_VERSION_PATCH}")

# Version definitions for the XtalOpt code itself ("XTALOPT_VER" can be used in src/ and tests/).
list(APPEND XTALOPT_COMPILE_DEFINITIONS XTALOPT_VER="${XtalOpt_VERSION_FULL}")

list(APPEND CMAKE_MODULE_PATH ${XtalOpt_SOURCE_DIR}/cmake)

# Place xtalopt.exe in build dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_custom_target(check_source_hierarchy
  COMMAND "${CMAKE_COMMAND}"
          "-DXTALOPT_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
          -P "${CMAKE_SOURCE_DIR}/cmake/CheckSourceHierarchy.cmake"
  COMMENT "Checking source hierarchy"
  VERBATIM)

# Require C++11 (Qt6 builds its own targets with a newer standard).
include(MacroUseCXX11)
use_cxx11()

# Automoc, AutoUIC, and AutoRCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(BUILD_XTALOPT_GUI
  "Build the XtalOpt GUI (requires Qt Widgets and Qwt)"
  OFF)

# Support Qt5.4+ and Qt6, prefer Qt6.
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Concurrent REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Concurrent REQUIRED)
if(BUILD_XTALOPT_GUI)
  find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
endif()

if(QT_VERSION_MAJOR EQUAL 5)
  set(_qt_detected_version
      "${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
  if(_qt_detected_version VERSION_LESS "5.4.0")
    message(FATAL_ERROR
            "Qt 5.4.0 or newer is required by the supported Qt "
            "compatibility baseline for this project.")
  endif()
endif()
message(STATUS "Using Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
list(APPEND XTALOPT_COMPILE_DEFINITIONS
     QT_VER="${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

if(BUILD_XTALOPT_GUI)
  list(APPEND XTALOPT_COMPILE_DEFINITIONS BUILD_XTALOPT_GUI)
  # Support Qwt 6.1 or later, built with the current Qt version.
  find_package(Qwt 6.1 REQUIRED)
  # The string variable "QWT_VER" can be used anywhere inside code
  message(STATUS "Using Qwt ${QWT_VERSION_STRING}")
  list(APPEND XTALOPT_COMPILE_DEFINITIONS QWT_VER="${QWT_VERSION_STRING}")

  # Use one Qwt target: system includes keep project warnings out of Qwt.
  add_library(Qwt::Qwt INTERFACE IMPORTED)
  set_target_properties(Qwt::Qwt PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${QWT_INCLUDE_DIRS}"
    INTERFACE_INCLUDE_DIRECTORIES "${QWT_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${QWT_LIBRARIES}")
  if(WIN32)
    if(QWT_IS_STATIC)
      set_property(TARGET Qwt::Qwt APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS QWT_STATIC)
    else()
      set_property(TARGET Qwt::Qwt APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS QWT_DLL)
    endif()
  endif()
else()
  list(APPEND XTALOPT_COMPILE_DEFINITIONS QWT_VER="N/A")
endif()
# engine and application includes are now target scoped.

find_package(Threads REQUIRED)

option( BUILD_TESTS
 	"Whether to compile the test suite as well as the main code."
 	OFF )

if(BUILD_TESTS)
  enable_testing()

endif(BUILD_TESTS)

option( BUILD_WITH_LIBSSH
  "Build the optional libssh method in addition to the always-available system ssh/scp method."
  OFF )
set(ENABLE_LIBSSH ${BUILD_WITH_LIBSSH})
unset(ENABLE_LIBSSH CACHE)

option( ENABLE_RPATH
        "Enable rpath support on Linux and Mac. This will automatically be turned on if BUILD_INDEPENDENT_PACKAGE is true"
        ON )

option( BUILD_INDEPENDENT_PACKAGE
        "Builds a self-contained, portable package. Bundles all non-system runtime dependencies into PREFIX/lib and sets RPATH accordingly."
        OFF )

if(APPLE)
  option( XTALOPT_MACOS_SIGN_BUNDLE
          "Ad-hoc sign the macOS app bundle after fixup_bundle rewrites its binaries during install and packaging."
          ON )
else()
  option( XTALOPT_MACOS_SIGN_BUNDLE
          "Ad-hoc sign the macOS app bundle after fixup_bundle rewrites its binaries during install and packaging."
          OFF )
endif()
set(XTALOPT_MACOS_CODESIGN_IDENTITY "-"
    CACHE STRING "macOS code signing identity used for the installed and packaged app bundle. Use '-' for ad-hoc signing.")

if(BUILD_INDEPENDENT_PACKAGE)
  set(ENABLE_RPATH ON)
endif(BUILD_INDEPENDENT_PACKAGE)

if(ENABLE_LIBSSH)
  message(STATUS "Using libssh + system ssh/scp SSH interfaces")
  list(APPEND XTALOPT_COMPILE_DEFINITIONS ENABLE_LIBSSH)
  # Older libssh needs the pthreads backend for thread safety; Windows
  #   builds lack it, so they require a libssh that handles this itself.
  if(WIN32)
    set(LibSSH_MIN_VERSION "0.8.0")
  else()
    set(LibSSH_MIN_VERSION "0.6.0")
  endif()
  find_package(LibSSH ${LibSSH_MIN_VERSION} REQUIRED)
  # src/search now sets the libssh include directory.
  list(APPEND XTALOPT_COMPILE_DEFINITIONS SSH_VER="${LibSSH_VERSION}+system")
else()
  message(STATUS "Using system ssh/scp SSH interface")
  list(APPEND XTALOPT_COMPILE_DEFINITIONS SSH_VER="system")
endif()

# Use CPack after the build options are set: package filename includes gui/ssh build types.
include(XtalOptCPack)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(external)
# src/xtalopt now sets external include directories.

# Use this to set the DEP_SEARCH_DIRS variable for searching for dependencies
include(DepSearchDirs)

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(tests)
  add_test(NAME check_source_hierarchy
           COMMAND "${CMAKE_COMMAND}"
                   "-DXTALOPT_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
                   -P "${CMAKE_SOURCE_DIR}/cmake/CheckSourceHierarchy.cmake")
endif(BUILD_TESTS)

foreach(_xtalopt_hierarchy_target
    common_core
    common_math
    atoms_core
    atoms_gui
    generatexrd_core
    generatexrd_gui
    search_core
    search_cli
    search_gui
    xtalopt_core
    xtalopt_gui
    xtalopt
    structuretool)
  if(TARGET "${_xtalopt_hierarchy_target}")
    add_dependencies("${_xtalopt_hierarchy_target}" check_source_hierarchy)
  endif()
endforeach()
