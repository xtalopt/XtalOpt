/* spacegroup_database.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __spacegroup_database_H__
#define __spacegroup_database_H__

#include "spacegroup.h"

Spacegroup tbl_get_spacegroup_database(int spacegroup_number, int axis,
                                   int origin);
Pointgroup tbl_get_pointgroup_database(int spacegroup_number);

#endif
