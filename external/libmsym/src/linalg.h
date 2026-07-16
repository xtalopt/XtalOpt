//
//  linalg.h
//  libmsym
//
//  Created by Marcus Johansson on 13/04/14.
//  Copyright (c) 2014 Marcus Johansson.
//
//  Distributed under the MIT License ( See LICENSE file or copy at http://opensource.org/licenses/MIT )
//

#ifndef __MSYM_LINALG_h
#define __MSYM_LINALG_h

void mleye(int l, double E[3][3]);
int vzero(const double v[3], double t);
int vparallel(const double v1[3], const double v2[3], double t);
int vperpendicular(const double v1[3], const double v2[3], double t);
double vnorm(double v[3]);
double vnorm2(const double v1[3], double v2[3]);
double vabs(const double v[3]);
void vinv(double v[3]);
void vcopy(const double vi[3], double vo[3]);
void vcross(const double v1i[3], const double v2i[3], double vr[3]);
double vcrossnorm(const double v1i[3], const double v2i[3], double vr[3]);
double vdot(const double v1[3], const double v2[3]);
int vequal(const double v1[3], const double v2[3], double t);
void vadd(const double v1[3], const double v2[3], double vr[3]);
void madd(const double A[3][3], const double B[3][3], double C[3][3]);
void vsub(const double v1[3], const double v2[3], double vr[3]);
void vscale(double s, const double v[3], double vr[3]);
void mscale(double s, const double m[3][3], double mr[3][3]);
void vproj_plane(double v[3], double plane[3], double proj[3]);
void vproj(const double v[3], const double u[3], double vo[3]);
void vcomplement(const double v1[3], double v2[3]);
double vangle(const double v1[3], const double v2[3]);
void vrotate(double theta, const double v[3], const double axis[3], double vr[3]);
void mrotate(double theta, const double axis[3], double m[3][3]);
void vreflect(const double v[3], const double axis[3], double vr[3]);
void mreflect(const double axis[3], double m[3][3]);
void mvmul(const double v[3], const double m[3][3], double r[3]);
void mmmul(const double A[3][3], const double B[3][3], double C[3][3]);
void minv(const double M[3][3], double I[3][3]);
double mdet(const double M[3][3]);
void mcopy(const double A[3][3], double B[3][3]);
void mtranspose(const double A[3][3], double B[3][3]);
int mequal(const double A[3][3], const double B[3][3], double t);
void malign(const double v[3], const double axis[3], double m[3][3]);
int ipow(int b, int e);
void jacobi(double m[6], double e[3], double ev[3][3], double threshold);

#endif /* defined(__MSYM_LINALG_h) */
