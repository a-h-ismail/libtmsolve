/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"

double factorial(double value)
{
    double result = 1;
    for (int i = 2; i <= value; ++i)
        result *= i;
    return result;
}

void _set_ans(double complex result)
{
    if (!isnan(creal(result)) && !isnan(cimag(result)))
        ans = result;
}

double complex calculate_expr(char *expr, bool enable_complex)
{
    double complex result;
    math_expr *math_struct;

    if (pre_parse_routine(expr) == false)
        return NAN;
    math_struct = parse_expr(expr, false, enable_complex);
    if (math_struct == NULL)
        return NAN;
    result = eval_math_expr(math_struct);
    _set_ans(result);
    delete_math_expr(math_struct);
    return result;
}

double complex calculate_expr_auto(char *expr)
{
    char *real_exclusive[] = {"%"};
    // Offset if the expression has an assignment operator
    char *local_expr = expr;
    int i, j;
    // 0: can't be complex due to real exclusive operations like modulo.
    // 1: can be complex, mostly 2n roots of negative numbers.
    // 2: Is complex, imaginary number appears in the expression, or complex exclusive operations like arg().
    uint8_t likely_complex = 1;

    if (!pre_parse_routine(expr))
        return NAN;

    // Skip assignment operator
    while ((i = f_search(local_expr, "=", 0, false)) != -1)
        local_expr += i + 1;

    // Look for real exclusive operations
    for (i = 0; i < array_length(real_exclusive); ++i)
    {
        j = f_search(local_expr, real_exclusive[i], 0, false);
        if (j != -1)
        {
            likely_complex = 0;
            break;
        }
    }
    // We have some hope that this is a complex expression
    i = f_search(local_expr, "i", 0, true);
    if (i != -1)
    {
        // The complex number was found despite having a real expression, report the problem.
        if (likely_complex == 0)
        {
            error_handler(ILLEGAL_COMPLEX_OP, EH_SAVE, EH_FATAL_ERROR, i);
            return NAN;
        }
        else
            likely_complex = 2;
    }

    // Look for user defined complex variables
    for (i = 0; i < variable_count; ++i)
    {
        if (cimag(variable_values[i]) != 0)
        {
            j = f_search(local_expr, variable_names[i], 0, true);
            if (j != -1)
            {
                likely_complex = 2;
                break;
            }
        }
    }

    // Special case of ans
    if (cimag(ans) != 0)
    {
        j = f_search(local_expr, "ans", 0, true);
        if (j != -1)
            likely_complex = 2;
    }

    math_expr *M;
    double complex result;
    switch (likely_complex)
    {
    case 0:

        M = parse_expr(expr, false, false);
        result = eval_math_expr(M);
        _set_ans(result);
        delete_math_expr(M);
        return result;

    case 1:
        M = parse_expr(expr, false, false);
        result = eval_math_expr(M);
        _set_ans(result);
        if (isnan(creal(result)))
        {
            // Check if the errors are fatal (like malformed syntax, division by zero...)
            int fatal = error_handler(NULL, EH_ERROR_COUNT, EH_MAIN_DB);
            if (fatal > 0)
                return NAN;

            // Clear errors caused by the first evaluation.
            error_handler(NULL, EH_CLEAR, EH_MAIN_DB);

            // The errors weren't fatal, possibly a complex answer.
            // Convert the math_struct to use complex functions.
            if (M != NULL)
            {
                convert_real_to_complex(M);
                // If the conversion failed due to real only functions, return a failure.
                if (M->enable_complex == false)
                {
                    delete_math_expr(M);
                    return NAN;
                }
            }
            else
                M = parse_expr(expr, false, true);

            result = eval_math_expr(M);
            _set_ans(result);
            delete_math_expr(M);
            return result;
        }
        else
        {
            delete_math_expr(M);
            return result;
        }
    case 2:
        M = parse_expr(expr, false, true);
        result = eval_math_expr(M);
        _set_ans(result);
        delete_math_expr(M);
        return result;
    }
    // Unlikely to fall out of the switch.
    error_handler(INTERNAL_ERROR, EH_SAVE, EH_FATAL_ERROR, 0);
    return NAN;
}

