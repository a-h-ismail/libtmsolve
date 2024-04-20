/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "int_parser.h"
#include "bitwise.h"
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

const tms_int_func tms_g_int_func[] = {{"not", tms_not},
                                       {"mask", tms_mask},
                                       {"mask_bit", tms_mask_bit},
                                       {"inv_mask", tms_inv_mask},
                                       {"ipv4_prefix", tms_ipv4_prefix},
                                       {"zeros", tms_zeros},
                                       {"ones", tms_ones}};

const int tms_g_int_func_count = array_length(tms_g_int_func);

const tms_int_extf tms_g_int_extf[] = {{"rr", tms_rr},
                                       {"rl", tms_rl},
                                       {"sr", tms_sr},
                                       {"sra", tms_sra},
                                       {"sl", tms_sl},
                                       {"nand", tms_nand},
                                       {"and", tms_and},
                                       {"xor", tms_xor},
                                       {"nor", tms_nor},
                                       {"or", tms_or},
                                       {"ipv4", tms_ipv4},
                                       {"dotted", tms_dotted},
                                       {"mask_range", tms_mask_range}};

const int tms_g_int_extf_count = array_length(tms_g_int_extf);

int _tms_read_int_operand(char *expr, int start, int64_t *result)
{
    int64_t value;
    int status;
    bool is_negative = false;

    // Avoid negative offsets
    if (start < 0)
        return -1;

    // Catch incorrect start like )5 (no implied multiplication allowed)
    if (start > 0 && !tms_is_valid_int_number_start(expr[start - 1]))
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, expr, start - 1);
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
                tms_error_handler(EH_SAVE, TMS_INT_PARSER, PARENTHESIS_MISSING, EH_FATAL, expr, start + strlen(name));
            else
                tms_error_handler(EH_SAVE, TMS_INT_PARSER, UNDEFINED_VARIABLE, EH_FATAL, expr, start);
            free(name);
            return -1;
        }
        free(name);
        break;

    case -2:
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INTEGER_OVERFLOW, EH_FATAL, expr, start);
        return -1;

    case -3:
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INT_TOO_LARGE, EH_FATAL, expr, start);
        return -1;

    default:
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, expr, start);
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

int _tms_set_runtime_int_var(char *expr, int i)
{
    if (i == 0)
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, expr, 0);
        return -1;
    }
    else
    {
        int j;
        // Check if another assignment operator is used
        j = tms_f_search(expr, "=", i + 1, false);
        if (j != -1)
        {
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, MULTIPLE_ASSIGNMENT_ERROR, EH_FATAL, expr, j);
            return -1;
        }

        char name[i + 1];
        strncpy(name, expr, i);
        name[i] = '\0';
        return tms_new_int_var(name);
    }
}

