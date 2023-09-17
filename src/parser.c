/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"
#include "parser.h"
#include "evaluator.h"
#include "tms_complex.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int tms_compare_subexpr_depth(const void *a, const void *b)
{
    if ((*((tms_math_subexpr *)a)).depth < (*((tms_math_subexpr *)b)).depth)
        return 1;
    else if ((*((tms_math_subexpr *)a)).depth > (*((tms_math_subexpr *)b)).depth)
        return -1;
    else
        return 0;
}

// Real domain functions
char *tms_r_func_name[] =
    {"fact", "abs", "ceil", "floor", "round", "sign", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log", NULL};
double (*tms_r_func_ptr[])(double) =
    {tms_fact, fabs, ceil, floor, round, tms_sign, sqrt, cbrt, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, tms_cos, tm_sin, tms_tan, log, log10};

// Extended functions, may take more than one argument (stored in a comma separated string)
char *tms_ext_func_name[] =
    {"integrate", "der", "logn", "hex", "oct", "bin", "rand", "randint", "int", NULL};
double complex (*tms_ext_func[])(tms_arg_list *) =
    {tms_integrate, tms_derivative, tms_logn, tms_hex, tms_oct, tms_bin, tms_rand, tms_randint, tms_int};

// Complex functions
char *tms_cmplx_func_name[] =
    {"fact", "abs", "arg", "ceil", "floor", "round", "sign", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log", NULL};
double complex (*tms_cmplx_func_ptr[])(double complex) =
    {tms_cfact, cabs_z, carg_z, tms_cceil, tms_cfloor, tms_cround, tms_csign, csqrt, tms_ccbrt, cacosh, casinh, catanh, cacos, casin, catan, ccosh, csinh, ctanh, tms_ccos, tms_csin, tms_ctan, tms_cln, tms_clog};

int _tms_set_runtime_var(char *expr, int i)
{
    if (i == 0)
    {
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, 0);
        return -1;
    }
    else
    {
        int j;
        // Check if another assignment operator is used
        j = tms_f_search(expr, "=", i + 1, false);
        _tms_g_expr = expr;
        if (j != -1)
        {
            tms_error_handler(EH_SAVE, MULTIPLE_ASSIGNMENT_ERROR, EH_FATAL_ERROR, j);
            return -1;
        }

        char name[i + 1];
        strncpy(name, expr, i);
        name[i] = '\0';
        return tms_new_var(name, false);
    }
}

tms_math_expr *_tms_init_math_expr(char *local_expr, bool enable_complex)
{
    int dyn_size = 8, i, j, s_index, length = strlen(local_expr), s_count;

    // Pointer to subexpressions heap array
    tms_math_subexpr *S;

    // Pointer to the math_expr generated
    tms_math_expr *M = malloc(sizeof(tms_math_expr));

    M->unknown_count = 0;
    M->x_data = NULL;
    M->enable_complex = enable_complex;
    M->subexpr_ptr = NULL;
    M->subexpr_count = 0;

    S = malloc(dyn_size * sizeof(tms_math_subexpr));

    int depth = 0;
    s_index = 0;
    // Determine the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        // Resize the subexpr array on the fly
        if (s_index == dyn_size)
        {
            dyn_size *= 2;
            S = realloc(S, dyn_size * sizeof(tms_math_subexpr));
        }
        if (local_expr[i] == '(')
        {
            S[s_index].func.extended = NULL;
            S[s_index].func_type = 0;
            S[s_index].exec_extf = true;
            S[s_index].nodes = NULL;

            // Treat extended functions as a subexpression
            for (j = 0; j < sizeof(tms_ext_func) / sizeof(*tms_ext_func); ++j)
            {
                int tmp;
                tmp = tms_r_search(local_expr, tms_ext_func_name[j], i - 1, true);

                // Found an extended function
                if (tmp != -1)
                {
                    S[s_index].subexpr_start = tmp;
                    S[s_index].solve_start = i + 1;
                    i = tms_find_closing_parenthesis(local_expr, i);
                    S[s_index].solve_end = i - 1;
                    S[s_index].depth = depth + 1;
                    S[s_index].func.extended = tms_ext_func[j];
                    S[s_index].func_type = 3;
                    break;
                }
            }
            if (S[s_index].func.extended != NULL)
            {
                ++s_index;
                continue;
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
                tms_error_handler(EH_SAVE, PARENTHESIS_EMPTY, EH_FATAL_ERROR, i);
                free(S);
                tms_delete_math_expr(M);
                return NULL;
            }

            if (S[s_index].solve_end == -2)
            {
                tms_error_handler(EH_SAVE, PARENTHESIS_NOT_CLOSED, EH_FATAL_ERROR, i);
                // S isn't part of M yet
                free(S);
                tms_delete_math_expr(M);
                return NULL;
            }
            ++s_index;
        }
        else if (local_expr[i] == ')')
            --depth;
    }
    // + 1 for the subexpression with depth 0
    s_count = s_index + 1;
    // Shrink the block to the required size
    S = realloc(S, s_count * sizeof(tms_math_subexpr));

    // Copy the pointer to the structure
    M->subexpr_ptr = S;

    M->subexpr_count = s_count;

    // The whole expression's "subexpression"
    S[s_index].depth = 0;
    S[s_index].solve_start = S[s_index].subexpr_start = 0;
    S[s_index].solve_end = length - 1;
    S[s_index].func.extended = NULL;
    S[s_index].nodes = NULL;
    S[s_index].func_type = 0;
    S[s_index].exec_extf = true;

    // Sort by depth (high to low)
    qsort(S, s_count, sizeof(tms_math_subexpr), tms_compare_subexpr_depth);
    return M;
}

