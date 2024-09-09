/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "tms_math_strs.h"
#include <inttypes.h>
#include <stdlib.h>

#ifndef OVERRIDE_DEFAULTS
#define operand_type int64_t
#define solve_unsafe _tms_int_solve_unsafe
#define solve_list tms_int_solve_list
#endif

operand_type *solve_list(tms_arg_list *expr_list)
{
    if (expr_list->count < 1)
        return NULL;
    operand_type *answer_list = malloc(expr_list->count * sizeof(operand_type));
    int status;
    for (int i = 0; i < expr_list->count; ++i)
    {
        status = _tms_int_solve_unsafe(expr_list->arguments[i], answer_list + i);
        if (status != 0)
        {
            free(answer_list);
            answer_list = NULL;
            break;
        }
    }

    return answer_list;
}