/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "string_tools.h"
#include "function.h"
#include <math.h>
// Generates a heap allocated AoS containing all variable members metadata and stores it in the math_struct
void *variable_nodes(math_expr *math_struct)
{
    int i = 0, subexpr_index, buffer_size = 50, buffer_step = 50;
    variable_data *variable_ptr = malloc(buffer_size * sizeof(variable_data));
    s_expression *subexpr_ptr = math_struct->subexps;
    node *i_node;

    for (subexpr_index = 0; subexpr_index < math_struct->subexpr_count; ++subexpr_index)
    {
        i_node = subexpr_ptr[subexpr_index].node_list + subexpr_ptr[subexpr_index].start_node;
        while (i_node != NULL)
        {
            if(i==buffer_size)
            {
                buffer_size += buffer_step;
                variable_ptr = realloc(variable_ptr, buffer_size * sizeof(variable_ptr));
            }
            // Case of variable left operand
            if (i_node->variable_operands & 0b1)
            {
                if (i_node->variable_operands & 0b100)
                    variable_ptr[i].is_negative = true;
                else
                    variable_ptr[i].is_negative = false;
                variable_ptr[i].pointer = &(i_node->left_operand);
                ++i;
            }
            if (i_node->variable_operands & 0b10)
            {
                if (i_node->variable_operands & 0b1000)
                    variable_ptr[i].is_negative = true;
                else
                    variable_ptr[i].is_negative = false;
                variable_ptr[i].pointer = &(i_node->right_operand);
                ++i;
            }
            i_node = i_node->next;
        }
    }
    variable_ptr = realloc(variable_ptr, i* sizeof(variable_ptr));
    math_struct->var_count = i;
    math_struct->variable_ptr = variable_ptr;
}
// Function that sets all double variables pointed to in the array with "value"
void replace_variable(variable_data *pointers, double value)
{
    int i;
    for (i = 0; i < g_var_count; ++i)
    {
        if (pointers[i].is_negative)
            *(pointers[i].pointer) = -value;
        else
            *(pointers[i].pointer) = value;
    }
}
// Function that calculates the derivative of f(x) for a specific value of x
double derivative(char *arguments)
{
    s_expression *exp_d;
    arg_list *args;
    variable_data *variables;
    args = get_arguments(arguments);
    if (args->arg_count != 2)
    {
        error_handler("Incorrect use of derivation function, provide 2 arguments.", 1, 1, -1);
        return false;
    }
    double x, f_prime, temp1, temp2;
    x = calculate_expr(args->arguments[1]);
    exp_d = parse_expr(args->arguments[0], true, false);
    variables = variable_nodes(exp_d);
    // Solve for x
    replace_variable(variables, x);
    temp1 = evaluate(exp_d);
    // Solve for x + (small value)
    replace_variable(variables, x + 1e-7);
    temp2 = evaluate(exp_d);
    // get the derivative
    f_prime = (temp2 - temp1) / (1e-7);
    free(variables);
    delete_subexps(exp_d);
    free_arg_list(args, true);
    return f_prime;
}
double calculate_function(char *exp, int a, int b, double x)
{
    char *temp = (char *)malloc(23 * (b - a + 1) * sizeof(char));
    double result;
    temp[b - a + 1] = '\0';
    strncpy(temp, exp + a, b - a + 1);
    var_to_val(temp, "x", x);
    result = calculate_expr(temp);
    free(temp);
    return result;
}
// Integrates a function in the specified bounds, using Simpson 3/8 formula
// Takes arguments as a char array
double integral_processor(char *arguments)
{
    arg_list *args;
    args = get_arguments(arguments);
    double lower_bound, upper_bound, result;
    lower_bound = calculate_expr(args->arguments[0]);
    upper_bound = calculate_expr(args->arguments[1]);
    result = integrate(args->arguments[2], lower_bound, upper_bound - lower_bound);
    free_arg_list(args, true);
    return result;
}
double integrate(char *exp, double a, double delta)
{
    int n;
    double integral, an, fn, rounds, temp;
    variable_data *var_array;
    s_expression *subexps;
    if (delta < 0)
    {
        a = delta + a;
        delta = -delta;
    }
    // Compile exp to the desired structure
    subexps = parse_expr(exp, true, false);
    var_array = variable_nodes(subexps);
    // Calculating rounds
    rounds = ceil(delta) * 16384;
    if (rounds > 1e7)
        rounds = 1e7;
    // Simpson 3/8 formula:
    // 3h/8[(y0 + yn) + 3(y1 + y2 + y4 + y5 + … + yn-1) + 2(y3 + y6 + y9 + … + yn-3)]
    // First step: y0 + yn
    replace_variable(var_array, a);
    integral = evaluate(subexps);
    replace_variable(var_array, a + delta);
    integral += evaluate(subexps);
    if (isnan(integral) == true)
    {
        error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
        free(subexps);
        free(var_array);
        return NAN;
    }
    for (n = 1, temp = 0; n < rounds; ++n)
    {
        if (n % 3 == 0)
            continue;
        an = a + delta * n / rounds;
        replace_variable(var_array, an);
        fn = evaluate(subexps);
        if (isnan(fn) == true)
        {
            error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
            free(subexps);
            free(var_array);
            return NAN;
        }
        temp += fn;
    }
    integral += 3 * temp;
    for (n = 3, temp = 0; n < rounds; n += 3)
    {
        an = a + delta * n / rounds;
        replace_variable(var_array, an);
        fn = evaluate(subexps);
        if (isnan(fn) == true)
        {
            error_handler("Error while calculating integral, make sure the function is defined on the integration interval.", 1, 1, -1);
            free(subexps);
            free(var_array);
            return NAN;
        }
        temp += fn;
    }
    integral += 2 * temp;
    integral *= 0.375 / rounds * delta;
    delete_subexps(subexps);
    free(var_array);
    return integral;
}
