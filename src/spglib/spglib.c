/* spglib.c */
/* Copyright (C) 2008 Atsushi Togo */

#include <stdio.h>
#include <string.h>
#include "spglib.h"
#include "refinement.h"
#include "cell.h"
#include "mathfunc.h"
#include "pointgroup.h"
#include "primitive.h"
#include "symmetry.h"
#include "symmetry_kpoint.h"
#include "spacegroup.h"

int spg_get_symmetry( int rotation[][3][3],
		      double translation[][3],
		      const int max_size,
		      SPGCONST double lattice[3][3],
		      SPGCONST double position[][3],
		      const int types[],
		      const int num_atom,
		      const double symprec ) {
  int i, j, size;
  Symmetry *symmetry;
  Cell *cell;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );
  symmetry = sym_get_operation( cell, symprec );

  if (symmetry->size > max_size) {
    fprintf(stderr, "spglib: Indicated max size(=%d) is less than number ", max_size);
    fprintf(stderr, "spglib: of symmetry operations(=%d).\n", symmetry->size);
    sym_free_symmetry( symmetry );
    return 0;
  }

  for (i = 0; i < symmetry->size; i++) {
    mat_copy_matrix_i3(rotation[i], symmetry->rot[i]);
    for (j = 0; j < 3; j++) {
      translation[i][j] = symmetry->trans[i][j];
    }
  }

  size = symmetry->size;

  cel_free_cell( cell );
  sym_free_symmetry( symmetry );

  return size;
}

int spg_get_multiplicity( SPGCONST double lattice[3][3],
			  SPGCONST double position[][3],
			  const int types[],
			  const int num_atom,
			  const double symprec )
{
  Symmetry *symmetry;
  Cell *cell;
  int size;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );
  symmetry = sym_get_operation( cell, symprec );

  size = symmetry->size;

  cel_free_cell( cell );
  sym_free_symmetry( symmetry );

  return size;
}

int spg_get_smallest_lattice( double smallest_lattice[3][3],
			      SPGCONST double lattice[3][3],
			      const double symprec )
{
  return lat_smallest_lattice_vector(smallest_lattice, lattice, symprec);
}

int spg_get_max_multiplicity( SPGCONST double lattice[3][3],
			      SPGCONST double position[][3],
			      const int types[],
			      const int num_atom,
			      const double symprec )
{
  Cell *cell;
  int num_max_multi;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );
  /* 48 is the magic number, which is the number of rotations */
  /* in the highest point symmetry Oh. */
  num_max_multi = sym_get_multiplicity( cell, symprec ) * 48;
  cel_free_cell( cell );

  return num_max_multi;
}

int spg_find_primitive( double lattice[3][3],
			double position[][3],
			int types[],
			const int num_atom,
			const double symprec )
{
  int i, j, num_prim_atom=0;
  Cell *cell, *primitive;
  VecDBL *pure_trans;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );

  /* find primitive cell */
  if (sym_get_multiplicity( cell, symprec ) > 1) {

    pure_trans = sym_get_pure_translation( cell, symprec );
    primitive = prm_get_primitive( cell, pure_trans, symprec );
    num_prim_atom = primitive->size;
    if ( num_prim_atom < num_atom && num_prim_atom > 0  ) {
      mat_copy_matrix_d3( lattice, primitive->lattice );
      for ( i = 0; i < primitive->size; i++ ) {
	types[i] = primitive->types[i];
	for (j=0; j<3; j++) {
	  position[i][j] = primitive->position[i][j];
	}
      }
    }
    cel_free_cell( primitive );
  } else {
    num_prim_atom = 0;
  }

  cel_free_cell( cell );
    
  return num_prim_atom;
}

int spg_get_international( char symbol[11],
			   SPGCONST double lattice[3][3],
			   SPGCONST double position[][3],
			   const int types[],
			   const int num_atom,
			   const double symprec )
{
  Cell *cell;
  Spacegroup spacegroup;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );
  spacegroup = spa_get_spacegroup( cell, symprec );
  if ( spacegroup.number > 0 ) {
    strcpy(symbol, spacegroup.international_short);
  }

  cel_free_cell( cell );
  
  return spacegroup.number;
}

int spg_get_schoenflies( char symbol[10],
			 SPGCONST double lattice[3][3],
			 SPGCONST double position[][3],
			 const int types[], const int num_atom,
			 const double symprec )
{
  Cell *cell;
  Spacegroup spacegroup;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );

  spacegroup = spa_get_spacegroup( cell, symprec );
  if ( spacegroup.number > 0 ) {
    strcpy(symbol, spacegroup.schoenflies);
  }

  cel_free_cell( cell );

  return spacegroup.number;
}

