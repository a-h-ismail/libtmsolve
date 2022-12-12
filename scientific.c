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
    if ((*((s_expression *)a)).depth < (*((s_expression *)b)).depth)
        return 1;
    else if ((*((s_expression *)a)).depth > (*((s_expression *)b)).depth)
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
char *function_name[] =
    {"fact", "abs", "ceil", "floor", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log"};
double (*math_function[])(double) =
    {factorial, fabs, ceil, floor, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, cos, sin, tan, log, log10};
// Extended functions, may take more than one parameter (stored in a comma separated string)
char *ext_function_name[] = {"int", "der"};
double (*ext_math_function[])(char *) =
    {integral_processor, derivative};

// Complex functions
char *cmplx_function_name[] =
    {"abs", "arg", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "log"};
double complex (*cmplx_function[])(double complex) =
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
    result = evaluate(math_struct);
    delete_math_expr(math_struct);
    free(expr_local);
    return result;
}

double complex evaluate(math_expr *math_struct)
{
    node *i_node;
    int subexpr_index = 0, subexpr_count = math_struct->subexpr_count;
    s_expression *subexpr_ptr = math_struct->subexpr_ptr;
    /*
    #ifdef DEBUG
        // Dumping node data
        do
        {
            if (i_node == NULL)
            {
                puts("Special function");
            }
            else
            {
                i_node = subexpr_ptr[subexpr_index].node_list + subexpr_ptr[subexpr_index].start_node;
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
    while (subexpr_index < subexpr_count)
    {
        // Case with special function
        if (subexpr_ptr[subexpr_index].node_list == NULL)
        {
            char *args;
            if (subexpr_ptr[subexpr_index].execute_extended)
            {
                int length = subexpr_ptr[subexpr_index].solve_end - subexpr_ptr[subexpr_index].solve_start + 1;
                args = malloc((length + 1) * sizeof(char));
                memcpy(args, glob_expr + subexpr_ptr[subexpr_index].solve_start, length * sizeof(char));
                args[length] = '\0';
                // Calling the extended function
                **(subexpr_ptr[subexpr_index].result) = (*(subexpr_ptr[subexpr_index].ext_function_ptr))(args);
                if (isnan((double)**(subexpr_ptr[subexpr_index].result)))
                {
                    error_handler(MATH_ERROR, 1, 0, subexpr_ptr[subexpr_index].solve_start);
                    return NAN;
                }
                subexpr_ptr[subexpr_index].execute_extended = false;
                free(args);
            }
            ++subexpr_index;

            continue;
        }
        i_node = subexpr_ptr[subexpr_index].node_list + subexpr_ptr[subexpr_index].start_node;

        if (subexpr_ptr[subexpr_index].op_count == 0)
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
        if (math_struct->enable_complex && subexpr_ptr[subexpr_index].cmplx_function_ptr != NULL)
            **(subexpr_ptr[subexpr_index].result) = (*(subexpr_ptr[subexpr_index].cmplx_function_ptr))(**(subexpr_ptr[subexpr_index].result));
        // Real function
        else if (!math_struct->enable_complex && subexpr_ptr[subexpr_index].function_ptr != NULL)
            **(subexpr_ptr[subexpr_index].result) = (*(subexpr_ptr[subexpr_index].function_ptr))(**(subexpr_ptr[subexpr_index].result));

        if (isnan((double)**(subexpr_ptr[subexpr_index].result)))
        {
            error_handler(MATH_ERROR, 1, 0, subexpr_ptr[subexpr_index].solve_start);
            return NAN;
        }
        ++subexpr_index;
    }
    return math_struct->answer;
}

bool set_variable_metadata(char *expr, node *x_node, char operand)
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
    int i, j, length, subexpr_count = 1;
    int subexpr_index;
    int buffer_size = 10, buffer_step = 10;
    length = strlen(expr);
    s_expression *subexpr_ptr;
    node *node_block, *tmp_node;
    math_expr *math_struct = malloc(sizeof(math_expr));
    math_struct->var_count = 0;
    math_struct->variable_ptr = NULL;
    math_struct->enable_complex = enable_complex;

    /*
    Parsing steps:
    - Locate and determine the depth of each subexpression (the whole expression's "subexpression" has a depth of 0)
    - Fill the expression data (start/end...)
    - Sort subexpressions by depth (high to low)
    - Parse subexpressions from high to low depth
    */

    length = strlen(expr);
    subexpr_ptr = malloc(buffer_size * sizeof(s_expression));

    int depth = 0;
    subexpr_index = 0;
    // Determining the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        // Resize the subexpr array on the fly
        if (subexpr_index == buffer_size)
        {
            buffer_size += buffer_step;
            subexpr_ptr = realloc(subexpr_ptr, buffer_size * sizeof(s_expression));
        }
        if (expr[i] == '(')
        {
            subexpr_ptr[subexpr_index].ext_function_ptr = NULL;
            // Treat extended functions as a subexpression
            for (j = 0; j < sizeof(ext_math_function) / sizeof(*ext_math_function); ++j)
            {
                int temp;
                temp = r_search(expr, ext_function_name[j], i - 1, true);
                if (temp != -1)
                {
                    subexpr_ptr[subexpr_index].expression_start = temp;
                    subexpr_ptr[subexpr_index].solve_start = i + 1;
                    i = find_closing_parenthesis(expr, i);
                    subexpr_ptr[subexpr_index].solve_end = i - 1;
                    subexpr_ptr[subexpr_index].depth = depth + 1;
                    subexpr_ptr[subexpr_index].ext_function_ptr = ext_math_function[j];
                    break;
                }
            }
            if (subexpr_ptr[subexpr_index].ext_function_ptr != NULL)
            {
                ++subexpr_index;
                continue;
            }
            // Normal case
            ++depth;
            subexpr_ptr[subexpr_index].solve_start = i + 1;
            subexpr_ptr[subexpr_index].depth = depth;
            // The expression start is the parenthesis, may change if a function is found
            subexpr_ptr[subexpr_index].expression_start = i;
            subexpr_ptr[subexpr_index].solve_end = find_closing_parenthesis(expr, i) - 1;
            ++subexpr_index;
        }
        else if (expr[i] == ')')
            --depth;
    }
    // + 1 for the subexpression with depth 0
    subexpr_count = subexpr_index + 1;
    // Shrink the block to the required size
    subexpr_ptr = realloc(subexpr_ptr, subexpr_count * sizeof(s_expression));
    // Copy the pointer to the structure
    math_struct->subexpr_ptr = subexpr_ptr;

    math_struct->subexpr_count = subexpr_count;
    for (i = 0; i < subexpr_count; ++i)
    {
        subexpr_ptr[i].function_ptr = NULL;
        subexpr_ptr[i].cmplx_function_ptr = NULL;
        subexpr_ptr[i].node_list = NULL;
        subexpr_ptr[i].execute_extended = true;
    }

    // The whole expression's "subexpression"
    subexpr_ptr[subexpr_index].depth = 0;
    subexpr_ptr[subexpr_index].solve_start = subexpr_ptr[subexpr_index].expression_start = 0;
    subexpr_ptr[subexpr_index].solve_end = length - 1;
    subexpr_ptr[subexpr_index].ext_function_ptr = NULL;

    // Sort by depth (high to low)
    qsort(subexpr_ptr, subexpr_count, sizeof(s_expression), compare_subexps_depth);

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
    - Set the result pointer of each node relying on its position and neighbor priorities
    - Set the subexp result double pointer to the result pointer of the last calculated node
    */
    for (subexpr_index = 0; subexpr_index < subexpr_count; ++subexpr_index)
    {
        if (subexpr_ptr[subexpr_index].ext_function_ptr != NULL)
        {
            subexpr_ptr[subexpr_index].node_list = NULL;
            subexpr_ptr[subexpr_index].result = malloc(sizeof(double complex *));
            continue;
        }

        // For simplicity
        solve_start = subexpr_ptr[subexpr_index].solve_start;
        solve_end = subexpr_ptr[subexpr_index].solve_end;

        operator_index = (int *)malloc(buffer_size * sizeof(int));
        // Count number of operators and store it's indexes
        for (i = solve_start, op_count = 0; i <= solve_end; ++i)
        {
            // Skipping over an already processed expression
            if (expr[i] == '(')
            {
                int previous_subexp;
                previous_subexp = subexp_start_at(subexpr_ptr, i + 1, subexpr_index, 2);
                if (previous_subexp != -1)
                    i = subexpr_ptr[previous_subexp].solve_end + 1;
            }
            else if (is_number(expr[i]))
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
                int total = sizeof(math_function) / sizeof(*math_function);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(expr, function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        subexpr_ptr[subexpr_index].function_ptr = math_function[i];
                        // Setting the start of the subexpression to the start of the function name
                        subexpr_ptr[subexpr_index].expression_start = j;
                        break;
                    }
                }
            }
            else
            {
                // Determining the number of complex functions to check
                int total = sizeof(cmplx_function) / sizeof(*cmplx_function);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(expr, cmplx_function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        subexpr_ptr[subexpr_index].cmplx_function_ptr = cmplx_function[i];
                        subexpr_ptr[subexpr_index].expression_start = j;
                        break;
                    }
                }
            }
        }

        subexpr_ptr[subexpr_index].op_count = op_count;
        // Allocating the array of nodes
        // Case of no operators, create a dummy node and store the value in op1
        if (op_count == 0)
            subexpr_ptr[subexpr_index].node_list = malloc(sizeof(node));
        else
            subexpr_ptr[subexpr_index].node_list = malloc(op_count * sizeof(node));

        node_block = subexpr_ptr[subexpr_index].node_list;

        // Checking if the expression is terminated with an operator
        if (op_count != 0 && operator_index[op_count - 1] == solve_end)
        {
            error_handler(RIGHT_OP_MISSING, 1, 1, operator_index[op_count - 1]);
            delete_math_expr(math_struct);
            free(operator_index);
            return NULL;
        }
        // Filling operations and index data into each node
        for (i = 0; i < op_count; ++i)
        {
            node_block[i].operator_index = operator_index[i];
            node_block[i].operator= expr[operator_index[i]];
        }
        free(operator_index);

        // Case of expression with one term, use one node with operand1 to hold the number
        if (op_count == 0)
        {
            i = subexp_start_at(subexpr_ptr, subexpr_ptr[subexpr_index].solve_start, subexpr_index, 1);
            subexpr_ptr[subexpr_index].node_list[0].var_metadata = 0;
            // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
            if (i != -1)
                *(subexpr_ptr[i].result) = &(node_block->left_operand);
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

            subexpr_ptr[subexpr_index].start_node = 0;
            subexpr_ptr[subexpr_index].result = &(node_block->node_result);
            node_block[0].next = NULL;
            // If the one term expression is the last one, use the math_struct answer
            if (subexpr_index == subexpr_count - 1)
            {
                node_block[0].node_result = &math_struct->answer;
                break;
            }
            continue;
        }
        else
        {
            // Filling each node's priority data
            priority_fill(node_block, op_count);
            // Filling nodes with operands, filling operands index and setting var_metadata to 0
            for (i = 0; i < op_count; ++i)
            {
                node_block[i].node_index = i;
                node_block[i].var_metadata = 0;
            }
        }

        // Trying to read first number into first node

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
                    status = subexp_start_at(subexpr_ptr, solve_start, subexpr_index, 1);
                    if (status == -1)
                    {
                        error_handler(SYNTAX_ERROR, 1, 1, solve_start);
                        delete_math_expr(math_struct);
                        return NULL;
                    }
                    else
                        *(subexpr_ptr[status].result) = &(node_block[0].left_operand);
                }
            }
        }

        // Intermediate nodes, read number to the appropriate node operand
        for (i = 0; i < op_count - 1; ++i)
        {
            if (node_block[i].priority >= node_block[i + 1].priority)
            {
                node_block[i].right_operand = read_value(expr, node_block[i].operator_index + 1, enable_complex);
                // Case of reading the number to the right operand of a node
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
                        status = subexp_start_at(subexpr_ptr, node_block[i].operator_index + 1, subexpr_index, 1);
                        if (status == -1)
                        {
                            error_handler(SYNTAX_ERROR, 1, 1, node_block[i].operator_index + 1);
                            delete_math_expr(math_struct);
                            return NULL;
                        }
                        else
                            *(subexpr_ptr[status].result) = &(node_block[i].right_operand);
                    }
                }
            }
            // Case of the node[i] having less priority than the next node, use next node's left_operand
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
                        status = subexp_start_at(subexpr_ptr, node_block[i].operator_index + 1, subexpr_index, 1);
                        if (status == -1)
                        {
                            error_handler(SYNTAX_ERROR, 1, 1, node_block[i].operator_index + 1);
                            delete_math_expr(math_struct);
                            return NULL;
                        }
                        else
                            *(subexpr_ptr[status].result) = &(node_block[i + 1].left_operand);
                    }
                }
            }
        }
        // Placing the last number in the last node
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
                status = subexp_start_at(subexpr_ptr, node_block[op_count - 1].operator_index + 1, subexpr_index, 1);
                if (status == -1)
                {
                    error_handler(SYNTAX_ERROR, 1, 1, node_block[i].operator_index + 1);
                    return NULL;
                }
                *(subexpr_ptr[status].result) = &(node_block[op_count - 1].right_operand);
            }
        }
        // Set the starting node by searching the first node with the highest priority
        for (i = 3; i > 0; --i)
        {
            for (j = 0; j < op_count; ++j)
            {
                if (node_block[j].priority == i)
                {
                    subexpr_ptr[subexpr_index].start_node = j;
                    // Exiting the main loop by setting i outside continue condition
                    i = -1;
                    break;
                }
            }
        }

        // Setting nodes order of execution
        i = subexpr_ptr[subexpr_index].start_node;
        int target_priority = node_block[i].priority;
        j = i + 1;
        while (target_priority > 0)
        {
            // Running through the nodes to find a node with the target priority
            while (j < op_count)
            {
                if (node_block[j].priority == target_priority)
                {
                    node_block[i].next = node_block + j;
                    // The node found is used as te start node for the next run
                    i = j;
                }
                ++j;
            }
            --target_priority;
            j = 0;
        }
        // Setting the next pointer of the last node to NULL
        node_block[i].next = NULL;

        // Set result pointers for each node based on position and priority
        tmp_node = &(node_block[subexpr_ptr[subexpr_index].start_node]);
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
            // Case of the first node or a node with no left candidate
            if (left_node == -1)
                tmp_node->node_result = &(node_block[right_node].left_operand);
            // Case of a node between 2 nodes
            else if (left_node > -1 && right_node < op_count)
            {
                if (node_block[left_node].priority >= node_block[right_node].priority)
                    tmp_node->node_result = &(node_block[left_node].right_operand);
                else
                    tmp_node->node_result = &(node_block[right_node].left_operand);
            }
            // Case of the last node
            else if (i == op_count - 1)
                tmp_node->node_result = &(node_block[left_node].right_operand);
            tmp_node = tmp_node->next;

            prev_index = i;
            prev_left = left_node;
            prev_right = right_node;
        }
        // Case of the last node in the traversal order, set result to be result of the subexpression
        subexpr_ptr[subexpr_index].result = &(tmp_node->node_result);
        // The last node in the last subexpression should point to math_struct answer
        if (subexpr_index == subexpr_count - 1)
            tmp_node->node_result = &math_struct->answer;
    }
    // Set the variables metadata
    if (enable_variables)
        set_variable_ptr(math_struct);
    return math_struct;
}

