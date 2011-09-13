/* site_symmetry.c */
/* Copyright (C) 2011 Atsushi Togo */

#include <stdio.h>
#include <stdlib.h>
#include "cell.h"
#include "mathfunc.h"
#include "symmetry.h"
#include "sitesym_database.h"

#include "debug.h"

static VecDBL * get_exact_positions( int * wyckoffs,
				     int * equiv_atoms,
				     SPGCONST Cell * bravais,
				     SPGCONST Symmetry * conv_sym,
				     const int hall_number,
				     const double symprec );
static int get_exact_location( double position[3],
			       SPGCONST Symmetry * conv_sym,
			       SPGCONST double bravais_lattice[3][3],
			       const int hall_number,
			       const double symprec );
static int get_Wyckoff_notation( double position[3],
				 SPGCONST Symmetry * conv_sym,
				 SPGCONST double bravais_lattice[3][3],
				 const int hall_number,
				 const double symprec );


VecDBL * ssm_get_exact_positions( int *wyckoffs,
				  int *equiv_atoms,
				  SPGCONST Cell * bravais,
				  SPGCONST Symmetry * conv_sym,
				  const int hall_number,
				  const double symprec )
{
  return get_exact_positions( wyckoffs,
			      equiv_atoms,
			      bravais,
			      conv_sym,
			      hall_number,
			      symprec );
}

static VecDBL * get_exact_positions( int * wyckoffs,
				     int * equiv_atoms,
				     SPGCONST Cell * bravais,
				     SPGCONST Symmetry * conv_sym,
				     const int hall_number,
				     const double symprec )
{
  int i, j, k, l, num_indep_atoms;
  double pos[3];
  int *indep_atoms;
  VecDBL *positions;

  debug_print("get_symmetrized_positions\n");

  indep_atoms = (int*) malloc( sizeof( int ) * bravais->size );
  positions = mat_alloc_VecDBL( bravais->size );
  num_indep_atoms = 0;

  for ( i = 0; i < bravais->size; i++ ) {
    /* Check if atom_i overlap to an atom already set at the exact position. */
    for ( j = 0; j < num_indep_atoms; j++ ) {
      for ( k = 0; k < conv_sym->size; k++ ) {
	mat_multiply_matrix_vector_id3( pos,
					conv_sym->rot[k],
					positions->vec[ indep_atoms[j] ] );
	for ( l = 0; l < 3; l++ ) {
	  pos[l] += conv_sym->trans[k][l];
	}
	if ( cel_is_overlap( pos,
			     bravais->position[i],
			     bravais->lattice,
			     symprec ) ) {
	  /* Equivalent atom was found. */
	  for ( l = 0; l < 3; l++ ) {
	    pos[l] -= mat_Nint( pos[l] );
	  }
	  mat_copy_vector_d3( positions->vec[i], pos );
	  wyckoffs[i] = wyckoffs[ indep_atoms[j] ];
	  equiv_atoms[i] = indep_atoms[j];
	  goto escape;
	}
      }
    }
    
    /* No equivalent atom was found. */
    indep_atoms[ num_indep_atoms ] = i;
    num_indep_atoms++;
    mat_copy_vector_d3( positions->vec[i], bravais->position[i] );
    wyckoffs[i] = get_exact_location( positions->vec[i],
				      conv_sym,
				      bravais->lattice,
				      hall_number,
				      symprec );
    equiv_atoms[i] = i;
  escape:
    ;
  }

  free( indep_atoms );
  indep_atoms = NULL;

  return positions;
}


