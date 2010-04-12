/* symmetry_kpoints.c */
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

#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "mathfunc.h"
#include "symmetry.h"

static PointSymmetry get_point_group_rotation(const double lattice[3][3], 
					      const Symmetry * symmetry,
					      const int is_time_reversal,
					      const double symprec,
					      const int num_q,
					      const double q[][3]);
static int get_ir_kpoints(int map[], const double kpoints[][3], const int num_kpoint,
			  const PointSymmetry * point_symmetry,
			  const double symprec);
static int get_ir_reciprocal_mesh(int grid_point[][3], int map[],
				  const int mesh[3], const int is_shift[3],
				  const PointSymmetry * point_symmetry);
static int get_ir_triplets( int triplets[][3],
			    int weight_triplets[],
			    int grid[][3],
			    const int num_triplets,
			    const int mesh[3],
			    const int is_time_reversal,
			    const double lattice[3][3],
			    const Symmetry *symmetry,
			    const double symprec );
static void address_to_grid( int grid_double[3], const int address,
			     const int mesh[3], const int is_shift[3] );
static void get_grid_points(int grid_point[3], const int grid[3], const int mesh[3]);
static void get_vector_modulo(int v[3], const int m[3]);
static int grid_to_address(const int grid[3], const int mesh[3], const int is_shift[3]);
static int check_input_values(const int num_kpoint, const int mesh[3]);



int kpt_get_irreducible_kpoints(int map[],
				const double kpoints[][3],
				const int num_kpoint,
				const double lattice[3][3],
				const Symmetry * symmetry,
				const int is_time_reversal,
				const double symprec)
{
  PointSymmetry point_symmetry;

  point_symmetry = get_point_group_rotation( lattice,
					     symmetry,
					     is_time_reversal,
					     symprec,
					     0, NULL );

  return get_ir_kpoints(map, kpoints, num_kpoint, &point_symmetry, symprec);
}

/* grid_point (e.g. 4x4x4 mesh)                               */
/*    [[ 0  0  0]                                             */
/*     [ 1  0  0]                                             */
/*     [ 2  0  0]                                             */
/*     [-1  0  0]                                             */
/*     [ 0  1  0]                                             */
/*     [ 1  1  0]                                             */
/*     [ 2  1  0]                                             */
/*     [-1  1  0]                                             */
/*     ....      ]                                            */
/*                                                            */
/* Each value of 'map' correspnds to the index of grid_point. */
int kpt_get_irreducible_reciprocal_mesh(int grid_point[][3],
					int map[],
					const int num_grid,
					const int mesh[3],
					const int is_shift[3],
					const int is_time_reversal,
					const double lattice[3][3],
					const Symmetry * symmetry,
					const double symprec)
{
  PointSymmetry point_symmetry;

  if (! check_input_values( num_grid, mesh ) )
    return 0;

  point_symmetry = get_point_group_rotation( lattice,
					     symmetry,
					     is_time_reversal,
					     symprec,
					     0, NULL );

  return get_ir_reciprocal_mesh(grid_point, map, mesh, is_shift, &point_symmetry);
}

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
					const double symprec )
{
  PointSymmetry point_symmetry;
  int n_ir, i;
  
  if (! check_input_values( num_grid, mesh ) )
    return 0;

  point_symmetry = get_point_group_rotation( lattice,
					     symmetry,
					     is_time_reversal,
					     symprec,
					     num_q, qpoints );

  return get_ir_reciprocal_mesh(grid_point, map, mesh, is_shift, &point_symmetry);
}

int kpt_get_triplets_reciprocal_mesh( int triplets[][3],
				      int weight_triplets[],
				      int grid_point[][3],
				      const int num_triplets,
				      const int num_grid,
				      const int mesh[3],
				      const int is_time_reversal,
				      const double lattice[3][3],
				      const Symmetry * symmetry,
				      const double symprec )
{
  int i;

  if (! check_input_values( num_grid, mesh ) )
    return 0;

  return get_ir_triplets( triplets,
			  weight_triplets,
			  grid_point,
			  num_triplets,
			  mesh,
			  is_time_reversal,
			  lattice,
			  symmetry,
			  symprec );
}

