#ifndef PARSER_H
#define PARSER_H
/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

/**
 * @file
 * @brief Declares all functions and structures used by the parser.
 */

#ifndef LOCAL_BUILD
#include <tmsolve/internals.h>
#else
#include "internals.h"

// Global variables.

/// @brief Stores the answer of the last calculation to allow reuse in future calculations.
extern double complex ans;

/// @brief Contains the names of scientific functions (for real numbers) like sin, cos...
extern char *r_function_name[];

/// @brief Contains the function pointers of scientific functions.
extern double (*r_function_ptr[])(double);

/// @brief Contains the names of complex numbers functions like sin, cos...
extern char *cmplx_function_name[];

/// @brief Contains the function pointers of scientific functions.
extern double complex (*cmplx_function_ptr[])(double complex);

/// @brief Contains the names of extended functions (functions with variable number of arguments, passed as a comma separated string).
extern char *ext_function_name[];

/// @brief Contains the function pointers of scientific functions.
extern double (*ext_math_function[])(char *);

/// @brief Comparator function for use with qsort(), compares the depth of 2 subexpressions.
/// @return 1 if a.depth < b.depth; -1 if a.depth > b.depth; 0 otherwise.
int compare_subexps_depth(const void *a, const void *b);

/// @brief Operator node, stores the required metadata for an operator and its operands.
typedef struct op_node
{
    /// The operator of this op_node.
    char operator;
    /// Index of the operator in the expression.
    int operator_index;
    /// Index of the op_node in the op_node array.
    int node_index;
    /**
     *  Used to store data about variable operands as follows:
     *  b0:left_operand, b1:right_operand, b2:left_operand_negative, b3:right_operand_negative.
     */
    uint8_t var_metadata;
    /// Node operator priority.
    uint8_t priority;

    double complex left_operand, right_operand, *node_result;
    /// Points to the next op_node in evaluation order.
    struct op_node *next;
} op_node;

/// @brief Holds the data required to locate and set a value to a variable in the expression.
typedef struct var_op_data
{
    /// @brief Pointer to the operand set as variable.
    double complex *var_ptr;
    /// @brief Set to true if the operand is negative.
    bool is_negative;
} var_op_data;

/// @brief Union to store function pointers
typedef union mfunc_pointers
{
    double (*real)(double);
    double complex (*cmplx)(double complex);
    double (*extended)(char *);
} fptr;

/// @brief Holds the metadata of a subexpression.
typedef struct m_subexpr
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
    struct op_node *nodes;

    /// @brief Points at one of the op_nodes result pointer, indicating that the answer of that node is the answer of this subexpression.
    /// @details The op_node does not need to be in the same instance of the subexpr struct.
    double complex **s_result;

    /// Stores the pointer of the function to execute
    fptr func;

    /// Stores the type of the function to execute (0: none, 1: real, 2: cmplx, 3: extended)
    uint8_t func_type;

    /// Enables execution of extended function, used to optimizing of nested extended functions like integration without thrashing performance.
    bool exec_extf;
} m_subexpr;

/// The standalone structure to hold all of an expression's metadata.
typedef struct math_expr
{
    /// The subexpression array created by parsing the math expression.
    m_subexpr *subexpr_ptr;

    /// Number of subexpression in this math expression.
    int subexpr_count;

    /// Number of variable operands.
    int var_count;

    /// Indicates the index of variable_values to copy the answer to.
    int variable_index;

    /// Array of variable operands metadata.
    var_op_data *var_data;

    /// Answer of the expression.
    double complex answer;

    /// Toggles complex support.
    bool enable_complex;
} math_expr;


#endif
#endif