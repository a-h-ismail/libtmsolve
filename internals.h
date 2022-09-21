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
extern int g_nb_unknowns;
extern double complex ans;
typedef struct node
{
    char operator;
    // Index of the operator in the expression
    int index;
    // Index of the node in the node array
    int node_index;
    // Used to store data about unknown nodes as follow:
    // b0:l_op, b1:r_op, b2:l_opNegative, b3:r_opNegative
    uint8_t variable_operands, priority;
    double LeftOperand, RightOperand, *result;
    struct node *next;
} node;
// Simple structure to hold pointer and sign of unknown members of an equation
typedef struct variable_data
{
    double *pointer;
    bool isNegative;
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