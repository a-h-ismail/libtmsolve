/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_FUNCTION_H
#define _TMS_FUNCTION_H

#ifndef LOCAL_BUILD
#include <tmsolve/c_complex_to_cpp.h>
#include <tmsolve/tms_math_strs.h>
#else
#include "c_complex_to_cpp.h"
#include "tms_math_strs.h"
#endif
/**
 * @file
 * @brief Contains extended functions declarations.
 */

/**
 * @brief Calculates the average of its arguments.
 */
int _tms_avg(tms_arg_list *args, tms_arg_list *labels, cdouble *result);

/**
 * @brief Calculates the minimun of its arguments.
 */
int _tms_min(tms_arg_list *args, tms_arg_list *labels, cdouble *result);

/**
 * @brief Calculates the maximum of its arguments.
 */
int _tms_max(tms_arg_list *args, tms_arg_list *labels, cdouble *result);

/**
 * @brief Expects two arguments, the value and the base.
 * @return The base logarithm for the specified value
 */
int _tms_logn(tms_arg_list *args, tms_arg_list *labels, cdouble *result);

/**
 * @brief Returns the integer part of the specified value (supports complex), will calculate the expression if provided.
 */
int _tms_int(tms_arg_list *L, tms_arg_list *labels, cdouble *result);

/**
 * @brief Generates a random decimal value in the range of INT_MIN;INT_MAX
 * @param L Expects an empty argument list.
 */
int _tms_rand(tms_arg_list *L, tms_arg_list *labels, cdouble *result);

/**
 * @brief Calculates the corresponding value for the provided hexadecimal representation.
 */
int _tms_hex(tms_arg_list *L, tms_arg_list *labels, cdouble *result);

/**
 * @brief Calculates the corresponding value for the provided octal representation.
 */
int _tms_oct(tms_arg_list *L, tms_arg_list *labels, cdouble *result);

/**
 * @brief Calculates the corresponding value for the provided binary representation.
 */
int _tms_bin(tms_arg_list *L, tms_arg_list *labels, cdouble *result);

/**
 * @brief Calculates the derivative of a function at a specific point.
 * @param L Argument list, expected two arguments.
 * @return The value of the derivative at the specified point.
 */
int _tms_derivative(tms_arg_list *L, tms_arg_list *labels, cdouble *result);

/**
 * @brief Calculates the bounded integral of a function.
 * @param L Argument list, expected three arguments.
 * @return integral(lower_bound,upper_bound,expression)
 */
int _tms_integrate(tms_arg_list *L, tms_arg_list *labels, cdouble *result);
#endif