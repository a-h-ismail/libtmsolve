/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_FUNCTION_H
#define _TMS_FUNCTION_H

#ifndef LOCAL_BUILD
#include <tmsolve/tms_math_strs.h>
#else
#include "tms_math_strs.h"
#endif
/**
 * @file
 * @brief Contains extended functions declarations.
 */

/**
 * @brief Calculates the average of its arguments.
 */
double complex tms_avg(tms_arg_list *args);

/**
 * @brief Calculates the minimun of its arguments.
 */
double complex tms_min(tms_arg_list *args);

/**
 * @brief Calculates the maximum of its arguments.
 */
double complex tms_max(tms_arg_list *args);

/**
 * @brief Expects two arguments, the value and the base.
 * @return The base logarithm for the specified value
 */
double complex tms_logn(tms_arg_list *args);

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