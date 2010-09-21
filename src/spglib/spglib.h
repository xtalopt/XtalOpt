/* spglib.h */
/* Copyright (C) 2008 Atsushi Togo */

/*
  ------------------------------------------------------------------

  See usages written in spglib.c.

  ------------------------------------------------------------------
 */

#ifndef __spglib_H__
#define __spglib_H__

int spg_get_symmetry(int ***rotation, double **translation,
		     const int max_size, const double lattice[3][3],
		     const double **position, const int *types, const int num_atom,
		     const double symprec);
int spg_get_conventional_symmetry( double bravais_lattice[3][3], 
				   int ***rotation, double **translation,
				   const int max_size, const double lattice[3][3],
				   const double **position, const int *types,
				   const int num_atom, const double symprec );
int spg_get_multiplicity(const double lattice[3][3], const double **position,
			 const int *types, const int num_atom, const double symprec);
void spg_get_smallest_lattice(double smallest_lattice[3][3], const double lattice[3][3],
			      const double symprec);
int spg_get_max_multiplicity(const double lattice[3][3], const double **position,
			     const int *types, const int num_atom, const double symprec);
int spg_find_primitive(double lattice[3][3], double (*position)[3],
                       int *types, const int num_atom, const double symprec);
void spg_show_symmetry(const double lattice[3][3], const double **position,
                       const int *types, const int num_atom, const double symprec);
int spg_get_international(char symbol[21], const double lattice[3][3],
                          const double (*position)[3], const int *types,
                          const int num_atom, const double symprec);
int spg_get_international_with_bravais(char symbol[21],
				       double lattice[3][3],
				       const double **position,
				       const int *types, const int num_atom,
				       const double symprec);
int spg_get_ir_kpoints(int *map, const double **kpoints,
		       const int num_kpoints,
		       const double lattice[3][3],
		       const double **position,
		       const int *types, const int num_atom,
		       const int is_time_reversal,
		       const double symprec);
int spg_get_ir_reciprocal_mesh(int **grid_point,
			       int *map, const int num_grid,
			       const int mesh[3], const int is_shift[3],
			       const int is_time_reversal,
			       const double lattice[3][3],
			       const double **position,
			       const int *types,
			       const int num_atom, const double symprec);
int spg_get_stabilized_reciprocal_mesh( int **grid_point,
				        int *map,
				        const int num_grid,
				        const int mesh[3],
				        const int is_shift[3],
				        const int is_time_reversal,
				        const double lattice[3][3],
					const int num_rot,
				        const int ***rotations,
				        const int num_q,
				        const double **qpoints,
				        const double symprec );
int spg_get_triplets_reciprocal_mesh( int **triplets,
				      int *weight_triplets,
				      int **grid_point,
				      const int num_triplets,
				      const int num_grid,
				      const int mesh[3],
				      const int is_time_reversal,
				      const double lattice[3][3],
				      const int num_rot,
				      const int ***rotations,
				      const double symprec );
int spg_get_triplets_reciprocal_mesh_with_q( int **triplets_with_q,
					     int *weight_triplets_with_q,
					     const int fixed_grid_number,
					     const int num_triplets,
					     const int **triplets,
					     const int *weight_triplets,
					     const int mesh[3],
					     const int is_time_reversal,
					     const double lattice[3][3],
					     const int num_rot,
					     const int ***rotations,
					     const double symprec );
int spg_get_schoenflies(char symbol[10],
			const double lattice[3][3],
                        const double **position,
			const int *types,
			const int num_atom,
                        const double symprec);

#endif
