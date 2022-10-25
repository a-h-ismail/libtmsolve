/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"
int g_var_count;
char *function_names[] = {"fact", "abs", "ceil", "floor", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log"};
double (*s_function[])(double) = {factorial, fabs, ceil, floor, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, cos, sin, tan, log, log10};
// Extended functions, may take more than one parameter (stored in a comma separated string)
char *ext_function_names[] = {"int", "der"};
double (*ext_s_function[])(char *) = {integral_processor, derivative};

double factorial(double value)
{
    double result = 1, i;
    for (i = 2; i <= value; ++i)
        result *= i;
    return result;
}
int s_process(char *expr, int p)
{
    int a, b, success_count;
    char op;
    double v, v1, v2;
    // find starting index of first operand by checking the position just after the previous operand
    a = find_startofnumber(expr, p - 1);
    if (is_infinite(expr, a))
    {
        error_handler("Infinite number used as first operand.", 1, 1, -1);
        return -1;
    }
    if (is_imaginary(expr, a, p - 1))
    {
        error_handler("Imaginary number not accepted.", 1, 0);
        return -1;
    }
    // find ending index of second operand
    b = find_endofnumber(expr, p + 1);
    if (is_infinite(expr, p + 1))
    {
        error_handler("Infinite number used as second operand.", 1, 1, -1);
        return -1;
    }
    if (is_imaginary(expr, p + 1, b))
    {
        error_handler("Imaginary number not accepted.", 1, 0);
        return -1;
    }
    success_count = sscanf(expr + a, "%lf%c%lf", &v1, &op, &v2);
    // return in case of sscanf failure.
    if (success_count != 3)
    {
        error_handler("Syntax error.", 1, 0);
        return -1;
    }
    // execute operation according to operand
    switch (op)
    {
    case '^':
        // case of -x^n, which is equivalent to -(x^n)
        // The second case (-x)^n is handled by the scientific interpreter
        if (expr[a] == '-')
        {
            v1 = -v1;
            ++a;
        }
        v = pow(v1, v2);
        if (isnan(v))
        {
            error_handler("Invalid power operation in the real domain.", 1, 0);
            return -1;
        }
        break;

    case '*':
        v = v1 * v2;
        break;

    case '/':
        // Obviously no division by zero except if you like crashing the process
        if (v2 != 0)
            v = v1 / v2;
        else
        {
            error_handler("Cannot divide by zero.", 1, 1, -1);
            return -1;
        }
        break;
    case '%':
        v = fmod(v1, v2);
        break;
    case '+':
        v = v1 + v2;
        break;

    case '-':
        v = v1 - v2;
        break;

    default:
        error_handler("Invalid arithmetic operand.", 1, 1, -1);
        return -1;
    }
    // function to print the result of the operation in the place of the operands and trim/extend space accordingly
    b = value_printer(expr, a, b, v);
    // returning the end of the number to the calling function to prevent unnecessary checking for operand.
    return b;
}
double calculate_expr(char *expr)
{
    double result;
    s_expression *subexp;
    // Check for empty input
    if (expr[0] == '\0')
    {
        error_handler("Empty input.", 1, 1, -1);
        return false;
    }
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    combine_add_subtract(expr, 0, strlen(expr) - 1);
    if (parenthesis_check(expr) == false)
        return false;
    if (implicit_multiplication(expr) == false)
        return false;
    subexp = parse_expr(expr, false);
    if (subexp == NULL)
        return NAN;
    result = evaluate(subexp);
    delete_subexps(subexp);
    return result;
}
// Evaluates the expresssion
double evaluate(s_expression *subexps)
{
    node *i_node;
    int current_subexp = 0;
    double result;
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
            i_node = subexps[current_subexp].node_list + subexps[current_subexp].start_node;
            while (i_node != NULL)
            {
                printf("%.8g %c %.8g -> ", i_node->left_operand, i_node->operator, i_node->right_operand);
                i_node = i_node->next;
            }
            printf("Function: %lli", subexps[current_subexp].function_ptr);
            puts("|");
        }
        ++current_subexp;
    } while (subexps[current_subexp - 1].last_subexp == false);
    puts("End");
    fflush(stdout);
