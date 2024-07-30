/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_INT_PARSER_H
#define _TMS_INT_PARSER_H

#ifndef LOCAL_BUILD
#include <tmsolve/tms_math_strs.h>
#else
#include "tms_math_strs.h"
#endif

extern const tms_int_func tms_g_int_func[];

extern const int tms_g_int_func_count;

extern const tms_int_extf tms_g_int_extf[];

extern const int tms_g_int_extf_count;

#define TMS_ENABLE_UNK 1

int _tms_set_int_operand(char *expr, tms_int_expr *M, tms_int_op_node *N, int op_start, int s_index, char operand);

/**
 * @brief The integer version of libtmsolve's parser.
 * @note This function automatically prints errors to stderr if any.
 * @param expr The expression to parse.
 * @return A (malloc'd) pointer to the generated int expression structure.
 */
tms_int_expr *tms_parse_int_expr(char *expr, int options, tms_arg_list *unknowns);

/// @brief The actual int parser logic is here, but it isn't thread safe and doesn't automatically print errors.
/// @warning Usage is not recommended unless you know what you're doing.
tms_int_expr *_tms_parse_int_expr_unsafe(char *expr, int options, tms_arg_list *unknowns);

/**
 * @brief Deletes an int expression.
 * @param M The expression to delete.
 */
void tms_delete_int_expr(tms_int_expr *M);

int _tms_read_int_operand(tms_int_expr *M, int start, int64_t *result);

void _tms_set_priority_int(tms_int_op_node *list, int op_count);

tms_int_expr *tms_dup_int_expr(tms_int_expr *M);

char *_tms_lookup_int_function_name(void *function, int func_type);
#endif