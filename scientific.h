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
extern char *function_names[];
extern double (*s_function[])(double);
extern char *ext_function_names[];
extern double (*ext_s_function[])(char *);
int s_process(char *exp, int p);
double solve_s_exp(s_expression *subexps);
s_expression *scientific_compiler(char *exp, bool enable_variables);
double factorial(double value);
double scientific_interpreter(char *exp);
double solve_region(char *exp, int a, int b);
void scientific_complex_picker(char *exp);
void delete_s_exp(s_expression *subexps);
int priority_test(char operator1, char operator2);
void priority_fill(node *list, int op_count);
int subexp_start_at(s_expression *expression, int start, int current_s_exp, int mode);
int s_exp_ending(s_expression *expression, int end, int current_s_exp, int s_exp_count);
int32_t *find_factors(int32_t a);
void fraction_processor(char *exp, bool wasdecimal);
bool decimal_to_fraction(char *exp, bool inverse_process);
void factorize(char *exp);
void nroot_solver(char *exp);
void fraction_reducer(int32_t *f_v1, int32_t *f_v2, int32_t *v1, int32_t *v2);
// Complex functions
int c_process(char *exp, int p);
bool complex_stage1_solver(char *exp, int a, int b);
int complex_stage2_solver(char *exp, int a, int b);
double complex read_complex(char *exp, int a, int b);
int s_complex_print(char *exp, int a, int b, double complex value);
void complex_print(double complex value);
bool is_imaginary(char *exp, int a, int b);
void complex_interpreter(char *exp);
#endif