/* qpoints are used to find stabilizers (operations). */
/* num_q is the number of the qpoints. */
static PointSymmetry get_point_group_rotation(const double lattice[3][3], 
					      const Symmetry * symmetry,
					      const int is_time_reversal,
					      const double symprec,
					      const int num_q,
					      const double qpoints[][3])
{
  int i, j, k, l, is_found, count = 0;
  double volume;
  double rot_d[3][3], lat_inv[3][3], glat[3][3], tmp_mat[3][3], grot_d[3][3];
  double vec[3], diff[3];
  double rotations[symmetry->size*2][3][3];
  PointSymmetry point_symmetry;
  const int time_reversal_rotation[3][3] = {
    {-1, 0, 0 },
    { 0,-1, 0 },
    { 0, 0,-1 }
  };


  volume = mat_get_determinant_d3(lattice);

  mat_inverse_matrix_d3(lat_inv, lattice, symprec);
  mat_transpose_matrix_d3(glat, lat_inv);
  mat_multiply_matrix_d3(tmp_mat, lat_inv, glat);
  
  for ( i = 0; i < symmetry->size; i++ ) {
    mat_cast_matrix_3i_to_3d(rot_d, symmetry->rot[i]);
    mat_get_similar_matrix_d3(grot_d, rot_d, tmp_mat, symprec / volume / volume);
    mat_cast_matrix_3d_to_3i(rotations[i], grot_d);
    mat_multiply_matrix_i3(rotations[symmetry->size+i], time_reversal_rotation,
			   rotations[i]);
  }

  for ( i = 0; i < symmetry->size * (1 + (is_time_reversal != 0)); i++ ) {
    is_found = 1;

    for ( j = 0; j < count; j++ ) {
      if (mat_check_identity_matrix_i3(point_symmetry.rot[j], rotations[i])) {
	is_found = 0;
	break;
      }
    }

    if ( is_found ) {
      for ( k = 0; k < num_q; k++ ) { /* Loop to find stabilizers */
	is_found = 0;
	mat_multiply_matrix_vector_id3( vec, rotations[i], qpoints[k] );

	for ( l = 0; l < num_q; l++ ) {
	  diff[0] = vec[0] - qpoints[l][0];
	  diff[1] = vec[1] - qpoints[l][1];
	  diff[2] = vec[2] - qpoints[l][2];
	
	  if ( mat_Dabs( diff[0] - mat_Nint( diff[0] ) ) < symprec &&
	       mat_Dabs( diff[1] - mat_Nint( diff[1] ) ) < symprec &&
	       mat_Dabs( diff[2] - mat_Nint( diff[2] ) ) < symprec ) {
	    is_found = 1;
	    break;
	  }
	}
	if ( is_found == 0 ) {
	  break;
	}
      }
      
      if ( is_found ) {
	mat_copy_matrix_i3(point_symmetry.rot[count], rotations[i]);
	count++;
      }
    }
  }

  point_symmetry.size = count;

  return point_symmetry;
}

