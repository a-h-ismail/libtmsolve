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

int _tms_avg(tms_arg_list *args, tms_arg_list *labels, double complex *result)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_EVALUATOR) == false)
        return -1;

    double complex tmp, total = 0;
    for (int i = 0; i < args->count; ++i)
    {
        tmp = tms_solve_e(args->arguments[i], NO_LOCK | ENABLE_CMPLX, labels);
        if (isnan(creal(tmp)))
            return -1;

        total += tmp;
    }

    *result = total / args->count;
    return 0;
}

int _tms_min(tms_arg_list *args, tms_arg_list *labels, double complex *result)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_EVALUATOR) == false)
        return -1;

    // Set min to the largest possible value so it would always be overwritten in the first iteration
    double complex min = INFINITY, tmp;

    for (int i = 0; i < args->count; ++i)
    {
        tmp = tms_solve_e(args->arguments[i], NO_LOCK | ENABLE_CMPLX, labels);
        if (isnan(creal(tmp)))
            return -1;

        if (cimag(tmp) != 0)
        {
            tms_save_error(TMS_EVALUATOR, ILLEGAL_COMPLEX_OP, EH_FATAL, NULL, 0);
            return -1;
        }

        if (creal(min) > creal(tmp))
            min = tmp;
    }

    *result = min;
    return 0;
}

int _tms_max(tms_arg_list *args, tms_arg_list *labels, double complex *result)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_EVALUATOR) == false)
        return -1;

    // Set max to the smallest possible value so it would always be overwritten in the first iteration
    double complex max = -INFINITY, tmp;

    for (int i = 0; i < args->count; ++i)
    {
        tmp = tms_solve_e(args->arguments[i], NO_LOCK | ENABLE_CMPLX, labels);
        if (isnan(creal(tmp)))
            return -1;

        if (cimag(tmp) != 0)
        {
            tms_save_error(TMS_EVALUATOR, ILLEGAL_COMPLEX_OP, EH_FATAL, NULL, 0);
            return -1;
        }

        if (creal(max) < creal(tmp))
            max = tmp;
    }

    *result = max;
    return 0;
}

int _tms_logn(tms_arg_list *args, tms_arg_list *labels, double complex *result)
{
    if (_tms_validate_args_count(2, args->count, TMS_EVALUATOR) == false)
        return -1;
    double complex value = tms_solve_e(args->arguments[0], NO_LOCK | ENABLE_CMPLX, labels);
    if (isnan(creal(value)))
        return -1;

    double complex base = tms_solve_e(args->arguments[1], NO_LOCK | ENABLE_CMPLX, labels);
    if (isnan(creal(base)))
        return -1;
    if (!tms_is_real(base))
    {
        tms_save_error(TMS_EVALUATOR, NO_COMPLEX_LOG_BASE, EH_FATAL, NULL, 0);
        return -1;
    }
    else
        *result = tms_cln(value) / log(base);
    return 0;
}

int _tms_int(tms_arg_list *args, tms_arg_list *labels, double complex *result)
{
    double real, imag;
    double complex tmp;

    if (_tms_validate_args_count(1, args->count, TMS_EVALUATOR) == false)
        return -1;
    tmp = tms_solve_e(args->arguments[0], NO_LOCK | ENABLE_CMPLX, labels);
    real = creal(tmp);
    imag = cimag(tmp);
    if (isnan(real))
        return -1;

    if (real > 0)
        real = floor(real);
    else
        real = ceil(real);

    if (imag > 0)
        imag = floor(imag);
    else
        imag = ceil(imag);

    *result = real + I * imag;
    return 0;
}

int _tms_rand(tms_arg_list *args, tms_arg_list *labels, double complex *result)
{
    if (_tms_validate_args_count(0, args->count, TMS_EVALUATOR) == false)
        return -1;

    *result = tms_random_weight();
    return 0;
}

int tms_base_n(tms_arg_list *args, int8_t base, double complex *result)
{
    if (_tms_validate_args_count(1, args->count, TMS_EVALUATOR) == false)
        return -1;

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
    *result = value;
    return 0;
}

int _tms_hex(tms_arg_list *L, tms_arg_list *labels, double complex *result)
{
    return tms_base_n(L, 16, result);
}

int _tms_oct(tms_arg_list *L, tms_arg_list *labels, double complex *result)
{
    return tms_base_n(L, 8, result);
}

int _tms_bin(tms_arg_list *L, tms_arg_list *labels, double complex *result)
{
    return tms_base_n(L, 2, result);
}