tms_int_expr *_tms_init_int_expr(char *expr)
{
    char *local_expr = strchr(expr, '=');

    if (local_expr == NULL)
        local_expr = expr;
    else
        ++local_expr;

    if (local_expr[0] == '\0')
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, MISSING_EXPRESSION, EH_FATAL, NULL);
        free(expr);
        return NULL;
    }

    int s_max = 8, i, j, s_i, length = strlen(local_expr), s_count;

    // Pointer to subexpressions heap array
    tms_int_subexpr *S;

    tms_int_expr *M = malloc(sizeof(tms_int_expr));

    M->S = NULL;
    M->subexpr_count = 0;
    M->expr = expr;
    M->local_expr = local_expr;

    S = malloc(s_max * sizeof(tms_int_subexpr));

    int depth = 0;
    s_i = 0;
    // Determine the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        DYNAMIC_RESIZE(S, s_i, s_max, tms_int_subexpr)
        if (local_expr[i] == '(')
        {
            S[s_i].func.extended = NULL;
            S[s_i].func_type = TMS_NOFUNC;
            S[s_i].exec_extf = true;
            S[s_i].nodes = NULL;

            // Treat extended functions as a subexpression
            if (i > 1 && tms_legal_char_in_name(local_expr[i - 1]))
            {
                char *name = tms_get_name(local_expr, i - 1, false);

                // The function name is not valid
                if (name == NULL)
                {
                    tms_error_handler(EH_SAVE, TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, local_expr, i - 1);
                    free(S);
                    tms_delete_int_expr(M);
                    return NULL;
                }

                j = tms_find_str_in_array(name, tms_g_int_extf, tms_g_int_extf_count, TMS_F_INT_EXTENDED);

                // It is an extended function indeed
                if (j != -1)
                {
                    S[s_i].subexpr_start = i - strlen(name);
                    S[s_i].solve_start = i + 1;
                    i = tms_find_closing_parenthesis(local_expr, i);
                    if (i == -1)
                    {
                        tms_error_handler(EH_SAVE, TMS_INT_PARSER, PARENTHESIS_NOT_CLOSED, EH_FATAL, local_expr,
                                          S[s_i].solve_start - 1);
                        M->S = S;
                        free(name);
                        tms_delete_int_expr(M);
                        return NULL;
                    }
                    S[s_i].solve_end = i - 1;
                    S[s_i].depth = depth + 1;
                    S[s_i].func.extended = tms_g_int_extf[j].ptr;
                    S[s_i].func_type = TMS_F_INT_EXTENDED;
                    S[s_i].start_node = -1;
                    S[s_i].op_count = 0;

                    ++s_i;
                    free(name);
                    continue;
                }
                free(name);
            }

            // Normal case
            ++depth;
            S[s_i].solve_start = i + 1;
            S[s_i].depth = depth;

            // The expression start is the parenthesis, may change if a function is found
            S[s_i].subexpr_start = i;
            S[s_i].solve_end = tms_find_closing_parenthesis(local_expr, i) - 1;

            // Empty parenthesis pair is only allowed for extended functions
            if (S[s_i].solve_end == i)
            {
                tms_error_handler(EH_SAVE, TMS_INT_PARSER, PARENTHESIS_EMPTY, EH_FATAL, local_expr, i);
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }

            if (S[s_i].solve_end == -2)
            {
                tms_error_handler(EH_SAVE, TMS_INT_PARSER, PARENTHESIS_NOT_CLOSED, EH_FATAL, local_expr, i);
                // S isn't part of M yet
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }
            ++s_i;
        }
        else if (local_expr[i] == ')')
        {
            // An extra ')'
            if (depth == 0)
            {
                tms_error_handler(EH_SAVE, TMS_INT_PARSER, PARENTHESIS_NOT_OPEN, EH_FATAL, local_expr, i);
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }
            else
                --depth;

            // Make sure a ')' is followed by an operator, ')' or \0
            if (!(tms_is_int_op(local_expr[i + 1]) || local_expr[i + 1] == ')' || local_expr[i + 1] == '\0'))
            {
                tms_error_handler(EH_SAVE, TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, local_expr, i + 1);
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

int *_tms_get_int_op_indexes(char *local_expr, tms_int_subexpr *S, int s_i)
{
    // For simplicity
    int solve_start = S[s_i].solve_start;
    int solve_end = S[s_i].solve_end;
    int buffer_size = 16;
    int op_count = 0;

    if (solve_start > solve_end)
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, local_expr, solve_start);
        return NULL;
    }

    int *operator_index = (int *)malloc(buffer_size * sizeof(int));
    // Count number of operators and store it's indexes
    for (int i = solve_start; i <= solve_end; ++i)
    {
        // Skip an already processed expression
        if (local_expr[i] == '(')
        {
            int previous_subexp;
            previous_subexp = _tms_find_int_subexpr_starting_at(S, i + 1, s_i, 2);
            if (previous_subexp != -1)
                i = S[previous_subexp].solve_end + 1;
        }
        else if (tms_legal_char_in_name(local_expr[i]))
            continue;

        else if (tms_is_int_op(local_expr[i]))
        {
            // Varying the array size on demand
            DYNAMIC_RESIZE(operator_index, op_count, buffer_size, int)

            operator_index[op_count] = i;
            // Skip a + or - located just after an operator
            if (local_expr[i + 1] == '-' || local_expr[i + 1] == '+')
                ++i;
            ++op_count;
        }
        else
        {
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, local_expr, i);
            free(operator_index);
            return NULL;
        }
    }

    // Send the number of ops to the parser
    S[s_i].op_count = op_count;

    return operator_index;
}

