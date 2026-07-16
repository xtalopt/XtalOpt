# No Qt code anywhere below; keep the root's AUTOMOC/AUTOUIC/AUTORCC from
#   generating pointless autogen steps for the external libraries.
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

add_subdirectory(xtalcomp)
add_subdirectory(randspg)
add_subdirectory(spglib)
add_subdirectory(qhull)
add_subdirectory(xraylib)
add_subdirectory(libmsym)

add_library(args STATIC args/src/args.c)
target_include_directories(args SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/args/src)
