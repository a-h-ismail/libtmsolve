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

/// @brief Operator node, stores the required metadata for an operator and its operators.
typedef struct op_node
{
    /// The operator of this op_node
    char operator;
    /// Index of the operator in the expression
    int operator_index;
    /// Index of the op_node in the op_node array
    int node_index;
    /**
     *  Used to store data about variable operands as follows:
     *  b0:left_operand, b1:right_operand, b2:left_operand_negative, b3:right_operand_negative
     */
    uint8_t var_metadata;
    /// Node operator priority
    uint8_t priority;

    double complex left_operand, right_operand, *node_result;
    /// Points to the next op_node in evaluation order
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

    /// Index of the op_node at which the subexpression parsing starts
    int start_node;

    /// The array of op_nodes composing this subexpression.
    struct op_node *subexpr_nodes;

    /// @brief Points at one of the op_nodes result pointer, indicating that the answer of that node is the answer of this subexpression.
    /// @details The op_node does not need to be in the same instance of the subexpr struct.
    double complex **s_result;

    /// Function to execute on the subexpression result.
    double (*function_ptr)(double);

    /// Complex function to execute on the subexpression result.
    double complex (*cmplx_function_ptr)(double complex);

    /// Extended function to execute.
    double (*ext_function_ptr)(char *);

    /// Enables execution of extended function, allows optimizing of nested extended functions like integration without thrashing performance.
    bool execute_extended;
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

    /// Array of variable operands metadata
    var_op_data *var_data;

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

/**
 * @brief Calculates the factorial.
 * @return value!
 */
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
 * @param M The math structure to evaluate.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex eval_math_expr(math_expr *M);

/**
 * @brief Sets the variable metadata in x_node to the left and/or right operand.
 * @param expr expr The expression string pointer, offset to the position where x_node left/right operand starts.
 * @param x_node The op_node structure to set the variable in.
 * @param operand Informs the function which operand to set as variable.
 * @return true if the variable x was found at either operands of x_node, false otherwise.
 */
bool set_variable_metadata(char *expr, op_node *x_node, char operand);
/**
 * @brief Parses a math expression into a structure.
 * @warning You should call parenthesis_check() and implicit_multiplication() before calling this function,
 *  because it depends on having a reasonably valid expression.
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
 * @brief Fills a op_node array with the priority of each op_node's operator.
 * @param list The op_node array to fill.
 * @param op_count The number of operators in the array. Equal to the number of nodes.
 */
void priority_fill(op_node *list, int op_count);

/**
 * @brief Finds the subexpression that starts at a specific index in the string.
 * @param S The subexpressions array to search.
 * @param start The starting index of the subexpression in the string.
 * @param s_index The index in the subexpression array to initiate searching, should be the index of the subexpression you are currently processing.
 * @param mode Determines if the value passed by start is the expression_start (mode==1) or solve_start (mode==2).
 * @return Depends on the mode, either 
 */
int find_subexpr_by_start(m_subexpr *S, int start, int s_index, int mode);

/**
 * @brief Finds the subexpression that ends at a specific index in the string.
 * @param S The subexpression array to search.
 * @param end The end index of the subexpression in the string.
 * @param s_index The index in the subexpression array to initiate searching, should be the index of the subexpression you are currently processing.
 * @param s_count The number of subexpressions in the expression.
 * @return 
 */
int find_subexpr_by_end(m_subexpr *S, int end, int s_index, int s_count);

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