/*
Copyright (C) 2021-2025 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "error_handler.h"
#include "evaluator.h"
#include "function.h"
#include "int_parser.h"
#include "internals.h"
#include "parser.h"
#include "string_tools.h"
#include "tms_complex.h"
#include "tms_math_strs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double tms_carg_d(double __x)
{
    return carg(__x);
}

// Wrapper functions for cos, sin and tan to round for very small values
double tms_cos(double __x)
{
    __x = cos(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

double tms_sin(double __x)
{
    __x = sin(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

double tms_tan(double __x)
{
    __x = tan(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

double tms_fact(double value)
{
    double result = 1;
    for (int i = 2; i <= value; ++i)
        result *= i;
    return result;
}

void tms_set_ans(double complex result)
{
    if (!tms_iscnan(result))
        tms_g_ans = result;
}

bool tms_is_integer(double value)
{
    if ((value - floor(value)) == 0)
        return true;
    else
        return false;
}

bool tms_is_real(double complex z)
{
    if (cimag(z) == 0)
        return true;
    else
        return false;
}

double tms_sign(double value)
{
    if (value == 0)
        return 0;
    else if (value > 0)
        return 1;
    else
        return -1;
}

double complex tms_solve_e(char *expr, int options, tms_arg_list *labels)
{
    double complex result;
    tms_math_expr *M;

    M = tms_parse_expr(expr, options, labels);
    if (M == NULL)
        return NAN;
    result = tms_evaluate(M, options);
    // Do not free the labels as they were allocated elsewhere not here
    M->labels = NULL;
    tms_delete_math_expr(M);
    return result;
}

double complex tms_solve(char *expr)
{
    int i, j;
    // 0: can't be complex due to real exclusive operations like modulo.
    // 1: can be complex, mostly 2n roots of negative numbers.
    // 2: Is complex, imaginary number appears in the expression, or complex exclusive operations like arg().
    uint8_t likely_complex = 1;

    // We have some hope that this is a complex expression
    i = tms_f_search(expr, "i", 0, true);
    if (i != -1)
        likely_complex = 2;

    size_t var_count;
    tms_var *all_vars = tms_get_all_vars(&var_count, false);
    // Look for user defined complex variables
    for (i = 0; i < var_count; ++i)
    {
        if (!tms_is_real(all_vars[i].value))
        {
            j = tms_f_search(expr, all_vars[i].name, 0, true);
            if (j != -1)
            {
                likely_complex = 2;
                break;
            }
        }
    }
    free(all_vars);

    // Special case of ans
    if (!tms_is_real(tms_g_ans))
    {
        j = tms_f_search(expr, "ans", 0, true);
        if (j != -1)
            likely_complex = 2;
    }

    tms_math_expr *M;
    double complex result;
    switch (likely_complex)
    {
        // No longer used, kept for legacy.
    case 0:

        M = tms_parse_expr(expr, 0, NULL);
        result = tms_evaluate(M, 0);
        tms_delete_math_expr(M);
        return result;

    case 1:
        // We don't want errors to be printed automatically (to keep the error database from being cleared)
        // And we want to keep the parser locked until we get the correct answer and print errors if any
        tms_lock_parser(TMS_PARSER);
        M = tms_parse_expr(expr, NO_LOCK, NULL);

        if (M == NULL)
        {
            // Parsing failed due to a fatal error, don't take further action
            if (tms_get_error_count(TMS_PARSER, EH_FATAL) != 0)
            {
                tms_print_errors(TMS_PARSER);
                tms_unlock_parser(TMS_PARSER);
                return NAN;
            }
            else
            {
                // Clear previous errors and try again with complex enabled
                tms_clear_errors(TMS_PARSER);
                M = tms_parse_expr(expr, NO_LOCK | ENABLE_CMPLX, NULL);

                // Failed again somehow with complex enabled, so abort
                if (M == NULL)
                {
                    tms_print_errors(TMS_PARSER);
                    tms_unlock_parser(TMS_PARSER);
                    return NAN;
                }
                else
                    tms_unlock_parser(TMS_PARSER);
            }
        }
        else
            tms_unlock_parser(TMS_PARSER);

        // At this point we have no problems with the parser
        if (M->enable_complex)
        {
            result = tms_evaluate(M, 0);
            tms_delete_math_expr(M);
            return result;
        }
        else
        {
            tms_lock_evaluator(TMS_EVALUATOR);
            result = tms_evaluate(M, NO_LOCK);

            if (tms_iscnan(result))
            {
                // Not a fatal error, convert to complex and try again
                if (tms_get_error_count(TMS_EVALUATOR | TMS_PARSER, EH_FATAL) == 0)
                {
                    tms_convert_real_to_complex(M);
                    // Conversion to complex failed
                    if (!M->enable_complex)
                    {
                        tms_delete_math_expr(M);
                        return NAN;
                    }
                    // Conversion succeeded, clear previous errors
                    tms_clear_errors(TMS_EVALUATOR | TMS_PARSER);
                    result = tms_evaluate(M, NO_LOCK);
                    if (tms_iscnan(result))
                        tms_print_errors(TMS_EVALUATOR | TMS_PARSER);
                }
                else
                    tms_print_errors(TMS_EVALUATOR | TMS_PARSER);
            }

            tms_unlock_evaluator(TMS_EVALUATOR);
            tms_delete_math_expr(M);
            return result;
        }

    case 2:
        M = tms_parse_expr(expr, ENABLE_CMPLX | PRINT_ERRORS, NULL);
        result = tms_evaluate(M, PRINT_ERRORS);
        tms_delete_math_expr(M);
        return result;
    }
    // Unlikely to fall out of the switch.
    tms_save_error(TMS_GENERAL, INTERNAL_ERROR, EH_FATAL, NULL, 0);
    return NAN;
}

int tms_int_solve(char *expr, int64_t *result)
{
    tms_int_expr *M;
    M = tms_parse_int_expr(expr, PRINT_ERRORS, NULL);
    if (M == NULL)
        return -1;

    int state = tms_int_evaluate(M, result, PRINT_ERRORS);
    tms_delete_int_expr(M);
    return state;
}

int tms_int_solve_e(char *expr, int64_t *result, int options, tms_arg_list *labels)
{
    tms_int_expr *M;
    M = tms_parse_int_expr(expr, options, labels);
    if (M == NULL)
        return -1;

    int state = tms_int_evaluate(M, result, options);
    // Labels were not allocated here so remove them to avoid being freed next
    M->labels = NULL;
    tms_delete_int_expr(M);
    return state;
}

tms_int_factor *tms_find_factors(int32_t value)
{
    int32_t dividend = 2;
    tms_int_factor *factor_list;
    int i = 0;

    factor_list = calloc(64, sizeof(tms_int_factor));
    if (value == 0)
        return factor_list;
    factor_list[0].factor = factor_list[0].power = 1;

    // Turning negative numbers into positive for factorization to work
    if (value < 0)
        value = -value;
    // Simple optimized factorization algorithm
    while (value != 1)
    {
        if (value % dividend == 0)
        {
            ++i;
            value = value / dividend;
            factor_list[i].factor = dividend;
            factor_list[i].power = 1;
            while (value % dividend == 0 && value >= dividend)
            {
                value = value / dividend;
                ++factor_list[i].power;
            }
        }
        else
        {
            // Optimize by skipping values greater than value/2 (in other terms, the value itself is a prime)
            if (dividend > value / 2 && value != 1)
            {
                ++i;
                factor_list[i].factor = value;
                factor_list[i].power = 1;
                break;
            }
        }
        // Optimizing factorization by skipping even values (> 2)
        if (dividend > 2)
            dividend = dividend + 2;
        else
            ++dividend;
    }
    // Case of a prime number
    if (i == 0)
    {
        factor_list[1].factor = value;
        factor_list[1].power = 1;
    }
    return factor_list;
}

void tms_reduce_fraction(tms_fraction *fraction_str)
{
    int i = 1, j = 1, min;
    tms_int_factor *num_factor, *denom_factor;
    num_factor = tms_find_factors(fraction_str->b);
    denom_factor = tms_find_factors(fraction_str->c);
    // If a zero was returned by the factorization function, nothing to do.
    if (num_factor->factor == 0 || denom_factor->factor == 0)
    {
        free(num_factor);
        free(denom_factor);
        return;
    }
    while (num_factor[i].factor != 0 && denom_factor[j].factor != 0)
    {
        if (num_factor[i].factor == denom_factor[j].factor)
        {
            // Subtract the minimun from the power of the num and denom
            min = find_min(num_factor[i].power, denom_factor[j].power);
            num_factor[i].power -= min;
            denom_factor[j].power -= min;
            ++i;
            ++j;
        }
        else
        {
            // If the factor at i is smaller increment i , otherwise increment j
            if (num_factor[i].factor < denom_factor[j].factor)
                ++i;
            else
                ++j;
        }
    }
    fraction_str->b = fraction_str->c = 1;
    // Calculate numerator and denominator after reduction
    for (i = 1; num_factor[i].factor != 0; ++i)
        fraction_str->b *= pow(num_factor[i].factor, num_factor[i].power);

    for (i = 1; denom_factor[i].factor != 0; ++i)
        fraction_str->c *= pow(denom_factor[i].factor, denom_factor[i].power);

    free(num_factor);
    free(denom_factor);
}
// Converts a floating point value to decimal representation a*b/c
tms_fraction tms_decimal_to_fraction(double value, int precision, bool inverse_process)
{
    int dec_point = 1, patt_start, patt_end, frac_length;
    bool success = false;
    char pattern[11], printed_value[17];
    // return frac_error to signal that conversion fails
    tms_fraction result, frac_error = {0, 0, 0};
    result.c = 0;

    if (fabs(value) > INT32_MAX || fabs(value) < pow(10, -log10(INT32_MAX) + 1))
        return frac_error;

    // Store the integer part in 'a'
    result.a = floor(value);
    value -= result.a;

    // Edge case due to floating point lack of precision
    if (1 - value < 1e-9)
        return frac_error;

    // The case of an integer
    if (value == 0)
        return frac_error;

    // Reduce the number of decimal places to print as much as the complete value has non decimal places
    // This avoids obtaining junk values (remember a double has ~15 digits of precision)
    // Precision can be overriden by the caller if not set to zero
    if (result.a != 0)
        sprintf(printed_value, "%.*lf", (precision > 0 ? precision : 14 - (int)log10(abs(result.a))), value);
    else
        sprintf(printed_value, "%.*lf", (precision > 0 ? precision : 14), value);

    // Removing trailing zeros
    for (int i = strlen(printed_value) - 1; i > dec_point; --i)
    {
        if (printed_value[i] != '0')
        {
            printed_value[i + 1] = '\0';
            break;
        }
    }

    frac_length = strlen(printed_value) - dec_point - 1;
    if (frac_length >= 10)
    {
        // Look for a pattern to detect fractions from periodic decimals
        for (patt_start = dec_point + 1; patt_start - dec_point < frac_length / 2; ++patt_start)
        {
            // First number in the pattern (to the right of the decimal_point)
            pattern[0] = printed_value[patt_start];
            pattern[1] = '\0';
            patt_end = patt_start + 1;
            while (true)
            {
                // First case: the "pattern" is smaller than the remaining decimal digits.
                if (strlen(printed_value) - patt_end > strlen(pattern))
                {
                    // If the pattern is found again in the remaining decimal digits, jump over it.
                    if (strncmp(pattern, printed_value + patt_end, strlen(pattern)) == 0)
                    {
                        patt_start += strlen(pattern);
                        patt_end += strlen(pattern);
                    }
                    // If not, copy the digits between p_start and p_end to the pattern.
                    else
                    {
                        strncpy(pattern, printed_value + patt_start, patt_end - patt_start + 1);
                        pattern[patt_end - patt_start + 1] = '\0';
                        ++patt_end;
                    }
                    // If the pattern matches the last decimal digits, stop the execution with success.
                    if (strlen(printed_value) - patt_end == strlen(pattern) &&
                        strncmp(pattern, printed_value + patt_end, strlen(pattern)) == 0)
                    {
                        success = true;
                        break;
                    }
                }
                // Second case: the pattern is bigger than the remaining digits.
                else
                {
                    // Consider a success the case where the remaining digits except the last one match the leftmost digits of the pattern.
                    if (strncmp(pattern, printed_value + patt_end, strlen(printed_value) - patt_end - 1) == 0)
                    {
                        success = true;
                        break;
                    }
                    else
                        break;
                }
            }
            if (success == true)
                break;
        }
    }
    // getting the fraction from the "pattern"
    if (success == true)
    {
        int pattern_start;
        // Just in case the pattern was found to be zeros due to minor rounding (like 5.0000000000000003)
        if (strcmp(pattern, "0") == 0)
            return frac_error;

        // Generate the denominator
        for (patt_start = 0; patt_start < strlen(pattern); ++patt_start)
            result.c += 9 * pow(10, patt_start);
        // Find the pattern start in case it doesn't start right after the decimal point (like 0.79999)
        pattern_start = tms_f_search(printed_value, pattern, dec_point + 1, false);
        if (pattern_start > dec_point + 1)
        {
            // Overflow detection using double
            double tmp = round(value * (pow(10, pattern_start - dec_point - 1 + strlen(pattern)) -
                                        pow(10, pattern_start - dec_point - 1)));
            if (tmp > INT32_MAX)
                return frac_error;
            else
                result.b = tmp;
            if (pattern_start - dec_point - 1 > 8)
                return frac_error;
            else
                result.c *= pow(10, pattern_start - dec_point - 1);
        }
        else
            sscanf(pattern, "%d", &result.b);
        tms_reduce_fraction(&result);
        return result;
    }
    // trying to get the fraction by inverting it then trying the algorithm again
    else if (inverse_process == false)
    {
        double inverse_value = 1 / value;
        // case where the fraction is likely of form 1/x
        if (fabs(inverse_value - floor(inverse_value)) < 1e-10)
        {
            result.b = 1;
            result.c = inverse_value;
            return result;
        }
        // Other cases like 3/17 (non periodic but the inverse is periodic)
        else
        {
            // Given that we are sending an inverted value that originated from a decimal only value
            // And given that the decimal value had an integer value that drops its precision
            // We need to inform the recursive call of the precision loss
            tms_fraction inverted = tms_decimal_to_fraction(inverse_value, 14 - log10(abs(result.a)), true);
            if (inverted.c != 0)
            {
                result.b = inverted.c;
                result.c = inverted.b + inverted.a * inverted.c;
                return result;
            }
        }
    }
    // Simple cases with finite decimal digits (don't bother with fractional parts greater than 5)
    if (frac_length < 6)
    {
        result.c = pow(10, frac_length);
        result.b = round(value * result.c);
        tms_reduce_fraction(&result);
        return result;
    }
    else
        return frac_error;
}
