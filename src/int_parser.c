/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "int_parser.h"
#include "bitwise.h"
#include "tms_math_strs.h"
#include "evaluator.h"
#include "function.h"
#include "internals.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_complex.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *tms_int_nfunc_name[] = {"not", NULL};
int (*tms_int_nfunc_ptr[])(int64_t, int64_t *) = {tms_not};

char *tms_int_extf_name[] = {"rr", "rl", "sr", "sra", "sl", "nand", "and", "xor", "nor", "or", NULL};
int (*tms_int_extf_ptr[])(tms_arg_list *, int64_t *) = {tms_rr, tms_rl, tms_sr, tms_sra, tms_sl, tms_nand, tms_and, tms_xor, tms_nor, tms_or};

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
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, expr, start - 1);
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
            if (tms_find_str_in_array(name, tms_int_nfunc_name, -1, TMS_NOFUNC) != -1 ||
                tms_find_str_in_array(name, tms_int_extf_name, -1, TMS_NOFUNC) != -1)
                tms_error_handler(EH_SAVE, PARENTHESIS_MISSING, EH_FATAL_ERROR, expr, start + strlen(name));
            else
                tms_error_handler(EH_SAVE, UNDEFINED_VARIABLE, EH_FATAL_ERROR, expr, start);
            free(name);
            return -1;
        }
        free(name);
        break;

    case -2:
        tms_error_handler(EH_SAVE, INTEGER_OVERFLOW, EH_FATAL_ERROR, expr, start);
        return -1;

    case -3:
        tms_error_handler(EH_SAVE, INT_TOO_LARGE, EH_FATAL_ERROR, expr, start);
        return -1;

    default:
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, expr, start);
        return -1;
    }

    if (is_negative)
        value = -value;

    *result = value;
    return 0;
}

int tms_compare_int_subexpr_depth(const void *a, const void *b)
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
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, expr, 0);
        return -1;
    }
    else
    {
        int j;
        // Check if another assignment operator is used
        j = tms_f_search(expr, "=", i + 1, false);
        if (j != -1)
        {
            tms_error_handler(EH_SAVE, MULTIPLE_ASSIGNMENT_ERROR, EH_FATAL_ERROR, expr, j);
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
        tms_error_handler(EH_SAVE, MISSING_EXPRESSION, EH_FATAL_ERROR, NULL);
        free(expr);
        return NULL;
    }

    int s_max = 8, i, j, s_index, length = strlen(local_expr), s_count;

    // Pointer to subexpressions heap array
    tms_int_subexpr *S;

    tms_int_expr *M = malloc(sizeof(tms_int_expr));

    M->subexpr_ptr = NULL;
    M->subexpr_count = 0;
    M->expr = expr;
    M->local_expr = local_expr;

    S = malloc(s_max * sizeof(tms_int_subexpr));

    int depth = 0;
    s_index = 0;
    // Determine the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        DYNAMIC_RESIZE(S, s_index, s_max, tms_int_subexpr)
        if (local_expr[i] == '(')
        {
            S[s_index].func.extended = NULL;
            S[s_index].func_type = TMS_NOFUNC;
            S[s_index].exec_extf = true;
            S[s_index].nodes = NULL;

            // Treat extended functions as a subexpression
            if (i > 1 && tms_legal_char_in_name(local_expr[i - 1]))
            {
                char *name = tms_get_name(local_expr, i - 1, false);

                // The function name is not valid
                if (name == NULL)
                {
                    tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, local_expr, i - 1);
                    free(S);
                    tms_delete_int_expr(M);
                    return NULL;
                }

                j = tms_find_str_in_array(name, tms_int_extf_name, array_length(tms_int_extf_ptr), TMS_F_INT_EXTENDED);

                // It is an extended function indeed
                if (j != -1)
                {
                    S[s_index].subexpr_start = i - strlen(name);
                    S[s_index].solve_start = i + 1;
                    i = tms_find_closing_parenthesis(local_expr, i);
                    S[s_index].solve_end = i - 1;
                    S[s_index].depth = depth + 1;
                    S[s_index].func.extended = tms_int_extf_ptr[j];
                    S[s_index].func_type = TMS_F_EXTENDED;
                    S[s_index].start_node = -1;
                    S[s_index].op_count = 0;

                    ++s_index;
                    free(name);
                    continue;
                }
                free(name);
            }

            // Normal case
            ++depth;
            S[s_index].solve_start = i + 1;
            S[s_index].depth = depth;

            // The expression start is the parenthesis, may change if a function is found
            S[s_index].subexpr_start = i;
            S[s_index].solve_end = tms_find_closing_parenthesis(local_expr, i) - 1;

            // Empty parenthesis pair is only allowed for extended functions
            if (S[s_index].solve_end == i)
            {
                tms_error_handler(EH_SAVE, PARENTHESIS_EMPTY, EH_FATAL_ERROR, local_expr, i);
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }

            if (S[s_index].solve_end == -2)
            {
                tms_error_handler(EH_SAVE, PARENTHESIS_NOT_CLOSED, EH_FATAL_ERROR, local_expr, i);
                // S isn't part of M yet
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }
            ++s_index;
        }
        else if (local_expr[i] == ')')
        {
            // An extra ')'
            if (depth == 0)
            {
                tms_error_handler(EH_SAVE, PARENTHESIS_NOT_OPEN, EH_FATAL_ERROR, local_expr, i);
                free(S);
                tms_delete_int_expr(M);
                return NULL;
            }
            else
                --depth;
        }
    }
    // + 1 for the subexpression with depth 0
    s_count = s_index + 1;
    // Shrink the block to the required size
    S = realloc(S, s_count * sizeof(tms_int_subexpr));

    // Copy the pointer to the structure
    M->subexpr_ptr = S;

    M->subexpr_count = s_count;

    // The whole expression's "subexpression"
    S[s_index].depth = 0;
    S[s_index].solve_start = S[s_index].subexpr_start = 0;
    S[s_index].solve_end = length - 1;
    S[s_index].func.extended = NULL;
    S[s_index].nodes = NULL;
    S[s_index].func_type = TMS_NOFUNC;
    S[s_index].exec_extf = true;

    // Sort by depth (high to low)
    qsort(S, s_count, sizeof(tms_int_subexpr), tms_compare_int_subexpr_depth);
    return M;
}

