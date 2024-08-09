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
#include <tmsolve/tms_math_strs.h>
#else
#include "tms_math_strs.h"
#endif
#include <stdbool.h>
#include <inttypes.h>

tms_math_expr *tms_dup_mexpr(tms_math_expr *M);

/**
 * @brief Sets the (non extended) function pointer in the subexpression.
 * @param local_expr Expression, offset from the assignment operator (if any).
 * @param M tms_math_expr being processed.
 * @param s_index Index of the current subexpression.
 * @return 0 on success, -1 on failure.
 */
int _tms_set_rcfunction_ptr(char *local_expr, tms_math_expr *M, int s_index);

/**
 * @brief Coverts a math_expr parsed with complex disabled into a complex enabled one.
 * @param M The math expression to change
 * @note If the function fails to convert the math_expr from real to complex, it won't change the M->enable_complex struct member,
 * use it to verify if the conversion succeeded.
 */
void tms_convert_real_to_complex(tms_math_expr *M);

#define TMS_ENABLE_UNK 1
#define TMS_ENABLE_CMPLX 2

/**
 * @brief Parses a math expression into a structure.
 * @note This function automatically prints errors to stderr if any.
 * @param expr The string containing the math expression.
 * @param options Provides the parser with options (currently: TMS_ENABLE_UNK, TMS_ENABLE_CMPLX). A 0 here means defaults.
 * @param unknowns If the option to enable unknowns is set, provide the unknowns names here.
 * @return A (malloc'd) pointer to the generated math structure.
 */
tms_math_expr *tms_parse_expr(char *expr, int options, tms_arg_list *unknowns);

/// @brief The actual parser logic is here, but it isn't thread safe and doesn't automatically print errors.
/// @warning Usage is not recommended unless you know what you're doing.
tms_math_expr *_tms_parse_expr_unsafe(char *expr, int options, tms_arg_list *unknowns);

int _tms_get_operand_value(tms_math_expr *M, int start, double complex *out);

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