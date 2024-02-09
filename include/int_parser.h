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

extern const tms_int_func tms_g_int_func[];

extern const int tms_g_int_func_count;

extern const tms_int_extf tms_g_int_extf[];

extern const int tms_g_int_extf_count;

int _tms_set_int_operand(char *expr, tms_int_expr *M, tms_int_op_node *N, int op_start, int s_index, char operand);

/**
 * @brief The integer version of libtmsolve's parser.
 * @param expr The expression to parse.
 * @return A (malloc'd) pointer to the generated int expression structure.
 */
tms_int_expr *tms_parse_int_expr(char *expr);

/**
 * @brief Deletes an int expression.
 * @param M The expression to delete.
 */
void tms_delete_int_expr(tms_int_expr *M);

int _tms_read_int_operand(char *expr, int start, int64_t *result);

int tms_find_int_subexpr_starting_at(tms_int_subexpr *S, int start, int s_index, int mode);

void tms_set_priority_int(tms_int_op_node *list, int op_count);

char *_tms_lookup_int_function_name(void *function, int func_type);
#endif