/* Site-symmetry is used to determine exact location of an atom */
/* R. W. Grosse-Kunstleve and P. D. Adams */
/* Acta Cryst. (2002). A58, 60-65 */
static int get_exact_location( double position[3],
			       SPGCONST Symmetry * conv_sym,
			       SPGCONST double bravais_lattice[3][3],
			       const int hall_number,
			       const double symprec )
{
  int i, j, k, num_sum;
  double sum_rot[3][3];
  double pos[3], sum_trans[3];

  num_sum = 0;
  for ( i = 0; i < 3; i++ ) {
    sum_trans[i] = 0.0;
    for ( j = 0; j < 3; j++ ) {
      sum_rot[i][j] = 0;
    }
  }
  
  for ( i = 0; i < conv_sym->size; i++ ) {
    mat_multiply_matrix_vector_id3( pos,
				    conv_sym->rot[i],
				    position );
    for ( j = 0; j < 3; j++ ) {
      pos[j] += conv_sym->trans[i][j];
    }

    if ( cel_is_overlap( pos,
			 position,
			 bravais_lattice,
			 symprec ) ) {
      for ( j = 0; j < 3; j++ ) {
	sum_trans[j] += conv_sym->trans[i][j] - 
	  mat_Nint( pos[j] - position[j] );
	for ( k = 0; k < 3; k++ ) {
	  sum_rot[j][k] += conv_sym->rot[i][j][k];
	}
      }
      num_sum++;
    }
  }

  for ( i = 0; i < 3; i++ ) {
    sum_trans[i] /= num_sum;
    for ( j = 0; j < 3; j++ ) {
      sum_rot[i][j] /= num_sum;
    }
  }

  /* (sum_rot|sum_trans) is the special-position operator. */
  /* Elements of sum_rot can be fractional values. */
  mat_multiply_matrix_vector_d3( position,
				 sum_rot,
				 position );

  for ( i = 0; i < 3; i++ ) {
    position[i] += sum_trans[i];
  }

  return get_Wyckoff_notation( position, conv_sym, bravais_lattice, hall_number, symprec );
}

static int get_Wyckoff_notation( double position[3],
				 SPGCONST Symmetry * conv_sym,
				 SPGCONST double bravais_lattice[3][3],
				 const int hall_number,
				 const double symprec )
{
  int i, j, k, num_sum, wyckoff_letter=0;
  int indices_wyc[2], indices_orbit[2];
  int rot[3][3];
  int at_orbit[192];
  double trans[3], orbit[3];
  VecDBL *pos_rot;

  pos_rot = mat_alloc_VecDBL( conv_sym->size );
  for ( i = 0; i < conv_sym->size; i++ ) {
    mat_multiply_matrix_vector_id3( pos_rot->vec[i], conv_sym->rot[i], position );
    for ( j = 0; j < 3; j++ ) {
      pos_rot->vec[i][j] += conv_sym->trans[i][j];
    }
  }

  ssmdb_get_wyckoff_indices( indices_wyc, hall_number );
  for ( i = 0; i < indices_wyc[1]; i++ ) {
    ssmdb_get_orbit_indices( indices_orbit, i + indices_wyc[0] );

    for ( j = 0; j < indices_orbit[1]; j++ ) {
      at_orbit[j] = 0;
      ssmdb_get_orbit( rot, trans, j + indices_orbit[0] );
      mat_multiply_matrix_vector_id3( orbit, rot, position );
      for ( k = 0; k < 3; k++ ) {
	orbit[k] += trans[k];
      }

      for ( k = 0; k < pos_rot->size; k++ ) {
	if ( cel_is_overlap( pos_rot->vec[k],
			     orbit,
			     bravais_lattice,
			     symprec ) ) {
	  at_orbit[j]++;
	}
      }
    }
    
    num_sum = 0;
    for ( j = 0; j < indices_orbit[1]; j++ ) {
      num_sum += at_orbit[j];
    }
    if ( num_sum == conv_sym->size ) {
      /* Database is made reversed order, e.g., gfedcba. */
      /* wyckoff is set 0 1 2 3 4... for a b c d e..., respectively. */
      wyckoff_letter = indices_wyc[1] - i - 1;
      break;
    }
  }

  mat_free_VecDBL( pos_rot );
  return wyckoff_letter;
}