void delete_math_expr(math_expr *math_struct)
{
    int i = 0;
    s_expression *subexps = math_struct->subexpr_ptr;
    for (i = 0; i < math_struct->subexpr_count; ++i)
    {
        if (subexps[i].ext_function_ptr != NULL)
            free(subexps[i].result);
        free(subexps[i].node_list);
    }
    free(subexps);
    free(math_struct->variable_ptr);
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
    // If a zero was returned by the factorizing function, nothing to do.
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
            // upon finding the minimun power we subtract it from the power of the num and denum
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
void priority_fill(node *list, int op_count)
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

int subexp_start_at(s_expression *m_expr, int start, int subexpr_i, int mode)
{
    int i;
    i = subexpr_i - 1;
    switch (mode)
    {
    case 1:
        while (i >= 0)
        {
            if (m_expr[i].expression_start == start)
                return i;
            else
                --i;
        }
        break;
    case 2:
        while (i >= 0)
        {
            if (m_expr[i].solve_start == start)
                return i;
            else
                --i;
        }
    }
    return -1;
}
// Function that finds the subexpression that ends at 'end'
int s_exp_ending(s_expression *expression, int end, int current_s_exp, int s_exp_count)
{
    int i;
    i = current_s_exp - 1;
    while (i < s_exp_count)
    {
        if (expression[i].solve_end == end)
            return i;
        else
            --i;
    }
    return -1;
}