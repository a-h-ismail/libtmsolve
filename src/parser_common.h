/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "tms_math_strs.h"

#ifndef OVERRIDE_DEFAULTS
#define operand_type double complex
#define math_expr tms_math_expr
#define math_subexpr tms_math_subexpr
#define op_node tms_op_node
#define PARSER TMS_PARSER
#define F_EXTENDED TMS_F_EXTENDED
#define F_USER TMS_F_USER
#define init_math_expr _tms_init_math_expr
#define delete_math_expr tms_delete_math_expr
#define delete_math_expr_members tms_delete_math_expr_members
#define ufunc tms_ufunc
#define extf tms_extf
#define get_ufunc_by_name tms_get_ufunc_by_name
#define get_extf_by_name tms_get_extf_by_name
#define is_op tms_is_op
#define get_operand_value _tms_get_operand_value
#define set_priority _tms_set_priority
#define MAX_PRIORITY 3
#endif

static int _tms_set_operand(math_expr *M, op_node *N, int op_start, int s_i, char operand, bool enable_labels);

static int _tms_find_subexpr_starting_at(math_subexpr *S, int start, int s_i, int8_t mode);

static int _tms_set_evaluation_order(math_subexpr *S);

static void _tms_generate_labels_refs(math_expr *M);

static int compare_subexpr_depth(const void *a, const void *b)
{
    if (((math_subexpr *)a)->depth < ((math_subexpr *)b)->depth)
        return 1;
    else if ((*((math_subexpr *)a)).depth > (*((math_subexpr *)b)).depth)
        return -1;
    else
        return 0;
}

math_expr *init_math_expr(char *expr)
{
    int s_max = 8, i, s_i, length = strlen(expr), s_count;

    // Pointer to subexpressions heap array
    math_subexpr *S;

    // Pointer to the math_expr generated
    math_expr *M = malloc(sizeof(math_expr));

    M->labeled_operands_count = 0;
    M->all_labeled_ops = NULL;
    M->labels = NULL;
    M->S = NULL;
    M->subexpr_count = 0;
    M->expr = expr;

    S = malloc(s_max * sizeof(math_subexpr));

    int depth = 0;
    s_i = 0;
    bool is_extended_or_runtime;
    // Determine the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        if (expr[i] == '(')
        {
            DYNAMIC_RESIZE(S, s_i, s_max, math_subexpr)
            is_extended_or_runtime = false;
            S[s_i].nodes = NULL;
            S[s_i].depth = ++depth;

            // Treat extended functions as a subexpression
            if (i > 0 && tms_legal_char_in_name(expr[i - 1]))
            {
                char *name = tms_get_name(expr, i - 1, false);

                // This means the function name used is not valid
                if (name == NULL)
                {
                    tms_save_error(PARSER, SYNTAX_ERROR, EH_FATAL, expr, i - 1);
                    free(S);
                    delete_math_expr(M);
                    return NULL;
                }
                const ufunc *ufunc_i = get_ufunc_by_name(name);
                const extf *extf_i = get_extf_by_name(name);
                // Name collision, should not happen using the library functions
                if (extf_i != NULL && ufunc_i != NULL)
                {
                    tms_save_error(PARSER, INTERNAL_ERROR, EH_FATAL, expr, i - 1);
                    free(S);
                    delete_math_expr(M);
                    return NULL;
                }
                // Found either extf or user function
                if (extf_i != NULL || ufunc_i != NULL)
                {
                    // Set the part common for the extended and user defined functions
                    // Remember, "i" here is at the open parenthesis
                    is_extended_or_runtime = true;
                    S[s_i].subexpr_start = i - strlen(name);
                    S[s_i].solve_start = i + 1;
                    S[s_i].solve_end = tms_find_closing_parenthesis(expr, i) - 1;
                    if (S[s_i].solve_end != -2)
                    {
                        S[s_i].start_node = -1;
                        // Generate the argument list at parsing time instead of during evaluation for better performance
                        char *arguments = tms_strndup(expr + i + 1, S[s_i].solve_end - i);
                        S[s_i].f_args = tms_get_args(arguments);
                        free(arguments);
                        // Set "i" at the end of the subexpression to avoid iterating within the extended/user function
                        i = S[s_i].solve_end;

                        // Specific to extended functions
                        if (extf_i != NULL)
                        {
                            S[s_i].func.extended = extf_i->ptr;
                            S[s_i].func_type = F_EXTENDED;
                            S[s_i].exec_extf = true;
                        }
                        // Specific to user functions
                        else
                        {
                            // Duplicate the function name to avoid breaking an expression if this user function is removed
                            // Remember that the mallocd name is removed when a user function is removed
                            // So this provides independence from the original ufunc (at the expense of more mallocs)
                            S[s_i].func.user = strdup(ufunc_i->name);
                            S[s_i].func_type = F_USER;
                        }
                    }
                }
                free(name);
            }
            if (is_extended_or_runtime == false)
            {
                // Not an extended function, either no function at all or a single variable function
                S[s_i].func.extended = NULL;
                S[s_i].f_args = NULL;
                S[s_i].func_type = TMS_NOFUNC;
                S[s_i].exec_extf = false;
                S[s_i].solve_start = i + 1;
                S[s_i].op_count = 0;

                // The expression start is the parenthesis, may change if a function is found
                S[s_i].subexpr_start = i;
                S[s_i].solve_end = tms_find_closing_parenthesis(expr, i) - 1;

                // Empty parenthesis pair is only allowed for extended functions
                if (S[s_i].solve_end == i)
                {
                    tms_save_error(PARSER, PARENTHESIS_EMPTY, EH_FATAL, expr, i);
                    free(S);
                    delete_math_expr(M);
                    return NULL;
                }
            }
            // Common between the 3 possible cases
            if (S[s_i].solve_end == -2)
            {
                tms_save_error(PARSER, PARENTHESIS_NOT_CLOSED, EH_FATAL, expr, i);
                // S isn't part of M yet
                free(S);
                delete_math_expr(M);
                return NULL;
            }
            ++s_i;
        }
        else if (expr[i] == ')')
        {
            // An extra ')'
            if (depth == 0)
            {
                tms_save_error(PARSER, PARENTHESIS_NOT_OPEN, EH_FATAL, expr, i);
                free(S);
                delete_math_expr(M);
                return NULL;
            }
            else
                --depth;

            // Make sure a ')' is followed by an operator, ')' or \0
            if (!(is_op(expr[i + 1]) || expr[i + 1] == ')' || expr[i + 1] == '\0'))
            {
                tms_save_error(PARSER, SYNTAX_ERROR, EH_FATAL, expr, i + 1);
                free(S);
                delete_math_expr(M);
                return NULL;
            }
        }
    }
    // + 1 for the subexpression with depth 0
    s_count = s_i + 1;
    // Shrink the block to the required size
    S = realloc(S, s_count * sizeof(math_subexpr));

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
    S[s_i].f_args = NULL;

    // Sort by depth (high to low)
    qsort(S, s_count, sizeof(math_subexpr), compare_subexpr_depth);
    return M;
}

