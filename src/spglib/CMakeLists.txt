set( spglib_SRCS 
     bravais.c
     bravais_art.c
     cell.c
     debug.c
     mathfunc.c
     mathfunc.h
     pointgroup.c
     primitive.c
     spacegroup.c
     spacegroup.h
     spacegroup_data.c
     spacegroup_data.h
     spacegroup_database.c
     spacegroup_database.h
     spglib.c
     symmetry.c
     symmetry_kpoint.c)

add_library(spglib STATIC ${spglib_SRCS})
# Suppress spglib warnings
set_target_properties( spglib PROPERTIES
		       COMPILE_FLAGS "-w")