int spg_refine_cell( double lattice[3][3],
		     double position[][3],
		     int types[],
		     const int num_atom,
		     const double symprec )
{
  int i, num_atom_bravais;
  Cell *cell, *bravais;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );

  bravais = ref_refine_cell( cell, symprec );
  cel_free_cell( cell );

  if ( bravais->size > 0 ) {
    mat_copy_matrix_d3( lattice, bravais->lattice );
    num_atom_bravais = bravais->size;
    for ( i = 0; i < bravais->size; i++ ) {
      types[i] = bravais->types[i];
      mat_copy_vector_d3( position[i], bravais->position[i] );
    }
  } else {
    num_atom_bravais = 0;
  }

  cel_free_cell( bravais );
  
  return num_atom_bravais;
}

int spg_get_ir_kpoints( int map[],
			SPGCONST double kpoints[][3],
			const int num_kpoint,
			SPGCONST double lattice[3][3],
			SPGCONST double position[][3],
			const int types[],
			const int num_atom,
			const int is_time_reversal,
			const double symprec )
{
  Symmetry *symmetry;
  Cell *cell;
  int num_ir_kpoint;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );
  symmetry = sym_get_operation( cell, symprec );

  num_ir_kpoint = kpt_get_irreducible_kpoints( map, kpoints, num_kpoint,
					       lattice, symmetry,
					       is_time_reversal, symprec );


  cel_free_cell( cell );
  sym_free_symmetry( symmetry );

  return num_ir_kpoint;
}

int spg_get_ir_reciprocal_mesh( int grid_point[][3],
				int map[],
				const int mesh[3],
				const int is_shift[3],
				const int is_time_reversal,
				SPGCONST double lattice[3][3],
				SPGCONST double position[][3],
				const int types[],
				const int num_atom,
				const double symprec )
{
  Symmetry *symmetry;
  Cell *cell;
  int num_ir;

  cell = cel_alloc_cell( num_atom );
  cel_set_cell( cell, lattice, position, types );
  symmetry = sym_get_operation( cell, symprec );

  num_ir = kpt_get_irreducible_reciprocal_mesh( grid_point,
						map,
						mesh,
						is_shift,
						is_time_reversal,
						lattice,
						symmetry,
						symprec );


  cel_free_cell( cell );
  sym_free_symmetry( symmetry );

  return num_ir;
}

int spg_get_stabilized_reciprocal_mesh( int grid_point[][3],
				        int map[],
				        const int mesh[3],
				        const int is_shift[3],
				        const int is_time_reversal,
				        SPGCONST double lattice[3][3],
					const int num_rot,
				        SPGCONST int rotations[][3][3],
				        const int num_q,
				        SPGCONST double qpoints[][3],
				        const double symprec )
{
  Symmetry *symmetry;
  int i, num_ir;
  
  symmetry = sym_alloc_symmetry( num_rot );
  for ( i = 0; i < num_rot; i++ ) {
    mat_copy_matrix_i3( symmetry->rot[i], rotations[i] );
  }

  num_ir = kpt_get_stabilized_reciprocal_mesh( grid_point,
					       map,
					       mesh,
					       is_shift,
					       is_time_reversal,
					       lattice,
					       symmetry,
					       num_q,
					       qpoints,
					       symprec );

  sym_free_symmetry( symmetry );

  return num_ir;
}

int spg_get_triplets_reciprocal_mesh( int triplets[][3],
				      int weight_triplets[],
				      int grid_point[][3],
				      const int num_triplets,
				      const int mesh[3],
				      const int is_time_reversal,
				      SPGCONST double lattice[3][3],
				      const int num_rot,
				      SPGCONST int rotations[][3][3],
				      const double symprec )
{
  Symmetry *symmetry;
  int i, num_ir;
  
  symmetry = sym_alloc_symmetry( num_rot );
  for ( i = 0; i < num_rot; i++ ) {
    mat_copy_matrix_i3( symmetry->rot[i], rotations[i] );
  }

  num_ir = kpt_get_triplets_reciprocal_mesh( triplets, 
					     weight_triplets,
					     grid_point,
					     num_triplets,
					     mesh,
					     is_time_reversal,
					     lattice,
					     symmetry,
					     symprec );

  sym_free_symmetry( symmetry );

  return num_ir;
}

int spg_get_triplets_reciprocal_mesh_with_q( int triplets_with_q[][3],
					     int weight_triplets_with_q[],
					     const int fixed_grid_number,
					     const int num_triplets,
					     SPGCONST int triplets[][3],
					     const int weight_triplets[],
					     const int mesh[3],
					     const int is_time_reversal,
					     SPGCONST double lattice[3][3],
					     const int num_rot,
					     SPGCONST int rotations[][3][3],
					     const double symprec )
{
  Symmetry *symmetry;
  int i, num_ir;
  
  symmetry = sym_alloc_symmetry( num_rot );
  for ( i = 0; i < num_rot; i++ ) {
    mat_copy_matrix_i3( symmetry->rot[i], rotations[i] );
  }

  num_ir = kpt_get_triplets_reciprocal_mesh_with_q( triplets_with_q,
						    weight_triplets_with_q,
						    fixed_grid_number,
						    num_triplets,
						    triplets,
						    weight_triplets,
						    mesh,
						    is_time_reversal,
						    lattice,
						    symmetry,
						    symprec );

  
  sym_free_symmetry( symmetry );

  return num_ir;
}


