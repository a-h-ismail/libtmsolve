/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
/**
 * @file
 * @brief Defines all string handling related macros, structures, globals and functions.
*/
// Errors
#define PARAMETER_MISSING "The function expects more arguments."
#define PARENTHESIS_EMPTY "Empty parenthesis pair."
#define PARENTHESIS_NOT_CLOSED "Open parenthesis has no closing parenthesis."
#define PARENTHESIS_NOT_OPEN "Extra closing parenthesis."
// Function declarations
bool is_infinite(char *expr, int index);

/**
 * @brief Finds the closing parenthesis that corresponds to an open parenthesis. 
 * @param expr The string to search.
 * @param i The index of the open parenthesis.
 * @return The index of the closing parenthesis, or -1 if an error occured.
 */
int find_closing_parenthesis(char *expr, int i);

/**
 * @brief Finds the open parenthesis that corresponds to a closing parenthesis.
 * @param expr The string to search.
 * @param p The index of the closing parenthesis.
 * @return The index of the open parenthesis, or -1 if an error occured.
 */
int find_opening_parenthesis(char *expr, int p);

/**
 * @brief Checks if the character is an arithmetic operator.
 * @param c The character to test.
 * @return true if the character is an operator, false otherwise.
 */
bool is_op(char c);

/**
 * @brief Finds the next occurence of an operator in the string.
 * @param expr The string to search.
 * @param i The index to begin searching from.
 * @return The index of the next operator in the string, or -1 if none is found.
 */
int next_op(char *expr, int i);

/**
 * @brief Deletes whitespaces from a string.
 * @param str The string to delete whitespace from it.
 */
void remove_whitespace(char *str);
void string_resizer(char *str, int o_end, int n_end);
bool implicit_multiplication(char **expr);
bool var_implicit_multiplication(char *expr);

/**
 * @brief Checks if the character c is a digit.
 * @return true if c is a digit (0-9), false otherwise.
 */
bool is_digit(char c);

/**
 * @brief Checks if the character c is a letter.
 * @return true if c is a letter (A-Z) or (a-z).
 */
bool is_alphabetic(char c);

/**
 * @brief Finds the end of a number knowing its start in the expression.
 * @param expr The expression to search.
 * @param start The start index of the number.
 * @return The index of the number's end, or -1 in case of failure.
 */
int find_endofnumber(char *expr, int start);

/**
 * @brief Finds the start of a number knowing its end in the expression.
 * @param expr The expression to search.
 * @param end The end index of the number.
 * @return The index of the number's start, or -1 in case of failure.
 */
int find_startofnumber(char *expr, int end);
// Function to find the first occurence of a string starting from i
int f_search(char *source, char *keyword, int index);
int r_search(char *source, char *keyword, int index, bool adjacent_search);
double complex read_value(char *expr, int start, bool enable_complex);
void nice_print(char *format, double value, bool is_first);
int find_add_subtract(char *expr, int i);
int second_deepest_parenthesis(char *expr);
int previous_open_parenthesis(char *expr, int p);
int previous_closed_parenthesis(char *expr, int p);
int next_open_parenthesis(char *expr, int p);
int next_closed_parenthesis(char *expr, int p);
int next_deepest_parenthesis(char *expr, int p);
bool parenthesis_check(char *expr);
bool combine_add_subtract(char *expr, int a, int b);
bool part_of_keyword(char *expr, char *keyword1, char *keyword2, int index);
bool valid_result(char *expr);
int priority_test(char operator1, char operator2);
arg_list* get_arguments(char *string);
void free_arg_list(arg_list *list, bool list_on_heap);