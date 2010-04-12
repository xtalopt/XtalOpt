/* symmetry.c */
/* Copyright (C) 2008 Atsushi Togo */

/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License */
/* as published by the Free Software Foundation; either version 2 */
/* of the License, or (at your option) any later version. */

/* This program is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with this program; if not, write to the Free Software */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. */

#include <stdio.h>
#include <stdlib.h>
#include "bravais.h"
#include "cell.h"
#include "debug.h"
#include "mathfunc.h"
#include "pointgroup.h"
#include "primitive.h"
#include "symmetry.h"

static int identity[3][3] = {
  {1, 0, 0},              /* order 1 */
  {0, 1, 0},
  {0, 0, 1}
};

static int inversion[3][3] = {
  {-1, 0, 0},             /* order 2 */
  { 0,-1, 0},
  { 0, 0,-1}
};

static int generator6[3][3] = {
  { 1,-1, 0},             /* order 6 */
  { 1, 0, 0},
  { 0, 0, 1}
};

static int generator3[3][3] = {
  { 0, 1, 0},              /* order 3 */
  { 0, 0, 1},
  { 1, 0, 0}
};

static int generator2m[3][3] = {
  {0, 1, 0},              /* order 2 */
  {1, 0, 0},
  {0, 0, 1}
};

static int generator2y[3][3] = {
  {-1, 0, 0},             /* order 2 */
  { 0, 1, 0},
  { 0, 0,-1}
};

static int generator2z[3][3] = {
  {-1, 0, 0},             /* order 2 */
  { 0,-1, 0},
  { 0, 0, 1}
};



static void generate_operation(int point_symmetry[][3][3],
			       int generator[3][3], int n_sym, int n_gen);
static PointSymmetry get_candidate(const Bravais * bravais,
				     const double lattice[3][3],
				     const double symprec);
static PointSymmetry get_conventional(Holohedry holohedry);
static int get_translation(double trans[][3], const int rot[3][3], const Cell *cell,
			   const double symprec);
static int get_operation(int rot[][3][3], double trans[][3], const Bravais *bravais,
			 const Cell * cell, const double symprec);
static int get_operation_supercell(int rot[][3][3], double trans[][3], const int num_sym, 
				   const int multi, const double pure_trans[][3], const Cell *cell,
				   const Cell *primitive, const double symprec);
/* static int get_sorted_positions(double position[][3], const Cell * cell, const double symprec); */
/* static int compare_vectors(const void *vec1, const void *vec2); */

Symmetry sym_new_symmetry(const int size)
{
  Symmetry symmetry;
  symmetry.size = size;
  if ((symmetry.rot =
       (int (*)[3][3]) malloc(sizeof(int[3][3]) * size)) == NULL) {
    fprintf(stderr, "spglib: Memory could not be allocated.");
    exit(1);
  }
  if ((symmetry.trans =
       (double (*)[3]) malloc(sizeof(double[3]) * size)) == NULL) {
    fprintf(stderr, "spglib: Memory could not be allocated.");
    exit(1);
  }
  return symmetry;
}

void sym_delete_symmetry(Symmetry *symmetry)
{
  free(symmetry->rot);
  free(symmetry->trans);
}

Symmetry sym_get_operation(const Bravais *bravais, const Cell *cell, const double symprec)
{
  int i, j, num_sym;
  int rot[cell->size * 48][3][3];
  double trans[cell->size * 48][3];
  Symmetry symmetry;
  
  num_sym = get_operation(rot, trans, bravais, cell, symprec);

  /* New a symmetry object */
  debug_print("*** get_symmetry (found symmetry operations) *** \n");
  debug_print("Lattice \n"); 
  debug_print_matrix_d3(cell->lattice);


  symmetry = sym_new_symmetry(num_sym);

  for (i = 0; i < num_sym; i++) {
    debug_print("--- %d ---\n", i + 1);
    debug_print_matrix_i3(rot[i]);
    debug_print("%f %f %f\n", trans[i][0], trans[i][1], trans[i][2]);
    
    mat_copy_matrix_i3(symmetry.rot[i], rot[i]);

    for (j = 0; j < 3; j++)
      symmetry.trans[i][j] = trans[i][j];
  }

  return symmetry;
}

