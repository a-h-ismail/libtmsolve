/*
Copyright (C) 2022-2025 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_STRING_TOOLS_H
#define _TMS_STRING_TOOLS_H
/**
 * @file
 * @brief Declares all string handling related macros, structures, globals and functions.
 */

#ifndef LOCAL_BUILD
#include <tmsolve/c_complex_to_cpp.h>
#include <tmsolve/tms_math_strs.h>
#else
#include "c_complex_to_cpp.h"
#include "tms_math_strs.h"
#endif

#include <stddef.h>

/**
 * @brief Acts exactly like strndup(), exists solely to handle some compilers not implementing strndup themselves
 */
char *tms_strndup(const char *source, size_t n);

/**
 * @brief Finds the closing parenthesis that corresponds to an open parenthesis.
 * @param expr The string to search.
 * @param i The index of the open parenthesis.
 * @return The index of the closing parenthesis, or -1 if an error occured.
 */
int tms_find_closing_parenthesis(char *expr, int i);

/**
 * @brief Finds the open parenthesis that corresponds to a closing parenthesis.
 * @param expr The string to search.
 * @param p The index of the closing parenthesis.
 * @return The index of the open parenthesis, or -1 if an error occured.
 */
int tms_find_opening_parenthesis(char *expr, int p);

/**
 * @brief Checks if the character is an arithmetic operator.
 * @param c The character to test.
 * @return true if the character is an operator, false otherwise.
 */
bool tms_is_op(char c);

int tms_is_long_op(char *expr);

char tms_long_op_to_char(char *expr);

bool tms_is_int_op(char c);

int tms_is_int_long_op(char *expr);

char tms_int_long_op_to_char(char *expr);

/**
 * @brief Checks if the character is a correct start delimiter for operators.
 */
bool tms_is_valid_number_start(char c);

/**
 * @brief Checks if the character is a correct end delimiter for operators.
 */
bool tms_is_valid_number_end(char c);

int8_t tms_bin_to_int(char c);

int8_t tms_dec_to_int(char c);

int8_t tms_oct_to_int(char c);

int8_t tms_hex_to_int(char c);

int8_t tms_detect_base(char *_s);

/**
 * @brief Finds the next occurence of an operator in the string.
 * @param expr The string to search.
 * @param i The index to begin searching from.
 * @return The index of the next operator in the string, or -1 if none is found.
 */
int tms_next_op(char *expr, int i);

/**
 * @brief Deletes whitespaces from a string.
 * @param str The string to delete whitespace from it.
 */
void tms_remove_whitespace(char *str);

/**
 * @brief Moves all characters starting from old_end to a new index: new_end
 * @param str The string to operate on.
 * @param old_end The old end index of the region to resize.
 * @param new_end The desired end index.
 * @warning Make sure that your string has enough space to handle an increase in the string length.
 */
void tms_resize_zone(char *str, int old_end, int new_end);

/**
 * @brief Finds the end of a number knowing its start in the expression.
 * @param expr The expression to search.
 * @param start The start index of the number.
 * @return The index of the number's end, or -1 in case of failure.
 */
int tms_find_endofnumber(char *expr, int start);

int tms_find_int_endofnumber(char *number, int start);

bool tms_is_valid_int_number_start(char c);

/**
 * @brief Finds the start of a number knowing its end in the expression.
 * @param expr The expression to search.
 * @param end The end index of the number.
 * @return The index of the number's start, or -1 in case of failure.
 */
int tms_find_startofnumber(char *expr, int end);

/**
 * @brief Forward search for a specified keyword.
 * @param str The string to be searched.
 * @param keyword The keyword to look for.
 * @param index The index from which the search will start.
 * @return The index of the first match of keyword in str, or -1 if no match is found.
 */
int tms_f_search(char *str, char *keyword, int index, bool match_word);

/**
 * @brief Reverse search for a specified keyword.
 * @param str The string to be searched.
 * @param keyword The keyword to look for.
 * @param index The index from which the search will start.
 * @param adjacent_search If set to true, the search will stop after going back strlen(keyword)-1 characters. Otherwise proceed normally.
 * @return The index of the first match of keyword in str, or -1 if no match is found.
 */
int tms_r_search(char *str, char *keyword, int index, bool adjacent_search);

/**
 * @brief Reads the value according to the rules of the specified base.
 * @param base Supported values: 2, 8, 10, 16
 * @return The value on success, or NaN on failure.
 */
double _tms_read_value_simple(char *number, int8_t base);

