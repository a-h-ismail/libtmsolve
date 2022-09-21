/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
bool is_infinite(char *exp, int index);
int find_closing_parenthesis(char *exp, int p);
int find_opening_parenthesis(char *exp, int p);
int find_deepest_parenthesis(char *exp);
bool is_op(char c);
int previousop(char *exp, int i);
int nextop(char *exp, int i);
void string_cleaner(char *exp);
void string_resizer(char *str, int o_end, int n_end);
int value_printer(char *exp, int a, int b, double v);
bool implicit_multiplication(char *exp);
bool var_implicit_multiplication(char *exp);
bool is_number(char c);
bool is_alphabetic(char c);
void variable_matcher(char *exp);
int find_endofnumber(char *exp, int start);
int find_startofnumber(char *exp, int end);
void var_to_val(char *exp, char *keyword, double value);
int s_search(char *source, char *keyword, int index);
int r_search(char *source, char *keyword, int index, bool adjacent_search);
double read_value(char *exp, int start);
void nice_print(char *format, double value, bool is_first);
int find_add_subtract(char *exp, int i);
int second_deepest_parenthesis(char *exp);
int previous_open_parenthesis(char *exp, int p);
int previous_closed_parenthesis(char *exp, int p);
int next_open_parenthesis(char *exp, int p);
int next_closed_parenthesis(char *exp, int p);
int next_deepest_parenthesis(char *exp, int p);
bool parenthesis_check(char *exp);
bool combine_add_subtract(char *exp, int a, int b);
bool part_of_keyword(char *exp, char *keyword1, char *keyword2, int index);
bool valid_result(char *exp);
void rps();