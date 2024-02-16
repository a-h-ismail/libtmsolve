/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef TMS_STRUCTS_H
#define TMS_STRUCTS_H
#include <inttypes.h>
#include <stdbool.h>
#include <complex.h>

/**
 * @file
 * @brief Declares math structures used to store parsed expressions.
 */

/// @brief Stores metadata related to extended functions arguments.
typedef struct tms_arg_list
{
    /// The number of arguments.
    int count;
    // Array of C strings, stores the arguments.
    char **arguments;
} tms_arg_list;

/// @brief Simple structure to wrap around C's complex type (used for bindings)
typedef struct _tms_complex_double
{
    double real;
    double imaginary;
} tms_complex_double;

typedef struct tms_rc_func
{
    char *name;
    double (*real)(double);
    double complex (*cmplx)(double complex);
} tms_rc_func;

typedef struct tms_extf
{
    char *name;
    double complex (*ptr)(tms_arg_list *);
} tms_extf;

typedef struct tms_int_func
{
    char *name;
    int (*ptr)(int64_t, int64_t *);
} tms_int_func;

typedef struct tms_int_extf
{
    char *name;
    int (*ptr)(tms_arg_list *, int64_t *);
} tms_int_extf;

/// @brief Runtime variable metadata of tmsolve.
typedef struct tms_var
{
    char *name;
    double complex value;
    bool is_constant;
} tms_var;

/// @brief Runtime variable metadata, 64 bit int variant.
typedef struct tms_int_var
{
    char *name;
    int64_t value;
} tms_int_var;

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

/// @brief User runtime function structure.
typedef struct tms_ufunc
{
    char *name;
    struct tms_math_expr *F;
} tms_ufunc;

/// @brief Union to store function pointers
typedef union tms_mfunc_ptrs
{
    double (*real)(double);
    double complex (*cmplx)(double complex);
    double complex (*extended)(tms_arg_list *);
    tms_ufunc *runtime;
} fptr;

/// @brief Union to store int function pointers
typedef union tms_int_functions
{
    int (*simple)(int64_t, int64_t *);
    int (*extended)(tms_arg_list *, int64_t *);

} int_fptr;

#define TMS_NOFUNC 0
#define TMS_F_REAL 1
#define TMS_F_CMPLX 2
#define TMS_F_EXTENDED 3
#define TMS_F_RUNTIME 4
#define TMS_F_INT64 5
#define TMS_F_INT_EXTENDED 6

#define TMS_V_DOUBLE 10
#define TMS_V_INT64 11

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
    double complex **result;

    /// Stores the pointer of the function to execute
    fptr func;

    /// Stores the type of the function to execute
    uint8_t func_type;

    /// Enables execution of extended function, used to optimize nested extended functions like integration.
    bool exec_extf;
} tms_math_subexpr;

/// @brief The standalone structure to hold all of an expression's metadata.
typedef struct tms_math_expr
{
    /// The full expression string, after removing whitespaces and compacting +/- operators
    char *expr;

    /// The string form of the expression, after adding offset to skip assignment operator
    char *local_expr;

    /// The subexpression array created by parsing the math expression.
    tms_math_subexpr *S;

    /// Number of subexpression in this math expression.
    int subexpr_count;

    /// Number of unknown operands.
    int unknowns_count;

    /// Indicates the index of tms_g_var_values to copy the answer to.
    int runvar_i;

    /// Array of unknown operands metadata.
    tms_unknown_operand *x_data;

    /// Answer of the expression.
    double complex answer;

    /// Toggles complex support.
    bool enable_complex;
} tms_math_expr;

/// @brief Operator node, stores the required metadata for an operator and its operands.
typedef struct tms_int_op_node
{
    /// The operator of this op_node.
    char operator;
    /// Index of the operator in the expression.
    int operator_index;
    /// Index of the op_node in the op_node array.
    int node_index;

    /// Node operator priority.
    uint8_t priority;

    int64_t left_operand, right_operand, *result;
    /// Points to the next op_node in evaluation order.
    struct tms_int_op_node *next;
} tms_int_op_node;

/// @brief Holds the metadata of an integer subexpression.
typedef struct tms_int_subexpr
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

    /// The array of int_op_nodes composing this subexpression.
    struct tms_int_op_node *nodes;

    /// @brief Set to one of the op_nodes result pointer, indicating that the answer of that node is the answer of this subexpression.
    int64_t **result;

    /// Stores the pointer of the function to execute
    int_fptr func;

    /// Stores the type of the function to execute
    uint8_t func_type;

    /// Enables execution of extended function
    bool exec_extf;
} tms_int_subexpr;

/// @brief The standalone structure to hold all of an integer expression's metadata.
typedef struct tms_int_expr
{
    /// The full expression string, after removing whitespaces and compacting +/- operators
    char *expr;

    /// The string form of the expression, after adding offset to skip assignment operator
    char *local_expr;

    /// The subexpression array created by parsing theexpression.
    tms_int_subexpr *S;

    /// Number of subexpression in this expression
    int subexpr_count;

    /// Answer of the expression
    int64_t answer;

    /// Indicates the index of tms_g_int_vars to copy the answer to.
    int runvar_i;
} tms_int_expr;

#endif