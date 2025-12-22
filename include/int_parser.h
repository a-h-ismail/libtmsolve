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

/**
 * @file
 * @brief Declares all functions related to int based expression parsing.
 */

int _tms_set_int_operand(char *expr, tms_int_expr *M, tms_int_op_node *N, int op_start, int s_index, char operand);

/**
 * @brief The integer version of libtmsolve's parser.
 * @param expr The expression to parse.
 * @param options Supported: NO_LOCK, PRINT_ERRORS.
 * @param labels List of named labels and optionally a values array to initialize labeled operands.
 * @return A (malloc'd) pointer to the generated int expression structure.
 */
tms_int_expr *tms_parse_int_expr(const char *expr, int options, tms_arg_list *labels);

/**
 * @brief Deletes an int expression.
 * @param M The expression to delete.
 */
void tms_delete_int_expr(tms_int_expr *M);

int _tms_read_int_operand(tms_int_expr *M, int start, int64_t *result);

void _tms_set_priority_int(tms_int_op_node *list, int op_count);

/**
 * @brief Duplicates an integer expression, returning an identical malloc'd one.
 * @return The new integer expression or NULL on failure.
 */
tms_int_expr *tms_dup_int_expr(tms_int_expr *M);
#endif