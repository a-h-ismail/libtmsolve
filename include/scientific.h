/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef SCIENTIFIC_H
#define SCIENTIFIC_H
/**
 * @file
 * @brief Declares all scientific related macros, structures, globals and functions.
 */
#ifndef LOCAL_BUILD
#include <tmsolve/parser.h>
#include <tmsolve/internals.h>
#else
#include "parser.h"
#include "internals.h"
#endif

/// Holds the data of a factor, for use with factorization related features.
typedef struct tms_int_factor
{
    int factor;
    int power;
} tms_int_factor;

/// Simple structure to store a fraction of the form a + b / c
typedef struct tms_fraction
{
    int a, b, c;
} tms_fraction;

/**
 * @brief Calculates the factorial.
 * @return value!
 */
double tms_factorial(double value);

/**
 * @brief Emulates pow() function with better performance for integer exponants.
 */
double tms_fast_pow(double x, double y);

/**
 * @brief Calculates a mathematical expression and returns the answer.
 * @param expr The string containing the math expression.
 * @param enable_complex Enables complex number calculation, set to false if you don't need complex values.
 * @note This function updates the global variable tms_g_ans if no error occurs.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex tms_solve_e(char *expr, bool enable_complex);

/**
 * @brief Calculates a mathematical expression and returns the answer, automatically handles complex numbers.
 * @param expr The string containing the math expression.
 * @note This function updates the global variable tms_g_ans if no error occurs.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex tms_solve(char *expr);


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
 * @param inverse_process Tells the function that the value it received is the inverse of the actual value.
 * For the first call, set it to false.
 * @return The fraction form of the value.
 */
tms_fraction tms_decimal_to_fraction(double value, bool inverse_process);

double complex tms_ccbrt(double complex z);
#endif