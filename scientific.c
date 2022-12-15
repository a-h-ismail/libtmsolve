/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"

int compare_subexps_depth(const void *a, const void *b)
{
    if ((*((m_subexpr *)a)).depth < (*((m_subexpr *)b)).depth)
        return 1;
    else if ((*((m_subexpr *)a)).depth > (*((m_subexpr *)b)).depth)
        return -1;
    else
        return 0;
}
double complex cabs_z(double complex z)
{
    return cabs(z);
}
double complex carg_z(double complex z)
{
    return carg(z);
}
char *r_function_name[] =
    {"fact", "abs", "ceil", "floor", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log"};
double (*r_function_ptr[])(double) =
    {factorial, fabs, ceil, floor, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, cos, sin, tan, log, log10};
// Extended functions, may take more than one parameter (stored in a comma separated string)
char *ext_function_name[] = {"int", "der"};
double (*ext_math_function[])(char *) =
    {integrate, derivative};

// Complex functions
char *cmplx_function_name[] =
    {"abs", "arg", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "log"};
double complex (*cmplx_function_ptr[])(double complex) =
    {cabs_z, carg_z, cacosh, casinh, catanh, cacos, casin, catan, ccosh, csinh, ctanh, ccos, csin, ctan, clog};

double factorial(double value)
{
    double result = 1;
    for (int i = 2; i <= value; ++i)
        result *= i;
    return result;
}

double complex calculate_expr(char *expr, bool enable_complex)
{
    double complex result;
    math_expr *math_struct;
    char *expr_local = strdup(expr);
    // Check for empty input
    if (expr_local[0] == '\0')
    {
        error_handler(NO_INPUT, 1, 1, -1);
        free(expr_local);
        return NAN;
    }
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    combine_add_subtract(expr_local, 0, strlen(expr_local) - 2);
    if (parenthesis_check(expr_local) == false)
    {
        free(expr_local);
        return NAN;
    }
    if (implicit_multiplication(&expr_local) == false)
    {
        free(expr_local);
        return NAN;
    }
    math_struct = parse_expr(expr_local, false, enable_complex);
    if (math_struct == NULL)
    {
        free(expr_local);
        return NAN;
    }
    result = eval_math_expr(math_struct);
    delete_math_expr(math_struct);
    free(expr_local);
    return result;
}

double complex eval_math_expr(math_expr *M)
{
    op_node *i_node;
    int s_index = 0, s_count = M->subexpr_count;
    m_subexpr *tmp_subexpr = M->subexpr_ptr;
    /*
    #ifdef DEBUG
        // Dumping op_node data
        do
        {
            if (i_node == NULL)
            {
                puts("Special function");
            }
            else
            {
                i_node = subexpr_ptr[subexpr_index].subexpr_nodes + subexpr_ptr[subexpr_index].start_node;
                while (i_node != NULL)
                {
                    printf("%.8g %c %.8g -> ", (double)i_node->left_operand, i_node->operator, (double)i_node->right_operand);
                    i_node = i_node->next;
                }
                printf("Function: %lli", subexpr_ptr[subexpr_index].function_ptr);
                puts("|");
            }
            ++subexpr_index;
        } while (subexpr_ptr[subexpr_index - 1].last_subexp == false);
        puts("End");
        fflush(stdout);
    #endif
    */
    while (s_index < s_count)
    {
        // Case with special function
        if (tmp_subexpr[s_index].subexpr_nodes == NULL)
        {
            char *args;
            if (tmp_subexpr[s_index].execute_extended)
            {
                int length = tmp_subexpr[s_index].solve_end - tmp_subexpr[s_index].solve_start + 1;
                args = malloc((length + 1) * sizeof(char));
                memcpy(args, glob_expr + tmp_subexpr[s_index].solve_start, length * sizeof(char));
                args[length] = '\0';
                // Calling the extended function
                **(tmp_subexpr[s_index].s_result) = (*(tmp_subexpr[s_index].ext_function_ptr))(args);
                if (isnan((double)**(tmp_subexpr[s_index].s_result)))
                {
                    error_handler(MATH_ERROR, 1, 0, tmp_subexpr[s_index].solve_start);
                    return NAN;
                }
                tmp_subexpr[s_index].execute_extended = false;
                free(args);
            }
            ++s_index;

            continue;
        }
        i_node = tmp_subexpr[s_index].subexpr_nodes + tmp_subexpr[s_index].start_node;

        if (tmp_subexpr[s_index].op_count == 0)
            *(i_node->node_result) = i_node->left_operand;
        else
            while (i_node != NULL)
            {
                switch (i_node->operator)
                {
                case '+':
                    *(i_node->node_result) = i_node->left_operand + i_node->right_operand;
                    break;

                case '-':
                    *(i_node->node_result) = i_node->left_operand - i_node->right_operand;
                    break;

                case '*':
                    *(i_node->node_result) = i_node->left_operand * i_node->right_operand;
                    break;

                case '/':
                    if (i_node->right_operand == 0)
                    {
                        error_handler(DIVISION_BY_ZERO, 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        error_handler(MODULO_ZERO, 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = fmod(i_node->left_operand, i_node->right_operand);
                    break;

                case '^':
                    *(i_node->node_result) = cpow(i_node->left_operand, i_node->right_operand);
                    break;
                }
                i_node = i_node->next;
            }
        // Executing function on the subexpression result
        // Complex function
        if (M->enable_complex && tmp_subexpr[s_index].cmplx_function_ptr != NULL)
            **(tmp_subexpr[s_index].s_result) = (*(tmp_subexpr[s_index].cmplx_function_ptr))(**(tmp_subexpr[s_index].s_result));
        // Real function
        else if (!M->enable_complex && tmp_subexpr[s_index].function_ptr != NULL)
            **(tmp_subexpr[s_index].s_result) = (*(tmp_subexpr[s_index].function_ptr))(**(tmp_subexpr[s_index].s_result));

        if (isnan((double)**(tmp_subexpr[s_index].s_result)))
        {
            error_handler(MATH_ERROR, 1, 0, tmp_subexpr[s_index].solve_start);
            return NAN;
        }
        ++s_index;
    }
    return M->answer;
}

bool set_variable_metadata(char *expr, op_node *x_node, char operand)
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

math_expr *parse_expr(char *expr, bool enable_variables, bool enable_complex)
{
    int i, j, length;
    // Number of subexpressions
    int s_count = 1;
    // Used for indexing of subexpressions
    int s_index;
    // Use for dynamic growing blocks.
    int buffer_size = 10, buffer_step = 10;

    length = strlen(expr);
    m_subexpr *tmp_subexpr;
    op_node *node_block, *tmp_node;
    math_expr *math_struct = malloc(sizeof(math_expr));
    math_struct->var_count = 0;
    math_struct->var_data = NULL;
    math_struct->enable_complex = enable_complex;

    /*
    Parsing steps:
    - Locate and determine the depth of each subexpression (the whole expression's "subexpression" has a depth of 0)
    - Fill the expression data (start/end...)
    - Sort subexpressions by depth (high to low)
    - Parse subexpressions from high to low depth
    */

    tmp_subexpr = malloc(buffer_size * sizeof(m_subexpr));

    int depth = 0;
    s_index = 0;
    // Determining the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        // Resize the subexpr array on the fly
        if (s_index == buffer_size)
        {
            buffer_size += buffer_step;
            tmp_subexpr = realloc(tmp_subexpr, buffer_size * sizeof(m_subexpr));
        }
        if (expr[i] == '(')
        {
            tmp_subexpr[s_index].ext_function_ptr = NULL;
            // Treat extended functions as a subexpression
            for (j = 0; j < sizeof(ext_math_function) / sizeof(*ext_math_function); ++j)
            {
                int temp;
                temp = r_search(expr, ext_function_name[j], i - 1, true);
                if (temp != -1)
                {
                    tmp_subexpr[s_index].subexpr_start = temp;
                    tmp_subexpr[s_index].solve_start = i + 1;
                    i = find_closing_parenthesis(expr, i);
                    tmp_subexpr[s_index].solve_end = i - 1;
                    tmp_subexpr[s_index].depth = depth + 1;
                    tmp_subexpr[s_index].ext_function_ptr = ext_math_function[j];
                    break;
                }
            }
            if (tmp_subexpr[s_index].ext_function_ptr != NULL)
            {
                ++s_index;
                continue;
            }
            // Normal case
            ++depth;
            tmp_subexpr[s_index].solve_start = i + 1;
            tmp_subexpr[s_index].depth = depth;
            // The expression start is the parenthesis, may change if a function is found
            tmp_subexpr[s_index].subexpr_start = i;
            tmp_subexpr[s_index].solve_end = find_closing_parenthesis(expr, i) - 1;
            ++s_index;
        }
        else if (expr[i] == ')')
            --depth;
    }
    // + 1 for the subexpression with depth 0
    s_count = s_index + 1;
    // Shrink the block to the required size
    tmp_subexpr = realloc(tmp_subexpr, s_count * sizeof(m_subexpr));
    // Copy the pointer to the structure
    math_struct->subexpr_ptr = tmp_subexpr;

    math_struct->subexpr_count = s_count;
    for (i = 0; i < s_count; ++i)
    {
        tmp_subexpr[i].function_ptr = NULL;
        tmp_subexpr[i].cmplx_function_ptr = NULL;
        tmp_subexpr[i].subexpr_nodes = NULL;
        tmp_subexpr[i].execute_extended = true;
    }

    // The whole expression's "subexpression"
    tmp_subexpr[s_index].depth = 0;
    tmp_subexpr[s_index].solve_start = tmp_subexpr[s_index].subexpr_start = 0;
    tmp_subexpr[s_index].solve_end = length - 1;
    tmp_subexpr[s_index].ext_function_ptr = NULL;

    // Sort by depth (high to low)
    qsort(tmp_subexpr, s_count, sizeof(m_subexpr), compare_subexps_depth);

    int *operator_index, solve_start, solve_end, op_count, status;
    buffer_size = buffer_step = 1000;

    /*
    How to parse a subexp:
    - Handle the special case of extended functions (example: int, der)
    - Create an array that determines the location of operators in the string
    - Check if the subexpression has a function to execute on the result, set pointer
    - Allocate the array of nodes
    - Use the operator index array to fill the nodes data on operators type and location
    - Fill nodes operator priority
    - Fill nodes with values or set as variable (using x)
    - Set nodes order of calculation (using *next)
    - Set the result pointer of each op_node relying on its position and neighbor priorities
    - Set the subexp result double pointer to the result pointer of the last calculated op_node
    */
    for (s_index = 0; s_index < s_count; ++s_index)
    {
        if (tmp_subexpr[s_index].ext_function_ptr != NULL)
        {
            tmp_subexpr[s_index].subexpr_nodes = NULL;
            tmp_subexpr[s_index].s_result = malloc(sizeof(double complex *));
            continue;
        }

        // For simplicity
        solve_start = tmp_subexpr[s_index].solve_start;
        solve_end = tmp_subexpr[s_index].solve_end;

        operator_index = (int *)malloc(buffer_size * sizeof(int));
        // Count number of operators and store it's indexes
        for (i = solve_start, op_count = 0; i <= solve_end; ++i)
        {
            // Skipping over an already processed expression
            if (expr[i] == '(')
            {
                int previous_subexp;
                previous_subexp = find_subexpr_by_start(tmp_subexpr, i + 1, s_index, 2);
                if (previous_subexp != -1)
                    i = tmp_subexpr[previous_subexp].solve_end + 1;
            }
            else if (is_digit(expr[i]))
                continue;

            else if (is_op(expr[i]))
            {
                // Skipping a + or - used in scientific notation (like 1e+5)
                if (i > 0 && (expr[i - 1] == 'e' || expr[i - 1] == 'E') && (expr[i] == '+' || expr[i] == '-'))
                {
                    ++i;
                    continue;
                }
                // Varying the array size on demand
                if (op_count == buffer_size)
                {
                    buffer_size += buffer_step;
                    operator_index = realloc(operator_index, buffer_size * sizeof(int));
                }
                operator_index[op_count] = i;
                // Skipping a + or - coming just after an operator
                if (expr[i + 1] == '-' || expr[i + 1] == '+')
                    ++i;
                ++op_count;
            }
        }

        // Searching for any function preceding the expression to set the function pointer
        if (solve_start > 1)
        {
            if (!enable_complex)
            {
                // Determining the number of functions to check at runtime
                int total = sizeof(r_function_ptr) / sizeof(*r_function_ptr);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(expr, r_function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        tmp_subexpr[s_index].function_ptr = r_function_ptr[i];
                        // Setting the start of the subexpression to the start of the function name
                        tmp_subexpr[s_index].subexpr_start = j;
                        break;
                    }
                }
            }
            else
            {
                // Determining the number of complex functions to check
                int total = sizeof(cmplx_function_ptr) / sizeof(*cmplx_function_ptr);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(expr, cmplx_function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        tmp_subexpr[s_index].cmplx_function_ptr = cmplx_function_ptr[i];
                        tmp_subexpr[s_index].subexpr_start = j;
                        break;
                    }
                }
            }
        }

        tmp_subexpr[s_index].op_count = op_count;
        // Allocating the array of nodes
        // Case of no operators, create a dummy op_node and store the value in op1
        if (op_count == 0)
            tmp_subexpr[s_index].subexpr_nodes = malloc(sizeof(op_node));
        else
            tmp_subexpr[s_index].subexpr_nodes = malloc(op_count * sizeof(op_node));

        node_block = tmp_subexpr[s_index].subexpr_nodes;

        // Checking if the expression is terminated with an operator
        if (op_count != 0 && operator_index[op_count - 1] == solve_end)
        {
            error_handler(RIGHT_OP_MISSING, 1, 1, operator_index[op_count - 1]);
            delete_math_expr(math_struct);
            free(operator_index);
            return NULL;
        }
        // Filling operations and index data into each op_node
        for (i = 0; i < op_count; ++i)
        {
            node_block[i].operator_index = operator_index[i];
            node_block[i].operator= expr[operator_index[i]];
        }
        free(operator_index);

        // Case of expression with one term, use one op_node with operand1 to hold the number
        if (op_count == 0)
        {
            i = find_subexpr_by_start(tmp_subexpr, tmp_subexpr[s_index].solve_start, s_index, 1);
            tmp_subexpr[s_index].subexpr_nodes[0].var_metadata = 0;
            // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
            if (i != -1)
                *(tmp_subexpr[i].s_result) = &(node_block->left_operand);
            else
            {
                node_block->left_operand = read_value(expr, solve_start, enable_complex);
                if (isnan((double)node_block->left_operand))
                {
                    status = set_variable_metadata(expr + solve_start, node_block, 'l');
                    if (!status)
                    {
                        error_handler(SYNTAX_ERROR, 1, 1, solve_start);
                        delete_math_expr(math_struct);
                        return NULL;
                    }
                }
            }

            tmp_subexpr[s_index].start_node = 0;
            tmp_subexpr[s_index].s_result = &(node_block->node_result);
            node_block[0].next = NULL;
            // If the one term expression is the last one, use the math_struct answer
            if (s_index == s_count - 1)
            {
                node_block[0].node_result = &math_struct->answer;
                break;
            }
            continue;
        }
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

        // Trying to read first number into first op_node

        // If a - or + sign is in the beginning of the expression. it will be treated as 0-val or 0+val
        if (expr[solve_start] == '-' || expr[solve_start] == '+')
            node_block[0].left_operand = 0;
        else
        {
            // Read a value to the left operand
            node_block[0].left_operand = read_value(expr, solve_start, enable_complex);
            // If reading a value fails, it is probably a variable like x
            if (isnan((double)node_block[0].left_operand))
            {
                // Checking for the variable 'x'
                if (enable_variables == true)
                    status = set_variable_metadata(expr + solve_start, node_block, 'l');
                else
                    status = false;
                // Variable x not found, the left operand is probably another subexpression
                if (!status)
                {
                    status = find_subexpr_by_start(tmp_subexpr, solve_start, s_index, 1);
                    if (status == -1)
                    {
                        error_handler(SYNTAX_ERROR, 1, 1, solve_start);
                        delete_math_expr(math_struct);
                        return NULL;
                    }
                    else
                        *(tmp_subexpr[status].s_result) = &(node_block[0].left_operand);
                }
            }
        }

        // Intermediate nodes, read number to the appropriate op_node operand
        for (i = 0; i < op_count - 1; ++i)
        {
            if (node_block[i].priority >= node_block[i + 1].priority)
            {
                node_block[i].right_operand = read_value(expr, node_block[i].operator_index + 1, enable_complex);
                // Case of reading the number to the right operand of a op_node
                if (isnan((double)node_block[i].right_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_metadata(expr + node_block[i].operator_index + 1, node_block + i, 'r');
                    else
                        status = false;
                    if (!status)
                    {
                        // Case of a subexpression result as right_operand, set its result pointer to right_operand
                        status = find_subexpr_by_start(tmp_subexpr, node_block[i].operator_index + 1, s_index, 1);
                        if (status == -1)
                        {
                            error_handler(SYNTAX_ERROR, 1, 1, node_block[i].operator_index + 1);
                            delete_math_expr(math_struct);
                            return NULL;
                        }
                        else
                            *(tmp_subexpr[status].s_result) = &(node_block[i].right_operand);
                    }
                }
            }
            // Case of the op_node[i] having less priority than the next op_node, use next op_node's left_operand
            else
            {
                // Read number
                node_block[i + 1].left_operand = read_value(expr, node_block[i].operator_index + 1, enable_complex);
                if (isnan((double)node_block[i + 1].left_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_metadata(expr + node_block[i].operator_index + 1, node_block + i + 1, 'l');
                    else
                        status = false;
                    // Again a subexpression
                    if (!status)
                    {
                        status = find_subexpr_by_start(tmp_subexpr, node_block[i].operator_index + 1, s_index, 1);
                        if (status == -1)
                        {
                            error_handler(SYNTAX_ERROR, 1, 1, node_block[i].operator_index + 1);
                            delete_math_expr(math_struct);
                            return NULL;
                        }
                        else
                            *(tmp_subexpr[status].s_result) = &(node_block[i + 1].left_operand);
                    }
                }
            }
        }
        // Placing the last number in the last op_node
        // Read a value
        node_block[op_count - 1].right_operand = read_value(expr, node_block[op_count - 1].operator_index + 1, enable_complex);
        if (isnan((double)node_block[op_count - 1].right_operand))
        {
            // Checking for the variable 'x'
            if (enable_variables == true)
                status = set_variable_metadata(expr + node_block[op_count - 1].operator_index + 1, node_block + op_count - 1, 'r');
            else
                status = false;
            if (!status)
            {
                // If an expression is the last term, find it and set the pointers
                status = find_subexpr_by_start(tmp_subexpr, node_block[op_count - 1].operator_index + 1, s_index, 1);
                if (status == -1)
                {
                    error_handler(SYNTAX_ERROR, 1, 1, node_block[i].operator_index + 1);
                    return NULL;
                }
                *(tmp_subexpr[status].s_result) = &(node_block[op_count - 1].right_operand);
            }
        }
        // Set the starting op_node by searching the first op_node with the highest priority
        for (i = 3; i > 0; --i)
        {
            for (j = 0; j < op_count; ++j)
            {
                if (node_block[j].priority == i)
                {
                    tmp_subexpr[s_index].start_node = j;
                    // Exiting the main loop by setting i outside continue condition
                    i = -1;
                    break;
                }
            }
        }

        // Setting nodes order of execution
        i = tmp_subexpr[s_index].start_node;
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
                    // The op_node found is used as te start op_node for the next run
                    i = j;
                }
                ++j;
            }
            --target_priority;
            j = 0;
        }
        // Setting the next pointer of the last op_node to NULL
        node_block[i].next = NULL;

        // Set result pointers for each op_node based on position and priority
        tmp_node = &(node_block[tmp_subexpr[s_index].start_node]);
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
                    left_node = prev_left;
                    break;
                }
                else if (tmp_node->priority <= node_block[left_node].priority)
                    --left_node;
                // Optimize the edge case of successive <= priority nodes like a+a+a+a+a+a+a+a by using previous results
                else
                    break;
            }

            while (right_node < op_count)
            {
                if (right_node == prev_index)
                {
                    right_node = prev_right;
                    break;
                }
                else if (tmp_node->priority < node_block[right_node].priority)
                    ++right_node;
                // Similar idea to the above

                else
                    break;
            }
            // Case of the first op_node or a op_node with no left candidate
            if (left_node == -1)
                tmp_node->node_result = &(node_block[right_node].left_operand);
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
        tmp_subexpr[s_index].s_result = &(tmp_node->node_result);
        // The last op_node in the last subexpression should point to math_struct answer
        if (s_index == s_count - 1)
            tmp_node->node_result = &math_struct->answer;
    }
    // Set the variables metadata
    if (enable_variables)
        set_var_data(math_struct);
    return math_struct;
}