int *_tms_get_int_op_indexes(char *local_expr, tms_int_subexpr *S, int s_index)
{
    // For simplicity
    int solve_start = S[s_index].solve_start;
    int solve_end = S[s_index].solve_end;
    int buffer_size = 16;
    int op_count = 0;

    if (solve_start > solve_end)
    {
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, local_expr, solve_start);
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
            previous_subexp = tms_find_int_subexpr_starting_at(S, i + 1, s_index, 2);
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
            tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, local_expr, i);
            free(operator_index);
            return NULL;
        }
    }

    // Send the number of ops to the parser
    S[s_index].op_count = op_count;

    return operator_index;
}

bool _tms_set_int_function_ptr(char *local_expr, tms_int_expr *M, int s_index)
{
    int i;
    tms_int_subexpr *S = &(M->subexpr_ptr[s_index]);
    int solve_start = S->solve_start;

    // Search for any function preceding the expression to set the function pointer
    if (solve_start > 1)
    {
        char *name = tms_get_name(local_expr, solve_start - 2, false);
        if (name == NULL)
            return true;

        if ((i = tms_find_str_in_array(name, tms_int_nfunc_name, -1, TMS_F_INT64)) != -1)
        {
            S->func.simple = tms_int_nfunc_ptr[i];
            S->func_type = TMS_F_REAL;
            // Set the start of the subexpression to the start of the function name
            S->subexpr_start = solve_start - strlen(tms_int_nfunc_name[i]) - 1;
            return true;
        }
        else
        {
            tms_error_handler(EH_SAVE, UNDEFINED_FUNCTION, EH_NONFATAL_ERROR, local_expr, solve_start - 2);
            free(name);
            return false;
        }
    }
    return true;
}