static int *_tms_get_operator_indexes(char *expr, math_subexpr *S, int s_i)
{
    // For simplicity
    int solve_start = S[s_i].solve_start;
    int solve_end = S[s_i].solve_end;
    int buffer_size = 16;
    int op_count = 0;

    if (solve_start > solve_end)
    {
        tms_save_error(PARSER, INTERNAL_ERROR, EH_FATAL, expr, solve_start);
        return NULL;
    }

    int *operator_index = (int *)malloc(buffer_size * sizeof(int));
    // Count number of operators and store it's indexes
    for (int i = solve_start; i <= solve_end; ++i)
    {
        // Skip an already processed expression
        if (expr[i] == '(')
        {
            int previous_subexp;
            previous_subexp = _tms_find_subexpr_starting_at(S, i + 1, s_i, 2);
            if (previous_subexp != -1)
                i = S[previous_subexp].solve_end + 1;
        }
        else if (tms_legal_char_in_name(expr[i]) || expr[i] == '.')
            continue;

        else if (is_op(expr[i]))
        {
            // Skip a + or - used in scientific notation (like 1e+5)
            if (i > 0 && (expr[i - 1] == 'e' || expr[i - 1] == 'E') && (expr[i] == '+' || expr[i] == '-'))
            {
                // Not every "e" followed by + is a scientific notation
                // Maybe a variable name that ends with an "e" is being added to something?
                int tmp = tms_name_bounds(expr, i - 1, false);

                // Not a valid name (either a number or something messed up, which will be caught later)
                if (tmp == -1)
                    continue;
            }
            // Varying the array size on demand
            DYNAMIC_RESIZE(operator_index, op_count, buffer_size, int)

            operator_index[op_count] = i;
            // Skip a + or - located just after an operator
            if (expr[i + 1] == '-' || expr[i + 1] == '+')
                ++i;
            ++op_count;
        }
        else
        {
            tms_save_error(PARSER, SYNTAX_ERROR, EH_FATAL, expr, i);
            free(operator_index);
            return NULL;
        }
    }

    // Send the number of ops to the parser
    S[s_i].op_count = op_count;

    return operator_index;
}