int sym_get_multiplicity(const Cell *cell, const double symprec)
{
  double trans[cell->size][3];
  return get_translation(trans, identity, cell, symprec);
}

int sym_get_pure_translation(double pure_trans[][3], const Cell *cell,
			     const double symprec)
{
  return get_translation(pure_trans, identity, cell, symprec);
}

/* Look for the translations which satisfy the input symmetry operation. */
/* This function is heaviest in this code. */
static int get_translation(double trans[][3], const int rot[3][3], const Cell *cell,
			   const double symprec)
{
  int i, j, k, l, count, num_trans = 0;
  double diff;
  double test_trans[3], tmp_vector[3], origin[3];
  
  /* atom 0 is the origine to measure the distance between atoms. */
  mat_multiply_matrix_vector_id3(origin, rot, cell->position[0]);

  for (i = 0; i < cell->size; i++) {	/* test translation */

    if (cell->types[i] != cell->types[0])
      continue;

    for (j = 0; j < 3; j++)
      test_trans[j] = cell->position[i][j] - origin[j];

    count = 0;

    for (j = 0; j < cell->size; j++) {	/* test nonsymmorphic operation for an atom */
      mat_multiply_matrix_vector_id3(tmp_vector, rot, cell->position[j]);

      for (k = 0; k < cell->size; k++) {	/* check overlap of atom_k and atom_l */

	if (cell->types[j] != cell->types[k])
	  continue;
	
	for (l = 0; l < 3; l++) {	/* pos_l = S*pos_k + test_translation ?  */

	  diff = cell->position[k][l] - tmp_vector[l] - test_trans[l];
	  diff = mat_Dabs(diff - mat_Nint(diff));

	  if (diff > symprec)
	    goto end_loop_k;
	}

	/* OK: atom_k on atom_j */
	count++;
	break;
      
      end_loop_k:
	;
      }

      /* Is all atoms OK ? */
      if (count < j + 1)
	break;
    }

    if (count == cell->size) {	/* all atoms OK ? */
      for (j = 0; j < 3; j++)
	trans[num_trans][j] = test_trans[j] - mat_Nint(test_trans[j] - symprec);
      num_trans++;
    }
  }

  return num_trans;
}

static int get_operation(int rot[][3][3], double trans[][3],
			 const Bravais *bravais, const Cell *cell,
			 const double symprec)
{
  double trans_tmp[cell->size][3], pure_trans[cell->size][3];
  int i, j, k, num_trans, num_sym = 0, multi;
  PointSymmetry lattice_sym;
  Cell primitive;

  multi = sym_get_pure_translation(pure_trans, cell, symprec);
  if( multi > 1 ) {
    primitive = prm_get_primitive(cell, symprec);
    if ( primitive.size == 0 ) {
      primitive = *cell;
    }
  } else {
    primitive = *cell;
  }

  lattice_sym = get_candidate(bravais, primitive.lattice, symprec);

  for (i = 0; i < lattice_sym.size; i++) {
    /* get translation corresponding to a rotation */
    num_trans = get_translation(trans_tmp, lattice_sym.rot[i], &primitive, symprec);

    for (j = 0; j < num_trans; j++) {
      for (k = 0; k < 3; k++) {
	trans[num_sym + j][k] = trans_tmp[j][k];
      }
      mat_copy_matrix_i3(rot[num_sym + j], lattice_sym.rot[i]);
    }
    num_sym += num_trans;
  }

  if( multi > 1 ) {
    num_sym = get_operation_supercell(rot, trans, num_sym, multi, pure_trans, cell, &primitive, symprec);
    cel_delete_cell(&primitive);
  }

  return num_sym;
}

