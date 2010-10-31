/* pointgroup.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __pointgroup_H__
#define __pointgroup_H__

#include "symmetry.h"
#include "bravais.h"

typedef struct {
    int table[10];
    char symbol[6];
    Holohedry holohedry;
} PointgroupData;

Holohedry ptg_get_holohedry( const Symmetry * symmetry );

#endif
