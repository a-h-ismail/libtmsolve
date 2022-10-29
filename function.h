/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef FUNCTION_H
#define FUNCTION_H
#include "internals.h"
void set_variable_ptr(math_expr *math_struct);
void set_variable(math_expr *math_struct, double complex value);
bool derivative_processor(char *exp);
double derivative(char *exp);
double integrate(char *exp, double a, double b);
double integral_processor(char *exp);
#endif