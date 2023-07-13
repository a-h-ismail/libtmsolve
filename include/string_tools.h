/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef LOCAL_BUILD
#include <tmsolve/internals.h>
#else
#include "internals.h"
#endif
/**
 * @file
 * @brief Declares all string handling related macros, structures, globals and functions.
 */
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

/**
 * @brief Moves all characters starting from old_end to a new index: new_end
 * @param str The string to operate on.
 * @param old_end
 * @param n_end
 */
void resize_zone(char *str, int old_end, int new_end);

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
 * @brief Checks if the character c is a letter or digit.
 * @return true if c is a letter (A-Z) or (a-z) or a digit.
 */
bool is_alphanum(char c);

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

/**
 * @brief Forward search for a specified keyword.
 * @param str The string to be searched.
 * @param keyword The keyword to look for.
 * @param index The index from which the search will start.
 * @return The index of the first match of keyword in str, or -1 if no match is found.
 */
int f_search(char *str, char *keyword, int index, bool match_word);

/**
 * @brief Reverse search for a specified keyword.
 * @param str The string to be searched.
 * @param keyword The keyword to look for.
 * @param index The index from which the search will start.
 * @param adjacent_search If set to true, the search will stop after going back strlen(keyword)-1 characters. Otherwise procced normally.
 * @return The index of the first match of keyword in str, or -1 if no match is found.
 */
int r_search(char *str, char *keyword, int index, bool adjacent_search);

/**
 * @brief Reads a value from the expression at start, supports numbers, constants and variables.
 * @param expr The string to read the value from.
 * @param start The index where the start of the value is located.
 * @param enable_complex Toggles complex number support.
 * @return The value read from the string, or NaN in case of failure.
 */
double complex read_value(char *expr, int start, bool enable_complex);

/**
 * @brief Finds the next occurence of add or subtract sign.
 * @param expr The string to be searched.
 * @param i The index from which the search starts.
 * @return The index of the occurence, -1 if none is found.
 */
int find_add_subtract(char *expr, int i);

/**
 * @brief Checks that each open parenthesis has a closing parenthesis and that no empty parenthesis pair exists.
 * @param expr The expression to check.
 * @return
 */
bool parenthesis_check(char *expr);

/// @brief Prints a value to standard output in a clean way.
/// @param value 
void print_value(double complex value);

/// @brief Checks the syntax of the expression, runs parenthesis_check implicitly.
/// @return true if the check doesn't find errors, false otherwise.
bool syntax_check(char *expr);

/**
 * @brief Combines adjacent add/subtract symbols into one (ex: ++++ becomes + and +-+ becomes -).
 * @param expr The expression to process.
 * @param a Start of the block to process.
 * @param b End of block to process.
 */
void combine_add_subtract(char *expr, int a, int b);

/**
 * @brief Compares the priority of 2 operators.
 * @return priority1 - priority2.
 */
int compare_priority(char operator1, char operator2);

/**
 * @brief Extracts arguments separated by "," from a string.
 * @param string The string containing the arguments.
 * @return A structure containing the arguments stored in separate strings.
 */
arg_list* get_arguments(char *string);

/**
 * @brief Ensures the expression is not empty, checks syntax and merges extra +/- signs.
 * @return true if the checks pass, false otherwise.
*/
bool pre_parse_routine(char *expr);

/**
 * @brief Determines if the specified character is allowed in variable names.
 */
bool legal_char_in_name(char c);

/**
 * @brief Detects if a keyword matches fully in the string at position i
 * @param str The string to look into.
 * @param i Index in the string (may be the start or end of the keyword)
 * @param keyword Pointer to the keyword.
 * @param match_from_start true if i is at the start of the keyword, false if at the end.
 * @return 
 */
bool match_word(char *str, int i, char *keyword, bool match_from_start);

/**
 * @brief Checks if the name conforms to variable name rules.
 * @return true if the name is valid, false otherwise.
 */
bool valid_name(char *name);

void free_arg_list(arg_list *list, bool list_on_heap);