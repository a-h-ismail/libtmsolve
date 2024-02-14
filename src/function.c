/*
Copyright (C) 2021-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "parser.h"
#include "evaluator.h"
#include "string_tools.h"
#include "function.h"
#include "m_errors.h"
#include "internals.h"
#include "tms_complex.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

double complex tms_avg(tms_arg_list *args)
{
    if (args->count == 0)
        return NAN;

    double complex tmp, total = 0;
    for (int i = 0; i < args->count; ++i)
    {
        tmp = tms_solve(args->arguments[i]);
        if (isnan(creal(tmp)))
            return NAN;

        total += tmp;
    }

    return total / args->count;
}

double complex tms_min(tms_arg_list *args)
{
    if (args->count == 0)
        return NAN;

    double complex min = INFINITY, tmp;

    for (int i = 0; i < args->count; ++i)
    {
        tmp = tms_solve(args->arguments[i]);
        if (isnan(creal(tmp)))
            return NAN;

        if (cimag(tmp) != 0)
        {
            tms_error_handler(EH_SAVE, TMS_EVALUATOR, ILLEGAL_COMPLEX_OP, EH_FATAL, NULL);
            return NAN;
        }

        if (creal(min) > creal(tmp))
            min = tmp;
    }

    return min;
}

double complex tms_max(tms_arg_list *args)
{
    if (args->count == 0)
        return NAN;

    double complex max = INFINITY, tmp;

    for (int i = 0; i < args->count; ++i)
    {
        tmp = tms_solve(args->arguments[i]);
        if (isnan(creal(tmp)))
            return NAN;

        if (cimag(tmp) != 0)
        {
            tms_error_handler(EH_SAVE, TMS_EVALUATOR, ILLEGAL_COMPLEX_OP, EH_FATAL, NULL);
            return NAN;
        }

        if (creal(max) < creal(tmp))
            max = tmp;
    }

    return max;
}

double complex tms_logn(tms_arg_list *args)
{
    if (_tms_validate_args_count(2, args->count, TMS_EVALUATOR) == false)
        return NAN;
    double complex value = tms_solve(args->arguments[0]);
    if (isnan(creal(value)))
        return NAN;

    double complex base = tms_solve(args->arguments[1]);
    if (isnan(creal(base)))
        return NAN;
    if (!tms_is_real(base))
    {
        tms_error_handler(EH_SAVE, TMS_EVALUATOR, NO_COMPLEX_LOG_BASE, EH_FATAL, NULL);
        return NAN;
    }
    else
        return tms_cln(value) / log(base);
}

double complex tms_int(tms_arg_list *args)
{
    double real, imag;
    double complex result;

    if (_tms_validate_args_count(1, args->count, TMS_EVALUATOR) == false)
        return NAN;
    result = tms_solve(args->arguments[0]);
    real = creal(result);
    imag = cimag(result);
    if (isnan(real))
        return NAN;

    if (real > 0)
        real = floor(real);
    else
        real = ceil(real);

    if (imag > 0)
        imag = floor(imag);
    else
        imag = ceil(imag);

    return real + I * imag;
}

double complex tms_randint(tms_arg_list *args)
{
    if (_tms_validate_args_count(0, args->count, TMS_EVALUATOR) == false)
        return NAN;

    return (double)rand() * pow(-1, rand() & 1);
}

double complex tms_rand(tms_arg_list *args)
{
    if (_tms_validate_args_count(0, args->count, TMS_EVALUATOR) == false)
        return NAN;

    double decimal = rand();
    // Generate a decimal part
    decimal /= pow(10, ceil(log10(decimal)));

    return ((double)rand() + decimal) * pow(-1, rand() & 1);
}

double complex tms_base_n(tms_arg_list *args, int8_t base)
{
    if (_tms_validate_args_count(1, args->count, TMS_EVALUATOR) == false)
        return NAN;

    double complex value;
    bool is_complex = false;
    int end = strlen(args->arguments[0]);
    if (args->arguments[0][end - 1] == 'i')
    {
        is_complex = true;
        args->arguments[0][end - 1] = '\0';
    }
    value = _tms_read_value_simple(args->arguments[0], base);
    if (is_complex)
        value *= I;
    return value;
}

double complex tms_hex(tms_arg_list *L)
{
    return tms_base_n(L, 16);
}

double complex tms_oct(tms_arg_list *L)
{
    return tms_base_n(L, 8);
}

double complex tms_bin(tms_arg_list *L)
{
    return tms_base_n(L, 2);
}

// Function that calculates the derivative of f(x) for a specific value of x
double complex tms_derivative(tms_arg_list *L)
{
    tms_math_expr *M;
    double epsilon = 1e-9;

    if (_tms_validate_args_count(2, L->count, TMS_EVALUATOR) == false)
        return NAN;

    double x, f_prime, fx1, fx2;

    x = tms_solve_e(L->arguments[1], false);
    if (isnan(x))
    {
        tms_error_handler(EH_CLEAR, TMS_PARSER);
        tms_error_handler(EH_CLEAR, TMS_EVALUATOR);
        return NAN;
    }
    M = tms_parse_expr(L->arguments[0], true, false);

    if (M == NULL)
        return NAN;

    // Scale epsilon with the dimensions of the required value.
    epsilon = x * epsilon;

    // Solve for x
    tms_set_unknown(M, x - epsilon);
    fx1 = tms_evaluate(M);
    // Solve for x + epsilon
    tms_set_unknown(M, x + epsilon);
    fx2 = tms_evaluate(M);
    // get the derivative
    f_prime = (fx2 - fx1) / (2 * epsilon);
    tms_delete_math_expr(M);
    return f_prime;
}

double complex tms_integrate(tms_arg_list *L)
{
    tms_math_expr *M;

    if (_tms_validate_args_count(3, L->count, TMS_EVALUATOR) == false)
        return NAN;

    int n;
    double lower_bound, upper_bound, result, an, fn, rounds, delta;

    lower_bound = tms_solve_e(L->arguments[0], false);
    upper_bound = tms_solve_e(L->arguments[1], false);
    if (isnan(lower_bound) || isnan(upper_bound))
    {
        tms_error_handler(EH_CLEAR, TMS_PARSER);
        tms_error_handler(EH_CLEAR, TMS_EVALUATOR);
        return NAN;
    }

    delta = upper_bound - lower_bound;
    if (delta < 0)
    {
        lower_bound = delta + lower_bound;
        delta = -delta;
    }

    // Compile the expression to the desired structure
    M = tms_parse_expr(L->arguments[2], true, false);

    if (M == NULL)
        return NAN;

    // Calculating the number of rounds
    rounds = ceil(delta) * 65536;

    if (rounds > 1e8)
        rounds = 1e8;
    // Simpson 3/8 formula:
    // 3h/8[(y0 + yn) + 3(y1 + y2 + y4 + y5 + … + yn-1) + 2(y3 + y6 + y9 + … + yn-3)]
    // First step: y0 + yn
    tms_set_unknown(M, lower_bound);
    result = tms_evaluate(M);
    tms_set_unknown(M, lower_bound + delta);
    result += tms_evaluate(M);
    if (isnan(result) == true)
    {
        tms_error_handler(EH_SAVE, TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL);
        tms_delete_math_expr(M);
        return NAN;
    }

    double part1 = 0, part2 = 0;
    // Use i to determine when n is divisible by 3 without doing a mod 3
    int i = 1;

    for (n = 1; n < rounds; ++n)
    {
        if (i == 3)
        {
            an = lower_bound + delta * n / rounds;
            tms_set_unknown(M, an);
            fn = tms_evaluate(M);
            if (isnan(fn) == true)
            {
                tms_error_handler(EH_SAVE, TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL);
                tms_delete_math_expr(M);
                return NAN;
            }
            part2 += fn;
            i = 1;
        }
        else
        {
            an = lower_bound + delta * n / rounds;
            tms_set_unknown(M, an);
            fn = tms_evaluate(M);
            if (isnan(fn) == true)
            {
                tms_error_handler(EH_SAVE, TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL);
                tms_delete_math_expr(M);
                return NAN;
            }
            part1 += fn;
            ++i;
        }
    }
    result += 3 * part1 + 2 * part2;

    result *= 0.375 * (delta / rounds);
    tms_delete_math_expr(M);
    return result;
}
