/*
Copyright (C) 2021-2023 Ahmad Ismail
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
 * @brief Function handling
*/

/**
 * @brief Generates a heap allocated array of structure containing all variable members metadata and stores it in the math_struct.
 * @param M The math_struct used to generate and store the metadata.
 * @remark You won't need to call this manually, the parser will call it if variables are enabled.
 */
void _set_var_data(math_expr *M);

/**
 * @brief Sets a value to all the variable members of math_struct.
 */
void set_variable(math_expr *M, double complex value);

/**
 * @brief Calculates the derivative of a function at a specific point.
 * @details The function expects 2 arguments stored in a string and comma ',' separated.\n 
 * The first argument is the function to derivate and the second is the point to calculate the derivative at.\n
 * The derivation is done using the definition of derivative.
 * @param expr The string containing the arguments.
 * @return The value of the derivative at the specified point.
 */
double derivative(char *expr);

/**
 * @brief Calculates the bounded integral of a function.
 * @details The function expects 3 arguments stored in a string and comma ',' separated.\n 
 * Expected arguments: upper_bound, lower_bound, function.\n 
 * Numerical integration using Simpson's 3/8 rule.
 * @param expr The arguments string.
 * @return Integration answer, or NaN in case of failure.
 */
double integrate(char *expr);
#endif