int *_tms_get_operator_indexes(char *local_expr, tms_math_subexpr *S, int s_index)
{
    // For simplicity
    int solve_start = S[s_index].solve_start;
    int solve_end = S[s_index].solve_end;
    int buffer_size = 16;
    int op_count = 0;

    if (solve_start > solve_end)
    {
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, solve_start);
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
            previous_subexp = tms_find_subexpr_starting_at(S, i + 1, s_index, 2);
            if (previous_subexp != -1)
                i = S[previous_subexp].solve_end + 1;
        }
        else if (isdigit(local_expr[i]))
            continue;

        else if (tms_is_op(local_expr[i]))
        {
            // Skip a + or - used in scientific notation (like 1e+5)
            if (i > 0 && (local_expr[i - 1] == 'e' || local_expr[i - 1] == 'E') && (local_expr[i] == '+' || local_expr[i] == '-'))
            {
                ++i;
                continue;
            }
            // Varying the array size on demand
            if (op_count == buffer_size)
            {
                buffer_size *= 2;
                operator_index = realloc(operator_index, buffer_size * sizeof(int));
            }
            operator_index[op_count] = i;
            // Skip a + or - located just after an operator
            if (local_expr[i + 1] == '-' || local_expr[i + 1] == '+')
                ++i;
            ++op_count;
        }
    }

    // Send the number of ops to the parser
    S[s_index].op_count = op_count;

    return operator_index;
}

bool _tms_set_function_ptr(char *local_expr, tms_math_expr *M, int s_index)
{
    int i, j;
    tms_math_subexpr *S = &(M->subexpr_ptr[s_index]);
    int solve_start = S->solve_start;

    // Search for any function preceding the expression to set the function pointer
    if (solve_start > 1 && tms_legal_char_in_name(local_expr[solve_start - 2]))
    {
        if (!M->enable_complex)
        {
            for (i = 0; i < array_length(tms_r_func_ptr); ++i)
            {
                j = tms_r_search(local_expr, tms_r_func_name[i], solve_start - 2, true);
                if (j != -1)
                {
                    S->func.real = tms_r_func_ptr[i];
                    S->func_type = 1;
                    // Setting the start of the subexpression to the start of the function name
                    S->subexpr_start = j;
                    break;
                }
            }
            // The function isn't defined for real operations (loop exited due to condition instead of break)
            if (i == array_length(tms_r_func_ptr))
            {
                tms_error_handler(EH_SAVE, UNDEFINED_FUNCTION, EH_NONFATAL_ERROR, solve_start - 2);
                return false;
            }
        }
        else
        {
            for (i = 0; i < array_length(tms_cmplx_func_ptr); ++i)
            {
                j = tms_r_search(local_expr, tms_cmplx_func_name[i], solve_start - 2, true);
                if (j != -1)
                {
                    S->func.cmplx = tms_cmplx_func_ptr[i];
                    S->func_type = 2;
                    S->subexpr_start = j;
                    break;
                }
            }
            // The complex function is not defined
            if (i == array_length(tms_cmplx_func_ptr))
            {
                tms_error_handler(EH_SAVE, UNDEFINED_FUNCTION, EH_NONFATAL_ERROR, solve_start - 2);
                return false;
            }
        }
    }
    return true;
}