static int get_operation_supercell(int rot[][3][3], double trans[][3],
				   const int num_sym, 
				   const int multi, const double pure_trans[][3], 
				   const Cell *cell, const Cell *primitive,
				   const double symprec)
{
  int i, j, k;
  int rot_prim[num_sym][3][3];
  double tmp_mat[3][3], coordinate[3][3], coordinate_inv[3][3], trans_prim[num_sym][3];

  debug_print("get_operation_supercell\n");

  /* Obtain ratio of bravais lattice and primitive lattice. P^-1*B */
  if (!(mat_inverse_matrix_d3( tmp_mat, primitive->lattice, symprec )))
    fprintf(stderr, "spglib: BUG in spglib in __LINE__, __FILE__.");

  mat_multiply_matrix_d3( coordinate, tmp_mat, cell->lattice );

  /* Obtain ratio of primitive lattice and bravais lattice. B^-1*P */
  if (!(mat_inverse_matrix_d3( coordinate_inv, coordinate, symprec )))
    fprintf(stderr, "spglib: BUG in spglib in __LINE__, __FILE__.");

  for( i = 0; i < num_sym; i++) {

    /* Translations for primitive cell in fractional coordinate */
    /* have to be recalculated for supercell */
    mat_multiply_matrix_vector_d3( trans[i], coordinate_inv, trans[i] );

    /* Rotations for primitive cell in fractional coordinate */
    /* have to be recalculated for supercell using similarity-like transformation */
    mat_cast_matrix_3i_to_3d(tmp_mat, rot[i]);
    if (!mat_get_similar_matrix_d3(tmp_mat, tmp_mat, coordinate, symprec)) {
      fprintf(stderr, "spglib: BUG in spglib in __LINE__, __FILE__.");
    }
    mat_cast_matrix_3d_to_3i(rot[i], tmp_mat);
    debug_print_matrix_i3(rot[i]);
  }

  /* Rotations and translations are backed up to re-use arrays rot[] and trans[]. */
  for( i = 0; i < num_sym; i++ ) {
    mat_copy_matrix_i3( rot_prim[i], rot[i] );
    for( j = 0; j < 3; j++ )
      trans_prim[i][j] = trans[i][j];
  }

  /* Rotations and translations plus pure translations are set. */
  for( i = 0; i < num_sym; i++ ) {
    for( j = 0; j < multi; j++ ) {
      mat_copy_matrix_i3( rot[ i * multi + j ], rot_prim[i] );
      for ( k = 0; k < 3; k++ )
	trans[i * multi + j][k] = trans_prim[i][k] + pure_trans[j][k];
    }
  }

  /* return number of symmetry operation of supercell */
  return num_sym * multi;
}

static PointSymmetry get_candidate(const Bravais * bravais,
				     const double lattice[3][3],
				     const double symprec)
{
  int i;
  double coordinate[3][3], tmp_matrix_d3[3][3];
  PointSymmetry lattice_sym;

  /* Obtain ratio of lattice and bravais lattice. B^-1*P */
  if (!(mat_inverse_matrix_d3(tmp_matrix_d3, bravais->lattice, symprec)))
    fprintf(stderr, "spglib: BUG in spglib in __LINE__, __FILE__.");

  mat_multiply_matrix_d3(coordinate, tmp_matrix_d3, lattice);

  debug_print("Primitive lattice P\n");
  debug_print_matrix_d3(lattice);
  debug_print("Bravais lattice B\n");
  debug_print_matrix_d3(bravais->lattice);
  debug_print("Holohedry: %d\n", bravais->holohedry);
  debug_print("Ratio of lattice and bravais lattice. B^-1*P.\n");
  debug_print_matrix_d3(coordinate);

  /* lattice_sym of conventional cell is replaced by that of primitive cell. */
  debug_print("*** get_candidate ***\n");

  debug_print("*** candidate in unit cell ***\n");
  lattice_sym = get_conventional(bravais->holohedry);

#ifdef DEBUG
  for (i = 0; i < lattice_sym.size; i++) {
    debug_print("-------- %d -------\n", i+1);
    debug_print_matrix_i3(lattice_sym.rot[i]);
  }
#endif

  debug_print("*** candidate in primitive cell ***\n");

  for (i = 0; i < lattice_sym.size; i++) {
    mat_cast_matrix_3i_to_3d(tmp_matrix_d3, lattice_sym.rot[i]);
    if (!mat_get_similar_matrix_d3(tmp_matrix_d3, tmp_matrix_d3,
                                   coordinate, symprec)) {
      debug_print_matrix_d3(tmp_matrix_d3);
      fprintf(stderr, "spglib BUG: Determinant is zero.\n");
    }

    mat_cast_matrix_3d_to_3i(lattice_sym.rot[i], tmp_matrix_d3);

  }

#ifdef DEBUG
  for (i = 0; i < lattice_sym.size; i++) {
    debug_print("-------- %d -------\n", i+1);
    debug_print_matrix_i3(lattice_sym.rot[i]);
  }
#endif


  return lattice_sym;
}

