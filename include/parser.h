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

/// @brief Contains real and complex functions metadata.
extern const tms_rc_func tms_g_rc_func[];

extern const int tms_g_rc_func_count;

/// @brief Contains extended function metadata.
extern const tms_extf tms_g_extf[];

extern const int tms_g_extf_count;

/// @brief Comparator function for use with qsort(), compares the depth of 2 subexpressions.
/// @return 1 if a.depth < b.depth; -1 if a.depth > b.depth; 0 otherwise.
int tms_compare_subexpr_depth(const void *a, const void *b);

/**
 * @brief First parsing step, detects assignment to runtime variables.
 * @param eq Index of the assignment operator.
 * @return Index of the variable in the global array.
 */
int _tms_set_runtime_var(char *expr, int eq);

/**
 * @brief Initializes the math expression structure.
 * @param expr Math expression string
 * @param enable_complex Set complex support status.
 * @return Pointer to the initialized structure.
 */
tms_math_expr *_tms_init_math_expr(char *expr, bool enable_complex);

/**
 * @brief Sets the (non extended) function pointer in the subexpression.
 * @param local_expr Expression, offset from the assignment operator (if any).
 * @param M tms_math_expr being processed.
 * @param s_index Index of the current subexpression.
 * @return 0 on success, -1 on failure.
 */
int _tms_set_rcfunction_ptr(char *local_expr, tms_math_expr *M, int s_index);

/**
 * @brief Returns the name of the function possessing the specified pointer.
 * @param function The function pointer to look for.
 * @param func_type Type of the function pointer (1:real, 2:complex, 3: extended)
 * @return A pointer to the name, or NULL if nothing is found.
 */
char *_tms_lookup_function_name(void *function, int func_type);

/**
 * @brief Returns the pointer to a function knowing its name.
 * @param function_name The name of the function.
 * @param is_complex Toggles between searching real and complex functions.
 * @return The function pointer, or NULL in case of failure.
 */
void *_tms_lookup_function_pointer(char *function_name, bool is_complex);

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