int_factor *find_factors(int32_t value)
{
    int32_t dividend = 2;
    int_factor *factor_list;
    int i = 0;

    factor_list = calloc(64, sizeof(int_factor));
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

void reduce_fraction(fraction *fraction_str)
{
    int i = 1, j = 1, min;
    int_factor *num_factor, *denom_factor;
    num_factor = find_factors(fraction_str->b);
    denom_factor = find_factors(fraction_str->c);
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
fraction decimal_to_fraction(double value, bool inverse_process)
{
    int decimal_point = 1, p_start, p_end, decimal_length;
    bool success = false;
    char pattern[11], printed_value[23];
    fraction result;

    // Using the denominator as a mean to report failure, if c==0 then the function failed
    result.c = 0;

    if (fabs(value) > INT32_MAX || fabs(value) < pow(10, -log10(INT32_MAX) + 1))
        return result;

    // Store the integer part in 'a'
    result.a = floor(value);
    value -= floor(value);

    // Edge case due to floating point lack of precision
    if (1 - value < 1e-9)
        return result;

    // The case of an integer
    if (value == 0)
        return result;

    sprintf(printed_value, "%.14lf", value);
    // Removing trailing zeros
    for (int i = strlen(printed_value) - 1; i > decimal_point; --i)
    {
        // Stop at the first non zero value and null terminate
        if (*(printed_value + i) != '0')
        {
            *(printed_value + i + 1) = '\0';
            break;
        }
    }

    decimal_length = strlen(printed_value) - decimal_point - 1;
    if (decimal_length >= 10)
    {
        // Look for a pattern to detect fractions from periodic decimals
        for (p_start = decimal_point + 1; p_start - decimal_point < decimal_length / 2; ++p_start)
        {
            // First number in the pattern (to the right of the decimal_point)
            pattern[0] = printed_value[p_start];
            pattern[1] = '\0';
            p_end = p_start + 1;
            while (true)
            {
                // First case: the "pattern" is smaller than the remaining decimal digits.
                if (strlen(printed_value) - p_end > strlen(pattern))
                {
                    // If the pattern is found again in the remaining decimal digits, jump over it.
                    if (strncmp(pattern, printed_value + p_end, strlen(pattern)) == 0)
                    {
                        p_start += strlen(pattern);
                        p_end += strlen(pattern);
                    }
                    // If not, copy the digits between p_start and p_end to the pattern.
                    else
                    {
                        strncpy(pattern, printed_value + p_start, p_end - p_start + 1);
                        pattern[p_end - p_start + 1] = '\0';
                        ++p_end;
                    }
                    // If the pattern matches the last decimal digits, stop the execution with success.
                    if (strlen(printed_value) - p_end == strlen(pattern) &&
                        strncmp(pattern, printed_value + p_end, strlen(pattern)) == 0)
                    {
                        success = true;
                        break;
                    }
                }
                // Second case: the pattern is bigger than the remaining digits.
                else
                {
                    // Consider a success the case where the remaining digits except the last one match the leftmost digits of the pattern.
                    if (strncmp(pattern, printed_value + p_end, strlen(printed_value) - p_end - 1) == 0)
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
    // Simple cases with finite decimal digits
    else
    {
        result.c = pow(10, decimal_length);
        result.b = round(value * result.c);
        reduce_fraction(&result);
        return result;
    }
    // getting the fraction from the "pattern"
    if (success == true)
    {
        int pattern_start;
        // Just in case the pattern was found to be zeors due to minor rounding (like 5.0000000000000003)
        if (pattern[0] == '0')
            return result;

        // Generate the denominator
        for (p_start = 0; p_start < strlen(pattern); ++p_start)
            result.c += 9 * pow(10, p_start);
        // Find the pattern start in case it doesn't start right after the decimal point (like 0.79999)
        pattern_start = f_search(printed_value, pattern, decimal_point + 1, false);
        if (pattern_start > decimal_point + 1)
        {
            result.b = round(value * (pow(10, pattern_start - decimal_point - 1 + strlen(pattern)) - pow(10, pattern_start - decimal_point - 1)));
            result.c *= pow(10, pattern_start - decimal_point - 1);
        }
        else
            sscanf(pattern, "%d", &result.b);
        reduce_fraction(&result);
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
            fraction inverted = decimal_to_fraction(inverse_value, true);
            if (inverted.c != 0)
            {
                // inverse of a + b / c is c / ( a *c + b )
                result.b = inverted.c;
                result.c = inverted.b + inverted.a * inverted.c;
            }
        }
    }
    return result;
}