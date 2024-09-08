/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_handler.h"
#include "evaluator.h"
#include "function.h"
#include "internals.h"
#include "parser.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_complex.h"

#define dup_mexpr tms_dup_mexpr

#include "common.h"

int _tms_set_rcfunction_ptr(char *expr, tms_math_expr *M, int s_i)
{
    tms_math_subexpr *S = &(M->S[s_i]);
    int solve_start = S->solve_start;

    // Search for any function preceding the expression to set the function pointer
    if (solve_start > 1)
    {
        char *name = tms_get_name(expr, solve_start - 2, false);
        if (name == NULL)
            return 0;

        const tms_rc_func *func;
        func = tms_get_rc_func_by_name(name);
        if (func == NULL)
        {
            tms_save_error(TMS_PARSER, UNDEFINED_FUNCTION, EH_NONFATAL, expr, solve_start - 2);
            free(name);
            return -1;
        }

        if (!M->enable_complex)
        {
            S->func.real = func->real;
            S->func_type = TMS_F_REAL;
        }
        else
        {
            S->func.cmplx = func->cmplx;
            S->func_type = TMS_F_CMPLX;
        }
        S->subexpr_start = solve_start - strlen(func->name) - 1;
        free(name);
    }
    return 0;
}

int _tms_get_operand_value(tms_math_expr *M, int start, double complex *out)
{
    double complex value = NAN;
    bool is_negative = false;
    char *expr = M->expr;

    // Avoid negative offsets
    if (start < 0)
        return -1;

    // Catch incorrect start like )5 (no implied multiplication allowed)
    if (start > 0 && !tms_is_valid_number_start(expr[start - 1]))
    {
        tms_save_error(TMS_PARSER, SYNTAX_ERROR, EH_FATAL, expr, start - 1);
        return -1;
    }

    if (expr[start] == '-')
    {
        is_negative = true;
        ++start;
    }
    else
        is_negative = false;

    value = tms_read_value(expr, start);

    // Failed to read value normally, it is probably a variable
    if (isnan(creal(value)))
    {
        char *name = tms_get_name(expr, start, true);
        if (name == NULL)
            return -2;

        // Check if the name matches a defined variable
        const tms_var *v = tms_get_var_by_name(name);
        if (v != NULL)
            value = v->value;
        // ans is a special case
        else if (strcmp(name, "ans") == 0)
            value = tms_g_ans;
        else
        {
            // The name is already used by a function
            if (tms_function_exists(name))
                tms_save_error(TMS_PARSER, PARENTHESIS_MISSING, EH_FATAL, expr, start + strlen(name));
            else
                tms_save_error(TMS_PARSER, UNDEFINED_VARIABLE, EH_FATAL, expr, start);
            free(name);
            return -3;
        }
        free(name);
    }

    if (!M->enable_complex && cimag(value) != 0)
    {
        tms_save_error(TMS_PARSER, COMPLEX_DISABLED, EH_NONFATAL, expr, start);
        return -4;
    }

    if (is_negative)
        value = -value;

    *out = value;
    return 0;
}

