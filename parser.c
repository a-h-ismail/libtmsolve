/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"
#include "parser.h"

int compare_subexps_depth(const void *a, const void *b)
{
    if ((*((m_subexpr *)a)).depth < (*((m_subexpr *)b)).depth)
        return 1;
    else if ((*((m_subexpr *)a)).depth > (*((m_subexpr *)b)).depth)
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
double complex ccbrt_cpow(double complex z)
{
    return cpow(z, 1 / 3);
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

char *r_function_name[] =
    {"fact", "abs", "ceil", "floor", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log", NULL};
double (*r_function_ptr[])(double) =
    {factorial, fabs, ceil, floor, sqrt, cbrt, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, rd_cos, rd_sin, rd_tan, log, log10};
// Extended functions, may take more than one parameter (stored in a comma separated string)
char *ext_function_name[] = {"int", "der", NULL};
double (*ext_math_function[])(char *) =
    {integrate, derivative};

// Complex functions
char *cmplx_function_name[] =
    {"abs", "arg", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "log", NULL};
double complex (*cmplx_function_ptr[])(double complex) =
    {cabs_z, carg_z, csqrt, ccbrt_cpow, cacosh, casinh, catanh, cacos, casin, catan, ccosh, csinh, ctanh, ccos, csin, ctan, clog};

int _set_runtime_var(char *expr, int i)
{
    int j;
    // Used to store the index of the variable to assign the answer to.
    int variable_index = -1;

    // Search for assignment operator, to enable user defined variables

    if (i == 0)
    {
        error_handler(SYNTAX_ERROR, 1, 1, 0);
        return -1;
    }
    else
    {
        // Check if another assignment operator is used
        j = f_search(expr, "=", i + 1, false);
        _glob_expr = expr;
        if (j != -1)
        {
            error_handler(MULTIPLE_ASSIGNMENT_ERROR, 1, 1, j);
            return -1;
        }
        // Store the variable name in this array
        char tmp[i + 1];
        strncpy(tmp, expr, i);
        tmp[i] = '\0';
        if (valid_name(tmp) == false)
        {
            error_handler(INVALID_VARIABLE_NAME, 1, 1, -1);
            return -1;
        }
        for (j = 0; j < variable_count; ++j)
        {
            // An existing variable is found
            if (strcmp(variable_names[j], tmp) == 0)
            {
                variable_index = j;
                break;
            }
        }
        if (j < hardcoded_variable_count)
        {
            error_handler(OVERWRITE_BUILTIN_VARIABLE, 1, 1, i);
            return -1;
        }
        // Create a new variable
        if (j == variable_count)
        {
            // Dynamically expand size
            if (variable_count == variable_max)
            {
                variable_max *= 2;
                variable_names = realloc(variable_names, variable_max * sizeof(char *));
                variable_values = realloc(variable_values, variable_max * sizeof(double complex));
            }
            variable_names[variable_count] = malloc((strlen(tmp) + 1) * sizeof(char));
            strcpy(variable_names[variable_count], tmp);
            variable_index = variable_count++;
        }
    }
    return variable_index;
}

math_expr *_init_math_expr(char *local_expr, bool enable_complex)
{
    int dyn_size = 8, i, j, s_index, length = strlen(local_expr), s_count;

    // Pointer to subexpressions heap array
    m_subexpr *S;

    // Pointer to the math_expr generated
    math_expr *M = malloc(sizeof(math_expr));

    M->var_count = 0;
    M->var_data = NULL;
    M->enable_complex = enable_complex;

    /*
    Parsing steps:
    - Locate and determine the depth of each subexpression (the whole expression's "subexpression" has a depth of 0)
    - Fill the expression data (start/end...)
    - Sort subexpressions by depth (high to low)
    - Parse subexpressions from high to low depth
    */

    S = malloc(dyn_size * sizeof(m_subexpr));

    int depth = 0;
    s_index = 0;
    // Determining the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        // Resize the subexpr array on the fly
        if (s_index == dyn_size)
        {
            dyn_size *= 2;
            S = realloc(S, dyn_size * sizeof(m_subexpr));
        }
        if (local_expr[i] == '(')
        {
            S[s_index].func.extended = NULL;
            S[s_index].func_type = 0;
            S[s_index].exec_extf = true;

            // Treat extended functions as a subexpression
            for (j = 0; j < sizeof(ext_math_function) / sizeof(*ext_math_function); ++j)
            {
                int tmp;
                tmp = r_search(local_expr, ext_function_name[j], i - 1, true);

                // Found an extended function
                if (tmp != -1)
                {
                    S[s_index].subexpr_start = tmp;
                    S[s_index].solve_start = i + 1;
                    i = find_closing_parenthesis(local_expr, i);
                    S[s_index].solve_end = i - 1;
                    S[s_index].depth = depth + 1;
                    S[s_index].func.extended = ext_math_function[j];
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
            S[s_index].solve_end = find_closing_parenthesis(local_expr, i) - 1;
            ++s_index;
        }
        else if (local_expr[i] == ')')
            --depth;
    }
    // + 1 for the subexpression with depth 0
    s_count = s_index + 1;
    // Shrink the block to the required size
    S = realloc(S, s_count * sizeof(m_subexpr));

    // Copy the pointer to the structure
    M->subexpr_ptr = S;

    M->subexpr_count = s_count;

    // The whole expression's "subexpression"
    S[s_index].depth = 0;
    S[s_index].solve_start = S[s_index].subexpr_start = 0;
    S[s_index].solve_end = length - 1;
    S[s_index].func.extended = NULL;
    S[s_index].func_type = 0;
    S[s_index].exec_extf = true;

    // Sort by depth (high to low)
    qsort(S, s_count, sizeof(m_subexpr), compare_subexps_depth);
    return M;
}

int *_get_operator_indexes(char *local_expr, m_subexpr *S, int s_index)
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
        // Skipping over an already processed expression
        if (local_expr[i] == '(')
        {
            int previous_subexp;
            previous_subexp = find_subexpr_by_start(S, i + 1, s_index, 2);
            if (previous_subexp != -1)
                i = S[previous_subexp].solve_end + 1;
        }
        else if (is_digit(local_expr[i]))
            continue;

        else if (is_op(local_expr[i]) && i != solve_start)
        {
            // Skipping a + or - used in scientific notation (like 1e+5)
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
            // Skipping a + or - coming just after an operator
            if (local_expr[i + 1] == '-' || local_expr[i + 1] == '+')
                ++i;
            ++op_count;
        }
    }

    // Send the number of ops to the parser
    S[s_index].op_count = op_count;

    return operator_index;
}

bool _set_function_ptr(char *local_expr, math_expr *M, int s_index, int *operator_index)
{
    int i, j;
    m_subexpr *S = M->subexpr_ptr;
    int solve_start = S[s_index].solve_start;

    // Searching for any function preceding the expression to set the function pointer
    if (solve_start > 1 && (is_alphabetic(local_expr[solve_start - 2]) || local_expr[solve_start - 2] == '_'))
    {
        if (!M->enable_complex)
        {
            // Determining the number of functions to check at runtime
            for (i = 0; i < array_length(r_function_ptr); ++i)
            {
                j = r_search(local_expr, r_function_name[i], solve_start - 2, true);
                if (j != -1)
                {
                    S[s_index].func.real = r_function_ptr[i];
                    S[s_index].func_type = 1;
                    // Setting the start of the subexpression to the start of the function name
                    S[s_index].subexpr_start = j;
                    break;
                }
            }
            // The function isn't defined for real operations (loop exited due to condition instead of break)
            if (i == array_length(r_function_ptr) + 1)
            {
                error_handler(UNDEFINED_FUNCTION, 1, 0, solve_start - 2);
                delete_math_expr(M);
                free(operator_index);
                return false;
            }
        }
        else
        {
            for (i = 0; i < array_length(cmplx_function_ptr); ++i)
            {
                j = r_search(local_expr, cmplx_function_name[i], solve_start - 2, true);
                if (j != -1)
                {
                    S[s_index].func.cmplx = cmplx_function_ptr[i];
                    S[s_index].func_type = 2;
                    S[s_index].subexpr_start = j;
                    break;
                }
            }
            // The complex function is not defined
            if (i == array_length(cmplx_function_ptr) + 1)
            {
                error_handler(UNDEFINED_FUNCTION, 1, 0, solve_start - 2);
                free(operator_index);
                delete_math_expr(M);
                return false;
            }
        }
    }
    return true;
}

int _init_nodes(char *local_expr, math_expr *M, int s_index, int *operator_index)
{
    m_subexpr *S = M->subexpr_ptr;
    int s_count = M->subexpr_count, op_count = S[s_index].op_count;
    int i, status;
    op_node *node_block;
    int solve_start = S[s_index].solve_start;
    int solve_end = S[s_index].solve_end;

    // Allocating nodes
    if (op_count == 0)
        S[s_index].nodes = malloc(sizeof(op_node));
    else
        S[s_index].nodes = malloc(op_count * sizeof(op_node));

    node_block = S[s_index].nodes;

    // Checking if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        error_handler(RIGHT_OP_MISSING, 1, 1, operator_index[op_count - 1]);
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
        i = find_subexpr_by_start(S, S[s_index].solve_start, s_index, 1);
        S[s_index].nodes[0].var_metadata = 0;
        // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
        if (i != -1)
            *(S[i].s_result) = &(node_block->left_operand);
        else
        {
            node_block->left_operand = read_value(local_expr, solve_start, M->enable_complex);
            if (isnan((double)node_block->left_operand))
            {
                status = set_variable_metadata(local_expr + solve_start, node_block, 'l');
                if (!status)
                {
                    error_handler(UNDEFINED_VARIABLE, 1, 1, solve_start);
                    return -1;
                }
            }
        }

        S[s_index].start_node = 0;
        S[s_index].s_result = &(node_block->node_result);
        node_block[0].next = NULL;
        // If the one term expression is the last one, use the math_struct answer
        if (s_index == s_count - 1)
        {
            node_block[0].node_result = &M->answer;
            // Signal to the parser that all subexpressions were completed
            return 2;
        }

        // Signal to the parser that processing this subexpression is done
        return 1;
    }
    // Case where at least one operator was found
    else
    {
        // Filling each op_node's priority data
        priority_fill(node_block, op_count);
        // Filling nodes with operands, filling operands index and setting var_metadata to 0
        for (i = 0; i < op_count; ++i)
        {
            node_block[i].node_index = i;
            node_block[i].var_metadata = 0;
        }
    }
    // Checking if the expression is terminated with an operator
    if (op_count != 0 && operator_index[op_count - 1] == solve_end)
    {
        error_handler(RIGHT_OP_MISSING, 1, 1, operator_index[op_count - 1]);
        return -1;
    }
    // Filling operations and index data into each op_node
    for (i = 0; i < op_count; ++i)
    {
        node_block[i].operator_index = operator_index[i];
        node_block[i].operator= local_expr[operator_index[i]];
    }
    return 0;
}

int _set_operands(char *local_expr, math_expr *M, int s_index, bool enable_variables)
{
    m_subexpr *S = M->subexpr_ptr;
    int op_count = S[s_index].op_count;
    int i, status;
    op_node *node_block = S[s_index].nodes;
    int solve_start = S[s_index].solve_start;

    // Reading the first number
    node_block[0].left_operand = read_value(local_expr, solve_start, M->enable_complex);
    // If reading a value fails, it is probably a variable like x
    if (isnan((double)node_block[0].left_operand))
    {
        // Checking for the variable 'x'
        if (enable_variables == true)
            status = set_variable_metadata(local_expr + solve_start, node_block, 'l');
        else
            status = false;
        // Variable x not found, the left operand is probably another subexpression
        if (!status)
        {
            status = find_subexpr_by_start(S, solve_start, s_index, 1);
            if (status == -1)
            {
                error_handler(UNDEFINED_VARIABLE, 1, 1, solve_start);
                return -1;
            }
            else
                *(S[status].s_result) = &(node_block[0].left_operand);
        }
    }

    // Intermediate nodes, read number to the appropriate op_node operand
    for (i = 0; i < op_count - 1; ++i)
    {
        if (node_block[i].priority >= node_block[i + 1].priority)
        {
            node_block[i].right_operand = read_value(local_expr, node_block[i].operator_index + 1, M->enable_complex);
            // Case of reading the number to the right operand of a op_node
            if (isnan((double)node_block[i].right_operand))
            {
                // Checking for the variable 'x'
                if (enable_variables == true)
                    status = set_variable_metadata(local_expr + node_block[i].operator_index + 1, node_block + i, 'r');
                else
                    status = false;
                if (!status)
                {
                    // Case of a subexpression result as right_operand, set its result pointer to right_operand
                    status = find_subexpr_by_start(S, node_block[i].operator_index + 1, s_index, 1);
                    if (status == -1)
                    {
                        error_handler(UNDEFINED_VARIABLE, 1, 1, node_block[i].operator_index + 1);
                        return -1;
                    }
                    else
                        *(S[status].s_result) = &(node_block[i].right_operand);
                }
            }
        }
        // Case of the op_node[i] having less priority than the next op_node, use next op_node's left_operand
        else
        {
            // Read number
            node_block[i + 1].left_operand = read_value(local_expr, node_block[i].operator_index + 1, M->enable_complex);
            if (isnan((double)node_block[i + 1].left_operand))
            {
                // Checking for the variable 'x'
                if (enable_variables == true)
                    status = set_variable_metadata(local_expr + node_block[i].operator_index + 1, node_block + i + 1, 'l');
                else
                    status = false;
                // Again a subexpression
                if (!status)
                {
                    status = find_subexpr_by_start(S, node_block[i].operator_index + 1, s_index, 1);
                    if (status == -1)
                    {
                        error_handler(UNDEFINED_VARIABLE, 1, 1, node_block[i].operator_index + 1);
                        return -1;
                    }
                    else
                        *(S[status].s_result) = &(node_block[i + 1].left_operand);
                }
            }
        }
    }
    // Placing the last number in the last op_node
    // Read a value
    node_block[op_count - 1].right_operand = read_value(local_expr, node_block[op_count - 1].operator_index + 1, M->enable_complex);
    if (isnan((double)node_block[op_count - 1].right_operand))
    {
        // Checking for the variable 'x'
        if (enable_variables == true)
            status = set_variable_metadata(local_expr + node_block[op_count - 1].operator_index + 1, node_block + op_count - 1, 'r');
        else
            status = false;
        if (!status)
        {
            // If an expression is the last term, find it and set the pointers
            status = find_subexpr_by_start(S, node_block[op_count - 1].operator_index + 1, s_index, 1);
            if (status == -1)
            {
                error_handler(UNDEFINED_VARIABLE, 1, 1, node_block[i].operator_index + 1);
                return -1;
            }
            *(S[status].s_result) = &(node_block[op_count - 1].right_operand);
        }
    }
    return 0;
}
void _set_evaluation_order(m_subexpr *S)
{
    int op_count = S->op_count;
    int i, j;
    op_node *node_block = S->nodes;
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
    int target_priority = node_block[i].priority;
    j = i + 1;
    while (target_priority > 0)
    {
        // Running through the nodes to find a op_node with the target priority
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
}
void _set_result_pointers(math_expr *M, int s_index)
{
    m_subexpr *S = M->subexpr_ptr;
    op_node *tmp_node, *node_block = S[s_index].nodes;

    int i, op_count = S[s_index].op_count;
    // Set result pointers for each op_node based on position and priority
    tmp_node = &(node_block[S[s_index].start_node]);
    int left_node, right_node, prev_index = -2, prev_left = -2, prev_right = -2;
    while (tmp_node->next != NULL)
    {
        i = tmp_node->node_index;
        // Finding the previous and next nodes to compare
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
                // Similar idea to the above
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
            tmp_node->node_result = &(node_block[right_node].left_operand);
        else if (right_node == op_count)
            tmp_node->node_result = &(node_block[left_node].right_operand);
        // Case of a op_node between 2 nodes
        else if (left_node > -1 && right_node < op_count)
        {
            if (node_block[left_node].priority >= node_block[right_node].priority)
                tmp_node->node_result = &(node_block[left_node].right_operand);
            else
                tmp_node->node_result = &(node_block[right_node].left_operand);
        }
        // Case of the last op_node
        else if (i == op_count - 1)
            tmp_node->node_result = &(node_block[left_node].right_operand);
        tmp_node = tmp_node->next;

        prev_index = i;
        prev_left = left_node;
        prev_right = right_node;
    }
    // Case of the last op_node in the traversal order, set result to be result of the subexpression
    S[s_index].s_result = &(tmp_node->node_result);
    // The last op_node in the last subexpression should point to math_struct answer
    if (s_index == M->subexpr_count - 1)
        tmp_node->node_result = &M->answer;
}

math_expr *parse_expr(char *expr, bool enable_variables, bool enable_complex)
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
    i = f_search(expr, "=", 0, false);
    if (i != -1)
    {
        variable_index = _set_runtime_var(expr, i);
        if (variable_index == -1)
            return NULL;
        local_expr = expr + i + 1;
    }
    else
        local_expr = expr;

    _glob_expr = local_expr;

    math_expr *M = _init_math_expr(local_expr, enable_complex);
    m_subexpr *S = M->subexpr_ptr;
    s_count=M->subexpr_count;

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
            S[s_index].nodes = NULL;
            S[s_index].s_result = malloc(sizeof(double complex *));
            continue;
        }

        // Get an array of the index of all operators and set their count
        int *operator_index = _get_operator_indexes(local_expr, S, s_index);

        status = _set_function_ptr(local_expr, M, s_index, operator_index);
        if (!status)
        {
            delete_math_expr(M);
            free(operator_index);
            return NULL;
        }

        status = _init_nodes(local_expr, M, s_index, operator_index);
        free(operator_index);
        // Exiting due to error
        if (status == -1)
        {
            delete_math_expr(M);
            return NULL;
        }
        else if (status == 1)
            continue;
        else if (status == 2)
            break;

        status = _set_operands(local_expr, M, s_index, enable_variables);
        if (status == -1)
        {
            delete_math_expr(M);
            return NULL;
        }

        _set_evaluation_order(S + s_index);
        _set_result_pointers(M, s_index);
    }
    // Set the variables metadata
    if (enable_variables)
        _set_var_data(M);

    // Detect
    if (local_expr != expr)
        M->variable_index = variable_index;
    else
        M->variable_index = -1;
    return M;
}

