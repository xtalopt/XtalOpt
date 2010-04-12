/* bravais.h */
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

#ifndef __bravasis_H__
#define __bravasis_H__

typedef enum {
  TRICLI = 1,
  MONOCLI,
  ORTHO,
  TETRA,
  RHOMB,
  TRIGO,
  HEXA,
  CUBIC
} Holohedry;

typedef enum {
  NO_CENTER = 0,
  BODY = -1,
  FACE = -3,
  A_FACE = 1,
  B_FACE = 2,
  C_FACE = 3,
  BASE = 4
} Centering;

typedef struct {
    Holohedry holohedry;
    Centering centering;
    double lattice[3][3];
} Bravais;


Bravais brv_get_brv_lattice(const double lattice_orig[3][3], const double symprec);
int brv_get_brv_lattice_in_loop(Bravais *bravais, const double min_lattice[3][3],
                                const double symprec);
void brv_smallest_lattice_vector(double lattice_new[3][3], const double lattice[3][3],
				 const double symprec);

#endif
