/*
Copyright (C) 2023-2026 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "int_parser.h"
#include "bitwise.h"
#include "error_handler.h"
#include "evaluator.h"
#include "function.h"
#include "internals.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_complex.h"
#include "tms_math_strs.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OVERRIDE_DEFAULTS
#define math_expr tms_int_expr
#define math_subexpr tms_int_subexpr
#define op_node tms_int_op_node
#define PARSER TMS_INT_PARSER
#define F_EXTENDED TMS_F_INT_EXTENDED
#define F_USER TMS_F_INT_USER
#define init_math_expr _tms_init_int_expr
#define delete_math_expr tms_delete_int_expr
#define delete_math_expr_members tms_delete_int_expr_members
#define ufunc tms_int_ufunc
#define extf tms_int_extf
#define get_ufunc_by_name tms_get_int_ufunc_by_name
#define get_extf_by_name tms_get_int_extf_by_name
#define operand_type int64_t
#define get_operand_value _tms_read_int_operand
#define is_op tms_is_int_op
#define is_long_op tms_is_int_long_op
#define long_op_to_char tms_int_long_op_to_char
#define set_priority _tms_set_priority_int
#define MAX_PRIORITY 7
#define dup_mexpr tms_dup_int_expr

#include "parser_common.h"

int _tms_read_int_operand(tms_int_expr *M, int start, int64_t *result)
{
    int64_t value;
    int status;
    bool is_negative = false;
    char *expr = M->expr;

    // Avoid negative offsets
    if (start < 0)
        return -1;

    if (expr[start] == '-')
    {
        is_negative = true;
        ++start;
    }
    else
        is_negative = false;

    status = tms_read_int_value(expr, start, &value);
    switch (status)
    {
    // Reading value was successful
    case 0:
        break;

        // Failed to read value normally, it is probably a variable
    case -1:
        char *name = tms_get_name(expr, start, true);
        if (name == NULL)
            return -1;

        // Check if the name matches a defined variable
        const tms_int_var *v = tms_get_int_var_by_name(name);
        if (v != NULL)
            value = v->value;
        // ans is a special variable
        else if (strcmp(name, "ans") == 0)
            value = tms_g_int_ans;
        else
        {
            // The name is already used by a function
            if (tms_int_function_exists(name))
                tms_save_error(TMS_INT_PARSER, PARENTHESIS_MISSING, EH_FATAL, expr, start + strlen(name));
            else
                tms_save_error(TMS_INT_PARSER, UNDEFINED_VARIABLE, EH_FATAL, expr, start);
            free(name);
            return -1;
        }
        free(name);
        break;

    case -2:
        tms_save_error(TMS_INT_PARSER, INTEGER_OVERFLOW, EH_FATAL, expr, start);
        return -1;

    case -3:
        tms_save_error(TMS_INT_PARSER, INT_TOO_LARGE, EH_FATAL, expr, start);
        return -1;

    default:
        tms_save_error(TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, expr, start);
        return -1;
    }

    if (is_negative)
        value = -value;

    *result = value;
    return 0;
}

int _tms_compare_int_subexpr_depth(const void *a, const void *b)
{
    if (((tms_int_subexpr *)a)->depth < ((tms_int_subexpr *)b)->depth)
        return 1;
    else if ((*((tms_int_subexpr *)a)).depth > (*((tms_int_subexpr *)b)).depth)
        return -1;
    else
        return 0;
}

int _tms_set_int_function_ptr(const char *expr, tms_int_expr *M, int s_i)
{
    tms_int_subexpr *S = &(M->S[s_i]);
    int solve_start = S->solve_start;

    // Search for any function preceding the expression to set the function pointer
    if (solve_start > 1)
    {
        char *name = tms_get_name(expr, solve_start - 2, false);
        if (name == NULL)
            return 0;

        const tms_int_func *func;
        func = tms_get_int_func_by_name(name);
        if (func == NULL)
        {
            tms_save_error(TMS_INT_PARSER, UNDEFINED_FUNCTION, EH_NONFATAL, expr, solve_start - 2);
            free(name);
            return -1;
        }

        S->func.simple = func->ptr;
        S->func_type = TMS_F_INT64;
        S->subexpr_start = solve_start - strlen(func->name) - 1;
        free(name);
    }
    return 0;
}

tms_int_expr *_tms_parse_int_expr_unsafe(const char *expr, int options, tms_arg_list *labels);

tms_int_expr *tms_parse_int_expr(const char *expr, int options, tms_arg_list *labels)
{
    if ((options & NO_LOCK) != 1)
        tms_lock_parser(TMS_INT_PARSER);

    if (tms_get_error_count(TMS_INT_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_clear_errors(TMS_INT_PARSER);
    }

    tms_int_expr *M = _tms_parse_int_expr_unsafe(expr, options, labels);
    if (M == NULL && (options & PRINT_ERRORS) != 0)
        tms_print_errors(TMS_INT_PARSER);

    if ((options & NO_LOCK) != 1)
        tms_unlock_parser(TMS_INT_PARSER);
    return M;
}

tms_int_expr *_tms_parse_int_expr_unsafe(const char *expr_const, int options, tms_arg_list *labels)
{
    // Number of subexpressions
    int s_count;
    // Current subexpression index
    int s_i;

    bool enable_labels = (labels != NULL);

    // Check for empty input
    if (expr_const[0] == '\0')
    {
        tms_save_error(TMS_INT_PARSER, NO_INPUT, EH_FATAL, NULL, 0);
        return NULL;
    }

    if (strlen(expr_const) > __INT_MAX__)
    {
        tms_save_error(TMS_INT_PARSER, EXPRESSION_TOO_LONG, EH_FATAL, NULL, 0);
        return NULL;
    }

    // Duplicate the expression sent to the parser, it may be constant
    char *expr = strdup(expr_const);

    tms_remove_whitespace(expr);
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    _tms_combine_add_sub(expr);

    tms_int_expr *M = _tms_init_int_expr(expr);
    if (M == NULL)
        return NULL;

    // Add the labels to the math expression if necessary
    M->labels = (enable_labels ? labels : NULL);

    tms_int_subexpr *S = M->S;
    s_count = M->subexpr_count;

    int status;

    /*
    How to parse a subexpression:
    - Handle the special case of extended functions
    - Create an array that determines the location of operators in the string
    - Check if the subexpression has a function to execute on the result, set pointer
    - Allocate the array of nodes
    - Use the operator index array to fill the nodes data on operators type and location
    - Fill nodes operator priority
    - Fill nodes with values or set as label (using x)
    - Set nodes order of calculation (using *next)
    - Set the result pointer of each op_node relying on its position and neighbor priorities
    - Set the subexpr result double pointer to the result pointer of the last op_node in the calculation order
    */

    for (s_i = 0; s_i < s_count; ++s_i)
    {
        // Extended functions use a subexpression without nodes, but the subexpression result pointer should point at something
        // Allocate a small block and use that for the result pointer
        if (S[s_i].func_type == TMS_F_INT_EXTENDED || S[s_i].func_type == TMS_F_INT_USER)
        {
            S[s_i].result = malloc(sizeof(int64_t *));
            continue;
        }

        // Get an array of the index of all operators and set their count
        int *operator_index = _tms_get_operator_indexes(expr, S, s_i);

        if (operator_index == NULL)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_int_function_ptr(expr, M, s_i);
        if (status == -1)
        {
            tms_delete_int_expr(M);
            free(operator_index);
            return NULL;
        }

        status = _tms_init_nodes(M, s_i, operator_index, enable_labels);
        free(operator_index);

        // Exiting due to error
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_all_operands(M, s_i, enable_labels);
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_evaluation_order(S + s_i);
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }
        _tms_set_result_pointers(M, s_i);
    }

    if (enable_labels)
    {
        _tms_generate_labels_refs(M);
        // If we have values, set them
        if (M->labels->payload != NULL)
            tms_set_int_labels_values(M, M->labels->payload);
    }

    return M;
}

void _tms_set_priority_int(tms_int_op_node *list, int op_count)
{
    char operators[] = {'p', '*', '/', '%', '+', '-', '<', '>', 'l', 'r', '&', '^', '|'};
    uint8_t priority[] = {7, 6, 6, 6, 5, 5, 4, 4, 4, 4, 3, 2, 1};
    int i, j;
    for (i = 0; i < op_count; ++i)
    {
        for (j = 0; j < array_length(operators); ++j)
        {
            if (list[i].op == operators[j])
            {
                list[i].priority = priority[j];
                break;
            }
        }
    }
}
