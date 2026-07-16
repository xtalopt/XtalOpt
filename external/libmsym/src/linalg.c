//
//  linalg.c
//  libmsym
//
//  Created by Marcus Johansson on 12/04/14.
//  Copyright (c) 2014 Marcus Johansson.
//
//  Distributed under the MIT License ( See LICENSE file or copy at http://opensource.org/licenses/MIT )
//

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "linalg.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419716939937510582
#endif

#define SQR(x) ((x)*(x))

static double msymCopySign(double magnitude, double sign){
    return sign < 0.0 ? -fabs(magnitude) : fabs(magnitude);
}

void mleye(int l, double E[3][3]){
    memset(E, 0, sizeof(double) * 3 * 3);
    for(int i = 0;i < l && i < 3;i++){
        E[i][i] = 1.0;
    }
}

void vrotate(double theta, const double v[3], const double axis[3], double vr[3]){
    double m[3][3];
    mrotate(theta, axis, m);
    mvmul(v, m, vr);
}

void vreflect(const double v[3], const double axis[3], double vr[3]){
    double m[3][3];
    mreflect(axis, m);
    mvmul(v, m, vr);
}

void malign(const double v[3], const double axis[3], double m[3][3]){
    double vn[3], axisn[3], cross[3], dot, skew[3][3], across, k;
    vnorm2(v, vn);
    vnorm2(axis, axisn);
    dot = vdot(vn, axisn);
    if(dot >= 1.0){
        mleye(3, m);
    } else if(dot <= -1.0){
        vcomplement(axis, cross);
        mrotate(M_PI, cross, m);
    } else {
        vcross(vn, axisn, cross);
        across = vabs(cross);
        k = (1 - dot) / SQR(across);

        skew[0][0] = 0.0;       skew[0][1] = -cross[2]; skew[0][2] = cross[1];
        skew[1][0] = cross[2];  skew[1][1] = 0.0;       skew[1][2] = -cross[0];
        skew[2][0] = -cross[1]; skew[2][1] = cross[0];  skew[2][2] = 0.0;

        mleye(3, m);
        madd(m, skew, m);
        mmmul(skew, skew, skew);
        mscale(k, skew, skew);
        madd(m, skew, m);
    }
}

void mrotate(double theta, const double axis[3], double m[3][3]){
    double c = cos(theta);
    double s = sin(theta);

    m[0][0] = c + (1 - c) * SQR(axis[0]);
    m[0][1] = (1 - c) * axis[0] * axis[1] - s * axis[2];
    m[0][2] = (1 - c) * axis[0] * axis[2] + s * axis[1];
    m[1][0] = (1 - c) * axis[0] * axis[1] + s * axis[2];
    m[1][1] = c + (1 - c) * axis[1] * axis[1];
    m[1][2] = (1 - c) * axis[2] * axis[1] - s * axis[0];
    m[2][0] = (1 - c) * axis[0] * axis[2] - s * axis[1];
    m[2][1] = (1 - c) * axis[1] * axis[2] + s * axis[0];
    m[2][2] = c + (1 - c) * axis[2] * axis[2];
}

void mreflect(const double axis[3], double m[3][3]){
    m[0][0] = 1 - 2 * SQR(axis[0]);
    m[1][1] = 1 - 2 * SQR(axis[1]);
    m[2][2] = 1 - 2 * SQR(axis[2]);
    m[0][1] = m[1][0] = -2 * axis[0] * axis[1];
    m[0][2] = m[2][0] = -2 * axis[0] * axis[2];
    m[1][2] = m[2][1] = -2 * axis[1] * axis[2];
}

int vzero(const double v[3], double t){
    return vabs(v) <= t;
}

int vparallel(const double v1[3], const double v2[3], double t){
    double tv1[3], tv2[3];
    vnorm2(v1, tv1);
    vnorm2(v2, tv2);
    return fabs(fabs(vdot(tv1, tv2)) - 1.0) <= t;
}