int _tms_init_int_nodes(char *local_expr, tms_int_expr *M, int s_index, int *operator_index)
{
    tms_int_subexpr *S = M->subexpr_ptr;
    int s_count = M->subexpr_count, op_count = S[s_index].op_count;
    int i, status;
    tms_int_op_node *NB;
    int solve_start = S[s_index].solve_start;
    int solve_end = S[s_index].solve_end;

    if (op_count < 0)
    {
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, local_expr, solve_start);
        return -1;
    }

    // Allocate nodes
    if (op_count == 0)
        S[s_index].nodes = malloc(sizeof(tms_int_op_node));
    else
        S[s_index].nodes = malloc(op_count * sizeof(tms_int_op_node));

    NB = S[s_index].nodes;

    // Check if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        tms_error_handler(EH_SAVE, RIGHT_OP_MISSING, EH_FATAL_ERROR, local_expr, operator_index[op_count - 1]);
        return -1;
    }
    // Fill operations and index data into each op_node
    for (i = 0; i < op_count; ++i)
    {
        NB[i].operator_index = operator_index[i];
        NB[i].operator= local_expr[operator_index[i]];
    }

    // Case of expression with one term, use one op_node with operand1 to hold the number
    if (op_count == 0)
    {
        i = tms_find_int_subexpr_starting_at(S, S[s_index].solve_start, s_index, 1);
        S[s_index].nodes[0].node_index = 0;
        //  Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
        if (i != -1)
            *(S[i].result) = &(NB->left_operand);
        else
        {
            status = _tms_read_int_operand(local_expr, solve_start, &(NB->left_operand));
            if (status != 0)
            {
                // Operand reading didn't write an error itself
                if (tms_error_handler(EH_ERROR_COUNT, EH_FATAL_ERROR) == 0)
                    tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, local_expr, solve_start);
                return -1;
            }
        }

        S[s_index].start_node = 0;
        S[s_index].result = &(NB->result);
        NB[0].next = NULL;
        // If the one term expression is the last one, use the math_struct answer
        if (s_index == s_count - 1)
        {
            NB[0].result = &M->answer;
            return TMS_BREAK;
        }

        // Signal to the parser that processing this subexpression is done (because it has no operators)
        return TMS_CONTINUE;
    }
    // Case where at least one operator was found
    else
    {
        // Set each op_node's priority data
        tms_set_priority_int(NB, op_count);

        for (i = 0; i < op_count; ++i)
            NB[i].node_index = i;
    }
    // Check if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        tms_error_handler(EH_SAVE, RIGHT_OP_MISSING, EH_FATAL_ERROR, local_expr, operator_index[op_count - 1]);
        return -1;
    }
    // Set operator type and index for each op_node
    for (i = 0; i < op_count; ++i)
    {
        NB[i].operator_index = operator_index[i];
        NB[i].operator= local_expr[operator_index[i]];
    }
    return 0;
}

int _tms_set_all_int_operands(char *local_expr, tms_int_expr *M, int s_index)
{
    tms_int_subexpr *S = M->subexpr_ptr;
    int op_count = S[s_index].op_count;
    int i, status;
    tms_int_op_node *NB = S[s_index].nodes;
    int solve_start = S[s_index].solve_start;

    // Read the first number
    // Treat +x and -x as 0-x and 0+x
    if (NB[0].operator_index == S[s_index].solve_start)
    {
        if (NB[0].operator== '+' || NB[0].operator== '-')
            NB[0].left_operand = 0;
        else
        {
            tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, local_expr, NB[0].operator_index);
            return -1;
        }
    }
    else
    {
        status = _tms_set_int_operand(local_expr, M, NB, solve_start, s_index, 'l');
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
            status = _tms_set_int_operand(local_expr, M, NB + i, NB[i].operator_index + 1, s_index, 'r');
            if (status == -1)
                return -1;
        }

        // x+y*z : y is set in the node containing z (node i+1) as the left operand
        else
        {
            status = _tms_set_int_operand(local_expr, M, NB + i + 1, NB[i].operator_index + 1, s_index, 'l');
            if (status == -1)
                return -1;
        }
    }
    // Set the last operand as the right operand of the last node
    status = _tms_set_int_operand(local_expr, M, NB + op_count - 1, NB[op_count - 1].operator_index + 1, s_index, 'r');
    if (status == -1)
        return -1;
    return 0;
}

int _tms_set_int_operand(char *expr, tms_int_expr *M, tms_int_op_node *N, int op_start, int s_index, char operand)
{
    tms_int_subexpr *S = M->subexpr_ptr;
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
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, expr, N->operator_index);
        return -1;
    }

    status = _tms_read_int_operand(expr, op_start, operand_ptr);

    if (status != 0)
    {
        // Case of a subexpression result as operand
        status = tms_find_int_subexpr_starting_at(S, op_start, s_index, 1);
        if (status == -1)
        {
            if (tms_error_handler(EH_ERROR_COUNT, EH_FATAL_ERROR) == 0)
                tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, expr, op_start);
            return -1;
        }
        else
            *(S[status].result) = operand_ptr;
    }
    return 0;
}

bool _tms_set_int_evaluation_order(tms_int_subexpr *S)
{
    int op_count = S->op_count;
    int i, j;
    tms_int_op_node *NB = S->nodes;

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
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, NULL);
        return false;
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
    return true;
}

