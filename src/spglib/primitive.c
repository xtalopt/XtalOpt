/* primitive.c */
/* Copyright (C) 2008 Atsushi Togo */

#include <stdio.h>
#include <stdlib.h>
#include "cell.h"
#include "lattice.h"
#include "mathfunc.h"
#include "primitive.h"
#include "symmetry.h"

#include "debug.h"

#define REDUCE_RATE 0.95

static int trim_cell( Cell * primitive,
		      int * mapping_table,
		      SPGCONST Cell * cell,
		      const double symprec );
static int set_primitive_positions( Cell * primitive,
				    const VecDBL * position,
				    const Cell * cell,
				    int * const * table );
static VecDBL * get_positions_primitive( SPGCONST Cell * cell,
					 SPGCONST double prim_lat[3][3] );
static int get_overlap_table( int ** table,
			      const int cell_size,
			      SPGCONST Cell *primitive,
			      const VecDBL * position,
			      const double symprec );
static void free_overlap_table( int ** table, const int size );
static int ** allocate_overlap_table( const int size );
static Cell * get_cell_with_smallest_lattice( SPGCONST Cell * cell,
					      const double symprec );
static Cell * get_primitive( int * mapping_table,
			     SPGCONST Cell * cell,
			     const VecDBL * pure_trans,
			     const double symprec );
static int get_primitive_lattice_vectors_iterative( double prim_lattice[3][3],
						    SPGCONST Cell * cell,
						    const VecDBL * pure_trans,
						    const double symprec );
static int get_primitive_lattice_vectors( double prim_lattice[3][3],
					  const VecDBL * vectors,
					  SPGCONST Cell * cell,
					  const double symprec );
static VecDBL * get_translation_candidates( const VecDBL * pure_trans );

Cell * prm_get_primitive( int * mapping_table,
			  SPGCONST Cell * cell,
			  const VecDBL *pure_trans,
			  const double symprec )
{
  int i;
  Cell *primitive;

  /* If primitive could not be found, primitive->size = -1 is returned. */
  /* If cell is already primitive cell, */
  /* primitive cell with smallest lattice is returned. */

  if ( pure_trans->size > 1 ) {
    primitive = get_primitive( mapping_table, cell, pure_trans, symprec );
  } else {
    primitive =  get_cell_with_smallest_lattice( cell, symprec );
    for ( i = 0; i < cell->size; i++ ) {
      mapping_table[i] = i;
    }
  }

  return primitive;
}

static Cell * get_cell_with_smallest_lattice( SPGCONST Cell * cell,
					      const double symprec )
{
  int i, j;
  double min_lat[3][3], trans_mat[3][3], inv_lat[3][3];
  Cell * smallest_cell;

  if ( lat_smallest_lattice_vector( min_lat,
				    cell->lattice,
				    symprec ) ) {
    mat_inverse_matrix_d3( inv_lat, min_lat, 0 );
    mat_multiply_matrix_d3( trans_mat, inv_lat, cell->lattice );
    smallest_cell = cel_alloc_cell( cell->size );
    mat_copy_matrix_d3( smallest_cell->lattice, min_lat );
    for ( i = 0; i < cell->size; i++ ) {
      smallest_cell->types[i] = cell->types[i];
      mat_multiply_matrix_vector_d3( smallest_cell->position[i],
				     trans_mat, cell->position[i] );
      for ( j = 0; j < 3; j++ ) {
	cell->position[i][j] -= mat_Nint( cell->position[i][j] );
      }
    }
  } else {
    smallest_cell = cel_alloc_cell( -1 );
  }

  return smallest_cell;
}

