/*
Copyright (C) 2022-2023,2025 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_SCIENTIFIC_H
#define _TMS_SCIENTIFIC_H
/**
 * @file
 * @brief Declares all scientific related macros, structures, globals and functions.
 */

#ifndef LOCAL_BUILD
#include <tmsolve/c_complex_to_cpp.h>
#include <tmsolve/tms_math_strs.h>
#else
#include "c_complex_to_cpp.h"
#include "tms_math_strs.h"
#endif

#include <inttypes.h>
#include <stdbool.h>

/// Holds the data of a factor, for use with factorization related features.
typedef struct tms_int_factor
{
    int factor;
    int power;
} tms_int_factor;

/// Simple structure to store a fraction of the form a + b / c
typedef struct tms_fraction
{
    int32_t a, b, c;
} tms_fraction;

/**
 * @brief Sets the answer global variable if the provided result is not NaN
 */
void tms_set_ans(cdouble result);

/**
 * @brief Checks if the value is an integer.
 */
bool tms_is_integer(double value);

/**
 * @brief Checks if a complex variable is real only.
 */
bool tms_is_real(cdouble z);

/**
 * @brief Calculates a mathematical expression and returns the answer.
 * @param expr The string form of the math expression.
 * @param options Modifies the behavior of the solving operation using flags (NO_LOCK, ENABLE_COMPLEX, PRINT_ERRORS)
 * @param labels The variable names to be passed to the parser, set to NULL if you don't need them.
 * @return The answer of the math expression, or NaN in case of failure.
 */
cdouble tms_solve_e(const char *expr, int options, tms_arg_list *labels);

/**
 * @brief Calculates a mathematical expression and returns the answer, automatically handles complex numbers.
 * @param expr The string containing the math expression.
 * @return The answer of the math expression, or NaN in case of failure.
 * @note Prints errors to stderr (if any).
 */
cdouble tms_solve(const char *expr);

/**
 * @brief Calculates an integer expression
 * @param expr The string form of the integer expression.
 * @param result Pointer to variable where answer will be stored.
 * @return 0 on success, -1 on failure.
 * @note Prints errors to stderr (if any).
 */
int tms_int_solve(char *expr, int64_t *result);

/**
 * @brief Calculates an integer expression
 * @param expr The string form of the integer expression.
 * @param result Pointer to variable where answer will be stored.
 * @param options Modifies the behavior of the solving operation using flags (NO_LOCK, PRINT_ERRORS)
 * @param labels The variable names to be passed to the parser, set to NULL if you don't need them.
 * @return 0 on success, -1 on failure.
 */
int tms_int_solve_e(char *expr, int64_t *result, int options, tms_arg_list *labels);

/**
 * @brief Calculates the factorial.
 * @return value!
 */
double tms_fact(double value);

/**
 * @brief Sign function
 * @return 1 for positive, -1 for negative, 0 otherwise.
 */
double tms_sign(double value);

double tms_carg_d(double __x);

double tms_cos(double x);

double tms_sin(double x);

double tms_tan(double x);

/**
 * @brief Finds the factors of a signed 32bit integer.
 * @param value The value to factorize.
 * @return An array (malloc'd) of factors and the number of occurences of each factor.
 */
tms_int_factor *tms_find_factors(int32_t value);

/**
 * @brief Reduces a fraction to its irreductible form.
 * @param fraction_r Pointer to the fraction to reduce.
 */
void tms_reduce_fraction(tms_fraction *fraction_r);

/**
 * @brief Converts a decimal value into its equivalent fraction if possible.
 * @param value The decimal value to convert.
 * @param precision Number of useful decimal places in value, set to 0 for default.
 * @param inverse_process Tells the function that the value it received is the inverse of the actual value, set it to false.
 * @return The fraction form of the value.
 */
tms_fraction tms_decimal_to_fraction(double value, int precision, bool inverse_process);
#endif