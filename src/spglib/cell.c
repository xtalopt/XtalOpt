/* cell.c */
/* Copyright (C) 2008 Atsushi Togo */

#include <stdlib.h>
#include <stdio.h>
#include "cell.h"
#include "mathfunc.h"

Cell cel_new_cell( const int size )
{
    Cell cell;
    int i, j;
    
    for ( i = 0; i < 3; i++ )
      for ( j = 0; j < 3; j++ )
	cell.lattice[i][j] = 0;
    
    cell.size = size;
    
    if ( size > 0 ) {
      if ((cell.types = (int *) malloc(sizeof(int) * size)) == NULL) {
        fprintf(stderr, "spglib: Memory of cell could not be allocated.");
        exit(1);
      }
      if ((cell.position =
	   (double (*)[3]) malloc(sizeof(double[3]) * size)) == NULL) {
        fprintf(stderr, "spglib: Memory of cell could not be allocated.");
        exit(1);
      }
    }

    return cell;
}

void cel_delete_cell( Cell * cell )
{
  if ( cell->size > 0 ) {
    free(cell->position);
    free(cell->types);
  }
}

void cel_set_cell( Cell * cell,
		   SPGCONST double lattice[3][3],
		   SPGCONST double position[][3],
		   const int types[] )
{
    int i, j;
    mat_copy_matrix_d3(cell->lattice, lattice);
    for (i = 0; i < cell->size; i++) {
        for (j = 0; j < 3; j++)
            cell->position[i][j] = position[i][j];
        cell->types[i] = types[i];
    }
}

/*
 * Convert a vector from fractional coordinates to cartesian coordinates
 */
void cel_frac_to_cart( Cell * cell,
		       const double frac[3],
		       double cart[3] )
{
  double v[3];
  mat_multiply_matrix_vector_d3(v, cell->lattice, frac);
  mat_copy_vector_d3(cart, v);
}

/*
 * Convert a vector from cartesian coordinates to fractional coordinates
 */
void cel_cart_to_frac( Cell * cell,
		       const double cart[3],
		       double frac[3],
		       const double precision )
{
  double v[3];
  double fracMat[3][3];
  mat_inverse_matrix_d3(fracMat, cell->lattice, precision);
  mat_multiply_matrix_vector_d3(v, fracMat, cart);
  mat_copy_vector_d3(frac, v);
}