static PointSymmetry get_conventional(Holohedry holohedry)
{
  int i, j, k;
  PointSymmetry lattice_sym;

  /* all clear */
  for (i = 0; i < 48; i++)
    for (j = 0; j < 3; j++)
      for (k = 0; k < 3; k++)
	lattice_sym.rot[i][j][k] = 0;
  lattice_sym.size = 0;

  /* indentity: this is seed. */
  mat_copy_matrix_i3(lattice_sym.rot[0], identity);

  /* inversion */
  generate_operation(lattice_sym.rot, inversion, 1, 2);

  switch (holohedry) {
  case CUBIC:
    generate_operation(lattice_sym.rot, generator2y, 2, 2);
    generate_operation(lattice_sym.rot, generator2z, 4, 2);
    generate_operation(lattice_sym.rot, generator2m, 8, 2);
    generate_operation(lattice_sym.rot, generator3, 16, 3);
    lattice_sym.size = 48;
    break;
  case HEXA:
  case TRIGO:
    generate_operation(lattice_sym.rot, generator2m, 2, 2);
    generate_operation(lattice_sym.rot, generator6, 4, 6);
    lattice_sym.size = 24;
    break;
  case RHOMB:
    generate_operation(lattice_sym.rot, generator2m, 2, 2);
    generate_operation(lattice_sym.rot, generator3, 4, 3);
    lattice_sym.size = 12;
    break;
  case TETRA:
    generate_operation(lattice_sym.rot, generator2y, 2, 2);
    generate_operation(lattice_sym.rot, generator2z, 4, 2);
    generate_operation(lattice_sym.rot, generator2m, 8, 2);
    lattice_sym.size = 16;
    break;
  case ORTHO:
    generate_operation(lattice_sym.rot, generator2y, 2, 2);
    generate_operation(lattice_sym.rot, generator2z, 4, 2);
    lattice_sym.size = 8;
    break;
  case MONOCLI:
    generate_operation(lattice_sym.rot, generator2y, 2, 2);
    lattice_sym.size = 4;
    break;
  case TRICLI:
    lattice_sym.size = 2;
    break;
  default:
    fprintf(stderr, "spglib BUG: no Bravais lattice found.\n");
  }

  return lattice_sym;
}

static void generate_operation(int point_symmetry[][3][3],
			       int generator[3][3], int n_sym, int n_gen)
{
  int i, j, count;
  int tmp_matrix[3][3];

  /* n_sym is number of symmetry operations, which was already counted. */
  /* n_gen is order, number of operations of the generator class. */
  for (i = 0; i < n_sym; i++) {
    for (j = 0; j < n_gen - 1; j++) {	/* "-1" comes from E (identity) in a class */
      count = i * (n_gen - 1) + j + n_sym;
      mat_multiply_matrix_i3(tmp_matrix, generator,
			     point_symmetry[count - n_sym]);
      mat_copy_matrix_i3(point_symmetry[count], tmp_matrix);
/*       debug_print("--- %d ---\n", count + 1); */
/*       debug_print_matrix_i3(tmp_matrix); */
    }
  }
}

