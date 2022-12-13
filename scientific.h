/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef SCIENTIFIC_H
#define SCIENTIFIC_H
/**
 * @file
 * @brief Declares all scientific related macros, structures, globals and functions.
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

/// @brief Stores the required metadata for an operand and its operators.
typedef struct node
{
    /// The operator of this node
    char operator;
    /// Index of the operator in the expression
    int operator_index;
    /// Index of the node in the node array
    int node_index;
    /**
     *  Used to store data about variable operands as follows:
     *  b0:left_operand, b1:right_operand, b2:left_operand_negative, b3:right_operand_negative
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
    /// Index of the node at which the subexpression parsing starts
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

/// The standalone structure to hold all of an expression's metadata.
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

/// @brief Use to store the answer of the last calculation to allow reuse
extern double complex ans;

extern char *glob_expr;
extern char *function_name[];
extern double (*math_function[])(double);
extern char *ext_function_name[];
extern double (*ext_math_function[])(char *);

// Scientific functions

/// @brief Comparator function for use with qsort(), compares the depth of 2 subexpressions.
/// @param a
/// @param b
/// @return 1 if a.depth < b.depth; -1 if a.depth > b.depth; 0 otherwise.
int compare_subexps_depth(const void *a, const void *b);

double factorial(double value);

/**
 * @brief Calculates a mathematical expression and returns the answer.
 * @param expr The string containing the math expression.
 * @param enable_complex Enables complex number calculation, set to false if you don't need complex values.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex calculate_expr(char *expr, bool enable_complex);

/**
 * @brief Evaluates a math_expr structure and calculates the result.
 * @param math_struct The math structure to evaluate.
 * @return The answer of the math expression, or NaN on error.
 */
double complex evaluate(math_expr *math_struct);

/**
 * @brief Sets the variable metadata in x_node to the left and/or right operand.
 * @param expr expr The expression string pointer, offset to the position where x_node left/right operand starts.
 * @param x_node The node structure to set the variable in.
 * @param operand Informs the function which operand to set as variable.
 * @return true if the variable x was found at either operands of x_node, false otherwise.
 */
bool set_variable_metadata(char *expr, node *x_node, char operand);
/**
 * @brief Parses a math expression into a structure.
 * @param expr The string containing the math expression.
 * @param enable_variables When set to true, the parser will look for variables (currently x only) and set its metadata in the corresponding nodes.
 * @param enable_complex When set to true, enables the parser to read complex values and set complex variant of scientific functions (disables integration, derivation, modulo and factorial).
 * @return A (malloc'd) pointer to the generated math structure.
 */
math_expr *parse_expr(char *expr, bool enable_variables, bool enable_complex);

/**
 * @brief Frees the memory used by a math_expr and its members.
 * @param math_struct The math_expr to delete.
 */
void delete_math_expr(math_expr *math_struct);


/**
 * @brief Fills a node array with the priority of each node's operator.
 * @param list The node array to fill.
 * @param op_count The number of operators in the array. Equal to the number of nodes.
 */
void priority_fill(node *list, int op_count);



/**
 * @brief Finds the subexpression that starts at a specific index in the string.
 * @param m_expr The subexpressions array to search.
 * @param start The starting index of the subexpression in the string.
 * @param subexpr_i The index in the subexpression array to initiate searching, should be the index of the subexpression you are currently processing.
 * @param mode Determines if the value passed by start is the expression_start (mode==1) or solve_start (mode==2).
 * @return 
 */
int subexp_start_at(s_expression *m_expr, int start, int subexpr_i, int mode);

int s_exp_ending(s_expression *expression, int end, int current_s_exp, int s_exp_count);

/**
 * @brief Finds the factors of a signed 32bit integer.
 * @param value The value to factorize.
 * @return An array (malloc'd) of factors and the number of occurences of each factor.
 */
int_factor *find_factors(int32_t value);

/**
 * @brief Reduces a fraction to its irreductible form.
 * @param fraction_r Pointer to the fraction to reduce.
 */
void reduce_fraction(fraction *fraction_r);

/**
 * @brief Converts a decimal value into its equivalent fraction if possible.
 * @param value The decimal value to convert.
 * @param inverse_process Tells the function that the value it received is the inverse of the actual value.
 * For the first call, set it to false.
 * @return The fraction form of the value.
 */
fraction decimal_to_fraction(double value, bool inverse_process);
#endif