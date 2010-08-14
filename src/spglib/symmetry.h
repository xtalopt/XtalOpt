/* symmetry.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __symmetry_H__
#define __symmetry_H__

#include "bravais.h"
#include "cell.h"

typedef struct {
  int size;
  int (*rot)[3][3];
  double (*trans)[3];
} Symmetry;

typedef struct {
  int rot[96][3][3];
  int size;
} PointSymmetry;

Symmetry sym_new_symmetry(const int size);
void sym_delete_symmetry(Symmetry * symmetry);
int sym_get_multiplicity(const Cell *cell, const double symprec);
Symmetry sym_get_operation(const Bravais *bravais, const Cell *cell, const double symprec);
int sym_get_pure_translation(double pure_trans[][3], const Cell *cell, const double symprec);
double sym_get_fractional_translation( double tranlation );

#endif
