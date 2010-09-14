/* primitive.c */
/* Copyright (C) 2008 Atsushi Togo */

#include <stdio.h>
#include <stdlib.h>
#include "cell.h"
#include "debug.h"
#include "mathfunc.h"
#include "primitive.h"
#include "symmetry.h"

static int is_overlap(const double a[3], const double b[3], const double symprec);
static int get_least_axes( double vectors[][3], const int multi, const Cell * cell,
			   const double symprec );
static int trim_cell(Cell * primitive, const Cell * cell, const double symprec);
static int get_primitive( Cell * primitive, const Cell * cell,
			  const double pure_trans[][3], const int multi,
			  const double symprec );

Cell prm_get_primitive(const Cell * cell, const double symprec)
{
  int i, multi;
  double **pure_trans;
  Cell primitive;

  pure_trans = (double**)malloc(cell->size * sizeof(double*));
  for (i = 0; i < cell->size; i++) {
    pure_trans[i] = (double*)malloc(3 * sizeof(double));
  }

  debug_print("*** prm_get_primitive ***\n");

  multi = sym_get_pure_translation( pure_trans, cell, symprec );

  if ( multi > 1 ) {
    /* Create primitive lattice */
    primitive = cel_new_cell(cell->size / multi);
    if ( get_primitive( &primitive, cell, pure_trans, multi, symprec ) ) {
      for (i = 0; i < cell->size; i++) {
        free(pure_trans[i]);
      }
      free(pure_trans);
      return primitive;
    } else {
      /* Sometimes primitive cell can not be found. */
      cel_delete_cell( &primitive );
    }
  }

  /* If primitive cell was not found, then primitive.size = 0 is returned */
  debug_print("Primitive cell could not be found.\n");
  primitive = cel_new_cell( 0 );

  for (i = 0; i < cell->size; i++) {
    free(pure_trans[i]);
  }
  free(pure_trans);

  return primitive;
}

static int get_primitive( Cell * primitive, const Cell * cell,
			  const double pure_trans[][3], const int multi,
			  const double symprec )
{
  int i, j;
  double prim_lattice[3][3], relative_lattice[3][3], **vectors;

  vectors = (double**)malloc( (multi+2) * sizeof(double*));
  for (i = 0; i < (multi+2); i++) {
    vectors[i] = (double*)malloc(3 * sizeof(double));
  }

  /* store pure translations in original cell */ 
  /* as trial primitive lattice vectors */
  for (i = 0; i < multi - 1; i++) {
    mat_copy_vector_d3( vectors[i], pure_trans[i + 1]);
  }

  /* store lattice translations of original cell */
  /* as trial primitive lattice vectors */
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      if (i == j) {
	vectors[i+multi-1][j] = 1;
      } else {
	vectors[i+multi-1][j] = 0;
      }
    }
  }

#ifdef DEBUG
  for (i = 0; i < multi + 2; i++) {
    debug_print("%d: %f %f %f\n", i + 1, vectors[i][0],
		vectors[i][1], vectors[i][2]);
  }
#endif

  /* Lattice of primitive cell is found among pure translation vectors */
  /* vectors[0], vectors[1], and vectors[2] are overwritten. */
  if ( ! get_least_axes( vectors, multi, cell, symprec ) ) {
    for (i = 0; i < (multi+2); i++) {
      free(vectors[i]);
    }
    free(vectors);
    return 0;
  }

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      relative_lattice[j][i] = vectors[i][j];
    }

    debug_print("found axis %d: %f %f %f\n", i + 1, vectors[i][0],
		vectors[i][1], vectors[i][2]);
  }

  /* A primitive lattice is obtained. */
  mat_multiply_matrix_d3( prim_lattice, cell->lattice, relative_lattice );

  /* Smallest lattice vectors are chosen. */
  brv_smallest_lattice_vector( primitive->lattice, prim_lattice, symprec );

  /* Fit atoms into new primitive cell */
  if (!trim_cell( primitive, cell, symprec)) {
    for (i = 0; i < (multi+2); i++) {
      free(vectors[i]);
    }
    free(vectors);
    return 0;
  }

  debug_print("Original cell lattice.\n");
  debug_print_matrix_d3(cell->lattice);
  debug_print("Found primitive lattice after choosing least axes.\n");
  debug_print_matrix_d3(prim_lattice);
  debug_print("Found primitive lattice after choosing smallest axes.\n");
  debug_print_matrix_d3(primitive->lattice);
  debug_print("Number of atoms in primitive cell: %d\n", primitive->size);
  debug_print("Volume: original %f --> primitive %f\n",
	      mat_get_determinant_d3(cell->lattice),
	      mat_get_determinant_d3(primitive->lattice));

  for (i = 0; i < (multi+2); i++) {
    free(vectors[i]);
  }
  free(vectors);
  return 1;
}