tms_math_expr *tms_parse_expr(char *expr, int options, tms_arg_list *labels)
{
    tms_lock_parser(TMS_PARSER);

    if (tms_get_error_count(TMS_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_clear_errors(TMS_PARSER);
    }

    tms_math_expr *M = _tms_parse_expr_unsafe(expr, options, labels);
    if (M == NULL)
        tms_print_errors(TMS_PARSER);

    tms_unlock_parser(TMS_PARSER);
    return M;
}

tms_math_expr *_tms_parse_expr_unsafe(char *expr, int options, tms_arg_list *labels)
{
    // Number of subexpressions
    int s_count;
    // Used for indexing of subexpressions
    int s_i;

    bool enable_labels = (options & ENABLE_LABELS) && 1;
    bool enable_complex = (options & ENABLE_CMPLX) && 1;

    if (strlen(expr) > __INT_MAX__)
    {
        tms_save_error(TMS_PARSER, EXPRESSION_TOO_LONG, EH_FATAL, NULL, 0);
        return NULL;
    }
    // Duplicate the expression sent to the parser, it may be constant
    expr = strdup(expr);

    // Check for empty input
    if (expr[0] == '\0')
    {
        tms_save_error(TMS_PARSER, NO_INPUT, EH_FATAL, NULL, 0);
        free(expr);
        return NULL;
    }
    tms_remove_whitespace(expr);
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    _tms_combine_add_sub(expr);

    tms_math_expr *M = _tms_init_math_expr(expr);
    if (M == NULL)
        return NULL;
    else
        M->enable_complex = enable_complex;

    // Add the labels to the math expression if necessary
    M->label_names = (enable_labels ? labels : NULL);

    // After calling expression initializer, no need to manually free the "expr" string
    // It is now a part of the math_expr struct and will be freed with it

    tms_math_subexpr *S = M->S;
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
    - Set the subexpr result double pointer to the result pointer of the last calculated op_node
    */

    for (s_i = 0; s_i < s_count; ++s_i)
    {
        // Extended functions use a subexpression without nodes, but the subexpression result pointer should point at something
        // Allocate a small block and use that for the result pointer
        if (S[s_i].func_type == TMS_F_EXTENDED || S[s_i].func_type == TMS_F_USER)
        {
            S[s_i].result = malloc(sizeof(double complex *));
            continue;
        }

        // Get an array of the index of all operators and set their count
        int *operator_index = _tms_get_operator_indexes(expr, S, s_i);

        if (operator_index == NULL)
        {
            tms_delete_math_expr(M);
            return NULL;
        }

        status = _tms_set_rcfunction_ptr(expr, M, s_i);
        if (status == -1)
        {
            tms_delete_math_expr(M);
            free(operator_index);
            return NULL;
        }

        status = _tms_init_nodes(M, s_i, operator_index, enable_labels);
        free(operator_index);

        // Exiting due to error
        if (status == -1)
        {
            tms_delete_math_expr(M);
            return NULL;
        }

        status = _tms_set_all_operands(M, s_i, enable_labels);
        if (status == -1)
        {
            tms_delete_math_expr(M);
            return NULL;
        }

        status = _tms_set_evaluation_order(S + s_i);
        if (status == -1)
        {
            tms_delete_math_expr(M);
            return NULL;
        }
        _tms_set_result_pointers(M, s_i);
    }

    // Set labels metadata
    if (enable_labels)
        _tms_generate_labels_refs(M);

    return M;
}

void tms_convert_real_to_complex(tms_math_expr *M)
{
    // You need to swap real functions for their complex counterparts.
    tms_math_subexpr *S = M->S;
    if (S != NULL)
    {
        int s_i;
        char *function_name;
        for (s_i = 0; s_i < M->subexpr_count; ++s_i)
        {
            if (S[s_i].func_type == TMS_F_REAL)
            {
                // Lookup the name of the real function and find the equivalent function pointer
                function_name = tms_get_name(M->expr, S[s_i].subexpr_start, true);
                if (function_name == NULL)
                {
                    tms_save_error(TMS_PARSER, INTERNAL_ERROR, EH_FATAL, M->expr, S[s_i].subexpr_start);
                    return;
                }
                const tms_rc_func *func = tms_get_rc_func_by_name(function_name);
                free(function_name);
                if (func == NULL)
                {
                    tms_save_error(TMS_PARSER, INTERNAL_ERROR, EH_FATAL, M->expr, S[s_i].subexpr_start);
                    return;
                }
                if (func->cmplx == NULL && func->real != NULL)
                {
                    tms_save_error(TMS_PARSER, INTERNAL_ERROR, EH_FATAL, M->expr, S[s_i].subexpr_start);
                    return;
                }
                S[s_i].func.cmplx = func->cmplx;
                S[s_i].func_type = TMS_F_CMPLX;
            }
        }
    }
    M->enable_complex = true;
}

void _tms_set_priority(tms_op_node *list, int op_count)
{
    char operators[6] = {'^', '*', '/', '%', '+', '-'};
    uint8_t priority[6] = {3, 2, 2, 2, 1, 1};
    int i, j;
    for (i = 0; i < op_count; ++i)
    {
        for (j = 0; j < array_length(operators); ++j)
        {
            if (list[i].operator== operators[j])
            {
                list[i].priority = priority[j];
                break;
            }
        }
    }
}

bool tms_is_deterministic(tms_math_expr *M)
{
    for (int i = 0; i < M->subexpr_count; ++i)
    {
        if (M->S[i].func.extended == tms_rand)
            return false;
    }
    return true;
}
