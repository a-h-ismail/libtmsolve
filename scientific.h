/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef SCIENTIFIC_H
#define SCIENTIFIC_H
/*
    Contains:
    * The declaration of all scientific related functions
    * The structures used by these functions
    * Global expression and answer variables
    * Defines error messages of this mode
*/
#include "internals.h"
// Error messages
#define DIVISION_BY_ZERO "Division by zero isn't defined."
#define MODULO_ZERO "Modulo zero implies a division by zero."
#define MATH_ERROR "Math error."
#define RIGHT_OP_MISSING "Missing right operand."

/// Holds the data of a factor, for use with factorization related features.
typedef struct int_factor
{
    int factor;
    int power;
} int_factor;

/// Stores the required metadata for an operand and its operators.
typedef struct node
{
    /// The operator of this node
    char operator;
    /// Index of the operator in the expression
    int operator_index;
    /// Index of the node in the node array
    int node_index;
    /** Used to store data about variable operands as follows:
    b0:left_operand, b1:right_operand, b2:left_operand_negative, b3:right_operand_negative
    */
    uint8_t var_metadata;
    /// Node operator priority
    uint8_t priority;

    double complex left_operand, right_operand, *node_result;
    /// Points to the next node in evaluation order
    struct node *next;
} node;

/// @brief Holds the data required to locate and set a variable in the expression.
typedef struct variable_data
{
    /// @brief Pointer to the operand set as variable.
    double complex *pointer;
    /// @brief Set to true if the operand is negative.
    bool is_negative;
} variable_data;

/// @brief Holds the metadata of a subexpression
typedef struct s_expression
{
    int op_count, depth;
    /*
    expression start
    ____v
    ____cos(pi/3)
    ________^__^
    solve_start, solve_end
    */
    int expression_start, solve_start, solve_end;
    // The index of the node at which the subexpression parsing starts
    int start_node;
    struct node *node_list;

    /// Each node has a result pointer, this pointer tells which one of the result pointers will carry the subexpression result.
    double complex **result;
    /// Function to execute on the subexpression result.
    double (*function_ptr)(double);
    /// Complex function to execute on the subexpression result.
    double complex (*cmplx_function_ptr)(double complex);
    /// Extended function to execute.
    double (*ext_function_ptr)(char *);
    /// Enables execution of extended function, allows optimizing of nested extended functions like integration without thrashing performance.
    bool execute_extended;
} s_expression;

/// The standalone structure to hold all of an expression's metadata
typedef struct math_expr
{
    /// The subexpression array created by parsing the math expression.
    s_expression *subexpr_ptr;

    /// Number of subexpression in this math expression.
    int subexpr_count;

    /// Number of variable operands.
    int var_count;

    /// Array of variable operands metadata
    variable_data *variable_ptr;

    /// Answer of the expression
    double complex answer;

    /// Toggles complex support
    bool enable_complex;
} math_expr;

/// Simple structure to store a fraction of the form a + b / c
typedef struct fraction
{
    double a, b, c;
} fraction;
// Global variables
extern double complex ans;
extern char *glob_expr;
extern char *function_name[];
extern double (*math_function[])(double);
extern char *ext_function_name[];
extern double (*ext_math_function[])(char *);

// Scientific functions
int compare_subexps_depth(const void *a, const void *b);
double complex evaluate(math_expr *math_struct);
math_expr *parse_expr(char *expr, bool enable_variables, bool enable_complex);
double factorial(double value);
double complex calculate_expr(char *expr, bool enable_complex);
double evaluate_region(char *expr, int a, int b);
void scientific_complex_picker(char *expr);
void delete_math_expr(math_expr *math_struct);
int priority_test(char operator1, char operator2);
void priority_fill(node *list, int op_count);
int subexp_start_at(s_expression *expression, int start, int current_s_exp, int mode);
int s_exp_ending(s_expression *expression, int end, int current_s_exp, int s_exp_count);
int_factor *find_factors(int32_t value);
void reduce_fraction(fraction *fraction_r);
fraction decimal_to_fraction(double value, bool inverse_process);
void factorize(char *expr);
void nroot_solver(char *expr);
void fraction_reducer(int32_t *f_v1, int32_t *f_v2, int32_t *v1, int32_t *v2);
// Complex functions
void complex_interpreter(char *expr);
#endif