
cmake_minimum_required(VERSION 3.10)

project (spglib)

# C
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build type
set(CMAKE_BUILD_TYPE Release)

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

# Suppress spglib warnings
set_target_properties(spglib PROPERTIES COMPILE_FLAGS "-w")
