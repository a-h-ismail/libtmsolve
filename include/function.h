/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef FUNCTION_H
#define FUNCTION_H

#ifndef LOCAL_BUILD
#include <tmsolve/internals.h>
#else
#include "internals.h"
#endif
/**
 * @file
 * @brief Contains extended functions definitions and assistant functions.
 */

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

double complex tms_hex(tms_arg_list *L);

double complex tms_oct(tms_arg_list *L);

double complex tms_bin(tms_arg_list *L);

/**
 * @brief Calculates the derivative of a function at a specific point.
 * @details The function expects 2 arguments stored in a string and comma ',' separated.\n
 * The first argument is the function to derivate and the second is the point to calculate the derivative at.\n
 * The derivation is done using the definition of derivative.
 * @param expr The string containing the arguments.
 * @return The value of the derivative at the specified point.
 */
double complex tms_derivative(tms_arg_list *args);

/**
 * @brief Calculates the bounded integral of a function.
 * @details The function expects 3 arguments stored in a string and comma ',' separated.\n
 * Expected arguments: upper_bound, lower_bound, function.\n
 * Numerical integration using Simpson's 3/8 rule.
 * @param expr The arguments string.
 * @return Integration answer, or NaN in case of failure.
 */
double complex tms_integrate(tms_arg_list *args);
#endif