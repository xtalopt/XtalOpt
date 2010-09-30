/* cell.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __cell_H__
#define __cell_H__

#include "mathfunc.h"

typedef struct {
    int size;
    double lattice[3][3];
    int *types;
    double (*position)[3];
} Cell;

Cell cel_new_cell( const int size );
void cel_delete_cell( Cell * cell );
void cel_set_cell( Cell * cell,
		   SPGCONST double lattice[3][3],
		   SPGCONST double position[][3],
		   const int types[] );
void cel_frac_to_cart( Cell * cell,
		       const double frac[3],
		       double cart[3] );
void cel_cart_to_frac( Cell * cell,
		       const double cart[3],
		       double frac[3],
		       const double precision );

#endif
