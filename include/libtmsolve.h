/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef TMSOLVE_H
#define TMSOLVE_H
#include "libtmsolve/scientific.h"
#include "libtmsolve/string_tools.h"
#include "libtmsolve/internals.h"
#include "libtmsolve/function.h"
#include "libtmsolve/matrix.h"
// Defining the clear console command
/*
To calculate needed exp size :
expression_length *(max_length) + (max_length - 1) * (implicit_multiplication_overhead) + 1(null terminator)*/
// Previous answer global variable
#define EXP_SIZE(size) ((size)*23 + (size - 1) * 3 + 1)
extern double complex ans;
extern int g_var_count;
#endif