int vperpendicular(const double v1[3], const double v2[3], double t){
    double tv1[3], tv2[3];
    vnorm2(v1, tv1);
    vnorm2(v2, tv2);
    return fabs(vdot(tv1, tv2)) <= t;
}

int vequal(const double v1[3], const double v2[3], double t){
    double vs[3], va[3];
    vsub(v1, v2, vs);
    vadd(v1, v2, va);
    return (vabs(vs) <= t && vabs(va) <= t) || (vabs(vs) / vabs(va) <= t);
}

void vproj_plane(double v[3], double plane[3], double proj[3]){
    double vp[3], nplane[3];
    vnorm2(plane, nplane);
    vscale(vdot(v, nplane), nplane, vp);
    vsub(v, vp, proj);
}

void vproj(const double v[3], const double u[3], double vo[3]){
    vscale(vdot(u, v) / vdot(u, u), u, vo);
}

void vcomplement(const double v1[3], double v2[3]){
    double c[2][3] = {
        {v1[2], v1[2], -v1[0] - v1[1]},
        {-v1[1] - v1[2], v1[0], v1[0]}
    };
    int i = ((v1[2] != 0.0) && (-v1[0] != v1[1]));
    vcopy(c[i], v2);
    vnorm(v2);
}

double vangle(const double v1[3], const double v2[3]){
    double c = vdot(v1, v2) / (vabs(v1) * vabs(v2));

    if(c > 1.0) c = 1.0;
    if(c < -1.0) c = -1.0;

    return acos(c);
}

