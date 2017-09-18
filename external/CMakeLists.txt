
include(ExternalProject)

if(ENABLE_XTALOPT)
  add_subdirectory(spglib)
  add_subdirectory(pugixml)
endif(ENABLE_XTALOPT)

if(ENABLE_MOLECULAR)

  # RDKit requires boost, so we must find boost first
  if(NOT Boost_INCLUDE_DIRS OR NOT Boost_LIBRARY_DIRS)
    message(STATUS "Boost_INCLUDE_DIRS or Boost_LIBRARIES is not set. Attempting to find Boost with find_package()")
    find_package(Boost COMPONENTS regex system thread REQUIRED)
    set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} PARENT_SCOPE)
    set(Boost_LIBRARY_DIRS ${Boost_LIBRARY_DIRS} PARENT_SCOPE)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} PARENT_SCOPE)
  else()
    set(Boost_LIBRARIES
        boost_regex
        boost_system
        boost_thread
    )
    set(Boost_LIBRARIES ${Boost_LIBRARIES} PARENT_SCOPE)
  endif()

  message(STATUS "Boost found")
  message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost library dirs: ${Boost_LIBRARY_DIRS}")
  message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

  if(NOT RDKit_INCLUDE_DIRS OR NOT RDKit_LIBRARY_DIRS)
    message(STATUS "RDKit_INCLUDE_DIRS or RDKit_LIBRARY_DIRS is not set. Attempting to find RDKit with find_package()")
    find_package(RDKit REQUIRED)
    set(RDKit_INCLUDE_DIRS ${RDKit_INCLUDE_DIRS} PARENT_SCOPE)
    if(RDKit_CONFIG)
      get_filename_component(RDKit_LIBRARY_DIRS ${RDKit_CONFIG} DIRECTORY)

      # This might be the case if the config file is in a build directory
      # Set the library dir to the "lib" directory if it exists
      if(EXISTS "${RDKit_LIBRARY_DIRS}/lib")
        set(RDKit_LIBRARY_DIRS "${RDKit_LIBRARY_DIRS}/lib")
      endif()

      # Otherwise, we'll just assume the library dir is the config dir
    else()
      # Let's hope it is one directory up. If it isn't, we'll get linking errors
      set(RDKit_LIBRARY_DIRS "${RDKit_INCLUDE_DIRS}/../lib")
    endif()
    set(RDKit_LIBRARY_DIRS ${RDKit_LIBRARY_DIRS} PARENT_SCOPE)
  endif()

  include(RDKitLibraries)
  set(RDKit_LIBRARIES ${RDKit_LIBRARIES} PARENT_SCOPE)

  message(STATUS "RDKit found.")
  message(STATUS "RDKit include dirs: ${RDKit_INCLUDE_DIRS}")
  message(STATUS "RDKit library dirs: ${RDKit_LIBRARY_DIRS}")
  message(STATUS "RDKit libraries: ${RDKit_LIBRARIES}")
endif()
