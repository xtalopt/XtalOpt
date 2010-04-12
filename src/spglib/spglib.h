/* spglib.h */
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

/*
  ------------------------------------------------------------------

  See usages written in spglib.c.

  ------------------------------------------------------------------
 */

#ifndef __spglib_H__
#define __spglib_H__

int spg_get_symmetry(int rotation[][3][3], double translation[][3],
		     const int max_size, const double lattice[3][3],
		     const double position[][3], const int types[], const int num_atom,
		     const double symprec);
int spg_get_multiplicity(const double lattice[3][3], const double position[][3],
			 const int types[], const int num_atom, const double symprec);
void spg_get_bravais_lattice(double bravais_lattice[3][3], const double lattice[3][3],
                             const double symprec);
void spg_get_smallest_lattice(double smallest_lattice[3][3], const double lattice[3][3],
			      const double symprec);
int spg_get_max_multiplicity(const double lattice[3][3], const double position[][3],
			     const int types[], const int num_atom, const double symprec);
int spg_find_primitive(double lattice[3][3], double position[][3],
                       int types[], const int num_atom, const double symprec);
void spg_show_symmetry(const double lattice[3][3], const double position[][3],
                       const int types[], const int num_atom, const double symprec);
int spg_get_international(char symbol[21], const double lattice[3][3],
                          const double position[][3], const int types[],
                          const int num_atom, const double symprec);
int spg_get_ir_kpoints(int map[], const double kpoints[][3],
		       const int num_kpoints,
		       const double lattice[3][3],
		       const double position[][3],
		       const int types[], const int num_atom,
		       const int is_time_reversal,
		       const double symprec);
int spg_get_ir_reciprocal_mesh(int grid_point[][3],
			       int map[], const int num_grid,
			       const int mesh[3], const int is_shift[3],
			       const int is_time_reversal,
			       const double lattice[3][3],
			       const double position[][3],
			       const int types[],
			       const int num_atom, const double symprec);
int spg_get_stabilized_reciprocal_mesh( int grid_point[][3],
				        int map[],
				        const int num_grid,
				        const int mesh[3],
				        const int is_shift[3],
				        const int is_time_reversal,
				        const double lattice[3][3],
					const int num_rot,
				        const int rotations[][3][3],
				        const int num_q,
				        const double qpoints[][3],
				        const double symprec );
int spg_get_triplets_reciprocal_mesh( int triplets[][3],
				      int weight_triplets[],
				      int grid_point[][3],
				      const int num_triplets,
				      const int num_grid,
				      const int mesh[3],
				      const int is_time_reversal,
				      const double lattice[3][3],
				      const int num_rot,
				      const int rotations[][3][3],
				      const double symprec );
int spg_get_schoenflies(char symbol[10], const double lattice[3][3],
                        const double position[][3], const int types[], const int num_atom,
                        const double symprec);

#endif