/* If primitive could not be found, primitive->size = -1 is returned. */
static Cell * get_primitive( int * mapping_table,
			     SPGCONST Cell * cell,
			     const VecDBL * pure_trans,
			     const double symprec )
{
  int multi;
  double prim_lattice[3][3];
  Cell * primitive;

  /* Primitive lattice vectors are searched. */
  /* To be consistent, sometimes tolerance is decreased iteratively. */
  /* The descreased tolerance is stored in 'static double tolerance'. */
  multi = get_primitive_lattice_vectors_iterative( prim_lattice,
						   cell,
						   pure_trans,
						   symprec );
  if ( ! multi  ) {
    goto not_found;
  }

  primitive = cel_alloc_cell( cell->size / multi );

  if ( ! lat_smallest_lattice_vector( primitive->lattice,
				      prim_lattice,
				      symprec ) ) {
    cel_free_cell( primitive );
    goto not_found;
  }

  /* Fit atoms into new primitive cell */
  if ( ! trim_cell( primitive, mapping_table, cell, symprec ) ) {
    cel_free_cell( primitive );
    goto not_found;
  }

  debug_print("Original cell lattice.\n");
  debug_print_matrix_d3(cell->lattice);
  debug_print("Found primitive lattice after choosing least axes.\n");
  debug_print_matrix_d3(primitive->lattice);
  debug_print("Number of atoms in primitive cell: %d\n", primitive->size);
  debug_print("Volume: original %f --> primitive %f\n",
	      mat_get_determinant_d3(cell->lattice),
	      mat_get_determinant_d3(primitive->lattice));

  /* found */
  return primitive;

 not_found:
  primitive = cel_alloc_cell( -1 );
  warning_print("spglib: Primitive cell could not found ");
  warning_print("(line %d, %s).\n", __LINE__, __FILE__);
  return primitive;
}


static int trim_cell( Cell * primitive,
		      int * mapping_table,
		      SPGCONST Cell * cell,
		      const double symprec )
{
  int ratio, i, count;
  VecDBL * position;
  int **overlap_table;

  overlap_table = allocate_overlap_table( cell->size );
  ratio = cell->size / primitive->size;

  /* Get reduced positions of atoms in original cell with respect to */
  /* primitive lattice */
  position = get_positions_primitive( cell, primitive->lattice );

  /* Create overlapping table */
  if ( ! get_overlap_table( overlap_table,
			    cell->size,
			    primitive,
			    position,
			    symprec ) ) { goto err; }

  /* cell to primitive cell mapping table */
  count = 0;
  for ( i = 0; i < cell->size; i++ ) {
    if ( overlap_table[i][0] == i ) {
      mapping_table[i] = count;
      count++;
    } else {
      mapping_table[i] = mapping_table[ overlap_table[i][0] ];
    }
  }

  /* Copy positions. Positions of overlapped atoms are averaged. */
  if ( ! set_primitive_positions( primitive,
				  position,
				  cell,
				  overlap_table ) )
    { goto err; }

  debug_print("Trimed position\n");
  debug_print_vectors_with_label( primitive->position,
				  primitive->types,
				  primitive->size );
  
  mat_free_VecDBL( position );
  free_overlap_table( overlap_table, cell->size );
  return 1;

 err:
  mat_free_VecDBL( position );
  free_overlap_table( overlap_table, cell->size );
  return 0;
}

