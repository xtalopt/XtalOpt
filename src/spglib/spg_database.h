/* spg_database.h */
/* Copyright (C) 2010 Atsushi Togo */

#ifndef __spg_database_H__
#define __spg_database_H__

#include "symmetry.h"

typedef struct {
  int spacegroup_number;
  char schoenflies[7];
  char hall_symbol[17];
  char international[32];
  char international_full[20];
} SpacegroupType;

SpacegroupType tbl_get_spg_type( int hall_number );
Symmetry tbl_get_spg_operation( int hall_number );

#endif