int _tms_set_int_function_ptr(char *local_expr, tms_int_expr *M, int s_i)
{
    int i;
    tms_int_subexpr *S = &(M->S[s_i]);
    int solve_start = S->solve_start;

    // Search for any function preceding the expression to set the function pointer
    if (solve_start > 1)
    {
        char *name = tms_get_name(local_expr, solve_start - 2, false);
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
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, UNDEFINED_FUNCTION, EH_FATAL, local_expr, solve_start - 2);
            free(name);
            return -1;
        }
    }
    return 0;
}

int _tms_init_int_nodes(char *local_expr, tms_int_expr *M, int s_i, int *operator_index)
{
    tms_int_subexpr *S = M->S;
    int op_count = S[s_i].op_count;
    int i;
    tms_int_op_node *NB;
    int solve_start = S[s_i].solve_start;
    int solve_end = S[s_i].solve_end;

    if (op_count < 0)
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, local_expr, solve_start);
        return -1;
    }

    S[s_i].nodes = malloc((op_count == 0 ? 1 : op_count) * sizeof(tms_int_op_node));
    NB = S[s_i].nodes;

    // Case where at least one operator was found
    if (op_count > 0)
    {
        // Check if the expression is terminated with an operator
        if (operator_index[op_count - 1] == solve_end)
        {
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, RIGHT_OP_MISSING, EH_FATAL, local_expr,
                              operator_index[op_count - 1]);
            return -1;
        }

        // Set operator type and index for each op_node
        for (i = 0; i < op_count; ++i)
        {
            NB[i].node_index = i;
            NB[i].operator_index = operator_index[i];
            NB[i].operator= local_expr[operator_index[i]];
        }

        // Set each op_node's priority data
        _tms_set_priority_int(NB, op_count);
    }
    else
    {
        NB->operator= '\0';
        NB->operator_index = -1;
        NB->priority = -1;
    }
    return 0;
}

int _tms_set_all_int_operands(char *local_expr, tms_int_expr *M, int s_i)
{
    tms_int_subexpr *S = M->S;
    int op_count = S[s_i].op_count;
    int i, status;
    tms_int_op_node *NB = S[s_i].nodes;
    int solve_start = S[s_i].solve_start;

    if (op_count == 0)
    {
        // Read to the left operand
        if (_tms_set_int_operand(local_expr, M, NB, solve_start, s_i, 'l'))
            return -1;
        else
            return 0;
    }

    // Read the first number
    // Treat +x and -x as 0-x and 0+x
    if (NB[0].operator_index == S[s_i].solve_start)
    {
        if (NB[0].operator== '+' || NB[0].operator== '-')
            NB[0].left_operand = 0;
        else
        {
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, local_expr, NB[0].operator_index);
            return -1;
        }
    }
    else
    {
        status = _tms_set_int_operand(local_expr, M, NB, solve_start, s_i, 'l');
        if (status == -1)
            return -1;
    }
    // Intermediate nodes, read number to the appropriate op_node operand
    for (i = 0; i < op_count - 1; ++i)
    {
        // x*y+z : y is set in the node containing x (node i) as the right operand
        // same in case of x-y+z
        if (NB[i].priority >= NB[i + 1].priority)
        {
            status = _tms_set_int_operand(local_expr, M, NB + i, NB[i].operator_index + 1, s_i, 'r');
            if (status == -1)
                return -1;
        }

        // x+y*z : y is set in the node containing z (node i+1) as the left operand
        else
        {
            status = _tms_set_int_operand(local_expr, M, NB + i + 1, NB[i].operator_index + 1, s_i, 'l');
            if (status == -1)
                return -1;
        }
    }
    // Set the last operand as the right operand of the last node
    status = _tms_set_int_operand(local_expr, M, NB + op_count - 1, NB[op_count - 1].operator_index + 1, s_i, 'r');
    if (status == -1)
        return -1;
    return 0;
}

