/* symmetry.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __symmetry_H__
#define __symmetry_H__

#include "bravais.h"
#include "cell.h"
#include "mathfunc.h"

typedef struct {
  int size;
  int (*rot)[3][3];
  double (*trans)[3];
} Symmetry;

typedef struct {
  int rot[96][3][3];
  int size;
} PointSymmetry;

Symmetry sym_new_symmetry( const int size );
void sym_delete_symmetry( Symmetry * symmetry );
int sym_get_multiplicity( SPGCONST Cell *cell,
			  const double symprec );
Symmetry sym_get_operation( SPGCONST Bravais *bravais,
			    SPGCONST Cell *cell,
			    const double symprec );
VecDBL * sym_get_pure_translation( SPGCONST Cell *cell,
				   const double symprec );
// double sym_get_fractional_translation( double tranlation );

#endif