int _tms_init_nodes(char *local_expr, tms_math_expr *M, int s_index, int *operator_index, bool enable_unknowns)
{
    tms_math_subexpr *S = M->subexpr_ptr;
    int s_count = M->subexpr_count, op_count = S[s_index].op_count;
    int i, status;
    tms_op_node *NB;
    int solve_start = S[s_index].solve_start;
    int solve_end = S[s_index].solve_end;

    if (op_count < 0)
    {
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, solve_start);
        return -1;
    }

    // Allocate nodes
    if (op_count == 0)
        S[s_index].nodes = malloc(sizeof(tms_op_node));
    else
        S[s_index].nodes = malloc(op_count * sizeof(tms_op_node));

    NB = S[s_index].nodes;

    // Check if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        tms_error_handler(EH_SAVE, RIGHT_OP_MISSING, EH_FATAL_ERROR, operator_index[op_count - 1]);
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
        i = tms_find_subexpr_starting_at(S, S[s_index].solve_start, s_index, 1);
        S[s_index].nodes[0].unknowns_data = 0;
        S[s_index].nodes[0].node_index = 0;
        //  Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
        if (i != -1)
            *(S[i].result) = &(NB->left_operand);
        else
        {
            NB->left_operand = _tms_set_operand_value(local_expr, solve_start, M->enable_complex);
            if (isnan((double)NB->left_operand))
            {
                if (tms_error_handler(EH_ERROR_COUNT, EH_FATAL_ERROR) != 0)
                    return -1;
                if (enable_unknowns)
                    status = tms_set_unknowns_data(local_expr + solve_start, NB, 'l');
                else
                    status = -1;

                if (status == -1)
                {
                    tms_error_handler(EH_SAVE, UNDEFINED_VARIABLE, EH_FATAL_ERROR, solve_start);
                    return -1;
                }
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
        tms_set_priority(NB, op_count);

        for (i = 0; i < op_count; ++i)
        {
            NB[i].node_index = i;
            NB[i].unknowns_data = 0;
        }
    }
    // Check if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        tms_error_handler(EH_SAVE, RIGHT_OP_MISSING, EH_FATAL_ERROR, operator_index[op_count - 1]);
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

int _tms_set_all_operands(char *local_expr, tms_math_expr *M, int s_index, bool enable_unknowns)
{
    tms_math_subexpr *S = M->subexpr_ptr;
    int op_count = S[s_index].op_count;
    int i, status;
    tms_op_node *NB = S[s_index].nodes;
    int solve_start = S[s_index].solve_start;

    // Read the first number
    // Treat +x and -x as 0-x and 0+x
    if (NB[0].operator_index == S[s_index].solve_start)
    {
        if (NB[0].operator== '+' || NB[0].operator== '-')
            NB[0].left_operand = 0;
        else
        {
            tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, NB[0].operator_index);
            return -1;
        }
    }
    else
    {
        status = _tms_set_operand(local_expr, M, NB, solve_start, s_index, 'l', enable_unknowns);
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
            status = _tms_set_operand(local_expr, M, NB + i, NB[i].operator_index + 1, s_index, 'r', enable_unknowns);
            if (status == -1)
                return -1;
        }

        // x+y^z : y is set in the node containing z (node i+1) as the left operand
        else
        {
            status = _tms_set_operand(local_expr, M, NB + i + 1, NB[i].operator_index + 1, s_index, 'l', enable_unknowns);
            if (status == -1)
                return -1;
        }
    }
    // Set the last operand as the right operand of the last node
    status = _tms_set_operand(local_expr, M, NB + op_count - 1, NB[op_count - 1].operator_index + 1, s_index, 'r', enable_unknowns);
    if (status == -1)
        return -1;
    return 0;
}

