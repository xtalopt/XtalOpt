set( spglib_SRCS 
     bravais_art.c
     bravais.c
     cell.c
     debug.c
     hall_symbol.c
     mathfunc.c
     pointgroup.c
     primitive.c
     spacegroup.c
     spacegroup_database.c
     spacegroup_data.c
     spg_database.c
     spglib.c
     spglib_f.c
     symmetry.c
     symmetry_kpoint.c
)

add_library(spglib STATIC ${spglib_SRCS})
# Suppress spglib warnings
set_target_properties( spglib PROPERTIES
		       COMPILE_FLAGS "-w")
