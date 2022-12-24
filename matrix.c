/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "matrix.h"
#include "internals.h"
// Generate a matrix with dimensions
matrix_str *new_matrix(int rows, int columns)
{
    matrix_str *matrix;
    matrix = malloc(sizeof(matrix_str));
    matrix->columns = columns;
    matrix->rows = rows;
    matrix->data = malloc(rows * sizeof(double *));
    for (int i = 0; i < rows; ++i)
        matrix->data[i] = malloc(columns * sizeof(double));
    return matrix;
}
// Delete a matrix structure
void delete_matrix(matrix_str *matrix)
{
    for (int i = 0; i < matrix->rows; ++i)
        free(matrix->data[i]);
    free(matrix->data);
    free(matrix);
}
// Generates a new matrix derived from "matrix" excluding row and col
matrix_str *remove_matrix_row_col(matrix_str *matrix, int row, int col)
{
    matrix_str *derived_matrix;
    derived_matrix = new_matrix(matrix->rows - 1, matrix->columns - 1);
    int parent_row, parent_col, derived_row, derived_col;
    for (parent_row = derived_row = 0; parent_row < matrix->rows; ++parent_row)
    {
        if (parent_row == row)
            continue;
        for (parent_col = derived_col = 0; parent_col < matrix->columns; ++parent_col)
        {
            if (parent_col == col)
                continue;
            else
                derived_matrix->data[derived_row][derived_col++] = matrix->data[parent_row][parent_col];
        }
        ++derived_row;
    }
    return derived_matrix;
}
// Multiply matrixes A and B and return a pointer to the resulting matrix, returns NULL in case of error
matrix_str *matrix_multiply(matrix_str *A, matrix_str *B)
{
    int i, j, k;
    matrix_str *result;
    if (A->columns != B->rows)
    {
        puts("Cannot multiply, incompatible dimensions.");
        return NULL;
    }
    result = new_matrix(A->rows, B->columns);
    // i defines the row at which we are working.
    for (i = 0; i < A->rows; ++i)
        for (j = 0; j < B->columns; ++j)
        {
            result->data[i][j] = 0;
            // In the next loop: i, j are constant, k sweeps the column of the B matrix
            for (k = 0; k < B->rows; ++k)
                result->data[i][j] += A->data[i][k] * B->data[k][j];
        }
    return result;
}

void replace_matrix_column(matrix_str *matrix, matrix_str *column_matrix, int column)
{
    for (int i = 0; i < column_matrix->rows; ++i)
        matrix->data[i][column] = column_matrix->data[i][0];
}
// Create a copy of matrix and returns a pointer to the copy
matrix_str *matrix_dup(matrix_str *matrix)
{
    matrix_str *copy;
    if (matrix == NULL)
        return NULL;

    copy = new_matrix(matrix->columns, matrix->rows);
    for (int i = 0; i < matrix->rows; ++i)
        memcpy(copy->data[i], matrix->data[i], matrix->columns * sizeof(double));
    return copy;
}

// Find the determinant of matrix A, returns NaN on error
double matrix_det(matrix_str *A)
{
    int i;
    matrix_str *B;
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
            B = remove_matrix_row_col(A, 0, i);
            det += tmp * matrix_det(B);
            delete_matrix(B);
        }
    }
    return det;
}
// Get the transpose of matrix M
matrix_str *matrix_tr(matrix_str *M)
{
    int i, j;
    matrix_str *transpose = new_matrix(M->columns, M->rows);
    for (i = 0; i < M->rows; ++i)
        for (j = 0; j < M->columns; ++j)
            transpose->data[j][i] = M->data[i][j];
    return transpose;
}
matrix_str *comatrix(matrix_str *M)
{
    int i, j;
    matrix_str *comatrix, *minor;
    double det;
    // Check for empty matrix
    if (M == NULL)
        return NULL;
    if (M->rows < 2 || M->rows != M->columns)
    {
        error_handler(INVALID_MATRIX, 1, 1);
        return NULL;
    }
    comatrix = new_matrix(M->rows, M->columns);
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
            minor = remove_matrix_row_col(M, i, j);
            det = matrix_det(minor);
            delete_matrix(minor);
            comatrix->data[i][j] = pow(-1, i + j) * det;
        }
    return comatrix;
}
matrix_str *matrix_inv(matrix_str *M)
{
    int i, j;
    double det;
    matrix_str *inverse, *tmp;
    // Check for empty matrix
    if (M == NULL)
        return NULL;
    if (M->rows != M->columns || M->rows < 2)
    {
        puts("Can't invert non square matrix.");
        return NULL;
    }
    inverse = comatrix(M);
    // tmp stores the comatrix pointer, otherwise it would be lost and memory leaked
    tmp = inverse;
    inverse = matrix_tr(inverse);
    delete_matrix(tmp);
    det = matrix_det(M);
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
