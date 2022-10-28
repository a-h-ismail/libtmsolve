/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef FUNCTION_H
#define FUNCTION_H
#include "internals.h"
void *variable_nodes(math_expr *math_struct);
void replace_variable(variable_data *pointers, double value);
double calculate_function(char *exp, int a, int b, double x);
bool derivative_processor(char *exp);
double derivative(char *exp);
double integrate(char *exp, double a, double b);
double integral_processor(char *exp);
#endif