#endif
    current_subexp = 0;
    do
    {
        if (subexps[current_subexp].node_list == NULL)
        {
            char *args;
            int length = subexps[current_subexp].solve_end - subexps[current_subexp].solve_start + 1;
            args = malloc((length + 1) * sizeof(char));
            memcpy(args, g_exp + subexps[current_subexp].solve_start, length);
            args[length] = '\0';
            **(subexps[current_subexp].result) = (*(subexps[current_subexp].ext_function_ptr))(args);
            if (isnan(**(subexps[current_subexp].result)))
            {
                error_handler("Math error", 1, 0, subexps[current_subexp].solve_start);
                return NAN;
            }
            ++current_subexp;
            free(args);
            continue;
        }
        i_node = subexps[current_subexp].node_list + subexps[current_subexp].start_node;

        if (subexps[current_subexp].last_subexp == true)
            *(subexps[current_subexp].result) = &result;

        if (subexps[current_subexp].op_count == 0)
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
                        error_handler("Error: Division by zero", 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        error_handler("Error: Modulo zero implies a Division by 0.", 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = fmod(i_node->left_operand, i_node->right_operand);
                    break;

                case '^':
                    *(i_node->node_result) = pow(i_node->left_operand, i_node->right_operand);
                    break;
                }
                i_node = i_node->next;
            }
        // Executing function on the subexpression result
        if (subexps[current_subexp].function_ptr != NULL)
        {
            **(subexps[current_subexp].result) = (*(subexps[current_subexp].function_ptr))(**(subexps[current_subexp].result));
            if (isnan(**(subexps[current_subexp].result)))
            {
                error_handler("Math error", 1, 0, subexps[current_subexp].solve_start);
                return NAN;
            }
        }

        ++current_subexp;
    } while (subexps[current_subexp - 1].last_subexp == false);
    return result;
}
// Sets the variable in x_node to the left or right operand "r","l"
// Returns true if the variable x was found, false otherwise
bool set_variable_in_node(char *exp, node *x_node, char operand)
{
    bool is_negative = false, is_variable = false;
    if (*exp == 'x')
        is_variable = true;
    else if (exp[1] == 'x')
    {
        if (*exp == '+')
            is_variable = true;
        else if (*exp == '-')
            is_variable = is_negative = true;
    }
    // The value is not the variable x
    else
        return false;
    if (is_variable)
    {
        ++g_var_count;
        // x as left op
        if (operand == 'l')
        {
            if (is_negative)
                x_node->variable_operands = x_node->variable_operands | 0b101;
            else
                x_node->variable_operands = x_node->variable_operands | 0b1;
        }
        // x as right op
        else if (operand == 'r')
        {
            if (is_negative)
                x_node->variable_operands = x_node->variable_operands | 0b1010;
            else
                x_node->variable_operands = x_node->variable_operands | 0b10;
        }
        else
        {
            puts("Invaild operand argument.");
            exit(2);
        }
    }
    return true;
}
// Parse a math expression into a s_expression array
s_expression *parse_expr(char *expr, bool enable_variables)
{
    int i, j, length, subexp_count = 1;
    int cur_subexpr;
    int buffer_size = 10, buffer_step = 10;
    length = strlen(expr);
    g_var_count = 0;
    s_expression *subexps_ptr;
    node *current_nodes, *i_node;

    /*
    Parsing steps:
    1- Locate and determine the depth of each subexpression (the whole expression has a depth of 0)
    2- Sort subexpressions by depth (high to low)
    3- Parse subexpressions from high to low depth
    */

    length = strlen(expr);

    subexps_ptr = malloc(buffer_size * sizeof(s_expression));

    int depth = 0;
    cur_subexpr = 0;
    // Determining the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        // Resize the subexp array on the fly
        if (cur_subexpr == buffer_size)
        {
            buffer_size += buffer_step;
            subexps_ptr = realloc(subexps_ptr, buffer_size * sizeof(s_expression));
        }
        if (expr[i] == '(')
        {
            subexps_ptr[cur_subexpr].ext_function_ptr = NULL;
            // Treat extended functions as a subexpression
            for (j = 0; j < sizeof(ext_s_function) / sizeof(*ext_s_function); ++j)
            {
                int temp;
                temp = r_search(expr, ext_function_names[j], i - 1, true);
                if (temp != -1)
                {
                    subexps_ptr[cur_subexpr].expression_start = temp;
                    subexps_ptr[cur_subexpr].solve_start = i + 1;
                    i = find_closing_parenthesis(expr, i);
                    subexps_ptr[cur_subexpr].solve_end = i - 1;
                    subexps_ptr[cur_subexpr].depth = depth + 1;
                    subexps_ptr[cur_subexpr].ext_function_ptr = ext_s_function[j];
                    break;
                }
            }
            if (subexps_ptr[cur_subexpr].ext_function_ptr != NULL)
            {
                ++cur_subexpr;
                continue;
            }
            // Normal case
            ++depth;
            subexps_ptr[cur_subexpr].solve_start = i + 1;
            subexps_ptr[cur_subexpr].depth = depth;
            // The expression start is the parenthesis, may change if a function is found
            subexps_ptr[cur_subexpr].expression_start = i;
            subexps_ptr[cur_subexpr].solve_end = find_closing_parenthesis(expr, i) - 1;
            ++cur_subexpr;
        }
        else if (expr[i] == ')')
            --depth;
    }
    // + 1 for the subexpression with depth 0
    subexp_count = cur_subexpr + 1;
    for (i = 0; i < subexp_count; ++i)
        subexps_ptr[i].function_ptr = NULL;
    // The whole expression "subexpression"
    subexps_ptr[cur_subexpr].depth = 0;
    subexps_ptr[cur_subexpr].solve_start = subexps_ptr[cur_subexpr].expression_start = 0;
    subexps_ptr[cur_subexpr].solve_end = length - 1;
    subexps_ptr[cur_subexpr].ext_function_ptr = NULL;

    // Sort by depth (high to low)
    qsort(subexps_ptr, subexp_count, sizeof(s_expression), compare_subexps_depth);

    int *operator_index, solve_start, solve_end, op_count, status;
    buffer_size = buffer_step = 1000;

    /*
    How to parse a subexp:
    - Set/reset the last_subexp bit (indicates to the solver when to stop and return the final answer)
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
    for (cur_subexpr = 0; cur_subexpr < subexp_count; ++cur_subexpr)
    {
        if (cur_subexpr == subexp_count - 1)
            subexps_ptr[cur_subexpr].last_subexp = true;
        else
            subexps_ptr[cur_subexpr].last_subexp = false;

        if (subexps_ptr[cur_subexpr].ext_function_ptr != NULL)
        {
            subexps_ptr[cur_subexpr].node_list = NULL;
            subexps_ptr[cur_subexpr].result = malloc(sizeof(double **));
            continue;
        }

        // For simplicity
        solve_start = subexps_ptr[cur_subexpr].solve_start;
        solve_end = subexps_ptr[cur_subexpr].solve_end;

        operator_index = (int *)malloc(buffer_size * sizeof(int));
        // Count number of operators and store it's indexes
        for (i = solve_start, op_count = 0; i <= solve_end; ++i)
        {
            // Skipping over an already processed expression
            if (expr[i] == '(')
            {
                int previous_subexp;
                previous_subexp = subexp_start_at(subexps_ptr, i + 1, cur_subexpr, 2);
                if (previous_subexp != -1)
                    i = subexps_ptr[previous_subexp].solve_end + 1;
            }
            else if (is_number(expr[i]))
                continue;

            else if (is_op(*(expr + i)))
            {
                // Skipping a + or - used in scientific notation (like 1e+5)
                if ((expr[i - 1] == 'e' || expr[i - 1] == 'E') && (expr[i] == '+' || expr[i] == '-'))
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
            // Determining the number of functions to check at runtime
            int total = sizeof(s_function) / sizeof(*s_function);
            for (i = 0; i < total; ++i)
            {
                j = r_search(expr, function_names[i], solve_start - 2, true);
                if (j != -1)
                {
                    subexps_ptr[cur_subexpr].function_ptr = s_function[i];
                    // Setting the start of the subexpression to the start of the function name
                    subexps_ptr[cur_subexpr].expression_start = j;
                    break;
                }
            }
        }

        subexps_ptr[cur_subexpr].op_count = op_count;
        // Allocating the array of nodes
        // Case of no operators, create a dummy node and store the value in op1
        if (op_count == 0)
            subexps_ptr[cur_subexpr].node_list = malloc(sizeof(node));
        else
            subexps_ptr[cur_subexpr].node_list = malloc(op_count * sizeof(node));

        current_nodes = subexps_ptr[cur_subexpr].node_list;

        // Checking if the expression is terminated with an operator
        if (op_count != 0 && operator_index[op_count - 1] == solve_end)
        {
            error_handler("Missing right operand", 1, 1, operator_index[op_count - 1]);
            delete_subexps(subexps_ptr);
            return NULL;
        }
        // Filling operations and index data into each node
        for (i = 0; i < op_count; ++i)
        {
            current_nodes[i].operator_index = operator_index[i];
            current_nodes[i].operator= expr[operator_index[i]];
        }
        free(operator_index);

        // Case of expression with one term, use one node with operand1 to hold the number
        if (op_count == 0)
        {
            i = subexp_start_at(subexps_ptr, subexps_ptr[cur_subexpr].solve_start, cur_subexpr, 1);
            subexps_ptr[cur_subexpr].node_list[0].variable_operands = 0;
            // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
            if (i != -1)
                *(subexps_ptr[i].result) = &(current_nodes->left_operand);
            else
            {
                current_nodes->left_operand = read_value(expr, solve_start);
                if (isnan(current_nodes->left_operand))
                {
                    status = set_variable_in_node(expr + solve_start, current_nodes, 'l');
                    if (!status)
                    {
                        error_handler("Syntax error.", 1, 1, solve_start);
                        return NULL;
                    }
                }
            }

            subexps_ptr[cur_subexpr].start_node = 0;
            subexps_ptr[cur_subexpr].result = &(current_nodes->node_result);
            current_nodes[0].next = NULL;
            continue;
        }
        else
        {
            // Filling each node's priority data
            priority_fill(current_nodes, op_count);
            // Filling nodes with operands, filling operands index and setting variable_operands to 0
            for (i = 0; i < op_count; ++i)
            {
                current_nodes[i].node_index = i;
                current_nodes[i].variable_operands = 0;
            }
        }

        // Trying to read first number into first node
        // If a - or + sign is in the beginning of the expression. it will be treated as 0-val or 0+val
        if (expr[solve_start] == '-' || expr[solve_start] == '+')
            current_nodes[0].left_operand = 0;
        else
        {
            current_nodes[0].left_operand = read_value(expr, solve_start);
            if (isnan(current_nodes[0].left_operand))
            {
                // Checking for the variable 'x'
                if (enable_variables == true)
                    status = set_variable_in_node(expr + solve_start, current_nodes, 'l');
                else
                    status = false;
                // Variable x not found, try to read number
                if (!status)
                {
                    status = subexp_start_at(subexps_ptr, solve_start, cur_subexpr, 1);
                    if (status == -1)
                    {
                        error_handler("Syntax error.", 1, 1, solve_start);
                        return NULL;
                    }
                    else
                        *(subexps_ptr[status].result) = &(current_nodes[0].left_operand);
                }
            }
        }

        // Intermediate nodes, read number to the appropriate node operand
        for (i = 0; i < op_count - 1; ++i)
        {
            if (current_nodes[i].priority >= current_nodes[i + 1].priority)
            {
                current_nodes[i].right_operand = read_value(expr, current_nodes[i].operator_index + 1);
                // Case of reading the number to the right operand of a node
                if (isnan(current_nodes[i].right_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_in_node(expr + current_nodes[i].operator_index + 1, current_nodes + i, 'r');
                    else
                        status = false;
                    if (!status)
                    {
                        // Case of a subexpression result as right_operand, set result pointer to right_operand
                        status = subexp_start_at(subexps_ptr, current_nodes[i].operator_index + 1, cur_subexpr, 1);
                        if (status == -1)
                        {
                            error_handler("Syntax error.", 1, 1, current_nodes[i].operator_index + 1);
                            return NULL;
                        }
                        else
                            *(subexps_ptr[status].result) = &(current_nodes[i].right_operand);
                    }
                }
            }
            // Case of the node[i] having less priority than the next node, use next node's left_operand
            else
            {
                // Read number
                current_nodes[i + 1].left_operand = read_value(expr, current_nodes[i].operator_index + 1);
                if (isnan(current_nodes[i + 1].left_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_in_node(expr + current_nodes[i].operator_index + 1, current_nodes + i + 1, 'l');
                    else
                        status = false;
                    if (!status)
                    {
                        status = subexp_start_at(subexps_ptr, current_nodes[i].operator_index + 1, cur_subexpr, 1);
                        if (status == -1)
                        {
                            error_handler("Syntax error.", 1, 1, current_nodes[i].operator_index + 1);
                            return NULL;
                        }
                        else
                            *(subexps_ptr[status].result) = &(current_nodes[i + 1].left_operand);
                    }
                }
            }
        }
        // Placing the last number in the last node
        current_nodes[op_count - 1].right_operand = read_value(expr, current_nodes[op_count - 1].operator_index + 1);
        if (isnan(current_nodes[op_count - 1].right_operand))
        {
            // Checking for the variable 'x'
            if (enable_variables == true)
                status = set_variable_in_node(expr + current_nodes[op_count - 1].operator_index + 1, current_nodes + op_count - 1, 'r');
            else
                status = false;
            if (!status)
            {
                // If an expression is the last term, find it and set the pointers
                status = subexp_start_at(subexps_ptr, current_nodes[op_count - 1].operator_index + 1, cur_subexpr, 1);
                if (status == -1)
                {
                    error_handler("Syntax error.", 1, 1, current_nodes[i].operator_index + 1);
                    return NULL;
                }
                *(subexps_ptr[status].result) = &(current_nodes[op_count - 1].right_operand);
            }
        }
        // Set the starting node by searching the first node with the highest priority
        for (i = 3; i > 0; --i)
        {
            for (j = 0; j < op_count; ++j)
            {
                if (current_nodes[j].priority == i)
                {
                    subexps_ptr[cur_subexpr].start_node = j;
                    // Making the main loop exit by setting i outside continue condition
                    i = -1;
                    break;
                }
            }
        }

        // Setting nodes order of execution
        i = subexps_ptr[cur_subexpr].start_node;
        int target_priority = current_nodes[i].priority;
        j = i + 1;
        while (target_priority > 0)
        {
            // Running through the nodes to find a node with the target priority
            while (j < op_count)
            {
                if (current_nodes[j].priority == target_priority)
                {
                    current_nodes[i].next = current_nodes + j;
                    // The node found is used as te start node for the next run
                    i = j;
                }
                ++j;
            }
            --target_priority;
            j = 0;
        }
        // Setting the next pointer of the last node to NULL
        current_nodes[i].next = NULL;

        // Set result pointers for each node based on position and priority
        i_node = &(current_nodes[subexps_ptr[cur_subexpr].start_node]);
        int l_node, r_node;
        while (i_node->next != NULL)
        {
            i = i_node->node_index;
            // Finding the previous and next nodes to compare
            l_node = i - 1, r_node = i + 1;

            // Optimizing contiguous block of same priority by using previous results (TODO)
            while (l_node != -1)
            {
                if (i_node->priority <= current_nodes[l_node].priority)
                    --l_node;
                else
                    break;
            }

            while (r_node < op_count)
            {
                if (i_node->priority < current_nodes[r_node].priority)
                    ++r_node;
                else
                    break;
            }
            // Case of the first node or a node with no left candidate
            if (l_node == -1)
                i_node->node_result = &(current_nodes[r_node].left_operand);
            // Case of a node between 2 nodes
            else if (l_node > -1 && r_node < op_count)
            {
                if (current_nodes[l_node].priority >= current_nodes[r_node].priority)
                    i_node->node_result = &(current_nodes[l_node].right_operand);
                else
                    i_node->node_result = &(current_nodes[r_node].left_operand);
            }
            // Case of the last node
            else if (i == op_count - 1)
                i_node->node_result = &(current_nodes[l_node].right_operand);
            i_node = i_node->next;
        }
        // Case of the last node in the traversal order, set result to be result of the subexpression
        subexps_ptr[cur_subexpr].result = &(i_node->node_result);
    }

    return subexps_ptr;
}
// Free a subexpressions array and its contents
void delete_subexps(s_expression *subexps)
{
    int i = 0, count;
    while (subexps[i].last_subexp == false)
        ++i;
    count = i + 1;
    for (i = 0; i < count; ++i)
    {
        if (subexps[i].ext_function_ptr != NULL)
            free(subexps[i].result);
        free(subexps[i].node_list);
    }
    free(subexps);
}
// Evaluate only the region of expr between a and b and return the answer.
double evaluate_region(char *expr, int a, int b)
{
    char *temp = (char *)malloc(23 * (b - a + 2) * sizeof(char));
    double result;
    temp[b - a + 1] = '\0';
    strncpy(temp, expr + a, b - a + 1);
    result = calculate_expr(temp);
    if (isnan(result))
        return NAN;
    free(temp);
    return result;
}

// Function that factorizes a 32 bit number, returns factors in a heap allocated array
// | factor0 | occurences_of_factor0 | factor1 | occurences_of_factor1 |
int32_t *find_factors(int32_t number)
{
    int32_t dividend = 2, *factors;
    int i = 0;
    // factors are stored in an 64 element array
    //*(factors+2*x) -> factors
    //*(factors+2*x+1) -> number of occurences of the factor *(factors+2*x)
    factors = (int32_t *)calloc(64, sizeof(int));
    if (number == 0)
        return factors;
    *(factors) = 1;
    *(factors + 1) = 1;
    // Turning negative numbers into positive for factorization to work
    if (number < 0)
        number = -number;
    // Simple factorization algorithm
    while (number != 1)
    {
        if (number % dividend == 0)
        {
            ++i;
            number = number / dividend;
            *(factors + (i * 2)) = dividend;
            *(factors + (i * 2) + 1) = 1;
            while (number % dividend == 0 && number >= dividend)
            {
                number = number / dividend;
                ++*(factors + (i * 2) + 1);
            }
        }
        else
        {
            // Optimize by skipping numbers greater than number/2 (in other terms, number itself is number prime)
            if (dividend > number / 2 && number != 1)
            {
                ++i;
                *(factors + (i * 2)) = number;
                *(factors + (i * 2) + 1) = 1;
                break;
            }
        }
        // Optimizing factorization by skipping even numbers (> 2)
        if (dividend > 2)
            dividend = dividend + 2;
        else
            ++dividend;
    }
    if (i == 0)
    {
        *(factors + 2) = number;
        *(factors + 3) = 1;
    }
    return factors;
}

void fraction_reducer(int32_t *num_factors, int32_t *denom_factors, int32_t *numerator, int32_t *denominator)
{
    int i = 1, j = 1, min;
    while (*(num_factors + 2 * i) != 0 && *(denom_factors + 2 * j) != 0)
    {
        if (*(num_factors + 2 * i) == *(denom_factors + 2 * j))
        {
            // upon finding the minimun power we subtract it from the power of the num and denum
            min = find_min(*(num_factors + (2 * i) + 1), *(denom_factors + (2 * j) + 1));
            *(num_factors + (2 * i) + 1) -= min;
            *(denom_factors + (2 * j) + 1) -= min;
            ++i;
            ++j;
        }
        else
        {
            // If the factor at i is smaller increment i , otherwise increment j
            if (*(num_factors + (2 * i)) < *(denom_factors + (2 * j)))
                ++i;
            else
                ++j;
        }
    }
    *numerator = 1;
    *denominator = 1;
    // Calculate numerator and denominator after reduction
    for (i = 1; *(num_factors + (2 * i)) != 0; ++i)
    {
        if (*(num_factors + (2 * i) + 1) != 0)
            *numerator *= pow(*(num_factors + (2 * i)), *(num_factors + (2 * i) + 1));
    }
    for (i = 1; *(denom_factors + (2 * i)) != 0; ++i)
    {
        if (*(denom_factors + (2 * i) + 1) != 0)
            *denominator *= pow(*(denom_factors + (2 * i)), *(denom_factors + (2 * i) + 1));
    }
}

void fraction_processor(char *exp, bool wasdecimal)
{
    int success_count;
    int32_t v1, v2, v, r;
    double check1, check2;
    success_count = sscanf(exp, "%lf/%lf", &check1, &check2);
    if (success_count != 2)
    {
        puts("Syntax error.");
        return;
    }
    // Given that fraction reducer operates on positive numbers, abs(int32_t_MIN)>2147483647 can't be accepted
    if (fabs(check1) > 2147483647 || fabs(check2) > 2147483647)
    {
        puts("Values must be in range [-2147483647,2147483647]");
        return;
    }
    v1 = check1;
    v2 = check2;
    v = v1 / v2;
    r = v1 % v2;
    if (r == 0)
    {
        printf("%s = %" PRId32 "", exp, v);
        if (wasdecimal == false)
            printf("\n");
        return;
    }
    if ((r != 0 || v1 < v2) && v1 != 0)
    {
        int32_t *f_v1, *f_v2;
        // flip signs of v1 and v2 if both are negative or if v2 only is negative to move the - to v1
        if ((v1 < 0 && v2 < 0) || (v1 > 0 && v2 < 0))
        {
            v1 = -v1;
            v2 = -v2;
        }
        if (v1 > v2)
            v1 = v1 % v2;
        if (v1 < 0)
        {
            v1 = -v1;
            v1 = v1 % v2;
            v1 = -v1;
        }
        f_v1 = find_factors(v1);
        f_v2 = find_factors(v2);
        if (v1 < 0)
        {
            v1 = -v1;
            fraction_reducer(f_v1, f_v2, &v1, &v2);
            v1 = -v1;
        }
        else
            fraction_reducer(f_v1, f_v2, &v1, &v2);
        if (v != 0)
        {
            if (v1 >= 0)
                printf("= %" PRId32 "/%" PRId32 " = %" PRId32 "+%" PRId32 "/%" PRId32 "", (v * v2 + v1), v2, v, v1, v2);
            else
                printf("= %" PRId32 "/%" PRId32 " = %" PRId32 "%" PRId32 "/%" PRId32 "", (v * v2 + v1), v2, v, v1, v2);
        }
        else
            printf("= %" PRId32 "/%" PRId32 "", v1, v2);
        if (wasdecimal == false)
        {
            s_process(exp, nextop(exp, 1));
            printf(" = %s\n", exp);
        }
        // Remember to not leak memory
        free(f_v1);
        free(f_v2);
    }
}

bool decimal_to_fraction(char *exp, bool inverse_process)
{
    int p = 0, i, j, fractionl;
    int32_t v1, v2 = 0;
    char pattern[14];
    double v = 0;
    bool success = false;
    // we read the value and rewrite it in case of scientific notation
    // ex: 1e-7 -> rewrite to 0.0000001
    sscanf(exp, "%lf", &v);
    if (fabs(v) > 2147483647 || v - (int32_t)v == 0 || fabs(v) < 1e-8)
        return false;
    if (fabs(v) < 1e-4)
    {
        sprintf(exp, "%.16lf", v);
        // Removing trailing zeros
        for (i = strlen(exp) - 1; i > p; --i)
        {
            // Stop at the first non zero value and null terminate
            if (*(exp + i) != '0')
            {
                *(exp + i + 1) = '\0';
                break;
            }
        }
    }
    for (i = 0; i < strlen(exp); ++i)
        if (*(exp + i) == '.')
        {
            p = i;
            break;
        }
    // return if no point is found
    if (p == 0)
        return false;
    fractionl = strlen(exp) - p - 1;
    if (fractionl >= 10)
    {
        // Look for a pattern to detect fractions from periodic decimals
        for (i = p + 1; i - p < fractionl / 2; ++i)
        {
            pattern[0] = *(exp + i);
            pattern[1] = '\0';
            j = i + 1;
            while (true)
            {
                // First case: the "pattern" is smaller than the remaining decimal digits.
                if (strlen(exp) - j > strlen(pattern))
                {
                    // If the pattern is found again in the remaining decimal digits, jump over it.
                    if (strncmp(pattern, exp + j, strlen(pattern)) == 0)
                        j += strlen(pattern);
                    // If not, copy the digits between i and j to the pattern for further testing
                    else
                    {
                        strncpy(pattern, exp + i, j - i + 1);
                        pattern[j - i + 1] = '\0';
                        ++j;
                    }
                    // If the pattern matches the last decimal digits, stop the execution with SUCCESS
                    if (strlen(exp) - j == strlen(pattern) && strncmp(pattern, exp + strlen(exp) + j, strlen(pattern) == 0))
                    {
                        success = true;
                        break;
                    }
                }
                // Second case: the pattern is bigger than the remaining digits.
                else
                {
                    // Consider a SUCCESS the case where the remaining digits except the last one match the leftmost digits of the pattern
                    if (strncmp(pattern, exp + j, strlen(exp) - j - 1) == 0)
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
        // getting the fraction from the "pattern"
        if (success == true)
        {
            int i2;
            if (strlen(pattern) + i - p - 1 < 8)
            {
                for (i2 = 0; i2 < strlen(pattern); ++i2)
                    v2 += 9 * pow(10, i2);
                v2 = v2 * pow(10, i - p - 1);
            }
            else
                return false;
            sscanf(pattern, "%" PRId32 " ", &v1);
            // i - p - 1 is the number of decimal digits between the period and the point
            v1 = v1 + (int32_t)(v * pow(10, i - p - 1)) * (v2 / pow(10, i - p - 1));
            sprintf(exp, "%" PRId32 "/%" PRId32 "", v1, v2);
            if (inverse_process == false)
            {
                // The ~ indicates that the fraction does not equal the decimal, but it is very close to it
                printf("~");
                fraction_processor(exp, true);
            }
            return true;
        }
    }
    else if (strlen(exp) - 1 < 10)
    {
        // Prevent integer overflow by returning if the value resulting will have more than 9 digits
        v2 = pow(10, fractionl);
        v1 = v * v2;
        sprintf(exp, "%" PRId32 "/%" PRId32 "", v1, v2);
        if (inverse_process == false)
        {
            fraction_processor(exp, true);
        }
        return true;
    }
    // trying to get the fraction by inverting it then trying the algorithm again
    if (inverse_process == false && fractionl > 10)
    {
        double inverse = 1 / v;
        char temp[22];
        // case where the fraction is likely of form 1/x
        if (fabs(inverse - round(inverse)) < 1e-7)
        {
            if (fabs(round(inverse)) == 1)
                printf("~=1");
            else
                printf("~=1/%" PRId32 "", (int32_t)round(inverse));
            return true;
        }
        sprintf(temp, "%.12lf", inverse);
        if (decimal_to_fraction(temp, true) == false)
            return false;
        else
        {
            sscanf(temp, "%" PRId32 "/%" PRId32 "", &v2, &v1);
            sprintf(exp, "%" PRId32 "/%" PRId32 "", v1, v2);
            printf("~");
            fraction_processor(exp, true);
            return true;
        }
    }
    return false;
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
// Function that finds the subexpression that starts from 'start'
// Mode determines if the value to return is expression_start (1) or solve_start (2)
int subexp_start_at(s_expression *expression, int start, int current_s_exp, int mode)
{
    int i;
    i = current_s_exp - 1;
    switch (mode)
    {
    case 1:
        while (i >= 0)
        {
            if (expression[i].expression_start == start)
                return i;
            else
                --i;
        }
        break;
    case 2:
        while (i >= 0)
        {
            if (expression[i].solve_start == start)
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