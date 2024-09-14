/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_EVALUATOR_H
#define _TMS_EVALUATOR_H
/**
 * @file
 * @brief Includes functions related to expression evaluation.
 */
#ifndef LOCAL_BUILD
#include <tmsolve/tms_math_strs.h>
#else
#include "tms_math_strs.h"
#endif

/**
 * @brief Evaluates a math_expr structure and calculates the result.
 * @param M Expression to evaluate.
 * @param options Supported: NO_LOCK and PRINT_ERRORS.
 * @note Thread safe, unless NO_LOCK is used.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex tms_evaluate(tms_math_expr *M, int options);

/**
 * @brief Calculates the answer for an int expression.
 * @param M Expression to evaluate.
 * @param result Pointer to a variable where the result will be stored.
 * @param options Supported: NO_LOCK and PRINT_ERRORS.
 * @note Thread safe, unless NO_LOCK is used.
 * @return 0 on success, -1 on failure.
 */
int tms_int_evaluate(tms_int_expr *M, int64_t *result, int options);

/**
 * @brief Sets the values of label operands.
 * @param M The math expression with label operands.
 * @param values_list A list of all labels values, should be indexed by the label ID.
 */
void tms_set_labels_values(tms_math_expr *M, double complex *values_list);

/**
 * @brief Sets the values of label operands.
 * @param M The int expression with label operands.
 * @param values_list A list of all labels values, should be indexed by the label ID.
 */
void tms_set_int_labels_values(tms_int_expr *M, int64_t *values_list);

/**  
 * @brief Dumps the data of the math expression M.
 * @details The dumped data includes: \n
 * Subexpression depth and function pointers. \n
 * Left and right operands of each node (and the operator). \n
 * Nodes ordered by evaluation order. \n
 * Result pointer of each subepression.
*/
void tms_dump_expr(tms_math_expr *M, bool was_evaluated);

/**  
 * @brief Dumps the data of an integer expression M.
 * @details The dumped data includes: \n
 * Subexpression depth and function pointers. \n
 * Left and right operands of each node (and the operator). \n
 * Nodes ordered by evaluation order. \n
 * Result pointer of each subepression.
*/
void tms_dump_int_expr(tms_int_expr *M, bool was_evaluated);

#endif