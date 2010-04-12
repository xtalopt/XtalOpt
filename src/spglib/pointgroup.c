/* pointgroup.c */
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bravais.h"
#include "debug.h"
#include "pointgroup.h"
#include "symmetry.h"
#include "mathfunc.h"
#include "spacegroup_data.h"

static void get_pointgroup_class_table(int table[10], const Symmetry * symmetry);
static void count_pointgroup_class_table(int table[10], const int order, const int order_inv,
					 const int symmetry[3][3]);
static int check_pointgroup_class_table(const int a[10], const int b[10]);
static PointgroupData get_pointgroup_data(const int num);
static PointgroupData get_pointgroup(const Symmetry * symmetry);

#ifdef DEBUG
static void print_pointgroup_comment(const Holohedry pointgroup_holohedry,
				     const Holohedry holohedry);
static void print_holohedry(const Holohedry holohedry);
#endif


Holohedry ptg_get_holohedry(const Holohedry holohedry, const Symmetry * symmetry)
{
  PointgroupData pointgroup_data;

  pointgroup_data = get_pointgroup(symmetry);

  /* Bravais lattice may be changed by the obtained symmetry operations. */
  debug_print("pointgroup: %s\n", pointgroup_data.symbol);
  debug_print("holohedry in bravais:%d,  in pointgroup_data: %d\n",
	      holohedry, pointgroup_data.holohedry);

#ifdef DEBUG
  if (pointgroup_data.holohedry != holohedry)
    print_pointgroup_comment(pointgroup_data.holohedry, holohedry);
#endif

  return pointgroup_data.holohedry;
}

static PointgroupData get_pointgroup(const Symmetry * symmetry)
{
  int i, table[10];
  PointgroupData pointgroup_data;

  for (i = 0; i < 10; i++)
    table[i] = 0;
  get_pointgroup_class_table(table, symmetry);

#ifdef DEBUG
  printf("*** check_pointgroup ***\n");
  printf("point group data\n");
  for (i = 0; i < 10; i++)
    printf("%2d ", table[i]);
  printf("\n");
#endif

  /* Search point group from 32 groups. */
  for (i = 0; i < 32; i++) {
    pointgroup_data = get_pointgroup_data(i);
    if (check_pointgroup_class_table(table, pointgroup_data.table)) {
      goto end;
    }
  }
  
  fprintf(stderr, "spglib BUG: No point group symbol found\n");

 end:

  return pointgroup_data;
}

static void get_pointgroup_class_table(int table[10], const Symmetry * symmetry)
{
  /* table[0] = -6 axis */
  /* table[1] = -4 axis */
  /* table[2] = -3 axis */
  /* table[3] = -2 axis */
  /* table[4] = -1 axis */
  /* table[5] =  1 axis */
  /* table[6] =  2 axis */
  /* table[7] =  3 axis */
  /* table[8] =  4 axis */
  /* table[9] =  6 axis */
  int i, j, order, order_inv;
  int test_rot[3][3], rot[3][3];
  int identity[3][3] = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1}
  };

  int inversion[3][3] = {
    {-1, 0, 0},
    {0, -1, 0},
    {0, 0, -1}
  };

  for (i = 0; i < symmetry->size; i++) {
    order = 0;
    order_inv = 0;
    mat_copy_matrix_i3(test_rot, identity);
    mat_copy_matrix_i3(rot, symmetry->rot[i]);

    for (j = 0; j < 6; j++) {
      mat_multiply_matrix_i3(test_rot, rot, test_rot);
      if (mat_check_identity_matrix_i3(test_rot, identity)) {
	order = j + 1;
	break;
      }
      if (mat_check_identity_matrix_i3(test_rot, inversion))
	order_inv = j + 1;
    }

    if (!order) {
      fprintf(stderr, "spglib BUG: Invalid rotation matrix\n");
    }

    debug_print("symmetry: %d\n", i+1);
    count_pointgroup_class_table(table, order, order_inv, rot);
  }

}

static int check_pointgroup_class_table(const int a[10], const int b[10])
{
  int i;
  for (i = 0; i < 10; i++)
    if (a[i] - b[i])
      return 0;
  return 1;
}

static void count_pointgroup_class_table(int table[10], const int order,
					 const int order_inv,
					 const int symmetry[3][3])
{
  int det;
  det = mat_get_determinant_i3(symmetry);

  debug_print("order %d, order_inv %d, det %d\n", order, 
	      order_inv, det);

  switch (order) {
  case 1:                    /* 1 */
    table[5]++;
    break;
  case 2:
    if (det == 1)           /* 2 */
      table[6]++;
    if (det == -1 && order_inv == 1)	/* -1 */
      table[4]++;
    if (det == -1 && order_inv == 0)	/* -2 */
      table[3]++;
    break;
  case 3:                    /* 3 */
    table[7]++;
    break;
  case 4:
    if (det == 1)           /* 4 */
      table[8]++;
    if (det == -1)          /* -4 */
      table[1]++;
    break;
  case 6:
    if (det == 1)           /* 6  */
      table[9]++;
    if (det == -1 && order_inv == 3)	/* -3 */
      table[2]++;
    if (det == -1 && order_inv == 0)	/* -6 */
      table[0]++;
    break;
  }
}