/**
 * @brief Emulates the behavior of sscanf with format specifier "%lf".
 * This implementation is faster than using sscanf and supports decimal exponent in scientific notation.
 * @param _s Start of the value to read
 * @return The result or NaN in case of failure.
 */
cdouble tms_read_value(char *_s, int start);

/**
 * @brief Reads an 64 bit int value
 * @param _s
 * @param start
 * @param result
 * @return 0 on success, -1 on failure.
 */
int tms_read_int_value(char *_s, int start, int64_t *result);

int _tms_read_int_helper(char *number, int8_t base, int64_t *result);

/**
 * @brief Finds the next occurence of add or subtract sign.
 * @param expr The string to be searched.
 * @param i The index from which the search starts.
 * @return The index of the occurence, -1 if none is found.
 */
int tms_find_add_subtract(char *expr, int i);

/**
 * @brief Checks that each open parenthesis has a closing parenthesis and that no empty parenthesis pair exists.
 * @param expr The expression to check.
 * @return
 */
int tms_parenthesis_check(char *expr);

/// @brief Prints a value to standard output in a clean way.
/// @param value
void tms_print_value(cdouble value);

/**
 * @brief Generates the string representation of the specified complex value
 * @return A malloc'd string with the value.
 */
char *tms_complex_to_str(cdouble value);

/**
 * @brief Combines adjacent add/subtract symbols into one (ex: ++++ becomes + and +-+ becomes -).
 * @param expr The expression to process.
 * @param a Start of the block to process.
 * @param b End of block to process.
 */
void _tms_combine_add_sub(char *expr);

/**
 * @brief Compares the priority of 2 operators.
 * @return priority1 - priority2.
 */
int tms_compare_priority(char operator1, char operator2);

/**
 * @brief Extracts arguments separated by "," from a string.
 * @param string The string containing the arguments.
 * @return A structure containing the arguments stored in separate strings.
 */
tms_arg_list *tms_get_args(char *string);

/**
 * @brief Converts an argument list back into its string form.
 * @return The comma separated string on success, or NULL on failure.
 */
char *tms_args_to_string(tms_arg_list *args);

/**
 * @brief Creates a copy of the argument list.
 * @return The new argument list, or NULL on failure.
 */
tms_arg_list *tms_dup_arg_list(tms_arg_list *L);

/**
 * @brief Determines if the specified character is allowed in variable names.
 */
bool tms_legal_char_in_name(char c);

/**
 * @brief Detects if a keyword matches in the string at position i
 * @param str The string to look into.
 * @param i Index in the string (may be the start or end of the keyword)
 * @param keyword Pointer to the keyword.
 * @param match_from_start true if i is at the start of the keyword, false if at the end.
 * @return
 */
bool tms_match_word(char *str, int i, char *keyword, bool match_from_start);

/**
 * @brief Checks if the name conforms to variable name rules.
 * @return true if the name is valid, false otherwise.
 */
bool tms_valid_name(char *name);

/**
 * @brief Frees an argument list created with tms_get_args()
 */
void tms_free_arg_list(tms_arg_list *list);

void tms_print_bin(int64_t value);

void tms_print_oct(int64_t value);

void tms_print_hex(int64_t value);

void tms_print_dot_decimal(int64_t value);

int tms_name_bounds(char *expr, int i, bool is_at_start);

/**
 * @brief Finds the name of the function/variable that starts/ends at "i"
 * @param expr
 * @param i
 * @param is_at_start
 * @return The name if found (allocated with malloc), or NULL if the name isn't valid.
 */
char *tms_get_name(char *expr, int i, bool is_at_start);

bool tms_legal_name(char *name);

/**
 * @brief Finds the index of a string within an array of strings
 * @param key The string to find
 * @param array The array of elements (not necessarily a char array)
 * @param arr_len Number of elements in the array
 * @param type Type of the elements in the array (_TMS_F_REAL...)
 * @return The index of the first match, or -1 if no match is found.
 */
int tms_find_str_in_array(char *key, const void *array, int arr_len, uint8_t type);

bool _tms_string_is_prefix(const char *target, const char *prefix);

/**
 * @brief Concatenate two strings into a new string
 * @param s1 The first string
 * @param s2 The second string
 * @return A new string, s1 + s2
 */
char *tms_strcat_dup(char *s1, char *s2);

/**
 * @brief Verifies if the provided array of strings is made of unique strings.
 * @return true if the array is made of unique elements, false otherwise.
 */
bool tms_is_unique_string_array(char **array, int size);

#endif