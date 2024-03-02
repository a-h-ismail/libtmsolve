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
#include <tmsolve/internals.h>
#include <tmsolve/tms_math_strs.h>
#else
#include "internals.h"
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
 * @brief Locates operators in the current subexpression.
 * @note The function sets op_count in the processed subexpression
 * @param local_expr Expression, offset from the assignment operator (if any).
 * @param S Subexpressions array pointer.
 * @param s_index Index of the current subexpression.
 * @return An int array containing the indexes of each operator.
 */
int *_tms_get_operator_indexes(char *local_expr, tms_math_subexpr *S, int s_index);

/**
 * @brief Sets the (non extended) function pointer in the subexpression.
 * @param local_expr Expression, offset from the assignment operator (if any).
 * @param M tms_math_expr being processed.
 * @param s_index Index of the current subexpression.
 * @return 0 on success, -1 on failure.
 */
int _tms_set_function_ptr(char *local_expr, tms_math_expr *M, int s_index);

/**
 * @brief Allocates nodes and sets basic metadata.
 * @param local_expr Expression, offset from the assignment operator (if any).
 * @param M math_expr being processed.
 * @param s_index Index of the current subexpression.
 * @param operator_index Operator indexes obtained using _tms_get_operator_indexes().
 * @param enable_unknowns Sets support for unknowns like x.
 * @return TMS_CONTINUE, TMS_BREAK, TMS_SUCCESS, TMS_ERROR
 */
int _tms_init_nodes(char *local_expr, tms_math_expr *M, int s_index, int *operator_index, bool enable_unknowns);

/**
 * @brief Sets right/left operands or unknown operand 'x' data for all nodes.
 * @param local_expr Expression, offset from the assignment operator (if any).
 * @param M math_expr being processed.
 * @param s_index Index of the current subexpression.
 * @param enable_vars Toggles support for unknown 'x'
 * @return -1 in case of failure.
 */
int _tms_set_all_operands(char *local_expr, tms_math_expr *M, int s_index, bool enable_unknowns);

/**
 * @brief Reads a value from the expression at start, supports numbers, constants and variables.
 * @param expr The string to read the value from.
 * @param start The index where the start of the value is located.
 * @param enable_complex Toggles complex number support.
 * @return The value read from the string, or NaN in case of failure.
 */
double complex _tms_set_operand_value(char *expr, int start, bool enable_complex);

/**
 * @brief Sets metadata/value for the specified operand.
 * @param expr Current expression.
 * @param M Current math expression structure.
 * @param N Pointer to the node containing the operand
 * @param op_start Start of the operand in the expression
 * @param s_index Index of the current subexpression
 * @param operand 'r' for right, 'l' for left.
 * @param enable_unknowns Enables unknowns (currently x).
 * @return 0 on success, -1 on failure.
 */
int _tms_set_operand(char *expr, tms_math_expr *M, tms_op_node *N, int op_start, int s_index, char operand, bool enable_unknowns);

/**
 * @brief Sets the *next pointer of all nodes.
 * @param S current subexpression being processed.
 */
int _tms_set_evaluation_order(tms_math_subexpr *S);

/**
 * @brief Set the operation result pointer for each node.
 * @param M math_expr being processed.
 * @param s_index Index of the current subexpression.
 */
void _tms_set_result_pointers(tms_math_expr *M, int s_index);

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

/**
 * @brief Sets the unknowns metadata in x_node to the left and/or right operand.
 * @param expr expr The expression string pointer, offset to the position where x_node left/right operand starts.
 * @param x_node The op_node structure to set the unknown in.
 * @param operand Informs the function which operand to set as unknown.
 * @return 0 if the unknown x was found at either operands of x_node, -1 otherwise.
 */
int _tms_set_unknowns_data(char *expr, tms_op_node *x_node, char operand);

/**
 * @brief Parses a math expression into a structure.
 * @note This function automatically prints errors to stderr if any.
 * @param expr The string containing the math expression.
 * @param enable_vars When set to true, the parser will look for unknowns (currently x only) and set its metadata in the corresponding nodes.\n
 * Use tms_set_unknown() to specify the value taken by the unknown.
 * @param enable_complex When set to true, enables the parser to read complex values and set complex variant of scientific functions.
 * @return A (malloc'd) pointer to the generated math structure.
 */
tms_math_expr *tms_parse_expr(char *expr, bool enable_unknowns, bool enable_complex);

/// @brief The actual parser logic is here, but it isn't thread safe and doesn't automatically print errors.
/// @warning Usage is not recommended unless you know what you're doing.
tms_math_expr *_tms_parse_expr_unsafe(char *expr, bool enable_unknowns, bool enable_complex);

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
 * @brief Finds the subexpression that starts at a specific index in the string.
 * @param S The subexpressions array to search.
 * @param start The starting index of the subexpression in the string.
 * @param s_index The index in the subexpression array to initiate searching, should be the index of the subexpression you are currently processing.
 * @param mode Determines if the value passed by start is the expression_start (mode==1) or solve_start (mode==2).
 * @return Depends on the mode, either
 */
int _tms_find_subexpr_starting_at(tms_math_subexpr *S, int start, int s_index, int8_t mode);

/**
 * @brief Finds the subexpression that ends at a specific index in the string.
 * @param S The subexpression array to search.
 * @param end The end index of the subexpression in the string.
 * @param s_index The index in the subexpression array to initiate searching, should be the index of the subexpression you are currently processing.
 * @param s_count The number of subexpressions in the expression.
 * @return
 */
int _tms_find_subexpr_ending_at(tms_math_subexpr *S, int end, int s_index, int s_count);

#endif