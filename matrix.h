/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include <math.h>
#include <stdlib.h>
typedef struct matrix_str
{
    int rows, columns;
    double **data;
} matrix_str;
matrix_str *new_matrix(int rows, int columns);
void delete_matrix(matrix_str *matrix);
matrix_str *remove_matrix_row_col(matrix_str *matrix, int row, int col);
matrix_str *matrix_multiply(matrix_str *A, matrix_str *B);
double matrix_det(matrix_str *A);
matrix_str *comatrix(matrix_str *M);
matrix_str *matrix_tr(matrix_str *M);
matrix_str *matrix_inv(matrix_str *M);
void remplace_matrix_column(matrix_str *matrix, matrix_str *matrix_column, int column);
matrix_str *matrix_copy(matrix_str *matrix);