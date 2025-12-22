/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_PARSER_H
#define _TMS_PARSER_H

/**
 * @file
 * @brief Declares all functions related to math expressions parsing.
 */

#ifndef LOCAL_BUILD
#include <tmsolve/c_complex_to_cpp.h>
#include <tmsolve/tms_math_strs.h>
#else
#include "c_complex_to_cpp.h"
#include "tms_math_strs.h"
#endif
#include <stdbool.h>
#include <inttypes.h>

/**
 * @brief Duplicates a math expression, returning an identical malloc'd one.
 * @return The new math expression or NULL on failure.
 */
tms_math_expr *tms_dup_mexpr(tms_math_expr *M);

int _tms_set_rcfunction_ptr(const char *expr, tms_math_expr *M, int s_index);

/**
 * @brief Coverts a math_expr parsed with complex disabled into a complex enabled one.
 * @param M The math expression to change
 * @note If the function fails to convert the math_expr from real to complex, it won't change the M->enable_complex struct member,
 * use it to verify if the conversion succeeded.
 */
void tms_convert_real_to_complex(tms_math_expr *M);

/**
 * @brief Parses a math expression into a structure.
 * @param expr The string containing the math expression.
 * @param options Provides the parser with options (currently: ENABLE_LABELS, ENABLE_CMPLX, NO_LOCK, PRINT_ERRORS)
 * @param labels If the option to enable labels is set, provide them here.
 * @return A (malloc'd) pointer to the generated math structure.
 */
tms_math_expr *tms_parse_expr(const char *expr, int options, tms_arg_list *labels);

int _tms_get_operand_value(tms_math_expr *M, int start, cdouble *out);

/**
 * @brief Frees the memory used by the members of a math_expr.
 * @param M The math_expr which members will be freed.
 */
void tms_delete_math_expr_members(tms_math_expr *M);

/**
 * @brief Frees the memory used by a math_expr and its members.
 * @param M The math_expr to delete.
 */
void tms_delete_math_expr(tms_math_expr *M);

/**
 * @brief Sets the priority of each op_node's operator in the provided array.
 * @param list The op_node array.
 * @param op_count Number of operators in the array. Equal to the number of nodes.
 */
void _tms_set_priority(tms_op_node *list, int op_count);

/**
 * @brief Checks if the specified math expression is deterministic (as in it has no random functions).
 */
bool tms_is_deterministic(tms_math_expr *M);

#endif