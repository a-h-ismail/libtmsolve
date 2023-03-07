/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "string_tools.h"
#include "function.h"
#include <math.h>
void _set_var_data(math_expr *M)
{
    int i = 0, s_index, buffer_size = 16;
    var_op_data *vars = malloc(buffer_size * sizeof(var_op_data));
    m_subexpr *subexpr_ptr = M->subexpr_ptr;
    op_node *i_node;

    for (s_index = 0; s_index < M->subexpr_count; ++s_index)
    {
        i_node = subexpr_ptr[s_index].subexpr_nodes + subexpr_ptr[s_index].start_node;
        while (i_node != NULL)
        {
            if (i == buffer_size)
            {
                buffer_size *= 2;
                vars = realloc(vars, buffer_size * sizeof(var_op_data));
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
        vars = realloc(vars, i * sizeof(var_op_data));
        M->var_count = i;
        M->var_data = vars;
    }
    else
        free(vars);
}
// Function that sets all variables pointed to in the array with "value"
void set_variable(math_expr *math_struct, double complex value)
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
double derivative(char *arguments)
{
    math_expr *math_struct;
    arg_list *args;
    args = get_arguments(arguments);
    if (args->arg_count != 2)
    {
        error_handler("Incorrect use of derivation function, provide 2 arguments.", 1, 1, -1);
        return false;
    }
    double x, f_prime, fx1, fx2;

    x = calculate_expr(args->arguments[1], false);
    math_struct = parse_expr(args->arguments[0], true, false);
    // Solve for x
    set_variable(math_struct, x);
    fx1 = eval_math_expr(math_struct);
    // Solve for x + (small value)
    set_variable(math_struct, x + 1e-8);
    fx2 = eval_math_expr(math_struct);
    // get the derivative
    f_prime = (fx2 - fx1) / (1e-8);
    delete_math_expr(math_struct);
    free_arg_list(args, true);
    return f_prime;
}

double integrate(char *arguments)
{
    math_expr *M;
    arg_list *args = get_arguments(arguments);
    int n;
    double lower_bound, upper_bound, result, an, fn, rounds, tmp, delta;

    lower_bound = calculate_expr(args->arguments[0], false);
    upper_bound = calculate_expr(args->arguments[1], false);

    delta = upper_bound - lower_bound;
    if (delta < 0)
    {
        lower_bound = delta + lower_bound;
        delta = -delta;
    }

    // Compile the expression to the desired structure
    M = parse_expr(args->arguments[2], true, false);

    // Calculating the number of rounds
    rounds = ceil(delta) * 65536;

    if (rounds > 1e8)
        rounds = 1e8;
    // Simpson 3/8 formula:
    // 3h/8[(y0 + yn) + 3(y1 + y2 + y4 + y5 + … + yn-1) + 2(y3 + y6 + y9 + … + yn-3)]
    // First step: y0 + yn
    set_variable(M, lower_bound);
    result = eval_math_expr(M);
    set_variable(M, lower_bound + delta);
    result += eval_math_expr(M);
    if (isnan(result) == true)
    {
        error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
        delete_math_expr(M);
        return NAN;
    }
    for (n = 1, tmp = 0; n < rounds; ++n)
    {
        if (n % 3 == 0)
            continue;
        an = lower_bound + delta * n / rounds;
        set_variable(M, an);
        fn = eval_math_expr(M);
        if (isnan(fn) == true)
        {
            error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
            delete_math_expr(M);
            return NAN;
        }
        tmp += fn;
    }
    result += 3 * tmp;
    for (n = 3, tmp = 0; n < rounds; n += 3)
    {
        an = lower_bound + delta * n / rounds;
        set_variable(M, an);
        fn = eval_math_expr(M);
        if (isnan(fn) == true)
        {
            error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
            delete_math_expr(M);
            return NAN;
        }
        tmp += fn;
    }
    result += 2 * tmp;
    result *= 0.375 * (delta / rounds);
    delete_math_expr(M);
    free_arg_list(args, true);
    return result;
}
