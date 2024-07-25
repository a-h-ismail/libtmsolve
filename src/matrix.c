/*
Copyright (C) 2021-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "matrix.h"
#include "error_handler.h"
#include "internals.h"
#include "m_errors.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

tms_matrix *tms_new_matrix(int rows, int columns)
{
    tms_matrix *matrix;
    matrix = malloc(sizeof(tms_matrix));
    matrix->columns = columns;
    matrix->rows = rows;
    matrix->data = malloc(rows * sizeof(double *));
    for (int i = 0; i < rows; ++i)
        matrix->data[i] = malloc(columns * sizeof(double));
    return matrix;
}
// Delete a matrix structure
void tms_delete_matrix(tms_matrix *matrix)
{
    for (int i = 0; i < matrix->rows; ++i)
        free(matrix->data[i]);
    free(matrix->data);
    free(matrix);
}

tms_matrix *tms_remove_matrix_row_col(tms_matrix *M, int row, int col)
{
    tms_matrix *new_M;
    new_M = tms_new_matrix(M->rows - 1, M->columns - 1);
    int parent_row, parent_col, derived_row, derived_col;
    for (parent_row = derived_row = 0; parent_row < M->rows; ++parent_row)
    {
        if (parent_row == row)
            continue;
        for (parent_col = derived_col = 0; parent_col < M->columns; ++parent_col)
        {
            if (parent_col == col)
                continue;
            else
                new_M->data[derived_row][derived_col++] = M->data[parent_row][parent_col];
        }
        ++derived_row;
    }
    return new_M;
}

void tms_round_to_identity_matrix(tms_matrix *M)
{
    int row, column;
    // Detect the all ones diagonal
    for (row = 0; row < M->rows; ++row)
    {
        column = row;
        if (fabs(1 - round(M->data[row][column])) < 1e-14)
            M->data[row][column] = 1;
        else
            return;
    }

    for (row = 0; row < M->rows; ++row)
        for (column = 0; column < M->columns; ++column)
        {
            if (row == column)
                continue;
            else if (fabs(M->data[row][column]) < 1e-14)
                M->data[row][column] = 0;
            else
                return;
        }
}

// Multiply matrixes A and B and return a pointer to the resulting matrix, returns NULL in case of error
tms_matrix *tms_matrix_multiply(tms_matrix *A, tms_matrix *B)
{
    int i, j, k;
    tms_matrix *result;
    if (A->columns != B->rows)
    {
        puts("Cannot multiply, incompatible dimensions.");
        return NULL;
    }
    result = tms_new_matrix(A->rows, B->columns);
    // i defines the row at which we are working.
    for (i = 0; i < A->rows; ++i)
        for (j = 0; j < B->columns; ++j)
        {
            result->data[i][j] = 0;
            // In the next loop: k sweeps the row 'i' of the left operand and column 'j' of the right operand
            for (k = 0; k < B->rows; ++k)
                result->data[i][j] += A->data[i][k] * B->data[k][j];
        }

    // Remove minor precision loss when multiplying the matrix and its inverse
    tms_round_to_identity_matrix(result);
    return result;
}

void tms_replace_matrix_col(tms_matrix *M, tms_matrix *column_matrix, int column)
{
    for (int i = 0; i < column_matrix->rows; ++i)
        M->data[i][column] = column_matrix->data[i][0];
}

tms_matrix *tms_matrix_dup(tms_matrix *matrix)
{
    tms_matrix *copy;
    if (matrix == NULL)
        return NULL;

    copy = tms_new_matrix(matrix->columns, matrix->rows);
    for (int i = 0; i < matrix->rows; ++i)
        memcpy(copy->data[i], matrix->data[i], matrix->columns * sizeof(double));
    return copy;
}

double tms_matrix_det(tms_matrix *A)
{
    int i;
    tms_matrix *B;
    double det = 0, tmp;
    if (A->rows != A->columns)
    {
        puts("Error, matrix must be square.");
        return NAN;
    }

    if (A->rows == 2)
    {
        det = A->data[0][0] * A->data[1][1] - A->data[1][0] * A->data[0][1];
        return det;
    }
    else
    {
        // i points at the column we are working at
        for (i = 0; i < A->columns; ++i)
        {
            // hold the value pointed to by (0,j) to multiply later
            if (i % 2 == 0)
                tmp = A->data[0][i];
            else
                tmp = -A->data[0][i];

            // Get matrix B from matrix A by excluding rows
            B = tms_remove_matrix_row_col(A, 0, i);
            det += tmp * tms_matrix_det(B);
            tms_delete_matrix(B);
        }
    }
    return det;
}

tms_matrix *tms_matrix_tr(tms_matrix *M)
{
    int i, j;
    tms_matrix *transpose = tms_new_matrix(M->columns, M->rows);
    for (i = 0; i < M->rows; ++i)
        for (j = 0; j < M->columns; ++j)
            transpose->data[j][i] = M->data[i][j];
    return transpose;
}

tms_matrix *tms_comatrix(tms_matrix *M)
{
    int i, j;
    tms_matrix *comatrix, *minor;
    double det;
    // Check for empty matrix
    if (M == NULL)
        return NULL;
    if (M->rows < 2 || M->rows != M->columns)
    {
        tms_save_error(TMS_MATRIX, INVALID_MATRIX, EH_FATAL, NULL, 0);
        return NULL;
    }
    comatrix = tms_new_matrix(M->rows, M->columns);
    if (comatrix->rows == 2)
    {
        comatrix->data[0][0] = M->data[1][1];
        comatrix->data[1][1] = M->data[0][0];
        comatrix->data[0][1] = -M->data[1][0];
        comatrix->data[1][0] = -M->data[0][1];
        return comatrix;
    }
    for (i = 0; i < M->rows; ++i)
        for (j = 0; j < M->columns; ++j)
        {
            minor = tms_remove_matrix_row_col(M, i, j);
            det = tms_matrix_det(minor);
            tms_delete_matrix(minor);
            comatrix->data[i][j] = pow(-1, i + j) * det;
        }
    return comatrix;
}

tms_matrix *tms_matrix_inv(tms_matrix *M)
{
    int i, j;
    double det;
    tms_matrix *inverse, *tmp;
    // Check for empty matrix
    if (M == NULL)
        return NULL;
    if (M->rows != M->columns || M->rows < 2)
    {
        puts("Can't invert non square matrix.");
        return NULL;
    }
    inverse = tms_comatrix(M);
    // tmp stores the comatrix pointer, otherwise it would be lost and memory leaked
    tmp = inverse;
    inverse = tms_matrix_tr(inverse);
    tms_delete_matrix(tmp);
    det = tms_matrix_det(M);
    if (det == 0)
    {
        puts("Matrix cannot be inverted, determinant = 0.");
        return NULL;
    }
    for (i = 0; i < inverse->rows; ++i)
        for (j = 0; j < inverse->columns; ++j)
            inverse->data[i][j] /= det;
    return inverse;
}
