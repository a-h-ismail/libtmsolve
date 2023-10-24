/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef TMS_INT_PARSER
#define TMS_INT_PARSER

#include "tms_math_strs.h"

#define TMS_CONTINUE 1
#define TMS_SUCCESS 0
#define TMS_BREAK 2
#define TMS_ERROR -1

int64_t _tms_read_int_helper(char *number, int8_t base);

tms_int_expr *tms_parse_int_expr(char *expr);

void tms_delete_int_expr(tms_int_expr *M);

int tms_find_int_subexpr_starting_at(tms_int_subexpr *S, int start, int s_index, int mode);
#endif