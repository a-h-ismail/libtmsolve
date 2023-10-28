/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef TMS_INT_PARSER
#define TMS_INT_PARSER

#ifndef LOCAL_BUILD
#include <tmsolve/tms_math_strs.h>
#else
#include "tms_math_strs.h"
#endif

#define TMS_CONTINUE 1
#define TMS_SUCCESS 0
#define TMS_BREAK 2
#define TMS_ERROR -1

extern char *tms_int_extf_name[];

extern char *tms_int_nfunc_name[];

int64_t _tms_read_int_helper(char *number, int8_t base);

int _tms_set_int_operand(char *expr, tms_int_expr *M, tms_int_op_node *N, int op_start, int s_index, char operand);

tms_int_expr *tms_parse_int_expr(char *expr);

void tms_delete_int_expr(tms_int_expr *M);

int tms_find_int_subexpr_starting_at(tms_int_subexpr *S, int start, int s_index, int mode);

void tms_set_priority_int(tms_int_op_node *list, int op_count);
#endif