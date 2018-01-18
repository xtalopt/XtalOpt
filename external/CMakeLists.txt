
include(ExternalProject)

if(ENABLE_XTALOPT)
  add_subdirectory(pugixml)
  add_subdirectory(randSpg)
  add_subdirectory(spglib)
  add_subdirectory(xtalcomp)
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

  find_package(RDKit REQUIRED)
  set(RDKit_INCLUDE_DIRS ${RDKit_INCLUDE_DIRS} PARENT_SCOPE)
  set(RDKit_LIBRARIES ${RDKit_LIBRARIES} PARENT_SCOPE)

  message(STATUS "RDKit found.")
endif()
