
# - find RDKit
# RDKit_INCLUDE_DIRS - Where to find RDKit header files (directory)
# RDKit_LIBRARY_DIRS - Where to find RDKit library files (directory)
# RDKit_FOUND - Set to TRUE if we found everything (library and includes)

# If RDKitROOT or RDBASE are set, they will be searched

# Copyright (c) 2010 Tim Vandermeersch
# Copyright (c) 2017 Patrick Avery

if(RDKit_INCLUDE_DIRS AND RDKit_LIBRARY_DIRS)
  set(RDKit_FOUND TRUE)
  return()
endif()

set(_include_search_paths "")
set(_library_search_paths "")

if(UNIX)
  set(_include_search_paths
      /usr/include/rdkit
  )
  set(_library_search_paths
      /usr/lib
  )
endif(UNIX)

find_path(RDKit_INCLUDE_DIRS GraphMol/Atom.h
          HINTS ${RDKit_ROOT}/Code
                ${RDBASE}/Code
          PATHS ${_include_search_paths})

find_library(RDKit_LIBRARY_DIRS RDKitGraphMol
             HINTS ${RDKit_ROOT}/lib
                   ${RDBASE}/lib
             PATHS ${_include_search_paths})

# Get the directory component of the file name
get_filename_component(RDKit_LIBRARY_DIRS ${RDKit_LIBRARY_DIRS} DIRECTORY)

if(RDKit_INCLUDE_DIRS AND RDKit_LIBRARY_DIRS)
  set(RDKit_FOUND TRUE)
endif()

if(RDKit_FOUND)
  message(STATUS "Found RDKit header file: ${RDKit_INCLUDE_DIRS}")
  message(STATUS "Found RDKit libraries: ${RDKit_LIBRARY_DIRS}")
else(RDKit_FOUND)
  if(RDKit_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find RDKit")
  else(RDKit_FIND_REQUIRED)
    message(STATUS "Optional package RDKit was not found")
  endif(RDKit_FIND_REQUIRED)
endif(RDKit_FOUND)
