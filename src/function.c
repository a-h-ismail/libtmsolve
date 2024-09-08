/*
Copyright (C) 2021-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "function.h"
#include "error_handler.h"
#include "evaluator.h"
#include "internals.h"
#include "m_errors.h"
#include "parser.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_complex.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

double complex tms_avg(tms_arg_list *args)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_EVALUATOR) == false)
        return NAN;

    double complex tmp, total = 0;
    for (int i = 0; i < args->count; ++i)
    {
        tmp = _tms_solve_e_unsafe(args->arguments[i], true);
        if (isnan(creal(tmp)))
            return NAN;

        total += tmp;
    }

    return total / args->count;
}

double complex tms_min(tms_arg_list *args)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_EVALUATOR) == false)
        return NAN;

    // Set min to the largest possible value so it would always be overwritten in the first iteration
    double complex min = INFINITY, tmp;

    for (int i = 0; i < args->count; ++i)
    {
        tmp = _tms_solve_e_unsafe(args->arguments[i], true);
        if (isnan(creal(tmp)))
            return NAN;

        if (cimag(tmp) != 0)
        {
            tms_save_error(TMS_EVALUATOR, ILLEGAL_COMPLEX_OP, EH_FATAL, NULL, 0);
            return NAN;
        }

        if (creal(min) > creal(tmp))
            min = tmp;
    }

    return min;
}

double complex tms_max(tms_arg_list *args)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_EVALUATOR) == false)
        return NAN;

    // Set max to the smallest possible value so it would always be overwritten in the first iteration
    double complex max = -INFINITY, tmp;

    for (int i = 0; i < args->count; ++i)
    {
        tmp = _tms_solve_e_unsafe(args->arguments[i], true);
        if (isnan(creal(tmp)))
            return NAN;

        if (cimag(tmp) != 0)
        {
            tms_save_error(TMS_EVALUATOR, ILLEGAL_COMPLEX_OP, EH_FATAL, NULL, 0);
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
    double complex value = _tms_solve_e_unsafe(args->arguments[0], true);
    if (isnan(creal(value)))
        return NAN;

    double complex base = _tms_solve_e_unsafe(args->arguments[1], true);
    if (isnan(creal(base)))
        return NAN;
    if (!tms_is_real(base))
    {
        tms_save_error(TMS_EVALUATOR, NO_COMPLEX_LOG_BASE, EH_FATAL, NULL, 0);
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
    result = _tms_solve_e_unsafe(args->arguments[0], true);
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
    if (_tms_validate_args_count(0, args->count, TMS_EVALUATOR) == false)
        return NAN;

    return tms_random_weight();
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

    double complex x, fx1, fx2;
    double f_prime;

    x = _tms_solve_e_unsafe(L->arguments[1], false);
    if (isnan(creal(x)))
    {
        tms_clear_errors(TMS_PARSER);
        tms_clear_errors(TMS_EVALUATOR);
        return NAN;
    }
    // Compile the expression to the desired structure
    tms_arg_list *tmp = tms_get_args("x");
    M = _tms_parse_expr_unsafe(L->arguments[0], ENABLE_LABELS, tmp);

    if (M == NULL)
    {
        tms_clear_errors(TMS_PARSER);
        return NAN;
    }

    if (!tms_is_deterministic(M))
    {
        tms_save_error(TMS_EVALUATOR, EXPR_NOT_DETERMINISTIC, EH_FATAL, NULL, 0);
        tms_delete_math_expr(M);
        return NAN;
    }

    // Scale epsilon with the dimensions of the required value.
    epsilon = x * epsilon;

    // Solve for x - epsilon
    x -= epsilon;
    tms_set_labels_values(M, &x);
    fx1 = _tms_evaluate_unsafe(M);
    // Solve for x + epsilon
    x += 2 * epsilon;
    tms_set_labels_values(M, &x);
    fx2 = _tms_evaluate_unsafe(M);
    tms_clear_errors(TMS_EVALUATOR);

    // get the derivative
    f_prime = (fx2 - fx1) / (2 * epsilon);
    if (isnan(f_prime))
        tms_save_error(TMS_EVALUATOR, NOT_DERIVABLE, EH_FATAL, NULL, 0);

    tms_delete_math_expr(M);
    return f_prime;
}

double complex tms_integrate(tms_arg_list *L)
{
    tms_math_expr *M;

    if (_tms_validate_args_count(3, L->count, TMS_EVALUATOR) == false)
        return NAN;

    int n;
    bool flip_result = false;
    double complex lower_bound, upper_bound, an;
    double result, rounds, delta, fn;

    lower_bound = _tms_solve_e_unsafe(L->arguments[0], false);
    upper_bound = _tms_solve_e_unsafe(L->arguments[1], false);
    if (isnan(creal(lower_bound)) || isnan(creal(upper_bound)))
    {
        tms_clear_errors(TMS_PARSER);
        tms_clear_errors(TMS_EVALUATOR);
        return NAN;
    }

    if (lower_bound == upper_bound)
        return 0;

    delta = upper_bound - lower_bound;
    // If delta is negative, flip lower and upper bounds then multiply the final answer by -1
    if (delta < 0)
    {
        flip_result = true;
        double tmp = lower_bound;
        lower_bound = upper_bound;
        upper_bound = tmp;
        delta = -delta;
    }

    // Compile the expression to the desired structure
    tms_arg_list *tmp = tms_get_args("x");
    M = _tms_parse_expr_unsafe(L->arguments[2], ENABLE_LABELS, tmp);

    if (M == NULL)
        return NAN;

    if (!tms_is_deterministic(M))
    {
        tms_save_error(TMS_EVALUATOR, EXPR_NOT_DETERMINISTIC, EH_FATAL, NULL, 0);
        tms_delete_math_expr(M);
        return NAN;
    }

    // Calculating the number of rounds
    rounds = ceil(delta) * 65536;

    if (rounds > 1e8)
        rounds = 1e8;
    // Simpson 3/8 formula:
    // 3h/8[(y0 + yn) + 3(y1 + y2 + y4 + y5 + … + yn-1) + 2(y3 + y6 + y9 + … + yn-3)]
    // First step: y0 + yn
    tms_set_labels_values(M, &lower_bound);
    result = _tms_evaluate_unsafe(M);
    lower_bound += delta;
    tms_set_labels_values(M, &lower_bound);
    lower_bound -= delta;
    result += _tms_evaluate_unsafe(M);
    // Clear errors collected from the previous evaluator calls
    tms_clear_errors(TMS_EVALUATOR);
    if (isnan(result))
    {
        tms_save_error(TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL, 0);
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
            tms_set_labels_values(M, &an);
            fn = _tms_evaluate_unsafe(M);
            if (isnan(fn) == true)
            {
                tms_save_error(TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL, 0);
                tms_delete_math_expr(M);
                return NAN;
            }
            part2 += fn;
            i = 1;
        }
        else
        {
            an = lower_bound + delta * n / rounds;
            tms_set_labels_values(M, &an);
            fn = _tms_evaluate_unsafe(M);
            if (isnan(fn) == true)
            {
                tms_save_error(TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL, 0);
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
    if (flip_result)
        result = -result;

    return result;
}