void vcross(const double v1i[3], const double v2i[3], double vr[3]){
    double v1[3], v2[3];
    vcopy(v1i, v1);
    vcopy(v2i, v2);
    vr[0] = v1[1] * v2[2] - v1[2] * v2[1];
    vr[1] = v1[2] * v2[0] - v1[0] * v2[2];
    vr[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

double vcrossnorm(const double v1i[3], const double v2i[3], double vr[3]){
    vcross(v1i, v2i, vr);
    return vnorm(vr);
}

double vdot(const double v1[3], const double v2[3]){
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void vadd(const double v1[3], const double v2[3], double vr[3]){
    vr[0] = v1[0] + v2[0];
    vr[1] = v1[1] + v2[1];
    vr[2] = v1[2] + v2[2];
}

void vscale(double s, const double v[3], double vr[3]){
    vr[0] = s * v[0];
    vr[1] = s * v[1];
    vr[2] = s * v[2];
}

void mscale(double s, const double m[3][3], double mr[3][3]){
    for(int i = 0;i < 3;i++){
        for(int j = 0;j < 3;j++){
            mr[i][j] = s * m[i][j];
        }
    }
}

void vsub(const double v1[3], const double v2[3], double vr[3]){
    vr[0] = v1[0] - v2[0];
    vr[1] = v1[1] - v2[1];
    vr[2] = v1[2] - v2[2];
}

double vabs(const double v[3]){
    return sqrt(SQR(v[0]) + SQR(v[1]) + SQR(v[2]));
}

void vinv(double v[3]){
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}

double vnorm(double v[3]){
    double norm = vabs(v);
    if(norm != 0.0){
        v[0] /= norm;
        v[1] /= norm;
        v[2] /= norm;
    }
    return norm;
}

double vnorm2(const double v1[3], double v2[3]){
    double abs = vabs(v1);
    double norm = 1.0 / (abs + DBL_MIN);
    vscale(norm, v1, v2);
    return abs;
}

void vcopy(const double vi[3], double vo[3]){
    vo[0] = vi[0];
    vo[1] = vi[1];
    vo[2] = vi[2];
}

void mvmul(const double v[3], const double m[3][3], double r[3]){
    double t[3];
    t[0] = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
    t[1] = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
    t[2] = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];
    r[0] = t[0];
    r[1] = t[1];
    r[2] = t[2];
}

void mmmul(const double A[3][3], const double B[3][3], double C[3][3]){
    double T[3][3];
    for(int i = 0;i < 3;i++){
        for(int j = 0;j < 3;j++){
            T[i][j] = 0.0;
            for(int k = 0;k < 3;k++){
                T[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    mcopy(T, C);
}

int mequal(const double A[3][3], const double B[3][3], double t){
    int e = 1;
    for(int i = 0;i < 3;i++){
        for(int j = 0;j < 3;j++){
            e &= (fabs(A[i][j] - B[i][j]) <= t);
        }
    }
    return e;
}

void jacobi(double m[6], double e[3], double ev[3][3], double threshold){
    double err = 1.0;
    e[0] = m[0];
    e[1] = m[3];
    e[2] = m[5];

    mleye(3, ev);

    while(err > 0){
        err = 0.0;
        for(int od = 0;od < 3;od++){
            int i = 1 << od, row = od >> 1, col = 1 + (od >> (od >> 1));
            double ami = fabs(m[i]), eps = ami / threshold;

            if(fabs(e[row]) + eps == fabs(e[row]) && fabs(e[col]) + eps == fabs(e[col])){
                m[i] = 0.0;
            } else if(ami > 0.0){
                double d, t, c, s;
                int ix, iy;
                double mix, miy;
                err = fmax(ami, err);
                d = e[col] - e[row];
                t = msymCopySign(2, d) * m[i] / (fabs(d) + sqrt(SQR(d) + 4 * SQR(m[i])));
                c = 1 / sqrt(1 + SQR(t));
                s = c * t;

                e[row] -= t * m[i];
                e[col] += t * m[i];
                m[i] = 0.0;

                for(int k = 0;k < 3;k++){
                    double evr = ev[k][row], evc = ev[k][col];
                    ev[k][row] = c * evr - s * evc;
                    ev[k][col] = s * evr + c * evc;
                }

                ix = col ^ 3;
                iy = 4 >> row;
                mix = m[ix];
                miy = m[iy];
                m[ix] = c * mix - s * miy;
                m[iy] = s * mix + c * miy;
            }
        }
    }
}

void madd(const double A[3][3], const double B[3][3], double C[3][3]){
    for(int i = 0;i < 3;i++){
        for(int j = 0;j < 3;j++){
            C[i][j] = A[i][j] + B[i][j];
        }
    }
}

void mcopy(const double A[3][3], double B[3][3]){
    for(int i = 0;i < 3;i++){
        for(int j = 0;j < 3;j++){
            B[i][j] = A[i][j];
        }
    }
}

double mdet(const double M[3][3]){
    double d0 = M[1][1] * M[2][2] - M[2][1] * M[1][2];
    double d1 = M[1][0] * M[2][2] - M[1][2] * M[2][0];
    double d2 = M[1][0] * M[2][1] - M[1][1] * M[2][0];
    return M[0][0] * d0 - M[0][1] * d1 + M[0][2] * d2;
}

void minv(const double M[3][3], double I[3][3]){
    double d0 = M[1][1] * M[2][2] - M[2][1] * M[1][2];
    double d1 = M[1][0] * M[2][2] - M[1][2] * M[2][0];
    double d2 = M[1][0] * M[2][1] - M[1][1] * M[2][0];
    double det = M[0][0] * d0 - M[0][1] * d1 + M[0][2] * d2;

    I[0][0] = d0 / det;
    I[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / det;
    I[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / det;
    I[1][0] = -d1 / det;
    I[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / det;
    I[1][2] = (M[1][0] * M[0][2] - M[0][0] * M[1][2]) / det;
    I[2][0] = d2 / det;
    I[2][1] = (M[2][0] * M[0][1] - M[0][0] * M[2][1]) / det;
    I[2][2] = (M[0][0] * M[1][1] - M[1][0] * M[0][1]) / det;
}

void mtranspose(const double A[3][3], double B[3][3]){
    for(int r = 0;r < 3;r++){
        for(int c = 0;c < 3;c++){
            B[c][r] = A[r][c];
        }
    }
}

int ipow(int b, int e){
    int r = 1;
    while(e){
        if(e & 1) r *= b;
        e >>= 1;
        b *= b;
    }
    return r;
}