int _tms_set_int_operand(char *expr, tms_int_expr *M, tms_int_op_node *N, int op_start, int s_i, char operand)
{
    tms_int_subexpr *S = M->S;
    int64_t *operand_ptr;
    int status;

    switch (operand)
    {
    case 'r':
        operand_ptr = &(N->right_operand);
        break;
    case 'l':
        operand_ptr = &(N->left_operand);
        break;
    default:
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, expr, N->operator_index);
        return -1;
    }

    // Check if the operand is the result of a subexpression
    status = _tms_find_int_subexpr_starting_at(S, op_start, s_i, 1);

    if (status == -1)
    {
        status = _tms_read_int_operand(expr, op_start, operand_ptr);
        if (status != 0)
        {
            if (tms_error_handler(EH_ERROR_COUNT, TMS_INT_PARSER, EH_FATAL) == 0)
                tms_error_handler(EH_SAVE, TMS_INT_PARSER, SYNTAX_ERROR, EH_FATAL, expr, op_start);
            return -1;
        }
    }
    else
        *(S[status].result) = operand_ptr;
    return 0;
}

int _tms_set_int_evaluation_order(tms_int_subexpr *S)
{
    int op_count = S->op_count;
    int i, j;
    tms_int_op_node *NB = S->nodes;

    if (op_count == 0)
    {
        S->start_node = 0;
        S->nodes->next = NULL;
    }
    else
    {
        // Set the starting op_node by searching the first op_node with the highest priority
        for (i = 5; i > 0; --i)
        {
            for (j = 0; j < op_count; ++j)
            {
                if (NB[j].priority == i)
                {
                    S->start_node = j;
                    // Break from the main loop by setting i outside loop condition
                    i = -1;
                    break;
                }
            }
        }

        i = S->start_node;
        if (i < 0)
        {
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, NULL);
            return -1;
        }
        int target_priority = NB[i].priority;
        j = i + 1;
        while (target_priority > 0)
        {
            // Run through the nodes to find an op_node with the target priority
            while (j < op_count)
            {
                if (NB[j].priority == target_priority)
                {
                    NB[i].next = NB + j;
                    // The next run starts from the next node
                    i = j;
                }
                ++j;
            }
            --target_priority;
            j = 0;
        }
        // Set the next pointer of the last op_node to NULL
        NB[i].next = NULL;
    }
    return 0;
}

void _tms_set_int_result_pointers(tms_int_expr *M, int s_i)
{
    tms_int_subexpr *S = M->S;
    tms_int_op_node *tmp_node, *NB = S[s_i].nodes;

    int i, op_count = S[s_i].op_count;
    // Set result pointers for each op_node based on position and priority
    tmp_node = NB + S[s_i].start_node;

    int left_node, right_node, prev_index = -2, prev_left = -2, prev_right = -2;
    while (tmp_node->next != NULL)
    {
        i = tmp_node->node_index;
        // Find the previous and next nodes to compare
        left_node = i - 1, right_node = i + 1;

        // Find the nearest previous node with lower priority
        while (left_node != -1)
        {
            if (left_node == prev_index)
            {
                // Optimize the edge case of successive <= priority nodes like a+a+a+a+a+a+a+a by using previous results
                left_node = prev_left;
                break;
            }
            else if (tmp_node->priority <= NB[left_node].priority)
                --left_node;
            else
                break;
        }

        // Find the nearest node with lower or equal priority
        while (right_node < op_count)
        {
            if (right_node == prev_index)
            {
                // Optimization
                right_node = prev_right;
                break;
            }
            else if (tmp_node->priority < NB[right_node].priority)
                ++right_node;
            else
                break;
        }

        // Case of the first op_node or a op_node with no left candidate
        if (left_node == -1)
            tmp_node->result = &(NB[right_node].left_operand);
        // Case of no right candidate
        else if (right_node == op_count)
            tmp_node->result = &(NB[left_node].right_operand);
        // Case of an op_node between 2 nodes
        else if (left_node > -1 && right_node < op_count)
        {
            if (NB[left_node].priority >= NB[right_node].priority)
                tmp_node->result = &(NB[left_node].right_operand);
            else
                tmp_node->result = &(NB[right_node].left_operand);
        }
        // Case of the last op_node
        else if (i == op_count - 1)
            tmp_node->result = &(NB[left_node].right_operand);
        tmp_node = tmp_node->next;

        prev_index = i;
        prev_left = left_node;
        prev_right = right_node;
    }
    // Case of the last op_node in the traversal order, set result to be result of the subexpression
    S[s_i].result = &(tmp_node->result);
    // The last op_node in the last subexpression should point to math_struct answer
    if (s_i == M->subexpr_count - 1)
        tmp_node->result = &M->answer;
}

