/* hall_symbol.c */
/* Copyright (C) 2010 Atsushi Togo */

#include "spg_database.h"
#include "symmetry.h"

int main();
static int test_spacegroup_type( void );
static int test_spacegroup_operation( void );

int main() {
  test_spacegroup_operation();
}

int test_spacegroup_operation( void ) {
  int i, j, k;
  Symmetry symmetry;

  for ( i = 0; i < 531; i++ ) {
    symmetry = tbl_get_spg_operation( i );
    for ( j = 0; j < symmetry.size; j++ ) {
      printf("---%d,%d---\n", i, j+1);
      for ( k = 0; k < 3; k++ ) {
	printf("%2d %2d %2d\n",
	       symmetry.rot[j][k][0],
	       symmetry.rot[j][k][1],
	       symmetry.rot[j][k][2]);
      }
      printf("%8.5f %8.5f %8.5f\n", 
	     symmetry.trans[j][0],
	     symmetry.trans[j][1],
	     symmetry.trans[j][2]);
    }
    sym_delete_symmetry( &symmetry );
  }
  return 1;
}

int test_spacegroup_type( void ) {
  SpacegroupType spacegroup;
  int i;

  for ( i = 0; i < 531; i++ ) {
    spacegroup = tbl_get_spg_type( i );
    printf("%d %s %s %s\n",
	   spacegroup.spacegroup_number,
	   spacegroup.schoenflies,
	   spacegroup.hall_symbol,
	   spacegroup.international,
	   spacegroup.international_full);
  }
  return 1;
}

