/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"
#include "parser.h"

int tms_compare_subexpr_depth(const void *a, const void *b)
{
    if ((*((tms_math_subexpr *)a)).depth < (*((tms_math_subexpr *)b)).depth)
        return 1;
    else if ((*((tms_math_subexpr *)a)).depth > (*((tms_math_subexpr *)b)).depth)
        return -1;
    else
        return 0;
}
// Some simple wrapper functions.
double complex cabs_z(double complex z)
{
    return cabs(z);
}
double complex carg_z(double complex z)
{
    return carg(z);
}
double complex tms_ccbrt(double complex z)
{
    return cpow(z, 1.0 / 3);
}

// Wrapper functions for cos, sin and tan to round for very small values

double rd_cos(double __x)
{
    __x = cos(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

double rd_sin(double __x)
{
    __x = sin(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

double rd_tan(double __x)
{
    __x = sin(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

char *tms_r_func_name[] =
    {"fact", "abs", "ceil", "floor", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log", NULL};
double (*tms_r_func_ptr[])(double) =
    {tms_factorial, fabs, ceil, floor, sqrt, cbrt, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, rd_cos, rd_sin, rd_tan, log, log10};
// Extended functions, may take more than one parameter (stored in a comma separated string)
char *tms_ext_func_name[] = {"int", "der", NULL};
double (*tms_ext_func[])(char *) =
    {tms_integrate, tms_derivative};

// Complex functions
char *tms_cmplx_func_name[] =
    {"abs", "arg", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "log", NULL};
double complex (*tms_cmplx_func_ptr[])(double complex) =
    {cabs_z, carg_z, csqrt, tms_ccbrt, cacosh, casinh, catanh, cacos, casin, catan, ccosh, csinh, ctanh, ccos, csin, ctan, clog};

int _tms_set_runtime_var(char *expr, int i)
{
    int j;
    // Used to store the index of the variable to assign the answer to.
    int variable_index = -1;

    // Search for assignment operator, to enable user defined variables

    if (i == 0)
    {
        tms_error_handler(SYNTAX_ERROR, EH_SAVE, EH_FATAL_ERROR, 0);
        return -1;
    }
    else
    {
        // Check if another assignment operator is used
        j = tms_f_search(expr, "=", i + 1, false);
        _tms_g_expr = expr;
        if (j != -1)
        {
            tms_error_handler(MULTIPLE_ASSIGNMENT_ERROR, EH_SAVE, EH_FATAL_ERROR, j);
            return -1;
        }
        // Store the variable name in this array
        char tmp[i + 1];
        strncpy(tmp, expr, i);
        tmp[i] = '\0';

        // Check if the name is allowed
        for (i = 0; i < tms_g_illegal_names_count; ++i)
        {
            if (strcmp(tmp, tms_g_illegal_names[i]) == 0)
            {
                tms_error_handler(ILLEGAL_VARIABLE_NAME, EH_SAVE, EH_FATAL_ERROR, 0);
                return -1;
            }
        }

        // Check if the name has illegal characters
        if (tms_valid_name(tmp) == false)
        {
            tms_error_handler(INVALID_VARIABLE_NAME, EH_SAVE, EH_FATAL_ERROR, 0);
            return -1;
        }
        for (j = 0; j < tms_g_var_count; ++j)
        {
            // An existing variable is found
            if (strcmp(tms_g_var_names[j], tmp) == 0)
            {
                variable_index = j;
                break;
            }
        }
        if (j < tms_builtin_var_count)
        {
            tms_error_handler(OVERWRITE_BUILTIN_VARIABLE, EH_SAVE, EH_FATAL_ERROR, i);
            return -1;
        }
        // Create a new variable
        if (j == tms_g_var_count)
        {
            // Dynamically expand size
            if (tms_g_var_count == tms_g_var_max)
            {
                tms_g_var_max *= 2;
                tms_g_var_names = realloc(tms_g_var_names, tms_g_var_max * sizeof(char *));
                tms_g_var_values = realloc(tms_g_var_values, tms_g_var_max * sizeof(double complex));
            }
            tms_g_var_names[tms_g_var_count] = malloc((strlen(tmp) + 1) * sizeof(char));
            // Initialize the new variable to zero
            tms_g_var_values[tms_g_var_count] = 0;
            strcpy(tms_g_var_names[tms_g_var_count], tmp);
            variable_index = tms_g_var_count++;
        }
    }
    return variable_index;
}

tms_math_expr *_tms_init_math_expr(char *local_expr, bool enable_complex)
{
    int dyn_size = 8, i, j, s_index, length = strlen(local_expr), s_count;

    // Pointer to subexpressions heap array
    tms_math_subexpr *S;

    // Pointer to the math_expr generated
    tms_math_expr *M = malloc(sizeof(tms_math_expr));

    M->var_count = 0;
    M->var_data = NULL;
    M->enable_complex = enable_complex;

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
        else if (tms_is_digit(local_expr[i]))
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
                tms_error_handler(UNDEFINED_FUNCTION, EH_SAVE, EH_NONFATAL_ERROR, solve_start - 2);
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
                tms_error_handler(UNDEFINED_FUNCTION, EH_SAVE, EH_NONFATAL_ERROR, solve_start - 2);
                return false;
            }
        }
    }
    return true;
}

int _tms_init_nodes(char *local_expr, tms_math_expr *M, int s_index, int *operator_index)
{
    tms_math_subexpr *S = M->subexpr_ptr;
    int s_count = M->subexpr_count, op_count = S[s_index].op_count;
    int i, status;
    tms_op_node *node_block;
    int solve_start = S[s_index].solve_start;
    int solve_end = S[s_index].solve_end;

    // Allocating nodes
    if (op_count == 0)
        S[s_index].nodes = malloc(sizeof(tms_op_node));
    else
        S[s_index].nodes = malloc(op_count * sizeof(tms_op_node));

    node_block = S[s_index].nodes;

    // Checking if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        tms_error_handler(RIGHT_OP_MISSING, EH_SAVE, EH_FATAL_ERROR, operator_index[op_count - 1]);
        return -1;
    }
    // Filling operations and index data into each op_node
    for (i = 0; i < op_count; ++i)
    {
        node_block[i].operator_index = operator_index[i];
        node_block[i].operator= local_expr[operator_index[i]];
    }

    // Case of expression with one term, use one op_node with operand1 to hold the number
    if (op_count == 0)
    {
        i = tms_find_subexpr_starting_at(S, S[s_index].solve_start, s_index, 1);
        S[s_index].nodes[0].var_metadata = 0;
        // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
        if (i != -1)
            *(S[i].result) = &(node_block->left_operand);
        else
        {
            node_block->left_operand = tms_read_value(local_expr, solve_start, M->enable_complex);
            if (isnan((double)node_block->left_operand))
            {
                status = tms_set_var_metadata(local_expr + solve_start, node_block, 'l');
                if (!status)
                {
                    tms_error_handler(UNDEFINED_VARIABLE, EH_SAVE, EH_FATAL_ERROR, solve_start);
                    return -1;
                }
            }
        }

        S[s_index].start_node = 0;
        S[s_index].result = &(node_block->result);
        node_block[0].next = NULL;
        // If the one term expression is the last one, use the math_struct answer
        if (s_index == s_count - 1)
        {
            node_block[0].result = &M->answer;
            // Signal to the parser that all subexpressions were completed
            return 2;
        }

        // Signal to the parser that processing this subexpression is done (because it has no operators)
        return 1;
    }
    // Case where at least one operator was found
    else
    {
        // Set each op_node's priority data
        tms_set_priority(node_block, op_count);

        for (i = 0; i < op_count; ++i)
        {
            node_block[i].node_index = i;
            node_block[i].var_metadata = 0;
        }
    }
    // Check if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        tms_error_handler(RIGHT_OP_MISSING, EH_SAVE, EH_FATAL_ERROR, operator_index[op_count - 1]);
        return -1;
    }
    // Set operator type and index for each op_node
    for (i = 0; i < op_count; ++i)
    {
        node_block[i].operator_index = operator_index[i];
        node_block[i].operator= local_expr[operator_index[i]];
    }
    return 0;
}

int _tms_set_operands(char *local_expr, tms_math_expr *M, int s_index, bool enable_variables)
{
    tms_math_subexpr *S = M->subexpr_ptr;
    int op_count = S[s_index].op_count;
    int i, status;
    tms_op_node *node_block = S[s_index].nodes;
    int solve_start = S[s_index].solve_start;

    // Reading the first number
    // Treat +x and -x as 0-x and 0+x
    if (node_block[0].operator_index == S[s_index].solve_start)
    {
        if (node_block[0].operator== '+' || node_block[0].operator== '-')
            node_block[0].left_operand = 0;
        else
        {
            tms_error_handler(SYNTAX_ERROR, EH_SAVE, EH_FATAL_ERROR, node_block[0].operator_index);
            return -1;
        }
    }
    else
    {
        node_block[0].left_operand = tms_read_value(local_expr, solve_start, M->enable_complex);
        // If reading a value fails, it is probably a variable like x
        if (isnan((double)node_block[0].left_operand))
        {
            // Checking for the variable 'x'
            if (enable_variables == true)
                status = tms_set_var_metadata(local_expr + solve_start, node_block, 'l');
            else
                status = false;
            // Variable x not found, the left operand is probably another subexpression
            if (!status)
            {
                status = tms_find_subexpr_starting_at(S, solve_start, s_index, 1);
                if (status == -1)
                {
                    tms_error_handler(UNDEFINED_VARIABLE, EH_SAVE, EH_FATAL_ERROR, solve_start);
                    return -1;
                }
                else
                    *(S[status].result) = &(node_block[0].left_operand);
            }
        }
    }
    // Intermediate nodes, read number to the appropriate op_node operand
    for (i = 0; i < op_count - 1; ++i)
    {
        if (node_block[i].priority >= node_block[i + 1].priority)
        {
            node_block[i].right_operand = tms_read_value(local_expr, node_block[i].operator_index + 1, M->enable_complex);
            // Case of reading the number to the right operand of a op_node
            if (isnan((double)node_block[i].right_operand))
            {
                // Checking for the variable 'x'
                if (enable_variables == true)
                    status = tms_set_var_metadata(local_expr + node_block[i].operator_index + 1, node_block + i, 'r');
                else
                    status = false;
                if (!status)
                {
                    // Case of a subexpression result as right_operand, set its result pointer to right_operand
                    status = tms_find_subexpr_starting_at(S, node_block[i].operator_index + 1, s_index, 1);
                    if (status == -1)
                    {
                        tms_error_handler(UNDEFINED_VARIABLE, EH_SAVE, EH_FATAL_ERROR, node_block[i].operator_index + 1);
                        return -1;
                    }
                    else
                        *(S[status].result) = &(node_block[i].right_operand);
                }
            }
        }
        // Case of the op_node[i] having less priority than the next op_node, use next op_node's left_operand
        else
        {
            // Read number
            node_block[i + 1].left_operand = tms_read_value(local_expr, node_block[i].operator_index + 1, M->enable_complex);
            if (isnan((double)node_block[i + 1].left_operand))
            {
                // Checking for the variable 'x'
                if (enable_variables == true)
                    status = tms_set_var_metadata(local_expr + node_block[i].operator_index + 1, node_block + i + 1, 'l');
                else
                    status = false;
                // Again a subexpression
                if (!status)
                {
                    status = tms_find_subexpr_starting_at(S, node_block[i].operator_index + 1, s_index, 1);
                    if (status == -1)
                    {
                        tms_error_handler(UNDEFINED_VARIABLE, EH_SAVE, EH_FATAL_ERROR, node_block[i].operator_index + 1);
                        return -1;
                    }
                    else
                        *(S[status].result) = &(node_block[i + 1].left_operand);
                }
            }
        }
    }
    // Place the last number in the last op_node
    // Read a value
    node_block[op_count - 1].right_operand = tms_read_value(local_expr, node_block[op_count - 1].operator_index + 1, M->enable_complex);
    if (isnan((double)node_block[op_count - 1].right_operand))
    {
        // Check for the variable 'x'
        if (enable_variables == true)
            status = tms_set_var_metadata(local_expr + node_block[op_count - 1].operator_index + 1, node_block + op_count - 1, 'r');
        else
            status = false;
        if (!status)
        {
            // If an expression is the last term, find it and set the pointers
            status = tms_find_subexpr_starting_at(S, node_block[op_count - 1].operator_index + 1, s_index, 1);
            if (status == -1)
            {
                tms_error_handler(UNDEFINED_VARIABLE, EH_SAVE, EH_FATAL_ERROR, node_block[i].operator_index + 1);
                return -1;
            }
            *(S[status].result) = &(node_block[op_count - 1].right_operand);
        }
    }
    return 0;
}

bool _tms_set_evaluation_order(tms_math_subexpr *S)
{
    int op_count = S->op_count;
    int i, j;
    tms_op_node *node_block = S->nodes;
    // Set the starting op_node by searching the first op_node with the highest priority
    for (i = 3; i > 0; --i)
    {
        for (j = 0; j < op_count; ++j)
        {
            if (node_block[j].priority == i)
            {
                S->start_node = j;
                // Exiting the main loop by setting i outside continue condition
                i = -1;
                break;
            }
        }
    }

    i = S->start_node;
    if (i < 0)
    {
        tms_error_handler(INTERNAL_ERROR, EH_SAVE, EH_FATAL_ERROR, 0);
        return false;
    }
    int target_priority = node_block[i].priority;
    j = i + 1;
    while (target_priority > 0)
    {
        // Run through the nodes to find a op_node with the target priority
        while (j < op_count)
        {
            if (node_block[j].priority == target_priority)
            {
                node_block[i].next = node_block + j;
                // The next run starts from the next node
                i = j;
            }
            ++j;
        }
        --target_priority;
        j = 0;
    }
    // Set the next pointer of the last op_node to NULL
    node_block[i].next = NULL;
    return true;
}

void _tms_set_result_pointers(tms_math_expr *M, int s_index)
{
    tms_math_subexpr *S = M->subexpr_ptr;
    tms_op_node *tmp_node, *node_block = S[s_index].nodes;

    int i, op_count = S[s_index].op_count;
    // Set result pointers for each op_node based on position and priority
    tmp_node = &(node_block[S[s_index].start_node]);
    int left_node, right_node, prev_index = -2, prev_left = -2, prev_right = -2;
    while (tmp_node->next != NULL)
    {
        i = tmp_node->node_index;
        // Find the previous and next nodes to compare
        left_node = i - 1, right_node = i + 1;

        while (left_node != -1)
        {
            if (left_node == prev_index)
            {
                // Optimize the edge case of successive <= priority nodes like a+a+a+a+a+a+a+a by using previous results
                left_node = prev_left;
                break;
            }
            else if (tmp_node->priority <= node_block[left_node].priority)
                --left_node;
            else
                break;
        }

        while (right_node < op_count)
        {
            if (right_node == prev_index)
            {
                // Optimization
                right_node = prev_right;
                break;
            }
            else if (tmp_node->priority < node_block[right_node].priority)
                ++right_node;
            else
                break;
        }

        // Case of the first op_node or a op_node with no left candidate
        if (left_node == -1)
            tmp_node->result = &(node_block[right_node].left_operand);
        else if (right_node == op_count)
            tmp_node->result = &(node_block[left_node].right_operand);
        // Case of a op_node between 2 nodes
        else if (left_node > -1 && right_node < op_count)
        {
            if (node_block[left_node].priority >= node_block[right_node].priority)
                tmp_node->result = &(node_block[left_node].right_operand);
            else
                tmp_node->result = &(node_block[right_node].left_operand);
        }
        // Case of the last op_node
        else if (i == op_count - 1)
            tmp_node->result = &(node_block[left_node].right_operand);
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

tms_math_expr *tms_parse_expr(char *expr, bool enable_variables, bool enable_complex)
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
    - Fill nodes with values or set as variable (using x)
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

        status = _tms_init_nodes(local_expr, M, s_index, operator_index);
        free(operator_index);

        // Exiting due to error
        if (status == -1)
        {
            tms_delete_math_expr(M);
            return NULL;
        }
        else if (status == 1)
            continue;
        else if (status == 2)
            break;

        status = _tms_set_operands(local_expr, M, s_index, enable_variables);
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

    // Set variables metadata
    if (enable_variables)
        _tms_set_var_data(M);

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
        tms_error_handler(INTERNAL_ERROR, EH_SAVE, EH_FATAL_ERROR, -1);
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

double complex tms_evaluate(tms_math_expr *M)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return NAN;

    tms_op_node *i_node;
    int s_index = 0, s_count = M->subexpr_count;
    // Subexpression pointer to access the subexpression array.
    tms_math_subexpr *S = M->subexpr_ptr;
    while (s_index < s_count)
    {
        // Case with special function
        if (S[s_index].nodes == NULL)
        {
            char *args;
            if (S[s_index].exec_extf)
            {
                // Backup the current global expression to let the extended function change it freely for its error reporting
                char *backup = _tms_g_expr;

                int length = S[s_index].solve_end - S[s_index].solve_start + 1;
                // Copy args from the expression to a separate array.
                args = malloc((length + 1) * sizeof(char));
                memcpy(args, _tms_g_expr + S[s_index].solve_start, length * sizeof(char));
                args[length] = '\0';
                // Calling the extended function
                **(S[s_index].result) = (*(S[s_index].func.extended))(args);

                // Restore
                _tms_g_expr = backup;

                if (isnan((double)**(S[s_index].result)))
                {
                    tms_error_handler(EXTF_FAILURE, EH_SAVE, EH_FATAL_ERROR, S[s_index].subexpr_start);
                    return NAN;
                }
                S[s_index].exec_extf = false;
                free(args);
            }
            ++s_index;

            continue;
        }
        i_node = S[s_index].nodes + S[s_index].start_node;

        if (S[s_index].op_count == 0)
            *(i_node->result) = i_node->left_operand;
        else
            while (i_node != NULL)
            {
                if (i_node->result == NULL)
                {
                    tms_error_handler(INTERNAL_ERROR, EH_SAVE, EH_FATAL_ERROR, -1);
                    return NAN;
                }
                switch (i_node->operator)
                {
                case '+':
                    *(i_node->result) = i_node->left_operand + i_node->right_operand;
                    break;

                case '-':
                    *(i_node->result) = i_node->left_operand - i_node->right_operand;
                    break;

                case '*':
                    *(i_node->result) = i_node->left_operand * i_node->right_operand;
                    break;

                case '/':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(DIVISION_BY_ZERO, EH_SAVE, EH_FATAL_ERROR, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(MODULO_ZERO, EH_SAVE, EH_FATAL_ERROR, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->result) = fmod(i_node->left_operand, i_node->right_operand);
                    break;

                case '^':
                    // Workaround for the edge case where something like (6i)^2 is producing a small imaginary part
                    if (cimag(i_node->right_operand) == 0 && round(creal(i_node->right_operand)) - creal(i_node->right_operand) == 0)
                    {
                        *(i_node->result) = 1;
                        int i;
                        if (creal(i_node->right_operand) > 0)
                            for (i = 0; i < creal(i_node->right_operand); ++i)
                                *(i_node->result) *= i_node->left_operand;

                        else if (creal(i_node->right_operand) < 0)
                            for (i = 0; i > creal(i_node->right_operand); --i)
                                *(i_node->result) /= i_node->left_operand;
                    }
                    else
                        *(i_node->result) = cpow(i_node->left_operand, i_node->right_operand);
                    break;
                }
                i_node = i_node->next;
            }

        // Executing function on the subexpression result
        switch (S[s_index].func_type)
        {
        case 1:
            **(S[s_index].result) = (*(S[s_index].func.real))(**(S[s_index].result));
            break;
        case 2:
            **(S[s_index].result) = (*(S[s_index].func.cmplx))(**(S[s_index].result));
            break;
        }

        if (isnan((double)**(S[s_index].result)))
        {
            tms_error_handler(MATH_ERROR, EH_SAVE, EH_NONFATAL_ERROR, S[s_index].solve_start, -1);
            return NAN;
        }
        ++s_index;
    }

    if (M->runvar_i != -1)
        tms_g_var_values[M->runvar_i] = M->answer;

    // tms_dump_expr(M, true);

    return M->answer;
}

bool tms_set_var_metadata(char *expr, tms_op_node *x_node, char operand)
{
    bool is_negative = false, is_variable = false;
    if (*expr == 'x')
        is_variable = true;
    else if (expr[1] == 'x')
    {
        if (*expr == '+')
            is_variable = true;
        else if (*expr == '-')
            is_variable = is_negative = true;
        else
            return false;
    }
    // The value is not the variable x
    else
        return false;
    if (is_variable)
    {
        // x as left op
        if (operand == 'l')
        {
            if (is_negative)
                x_node->var_metadata = x_node->var_metadata | 0b101;
            else
                x_node->var_metadata = x_node->var_metadata | 0b1;
        }
        // x as right op
        else if (operand == 'r')
        {
            if (is_negative)
                x_node->var_metadata = x_node->var_metadata | 0b1010;
            else
                x_node->var_metadata = x_node->var_metadata | 0b10;
        }
        else
        {
            puts("Invaild operand argument.");
            exit(2);
        }
    }
    return true;
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
    free(M->var_data);
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

bool _print_operand_source(tms_math_subexpr *S, double complex *operand_ptr, int s_index, bool was_evaluated)
{
    int n;
    while (s_index != -1)
    {
        for (n = 0; n < S[s_index].op_count; ++n)
        {
            if (operand_ptr == S[s_index].nodes[n].result)
            {
                printf("[S%d;N%d]", s_index, n);
                if (was_evaluated)
                {
                    printf(" = ");
                    tms_print_value(*operand_ptr);
                }
                return true;
            }
        }
        if (operand_ptr == *(S[s_index].result))
        {
            printf("[S:%d]", s_index);
            return true;
        }
        --s_index;
    }
    return false;
}

void tms_dump_expr(tms_math_expr *M, bool was_evaluated)
{
    int s_index = 0;
    int s_count = M->subexpr_count;
    tms_math_subexpr *S = M->subexpr_ptr;
    char *tmp = NULL;
    tms_op_node *tmp_node;
    puts("Dumping expression data:\n");
    while (s_index < s_count)
    {
        // Find the name of the function to execute
        if (S[s_index].func_type != 0)
            tmp = _tms_lookup_function_name(S[s_index].func.real, S[s_index].func_type);
        else
            tmp = NULL;

        printf("subexpr %d:\n ftype = %u, fname = %s, depth = %d\n",
               s_index, S[s_index].func_type, tmp, S[s_index].depth);
        // Lookup result pointer location
        for (int i = 0; i < s_count; ++i)
        {
            for (int j = 0; j < S[i].op_count; ++j)
            {
                if (S[i].nodes[j].result == *(S[s_index].result))
                {
                    printf("result at subexpr %d, node %d\n\n", i, j);
                    break;
                }
            }
            // Case of a no op expression
            if (S[i].op_count == 0 && S[i].nodes != NULL && S[i].nodes[0].result == *(S[s_index].result))
                printf("result at subexpr %d, node 0\n\n", i);
        }
        tmp_node = S[s_index].nodes + S[s_index].start_node;

        // Dump nodes data
        while (tmp_node != NULL)
        {
            printf("[%d]: ", tmp_node->node_index);
            printf("( ");

            if (_print_operand_source(S, &(tmp_node->left_operand), s_index, was_evaluated) == false)
                tms_print_value(tmp_node->left_operand);

            printf(" )");
            // No one wants to see uninitialized values (skip for nodes used to hold one number)
            if (S[s_index].op_count != 0)
            {
                printf(" %c ", tmp_node->operator);
                printf("( ");

                if (_print_operand_source(S, &(tmp_node->right_operand), s_index, was_evaluated) == false)
                    tms_print_value(tmp_node->right_operand);

                printf(" )");
            }

            if (tmp_node->next != NULL)
            {
                int i;
                char left_or_right = '\0';
                for (i = 0; i < S[s_index].op_count; ++i)
                {
                    if (&(S[s_index].nodes[i].right_operand) == tmp_node->result)
                    {
                        left_or_right = 'R';
                        break;
                    }
                    else if (&(S[s_index].nodes[i].left_operand) == tmp_node->result)
                    {
                        left_or_right = 'L';
                        break;
                    }

                    if (S[s_index].op_count == 0 && &(S[s_index].nodes->right_operand) == tmp_node->result)
                    {
                        i = 0;
                        left_or_right = 'R';
                    }
                }

                printf(" a: %d%c --> ", i, left_or_right);
            }
            else
                printf(" --> ");
            printf("\n");
            tmp_node = tmp_node->next;
        }
        ++s_index;
        puts("end\n");
    }
}