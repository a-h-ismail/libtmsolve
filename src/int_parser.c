/*
Copyright (C) 2023-2024 Ahmad Ismail
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
#define operand_type int64_t
#define get_operand_value _tms_read_int_operand
#define is_op tms_is_int_op
#define set_priority _tms_set_priority_int
#define MAX_PRIORITY 5
#define dup_mexpr tms_dup_int_expr

#include "common.h"

const tms_int_func tms_g_int_func[] = {{"not", tms_not},
                                       {"mask", tms_mask},
                                       {"mask_bit", tms_mask_bit},
                                       {"inv_mask", tms_inv_mask},
                                       {"ipv4_prefix", tms_ipv4_prefix},
                                       {"zeros", tms_zeros},
                                       {"ones", tms_ones}};

const int tms_g_int_func_count = array_length(tms_g_int_func);

const tms_int_extf tms_g_int_extf[] = {{"rand", tms_int_rand}, {"rr", tms_rr},
                                       {"rl", tms_rl},         {"sr", tms_sr},
                                       {"sra", tms_sra},       {"sl", tms_sl},
                                       {"nand", tms_nand},     {"and", tms_and},
                                       {"xor", tms_xor},       {"nor", tms_nor},
                                       {"or", tms_or},         {"ipv4", tms_ipv4},
                                       {"dotted", tms_dotted}, {"mask_range", tms_mask_range}};

const int tms_g_int_extf_count = array_length(tms_g_int_extf);

int _tms_read_int_operand(tms_int_expr *M, int start, int64_t *result)
{
    int64_t value;
    int status;
    bool is_negative = false;
    char *expr = M->expr;

    // Avoid negative offsets
    if (start < 0)
        return -1;

    // Catch incorrect start like )5 (no implied multiplication allowed)
    if (start > 0 && !tms_is_valid_int_number_start(expr[start - 1]))
    {
        tms_save_error(TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, expr, start - 1);
        return -1;
    }

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

        int i = tms_find_str_in_array(name, tms_g_int_vars, tms_g_int_var_count, TMS_V_INT64);
        if (i != -1)
            value = tms_g_int_vars[i].value;
        // ans is a special case
        else if (strcmp(name, "ans") == 0)
            value = tms_g_int_ans;
        else
        {
            // The name is already used by a function
            if (tms_find_str_in_array(name, tms_g_all_int_func_names, tms_g_all_int_func_count, TMS_NOFUNC) != -1)
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

    *result = value & tms_int_mask;
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

tms_int_expr *_tms_init_int_expr(char *expr)
{
    int s_max = 8, i, j, s_i, length = strlen(expr), s_count;

    // Pointer to subexpressions heap array
    tms_int_subexpr *S;

    tms_int_expr *M = malloc(sizeof(tms_int_expr));

    M->S = NULL;
    M->subexpr_count = 0;
    M->expr = expr;

    S = malloc(s_max * sizeof(tms_int_subexpr));

    int depth = 0;
    s_i = 0;
    bool is_extended;
    // Determine the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        if (expr[i] == '(')
        {
            DYNAMIC_RESIZE(S, s_i, s_max, tms_int_subexpr)
            is_extended = false;
            S[s_i].nodes = NULL;
            S[s_i].depth = ++depth;

            // Treat extended functions as a subexpression
            if (i > 1 && tms_legal_char_in_name(expr[i - 1]))
            {
                char *name = tms_get_name(expr, i - 1, false);

                // The function name is not valid
                if (name == NULL)
                {
                    tms_save_error(TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, expr, i - 1);
                    free(S);
                    tms_delete_int_expr(M);
                    return NULL;
                }

                j = tms_find_str_in_array(name, tms_g_int_extf, tms_g_int_extf_count, TMS_F_INT_EXTENDED);

                // It is an extended function indeed
                if (j != -1)
                {
                    is_extended = true;
                    S[s_i].subexpr_start = i - strlen(name);
                    S[s_i].solve_start = i + 1;
                    i = tms_find_closing_parenthesis(expr, i);
                    if (i == -1)
                    {
                        tms_save_error(TMS_INT_PARSER, PARENTHESIS_NOT_CLOSED, EH_FATAL, expr, S[s_i].solve_start - 1);
                        M->S = S;
                        free(name);
                        tms_delete_int_expr(M);
                        return NULL;
                    }
                    S[s_i].solve_end = i - 1;
                    S[s_i].func.extended = tms_g_int_extf[j].ptr;
                    S[s_i].func_type = TMS_F_INT_EXTENDED;
                    S[s_i].start_node = -1;
                    S[s_i].op_count = 0;
                    S[s_i].exec_extf = true;

                    // Decrement i so that the loop counter would hit the closing parenthesis and perform checks
                    --i;
                }
                free(name);
            }
            if (is_extended == false)
            {
                // Not an extended function, either no function at all or a regular function
                S[s_i].solve_start = i + 1;
                S[s_i].func.extended = NULL;
                S[s_i].func_type = TMS_NOFUNC;
                S[s_i].exec_extf = false;

                // The expression start is the parenthesis, may change if a function is found
                S[s_i].subexpr_start = i;
                S[s_i].solve_end = tms_find_closing_parenthesis(expr, i) - 1;

                // Empty parenthesis pair is only allowed for extended functions
                if (S[s_i].solve_end == i)
                {
                    tms_save_error(TMS_INT_PARSER, PARENTHESIS_EMPTY, EH_FATAL, expr, i);
                    free(S);
                    tms_delete_int_expr(M);
                    return NULL;
                }

                if (S[s_i].solve_end == -2)
                {
                    tms_save_error(TMS_INT_PARSER, PARENTHESIS_NOT_CLOSED, EH_FATAL, expr, i);
                    // S isn't part of M yet
                    free(S);
                    tms_delete_int_expr(M);
                    return NULL;
                }
            }
            ++s_i;
        }
        else if (expr[i] == ')')
        {
            // An extra ')'
            if (depth == 0)
            {
                tms_save_error(TMS_INT_PARSER, PARENTHESIS_NOT_OPEN, EH_FATAL, expr, i);
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }
            else
                --depth;

            // Make sure a ')' is followed by an operator, ')' or \0
            if (!(tms_is_int_op(expr[i + 1]) || expr[i + 1] == ')' || expr[i + 1] == '\0'))
            {
                tms_save_error(TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, expr, i + 1);
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }
        }
    }
    // + 1 for the subexpression with depth 0
    s_count = s_i + 1;
    // Shrink the block to the required size
    S = realloc(S, s_count * sizeof(tms_int_subexpr));

    // Copy the pointer to the structure
    M->S = S;

    M->subexpr_count = s_count;

    // The whole expression's "subexpression"
    S[s_i].depth = 0;
    S[s_i].solve_start = S[s_i].subexpr_start = 0;
    S[s_i].solve_end = length - 1;
    S[s_i].func.extended = NULL;
    S[s_i].nodes = NULL;
    S[s_i].func_type = TMS_NOFUNC;
    S[s_i].exec_extf = true;

    // Sort by depth (high to low)
    qsort(S, s_count, sizeof(tms_int_subexpr), _tms_compare_int_subexpr_depth);
    return M;
}

int _tms_set_int_function_ptr(char *expr, tms_int_expr *M, int s_i)
{
    int i;
    tms_int_subexpr *S = &(M->S[s_i]);
    int solve_start = S->solve_start;

    // Search for any function preceding the expression to set the function pointer
    if (solve_start > 1)
    {
        char *name = tms_get_name(expr, solve_start - 2, false);
        if (name == NULL)
            return 0;

        if ((i = tms_find_str_in_array(name, tms_g_int_func, tms_g_int_func_count, TMS_F_INT64)) != -1)
        {
            S->func.simple = tms_g_int_func[i].ptr;
            S->func_type = TMS_F_INT64;
            // Set the start of the subexpression to the start of the function name
            S->subexpr_start = solve_start - strlen(tms_g_int_func[i].name) - 1;
            free(name);
            return 0;
        }
        else
        {
            tms_save_error(TMS_INT_PARSER, UNDEFINED_FUNCTION, EH_FATAL, expr, solve_start - 2);
            free(name);
            return -1;
        }
    }
    return 0;
}

tms_int_expr *tms_parse_int_expr(char *expr, int options, tms_arg_list *unknowns)
{
    tms_lock_parser(TMS_INT_PARSER);

    if (tms_get_error_count(TMS_INT_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_clear_errors(TMS_INT_PARSER);
    }

    tms_int_expr *M = _tms_parse_int_expr_unsafe(expr, options, unknowns);
    if (M == NULL)
        tms_print_errors(TMS_INT_PARSER);

    tms_unlock_parser(TMS_INT_PARSER);
    return M;
}

tms_int_expr *_tms_parse_int_expr_unsafe(char *expr, int options, tms_arg_list *unknowns)
{
    // Number of subexpressions
    int s_count;
    // Current subexpression index
    int s_i;

    bool enable_unknowns = (options & TMS_ENABLE_UNK) && 1;

    // Check for empty input
    if (expr[0] == '\0')
    {
        tms_save_error(TMS_INT_PARSER, NO_INPUT, EH_FATAL, NULL, 0);
        return NULL;
    }

    if (strlen(expr) > __INT_MAX__)
    {
        tms_save_error(TMS_INT_PARSER, EXPRESSION_TOO_LONG, EH_FATAL, NULL, 0);
        return NULL;
    }

    // Duplicate the expression sent to the parser, it may be constant
    expr = strdup(expr);

    tms_remove_whitespace(expr);
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    _tms_combine_add_sub(expr);

    tms_int_expr *M = _tms_init_int_expr(expr);
    if (M == NULL)
        return NULL;

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
    - Fill nodes with values or set as unknown (using x)
    - Set nodes order of calculation (using *next)
    - Set the result pointer of each op_node relying on its position and neighbor priorities
    - Set the subexpr result double pointer to the result pointer of the last op_node in the calculation order
    */

    for (s_i = 0; s_i < s_count; ++s_i)
    {
        // Extended functions use a subexpression without nodes, but the subexpression result pointer should point at something
        // Allocate a small block and use that for the result pointer
        if (S[s_i].func_type == TMS_F_INT_EXTENDED)
        {
            S[s_i].result = malloc(sizeof(double complex *));
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

        status = _tms_init_nodes(M, s_i, operator_index, enable_unknowns);
        free(operator_index);

        // Exiting due to error
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_all_operands(M, s_i, enable_unknowns);
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

    return M;
}

void tms_delete_int_expr(tms_int_expr *M)
{
    if (M == NULL)
        return;

    int i = 0;
    tms_int_subexpr *S = M->S;
    for (i = 0; i < M->subexpr_count; ++i)
    {
        if (S[i].func_type == TMS_F_INT_EXTENDED)
            free(S[i].result);
        free(S[i].nodes);
    }
    free(S);
    free(M->expr);
    free(M);
}

void _tms_set_priority_int(tms_int_op_node *list, int op_count)
{
    char operators[] = {'*', '/', '%', '+', '-', '&', '^', '|'};
    uint8_t priority[] = {5, 5, 5, 4, 4, 3, 2, 1};
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

char *_tms_lookup_int_function_name(void *function, int func_type)
{
    int i;
    switch (func_type)
    {
    case TMS_F_INT64:
        for (i = 0; i < tms_g_int_func_count; ++i)
        {
            if (function == (void *)(tms_g_int_func[i].ptr))
                break;
        }
        if (i < tms_g_int_func_count)
            return tms_g_int_func[i].name;
        else
            break;

    case TMS_F_INT_EXTENDED:
        for (i = 0; i < tms_g_int_extf_count; ++i)
        {
            if (function == (void *)(tms_g_int_extf[i].ptr))
                break;
        }
        if (i < tms_g_int_extf_count)
            return tms_g_int_extf[i].name;
        else
            break;
    default:
        tms_save_error(TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, NULL, 0);
    }

    return NULL;
}
