option( ENABLE_XTALOPT
	"Build the XtalOpt extension"
	ON )
option( ENABLE_RANDOMDOCK
	"Build the RandomDock extension"
	OFF )


add_subdirectory(libssh)
add_subdirectory(globalsearch)

if (ENABLE_XTALOPT)
  add_subdirectory(spglib)
  add_subdirectory(xtalopt)
endif (ENABLE_XTALOPT)

if (ENABLE_RANDOMDOCK)
  add_subdirectory(randomdock)
endif (ENABLE_RANDOMDOCK)