void delete_math_expr(math_expr *math_struct)
{
    int i = 0;
    m_subexpr *subexps = math_struct->subexpr_ptr;
    for (i = 0; i < math_struct->subexpr_count; ++i)
    {
        if (subexps[i].ext_function_ptr != NULL)
            free(subexps[i].s_result);
        free(subexps[i].subexpr_nodes);
    }
    free(subexps);
    free(math_struct->var_data);
    free(math_struct);
}

int_factor *find_factors(int32_t value)
{
    int32_t dividend = 2;
    int_factor *factor_list;
    int i = 0;

    factor_list = calloc(64, sizeof(int_factor));
    if (value == 0)
        return factor_list;
    factor_list[0].factor = factor_list[0].power = 1;

    // Turning negative numbers into positive for factorization to work
    if (value < 0)
        value = -value;
    // Simple optimized factorization algorithm
    while (value != 1)
    {
        if (value % dividend == 0)
        {
            ++i;
            value = value / dividend;
            factor_list[i].factor = dividend;
            factor_list[i].power = 1;
            while (value % dividend == 0 && value >= dividend)
            {
                value = value / dividend;
                ++factor_list[i].power;
            }
        }
        else
        {
            // Optimize by skipping values greater than value/2 (in other terms, the value itself is a prime)
            if (dividend > value / 2 && value != 1)
            {
                ++i;
                factor_list[i].factor = value;
                factor_list[i].power = 1;
                break;
            }
        }
        // Optimizing factorization by skipping even values (> 2)
        if (dividend > 2)
            dividend = dividend + 2;
        else
            ++dividend;
    }
    // Case of a prime number
    if (i == 0)
    {
        factor_list[1].factor = value;
        factor_list[1].power = 1;
    }
    return factor_list;
}

