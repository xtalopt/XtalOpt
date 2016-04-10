set( spglib_SRCS
cell.c
cell.h
debug.c
debug.h
hall_symbol.c
hall_symbol.h
kgrid.c
kgrid.h
kpoint.c
kpoint.h
lattice.c
lattice.h
mathfunc.c
mathfunc.h
niggli.c
niggli.h
pointgroup.c
pointgroup.h
primitive.c
primitive.h
refinement.c
refinement.h
sitesym_database.c
sitesym_database.h
site_symmetry.c
site_symmetry.h
spacegroup.c
spacegroup.h
spg_database.c
spg_database.h
spglib.c
spglib_f.c
spglib.h
spin.c
spin.h
symmetry.c
symmetry.h
tetrahedron_method.c
tetrahedron_method.h
triplet_kpoint.c
triplet_kpoint.h
)

add_library(spglib STATIC ${spglib_SRCS})
# Suppress spglib warnings
set_target_properties( spglib PROPERTIES
		       COMPILE_FLAGS "-w")
