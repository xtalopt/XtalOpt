option( ENABLE_XTALOPT
	"Build the XtalOpt extension"
	ON )
option( ENABLE_RANDOMDOCK
	"Build the RandomDock extension"
	OFF )


add_subdirectory(generic)
add_subdirectory(spglib)

if (ENABLE_XTALOPT)
  add_subdirectory(xtalopt)
endif (ENABLE_XTALOPT)

if (ENABLE_RANDOMDOCK)
  add_subdirectory(randomdock)
endif (ENABLE_RANDOMDOCK)