void reduce_fraction(fraction *fraction_str)
{
    int i = 1, j = 1, min;
    int_factor *num_factor, *denom_factor;
    num_factor = find_factors(fraction_str->b);
    denom_factor = find_factors(fraction_str->c);
    // If a zero was returned by the factorization function, nothing to do.
    if (num_factor->factor == 0 || denom_factor->factor == 0)
    {
        free(num_factor);
        free(denom_factor);
        return;
    }
    while (num_factor[i].factor != 0 && denom_factor[j].factor != 0)
    {
        if (num_factor[i].factor == denom_factor[j].factor)
        {
            // Subtract the minimun from the power of the num and denom
            min = find_min(num_factor[i].power, denom_factor[j].power);
            num_factor[i].power -= min;
            denom_factor[j].power -= min;
            ++i;
            ++j;
        }
        else
        {
            // If the factor at i is smaller increment i , otherwise increment j
            if (num_factor[i].factor < denom_factor[j].factor)
                ++i;
            else
                ++j;
        }
    }
    fraction_str->b = fraction_str->c = 1;
    // Calculate numerator and denominator after reduction
    for (i = 1; num_factor[i].factor != 0; ++i)
        fraction_str->b *= pow(num_factor[i].factor, num_factor[i].power);

    for (i = 1; denom_factor[i].factor != 0; ++i)
        fraction_str->c *= pow(denom_factor[i].factor, denom_factor[i].power);

    free(num_factor);
    free(denom_factor);
}
// Converts a floating point value to decimal representation a*b/c
fraction decimal_to_fraction(double value, bool inverse_process)
{
    int decimal_point = 1, i, j, decimal_length;
    bool success = false;
    char pattern[11], printed_value[23];
    fraction result;

    // Using the denominator as a mean to report failure, if c==0 then the function failed
    result.c = 0;

    if (fabs(value) > INT32_MAX || fabs(value) < pow(10, -log10(INT32_MAX) + 1))
        return result;

    // Store the integer part in 'a'
    result.a = floor(value);
    value -= floor(value);

    // Edge case due to floating point lack of precision
    if (1 - value < 1e-9)
        return result;

    // The case of an integer
    if (value == 0)
        return result;

    sprintf(printed_value, "%.14lf", value);
    // Removing trailing zeros
    for (i = strlen(printed_value) - 1; i > decimal_point; --i)
    {
        // Stop at the first non zero value and null terminate
        if (*(printed_value + i) != '0')
        {
            *(printed_value + i + 1) = '\0';
            break;
        }
    }

    decimal_length = strlen(printed_value) - decimal_point - 1;
    if (decimal_length >= 10)
    {
        // Look for a pattern to detect fractions from periodic decimals
        for (i = decimal_point + 1; i - decimal_point < decimal_length / 2; ++i)
        {
            // First number in the pattern (to the right of the decimal_point)
            pattern[0] = printed_value[i];
            pattern[1] = '\0';
            j = i + 1;
            while (true)
            {
                // First case: the "pattern" is smaller than the remaining decimal digits.
                if (strlen(printed_value) - j > strlen(pattern))
                {
                    // If the pattern is found again in the remaining decimal digits, jump over it.
                    if (strncmp(pattern, printed_value + j, strlen(pattern)) == 0)
                        j += strlen(pattern);
                    // If not, copy the digits between i and j to the pattern for further testing
                    else
                    {
                        strncpy(pattern, printed_value + i, j - i + 1);
                        pattern[j - i + 1] = '\0';
                        ++j;
                    }
                    // If the pattern matches the last decimal digits, stop the execution with SUCCESS
                    if (strlen(printed_value) - j == strlen(pattern) &&
                        strncmp(pattern, printed_value + strlen(printed_value) + j, strlen(pattern)) == 0)
                    {
                        success = true;
                        break;
                    }
                }
                // Second case: the pattern is bigger than the remaining digits.
                else
                {
                    // Consider a SUCCESS the case where the remaining digits except the last one match the leftmost digits of the pattern
                    if (strncmp(pattern, printed_value + j, strlen(printed_value) - j - 1) == 0)
                    {
                        success = true;
                        break;
                    }
                    else
                        break;
                }
            }
            if (success == true)
                break;
        }
    }
    // Simple cases with finite decimal digits
    else
    {
        result.c = pow(10, decimal_length);
        result.b = round(value * result.c);
        reduce_fraction(&result);
        return result;
    }
    // getting the fraction from the "pattern"
    if (success == true)
    {
        int pattern_start;
        // Just in case the pattern was found to be zeors due to minor rounding (like 5.0000000000000003)
        if (pattern[0] == '0')
            return result;
        // Generate the denominator
        for (i = 0; i < strlen(pattern); ++i)
            result.c += 9 * pow(10, i);
        // Find the pattern start in case it doesn't start right after the decimal point (like 0.79999)
        pattern_start = f_search(printed_value, pattern, decimal_point + 1);
        if (pattern_start > decimal_point + 1)
        {
            result.b = round(value * (pow(10, pattern_start - decimal_point - 1 + strlen(pattern)) - pow(10, pattern_start - decimal_point - 1)));
            result.c *= pow(10, pattern_start - decimal_point - 1);
        }
        else
            sscanf(pattern, "%lf", &result.b);
        reduce_fraction(&result);
        return result;
    }
    // trying to get the fraction by inverting it then trying the algorithm again
    else if (inverse_process == false)
    {
        double inverse_value = 1 / value;
        // case where the fraction is likely of form 1/x
        if (fabs(inverse_value - floor(inverse_value)) < 1e-10)
        {
            result.b = 1;
            result.c = inverse_value;
            return result;
        }
        // Other cases like 3/17 (non periodic but the inverse is periodic)
        else
        {
            fraction inverted = decimal_to_fraction(inverse_value, true);
            if (inverted.c != 0)
            {
                // inverse of a + b / c is c / ( a *c + b )
                result.b = inverted.c;
                result.c = inverted.b + inverted.a * inverted.c;
            }
        }
    }
    return result;
}
void priority_fill(op_node *list, int op_count)
{
    char operators[6] = {'^', '*', '/', '%', '+', '-'};
    uint8_t priority[6] = {3, 2, 2, 2, 1, 1};
    int i, j;
    for (i = 0; i < op_count; ++i)
    {
        for (j = 0; j < 7; ++j)
        {
            if (list[i].operator== operators[j])
            {
                list[i].priority = priority[j];
                break;
            }
        }
    }
}

int find_subexpr_by_start(m_subexpr *S, int start, int s_index, int mode)
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
int find_subexpr_by_end(m_subexpr *S, int end, int s_index, int s_count)
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