/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "matrix.h"
#include "internals.h"
// Warning: next section requires some algebra
double *find_subm(double *A, int i, int j)
{
    double *B;
    int a, i2, x, y, j2;
    a = *A;
    B = (double *)malloc(((a - 1) * (a - 1) + 2) * sizeof(double));
    *B = *(B + 1) = a - 1;
    B = B + 2;
    A = A + 2;
    y = 0;
    for (i2 = 0; i2 < a; ++i2)
    {
        x = 0;
        //first we select the row to copy
        if (i2 == i)
        {
            ++i2;
            //break on the end of the matrix
            if (i2 == a)
                break;
        }
        for (j2 = 0; j2 < a; ++j2)
        {
            //we select the row to copy
            if (j2 == j)
            {
                ++j2;
                if (j2 == a)
                    break;
            }
            *(B + (a - 1) * y + x) = *(A + (a * i2) + j2);
            ++x;
        }
        ++y;
    }
    return B - 2;
}
//Multiply matrixes A and B and return a pointer to the resulting matrix, returns NULL in case of error
double *matrix_multiply(double *A, double *B)
{
    int i, j, y, a, b, d;
    double *R;
    if (*(A + 1) != *B)
    {
        puts("Cannot multiply, incompatible dimensions.");
        return NULL;
    }
    R = (double *)calloc(*A * *(B + 1) + 2, sizeof(double));
    a = *A;
    b = *(A + 1);
    d = *(B + 1);
    //We offset A and B by 2 because first 2 elements are the dimensions
    A = A + 2;
    B = B + 2;
    *R = a;
    *(R + 1) = d;
    R = R + 2;
    //i defines the row at which we are working.
    for (i = 0; i < a; i++)
        for (j = 0; j < d; ++j)
        {
            y = 0;
            //take a couple minutes and read carefully, it isn't that hard
            while (y < d)
            {
                *(R + (d * i) + j) += *(A + (i * b) + y) * *(B + (y * d) + j);
                ++y;
            }
        }
    return R - 2;
}
//Function that replaces the column of the matrix with the matrix_column sent as argument
void remplace_matrix_column(double *matrix, double *matrix_column, int column)
{
    int a, b, i;
    a = *matrix;
    b = *(matrix + 1);
    for (i = 0; i < b; ++i)
        *(matrix + a * i + column + 2) = *(matrix_column + i);
}
//Create a copy of matrix and returns a pointer to the copy
double *matrix_copy(double *matrix)
{
    double *copy;
    int a, b, i, n;
    if (matrix == NULL)
        return NULL;
    a = *matrix;
    b = *(matrix + 1);
    n = a * b + 2;
    copy = (double *)malloc(n * sizeof(double));
    if (copy == NULL)
        return NULL;
    for (i = 0; i < n; ++i)
        copy[i] = matrix[i];
    return copy;
}
//Find the detrminant of function A, returns NaN on error
double matrix_det(double *A)
{
    int a, j;
    double mult, *B, det = 0;
    if (*A != *(A + 1))
    {
        puts("Error, matrix must be square.");
        return NAN;
    }
    a = *A;
    if (a == 2)
    {
        det = (*(A + 2) * *(A + 5)) - (*(A + 3) * *(A + 4));
        return det;
    }
    else
    {
        A = A + 2;
        //here j points at the column we are working at
        for (j = 0; j < a; ++j)
        {
            //hold the value pointed to by (0,j) to multiply later
            if (j % 2 == 0)
                mult = *(A + j);
            else
                mult = -*(A + j);
            //create a sub array to hold the matrix of order a-1
            B = find_subm(A - 2, 0, j);
            det += mult * matrix_det(B);
            free(B);
        }
    }
    return det;
}
double *matrix_tr(double *M)
{
    int i, j, a, b;
    double *R;
    a = *M;
    b = *(M + 1);
    R = (double *)malloc((a * b + 2) * sizeof(double));
    *R = b;
    *(R + 1) = a;
    R = R + 2;
    M = M + 2;
    for (i = 0; i < a; ++i)
        for (j = 0; j < b; ++j)
            *(R + (j * a) + i) = *(M + (i * b) + j);
    return R - 2;
}
double *comatrix(double *M)
{
    int i, j, a, b;
    double *R, *sub, det;
    //Check for empty matrix
    if (M == NULL)
        return NULL;
    a = *M;
    b = *(M + 1);
    R = (double *)malloc((a * b + 2) * sizeof(double));
    *R = a;
    *(R + 1) = b;
    if (a == 1 || a != b)
    {
        puts("Error, invalid matrix.");
        return NULL;
    }
    //Don't forget to decrement the pointer by 2 before sending it anywhere where free exists
    //You don't want to corrupt your heap, do you?
    R = R + 2;
    if (a == 2)
    {
        M = M + 2;
        *R = *(M + 3);
        *(R + 3) = *M;
        *(R + 1) = -*(M + 2);
        *(R + 2) = -*(M + 1);
        return R - 2;
    }
    for (i = 0; i < a; ++i)
        for (j = 0; j < b; ++j)
        {
            sub = find_subm(M, i, j);
            det = matrix_det(sub);
            free(sub);
            *(R + (i * b) + j) = pow(-1, i + j + 2) * det;
        }
    return R - 2;
}
double *matrix_inv(double *M)
{
    int a, b, i;
    double det, *R, *prevR;
    //Check for empty matrix
    if (M == NULL)
        return NULL;
    a = *M;
    b = *(M + 1);
    if (a != b || a < 2)
    {
        puts("Error, invalid matrix");
        return NULL;
    }
    R = comatrix(M);
    prevR = R;
    R = matrix_tr(R);
    free(prevR);
    det = matrix_det(M);
    if (det == 0)
    {
        puts("Matrix cannot be inverted, determinant = 0.");
        return NULL;
    }
    R = R + 2;
    for (i = 0; i < a * b; ++i)
        *(R + i) = *(R + i) / det;
    return R - 2;
}
