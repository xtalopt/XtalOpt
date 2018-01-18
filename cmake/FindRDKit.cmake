
# - find RDKit
# RDKit_INCLUDE_DIRS - Where to find RDKit header files (directory)
# RDKit_LIBRARY_DIRS - Where to find RDKit library files (directory)
# RDKit_FOUND - Set to TRUE if we found everything (library and includes)

# If RDKitROOT or RDBASE are set, they will be searched

# Copyright (c) 2010 Tim Vandermeersch
# Copyright (c) 2017 Patrick Avery

if(RDKit_INCLUDE_DIRS AND RDKit_LIBRARIES)
  set(RDKit_FOUND TRUE)
  return()
endif()

set(_include_search_paths "")
set(_library_search_paths "")

if(UNIX)
  set(_include_search_paths
      /usr/include/rdkit
      /usr/local/include/rdkit
  )
  set(_library_search_paths
      /usr/lib
      /usr/local/lib
  )
endif(UNIX)

find_path(RDKit_INCLUDE_DIRS GraphMol/Atom.h
          HINTS ${RDKit_ROOT}/Code
                ${RDBASE}/Code
                ${RDBASE}/include
                ${RDBASE}/include/rdkit
          PATHS ${_include_search_paths})

# This will set RDKit_LIBRARIES to contain all the library names
include(RDKitLibraries)

# We need to find each one
foreach(_lib ${RDKit_LIBRARIES})
  find_library(_tmplib ${_lib}
               HINTS ${RDKit_ROOT}/lib
                     ${RDKit_ROOT}/build/lib
                     ${RDBASE}/lib
                     ${RDBASE}/build/lib
               PATHS ${_include_search_paths})

  if("${_tmplib}" STREQUAL "_tmplib-NOTFOUND")
    message(SEND_ERROR "Could not find ${_lib}")
    set(_rdk_libs "")
    break()
  endif()

  set(_rdk_libs ${_rdk_libs} ${_tmplib})
  unset(_tmplib CACHE)
endforeach()

set(RDKit_LIBRARIES ${_rdk_libs})

if(RDKit_INCLUDE_DIRS AND RDKit_LIBRARIES)
  set(RDKit_FOUND TRUE)
endif()

if(RDKit_FOUND)
  message(STATUS "Found RDKit header file: ${RDKit_INCLUDE_DIRS}")
  message(STATUS "Found RDKit libraries: ${RDKit_LIBRARIES}")
else(RDKit_FOUND)
  if(RDKit_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find RDKit")
  else(RDKit_FIND_REQUIRED)
    message(STATUS "Optional package RDKit was not found")
  endif(RDKit_FIND_REQUIRED)
endif(RDKit_FOUND)
