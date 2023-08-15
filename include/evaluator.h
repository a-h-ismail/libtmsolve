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
#include <tmsolve/parser.h>
#include <tmsolve/string_tools.h>
#include <tmsolve/internals.h>
#include <tmsolve/scientific.h>
#else
#include "parser.h"
#include "string_tools.h"
#include "internals.h"
#include "scientific.h"
#endif

/**
 * @brief Evaluates a math_expr structure and calculates the result.
 * @param M The math structure to evaluate.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex tms_evaluate(tms_math_expr *M);

/**
 * @brief Generates a heap allocated array of structure containing all variable members metadata and stores it in M.
 * @param M The math_expr used to generate and store the metadata.
 * @remark You won't need to call this manually, the parser will call it if variables are enabled.
 */
void _tms_set_var_data(tms_math_expr *M);

/**
 * @brief Sets a value to all the variable members of M.
 */
void _tms_set_variable(tms_math_expr *M, double complex value);

/// @brief Dumps the data of the math expression M.
/// @details The dumped data includes: \n
/// - Subexpression depth and function pointers. \n
/// - Left and right operands of each node (and the operator). \n
/// - Nodes ordered by evaluation order. \n
/// - Result pointer of each subepression.
void tms_dump_expr(tms_math_expr *M, bool was_evaluated);
#endif