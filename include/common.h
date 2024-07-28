#include "tms_math_strs.h"

#ifndef OVERRIDE_DEFAULTS
#define math_expr tms_math_expr
#define math_subexpr tms_math_subexpr
#define op_node tms_op_node
#define PARSER TMS_PARSER
#endif

int *_tms_get_operator_indexes(char *expr, math_subexpr *S, int s_i)
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

        else if (tms_is_op(expr[i]))
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

int _tms_set_evaluation_order(math_subexpr *S)
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
        for (i = 3; i > 0; --i)
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

void _tms_set_result_pointers(math_expr *M, int s_i)
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

int _tms_set_all_operands(char *expr, math_expr *M, int s_i, bool enable_unknowns)
{
    math_subexpr *S = M->S;
    int op_count = S[s_i].op_count;
    int i, status;
    op_node *NB = S[s_i].nodes;
    int solve_start = S[s_i].solve_start;

    if (op_count == 0)
    {
        // Read to the left operand
        if (_tms_set_operand(expr, M, NB, solve_start, s_i, 'l', enable_unknowns))
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
        status = _tms_set_operand(expr, M, NB, solve_start, s_i, 'l', enable_unknowns);
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
            status = _tms_set_operand(expr, M, NB + i, NB[i].operator_index + 1, s_i, 'r', enable_unknowns);
            if (status == -1)
                return -1;
        }

        // x+y^z : y is set in the node containing z (node i+1) as the left operand
        else
        {
            status = _tms_set_operand(expr, M, NB + i + 1, NB[i].operator_index + 1, s_i, 'l', enable_unknowns);
            if (status == -1)
                return -1;
        }
    }
    // Set the last operand as the right operand of the last node
    status =
        _tms_set_operand(expr, M, NB + op_count - 1, NB[op_count - 1].operator_index + 1, s_i, 'r', enable_unknowns);
    if (status == -1)
        return -1;
    return 0;
}

int _tms_find_subexpr_starting_at(math_subexpr *S, int start, int s_i, int8_t mode)
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
int _tms_find_subexpr_ending_at(math_subexpr *S, int end, int s_i, int s_count)
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
