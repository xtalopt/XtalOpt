/* cell.c */
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

#include <stdlib.h>
#include <stdio.h>
#include "cell.h"
#include "mathfunc.h"

Cell cel_new_cell(const int size)
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

void cel_delete_cell(Cell * cell)
{
  if ( cell->size > 0 ) {
    free(cell->position);
    free(cell->types);
  }
}

void cel_set_cell(Cell * cell, const double lattice[3][3], const double position[][3],
              const int types[])
{
    int i, j;
    mat_copy_matrix_d3(cell->lattice, lattice);
    for (i = 0; i < cell->size; i++) {
        for (j = 0; j < 3; j++)
            cell->position[i][j] = position[i][j];
        cell->types[i] = types[i];
    }
}

