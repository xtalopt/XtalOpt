cmake_minimum_required(VERSION 3.12...3.31)

project (spglib)

# C
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build type - default to Release if not specified by the user: spglib does
#   the heavy symmetry work and would otherwise be built without any
#   optimization. This only applies to this directory, not the whole project.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Source files
set(SPGLIB_SOURCES
        arithmetic.c
        cell.c
        debug.c
        delaunay.c
        determination.c
        hall_symbol.c
        kgrid.c
        kpoint.c
        magnetic_spacegroup.c
        mathfunc.c
        msg_database.c
        niggli.c
        overlap.c
        pointgroup.c
        primitive.c
        refinement.c
        site_symmetry.c
        sitesym_database.c
        spacegroup.c
        spg_database.c
        spglib.c
        spin.c
        symmetry.c
)

# Library (static)
add_library(spglib STATIC ${SPGLIB_SOURCES})

# Suppress spglib warnings (GCC/Clang only; MSVC uses /W0 or defaults)
if(NOT MSVC)
  target_compile_options(spglib PRIVATE -w)
endif()

# We build spglib as a static library, so on Windows the headers must not
#   declare the functions dllimport.
if(WIN32)
  target_compile_definitions(spglib PUBLIC SPG_STATIC_LIBRARY)
endif()
