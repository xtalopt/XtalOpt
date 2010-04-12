/* symmetry_kpoints.h */
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

#ifndef __symmetry_kpoints_H__
#define __symmetry_kpoints_H__

#include "symmetry.h"

int kpt_get_irreducible_kpoints(int map[],
				const double kpoints[][3], 
				const int num_kpoint,
				const double lattice[3][3],
				const Symmetry * symmetry,
				const int is_time_reversal,
				const double symprec);
int kpt_get_irreducible_reciprocal_mesh(int grid_point[][3],
					int map[],
					const int num_kpoint,
					const int mesh[3],
					const int is_shift[3],
					const int is_time_reversal,
					const double lattice[3][3],
					const Symmetry * symmetry,
					const double symprec);
int kpt_get_stabilized_reciprocal_mesh( int grid_point[][3],
					int map[],
					const int num_grid,
					const int mesh[3],
					const int is_shift[3],
					const int is_time_reversal,
					const double lattice[3][3],
					const Symmetry * symmetry,
					const int num_q,
					const double qpoints[][3],
					const double symprec );
int kpt_get_triplets_reciprocal_mesh( int triplets[][3],
				      int weight_triplets[],
				      int grid_point[][3],
				      const int num_triplets,
				      const int num_grid,
				      const int mesh[3],
				      const int is_time_reversal,
				      const double lattice[3][3],
				      const Symmetry * symmetry,
				      const double symprec );
  
#endif
