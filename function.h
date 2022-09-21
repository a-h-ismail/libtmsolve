/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef FUNCTION_H
#define FUNCTION_H
double calculate_function(char *exp, int a, int b, double x);
bool derivative_processor(char *exp);
bool derivative(char *exp);
double integrate(char *exp, double a, double b);
bool integral_processor(char *exp);
#endif