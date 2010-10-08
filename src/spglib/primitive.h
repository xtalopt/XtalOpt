/* primitive.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __primitive_H__
#define __primitive_H__

#include "symmetry.h"
#include "cell.h"
#include "mathfunc.h"

Cell prm_get_primitive( SPGCONST Cell * cell,
			const double symprec );

#endif
