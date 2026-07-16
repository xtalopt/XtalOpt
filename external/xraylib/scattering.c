/*
Copyright (c) 2009, Bruno Golosio, Antonio Brunetti, Manuel Sanchez del Rio,
Tom Schoonjans and Teemu Ikonen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of the contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY Bruno Golosio, Antonio Brunetti, Manuel Sanchez
del Rio, Tom Schoonjans and Teemu Ikonen ''AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL Bruno Golosio, Antonio Brunetti, Manuel Sanchez del Rio, Tom
Schoonjans and Teemu Ikonen BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

This file is a reduced subset derived from upstream xraylib 4.2.1. Only
FF_Rayl() and MomentTransf() are kept for XtalOpt XRD support.
*/

#include "config.h"

#include <math.h>

#include "splint.h"
#include "xrayglob.h"
#include "xraylib-error-private.h"

double FF_Rayl(int Z, double q, xrl_error **error)
{
  double FF;
  int splint_rv;

  if (Z < 1 || Z > ZMAX || Nq_Rayl[Z] <= 0) {
    xrl_set_error_literal(error, XRL_ERROR_INVALID_ARGUMENT, Z_OUT_OF_RANGE);
    return 0.0;
  }

  if (q == 0.0)
    return Z;

  if (q < 0.0) {
    xrl_set_error_literal(error, XRL_ERROR_INVALID_ARGUMENT, NEGATIVE_Q);
    return 0.0;
  }

  splint_rv = splint(q_Rayl_arr[Z] - 1, FF_Rayl_arr[Z] - 1, FF_Rayl_arr2[Z] - 1,
                     Nq_Rayl[Z], q, &FF, error);

  if (!splint_rv)
    return 0.0;

  return FF;
}

double MomentTransf(double E, double theta, xrl_error **error)
{
  if (E <= 0.0) {
    xrl_set_error_literal(error, XRL_ERROR_INVALID_ARGUMENT, NEGATIVE_ENERGY);
    return 0.0;
  }

  return E / KEV2ANGST * sin(theta / 2.0);
}
