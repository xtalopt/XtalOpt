option( ENABLE_EXAMPLESEARCH
	"Build the examplesearch extension"
	OFF )
option( ENABLE_XTALOPT
	"Build the XtalOpt extension"
	ON )
option( ENABLE_RANDOMDOCK
	"Build the RandomDock extension"
	OFF )
option( ENABLE_GAPC
	"Build the evolutionary cluster search extension"
	OFF )

add_subdirectory(globalsearch)

if (ENABLE_EXAMPLESEARCH)
   add_subdirectory(examplesearch)
endif (ENABLE_EXAMPLESEARCH)

if (ENABLE_XTALOPT)
   add_subdirectory(randSpg)
   # If we are using MSVC, we should be nesting xtalopt within Avogadro,
   # so spglib will already have been built (it is part of the Avogadro
   # package already).
   if(NOT MSVC)
     add_subdirectory(spglib)
   endif(NOT MSVC)
   add_subdirectory(xtalcomp)
   add_subdirectory(xtalopt)
endif (ENABLE_XTALOPT)

if (ENABLE_RANDOMDOCK)
   add_subdirectory(randomdock)
endif (ENABLE_RANDOMDOCK)

if (ENABLE_GAPC)
   add_subdirectory(gapc)
endif (ENABLE_GAPC)
