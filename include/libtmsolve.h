/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef TMSOLVE_H
#define TMSOLVE_H
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <stdarg.h>
#include <inttypes.h>
// Defining the clear console command
/*
To calculate needed exp size :
expression_length *(max_length) + (max_length - 1) * (implicit_multiplication_overhead) + 1(null terminator)*/
// Previous answer global variable
#define EXP_SIZE(size) ((size)*23 + (size - 1) * 3 + 1)
extern double complex ans;
extern int g_nb_unknowns;
typedef struct node
{
    char operator;
    // Index of the operator in the expression
    int index;
    // Index of the node in the node array
    int node_index;
    // Used to store data about unknown nodes as follow:
    // b0:l_op, b1:r_op, b2:l_opNegative, b3:r_opNegative
    uint8_t variable_operands, priority;
    double LeftOperand, RightOperand, *result;
    struct node *next;
} node;
// Simple structure to hold pointer and sign of unknown members of an equation
typedef struct variable_data
{
    double *pointer;
    bool isNegative;
} variable_data;
typedef struct s_expression
{
    int op_count, depth;
    /*
    expression start
        |
        v
        cos(pi/3)
           ^    ^
    solve_start |
               end_index
    */
    int expression_start, solve_start, end_index;
    // The index of the node at which the subexpression solving start
    int start_node;
    struct node *node_list;
    double **result;
    // function to execute on the final result
    double (*function_ptr)(double);
    bool last_exp;
} s_expression;
// utility functions:
void utility_features();
#endif
void s_input(char **buffer, char *prompt, size_t n);
void flush_stdin();
int32_t *find_factors(int32_t a);
int find_min(int a, int b);
int error_handler(char *error, int arg, ...);
int find_closing_parenthesis(char *exp, int p);
int find_opening_parenthesis(char *exp, int p);
int find_deepest_parenthesis(char *exp);
void fraction_reducer(int32_t *f_v1, int32_t *f_v2, int32_t *v1, int32_t *v2);
bool is_op(char c);
char previousop(char *exp, int i);
int nextop(char *exp, int i);
void string_cleaner(char *exp);
void string_resizer(char *str, int o_end, int n_end);
int value_printer(char *exp, int a, int b, double v);
void error_print(char *, int);
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
void fraction_processor(char *exp, bool wasdecimal);
bool decimal_to_fraction(char *exp, bool inverse_process);
void factorize(char *exp);
void nroot_solver(char *exp);
void utility_functions(char *exp);
double get_value(char *prompt);
double read_value(char *exp, int start);
void nice_print(char *format, double value, bool is_first);
int find_add_subtract(char *exp, int i);
int second_deepest_parenthesis(char *exp);
int previous_open_parenthesis(char *exp, int p);
int previous_closed_parenthesis(char *exp, int p);
int next_open_parenthesis(char *exp, int p);
int next_closed_parenthesis(char *exp, int p);
int next_deepest_parenthesis(char *exp, int p);
int compare_subexps_depth(const void *a, const void *b);
bool parenthesis_check(char *exp);
bool combine_add_subtract(char *exp, int a, int b);
bool part_of_keyword(char *exp, char *keyword1, char *keyword2, int index);
bool valid_result(char *exp);
int subexp_start_at(s_expression **expression, int start, int current_s_exp, int mode);
int s_exp_ending(s_expression **expression, int end, int current_s_exp, int s_exp_count);
// scientific solving functions
void scientific_calculator(char *exp, bool calledbydefault);
int s_process(char *exp, int p);
double solve_s_exp(s_expression **subexps);
double pre_scientific_interpreter(char *exp);
s_expression **scientific_compiler(char *exp);
double factorial(double value);
double scientific_interpreter(char *exp, bool int_der);
double solve_region(char *exp, int a, int b);
void scientific_complex_picker(char *exp);
void delete_s_exp(s_expression **subexps);
int priority_test(char operator1, char operator2);
void priority_fill(node *list, int op_count);
void rps();
// matrix functions
double *matrix_input(int a, int b);
void matrix_print(double *A);
double *find_subm(double *A, int i, int j);
double *matrix_multiply(double *A, double *B);
double matrix_det(double *A);
double *comatrix(double *M);
double *matrix_tr(double *M);
double *matrix_inv(double *M);
void remplace_matrix_column(double *matrix, double *matrix_column, int column);
double *matrix_copy(double *matrix);
void matrix_mode();
// function mode functions
void function_calculator();
double calculate_function(char *exp, int a, int b, double x);
bool derivative_processor(char *exp);
bool derivative(char *exp);
double integrate(char *exp, double a, double b);
bool integral_processor(char *exp);
// Equation mode functions
void equation_mode();
void system_solver(int degree);
void equation_solver(int degree);
// Complex functions
void complex_mode();
int c_process(char *exp, int p);
bool complex_stage1_solver(char *exp, int a, int b);
int complex_stage2_solver(char *exp, int a, int b);
double complex read_complex(char *exp, int a, int b);
int s_complex_print(char *exp, int a, int b, double complex value);
void complex_print(double complex value);
bool is_imaginary(char *exp, int a, int b);
void complex_interpreter(char *exp);
#endif