/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef PARSER_H
#define PARSER_H

/**
 * @file
 * @brief Declares all functions and structures used by the parser.
 */

#ifndef LOCAL_BUILD
#include <tmsolve/internals.h>
#else
#include "internals.h"
#endif
#include <stdbool.h>
#include <inttypes.h>

// Global variables

/// @brief Contains the names of scientific functions (for real numbers) like sin, cos...
extern char *tms_r_func_name[];

/// @brief Contains the function pointers of scientific functions.
extern double (*tms_r_func_ptr[])(double);

/// @brief Contains the names of complex numbers functions like sin, cos...
extern char *tms_cmplx_func_name[];

/// @brief Contains the function pointers of scientific functions.
extern double complex (*tms_cmplx_func_ptr[])(double complex);

/// @brief Contains the names of extended functions (functions with variable number of arguments, passed as a comma separated string).
extern char *tms_ext_func_name[];

/// @brief Contains the function pointers of extended functions.
extern double complex (*tms_ext_func[])(tms_arg_list *);

/// @brief Comparator function for use with qsort(), compares the depth of 2 subexpressions.
/// @return 1 if a.depth < b.depth; -1 if a.depth > b.depth; 0 otherwise.
int tms_compare_subexpr_depth(const void *a, const void *b);

/// @brief Operator node, stores the required metadata for an operator and its operands.
typedef struct tms_op_node
{
    /// The operator of this op_node.
    char operator;
    /// Index of the operator in the expression.
    int operator_index;
    /// Index of the op_node in the op_node array.
    int node_index;
    /**
     * Used to store data about unknown operands as follows:
     * b0:left_operand, b1:right_operand, b2:left_operand_negative, b3:right_operand_negative.
     * Use masks UNK_* with bitwise OR to set the bits correctly
     */
    uint8_t unknowns_data;
    /// Node operator priority.
    uint8_t priority;

    double complex left_operand, right_operand, *result;
    /// Points to the next op_node in evaluation order.
    struct tms_op_node *next;
} tms_op_node;

#define UNK_LEFT 0b1
#define UNK_RIGHT 0b10
#define UNK_LNEG 0b100
#define UNK_RNEG 0b1000

/// @brief Holds the data required to locate and set a value to an unknown in the expression.
typedef struct tms_unknown_operand
{
    /// @brief Pointer to the unknown operand.
    double complex *unknown_ptr;
    /// @brief Set to true if the operand is negative.
    bool is_negative;
} tms_unknown_operand;

/// @brief Union to store function pointers
typedef union tms_mfunc_ptrs
{
    double (*real)(double);
    double complex (*cmplx)(double complex);
    double complex (*extended)(tms_arg_list *);
} fptr;

/// @brief Holds the metadata of a subexpression.
typedef struct tms_math_subexpr
{
    /// @brief Number of operators in this subexpression.
    int op_count;

    /// @brief Depth of the subexpression, starts from 0 for the whole expression and adds 1 for each nested parenthesis pair.
    int depth;

    /// @brief The start index of the actual subexpression to parse,
    /// skips the function name (if any) and points to the beginning of the numerical expression.
    int solve_start;

    /// @brief The start index of the subexpression in the expression.
    /// @details If the subexpression has a function call, the index will be at the first character of the function name, otherwise equals to solve_start.
    int subexpr_start;

    /// @brief The end index of the subexpression, just before the close parenthesis.
    int solve_end;

    /// Index of the op_node at which the subexpression parsing starts.
    int start_node;

    /// The array of op_nodes composing this subexpression.
    struct tms_op_node *nodes;

    /// @brief Set to one of the op_nodes result pointer, indicating that the answer of that node is the answer of this subexpression.
    /// @details The op_node does not need to be in the same instance of the subexpr struct.
    double complex **result;

    /// Stores the pointer of the function to execute
    fptr func;

    /// Stores the type of the function to execute (0: none, 1: real, 2: cmplx, 3: extended)
    uint8_t func_type;

    /// Enables execution of extended function, used to optimize nested extended functions like integration.
    bool exec_extf;
} tms_math_subexpr;

/// The standalone structure to hold all of an expression's metadata.
typedef struct tms_math_expr
{
    /// The subexpression array created by parsing the math expression.
    tms_math_subexpr *subexpr_ptr;

    /// Number of subexpression in this math expression.
    int subexpr_count;

    /// Number of unknown operands.
    int unknown_count;

    /// Indicates the index of tms_g_var_values to copy the answer to.
    int runvar_i;

    /// Array of unknown operands metadata.
    tms_unknown_operand *x_data;

    /// Answer of the expression.
    double complex answer;

    /// Toggles complex support.
    bool enable_complex;
} tms_math_expr;

#define TMS_CONTINUE 1
#define TMS_SUCCESS 0
#define TMS_BREAK 2
#define TMS_ERROR -1

/**
 * @brief First parsing step, detects assignment to runtime variables.
 * @param eq Index of the assignment operator.
 * @return Index of the variable in the global array.
 */
int _tms_set_runtime_var(char *expr, int eq);

/**
 * @brief Initializes the math expression structure.
 * @param local_expr Expression, offset from the assignment operator (if any).
 * @param enable_complex Set complex support status.
 * @return Pointer to the initialized structure.
 */
tms_math_expr *_tms_init_math_expr(char *local_expr, bool enable_complex);

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
 * @return true on success, false on failure.
 */
bool _tms_set_function_ptr(char *local_expr, tms_math_expr *M, int s_index);

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
bool _tms_set_evaluation_order(tms_math_subexpr *S);

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
int tms_set_unknowns_data(char *expr, tms_op_node *x_node, char operand);

/**
 * @brief Parses a math expression into a structure.
 * @warning You should call tms_syntax_check() before calling this function, because it depends on having an expression with valid syntax.
 * @param expr The string containing the math expression.
 * @param enable_vars When set to true, the parser will look for unknowns (currently x only) and set its metadata in the corresponding nodes.\n
 * Use tms_set_unknown() to specify the value taken by the unknown.
 * @param enable_complex When set to true, enables the parser to read complex values and set complex variant of scientific functions.
 * @return A (malloc'd) pointer to the generated math structure.
 */
tms_math_expr *tms_parse_expr(char *expr, bool enable_unknowns, bool enable_complex);

/**
 * @brief Frees the memory used by a math_expr and its members.
 * @param math_struct The math_expr to delete.
 */
void tms_delete_math_expr(tms_math_expr *math_struct);

/**
 * @brief Sets the priority of each op_node's operator in the provided array.
 * @param list The op_node array.
 * @param op_count Number of operators in the array. Equal to the number of nodes.
 */
void tms_set_priority(tms_op_node *list, int op_count);

/**
 * @brief Finds the subexpression that starts at a specific index in the string.
 * @param S The subexpressions array to search.
 * @param start The starting index of the subexpression in the string.
 * @param s_index The index in the subexpression array to initiate searching, should be the index of the subexpression you are currently processing.
 * @param mode Determines if the value passed by start is the expression_start (mode==1) or solve_start (mode==2).
 * @return Depends on the mode, either
 */
int tms_find_subexpr_starting_at(tms_math_subexpr *S, int start, int s_index, int mode);

/**
 * @brief Finds the subexpression that ends at a specific index in the string.
 * @param S The subexpression array to search.
 * @param end The end index of the subexpression in the string.
 * @param s_index The index in the subexpression array to initiate searching, should be the index of the subexpression you are currently processing.
 * @param s_count The number of subexpressions in the expression.
 * @return
 */
int tms_find_subexpr_ending_at(tms_math_subexpr *S, int end, int s_index, int s_count);

#endif