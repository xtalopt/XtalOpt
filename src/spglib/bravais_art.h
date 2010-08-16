/* bravais_art.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __bravais_art_H__
#define __bravais_art_H__

#include "bravais.h"
#include "cell.h"
#include "symmetry.h"

int art_get_artificial_bravais(Bravais *bravais, const Symmetry *symmetry,
			       const Cell *cell, const Holohedry holohedry,
			       const double symprec);

#endif