char *lookup_function_name(void *function, bool is_complex)
{
    int i;
    if (is_complex)
    {
        for (i = 0; i < array_length(cmplx_function_ptr); ++i)
        {
            if (function == (void *)(cmplx_function_ptr[i]))
                break;
        }
        if (i < array_length(cmplx_function_ptr))
            return cmplx_function_name[i];
    }
    else
    {
        for (i = 0; i < array_length(r_function_ptr); ++i)
        {
            if (function == (void *)(r_function_ptr[i]))
                break;
        }
        if (i < array_length(r_function_ptr))
            return r_function_name[i];
    }

    return NULL;
}

void *lookup_function_pointer(char *function_name, bool is_complex)
{
    int i;
    if (is_complex)
    {
        for (i = 0; i < array_length(cmplx_function_name); ++i)
        {
            if (strcmp(cmplx_function_name[i], function_name) == 0)
                break;
        }
        if (i < array_length(cmplx_function_name))
            return cmplx_function_ptr[i];
    }
    else
    {
        for (i = 0; i < array_length(r_function_name); ++i)
        {
            if (strcmp(r_function_name[i], function_name) == 0)
                break;
        }
        if (i < array_length(r_function_name))
            return r_function_ptr[i];
    }

    return NULL;
}

void convert_real_to_complex(math_expr *M)
{
    // You need to swap real functions for their complex counterparts.
    m_subexpr *S = M->subexpr_ptr;
    if (S != NULL)
    {
        int s_index;
        char *function_name;
        for (s_index = 0; s_index < M->subexpr_count; ++s_index)
        {
            // Lookup the name of the real function and find the equivalent function pointer
            function_name = lookup_function_name(S[s_index].func.real, false);
            if (function_name == NULL)
                return;
            S->func.cmplx = lookup_function_pointer(function_name, true);
            S->func_type = 2;
        }
    }
    M->enable_complex = true;
}