static int set_primitive_positions( Cell * primitive,
				    const VecDBL * position,
				    const Cell * cell,
				    int * const * table )
{
  int i, j, k, ratio, count;
  int *is_equivalent = (int*)malloc(cell->size * sizeof(int));

  ratio = cell->size / primitive->size;
  for (i = 0; i < cell->size; i++) {
    is_equivalent[i] = 0;
  }

  /* Copy positions. Positions of overlapped atoms are averaged. */
  count = 0;
  for ( i = 0; i < cell->size; i++ )

    if ( ! is_equivalent[i] ) {

      debug_print("Trimming... i=%d count=%d\n", i, count);
      primitive->types[count] = cell->types[i];

      for ( j = 0; j < 3; j++ ) {
	primitive->position[count][j] = 0;
      }

      for ( j = 0; j < ratio; j++ ) {	/* overlap atoms */
        if ( table[i][j] < 0 ) { /* check if table is correctly bulit. */
          break;
	}

	is_equivalent[table[i][j]] = 1;

	for (k = 0; k < 3; k++) {

	  /* boundary treatment */
	  if (mat_Dabs(position->vec[table[i][0]][k] -
		       position->vec[table[i][j]][k]) > 0.5) {

	    if (position->vec[table[i][j]][k] < 0) {
	      primitive->position[count][k]
		= primitive->position[count][k] +
		position->vec[table[i][j]][k] + 1;
	    } else {
	      primitive->position[count][k]
		= primitive->position[count][k] +
		position->vec[table[i][j]][k] - 1;
	    }

	  } else {
	    primitive->position[count][k]
	      = primitive->position[count][k] +
	      position->vec[table[i][j]][k];
	  }
	}
	
      }

      for (j = 0; j < 3; j++) {	/* take average and reduce */

	primitive->position[count][j] =
	  primitive->position[count][j] / ratio;

	primitive->position[count][j] =
	  primitive->position[count][j] -
	  mat_Nint( primitive->position[count][j] );
      }
      count++;
    }

  free(is_equivalent);
  is_equivalent = NULL;

  debug_print("Count: %d Size of cell: %d Size of primitive: %d\n", count, cell->size, primitive->size);
  if ( ! ( count == primitive->size ) ) {
    warning_print("spglib: Atomic positions of primitive cell could not be determined ");
    warning_print("(line %d, %s).\n", __LINE__, __FILE__);
    goto err;
  }

  return 1;

 err:
  return 0;
}

static VecDBL * get_positions_primitive( SPGCONST Cell * cell,
					 SPGCONST double prim_lat[3][3] )
{
  int i, j;
  double tmp_matrix[3][3], axis_inv[3][3];
  VecDBL * position;

  position = mat_alloc_VecDBL( cell->size );

  mat_inverse_matrix_d3(tmp_matrix, prim_lat, 0);
  mat_multiply_matrix_d3(axis_inv, tmp_matrix, cell->lattice);

  /* Send atoms into the primitive cell */
  debug_print("Positions in new axes reduced to primitive cell\n");
  for (i = 0; i < cell->size; i++) {
    mat_multiply_matrix_vector_d3( position->vec[i],
				   axis_inv, cell->position[i] );
    for (j = 0; j < 3; j++) {
      position->vec[i][j] -= mat_Nint( position->vec[i][j] );
    }
    debug_print("%d: %f %f %f\n", i + 1,
		position->vec[i][0], 
		position->vec[i][1],
		position->vec[i][2]);
  }

  return position;
}

static int get_overlap_table( int **overlap_table,
			      const int cell_size,
			      SPGCONST Cell *primitive,
			      const VecDBL * position,
			      const double symprec )
{
  int i, j, is_found, attempt, count, ratio;
  int count_error=0, old_count_error=0;
  double trim_tolerance, tol_adjust;

  ratio = cell_size / primitive->size;
  
  trim_tolerance = symprec;
  tol_adjust = trim_tolerance/2.0;

  is_found = 0;

  /* Break when is_found */
  for ( attempt = 0; attempt < 1000; attempt++ ) {
    is_found = 1;
    debug_print("Trim attempt %d: tolerance=%f\n",attempt+1,trim_tolerance);

    /* Each value of -1 has to be overwritten by 0 or positive numbers. */
    for (i = 0; i < cell_size; i++) {
      for (j = 0; j < cell_size; j++) {
        overlap_table[i][j] = -1;
      }
    }

    for (i = 0; i < cell_size; i++) {

      count = 0;
      for (j = 0; j < cell_size; j++) {
        if ( cel_is_overlap( position->vec[i], position->vec[j],
			     primitive->lattice, trim_tolerance ) ) {
          overlap_table[i][count] = j;
          count++;
        }
      }

      /* Adjust tolerance to avoid too much and too few overlaps */
      if (count != ratio) { /* check overlapping number */
        count_error = count - ratio;

        /* Initialize count error if needed: */
        if (old_count_error == 0) {
          old_count_error = count - ratio;
        }

        /* Adjust the tolerance adjustment if needed */
        if ( ( old_count_error > 0 && count_error < 0 ) ||
             ( old_count_error < 0 && count_error > 0 ) ||
             trim_tolerance - tol_adjust <= 0 ) {
          tol_adjust /= 2.0;
        }
        old_count_error = count_error;

        debug_print("Bad tolerance: count=%d ratio=%d tol_adjust=%f\n",
		    count,ratio,tol_adjust);

        if ( count_error > 0 ) { trim_tolerance -= tol_adjust; }
        else { trim_tolerance += tol_adjust; }          

	is_found = 0;
	break;
      }
    }
    if ( is_found ) { break; }
  }
  
  if ( ! is_found ) {
    warning_print("spglib: Could not trim cell into primitive ");
    warning_print("(line %d, %s).\n", __LINE__, __FILE__);
    goto err;
  }

  return 1;

 err:
  return 0;
}

