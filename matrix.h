/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include <math.h>
#include <stdlib.h>
double *find_subm(double *A, int i, int j);
double *matrix_multiply(double *A, double *B);
double matrix_det(double *A);
double *comatrix(double *M);
double *matrix_tr(double *M);
double *matrix_inv(double *M);
void remplace_matrix_column(double *matrix, double *matrix_column, int column);
double *matrix_copy(double *matrix);