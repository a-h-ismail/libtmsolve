/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include <math.h>
#include <stdlib.h>
/**
 * @file
 * @brief Declares all matrix related macros, structures, globals and functions.
*/
// Errors
#define INVALID_MATRIX "Invalid matrix."
/**
 * @brief Stores the metadata of a 2D matrix.
 */
typedef struct matrix_str
{
    /// Number of rows.
    int rows;
    /// Number of columns
    int columns;
    /// Double array storing the members of the 2D matrix.
    double **data;
} matrix_str;

/**
 * @brief Allocates a new matrix with dimensions row*columns.
 * @param rows Number of rows.
 * @param columns Number of columns
 * @return A malloc'd pointer to the new matrix.
 */
matrix_str *new_matrix(int rows, int columns);

/**
 * @brief Deletes a matrix generated using new_matrix().
 * @param matrix The mtrix to delete.
 */
void delete_matrix(matrix_str *matrix);

matrix_str *remove_matrix_row_col(matrix_str *matrix, int row, int col);

/**
 * @brief Multiplies matrixes A and B.
 * @return A new matrix, answer of A*B.
 */
matrix_str *matrix_multiply(matrix_str *A, matrix_str *B);

/**
 * @brief Calculates the determinant of a matrix.
 * @return The determinant, or NaN in case of failure.
 */
double matrix_det(matrix_str *A);

/**
 * @brief Calculates the comatrix of the matrix M.
 * @return The comatrix, or NULL in case of failure.
 */
matrix_str *comatrix(matrix_str *M);

/**
 * @brief Calculates the transpose of the matrix M.
 * @return The transposed matrix, or NULL in case of failure.
 */
matrix_str *matrix_tr(matrix_str *M);

/**
 * @brief Calculates the inverse of the matrix M.
 * @return The inverse matrix, or NULL in case of failure.
 */
matrix_str *matrix_inv(matrix_str *M);

/**
 * @brief Replaces the column of the matrix with the matrix_column sent as argument
 * @param matrix The matrix that will have its column replaced.
 * @param column_matrix The column matrix, should have the same number of rows as matrix.
 * @param column The column to replace.
 */
void replace_matrix_column(matrix_str *matrix, matrix_str *column_matrix, int column);

/**
 * @brief Duplicates a matrix
 * @return An identical malloc'd matrix.
 */
matrix_str *matrix_copy(matrix_str *matrix);