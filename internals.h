/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef INTERNALS_H
#define INTERNALS_H
//Contains the declaration of general purpose functions used for the calculator operation, and the most used headers
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>
#include <complex.h>
#include <string.h>
typedef struct arg_list
{
    int arg_count;
    char **arguments;
} arg_list;
int error_handler(char *error, int arg, ...);
void error_print(char *, int);
int find_min(int a, int b);
#endif