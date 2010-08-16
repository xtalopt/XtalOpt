/* mathfunc.h */
/* Copyright (C) 2008 Atsushi Togo */

#ifndef __mathfunc_H__
#define __mathfunc_H__

double mat_get_determinant_d3(const double a[3][3]);
double mat_get_determinant_i3(const int a[3][3]);
void mat_copy_matrix_d3(double a[3][3], const double b[3][3]);
void mat_copy_matrix_i3(int a[3][3], const int b[3][3]);
void mat_copy_vector_d3(double a[3], const double b[3]);
void mat_copy_vector_i3(int a[3], const int b[3]);
int mat_check_identity_matrix_i3(const int a[3][3], const int b[3][3]);
void mat_multiply_matrix_d3(double m[3][3], const double a[3][3], const double b[3][3]);
void mat_multiply_matrix_i3(int m[3][3], const int a[3][3], const int b[3][3]);
void mat_multiply_matrix_di3(double m[3][3], const double a[3][3], const int b[3][3]);
void mat_multiply_matrix_vector_i3(int v[3], const int a[3][3], const int b[3]);
void mat_multiply_matrix_vector_d3(double v[3], const double a[3][3], const double b[3]);
void mat_multiply_matrix_vector_id3(double v[3], const int a[3][3], const double b[3]);
void mat_cast_matrix_3i_to_3d(double m[3][3], const int a[3][3]);
void mat_cast_matrix_3d_to_3i(int m[3][3], const double a[3][3]);
int mat_inverse_matrix_d3(double m[3][3], const double a[3][3], const double precision);
int mat_get_similar_matrix_d3(double m[3][3], const double a[3][3],
			      const double b[3][3], const double precision);
void mat_transpose_matrix_d3(double a[3][3], const double b[3][3]);
void mat_transpose_matrix_i3(int a[3][3], const int b[3][3]);
double mat_Dabs(const double a);
int mat_Nint(const double a);
double mat_Dmod1(const double a, const double prec);

#endif
