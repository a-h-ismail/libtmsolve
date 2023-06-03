/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef SCIENTIFIC_H
#define SCIENTIFIC_H
/**
 * @file
 * @brief Declares all scientific related macros, structures, globals and functions.
 */
#ifndef LOCAL_BUILD
#include <tmsolve/parser.h>
#include <tmsolve/internals.h>
#else
#include "parser.h"
#include "internals.h"
#endif

/// Holds the data of a factor, for use with factorization related features.
typedef struct int_factor
{
    int factor;
    int power;
} int_factor;

/// Simple structure to store a fraction of the form a + b / c
typedef struct fraction
{
    int a, b, c;
} fraction;

/**
 * @brief Calculates the factorial.
 * @return value!
 */
double factorial(double value);

/**
 * @brief Calculates a mathematical expression and returns the answer.
 * @param expr The string containing the math expression.
 * @param enable_complex Enables complex number calculation, set to false if you don't need complex values.
 * @note This function does not affect the global variable ans, you have to explicitly assign the return value to ans.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex calculate_expr(char *expr, bool enable_complex);

/**
 * @brief Calculates a mathematical expression and returns the answer, automatically handles complex numbers.
 * @param expr The string containing the math expression.
 * @note This function does not affect the global variable ans, you have to explicitly assign the return value to ans.
 * @return The answer of the math expression, or NaN in case of failure.
 */
double complex calculate_expr_auto(char *expr);

/**
 * @brief Returns the name of the function possessing the specified pointer.
 * @param function The function pointer to look for.
 * @param is_complex Toggles between searching real and complex functions.
 * @return A pointer to the name, or NULL if nothing is found.
 */
char *lookup_function_name(void *function, bool is_complex);

/**
 * @brief Returns the pointer to a function knowing its name.
 * @param function_name The name of the function.
 * @param is_complex Toggles between searching real and complex functions.
 * @return The function pointer, or NULL in case of failure.
 */
void *lookup_function_pointer(char *function_name, bool is_complex);

/**
 * @brief Coverts a math_expr parsed with complex disabled into a complex enabled one.
 * @param M The math expression to change
 * @note If the function fails to convert the math_expr from real to complex, it won't change the M->enable_complex struct member,
 * use it to verify if the conversion succeeded.
 */
void convert_real_to_complex(math_expr *M);

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
 * @warning You should call syntax_check() before calling this function, because it depends on having an expression with valid syntax.
 * @param expr The string containing the math expression.
 * @param enable_variables When set to true, the parser will look for variables (currently x only) and set its metadata in the corresponding nodes.\n
 * Use set_variable() from function.h to specify the value taken by the variable.
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

double complex ccbrt_cpow(double complex z);

/// @brief Dumps the data of the math expression M.
/// @details The dumped data includes: \n
/// - Subexpression depth and function pointers. \n
/// - Left and right operands of each node (and the operator). \n
/// - Nodes ordered by evaluation order. \n
/// - Result pointer of each subepression.
void dump_expr_data(math_expr *M);
#endif