/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"
int g_nb_unknowns;
double factorial(double value)
{
    double result = 1, i;
    for (i = 2; i <= value; ++i)
        result *= i;
    return result;
}
int s_process(char *exp, int p)
{
    int a, b, success_count;
    char op;
    double v, v1, v2;
    // find starting index of first operand by checking the position just after the previous operand
    a = find_startofnumber(exp, p - 1);
    if (is_infinite(exp, a))
    {
        error_handler("Infinite number used as first operand.", 1, 1, -1);
        return -1;
    }
    if (is_imaginary(exp, a, p - 1))
    {
        error_handler("Imaginary number not accepted.", 1, 0);
        return -1;
    }
    // find ending index of second operand
    b = find_endofnumber(exp, p + 1);
    if (is_infinite(exp, p + 1))
    {
        error_handler("Infinite number used as second operand.", 1, 1, -1);
        return -1;
    }
    if (is_imaginary(exp, p + 1, b))
    {
        error_handler("Imaginary number not accepted.", 1, 0);
        return -1;
    }
    success_count = sscanf(exp + a, "%lf%c%lf", &v1, &op, &v2);
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
        if (exp[a] == '-')
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
    b = value_printer(exp, a, b, v);
    // returning the end of the number to the calling function to prevent unnecessary checking for operand.
    return b;
}
// Function that checks syntax, runs implied multiplication, replace variables and solve the expression
double pre_scientific_interpreter(char *exp)
{
    if (parenthesis_check(exp) == false)
        return false;
    if (implicit_multiplication(exp) == false)
        return false;
    // variable_matcher(exp);
    return scientific_interpreter(exp, false);
}
double scientific_interpreter(char *exp, bool int_der)
{
    int i = 0;
    bool success;
    double result;
    s_expression **subexp;
    // Check for empty input
    if (exp[0] == '\0')
    {
        error_handler("Empty input.", 1, 1, -1);
        return false;
    }
    if (int_der == false)
    {
        // Run integration and derivation algorithm
        success = integral_processor(exp);
        if (success == false)
            return NAN;
        // Scan for derivation sign d/dx
        while (i != -1)
        {
            i = s_search(exp, "d/dx", i);
            if (i != -1)
                if (derivative(exp + i) == false)
                    return NAN;
        }
    }
    subexp = scientific_compiler(exp);
    if (subexp == NULL)
        return NAN;
    result = solve_s_exp(subexp);
    delete_s_exp(subexp);
    return result;
}
// Solve a compiled list of subexpressions and return the final result
double solve_s_exp(s_expression **subexps)
{
    node *i_node;
    int current_subexp = 0;
    double result;
#ifdef DEBUG
    // Dumping node data
    do
    {
        i_node = subexps[current_subexp]->node_list + subexps[current_subexp]->start_node;
        while (i_node != NULL)
        {
            printf("%.8g %c %.8g -> ", i_node->LeftOperand, i_node->operator, i_node->RightOperand);
            i_node = i_node->next;
        }
        printf("Function: %lli", subexps[current_subexp]->function_ptr);
        puts("|");
        ++current_subexp;
    } while (subexps[current_subexp - 1]->last_exp == false);
    puts("End");
    fflush(stdout);
#endif
    current_subexp = 0;
    do
    {
        i_node = subexps[current_subexp]->node_list + subexps[current_subexp]->start_node;

        if (subexps[current_subexp]->last_exp == true)
            *(subexps[current_subexp]->result) = &result;

        if (subexps[current_subexp]->op_count == 0)
            *(i_node->result) = i_node->LeftOperand;
        else
            while (i_node != NULL)
            {
                switch (i_node->operator)
                {
                case '+':
                    *(i_node->result) = i_node->LeftOperand + i_node->RightOperand;
                    break;

                case '-':
                    *(i_node->result) = i_node->LeftOperand - i_node->RightOperand;
                    break;

                case '*':
                    *(i_node->result) = i_node->LeftOperand * i_node->RightOperand;
                    break;

                case '/':
                    if (i_node->RightOperand == 0)
                    {
                        error_handler("Error: Division by zero", 1, 1, i_node->index);
                        return NAN;
                    }
                    *(i_node->result) = i_node->LeftOperand / i_node->RightOperand;
                    break;

                case '%':
                    if (i_node->RightOperand == 0)
                    {
                        error_handler("Error: Modulo zero implies a Division by 0.", 1, 1, i_node->index);
                        return NAN;
                    }
                    *(i_node->result) = fmod(i_node->LeftOperand, i_node->RightOperand);
                    break;

                case '^':
                    *(i_node->result) = pow(i_node->LeftOperand, i_node->RightOperand);
                    break;
                }
                i_node = i_node->next;
            }
        // Executing function on the subexpression result
        if (subexps[current_subexp]->function_ptr != NULL)
        {
            **(subexps[current_subexp]->result) = (*(subexps[current_subexp]->function_ptr))(**(subexps[current_subexp]->result));
            if (isnan(**(subexps[current_subexp]->result)))
            {
                error_handler("Math error", 1, 0, subexps[current_subexp]->solve_start);
                return NAN;
            }
        }

        ++current_subexp;
    } while (subexps[current_subexp - 1]->last_exp == false);
    return result;
}
// Sets the unknown in the node x_node in the left or right operand "r","l"
bool set_unknown(char *exp, node *x_node, char operand)
{
    bool isNegative = false, isUnknown = false;
    if (*exp == 'x')
        isUnknown = true;
    else if (exp[1] == 'x')
    {
        if (*exp == '+')
            isUnknown = true;
        else if (*exp == '-')
            isUnknown = isNegative = true;
    }
    // The value is not the unknown x
    else
        return false;
    if (isUnknown)
    {
        ++g_nb_unknowns;
        // x as left op
        if (operand == 'l')
        {
            if (isNegative)
                x_node->variable_operands = x_node->variable_operands | 0b101;
            else
                x_node->variable_operands = x_node->variable_operands | 0b1;
        }
        // x as right op
        else if (operand == 'r')
        {
            if (isNegative)
                x_node->variable_operands = x_node->variable_operands | 0b1010;
            else
                x_node->variable_operands = x_node->variable_operands | 0b10;
        }
        else
        {
            // In case an invalid operand symbol is received, you probably messed up badly
            puts("This error should not appear unless you screwed up badly.");
            exit(2);
        }
    }
    return true;
}
s_expression **scientific_compiler(char *exp)
{
    int length, solve_start, solve_end, i, j, op_count, status, subexp_count = 1;
    int current_subexp, *operator_indexes, buffer_size = 10000;
    length = strlen(exp);
    g_nb_unknowns = 0;
    s_expression **subexps_ptr;
    node *current_nodes, *i_node;
    char *function_names[] = {"fact", "abs", "ceil", "floor", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log"};
    double (*s_function[])(double) = {factorial, fabs, ceil, floor, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, cos, sin, tan, log, log10};

    /*
    Compiling steps:
    1- Locate and determine the depth of each subexpression (the whole expression has a depth of 0)
    2- Sort subexpressions by depth (high to low)
    3- Parse subexpressions from high to low priority
    */

    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    combine_add_subtract(exp, 0, length - 1);
    length = strlen(exp);

    // Counting the number of subexpressions
    for (i = 0; i < length; ++i)
    {
        if (exp[i] == '(')
            ++subexp_count;
    }
    // Allocate subexps array of pointers
    subexps_ptr = malloc(subexp_count * sizeof(s_expression *));
    // Allocating memory for each subexp and initialize function pointer
    for (i = 0; i < subexp_count; ++i)
    {
        subexps_ptr[i] = malloc(sizeof(s_expression));
        subexps_ptr[i]->function_ptr = NULL;
    }

    int depth = 0;
    current_subexp = 0;
    // Determining the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length - 1; ++i)
    {
        if (exp[i] == '(')
        {
            ++depth;
            subexps_ptr[current_subexp]->depth = depth;
            subexps_ptr[current_subexp]->solve_start = i + 1;
            // The expression start is the parenthesis, may change if a function is found
            subexps_ptr[current_subexp]->expression_start = i;
            subexps_ptr[current_subexp]->end_index = find_closing_parenthesis(exp, i);
            ++current_subexp;
        }
        else if (exp[i] == ')')
            --depth;
    }
    // The whole expression "subexpression"
    subexps_ptr[current_subexp]->depth = 0;
    subexps_ptr[current_subexp]->solve_start = subexps_ptr[current_subexp]->expression_start = 0;
    subexps_ptr[current_subexp]->end_index = length - 1;

    // Sort by depth (high to low)
    qsort(subexps_ptr, subexp_count, sizeof(s_expression *), compare_subexps_depth);

    // Parse each subexpression
    for (current_subexp = 0; current_subexp < subexp_count; ++current_subexp)
    {
        // Getting start/end indexes of the subexpressions
        solve_start = subexps_ptr[current_subexp]->solve_start;
        solve_end = subexps_ptr[current_subexp]->end_index;
        operator_indexes = (int *)malloc(buffer_size * sizeof(int));
        // Count number of operators and store it's indexes
        for (i = solve_start, op_count = 0; i <= solve_end; ++i)
        {
            // Skipping over an already processed expression
            if (exp[i] == '(')
            {
                int previous_subexp;
                previous_subexp = subexp_start_at(subexps_ptr, i + 1, current_subexp, 2);
                i = subexps_ptr[previous_subexp]->end_index;
            }
            else if (is_number(exp[i]))
                continue;
            // An operator was found
            else if (is_op(*(exp + i)))
            {
                // Skipping a + or - used in scientific notation (like 1e+5)
                if ((exp[i - 1] == 'e' || exp[i - 1] == 'E') && (exp[i] == '+' || exp[i] == '-'))
                {
                    ++i;
                    continue;
                }
                // Varying the array size on demand
                if (op_count == buffer_size)
                {
                    buffer_size += 10000;
                    operator_indexes = realloc(operator_indexes, buffer_size * sizeof(int));
                }
                operator_indexes[op_count] = i;
                // Skipping a + or - coming just after an operator
                if (exp[i + 1] == '-' || exp[i + 1] == '+')
                    ++i;
                ++op_count;
            }
        }

        // Searching for any function preceding the expression to set the function pointer
        if (solve_start > 1)
        {
            for (i = 0; i < 18; ++i)
            {
                j = r_search(exp, function_names[i], solve_start - 2, true);
                if (j != -1)
                {
                    subexps_ptr[current_subexp]->function_ptr = s_function[i];
                    // Setting the start of the subexpression to the start of the function name
                    subexps_ptr[current_subexp]->expression_start = j;
                    break;
                }
            }
        }

        subexps_ptr[current_subexp]->op_count = op_count;
        // Allocating the array of nodes
        // Case of no operators, create a dummy node and store the value in op1
        if (op_count == 0)
            subexps_ptr[current_subexp]->node_list = malloc(sizeof(node));
        else
            subexps_ptr[current_subexp]->node_list = malloc(op_count * sizeof(node));

        // Setting last_exp bit
        if (current_subexp == subexp_count - 1)
            subexps_ptr[current_subexp]->last_exp = true;
        else
            subexps_ptr[current_subexp]->last_exp = false;
        current_nodes = subexps_ptr[current_subexp]->node_list;

        // Checking if the expression is termianted with an operator
        if (op_count != 0 && operator_indexes[op_count - 1] == solve_end)
        {
            error_handler("Missing right operand", 1, 1, operator_indexes[op_count - 1]);
            delete_s_exp(subexps_ptr);
            return NULL;
        }
        // Filling operations and index data into each node
        for (i = 0; i < op_count; ++i)
        {
            current_nodes[i].index = operator_indexes[i];
            current_nodes[i].operator= exp[operator_indexes[i]];
        }
        free(operator_indexes);

        // Case of expression with one term, use one node with operand1 to hold the number
        if (op_count == 0)
        {

            i = subexp_start_at(subexps_ptr, subexps_ptr[current_subexp]->solve_start, current_subexp, 1);
            // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
            if (i != -1)
                *(subexps_ptr[i]->result) = &(current_nodes->LeftOperand);
            else
            {
                current_nodes->LeftOperand = read_value(exp, solve_start);
                if (isnan(current_nodes->LeftOperand))
                {
                    status = set_unknown(exp + solve_start, current_nodes, 'l');
                    if (!status)
                    {
                        error_handler("Syntax error.", 1, 1, solve_start);
                        return NULL;
                    }
                }
            }

            subexps_ptr[current_subexp]->start_node = 0;
            subexps_ptr[current_subexp]->result = &(current_nodes->result);
            current_nodes[0].next = NULL;
            continue;
        }
        else
        {
            // Filling each node's priority data
            priority_fill(current_nodes, op_count);
            // Filling nodes with operands, filling operands n_index (index of the node in the array) and setting variable_operands to 0
            for (i = 0; i < op_count; ++i)
            {
                current_nodes[i].node_index = i;
                current_nodes[i].variable_operands = 0;
            }
        }

        // Trying to read first number into first node
        // If a -/+ sign is in the beginning of the expression. it will be treated as 0-/+x
        if (exp[solve_start] == '-' || exp[solve_start] == '+')
            current_nodes[0].LeftOperand = 0;
        else
        {
            current_nodes[0].LeftOperand = read_value(exp, solve_start);
            if (isnan(current_nodes[0].LeftOperand))
            {
                // Checking for the unknown 'x'
                status = set_unknown(exp + solve_start, current_nodes, 'l');
                if (!status)
                {
                    status = subexp_start_at(subexps_ptr, solve_start, current_subexp, 1);
                    if (status == -1)
                    {
                        error_handler("Syntax error.", 1, 1, solve_start);
                        return NULL;
                    }
                    else
                        *(subexps_ptr[status]->result) = &(current_nodes[0].LeftOperand);
                }
            }
        }

        // Intermediate nodes
        for (i = 0; i < op_count - 1; ++i)
        {
            if (current_nodes[i].priority >= current_nodes[i + 1].priority)
            {
                current_nodes[i].RightOperand = read_value(exp, current_nodes[i].index + 1);
                // Case of reading the number to operand2 of a node
                if (isnan(current_nodes[i].RightOperand))
                {
                    // Checking for the unknown 'x'
                    status = set_unknown(exp + current_nodes[i].index + 1, current_nodes + i, 'r');
                    if (!status)
                    {
                        // Case of a subexpression result as operand2, set result pointer to operand2
                        status = subexp_start_at(subexps_ptr, current_nodes[i].index + 1, current_subexp, 1);
                        if (status == -1)
                        {
                            error_handler("Syntax error.", 1, 1, current_nodes[i].index + 1);
                            return NULL;
                        }
                        else
                            *(subexps_ptr[status]->result) = &(current_nodes[i].RightOperand);
                    }
                }
            }
            // Case of the node[i] having less priority than the next node, use next node's operand1
            else
            {
                // Read number
                current_nodes[i + 1].LeftOperand = read_value(exp, current_nodes[i].index + 1);
                if (isnan(current_nodes[i + 1].LeftOperand))
                {
                    // Checking for the unknown 'x'
                    status = set_unknown(exp + current_nodes[i].index + 1, current_nodes + i + 1, 'l');
                    if (!status)
                    {
                        status = subexp_start_at(subexps_ptr, current_nodes[i].index + 1, current_subexp, 1);
                        if (status == -1)
                        {
                            error_handler("Syntax error.", 1, 1, current_nodes[i].index + 1);
                            return NULL;
                        }
                        else
                            *(subexps_ptr[status]->result) = &(current_nodes[i + 1].LeftOperand);
                    }
                }
            }
        }
        // Placing the last number in the last node
        current_nodes[op_count - 1].RightOperand = read_value(exp, current_nodes[op_count - 1].index + 1);
        if (isnan(current_nodes[op_count - 1].RightOperand))
        {
            // Checking for the unknown 'x'
            status = set_unknown(exp + current_nodes[op_count - 1].index + 1, current_nodes + op_count - 1, 'r');
            if (!status)
            {
                // If an expression is the last term, find it and set the pointers
                status = subexp_start_at(subexps_ptr, current_nodes[op_count - 1].index + 1, current_subexp, 1);
                if (status == -1)
                {
                    error_handler("Syntax error.", 1, 1, current_nodes[i].index + 1);
                    return NULL;
                }
                *(subexps_ptr[status]->result) = &(current_nodes[op_count - 1].RightOperand);
            }
        }
        // Set the starting node by searching the first node with the highest priority
        for (i = 3; i > 0; --i)
        {
            for (j = 0; j < op_count; ++j)
            {
                if (current_nodes[j].priority == i)
                {
                    subexps_ptr[current_subexp]->start_node = j;
                    // Making the main loop exit by setting i outside continue condition
                    i = -1;
                    break;
                }
            }
        }

        // Setting nodes order of execution
        // Preparing start node
        i = subexps_ptr[current_subexp]->start_node;
        // Setting target priority
        int next_priority = current_nodes[i].priority;
        j = i + 1;
        while (next_priority > 0)
        {
            // Running through the nodes to find a node with the target priority
            while (j < op_count)
            {
                if (current_nodes[j].priority == next_priority)
                {
                    current_nodes[i].next = current_nodes + j;
                    // The node found is then used as te start node for the next run
                    i = j;
                }
                ++j;
            }
            // On the end of each search, decrement target priority and start from the beginning
            --next_priority;
            j = 0;
        }
        // Setting the next pointer of the last node to NULL
        current_nodes[i].next = NULL;

        // Creating the necessary result redirections between nodes
        // Setting the starting node
        i_node = &(current_nodes[subexps_ptr[current_subexp]->start_node]);
        int l_node, r_node, prev_l_node = -2, prev_r_node = -2;
        while (i_node->next != NULL)
        {
            i = i_node->node_index;
            // Finding the previous and next nodes to compare
            l_node = i - 1, r_node = i + 1;

            // Optimizing contiguous block of same priority by using previous results
            if (l_node > -1 && i_node->priority <= current_nodes[l_node].priority && prev_l_node != -2)
                l_node = prev_l_node;
            else
            {
                while (l_node != -1)
                {
                    if (i_node->priority <= current_nodes[l_node].priority)
                        --l_node;
                    else
                        break;
                }
            }
            if (r_node < op_count && i_node->priority < current_nodes[r_node].priority && prev_r_node != -2)
                r_node = prev_r_node;
            else
            {
                while (r_node < op_count)
                {
                    if (i_node->priority < current_nodes[r_node].priority)
                        ++r_node;
                    else
                        break;
                }
            }
            prev_l_node = l_node;
            prev_r_node = r_node;
            // Case of the first node or a node with no left candidate
            if (l_node == -1)
                i_node->result = &(current_nodes[r_node].LeftOperand);
            // Case of a node between 2 nodes
            else if (l_node > -1 && r_node < op_count)
            {
                if (current_nodes[l_node].priority >= current_nodes[r_node].priority)
                    i_node->result = &(current_nodes[l_node].RightOperand);
                else
                    i_node->result = &(current_nodes[r_node].LeftOperand);
            }
            // Case of the last node
            else if (i == op_count - 1)
                i_node->result = &(current_nodes[l_node].RightOperand);
            i_node = i_node->next;
        }
        // Case of the last node in the traversal order, set result to be result of the subexpression
        subexps_ptr[current_subexp]->result = &(i_node->result);
    }

    return subexps_ptr;
}
// Free a subexpressions array and its contents
void delete_s_exp(s_expression **subexps)
{
    int i = 0, count;
    while (subexps[i]->last_exp == false)
        ++i;
    count = i + 1;
    for (i = 0; i < count; ++i)
    {
        // Case of an incomplete expression due to error
        if (subexps[i] == NULL)
            break;
        // Normal case
        free(subexps[i]->node_list);
        free(subexps[i]);
    }
    free(subexps);
}
// Solve only the region between a and b and return the answer.
double solve_region(char *exp, int a, int b)
{
    char *temp = (char *)malloc(23 * (b - a + 2) * sizeof(char));
    double result;
    temp[b - a + 1] = '\0';
    strncpy(temp, exp + a, b - a + 1);
    result = scientific_interpreter(temp, false);
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
// Mode determines if the value to return is the beginning of the subexp (1) or the first number in it (2)
int subexp_start_at(s_expression **expression, int start, int current_s_exp, int mode)
{
    int i;
    i = current_s_exp - 1;
    switch (mode)
    {
    case 1:
        while (i >= 0)
        {
            if (expression[i]->expression_start == start)
                return i;
            else
                --i;
        }
        break;
    case 2:
        while (i >= 0)
        {
            if (expression[i]->solve_start == start)
                return i;
            else
                --i;
        }
    }
    return -1;
}
// Function that finds the subexpression that ends at 'end'
int s_exp_ending(s_expression **expression, int end, int current_s_exp, int s_exp_count)
{
    int i;
    i = current_s_exp - 1;
    while (i < s_exp_count)
    {
        if (expression[i]->end_index == end)
            return i;
        else
            --i;
    }
    return -1;
}