static int get_ir_kpoints(int map[], const double kpoints[][3], const int num_kpoint,
		  const PointSymmetry * point_symmetry, const double symprec)
{
  int i, j, k, l, num_ir_kpoint = 0, is_found;
  int ir_map[num_kpoint];
  double ir_kpoint[num_kpoint][3];
  double kpt_rot[3], diff[3];

  for ( i = 0; i < num_kpoint; i++ ) {

    map[i] = i;

    is_found = 1;

    for ( j = 0; j < point_symmetry->size; j++ ) {
      mat_multiply_matrix_vector_id3(kpt_rot, point_symmetry->rot[j], kpoints[i]);

      for ( k = 0; k < 3; k++ ) {
	diff[k] = kpt_rot[k] - kpoints[i][k];
	diff[k] = diff[k] - mat_Nint(diff[k]);
      }

      if ( mat_Dabs(diff[0]) < symprec && 
	   mat_Dabs(diff[1]) < symprec && 
	   mat_Dabs(diff[2]) < symprec ) {
	continue;
      }
      
      for ( k = 0; k < num_ir_kpoint; k++ ) {
	mat_multiply_matrix_vector_id3(kpt_rot, point_symmetry->rot[j], kpoints[i]);

	for ( l = 0; l < 3; l++ ) {
	  diff[l] = kpt_rot[l] - ir_kpoint[k][l];
	  diff[l] = diff[l] - mat_Nint(diff[l]);
	}

	if ( mat_Dabs(diff[0]) < symprec && 
	     mat_Dabs(diff[1]) < symprec && 
	     mat_Dabs(diff[2]) < symprec ) {
	  is_found = 0;
	  map[i] = ir_map[k];
	  break;
	}
      }

      if ( ! is_found )
	break;
    }

    if ( is_found ) {
      mat_copy_vector_d3(ir_kpoint[num_ir_kpoint], kpoints[i]);
      ir_map[num_ir_kpoint] = i;
      num_ir_kpoint++;
    }
  }

  return num_ir_kpoint;
}

static int get_ir_reciprocal_mesh(int grid[][3], int map[],
				  const int mesh[3], const int is_shift[3],
				  const PointSymmetry * point_symmetry)
{
  /* In the following loop, mesh is doubled. */
  /* Even and odd mesh numbers correspond to */
  /* is_shift[i] = 0 and 1, respectively. */
  /* is_shift = [0,0,0] gives Gamma center mesh. */
  /* grid: reducible grid points */
  /* map: the mapping from each point to ir-point. */
  int i, j, k, l, address, address_rot, num_ir = 0;
  int grid_double[3], grid_rot[3], mesh_double[3];

  for ( i = 0; i < 3; i++ )
    mesh_double[i] = mesh[i] * 2;

  /* "-1" means the element is not touched yet. */
  for ( i = 0; i < mesh[0] * mesh[1] * mesh[2]; i++ ) {
    map[i] = -1;
  }

  for ( i = 0; i < mesh_double[2]; i++ ) {
    if ( ( is_shift[2] && i % 2 == 0 ) ||
	 ( is_shift[2] == 0 && i % 2 != 0 ) ) 
      continue;

    for ( j = 0; j < mesh_double[1]; j++ ) {
      if ( ( is_shift[1] && j % 2 == 0 ) ||
	   ( is_shift[1] == 0 && j % 2 != 0 ) ) 
	continue;
      
      for ( k = 0; k < mesh_double[0]; k++ ) {
	if ( ( is_shift[0] && k % 2 == 0 ) ||
	     ( is_shift[0] == 0 && k % 2 != 0 ) ) 
	  continue;

	grid_double[0] = k;
	grid_double[1] = j;
	grid_double[2] = i;
	
	address = grid_to_address( grid_double, mesh, is_shift );
	get_grid_points(grid[ address ], grid_double, mesh);

	for ( l = 0; l < point_symmetry->size; l++ ) {

	  mat_multiply_matrix_vector_i3( grid_rot, point_symmetry->rot[l], grid_double );
	  get_vector_modulo(grid_rot, mesh_double);
	  address_rot = grid_to_address( grid_rot, mesh, is_shift );

	  if ( address_rot > -1 ) { /* Invalid if even --> odd or odd --> even */
	    if ( map[ address_rot ] > -1 ) {
	      map[ address ] = map[ address_rot ];
	      break;
	    }
	  }
	}

	/* Set itself to the map when equivalent point */
	/* with smaller numbering could not be found. */
	if ( map[ address ] == -1 ) {
	  map[ address ] = address;
	  num_ir++;
	}
      }
    }
  }

  return num_ir;
}

