/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef SCIENTIFIC_H
#define SCIENTIFIC_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include "internals.h"
typedef struct int_factor
{
    int factor;
    int power;
} int_factor;
extern char *function_name[];
extern double (*math_function[])(double);
extern char *ext_function_name[];
extern double (*ext_math_function[])(char *);
int s_process(char *expr, int p);
double complex evaluate(math_expr *math_struct);
math_expr *parse_expr(char *expr, bool enable_variables, bool enable_complex);
double factorial(double value);
double complex calculate_expr(char *expr, bool enable_complex);
double evaluate_region(char *expr, int a, int b);
void scientific_complex_picker(char *expr);
void delete_math_expr(math_expr *math_struct);
int priority_test(char operator1, char operator2);
void priority_fill(node *list, int op_count);
int subexp_start_at(s_expression *expression, int start, int current_s_exp, int mode);
int s_exp_ending(s_expression *expression, int end, int current_s_exp, int s_exp_count);
int_factor *find_factors(int32_t value);
void reduce_fraction(fraction *fraction_r);
fraction decimal_to_fraction(double value, bool inverse_process);
void factorize(char *expr);
void nroot_solver(char *expr);
void fraction_reducer(int32_t *f_v1, int32_t *f_v2, int32_t *v1, int32_t *v2);
// Complex functions
int c_process(char *expr, int p);
bool complex_stage1_solver(char *expr, int a, int b);
int complex_stage2_solver(char *expr, int a, int b);
double complex read_complex(char *expr, int a, int b);
int s_complex_print(char *expr, int a, int b, double complex value);
void complex_print(double complex value);
bool is_imaginary(char *expr, int a, int b);
void complex_interpreter(char *expr);
#endif