static void free_overlap_table( int **table, const int size )
{
  int i;
  for ( i = 0; i < size; i++ ) {
    free( table[i] );
    table[i] = NULL;
  }
  free( table );
  table = NULL;
}

static int ** allocate_overlap_table( const int size )
{
  int i;
  int **table = (int**)malloc(size * sizeof(int*));
  for (i = 0; i < size; i++) {
    table[i] = (int*)malloc(size * sizeof(int));
  }
  return table;
}


static int get_primitive_lattice_vectors_iterative( double prim_lattice[3][3],
						    SPGCONST Cell * cell,
						    const VecDBL * pure_trans,
						    const double symprec )
{
  int i, multi, attempt;
  double tolerance;
  VecDBL * vectors, * pure_trans_reduced, *tmp_vec;

  tolerance = symprec;
  pure_trans_reduced = mat_alloc_VecDBL( pure_trans->size );
  for ( i = 0; i < pure_trans->size; i++ ) {
    mat_copy_vector_d3( pure_trans_reduced->vec[i], pure_trans->vec[i] );
  }
  
  for ( attempt = 0; attempt < 100; attempt++ ) {
    multi = pure_trans_reduced->size;
    vectors = get_translation_candidates( pure_trans_reduced );

    /* Lattice of primitive cell is found among pure translation vectors */
    if ( get_primitive_lattice_vectors( prim_lattice,
					vectors,
					cell,
					tolerance ) ) {

      mat_free_VecDBL( vectors );
      mat_free_VecDBL( pure_trans_reduced );

      goto found;
    } else {

      tmp_vec = mat_alloc_VecDBL( multi );
      for ( i = 0; i < multi; i++ ) {
	mat_copy_vector_d3( tmp_vec->vec[i], pure_trans_reduced->vec[i] );
      }
      mat_free_VecDBL( pure_trans_reduced );
      pure_trans_reduced = sym_reduce_pure_translation( cell,
							tmp_vec,
							tolerance );
      debug_print("Tolerance is reduced to %f (%d), size = %d\n",
		  tolerance, attempt, pure_trans_reduced->size);

      mat_free_VecDBL( tmp_vec );
      mat_free_VecDBL( vectors );

      tolerance *= REDUCE_RATE;
    }
  }

  /* Not found */
  return 0;

 found:
#ifdef SPGWARNING
  if ( attempt > 0 ) {
    printf("spglib: Tolerance to find primitive lattice vectors was changed to %f ", tolerance);
  }
#endif
  return multi;
}

