
include(ExternalProject)

if(ENABLE_XTALOPT)
  add_subdirectory(spglib)
  add_subdirectory(pugixml)
endif(ENABLE_XTALOPT)

if(ENABLE_MOLECULAR)
  find_package(RDKit)
  if(NOT RDKit_FOUND)

    message(STATUS "RDKit not found. Attempting to use RDKit submodule.")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rdkit")
      message(STATUS "RDKit not found, and RDKit submodule is not cloned")
      message(STATUS "Either install RDKit in your system and add it to your path, or clone the RDKit submodule")
      message(STATUS "If you wish to use the RDKit submodule, run the following command in the XtalOpt directory: 'git submodule update --init --recursive'")
      message(FATAL_ERROR "Exiting")
    endif()

    # First, try to find Boost
    find_package(Boost COMPONENTS regex system thread)

    # If we haven't found it, we will need to download it as well
    if(NOT Boost_FOUND)
      message(STATUS "Boost (for RDKit) not found. Downloading and compiling it separately")
      include(External_boost)
    else()
      message(STATUS "Boost found")
      message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
      message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
    endif()

    # This will automatically try to find Boost itself
    include(External_rdkit)

  else(NOT RDKit_FOUND)
    include(RDKitLibraries)
    message(STATUS "RDKit found.")
    message(STATUS "RDKit include dirs: ${RDKit_INCLUDE_DIRS}")

    set(RDKit_LIBRARY_DIRS "${RDKit_INCLUDE_DIRS}/../lib")
    message(STATUS "RDKit library dirs: ${RDKit_LIBRARY_DIRS}")
    message(STATUS "RDKit libraries: ${RDKit_LIBRARIES}")
    # Propagate the variables up one level
    set(RDKit_INCLUDE_DIRS "${RDKit_INCLUDE_DIRS}" PARENT_SCOPE)
    set(RDKit_LIBRARY_DIRS "${RDKit_LIBRARY_DIRS}" PARENT_SCOPE)
    set(RDKit_LIBRARIES "${RDKit_LIBRARIES}" PARENT_SCOPE)
  endif(NOT RDKit_FOUND)
endif()