void _tms_set_int_result_pointers(tms_int_expr *M, int s_index)
{
    tms_int_subexpr *S = M->subexpr_ptr;
    tms_int_op_node *tmp_node, *NB = S[s_index].nodes;

    int i, op_count = S[s_index].op_count;
    // Set result pointers for each op_node based on position and priority
    tmp_node = NB + S[s_index].start_node;

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
    S[s_index].result = &(tmp_node->result);
    // The last op_node in the last subexpression should point to math_struct answer
    if (s_index == M->subexpr_count - 1)
        tmp_node->result = &M->answer;
}

tms_int_expr *tms_parse_int_expr(char *expr)
{
    int i;
    // Number of subexpressions
    int s_count;
    // Current subexpression index
    int s_index;
    // Stores the index of runtime var that will receive a copy of the answer
    int variable_index = -1;
    // Local expression may be offset compared to the expression due to the assignment operator (if it exists).
    char *local_expr;

    // Check for empty input
    if (expr[0] == '\0')
    {
        tms_error_handler(EH_SAVE, NO_INPUT, EH_FATAL_ERROR, NULL);
        return NULL;
    }

    // Duplicate the expression sent to the parser, it may be constant
    expr = strdup(expr);

    tms_remove_whitespace(expr);
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    tms_combine_add_sub(expr, 0, strlen(expr) - 2);
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

    tms_int_subexpr *S = M->subexpr_ptr;
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

    for (s_index = 0; s_index < s_count; ++s_index)
    {
        // Extended functions use a subexpression without nodes, but the subexpression result pointer should point at something
        // Allocate a small block and use that for the result pointer
        if (S[s_index].func_type == TMS_F_EXTENDED)
        {
            S[s_index].result = malloc(sizeof(double complex *));
            continue;
        }

        // Get an array of the index of all operators and set their count
        int *operator_index = _tms_get_int_op_indexes(local_expr, S, s_index);

        if (operator_index == NULL)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_int_function_ptr(local_expr, M, s_index);
        if (!status)
        {
            tms_delete_int_expr(M);
            free(operator_index);
            return NULL;
        }

        status = _tms_init_int_nodes(local_expr, M, s_index, operator_index);
        free(operator_index);

        // Exiting due to error
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }
        else if (status == TMS_CONTINUE)
            continue;
        else if (status == TMS_BREAK)
            break;

        status = _tms_set_all_int_operands(local_expr, M, s_index);
        if (status == -1)
        {
            tms_delete_int_expr(M);
            return NULL;
        }

        status = _tms_set_int_evaluation_order(S + s_index);
        if (status == false)
        {
            tms_delete_int_expr(M);
            return NULL;
        }
        _tms_set_int_result_pointers(M, s_index);
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
    tms_int_subexpr *S = M->subexpr_ptr;
    for (i = 0; i < M->subexpr_count; ++i)
    {
        if (S[i].func_type == 3)
            free(S[i].result);
        free(S[i].nodes);
    }
    free(S);
    free(M->expr);
    free(M);
}

void tms_set_priority_int(tms_int_op_node *list, int op_count)
{
    char operators[] = {'*', '/', '%', '+', '-', '&', '^', '|'};
    uint8_t priority[] = {5, 5, 4, 4, 4, 3, 2, 1};
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

int tms_find_int_subexpr_starting_at(tms_int_subexpr *S, int start, int s_index, int mode)
{
    int i;
    i = s_index - 1;
    switch (mode)
    {
    case 1:
        while (i >= 0)
        {
            if (S[i].subexpr_start == start)
                return i;
            else
                --i;
        }
        break;
    case 2:
        while (i >= 0)
        {
            if (S[i].solve_start == start)
                return i;
            else
                --i;
        }
    }
    return -1;
}
// Function that finds the subexpression that ends at 'end'
int tms_find_int_subexpr_ending_at(tms_int_subexpr *S, int end, int s_index, int s_count)
{
    int i;
    i = s_index - 1;
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
    case TMS_F_REAL:
        for (i = 0; i < array_length(tms_int_nfunc_ptr); ++i)
        {
            if (function == (void *)(tms_int_nfunc_ptr[i]))
                break;
        }
        if (i < array_length(tms_int_nfunc_ptr))
            return tms_int_nfunc_name[i];
        break;

    case TMS_F_EXTENDED:
        for (i = 0; i < array_length(tms_int_extf_name); ++i)
        {
            if (function == (void *)(tms_int_extf_ptr[i]))
                break;
        }
        if (i < array_length(tms_int_extf_name))
            return tms_int_extf_name[i];
        break;
    default:
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, NULL);
    }

    return NULL;
}