// Function that calculates the derivative of f(x) for a specific value of x
int _tms_derivative(tms_arg_list *L, tms_arg_list *labels, double complex *result)
{
    tms_math_expr *M;
    double epsilon = 1e-9;

    if (_tms_validate_args_count(2, L->count, TMS_EVALUATOR) == false)
        return -1;

    if (labels != NULL && tms_find_str_in_array("x", labels->arguments, labels->count, TMS_NOFUNC) != -1)
    {
        tms_save_error(TMS_EVALUATOR, X_NOT_ALLOWED, EH_FATAL, NULL, -1);
        return -1;
    }

    double complex x, fx1, fx2;
    double f_prime;

    x = tms_solve_e(L->arguments[1], NO_LOCK | ENABLE_CMPLX, labels);
    if (isnan(creal(x)))
    {
        tms_clear_errors(TMS_PARSER);
        tms_clear_errors(TMS_EVALUATOR);
        return -1;
    }
    // Compile the expression to the desired structure
    tms_arg_list *tmp = tms_get_args("x");
    M = tms_parse_expr(L->arguments[0], NO_LOCK, tmp);

    if (M == NULL)
    {
        tms_clear_errors(TMS_PARSER);
        return -1;
    }

    if (!tms_is_deterministic(M))
    {
        tms_save_error(TMS_EVALUATOR, EXPR_NOT_DETERMINISTIC, EH_FATAL, NULL, 0);
        tms_delete_math_expr(M);
        return -1;
    }

    // Scale epsilon with the dimensions of the required value.
    epsilon = x * epsilon;

    // Solve for x - epsilon
    x -= epsilon;
    tms_set_labels_values(M, &x);
    fx1 = tms_evaluate(M, NO_LOCK);
    // Solve for x + epsilon
    x += 2 * epsilon;
    tms_set_labels_values(M, &x);
    fx2 = tms_evaluate(M, NO_LOCK);
    tms_clear_errors(TMS_EVALUATOR);

    // get the derivative
    f_prime = (fx2 - fx1) / (2 * epsilon);
    if (isnan(f_prime))
        tms_save_error(TMS_EVALUATOR, NOT_DERIVABLE, EH_FATAL, NULL, 0);

    tms_delete_math_expr(M);
    *result = f_prime;
    return 0;
}

int _tms_integrate(tms_arg_list *L, tms_arg_list *labels, double complex *result)
{
    tms_math_expr *M;

    if (_tms_validate_args_count(3, L->count, TMS_EVALUATOR) == false)
        return -1;
    if (labels != NULL && tms_find_str_in_array("x", labels->arguments, labels->count, TMS_NOFUNC) != -1)
    {
        tms_save_error(TMS_EVALUATOR, X_NOT_ALLOWED, EH_FATAL, NULL, -1);
        return -1;
    }

    int n;
    bool flip_result = false;
    double complex lower_bound, upper_bound, an;
    double integration_ans, rounds, delta, fn;

    lower_bound = tms_solve_e(L->arguments[0], NO_LOCK | ENABLE_CMPLX, labels);
    upper_bound = tms_solve_e(L->arguments[1], NO_LOCK | ENABLE_CMPLX, labels);
    if (isnan(creal(lower_bound)) || isnan(creal(upper_bound)))
    {
        tms_clear_errors(TMS_PARSER | TMS_EVALUATOR);
        return -1;
    }

    if (lower_bound == upper_bound)
    {
        *result = 0;
        return 0;
    }

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
    M = tms_parse_expr(L->arguments[2], NO_LOCK, tmp);

    if (M == NULL)
        return -1;

    if (!tms_is_deterministic(M))
    {
        tms_save_error(TMS_EVALUATOR, EXPR_NOT_DETERMINISTIC, EH_FATAL, NULL, 0);
        tms_delete_math_expr(M);
        return -1;
    }

    // Calculating the number of rounds
    rounds = ceil(delta) * 65536;

    if (rounds > 1e8)
        rounds = 1e8;
    // Simpson 3/8 formula:
    // 3h/8[(y0 + yn) + 3(y1 + y2 + y4 + y5 + … + yn-1) + 2(y3 + y6 + y9 + … + yn-3)]
    // First step: y0 + yn
    tms_set_labels_values(M, &lower_bound);
    integration_ans = tms_evaluate(M, NO_LOCK);
    lower_bound += delta;
    tms_set_labels_values(M, &lower_bound);
    lower_bound -= delta;
    integration_ans += tms_evaluate(M, NO_LOCK);
    // Clear errors collected from the previous evaluator calls
    tms_clear_errors(TMS_EVALUATOR);
    if (isnan(integration_ans))
    {
        tms_save_error(TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL, 0);
        tms_delete_math_expr(M);
        return -1;
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
            fn = tms_evaluate(M, NO_LOCK);
            if (isnan(fn) == true)
            {
                tms_save_error(TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL, 0);
                tms_delete_math_expr(M);
                return -1;
            }
            part2 += fn;
            i = 1;
        }
        else
        {
            an = lower_bound + delta * n / rounds;
            tms_set_labels_values(M, &an);
            fn = tms_evaluate(M, NO_LOCK);
            if (isnan(fn) == true)
            {
                tms_save_error(TMS_EVALUATOR, INTEGRAl_UNDEFINED, EH_FATAL, NULL, 0);
                tms_delete_math_expr(M);
                return -1;
            }
            part1 += fn;
            ++i;
        }
    }
    integration_ans += 3 * part1 + 2 * part2;

    integration_ans *= 0.375 * (delta / rounds);
    tms_delete_math_expr(M);
    if (flip_result)
        integration_ans = -integration_ans;

    *result = integration_ans;
    return 0;
}
