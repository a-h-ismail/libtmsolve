/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "string_tools.h"
#include "function.h"
#include <math.h>
//Generates a heap allocated array containing all unknown members metadata (location,sign)
variable_data *variable_nodes(s_expression *subexps)
{
    int i = 0, current_subexp = 0;
    variable_data *variable_list = malloc(g_var_count * sizeof(variable_data));
    node *i_node;
    do
    {
        i_node = subexps[current_subexp].node_list + subexps[current_subexp].start_node;
        while (i_node != NULL)
        {
            //Case of variable left operand
            if (i_node->variable_operands & 0b1)
            {
                if (i_node->variable_operands & 0b100)
                    variable_list[i].is_negative = true;
                else
                    variable_list[i].is_negative = false;
                variable_list[i].pointer = &(i_node->LeftOperand);
                ++i;
            }
            if (i_node->variable_operands & 0b10)
            {
                if (i_node->variable_operands & 0b1000)
                    variable_list[i].is_negative = true;
                else
                    variable_list[i].is_negative = false;
                variable_list[i].pointer = &(i_node->RightOperand);
                ++i;
            }
            i_node = i_node->next;
        }
        ++current_subexp;
    } while (subexps[current_subexp - 1].last_exp == false);
    return variable_list;
}
//Function that sets all double variables pointed to in the array with "value"
void replace_unknowns(variable_data *pointers, double value)
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
//Function that calculates the derivative
bool derivative(char *exp)
{
    int j=0, next, open1 = 4, open2, close1, close2;
    double n1, n2, fn1, fn2, der;
    char *temp;
    //The function must receive an expression of the form d/dx(f(x))(n1) where n1 is the value where we want to calculate the derivative
    //We are looking for the parenthesis that closes d/dx
    if (exp[open1] == '(')
        close1 = find_closing_parenthesis(exp, open1);
    else
    {
        error_handler("First parameter of derivation missing.", 1, 1, -1);
        return false;
    }
    if (exp[close1 + 1] == '(')
    {
        open2 = close1 + 1;
        close2 = find_closing_parenthesis(exp, open2);
    }
    else
    {
        error_handler("Second parameter of derivation missing.", 1, 1, -1);
        return false;
    }
    //Checking for nested derivation
    next = s_search(exp, "d/dx", open1 + 1);
    while (next != -1 && next < close1)
    {
        if (derivative(exp + next) == false)
            return false;
        else
        {
            close1 = find_closing_parenthesis(exp, open1);
            open2 = close1 + 1;
            close2 = find_closing_parenthesis(exp, open2);
            next = s_search(exp, "d/dx", next);
        }
    }
    next = s_search(exp, "int", open1 + 1);
    while (next != -1 && next < j)
    {
        if (integral_processor(exp + next) == false)
            return false;
        else
        {
            close1 = find_closing_parenthesis(exp, open1);
            open2 = close1 + 1;
            close2 = find_closing_parenthesis(exp, open2);
            next = s_search(exp, "int", open1 + 1);
        }
    }
    //Calculating n1
    n1 = solve_region(exp, open2 + 1, close2 - 1);
    if (isnan(n1) == true)
        return false;
    n2 = n1 + n1 / 1e6;
    //Calculating fn1
    temp = (char *)malloc(23*(close1 - open1) * sizeof(char));
    strncpy(temp, exp + open1 + 1, close1 - open1 - 1);
    temp[close1 - open1 - 1] = '\0';
    var_to_val(temp, "x", n1);
    fn1 = scientific_interpreter(temp, true);
    if (isnan(fn1))
        ;
    return false;
    //Calculating fn2
    strncpy(temp, exp + open1 + 1, close1 - open1 - 1);
    temp[close1 - open1 - 1] = '\0';
    var_to_val(temp, "x", n2);
    fn2 = scientific_interpreter(temp, true);
    if (isnan(fn2))
        return false;
    der = (fn2 - fn1) / (n2 - n1);
    //workaround to compensate for approximations
    if (fabs(der - round(der)) < 1e-5)
        der = round(der);
    value_printer(exp, 0, close2, der);
    free(temp);
    return true;
}
double calculate_function(char *exp, int a, int b, double x)
{
    char *temp = (char *)malloc(23*(b - a + 1) * sizeof(char));
    double result;
    temp[b - a + 1] = '\0';
    strncpy(temp, exp + a, b - a + 1);
    var_to_val(temp, "x", x);
    result = scientific_interpreter(temp, true);
    free(temp);
    return result;
}
bool integral_processor(char *exp)
{
    int open1, open2, close1, close2, n, semicolon = 0, start = 0, next;
    double a, b, integral;
    char *temp;
    start = s_search(exp, "int", 0);
    while (start != -1)
    {
        open1 = start + 3;
        //Format:int(a;b)(f(x))
        //close1 is the index of the first closing parenthesis, close2 is the second one.
        if (exp[open1] == '(')
            close1 = find_closing_parenthesis(exp, open1);
        else
        {
            error_handler("First parameter of integration missing.", 1, 1, -1);
            return false;
        }
        if (exp[close1 + 1] == '(')
        {
            open2 = close1 + 1;
            close2 = find_closing_parenthesis(exp, open2);
        }
        else
        {
            error_handler("Second parameter of integration missing.", 1, 1, -1);
            return false;
        }
        next = s_search(exp, "int", open2 + 1);
        while (next != -1 && next < close2)
        {
            if (integral_processor(exp + next) == false)
                return false;
            close2 = find_closing_parenthesis(exp, open2);
            next = s_search(exp, "int", next);
        }
        next = s_search(exp, "d/dx", open2 + 1);
        while (next != -1 && next < close2)
        {
            if (derivative(exp + next) == false)
                return false;
            close2 = find_closing_parenthesis(exp, open2);
            next = s_search(exp, "d/dx", next);
        }
        //Locating the semicolon
        for (n = open1 + 1; n < close1; ++n)
            if (exp[n] == ';')
            {
                semicolon = n;
                break;
            }
        if (semicolon == 0)
        {
            error_handler("Missing semicolon in integral region parameter.", 1, 1, -1);
            return false;
        }
        //Getting a and b
        a = solve_region(exp, open1 + 1, semicolon - 1);
        b = solve_region(exp, semicolon + 1, close1 - 1);
        if (isnan(a) || isnan(b))
        {
            error_handler("Error while calculating integration interval.", 1, 1, -1);
            return false;
        }
        if (a == b)
        {
            value_printer(exp, start, close2, 0);
            start = s_search(exp, "int", open2 + 1);
            continue;
        }
        //Copy the function to integrate to temp
        temp = (char *)malloc(23*(close2 - open2) * sizeof(char));
        strncpy(temp, exp + open2 + 1, close2 - open2 - 1);
        temp[close2 - open2 - 1] = '\0';
        integral = integrate(temp, a, b - a);
        if (a > b)
            integral = -integral;
        value_printer(exp, start, close2, integral);
        start = s_search(exp, "int", start);
        free(temp);
    }
    return true;
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
    //Compile exp to the desired structure
    subexps = scientific_compiler(exp,true);
    var_array = variable_nodes(subexps);
    //Calculating rounds
    rounds = ceil(delta) * 4096;
    if (rounds > 1e6)
        rounds = 1e6;
    //Simpson 3/8 formula:
    //3h/8[(y0 + yn) + 3(y1 + y2 + y4 + y5 + … + yn-1) + 2(y3 + y6 + y9 + … + yn-3)]
    //First step: y0 + yn
    replace_unknowns(var_array, a);
    integral = solve_s_exp(subexps);
    replace_unknowns(var_array, a + delta);
    integral += solve_s_exp(subexps);
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
        replace_unknowns(var_array, an);
        fn = solve_s_exp(subexps);
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
        replace_unknowns(var_array, an);
        fn = solve_s_exp(subexps);
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
    free(subexps);
    free(var_array);
    return integral;
}