int _tms_set_operand(char *expr, tms_math_expr *M, tms_op_node *N, int op_start, int s_index, char operand, bool enable_unknowns)
{
    tms_math_subexpr *S = M->subexpr_ptr;
    double complex *operand_ptr;
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
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, N->operator_index);
        return -1;
    }

    *operand_ptr = _tms_set_operand_value(expr, op_start, M->enable_complex);
    // Case of reading the number to the right operand of a op_node
    if (isnan((double)*operand_ptr))
    {
        if (tms_error_handler(EH_ERROR_COUNT, EH_FATAL_ERROR) != 0)
            return -1;
        // Checking for the unknown 'x'
        if (enable_unknowns == true)
            status = tms_set_unknowns_data(expr + op_start, N, operand);
        else
            status = -1;
        if (status == -1)
        {
            // Case of a subexpression result as right_operand, set its result pointer to right_operand
            status = tms_find_subexpr_starting_at(S, op_start, s_index, 1);
            if (status == -1)
            {
                tms_error_handler(EH_SAVE, UNDEFINED_VARIABLE, EH_FATAL_ERROR, op_start);
                return -1;
            }
            else
                *(S[status].result) = operand_ptr;
        }
    }
    return 0;
}

bool _tms_set_evaluation_order(tms_math_subexpr *S)
{
    int op_count = S->op_count;
    int i, j;
    tms_op_node *NB = S->nodes;

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
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, 0);
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

void _tms_set_result_pointers(tms_math_expr *M, int s_index)
{
    tms_math_subexpr *S = M->subexpr_ptr;
    tms_op_node *tmp_node, *NB = S[s_index].nodes;

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

tms_math_expr *tms_parse_expr(char *expr, bool enable_unknowns, bool enable_complex)
{
    int i;
    // Number of subexpressions
    int s_count;
    // Used for indexing of subexpressions
    int s_index;
    // Used to store the index of the variable to assign the answer to.
    int variable_index = -1;
    // Local expression may be offset compared to the expression due to the assignment operator (if it exists).
    char *local_expr;

    // Search for assignment operator, to enable user defined variables
    i = tms_f_search(expr, "=", 0, false);
    if (i != -1)
    {
        variable_index = _tms_set_runtime_var(expr, i);
        if (variable_index == -1)
            return NULL;
        local_expr = expr + i + 1;
    }
    else
        local_expr = expr;

    _tms_g_expr = local_expr;

    tms_math_expr *M = _tms_init_math_expr(local_expr, enable_complex);
    if (M == NULL)
        return NULL;

    tms_math_subexpr *S = M->subexpr_ptr;
    s_count = M->subexpr_count;

    int status;

    /*
    How to parse a subexpression:
    - Handle the special case of extended functions (currently: int, der)
    - Create an array that determines the location of operators in the string
    - Check if the subexpression has a function to execute on the result, set pointer
    - Allocate the array of nodes
    - Use the operator index array to fill the nodes data on operators type and location
    - Fill nodes operator priority
    - Fill nodes with values or set as unknown (using x)
    - Set nodes order of calculation (using *next)
    - Set the result pointer of each op_node relying on its position and neighbor priorities
    - Set the subexpr result double pointer to the result pointer of the last calculated op_node
    */

    for (s_index = 0; s_index < s_count; ++s_index)
    {
        // Special case of extended functions
        if (S[s_index].func_type == 3)
        {
            S[s_index].result = malloc(sizeof(double complex *));
            continue;
        }

        // Get an array of the index of all operators and set their count
        int *operator_index = _tms_get_operator_indexes(local_expr, S, s_index);

        status = _tms_set_function_ptr(local_expr, M, s_index);
        if (!status)
        {
            tms_delete_math_expr(M);
            free(operator_index);
            return NULL;
        }

        status = _tms_init_nodes(local_expr, M, s_index, operator_index, enable_unknowns);
        free(operator_index);

        // Exiting due to error
        if (status == -1)
        {
            tms_delete_math_expr(M);
            return NULL;
        }
        else if (status == TMS_CONTINUE)
            continue;
        else if (status == TMS_BREAK)
            break;

        status = _tms_set_all_operands(local_expr, M, s_index, enable_unknowns);
        if (status == -1)
        {
            tms_delete_math_expr(M);
            return NULL;
        }

        status = _tms_set_evaluation_order(S + s_index);
        if (status == false)
        {
            tms_delete_math_expr(M);
            return NULL;
        }
        _tms_set_result_pointers(M, s_index);
    }

    // Set unknowns metadata
    if (enable_unknowns)
        _tms_set_unknowns_data(M);

    // Detect assignment operator (local_expr offset from expr)
    if (local_expr != expr)
        M->runvar_i = variable_index;
    else
        M->runvar_i = -1;
    return M;
}

char *_tms_lookup_function_name(void *function, int func_type)
{
    int i;
    switch (func_type)
    {
    case 1:
        for (i = 0; i < array_length(tms_r_func_ptr); ++i)
        {
            if (function == (void *)(tms_r_func_ptr[i]))
                break;
        }
        if (i < array_length(tms_r_func_ptr))
            return tms_r_func_name[i];
        break;

    case 2:
        for (i = 0; i < array_length(tms_cmplx_func_ptr); ++i)
        {
            if (function == (void *)(tms_cmplx_func_ptr[i]))
                break;
        }
        if (i < array_length(tms_cmplx_func_ptr))
            return tms_cmplx_func_name[i];
        break;

    case 3:
        for (i = 0; i < array_length(tms_ext_func); ++i)
        {
            if (function == (void *)(tms_ext_func[i]))
                break;
        }
        if (i < array_length(tms_ext_func))
            return tms_ext_func_name[i];
        break;
    default:
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, -1);
    }

    return NULL;
}

