# String version variable "XTALOPT_VER" for the use anywhere inside the source code
add_definitions(-DXTALOPT_VER="${XtalOpt_VERSION_MAJOR}.${XtalOpt_VERSION_MINOR}")

add_subdirectory(globalsearch)

if (ENABLE_EXAMPLESEARCH)
   add_subdirectory(examplesearch)
endif (ENABLE_EXAMPLESEARCH)

if (ENABLE_XTALOPT)
   add_subdirectory(xtalopt)
endif (ENABLE_XTALOPT)

if (ENABLE_RANDOMDOCK)
   add_subdirectory(randomdock)
endif (ENABLE_RANDOMDOCK)

if (ENABLE_GAPC)
   add_subdirectory(gapc)
endif (ENABLE_GAPC)