static PointgroupData get_pointgroup_data(const int num)
{
  PointgroupData pointgroup_data[32] = {
    {
      {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
      "    1",
      TRICLI
    },
    {
      {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
      "   -1",
      TRICLI
    },
    {
      {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
      "    2",
      MONOCLI
    },
    {
      {0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
      "   -2",
      MONOCLI
    },
    {
      {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
      "  2/m",
      MONOCLI
    },
    {
      {0, 0, 0, 0, 0, 1, 3, 0, 0, 0},
      "  222",
      ORTHO
    },
    {
      {0, 0, 0, 2, 0, 1, 1, 0, 0, 0},
      "  mm2",
      ORTHO
    },
    {
      {0, 0, 0, 3, 1, 1, 3, 0, 0, 0},
      "  mmm",
      ORTHO
    },
    {
      {0, 0, 0, 0, 0, 1, 1, 0, 2, 0},
      "    4",
      TETRA
    },
    {
      {0, 2, 0, 0, 0, 1, 1, 0, 0, 0},
      "   -4",
      TETRA
    },
    {
      {0, 2, 0, 1, 1, 1, 1, 0, 2, 0},
      "  4/m",
      TETRA
    },
    {
      {0, 0, 0, 0, 0, 1, 5, 0, 2, 0},
      "  422",
      TETRA
    },
    {
      {0, 0, 0, 4, 0, 1, 1, 0, 2, 0},
      "  4mm",
      TETRA
    },
    {
      {0, 2, 0, 2, 0, 1, 3, 0, 0, 0},
      " -42m",
      TETRA
    },
    {
      {0, 2, 0, 5, 1, 1, 5, 0, 2, 0},
      "4/mmm",
      TETRA
    },
    {
      {0, 0, 0, 0, 0, 1, 0, 2, 0, 0},
      "    3",
      TRIGO
    },
    {
      {0, 0, 2, 0, 1, 1, 0, 2, 0, 0},
      "   -3",
      TRIGO
    },
    {
      {0, 0, 0, 0, 0, 1, 3, 2, 0, 0},
      "   32",
      TRIGO
    },
    {
      {0, 0, 0, 3, 0, 1, 0, 2, 0, 0},
      "   3m",
      TRIGO
    },
    {
      {0, 0, 2, 3, 1, 1, 3, 2, 0, 0},
      "  -3m",
      TRIGO
    },
    {
      {0, 0, 0, 0, 0, 1, 1, 2, 0, 2},
      "    6",
      HEXA
    },
    {
      {2, 0, 0, 1, 0, 1, 0, 2, 0, 0},
      "   -6",
      HEXA
    },
    {
      {2, 0, 2, 1, 1, 1, 1, 2, 0, 2},
      "  6/m",
      HEXA
    },
    {
      {0, 0, 0, 0, 0, 1, 7, 2, 0, 2},
      "  622",
      HEXA
    },
    {
      {0, 0, 0, 6, 0, 1, 1, 2, 0, 2},
      "  6mm",
      HEXA
    },
    {
      {2, 0, 0, 4, 0, 1, 3, 2, 0, 0},
      " -62m",
      HEXA
    },
    {
      {2, 0, 2, 7, 1, 1, 7, 2, 0, 2},
      "6/mmm",
      HEXA
    },
    {
      {0, 0, 0, 0, 0, 1, 3, 8, 0, 0},
      "   23",
      CUBIC
    },
    {
      {0, 0, 8, 3, 1, 1, 3, 8, 0, 0},
      "  m-3",
      CUBIC
    },
    {
      {0, 0, 0, 0, 0, 1, 9, 8, 6, 0},
      "  432",
      CUBIC
    },
    {
      {0, 6, 0, 6, 0, 1, 3, 8, 0, 0},
      " -43m",
      CUBIC
    },
    {
      {0, 6, 8, 9, 1, 1, 9, 8, 6, 0},
      " m-3m",
      CUBIC
    }
  };
  return pointgroup_data[num];
}

#ifdef DEBUG
static void print_pointgroup_comment(const Holohedry pointgroup_holohedry,
				     const Holohedry holohedry)
{
  printf("Comment:\n");
  printf(" Bravais lattice determined only from input lattice doesn't\n");
  printf(" match Bravais lattice determined from symmetry operations.\n");
  printf(" The Bravais lattice is changed:\n");
  printf("                                *** ");
  print_holohedry(holohedry);
  printf(" --> ");
  print_holohedry(pointgroup_holohedry);
  printf(" ***\n\n");
}

static void print_holohedry(const Holohedry holohedry)
{
  switch (holohedry) {
  case TRICLI:
    debug_print("Triclinic");
    break;
  case MONOCLI:
    debug_print("Monoclinic");
    break;
  case ORTHO:
    debug_print("Orthorhombic");
    break;
  case TETRA:
    debug_print("Tetragonal");
    break;
  case RHOMB:
    debug_print("Rhombohedral");
    break;
  case TRIGO:
    debug_print("Trigonal");
    break;
  case HEXA:
    debug_print("Hexagonal");
    break;
  case CUBIC:
    debug_print("Cubic");
    break;
  }
}

#endif