/* Unique q-point triplets that conserve the momentum, */
/* q+q'+q''=G, are obtained. */
static int get_ir_triplets( int triplets[][3],
			    int weight_triplets[],
			    int grid[][3],
			    const int max_num_triplets,
			    const int mesh[3],
			    const int is_time_reversal,
			    const double lattice[3][3],
			    const Symmetry *symmetry,
			    const double symprec )
{
  int i, j, l, m, num_ir, is_found, weight, weight_q, num_triplets=0, num_unique_q=0;
  int mesh_double[3], address[3], tmp_address[3], tmp_grid_double[3], is_shift[3];
  int grid_double[3][3], grid_rot[3][3], tmp_grid_rot[3];
  const int num_mesh = mesh[0] * mesh[1] * mesh[2];
  int map[num_mesh], map_q[num_mesh], map_sym[symmetry->size][num_mesh], unique_q[num_mesh];
  double q[3];
  PointSymmetry point_symmetry, point_symmetry_q;
  
  point_symmetry = get_point_group_rotation( lattice,
					     symmetry,
					     is_time_reversal,
					     symprec,
					     0, NULL );

  for ( i = 0; i < 3; i++ )
    is_shift[i] = 0;

  num_ir = get_ir_reciprocal_mesh( grid, map, mesh, is_shift, &point_symmetry );

  int map_triplets[num_ir][num_mesh];

  for ( i = 0; i < 3; i++ )
    mesh_double[i] = mesh[i] * 2;

  /* Memory space check */
  if ( num_ir * num_mesh < max_num_triplets ) {
    fprintf(stderr, "spglib: More memory space for triplets is required.");
    return 0;
  }

  /* Prepare triplet mapping table to enhance speed of query */
  /* 'unique_q' table is prepared for saving memory space */
  for ( i = 0; i < num_mesh; i++ ) {
    if ( i == map[i] ) {
      unique_q[i] = num_unique_q;
      num_unique_q++;
    } 
    else {
      unique_q[i] = unique_q[map[i]];
    }
  }

  for ( i = 0; i < num_ir; i++ )
    for ( j = 0; j < num_mesh; j++ )
      map_triplets[i][j] = 0;

  /* Prepare grid point mapping table */
  for ( i = 0; i < point_symmetry.size; i++ ) {
    for ( j = 0; j < num_mesh; j++ ) {
      address_to_grid( tmp_grid_double, j, mesh, is_shift );
      mat_multiply_matrix_vector_i3( tmp_grid_rot, point_symmetry.rot[i], tmp_grid_double );
      get_vector_modulo( tmp_grid_rot, mesh_double );
      map_sym[i][j] = grid_to_address( tmp_grid_rot, mesh, is_shift );
    }
  }

  /* Search triplets without considersing combination */
  for ( i = 0; i < num_mesh; i++ ) {
    if ( i != map[ i ] )
      continue;

    weight = 0;
    for ( j = 0; j < num_mesh; j++ ) {
/*       printf("%d: %d ( %d, %d, %d)\n", j, map[j], grid[j][0],grid[j][1],grid[j][2]); */
      if ( i == map[j] )
	weight++;
    }

    printf("%d/%d: weight = %d\n", map[i]+1, num_mesh, weight);

    address_to_grid( grid_double[0], i, mesh, is_shift );
    for ( j = 0; j < 3; j++ )
      q[j] = (double)grid_double[0][j] / mesh_double[j];

    point_symmetry_q = get_point_group_rotation( lattice,
						 symmetry,
						 is_time_reversal,
						 symprec,
						 1, q );

    get_ir_reciprocal_mesh(grid, map_q, mesh, is_shift, &point_symmetry_q);

    for ( j = 0; j < num_mesh; j++ ) {
      if ( j != map_q[ j ] )
	continue;

      weight_q = 0;
      for ( l = 0; l < num_mesh; l++ )
	if ( j == map_q[l] )
	  weight_q++;

      address_to_grid( grid_double[1], j, mesh, is_shift );

      grid_double[2][0] = - grid_double[0][0] - grid_double[1][0];
      grid_double[2][1] = - grid_double[0][1] - grid_double[1][1];
      grid_double[2][2] = - grid_double[0][2] - grid_double[1][2];
      get_vector_modulo( grid_double[2], mesh_double );

      is_found = 0;

      for ( l = 0; l < point_symmetry.size; l++ ) {

	for ( m = 0; m < 3; m++ ) {

	  address[m%3] = map_sym[l][i];
	  address[(m+1)%3] = map_sym[l][j];
	  address[(m+2)%3] = map_sym[l][grid_to_address( grid_double[2], mesh, is_shift )];
	
	  if ( address[0] == map[address[0]] ) {
	    if ( map_triplets[ unique_q[address[0]] ][ address[1] ] ) {
	      is_found = 1;
	      map_triplets[ unique_q[address[0]] ][ address[1] ] += weight * weight_q;
	      break;
	    }
	  }

	  if ( address[1] == map[address[1]] ) {
	    if ( map_triplets[ unique_q[address[1]] ][ address[0] ] ) {
	      is_found = 1;
	      map_triplets[ unique_q[address[1]] ][ address[0] ] += weight * weight_q;
	      break;
	    }
	  }
	}
	
	if ( is_found )
	  break;
      }

      if (! is_found ) {
	triplets[num_triplets][0] = i;
	triplets[num_triplets][1] = j;
	triplets[num_triplets][2] = grid_to_address( grid_double[2], mesh, is_shift );
	num_triplets++;
	map_triplets[unique_q[i]][j] += weight * weight_q;
      }
    }
  }

  for ( i = 0; i < num_triplets; i++ )
    weight_triplets[i] = map_triplets[ unique_q[triplets[i][0]] ][ triplets[i][1] ];

  return num_triplets;
}