static int get_primitive_lattice_vectors( double prim_lattice[3][3],
					  const VecDBL * vectors,
					  SPGCONST Cell * cell,
					  const double symprec )
{
  int i, j, k, size;
  double initial_volume, volume;
  double relative_lattice[3][3], min_vectors[3][3], tmp_lattice[3][3];
  double inv_mat_dbl[3][3];
  int inv_mat_int[3][3];

  debug_print("*** get_primitive_lattice_vectors ***\n");

  size = vectors->size;
  initial_volume = mat_Dabs(mat_get_determinant_d3(cell->lattice));
  debug_print("initial volume: %f\n", initial_volume);

  /* check volumes of all possible lattices, find smallest volume */
  for (i = 0; i < size; i++) {
    for (j = i + 1; j < size; j++) {
      for (k = j + 1; k < size; k++) {
	mat_multiply_matrix_vector_d3( tmp_lattice[0],
				       cell->lattice,
				       vectors->vec[i] );
	mat_multiply_matrix_vector_d3( tmp_lattice[1],
				       cell->lattice,
				       vectors->vec[j] );
	mat_multiply_matrix_vector_d3( tmp_lattice[2],
				       cell->lattice,
				       vectors->vec[k] );
	volume = mat_Dabs( mat_get_determinant_d3( tmp_lattice ) );
	if ( volume > symprec ) {
	  debug_print("temporary volume of primitive cell: %f\n", volume );
	  debug_print("volume of original cell: %f\n", initial_volume );
	  debug_print("multi and calculated multi: %d, %d\n", size-2, mat_Nint( initial_volume / volume ) );
	  if ( mat_Nint( initial_volume / volume ) == size-2 ) {
	    mat_copy_vector_d3(min_vectors[0], vectors->vec[i]);
	    mat_copy_vector_d3(min_vectors[1], vectors->vec[j]);
	    mat_copy_vector_d3(min_vectors[2], vectors->vec[k]);
	    goto ret;
	  }
	}
      }
    }
  }

  /* Not found */
  warning_print("spglib: Primitive lattice vectors cound not be found ");
  warning_print("(line %d, %s).\n", __LINE__, __FILE__);
  return 0;

  /* Found */
 ret:
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      relative_lattice[j][i] = min_vectors[i][j];
    }
  }

  mat_inverse_matrix_d3( inv_mat_dbl, relative_lattice, 0 );
  mat_cast_matrix_3d_to_3i( inv_mat_int, inv_mat_dbl );
  if ( abs( mat_get_determinant_i3( inv_mat_int ) ) == size-2 ) {
    mat_cast_matrix_3i_to_3d( inv_mat_dbl, inv_mat_int );
    mat_inverse_matrix_d3( relative_lattice, inv_mat_dbl, 0 );
  } else {
    warning_print("spglib: Primitive lattice cleaning is incomplete ");
    warning_print("(line %d, %s).\n", __LINE__, __FILE__);
  }
  mat_multiply_matrix_d3( prim_lattice, cell->lattice, relative_lattice );

  debug_print("Oritinal relative_lattice\n");
  debug_print_matrix_d3( relative_lattice );
  debug_print("Cleaned relative_lattice\n");
  debug_print_matrix_d3( relative_lattice );
  debug_print("Primitive lattice\n");
  debug_print_matrix_d3( prim_lattice );

  return 1;  
}

static VecDBL * get_translation_candidates( const VecDBL * pure_trans )
{
  int i, j, multi;
  VecDBL * vectors;

  multi = pure_trans->size;
  vectors = mat_alloc_VecDBL( multi+2 );

  /* store pure translations in original cell */ 
  /* as trial primitive lattice vectors */
  for (i = 0; i < multi - 1; i++) {
    mat_copy_vector_d3( vectors->vec[i], pure_trans->vec[i + 1]);
  }

  /* store lattice translations of original cell */
  /* as trial primitive lattice vectors */
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      if (i == j) {
	vectors->vec[i+multi-1][j] = 1;
      } else {
	vectors->vec[i+multi-1][j] = 0;
      }
    }
  }

#ifdef DEBUG
  for (i = 0; i < multi + 2; i++) {
    debug_print("%d: %f %f %f\n", i + 1, vectors->vec[i][0],
		vectors->vec[i][1], vectors->vec[i][2]);
  }
#endif

  return vectors;
}

