# - Try to find LibSSH
# Once done this will define
#
#  LIBSSH_FOUND - system has LibSSH
#  LIBSSH_INCLUDE_DIRS - the LibSSH include directory
#  LIBSSH_LIBRARIES - Link these to use LibSSH
#
#  Copyright (c) 2009 Andreas Schneider <mail@cynapses.org>
#  Copyright (c) 2017 Patrick Avery (creation of _libssh_check_version macro)
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

macro(_libssh_check_version)
  if(EXISTS ${LIBSSH_INCLUDE_DIRS}/libssh/libssh_version.h)
    set(verlibssh "libssh_version.h")
  else()
    set(verlibssh "libssh.h")
  endif()
  file(STRINGS ${LIBSSH_INCLUDE_DIRS}/libssh/${verlibssh} LIBSSH_VERSION_MAJOR
    REGEX "#define[ ]+LIBSSH_VERSION_MAJOR[ ]+[0-9]+")
  # Older versions of libssh like libssh-0.2 have LIBSSH_VERSION but not LIBSSH_VERSION_MAJOR
  if(LIBSSH_VERSION_MAJOR)
    string(REGEX MATCH "[0-9]+" LIBSSH_VERSION_MAJOR ${LIBSSH_VERSION_MAJOR})
    file(STRINGS ${LIBSSH_INCLUDE_DIRS}/libssh/${verlibssh} LIBSSH_VERSION_MINOR
         REGEX "#define[ ]+LIBSSH_VERSION_MINOR[ ]+[0-9]+")
    string(REGEX MATCH "[0-9]+" LIBSSH_VERSION_MINOR ${LIBSSH_VERSION_MINOR})
    file(STRINGS ${LIBSSH_INCLUDE_DIRS}/libssh/${verlibssh} LIBSSH_VERSION_PATCH
         REGEX "#define[ ]+LIBSSH_VERSION_MICRO[ ]+[0-9]+")
    string(REGEX MATCH "[0-9]+" LIBSSH_VERSION_PATCH ${LIBSSH_VERSION_PATCH})

    set(LibSSH_VERSION ${LIBSSH_VERSION_MAJOR}.${LIBSSH_VERSION_MINOR}.${LIBSSH_VERSION_PATCH})

    include(FindPackageVersionCheck)
    find_package_version_check(LibSSH DEFAULT_MSG)
  else()
    message(STATUS "LIBSSH_VERSION_MAJOR not found in ${LIBSSH_INCLUDE_DIRS}/libssh/${verlibssh}, assuming libssh is too old")
    set(LIBSSH_FOUND FALSE)
  endif()
endmacro()

if(LIBSSH_LIBRARIES AND LIBSSH_INCLUDE_DIRS)
  # Use cached values.
  set(LIBSSH_FOUND TRUE)
  _libssh_check_version()
else()

  # Also check the MacPorts and Fink paths.
  find_path(LIBSSH_INCLUDE_DIR
    NAMES
      libssh/libssh.h
    PATHS
      /opt/local/include
      /sw/include
  )

  find_library(SSH_LIBRARY
    NAMES
      ssh
      libssh
    PATHS
      /opt/local/lib
      /sw/lib
  )

  set(LIBSSH_INCLUDE_DIRS
    ${LIBSSH_INCLUDE_DIR}
  )

  if(LIBSSH_INCLUDE_DIR AND SSH_LIBRARY)
    set(LIBSSH_LIBRARIES
      ${LIBSSH_LIBRARIES}
      ${SSH_LIBRARY}
    )

    set(LIBSSH_FOUND TRUE)

    if(LibSSH_FIND_VERSION)
      _libssh_check_version()
    endif()
  endif()

  # Hide these cache values in the advanced view.
  mark_as_advanced(LIBSSH_INCLUDE_DIR SSH_LIBRARY)

endif()

# A static libssh does not record its own dependencies; ask pkg-config
# for the full static link closure.
if(SSH_LIBRARY MATCHES "\\.a$")
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBSSH QUIET libssh)
  endif()
  if(PC_LIBSSH_STATIC_LDFLAGS)
    set(LIBSSH_LIBRARIES
      ${LIBSSH_LIBRARIES}
      ${PC_LIBSSH_STATIC_LDFLAGS}
    )
  else()
    message(WARNING
            "Found a static libssh (${SSH_LIBRARY}) but could not determine "
            "its dependencies; the crypto backend (and zlib) may need to be "
            "added to the link line manually.")
  endif()
endif()

# libssh older than 0.8.0 keeps its threading support in a separate
# libssh_threads library, which must be linked as well.
if(LIBSSH_FOUND AND LibSSH_VERSION AND LibSSH_VERSION VERSION_LESS "0.8.0")
  find_library(SSH_THREADS_LIBRARY
    NAMES
      ssh_threads
      libssh_threads
    PATHS
      /opt/local/lib
      /sw/lib
  )
  if(SSH_THREADS_LIBRARY)
    set(LIBSSH_LIBRARIES
      ${LIBSSH_LIBRARIES}
      ${SSH_THREADS_LIBRARY}
    )
  else()
    message(FATAL_ERROR
            "libssh ${LibSSH_VERSION} requires the libssh_threads library, "
            "which was not found.")
  endif()
  mark_as_advanced(SSH_THREADS_LIBRARY)
endif()

# Prevent an old libssh version from being accepted.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibSSH
  REQUIRED_VARS LIBSSH_LIBRARIES LIBSSH_INCLUDE_DIRS
  VERSION_VAR LibSSH_VERSION)