static int trim_cell(Cell * primitive, const Cell * cell, const double symprec)
{
  int i, j, k, count, ratio, attempt, finished, count_error=0, old_count_error=0;
  int **table, *check_table;
  double axis_inv[3][3], tmp_matrix[3][3], **position;
  double trim_tolerance, tol_adjust;

  table = (int**)malloc(cell->size * sizeof(int*));
  check_table = (int*)malloc(cell->size * sizeof(int));
  position = (double**)malloc(cell->size * sizeof(double*));
  for (i = 0; i < cell->size; i++) {
    table[i] = (int*)malloc(cell->size * sizeof(int));
    position[i] = (double*)malloc(3 * sizeof(double));
  }

  ratio = cell->size / primitive->size;
  trim_tolerance = ratio * symprec;
  tol_adjust = trim_tolerance/2.0;

  mat_inverse_matrix_d3(tmp_matrix, primitive->lattice, symprec);
  mat_multiply_matrix_d3(axis_inv, tmp_matrix, cell->lattice);

  /* Send atoms into the primitive cell */
  debug_print("Positions in new axes reduced to primitive cell\n");
  for (i = 0; i < cell->size; i++) {
    mat_multiply_matrix_vector_d3( position[i], axis_inv, cell->position[i] );
    for (j = 0; j < 3; j++)
      position[i][j] = position[i][j] - mat_Nint( position[i][j] );

    debug_print("%d: %f %f %f\n", i + 1, position[i][0], position[i][1], position[i][2]);
  }

  /* Create overlapping table */
  finished = 0; /* Boolean for do-while loop */
  attempt = 0; /* Count how many times trimming has been attempted. Stop trying after 1000. */
  do {
    attempt++;
    finished = 1;
    debug_print("Trim attempt %d: tolerance=%f\n",attempt,trim_tolerance);
    for (i = 0; i < cell->size; i++)
      for (j = 0; j < cell->size; j++)
        table[i][j] = -1;

    for (i = 0; i < cell->size; i++) {
      count = 0;
      for (j = 0; j < cell->size; j++) {
        if ( is_overlap( position[i], position[j], trim_tolerance ) ) {
          table[i][count] = j;
          count++;
        }
      }
      if (count != ratio) {
        count_error = count - ratio;
        /* Initialize count error if needed: */
        if (old_count_error == 0) {
          old_count_error = count - ratio;
        }
        /* Adjust the tolerance adjustment if needed */
        if ( (old_count_error > 0 && count_error < 0) ||
             (old_count_error < 0 && count_error > 0) ||
             trim_tolerance - tol_adjust <= 0 ){
          tol_adjust /= 2.0;
        }
        old_count_error = count_error;
        debug_print("Bad tolerance: count=%d ratio=%d tol_adjust=%f\n",count,ratio,tol_adjust);
        if (count_error > 0) { /* tolerance is too large, reduce */
          trim_tolerance -= tol_adjust;
          finished = 0; /* Set as not finished and restart. */
          i = cell->size;
        } else { /* tolerance is too small, increase */
          trim_tolerance += tol_adjust;
          finished = 0;
          i = cell->size;
        }          
      }
    }
  } while (!finished && attempt < 1000);
  if (attempt >= 1000) {
    fprintf(stderr, "Bug: Could not trim cell into primitive.\n");
    for (i = 0; i < cell->size; i++) {
      free(table[i]);
      free(position[i]);
    }
    free(table);
    free(check_table);
    free(position);
	return 0;
  }

#ifdef DEBUG
  debug_print("Overlaping table\n");
  for (i = 0; i < cell->size; i++) {
    debug_print("%2d: ", count);
    for (j = 0; j < count; j++)
      debug_print("%2d ", table[i][j]);
    debug_print("\n");
  }
#endif



  /* Copy positions. Positions of overlapped atoms are averaged. */
  for (i = 0; i < cell->size; i++)
    check_table[i] = 0;
  count = 0;

  for (i = 0; i < cell->size; i++)

    if (!check_table[i]) {
      debug_print("Trimming... i=%d count=%d\n", i, count);
      primitive->types[count] = cell->types[i];

      for (j = 0; j < 3; j++)
	primitive->position[count][j] = 0;

      for (j = 0; j < ratio; j++) {	/* overlap atoms */
        if (table[i][j] < 0)
          break;

	for (k = 0; k < 3; k++)

	  /* boundary treatment */
	  if (mat_Dabs(position[table[i][0]][k] -
		       position[table[i][j]][k]) > 0.5)

	    if (position[table[i][j]][k] < 0)
	      primitive->position[count][k]
		= primitive->position[count][k] +
		position[table[i][j]][k] + 1;

	    else
	      primitive->position[count][k]
		= primitive->position[count][k] +
		position[table[i][j]][k] - 1;

	  else
	    primitive->position[count][k]
	      = primitive->position[count][k] +
	      position[table[i][j]][k];
	check_table[table[i][j]] = 1;
      }

      for (j = 0; j < 3; j++) {	/* take average and reduce */

	primitive->position[count][j] =
	  primitive->position[count][j] / ratio;

	primitive->position[count][j] =
	  primitive->position[count][j] -
	  mat_Nint(primitive->position[count][j] - symprec);
      }
      count++;
    }

  debug_print("Count: %d Size of cell: %d Size of primitive: %d\n", count, cell->size, primitive->size);
  if (count != primitive->size) {
    fprintf(stderr, "Bug: Primitive cell could not be found.\n");
  }

  debug_print("Trimed position\n");
  debug_print_vectors_with_label(primitive->position, primitive->types,
				 primitive->size);

  for (i = 0; i < cell->size; i++) {
    free(table[i]);
    free(position[i]);
  }
  free(table);
  free(check_table);
  free(position);
  return 1;
}

