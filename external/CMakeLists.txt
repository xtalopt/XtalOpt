
include(ExternalProject)

if(ENABLE_XTALOPT)
  add_subdirectory(pugixml)
  add_subdirectory(randSpg)
  add_subdirectory(spglib)
  add_subdirectory(xtalcomp)
endif(ENABLE_XTALOPT)
