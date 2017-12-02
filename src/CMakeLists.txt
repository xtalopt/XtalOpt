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
