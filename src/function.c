/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "string_tools.h"
#include "function.h"
#include <math.h>

bool _validate_args_count(int expected, int actual)
{
    if (expected == actual)
        return true;
    else if (expected > actual)
        tms_error_handler(TOO_FEW_ARGS, EH_SAVE, EH_FATAL_ERROR, -1);
    else if (expected < actual)
        tms_error_handler(TOO_MANY_ARGS, EH_SAVE, EH_FATAL_ERROR, -1);
    return false;
}
void _tms_set_var_data(tms_math_expr *M)
{
    int i = 0, s_index, buffer_size = 16;
    tms_var_operand *vars = malloc(buffer_size * sizeof(tms_var_operand));
    tms_math_subexpr *subexpr_ptr = M->subexpr_ptr;
    tms_op_node *i_node;

    for (s_index = 0; s_index < M->subexpr_count; ++s_index)
    {
        i_node = subexpr_ptr[s_index].nodes + subexpr_ptr[s_index].start_node;
        while (i_node != NULL)
        {
            if (i == buffer_size)
            {
                buffer_size *= 2;
                vars = realloc(vars, buffer_size * sizeof(tms_var_operand));
            }
            // Case of variable left operand
            if (i_node->var_metadata & 0b1)
            {
                if (i_node->var_metadata & 0b100)
                    vars[i].is_negative = true;
                else
                    vars[i].is_negative = false;
                vars[i].var_ptr = &(i_node->left_operand);
                ++i;
            }
            // Case of a variable right operand
            if (i_node->var_metadata & 0b10)
            {
                if (i_node->var_metadata & 0b1000)
                    vars[i].is_negative = true;
                else
                    vars[i].is_negative = false;
                vars[i].var_ptr = &(i_node->right_operand);
                ++i;
            }
            i_node = i_node->next;
        }
    }
    if (i != 0)
    {
        vars = realloc(vars, i * sizeof(tms_var_operand));
        M->var_count = i;
        M->var_data = vars;
    }
    else
        free(vars);
}
// Function that sets all variables pointed to in the array with "value"
void _tms_set_variable(tms_math_expr *math_struct, double complex value)
{
    int i;
    for (i = 0; i < math_struct->var_count; ++i)
    {
        if (math_struct->var_data[i].is_negative)
            *(math_struct->var_data[i].var_ptr) = -value;
        else
            *(math_struct->var_data[i].var_ptr) = value;
    }
}
// Function that calculates the derivative of f(x) for a specific value of x
double tms_derivative(char *arguments)
{
    tms_math_expr *M;
    tms_arg_list *args;
    args = tms_get_args(arguments);

    if (_validate_args_count(2, args->count) == false)
        return NAN;
    double x, f_prime, fx1, fx2;

    x = tms_solve_e(args->arguments[1], false);
    M = tms_parse_expr(args->arguments[0], true, false);

    if (M == NULL)
        return NAN;

    // Solve for x
    _tms_set_variable(M, x);
    fx1 = tms_evaluate(M);
    // Solve for x + (small value)
    _tms_set_variable(M, x + 1e-8);
    fx2 = tms_evaluate(M);
    // get the derivative
    f_prime = (fx2 - fx1) / (1e-8);
    tms_delete_math_expr(M);
    tms_free_arg_list(args, true);
    return f_prime;
}

double tms_integrate(char *arguments)
{
    tms_math_expr *M;
    tms_arg_list *args = tms_get_args(arguments);

    if (_validate_args_count(3, args->count) == false)
        return NAN;

    int n;
    double lower_bound, upper_bound, result, an, fn, rounds, delta;

    lower_bound = tms_solve_e(args->arguments[0], false);
    upper_bound = tms_solve_e(args->arguments[1], false);

    delta = upper_bound - lower_bound;
    if (delta < 0)
    {
        lower_bound = delta + lower_bound;
        delta = -delta;
    }

    // Compile the expression to the desired structure
    M = tms_parse_expr(args->arguments[2], true, false);

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
        tms_error_handler(INTEGRAl_UNDEFINED, EH_SAVE, EH_FATAL_ERROR, -1);
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
                tms_error_handler(INTEGRAl_UNDEFINED, EH_SAVE, EH_FATAL_ERROR, -1);
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
                tms_error_handler(INTEGRAl_UNDEFINED, EH_SAVE, EH_FATAL_ERROR, -1);
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
    tms_free_arg_list(args, true);
    return result;
}
