/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef FUNCTION_H
#define FUNCTION_H

#ifndef LOCAL_BUILD
#include <tmsolve/string_tools.h>
#else
#include "string_tools.h"
#endif
/**
 * @file
 * @brief Contains extended functions definitions.
 */

/**
 * @brief Returns the integer part of the specified value (supports complex), will calculate the expression if provided.
 */
double complex tms_int(tms_arg_list *L);

/**
 * @brief Generates a random integer value in the range of INT_MIN;INT_MAX
 * @param L Expects an empty argument list.
 */
double complex tms_randint(tms_arg_list *L);

/**
 * @brief Generates a random decimal value in the range of INT_MIN;INT_MAX
 * @param L Expects an empty argument list.
 */
double complex tms_rand(tms_arg_list *L);

/**
 * @brief Calculates the corresponding value for the provided hexadecimal representation.
 */
double complex tms_hex(tms_arg_list *L);

/**
 * @brief Calculates the corresponding value for the provided octal representation.
 */
double complex tms_oct(tms_arg_list *L);

/**
 * @brief Calculates the corresponding value for the provided binary representation.
 */
double complex tms_bin(tms_arg_list *L);

/**
 * @brief Calculates the derivative of a function at a specific point.
 * @param L Argument list, expected two arguments.
 * @return The value of the derivative at the specified point.
 */
double complex tms_derivative(tms_arg_list *L);

/**
 * @brief Calculates the bounded integral of a function.
 * @param L Argument list, expected three arguments.
 * @return integral(lower_bound,upper_bound,expression)
 */
double complex tms_integrate(tms_arg_list *L);
#endif