static int grid_to_address(const int grid_double[3], const int mesh[3], const int is_shift[3])
{
  int i, grid[3];

  for ( i = 0; i < 3; i++ ) {
    if ( grid_double[i] % 2 == 0 && (! is_shift[i])  ) {
      grid[i] = grid_double[i] / 2;
    } else {
      if ( grid_double[i] % 2 != 0 && is_shift[i] ) {
	grid[i] = ( grid_double[i] - 1 ) / 2;
      } else {
	return -1;
      }
    }
  }
  
  return grid[2] * mesh[0] * mesh[1] + grid[1] * mesh[0] + grid[0];
}

static void address_to_grid( int grid_double[3], const int address,
			     const int mesh[3], const int is_shift[3] )
{
  int i;
  int grid[3];

  grid[2] = address / ( mesh[0] * mesh[1] );
  grid[1] = ( address - grid[2] * mesh[0] * mesh[1] ) / mesh[0];
  grid[0] = address % mesh[0];

  for ( i = 0; i < 3; i++ ) {
    grid_double[i] = grid[i] * 2 + is_shift[i];
  }
}

static void get_grid_points(int grid[3], const int grid_double[3], const int mesh[3])
{
  int i;

  for ( i = 0; i < 3; i++ ) {
    if ( grid_double[i] % 2 == 0 ) {
      grid[i] = grid_double[i] / 2;
    } else {
      grid[i] = ( grid_double[i] - 1 ) / 2;
    }
    
    grid[i] = grid[i] - mesh[i] * ( grid[i] > mesh[i] / 2 );
  }  
}

static void get_vector_modulo(int v[3], const int m[3])
{
  int i;

  for ( i = 0; i < 3; i++ ) {
    v[i] = v[i] % m[i];

    if ( v[i] < 0 )
      v[i] += m[i];
  }
}

static int check_input_values(const int num_kpoint, const int mesh[3])
{
  if ( num_kpoint < mesh[0] * mesh[1] * mesh[2] ) {
    fprintf(stderr, "spglib: More memory space for grid points is required.");
    return 0;
  }

  if ( mesh[0] < 1 || mesh[1] < 1 || mesh[2] < 1 ) {
    fprintf(stderr, "spglib: Each mesh number has to be positive.");
    return 0;
  }

  return 1;
}

