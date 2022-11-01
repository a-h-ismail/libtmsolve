/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef INTERNALS_H
#define INTERNALS_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>
#include <complex.h>
#include <string.h>
extern double complex ans;
extern char *g_exp;
typedef struct arg_list
{
    int arg_count;
    char **arguments;
} arg_list;
typedef struct node
{
    char operator;
    // Index of the operator in the expression
    int operator_index;
    // Index of the node in the node array
    int node_index;
    // Used to store data about variable operands nodes as follow:
    // b0:left_operand, b1:right_operand, b2:l_op_negative, b3:r_op_negative
    uint8_t var_metadata;
    // Node operator priority
    uint8_t priority;

    double complex left_operand, right_operand, *node_result;
    struct node *next;
} node;
// Simple structure to hold pointer and sign of unknown members of an equation
typedef struct variable_data
{
    double complex *pointer;
    bool is_negative;
} variable_data;
typedef struct s_expression
{
    int op_count, depth;
    /*
    expression start
        |
        v
        cos(pi/3)
            ^  ^
    solve_start |
               solve_end
    */
    int expression_start, solve_start, solve_end;
    // The index of the node at which the subexpression parsing starts
    int start_node;
    struct node *node_list;
    /*
    The result is a double pointer because the subexpression result is determined later (it links to nodes of other subexps or ans)
    Keep in mind the result is carried by the last node in order (the pointer points to the result pointer of last node).
    */
    double complex **result;
    // Function to execute on the final result
    double (*function_ptr)(double);
    // Complex function to execute
    double complex (*cmplx_function_ptr)(double complex);
    // Extended function to execute
    double (*ext_function_ptr)(char *);
    // Enables execution of special function, allows optimizing of nested extended functions like integration
    bool execute_extended;
} s_expression;
// The standalone structure to hold all of an expression's metadata
typedef struct math_expr
{
    // The subexpressions forming the math expression after parsing
    s_expression *subexpr_ptr;
    // Number of subexpressions
    int subexpr_count;
    // Variable operands count
    int var_count;
    variable_data *variable_ptr;
    // Answer of the expression
    double complex answer;
    // Set to true if the expression was parsed with complex enabled
    bool enable_complex;
} math_expr;

// Simple structure to store a fraction of the form a + b / c
typedef struct fraction
{
    // value = a + b / c
    double a, b, c;
} fraction;
int error_handler(char *error, int arg, ...);
void error_print(char *, int);
int find_min(int a, int b);
int compare_subexps_depth(const void *a, const void *b);
#endif