/* Copyright (C) 2010 Atsushi Togo */
/* All rights reserved. */

/* This file is part of spglib. */

/* Redistribution and use in source and binary forms, with or without */
/* modification, are permitted provided that the following conditions */
/* are met: */

/* * Redistributions of source code must retain the above copyright */
/*   notice, this list of conditions and the following disclaimer. */

/* * Redistributions in binary form must reproduce the above copyright */
/*   notice, this list of conditions and the following disclaimer in */
/*   the documentation and/or other materials provided with the */
/*   distribution. */

/* * Neither the name of the spglib project nor the names of its */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission. */

/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS */
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS */
/* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE */
/* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, */
/* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; */
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER */
/* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT */
/* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN */
/* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE */
/* POSSIBILITY OF SUCH DAMAGE. */

#ifndef __delaunay_H__
#define __delaunay_H__

#include "base.h"
#include "mathfunc.h"

SPG_API_TEST int del_delaunay_reduce(double lattice_new[3][3],
                                     double const lattice[3][3],
                                     double const symprec);
SPG_API_TEST int del_layer_delaunay_reduce(double min_lattice[3][3],
                                           double const lattice[3][3],
                                           int const aperiodic_axis,
                                           double const symprec);

// @brief Delaunay reduction for monoclinic/oblique or monoclinic/rectangular
// @param[out] red_lattice
// @param[in] lattice
// @param[in] unique_axis
//            Two-fold axis or mirror-plane-perpendicular axis
// @param[in] aperiodic_axis
// @param[in] symprec
// @note For Monoclinic/oblique, the unique axis is also the aperiodic axis.
//       Axes are {j, k, unique_axis(=aperiodic_axis)}.
//       For Monoclinic/rectangular, axes are {unique_axis, j,
//       k(=aperiodic_axis)}. j and k are delaunay reduced, which can be
//       incomplete for Monoclinic/Rectangular
int del_layer_delaunay_reduce_2D(double min_lattice[3][3],
                                 double const lattice[3][3],
                                 int const unique_axis,
                                 int const aperiodic_axis,
                                 double const symprec);

#endif