static int is_overlap(const double a[3], const double b[3], const double symprec)
{
  if ( ( mat_Dabs(a[0] - b[0]) < symprec ||
	 mat_Dabs( mat_Dabs(a[0] - b[0]) - 1) < symprec ) &&
       ( mat_Dabs(a[1] - b[1]) < symprec ||
	 mat_Dabs(mat_Dabs(a[1] - b[1]) - 1) < symprec ) &&
       ( mat_Dabs(a[2] - b[2]) < symprec ||
	 mat_Dabs(mat_Dabs(a[2] - b[2]) - 1) < symprec ) ) {
    return 1;
  }
  return 0;
}

static int get_least_axes( double vectors[][3], const int multi,
			   const Cell * cell, const double symprec )
{
  int i, j, k;
  double initial_volume, volume, min_vectors[3][3], tmp_lattice[3][3];

  debug_print("*** get_least_axes ***\n");

  initial_volume = mat_Dabs(mat_get_determinant_d3(cell->lattice));
  debug_print("initial volume: %f\n", initial_volume);

  /* check volumes of all possible lattices, find smallest volume */
  for (i = 0; i < multi + 2; i++) {
    for (j = i + 1; j < multi + 2; j++) {
      for (k = j + 1; k < multi + 2; k++) {
	mat_multiply_matrix_vector_d3( tmp_lattice[0],
				       cell->lattice,
				       vectors[i] );
	mat_multiply_matrix_vector_d3( tmp_lattice[1],
				       cell->lattice,
				       vectors[j] );
	mat_multiply_matrix_vector_d3( tmp_lattice[2],
				       cell->lattice,
				       vectors[k] );
	volume = mat_Dabs( mat_get_determinant_d3( tmp_lattice ) );
	if ( mat_Dabs( volume ) > symprec ) {
	  debug_print("temporary volume of primitive cell: %f\n", volume );
	  debug_print("volume of original cell: %f\n", initial_volume );
	  debug_print("multi and calculated multi: %d, %d\n", multi, mat_Nint( initial_volume / volume ) );
	  if ( mat_Nint( initial_volume / volume ) == multi ) {
	    mat_copy_vector_d3(min_vectors[0], vectors[i]);
	    mat_copy_vector_d3(min_vectors[1], vectors[j]);
	    mat_copy_vector_d3(min_vectors[2], vectors[k]);
	    mat_copy_vector_d3(vectors[0], min_vectors[0]);
	    mat_copy_vector_d3(vectors[1], min_vectors[1]);
	    mat_copy_vector_d3(vectors[2], min_vectors[2]);
	    return 1;
	  }
	}
      }
    }
  }

  /* Not found */
  return 0;
}

