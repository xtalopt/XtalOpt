/* bravais.c */
/* Copyright (C) 2008 Atsushi Togo */

#include <stdio.h>
#include <stdlib.h>
/* #include <math.h> */
#include "bravais.h"
#include "debug.h"
#include "mathfunc.h"

static int relative_lattice1[39192][3][3];
static int relative_lattice2[31848][3][3];
static int relative_lattice4[20352][3][3];

static int get_brv_cubic( Bravais *bravais,
			  SPGCONST double min_lattice[3][3],
			  const double symprec );
static int get_brv_tetra( Bravais *bravais,
			  SPGCONST double min_lattice[3][3],
			  const double symprec );
static int get_brv_hexa( Bravais *bravais,
			 SPGCONST double min_lattice[3][3],
			 const double symprec );
static int get_brv_rhombo( Bravais *bravais,
			   SPGCONST double min_lattice[3][3],
			   const double symprec );
static int get_brv_ortho( Bravais *bravais,
			  SPGCONST double min_lattice[3][3],
			  const double symprec );
static int get_brv_monocli( Bravais *bravais,
			    SPGCONST double min_lattice[3][3],
			    const double symprec );
static int brv_cubic_I_center( double lattice[3][3],
			       SPGCONST double min_lattice[3][3],
			       const double symprec );
static int brv_cubic_F_center( double lattice[3][3],
			       SPGCONST double min_lattice[3][3],
			       const double symprec );
static int brv_tetra_primitive( double lattice[3][3],
				SPGCONST double min_lattice[3][3],
				const double symprec );
static int brv_tetra_body( double lattice[3][3],
			   SPGCONST double min_lattice[3][3],
			   const double symprec );
static Centering brv_ortho_base_I_center( double lattice[3][3],
					  SPGCONST double min_lattice[3][3],
					  const double symprec);
static Centering get_base_center( SPGCONST double brv_lattice[3][3],
				  SPGCONST double min_lattice[3][3],
				  const double symprec );
static int brv_ortho_F_center( double lattice[3][3],
			       SPGCONST double min_lattice[3][3],
			       const double symprec );
static int brv_rhombo( double lattice[3][3],
		       SPGCONST double min_lattice[3][3],
		       const double symprec );
static void set_brv_monocli( Bravais *bravais,
			     const double symprec );
static int brv_monocli_primitive( double lattice[3][3],
				  SPGCONST double min_lattice[3][3],
				  const double symprec );
static Centering brv_monocli_base_center( double lattice[3][3],
					  SPGCONST double min_lattice[3][3],
					  const double symprec );
static void check_angle90( int angle_90[3],
			   SPGCONST double lattice[3][3],
			   const double symprec );
static void check_equal_edge( int edge_equal[3],
			      SPGCONST double lattice[3][3],
			      const double symprec);
static int check_cubic( SPGCONST double lattice[3][3],
			const double symprec );
static int check_hexa( SPGCONST double lattice[3][3],
		       const double symprec );
static int check_tetra( SPGCONST double lattice[3][3],
			const double symprec );
static int check_ortho( SPGCONST double lattice[3][3],
			const double symprec );
static int check_monocli( SPGCONST double lattice[3][3],
			  const double symprec );
static int check_rhombo( SPGCONST double lattice[3][3],
			 const double symprec );
static void set_relative_lattice( void );
static int exhaustive_search( double lattice[3][3],
			      SPGCONST double min_lattice[3][3],
			      int (*check_bravais)(SPGCONST double lattice[3][3], const double symprec),
			     const Centering centering,
			     const double symprec );
static void get_right_hand_lattice( double lattice[3][3],
				    const double symprec );
static void get_projection( double projection[3][3],
			    SPGCONST double min_lattice[3][3],
			    SPGCONST double lattice[3][3] );
static void get_Delaunay_reduction( double red_lattice[3][3], 
				   SPGCONST double lattice[3][3],
				   SPGCONST double symprec );
static int get_Delaunay_reduction_basis( double basis[4][3],
					 double symprec );
static void get_exteneded_basis( double basis[4][3],
				 SPGCONST double lattice[3][3] );

/* math */
static void get_metric( double metric[3][3],
			SPGCONST double lattice[3][3]);

/**********************/
/**********************/
/** Public functions **/
/**********************/
/**********************/
Bravais brv_get_brv_lattice( SPGCONST double lattice_orig[3][3],
			     const double symprec )
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

  set_relative_lattice();
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
int brv_get_brv_lattice_in_loop( Bravais *bravais,
				 SPGCONST double min_lattice[3][3],
				 const double symprec )
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

void brv_smallest_lattice_vector( double min_lattice[3][3],
				  SPGCONST double lattice[3][3],
				  const double symprec )
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