tms_int_expr *tms_parse_int_expr(char *expr)
{
    tms_lock_parser(TMS_INT_PARSER);

    if (tms_error_handler(EH_ERROR_COUNT, TMS_INT_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_error_handler(EH_CLEAR, TMS_INT_PARSER);
    }

    tms_int_expr *M = _tms_parse_int_expr_unsafe(expr);
    if (M == NULL)
        tms_error_handler(EH_PRINT, TMS_INT_PARSER);

    tms_unlock_parser(TMS_INT_PARSER);
    return M;
}

tms_int_expr *_tms_parse_int_expr_unsafe(char *expr)
{
    int i;
    // Number of subexpressions
    int s_count;
    // Current subexpression index
    int s_i;
    // Stores the index of runtime var that will receive a copy of the answer
    int variable_index = -1;
    // Local expression may be offset compared to the expression due to the assignment operator (if it exists).
    char *local_expr;

    // Check for empty input
    if (expr[0] == '\0')
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, NO_INPUT, EH_FATAL, NULL);
        return NULL;
    }

    if (strlen(expr) > __INT_MAX__)
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, EXPRESSION_TOO_LONG, EH_FATAL, NULL);
        return NULL;
    }

    // Duplicate the expression sent to the parser, it may be constant
    expr = strdup(expr);

    tms_remove_whitespace(expr);
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    _tms_combine_add_sub(expr);
    // Search for assignment operator, to enable user defined variables
    i = tms_f_search(expr, "=", 0, false);
    if (i != -1)
    {
        variable_index = _tms_set_runtime_int_var(expr, i);
        if (variable_index == -1)
        {
            free(expr);
            return NULL;
        }
        local_expr = expr + i + 1;
    }
    else
        local_expr = expr;

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
        int *operator_index = _tms_get_int_op_indexes(local_expr, S, s_i);

        if (operator_index == NULL)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_int_function_ptr(local_expr, M, s_i);
        if (status == -1)
        {
            tms_delete_int_expr(M);
            free(operator_index);
            return NULL;
        }

        status = _tms_init_int_nodes(local_expr, M, s_i, operator_index);
        free(operator_index);

        // Exiting due to error
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_all_int_operands(local_expr, M, s_i);
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_int_evaluation_order(S + s_i);
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }
        _tms_set_int_result_pointers(M, s_i);
    }

    // Detect assignment operator (local_expr offset from expr)
    if (local_expr != expr)
        M->runvar_i = variable_index;
    else
        M->runvar_i = -1;
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

int _tms_find_int_subexpr_starting_at(tms_int_subexpr *S, int start, int s_i, int mode)
{
    int i;
    // If a subexpression is an operand in another subexpression, it will have a depth higher by only 1
    // Skip deeper subexpression to reduce search time
    int target_depth = S[s_i].depth + 1;
    i = s_i - 1;

    switch (mode)
    {
    case 1:
        while (i >= 0)
        {
            if (S[i].depth > target_depth)
                break;

            if (S[i].subexpr_start == start)
                return i;
            else
                --i;
        }
        break;
    case 2:
        while (i >= 0)
        {
            if (S[i].depth > target_depth)
                break;

            if (S[i].solve_start == start)
                return i;
            else
                --i;
        }
    }
    return -1;
}
// Function that finds the subexpression that ends at 'end'
int _tms_find_int_subexpr_ending_at(tms_int_subexpr *S, int end, int s_i, int s_count)
{
    int i;
    i = s_i - 1;
    while (i < s_count)
    {
        if (S[i].solve_end == end)
            return i;
        else
            --i;
    }
    return -1;
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
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INTERNAL_ERROR, EH_FATAL, NULL);
    }

    return NULL;
}
