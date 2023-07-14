/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef LOCAL_BUILD
#include <tmsolve/internals.h>
#else
#include "internals.h"
#endif
/**
 * @file
 * @brief Declares all matrix related macros, structures, globals and functions.
 */

/**
 * @brief Stores the metadata of a 2D matrix.
 */
typedef struct tms_matrix
{
    /// Number of rows.
    int rows;
    /// Number of columns
    int columns;
    /// Double array storing the members of the 2D matrix.
    double **data;
} tms_matrix;

/**
 * @brief Allocates a new matrix with dimensions row*columns.
 * @param rows Number of rows.
 * @param columns Number of columns
 * @return A malloc'd pointer to the new matrix.
 */
tms_matrix *tms_new_matrix(int rows, int columns);

/**
 * @brief Deletes a matrix generated using tms_new_matrix().
 * @param matrix The matrix to delete.
 */
void tms_delete_matrix(tms_matrix *matrix);

tms_matrix *tms_remove_matrix_row_col(tms_matrix *matrix, int row, int col);

/**
 * @brief Multiplies matrixes A and B.
 * @return A new matrix, answer of A*B.
 */
tms_matrix *tms_matrix_multiply(tms_matrix *A, tms_matrix *B);

/**
 * @brief Calculates the determinant of a matrix.
 * @return The determinant, or NaN in case of failure.
 */
double tms_matrix_det(tms_matrix *A);

/**
 * @brief Calculates the comatrix of the matrix M.
 * @return The comatrix, or NULL in case of failure.
 */
tms_matrix *tms_comatrix(tms_matrix *M);

/**
 * @brief Calculates the transpose of the matrix M.
 * @return The transposed matrix, or NULL in case of failure.
 */
tms_matrix *tms_matrix_tr(tms_matrix *M);

/**
 * @brief Calculates the inverse of the matrix M.
 * @return The inverse matrix, or NULL in case of failure.
 */
tms_matrix *tms_matrix_inv(tms_matrix *M);

/**
 * @brief Replaces the column of the matrix with the matrix_column sent as argument
 * @param matrix The matrix that will have its column replaced.
 * @param column_matrix The column matrix, should have the same number of rows as matrix.
 * @param column The column to replace.
 */
void tms_replace_matrix_col(tms_matrix *matrix, tms_matrix *column_matrix, int column);

/**
 * @brief Duplicates a matrix
 * @return An identical malloc'd matrix.
 */
tms_matrix *tms_matrix_dup(tms_matrix *matrix);