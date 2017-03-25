set( spglib_SRCS
arithmetic.c
cell.c
debug.c
delaunay.c
hall_symbol.c
kgrid.c
kpoint.c
mathfunc.c
niggli.c
pointgroup.c
primitive.c
refinement.c
sitesym_database.c
site_symmetry.c
spacegroup.c
spg_database.c
spglib.c
spglib_f.c
spin.c
symmetry.c
tetrahedron_method.c
)

add_library(spglib STATIC ${spglib_SRCS})
# Suppress spglib warnings
set_target_properties( spglib PROPERTIES
		       COMPILE_FLAGS "-w")
