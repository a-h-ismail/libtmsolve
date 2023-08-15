/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "string_tools.h"
#include "function.h"
#include <math.h>
#include <time.h>

bool _validate_args_count(int expected, int actual)
{
    if (expected == actual)
        return true;
    else if (expected > actual)
        tms_error_handler(EH_SAVE, TOO_FEW_ARGS, EH_FATAL_ERROR, -1);
    else if (expected < actual)
        tms_error_handler(EH_SAVE, TOO_MANY_ARGS, EH_FATAL_ERROR, -1);
    return false;
}

double complex tms_int(tms_arg_list *args)
{
    double real, imag;
    double complex result;

    if (_validate_args_count(1, args->count) == false)
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

double complex tms_rand(tms_arg_list *args)
{
    if (_validate_args_count(0, args->count) == false)
        return NAN;

    double tmp = rand();
    // Generate a decimal part
    tmp /= pow(10, ceil(log10(tmp)));

    return ((double)rand() + tmp) * pow(-1, rand());
}

double complex tms_base_n(tms_arg_list *args, int8_t base)
{
    if (_validate_args_count(1, args->count) == false)
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

    if (_validate_args_count(2, L->count) == false)
        return NAN;

    double x, f_prime, fx1, fx2;

    x = tms_solve_e(L->arguments[1], false);
    if (isnan(x))
    {
        tms_error_handler(EH_CLEAR, EH_MAIN_DB);
        return NAN;
    }
    M = tms_parse_expr(L->arguments[0], true, false);

    if (M == NULL)
        return NAN;

    // Scale epsilon with the dimensions of the required value.
    epsilon = x * epsilon;

    // Solve for x
    _tms_set_variable(M, x - epsilon);
    fx1 = tms_evaluate(M);
    // Solve for x + epsilon
    _tms_set_variable(M, x + epsilon);
    fx2 = tms_evaluate(M);
    // get the derivative
    f_prime = (fx2 - fx1) / (2 * epsilon);
    tms_delete_math_expr(M);
    return f_prime;
}

double complex tms_integrate(tms_arg_list *L)
{
    tms_math_expr *M;

    if (_validate_args_count(3, L->count) == false)
        return NAN;

    int n;
    double lower_bound, upper_bound, result, an, fn, rounds, delta;

    lower_bound = tms_solve_e(L->arguments[0], false);
    upper_bound = tms_solve_e(L->arguments[1], false);
    if (isnan(lower_bound) || isnan(upper_bound))
    {
        tms_error_handler(EH_CLEAR, EH_MAIN_DB);
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
    _tms_set_variable(M, lower_bound);
    result = tms_evaluate(M);
    _tms_set_variable(M, lower_bound + delta);
    result += tms_evaluate(M);
    if (isnan(result) == true)
    {
        tms_error_handler(EH_SAVE, INTEGRAl_UNDEFINED, EH_FATAL_ERROR, -1);
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
            _tms_set_variable(M, an);
            fn = tms_evaluate(M);
            if (isnan(fn) == true)
            {
                tms_error_handler(EH_SAVE, INTEGRAl_UNDEFINED, EH_FATAL_ERROR, -1);
                tms_delete_math_expr(M);
                return NAN;
            }
            part2 += fn;
            i = 1;
        }
        else
        {
            an = lower_bound + delta * n / rounds;
            _tms_set_variable(M, an);
            fn = tms_evaluate(M);
            if (isnan(fn) == true)
            {
                tms_error_handler(EH_SAVE, INTEGRAl_UNDEFINED, EH_FATAL_ERROR, -1);
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
