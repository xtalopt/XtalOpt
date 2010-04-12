/* debug.h */
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

#ifndef __debug_H__
#define __debug_H__

#ifdef DEBUG
#include "bravais.h"
#define debug_print(...) printf(__VA_ARGS__)
#define debug_print_matrix_d3( a ) dbg_print_matrix_d3( a )
#define debug_print_matrix_i3( a ) dbg_print_matrix_i3( a )
#define debug_print_vectors_d3(...) dbg_print_vectors_d3(__VA_ARGS__)
#define debug_print_vectors_with_label(...) dbg_print_vectors_with_label(__VA_ARGS__)
#define debug_print_holohedry( a ) dbg_print_holohedry( a )

void dbg_print_matrix_d3(const double a[3][3]);
void dbg_print_matrix_i3(const int a[3][3]);
void dbg_print_vectors_d3(const double a[][3], const int size);
void dbg_print_vectors_with_label(const double a[][3], const int b[], const int size);
void dbg_print_holohedry(const Bravais *bravais);

#else
#define debug_print(...)
#define debug_print_matrix_d3( a )
#define debug_print_matrix_i3( a )
#define debug_print_vectors_d3(...)
#define debug_print_vectors_with_label(...)
#define debug_print_holohedry(a)
#endif

#endif