void *_tms_lookup_function_pointer(char *function_name, bool is_complex)
{
    int i;
    if (is_complex)
    {
        for (i = 0; i < array_length(tms_cmplx_func_name); ++i)
        {
            if (strcmp(tms_cmplx_func_name[i], function_name) == 0)
                break;
        }
        if (i < array_length(tms_cmplx_func_name))
            return tms_cmplx_func_ptr[i];
    }
    else
    {
        for (i = 0; i < array_length(tms_r_func_name); ++i)
        {
            if (strcmp(tms_r_func_name[i], function_name) == 0)
                break;
        }
        if (i < array_length(tms_r_func_name))
            return tms_r_func_ptr[i];
    }

    return NULL;
}

void tms_convert_real_to_complex(tms_math_expr *M)
{
    // You need to swap real functions for their complex counterparts.
    tms_math_subexpr *S = M->subexpr_ptr;
    if (S != NULL)
    {
        int s_index;
        char *function_name;
        for (s_index = 0; s_index < M->subexpr_count; ++s_index)
        {
            if (S[s_index].func_type != 1)
                continue;
            // Lookup the name of the real function and find the equivalent function pointer
            function_name = _tms_lookup_function_name(S[s_index].func.real, 1);
            if (function_name == NULL)
                return;
            S[s_index].func.cmplx = _tms_lookup_function_pointer(function_name, true);
            S[s_index].func_type = 2;
        }
    }
    M->enable_complex = true;
}

int tms_set_unknowns_data(char *expr, tms_op_node *x_node, char operand)
{
    bool is_negative = false, is_x = false;
    if (tms_match_word(expr, 0, "x", true))
        is_x = true;
    else if (tms_match_word(expr, 1, "x", true))
    {
        if (*expr == '+')
            is_x = true;
        else if (*expr == '-')
            is_x = is_negative = true;
        else
            return -1;
    }
    // The value is not the unknown x
    else
        return -1;
    if (is_x)
    {
        // x as left operand, set bits using bitwise OR
        if (operand == 'l')
        {
            x_node->unknowns_data |= UNK_LEFT;
            if (is_negative)
                x_node->unknowns_data |= UNK_LNEG;
        }
        // x as right op
        else if (operand == 'r')
        {
            x_node->unknowns_data |= UNK_RIGHT;
            if (is_negative)
                x_node->unknowns_data |= UNK_RNEG;
        }
        else
        {
            tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, -1);
            return -1;
        }
    }
    return 0;
}

void tms_delete_math_expr(tms_math_expr *M)
{
    if (M == NULL)
        return;

    int i = 0;
    tms_math_subexpr *S = M->subexpr_ptr;
    for (i = 0; i < M->subexpr_count; ++i)
    {
        if (S[i].func_type == 3)
            free(S[i].result);
        free(S[i].nodes);
    }
    free(S);
    free(M->x_data);
    free(M);
}

void tms_set_priority(tms_op_node *list, int op_count)
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

int tms_find_subexpr_starting_at(tms_math_subexpr *S, int start, int s_index, int mode)
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
int tms_find_subexpr_ending_at(tms_math_subexpr *S, int end, int s_index, int s_count)
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