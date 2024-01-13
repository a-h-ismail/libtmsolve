/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef EVALUATOR_H
#define EVALUATOR_H
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
 * @param M The math structure to evaluate.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex tms_evaluate(tms_math_expr *M);

/**
 * @brief Calculates the answer for an int expression
 * @param M
 * @param result
 * @return 0 on success, -1 on failure
 */
int tms_int_evaluate(tms_int_expr *M, int64_t *result);

/**
 * @brief Generates a heap allocated array of structure containing all unknown members metadata and stores it in M.
 * @param M The math_expr used to generate and store the metadata.
 * @remark You won't need to call this manually, the parser will call it if unknowns are enabled.
 */
void _tms_set_unknowns_data(tms_math_expr *M);

/**
 * @brief Sets a value to all the unknown members of M.
 */
void tms_set_unknown(tms_math_expr *M, double complex value);

/// @brief Dumps the data of the math expression M.
/// @details The dumped data includes: \n
/// - Subexpression depth and function pointers. \n
/// - Left and right operands of each node (and the operator). \n
/// - Nodes ordered by evaluation order. \n
/// - Result pointer of each subepression.
void tms_dump_expr(tms_math_expr *M, bool was_evaluated);

void tms_dump_int_expr(tms_int_expr *M, bool was_evaluated);

#endif