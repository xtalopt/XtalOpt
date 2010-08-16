/* spacegroup_data.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __spacegroup_data_H__
#define __spacegroup_data_H__

#include "bravais.h"
#include "symmetry.h"

typedef struct {
    int class_table[32];
    int spacegroup;
} SpacegroupData;

int tbl_get_spacegroup_data(const Symmetry * symmetry, const Bravais * bravais,
			const int rot_class[], const double symprec);


#endif