static int _tms_set_evaluation_order(math_subexpr *S)
{
    int op_count = S->op_count;
    int i, j;
    op_node *NB = S->nodes;

    if (op_count == 0)
    {
        S->start_node = 0;
        S->nodes->next = NULL;
    }
    else
    {
        // Set the starting op_node by searching the first op_node with the highest priority
        for (i = MAX_PRIORITY; i > 0; --i)
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
            tms_save_error(PARSER, INTERNAL_ERROR, EH_FATAL, NULL, 0);
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

static void _tms_set_result_pointers(math_expr *M, int s_i)
{
    math_subexpr *S = M->S;
    op_node *tmp_node, *NB = S[s_i].nodes;

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

static int _tms_set_all_operands(math_expr *M, int s_i, bool enable_labels)
{
    math_subexpr *S = M->S;
    char *expr = M->expr;
    int op_count = S[s_i].op_count;
    int i, status;
    op_node *NB = S[s_i].nodes;
    int solve_start = S[s_i].solve_start;

    if (op_count == 0)
    {
        // Read to the left operand
        if (_tms_set_operand(M, NB, solve_start, s_i, 'l', enable_labels))
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
            tms_save_error(PARSER, SYNTAX_ERROR, EH_FATAL, expr, NB[0].operator_index);
            return -1;
        }
    }
    else
    {
        status = _tms_set_operand(M, NB, solve_start, s_i, 'l', enable_labels);
        if (status == -1)
            return -1;
    }
    // Intermediate nodes, read number to the appropriate op_node operand
    for (i = 0; i < op_count - 1; ++i)
    {
        // x^y+z : y is set in the node containing x (node i) as the right operand
        // same in case of x-y+z
        if (NB[i].priority >= NB[i + 1].priority)
        {
            status = _tms_set_operand(M, NB + i, NB[i].operator_index + 1, s_i, 'r', enable_labels);
            if (status == -1)
                return -1;
        }

        // x+y^z : y is set in the node containing z (node i+1) as the left operand
        else
        {
            status = _tms_set_operand(M, NB + i + 1, NB[i].operator_index + 1, s_i, 'l', enable_labels);
            if (status == -1)
                return -1;
        }
    }
    // Set the last operand as the right operand of the last node
    status = _tms_set_operand(M, NB + op_count - 1, NB[op_count - 1].operator_index + 1, s_i, 'r', enable_labels);
    if (status == -1)
        return -1;
    return 0;
}

static int _tms_find_subexpr_starting_at(math_subexpr *S, int start, int s_i, int8_t mode)
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

static int _tms_set_labels(math_expr *M, int start, op_node *x_node, char rl)
{
    char *expr = M->expr;
    bool is_negative = false;

    char *name = tms_get_name(expr, start, true);
    if (name == NULL)
    {
        tms_save_error(PARSER, SYNTAX_ERROR, EH_FATAL, M->expr, start);
        return -1;
    }

    int id = tms_find_str_in_array(name, M->labels->arguments, M->labels->count, TMS_NOFUNC);
    free(name);
    if (id == -1)
        return -1;

    if (expr[start] == '+')
    {
        is_negative = false;
        ++start;
    }
    else if (expr[start] == '-')
    {
        is_negative = true;
        ++start;
    }

    if (rl == 'l')
    {
        x_node->labels |= LABEL_LEFT;
        SET_LEFT_ID(x_node->labels, id);
        if (is_negative)
            x_node->labels |= LABEL_LNEG;
    }
    else if (rl == 'r')
    {
        x_node->labels |= LABEL_RIGHT;
        SET_RIGHT_ID(x_node->labels, id);
        if (is_negative)
            x_node->labels |= LABEL_RNEG;
    }
    else
    {
        tms_save_error(PARSER, INTERNAL_ERROR, EH_FATAL, M->expr, start);
        return -1;
    }

    return 0;
}

static int _tms_set_operand(math_expr *M, op_node *N, int op_start, int s_i, char operand, bool enable_labels)
{
    math_subexpr *S = M->S;
    operand_type *operand_ptr;
    char *expr = M->expr;
    int tmp;

    switch (operand)
    {
    case 'r':
        operand_ptr = &(N->right_operand);
        break;
    case 'l':
        operand_ptr = &(N->left_operand);
        break;
    default:
        tms_save_error(PARSER, INTERNAL_ERROR, EH_FATAL, expr, N->operator_index);
        return -1;
    }

    // Check if the operand is the result of a subexpression
    tmp = _tms_find_subexpr_starting_at(S, op_start, s_i, 1);

    // The operand is a variable or a numeric value
    if (tmp == -1)
    {
        int status = get_operand_value(M, op_start, operand_ptr);

        if (status != 0)
        {
            // Set labels to the specified operand
            if (enable_labels == true)
            {
                tmp = _tms_set_labels(M, op_start, N, operand);
                if (tmp == -1)
                {
                    // If the value reader failed with no error reported, set the error to be a syntax error
                    if (tms_get_error_count(PARSER, EH_ALL_ERRORS) == 0)
                        tms_save_error(PARSER, SYNTAX_ERROR, EH_FATAL, expr, op_start);
                    return -1;
                }
                else
                    tms_clear_errors(PARSER);
            }
            else
            {
                // If the value reader failed with no error reported, set the error to be a syntax error
                if (tms_get_error_count(PARSER, EH_ALL_ERRORS) == 0)
                    tms_save_error(PARSER, SYNTAX_ERROR, EH_FATAL, expr, op_start);
                return -1;
            }
        }
    }
    else
        // It's official: we have a subexpression result as operand
        *(S[tmp].result) = operand_ptr;

    return 0;
}

static int _tms_init_nodes(math_expr *M, int s_i, int *operator_index, bool enable_labels)
{
    math_subexpr *S = M->S;
    int op_count = S[s_i].op_count;
    int i;
    op_node *NB;
    int solve_start = S[s_i].solve_start;
    int solve_end = S[s_i].solve_end;
    char *expr = M->expr;

    if (op_count < 0)
    {
        tms_save_error(PARSER, INTERNAL_ERROR, EH_FATAL, expr, solve_start);
        return -1;
    }

    // Allocate nodes
    S[s_i].nodes = malloc((op_count == 0 ? 1 : op_count) * sizeof(op_node));

    NB = S[s_i].nodes;
    // Case where at least one operator was found
    if (op_count > 0)
    {
        // Check if the expression is terminated with an operator
        if (operator_index[op_count - 1] == solve_end)
        {
            tms_save_error(PARSER, RIGHT_OP_MISSING, EH_FATAL, expr, operator_index[op_count - 1]);
            return -1;
        }

        for (i = 0; i < op_count; ++i)
        {
            NB[i].operator_index = operator_index[i];
            NB[i].operator= expr[operator_index[i]];
        }

        // Set each op_node's operator priority data
        set_priority(NB, op_count);

        for (i = 0; i < op_count; ++i)
        {
            NB[i].node_index = i;
            NB[i].labels = 0;
            NB[i].operator_index = operator_index[i];
            NB[i].operator= expr[operator_index[i]];
        }
        // Check if the expression is terminated with an operator
        if (operator_index[op_count - 1] == solve_end)
        {
            tms_save_error(PARSER, RIGHT_OP_MISSING, EH_FATAL, expr, operator_index[op_count - 1]);
            return -1;
        }
    }
    // No operands at all
    else
    {
        NB->operator= '\0';
        NB->labels = 0;
        NB->operator_index = -1;
        NB->priority = -1;
    }
    return 0;
}

math_expr *dup_mexpr(math_expr *M)
{
    if (M == NULL)
        return NULL;

    math_expr *NM = malloc(sizeof(math_expr));
    // Copy the math expression
    *NM = *M;
    NM->expr = strdup(M->expr);
    NM->labels = tms_dup_arg_list(M->labels);
    NM->S = malloc(NM->subexpr_count * sizeof(math_subexpr));
    // Copy subexpressions
    memcpy(NM->S, M->S, M->subexpr_count * sizeof(math_subexpr));

    int op_count, node_count;
    math_subexpr *S, *NS;

    // Allocate nodes and set the result pointers
    // Here NM is the new math expression and NS is the new subexpressions array
    for (int s = 0; s < NM->subexpr_count; ++s)
    {
        // New variables to keep lines from becoming too long
        NS = NM->S + s;
        S = M->S + s;
        *NS = *S;
        if (S->nodes != NULL)
        {
            op_count = S->op_count;
            node_count = (op_count > 0 ? op_count : 1);
            NS->nodes = malloc(node_count * sizeof(op_node));
            // Copy nodes
            memcpy(NS->nodes, S->nodes, node_count * sizeof(op_node));

            // Copying nodes will also copy result and next pointers which point to the original node members
            // Regenerate the pointers using parser functions
            _tms_set_evaluation_order(NS);
            _tms_set_result_pointers(NM, s);
        }
        else
        {
            // An extended/user function subexpr
            NS->result = malloc(sizeof(operand_type *));
            NS->f_args = tms_dup_arg_list(S->f_args);
            if (S->func_type == F_USER)
                NS->func.user = strdup(S->func.user);
        }
    }

    // Fix the subexpressions result pointers
    for (int s = 0; s < NM->subexpr_count; ++s)
    {
        S = M->S + s;
        void *result = S->result, *start, *end, *tmp;

        // Find which operand in the original expression received the answer of the current subexpression
        tmp = *(operand_type **)result;
        for (int i = 0; i < M->subexpr_count; ++i)
        {
            if (M->S[i].nodes == NULL)
                continue;

            op_count = M->S[i].op_count;
            node_count = (op_count > 0 ? op_count : 1);
            start = M->S[i].nodes;
            end = M->S[i].nodes + node_count;

            // The nodes are contiguous, so we can pinpoint the correct node by pointer comparisons
            if (tmp >= start && tmp <= end)
            {
                int n = (tmp - start) / sizeof(op_node);

                if (&(M->S[i].nodes[n].right_operand) == *(operand_type **)result)
                    *(NM->S[s].result) = &(NM->S[i].nodes[n].right_operand);
                else if (&(M->S[i].nodes[n].left_operand) == *(operand_type **)result)
                    *(NM->S[s].result) = &(NM->S[i].nodes[n].left_operand);

                break;
            }
        }
    }

    // If the nodes have labels, regenerate the labels pointers array
    if (NM->labeled_operands_count > 0)
        _tms_generate_labels_refs(NM);

    return NM;
}

static void _tms_generate_labels_refs(math_expr *M)
{
    int i = 0, s_i, buffer_size = 16;
    tms_labeled_operand *x_data = malloc(buffer_size * sizeof(tms_labeled_operand));
    math_subexpr *subexpr_ptr = M->S;
    op_node *i_node;

    for (s_i = 0; s_i < M->subexpr_count; ++s_i)
    {
        if (subexpr_ptr[s_i].nodes == NULL)
            continue;

        i_node = subexpr_ptr[s_i].nodes + subexpr_ptr[s_i].start_node;
        while (i_node != NULL)
        {
            if (i == buffer_size)
            {
                buffer_size *= 2;
                x_data = realloc(x_data, buffer_size * sizeof(tms_labeled_operand));
            }
            // Case of label left operand
            if (i_node->labels & LABEL_LEFT)
            {
                x_data[i].is_negative = i_node->labels & LABEL_LNEG;
                x_data[i].ptr = &(i_node->left_operand);
                x_data[i].id = GET_LEFT_ID(i_node->labels);
                i_node->left_operand = 0;
                ++i;
            }
            // Case of label right operand
            if (i_node->labels & LABEL_RIGHT)
            {
                x_data[i].is_negative = i_node->labels & LABEL_RNEG;
                x_data[i].ptr = &(i_node->right_operand);
                x_data[i].id = GET_RIGHT_ID(i_node->labels);
                i_node->right_operand = 0;
                ++i;
            }
            i_node = i_node->next;
        }
    }
    if (i != 0)
    {
        x_data = realloc(x_data, i * sizeof(tms_labeled_operand));
        M->labeled_operands_count = i;
        M->all_labeled_ops = x_data;
    }
    else
        free(x_data);
}

void delete_math_expr_members(math_expr *M)
{
    if (M == NULL)
        return;

    int i = 0;
    math_subexpr *S = M->S;
    for (i = 0; i < M->subexpr_count; ++i)
    {
        if (S[i].func_type == F_EXTENDED || S[i].func_type == F_USER)
        {
            free(S[i].result);
            tms_free_arg_list(S[i].f_args);
        }
        else
            free(S[i].nodes);
        // User function names are mallocd unlike other function types that use a function pointer
        if (S[i].func_type == F_USER)
            free(S[i].func.user);
    }
    free(S);
    free(M->all_labeled_ops);
    free(M->expr);
    tms_free_arg_list(M->labels);
}

void delete_math_expr(math_expr *M)
{
    delete_math_expr_members(M);
    free(M);
}
