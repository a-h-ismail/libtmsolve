/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "string_tools.h"
#include "function.h"
#include <math.h>
// Generates a heap allocated AoS containing all variable members metadata and stores it in the math_struct
void set_variable_ptr(math_expr *math_struct)
{
    int i = 0, subexpr_index, buffer_size = 50, buffer_step = 50;
    variable_data *variable_ptr = malloc(buffer_size * sizeof(variable_data));
    s_expression *subexpr_ptr = math_struct->subexpr_ptr;
    node *i_node;

    for (subexpr_index = 0; subexpr_index < math_struct->subexpr_count; ++subexpr_index)
    {
        i_node = subexpr_ptr[subexpr_index].node_list + subexpr_ptr[subexpr_index].start_node;
        while (i_node != NULL)
        {
            if (i == buffer_size)
            {
                buffer_size += buffer_step;
                variable_ptr = realloc(variable_ptr, buffer_size * sizeof(variable_ptr));
            }
            // Case of variable left operand
            if (i_node->var_metadata & 0b1)
            {
                if (i_node->var_metadata & 0b100)
                    variable_ptr[i].is_negative = true;
                else
                    variable_ptr[i].is_negative = false;
                variable_ptr[i].pointer = &(i_node->left_operand);
                ++i;
            }
            if (i_node->var_metadata & 0b10)
            {
                if (i_node->var_metadata & 0b1000)
                    variable_ptr[i].is_negative = true;
                else
                    variable_ptr[i].is_negative = false;
                variable_ptr[i].pointer = &(i_node->right_operand);
                ++i;
            }
            i_node = i_node->next;
        }
    }
    variable_ptr = realloc(variable_ptr, i * sizeof(variable_ptr));
    math_struct->var_count = i;
    math_struct->variable_ptr = variable_ptr;
}
// Function that sets all variables pointed to in the array with "value"
void set_variable(math_expr *math_struct, double complex value)
{
    int i;
    for (i = 0; i < math_struct->var_count; ++i)
    {
        if (math_struct->variable_ptr[i].is_negative)
            *(math_struct->variable_ptr[i].pointer) = -value;
        else
            *(math_struct->variable_ptr[i].pointer) = value;
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
    // Perform implied multiplication because the special function was skipped
    implicit_multiplication(&(args->arguments[1]));
    
    x = calculate_expr(args->arguments[1], false);
    math_struct = parse_expr(args->arguments[0], true, false);
    // Solve for x
    set_variable(math_struct, x);
    fx1 = evaluate(math_struct);
    // Solve for x + (small value)
    set_variable(math_struct, x + 1e-8);
    fx2 = evaluate(math_struct);
    // get the derivative
    f_prime = (fx2 - fx1) / (1e-8);
    delete_math_expr(math_struct);
    free_arg_list(args, true);
    return f_prime;
}

double integral_processor(char *arguments)
{
    math_expr *math_struct;
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
    // Perform implied multiplication because the special function was skipped
    implicit_multiplication(&(args->arguments[2]));

    // Compile the expression to the desired structure
    math_struct = parse_expr(args->arguments[2], true, false);

    // Calculating the number of rounds
    rounds = ceil(delta) * 16384000;
    if (rounds > 1e7)
        rounds = 1e7;
    // Simpson 3/8 formula:
    // 3h/8[(y0 + yn) + 3(y1 + y2 + y4 + y5 + … + yn-1) + 2(y3 + y6 + y9 + … + yn-3)]
    // First step: y0 + yn
    set_variable(math_struct, lower_bound);
    result = evaluate(math_struct);
    set_variable(math_struct, lower_bound + delta);
    result += evaluate(math_struct);
    if (isnan(result) == true)
    {
        error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
        delete_math_expr(math_struct);
        return NAN;
    }
    for (n = 1, tmp = 0; n < rounds; ++n)
    {
        if (n % 3 == 0)
            continue;
        an = lower_bound + delta * n / rounds;
        set_variable(math_struct, an);
        fn = evaluate(math_struct);
        if (isnan(fn) == true)
        {
            error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
            delete_math_expr(math_struct);
            return NAN;
        }
        tmp += fn;
    }
    result += 3 * tmp;
    for (n = 3, tmp = 0; n < rounds; n += 3)
    {
        an = lower_bound + delta * n / rounds;
        set_variable(math_struct, an);
        fn = evaluate(math_struct);
        if (isnan(fn) == true)
        {
            error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
            delete_math_expr(math_struct);
            return NAN;
        }
        tmp += fn;
    }
    result += 2 * tmp;
    result *= 0.375 * (delta / rounds);
    delete_math_expr(math_struct);
    free_arg_list(args, true);
    return result;
}
