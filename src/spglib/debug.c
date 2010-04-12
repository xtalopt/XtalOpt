/* debug.c */
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

#ifdef DEBUG
#include <stdio.h>
#include "bravais.h"
#include "debug.h"

void dbg_print_matrix_d3(const double a[3][3])
{
    int i;
    for (i = 0; i < 3; i++) {
        printf("%f %f %f\n", a[i][0], a[i][1], a[i][2]);
    }
}

void dbg_print_matrix_i3(const int a[3][3])
{
    int i;
    for (i = 0; i < 3; i++) {
        printf("%d %d %d\n", a[i][0], a[i][1], a[i][2]);
    }
}

void dbg_print_vectors_d3(const double a[][3], const int size)
{
    int i;
    for (i = 0; i < size; i++) {
        printf("%d: %f %f %f\n", i + 1, a[i][0], a[i][1], a[i][2]);
    }
}

void dbg_print_vectors_with_label(const double a[][3], const int b[], const int size)
{
    int i;
    for (i = 0; i < size; i++) {
        printf("%d: %f %f %f\n", b[i], a[i][0], a[i][1], a[i][2]);
    }
}

void dbg_print_holohedry(const Bravais *bravais)
{
  switch (bravais->centering) {
  case NO_CENTER:
    break;
  case BASE:
    printf("Bace center ");
    break;
  case BODY:
    printf("Body center ");
    break;
  case FACE:
    printf("Face center ");
    break;
  case A_FACE:
    printf("A center ");
    break;
  case B_FACE:
    printf("B center ");
    break;
  case C_FACE:
    printf("C center ");
    break;
  }

  switch (bravais->holohedry) {
  case CUBIC:
    printf("Cubic\n");
    break;
  case HEXA:
    printf("Hexagonal\n");
    break;
  case TRIGO:
    printf("Trigonal\n");
    break;
  case RHOMB:
    printf("Rhombohedral\n");
    break;
  case TETRA:
    printf("Tetragonal\n");
    break;
  case ORTHO:
    printf("Orthogonal\n");
    break;
  case MONOCLI:
    printf("Monoclinic\n");
    break;
  case TRICLI:
    printf("Triclinic\n");
    break;
  }
}

#endif