static void get_projection( double projection[3][3],
			    SPGCONST double min_lattice[3][3],
			    SPGCONST double lattice[3][3] )
{
  double tmp_matrix[3][3];
  
  mat_transpose_matrix_d3(tmp_matrix, min_lattice);
  mat_multiply_matrix_d3(projection, tmp_matrix, lattice);
}

/***********/
/*  Cubic  */
/***********/
static int get_brv_cubic( Bravais *bravais,
			  SPGCONST double min_lattice[3][3],
			  const double symprec )
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

  /* Not found */
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
  
static int check_cubic( SPGCONST double lattice[3][3],
			const double symprec )
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

static int brv_cubic_F_center( double lattice[3][3],
			       SPGCONST double min_lattice[3][3],
			       const double symprec )
{
  return exhaustive_search( lattice, min_lattice,
			    check_cubic, FACE, symprec );

}

static int brv_cubic_I_center( double lattice[3][3],
			       SPGCONST double min_lattice[3][3],
			       const double symprec )
{
  return exhaustive_search( lattice, min_lattice,
			    check_cubic, BODY, symprec );
}

/****************/
/*  Tetragonal  */
/****************/
static int get_brv_tetra( Bravais *bravais,
			  SPGCONST double min_lattice[3][3],
			  const double symprec )
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
  if (brv_tetra_body(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = BODY;
    debug_print("tetra, I-center\n");
    goto found;
  }

  /* Not found */
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

static int check_tetra( SPGCONST double lattice[3][3],
			const double symprec)
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

static int brv_tetra_primitive( double lattice[3][3],
				SPGCONST double min_lattice[3][3],
				const double symprec )
{
  return exhaustive_search( lattice, min_lattice,
			    check_tetra, NO_CENTER, symprec );
}

static int brv_tetra_body( double lattice[3][3],
			   SPGCONST double min_lattice[3][3],
			   const double symprec )
{
  return exhaustive_search( lattice, min_lattice,
			    check_tetra, BODY, symprec );
}

/******************/
/*  Orthorhombic  */
/******************/
static int get_brv_ortho( Bravais *bravais,
			  SPGCONST double min_lattice[3][3],
			  const double symprec )
{
  Centering centering;

  /* orthorhombic-P */
  if (check_ortho(min_lattice, symprec)) {
    mat_copy_matrix_d3(bravais->lattice, min_lattice);
    bravais->centering = NO_CENTER;
    goto found;
  }

  /* orthorhombic-C (or A,B) or I */
  debug_print("Checking Ortho base or body center ...\n");
  centering = brv_ortho_base_I_center(bravais->lattice, min_lattice, symprec);
  if (centering) {
    bravais->centering = centering;
    goto found;
  }

  /* orthorhombic-F */
  debug_print("Checking Ortho face center ...\n");
  if (brv_ortho_F_center(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = FACE;
    goto found;
  }

  /* Not found */
  return 0;

  /* Found */
 found:
  return 1;
}

static int check_ortho( SPGCONST double lattice[3][3],
			const double symprec )
{
  int angle_90[3];

  check_angle90(angle_90, lattice, symprec);
 
  if (angle_90[0] && angle_90[1] && angle_90[2]) {
    return 1;
  }
  
  return 0;
}

static Centering brv_ortho_base_I_center( double lattice[3][3],
					  SPGCONST double min_lattice[3][3],
					  const double symprec )
{
  if (exhaustive_search( lattice, min_lattice,
			 check_ortho, BASE, symprec ))
    return get_base_center(lattice, min_lattice, symprec);

  return NO_CENTER;
}

static int brv_ortho_F_center( double lattice[3][3],
			       SPGCONST double min_lattice[3][3],
			       const double symprec )
{
  if (exhaustive_search( lattice, min_lattice,
			 check_ortho, FACE, symprec ))
    return 1;

  return 0;
}


/************************************/
/*  Hexagonal and Trigonal systems  */
/************************************/
static int get_brv_hexa( Bravais *bravais,
			 SPGCONST double min_lattice[3][3],
			 const double symprec )
{
  if (exhaustive_search(bravais->lattice, min_lattice, check_hexa,
			NO_CENTER, symprec)) {
    bravais->centering = NO_CENTER;
    return 1;
  }

  return 0;
}

static int check_hexa( SPGCONST double lattice[3][3],
		       const double symprec )
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
static int get_brv_rhombo( Bravais *bravais,
			   SPGCONST double min_lattice[3][3],
			   const double symprec )
{
  int edge_equal[3];
  check_equal_edge(edge_equal, min_lattice, symprec);

  if (brv_rhombo(bravais->lattice, min_lattice, symprec)) {
    bravais->centering = NO_CENTER;
    return 1;
  }

  return 0;
}

static int check_rhombo( SPGCONST double lattice[3][3],
			 const double symprec )
{
  double metric[3][3];
  int edge_equal[3];

  get_metric(metric, lattice);
  check_equal_edge(edge_equal, lattice, symprec);

  if (edge_equal[0] && edge_equal[1] && edge_equal[2] &&
      (mat_Dabs(metric[0][1] - metric[1][2]) < symprec * symprec ) &&
      (mat_Dabs(metric[0][1] - metric[0][2]) < symprec * symprec )) {

    return 1;
  }

  return 0;
}

static int brv_rhombo( double lattice[3][3],
		       SPGCONST double min_lattice[3][3],
		       const double symprec )
{
  if (exhaustive_search( lattice, min_lattice,
			 check_rhombo, NO_CENTER, symprec )) {
    return 1;
  }

  return 0;
}

/***********************/
/*  Monoclinic system  */
/***********************/
static int get_brv_monocli( Bravais *bravais,
			    SPGCONST double min_lattice[3][3],
			    const double symprec )
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


static int check_monocli( SPGCONST double lattice[3][3],
			  const double symprec )
{
  int angle_90[3];

  check_angle90(angle_90, lattice, symprec);

  if (angle_90[0] && angle_90[2]) {
    return 1;
  }

  /* Not found */
  return 0;
}

static void set_brv_monocli( Bravais *bravais,
			     const double symprec )
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

static int brv_monocli_primitive( double lattice[3][3],
				  SPGCONST double min_lattice[3][3],
				  const double symprec )
{
  return exhaustive_search( lattice, min_lattice,
			    check_monocli, NO_CENTER, symprec );
}

static Centering brv_monocli_base_center( double lattice[3][3],
					  SPGCONST double min_lattice[3][3],
					  const double symprec )
{
  int found;
  Centering centering = NO_CENTER;
  
  found = exhaustive_search( lattice, min_lattice,
			     check_monocli, BASE, symprec );

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
static void check_angle90( int angle_90[3],
			   SPGCONST double lattice[3][3],
			   const double symprec )
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

static void check_equal_edge( int edge_equal[3],
			      SPGCONST double lattice[3][3],
			      const double symprec )
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

static void get_metric( double metric[3][3],
			SPGCONST double lattice[3][3])
{
  double lattice_t[3][3];
  mat_transpose_matrix_d3(lattice_t, lattice);
  mat_multiply_matrix_d3(metric, lattice_t, lattice);
}

static void set_relative_lattice(void)
{
  /* const int num_relative_lattice1 = 39192; */
  /* const int num_relative_lattice2 = 31848; */
  /* const int num_relative_lattice4 = 20352; */
  const int num_relative_axes = 74;
  const int relative_axes[][3] = {
    { 1, 0, 0},
    { 0, 1, 0},
    { 0, 0, 1},
    {-1, 0, 0},
    { 0,-1, 0}, /* 5 */
    { 0, 0,-1},
    { 0, 1, 1},
    { 1, 0, 1},
    { 1, 1, 0},
    { 0,-1,-1}, /* 10 */
    {-1, 0,-1},
    {-1,-1, 0},
    { 0, 1,-1},
    {-1, 0, 1},
    { 1,-1, 0}, /* 15 */
    { 0,-1, 1},
    { 1, 0,-1},
    {-1, 1, 0},
    { 1, 1, 1},
    {-1,-1,-1}, /* 20 */
    {-1, 1, 1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1,-1,-1},
    {-1, 1,-1}, /* 25 */
    {-1,-1, 1},
    { 0, 1, 2},
    { 2, 0, 1},
    { 1, 2, 0},
    { 0,-1,-2}, /* 30 */
    {-2, 0,-1},
    {-1,-2, 0},
    { 0, 2, 1},
    { 1, 0, 2},
    { 2, 1, 0}, /* 35 */
    { 0,-2,-1},
    {-1, 0,-2},
    {-2,-1, 0},
    { 0,-1, 2},
    { 2, 0,-1}, /* 40 */
    {-1, 2, 0},
    { 0, 1,-2},
    {-2, 0, 1},
    { 1,-2, 0},
    { 0,-2, 1}, /* 45 */
    { 1, 0,-2},
    {-2, 1, 0},
    { 0, 2,-1},
    {-1, 0, 2},
    { 2,-1, 0}, /* 50 */
    { 2, 1, 1},
    { 1, 2, 1},
    { 1, 1, 2},
    {-2,-1,-1},
    {-1,-2,-1}, /* 55 */
    {-1,-1,-2},
    { 2,-1,-1},
    {-1, 2,-1},
    {-1,-1, 2},
    {-2, 1, 1}, /* 60 */
    { 1,-2, 1},
    { 1, 1,-2},
    { 2, 1,-1},
    {-1, 2, 1},
    { 1,-1, 2}, /* 65 */
    {-2,-1, 1},
    { 1,-2,-1},
    {-1, 1,-2},
    { 2,-1, 1},
    { 1, 2,-1}, /* 70 */
    {-1, 1, 2},
    {-2, 1,-1},
    {-1,-2, 1},
    { 1,-1,-2},
  };

  int i, j, k, l, det;
  int coordinate[3][3];
  int count1 = 0, count2 = 0, count4 = 0;
  
  for (i = 0; i < num_relative_axes; i++) {
    for (j = 0; j < num_relative_axes; j ++) {
      for (k = 0; k < num_relative_axes; k++) {
	for (l = 0; l < 3; l++) {
	  coordinate[l][0] = relative_axes[i][l];
	  coordinate[l][1] = relative_axes[j][l];
	  coordinate[l][2] = relative_axes[k][l];
	}

	det = mat_get_determinant_i3(coordinate);
	if ( det == 1 ) {
	  mat_copy_matrix_i3( relative_lattice1[count1], coordinate );
	  count1++;
	} else {
	  if ( det == 2 ) {
	    mat_copy_matrix_i3( relative_lattice2[count2], coordinate );
	    count2++;
	  } else {
	    if ( det == 4 ) {
	      mat_copy_matrix_i3( relative_lattice4[count4], coordinate );
	      count4++;
	    }  
	  }
	}
      }
    }
  }
  /* printf("%d %d %d\n", count1, count2, count4 ); */
}

static int exhaustive_search( double lattice[3][3],
			      SPGCONST double min_lattice[3][3],
			      int (*check_bravais)(SPGCONST double lattice[3][3], const double symprec),
			      const Centering centering,
			      const double symprec )
{
  int i, factor = 1;
  int num_relative_lattice;

  switch (centering) {
  case NO_CENTER:
    num_relative_lattice = 39192;
    factor = 1;
    break;
  case BODY:
  case BASE:
  case A_FACE:
  case B_FACE:
  case C_FACE:
    factor = 2;
    num_relative_lattice = 31848;
    break;
  case FACE:
    factor = 4;
    num_relative_lattice = 20352;
    break;
  }

  for ( i = 0; i < num_relative_lattice; i++ ) {
    if ( factor == 1 ) {
      mat_multiply_matrix_di3( lattice, min_lattice, relative_lattice1[i] );
    }
    if ( factor == 2 ) {
      mat_multiply_matrix_di3( lattice, min_lattice, relative_lattice2[i] );
    }
    if ( factor == 4 ) {
      mat_multiply_matrix_di3( lattice, min_lattice, relative_lattice4[i] );
    }

    if ( mat_Dabs(mat_get_determinant_d3(lattice)) > symprec ) {
      if ((*check_bravais)(lattice, symprec)) {
	/* printf("%d\n", factor); */
	/* for ( j = 0; j < 3; j++ ) { */
	/*   if ( factor == 1 )  */
	/*     printf("%d %d %d\n", */
	/* 	   relative_lattice1[i][j][0], */
	/* 	   relative_lattice1[i][j][1], */
	/* 	   relative_lattice1[i][j][2]); */
	/*   if ( factor == 2 )  */
	/*     printf("%d %d %d\n", */
	/* 	   relative_lattice2[i][j][0], */
	/* 	   relative_lattice2[i][j][1], */
	/* 	   relative_lattice2[i][j][2]); */
	/*   if ( factor == 4 )  */
	/*     printf("%d %d %d\n", */
	/* 	   relative_lattice4[i][j][0], */
	/* 	   relative_lattice4[i][j][1], */
	/* 	   relative_lattice4[i][j][2]); */
	/* } */
	return 1;
      }
    }
  }

  return 0;
}

static Centering get_base_center( SPGCONST double brv_lattice[3][3],
				  SPGCONST double min_lattice[3][3],
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

  debug_print("get_base_center\n");
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

static void get_right_hand_lattice( double lattice[3][3],
				    const double symprec )
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
static void get_Delaunay_reduction( double red_lattice[3][3], 
				    SPGCONST double lattice[3][3],
				    const double symprec )
{
  int i, j;
  double basis[4][3];

  get_exteneded_basis(basis, lattice);

  while (1) {
    if (get_Delaunay_reduction_basis(basis, symprec)) {
      break;
    }
  }

  for ( i = 0; i < 3; i++ ) 
    for ( j = 0; j < 3; j++ )
      red_lattice[i][j] = basis[j][i];

  debug_print("Delaunay reduction\n");
  debug_print_matrix_d3(red_lattice);
#ifdef DEBUG
  double metric[3][3];
  get_metric(metric, red_lattice);
  debug_print("Metric of Delaunay reduction\n");
  debug_print_matrix_d3(metric);
#endif

}

static int get_Delaunay_reduction_basis( double basis[4][3],
					 double symprec )
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

static void get_exteneded_basis( double basis[4][3],
				 SPGCONST double lattice[3][3] )
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

