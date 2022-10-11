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
extern int g_var_count;
extern double complex ans;
extern char *g_exp;
typedef struct node
{
    char operator;
    // Index of the operator in the expression
    int index;
    // Index of the node in the node array
    int node_index;
    // Used to store data about unknown nodes as follow:
    // b0:l_operand, b1:r_operand, b2:l_op_negative, b3:r_op_negative
    uint8_t variable_operands;
    // Node operator priority
    uint8_t priority;

    double LeftOperand, RightOperand, *node_result;
    struct node *next;
} node;
// Simple structure to hold pointer and sign of unknown members of an equation
typedef struct variable_data
{
    double *pointer;
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
           ^    ^
    solve_start |
               end_index
    */
    int expression_start, solve_start, end_index;
    // The index of the node at which the subexpression solving start
    int start_node;
    struct node *node_list;
    /*
    The result is a double pointer because the subexpression result is determined later (it links to nodes of other subexps or ans)
    Keep in mind the result is carried by the last node in order (the pointer points to the result pointer of last node).
    I thought about saving the last node index and using that, but subexp[current_subexp].node_list[last_node].node_result is too long
     */
    double **result;
    // function to execute on the final result
    double (*function_ptr)(double);
    bool last_exp;
} s_expression;
int error_handler(char *error, int arg, ...);
void error_print(char *, int);
int find_min(int a, int b);
int compare_subexps_depth(const void *a, const void *b);
#endif