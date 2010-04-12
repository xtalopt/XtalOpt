/* bravais.c */
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
#include "debug.h"
#include "mathfunc.h"

static int get_brv_cubic(Bravais *bravais, const double min_lattice[3][3],
			 const double symprec);
static int get_brv_tetra(Bravais *bravais, const double min_lattice[3][3], const double symprec);
static int get_brv_hexa(Bravais *bravais, const double min_lattice[3][3], const double symprec);
static int get_brv_rhombo(Bravais *bravais, const double min_lattice[3][3], const double symprec);
static int get_brv_ortho(Bravais *bravais, const double min_lattice[3][3], const double symprec);
static int get_brv_monocli(Bravais *bravais, const double min_lattice[3][3], const double symprec);
static int brv_exhaustive_search(double lattice[3][3], const double min_lattice[3][3],
				 int (*check_bravais)(const double lattice[3][3], const double symprec),
				 const int relative_axes[][3], const int num_axes, const Centering centering,
				 const double symprec);
static int brv_cubic_I_center(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_cubic_F_center(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_tetra_primitive(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_tetra_one(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_tetra_two(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_tetra_three(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static Centering brv_ortho_base_center(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static Centering get_base_center(const double brv_lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_ortho_I_center(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_ortho_F_center(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_rhombo_two(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static int brv_rhombo_three(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static void set_brv_monocli(Bravais *bravais, const double symprec);
static int brv_monocli_primitive(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static Centering brv_monocli_base_center(double lattice[3][3], const double min_lattice[3][3], const double symprec);
static void check_angle90(int angle_90[3], const double lattice[3][3], const double symprec);
static void check_equal_edge(int edge_equal[3], const double lattice[3][3], const double symprec);
static int check_cubic(const double lattice[3][3], const double symprec);
static int check_hexa(const double lattice[3][3], const double symprec);
static int check_tetra(const double lattice[3][3], const double symprec);
static int check_ortho(const double lattice[3][3], const double symprec);
static int check_monocli(const double lattice[3][3], const double symprec);
static int check_rhombo(const double lattice[3][3], const double symprec);
static void get_right_hand_lattice(double lattice[3][3], const double symprec);
static void get_projection(double projection[3][3], const double min_lattice[3][3], const double lattice[3][3]);
static void get_Delaunay_reduction(double red_lattice[3][3], 
				   const double lattice[3][3],
				   const double symprec);
static int get_Delaunay_reduction_basis(double basis[4][3], double symprec);
static void get_exteneded_basis(double basis[4][3], const double lattice[3][3]);
static int compare_vectors(const void *_vec1, const void *_vec2);
static void get_smallest_basis(double basis[4][3], double symprec);

/* math */
static void get_metric(double metric[3][3], const double lattice[3][3]);

/**********************/
/**********************/
/** Public functions **/
/**********************/
/**********************/
Bravais brv_get_brv_lattice(const double lattice_orig[3][3], const double symprec)
{
  Bravais bravais;
  double min_lattice[3][3];
  int i;
  Holohedry holohedries[] = {
    CUBIC,
    HEXA,
    RHOMB,
    TETRA,
    ORTHO,
    MONOCLI,
    TRICLI
  };

  brv_smallest_lattice_vector(min_lattice, lattice_orig, symprec);

#ifdef DEBUG
  double metric[3][3];
  int angle_90[3], edge_equal[3];
  debug_print("*** brv_get_brv_lattice ***\n");
  debug_print("Original lattice\n");
  debug_print_matrix_d3(lattice_orig);
  debug_print("Minimum lattice\n");
  debug_print_matrix_d3(min_lattice);
  debug_print("Metric tensor of minimum lattice\n");
  get_metric(metric, min_lattice);
  debug_print_matrix_d3(metric);
  check_angle90(angle_90, min_lattice, symprec);
  check_equal_edge(edge_equal, min_lattice, symprec);
  printf("angle_90: %d %d %d\n", angle_90[0], angle_90[1], angle_90[2]);
  printf("equal:    %d %d %d\n", edge_equal[0], edge_equal[1], edge_equal[2]);
#endif

  for (i = 0; i < 7; i++) {
    bravais.holohedry = holohedries[i];
    debug_print_holohedry(&bravais);
    if ( brv_get_brv_lattice_in_loop(&bravais, min_lattice, symprec) ) {
      break;
    }
  }

  debug_print("Bravais lattice\n");
  debug_print_holohedry(&bravais);
  debug_print_matrix_d3(bravais.lattice);
  debug_print_matrix_d3(bravais.lattice);

  return bravais;
}

/* Note: bravais is overwritten. */
int brv_get_brv_lattice_in_loop(Bravais *bravais, const double min_lattice[3][3],
                                const double symprec)
{
  switch (bravais->holohedry) {

  case CUBIC:
    if (get_brv_cubic(bravais, min_lattice, symprec))
      goto ok;
    break;
    
  case TETRA:
    if (get_brv_tetra(bravais, min_lattice, symprec))
      goto ok;
    break;
  
  case ORTHO:
    if (get_brv_ortho(bravais, min_lattice, symprec))
      goto ok;
    break;

  case HEXA:
  case TRIGO:
    if (get_brv_hexa(bravais, min_lattice, symprec))
      goto ok;
    break;

  case RHOMB:
    if (get_brv_rhombo(bravais, min_lattice, symprec))
      goto ok;
    break;

  case MONOCLI:
    if (get_brv_monocli(bravais, min_lattice, symprec))
      goto ok;
    break;

  case TRICLI:
  default:
    debug_print("triclinic\n");
    mat_copy_matrix_d3(bravais->lattice, min_lattice);
    bravais->centering = NO_CENTER;
    goto ok;
  }

  return 0;
  
 ok:
  /* Flip if determinant is minus. */
  get_right_hand_lattice(bravais->lattice, symprec);
  return 1;
}

void brv_smallest_lattice_vector(double min_lattice[3][3], const double lattice[3][3],
				 const double symprec)
{
  int i, j;
  double tmp_matrix[3][3], projection[3][3];

  get_Delaunay_reduction(min_lattice, lattice, symprec);
  
  debug_print("New lattice before forcing right handed orientation\n");
  debug_print_matrix_d3(min_lattice);

  /* Choose first vector as most overwrapping with the original first vector. */
  get_projection(projection, min_lattice, lattice);

  debug_print("Projection\n");
  debug_print("%f %f %f\n", projection[0][0], projection[1][0], projection[2][0]);
  debug_print("%f %f %f\n", projection[0][1], projection[1][1], projection[2][1]);
  debug_print("%f %f %f\n", projection[0][2], projection[1][2], projection[2][2]);

  /* Choose first axis (well projected one) */
  i = 0;
    
  if (mat_Dabs(projection[1][0]) - mat_Dabs(projection[0][0]) > symprec)
    i = 1;

  if (mat_Dabs(projection[2][0]) - mat_Dabs(projection[i][0]) > symprec)
    i = 2;


  /* Swap axes */
  mat_copy_matrix_d3(tmp_matrix, min_lattice);

  for (j = 0; j < 3; j++) {
    min_lattice[j][0] = tmp_matrix[j][i];
    min_lattice[j][i] = tmp_matrix[j][0];
  }

  /* Flip first axis */
  if (projection[i][0] < -symprec)
    for (j = 0; j < 3; j++)
      min_lattice[j][0] = -min_lattice[j][0];


  /* Choose second axis (better projected one) */
  i = 1;

  if (mat_Dabs(projection[2][0]) - mat_Dabs(projection[1][0]) > symprec) {
    i = 2;

    /* Swap axes */
    mat_copy_matrix_d3(tmp_matrix, min_lattice);

    for (j = 0; j < 3; j++) {
      min_lattice[j][1] = tmp_matrix[j][i];
      min_lattice[j][i] = tmp_matrix[j][1];
    }
  }

  /* Flip second axis */
  if (projection[i][0] < -symprec)
    for (j = 0; j < 3; j++)
      min_lattice[j][1] = -min_lattice[j][1];
    
  /*   Right-handed orientation */
  if (mat_get_determinant_d3(min_lattice) < -symprec*symprec*symprec) {

    /* Flip third axis */
    for (j = 0; j < 3; j++)
      min_lattice[j][2] = -min_lattice[j][2];
  }
}

static void get_projection(double projection[3][3], const double min_lattice[3][3],
			   double const lattice[3][3])
{
  double tmp_matrix[3][3];
  
  mat_transpose_matrix_d3(tmp_matrix, min_lattice);
  mat_multiply_matrix_d3(projection, tmp_matrix, lattice);
}

/***********/
/*  Cubic  */
/***********/
static int get_brv_cubic(Bravais *bravais, const double min_lattice[3][3],
			 const double symprec)
{
  int i, j, count = 0;
  int edge_equal[3];
  double sum = 0;

  check_equal_edge(edge_equal, min_lattice, symprec);
  if (!(edge_equal[0] && edge_equal[1] && edge_equal[2]))
    return 0;

  /* Cubic-P */
  if (check_cubic(min_lattice, symprec)) {
    mat_copy_matrix_d3(bravais->lattice, min_lattice);
    bravais->centering = NO_CENTER;
    debug_print("cubic, no-centering\n");
    goto found;
  }

  /* Cubic-I */
  if (brv_cubic_I_center(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = BODY;
    debug_print("cubic, I-center\n");
    goto found;
  }

  /* Cubic-F */
  if (brv_cubic_F_center(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = FACE;
    debug_print("cubic, F-center\n");
    goto found;
  }

 not_found:
  return 0;

 found:
  /* If the Bravais lattice basis vectors are parallel to cartesian axes, */
  /* they are normalized along (1,0,0), (0,1,0), (0,0,1). */
  for ( i = 0; i < 3; i++ ) {
    for ( j = 0; j < 3; j++ ) {
      if ( mat_Dabs( bravais->lattice[i][j] ) < symprec ) {
	count += 1;
      }
      else {
	sum += mat_Dabs( bravais->lattice[i][j] );
      }
    }
  }

  if ( count == 6 ) {
    for ( i = 0; i < 3; i++ ) {
      for ( j = 0; j < 3; j++ ) {
	if ( i == j ) {
	  bravais->lattice[i][j] = sum / 3;
	} 
	else {
	  bravais->lattice[i][j] = 0;
	}
      }
    }
  }
  return 1;
}
  
static int check_cubic(const double lattice[3][3], const double symprec)
{
  int angle_90[3], edge_equal[3];
  check_angle90(angle_90, lattice, symprec);
  check_equal_edge(edge_equal, lattice, symprec);
 
  if (angle_90[0] && angle_90[1] && angle_90[2] && 
      edge_equal[0] && edge_equal[1] && edge_equal[2]) {
    return 1;
  }

  return 0;
}

static int brv_cubic_F_center(double lattice[3][3], const double min_lattice[3][3],
			      const double symprec)
{
  const int relative_axes[22][3] = {
    {-1, 1, 1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1, 1, 1},
    { 0, 1, 1}, /* 5 */
    { 1, 0, 1},
    { 1, 1, 0},
    { 0, 1,-1},
    {-1, 0, 1},
    { 1,-1, 0}, /* 10 */
    { 2, 1, 1},
    { 1, 2, 1},
    { 1, 1, 2},
    { 2, 1,-1},
    {-1, 2, 1}, /* 15 */
    { 1,-1, 2},
    { 2,-1,-1},
    {-1, 2,-1},
    {-1,-1, 2},
    { 2,-1, 1}, /* 20 */
    { 1, 2,-1},
    {-1, 1, 2},
  };

  return brv_exhaustive_search(lattice, min_lattice, check_cubic, relative_axes,
			       22, FACE, symprec);
}

static int brv_cubic_I_center(double lattice[3][3], const double min_lattice[3][3],
			      const double symprec)
{
  const int relative_axes[6][3] = {
    { 0, 1, 1},
    { 1, 0, 1},
    { 1, 1, 0},
    { 0, 1,-1},
    {-1, 0, 1},
    { 1,-1, 0},
  };

  return brv_exhaustive_search(lattice, min_lattice, check_cubic, relative_axes,
			       6, BODY, symprec);
}

/****************/
/*  Tetragonal  */
/****************/
static int get_brv_tetra(Bravais *bravais, const double min_lattice[3][3],
			 const double symprec)
{
  int i, j, count;
  int angle_90[3], edge_equal[3];
  double length[3];

  check_angle90(angle_90, min_lattice, symprec);
  check_equal_edge(edge_equal, min_lattice, symprec);

  /* Tetragonal-P */
  if ((angle_90[0] && angle_90[1] && angle_90[2]) &&
      (edge_equal[0] || edge_equal[1] || edge_equal[2])) {
    if (brv_tetra_primitive(bravais->lattice, min_lattice, symprec)) {
      bravais->centering = NO_CENTER;
      debug_print("tetra, no-centering\n");
      goto found;
    }
  }

  /* Tetragonal-I */
  /* There are three patterns. */
  /* One or two or three primitive axes orient to the body center. */

  /* One */
  if ((angle_90[0] && edge_equal[0]) || (angle_90[1] && edge_equal[1])
      || (angle_90[2] && edge_equal[2])) {
    if (brv_tetra_one(bravais->lattice, min_lattice, symprec)) {
      debug_print("tetra1, I-center\n");
      bravais->centering = BODY;
      goto found;
    }
  }

  /* Three */
  /* All thses axes orient to the body center. */
  if (edge_equal[0] && edge_equal[1] && edge_equal[2]) {
    if (brv_tetra_three(bravais->lattice, min_lattice, symprec)) {
      bravais->centering = BODY;
      debug_print("tetra three, I-center\n");
      goto found;
    }
  }

  /* Two */
  if (edge_equal[0] || edge_equal[1] || edge_equal[2]) {
    if (brv_tetra_two(bravais->lattice, min_lattice, symprec)) {
      bravais->centering = BODY;
      debug_print("tetra two, I-center\n");
      goto found;
    }
  }

 not_found:
  return 0;

 found:
  /* If the Bravais lattice basis vectors are parallel to cartesian axes, */
  /* they are normalized along (1,0,0), (0,1,0), (0,0,1). */
  for ( i = 0; i < 3; i++ ) {
    for ( j = 0; j < 3; j++ ) {
      if ( mat_Dabs( bravais->lattice[i][j] ) < symprec ) {
	count += 1;
      }
      else {
	length[i] = bravais->lattice[i][j];
      }
    }
  }

  if ( count == 6 ) {
    for ( i = 0; i < 3; i++ ) {
      for ( j = 0; j < 3; j++ ) {
	if ( i == j ) {
	  if ( j == 2 ) {
	    bravais->lattice[i][j] = mat_Dabs( length[2] );
	  }
	  else {
	    bravais->lattice[i][j] =
	      ( mat_Dabs( length[0] ) + mat_Dabs( length[1] ) ) / 2;
	  }
	} 
	else {
	  bravais->lattice[i][j] = 0;
	}
      }
    }
  }
  return 1;
}

static int check_tetra(const double lattice[3][3], const double symprec)
{
  int angle_90[3], edge_equal[3];

  check_angle90(angle_90, lattice, symprec);
  check_equal_edge(edge_equal, lattice, symprec);
 
  if (angle_90[0] && angle_90[1] && angle_90[2] && edge_equal[2]) {
    debug_print("*** check_tetra ***\n");
    debug_print_matrix_d3(lattice);
    return 1;
  }

  return 0;
}

static int brv_tetra_primitive(double lattice[3][3], const double min_lattice[3][3],
			       const double symprec)
{
  const int relative_axes[3][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1}
  };

  return brv_exhaustive_search(lattice, min_lattice, check_tetra, relative_axes,
			       3, NO_CENTER, symprec);
}

static int brv_tetra_one(double lattice[3][3], const double min_lattice[3][3],
			 const double symprec)
{
  const int relative_axes[15][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 1, 1, 2},
    {-1, 1, 2},
    { 1,-1, 2},
    {-1,-1, 2},
    { 1, 2, 1},
    {-1, 2, 1},
    { 1, 2,-1},
    {-1, 2,-1},
    { 2, 1, 1},
    { 2,-1, 1},
    { 2, 1,-1},
    { 2,-1,-1}
  };

  return brv_exhaustive_search(lattice, min_lattice, check_tetra, relative_axes,
			       15, BODY, symprec);
}

static int brv_tetra_two(double lattice[3][3], const double min_lattice[3][3],
			 const double symprec)
{
  const int relative_axes[13][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 1, 1, 0},
    { 1,-1, 0},
    { 0, 1, 1},
    { 0, 1,-1},
    { 1, 0, 1},
    {-1, 0, 1},
    {-1, 1, 1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1, 1, 1},
  };

  return brv_exhaustive_search(lattice, min_lattice, check_tetra, relative_axes,
			       13, BODY, symprec);
}

static int brv_tetra_three(double lattice[3][3], const double min_lattice[3][3],
			   const double symprec)
{
  const int relative_axes[6][3] = {
    { 0, 1, 1},
    { 1, 0, 1},
    { 1, 1, 0},
    { 0, 1,-1},
    {-1, 0, 1},
    { 1,-1, 0},
  };

  return brv_exhaustive_search(lattice, min_lattice, check_tetra, relative_axes,
			       6, BODY, symprec);
}

/******************/
/*  Orthorhombic  */
/******************/
static int get_brv_ortho(Bravais *bravais, const double min_lattice[3][3],
			 const double symprec)
{
  Centering centering;

  /* orthorhombic-P */
  if (check_ortho(min_lattice, symprec)) {
    mat_copy_matrix_d3(bravais->lattice, min_lattice);
    bravais->centering = NO_CENTER;
    goto found;
  }

  /* orthorhombic-C (or A,B) */
  centering = brv_ortho_base_center(bravais->lattice, min_lattice, symprec);
  if (centering) {
    bravais->centering = centering;
    goto found;
  }

  /* orthorhombic-I */
  if (brv_ortho_I_center(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = BODY;
    goto found;
  }

  /* orthorhombic-F */
  if (brv_ortho_F_center(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = FACE;
    goto found;
  }

  /* Not found */
 not_found:
  return 0;

  /* Found */
 found:
  return 1;
}

static int check_ortho(const double lattice[3][3], const double symprec)
{
  int angle_90[3];

  check_angle90(angle_90, lattice, symprec);
 
  if (angle_90[0] && angle_90[1] && angle_90[2]) {
    return 1;
  }
  
  return 0;
}

static Centering brv_ortho_base_center(double lattice[3][3],
				       const double min_lattice[3][3],
				       const double symprec)
{
  const int relative_axes_one[15][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 1, 0, 2},
    {-1, 0, 2},
    { 0, 1, 2},
    { 0,-1, 2},
    { 0, 2, 1},
    { 0, 2,-1},
    { 1, 2, 0},
    {-1, 2, 0},
    { 2, 1, 0},
    { 2,-1, 0},
    { 2, 0, 1},
    { 2, 0,-1}
  };

  const int relative_axes_two[9][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 0, 1, 1},
    { 0, 1,-1},
    { 1, 0, 1},
    {-1, 0, 1},
    { 1, 1, 0},
    { 1,-1, 0},
  };


  /* One axis orients to the base center. */
  if (brv_exhaustive_search(lattice, min_lattice, check_ortho, relative_axes_one,
			    15, BASE, symprec))
    return get_base_center(lattice, min_lattice, symprec);


  /* Two axes orient to the base center. */
  if (brv_exhaustive_search(lattice, min_lattice, check_ortho, relative_axes_two,
			    9, BASE, symprec))
    return get_base_center(lattice, min_lattice, symprec);


  return NO_CENTER;
}

static int brv_ortho_I_center(double lattice[3][3], const double min_lattice[3][3],
			      const double symprec)
{
  /* Basically same as Tetra-I */

  const int relative_axes_one[15][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 1, 1, 2},
    {-1, 1, 2}, /*  5 */
    { 1,-1, 2},
    {-1,-1, 2},
    { 1, 2, 1},
    {-1, 2, 1},
    { 1, 2,-1}, /* 10 */
    {-1, 2,-1},
    { 2, 1, 1},
    { 2,-1, 1},
    { 2, 1,-1},
    { 2,-1,-1}  /* 15 */
  };

  const int relative_axes_two[13][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 1, 1, 0},
    { 1,-1, 0}, /*  5 */
    { 0, 1, 1},
    { 0, 1,-1},
    { 1, 0, 1},
    {-1, 0, 1},
    {-1, 1, 1}, /* 10 */
    { 1,-1, 1},
    { 1, 1,-1},
    { 1, 1, 1},
  };

  const int relative_axes_three[6][3] = {
    { 0, 1, 1},
    { 1, 0, 1},
    { 1, 1, 0},
    { 0, 1,-1},
    {-1, 0, 1}, /*  5 */
    { 1,-1, 0},
  };


  /* One axis orients to the I center. */
  if (brv_exhaustive_search(lattice, min_lattice, check_ortho, relative_axes_one,
			    15, BODY, symprec))
    return 1;

  /* Two axes orient to the I center. */
  if (brv_exhaustive_search(lattice, min_lattice, check_ortho, relative_axes_two,
			    13, BODY, symprec))
    return 1;

  /* Three axes orient to the I center. */
  if (brv_exhaustive_search(lattice, min_lattice, check_ortho, relative_axes_three,
			    6, BODY, symprec))
    return 1;

  return 0;
}

static int brv_ortho_F_center(double lattice[3][3], const double min_lattice[3][3],
			      const double symprec)
{
  /* At least two axes orient to the face center. */
  const int relative_axes_two[21][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 1, 1, 0},
    { 1,-1, 0}, /* 5 */
    { 0, 1, 1},
    { 0, 1,-1},
    { 1, 0, 1},
    {-1, 0, 1},
    { 1, 0, 2}, /* 10 */
    {-1, 0, 2},
    { 0, 1, 2},
    { 0,-1, 2},
    { 0, 2, 1},
    { 0, 2,-1}, /* 15 */
    { 1, 2, 0},
    {-1, 2, 0},
    { 2, 1, 0},
    { 2,-1, 0},
    { 2, 0, 1}, /* 20 */
    { 2, 0,-1}
  };

  const int relative_axes_three[22][3] = {
    {-1, 1, 1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1, 1, 1},
    { 0, 1, 1}, /* 5 */
    { 1, 0, 1},
    { 1, 1, 0},
    { 0, 1,-1},
    {-1, 0, 1},
    { 1,-1, 0}, /* 10 */
    { 2, 1, 1},
    { 1, 2, 1},
    { 1, 1, 2},
    { 2, 1,-1},
    {-1, 2, 1}, /* 15 */
    { 1,-1, 2},
    { 2,-1,-1},
    {-1, 2,-1},
    {-1,-1, 2},
    { 2,-1, 1}, /* 20 */
    { 1, 2,-1},
    {-1, 1, 2},
  };


  /* Two axes orient to the F center. */
  if (brv_exhaustive_search(lattice, min_lattice, check_ortho, relative_axes_two,
			    21, FACE, symprec))
    return 1;

  /* Three axes orient to the F center. */
  if (brv_exhaustive_search(lattice, min_lattice, check_ortho, relative_axes_three,
			    22, FACE, symprec))
    return 1;

  return 0;
}



/************************************/
/*  Hexagonal and Trigonal systems  */
/************************************/
static int get_brv_hexa(Bravais *bravais, const double min_lattice[3][3],
			const double symprec)
{
  const int relative_axes[5][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    {-1, 0, 0},
    { 0,-1, 0},
  };

  if (brv_exhaustive_search(bravais->lattice, min_lattice, check_hexa,
			    relative_axes, 5, NO_CENTER, symprec)) {
    bravais->centering = NO_CENTER;
    return 1;
  }

  return 0;
}

static int check_hexa(const double lattice[3][3], const double symprec)
{
  int angle_90[3], edge_equal[3];
  double ratio, metric[3][3];

  check_angle90(angle_90, lattice, symprec);
  check_equal_edge(edge_equal, lattice, symprec);

  if (angle_90[0] && angle_90[1] && edge_equal[2]) {
    get_metric(metric, lattice);
    ratio = metric[0][1] / metric[0][0];

    if ( mat_Dabs(ratio + 0.5) < symprec ) {
      return 1;
    }
  }

  return 0;
}

/*************************/
/*  Rhombohedral system  */
/*************************/
static int get_brv_rhombo(Bravais *bravais, const double min_lattice[3][3],
			  const double symprec)
{
  int edge_equal[3];
  check_equal_edge(edge_equal, min_lattice, symprec);

  /* One or two or three axes orient to the rhombochedral lattice points. */

  /* Three */
  if (edge_equal[0] && edge_equal[1] && edge_equal[2]) {
    if (brv_rhombo_three(bravais->lattice, min_lattice, symprec)) {
      bravais->centering = NO_CENTER;
      return 1;
    }
  }

  /* Two of three are in the basal plane or Two are the rhombo axes. */
  if (edge_equal[0] || edge_equal[1] || edge_equal[2]) {
    if (brv_rhombo_two(bravais->lattice, min_lattice, symprec)) {
      bravais->centering = NO_CENTER;
      return 1;
    }
  }

  return 0;
}

static int check_rhombo(const double lattice[3][3], const double symprec)
{
  double metric[3][3];
  int edge_equal[3];

  get_metric(metric, lattice);
  check_equal_edge(edge_equal, lattice, symprec);

  if (edge_equal[0] && edge_equal[1] && edge_equal[2] &&
      (mat_Dabs((metric[0][1] - metric[1][2]) / metric[0][1]) < symprec * symprec) &&
      (mat_Dabs((metric[0][1] - metric[0][2]) / metric[0][1]) < symprec * symprec)) {
    return 1;
  }

  return 0;
}

static int brv_rhombo_two(double lattice[3][3], const double min_lattice[3][3], const double symprec)
{
  const int relative_axes_rhombo[13][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    {-1, 0, 0},
    { 0,-1, 0}, /*  5 */
    { 0, 0,-1},
    {-1, 1, 1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1,-1,-1}, /* 10 */
    {-1, 1,-1},
    {-1,-1, 1},
    { 1, 1, 1} 
  };

  const int relative_axes_base[26][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    {-1, 0, 0},
    { 0,-1, 0}, /*  5 */ 
    { 0, 0,-1},
    {-1, 1, 1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1,-1,-1}, /* 10 */
    {-1, 1,-1},
    {-1,-1, 1},
    { 0, 1, 1},
    { 1, 0, 1},
    { 1, 1, 0}, /* 15 */ 
    { 0, 1,-1},
    {-1, 0, 1},
    { 1,-1, 0},
    { 0,-1, 1},
    { 1, 0,-1}, /* 20 */
    {-1, 1, 0},
    { 0,-1,-1},
    {-1, 0,-1},
    {-1,-1, 0},
    { 1, 1, 1}, /* 25 */
    {-1,-1,-1} 
  };

  if (brv_exhaustive_search(lattice, min_lattice, check_rhombo,
			    relative_axes_rhombo, 13, NO_CENTER, symprec)) {
    return 1;
  }

  if (brv_exhaustive_search(lattice, min_lattice, check_rhombo, relative_axes_base,
			    26, NO_CENTER, symprec)) {
    return 1;
  }

  return 0;
}

static int brv_rhombo_three(double lattice[3][3], const double min_lattice[3][3], 
			    const double symprec)
{
  const int relative_axes[5][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 0,-1, 0},
    { 0, 0,-1},
  };

  return brv_exhaustive_search(lattice, min_lattice, check_rhombo, relative_axes,
			       5, NO_CENTER, symprec);
}

/***********************/
/*  Monoclinic system  */
/***********************/
static int get_brv_monocli(Bravais *bravais, const double min_lattice[3][3],
			   const double symprec)
{
  Centering centering;

  /* Monoclinic definition: */
  /*  second setting, i.e., alpha=gamma=90 deg. beta is not 90 deg. */

  /* Monoclinic-P */
  if (brv_monocli_primitive(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = NO_CENTER;
    debug_print("Monoclinic-P\n");
    goto end;
  }

  /* Monoclinic base center*/
  centering = brv_monocli_base_center(bravais->lattice, min_lattice, symprec);
  if (centering) {
    bravais->centering = centering;
    debug_print("Monoclinic base center\n");
    set_brv_monocli(bravais, symprec);
    goto end;
  }

  return 0;

 end:
  return 1;
}


static int check_monocli(const double lattice[3][3], const double symprec)
{
  int angle_90[3];

  check_angle90(angle_90, lattice, symprec);

  if (angle_90[0] && angle_90[2]) {
    return 1;
  }

  /* Not found */
  return 0;
}

static void set_brv_monocli(Bravais *bravais, const double symprec)
{
  int i, angle_90[3];
  Centering centering;
  double tmp_lattice[3][3], lattice[3][3];

  mat_copy_matrix_d3(lattice, bravais->lattice);
  centering = bravais->centering;
  mat_copy_matrix_d3(tmp_lattice, lattice);
  check_angle90(angle_90, lattice, symprec);

  if (angle_90[1] && angle_90[2]) {
    for (i = 0; i < 3; i++) {
      lattice[i][0] =  tmp_lattice[i][1];
      lattice[i][1] = -tmp_lattice[i][0];
      lattice[i][2] =  tmp_lattice[i][2];
    }

    if (centering == B_FACE)
      centering = A_FACE;

  } else {
    if (angle_90[0] && angle_90[1]) {
      for (i = 0; i < 3; i++) {
	lattice[i][0] =  tmp_lattice[i][0];
	lattice[i][1] = -tmp_lattice[i][2];
	lattice[i][2] =  tmp_lattice[i][1];
      }

      if (centering == B_FACE)
	centering = C_FACE;
    }
  }

  mat_copy_matrix_d3(tmp_lattice, lattice);

  if (centering == A_FACE) {
    for (i = 0; i < 3; i++) {
      lattice[i][0] = -tmp_lattice[i][2];
      lattice[i][1] =  tmp_lattice[i][1];
      lattice[i][2] =  tmp_lattice[i][0];
    }
    centering = C_FACE;
  }

  if (centering == BODY) {

    for (i = 0; i < 3; i++) {
      lattice[i][0] = tmp_lattice[i][0] + tmp_lattice[i][2];
      lattice[i][1] = tmp_lattice[i][1];
      lattice[i][2] = tmp_lattice[i][2];
    }
    centering = C_FACE;
  }

  mat_copy_matrix_d3(bravais->lattice, lattice);
  bravais->centering = centering;
}

static int brv_monocli_primitive(double lattice[3][3],
				 const double min_lattice[3][3],
				 const double symprec)
{
  const int relative_axes[3][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1}
  };

  return brv_exhaustive_search(lattice, min_lattice, check_monocli, relative_axes,
			       3, NO_CENTER, symprec);
}

static Centering brv_monocli_base_center(double lattice[3][3],
					 const double min_lattice[3][3],
					 const double symprec)
{
  int found;
  Centering centering = NO_CENTER;
  
  int relative_axes_one[15][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 1, 0, 2},
    {-1, 0, 2}, /*  5 */
    { 0, 1, 2},
    { 0,-1, 2},
    { 0, 2, 1},
    { 0, 2,-1},
    { 1, 2, 0}, /* 10 */
    {-1, 2, 0},
    { 2, 1, 0},
    { 2,-1, 0},
    { 2, 0, 1},
    { 2, 0,-1}  /* 15 */
  };

  int relative_axes_two_three[34][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    { 0, 1, 1},
    { 0, 1,-1}, /*  5 */
    { 1, 0, 1},
    {-1, 0, 1},
    { 1, 1, 0},
    { 1,-1, 0},
    { 1, 0, 2}, /* 10 */
    {-1, 0, 2},
    { 0, 1, 2},
    { 0,-1, 2},
    { 0, 2, 1},
    { 0, 2,-1}, /* 15 */
    { 1, 2, 0},
    {-1, 2, 0},
    { 2, 1, 0},
    { 2,-1, 0},
    { 2, 0, 1}, /* 20 */
    { 2, 0,-1},
    { 1, 1, 2},
    {-1, 1, 2},
    {-1,-1, 2},
    { 1, 2, 1}, /* 25 */
    { 1, 2,-1},
    {-1, 2,-1},
    { 2, 1, 1},
    { 2,-1, 1},
    { 2, 1,-1}, /* 30 */
    {-1, 1, 1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1, 1, 1}  /* 34 */
  };

  /* One axis orients to the base center. */
  found = brv_exhaustive_search(lattice, min_lattice, check_monocli, relative_axes_one,
				15, BASE, symprec);

  /* Two or three axes orient to the base center. */
  if (!found) {
   found = brv_exhaustive_search(lattice, min_lattice, check_monocli,
				 relative_axes_two_three, 34, BASE, symprec);
  }

  if (found) {
    brv_smallest_lattice_vector(lattice, lattice, symprec);
    centering = get_base_center(lattice, min_lattice, symprec);
    debug_print("Monocli centering: %d\n",  centering);
    return centering;
  }

  return NO_CENTER;
}


/*******************************/
/* The other private functions */
/*******************************/
static void check_angle90(int angle_90[3], const double lattice[3][3],
			  const double symprec)
{
  int i, c0, c1;
  double metric[3][3];

  get_metric(metric, lattice);
  for (i = 0; i < 3; i++) {

    c0 = (i + 1) % 3;
    c1 = (i + 2) % 3;

    if ( mat_Dabs(metric[c0][c1] * metric[c1][c0]) / metric[c0][c0] / metric[c1][c1]
        < symprec * symprec ) {        /* orthogonal */
      angle_90[i] = 1;
    }
    else {
      angle_90[i] = 0;
    }
  }
}

static void check_equal_edge(int edge_equal[3], const double lattice[3][3],
			     const double symprec)
{
  int i, c0, c1;
  double metric[3][3];

  get_metric(metric, lattice);
  for (i = 0; i < 3; i++) {

    c0 = (i + 1) % 3;
    c1 = (i + 2) % 3;

    if ( mat_Dabs(metric[c0][c0] - metric[c1][c1]) / metric[c0][c0]
	 < symprec * symprec) {	/* Equal edges */
      edge_equal[i] = 1;
    }
    else {
      edge_equal[i] = 0;
    }
  }
}

static void get_metric(double metric[3][3], const double lattice[3][3])
{
  double lattice_t[3][3];
  mat_transpose_matrix_d3(lattice_t, lattice);
  mat_multiply_matrix_d3(metric, lattice_t, lattice);
}

static int brv_exhaustive_search(double lattice[3][3], const double min_lattice[3][3],
				 int (*check_bravais)(const double lattice[3][3], const double symprec),
				 const int relative_axes[][3], const int num_axes,
				 const Centering centering, const double symprec)
{
  int i, j, k, l, factor = 1, coordinate[3][3];
  double tmp_matrix[3][3];

  switch (centering) {
  case NO_CENTER:
    factor = 1;
    break;
  case BODY:
  case BASE:
  case A_FACE:
  case B_FACE:
  case C_FACE:
    factor = 2;
    break;
  case FACE:
    factor = 4;
    break;
  }

  for (i = 0; i < num_axes; i++) {
    for (j = 0; j < num_axes; j ++) {
      for (k = 0; k < num_axes; k++) {

	for (l = 0; l < 3; l++) {
	  coordinate[l][0] = relative_axes[i][l];
	  coordinate[l][1] = relative_axes[j][l];
	  coordinate[l][2] = relative_axes[k][l];
	}
	    
	if (mat_Dabs(mat_get_determinant_i3(coordinate)) == factor) { 

	  mat_cast_matrix_3i_to_3d(tmp_matrix, coordinate);
	  mat_multiply_matrix_d3(lattice, min_lattice, tmp_matrix);

	  if ((*check_bravais)(lattice, symprec) &&
	      mat_Dabs(mat_get_determinant_d3(lattice)) > symprec)
	    return 1;
	}
      }
    }
  }

  return 0;
}

static Centering get_base_center(const double brv_lattice[3][3],
				 const double min_lattice[3][3],
				 const double symprec) {
  int i;
  Centering centering = NO_CENTER;
  double tmp_matrix[3][3], axis[3][3];

  if (mat_inverse_matrix_d3(tmp_matrix, brv_lattice, symprec)) {
    mat_multiply_matrix_d3(axis, tmp_matrix, min_lattice);
  } else {
    fprintf(stderr, "spglib BUG: %s in %s\n", __FUNCTION__, __FILE__);
    return NO_CENTER;
  }

  debug_print_matrix_d3(min_lattice);
  debug_print_matrix_d3(brv_lattice);
  debug_print("%f %f %f\n", axis[0][0], axis[0][1], axis[0][2]);
  debug_print("%f %f %f\n", axis[1][0], axis[1][1], axis[1][2]);
  debug_print("%f %f %f\n", axis[2][0], axis[2][1], axis[2][2]);

  /* C center */
  for (i = 0; i < 3; i++) {
    if ((mat_Dabs(mat_Dabs(axis[0][i]) - 0.5) < symprec) &&
	(mat_Dabs(mat_Dabs(axis[1][i]) - 0.5) < symprec) &&
	(!(mat_Dabs(mat_Dabs(axis[2][i]) - 0.5) < symprec))) {
      centering = C_FACE;
      goto end;
    }
  }

  /* A center */
  for (i = 0; i < 3; i++) {
    if ((!(mat_Dabs(mat_Dabs(axis[0][i]) - 0.5) < symprec)) &&
	(mat_Dabs(mat_Dabs(axis[1][i]) - 0.5) < symprec) &&
	(mat_Dabs(mat_Dabs(axis[2][i]) - 0.5) < symprec)) {
      centering = A_FACE;
      goto end;
    }
  }

  /* B center */
  for (i = 0; i < 3; i++) {
    if ((mat_Dabs(mat_Dabs(axis[0][i]) - 0.5) < symprec) &&
	(!(mat_Dabs(mat_Dabs(axis[1][i]) - 0.5) < symprec)) &&
	(mat_Dabs(mat_Dabs(axis[2][i]) - 0.5) < symprec)) {
      centering = B_FACE;
      goto end;
    }
  }

  /* body center */
  for (i = 0; i < 3; i++) {
    if ((mat_Dabs(mat_Dabs(axis[0][i]) - 0.5) < symprec) &&
	(mat_Dabs(mat_Dabs(axis[1][i]) - 0.5) < symprec) &&
	(mat_Dabs(mat_Dabs(axis[2][i]) - 0.5) < symprec)) {
      centering = BODY;
      goto end;
    }
  }

  /* This should not happen. */
  fprintf(stderr, "spglib BUG: %s in %s\n", __FUNCTION__, __FILE__);
  return NO_CENTER;

 end:
  return centering;
}

static void get_right_hand_lattice(double lattice[3][3], const double symprec)
{
  int i;

  if (mat_get_determinant_d3(lattice) < symprec) {
    for (i = 0; i < 3; i++) {
      lattice[i][0] = -lattice[i][0];
      lattice[i][1] = -lattice[i][1];
      lattice[i][2] = -lattice[i][2];
    }
  }
}




/* Delaunay reduction */
/* Pointers can be found in International table A. */
static void get_Delaunay_reduction(double red_lattice[3][3], 
				   const double lattice[3][3],
				   const double symprec)
{
  int i, j, cell_type;
  double basis[4][3], tmp_lattice[3][3];

  get_exteneded_basis(basis, lattice);

  while (1) {
    if (get_Delaunay_reduction_basis(basis, symprec)) {
      break;
    }
  }

/*   get_smallest_basis(basis, symprec); */
  for ( i = 0; i < 3; i++ ) 
    for ( j = 0; j < 3; j++ )
      red_lattice[i][j] = basis[j][i];

  debug_print("Delaunay reduction\n");
  debug_print_matrix_d3(red_lattice);

}

static int compare_vectors(const void *_vec1, const void *_vec2)
{
  double *vec1 = (double *)_vec1;
  double *vec2 = (double *)_vec2;

  return (vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2] >
	  vec2[0] * vec2[0] + vec2[1] * vec2[1] + vec2[2] * vec2[2]);
}

static int get_Delaunay_reduction_basis(double basis[4][3], double symprec)
{
  int i, j, k, l;
  double dot_product;

  for ( i = 0; i < 4; i++ ) {
    for ( j = i+1; j < 4; j++ ) {
      dot_product = 0.0;
      for ( k = 0; k < 3; k++ ) {
	dot_product += basis[i][k] * basis[j][k];
      }
      if ( dot_product > symprec ) {
	for ( k = 0; k < 4; k++ ) {
	  if ( k != i && k != j ) {
	    for ( l = 0; l < 3; l++ ) {
	      basis[k][l] = basis[i][l] + basis[k][l];
	    }
	  }
	}
	for ( k = 0; k < 3; k++ ) {
	  basis[i][k] = -basis[i][k];
	  basis[j][k] = basis[j][k];
	}
	return 0;
      }
    }
  }

  return 1;
}

static void get_smallest_basis(double basis[4][3], double symprec)
{
  int i, j, k, l;
  double vectors[20][3], tmp_lattice[3][3], metric[3][3];
  double sum, min, vol, min_vol;

  /* ----------------------- */
  /* Look for shortest three */
  /* ----------------------- */
  for ( i = 0; i < 4; i++ ) {
    for ( j = 0; j < 3; j++ ) {
      vectors[i][j] = basis[i][j];
    }
  }

  for ( i = 0; i < 3; i++ ) {
    vectors[4][i] = basis[0][i] + basis[1][i];
    vectors[5][i] = basis[1][i] + basis[2][i];
    vectors[6][i] = basis[2][i] + basis[0][i];
  }

  qsort(vectors, 7, sizeof(vectors[0]), (int (*)(const void*, const void*))compare_vectors);

  min = ( vectors[6][0]*vectors[6][0] + vectors[6][1]*vectors[6][1] + 
	  vectors[6][2]*vectors[6][2] ) * 4;
  for ( i = 0; i < 7; i++ ) {
    for ( j = i+1; j < 7; j++ ) {
      for ( k = j+1; k < 7; k++ ) {
	for ( l = 0; l < 3; l++ ) {
	  tmp_lattice[0][l] = vectors[i][l];
	  tmp_lattice[1][l] = vectors[j][l];
	  tmp_lattice[2][l] = vectors[k][l];
	}
	vol = mat_Dabs(mat_get_determinant_d3(tmp_lattice));
	if ( vol > symprec ) {
	  get_metric(metric, tmp_lattice);
	  sum = metric[0][0] + metric[1][1] + metric[2][2];
	  for ( l = 0; l < 3; l++ ) {
	    sum += (tmp_lattice[0][l] + tmp_lattice[1][l] + tmp_lattice[2][l]) * 
	      (tmp_lattice[0][l] + tmp_lattice[1][l] + tmp_lattice[2][l]);
	  }
	  if ( mat_Dabs(sum - min) < symprec*2 ) {
	    debug_print("1: sum %f (%f), vol %f (%f)\n", sum, min, vol, min_vol);
	    min = sum;
	    min_vol = vol;
	    for ( l = 0; l < 3; l++ ) {
	      basis[0][l] = vectors[i][l];
	      basis[1][l] = vectors[j][l];
	      basis[2][l] = vectors[k][l];
	      basis[3][l] = -vectors[i][l]-vectors[j][l]-vectors[k][l];
	    }
	  }
	}
      }
    }
  }
}


static void get_exteneded_basis(double basis[4][3], const double lattice[3][3])
{
  int i, j;

  for ( i = 0; i < 3; i++ ) {
    for ( j = 0; j < 3; j++ ) {
      basis[i][j] = lattice[j][i];
    }
  }

  for ( i = 0; i < 3; i++ ) {
    basis[3][i] = -lattice[i][0] -lattice[i][1] -lattice[i][2];
  }
}

