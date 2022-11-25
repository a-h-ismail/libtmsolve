/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
// Errors
#define PARAMETER_MISSING "The function expects more arguments."
#define PARENTHESIS_EMPTY "Empty parenthesis pair."
#define PARENTHESIS_NOT_CLOSED "Open parenthesis has no closing parenthesis."
#define PARENTHESIS_NOT_OPEN "Extra closing parenthesis."
// Function declarations
bool is_infinite(char *expr, int index);
int find_closing_parenthesis(char *expr, int p);
int find_opening_parenthesis(char *expr, int p);
int find_deepest_parenthesis(char *expr);
bool is_op(char c);
int previousop(char *expr, int i);
int nextop(char *expr, int i);
void remove_whitespace(char *expr);
void string_resizer(char *str, int o_end, int n_end);
int value_printer(char *expr, int a, int b, double v);
bool implicit_multiplication(char **expr);
bool var_implicit_multiplication(char *expr);
bool is_number(char c);
bool is_alphabetic(char c);
void variable_matcher(char *expr);
int find_endofnumber(char *expr, int start);
int find_startofnumber(char *expr, int end);
void var_to_val(char *expr, char *keyword, double value);
int s_search(char *source, char *keyword, int index);
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
arg_list* get_arguments(char *string);
void free_arg_list(arg_list *list, bool list_on_heap);