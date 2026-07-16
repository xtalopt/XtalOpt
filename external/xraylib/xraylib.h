/*
Copyright (c) 2009-2021, Bruno Golosio, Antonio Brunetti,
Manuel Sanchez del Rio, Tom Schoonjans and Teemu Ikonen
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

This header is a reduced interface derived from upstream xraylib 4.2.1 and
vendored for XtalOpt.
*/
#ifndef XTALOPT_XRAYLIB_H
#define XTALOPT_XRAYLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#ifndef XRL_EXTERN
#define XRL_EXTERN extern
#endif

#define KEV2ANGST 12.39841930
#define ZMAX 120

#include "xraylib-error.h"

XRL_EXTERN double FF_Rayl(int Z, double q, xrl_error **error);
XRL_EXTERN double MomentTransf(double E, double theta, xrl_error **error);

#ifdef __cplusplus
}
#endif

#endif
